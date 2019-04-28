//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef WEAPON_COOP_BASE_H
#define WEAPON_COOP_BASE_H

#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon_shared.h"
#include "hlss_weapon_id.h"

#ifdef CLIENT_DLL
    #define CWeaponCoopBase C_WeaponCoopBase
    void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip );
#endif

//================================================================================
// La base para un arma cooperativo
//================================================================================
class CWeaponCoopBase : public CBaseCombatWeapon
{
public:
    DECLARE_CLASS( CWeaponCoopBase, CBaseCombatWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
    DECLARE_DATADESC();

    CWeaponCoopBase();

    // Predicción
    virtual bool IsPredicted() const { return true; }

    #ifdef CLIENT_DLL
    virtual void OnDataChanged( DataUpdateType_t type );
    virtual bool ShouldPredict();
    #endif

	virtual int GetWeaponID(void) const;

    //
    virtual void Spawn();
    virtual void WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );

    virtual void PrimaryAttack();

	virtual int GetActivityWeaponRole(void);
	virtual int GetActivityWeaponVariant(void);

	virtual acttable_t *ActivityList(int &iActivityCount);
	// HL2
	static acttable_t s_acttableSMG1[];
	static acttable_t s_acttablePistol[];
	static acttable_t s_acttableMelee[];
	static acttable_t s_acttableCrossbow[];
	static acttable_t s_acttableShotgun[];
	static acttable_t s_acttableGrenade[];
	static acttable_t s_acttableRPG[];
	static acttable_t s_acttablePhysgun[];
	static acttable_t s_acttableSlam[];
	static acttable_t s_acttableMelee2[];
	static acttable_t s_acttablePython[];
	static acttable_t s_acttableAR2[];

	//BMS
	static acttable_t s_acttableMP5[];
	static acttable_t s_acttableGlock[];
	static acttable_t s_acttableTau[];
	static acttable_t s_acttableGluon[];
	static acttable_t s_acttableHiveHand[];
	static acttable_t s_acttableSatchel[];
	static acttable_t s_acttableSnark[];
	static acttable_t s_acttableTripmine[];
};

#endif // WEAPON_COOP_BASE_H