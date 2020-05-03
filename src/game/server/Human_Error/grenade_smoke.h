//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_BUGBAIT_H
#define GRENADE_BUGBAIT_H
#ifdef _WIN32
#pragma once
#endif

#include "smoke_trail.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "Sprite.h"
#include "SpriteTrail.h"

//Radius of the bugbait's effect on other creatures
extern ConVar smoke_grenade_radius;


class CGrenadeSmoke : public CGrenadeFrag
{
	DECLARE_CLASS( CGrenadeSmoke, CGrenadeFrag);
	DECLARE_SERVERCLASS();

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
					
	~CGrenadeSmoke( void );

	virtual void	CreateEffects(void);
	virtual void	BlipSound();
	void	Precache(void);
public:
	void	Detonate();
	void	SmokeThink();

	bool	IsSmoking() { return m_bStartSmoke.Get(); }
private:

	CNetworkVar( bool, m_bStartSmoke );
};

extern CBaseGrenade *SmokeGrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer );

#endif // GRENADE_BUGBAIT_H
