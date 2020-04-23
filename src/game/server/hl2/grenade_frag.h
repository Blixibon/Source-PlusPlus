//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_FRAG_H
#define GRENADE_FRAG_H
#pragma once

class CBaseGrenade;
class CSprite;
class CSpriteTrail;
struct edict_t;

#include "player_pickup.h"
#include "basegrenade_shared.h"

class CGrenadeFrag : public CBaseGrenade
{
	DECLARE_CLASS(CGrenadeFrag, CBaseGrenade);

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
protected:
	~CGrenadeFrag(void);

public:
	void	Spawn(void);
	void	OnRestore(void);
	void	Precache(void);
	bool	CreateVPhysics(void);
	virtual void	CreateEffects(void);
	void	SetTimer(float detonateDelay, float warnDelay);
	void	SetVelocity(const Vector& velocity, const AngularImpulse& angVelocity);
	int		OnTakeDamage(const CTakeDamageInfo& inputInfo);
	virtual void	BlipSound();
	void	DelayThink();
	void	VPhysicsUpdate(IPhysicsObject* pPhysics);
	void	OnPhysGunPickup(CBasePlayer* pPhysGunUser, PhysGunPickup_t reason);
	void	SetCombineSpawned(bool combineSpawned) { m_combineSpawned = combineSpawned; }
	bool	IsCombineSpawned(void) const { return m_combineSpawned; }
	void	SetPunted(bool punt) { m_punted = punt; }
	bool	WasPunted(void) const { return m_punted; }

	virtual const char* GetGrenadeModel();

	// this function only used in episodic.
#if defined(HL2_EPISODIC) && 0 // FIXME: HandleInteraction() is no longer called now that base grenade derives from CBaseAnimating
	bool	HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt);
#endif 

	void	InputSetTimer(inputdata_t& inputdata);

protected:
	CHandle<CSprite>		m_pMainGlow;
	CHandle<CSpriteTrail>	m_pGlowTrail;

	float	m_flNextBlipTime;
	bool	m_inSolid;
	bool	m_combineSpawned;
	bool	m_punted;
};

CBaseGrenade *Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned, const char *pszClassname = "npc_grenade_frag" );
bool	Fraggrenade_WasPunted( const CBaseEntity *pEntity );
bool	Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity );

#endif // GRENADE_FRAG_H
