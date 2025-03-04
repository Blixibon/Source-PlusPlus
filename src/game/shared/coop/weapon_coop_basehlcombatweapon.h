//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef WEAPON_COOP_BASEHLCOMBATWEAPON_H
#define WEAPON_COOP_BASEHLCOMBATWEAPON_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_coop_base.h"

#ifdef CLIENT_DLL
    #define CWeaponCoopBaseHLCombat C_WeaponCoopBaseHLCombat
#endif

//================================================================================
// La base para un arma cooperativo
//================================================================================
class CWeaponCoopBaseHLCombat : public CWeaponCoopBase
{
public:
    DECLARE_CLASS( CWeaponCoopBaseHLCombat, CWeaponCoopBase );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
    DECLARE_DATADESC();
    
    CWeaponCoopBaseHLCombat() { }

    virtual bool WeaponShouldBeLowered( void );

	virtual bool CanLower(void);
	virtual bool Ready( void );
	virtual bool Lower( void );
	virtual bool Deploy( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void WeaponIdle( void );

	virtual void AddViewmodelBob(CBaseViewModel* viewmodel, Vector& origin, QAngle& angles);

    virtual void ItemHolsterFrame( void );

	virtual Vector GetBulletSpread( WeaponProficiency_t proficiency );
	virtual float GetSpreadBias( WeaponProficiency_t proficiency );

	virtual const WeaponProficiencyInfo_t *GetProficiencyValues();
	static const WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

	virtual Activity		ActivityOverride(Activity baseAct, bool *pRequired);
	static acttable_t s_acttableLowered[];
	static acttable_t s_acttableAimDownSights[];

	virtual Activity		GetSprintActivity() 
	{
		return GetWpnData().bHasActivity[VM_ACTIVITY_SPRINT] ? (Activity)GetWpnData().iScriptedVMActivities[VM_ACTIVITY_SPRINT] : ACT_INVALID;
	}

	bool CanSprint();
	bool HasFidget();

protected:
    CNetworkVar(bool, m_bLowered);			// Whether the viewmodel is raised or lowered
	CNetworkVar(float, m_flRaiseTime);		// If lowered, the time we should raise the viewmodel
	CNetworkVar(float, m_flHolsterTime);	// When the weapon was holstered
	CNetworkVar(float, m_flTimeLastAction);

	void NotifyWeaponAction();

private:
    CWeaponCoopBaseHLCombat( const CWeaponCoopBaseHLCombat & );
};

#endif // WEAPON_COOP_BASEHLCOMBATWEAPON_H