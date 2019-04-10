//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the soldier version of the combine, analogous to the HL1 grunt.
//
//=============================================================================//

#include "cbase.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "npc_combines.h"
#include "bitstring.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "ndebugoverlay.h"
#include "npcevent.h"
#include "hl2/hl2_player.h"
#include "game.h"
#include "ammodef.h"
#include "explode.h"
#include "ai_memory.h"
#include "Sprite.h"
#include "soundenvelope.h"
#include "weapon_physcannon.h"
#include "hl2_gamerules.h"
#include "gameweaponmanager.h"
#include "vehicle_base.h"
#include "peter\population_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_combine_s_health( "sk_combine_s_health","0");
ConVar	sk_combine_s_kick( "sk_combine_s_kick","0");

ConVar sk_combine_guard_health( "sk_combine_guard_health", "0");
ConVar sk_combine_guard_kick( "sk_combine_guard_kick", "0");
 
// Whether or not the combine guard should spawn health on death
ConVar combine_guard_spawn_health( "combine_guard_spawn_health", "1" );

extern ConVar sk_plr_dmg_buckshot;	
extern ConVar sk_plr_num_shotgun_pellets;

//Whether or not the combine should spawn health on death
ConVar	combine_spawn_health( "combine_spawn_health", "1" );

typedef struct
{
	const char *soldierModel;
	const char *eliteModel;
	const char *soldierShotgunModel;
	const char *eliteShotgunModel;
} combineModel_t;

const char *GetSoldierModel(combineModel_t variant, bool bElite, bool bHasShotgun)
{
	if (bElite)
	{
		if (bHasShotgun && variant.eliteShotgunModel)
		{
			return variant.eliteShotgunModel;
		}
		else
		{
			return variant.eliteModel;
		}
	}
	else
	{
		if (bHasShotgun && variant.soldierShotgunModel)
		{
			return variant.soldierShotgunModel;
		}
		else
		{
			return variant.soldierModel;
		}
	}
}

enum
{
	COMBINE_MODEL_NONE = -1,

	COMBINE_MODEL_NORMAL = 0,
	COMBINE_MODEL_PRISONGUARD,
	COMBINE_MODEL_SUPERSOLDIER,
	COMBINE_MODEL_HUNTER,
	COMBINE_MODEL_OUTLAND,
	COMBINE_MODEL_OUTLANDGUARD,
	COMBINE_MODEL_ADVISORGUARD,
	COMBINE_MODEL_SYNTH,

	MAX_COMBINE_MODELS
};

combineModel_t g_CombineModels[MAX_COMBINE_MODELS] = {
	{"models/combine_soldier.mdl", "models/combine_elite_soldier.mdl", "models/combine_soldier_shotgun.mdl", "models/combine_super_shotgunner.mdl"},
	{"models/combine_soldier_prisonguard.mdl", "models/combine_elite_guard.mdl", "models/combine_soldier_prisonguard_shotgun.mdl", nullptr},
	{"models/combine_super_elite_soldier.mdl", "models/combine_super_soldier.mdl", nullptr, nullptr},
	{ "models/combine_hunter_soldier.mdl", "models/combine_hunter.mdl", nullptr, nullptr },
	{ "models/combine_outland.mdl", "models/combine_super_outland.mdl", "models/combine_outland_shotgun.mdl", nullptr },
	{ "models/combine_outland_prisonguard.mdl", "models/combine_super_outland.mdl", "models/combine_outland_prisonguard_shotgun.mdl", nullptr },
	{ "models/combine_advisor_guard_soldier.mdl", "models/combine_advisor_guard.mdl", nullptr, nullptr },
	{ "models/cmb_synth_soldier.mdl", "models/cmb_synth_elite.mdl", nullptr, nullptr },
};

const char *g_pszCombinePopStrings[MAX_COMBINE_MODELS] = {
	"soldier",
	"prisonguard",
	"supersoldier",
	"hunter",
	"outland",
	"outlandguard",
	"advisorguard",
	"synth",
};

CPopulationDefinition g_CombineSoldierPopulation("combine_soldier", g_pszCombinePopStrings, ARRAYSIZE(g_pszCombinePopStrings));

LINK_ENTITY_TO_CLASS( npc_combine_s, CNPC_CombineS );


#define AE_SOLDIER_BLOCK_PHYSICS		20 // trying to block an incoming physics object

extern Activity ACT_WALK_EASY;
extern Activity ACT_WALK_MARCH;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineS::Spawn( void )
{
	SelectModel();

	Precache();
	SetModel( STRING( GetModelName() ) );

	if( IsElite() )
	{
		// Stronger, tougher.
		SetHealth( sk_combine_guard_health.GetFloat() );
		SetMaxHealth( sk_combine_guard_health.GetFloat() );
		SetKickDamage( sk_combine_guard_kick.GetFloat() );
	}
	else
	{
		SetHealth( sk_combine_s_health.GetFloat() );
		SetMaxHealth( sk_combine_s_health.GetFloat() );
		SetKickDamage( sk_combine_s_kick.GetFloat() );
	}

	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );

	BaseClass::Spawn();

#if HL2_EPISODIC
	if (m_iUseMarch && !HasSpawnFlags(SF_NPC_START_EFFICIENT))
	{
		Msg( "Soldier %s is set to use march anim, but is not an efficient AI. The blended march anim can only be used for dead-ahead walks!\n", GetDebugName() );
	}
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/*string_t CNPC_CombineS::GetModelName() const
{
	string_t iszModelName = BaseClass::GetModelName();

	//
	// If the model refers to an obsolete model, pretend it was blank
	// so that we pick the new default model.
	//
	if (!Q_strnicmp(STRING(iszModelName), "models/c17_", 11) ||
		!Q_strnicmp(STRING(iszModelName), "models/male", 11) ||
		!Q_strnicmp(STRING(iszModelName), "models/female", 13) ||
		!Q_strnicmp(STRING(iszModelName), "models/citizen", 14) ||
		Q_stristr(STRING(iszModelName), "male_cheaple.mdl"))
	{
		return NULL_STRING;
	}

	return iszModelName;
}*/

void CNPC_CombineS::SelectModel()
{
	string_t iszModelName = GetModelName();

	if (!Q_stricmp(STRING(iszModelName), "models/combine_super_soldier.mdl"))
	{
		m_fIsElite = true;
	}

	if (!Q_stricmp(STRING(iszModelName), "models/combine_soldier.mdl") ||
		!Q_stricmp(STRING(iszModelName), "models/combine_super_soldier.mdl") ||
		!Q_stricmp(STRING(iszModelName), "models/combine_soldier_prisonguard.mdl"))
	{
		iszModelName = NULL_STRING;
	}

	if (!iszModelName)
	{
		if (m_iSoldierVariant <= COMBINE_MODEL_NONE || m_iSoldierVariant >= MAX_COMBINE_MODELS)
		{
			m_iSoldierVariant = g_CombineSoldierPopulation.GetRandom();
		}

		const char *pszModelName = GetSoldierModel(g_CombineModels[m_iSoldierVariant], m_fIsElite, m_spawnEquipment == FindPooledString("weapon_shotgun"));

		SetModelName(MAKE_STRING(pszModelName));
	}
	else
	{
		m_iSoldierVariant = COMBINE_MODEL_NORMAL;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_CombineS::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );

	UTIL_PrecacheOther( "item_healthvial" );
	UTIL_PrecacheOther( "weapon_frag" );
	UTIL_PrecacheOther( "item_ammo_ar2_altfire" );

	BaseClass::Precache();
}

int CNPC_CombineS::GetVoiceType()
{
	if (m_iSoldierVariant == COMBINE_MODEL_SYNTH)
		return COMBINE_VOICE_SYNTH;

	return BaseClass::GetVoiceType();
}

void CNPC_CombineS::DeathSound( const CTakeDamageInfo &info )
{
	// NOTE: The response system deals with this at the moment
	if ( GetFlags() & FL_DISSOLVING )
		return;

	GetSentences()->Speak( "COMBINE_DIE", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS ); 
}


//-----------------------------------------------------------------------------
// Purpose: Soldiers use CAN_RANGE_ATTACK2 to indicate whether they can throw
//			a grenade. Because they check only every half-second or so, this
//			condition must persist until it is updated again by the code
//			that determines whether a grenade can be thrown, so prevent the 
//			base class from clearing it out. (sjb)
//-----------------------------------------------------------------------------
void CNPC_CombineS::ClearAttackConditions( )
{
	bool fCanRangeAttack2 = HasCondition( COND_CAN_RANGE_ATTACK2 );

	// Call the base class.
	BaseClass::ClearAttackConditions();

	if( fCanRangeAttack2 )
	{
		// We don't allow the base class to clear this condition because we
		// don't sense for it every frame.
		SetCondition( COND_CAN_RANGE_ATTACK2 );
	}
}

void CNPC_CombineS::PrescheduleThink( void )
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
void CNPC_CombineS::BuildScheduleTestBits( void )
{
	//Interrupt any schedule with physics danger (as long as I'm not moving or already trying to block)
	if ( m_flGroundSpeed == 0.0 && !IsCurSchedule( SCHED_FLINCH_PHYSICS ) )
	{
		SetCustomInterruptCondition( COND_HEAR_PHYSICS_DANGER );
	}

	BaseClass::BuildScheduleTestBits();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_CombineS::SelectSchedule ( void )
{
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CNPC_CombineS::GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info )
{
	switch( iHitGroup )
	{
	case HITGROUP_HEAD:
		{
			// Soldiers take double headshot damage
			return 2.0f;
		}
	}

	return BaseClass::GetHitgroupDamageMultiplier( iHitGroup, info );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_CombineS::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case AE_SOLDIER_BLOCK_PHYSICS:
		DevMsg( "BLOCKING!\n" );
		m_fIsBlocking = true;
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

void CNPC_CombineS::OnChangeActivity( Activity eNewActivity )
{
	// Any new sequence stops us blocking.
	m_fIsBlocking = false;

	BaseClass::OnChangeActivity( eNewActivity );

#if HL2_EPISODIC
	// Give each trooper a varied look for his march. Done here because if you do it earlier (eg Spawn, StartTask), the
	// pose param gets overwritten.
	/*if (m_iUseMarch)
	{
		SetPoseParameter("casual", RandomFloat());
	}*/
#endif
}

void CNPC_CombineS::OnListened()
{
	BaseClass::OnListened();

	if ( HasCondition( COND_HEAR_DANGER ) && HasCondition( COND_HEAR_PHYSICS_DANGER ) )
	{
		if ( HasInterruptCondition( COND_HEAR_DANGER ) )
		{
			ClearCondition( COND_HEAR_PHYSICS_DANGER );
		}
	}

	// debugging to find missed schedules
#if 0
	if ( HasCondition( COND_HEAR_DANGER ) && !HasInterruptCondition( COND_HEAR_DANGER ) )
	{
		DevMsg("Ignore danger in %s\n", GetCurSchedule()->GetName() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CNPC_CombineS::Event_Killed( const CTakeDamageInfo &info )
{
	// Don't bother if we've been told not to, or the player has a megaphyscannon
	if ( combine_spawn_health.GetBool() == false || PlayerHasMegaPhysCannon() )
	{
		BaseClass::Event_Killed( info );
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( info.GetAttacker() );

	if ( !pPlayer )
	{
		CPropVehicleDriveable *pVehicle = dynamic_cast<CPropVehicleDriveable *>( info.GetAttacker() ) ;
		if ( pVehicle && pVehicle->GetDriver() && pVehicle->GetDriver()->IsPlayer() )
		{
			pPlayer = assert_cast<CBasePlayer *>( pVehicle->GetDriver() );
		}
	}

	if ( pPlayer != NULL )
	{
		// Elites drop alt-fire ammo, so long as they weren't killed by dissolving.
		if( IsElite() )
		{
#ifdef HL2_EPISODIC
			if ( HasSpawnFlags( SF_COMBINE_NO_AR2DROP ) == false )
#endif
			{
				CBaseEntity *pItem = DropItem( "item_ammo_ar2_altfire", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );

				if ( pItem )
				{
					IPhysicsObject *pObj = pItem->VPhysicsGetObject();

					if ( pObj )
					{
						Vector			vel		= RandomVector( -64.0f, 64.0f );
						AngularImpulse	angImp	= RandomAngularImpulse( -300.0f, 300.0f );

						vel[2] = 0.0f;
						pObj->AddVelocity( &vel, &angImp );
					}

					if( info.GetDamageType() & DMG_DISSOLVE )
					{
						CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pItem);

						if( pAnimating )
						{
							pAnimating->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
						}
					}
					else
					{
						WeaponManager_AddManaged( pItem );
					}
				}
			}
		}

		IHalfLife2 *pHL2GameRules = HL2GameRules();

		// Attempt to drop health
		if ( pHL2GameRules->NPC_ShouldDropHealth( pPlayer ) )
		{
			DropItem( "item_healthvial", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
			pHL2GameRules->NPC_DroppedHealth();
		}
		
		if ( HasSpawnFlags( SF_COMBINE_NO_GRENADEDROP ) == false )
		{
			// Attempt to drop a grenade
			if ( pHL2GameRules->NPC_ShouldDropGrenade( pPlayer ) )
			{
				DropItem( "weapon_frag", WorldSpaceCenter()+RandomVector(-4,4), RandomAngle(0,360) );
				pHL2GameRules->NPC_DroppedGrenade();
			}
		}
	}

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineS::IsLightDamage( const CTakeDamageInfo &info )
{
	return BaseClass::IsLightDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineS::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Combine considers AR2 fire to be heavy damage
	if ( info.GetAmmoType() == GetAmmoDef()->Index("AR2") )
		return true;

	// 357 rounds are heavy damage
	if ( info.GetAmmoType() == GetAmmoDef()->Index("357") )
		return true;

	// Shotgun blasts where at least half the pellets hit me are heavy damage
	if ( info.GetDamageType() & DMG_BUCKSHOT )
	{
		int iHalfMax = sk_plr_dmg_buckshot.GetFloat() * sk_plr_num_shotgun_pellets.GetInt() * 0.5;
		if ( info.GetDamage() >= iHalfMax )
			return true;
	}

	// Rollermine shocks
	if( (info.GetDamageType() & DMG_SHOCK) && hl2_episodic.GetBool() )
	{
		return true;
	}

	return BaseClass::IsHeavyDamage( info );
}

#if HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: Translate base class activities into combot activites
//-----------------------------------------------------------------------------
Activity CNPC_CombineS::NPC_TranslateActivity( Activity eNewActivity )
{
	// If the special ep2_outland_05 "use march" flag is set, use the more casual marching anim.
	if ( m_iUseMarch && eNewActivity == ACT_WALK )
	{
		eNewActivity = ACT_WALK_EASY;
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}
#endif

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_CombineS)
#ifdef HL2_EPISODIC
DEFINE_KEYFIELD(m_iUseMarch, FIELD_INTEGER, "usemarch"),
#endif
DEFINE_KEYFIELD(m_iSoldierVariant, FIELD_INTEGER, "soldier_variant"),
END_DATADESC()