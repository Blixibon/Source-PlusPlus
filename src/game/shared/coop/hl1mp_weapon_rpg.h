//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: RPG
//
//=============================================================================//


#ifndef WEAPON_HL1_RPG_H
#define WEAPON_HL1_RPG_H
#ifdef _WIN32
#pragma once
#endif


#include "hl1mp_basecombatweapon_shared.h"
//#include "sprite.h"
//#include "NPCEvent.h"
//#include "smoke_trail.h"
//#include "hl1_basegrenade.h"

#ifdef CLIENT_DLL
#include "iviewrender_beams.h"
#include "c_smoke_trail.h"
#endif

#ifndef CLIENT_DLL
#include "smoke_trail.h"
#include "sprite.h"
#include "NPCEvent.h"
#include "beam_shared.h"
#include "hl1_basegrenade.h"

class CHL1WeaponRPG;

//###########################################################################
//	CRpgRocket
//###########################################################################
class CRpgRocket : public CHL1BaseGrenade
{
	DECLARE_CLASS( CRpgRocket, CHL1BaseGrenade );
	DECLARE_SERVERCLASS();

public:
	CRpgRocket();

	Class_T Classify( void ) { return CLASS_NONE; }
	
	void Spawn( void );
	void Precache( void );
	void RocketTouch( CBaseEntity *pOther );
	void IgniteThink( void );
	void SeekThink( void );

	virtual void Detonate( void );

	static CRpgRocket *Create( const Vector &vecOrigin, const QAngle &angAngles, CBasePlayer *pentOwner = NULL );

	CHandle<CHL1WeaponRPG>		m_hOwner;
	float					m_flIgniteTime;
	int						m_iTrail;
	
	DECLARE_DATADESC();
};


#endif

#ifdef CLIENT_DLL
#define CLaserDot C_LaserDot
#endif

class CLaserDot;

#ifdef CLIENT_DLL
#define CHL1WeaponRPG C_HL1WeaponRPG
#endif

//-----------------------------------------------------------------------------
// CHL1WeaponRPG
//-----------------------------------------------------------------------------
class CHL1WeaponRPG : public CCoopHL1CombatWeapon
{
	DECLARE_CLASS( CHL1WeaponRPG, CCoopHL1CombatWeapon);
public:

	CHL1WeaponRPG( void );
	~CHL1WeaponRPG();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_RPG; }

	void	ItemPostFrame( void );
	void	Precache( void );
	bool	Deploy( void );
	void	PrimaryAttack( void );
	void	WeaponIdle( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void	NotifyRocketDied( void );
	bool	Reload( void );

	void	Drop( const Vector &vecVelocity );

	virtual int	GetDefaultClip1( void ) const;

//	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

private:
	void	CreateLaserPointer( void );
	void	UpdateSpot( void );
	bool	IsGuiding( void );
	void	StartGuiding( void );
	void	StopGuiding( void );

#ifndef CLIENT_DLL
//	DECLARE_ACTTABLE();
#endif

private:
//	bool				m_bGuiding;
//	CHandle<CLaserDot>	m_hLaserDot;
//	CHandle<CRpgRocket>	m_hMissile;
//	bool				m_bIntialStateUpdate;
//	bool				m_bLaserDotSuspended;
//	float				m_flLaserDotReviveTime;

	CNetworkVar( bool, m_bGuiding );
	CNetworkVar( bool, m_bIntialStateUpdate );
	CNetworkVar( bool, m_bLaserDotSuspended );
	CNetworkVar( float, m_flLaserDotReviveTime );

	CNetworkHandle( CBaseEntity, m_hMissile );

#ifndef CLIENT_DLL
	EHANDLE	m_hLaserDot;
#endif
};


#endif	// WEAPON_RPG_H
