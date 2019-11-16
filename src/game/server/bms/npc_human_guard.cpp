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
#include "Sprite.h"
#include "hlss_weapon_id.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HGUARD_MODEL "models/humans/guard.mdl"
#define HGUARD_MODEL2 "models/humans/guard_02.mdl"
#define HGUARD_AE

#define SF_CITIZEN_FOLLOW			( 1 << 16 )	//65536

extern int AE_METROPOLICE_DRAW_PISTOL;		// was	50

extern int ACT_METROPOLICE_DRAW_PISTOL;

ConVar	sk_security_health("sk_security_health", "0");







//=========================================================
// Barney activities
//=========================================================

class CNPC_HumanGuard : public CNPC_BaseColleague
{
public:
	DECLARE_CLASS(CNPC_HumanGuard, CNPC_BaseColleague);
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel();
		BaseClass::Precache();
	}

	void	Spawn(void);
	void	Activate();
	void	SelectModel();
	Class_T Classify(void);
	void	Weapon_Equip(CBaseCombatWeapon *pWeapon);

	virtual void EnableHelmet() { SetBodygroup(2, 1); }

	//bool CreateBehaviors(void);
#ifdef HGUARD_AE
	void HandleAnimEvent(animevent_t *pEvent);
#endif
	//bool ShouldLookForBetterWeapon() { return true; }

	//void OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior);

	int SelectSchedule();
	virtual int		TranslateSchedule(int scheduleType);

	//void 			PrescheduleThink();

	void DeathSound(const CTakeDamageInfo &info);
	/*void GatherConditions();
	void UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void 			CommanderUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	bool			CanJoinPlayerSquad();
	void			AddToPlayerSquad();
	void			RemoveFromPlayerSquad();
	void 			TogglePlayerSquadState();

	void		ModifyOrAppendCriteria(AI_CriteriaSet& set);

	void 			FixupPlayerSquad();
	void 			ClearFollowTarget();
	void 			UpdateFollowCommandPoint();
	bool			IsFollowingCommandPoint();
	bool 			TargetOrder(CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies);

	void			OnRestore();

	CAI_BaseNPC *	GetSquadCommandRepresentative();
	bool IsCommandable()	{ return IsInPlayerSquad(); }

	bool HaveCommandGoal() const
	{
		if (GetCommandGoal() != vec3_invalid)
			return true;
		return false;
	}*/

	virtual void		ProcessSceneEvents(void);

	/*CAI_FuncTankBehavior		m_FuncTankBehavior;
	COutputEvent				m_OnPlayerUse;*/

	DEFINE_CUSTOM_AI;

	bool HasDefaultWeapon(string_t strWeapon) { return (!strcmp(STRING(strWeapon), "0") || !Q_strnicmp(STRING(strWeapon), "Default", 7) || !Q_strnicmp(STRING(strWeapon), "Random", 6)); }
	const char *GetDefaultWeapon() { return "weapon_glock_bms"; }

protected:
	
	

	//string_t		m_iszOriginalSquad;
	//float			m_flTimeJoinedPlayerSquad;
	//bool			m_bWasInPlayerSquad;
	////EHANDLE			m_hFollowSprite;

	//enum CriteriaType
	//{
	//	TYPE_ZERO = 0,
	//	TYPE_PRE,
	//	TYPE_POST,
	//	TYPE_PROVOKED
	//};

	//int m_iCriteriaSet;

	enum
	{
		SCHED_HGUARD_DRAW_PISTOL = BaseClass::NEXT_SCHEDULE,
	};

	//void SetupCustomCriteria(CriteriaType type);

	static colleagueModel_t gm_Models[];

	bool			m_fWeaponDrawn;		// Is my weapon drawn? (ready to use)

	void		InitRandomFlexes();
	bool m_bFlexInit;
	RndFlexData	m_RndFlexData[NUM_RND_HEAD_FLEXES];
	
};


LINK_ENTITY_TO_CLASS(npc_human_security, CNPC_HumanGuard);

//---------------------------------------------------------
// 
//---------------------------------------------------------
//IMPLEMENT_SERVERCLASS_ST(CNPC_HumanGuard, DT_NPC_Barney)
//END_SEND_TABLE()


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_HumanGuard)
//						m_FuncTankBehavior
//DEFINE_OUTPUT(m_OnPlayerUse, "OnPlayerUse"),
DEFINE_KEYFIELD(m_fWeaponDrawn, FIELD_BOOLEAN, "weapondrawn"),
//DEFINE_FIELD(m_iCriteriaSet,FIELD_INTEGER),
//DEFINE_FIELD(m_hFollowSprite, FIELD_EHANDLE),
DEFINE_EMBEDDED_AUTO_ARRAY(m_RndFlexData),
DEFINE_FIELD(m_bFlexInit,FIELD_BOOLEAN),
//DEFINE_USEFUNC(CommanderUse),
//DEFINE_USEFUNC(UseFunc),
END_DATADESC()

colleagueModel_t CNPC_HumanGuard::gm_Models[] =
{
	{ HGUARD_MODEL,	"models/humans/guard_hurt.mdl" },
	{ HGUARD_MODEL2,	"models/humans/guard_hurt.mdl" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanGuard::SelectModel()
{
	/*if (RandomFloat() >= 0.75f)
		SetModelName(AllocPooledString(HGUARD_MODEL2));
	else
		SetModelName(AllocPooledString(HGUARD_MODEL));*/

	SetModelName(AllocPooledString(ChooseColleagueModel(gm_Models)));
}

void CNPC_HumanGuard::Activate()
{
	
	
		//SetupCustomCriteria(TYPE_PRE);
	

	BaseClass::Activate();
}

void CNPC_HumanGuard::ProcessSceneEvents(void)
{
	BaseClass::ProcessSceneEvents();

	if (m_bFlexInit)
	{
		for each (RndFlexData data in m_RndFlexData)
		{
			if (data.bValid)
				SetFlexWeight((LocalFlexController_t)data.index, data.flvalue);
		}
	}
}

void CNPC_HumanGuard::InitRandomFlexes()
{
	for (int i = 0; i < ARRAYSIZE(g_szRandomFlexControls); i++)
	{
		if (FindFlexController(g_szRandomFlexControls[i]))
		{
			m_RndFlexData[i] = RndFlexData(FindFlexController(g_szRandomFlexControls[i]), RandomFloat());
		}
		else
			m_RndFlexData[i] = RndFlexData(false);
	}

	m_bFlexInit = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanGuard::Spawn(void)
{
	Precache();
	InitRandomFlexes();

	m_iHealth = 80;




	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	NPCInit();

	

	if (GetActiveWeapon())
	{
		CBaseCombatWeapon *pWeapon;

		pWeapon = GetActiveWeapon();

		if (!FClassnameIs(pWeapon, "weapon_pistol") && !FClassnameIs(pWeapon, "weapon_glock_bms"))
		{
			m_fWeaponDrawn = true;
		}

		if (!m_fWeaponDrawn)
		{
			GetActiveWeapon()->AddEffects(EF_NODRAW);
			SetBodygroup(FindBodygroupByName("holster"), 1);
		}
	}
	

	SetUse(&CNPC_HumanGuard::CommanderUse); 

	m_nSkin = random->RandomInt(0, GetModelPtr()->numskinfamilies() - 1);
	if (RandomFloat() >= 0.75f)
		EnableHelmet();

	int iChest = FindBodygroupByName("chest");

	if (iChest >= 0)
		SetBodygroup(iChest, RandomInt(0, GetBodygroupCount(iChest) - 1));

	

	/*Vector vecSpriteOrigin = GetAbsOrigin();
	vecSpriteOrigin.z += GetHullHeight();
	vecSpriteOrigin.z += 5.0f;

	m_hFollowSprite = CSprite::SpriteCreate("sprites/waypoint_move.vmt", vecSpriteOrigin, false);
	m_hFollowSprite->SetParent(this);
	variant_t varScale;
	varScale.SetFloat(0.125f);
	m_hFollowSprite->AcceptInput("SetScale", this, this, varScale, 0);

	variant_t emptyvariant;

	m_hFollowSprite->AcceptInput("HideSprite", this, this, emptyvariant, 0);*/
	
	SetNPCFootstepSounds(NPC_STEP_SOUND_MATERIAL, NPC_STEP_SOUND_MATERIAL, "NPC_MetroPolice", "NPC_MetroPolice");
}



int CNPC_HumanGuard::SelectSchedule()
{
	
	if (m_NPCState == NPC_STATE_COMBAT && !m_fWeaponDrawn)
	{
		return SCHED_HGUARD_DRAW_PISTOL;
	}

	return BaseClass::SelectSchedule();
}

int CNPC_HumanGuard::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_WAKE_ANGRY:
	case SCHED_RANGE_ATTACK1:
	{
		if (!m_fWeaponDrawn)
			return SCHED_HGUARD_DRAW_PISTOL;
	}
	break;

	}
	return BaseClass::TranslateSchedule(scheduleType);
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_HumanGuard::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_HumanGuard::Weapon_Equip(CBaseCombatWeapon *pWeapon)
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
#ifdef HGUARD_AE
void CNPC_HumanGuard::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->event == AE_METROPOLICE_DRAW_PISTOL)
	{
		m_fWeaponDrawn = true;
		if (GetActiveWeapon())
		{
			GetActiveWeapon()->RemoveEffects(EF_NODRAW);
			SetBodygroup(FindBodygroupByName("holster"), 2);
		}
		return;
	}
	else if (pEvent->event == AE_NPC_DRAW)
	{
		if (GetActiveWeapon() && (GetActiveWeapon()->GetWeaponID() == HLSS_WEAPON_ID_GLOCK_BMS || GetActiveWeapon()->GetWeaponID() == HLSS_WEAPON_ID_PISTOL))
		{
			m_fWeaponDrawn = true;
			SetBodygroup(FindBodygroupByName("holster"), 2);
		}
		// Fall through to base
	}
	else if (pEvent->event == AE_NPC_HOLSTER)
	{
		if (GetActiveWeapon() && (GetActiveWeapon()->GetWeaponID() == HLSS_WEAPON_ID_GLOCK_BMS || GetActiveWeapon()->GetWeaponID() == HLSS_WEAPON_ID_PISTOL))
		{
			SetBodygroup(FindBodygroupByName("holster"), 1);
		}
		// Fall through to base
	}

	/*switch (pEvent->event)
	{
	case NPC_EVENT_LEFTFOOT:
	{
		EmitSound("NPC_Barney.FootstepLeft", pEvent->eventtime);
	}
	break;
	case NPC_EVENT_RIGHTFOOT:
	{
		EmitSound("NPC_Barney.FootstepRight", pEvent->eventtime);
	}
	break;

	default:*/
		BaseClass::HandleAnimEvent(pEvent);
	//	break;
	//}
}
#endif
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_HumanGuard::DeathSound(const CTakeDamageInfo &info)
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	Speak(TLK_DEATH);

}



//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_human_security, CNPC_HumanGuard)

DECLARE_ANIMEVENT(AE_METROPOLICE_DRAW_PISTOL);

DECLARE_ACTIVITY(ACT_METROPOLICE_DRAW_PISTOL);
//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
SCHED_HGUARD_DRAW_PISTOL,

"	Tasks"
"		TASK_STOP_MOVING				0"
"		TASK_PLAY_SEQUENCE_FACE_ENEMY	ACTIVITY:ACT_METROPOLICE_DRAW_PISTOL"
"		TASK_WAIT_FACE_ENEMY			0.1"
"	"
"	Interrupts"
"	"
);

AI_END_CUSTOM_NPC()

class CNPC_FemSecurity : public CNPC_HumanGuard
{
public:
	DECLARE_CLASS(CNPC_FemSecurity, CNPC_HumanGuard);

	void	SelectModel();
	void	Spawn();
	virtual void EnableHelmet()
	{
		int iHair = FindBodygroupByName("head");
		SetBodygroup(iHair, 5);
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_FemSecurity::SelectModel()
{
	SetModelName(AllocPooledString("models/kake/heartbit_female_guards3.mdl"));
}

void CNPC_FemSecurity::Spawn()
{
	BaseClass::Spawn();

	int iHair = FindBodygroupByName("head");

	if (GetBodygroup(iHair) < 5)
	{
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
	}

	int iGlasses = FindBodygroupByName("glasses");
	SetBodygroup(iGlasses, RandomInt(0, 6));

}

LINK_ENTITY_TO_CLASS(npc_human_security_female, CNPC_FemSecurity);