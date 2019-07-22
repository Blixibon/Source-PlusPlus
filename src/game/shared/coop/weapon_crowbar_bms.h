//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_CROWBAR_H
#define WEAPON_CROWBAR_H

#include "weapon_coop_basebludgeon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef CLIENT_DLL
#define CBMSWeaponCrowbar C_WeaponCrowbar
#endif

#define	CROWBAR_RANGE	75.0f
#define	CROWBAR_REFIRE	0.25f

//-----------------------------------------------------------------------------
// CBMSWeaponCrowbar
//-----------------------------------------------------------------------------

class CBMSWeaponCrowbar : public CWeaponCoopBaseBludgeon
{
public:
	DECLARE_CLASS( CBMSWeaponCrowbar, CWeaponCoopBaseBludgeon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	//DECLARE_ACTTABLE();

	CBMSWeaponCrowbar();

	float		GetRange( void )		{	return	CROWBAR_RANGE;	}
	float		GetFireRate( void )		{	return	CROWBAR_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack( void )	{	return;	}

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_CROWBAR_BMS; }

	// Animation event
    #ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
    virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
    #endif

private:
    CBMSWeaponCrowbar( CBMSWeaponCrowbar & );
};

#endif // WEAPON_CROWBAR_H
