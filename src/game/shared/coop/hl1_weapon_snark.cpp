//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Snark
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "NPCEvent.h"
#include "hl1_basecombatweapon_shared.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "beam_shared.h"
#ifndef CLIENT_DLL
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "soundent.h"
#include "game.h"
#include "hl1_npc_snark.h"
#else
#define CWeaponSnark C_WeaponSnark
#endif


//-----------------------------------------------------------------------------
// CWeaponSnark
//-----------------------------------------------------------------------------


#define SNARK_NEST_MODEL	"models/w_sqknest.mdl"


class CWeaponSnark : public CCoopHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponSnark, CCoopHL1CombatWeapon);
public:

	CWeaponSnark( void );

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_SNARK; }

	void	Precache( void );
	void	PrimaryAttack( void );
	void	WeaponIdle( void );
	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

private:
	CNetworkVar(bool, m_bJustThrown);
};

LINK_ENTITY_TO_CLASS( weapon_snark, CWeaponSnark );

PRECACHE_WEAPON_REGISTER( weapon_snark );

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSnark, DT_WeaponSnark);

BEGIN_NETWORK_TABLE( CWeaponSnark, DT_WeaponSnark )
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bJustThrown)),
#else
SendPropBool(SENDINFO(m_bJustThrown)),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CWeaponSnark )
	DEFINE_FIELD( m_bJustThrown, FIELD_BOOLEAN ),
END_DATADESC()

BEGIN_PREDICTION_DATA(CWeaponSnark)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_bJustThrown, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE)
#endif
END_PREDICTION_DATA()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSnark::CWeaponSnark( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= true;
	m_bJustThrown		= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSnark::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "WpnSnark.PrimaryAttack" );
	PrecacheScriptSound( "WpnSnark.Deploy" );

	UTIL_PrecacheOther("monster_snark");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSnark::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	Vector vecForward;
	pPlayer->EyeVectors( &vecForward );

	// find place to toss monster
	// Does this need to consider a crouched player?
	Vector vecStart	= pPlayer->WorldSpaceCenter() + (vecForward * 20);
	Vector vecEnd	= vecStart + (vecForward * 44);
	trace_t tr;
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.allsolid || tr.startsolid || tr.fraction <= 0.25 )
		return;

	// player "shoot" animation
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL
	CSnark *pSnark = (CSnark*)Create( "monster_snark", tr.endpos, pPlayer->EyeAngles(), GetOwner() );
	if ( pSnark )
	{
		pSnark->SetAbsVelocity( vecForward * 200 + pPlayer->GetAbsVelocity() );
	}

	CSoundEnt::InsertSound(SOUND_DANGER, GetAbsOrigin(), 200, 0.2);
#endif
	// play hunt sound
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "WpnSnark.PrimaryAttack" );


	pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

	m_bJustThrown = true;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
	SetWeaponIdleTime( gpGlobals->curtime + 1.0 );
}

void CWeaponSnark::WeaponIdle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( !HasWeaponIdleTimeElapsed() )
		return;

	if ( m_bJustThrown )
	{
		m_bJustThrown = false;

		if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		{
			if ( !pPlayer->SwitchToNextBestWeapon( pPlayer->GetActiveWeapon() ) )
				Holster();
		}
		else
		{
			SendWeaponAnim( ACT_VM_DRAW );
			SetWeaponIdleTime( gpGlobals->curtime + random->RandomFloat( 10, 15 ) );
		}
	}
	else
	{
		if ( random->RandomFloat( 0, 1 ) <= 0.75 )
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else
		{
			SendWeaponAnim( ACT_VM_FIDGET );
		}
	}
}

bool CWeaponSnark::Deploy( void )
{
	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "WpnSnark.Deploy" );

	return BaseClass::Deploy();
}

bool CWeaponSnark::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return false;
	}

	if ( !BaseClass::Holster( pSwitchingTo ) )
	{
		return false;
	}
#ifndef CLIENT_DLL
	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		SetThink( &CWeaponSnark::DestroyItem );
		SetNextThink( gpGlobals->curtime + 0.1 );
	}
#endif
	pPlayer->SetNextAttack( gpGlobals->curtime + 0.5 );

	return true;
}
