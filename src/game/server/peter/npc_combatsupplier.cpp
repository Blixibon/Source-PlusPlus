#include "cbase.h"
#include "npc_combatsupplier.h"
#include "ammodef.h"
#include "ai_squad.h"
#include "npcevent.h"

int ACT_CIT_HEAL;

// Animation events
int AE_CITIZEN_HEAL;


ConVar	sk_citizen_heal_player("sk_citizen_heal_player", "25");
ConVar	sk_citizen_heal_player_delay("sk_citizen_heal_player_delay", "25");
ConVar	sk_citizen_giveammo_player_delay("sk_citizen_giveammo_player_delay", "10");
ConVar	sk_citizen_heal_player_min_pct("sk_citizen_heal_player_min_pct", "0.60");
ConVar	sk_citizen_heal_player_min_forced("sk_citizen_heal_player_min_forced", "10.0");
ConVar	sk_citizen_heal_ally("sk_citizen_heal_ally", "30");
ConVar	sk_citizen_heal_ally_delay("sk_citizen_heal_ally_delay", "20");
ConVar	sk_citizen_heal_ally_min_pct("sk_citizen_heal_ally_min_pct", "0.90");
ConVar	sk_citizen_player_stare_time("sk_citizen_player_stare_time", "1.0");
ConVar  sk_citizen_player_stare_dist("sk_citizen_player_stare_dist", "72");
ConVar	sk_citizen_stare_heal_time("sk_citizen_stare_heal_time", "5");

ConVar  npc_citizen_medic_emit_sound("npc_citizen_medic_emit_sound", "1");
#ifdef HL2_EPISODIC
// todo: bake these into pound constants (for now they're not just for tuning purposes)
ConVar  npc_citizen_heal_chuck_medkit("npc_citizen_heal_chuck_medkit", "1", FCVAR_ARCHIVE, "Set to 1 to use new experimental healthkit-throwing medic.");
ConVar npc_citizen_medic_throw_style("npc_citizen_medic_throw_style", "1", FCVAR_ARCHIVE, "Set to 0 for a lobbier trajectory");
ConVar npc_citizen_medic_throw_speed("npc_citizen_medic_throw_speed", "650");
ConVar	sk_citizen_heal_toss_player_delay("sk_citizen_heal_toss_player_delay", "26", FCVAR_NONE, "how long between throwing healthkits");


#define MEDIC_THROW_SPEED npc_citizen_medic_throw_speed.GetFloat()
#define USE_EXPERIMENTAL_MEDIC_CODE() (npc_citizen_heal_chuck_medkit.GetBool() && NameMatches("griggs"))
#endif


const float HEAL_MOVE_RANGE = 30 * 12;
const float HEAL_TARGET_RANGE = 120; // 10 feet
#ifdef HL2_EPISODIC
const float HEAL_TOSS_TARGET_RANGE = 480; // 40 feet when we are throwing medkits 
const float HEAL_TARGET_RANGE_Z = 72; // a second check that Gordon isn't too far above us -- 6 feet
#endif

string_t CNPC_CombatSupplier::gm_iszCurrentAmmo = NULL_STRING;
string_t CNPC_CombatSupplier::gm_iszAutoAmmo = NULL_STRING;

BEGIN_DATADESC(CNPC_CombatSupplier)

DEFINE_FIELD(m_flPlayerHealTime, FIELD_TIME),
DEFINE_FIELD(m_flAllyHealTime, FIELD_TIME),
DEFINE_FIELD(m_flPlayerGiveAmmoTime, FIELD_TIME),
DEFINE_KEYFIELD(m_iszAmmoSupply, FIELD_STRING, "ammosupply"),
DEFINE_KEYFIELD(m_iAmmoAmount, FIELD_INTEGER, "ammoamount"),
DEFINE_FIELD(m_flTimePlayerStare, FIELD_TIME),
DEFINE_FIELD(m_flTimeNextHealStare, FIELD_TIME),
#if HL2_EPISODIC
DEFINE_INPUTFUNC(FIELD_VOID, "ThrowHealthKit", InputForceHealthKitToss),
#endif
END_DATADESC();

void CNPC_CombatSupplier::Precache()
{
	BaseClass::Precache();

	gm_iszCurrentAmmo = AllocPooledString("Current");
	gm_iszAutoAmmo = AllocPooledString("Auto");
}

void CNPC_CombatSupplier::Spawn()
{
	BaseClass::Spawn();

	m_flTimePlayerStare = FLT_MAX;
}

void CNPC_CombatSupplier::GatherHealConditions()
{
	// If the player is standing near a medic and can see the medic, 
	// assume the player is 'staring' and wants health.
	if (CanHeal())
	{
		CBasePlayer* pPlayer = GetBestPlayer();

		if (!pPlayer)
		{
			m_flTimePlayerStare = FLT_MAX;
			return;
		}

		float flDistSqr = (GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length2DSqr();
		float flStareDist = sk_citizen_player_stare_dist.GetFloat();
		float flPlayerDamage = pPlayer->GetMaxHealth() - pPlayer->GetHealth();

		if (pPlayer->IsAlive() && flPlayerDamage > 0 && (flDistSqr <= flStareDist * flStareDist) && pPlayer->FInViewCone(this) && pPlayer->FVisible(this))
		{
			if (m_flTimePlayerStare == FLT_MAX)
			{
				// Player wasn't looking at me at last think. He started staring now.
				m_flTimePlayerStare = gpGlobals->curtime;
			}

			// Heal if it's been long enough since last time I healed a staring player.
			if (gpGlobals->curtime - m_flTimePlayerStare >= sk_citizen_player_stare_time.GetFloat() && gpGlobals->curtime > m_flTimeNextHealStare && !IsCurSchedule(SCHED_SUPPLIER_HEAL))
			{
				if (ShouldHealTarget(pPlayer, true))
				{
					SetCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
				}
				else
				{
					m_flTimeNextHealStare = gpGlobals->curtime + sk_citizen_stare_heal_time.GetFloat() * .5f;
					ClearCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
				}
			}

#ifdef HL2_EPISODIC
			// Heal if I'm on an assault. The player hasn't had time to stare at me.
			if (hl2_episodic.GetBool() && m_AssaultBehavior.IsRunning() && IsMoving())
			{
				SetCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
			}
#endif
		}
		else
		{
			m_flTimePlayerStare = FLT_MAX;
		}
	}
}

void CNPC_CombatSupplier::GatherConditions()
{
	BaseClass::GatherConditions();

	GatherHealConditions();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CombatSupplier::CanHeal()
{
	if (!IsMedic() && !IsAmmoResupplier())
		return false;

	if (!hl2_episodic.GetBool())
	{
		// If I'm not armed, my priority should be to arm myself.
		if (IsMedic() && !GetActiveWeapon())
			return false;
	}

	if (IsInAScript() || (m_NPCState == NPC_STATE_SCRIPT))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_CombatSupplier::ShouldHealTarget(CBaseEntity* pTarget, bool bActiveUse)
{
	Disposition_t disposition;

	if (!pTarget && ((disposition = IRelationType(pTarget)) != D_LI && disposition != D_NU))
		return false;

	// Don't heal if I'm in the middle of talking
	if (IsSpeaking())
		return false;

	bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter* pCCTarget = pTarget->MyCombatCharacterPointer();
	if (pCCTarget != NULL && pCCTarget->IsInAVehicle())
		return false;

	if (IsMedic())
	{
		Vector toPlayer = (pTarget->GetAbsOrigin() - GetAbsOrigin());
		if ((bActiveUse || !HaveCommandGoal() || toPlayer.Length() < HEAL_TARGET_RANGE)
#ifdef HL2_EPISODIC
			&& fabs(toPlayer.z) < HEAL_TARGET_RANGE_Z
#endif
			)
		{
			if (pTarget->m_iHealth > 0)
			{
				if (bActiveUse)
				{
					// Ignore heal requests if we're going to heal a tiny amount
					float timeFullHeal = m_flPlayerHealTime;
					float timeRecharge = sk_citizen_heal_player_delay.GetFloat();
					float maximumHealAmount = sk_citizen_heal_player.GetFloat();
					float healAmt = (maximumHealAmount * (1.0 - (timeFullHeal - gpGlobals->curtime) / timeRecharge));
					if (healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth)
						healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
					if (healAmt < sk_citizen_heal_player_min_forced.GetFloat())
						return false;

					return (pTarget->m_iMaxHealth > pTarget->m_iHealth);
				}

				// Are we ready to heal again?
				bool bReadyToHeal = ((bTargetIsPlayer && m_flPlayerHealTime <= gpGlobals->curtime) ||
					(!bTargetIsPlayer && m_flAllyHealTime <= gpGlobals->curtime));

				// Only heal if we're ready
				if (bReadyToHeal)
				{
					int requiredHealth;

					if (bTargetIsPlayer)
						requiredHealth = pTarget->GetMaxHealth() - sk_citizen_heal_player.GetFloat();
					else
						requiredHealth = pTarget->GetMaxHealth() * sk_citizen_heal_player_min_pct.GetFloat();

					if ((pTarget->m_iHealth <= requiredHealth) && IRelationType(pTarget) == D_LI)
						return true;
				}
			}
		}
	}

	// Only players need ammo
	if (IsAmmoResupplier() && bTargetIsPlayer)
	{
		if (m_flPlayerGiveAmmoTime <= gpGlobals->curtime)
		{
			int iAmmoType = -1;
			if (m_iszAmmoSupply == gm_iszCurrentAmmo)
			{
				if (GetActiveWeapon())
					iAmmoType = GetActiveWeapon()->GetPrimaryAmmoType();
			}
			else if (m_iszAmmoSupply == gm_iszAutoAmmo)
			{
				CBaseCombatCharacter* pBCC = pTarget->MyCombatCharacterPointer();
				if (pBCC && pBCC->GetActiveWeapon())
					iAmmoType = pBCC->GetActiveWeapon()->GetPrimaryAmmoType();
			}
			else
			{
				iAmmoType = GetAmmoDef()->Index(STRING(m_iszAmmoSupply));
			}

			if (iAmmoType == -1)
			{
				DevMsg("ERROR: Citizen attempting to give unknown ammo type (%s)\n", STRING(m_iszAmmoSupply));
			}
			else
			{
				// Does the player need the ammo we can give him?
				int iMax = GetAmmoDef()->MaxCarry(iAmmoType);
				int iCount = ((CBasePlayer*)pTarget)->GetAmmoCount(iAmmoType);
				if (!iCount || ((iMax - iCount) >= m_iAmmoAmount))
				{
					// Only give the player ammo if he has a weapon that uses it
					if (((CBasePlayer*)pTarget)->Weapon_GetWpnForAmmo(iAmmoType))
						return true;
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombatSupplier::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SUPPLIER_HEAL:
#if HL2_EPISODIC
	case TASK_SUPPLIER_HEAL_TOSS:
#endif
		if (IsMedic())
		{
			if (GetTarget() && GetTarget()->IsPlayer() && GetTarget()->m_iMaxHealth == GetTarget()->m_iHealth)
			{
				// Doesn't need us anymore
				TaskComplete();
				break;
			}

			Speak(TLK_HEAL);
		}
		else if (IsAmmoResupplier())
		{
			Speak(TLK_GIVEAMMO);
		}
		SetIdealActivity((Activity)ACT_CIT_HEAL);
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombatSupplier::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MOVE_TO_TARGET_RANGE:
	{
		// If we're moving to heal a target, and the target dies, stop
		if (IsCurSchedule(SCHED_SUPPLIER_HEAL) && (!GetTarget() || !GetTarget()->IsAlive()))
		{
			TaskFail(FAIL_NO_TARGET);
			return;
		}

		BaseClass::RunTask(pTask);
		break;
	}

	case TASK_SUPPLIER_HEAL:
		if (IsSequenceFinished())
		{
			TaskComplete();
		}
		else if (!GetTarget())
		{
			// Our heal target was killed or deleted somehow.
			TaskFail(FAIL_NO_TARGET);
		}
		else
		{
			if ((GetTarget()->GetAbsOrigin() - GetAbsOrigin()).Length2D() > HEAL_MOVE_RANGE / 2)
				TaskComplete();

			GetMotor()->SetIdealYawToTargetAndUpdate(GetTarget()->GetAbsOrigin());
		}
		break;


#if HL2_EPISODIC
	case TASK_SUPPLIER_HEAL_TOSS:
		if (IsSequenceFinished())
		{
			TaskComplete();
		}
		else if (!GetTarget())
		{
			// Our heal target was killed or deleted somehow.
			TaskFail(FAIL_NO_TARGET);
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate(GetTarget()->GetAbsOrigin());
		}
		break;

#endif

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

void CNPC_CombatSupplier::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->event == AE_CITIZEN_HEAL)
	{
		// Heal my target (if within range)
#if HL2_EPISODIC
		if (USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic())
		{
			CBaseCombatCharacter* pTarget = dynamic_cast<CBaseCombatCharacter*>(GetTarget());
			Assert(pTarget);
			if (pTarget)
			{
				m_flPlayerHealTime = gpGlobals->curtime + sk_citizen_heal_toss_player_delay.GetFloat();;
				TossHealthKit(pTarget, Vector(48.0f, 0.0f, 0.0f));
			}
		}
		else
		{
			Heal();
		}
#else
		Heal();
#endif
		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

void CNPC_CombatSupplier::TaskFail(AI_TaskFailureCode_t code)
{
	// If our heal task has failed, push out the heal time
	if (IsCurSchedule(SCHED_SUPPLIER_HEAL))
	{
		m_flPlayerHealTime = gpGlobals->curtime + sk_citizen_heal_ally_delay.GetFloat();
	}

	BaseClass::TaskFail(code);
}

#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Determine if the citizen is in a position to be throwing medkits
//-----------------------------------------------------------------------------
bool CNPC_CombatSupplier::ShouldHealTossTarget(CBaseEntity* pTarget, bool bActiveUse)
{
	Disposition_t disposition;

	Assert(IsMedic());
	if (!IsMedic())
		return false;

	if (!pTarget && ((disposition = IRelationType(pTarget)) != D_LI && disposition != D_NU))
		return false;

	// Don't heal if I'm in the middle of talking
	if (IsSpeaking())
		return false;

	bool bTargetIsPlayer = pTarget->IsPlayer();

	// Don't heal or give ammo to targets in vehicles
	CBaseCombatCharacter* pCCTarget = pTarget->MyCombatCharacterPointer();
	if (pCCTarget != NULL && pCCTarget->IsInAVehicle())
		return false;

	Vector toPlayer = (pTarget->GetAbsOrigin() - GetAbsOrigin());
	if (bActiveUse || !HaveCommandGoal() || toPlayer.Length() < HEAL_TOSS_TARGET_RANGE)
	{
		if (pTarget->m_iHealth > 0)
		{
			if (bActiveUse)
			{
				// Ignore heal requests if we're going to heal a tiny amount
				float timeFullHeal = m_flPlayerHealTime;
				float timeRecharge = sk_citizen_heal_player_delay.GetFloat();
				float maximumHealAmount = sk_citizen_heal_player.GetFloat();
				float healAmt = (maximumHealAmount * (1.0 - (timeFullHeal - gpGlobals->curtime) / timeRecharge));
				if (healAmt > pTarget->m_iMaxHealth - pTarget->m_iHealth)
					healAmt = pTarget->m_iMaxHealth - pTarget->m_iHealth;
				if (healAmt < sk_citizen_heal_player_min_forced.GetFloat())
					return false;

				return (pTarget->m_iMaxHealth > pTarget->m_iHealth);
			}

			// Are we ready to heal again?
			bool bReadyToHeal = ((bTargetIsPlayer && m_flPlayerHealTime <= gpGlobals->curtime) ||
				(!bTargetIsPlayer && m_flAllyHealTime <= gpGlobals->curtime));

			// Only heal if we're ready
			if (bReadyToHeal)
			{
				int requiredHealth;

				if (bTargetIsPlayer)
					requiredHealth = pTarget->GetMaxHealth() - sk_citizen_heal_player.GetFloat();
				else
					requiredHealth = pTarget->GetMaxHealth() * sk_citizen_heal_player_min_pct.GetFloat();

				if ((pTarget->m_iHealth <= requiredHealth) && IRelationType(pTarget) == D_LI)
					return true;
			}
		}
	}

	return false;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombatSupplier::Heal()
{
	if (!CanHeal())
		return;

	CBaseEntity* pTarget = GetTarget();

	Vector target = pTarget->GetAbsOrigin() - GetAbsOrigin();
	if (target.Length() > HEAL_TARGET_RANGE * 2)
		return;

	// Don't heal a player that's staring at you until a few seconds have passed.
	m_flTimeNextHealStare = gpGlobals->curtime + sk_citizen_stare_heal_time.GetFloat();

	if (IsMedic())
	{
		float timeFullHeal;
		float timeRecharge;
		float maximumHealAmount;
		if (pTarget->IsPlayer())
		{
			timeFullHeal = m_flPlayerHealTime;
			timeRecharge = sk_citizen_heal_player_delay.GetFloat();
			maximumHealAmount = sk_citizen_heal_player.GetFloat();
			m_flPlayerHealTime = gpGlobals->curtime + timeRecharge;
		}
		else
		{
			timeFullHeal = m_flAllyHealTime;
			timeRecharge = sk_citizen_heal_ally_delay.GetFloat();
			maximumHealAmount = sk_citizen_heal_ally.GetFloat();
			m_flAllyHealTime = gpGlobals->curtime + timeRecharge;
		}

		float healAmt = (maximumHealAmount * (1.0 - (timeFullHeal - gpGlobals->curtime) / timeRecharge));

		if (healAmt > maximumHealAmount)
			healAmt = maximumHealAmount;
		else
			healAmt = RoundFloatToInt(healAmt);

		if (healAmt > 0)
		{
			if (pTarget->IsPlayer() && npc_citizen_medic_emit_sound.GetBool())
			{
				CPASAttenuationFilter filter(pTarget, "HealthKit.Touch");
				EmitSound(filter, pTarget->entindex(), "HealthKit.Touch");
			}

			pTarget->TakeHealth(healAmt, DMG_GENERIC);
			pTarget->RemoveAllDecals();
		}
	}

	if (IsAmmoResupplier())
	{
		// Non-players don't use ammo
		if (pTarget->IsPlayer())
		{
			int iAmmoType = -1;
			if (m_iszAmmoSupply == gm_iszCurrentAmmo)
			{
				if (GetActiveWeapon())
					iAmmoType = GetActiveWeapon()->GetPrimaryAmmoType();
			}
			else if (m_iszAmmoSupply == gm_iszAutoAmmo)
			{
				CBaseCombatCharacter* pBCC = pTarget->MyCombatCharacterPointer();
				if (pBCC && pBCC->GetActiveWeapon())
					iAmmoType = pBCC->GetActiveWeapon()->GetPrimaryAmmoType();
			}
			else
			{
				iAmmoType = GetAmmoDef()->Index(STRING(m_iszAmmoSupply));
			}

			if (iAmmoType == -1)
			{
				DevMsg("ERROR: Citizen attempting to give unknown ammo type (%s)\n", STRING(m_iszAmmoSupply));
			}
			else
			{
				((CBasePlayer*)pTarget)->GiveAmmo(m_iAmmoAmount, iAmmoType, false);
			}

			m_flPlayerGiveAmmoTime = gpGlobals->curtime + sk_citizen_giveammo_player_delay.GetFloat();
		}
	}
}

void CNPC_CombatSupplier::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if (IsMedic() && IsCustomInterruptConditionSet(COND_HEAR_MOVE_AWAY))
	{
		if (!IsCurSchedule(SCHED_RELOAD, false))
		{
			// Since schedule selection code prioritizes reloading over requests to heal
			// the player, we must prevent this condition from breaking the reload schedule.
			SetCustomInterruptCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
		}

		SetCustomInterruptCondition(COND_SUPPLIER_COMMANDHEAL);
	}

	if (hl2_episodic.GetBool())
	{
		if (IsMedic() && m_AssaultBehavior.IsRunning())
		{
			if (!IsCurSchedule(SCHED_RELOAD, false))
			{
				SetCustomInterruptCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
			}

			SetCustomInterruptCondition(COND_SUPPLIER_COMMANDHEAL);
		}
	}
	else
	{
		if (IsMedic() && m_AssaultBehavior.IsRunning() && !IsMoving())
		{
			if (!IsCurSchedule(SCHED_RELOAD, false))
			{
				SetCustomInterruptCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
			}

			SetCustomInterruptCondition(COND_SUPPLIER_COMMANDHEAL);
		}
	}
}

//-----------------------------------------------------------------------------
// Determine if citizen should perform heal action.
//-----------------------------------------------------------------------------
int CNPC_CombatSupplier::SelectScheduleHeal()
{
	// episodic medics may toss the healthkits rather than poke you with them
#if HL2_EPISODIC

	if (CanHeal())
	{
		CBaseEntity* pEntity = PlayerInRange(GetLocalOrigin(), HEAL_TOSS_TARGET_RANGE);
		if (pEntity)
		{
			if (USE_EXPERIMENTAL_MEDIC_CODE() && IsMedic())
			{
				// use the new heal toss algorithm
				if (ShouldHealTossTarget(pEntity, HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST)))
				{
					SetTarget(pEntity);
					return SCHED_SUPPLIER_HEAL_TOSS;
				}
			}
			else if (PlayerInRange(GetLocalOrigin(), HEAL_MOVE_RANGE))
			{
				// use old mechanism for ammo
				if (ShouldHealTarget(pEntity, HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST)))
				{
					SetTarget(pEntity);
					return SCHED_SUPPLIER_HEAL;
				}
			}

		}

		if (m_pSquad)
		{
			pEntity = NULL;
			float distClosestSq = HEAL_MOVE_RANGE * HEAL_MOVE_RANGE;
			float distCurSq;

			AISquadIter_t iter;
			CAI_BaseNPC* pSquadmate = m_pSquad->GetFirstMember(&iter);
			while (pSquadmate)
			{
				if (pSquadmate != this)
				{
					distCurSq = (GetAbsOrigin() - pSquadmate->GetAbsOrigin()).LengthSqr();
					if (distCurSq < distClosestSq && ShouldHealTarget(pSquadmate))
					{
						distClosestSq = distCurSq;
						pEntity = pSquadmate;
					}
				}

				pSquadmate = m_pSquad->GetNextMember(&iter);
			}

			if (pEntity)
			{
				SetTarget(pEntity);
				return SCHED_SUPPLIER_HEAL;
			}
		}
	}
	else
	{
		if (HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST))
			DevMsg("Would say: sorry, need to recharge\n");
	}

	return SCHED_NONE;

#else

	if (CanHeal())
	{
		CBaseEntity* pEntity = PlayerInRange(GetLocalOrigin(), HEAL_MOVE_RANGE);
		if (pEntity && ShouldHealTarget(pEntity, HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST)))
		{
			SetTarget(pEntity);
			return SCHED_SUPPLIER_HEAL;
		}

		if (m_pSquad)
		{
			pEntity = NULL;
			float distClosestSq = HEAL_MOVE_RANGE * HEAL_MOVE_RANGE;
			float distCurSq;

			AISquadIter_t iter;
			CAI_BaseNPC* pSquadmate = m_pSquad->GetFirstMember(&iter);
			while (pSquadmate)
			{
				if (pSquadmate != this)
				{
					distCurSq = (GetAbsOrigin() - pSquadmate->GetAbsOrigin()).LengthSqr();
					if (distCurSq < distClosestSq && ShouldHealTarget(pSquadmate))
					{
						distClosestSq = distCurSq;
						pEntity = pSquadmate;
					}
				}

				pSquadmate = m_pSquad->GetNextMember(&iter);
			}

			if (pEntity)
			{
				SetTarget(pEntity);
				return SCHED_SUPPLIER_HEAL;
			}
		}
	}
	else
	{
		if (HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST))
			DevMsg("Would say: sorry, need to recharge\n");
	}

	return SCHED_NONE;

#endif
}

int CNPC_CombatSupplier::SelectSchedulePriorityAction()
{
	int schedule = SelectScheduleHeal();
	if (schedule != SCHED_NONE)
		return schedule;

	schedule = BaseClass::SelectSchedulePriorityAction();
	if (schedule != SCHED_NONE)
		return schedule;

	return SCHED_NONE;
}

#if HL2_EPISODIC
//-----------------------------------------------------------------------------
// Like Heal(), but tosses a healthkit in front of the player rather than just juicing him up.
//-----------------------------------------------------------------------------
void	CNPC_CombatSupplier::TossHealthKit(CBaseCombatCharacter* pThrowAt, const Vector& offset)
{
	Assert(pThrowAt);

	Vector forward, right, up;
	GetVectors(&forward, &right, &up);
	Vector medKitOriginPoint = WorldSpaceCenter() + (forward * 20.0f);
	Vector destinationPoint;
	// this doesn't work without a moveparent: pThrowAt->ComputeAbsPosition( offset, &destinationPoint );
	VectorTransform(offset, pThrowAt->EntityToWorldTransform(), destinationPoint);
	// flatten out any z change due to player looking up/down
	destinationPoint.z = pThrowAt->EyePosition().z;

	Vector tossVelocity;

	if (npc_citizen_medic_throw_style.GetInt() == 0)
	{
		CTraceFilterSkipTwoEntities tracefilter(this, pThrowAt, COLLISION_GROUP_NONE);
		tossVelocity = VecCheckToss(this, &tracefilter, medKitOriginPoint, destinationPoint, 0.233f, 1.0f, false);
	}
	else
	{
		tossVelocity = VecCheckThrow(this, medKitOriginPoint, destinationPoint, MEDIC_THROW_SPEED, 1.0f);

		if (vec3_origin == tossVelocity)
		{
			// if out of range, just throw it as close as I can
			tossVelocity = destinationPoint - medKitOriginPoint;

			// rotate upwards against gravity
			float len = VectorLength(tossVelocity);
			tossVelocity *= (MEDIC_THROW_SPEED / len);
			tossVelocity.z += 0.57735026918962576450914878050196 * MEDIC_THROW_SPEED;
		}
	}

	// create a healthkit and toss it into the world
	CBaseEntity* pHealthKit = CreateEntityByName("item_healthkit");
	Assert(pHealthKit);
	if (pHealthKit)
	{
		pHealthKit->SetAbsOrigin(medKitOriginPoint);
		pHealthKit->SetOwnerEntity(this);
		// pHealthKit->SetAbsVelocity( tossVelocity );
		DispatchSpawn(pHealthKit);

		{
			IPhysicsObject* pPhysicsObject = pHealthKit->VPhysicsGetObject();
			Assert(pPhysicsObject);
			if (pPhysicsObject)
			{
				unsigned int cointoss = random->RandomInt(0, 0xFF); // int bits used for bools

				// some random precession
				Vector angDummy(random->RandomFloat(-200, 200), random->RandomFloat(-200, 200),
					cointoss & 0x01 ? random->RandomFloat(200, 600) : -1.0f * random->RandomFloat(200, 600));
				pPhysicsObject->SetVelocity(&tossVelocity, &angDummy);
			}
		}
	}
	else
	{
		Warning("Citizen tried to heal but could not spawn item_healthkit!\n");
	}

}

//-----------------------------------------------------------------------------
// cause an immediate call to TossHealthKit with some default numbers
//-----------------------------------------------------------------------------
void	CNPC_CombatSupplier::InputForceHealthKitToss(inputdata_t& inputdata)
{
	TossHealthKit(UTIL_GetLocalPlayer(), Vector(48.0f, 0.0f, 0.0f));
}

void CNPC_CombatSupplier::PredictPlayerPush()
{
	CBasePlayer* pPlayer = GetBestPlayer();
	if (!pPlayer)
		return;

	if (HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST))
		return;

	bool bHadPlayerPush = HasCondition(COND_PLAYER_PUSHING);

	BaseClass::PredictPlayerPush();

	if (!bHadPlayerPush && HasCondition(COND_PLAYER_PUSHING) &&
		pPlayer->FInViewCone(this) && CanHeal())
	{
		if (ShouldHealTarget(pPlayer, true))
		{
			ClearCondition(COND_PLAYER_PUSHING);
			SetCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
		}
	}
}

#endif

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_combatsupplier, CNPC_CombatSupplier)

DECLARE_TASK(TASK_SUPPLIER_HEAL)
#if HL2_EPISODIC
DECLARE_TASK(TASK_SUPPLIER_HEAL_TOSS)
#endif

DECLARE_ACTIVITY(ACT_CIT_HEAL)

DECLARE_CONDITION(COND_SUPPLIER_PLAYERHEALREQUEST)
DECLARE_CONDITION(COND_SUPPLIER_COMMANDHEAL)

DECLARE_ANIMEVENT(AE_CITIZEN_HEAL)

//=========================================================
	// > SCHED_SCI_HEAL
	//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SUPPLIER_HEAL,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET				0"
	"		TASK_MOVE_TO_TARGET_RANGE			50"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
	//		"		TASK_SAY_HEAL						0"
	//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
	"		TASK_SUPPLIER_HEAL							0"
	//		"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
	"	"
	"	Interrupts"
)

#if HL2_EPISODIC
//=========================================================
// > SCHED_CITIZEN_HEAL_TOSS
// this is for the episodic behavior where the citizen hurls the medkit
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SUPPLIER_HEAL_TOSS,

	"	Tasks"
	//  "		TASK_GET_PATH_TO_TARGET				0"
	//  "		TASK_MOVE_TO_TARGET_RANGE			50"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
	//	"		TASK_SAY_HEAL						0"
	//	"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_ARM"
	"		TASK_SUPPLIER_HEAL_TOSS							0"
	//	"		TASK_PLAY_SEQUENCE_FACE_TARGET		ACTIVITY:ACT_DISARM"
	"	"
	"	Interrupts"
)
#endif

AI_END_CUSTOM_NPC()