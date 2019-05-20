//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef WEAPON_MATTSPIPE_H
#define WEAPON_MATTSPIPE_H

#include "weapon_coop_basebludgeon.h"

#if defined( _WIN32 )
#pragma once
#endif

#ifdef CLIENT_DLL
#define CWeaponMattsPipe C_WeaponMattsPipe
#endif

#define	CROWBAR_RANGE	75.0f
#define	CROWBAR_REFIRE	0.75f

//-----------------------------------------------------------------------------
// CWeaponMattsPipe
//-----------------------------------------------------------------------------

class CWeaponMattsPipe : public CWeaponCoopBaseBludgeon
{
public:
	DECLARE_CLASS( CWeaponMattsPipe, CWeaponCoopBaseBludgeon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	//DECLARE_ACTTABLE();

	CWeaponMattsPipe();

	float		GetRange( void )		{	return	CROWBAR_RANGE;	}
	float		GetFireRate( void )		{	return	CROWBAR_REFIRE;	}

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack( void )	{	return;	}

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_LEADPIPE; }

	// Animation event
    #ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
    virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
    #endif

private:
    CWeaponMattsPipe( CWeaponMattsPipe & );
};

#endif // WEAPON_MATTSPIPE_H
