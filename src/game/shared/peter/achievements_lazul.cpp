#include "cbase.h"

#ifdef CLIENT_DLL
#include "achievementmgr.h"
#include "baseachievement.h"
#include "c_user_message_register.h"
#include "c_team.h"
#include "vprof.h"
#include "lazuul_gamerules.h"
#include "weapon_coop_base.h"
#include "ammodef.h"

#include "memdbgon.h"

void MsgFunc_AchievementMapEvent(bf_read &msg)
{
	char szEvent[64];
	msg.ReadString(szEvent, 64);
	CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>(engine->GetAchievementMgr());
	if (!pAchievementMgr)
		return;
	pAchievementMgr->OnMapEvent(szEvent);
}

class CLazAchievementMgr : public CAchievementMgr
{
	DECLARE_CLASS_GAMEROOT(CLazAchievementMgr, CAchievementMgr);
public:
	virtual bool Init();

protected:
	virtual void FireGameEvent(IGameEvent *event);
	virtual void OnKillEvent(C_BaseEntity *pVictim, C_BaseEntity *pAttacker, C_BaseEntity *pInflictor, IGameEvent *event);
};

CLazAchievementMgr g_AcheivementMgrLazul;

//-----------------------------------------------------------------------------
// Purpose: Counts the accumulated # of primary and secondary attacks from all
//			weapons (except grav gun).  If bBulletOnly is true, only counts
//			attacks with ammo that does bullet damage.
//-----------------------------------------------------------------------------
int CalcPlayerAttacks(bool bBulletOnly)
{
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	CAmmoDef *pAmmoDef = GetAmmoDef();
	if (!pPlayer || !pAmmoDef)
		return 0;

	int iTotalAttacks = 0;
	int iWeapons = pPlayer->WeaponCount();
	for (int i = 0; i < iWeapons; i++)
	{
		CWeaponCoopBase *pWeapon = dynamic_cast<CWeaponCoopBase *>(pPlayer->GetWeapon(i));
		if (pWeapon)
		{
			// add primary attacks if we were asked for all attacks, or only if it uses bullet ammo if we were asked to count bullet attacks
			if (!bBulletOnly || (pAmmoDef->m_AmmoType[pWeapon->GetPrimaryAmmoType()].nDamageType == DMG_BULLET))
			{
				iTotalAttacks += pWeapon->m_iPrimaryAttacks;
			}
			// add secondary attacks if we were asked for all attacks, or only if it uses bullet ammo if we were asked to count bullet attacks
			if (!bBulletOnly || (pAmmoDef->m_AmmoType[pWeapon->GetSecondaryAmmoType()].nDamageType == DMG_BULLET))
			{
				iTotalAttacks += pWeapon->m_iSecondaryAttacks;
			}
		}
	}
	return iTotalAttacks;
}

FORCEINLINE bool NamesMatch(const char *pszQuery, const char *pszNameToMatch)
{
	if (!pszNameToMatch || *pszNameToMatch == 0)
		return (!pszQuery || *pszQuery == 0 || *pszQuery == '*');

	// If the pointers are identical, we're identical
	if (pszNameToMatch == pszQuery)
		return true;

	while (*pszNameToMatch && *pszQuery)
	{
		unsigned char cName = *pszNameToMatch;
		unsigned char cQuery = *pszQuery;
		// simple ascii case conversion
		if (cName == cQuery)
			;
		else if (cName - 'A' <= (unsigned char)'Z' - 'A' && cName - 'A' + 'a' == cQuery)
			;
		else if (cName - 'a' <= (unsigned char)'z' - 'a' && cName - 'a' + 'A' == cQuery)
			;
		else
			break;
		++pszNameToMatch;
		++pszQuery;
	}

	if (*pszQuery == 0 && *pszNameToMatch == 0)
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if (*pszQuery == '*')
		return true;

	return false;
}

bool CLazAchievementMgr::Init()
{
	// We can be created on either client (for multiplayer games) or server
	// (for single player), so register ourselves with the engine so UI has a uniform place 
	// to go get the pointer to us

#ifdef _DEBUG
	// There can be only one achievement manager instance; no one else should be registered
	IAchievementMgr *pAchievementMgr = engine->GetAchievementMgr();
	Assert(NULL == pAchievementMgr);
#endif // _DEBUG

	// register ourselves
	engine->SetAchievementMgr(this);

	// register for events
	ListenForGameEvent("npc_death");
	ListenForGameEvent("game_init");
	ListenForGameEvent("player_death");
	ListenForGameEvent("player_stats_updated");
	usermessages->HookMessage("AchievementEvent", MsgFunc_AchievementEvent);
	usermessages->HookMessage("AchievementMapEvent", MsgFunc_AchievementMapEvent);

	ListenForGameEvent("localplayer_changeclass");
	ListenForGameEvent("localplayer_changeteam");
	ListenForGameEvent("teamplay_round_start");
	ListenForGameEvent("teamplay_round_win");

	return true;
}

void CLazAchievementMgr::OnKillEvent(C_BaseEntity * pVictim, C_BaseEntity * pAttacker, C_BaseEntity * pInflictor, IGameEvent * event)
{
	// can have a NULL victim on client if victim has never entered local player's PVS
	if (!pVictim)
		return;

	// if single-player game, calculate if the attacker is the local player and if the victim is the player enemy
	bool bAttackerIsPlayer = false;
	bool bVictimIsPlayerEnemy = false;
#ifdef GAME_DLL
	if (!g_pGameRules->IsMultiplayer())
	{
		CBasePlayer *pLocalPlayer = UTIL_GetLocalPlayer();
		if (pLocalPlayer)
		{
			if (pAttacker == pLocalPlayer)
			{
				bAttackerIsPlayer = true;
			}

			CBaseCombatCharacter *pBCC = dynamic_cast<CBaseCombatCharacter *>(pVictim);
			if (pBCC && (D_HT == pBCC->IRelationType(pLocalPlayer)))
			{
				bVictimIsPlayerEnemy = true;
			}
		}
	}
#else
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	bVictimIsPlayerEnemy = !pLocalPlayer->InSameTeam(pVictim);
	if (pAttacker == pLocalPlayer)
	{
		bAttackerIsPlayer = true;
	}
#endif // GAME_DLL

	// look through all the kill event listeners and notify any achievements whose filters we pass
	//FOR_EACH_VEC(m_vecKillEventListeners, iAchievement)
	for (int iAchievement = 0; iAchievement < (m_vecKillEventListeners).Count(); iAchievement++)
	{
		CBaseAchievement *pAchievement = m_vecKillEventListeners[iAchievement];

		if (!pAchievement->IsActive())
			continue;

#ifdef CLIENT_DLL
		// Swallow kill events that can't be earned right now
		if (!pAchievement->LocalPlayerCanEarn())
			continue;
#endif

		// if this achievement only looks for kills where attacker is player and that is not the case here, skip this achievement
		if ((pAchievement->GetFlags() & ACH_FILTER_ATTACKER_IS_PLAYER) && !bAttackerIsPlayer)
			continue;

		// if this achievement only looks for kills where victim is killer enemy and that is not the case here, skip this achievement
		if ((pAchievement->GetFlags() & ACH_FILTER_VICTIM_IS_PLAYER_ENEMY) && !bVictimIsPlayerEnemy)
			continue;

#if GAME_DLL
		// if this achievement only looks for a particular victim class name and this victim is a different class, skip this achievement
		const char *pVictimClassNameFilter = pAchievement->m_pVictimClassNameFilter;
		if (pVictimClassNameFilter && !pVictim->ClassMatches(pVictimClassNameFilter))
			continue;

		// if this achievement only looks for a particular inflictor class name and this inflictor is a different class, skip this achievement
		const char *pInflictorClassNameFilter = pAchievement->m_pInflictorClassNameFilter;
		if (pInflictorClassNameFilter && ((NULL == pInflictor) || !pInflictor->ClassMatches(pInflictorClassNameFilter)))
			continue;

		// if this achievement only looks for a particular attacker class name and this attacker is a different class, skip this achievement
		const char *pAttackerClassNameFilter = pAchievement->m_pAttackerClassNameFilter;
		if (pAttackerClassNameFilter && ((NULL == pAttacker) || !pAttacker->ClassMatches(pAttackerClassNameFilter)))
			continue;

		// if this achievement only looks for a particular inflictor entity name and this inflictor has a different name, skip this achievement
		const char *pInflictorEntityNameFilter = pAchievement->m_pInflictorEntityNameFilter;
		if (pInflictorEntityNameFilter && ((NULL == pInflictor) || !pInflictor->NameMatches(pInflictorEntityNameFilter)))
			continue;
#else
		// if this achievement only looks for a particular victim class name and this victim is a different class, skip this achievement
		const char *pVictimClassNameFilter = pAchievement->m_pVictimClassNameFilter;
		if (pVictimClassNameFilter && !NamesMatch(pVictimClassNameFilter, event->GetString("victim_name")))
			continue;

		// if this achievement only looks for a particular inflictor class name and this inflictor is a different class, skip this achievement
		const char *pInflictorClassNameFilter = pAchievement->m_pInflictorClassNameFilter;
		if (pInflictorClassNameFilter)
		{
			// Fix for HL2 pistol to prevent conflict with TF2 pistol. Must do this before stripping prefix.
			if (V_strcmp(pInflictorClassNameFilter, "weapon_pistol") == 0)
			{
				pInflictorClassNameFilter = "weapon_pistol_hl";
			}

			// strip certain prefixes from inflictor's classname
			const char* prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "weapon_", "npc_", "func_", "prop_vehicle_", "prop_", "monster_" };
			for (int i = 0; i < ARRAYSIZE(prefix); i++)
			{
				// if prefix matches, advance the string pointer past the prefix
				int len = V_strlen(prefix[i]);
				if (V_strncmp(pInflictorClassNameFilter, prefix[i], len) == 0)
				{
					pInflictorClassNameFilter += len;
					break;
				}
			}

			if (!NamesMatch(pInflictorClassNameFilter, event->GetString("weapon")))
				continue;
		}

		// if this achievement only looks for a particular attacker class name and this attacker is a different class, skip this achievement
		const char *pAttackerClassNameFilter = pAchievement->m_pAttackerClassNameFilter;
		if (pAttackerClassNameFilter && !NamesMatch(pAttackerClassNameFilter, event->GetString("attacker_name")))
			continue;
#endif // GAME_DLL

		// we pass all filters for this achievement, notify the achievement of the kill
		pAchievement->Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
	}
}

void CLazAchievementMgr::FireGameEvent(IGameEvent * event)
{
	VPROF_("CAchievementMgr::FireGameEvent", 1, VPROF_BUDGETGROUP_STEAM, false, 0);
	const char *name = event->GetName();
	if (name == NULL) { return; }
	if (0 == Q_strcmp(name, "npc_death"))
	{
		CBaseEntity *pVictim = ClientEntityList().GetEnt(event->GetInt("victim_index"));
		CBaseEntity *pAttacker = ClientEntityList().GetEnt(event->GetInt("attacker_index"));
		CBaseEntity *pInflictor = ClientEntityList().GetEnt(event->GetInt("weapon_index"));
		OnKillEvent(pVictim, pAttacker, pInflictor, event);
	}
	else if (0 == Q_strcmp(name, "game_init"))
	{
		// clear all state as though we were loading a saved game, but without loading the game
		PreRestoreSavedGame();
		PostRestoreSavedGame();
	}
#ifdef CLIENT_DLL
	else if (0 == Q_strcmp(name, "player_death"))
	{
		CBaseEntity *pVictim = ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid")));
		CBaseEntity *pAttacker = ClientEntityList().GetEnt(event->GetInt("attacker_index"));
		CBaseEntity *pInflictor = ClientEntityList().GetEnt(event->GetInt("weapon_index"));
		OnKillEvent(pVictim, pAttacker, pInflictor, event);
	}
	else if (0 == Q_strcmp(name, "localplayer_changeclass"))
	{
		// keep track of when the player last changed class
		m_flLastClassChangeTime = gpGlobals->curtime;
	}
	else if (0 == Q_strcmp(name, "localplayer_changeteam"))
	{
		// keep track of the time of transitions to and from a game team
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if (pLocalPlayer)
		{
			int iTeam = pLocalPlayer->GetTeamNumber();
			if (iTeam > TEAM_SPECTATOR)
			{
				if (0 == m_flTeamplayStartTime)
				{
					// player transitioned from no/spectator team to a game team, mark the time
					m_flTeamplayStartTime = gpGlobals->curtime;
				}
			}
			else
			{
				// player transitioned to no/spectator team, clear the teamplay start time
				m_flTeamplayStartTime = 0;
			}
		}
	}
	else if (0 == Q_strcmp(name, "teamplay_round_start"))
	{
		if (event->GetBool("full_reset"))
		{
			// we're starting a full round, clear miniround count
			m_iMiniroundsCompleted = 0;
		}
	}
	else if (0 == Q_strcmp(name, "teamplay_round_win"))
	{
		if (false == event->GetBool("full_round", true))
		{
			// we just finished a miniround but the round is continuing, increment miniround count
			m_iMiniroundsCompleted++;
		}
	}
	else if (0 == Q_strcmp(name, "player_stats_updated"))
	{
		//FOR_EACH_MAP(m_mapAchievement, i)
		for (int i = (m_mapAchievement).FirstInorder(); (m_mapAchievement).IsUtlMap && i != (m_mapAchievement).InvalidIndex(); i = (m_mapAchievement).NextInorder(i))
		{
			CBaseAchievement *pAchievement = m_mapAchievement[i];
			pAchievement->OnPlayerStatsUpdate();
		}
	}
#endif // CLIENT_DLL
}

class CRebelMapAchievement : public CMapAchievement
{
public:
	virtual bool LocalPlayerCanEarn(void)
	{
		return (GetLocalPlayerTeam() == TEAM_REBELS);
	}
};

#define DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_( achievementID, achievementName, gameDirFilter, iPointValue, bHidden ) \
class CAchievement##achievementID : public CRebelMapAchievement {};		\
DECLARE_ACHIEVEMENT_( CAchievement##achievementID, achievementID, achievementName, gameDirFilter, iPointValue, bHidden )	\

#define DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT( achievementID, achievementName, iPointValue )	\
	DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_( achievementID, achievementName, NULL, iPointValue, false )

#define DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN( achievementID, achievementName, iPointValue )	\
	DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_( achievementID, achievementName, NULL, iPointValue, true )

// helper class for achievements that check that the player was playing on a game team for the full round
class CTFAchievementFullRound : public CBaseAchievement
{
	DECLARE_CLASS(CTFAchievementFullRound, CBaseAchievement);
public:
	void Init()
	{
		m_iFlags |= ACH_FILTER_FULL_ROUND_ONLY;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_win");
	}

	void FireGameEvent_Internal(IGameEvent *event)
	{
		if (0 == Q_strcmp(event->GetName(), "teamplay_round_win"))
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if (pLocalPlayer)
			{
				// is the player currently on a game team?
				int iTeam = pLocalPlayer->GetTeamNumber();
				if (iTeam >= FIRST_GAME_TEAM)
				{
					float flRoundTime = event->GetFloat("round_time", 0);
					if (flRoundTime > 0)
					{
						Event_OnRoundComplete(flRoundTime, event);
					}
				}
			}
		}
	}

	virtual void Event_OnRoundComplete(float flRoundTime, IGameEvent *event) = 0;

};

#pragma region ORANGEBOX
class CAchievementTFGetHeadshots : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL);
		SetGoal(25);
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		// was this a headshot by this player?
		if ((pAttacker == C_BasePlayer::GetLocalPlayer()) && (event->GetInt("customkill") == TF_DMG_CUSTOM_HEADSHOT))
		{
			// Increment count.  Count will also get slammed whenever we get a stats update from server.  They should generally agree,
			// but server is authoritative.
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTFGetHeadshots, ACHIEVEMENT_TF_GET_HEADSHOTS, "TF_GET_HEADSHOTS", 5);


class CAchievementTFGetConsecutiveKillsNoDeaths : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL);
		SetGoal(1);
		m_iConsecutiveKills = 0;
	}

	virtual bool IsActive()
	{
		if (!gpGlobals->maxClients <= 1)
			return false;

		return CBaseAchievement::IsActive();
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if (pLocalPlayer == pVictim)
		{
			m_iConsecutiveKills = 0;
		}
		else if (pLocalPlayer == pAttacker)
		{
			m_iConsecutiveKills++;
			if (5 == m_iConsecutiveKills)
			{
				IncrementCount();
			}
		}
	}
	int m_iConsecutiveKills;
};
DECLARE_ACHIEVEMENT(CAchievementTFGetConsecutiveKillsNoDeaths, ACHIEVEMENT_TF_GET_CONSECUTIVEKILLS_NODEATHS, "TF_GET_CONSECUTIVEKILLS_NODEATHS", 10);

class CAchievementTFGetMultipleKills : public CBaseAchievement
{
	void Init()
	{
		// listen for player kill enemy events, base class will increment count each time that happens
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL);
		SetGoal(1000);
	}
};
DECLARE_ACHIEVEMENT(CAchievementTFGetMultipleKills, ACHIEVEMENT_TF_GET_MULTIPLEKILLS, "TF_GET_MULTIPLEKILLS", 15);


class CAchievementPortalDetachAllCameras : public CBaseAchievement
{
protected:
	virtual void ListenForEvents()
	{
		ListenForGameEvent("security_camera_detached");
	}

	void FireGameEvent_Internal(IGameEvent *event)
	{
		if (0 == Q_strcmp(event->GetName(), "security_camera_detached"))
		{
			IncrementCount();
		}
	}
public:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_WITH_GAME);
		SetGoal(33);
		ListenForGameEvent("security_camera_detached");
	}
};
DECLARE_ACHIEVEMENT(CAchievementPortalDetachAllCameras, ACHIEVEMENT_PORTAL_DETACH_ALL_CAMERAS, "PORTAL_DETACH_ALL_CAMERAS", 5);

class CAchievementPortalHitTurretWithTurret : public CBaseAchievement
{
protected:
	virtual void ListenForEvents()
	{
		ListenForGameEvent("turret_hit_turret");
	}

	void FireGameEvent_Internal(IGameEvent *event)
	{
		if (0 == Q_strcmp(event->GetName(), "turret_hit_turret"))
		{
			IncrementCount();
		}
	}
public:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		ListenForGameEvent("turret_hit_turret");
	}
};
DECLARE_ACHIEVEMENT(CAchievementPortalHitTurretWithTurret, ACHIEVEMENT_PORTAL_HIT_TURRET_WITH_TURRET, "PORTAL_HIT_TURRET_WITH_TURRET", 5);

class CAchievementEp1BeatCitizenEscortNoCitizenDeaths : public CFailableAchievement
{
protected:

	void Init()
	{
		SetFlags(ACH_LISTEN_MAP_EVENTS | ACH_LISTEN_KILL_EVENTS | ACH_SAVE_WITH_GAME);
		SetGoal(1);
	}

	virtual bool LocalPlayerCanEarn(void)
	{
		return (GetLocalPlayerTeam() == TEAM_REBELS);
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		if (IsActive() && pVictim && pVictim->InLocalTeam())
			SetFailed();
	}

	// map event where achievement is activated
	virtual const char *GetActivationEventName() { return "EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS_START"; }
	// map event where achievement is evaluated for success
	virtual const char *GetEvaluationEventName() { return "EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS_END"; }
};
DECLARE_ACHIEVEMENT(CAchievementEp1BeatCitizenEscortNoCitizenDeaths, ACHIEVEMENT_EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS, "EP1_BEAT_CITIZENESCORT_NOCITIZENDEATHS", 15);

class CAchievementEp1KillAntlionsWithCar : public CBaseAchievement
{
protected:

	void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("physics");
		SetVictimFilter("npc_antlion");
		SetGoal(15);
	}
	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		// any model that passed previous filters and begins with "props_vehicles" is a physics car
		const char *pszName = GetModelName(pInflictor);
		const char szPrefix[] = "props_vehicles";
		if (0 == Q_strncmp(pszName, szPrefix, ARRAYSIZE(szPrefix) - 1))
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementEp1KillAntlionsWithCar, ACHIEVEMENT_EP1_KILL_ANTLIONS_WITHCARS, "EP1_KILL_ANTLIONS_WITHCARS", 5);

class CAchievementHLXKillWithPhysicsObjects : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("physics");
		SetGoal(30);
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		int iDamageBits = event->GetInt("damagebits");
		// was victim killed with crushing damage?
		if (iDamageBits & DMG_CRUSH)
		{
			IncrementCount();
		}
	}

};
DECLARE_ACHIEVEMENT(CAchievementHLXKillWithPhysicsObjects, ACHIEVEMENT_HLX_KILL_ENEMIES_WITHPHYSICS, "HLX_KILL_ENEMIES_WITHPHYSICS", 5);

class CAchievementHLXKillWithManhack : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("npc_manhack");
		SetGoal(5);
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		// We've already filtered to only get called when a player enemy gets killed with a manhack.  Now just check for the
		// case of player smashing manhack into something, in which case the manhack is both the victim and inflictor.
		// If that's not the case, this is a player kill w/manhack.
		if (pVictim != pInflictor)
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementHLXKillWithManhack, ACHIEVEMENT_HLX_KILL_ENEMIES_WITHMANHACK, "HLX_KILL_ENEMIES_WITHMANHACK", 5);

class CAchievementHLXKillWithOneEnergyBall : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("prop_combine_ball");
		SetGoal(1);
		m_pLastInflictor = NULL;
		m_iLocalCount = 0;
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		// to count # of kills with same energy ball, keep track of previous inflictor
		if (m_pLastInflictor != NULL && pInflictor != m_pLastInflictor)
		{
			// new inflictor, start the count over at 1
			m_iLocalCount = 1;
		}
		else
		{
			// same inflictor, keep counting
			m_iLocalCount++;
			if (5 == m_iLocalCount)
			{
				IncrementCount();
			}
		}
		// keep track of last inflictor
		m_pLastInflictor = pInflictor;
	}
	CBaseEntity *m_pLastInflictor;
	int m_iLocalCount;
};
DECLARE_ACHIEVEMENT(CAchievementHLXKillWithOneEnergyBall, ACHIEVEMENT_HLX_KILL_ENEMIES_WITHONEENERGYBALL, "HLX_KILL_ENEMIES_WITHONEENERGYBALL", 5);

class CAchievementHL2BeatRavenholmNoWeapons : public CFailableAchievement
{
	DECLARE_CLASS(CAchievementHL2BeatRavenholmNoWeapons, CFailableAchievement);

	void Init()
	{
		SetFlags(ACH_LISTEN_MAP_EVENTS | ACH_SAVE_WITH_GAME);
		SetGoal(1);
		m_iInitialAttackCount = 0;
	}

	// map event where achievement is activated
	virtual const char *GetActivationEventName() { return "HL2_BEAT_RAVENHOLM_NOWEAPONS_START"; }
	// map event where achievement is evaluated for success
	virtual const char *GetEvaluationEventName() { return "HL2_BEAT_RAVENHOLM_NOWEAPONS_END"; }

	virtual void PreRestoreSavedGame()
	{
		m_iInitialAttackCount = 0;
		BaseClass::PreRestoreSavedGame();
	}

	virtual void OnActivationEvent()
	{
		// get current # of attacks by player w/all weapons (except grav gun) and store that
		m_iInitialAttackCount = CalcPlayerAttacks(false);
		BaseClass::OnActivationEvent();
	}

	virtual void OnEvaluationEvent()
	{
		// get current # of attacks by player w/all weapons (except grav gun) 
		int iCurAttackCount = CalcPlayerAttacks(false);
		// compare to # of attacks when we started
		if (iCurAttackCount > m_iInitialAttackCount)
		{
			// if there have been any more weapon attacks, achievement fails
			SetFailed();
		}
		BaseClass::OnEvaluationEvent();
	}

	// additional status for debugging
	virtual void PrintAdditionalStatus()
	{
		if (m_bActivated)
		{
			Msg("Starting wpn attacks: %d  Current wpn attacks: %d\n", m_iInitialAttackCount, CalcPlayerAttacks(false));
		}
	}

	int m_iInitialAttackCount;
public:
	DECLARE_DATADESC()
};
DECLARE_ACHIEVEMENT(CAchievementHL2BeatRavenholmNoWeapons, ACHIEVEMENT_HL2_BEAT_RAVENHOLM_NOWEAPONS, "HL2_BEAT_RAVENHOLM_NOWEAPONS", 25);

BEGIN_DATADESC(CAchievementHL2BeatRavenholmNoWeapons)
DEFINE_FIELD(m_iInitialAttackCount, FIELD_INTEGER),
END_DATADESC()


class CAchievementHL2KillGunships : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetVictimFilter("npc_combinegunship");
		SetGoal(6);	// note: goal is really six, although #define is "THREEGUNSHIPS"
	}
};
DECLARE_ACHIEVEMENT(CAchievementHL2KillGunships, ACHIEVEMENT_HL2_KILL_THREEGUNSHIPS, "HL2_KILL_THREEGUNSHIPS", 5);

class CAchievementHL2KillEnemiesWithAntlions : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("npc_antlion");
		SetGoal(50);
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if (pPlayer)
		{
			// Only count antlion kills once player owns bugbait. 
			if (pPlayer->Weapon_OwnsThisType("weapon_bugbait"))
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementHL2KillEnemiesWithAntlions, ACHIEVEMENT_HL2_KILL_ENEMIES_WITHANTLIONS, "HL2_KILL_ENEMIES_WITHANTLIONS", 10);


class CAchievementHL2KillEnemyWithToilet : public CBaseAchievement
{
	void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("prop_physics");
		SetGoal(1);
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		const char *pszName = GetModelName(pInflictor);

		// skip past any directories and get just the file name
		pszName = V_UnqualifiedFileName(pszName);
		// if model name matches one of the toilets, this counts
		if ((0 == Q_stricmp(pszName, "FurnitureToilet001a.mdl")) || (0 == Q_stricmp(pszName, "prison_toilet01.mdl")))
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementHL2KillEnemyWithToilet, ACHIEVEMENT_HL2_KILL_ENEMY_WITHTOILET, "HL2_KILL_ENEMY_WITHTOILET", 5);

class CAchievementHL2FindAllLambdas : public CBaseAchievement
{
	virtual void Init()
	{
		static const char *szComponents[] =
		{
			"HL2_LAMDACACHE_KLEINERSLAB", "HL2_LAMDACACHE_CANALSSTATION", "HL2_LAMDACACHE_VENTCRAWL", "HL2_LAMDACACHE_CANALSTUNNEL",
			"HL2_LAMDACACHE_SEWERGRATE", "HL2_LAMDACACHE_STEAMPIPE", "HL2_LAMDACACHE_CURVEDROOM", "HL2_LAMDACACHE_SHANTYTOWN",
			"HL2_LAMDACACHE_TUNNELLADDER", "HL2_LAMDACACHE_REDBARN", "HL2_LAMDACACHE_ZOMBIEAMBUSH", "HL2_LAMDACACHE_BELOWAPCS",
			"HL2_LAMDACACHE_COUNTERWEIGHT", "HL2_LAMDACACHE_RAILWAYBRIDGE", "HL2_LAMDACACHE_TUNNELPLATFORMS", "HL2_LAMDACACHE_BANKEDCANAL",
			"HL2_LAMDACACHE_CANALWALL", "HL2_LAMDACACHE_CHANNELSPLIT", "HL2_LAMDACACHE_BMEDOCK", "HL2_LAMDACACHE_GENERATORS",
			"HL2_LAMDACACHE_CARCRUSHERARENA", "HL2_LAMDACACHE_RAVENHOLMATTIC", "HL2_LAMDACACHE_MINETUNNELEXIT",
			"HL2_LAMDACACHE_COASTSHACK", "HL2_LAMDACACHE_POISONSHACK", "HL2_LAMDACACHE_GUNSHIPVAN", "HL2_LAMDACACHE_SUICIDECITIZEN",
			"HL2_LAMDACACHE_RAILROADSHACK", "HL2_LAMDACACHE_COASTABOVEBATTERY", "HL2_LAMDACACHE_SANDSHACK", "HL2_LAMDACACHE_GMANCACHE",
			"HL2_LAMDACACHE_CELLCACHE", "HL2_LAMDACACHE_POISONLAUNDRY", "HL2_LAMDACACHE_SODAMACHINE",
			"HL2_LAMDACACHE_STREETWARDOGWALL", "HL2_LAMDACACHE_STREETWARSHACK", "HL2_LAMDACACHE_STREETWARFENCE", "HL2_LAMDACACHE_FREEWAYTUNNEL", "HL2_LAMDACACHE_DRAWBRIDGE",
			"HL2_LAMDACACHE_PLAZAFENCE", "HL2_LAMDACACHE_SEWERSCATWALKS", "HL2_LAMDACACHE_POISONZOMBIEALCOVE", "HL2_LAMDACACHE_PIPEHOPTUNNEL",
			"HL2_LAMDACACHE_ENDOFC1712B", "HL2_LAMDACACHE_EXITCATWALK"
		};
		SetFlags(ACH_HAS_COMPONENTS | ACH_LISTEN_COMPONENT_EVENTS | ACH_SAVE_GLOBAL);
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE(szComponents);
		SetComponentPrefix("HL2_LAMDACACHE");
		SetGoal(m_iNumComponents);
	}
};
DECLARE_ACHIEVEMENT(CAchievementHL2FindAllLambdas, ACHIEVEMENT_HL2_FIND_ALLLAMBDAS, "HL2_FIND_ALLLAMBDAS", 15);

class CAchievementEp2KillPoisonAntlion : public CBaseAchievement
{
protected:

	virtual void Init()
	{
		SetVictimFilter("npc_antlionworker");
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetGoal(1);
	}
};
DECLARE_ACHIEVEMENT(CAchievementEp2KillPoisonAntlion, ACHIEVEMENT_EP2_KILL_POISONANTLION, "EP2_KILL_POISONANTLION", 5);

class CAchievementEp2KillEnemiesWithCar : public CBaseAchievement
{
protected:

	virtual void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_WITH_GAME);
		SetInflictorFilter("prop_vehicle_j*");
		SetGoal(20);
	}
};
DECLARE_ACHIEVEMENT(CAchievementEp2KillEnemiesWithCar, ACHIEVEMENT_EP2_KILL_ENEMIES_WITHCAR, "EP2_KILL_ENEMIES_WITHCAR", 5);

class CAchievementEp2FindAllWebCaches : public CBaseAchievement
{
	virtual void Init()
	{
		static const char *szComponents[] =
		{
			"EP2_WEBCACHE_01", "EP2_WEBCACHE_02", "EP2_WEBCACHE_03", "EP2_WEBCACHE_04",
			"EP2_WEBCACHE_05", "EP2_WEBCACHE_06", "EP2_WEBCACHE_07", "EP2_WEBCACHE_08", "EP2_WEBCACHE_09"
		};
		SetFlags(ACH_HAS_COMPONENTS | ACH_LISTEN_COMPONENT_EVENTS | ACH_SAVE_GLOBAL);
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE(szComponents);
		SetComponentPrefix("EP2_WEBCACHE");
		SetGoal(m_iNumComponents);
	}

	// don't show progress notifications for this achievement, it's distracting
	virtual bool ShouldShowProgressNotification() { return false; }
};
DECLARE_ACHIEVEMENT(CAchievementEp2FindAllWebCaches, ACHIEVEMENT_EP2_BREAK_ALLWEBS, "EP2_BREAK_ALLWEBS", 5);

class CAchievementEp2FindAllRadarCaches : public CBaseAchievement
{
	virtual void Init()
	{
		static const char *szComponents[] =
		{
			"EP2_RADARCACHE_VAN", "EP2_RADARCACHE_SHACK", "EP2_RADARCACHE_RPG", "EP2_RADARCACHE_CAVE", "EP2_RADARCACHE_HANGING"
		};
		SetFlags(ACH_HAS_COMPONENTS | ACH_LISTEN_COMPONENT_EVENTS | ACH_SAVE_GLOBAL);
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE(szComponents);
		SetComponentPrefix("EP2_RADARCACHE");
		SetGoal(m_iNumComponents);
	}
};
DECLARE_ACHIEVEMENT(CAchievementEp2FindAllRadarCaches, ACHIEVEMENT_EP2_FIND_ALLRADARCACHES, "EP2_FIND_ALLRADARCACHES", 10);


// achievements which are won by a map event firing once
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP2_BEAT_ANTLIONINVASION, "EP2_BEAT_ANTLIONINVASION", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_EP2_BEAT_ANTLIONGUARDS, "EP2_BEAT_ANTLIONGUARDS", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_EP2_BEAT_HUNTERAMBUSH, "EP2_BEAT_HUNTERAMBUSH", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_EP2_KILL_COMBINECANNON, "EP2_KILL_COMBINECANNON", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP2_BEAT_RACEWITHDOG, "EP2_BEAT_RACEWITHDOG", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP2_BEAT_ROCKETCACHEPUZZLE, "EP2_BEAT_ROCKETCACHEPUZZLE", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_EP2_BEAT_WHITEFORESTINN, "EP2_BEAT_WHITEFORESTINN", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP2_PUT_ITEMINROCKET, "EP2_PUT_ITEMINROCKET", 30);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_EP2_BEAT_MISSILESILO2, "EP2_BEAT_MISSILESILO2", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP2_BEAT_OUTLAND12_NOBUILDINGSDESTROYED, "EP2_BEAT_OUTLAND12_NOBUILDINGSDESTROYED", 35);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_EP2_BEAT_GAME, "EP2_BEAT_GAME", 20);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP1_BEAT_MAINELEVATOR, "EP1_BEAT_MAINELEVATOR", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP1_BEAT_CITADELCORE, "EP1_BEAT_CITADELCORE", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP1_BEAT_CITADELCORE_NOSTALKERKILLS, "EP1_BEAT_CITADELCORE_NOSTALKERKILLS", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP1_BEAT_GARAGEELEVATORSTANDOFF, "EP1_BEAT_GARAGEELEVATORSTANDOFF", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP1_BEAT_HOSPITALATTICGUNSHIP, "EP1_BEAT_HOSPITALATTICGUNSHIP", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_EP1_BEAT_GAME, "EP1_BEAT_GAME", 20);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_HL2_HIT_CANCOP_WITHCAN, "HL2_HIT_CANCOP_WITHCAN", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_HL2_PUT_CANINTRASH, "HL2_PUT_CANINTRASH", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_ESCAPE_APARTMENTRAID, "HL2_ESCAPE_APARTMENTRAID", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BREAK_MINITELEPORTER, "HL2_BREAK_MINITELEPORTER", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_GET_CROWBAR, "HL2_GET_CROWBAR", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_GET_AIRBOAT, "HL2_GET_AIRBOAT", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_GET_AIRBOATGUN, "HL2_GET_AIRBOATGUN", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_FIND_VORTIGAUNTCAVE, "HL2_FIND_VORTIGAUNTCAVE", 13);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_KILL_CHOPPER, "HL2_KILL_CHOPPER", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_FIND_HEVFACEPLATE, "HL2_FIND_HEVFACEPLATE", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_GET_GRAVITYGUN, "HL2_GET_GRAVITYGUN", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_MAKEABASKET, "HL2_MAKEABASKET", 2);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_HL2_BEAT_CEMETERY, "HL2_BEAT_CEMETERY", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_KILL_ENEMIES_WITHCRANE, "HL2_KILL_ENEMIES_WITHCRANE", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_PIN_SOLDIER_TOBILLBOARD, "HL2_PIN_SOLDIER_TOBILLBOARD", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_HL2_KILL_ODESSAGUNSHIP, "HL2_KILL_ODESSAGUNSHIP", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BEAT_DONTTOUCHSAND, "HL2_BEAT_DONTTOUCHSAND", 20);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BEAT_TURRETSTANDOFF2, "HL2_BEAT_TURRETSTANDOFF2", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_FOLLOW_FREEMAN, "HL2_FOLLOWFREEMAN", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BEAT_TOXICTUNNEL, "HL2_BEAT_TOXICTUNNEL", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BEAT_PLAZASTANDOFF, "HL2_BEAT_PLAZASTANDOFF", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_KILL_ALLC1709SNIPERS, "HL2_KILL_ALLC1709SNIPERS", 5);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BEAT_SUPRESSIONDEVICE, "HL2_BEAT_SUPRESSIONDEVICE", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_HL2_BEAT_C1713STRIDERSTANDOFF, "HL2_BEAT_C1713STRIDERSTANDOFF", 10);
DECLARE_REBEL_MAP_EVENT_ACHIEVEMENT_HIDDEN(ACHIEVEMENT_HL2_BEAT_GAME, "HL2_BEAT_GAME", 25);
DECLARE_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_PORTAL_GET_PORTALGUNS, "PORTAL_GET_PORTALGUNS", 5);
DECLARE_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_PORTAL_KILL_COMPANIONCUBE, "PORTAL_KILL_COMPANIONCUBE", 5);
DECLARE_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_PORTAL_ESCAPE_TESTCHAMBERS, "PORTAL_ESCAPE_TESTCHAMBERS", 5);
DECLARE_MAP_EVENT_ACHIEVEMENT(ACHIEVEMENT_PORTAL_BEAT_GAME, "PORTAL_BEAT_GAME", 10);
#pragma endregion

#endif