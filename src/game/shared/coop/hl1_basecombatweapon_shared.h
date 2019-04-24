//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "weapon_coop_base.h"

#ifndef BASEHLCOMBATWEAPON_SHARED_H
#define BASEHLCOMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
#define CCoopHL1CombatWeapon C_CoopHL1CombatWeapon
#endif

class CCoopHL1CombatWeapon : public CWeaponCoopBase
{
	DECLARE_CLASS(CCoopHL1CombatWeapon, CWeaponCoopBase);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC()
public:
	void Spawn( void );

	void EjectShell(CBaseEntity* pPlayer, int iType);

	//All weapons can be picked up by NPCs by default
	virtual bool			CanBePickedUpByNPCs(void) { return false; }

	CBasePlayer* GetPlayerOwner() const;
public:
// Server Only Methods
#if !defined( CLIENT_DLL )
	virtual void Precache();

	void FallInit( void );						// prepare to fall to the ground
	void FallThink( void );						// make the weapon fall to the ground after spawning

	Vector GetSoundEmissionOrigin() const;
#else

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );

#endif
};

#endif // BASEHLCOMBATWEAPON_SHARED_H
