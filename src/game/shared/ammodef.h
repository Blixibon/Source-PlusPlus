//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Holds defintion for game ammo types
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef AI_AMMODEF_H
#define AI_AMMODEF_H

#ifdef _WIN32
#pragma once
#endif

class ConVar;

struct Ammo_t 
{
	char 				*pName;
	int64				nDamageType;
	int					eTracerType;
	float				physicsForceImpulse;
	int					nMinSplashSize;
	int					nMaxSplashSize;

	int					nFlags;

	// Values for player/NPC damage and carrying capability
	// If the integers are set, they override the CVars
	int					pPlrDmg;		// CVar for player damage amount
	int					pNPCDmg;		// CVar for NPC damage amount
	int					pMaxCarry;		// CVar for maximum number can carry
	const ConVar*		pPlrDmgCVar;	// CVar for player damage amount
	const ConVar*		pNPCDmgCVar;	// CVar for NPC damage amount
	const ConVar*		pMaxCarryCVar;	// CVar for maximum number can carry
};

// Used to tell AmmoDef to use the cvars, not the integers
#define		USE_CVAR		-1
// Ammo is infinite
#define		INFINITE_AMMO	-2

enum AmmoTracer_t
{
	TRACER_NONE,
	TRACER_LINE,
	TRACER_RAIL,
	TRACER_BEAM,
	TRACER_LINE_AND_WHIZ,
};

enum AmmoFlags_t
{
	AMMO_FORCE_DROP_IF_CARRIED = 0x1,
	AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER = 0x2,
	AMMO_DAMAGE_BASED_ON_VICTIM = 0x4,
};


#include "shareddefs.h"

//=============================================================================
//	>> CAmmoDef
//=============================================================================
class CAmmoDef
{

public:
	int					m_nAmmoIndex;

	Ammo_t				m_AmmoType[MAX_AMMO_TYPES];

	virtual Ammo_t				*GetAmmoOfIndex(int nAmmoIndex);
	virtual int					Index(const char *psz);
	virtual int					PlrDamage(int nAmmoIndex);
	virtual int					NPCDamage(int nAmmoIndex);
	virtual int					MaxCarry(int nAmmoIndex);
	virtual int64				DamageType(int nAmmoIndex);
	virtual int					TracerType(int nAmmoIndex);
	virtual float				DamageForce(int nAmmoIndex);
	virtual int					MinSplashSize(int nAmmoIndex);
	virtual int					MaxSplashSize(int nAmmoIndex);
	virtual int					Flags(int nAmmoIndex);

	virtual void				AddAmmoType(char const* name, int64 damageType, int tracerType, int plr_dmg, int npc_dmg, int carry, float physicsForceImpulse, int nFlags, int minSplashSize = 4, int maxSplashSize = 8 );
	virtual void				AddAmmoType(char const* name, int64 damageType, int tracerType, char const* plr_cvar, char const* npc_var, char const* carry_cvar, float physicsForceImpulse, int nFlags, int minSplashSize = 4, int maxSplashSize = 8 );

	CAmmoDef(void);
	virtual ~CAmmoDef( void );

protected:
	virtual bool		AddAmmoType(char const* name, int64 damageType, int tracerType, int nFlags, int minSplashSize, int maxSplashSize );
};


// Get the global ammodef object. This is usually implemented in each mod's game rules file somewhere,
// so the mod can setup custom ammo types.
CAmmoDef* GetAmmoDef();


#endif // AI_AMMODEF_H
 
