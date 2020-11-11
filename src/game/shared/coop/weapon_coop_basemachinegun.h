//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef WEAPON_COOP_BASEMACHINEGUN_H
#define WEAPON_COOP_BASEMACHINEGUN_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_coop_basehlcombatweapon.h"

#ifdef CLIENT_DLL
    #define CWeaponCoopMachineGun C_WeaponCoopMachineGun
#endif

//================================================================================
// La base para un arma cooperativo
//================================================================================
class CWeaponCoopMachineGun : public CWeaponCoopBaseHLCombat
{
public:
    DECLARE_CLASS( CWeaponCoopMachineGun, CWeaponCoopBaseHLCombat);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();
    DECLARE_DATADESC();

    CWeaponCoopMachineGun() { }

    // Default calls through to m_hOwner, but plasma weapons can override and shoot projectiles here.
	virtual void ItemPostFrame( void );
    virtual void PrimaryAttack(void);
	virtual bool Deploy( void );

    virtual const Vector &GetBulletSpread( void );
    int WeaponSoundRealtime( WeaponSound_t shoot_type );
    void UpdateNPCShotCounter();

    static void DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

protected:
    CNetworkVar(int, m_nShotsFired);	// Number of consecutive shots fired
    float	m_flNextSoundTime;	// real-time clock of when to make next sound

#ifndef CLIENT_DLL
    float m_flTimeLastNPCFired;
#endif // !CLIENT_DLL

private:
    CWeaponCoopMachineGun( const CWeaponCoopMachineGun & );
};

#endif // WEAPON_COOP_BASEMACHINEGUN_H