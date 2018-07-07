//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		medkit wpn
//
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon_shared.h"
#include "hl2_player_shared.h"
#include "weapon_crowbar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
//-----------------------------------------------------------------------------
// CWeaponMedkit
//-----------------------------------------------------------------------------
class CWeaponMedKit : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponMedKit, CBaseHLCombatWeapon);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
public:
	CWeaponMedKit(void){;}

	void PrimaryAttack();
	//void SecondayAttack();
	
	DECLARE_ACTTABLE();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponMedKit, DT_WeaponMedKit)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_medkit, CWeaponMedKit );
PRECACHE_WEAPON_REGISTER( weapon_medkit );

acttable_t	CWeaponMedKit::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM,				 true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SLAM,				false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SLAM,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SLAM,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SLAM,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM,false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SLAM,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SLAM,				false },

	{ ACT_MP_STAND_IDLE,				ACT_DOD_STAND_IDLE_TNT,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_DOD_CROUCH_IDLE_TNT,				false },
	{ ACT_MP_RUN,						ACT_DOD_WALK_IDLE_TNT,					false },
	{ ACT_MP_CROUCHWALK,				ACT_DOD_CROUCHWALK_IDLE_TNT,			false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_DOD_PRIMARYATTACK_GREN_FRAG,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_DOD_PRIMARYATTACK_GREN_FRAG,		false },
	//{ ACT_MP_RELOAD_STAND,			ACT_DOD_RELOAD_MP40,					false },
	//{ ACT_MP_RELOAD_CROUCH,			ACT_DOD_RELOAD_CROUCH_MP40,				false },
	{ ACT_MP_JUMP,						ACT_MP_JUMP,							false },
};

IMPLEMENT_ACTTABLE(CWeaponMedKit);

void CWeaponMedKit::PrimaryAttack()
{
	CHL2_Player *pPlayer = ToHL2Player(GetOwner());

	if (!pPlayer)
		return;

	Msg("todo: implement attacks for medkit wpn\n");

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
}
