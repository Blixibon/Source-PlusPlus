//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Fizzle effects for portal.
//
//=============================================================================//

#include "cbase.h"
#include "ClientEffectPrecacheSystem.h"
#include "fx.h"
#include "FX_Sparks.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#include "particles_ez.h"
#include "decals.h"
#include "engine/IEngineSound.h"
#include "fx_quad.h"
#include "engine/IVDebugOverlay.h"
#include "shareddefs.h"
#include "portal_shareddefs.h"
#include "effect_color_tables.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class C_PortalBlast : public C_BaseEntity
{
	DECLARE_CLASS( C_PortalBlast, C_BaseAnimating );

public:

	static void		Create( bool bIsPortal2, PortalPlacedByType ePlacedBy, const Vector &vStart, const Vector &vEnd, const QAngle &qAngles, float fDeathTime, int iLinkage);

	void			Init( bool bIsPortal2, PortalPlacedByType ePlacedBy, const Vector &vStart, const Vector &vEnd, const QAngle &qAngles, float fDeathTime, int iLinkage );

	virtual void	ClientThink( void );

private:

	Vector	m_ptCreationPoint;
	Vector	m_ptDeathPoint;
	Vector	m_ptAimPoint;

	float	m_fCreationTime;
	float	m_fDeathTime;
};


void C_PortalBlast::Create( bool bIsPortal2, PortalPlacedByType ePlacedBy, const Vector &vStart, const Vector &vEnd, const QAngle &qAngles, float fDeathTime, int iLinkage)
{
	C_PortalBlast *pPortalBlast = new C_PortalBlast;
	pPortalBlast->Init( bIsPortal2, ePlacedBy, vStart, vEnd, qAngles, fDeathTime, iLinkage );
}


void C_PortalBlast::Init( bool bIsPortal2, PortalPlacedByType ePlacedBy, const Vector &vStart, const Vector &vEnd, const QAngle &qAngles, float fDeathTime, int iLinkage)
{
	ClientEntityList().AddNonNetworkableEntity( this );
	ClientThinkList()->SetNextClientThink( GetClientHandle(), CLIENT_THINK_ALWAYS );

	AddToLeafSystem( RENDER_GROUP_OPAQUE_ENTITY );

	SetThink( &C_PortalBlast::ClientThink );
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	m_ptCreationPoint = vStart;
	m_ptDeathPoint = vEnd;

	SetAbsOrigin( m_ptCreationPoint );

	m_fCreationTime = gpGlobals->curtime;
	m_fDeathTime = fDeathTime;

	if ( m_fDeathTime - 0.1f < m_fCreationTime )
	{
		m_fDeathTime = m_fCreationTime + 0.1f;
	}

	Vector vForward;
	AngleVectors( qAngles, &vForward );

	m_ptAimPoint = m_ptCreationPoint + vForward * m_ptCreationPoint.DistTo( m_ptDeathPoint );

	if (iLinkage > 0)
	{
		CNewParticleEffect* pEffect = nullptr;
		if (ePlacedBy == PORTAL_PLACED_BY_PLAYER)
			pEffect = ParticleProp()->Create("portal_x_projectile_stream", PATTACH_ABSORIGIN_FOLLOW);
		else
			pEffect = ParticleProp()->Create("portal_x_projectile_stream_pedestal", PATTACH_ABSORIGIN_FOLLOW);

		Vector vecRet;
		Color clrPortal = UTIL_Portal_Color(bIsPortal2 ? 2 : 1, iLinkage);
		vecRet.x = clrPortal.r() / 255.f;
		vecRet.y = clrPortal.g() / 255.f;
		vecRet.z = clrPortal.b() / 255.f;
		pEffect->SetControlPoint(1, vecRet);
	}
	else
	{
		if (ePlacedBy == PORTAL_PLACED_BY_PLAYER)
			ParticleProp()->Create(((bIsPortal2) ? ("portal_2_projectile_stream") : ("portal_1_projectile_stream")), PATTACH_ABSORIGIN_FOLLOW);
		else
			ParticleProp()->Create(((bIsPortal2) ? ("portal_2_projectile_stream_pedestal") : ("portal_1_projectile_stream_pedestal")), PATTACH_ABSORIGIN_FOLLOW);
	}
}

void C_PortalBlast::ClientThink( void )
{
	if ( m_fCreationTime == 0.0f && m_fDeathTime == 0.0f )
	{
		// Die!
		Remove();
		return;
	}

	float fT = ( gpGlobals->curtime - m_fCreationTime ) / ( m_fDeathTime - m_fCreationTime );

	if ( fT >= 1.0f )
	{
		// Ready to die! But we want one more frame in the final position
		SetAbsOrigin( m_ptDeathPoint );

		m_fCreationTime = 0.0f;
		m_fDeathTime = 0.0f;

		return;
	}

	// Set the interpolated position
	Vector vTarget = m_ptAimPoint * ( 1.0f - fT ) + m_ptDeathPoint * fT;
	SetAbsOrigin( m_ptCreationPoint * ( 1.0f - fT ) + vTarget * fT );
}


void PortalBlastCallback( const CEffectData & data )
{
	C_PortalBlast::Create( ( data.m_nColor == 1 ) ? ( false ) : ( true ), (PortalPlacedByType)data.m_nDamageType, data.m_vOrigin, data.m_vStart, data.m_vAngles, data.m_flScale, data.m_nHitBox );
}

DECLARE_CLIENT_EFFECT( "PortalBlast", PortalBlastCallback );