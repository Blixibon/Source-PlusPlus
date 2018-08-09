#include "cbase.h"
#include "ai_squad.h"
#include "ai_pathfinder.h"
#include "ai_route.h"
#include "npc_playerfollower.h"
#include "hl2_player.h"

extern ConVar	ai_citizen_debug_commander;
#define DebuggingCommanderMode() (ai_citizen_debug_commander.GetBool() && (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))

ConVar npc_player_squad_size("npc_player_squad_size", "4", FCVAR_CHEAT | FCVAR_REPLICATED);


extern ConVar player_squad_autosummon_time;
extern ConVar player_squad_autosummon_move_tolerance;
extern ConVar player_squad_autosummon_player_tolerance;
extern ConVar player_squad_autosummon_time_after_combat;
extern ConVar player_squad_autosummon_debug;

int __cdecl BMSSquadSortFunc(const BMSSquadMemberInfo_t *pLeft, const BMSSquadMemberInfo_t *pRight)
{
	if (pLeft->bSeesPlayer && !pRight->bSeesPlayer)
	{
		return -1;
	}

	if (!pLeft->bSeesPlayer && pRight->bSeesPlayer)
	{
		return 1;
	}

	return (pLeft->distSq - pRight->distSq);
}

BEGIN_DATADESC(CNPC_PlayerFollower)
DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_OUTPUT(m_OnJoinedPlayerSquad, "OnJoinedPlayerSquad"),
DEFINE_OUTPUT(m_OnLeftPlayerSquad, "OnLeftPlayerSquad"),
DEFINE_OUTPUT(m_OnFollowOrder, "OnFollowOrder"),
DEFINE_OUTPUT(m_OnStationOrder, "OnStationOrder"),
DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
//DEFINE_OUTPUT(m_OnNavFailBlocked, "OnNavFailBlocked"),
DEFINE_FIELD(m_iszOriginalSquad, FIELD_STRING),
DEFINE_FIELD(m_flTimeJoinedPlayerSquad, FIELD_TIME),
DEFINE_FIELD(m_bWasInPlayerSquad, FIELD_BOOLEAN),
DEFINE_FIELD(m_hSavedFollowGoalEnt, FIELD_EHANDLE),
//DEFINE_KEYFIELD(m_bNotifyNavFailBlocked, FIELD_BOOLEAN, "notifynavfailblocked"),
DEFINE_KEYFIELD(m_bNeverLeavePlayerSquad, FIELD_BOOLEAN, "neverleaveplayersquad"),
DEFINE_KEYFIELD(m_iszDenyCommandConcept, FIELD_STRING, "denycommandconcept"),
DEFINE_FIELD(m_flTimeLastCloseToPlayer, FIELD_TIME),
DEFINE_EMBEDDED(m_AutoSummonTimer),
DEFINE_FIELD(m_vAutoSummonAnchor, FIELD_POSITION_VECTOR),
DEFINE_USEFUNC(CommanderUse),
DEFINE_FIELD(m_pfnBaseUse, FIELD_FUNCTION),
END_DATADESC();

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CSimpleSimTimer CNPC_PlayerFollower::gm_PlayerSquadEvaluateTimer;

void CNPC_PlayerFollower::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	UpdatePlayerSquad();
	UpdateFollowCommandPoint();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::PostNPCInit()
{
	m_bAutoSquadPreventDoubleAdd = false;

	if (!gEntList.FindEntityByClassname(NULL, COMMAND_POINT_CLASSNAME))
	{
		CreateEntityByName(COMMAND_POINT_CLASSNAME);
	}

	if (IsInPlayerSquad())
	{
		FixupPlayerSquad();
	}
	

	BaseClass::PostNPCInit();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::OnRestore()
{
	gm_PlayerSquadEvaluateTimer.Force();

	BaseClass::OnRestore();

	if (!gEntList.FindEntityByClassname(NULL, COMMAND_POINT_CLASSNAME))
	{
		CreateEntityByName(COMMAND_POINT_CLASSNAME);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::IsCommandMoving()
{
	if (AI_IsSinglePlayer() && IsInPlayerSquad())
	{
		if (m_FollowBehavior.GetFollowTarget() == UTIL_GetLocalPlayer() ||
			IsFollowingCommandPoint())
		{
			return (m_FollowBehavior.IsMovingToFollowTarget());
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::ShouldAutoSummon()
{
	if (!AI_IsSinglePlayer() || !IsFollowingCommandPoint() || !IsInPlayerSquad())
		return false;

	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();

	float distMovedSq = (pPlayer->GetAbsOrigin() - m_vAutoSummonAnchor).LengthSqr();
	float moveTolerance = player_squad_autosummon_move_tolerance.GetFloat() * 12;
	const Vector &vCommandGoal = GetCommandGoal();

	if (distMovedSq < Square(moveTolerance * 10) && (GetAbsOrigin() - vCommandGoal).LengthSqr() > Square(10 * 12) && IsCommandMoving())
	{
		m_AutoSummonTimer.Set(player_squad_autosummon_time.GetFloat());
		if (player_squad_autosummon_debug.GetBool())
			DevMsg("Waiting for arrival before initiating autosummon logic\n");
	}
	else if (m_AutoSummonTimer.Expired())
	{
		bool bSetFollow = false;
		bool bTestEnemies = true;

		// Auto summon unconditionally if a significant amount of time has passed
		if (gpGlobals->curtime - m_AutoSummonTimer.GetNext() > player_squad_autosummon_time.GetFloat() * 2)
		{
			bSetFollow = true;
			if (player_squad_autosummon_debug.GetBool())
				DevMsg("Auto summoning squad: long time (%f)\n", (gpGlobals->curtime - m_AutoSummonTimer.GetNext()) + player_squad_autosummon_time.GetFloat());
		}

		// Player must move for autosummon
		if (distMovedSq > Square(12))
		{
			bool bCommandPointIsVisible = pPlayer->FVisible(vCommandGoal + pPlayer->GetViewOffset());

			// Auto summon if the player is close by the command point
			if (!bSetFollow && bCommandPointIsVisible && distMovedSq > Square(24))
			{
				float closenessTolerance = player_squad_autosummon_player_tolerance.GetFloat() * 12;
				if ((pPlayer->GetAbsOrigin() - vCommandGoal).LengthSqr() < Square(closenessTolerance) &&
					((m_vAutoSummonAnchor - vCommandGoal).LengthSqr() > Square(closenessTolerance)))
				{
					bSetFollow = true;
					if (player_squad_autosummon_debug.GetBool())
						DevMsg("Auto summoning squad: player close to command point (%f)\n", (GetAbsOrigin() - vCommandGoal).Length());
				}
			}

			// Auto summon if moved a moderate distance and can't see command point, or moved a great distance
			if (!bSetFollow)
			{
				if (distMovedSq > Square(moveTolerance * 2))
				{
					bSetFollow = true;
					bTestEnemies = (distMovedSq < Square(moveTolerance * 10));
					if (player_squad_autosummon_debug.GetBool())
						DevMsg("Auto summoning squad: player very far from anchor (%f)\n", sqrt(distMovedSq));
				}
				else if (distMovedSq > Square(moveTolerance))
				{
					if (!bCommandPointIsVisible)
					{
						bSetFollow = true;
						if (player_squad_autosummon_debug.GetBool())
							DevMsg("Auto summoning squad: player far from anchor (%f)\n", sqrt(distMovedSq));
					}
				}
			}
		}

		// Auto summon only if there are no readily apparent enemies
		if (bSetFollow && bTestEnemies)
		{
			for (int i = 0; i < g_AI_Manager.NumAIs(); i++)
			{
				CAI_BaseNPC *pNpc = g_AI_Manager.AccessAIs()[i];
				float timeSinceCombatTolerance = player_squad_autosummon_time_after_combat.GetFloat();

				if (pNpc->IsInPlayerSquad())
				{
					if (gpGlobals->curtime - pNpc->GetLastAttackTime() > timeSinceCombatTolerance ||
						gpGlobals->curtime - pNpc->GetLastDamageTime() > timeSinceCombatTolerance)
						continue;
				}
				else if (pNpc->GetEnemy())
				{
					CBaseEntity *pNpcEnemy = pNpc->GetEnemy();
					if (!IsSniper(pNpc) && (gpGlobals->curtime - pNpc->GetEnemyLastTimeSeen()) > timeSinceCombatTolerance)
						continue;

					if (pNpcEnemy == pPlayer)
					{
						if (pNpc->CanBeAnEnemyOf(pPlayer))
						{
							bSetFollow = false;
							break;
						}
					}
					else if (pNpcEnemy->IsNPC() && (pNpcEnemy->MyNPCPointer()->GetSquad() == GetSquad() || pNpcEnemy->Classify() == CLASS_PLAYER_ALLY_VITAL))
					{
						if (pNpc->CanBeAnEnemyOf(this))
						{
							bSetFollow = false;
							break;
						}
					}
				}
			}
			if (!bSetFollow && player_squad_autosummon_debug.GetBool())
				DevMsg("Auto summon REVOKED: Combat recent \n");
		}

		return bSetFollow;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: return TRUE if the commander mode should try to give this order
//			to more people. return FALSE otherwise. For instance, we don't
//			try to send all 3 selectedcitizens to pick up the same gun.
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::TargetOrder(CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies)
{
	if (pTarget->IsPlayer())
	{
		// I'm the target! Toggle follow!
		if (m_FollowBehavior.GetFollowTarget() != pTarget)
		{
			ClearFollowTarget();
			SetCommandGoal(vec3_invalid);

			// Turn follow on!
			m_AssaultBehavior.Disable();
			m_FollowBehavior.SetFollowTarget(pTarget);
			m_FollowBehavior.SetParameters(AIF_SIMPLE);
			if (ShouldAutosquad())
				SpeakCommandResponse(TLK_STARTFOLLOW);

			m_OnFollowOrder.FireOutput(this, this);
		}
		//else if (m_FollowBehavior.GetFollowTarget() == pTarget)
		//{
		//	// Stop following.
		//	m_FollowBehavior.SetFollowTarget(NULL);
		//	SpeakCommandResponse(TLK_STOPFOLLOW);
		//}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Turn off following before processing a move order.
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::MoveOrder(const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies)
{
	if (!AI_IsSinglePlayer())
		return;

	if (hl2_episodic.GetBool() && m_iszDenyCommandConcept != NULL_STRING)
	{
		SpeakCommandResponse(STRING(m_iszDenyCommandConcept));
		return;
	}

	CHL2_Player *pPlayer = (CHL2_Player *)UTIL_GetLocalPlayer();

	m_AutoSummonTimer.Set(player_squad_autosummon_time.GetFloat());
	m_vAutoSummonAnchor = pPlayer->GetAbsOrigin();

	if (m_StandoffBehavior.IsRunning())
	{
		m_StandoffBehavior.SetStandoffGoalPosition(vecDest);
	}

	// If in assault, cancel and move.
	if (m_AssaultBehavior.HasHitRallyPoint() && !m_AssaultBehavior.HasHitAssaultPoint())
	{
		m_AssaultBehavior.Disable();
		ClearSchedule("Moving from rally point to assault point");
	}

	bool spoke = false;

	CAI_BaseNPC *pClosest = NULL;
	float closestDistSq = FLT_MAX;

	for (int i = 0; i < numAllies; i++)
	{
		if (Allies[i]->IsInPlayerSquad())
		{
			Assert(Allies[i]->IsCommandable());
			float distSq = (pPlayer->GetAbsOrigin() - Allies[i]->GetAbsOrigin()).LengthSqr();
			if (distSq < closestDistSq)
			{
				pClosest = Allies[i];
				closestDistSq = distSq;
			}
		}
	}

	if (m_FollowBehavior.GetFollowTarget() && !IsFollowingCommandPoint())
	{
		ClearFollowTarget();
#if 0
		if ((pPlayer->GetAbsOrigin() - GetAbsOrigin()).LengthSqr() < Square(180) &&
			((vecDest - pPlayer->GetAbsOrigin()).LengthSqr() < Square(120) ||
			(vecDest - GetAbsOrigin()).LengthSqr() < Square(120)))
		{
			if (pClosest == this)
				SpeakIfAllowed(TLK_STOPFOLLOW);
			spoke = true;
		}
#endif
	}

	if (!spoke && pClosest == this)
	{
		float destDistToPlayer = (vecDest - pPlayer->GetAbsOrigin()).Length();
		float destDistToClosest = (vecDest - GetAbsOrigin()).Length();
		CFmtStr modifiers("commandpoint_dist_to_player:%.0f,"
			"commandpoint_dist_to_npc:%.0f",
			destDistToPlayer,
			destDistToClosest);

		SpeakCommandResponse(TLK_COMMANDED, modifiers);
	}

	m_OnStationOrder.FireOutput(this, this);

	BaseClass::MoveOrder(vecDest, Allies, numAllies);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::OnMoveOrder()
{
	SetReadinessLevel(AIRL_STIMULATED, false, false);
	BaseClass::OnMoveOrder();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::UpdateFollowCommandPoint()
{
	if (!AI_IsSinglePlayer())
		return;

	if (IsInPlayerSquad())
	{
		if (HaveCommandGoal())
		{
			CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
			CBaseEntity *pCommandPoint = gEntList.FindEntityByClassname(NULL, COMMAND_POINT_CLASSNAME);

			if (!pCommandPoint)
			{
				DevMsg("**\nVERY BAD THING\nCommand point vanished! Creating a new one\n**\n");
				pCommandPoint = CreateEntityByName(COMMAND_POINT_CLASSNAME);
			}

			if (pFollowTarget != pCommandPoint)
			{
				pFollowTarget = pCommandPoint;
				m_FollowBehavior.SetFollowTarget(pFollowTarget);
				m_FollowBehavior.SetParameters(AIF_COMMANDER);
			}

			if ((pCommandPoint->GetAbsOrigin() - GetCommandGoal()).LengthSqr() > 0.01)
			{
				UTIL_SetOrigin(pCommandPoint, GetCommandGoal(), false);
			}
		}
		else
		{
			if (IsFollowingCommandPoint())
				ClearFollowTarget();
			if (m_FollowBehavior.GetFollowTarget() != UTIL_GetLocalPlayer())
			{
				DevMsg("Expected to be following player, but not\n");
				m_FollowBehavior.SetFollowTarget(UTIL_GetLocalPlayer());
				m_FollowBehavior.SetParameters(AIF_SIMPLE);
			}
		}
	}
	else if (IsFollowingCommandPoint())
		ClearFollowTarget();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::ShouldAcceptGoal(CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal)
{
	if (BaseClass::ShouldAcceptGoal(pBehavior, pGoal))
	{
		CAI_FollowBehavior *pFollowBehavior = dynamic_cast<CAI_FollowBehavior *>(pBehavior);
		if (pFollowBehavior)
		{
			if (IsInPlayerSquad())
			{
				m_hSavedFollowGoalEnt = (CAI_FollowGoal *)pGoal;
				return false;
			}
		}
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::OnClearGoal(CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal)
{
	if (m_hSavedFollowGoalEnt == pGoal)
		m_hSavedFollowGoalEnt = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
struct FollowerSquadCandidate_t
{
	CNPC_PlayerFollower *pCitizen;
	bool		  bIsInSquad;
	float		  distSq;
	int			  iSquadIndex;
};

void CNPC_PlayerFollower::UpdatePlayerSquad()
{
	if (!AI_IsSinglePlayer())
		return;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ((pPlayer->GetAbsOrigin().AsVector2D() - GetAbsOrigin().AsVector2D()).LengthSqr() < Square(20 * 12))
		m_flTimeLastCloseToPlayer = gpGlobals->curtime;

	if (!gm_PlayerSquadEvaluateTimer.Expired())
		return;

	gm_PlayerSquadEvaluateTimer.Set(2.0);

	// Remove stragglers
	CAI_Squad *pPlayerSquad = g_AI_SquadManager.FindSquad(MAKE_STRING(PLAYER_SQUADNAME));
	if (pPlayerSquad)
	{
		CUtlVector<CNPC_PlayerFollower *> squadMembersToRemove;
		AISquadIter_t iter;

		for (CAI_BaseNPC *pPlayerSquadMember = pPlayerSquad->GetFirstMember(&iter); pPlayerSquadMember; pPlayerSquadMember = pPlayerSquad->GetNextMember(&iter))
		{
			CNPC_PlayerFollower *pCitizen = dynamic_cast<CNPC_PlayerFollower *>(pPlayerSquadMember);

			if (pCitizen == nullptr)
				continue;

			if ((!pCitizen->m_bNeverLeavePlayerSquad &&
				pCitizen->m_FollowBehavior.GetFollowTarget() &&
				!pCitizen->m_FollowBehavior.FollowTargetVisible() &&
				pCitizen->m_FollowBehavior.GetNumFailedFollowAttempts() > 0 &&
				gpGlobals->curtime - pCitizen->m_FollowBehavior.GetTimeFailFollowStarted() > 20 &&
				(fabsf((pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().z - pCitizen->GetAbsOrigin().z)) > 196 ||
				(pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D()).LengthSqr() > Square(50 * 12))) ||
				!pCitizen->IsPlayerAlly(pPlayer))
			{
				if (DebuggingCommanderMode())
				{
					DevMsg("Player follower is lost (%d, %f, %d)\n",
						pCitizen->m_FollowBehavior.GetNumFailedFollowAttempts(),
						gpGlobals->curtime - pCitizen->m_FollowBehavior.GetTimeFailFollowStarted(),
						(int)((pCitizen->m_FollowBehavior.GetFollowTarget()->GetAbsOrigin().AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D()).Length()));
				}

				squadMembersToRemove.AddToTail(pCitizen);
			}
		}

		for (int i = 0; i < squadMembersToRemove.Count(); i++)
		{
			squadMembersToRemove[i]->RemoveFromPlayerSquad();
		}
	}

	// Autosquadding
	const float JOIN_PLAYER_XY_TOLERANCE_SQ = Square(36 * 12);
	const float UNCONDITIONAL_JOIN_PLAYER_XY_TOLERANCE_SQ = Square(12 * 12);
	const float UNCONDITIONAL_JOIN_PLAYER_Z_TOLERANCE = 5 * 12;
	const float SECOND_TIER_JOIN_DIST_SQ = Square(48 * 12);
	if (pPlayer && ShouldAutosquad() && !(pPlayer->GetFlags() & FL_NOTARGET) && pPlayer->IsAlive())
	{
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		CUtlVector<FollowerSquadCandidate_t> candidates;
		const Vector &vPlayerPos = pPlayer->GetAbsOrigin();
		bool bFoundNewGuy = false;
		int i;

		for (i = 0; i < g_AI_Manager.NumAIs(); i++)
		{
			if (ppAIs[i]->GetState() == NPC_STATE_DEAD)
				continue;

			CNPC_PlayerFollower *pCitizen = dynamic_cast<CNPC_PlayerFollower *>(ppAIs[i]);

			if (pCitizen == nullptr)
				continue;

			if (!pCitizen->ShouldAutosquad())
				continue;
			
			int iNew;

			if (pCitizen->IsInPlayerSquad())
			{
				iNew = candidates.AddToTail();
				candidates[iNew].pCitizen = pCitizen;
				candidates[iNew].bIsInSquad = true;
				candidates[iNew].distSq = 0;
				candidates[iNew].iSquadIndex = pCitizen->GetSquad()->GetSquadIndex(pCitizen);
			}
			else
			{
				float distSq = (vPlayerPos.AsVector2D() - pCitizen->GetAbsOrigin().AsVector2D()).LengthSqr();
				if (distSq > JOIN_PLAYER_XY_TOLERANCE_SQ &&
					(pCitizen->m_flTimeJoinedPlayerSquad == 0 || gpGlobals->curtime - pCitizen->m_flTimeJoinedPlayerSquad > 60.0) &&
					(pCitizen->m_flTimeLastCloseToPlayer == 0 || gpGlobals->curtime - pCitizen->m_flTimeLastCloseToPlayer > 15.0))
					continue;

				if (!pCitizen->CanJoinPlayerSquad())
					continue;

				bool bShouldAdd = false;

				if (pCitizen->HasCondition(COND_SEE_PLAYER))
					bShouldAdd = true;
				else
				{
					bool bPlayerVisible = pCitizen->FVisible(pPlayer);
					if (bPlayerVisible)
					{
						if (pCitizen->HasCondition(COND_HEAR_PLAYER))
							bShouldAdd = true;
						else if (distSq < UNCONDITIONAL_JOIN_PLAYER_XY_TOLERANCE_SQ && fabsf(vPlayerPos.z - pCitizen->GetAbsOrigin().z) < UNCONDITIONAL_JOIN_PLAYER_Z_TOLERANCE)
							bShouldAdd = true;
					}
				}

				if (bShouldAdd)
				{
					// @TODO (toml 05-25-04): probably everyone in a squad should be a candidate if one of them sees the player
					AI_Waypoint_t *pPathToPlayer = pCitizen->GetPathfinder()->BuildRoute(pCitizen->GetAbsOrigin(), vPlayerPos, pPlayer, 5 * 12, NAV_NONE, true);
					GetPathfinder()->UnlockRouteNodes(pPathToPlayer);

					if (!pPathToPlayer)
						continue;

					CAI_Path tempPath;
					tempPath.SetWaypoints(pPathToPlayer); // path object will delete waypoints

					iNew = candidates.AddToTail();
					candidates[iNew].pCitizen = pCitizen;
					candidates[iNew].bIsInSquad = false;
					candidates[iNew].distSq = distSq;
					candidates[iNew].iSquadIndex = -1;

					bFoundNewGuy = true;
				}
			}
		}

		if (bFoundNewGuy)
		{
			// Look for second order guys
			int initialCount = candidates.Count();
			for (i = 0; i < initialCount; i++)
				candidates[i].pCitizen->m_bAutoSquadPreventDoubleAdd = true; // Prevents double-add
			for (i = 0; i < initialCount; i++)
			{
				if (candidates[i].iSquadIndex == -1)
				{
					for (int j = 0; j < g_AI_Manager.NumAIs(); j++)
					{
						if (ppAIs[j]->GetState() == NPC_STATE_DEAD)
							continue;

						CNPC_PlayerFollower *pCitizen = dynamic_cast<CNPC_PlayerFollower *>(ppAIs[i]);

						if (pCitizen == nullptr)
							continue;

						if (!pCitizen->ShouldAutosquad())
							continue;

						if (pCitizen->m_bAutoSquadPreventDoubleAdd)
							continue;

						float distSq = (vPlayerPos - pCitizen->GetAbsOrigin()).Length2DSqr();
						if (distSq > JOIN_PLAYER_XY_TOLERANCE_SQ)
							continue;

						distSq = (candidates[i].pCitizen->GetAbsOrigin() - pCitizen->GetAbsOrigin()).Length2DSqr();
						if (distSq > SECOND_TIER_JOIN_DIST_SQ)
							continue;

						if (!pCitizen->CanJoinPlayerSquad())
							continue;

						if (!pCitizen->FVisible(pPlayer))
							continue;

						int iNew = candidates.AddToTail();
						candidates[iNew].pCitizen = pCitizen;
						candidates[iNew].bIsInSquad = false;
						candidates[iNew].distSq = distSq;
						candidates[iNew].iSquadIndex = -1;
						pCitizen->m_bAutoSquadPreventDoubleAdd = true; // Prevents double-add
					}
				}
			}
			for (i = 0; i < candidates.Count(); i++)
				candidates[i].pCitizen->m_bAutoSquadPreventDoubleAdd = false;

			if (candidates.Count() > MAX_PLAYER_SQUAD)
			{
				candidates.Sort(PlayerSquadCandidateSortFunc);

				for (i = MAX_PLAYER_SQUAD; i < candidates.Count(); i++)
				{
					if (candidates[i].pCitizen->IsInPlayerSquad())
					{
						candidates[i].pCitizen->RemoveFromPlayerSquad();
					}
				}
			}

			if (candidates.Count())
			{
				CNPC_PlayerFollower *pClosest = NULL;
				float closestDistSq = FLT_MAX;
				int nJoined = 0;

				for (i = 0; i < candidates.Count() && i < MAX_PLAYER_SQUAD; i++)
				{
					if (!candidates[i].pCitizen->IsInPlayerSquad())
					{
						candidates[i].pCitizen->AddToPlayerSquad();
						nJoined++;

						if (candidates[i].distSq < closestDistSq)
						{
							pClosest = candidates[i].pCitizen;
							closestDistSq = candidates[i].distSq;
						}
					}
				}

				if (pClosest)
				{
					if (!pClosest->SpokeConcept(TLK_JOINPLAYER))
					{
						pClosest->SpeakCommandResponse(TLK_JOINPLAYER, CFmtStr("numjoining:%d", nJoined));
					}
					else
					{
						pClosest->SpeakCommandResponse(TLK_STARTFOLLOW);
					}

					for (i = 0; i < candidates.Count() && i < MAX_PLAYER_SQUAD; i++)
					{
						candidates[i].pCitizen->SetSpokeConcept(TLK_JOINPLAYER, NULL);
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_PlayerFollower::PlayerSquadCandidateSortFunc(const FollowerSquadCandidate_t *pLeft, const FollowerSquadCandidate_t *pRight)
{
	// "Bigger" means less approprate 
	CNPC_PlayerFollower *pLeftCitizen = pLeft->pCitizen;
	CNPC_PlayerFollower *pRightCitizen = pRight->pCitizen;

	// Non-autosquaders must stay
	if (!pLeftCitizen->ShouldAutosquad() && pRightCitizen->ShouldAutosquad())
		return -1;

	if (pLeftCitizen->ShouldAutosquad() && !pRightCitizen->ShouldAutosquad())
		return 1;

	// Medics are better than anyone
	if (pLeftCitizen->IsMedic() && !pRightCitizen->IsMedic())
		return -1;

	if (!pLeftCitizen->IsMedic() && pRightCitizen->IsMedic())
		return 1;

	CBaseCombatWeapon *pLeftWeapon = pLeftCitizen->GetActiveWeapon();
	CBaseCombatWeapon *pRightWeapon = pRightCitizen->GetActiveWeapon();

	// People with weapons are better than those without
	if (pLeftWeapon && !pRightWeapon)
		return -1;

	if (!pLeftWeapon && pRightWeapon)
		return 1;

	// Existing squad members are better than non-members
	if (pLeft->bIsInSquad && !pRight->bIsInSquad)
		return -1;

	if (!pLeft->bIsInSquad && pRight->bIsInSquad)
		return 1;

	// New squad members are better than older ones
	if (pLeft->bIsInSquad && pRight->bIsInSquad)
		return pRight->iSquadIndex - pLeft->iSquadIndex;

	// Finally, just take the closer
	return (int)(pRight->distSq - pLeft->distSq);
}

void CNPC_PlayerFollower::FixupPlayerSquad()
{
	if (!AI_IsSinglePlayer())
		return;

	m_flTimeJoinedPlayerSquad = gpGlobals->curtime;
	m_bWasInPlayerSquad = true;
	/*if (m_pSquad->NumMembers() > MAX_PLAYER_SQUAD)
	{
	CAI_BaseNPC *pFirstMember = m_pSquad->GetFirstMember(NULL);
	m_pSquad->RemoveFromSquad(pFirstMember);
	pFirstMember->ClearCommandGoal();

	CNPC_HumanGuard *pFirstMemberCitizen = dynamic_cast< CNPC_HumanGuard * >(pFirstMember);
	if (pFirstMemberCitizen)
	{
	pFirstMemberCitizen->ClearFollowTarget();
	}
	else
	{
	CAI_FollowBehavior *pOldMemberFollowBehavior;
	if (pFirstMember->GetBehavior(&pOldMemberFollowBehavior))
	{
	pOldMemberFollowBehavior->SetFollowTarget(NULL);
	}
	}
	}*/

	ClearFollowTarget();

	CAI_BaseNPC *pLeader = NULL;
	AISquadIter_t iter;
	for (CAI_BaseNPC *pAllyNpc = m_pSquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = m_pSquad->GetNextMember(&iter))
	{
		if (pAllyNpc->IsCommandable())
		{
			pLeader = pAllyNpc;
			break;
		}
	}

	if (pLeader && pLeader != this)
	{
		const Vector &commandGoal = pLeader->GetCommandGoal();
		if (commandGoal != vec3_invalid)
		{
			SetCommandGoal(commandGoal);
			SetCondition(COND_RECEIVED_ORDERS);
			OnMoveOrder();
		}
		else
		{
			CAI_FollowBehavior *pLeaderFollowBehavior;
			if (pLeader->GetBehavior(&pLeaderFollowBehavior))
			{
				m_FollowBehavior.SetFollowTarget(pLeaderFollowBehavior->GetFollowTarget());
				m_FollowBehavior.SetParameters(m_FollowBehavior.GetFormation());
			}

		}
	}
	else
	{
		m_FollowBehavior.SetFollowTarget(UTIL_GetLocalPlayer());
		m_FollowBehavior.SetParameters(AIF_SIMPLE);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::ClearFollowTarget()
{
	m_FollowBehavior.SetFollowTarget(NULL);
	m_FollowBehavior.SetParameters(AIF_SIMPLE);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::IsFollowingCommandPoint()
{
	CBaseEntity *pFollowTarget = m_FollowBehavior.GetFollowTarget();
	if (pFollowTarget)
		return FClassnameIs(pFollowTarget, COMMAND_POINT_CLASSNAME);
	return false;
}

CAI_BaseNPC *CNPC_PlayerFollower::GetSquadCommandRepresentative()
{
	if (!AI_IsSinglePlayer())
		return NULL;

	if (IsInPlayerSquad())
	{
		static float lastTime;
		static AIHANDLE hCurrent;

		if (gpGlobals->curtime - lastTime > 2.0 || !hCurrent || !hCurrent->IsInPlayerSquad()) // hCurrent will be NULL after level change
		{
			lastTime = gpGlobals->curtime;
			hCurrent = NULL;

			CUtlVectorFixed<BMSSquadMemberInfo_t, MAX_SQUAD_MEMBERS> candidates;
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

			if (pPlayer)
			{
				AISquadIter_t iter;
				for (CAI_BaseNPC *pAllyNpc = m_pSquad->GetFirstMember(&iter); pAllyNpc; pAllyNpc = m_pSquad->GetNextMember(&iter))
				{
					if (pAllyNpc->IsCommandable() && dynamic_cast<CAI_PlayerAlly *>(pAllyNpc))
					{
						int i = candidates.AddToTail();
						candidates[i].pMember = (CAI_PlayerAlly *)(pAllyNpc);
						candidates[i].bSeesPlayer = pAllyNpc->HasCondition(COND_SEE_PLAYER);
						candidates[i].distSq = (pAllyNpc->GetAbsOrigin() - pPlayer->GetAbsOrigin()).LengthSqr();
					}
				}

				if (candidates.Count() > 0)
				{
					candidates.Sort(BMSSquadSortFunc);
					hCurrent = candidates[0].pMember;
				}
			}
		}

		if (hCurrent != NULL)
		{
			Assert(dynamic_cast<CAI_PlayerAlly *>(hCurrent.Get()) && hCurrent->IsInPlayerSquad());
			return hCurrent;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::CanJoinPlayerSquad()
{
	if (!AI_IsSinglePlayer())
		return false;

	if (m_NPCState == NPC_STATE_SCRIPT || m_NPCState == NPC_STATE_PRONE)
		return false;

	if (IsInAScript())
		return false;

	// Don't bother people who don't want to be bothered
	if (!CanBeUsedAsAFriend())
		return false;

	if (IRelationType(UTIL_GetLocalPlayer()) != D_LI)
		return false;

	if (IsInPlayerSquad() && m_bNeverLeavePlayerSquad)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::ShouldAlwaysThink()
{
	return (BaseClass::ShouldAlwaysThink() || IsInPlayerSquad());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::AddToPlayerSquad()
{
	Assert(!IsInPlayerSquad());

	m_iszOriginalSquad = m_SquadName;

	AddToSquad(AllocPooledString(PLAYER_SQUADNAME));
	m_hSavedFollowGoalEnt = m_FollowBehavior.GetFollowGoal();
	m_FollowBehavior.SetFollowGoalDirect(NULL);
	m_FollowBehavior.SetFollowTarget(UTIL_GetLocalPlayer());

	FixupPlayerSquad();


	SetCondition(COND_PLAYER_ADDED_TO_SQUAD);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::RemoveFromPlayerSquad()
{
	Assert(IsInPlayerSquad());

	ClearFollowTarget();

	ClearCommandGoal();
	if (m_iszOriginalSquad != NULL_STRING && strcmp(STRING(m_iszOriginalSquad), PLAYER_SQUADNAME) != 0)
		AddToSquad(m_iszOriginalSquad);
	else
		RemoveFromSquad();

	if (m_hSavedFollowGoalEnt)
		m_FollowBehavior.SetFollowGoal(m_hSavedFollowGoalEnt);

	SetCondition(COND_PLAYER_REMOVED_FROM_SQUAD);



	// Don't evaluate the player squad for 2 seconds. 
	gm_PlayerSquadEvaluateTimer.Set(2.0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::TogglePlayerSquadState()
{
	if (!AI_IsSinglePlayer())
		return;

	if (!IsInPlayerSquad())
	{
		AddToPlayerSquad();

		if (HaveCommandGoal())
		{
			SpeakCommandResponse(TLK_COMMANDED);
		}
		else /*if (m_FollowBehavior.GetFollowTarget()->IsPlayer())*/
		{
			if (ShouldAutosquad())
			{
				SpeakCommandResponse(TLK_STARTFOLLOW);
			}
			else
			{
				bool bSemaphore = m_bDontUseSemaphore;
				m_bDontUseSemaphore = true;
				SpeakIfAllowed(TLK_STARTFOLLOW, NULL, true);
				m_bDontUseSemaphore = bSemaphore;
			}
		}
	}
	else
	{
		if (ShouldAutosquad())
		{
			SpeakCommandResponse(TLK_STOPFOLLOW);
		}
		else
		{
			bool bSemaphore = m_bDontUseSemaphore;
			m_bDontUseSemaphore = true;
			SpeakIfAllowed(TLK_STOPFOLLOW, NULL, true);
			m_bDontUseSemaphore = bSemaphore;
		}

		RemoveFromPlayerSquad();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::CommanderUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_OnPlayerUse.FireOutput(pActivator, pCaller);

	// Under these conditions, citizens will refuse to go with the player.
	// Robin: NPCs should always respond to +USE even if someone else has the semaphore.
	if (!AI_IsSinglePlayer() || !CanJoinPlayerSquad())
	{
		if (m_pfnBaseUse != nullptr)
			(this->*m_pfnBaseUse)(pActivator, pCaller, useType, value);
		
		return;
	}

	if (pActivator == UTIL_GetLocalPlayer() && !ShouldAutosquad())
	{
		// Don't say hi after you've been addressed by the player
		SetSpokeConcept(TLK_HELLO, NULL);

		TogglePlayerSquadState();

	}

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::SetSquad(CAI_Squad *pSquad)
{
	bool bWasInPlayerSquad = IsInPlayerSquad();

	BaseClass::SetSquad(pSquad);

	if (IsInPlayerSquad() && !bWasInPlayerSquad)
	{
		m_OnJoinedPlayerSquad.FireOutput(this, this);
	}
	else if (!IsInPlayerSquad() && bWasInPlayerSquad)
	{
		m_OnLeftPlayerSquad.FireOutput(this, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_PlayerFollower::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if (!IsCurSchedule(SCHED_NEW_WEAPON))
	{
		SetCustomInterruptCondition(COND_RECEIVED_ORDERS);
	}

	if (GetCurSchedule()->HasInterrupt(COND_IDLE_INTERRUPT))
	{
		SetCustomInterruptCondition(COND_BETTER_WEAPON_AVAILABLE);
	}
}

//-----------------------------------------------------------------------------
bool CNPC_PlayerFollower::SpeakCommandResponse(AIConcept_t concept, const char *modifiers)
{
	return SpeakIfAllowed(concept,
		CFmtStr("numselected:%d,"
			"useradio:%d%s",
			(GetSquad()) ? GetSquad()->NumMembers() : 1,
			0,
			(modifiers) ? CFmtStr(",%s", modifiers).operator const char *() : ""));
}