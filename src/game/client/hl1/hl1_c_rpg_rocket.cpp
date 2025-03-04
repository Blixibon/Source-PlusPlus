//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "dlight.h"
#include "tempent.h"
#include "iefx.h"
#include "c_te_legacytempents.h"
#include "basegrenade_shared.h"
#include "particle_property.h"


class C_RpgRocket : public C_BaseGrenade
{
public:
	DECLARE_CLASS( C_RpgRocket, C_BaseGrenade );
	DECLARE_CLIENTCLASS();

public:
	C_RpgRocket( void ) { }
	C_RpgRocket( const C_RpgRocket & );

public:
	void	CreateLightEffects( void );
	CNewParticleEffect* m_pRocketFlare;
};


IMPLEMENT_CLIENTCLASS_DT( C_RpgRocket, DT_RpgRocket, CRpgRocket )
END_RECV_TABLE()


void C_RpgRocket::CreateLightEffects( void )
{
	dlight_t *dl;
	if ( IsEffectActive(EF_DIMLIGHT) )
	{			
		dl = effects->CL_AllocDlight ( index );
		dl->origin = GetAbsOrigin();
		dl->color.r = dl->color.g = dl->color.b = 100;
		dl->radius = 200;
		dl->die = gpGlobals->curtime + 0.001;

		if (!m_pRocketFlare)
			m_pRocketFlare = ParticleProp()->Create("rpg_firetrail", PATTACH_ABSORIGIN_FOLLOW);
	}
	else if (m_pRocketFlare)
	{
		ParticleProp()->StopEmission(m_pRocketFlare);
		m_pRocketFlare = nullptr;
	}
}
