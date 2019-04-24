//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the MP5 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONMP5_H
#define	WEAPONMP5_H

#ifdef CLIENT_DLL
#else
#include "hl1_basegrenade.h"
#endif
#include "hl1mp_basecombatweapon_shared.h"

#ifdef CLIENT_DLL
class CGrenadeMP5;
#else
#endif

#ifdef CLIENT_DLL
#define CWeaponMP5 C_WeaponMP5
#endif

class CWeaponMP5 : public CCoopHL1CombatWeapon
{
	DECLARE_CLASS( CWeaponMP5, CCoopHL1CombatWeapon);
public:

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponMP5();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_SMG1; }

	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	DryFire( void );
	void	WeaponIdle( void );

//	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};


#endif	//WEAPONMP5_H
