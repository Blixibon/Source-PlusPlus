//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "gamestats.h"
#include "weapon_coop_basehlcombatweapon.h"


#ifdef CLIENT_DLL
    #define CWeaponDeagle C_WeaponDeagle

    #include "c_te_effect_dispatch.h"
#else
    #include "basehlcombatweapon.h"
    #include "basecombatcharacter.h"
    #include "ai_basenpc.h"
    #include "player.h"
    #include "soundent.h"
    #include "game.h"
    #include "te_effect_dispatch.h"
    #include "ilagcompensationmanager.h"
#endif

#include "hl2_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponDeagle
//-----------------------------------------------------------------------------

class CWeaponDeagle : public CWeaponCoopBaseHLCombat
{
	DECLARE_CLASS( CWeaponDeagle, CWeaponCoopBaseHLCombat );
public:

	CWeaponDeagle( void );

	void	PrimaryAttack( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	float	WeaponAutoAimScale()	{ return 0.6f; }

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_DEAGLE; }

	DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
   // DECLARE_ACTTABLE();
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CWeaponDeagle );
PRECACHE_WEAPON_REGISTER( weapon_deagle );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDeagle, DT_WeaponDeagle )

BEGIN_NETWORK_TABLE( CWeaponDeagle, DT_WeaponDeagle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDeagle )
END_PREDICTION_DATA()

BEGIN_DATADESC( CWeaponDeagle )
END_DATADESC()

//acttable_t CWeaponDeagle::m_acttable[] = 
//{
//	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
//	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
//	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
//	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
//	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
//	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
//	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
//	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
//};

//IMPLEMENT_ACTTABLE( CWeaponDeagle );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDeagle::CWeaponDeagle( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				CEffectData data;

				// Emit six spent shells
				for ( int i = 0; i < 6; i++ )
				{
					data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
					data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );

                    #ifndef CLIENT_DLL
					data.m_nEntIndex = entindex();
                    #else
                    data.m_hEntity = this;
                    #endif

					DispatchEffect( "ShellEject", data );
				}

				break;
			}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

#ifndef CLIENT_DLL
	//m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	CHL2_Player* pHL2 = ToHL2Player(pPlayer);
	if (pHL2)
		pHL2->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);


#ifndef CLIENT_DLL
    pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

    //Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

	pPlayer->SnapEyeAngles( angles );
#endif

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );
    
#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );
#endif

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}
