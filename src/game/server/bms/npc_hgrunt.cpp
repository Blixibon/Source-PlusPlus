//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "ai_route.h"
#include "ai_interactions.h"
#include "ai_tacticalservices.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_basehumanoid.h"
#include "ai_behavior.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_functank.h"
#include "ai_behavior_rappel.h"
#include "ai_behavior_actbusy.h"
#include "ai_sentence.h"
#include "ai_baseactor.h"
#include "activitylist.h"
#include "player.h"
#include "basecombatweapon.h"
#include "basegrenade_shared.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "grenade_frag.h"
#include "ndebugoverlay.h"
#include "weapon_physcannon.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "npc_headcrab.h"
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"
#include "gameweaponmanager.h"
#include "vehicle_base.h"
#include "ammodef.h"
#include "players_system.h"
#include "npc_combine.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//criterion TLK_HG_GREN concept TLK_HG_GREN required
//criterion TLK_HG_ALERT concept TLK_HG_ALERT required
//criterion TLK_HG_MONST concept TLK_HG_MONST required
//criterion TLK_HG_COVER concept TLK_HG_COVER required
//criterion TLK_HG_THROW concept TLK_HG_THROW required
//criterion TLK_HG_TAUNT concept TLK_HG_TAUNT required
//criterion TLK_HG_CHARGE concept TLK_HG_CHARGE required
//criterion TLK_HG_IDLE concept TLK_HG_IDLE required
//criterion TLK_HG_QUEST concept TLK_HG_QUEST required
//criterion TLK_HG_ANSWER concept TLK_HG_ANSWER required
//criterion TLK_HG_CLEAR concept TLK_HG_CLEAR required
//criterion TLK_HG_MEDIC concept TLK_HG_MEDIC required
//criterion TLK_HG_HURTARM concept TLK_HG_HURTARM required
//criterion TLK_HG_HURTLEG concept TLK_HG_HURTLEG required
//criterion TLK_HG_HEAR_COMBAT concept TLK_HG_HEAR_COMBAT required

#define TLK_HG_GREN "TLK_HG_GREN"
#define TLK_HG_ALERT "TLK_HG_ALERT"
#define TLK_HG_MONST "TLK_HG_MONST"
#define TLK_HG_COVER "TLK_HG_COVER"
#define TLK_HG_THROW "TLK_HG_THROW"
#define TLK_HG_TAUNT "TLK_HG_TAUNT"
#define TLK_HG_CHARGE "TLK_HG_CHARGE"
#define TLK_HG_IDLE "TLK_HG_IDLE"
#define TLK_HG_QUEST "TLK_HG_QUEST"
#define TLK_HG_ANSWER "TLK_HG_ANSWER"
#define TLK_HG_CLEAR "TLK_HG_CLEAR"
#define TLK_HG_MEDIC "TLK_HG_MEDIC"
#define TLK_HG_HURTARM "TLK_HG_HURTARM"
#define TLK_HG_HURTLEG "TLK_HG_HURTLEG"
#define TLK_HG_HEAR_COMBAT "TLK_HG_HEAR_COMBAT"
#define TLK_HG_ONFIRE "TLK_HG_BURN"

#define TLK_STARTCOMBAT		"TLK_STARTCOMBAT"
#define TLK_SHOT			"TLK_SHOT"

#define SF_HGRUNT_NO_FREEMAN_SPEECH ( 1 << 18 )

const char *gm_szRndWeapons[2] =
{
	//"weapon_glock",
	"weapon_mp5_bms",
	"weapon_shotgun_bms"
};

//=========================================================
//	>> CNPC_Human_Grunt
//=========================================================
class CNPC_Human_Grunt : public CNPC_Combine
{
	DECLARE_CLASS(CNPC_Human_Grunt, CNPC_Combine);
#if HL2_EPISODIC
	DECLARE_DATADESC();
#endif

public:
	void		Spawn(void);
	void		Precache(void);
	void		DeathSound(const CTakeDamageInfo &info);
	void		PrescheduleThink(void);
	void		BuildScheduleTestBits(void);
	int			SelectSchedule(void);
	float		GetHitgroupDamageMultiplier(int iHitGroup, const CTakeDamageInfo &info);
	void		HandleAnimEvent(animevent_t *pEvent);
	void		OnChangeActivity(Activity eNewActivity);
	void		Event_Killed(const CTakeDamageInfo &info);
	void		OnListened();
	bool		IsMedic() { return FClassnameIs(this, "npc_human_medic"); }

	Class_T		Classify() { return CLASS_HECU; }

	virtual void	ModifyOrAppendCriteria(AI_CriteriaSet& set);
	void		ModifyOrAppendDerivedCriteria(AI_CriteriaSet& set);

	void		ClearAttackConditions(void);

	virtual int TranslateSchedule(int iScheduleType);

	bool		m_fIsBlocking;

	bool		IsLightDamage(const CTakeDamageInfo &info);
	bool		IsHeavyDamage(const CTakeDamageInfo &info);

	virtual	bool		AllowedToIgnite(void) { return true; }

	bool	HasDefaultWeapon(string_t strWeapon) { return (!strcmp(STRING(strWeapon), "0") || !Q_strnicmp(STRING(strWeapon), "Default", 7) || !Q_strnicmp(STRING(strWeapon), "Random", 6)); }
	const char *GetDefaultWeapon()
	{
		return gm_szRndWeapons[RandomInt(0, 1)];
	}

	virtual int			GetSpecialDSP(void)
	{
		return (GetGruntVoiceType() == VOICE_MASK_DYNAMIC) ? 55 : 0;
	}

	virtual gender_t		GetActorGender()
	{
		return (GetGruntVoiceType() >= VOICE_MASK_STATIC) ? GENDER_FEMALE : GENDER_MALE;
	}

	virtual int GetGruntVoiceType() { return m_nVoiceType; }

	enum VoiceType_e
	{
		VOICE_NORMAL = 0,
		VOICE_MASK_DYNAMIC,
		VOICE_MASK_STATIC,
		VOICE_MASK_GRUFF,
		VOICE_MASK_YOUNG,

		NUM_VOICE_TYPES
	};

private:
	bool		ShouldHitPlayer(const Vector &targetDir, float targetDist);

	

#if HL2_EPISODIC
public:
	Activity	NPC_TranslateActivity(Activity eNewActivity);

protected:
	/// whether to use the more casual march anim in ep2_outland_05
	int			m_iUseMarch;
#endif

	int m_nVoiceType;
};

ConVar	sk_human_grunt_health("sk_human_grunt_health", "0");
ConVar	npc_marines_melee_dmg("npc_marines_melee_dmg", "0");

ConVar sk_human_commander_health("sk_human_commander_health", "0");
//ConVar sk_combine_guard_kick("sk_combine_guard_kick", "0");

// Whether or not the combine guard should spawn health on death
extern ConVar combine_guard_spawn_health;

extern ConVar sk_plr_dmg_buckshot;
extern ConVar sk_plr_num_shotgun_pellets;

//Whether or not the combine should spawn health on death
extern ConVar	combine_spawn_health;

LINK_ENTITY_TO_CLASS(npc_human_grunt, CNPC_Human_Grunt);
LINK_ENTITY_TO_CLASS(npc_human_medic, CNPC_Human_Grunt);
LINK_ENTITY_TO_CLASS(npc_human_commander, CNPC_Human_Grunt);


#define AE_SOLDIER_BLOCK_PHYSICS		20 // trying to block an incoming physics object

extern Activity ACT_WALK_EASY;
extern Activity ACT_WALK_MARCH;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Human_Grunt::Spawn(void)
{
	Precache();
	SetModel(STRING(GetModelName()));

	if (IsElite())
	{
		// Stronger, tougher.
		SetHealth(sk_human_commander_health.GetFloat());
		SetMaxHealth(sk_human_commander_health.GetFloat());
		SetKickDamage(npc_marines_melee_dmg.GetFloat());
	}
	else
	{
		SetHealth(sk_human_grunt_health.GetFloat());
		SetMaxHealth(sk_human_grunt_health.GetFloat());
		SetKickDamage(npc_marines_melee_dmg.GetFloat());
	}

	CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_DOORS_GROUP);

	BaseClass::Spawn();

#if HL2_EPISODIC
	if (m_iUseMarch && !HasSpawnFlags(SF_NPC_START_EFFICIENT))
	{
		Msg("Soldier %s is set to use march anim, but is not an efficient AI. The blended march anim can only be used for dead-ahead walks!\n", GetDebugName());
	}
#endif

	m_nSkin = random->RandomInt(0, GetModelPtr()->numskinfamilies() - 1);

	m_nVoiceType = VOICE_NORMAL;

	if (FClassnameIs(this, "npc_human_medic"))
	{
		SetBodygroup(FindBodygroupByName("helmet_medic"), 1);
	}
	else if (IsElite())
	{
		if (RandomInt(1, 3) == 1)
			SetBodygroup(FindBodygroupByName("head"), 3);

		// --Add Baret Here
		CPhysicsProp *pProp = static_cast<CPhysicsProp *>(CreateEntityByName("prop_physics_override"));
		if (pProp != NULL)
		{
			// Set the model
			pProp->SetModelName(AllocPooledString("models/humans/props/marine_beret.mdl"));
			DispatchSpawn(pProp);
			pProp->VPhysicsDestroyObject();
			pProp->FollowEntity(this, true);
			m_AttachedEntities.AddToTail(pProp);
		}
	}
	else if (RandomInt(1, 3) == 1)
	{
		SetBodygroup(FindBodygroupByName("gasmask_nv"), 1);
		m_nVoiceType = RandomInt(VOICE_MASK_GRUFF, VOICE_MASK_YOUNG);
	}
	else
	{
		int iRand = RandomInt(0, 2);
		SetBodygroup(FindBodygroupByName("head"), iRand);
		if (iRand == 2)
			m_nVoiceType = RandomInt(VOICE_MASK_DYNAMIC, VOICE_MASK_STATIC);
	}

	SetBodygroup(FindBodygroupByName("gloves"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("packs_chest"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("packs_hips"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("packs_thigh"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("holster"), 1);
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Human_Grunt::Precache()
{
	BaseClass::Precache();

	if (!GetModelName())
	{
		if (m_spawnEquipment == AllocPooledString("weapon_mp5_bms"))
			SetModelName(MAKE_STRING("models/humans/marine.mdl"));
		else
			SetModelName(MAKE_STRING("models/humans/marine_sg.mdl"));
	}

	if (FClassnameIs(this, "npc_human_commander"))
	{
		m_fIsElite = true;
	}
	else
	{
		m_fIsElite = false;
	}

	PrecacheModel(STRING(GetModelName()));

	PrecacheScriptSound("NPC_HGrunt.Death");

	UTIL_PrecacheOther("item_healthvial");
	UTIL_PrecacheOther("weapon_frag");
	UTIL_PrecacheOther("item_ammo_ar2_altfire");
	UTIL_PrecacheOther("prop_physics_override", "models/humans/props/marine_beret.mdl");
}

void CNPC_Human_Grunt::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendCriteria(set);

	switch (GetGruntVoiceType())
	{
	default:
		set.AppendCriteria("voicetype", "normal");
		break;
	case VOICE_MASK_GRUFF:
		set.AppendCriteria("voicetype", "gruff");
		break;
	case VOICE_MASK_YOUNG:
		set.AppendCriteria("voicetype", "young");
		break;
	}
}

void CNPC_Human_Grunt::ModifyOrAppendDerivedCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendDerivedCriteria(set);

	int iIndex = set.FindCriterionIndex("enemyplayerfreeman");

	if (!HasSpawnFlags(SF_HGRUNT_NO_FREEMAN_SPEECH) && iIndex > -1 && atoi(set.GetValue(iIndex)) > 0)
		set.AppendCriteria("freemanlines", "1");
	else
		set.AppendCriteria("freemanlines", "0");
}


void CNPC_Human_Grunt::DeathSound(const CTakeDamageInfo &info)
{
	// NOTE: The response system deals with this at the moment
	if (GetFlags() & FL_DISSOLVING)
		return;

	EmitSound("NPC_HGrunt.Death");
}


//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPC_Human_Grunt::ClearAttackConditions()
{
	bool fCanRangeAttack2 = HasCondition(COND_CAN_RANGE_ATTACK2);

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if (fCanRangeAttack2)
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition(COND_CAN_RANGE_ATTACK2);
	}
}

int CNPC_Human_Grunt::TranslateSchedule(int iScheduleType)
{
	int iBaseTranslation = BaseClass::TranslateSchedule(iScheduleType);

	if (iBaseTranslation == SCHED_COMBINE_BURNING_STAND)
		Speak(TLK_HG_ONFIRE);

	return iBaseTranslation;
}

void CNPC_Human_Grunt::PrescheduleThink(void)
{
	/*//FIXME: This doesn't need to be in here, it's all debug info
	if( HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
	// Don't react unless we see the item!!
	CSound *pSound = NULL;

	pSound = GetLoudestSoundOfType( SOUND_PHYSICS_DANGER );

	if( pSound )
	{
	if( FInViewCone( pSound->GetSoundReactOrigin() ) )
	{
	DevMsg( "OH CRAP!\n" );
	NDebugOverlay::Line( EyePosition(), pSound->GetSoundReactOrigin(), 0, 0, 255, false, 2.0f );
	}
	}
	}
	*/

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
// Purpose: Allows for modification of the interrupt mask for the current schedule.
//			In the most cases the base implementation should be called first.
//-----------------------------------------------------------------------------
void CNPC_Human_Grunt::BuildScheduleTestBits(void)
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if (m_flGroundSpeed == 0.0 && !IsCurSchedule(SCHED_FLINCH_PHYSICS))
	{
		SetCustomInterruptCondition(COND_HEAR_PHYSICS_DANGER);
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Human_Grunt::SelectSchedule(void)
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_Human_Grunt::GetHitgroupDamageMultiplier(int iHitGroup, const CTakeDamageInfo &info)
{
	switch (iHitGroup)
	{
	case HITGROUP_HEAD:
	{
		// Soldiers take double headshot damage
		return 2.0f;
	}
	}

	return BaseClass::GetHitgroupDamageMultiplier(iHitGroup, info);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Human_Grunt::HandleAnimEvent(animevent_t *pEvent)
{
	switch (pEvent->event)
	{
	case AE_SOLDIER_BLOCK_PHYSICS:
		DevMsg("BLOCKING!\n");
		m_fIsBlocking = true;
		break;

	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

void CNPC_Human_Grunt::OnChangeActivity(Activity eNewActivity)
{
	// Any new sequence stops us blocking.
	m_fIsBlocking = false;

	BaseClass::OnChangeActivity(eNewActivity);

#if HL2_EPISODIC
	// Give each trooper a varied look for his march. Done here because if you do it earlier (eg Spawn, StartTask), the
	// pose param gets overwritten.
	if (m_iUseMarch)
	{
		SetPoseParameter("casual", RandomFloat());
	}
#endif
}

void CNPC_Human_Grunt::OnListened()
{
	BaseClass::OnListened();

	if (HasCondition(COND_HEAR_DANGER) && HasCondition(COND_HEAR_PHYSICS_DANGER))
	{
		if (HasInterruptCondition(COND_HEAR_DANGER))
		{
			ClearCondition(COND_HEAR_PHYSICS_DANGER);
		}
	}

	/*if (m_NPCState < NPC_STATE_COMBAT)
	{
		if (HasCondition(COND_HEAR_COMBAT))
		{
			Speak(TLK_HG_HEAR_COMBAT);
		}
	}*/

	// debugging to find missed schedules
#if 0
	if (HasCondition(COND_HEAR_DANGER) && !HasInterruptCondition(COND_HEAR_DANGER))
	{
		DevMsg("Ignore danger in %s\n", GetCurSchedule()->GetName());
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPC_Human_Grunt::Event_Killed(const CTakeDamageInfo &info)
{
	// Don't bother if we've been told not to, or the player has a megaphyscannon
	if (combine_spawn_health.GetBool() == false || PlayerHasMegaPhysCannon())
	{
		BaseClass::Event_Killed(info);
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer(info.GetAttacker());

	if (!pPlayer)
	{
		CPropVehicleDriveable *pVehicle = dynamic_cast<CPropVehicleDriveable *>(info.GetAttacker());
		if (pVehicle && pVehicle->GetDriver() && pVehicle->GetDriver()->IsPlayer())
		{
			pPlayer = assert_cast<CBasePlayer *>(pVehicle->GetDriver());
		}
	}

	if (pPlayer != NULL)
	{
		// Elites drop alt-fire ammo, so long as they weren't killed by dissolving.
		/*if (IsElite())
		{
#ifdef HL2_EPISODIC
			if (HasSpawnFlags(SF_COMBINE_NO_AR2DROP) == false)
#endif
			{
				CBaseEntity *pItem = DropItem("item_ammo_ar2_altfire", WorldSpaceCenter() + RandomVector(-4, 4), RandomAngle(0, 360));

				if (pItem)
				{
					IPhysicsObject *pObj = pItem->VPhysicsGetObject();

					if (pObj)
					{
						Vector			vel = RandomVector(-64.0f, 64.0f);
						AngularImpulse	angImp = RandomAngularImpulse(-300.0f, 300.0f);

						vel[2] = 0.0f;
						pObj->AddVelocity(&vel, &angImp);
					}

					if (info.GetDamageType() & DMG_DISSOLVE)
					{
						CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pItem);

						if (pAnimating)
						{
							pAnimating->Dissolve(NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL);
						}
					}
					else
					{
						WeaponManager_AddManaged(pItem);
					}
				}
			}
		}*/

		IHalfLife2* pHL2GameRules = HL2GameRules();

		// Attempt to drop health
		if (pHL2GameRules->NPC_ShouldDropHealth(pPlayer))
		{
			DropItem("item_healthvial", WorldSpaceCenter() + RandomVector(-4, 4), RandomAngle(0, 360));
			pHL2GameRules->NPC_DroppedHealth();
		}

		if (HasSpawnFlags(SF_COMBINE_NO_GRENADEDROP) == false)
		{
			// Attempt to drop a grenade
			if (pHL2GameRules->NPC_ShouldDropGrenade(pPlayer))
			{
				DropItem("weapon_frag", WorldSpaceCenter() + RandomVector(-4, 4), RandomAngle(0, 360));
				pHL2GameRules->NPC_DroppedGrenade();
			}
		}
	}

	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Human_Grunt::IsLightDamage(const CTakeDamageInfo &info)
{
	return BaseClass::IsLightDamage(info);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Human_Grunt::IsHeavyDamage(const CTakeDamageInfo &info)
{
	// Combine considers AR2 fire to be heavy damage
	if (info.GetAmmoType() == GetAmmoDef()->Index("AR2"))
		return true;

	// 357 rounds are heavy damage
	if (info.GetAmmoType() == GetAmmoDef()->Index("357"))
		return true;

	// Shotgun blasts where at least half the pellets hit me are heavy damage
	if (info.GetDamageType() & DMG_BUCKSHOT)
	{
		int iHalfMax = sk_plr_dmg_buckshot.GetFloat() * sk_plr_num_shotgun_pellets.GetInt() * 0.5;
		if (info.GetDamage() >= iHalfMax)
			return true;
	}

	// Rollermine shocks
	if ((info.GetDamageType() & DMG_SHOCK) && hl2_episodic.GetBool())
	{
		return true;
	}

	return BaseClass::IsHeavyDamage(info);
}

#if HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: Translate base class activities into combot activites
//-----------------------------------------------------------------------------
Activity CNPC_Human_Grunt::NPC_TranslateActivity(Activity eNewActivity)
{
	// If the special ep2_outland_05 "use march" flag is set, use the more casual marching anim.
	if (m_iUseMarch && eNewActivity == ACT_WALK)
	{
		eNewActivity = ACT_WALK_MARCH;
	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_Human_Grunt)

DEFINE_KEYFIELD(m_iUseMarch, FIELD_INTEGER, "usemarch"),
DEFINE_FIELD(m_nVoiceType, FIELD_INTEGER),

END_DATADESC()
#endif