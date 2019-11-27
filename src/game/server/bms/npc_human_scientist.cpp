//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_basecollegue.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"
#include "ai_behavior_functank.h"
#include "bms_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MSCI_MODEL "models/humans/scientist.mdl"
#define MSCI_MODEL2 "models/humans/scientist_02.mdl"
#define FSCI_MODEL "models/humans/scientist_female.mdl"

#define SF_CITIZEN_FOLLOW			( 1 << 16 )	//65536

//extern const float HEAL_MOVE_RANGE;
//extern const float HEAL_TARGET_RANGE;
//extern const float HEAL_TARGET_RANGE_Z;
const float HEAL_MOVE_RANGE = 30 * 12;
const float HEAL_TARGET_RANGE = 120; // 10 feet
const float HEAL_TOSS_TARGET_RANGE = 480; // 40 feet when we are throwing medkits 
const float HEAL_TARGET_RANGE_Z = 72; // a second check that Gordon isn't too far above us -- 6 feet

extern ConVar	sk_citizen_heal_player;
extern ConVar	sk_citizen_heal_player_delay;
extern ConVar	sk_citizen_heal_player_min_pct;
extern ConVar	sk_citizen_heal_player_min_forced;
extern ConVar	sk_citizen_heal_ally;
extern ConVar	sk_citizen_heal_ally_delay;
extern ConVar	sk_citizen_heal_ally_min_pct;
extern ConVar	sk_citizen_player_stare_time;
extern ConVar  sk_citizen_player_stare_dist;
extern ConVar	sk_citizen_stare_heal_time;
extern ConVar	npc_citizen_medic_emit_sound;

int AE_HEAL;

//ConVar	sk_security_health("sk_security_health", "0");

//=========================================================
// Barney activities
//=========================================================

class CNPC_BaseScientist : public CNPC_BaseColleague
{
public:
	DECLARE_CLASS(CNPC_BaseScientist, CNPC_BaseColleague);
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		BaseClass::Precache();
		UTIL_PrecacheOther("item_syringe");
	}

	void	Spawn(void);
	void	SelectModel();
	Class_T Classify(void);
	void	Weapon_Equip(CBaseCombatWeapon *pWeapon);

	bool 			IsMedic() { return true; }

	//bool CreateBehaviors(void);

	void HandleAnimEvent(animevent_t *pEvent);

	virtual int		TranslateSchedule(int nSchedule);

	bool 			CanHeal();
	void 			Heal();

	void			UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	//bool ShouldLookForBetterWeapon() { return true; }

	//void OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior);

	void DeathSound(const CTakeDamageInfo &info);

	WeaponProficiency_t CalcWeaponProficiency(CBaseCombatWeapon *pWeapon)
	{
		return WEAPON_PROFICIENCY_POOR;
	}

	/*CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;*/

	DEFINE_CUSTOM_AI;

protected:

	enum
	{
		SCHED_SCIENTIST_HEAL_ALLY = BaseClass::NEXT_SCHEDULE,
	};

	static colleagueModel_t gm_Models[];


//	enum CriteriaType
//	{
//		TYPE_ZERO = 0,
//		TYPE_PRE,
//		TYPE_POST,
//		TYPE_PROVOKED
//	};
//
//	void SetupCustomCriteria(CriteriaType type);
//
//	string_t		m_iszOriginalSquad;
//	float			m_flTimeJoinedPlayerSquad;
//	bool			m_bWasInPlayerSquad;

};


LINK_ENTITY_TO_CLASS(npc_human_scientist, CNPC_BaseScientist);

//---------------------------------------------------------
// 
//---------------------------------------------------------
//IMPLEMENT_SERVERCLASS_ST(CNPC_BaseScientist, DT_NPC_Barney)
//END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_BaseScientist)
//						m_FuncTankBehavior
//DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
//DEFINE_FIELD(m_iszOriginalSquad, FIELD_STRING),
//DEFINE_FIELD(m_flTimeJoinedPlayerSquad, FIELD_TIME),
//DEFINE_FIELD(m_bWasInPlayerSquad, FIELD_BOOLEAN),
//DEFINE_USEFUNC(CommanderUse),
//DEFINE_USEFUNC(UseFunc),
END_DATADESC()

colleagueModel_t CNPC_BaseScientist::gm_Models[] =
{
	{ MSCI_MODEL,	"models/humans/scientist_hurt.mdl", 0, 15 },
	{ MSCI_MODEL2,	"models/humans/scientist_hurt_02.mdl", 1, 7 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScientist::SelectModel()
{
	/*if (RandomFloat() >= 0.75f)
		SetModelName(AllocPooledString(MSCI_MODEL2));
	else
		SetModelName(AllocPooledString(MSCI_MODEL));*/

	SetModelName(AllocPooledString(ChooseColleagueModel(gm_Models, 2, m_nSkin.GetForModify())));
}

#define SCI_MAX_GLASSES 6

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseScientist::Spawn(void)
{
	m_iHealth = 80;



	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);
	//CapabilitiesRemove(bits_CAP_USE_WEAPONS);

	NPCInit();

	


	SetUse(&CNPC_BaseScientist::CommanderUse);

	//m_nSkin = gm_iLastChosenSkin;
	int iGlasses = FindBodygroupByName("glasses");
	SetBodygroup(iGlasses, RandomInt(0, SCI_MAX_GLASSES));
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_BaseScientist::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseScientist::Weapon_Equip(CBaseCombatWeapon *pWeapon)
{
	BaseClass::Weapon_Equip(pWeapon);

	if (hl2_episodic.GetBool() && FClassnameIs(pWeapon, "weapon_ar2"))
	{
		// Allow Barney to defend himself at point-blank range in c17_05.
		pWeapon->m_fMinRange1 = 0.0f;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_BaseScientist::HandleAnimEvent(animevent_t *pEvent)
{
	//switch (pEvent->event)
	//{
	//	/*case NPC_EVENT_LEFTFOOT:
	//	{
	//	EmitSound("NPC_Barney.FootstepLeft", pEvent->eventtime);
	//	}
	//	break;
	//	case NPC_EVENT_RIGHTFOOT:
	//	{
	//	EmitSound("NPC_Barney.FootstepRight", pEvent->eventtime);
	//	}
	//	break;*/

	//default:
		if (pEvent->event == AE_HEAL)
		{
			Heal();
		}
		else
		{
			BaseClass::HandleAnimEvent(pEvent);
		}
		/*break;
	}*/
}

int CNPC_BaseScientist::TranslateSchedule(int nSchedule)
{
	switch (nSchedule)
	{
	case SCHED_SUPPLIER_HEAL:
	case SCHED_SUPPLIER_HEAL_TOSS:
		return SCHED_SCIENTIST_HEAL_ALLY;
		break;
	default:
		return BaseClass::TranslateSchedule(nSchedule);
		break;
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_BaseScientist::DeathSound(const CTakeDamageInfo &info)
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	Speak(TLK_DEATH);

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseScientist::CanHeal()
{
	if (!IsMedic())
		return false;

	if (GetActiveWeapon() && m_NPCState == NPC_STATE_COMBAT)
		return false;

	if (IsInAScript() || (m_NPCState == NPC_STATE_SCRIPT))
		return false;

	return true;
}

void CNPC_BaseScientist::UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (HasCondition(COND_SUPPLIER_PLAYERHEALREQUEST))
		return;

	CBasePlayer *pPlayer = pActivator->IsPlayer() ? (CBasePlayer *)pActivator : GetBestPlayer();
	if (pPlayer && pPlayer->FInViewCone(this) && CanHeal())
	{
		if (ShouldHealTarget(pPlayer, true))
		{
			SetCondition(COND_SUPPLIER_PLAYERHEALREQUEST);
			return;
		}
	}

	BaseClass::UseFunc(pActivator, pCaller, useType, value);
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseScientist::Heal()
{
	if (!CanHeal())
		return;

	CBaseEntity *pTarget = GetTarget();

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
			if (/*pTarget->IsPlayer() && */npc_citizen_medic_emit_sound.GetBool())
			{
				CPASAttenuationFilter filter(pTarget, "Syringe.Touch");
				EmitSound(filter, pTarget->entindex(), "Syringe.Touch");
			}

			pTarget->TakeHealth(healAmt, DMG_GENERIC);
			pTarget->RemoveAllDecals();
		}
	}
}



//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_human_scientist, CNPC_BaseScientist)

DECLARE_ANIMEVENT(AE_HEAL)

DEFINE_SCHEDULE
(
	SCHED_SCIENTIST_HEAL_ALLY,

	"	Tasks"
	"		TASK_GET_PATH_TO_TARGET				0"
	"		TASK_MOVE_TO_TARGET_RANGE			50"
	"		TASK_STOP_MOVING					0"
	"		TASK_FACE_IDEAL						0"
	"		TASK_WEAPON_HOLSTER					0"
	"		TASK_HEAL							0"
	"		TASK_WEAPON_UNHOLSTER				1"
	"	"
	"	Interrupts"
	)

AI_END_CUSTOM_NPC()




class CNPC_FemScientist : public CNPC_BaseScientist
{
public:
	DECLARE_CLASS(CNPC_FemScientist, CNPC_BaseScientist);

	void	SelectModel();
	void	Spawn();


};



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FemScientist::SelectModel()
{
	SetModelName(AllocPooledString(FSCI_MODEL));
}

void CNPC_FemScientist::Spawn()
{
	BaseClass::Spawn();

	m_nSkin = RandomInt(0, GetModelPtr()->numskinfamilies() - 1);

	int iHair = FindBodygroupByName("hair");

	switch (m_nSkin)
	{
	case 1:
		SetBodygroup(iHair, 2);
		break;
	case 2:
		SetBodygroup(iHair, 4);
		break;
	case 3:
		SetBodygroup(iHair, 3);
		break;
	case 4:
		SetBodygroup(iHair, 4);
		break;
	case 5:
		SetBodygroup(iHair, 4);
		break;
	case 6:
		SetBodygroup(iHair, 4);
		break;
	case 0:
	default:
		SetBodygroup(iHair, 0);
		break;
	}

	SetBodygroup(0, RandomInt(0, 1));
}

LINK_ENTITY_TO_CLASS(npc_human_scientist_female, CNPC_FemScientist);

//{
//	// Skin 0 wants bodygroup "hair" to be "0".
//	"0"
//	{
//		"hair"	"0"
//	}
//
//	// Skin 1 wants bodygroup "hair" to be "2".
//	"1"
//	{
//		"hair"	"2"
//	}
//
//	"2"
//	{
//		"hair"	"4"
//	}
//
//	"3"
//	{
//		"hair"	"3"
//	}
//
//	"4"
//	{
//		"hair"	"4"
//	}
//
//	"5"
//	{
//		"hair"	"4"
//	}
//
//	"6"
//	{
//		"hair"	"4"
//	}
//}

class CNPC_ScientistKleiner : public CNPC_BaseScientist
{
public:
	DECLARE_CLASS(CNPC_ScientistKleiner, CNPC_BaseScientist);

	void	SelectModel();
	void	Spawn();
	Class_T Classify()
	{
		return CLASS_PLAYER_ALLY_VITAL;
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_ScientistKleiner::SelectModel()
{
	SetModelName(AllocPooledString("models/humans/scientist_kliener.mdl"));
}

void CNPC_ScientistKleiner::Spawn()
{
	AddSpawnFlags(SF_CITIZEN_FOLLOW);
	BaseClass::Spawn();
}

class CNPC_ScientistEli : public CNPC_ScientistKleiner
{
public:
	DECLARE_CLASS(CNPC_ScientistEli, CNPC_ScientistKleiner);

	void	SelectModel();

};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_ScientistEli::SelectModel()
{
	SetModelName(AllocPooledString("models/humans/scientist_eli.mdl"));
}

LINK_ENTITY_TO_CLASS(npc_human_scientist_kleiner, CNPC_ScientistKleiner);
LINK_ENTITY_TO_CLASS(npc_human_scientist_eli, CNPC_ScientistEli);