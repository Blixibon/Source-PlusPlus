//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hl1_basecombatweapon_shared.h"
#include "bobvars.h"

#include "effect_dispatch_data.h"

#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"
#else
#include "te_effect_dispatch.h"
#endif

#include "hl2_player_shared.h"



LINK_ENTITY_TO_CLASS( basehl1combatweapon, CCoopHL1CombatWeapon );

BEGIN_DATADESC(CCoopHL1CombatWeapon)
#ifndef CLIENT_DLL
DEFINE_THINKFUNC(FallThink),
#endif
END_DATADESC();

IMPLEMENT_NETWORKCLASS_ALIASED(CoopHL1CombatWeapon, DT_CoopHL1CombatWeapon);

BEGIN_NETWORK_TABLE(CCoopHL1CombatWeapon, DT_CoopHL1CombatWeapon)
END_NETWORK_TABLE();

BEGIN_PREDICTION_DATA(CCoopHL1CombatWeapon)
END_PREDICTION_DATA();


void CCoopHL1CombatWeapon::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BBOX );
	m_flNextEmptySoundTime = 0.0f;

	// Weapons won't show up in trace calls if they are being carried...
	RemoveEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	m_iState = WEAPON_NOT_CARRIED;
	// Assume 
	m_nViewModelIndex = 0;

	// If I use clips, set my clips to the default
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1 = GetDefaultClip1();
	}
	else
	{
		SetPrimaryAmmoCount( GetDefaultClip1() );
		m_iClip1 = WEAPON_NOCLIP;
	}
	if ( UsesClipsForAmmo2() )
	{
		m_iClip2 = GetDefaultClip2();
	}
	else
	{
		SetSecondaryAmmoCount( GetDefaultClip2() );
		m_iClip2 = WEAPON_NOCLIP;
	}

	SetModel( GetWorldModel() );

#if !defined( CLIENT_DLL )
	FallInit();
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetBlocksLOS( false );

	// Default to non-removeable, because we don't want the
	// game_weapon_manager entity to remove weapons that have
	// been hand-placed by level designers. We only want to remove
	// weapons that have been dropped by NPC's.
	SetRemoveable( false );
#endif

	//Make weapons easier to pick up in MP.
	if ( g_pGameRules->IsMultiplayer() )
	{
		CollisionProp()->UseTriggerBounds( true, 36 );
	}
	else
	{
		CollisionProp()->UseTriggerBounds( true, 24 );
	}

	// Use more efficient bbox culling on the client. Otherwise, it'll setup bones for most
	// characters even when they're not in the frustum.
	AddEffects( EF_BONEMERGE_FASTCULL );
}

CBasePlayer* CCoopHL1CombatWeapon::GetPlayerOwner() const
{
	return dynamic_cast<CBasePlayer*>(GetOwner());
}

void CCoopHL1CombatWeapon::EjectShell(CBaseEntity* pPlayer, int iType)
{
	QAngle angShellAngles = pPlayer->GetAbsAngles();

	Vector vecForward, vecRight, vecUp;
	AngleVectors(angShellAngles, &vecForward, &vecRight, &vecUp);

	Vector vecShellPosition = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();
	switch (iType)
	{
	case 0:
	default:
		vecShellPosition += vecRight * 4;
		vecShellPosition += vecUp * -12;
		vecShellPosition += vecForward * 20;
		break;
	case 1:
		vecShellPosition += vecRight * 6;
		vecShellPosition += vecUp * -12;
		vecShellPosition += vecForward * 32;
		break;
	}

	Vector vecShellVelocity = vec3_origin; // pPlayer->GetAbsVelocity();
	vecShellVelocity += vecRight * random->RandomFloat(50, 70);
	vecShellVelocity += vecUp * random->RandomFloat(100, 150);
	vecShellVelocity += vecForward * 25;

	angShellAngles.x = 0;
	angShellAngles.z = 0;

	CEffectData	data;
	data.m_vStart = vecShellVelocity;
	data.m_vOrigin = vecShellPosition;
	data.m_vAngles = angShellAngles;
	data.m_fFlags = iType;

	DispatchEffect("HL1ShellEject", data);
}

#if defined( CLIENT_DLL )

#define	HL1_BOB_CYCLE_MIN	1.0f
#define	HL1_BOB_CYCLE_MAX	0.45f
#define	HL1_BOB			0.002f
#define	HL1_BOB_UP		0.5f

extern float	g_lateralBob;
extern float	g_verticalBob;

// Register these cvars if needed for easy tweaking
static ConVar	v_iyaw_cycle( "v_iyaw_cycle", "2"/*, FCVAR_UNREGISTERED*/ );
static ConVar	v_iroll_cycle( "v_iroll_cycle", "0.5"/*, FCVAR_UNREGISTERED*/ );
static ConVar	v_ipitch_cycle( "v_ipitch_cycle", "1"/*, FCVAR_UNREGISTERED*/ );
static ConVar	v_iyaw_level( "v_iyaw_level", "0.3"/*, FCVAR_UNREGISTERED*/ );
static ConVar	v_iroll_level( "v_iroll_level", "0.1"/*, FCVAR_UNREGISTERED*/ );
static ConVar	v_ipitch_level( "v_ipitch_level", "0.3"/*, FCVAR_UNREGISTERED*/ );

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float g_bob;
float CCoopHL1CombatWeapon::CalcViewmodelBob( void )
{
	static  float bob;
	static   float bobtime;
	static   float lastbobtime;
	float   cycle;

	CBasePlayer *player = ToBasePlayer(GetOwner());

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ((!gpGlobals->frametime) || (player == NULL))
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	lastbobtime = gpGlobals->curtime;

	bobtime += gpGlobals->frametime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime / cl_bobcycle.GetFloat()) * cl_bobcycle.GetFloat();
	cycle /= cl_bobcycle.GetFloat();

	if (cycle < cl_bobup.GetFloat())
	{
		cycle = M_PI * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - cl_bobup.GetFloat()) / (1.0 - cl_bobup.GetFloat());
	}

	//Find the speed of the player
	Vector2D vel = player->GetLocalVelocity().AsVector2D();

	bob = sqrt(vel[0] * vel[0] + vel[1] * vel[1]) * cl_bob.GetFloat();
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);
	bob = min(bob, 4.f);
	bob = max(bob, -7.f);

	g_bob = bob;

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
float v_idlescale = 0.0f;
//float aBob = 0.0f;
void CCoopHL1CombatWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	Vector   forward, right, up;
	QAngle oldAngles = angles;
	AngleVectors(angles, &forward, &right, &up);

	CalcViewmodelBob();
	static float time2 = 0.0f;
	time2 += gpGlobals->frametime;

	//angles[ROLL] += v_idlescale * sin(time2*0.5) * 0.1;
	//angles[PITCH] += v_idlescale * sin(time2 * 1) * 0.3;
	//angles[YAW] += v_idlescale * sin(time2 * 2) * 0.3;
	//aBob += g_bob;
	// Apply bob, but scaled down to 40%
	for (int i = 0; i < 3; i++)
	{
		origin[i] += g_bob * 0.4 * forward[i];
	}

	//origin[2] += g_bob;

	// throw in a little tilt.
	angles[YAW] -= g_bob  * 0.5f;
	angles[ROLL] -= g_bob * 1.0f;
	angles[PITCH] -= g_bob * 0.3f;

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	origin[2] -= 1;
}


#else

Vector CCoopHL1CombatWeapon::GetSoundEmissionOrigin() const
{
	if ( gpGlobals->maxClients == 1 || !GetOwner() )
		return CBaseCombatWeapon::GetSoundEmissionOrigin();

//	Vector vecOwner = GetOwner()->GetSoundEmissionOrigin();
//	Vector vecThis = WorldSpaceCenter();
//	DevMsg("SoundEmissionOrigin: Owner: %4.1f,%4.1f,%4.1f Default:%4.1f,%4.1f,%4.1f\n",
//			vecOwner.x, vecOwner.y, vecOwner.z,
//			vecThis.x, vecThis.y, vecThis.z );

	// TEMP fix for HL1MP... sounds are sometimes beeing emitted underneath the ground
	return GetOwner()->GetSoundEmissionOrigin();
}

void CCoopHL1CombatWeapon::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound("BaseCombatWeapon.WeaponDrop");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoopHL1CombatWeapon::FallInit(void)
{
	SetModel(GetWorldModel());
	SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_TRIGGER);
	AddSolidFlags(FSOLID_NOT_SOLID);

	SetPickupTouch();

	SetThink(&CCoopHL1CombatWeapon::FallThink);

	SetNextThink(gpGlobals->curtime + 0.1f);

	// HACKHACK - On ground isn't always set, so look for ground underneath
	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() - Vector(0, 0, 2), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction < 1.0)
	{
		SetGroundEntity(tr.m_pEnt);
	}

	SetViewOffset(Vector(0, 0, 8));
}


//-----------------------------------------------------------------------------
// Purpose: Items that have just spawned run this think to catch them when 
//			they hit the ground. Once we're sure that the object is grounded, 
//			we change its solid type to trigger and set it in a large box that 
//			helps the player get it.
//-----------------------------------------------------------------------------
void CCoopHL1CombatWeapon::FallThink(void)
{
	SetNextThink(gpGlobals->curtime + 0.1f);

	if (GetFlags() & FL_ONGROUND)
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if (GetOwnerEntity())
		{
			EmitSound("BaseCombatWeapon.WeaponDrop");
		}

		// lie flat
		QAngle ang = GetAbsAngles();
		ang.x = 0;
		ang.z = 0;
		SetAbsAngles(ang);

		Materialize();

		SetSize(Vector(-24, -24, 0), Vector(24, 24, 16));
	}
}

#endif