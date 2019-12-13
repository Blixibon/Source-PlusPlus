//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "weapon_coop_basehlcombatweapon.h"

#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "hl2_gamerules.h"
#include "bobvars.h"

#ifdef CLIENT_DLL
extern IVModelInfoClient* modelinfo;
#else
extern IVModelInfo* modelinfo;
#endif

#ifdef CLIENT_DLL
    #include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	#include "hud_crosshair.h"
#else
	#include "vphysics/constraints.h"
    #include "ilagcompensationmanager.h"
#endif

#include "hl2_player_shared.h"

//================================================================================
// Comandos
//================================================================================

extern ConVar sk_auto_reload_time;

#ifdef CLIENT_DLL
#define	HL2_BOB_CYCLE_MIN	1.0f
#define	HL2_BOB_CYCLE_MAX	0.45f
#define	HL2_BOB			0.002f
#define	HL2_BOB_UP		0.5f

extern float	g_lateralBob;
extern float	g_verticalBob;

// Register these cvars if needed for easy tweaking
static ConVar	v_iyaw_cycle( "v_iyaw_cycle", "2", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iroll_cycle( "v_iroll_cycle", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_ipitch_cycle( "v_ipitch_cycle", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iyaw_level( "v_iyaw_level", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iroll_level( "v_iroll_level", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_ipitch_level( "v_ipitch_level", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT );
#endif

//================================================================================
// Informaci�n y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCoopBaseHLCombat, DT_WeaponCoopBaseHLCombat );

BEGIN_NETWORK_TABLE(CWeaponCoopBaseHLCombat, DT_WeaponCoopBaseHLCombat)
#ifndef CLIENT_DLL
SendPropTime(SENDINFO(m_flRaiseTime)),
SendPropTime(SENDINFO(m_flHolsterTime)),
SendPropBool(SENDINFO(m_bLowered)),
SendPropTime(SENDINFO(m_flTimeLastAction)),
#else
RecvPropTime(RECVINFO(m_flRaiseTime)),
RecvPropTime(RECVINFO(m_flHolsterTime)),
RecvPropBool(RECVINFO(m_bLowered)),
RecvPropTime(RECVINFO(m_flTimeLastAction)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponCoopBaseHLCombat)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_bLowered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flRaiseTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flHolsterTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_flTimeLastAction, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

#ifndef CLIENT_DLL
#include "globalstate.h"
#endif

BEGIN_DATADESC(CWeaponCoopBaseHLCombat)
DEFINE_FIELD(m_bLowered, FIELD_BOOLEAN),
DEFINE_FIELD(m_flRaiseTime, FIELD_TIME),
DEFINE_FIELD(m_flHolsterTime, FIELD_TIME),
DEFINE_FIELD(m_flTimeLastAction, FIELD_TIME),
END_DATADESC();

//================================================================================
//================================================================================
bool CWeaponCoopBaseHLCombat::WeaponShouldBeLowered( void )
{
	if (IsIronsighted())
		return false;

	// Can't be in the middle of another animation
  	if ( GetIdealActivity() != ACT_VM_IDLE_LOWERED && GetIdealActivity() != ACT_VM_IDLE &&
		 GetIdealActivity() != ACT_VM_IDLE_TO_LOWERED && GetIdealActivity() != ACT_VM_LOWERED_TO_IDLE )
  		return false;

	if ( m_bLowered )
		return true;
	
#if !defined( CLIENT_DLL )
	if ( GlobalEntity_GetState("friendly_encounter") == GLOBAL_ON )
		return true;
#endif

	return false;
}

bool CWeaponCoopBaseHLCombat::CanLower()
{
	if (SelectWeightedSequence(ACT_VM_IDLE_LOWERED) == ACTIVITY_NOT_AVAILABLE)
		return false;
	return true;
}

//================================================================================
//================================================================================
bool CWeaponCoopBaseHLCombat::Ready( void )
{
	//Don't bother if we don't have the animation
	if ( SelectWeightedSequence( ACT_VM_LOWERED_TO_IDLE ) == ACTIVITY_NOT_AVAILABLE )
		return false;

	m_bLowered = false;	
	m_flRaiseTime = gpGlobals->curtime + 0.5f;
	m_flTimeLastAction = gpGlobals->curtime;
	return true;
}

//================================================================================
//================================================================================
bool CWeaponCoopBaseHLCombat::Lower( void )
{
	//Don't bother if we don't have the animation
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
		return false;

	m_bLowered = true;
	m_flTimeLastAction = gpGlobals->curtime;
	return true;
}

//================================================================================
//================================================================================
bool CWeaponCoopBaseHLCombat::Deploy( void )
{
	m_flTimeLastAction = gpGlobals->curtime;

	// If we should be lowered, deploy in the lowered position
	// We have to ask the player if the last time it checked, the weapon was lowered
	if ( m_bHasBeenDeployed && GetOwner() && GetOwner()->IsPlayer() )
	{
		CHL2_Player *pPlayer = ToHL2Player( GetOwner() );

		if ( pPlayer && pPlayer->IsWeaponLowered() )
		{
			if ( SelectWeightedSequence(ACT_VM_IDLE_LOWERED) != ACTIVITY_NOT_AVAILABLE )
			{
				if ( DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_IDLE_LOWERED, (char*)GetAnimPrefix() ) )
				{
					m_bLowered = true;

					// Stomp the next attack time to fix the fact that the lower idles are long
					pPlayer->SetNextAttack( gpGlobals->curtime + 1.0 );
					m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
					m_flNextSecondaryAttack	= gpGlobals->curtime + 1.0;
					return true;
				}
			}
		}
	}

	m_bLowered = false;
	return BaseClass::Deploy();
}

//================================================================================
//================================================================================
bool CWeaponCoopBaseHLCombat::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) )
	{
		SetWeaponVisible( false );
		m_flHolsterTime = gpGlobals->curtime;
		return true;
	}

	return false;
}

//================================================================================
//================================================================================
void CWeaponCoopBaseHLCombat::WeaponIdle( void )
{
	if (HasWeaponIdleTimeElapsed())
	{
		m_bHasBeenDeployed = true;
	}

	CHL2_Player* pPlayer = ToHL2Player(GetOwner());
	if (CanSprint() && pPlayer && pPlayer->IsSprinting() && pPlayer->GetAbsVelocity().IsLengthGreaterThan(90.f))
	{
		// Move to lowered position if we're not there yet
		if (GetActivity() != GetSprintActivity() && GetActivity() != ACT_VM_SPRINT_ENTER
			&& GetActivity() != ACT_TRANSITION)
		{
			SendWeaponAnim(GetSprintActivity());
			m_flTimeLastAction = gpGlobals->curtime;
		}
		else if (HasWeaponIdleTimeElapsed())
		{
			// Keep idling low
			SendWeaponAnim(GetSprintActivity());
			m_flTimeLastAction = gpGlobals->curtime;
		}
	}
	//See if we should idle high or low
	else if ( WeaponShouldBeLowered() )
	{
		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED 
			 && GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
			m_flTimeLastAction = gpGlobals->curtime;
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
			m_flTimeLastAction = gpGlobals->curtime;
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( (m_flRaiseTime < gpGlobals->curtime && GetActivity() == ACT_VM_IDLE_LOWERED) || GetActivity() == GetSprintActivity())
		{
			SendWeaponAnim( ACT_VM_IDLE );
			m_flTimeLastAction = gpGlobals->curtime;
		}
		else if (HasFidget() && gpGlobals->curtime - m_flTimeLastAction > 15.f && random->RandomFloat(0, 1) <= 0.9)
		{
			SendWeaponAnim(GetWpnData().iScriptedVMActivities[VM_ACTIVITY_FIDGET]);
			m_flTimeLastAction = gpGlobals->curtime;
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}

//================================================================================
//================================================================================
void CWeaponCoopBaseHLCombat::ItemHolsterFrame()
{
	BaseClass::ItemHolsterFrame();

	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Just load the clip with no animations
		FinishReload();
		m_flHolsterTime = gpGlobals->curtime;
	}
}

#ifdef CLIENT_DLL
float CWeaponCoopBaseHLCombat::CalcViewmodelBob( void )
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;
	
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	speed = clamp( speed, -320, 320 );

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
	
	bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime/ cl_bobcycle.GetFloat())* cl_bobcycle.GetFloat();
	cycle /= cl_bobcycle.GetFloat();

	if ( cycle < cl_bobup.GetFloat())
	{
		cycle = M_PI * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI + M_PI*(cycle- cl_bobup.GetFloat())/(1.0 - cl_bobup.GetFloat());
	}
	
	g_verticalBob = speed*0.005f;
	g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

	g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime/ cl_bobcycle.GetFloat() *2)* cl_bobcycle.GetFloat() *2;
	cycle /= cl_bobcycle.GetFloat() *2;

	if ( cycle < cl_bobup.GetFloat())
	{
		cycle = M_PI * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI + M_PI*(cycle- cl_bobup.GetFloat())/(1.0 - cl_bobup.GetFloat());
	}

	g_lateralBob = speed*0.005f;
	g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
	g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f );
	
	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CWeaponCoopBaseHLCombat::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	if (viewmodel->GetSequenceActivity(viewmodel->GetSequence()) == GetSprintActivity())
		return;

	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	CalcViewmodelBob();

	// Apply bob, but scaled down to 40%
	VectorMA( origin, g_verticalBob * 0.1f, forward, origin );
	
	// Z bob a bit more
	origin[2] += g_verticalBob * 0.1f;
	
	// bob the angles
	angles[ ROLL ]	+= g_verticalBob * 0.5f;
	angles[ PITCH ]	-= g_verticalBob * 0.4f;

	angles[ YAW ]	-= g_lateralBob  * 0.3f;

	VectorMA( origin, g_lateralBob * 0.8f, right, origin );
}

//-----------------------------------------------------------------------------
Vector CWeaponCoopBaseHLCombat::GetBulletSpread( WeaponProficiency_t proficiency )
{
	return BaseClass::GetBulletSpread( proficiency );
}

//-----------------------------------------------------------------------------
float CWeaponCoopBaseHLCombat::GetSpreadBias( WeaponProficiency_t proficiency )
{
	return BaseClass::GetSpreadBias( proficiency );
}
//-----------------------------------------------------------------------------

const WeaponProficiencyInfo_t *CWeaponCoopBaseHLCombat::GetProficiencyValues()
{
	return NULL;
}
#else
float CWeaponCoopBaseHLCombat::CalcViewmodelBob( void )
{
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CWeaponCoopBaseHLCombat::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
}


//-----------------------------------------------------------------------------
Vector CWeaponCoopBaseHLCombat::GetBulletSpread( WeaponProficiency_t proficiency )
{
	Vector baseSpread = BaseClass::GetBulletSpread( proficiency );

	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	float flModifier = (pProficiencyValues)[ proficiency ].spreadscale;
	return ( baseSpread * flModifier );
}

//-----------------------------------------------------------------------------
float CWeaponCoopBaseHLCombat::GetSpreadBias( WeaponProficiency_t proficiency )
{
	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	return (pProficiencyValues)[ proficiency ].bias;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponCoopBaseHLCombat::GetProficiencyValues()
{
	return GetDefaultProficiencyValues();
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeaponCoopBaseHLCombat::GetDefaultProficiencyValues()
{
	// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
	static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
	{
		{ 2.50, 1.0	},
		{ 2.00, 1.0	},
		{ 1.50, 1.0	},
		{ 1.25, 1.0 },
		{ 1.00, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return g_BaseWeaponProficiencyTable;
}
#endif

acttable_t CWeaponCoopBaseHLCombat::s_acttableLowered[] =
{
	//{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	//{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SHOTGUN_RELAXED,		true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_RPG,					true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RPG_RELAXED,			true },
	{ ACT_RUN,						ACT_RUN_RPG_RELAXED,			true },

	
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PASSIVE, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PASSIVE, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PASSIVE, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PASSIVE, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PASSIVE, false },
	{ ACT_HL2MP_SWIM, ACT_HL2MP_SWIM_PASSIVE, false },
	{ ACT_HL2MP_WALK, ACT_HL2MP_WALK_PASSIVE, false },
};

Activity CWeaponCoopBaseHLCombat::ActivityOverride(Activity baseAct, bool *pRequired)
{
	if (/*CanLower() &&*/ WeaponShouldBeLowered() || m_bLowered)
	{
		for (int i = 0; i < ARRAYSIZE(s_acttableLowered); i++)
		{
			const acttable_t& act = s_acttableLowered[i];
			if (baseAct == act.baseAct)
			{
				if (pRequired)
				{
					*pRequired = act.required;
				}
				return (Activity)act.weaponAct;
			}
		}
	}

	return BaseClass::ActivityOverride(baseAct, pRequired);
}

bool CWeaponCoopBaseHLCombat::CanSprint()
{
	return (GetSprintActivity() != ACT_INVALID && SelectWeightedSequence(GetSprintActivity()) != ACT_INVALID);
}

bool CWeaponCoopBaseHLCombat::HasFidget()
{
	if (!GetWpnData().bHasActivity[VM_ACTIVITY_FIDGET])
		return false;

	int iActivity = GetWpnData().iScriptedVMActivities[VM_ACTIVITY_FIDGET];

	return (iActivity != ACT_INVALID && SelectWeightedSequence((Activity)iActivity) != ACT_INVALID);
}
