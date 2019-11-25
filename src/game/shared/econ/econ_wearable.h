//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//========================================================================//

#ifndef ECON_WEARABLE_H
#define ECON_WEARABLE_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

#define MAX_WEARABLES_SENT_FROM_SERVER	5
#define PARTICLE_MODIFY_STRING_SIZE		128

#if defined( CLIENT_DLL )
#define CEconWearable C_EconWearable
#endif


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEconWearable : public CEconEntity
{
	DECLARE_CLASS( CEconWearable, CEconEntity );
	DECLARE_NETWORKCLASS();

public:

	virtual void			Spawn( void );
	virtual bool			IsWearable( void ) { return true; }
	virtual int				GetSkin( void );
	virtual void			SetParticle( const char* name );
	virtual void			UpdateWearableBodyGroups(CBaseCombatCharacter*pPlayer );
	virtual void			GiveTo( CBaseEntity *pEntity );

#ifdef GAME_DLL
	virtual void			Equip( CBaseCombatCharacter *pPlayer );
	virtual void			UnEquip(CBaseCombatCharacter*pPlayer );
#else
	virtual void			OnDataChanged(DataUpdateType_t type);
	virtual	ShadowType_t	ShadowCastType( void );
	virtual bool			ShouldDraw( void );
	virtual int				DrawModel(int flags);
#endif

private:

#ifdef GAME_DLL
	CNetworkString( m_ParticleName, PARTICLE_MODIFY_STRING_SIZE );
#else
	char m_ParticleName[PARTICLE_MODIFY_STRING_SIZE];
	CNewParticleEffect *m_pUnusualParticle;
#endif

};

#endif
