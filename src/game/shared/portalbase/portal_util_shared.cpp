//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "portal_util_shared.h"
#include "prop_portal_shared.h"
#include "portal_shareddefs.h"
#include "portal_collideable_enumerator.h"
#include "beam_shared.h"
#include "CollisionUtils.h"
#include "util_shared.h"
#include "portal_placement.h"
#include "debugoverlay_shared.h"
#ifndef CLIENT_DLL
	#include "Util.h"
	#include "NDebugOverlay.h"
	#include "env_debughistory.h"
#else
	#include "c_portal_player.h"
#endif
#ifdef HL2_LAZUL
#ifndef CLIENT_DLL
#include "peter/laz_player_resource.h"
#else
#include "peter/c_laz_player_resource.h"
#endif
#endif // HL2_LAZUL

#include "PortalSimulation.h"

#ifdef CLIENT_DLL
#define PORTAL_TRACE_PARTITIONS PARTITION_CLIENT_SOLID_EDICTS
#else
#define PORTAL_TRACE_PARTITIONS PARTITION_ENGINE_SOLID_EDICTS
#endif // CLIENT_DLL

bool TraceBoxAA(const Ray_t& ray, const CPhysCollide* pCollide, trace_t* ptr)
{
	physcollision->TraceBox(ray, pCollide, vec3_origin, vec3_angle, ptr);
	return ptr->DidHit();
}

bool g_bAllowForcePortalTrace = false;
bool g_bForcePortalTrace = false;
bool g_bBulletPortalTrace = false;

ConVar sv_portal_trace_vs_world ("sv_portal_trace_vs_world", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Use traces against portal environment world geometry" );
ConVar sv_portal_trace_vs_displacements ("sv_portal_trace_vs_displacements", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Use traces against portal environment displacement geometry" );
ConVar sv_portal_trace_vs_holywall ("sv_portal_trace_vs_holywall", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Use traces against portal environment carved wall" );
ConVar sv_portal_trace_vs_staticprops ("sv_portal_trace_vs_staticprops", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Use traces against portal environment static prop geometry" );
ConVar sv_use_find_closest_passable_space ("sv_use_find_closest_passable_space", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Enables heavy-handed player teleporting stuck fix code." );
ConVar sv_use_transformed_collideables("sv_use_transformed_collideables", "1", FCVAR_REPLICATED | FCVAR_CHEAT, "Disables traces against remote portal moving entities using transforms to bring them into local space." );

ConVar portal_trace_shrink_ray_each_query("portal_trace_shrink_ray_each_query", "0", FCVAR_REPLICATED | FCVAR_CHEAT);
ConVar portal_beamtrace_optimization("portal_beamtrace_optimization", "1", FCVAR_REPLICATED | FCVAR_CHEAT);

extern ConVar portal_clone_displacements;

class CTransformedCollideable : public ICollideable //wraps an existing collideable, but transforms everything that pertains to world space by another transform
{
public:
	VMatrix m_matTransform; //the transformation we apply to the wrapped collideable
	VMatrix m_matInvTransform; //cached inverse of m_matTransform

	ICollideable *m_pWrappedCollideable; //the collideable we're transforming without it knowing

	struct CTC_ReferenceVars_t
	{
		Vector m_vCollisionOrigin;
		QAngle m_qCollisionAngles;
		matrix3x4_t m_matCollisionToWorldTransform;
		matrix3x4_t m_matRootParentToWorldTransform;
	}; 

	mutable CTC_ReferenceVars_t m_ReferencedVars; //when returning a const reference, it needs to point to something, so here we go

	//abstract functions which require no transforms, just pass them along to the wrapped collideable
	virtual IHandleEntity	*GetEntityHandle() { return m_pWrappedCollideable->GetEntityHandle(); }
	virtual const Vector&	OBBMins() const { return m_pWrappedCollideable->OBBMins(); };
	virtual const Vector&	OBBMaxs() const { return m_pWrappedCollideable->OBBMaxs(); };
	virtual const Vector&	OBBMinsPreScaled() const { return m_pWrappedCollideable->OBBMinsPreScaled(); };
	virtual const Vector&	OBBMaxsPreScaled() const { return m_pWrappedCollideable->OBBMaxsPreScaled(); };
	virtual int				GetCollisionModelIndex() { return m_pWrappedCollideable->GetCollisionModelIndex(); };
	virtual const model_t*	GetCollisionModel() { return m_pWrappedCollideable->GetCollisionModel(); };
	virtual SolidType_t		GetSolid() const { return m_pWrappedCollideable->GetSolid(); };
	virtual int				GetSolidFlags() const { return m_pWrappedCollideable->GetSolidFlags(); };
	virtual IClientUnknown*	GetIClientUnknown() { return m_pWrappedCollideable->GetIClientUnknown(); };
	virtual int				GetCollisionGroup() const { return m_pWrappedCollideable->GetCollisionGroup(); };
	virtual bool			ShouldTouchTrigger( int triggerSolidFlags ) const { return m_pWrappedCollideable->ShouldTouchTrigger(triggerSolidFlags); };

	//slightly trickier functions
	virtual void			WorldSpaceTriggerBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs ) const;
	virtual bool			TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual const Vector&	GetCollisionOrigin() const;
	virtual const QAngle&	GetCollisionAngles() const;
	virtual const matrix3x4_t&	CollisionToWorldTransform() const;
	virtual void			WorldSpaceSurroundingBounds( Vector *pVecMins, Vector *pVecMaxs );
	virtual const matrix3x4_t	*GetRootParentToWorldTransform() const;
};

void CTransformedCollideable::WorldSpaceTriggerBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs ) const
{
	m_pWrappedCollideable->WorldSpaceTriggerBounds( pVecWorldMins, pVecWorldMaxs );
	
	if( pVecWorldMins )
		*pVecWorldMins = m_matTransform * (*pVecWorldMins);

	if( pVecWorldMaxs )
		*pVecWorldMaxs = m_matTransform * (*pVecWorldMaxs);
}

bool CTransformedCollideable::TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	//TODO: Transform the ray by inverse matTransform and transform the trace results by matTransform? AABB Errors arise by transforming the ray.
    return m_pWrappedCollideable->TestCollision( ray, fContentsMask, tr );
}

bool CTransformedCollideable::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	//TODO: Transform the ray by inverse matTransform and transform the trace results by matTransform? AABB Errors arise by transforming the ray.
	return m_pWrappedCollideable->TestHitboxes( ray, fContentsMask, tr );
}

const Vector& CTransformedCollideable::GetCollisionOrigin() const
{
	m_ReferencedVars.m_vCollisionOrigin = m_matTransform * m_pWrappedCollideable->GetCollisionOrigin();
	return m_ReferencedVars.m_vCollisionOrigin;
}

const QAngle& CTransformedCollideable::GetCollisionAngles() const
{
	m_ReferencedVars.m_qCollisionAngles = TransformAnglesToWorldSpace( m_pWrappedCollideable->GetCollisionAngles(), m_matTransform.As3x4() );
	return m_ReferencedVars.m_qCollisionAngles;
}

const matrix3x4_t& CTransformedCollideable::CollisionToWorldTransform() const
{
	//1-2 order correct?
	ConcatTransforms( m_matTransform.As3x4(), m_pWrappedCollideable->CollisionToWorldTransform(), m_ReferencedVars.m_matCollisionToWorldTransform );
	return m_ReferencedVars.m_matCollisionToWorldTransform;
}

void CTransformedCollideable::WorldSpaceSurroundingBounds( Vector *pVecMins, Vector *pVecMaxs )
{
	if( (pVecMins == NULL) && (pVecMaxs == NULL) )
		return;

	Vector vMins, vMaxs;
	m_pWrappedCollideable->WorldSpaceSurroundingBounds( &vMins, &vMaxs );

	TransformAABB( m_matTransform.As3x4(), vMins, vMaxs, vMins, vMaxs );

	if( pVecMins )
		*pVecMins = vMins;
	if( pVecMaxs )
		*pVecMaxs = vMaxs;
}

const matrix3x4_t* CTransformedCollideable::GetRootParentToWorldTransform() const
{
	const matrix3x4_t *pWrappedVersion = m_pWrappedCollideable->GetRootParentToWorldTransform();
	if( pWrappedVersion == NULL )
		return NULL;

	ConcatTransforms( m_matTransform.As3x4(), *pWrappedVersion, m_ReferencedVars.m_matRootParentToWorldTransform );
	return &m_ReferencedVars.m_matRootParentToWorldTransform;
}

Color UTIL_Portal_Color( int iPortal, int iLinkageID )
{
	if (iPortal == 0)
	{
		// GRAVITY BEAM
		return Color(242, 202, 167, 255);
	}
	else
	{
		if (gpGlobals->maxClients < 3)
		{
			switch (iLinkageID)
			{
			case 0:
			default:
				if (iPortal == 2)
				{
					// PORTAL 2
					return Color(255, 160, 32, 255);
				}
				else
				{
					// PORTAL 1
					return Color(64, 160, 255, 255);
				}
				break;
			case 1:
				if (iPortal == 2)
				{
					// PORTAL 2
					return Color(0.075 * 255, 0, 0.824 * 255, 255);
				}
				else
				{
					// PORTAL 1
					return Color(0.125 * 255, 0.500 * 255, 0.824 * 255, 255);
				}
				break;
			case 2:
				if (iPortal == 2)
				{
					// PORTAL 2
					return Color(0.225 * 255, 0.010 * 255, 0.010 * 255, 255);
				}
				else
				{
					// PORTAL 1
					return Color(255, 0.705 * 255, 0.125 * 255, 255);
				}
				break;
			}
		}
		else if (iLinkageID == 0)
		{
			if (iPortal == 2)
			{
				// PORTAL 2
				return Color(255, 160, 32, 255);
			}
			else
			{
				// PORTAL 1
				return Color(64, 160, 255, 255);
			}
		}
		else
		{
#ifdef HL2_LAZUL
#ifndef CLIENT_DLL
			CLAZPlayerResource* pLazRes = (CLAZPlayerResource*)g_pPlayerResource;
#else
			C_LAZ_PlayerResource* pLazRes = (C_LAZ_PlayerResource*)g_PR;
#endif // !CLIENT_DLL
			if (pLazRes)
				return pLazRes->GetPlayerColor(iLinkageID, (iPortal == 2) ? PLRCOLOR_PORTAL2 : PLRCOLOR_PORTAL1);
#endif // HL2_LAZUL
		}
	}

	return Color( 255, 255, 255, 255 );
}

void UTIL_Portal_Trace_Filter( CTraceFilterSimpleClassnameList *traceFilterPortalShot )
{
#ifndef CLIENT_DLL
	traceFilterPortalShot->AddClassnameToIgnore("prop_physics");
	traceFilterPortalShot->AddClassnameToIgnore("func_physbox");
	traceFilterPortalShot->AddClassnameToIgnore("npc_portal_turret_floor");
	traceFilterPortalShot->AddClassnameToIgnore("prop_energy_ball");
	traceFilterPortalShot->AddClassnameToIgnore("npc_security_camera");
	traceFilterPortalShot->AddClassnameToIgnore("player");
	traceFilterPortalShot->AddClassnameToIgnore("simple_physics_prop");
	traceFilterPortalShot->AddClassnameToIgnore("simple_physics_brush");
	traceFilterPortalShot->AddClassnameToIgnore("prop_ragdoll");
	traceFilterPortalShot->AddClassnameToIgnore("prop_glados_core");
#else
	traceFilterPortalShot->AddClassnameToIgnore("class C_PhysicsProp");
	traceFilterPortalShot->AddClassnameToIgnore("class C_PhysBox");
	traceFilterPortalShot->AddClassnameToIgnore("class C_NPC_Portal_FloorTurret");
	traceFilterPortalShot->AddClassnameToIgnore("class C_PropEnergyBall");
	//traceFilterPortalShot->AddClassnameToIgnore("npc_security_camera");
	traceFilterPortalShot->AddClassnameToIgnore("player");
	//traceFilterPortalShot->AddClassnameToIgnore("simple_physics_prop");
	//traceFilterPortalShot->AddClassnameToIgnore("simple_physics_brush");
	traceFilterPortalShot->AddClassnameToIgnore("class C_ServerRagdoll");
	//traceFilterPortalShot->AddClassnameToIgnore("prop_glados_core");
#endif // !CLIENT_DLL

}


CProp_Portal* UTIL_Portal_FirstAlongRay( const Ray_t &ray, float &fMustBeCloserThan )
{
	CProp_Portal *pIntersectedPortal = NULL;

	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount != 0 )
	{
		CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();

		for( int i = 0; i != iPortalCount; ++i )
		{
			CProp_Portal *pTempPortal = pPortals[i];
			if( pTempPortal->IsActivedAndLinked() )
			{
				float fIntersection = UTIL_IntersectRayWithPortal( ray, pTempPortal );
				if( fIntersection >= 0.0f && fIntersection < fMustBeCloserThan )
				{
					//within range, now check directionality
					if( pTempPortal->m_plane_Origin.normal.Dot( ray.m_Delta ) < 0.0f )
					{
						//qualifies for consideration, now it just has to compete for closest
						pIntersectedPortal = pTempPortal;
						fMustBeCloserThan = fIntersection;
					}
				}
			}
		}
	}

	return pIntersectedPortal;
}


bool UTIL_Portal_TraceRay_Bullets( const CProp_Portal *pPortal, const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace, bool bTraceHolyWall )
{
	if( !pPortal || !pPortal->IsActivedAndLinked() )
	{
		//not in a portal environment, use regular traces
		enginetrace->TraceRay( ray, fMask, pTraceFilter, pTrace );
		return false;
	}

	trace_t trReal;

	enginetrace->TraceRay( ray, fMask, pTraceFilter, &trReal );

	Vector vRayNormal = ray.m_Delta;
	VectorNormalize( vRayNormal );

	Vector vPortalForward;
	pPortal->GetVectors( &vPortalForward, 0, 0 );

	// If the ray isn't going into the front of the portal, just use the real trace
	if ( vPortalForward.Dot( vRayNormal ) > 0.0f )
	{
		*pTrace = trReal;
		return false;
	}

	// If the real trace collides before the portal plane, just use the real trace
	float fPortalFraction = UTIL_IntersectRayWithPortal( ray, pPortal );

	if ( fPortalFraction == -1.0f || trReal.fraction + 0.0001f < fPortalFraction )
	{
		// Didn't intersect or the real trace intersected closer
		*pTrace = trReal;
		return false;
	}

	Ray_t rayPostPortal;
	rayPostPortal = ray;
	rayPostPortal.m_Start = ray.m_Start + ray.m_Delta * fPortalFraction;
	rayPostPortal.m_Delta = ray.m_Delta * ( 1.0f - fPortalFraction );

	VMatrix matThisToLinked = pPortal->MatrixThisToLinked();

	Ray_t rayTransformed;
	UTIL_Portal_RayTransform( matThisToLinked, rayPostPortal, rayTransformed );

	// After a bullet traces through a portal it can hit the player that fired it
	CTraceFilterSimple *pSimpleFilter = dynamic_cast<CTraceFilterSimple*>(pTraceFilter);
	const IHandleEntity *pPassEntity = NULL;
	if ( pSimpleFilter )
	{
		pPassEntity = pSimpleFilter->GetPassEntity();
		pSimpleFilter->SetPassEntity( 0 );
	}

	trace_t trPostPortal;
	enginetrace->TraceRay( rayTransformed, fMask, pTraceFilter, &trPostPortal );

	if ( pSimpleFilter )
	{
		pSimpleFilter->SetPassEntity( pPassEntity );
	}

	//trPostPortal.startpos = ray.m_Start;
	UTIL_Portal_PointTransform( matThisToLinked, ray.m_Start, trPostPortal.startpos );
	trPostPortal.fraction = trPostPortal.fraction * ( 1.0f - fPortalFraction ) + fPortalFraction;

	*pTrace = trPostPortal;

	return true;
}

CProp_Portal* UTIL_Portal_TraceRay_Beam( const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, float *pfFraction )
{
	// Do a regular trace
	trace_t tr;
	UTIL_TraceLine( ray.m_Start, ray.m_Start + ray.m_Delta, fMask, pTraceFilter, &tr );
	float fMustBeCloserThan = tr.fraction + 0.0001f;

	CProp_Portal *pIntersectedPortal = UTIL_Portal_FirstAlongRay( ray, fMustBeCloserThan );

	*pfFraction = fMustBeCloserThan; //will be real trace distance if it didn't hit a portal
	return pIntersectedPortal;
}


bool UTIL_Portal_Trace_Beam( const CBeam *pBeam, Vector &vecStart, Vector &vecEnd, Vector &vecIntersectionStart, Vector &vecIntersectionEnd, ITraceFilter *pTraceFilter )
{
	vecStart = pBeam->GetAbsStartPos();
	vecEnd = pBeam->GetAbsEndPos();

	// Trace to see if we've intersected a portal
	float fEndFraction;
	Ray_t rayBeam;

	bool bIsReversed = ( pBeam->GetBeamFlags() & FBEAM_REVERSED ) != 0x0;

	if ( !bIsReversed )
		rayBeam.Init( vecStart, vecEnd );
	else
		rayBeam.Init( vecEnd, vecStart );

	CProp_Portal *pPortal = UTIL_Portal_TraceRay_Beam( rayBeam, MASK_SHOT, pTraceFilter, &fEndFraction );

	// If we intersected a portal we need to modify the start and end points to match the actual trace through portal drawing extents
	if ( !pPortal )
		return false;

	// Modify the start and end points to match the actual trace through portal drawing extents
	vecStart = rayBeam.m_Start;

	Vector vecIntersection = rayBeam.m_Start + rayBeam.m_Delta * fEndFraction;

	int iNumLoops = 0;

	// Loop through the portals (at most 16 times)
	while ( pPortal && iNumLoops < 16 )
	{
		// Get the point that we hit a portal or wall
		vecIntersectionStart = vecIntersection;

		VMatrix matThisToLinked = pPortal->MatrixThisToLinked();

		// Get the transformed positions of the sub beam in the other portal's space
		UTIL_Portal_PointTransform( matThisToLinked, vecIntersectionStart, vecIntersectionEnd );
		UTIL_Portal_PointTransform( matThisToLinked, rayBeam.m_Start + rayBeam.m_Delta, vecEnd );

		CTraceFilterSkipClassname traceFilter( pPortal->m_hLinkedPortal, "prop_energy_ball", COLLISION_GROUP_NONE );

		rayBeam.Init( vecIntersectionEnd, vecEnd );
		pPortal = UTIL_Portal_TraceRay_Beam( rayBeam, MASK_SHOT, &traceFilter, &fEndFraction );
		vecIntersection = rayBeam.m_Start + rayBeam.m_Delta * fEndFraction;

		++iNumLoops;
	}

	vecEnd = vecIntersection;

	return true;
}


void UTIL_Portal_TraceRay_With( const CProp_Portal *pPortal, const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace, bool bTraceHolyWall )
{
	//check to see if the player is theoretically in a portal environment
	if( !pPortal || !pPortal->m_PortalSimulator.IsReadyToSimulate() )
	{
		//not in a portal environment, use regular traces
		enginetrace->TraceRay( ray, fMask, pTraceFilter, pTrace );
	}
	else
	{		

		trace_t RealTrace;
		enginetrace->TraceRay( ray, fMask, pTraceFilter, &RealTrace );

		trace_t PortalTrace;
		UTIL_Portal_TraceRay( pPortal, ray, fMask, pTraceFilter, &PortalTrace, bTraceHolyWall );

		if( !g_bForcePortalTrace && !RealTrace.startsolid && PortalTrace.fraction <= RealTrace.fraction )
		{
			*pTrace = RealTrace;
			return;
		}

		if ( g_bAllowForcePortalTrace )
		{
			g_bForcePortalTrace = true;
		}

		*pTrace = PortalTrace;

		// If this ray has a delta, make sure its towards the portal before we try to trace across portals
		Vector vDirection = ray.m_Delta;
		VectorNormalize( vDirection );
		Vector vPortalForward;
		pPortal->GetVectors( &vPortalForward, 0, 0 );
		
		float flDot = -1.0f;
		if ( ray.m_IsSwept )
		{
			flDot = vDirection.Dot( vPortalForward );
		} 

		// TODO: Translate extents of rays properly, tracing extruded box rays across portals causes collision bugs
		//		 Until this is fixed, we'll only test true rays across portals
		if ( flDot < 0.0f && /*PortalTrace.fraction == 1.0f &&*/ ray.m_IsRay)
		{
			// Check if we're hitting stuff on the other side of the portal
			trace_t PortalLinkedTrace;
			UTIL_PortalLinked_TraceRay( pPortal, ray, fMask, pTraceFilter, &PortalLinkedTrace, bTraceHolyWall );

			if ( PortalLinkedTrace.fraction < pTrace->fraction )
			{
				// Only collide with the cross-portal objects if this trace crossed a portal
				if ( UTIL_DidTraceTouchPortals( ray, PortalLinkedTrace ) )
				{
					*pTrace = PortalLinkedTrace;
				}
			}
		}

		if( pTrace->fraction < 1.0f )
		{
			pTrace->contents = RealTrace.contents;
			pTrace->surface = RealTrace.surface;
		}

	}
}


//-----------------------------------------------------------------------------
// Purpose: Tests if a ray touches the surface of any portals
// Input  : ray - the ray to be tested against portal surfaces
//			trace - a filled-in trace corresponding to the parameter ray 
// Output : bool - false if the 'ray' parameter failed to hit any portal surface
//		    pOutLocal - the portal touched (if any)
//			pOutRemote - the portal linked to the portal touched
//-----------------------------------------------------------------------------
bool UTIL_DidTraceTouchPortals( const Ray_t& ray, const trace_t& trace, CProp_Portal** pOutLocal, CProp_Portal** pOutRemote )
{
	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount == 0 )
	{
		if( pOutLocal )
			*pOutLocal = NULL;

		if( pOutRemote )
			*pOutRemote = NULL;

		return false;
	}

	CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();
	CProp_Portal *pIntersectedPortal = NULL;

	if( ray.m_IsSwept )
	{
		float fMustBeCloserThan = trace.fraction + 0.0001f;

		pIntersectedPortal = UTIL_Portal_FirstAlongRay( ray, fMustBeCloserThan );
	}
	
	if( (pIntersectedPortal == NULL) && !ray.m_IsRay )
	{
		//haven't hit anything yet, try again with box tests

		Vector ptRayEndPoint = trace.endpos - ray.m_StartOffset; // The trace added the start offset to the end position, so remove it for the box test
		CProp_Portal **pBoxIntersectsPortals = (CProp_Portal **)stackalloc( sizeof(CProp_Portal *) * iPortalCount );
		int iBoxIntersectsPortalsCount = 0;

		for( int i = 0; i != iPortalCount; ++i )
		{
			CProp_Portal *pTempPortal = pPortals[i];
			if( (pTempPortal->m_bActivated) && 
				(pTempPortal->m_hLinkedPortal.Get() != NULL) )
			{
				if( UTIL_IsBoxIntersectingPortal( ptRayEndPoint, ray.m_Extents, pTempPortal, 0.00f ) )
				{
					pBoxIntersectsPortals[iBoxIntersectsPortalsCount] = pTempPortal;
					++iBoxIntersectsPortalsCount;
				}
			}
		}

		if( iBoxIntersectsPortalsCount > 0 )
		{
			pIntersectedPortal = pBoxIntersectsPortals[0];
			
			if( iBoxIntersectsPortalsCount > 1 )
			{
				//hit more than one, use the closest
				float fDistToBeat = (ptRayEndPoint - pIntersectedPortal->GetAbsOrigin()).LengthSqr();

				for( int i = 1; i != iBoxIntersectsPortalsCount; ++i )
				{
					float fDist = (ptRayEndPoint - pBoxIntersectsPortals[i]->GetAbsOrigin()).LengthSqr();
					if( fDist < fDistToBeat )
					{
						pIntersectedPortal = pBoxIntersectsPortals[i];
						fDistToBeat = fDist;
					}
				}
			}
		}
	}

	if( pIntersectedPortal == NULL )
	{
		if( pOutLocal )
			*pOutLocal = NULL;

		if( pOutRemote )
			*pOutRemote = NULL;

		return false;
	}
	else
	{
		// Record the touched portals and return
		if( pOutLocal )
			*pOutLocal = pIntersectedPortal;

		if( pOutRemote )
			*pOutRemote = pIntersectedPortal->m_hLinkedPortal.Get();

		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Redirects the trace to either a trace that uses portal environments, or if a 
//			global boolean is set, trace with a special bullets trace.
//			NOTE: UTIL_Portal_TraceRay_With will use the default world trace if it gets a NULL portal pointer
// Input  : &ray - the ray to use to trace
//			fMask - collision mask
//			*pTraceFilter - customizable filter on the trace
//			*pTrace - trace struct to fill with output info
//-----------------------------------------------------------------------------
CProp_Portal* UTIL_Portal_TraceRay( const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace, bool bTraceHolyWall )
{
	float fMustBeCloserThan = 2.0f;
	CProp_Portal *pIntersectedPortal = UTIL_Portal_FirstAlongRay( ray, fMustBeCloserThan );

	if ( g_bBulletPortalTrace )
	{
		if ( UTIL_Portal_TraceRay_Bullets( pIntersectedPortal, ray, fMask, pTraceFilter, pTrace, bTraceHolyWall ) )
			return pIntersectedPortal;

		// Bullet didn't actually go through portal
		return NULL;

	}
	else
	{
		UTIL_Portal_TraceRay_With( pIntersectedPortal, ray, fMask, pTraceFilter, pTrace, bTraceHolyWall );
		return pIntersectedPortal;
	}
}

CProp_Portal* UTIL_Portal_TraceRay( const Ray_t &ray, unsigned int fMask, const IHandleEntity *ignore, int collisionGroup, trace_t *pTrace, bool bTraceHolyWall )
{
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	return UTIL_Portal_TraceRay( ray, fMask, &traceFilter, pTrace, bTraceHolyWall );
}

//extern ConVar sv_portal_new_player_trace;
//-----------------------------------------------------------------------------
// Purpose: This version of traceray only traces against the portal environment of the specified portal.
// Input  : *pPortal - the portal whose physics we will trace against
//			&ray - the ray to trace with
//			fMask - collision mask
//			*pTraceFilter - customizable filter to determine what it hits
//			*pTrace - the trace struct to fill in with results
//			bTraceHolyWall - if this trace is to test against the 'holy wall' geometry
//-----------------------------------------------------------------------------
//extern bool g_bSpewTraceStuck;
//void UTIL_Portal_TraceRay_PreTraceChanges(const CProp_Portal* pPortal, const Ray_t& ray, unsigned int fMask, ITraceFilter* pTraceFilter, trace_t* pTrace, bool bTraceHolyWall);
void UTIL_Portal_TraceRay(const CProp_Portal* pPortal, const Ray_t& ray, unsigned int fMask, ITraceFilter* pTraceFilter, trace_t* pTrace, bool bTraceHolyWall)
{
	//if (!sv_portal_new_player_trace.GetBool())
	//{
	//	//it's not funny anymore, go back
	//	return UTIL_Portal_TraceRay_PreTraceChanges(pPortal, ray, fMask, pTraceFilter, pTrace, bTraceHolyWall);
	//}

#if defined ( PORTAL_TRACE_LOGGING )
	int keptType = -1;
#endif

	Assert(pPortal->m_PortalSimulator.IsReadyToSimulate()); //a trace shouldn't make it down this far if the portal is incapable of changing the results of the trace

	CTraceFilterHitAll traceFilterHitAll;
	if (!pTraceFilter)
	{
		pTraceFilter = &traceFilterHitAll;
	}

	UTIL_ClearTrace(*pTrace);
	pTrace->startpos = ray.m_Start + ray.m_StartOffset;
	pTrace->endpos = pTrace->startpos + ray.m_Delta;

	// We shrink the ray when we hit something so subsequent traces can early-out on hits that would be further away
	Ray_t queryRay = ray;

	trace_t TempTrace;
	int counter;

	const CPortalSimulator& portalSimulator = pPortal->m_PortalSimulator;
	const PS_InternalData_t& portalSimulatorData = portalSimulator.GetInternalData();
	CPortalSimulator* pLinkedPortalSimulator = portalSimulator.GetLinkedPortalSimulator();

	//bool bTraceDisplacements = sv_portal_trace_vs_displacements.GetBool();
	bool bTraceStaticProps = sv_portal_trace_vs_staticprops.GetBool();
	if (sv_portal_trace_vs_holywall.GetBool() == false)
		bTraceHolyWall = false;

	bool bTraceTransformedGeometry = ((pLinkedPortalSimulator != NULL) && bTraceHolyWall && (portalSimulator.IsRayInPortalHole(queryRay) != RIPHR_NOT_TOUCHING_HOLE));

	bool bCopyBackBrushTraceData = false;



	// Traces vs world
	if (pTraceFilter->GetTraceType() != TRACE_ENTITIES_ONLY)
	{
		if (sv_portal_trace_vs_world.GetBool())
		{
			for (int iBrushSet = 0; iBrushSet != ARRAYSIZE(portalSimulatorData.Simulation.Static.World.Brushes.BrushSets); ++iBrushSet)
			{
				const PS_SD_Static_BrushSet_t* pBrushSet = &portalSimulatorData.Simulation.Static.World.Brushes.BrushSets[iBrushSet];
				if (((pBrushSet->iSolidMask & fMask) != 0) && pBrushSet->pCollideable &&
					TraceBoxAA(queryRay, pBrushSet->pCollideable, &TempTrace))
				{
					bCopyBackBrushTraceData = true;

					bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
					if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
					{
						*pTrace = TempTrace;

						if (portal_trace_shrink_ray_each_query.GetBool())
						{
							queryRay.m_Delta *= pTrace->fraction;
						}
					}

					if (pTrace->DidHit() && portal_trace_shrink_ray_each_query.GetBool())
					{
						queryRay.m_Delta *= pTrace->fraction;
					}

					Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));

#if defined ( PORTAL_TRACE_LOGGING )
					keptType = BRUSHES;
					s_TraceLogger.LogTrace(BRUSHES);
#endif
				}
			}
		}

		if (bTraceStaticProps)
		{
			bool bFilterStaticProps = (pTraceFilter->GetTraceType() == TRACE_EVERYTHING_FILTER_PROPS);
			//local clipped static props
			{
				int iLocalStaticCount = portalSimulatorData.Simulation.Static.World.StaticProps.ClippedRepresentations.Count();
				if (iLocalStaticCount != 0 && portalSimulatorData.Simulation.Static.World.StaticProps.bCollisionExists)
				{
					const PS_SD_Static_World_StaticProps_ClippedProp_t* pCurrentProp = portalSimulatorData.Simulation.Static.World.StaticProps.ClippedRepresentations.Base();
					const PS_SD_Static_World_StaticProps_ClippedProp_t* pStop = pCurrentProp + iLocalStaticCount;
					do
					{
						if ((!bFilterStaticProps || pTraceFilter->ShouldHitEntity(pCurrentProp->pSourceProp, fMask)) &&
							TraceBoxAA(queryRay, pCurrentProp->pCollide, &TempTrace))
						{
#if defined ( PORTAL_TRACE_LOGGING )
							s_TraceLogger.LogTrace(STATIC_PROPS);
#endif

							bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
							if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
							{
								*pTrace = TempTrace;
								pTrace->surface.flags = pCurrentProp->iTraceSurfaceFlags;
								pTrace->surface.surfaceProps = pCurrentProp->iTraceSurfaceProps;
								pTrace->surface.name = pCurrentProp->szTraceSurfaceName;
								pTrace->contents = pCurrentProp->iTraceContents;
								pTrace->m_pEnt = pCurrentProp->pTraceEntity;
								Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
								if (portal_trace_shrink_ray_each_query.GetBool())
								{
									queryRay.m_Delta *= pTrace->fraction;
								}
#if defined ( PORTAL_TRACE_LOGGING )
								keptType = STATIC_PROPS;
#endif
							}
						}

						++pCurrentProp;
					} while (pCurrentProp != pStop);
				}
			}

			if (bTraceHolyWall)
			{
				//remote clipped static props transformed into our wall space
				if (bTraceTransformedGeometry && (pTraceFilter->GetTraceType() != TRACE_WORLD_ONLY) && sv_portal_trace_vs_staticprops.GetBool())
				{
					int iLocalStaticCount = pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.StaticProps.ClippedRepresentations.Count();
					if (iLocalStaticCount != 0)
					{
						const PS_SD_Static_World_StaticProps_ClippedProp_t* pCurrentProp = pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.StaticProps.ClippedRepresentations.Base();
						const PS_SD_Static_World_StaticProps_ClippedProp_t* pStop = pCurrentProp + iLocalStaticCount;
						Vector vTransform = portalSimulatorData.Placement.ptaap_LinkedToThis.ptShrinkAlignedOrigin;
						QAngle qTransform = portalSimulatorData.Placement.ptaap_LinkedToThis.qAngleTransform;

						do
						{
							if (!bFilterStaticProps || pTraceFilter->ShouldHitEntity(pCurrentProp->pSourceProp, fMask))
							{
								physcollision->TraceBox(queryRay, pCurrentProp->pCollide, vTransform, qTransform, &TempTrace);
#if defined ( PORTAL_TRACE_LOGGING )
								s_TraceLogger.LogTrace(REMOTE_STATIC_PROPS);
#endif
								bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
								if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
								{
									*pTrace = TempTrace;
									pTrace->surface.flags = pCurrentProp->iTraceSurfaceFlags;
									pTrace->surface.surfaceProps = pCurrentProp->iTraceSurfaceProps;
									pTrace->surface.name = pCurrentProp->szTraceSurfaceName;
									pTrace->contents = pCurrentProp->iTraceContents;
									pTrace->m_pEnt = pCurrentProp->pTraceEntity;
									Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
									if (portal_trace_shrink_ray_each_query.GetBool())
									{
										queryRay.m_Delta *= pTrace->fraction;
									}
#if defined ( PORTAL_TRACE_LOGGING )
									keptType = REMOTE_STATIC_PROPS;
#endif
								}
							}

							++pCurrentProp;
						} while (pCurrentProp != pStop);
					}
				}
			}
		}

		if (portalSimulatorData.Simulation.Static.World.Displacements.pCollideable && sv_portal_trace_vs_world.GetBool() && portal_clone_displacements.GetBool())
		{
			physcollision->TraceBox(queryRay, portalSimulatorData.Simulation.Static.World.Displacements.pCollideable, vec3_origin, vec3_angle, &TempTrace);
#if defined ( PORTAL_TRACE_LOGGING )
			s_TraceLogger.LogTrace(DISP);
#endif
			bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
			if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
			{
				*pTrace = TempTrace;
				bCopyBackBrushTraceData = true;
				//pTrace->dispFlags |= DISPSURF_FLAG_SURFACE | DISPSURF_FLAG_WALKABLE;
				Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
				if (portal_trace_shrink_ray_each_query.GetBool())
				{
					queryRay.m_Delta *= pTrace->fraction;
				}
#if defined ( PORTAL_TRACE_LOGGING )
				keptType = DISP;
#endif
			}
		}

		if (bTraceHolyWall)
		{
			for (int iBrushSet = 0; iBrushSet != ARRAYSIZE(portalSimulatorData.Simulation.Static.Wall.Local.Brushes.BrushSets); ++iBrushSet)
			{
				const PS_SD_Static_BrushSet_t* pBrushSet = &portalSimulatorData.Simulation.Static.Wall.Local.Brushes.BrushSets[iBrushSet];
				if (((pBrushSet->iSolidMask & fMask) != 0) && pBrushSet->pCollideable &&
					TraceBoxAA(queryRay, pBrushSet->pCollideable, &TempTrace))
				{
#if defined ( PORTAL_TRACE_LOGGING )
					s_TraceLogger.LogTrace(HOLYWALL_BRUSHES);
#endif
					bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
					if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
					{
						*pTrace = TempTrace;
						bCopyBackBrushTraceData = true;
						Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
						if (portal_trace_shrink_ray_each_query.GetBool())
						{
							queryRay.m_Delta *= pTrace->fraction;
						}
#if defined ( PORTAL_TRACE_LOGGING )
						keptType = HOLYWALL_BRUSHES;
#endif
					}
				}
			}

			if (portalSimulatorData.Simulation.Static.Wall.Local.Tube.pCollideable)
			{
				TraceBoxAA(queryRay, portalSimulatorData.Simulation.Static.Wall.Local.Tube.pCollideable, &TempTrace);

#if defined ( PORTAL_TRACE_LOGGING )
				s_TraceLogger.LogTrace(HOLYWALL_TUBE);
#endif
				bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
				if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
				{
					*pTrace = TempTrace;
					bCopyBackBrushTraceData = true;
					if (portal_trace_shrink_ray_each_query.GetBool())
					{
						queryRay.m_Delta *= pTrace->fraction;
					}
#if defined ( PORTAL_TRACE_LOGGING )
					keptType = HOLYWALL_TUBE;
#endif
				}
			}

			//if( portalSimulatorData.Simulation.Static.Wall.RemoteTransformedToLocal.Brushes.pCollideable && sv_portal_trace_vs_world.GetBool() )
			if (bTraceTransformedGeometry)
			{
				for (int iBrushSet = 0; iBrushSet != ARRAYSIZE(pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets); ++iBrushSet)
				{
					const PS_SD_Static_BrushSet_t* pBrushSet = &pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet];
					if (((pBrushSet->iSolidMask & fMask) != 0) && pBrushSet->pCollideable)
					{
						physcollision->TraceBox(queryRay, pBrushSet->pCollideable, portalSimulatorData.Placement.ptaap_LinkedToThis.ptOriginTransform, portalSimulatorData.Placement.ptaap_LinkedToThis.qAngleTransform, &TempTrace);
#if defined ( PORTAL_TRACE_LOGGING )
						s_TraceLogger.LogTrace(HOLYWALL_TRANSLATED_BRUSHES);
#endif
						bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
						if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
						{
							*pTrace = TempTrace;
							bCopyBackBrushTraceData = true;
							if (portal_trace_shrink_ray_each_query.GetBool())
							{
								queryRay.m_Delta *= pTrace->fraction;
							}
							Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
#if defined ( PORTAL_TRACE_LOGGING )
							keptType = HOLYWALL_TRANSLATED_BRUSHES;
#endif
						}
					}
				}
			}
		}

		if (bCopyBackBrushTraceData)
		{
			pTrace->surface = portalSimulatorData.Simulation.Static.SurfaceProperties.surface;
			pTrace->contents = portalSimulatorData.Simulation.Static.SurfaceProperties.contents;
			pTrace->m_pEnt = portalSimulatorData.Simulation.Static.SurfaceProperties.pEntity;

			bCopyBackBrushTraceData = false;
		}
	}

	// Traces vs entities
	if (pTraceFilter->GetTraceType() != TRACE_WORLD_ONLY)
	{

		//solid entities
		CPortalCollideableEnumerator enumerator(pPortal);

		int PartitionMask;
#if defined( CLIENT_DLL )
		PartitionMask = PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_STATIC_PROPS;
#else
		PartitionMask = PARTITION_ENGINE_SOLID_EDICTS | PARTITION_ENGINE_STATIC_PROPS;
#endif

		::partition->EnumerateElementsAlongRay(PartitionMask, queryRay, false, &enumerator);
		for (counter = 0; counter != enumerator.m_iHandleCount; ++counter)
		{
			if (staticpropmgr->IsStaticProp(enumerator.m_pHandles[counter]))
			{
				//if( bFilterStaticProps && !pTraceFilter->ShouldHitEntity( enumerator.m_pHandles[counter], fMask ) )
				continue; //static props are handled separately, with clipped versions
			}
			else if (!pTraceFilter->ShouldHitEntity(enumerator.m_pHandles[counter], fMask))
			{
				continue;
			}

			CBaseEntity* pEnumeratedEntity = EntityFromEntityHandle(enumerator.m_pHandles[counter]);

			//If we have a carved representation of this entity, trace against that instead of the real thing
			CPhysCollide* pCarvedCollide = portalSimulator.IsEntityCarvedByPortal(pEnumeratedEntity) ? portalSimulator.GetCollideForCarvedEntity(pEnumeratedEntity) : NULL;
			if (pCarvedCollide != NULL)
			{
				ICollideable* pUncarvedCollideable = pEnumeratedEntity->GetCollideable();
				physcollision->TraceBox(queryRay, pCarvedCollide, pUncarvedCollideable->GetCollisionOrigin(), pUncarvedCollideable->GetCollisionAngles(), &TempTrace);
#if defined ( PORTAL_TRACE_LOGGING )
				s_TraceLogger.LogTrace(ENTITIES);
#endif

				bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
				if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
				{
					//copy the trace data from the carved trace
					*pTrace = TempTrace;

					//then trace against the real thing for surface data.
					//TODO: There's got to be a way to store this off and look it up intelligently. But I can't seem to find surface info without a trace, making the results only valid for that trace.
					enginetrace->ClipRayToEntity(queryRay, fMask, enumerator.m_pHandles[counter], &TempTrace);
					pTrace->contents = TempTrace.contents;
					pTrace->surface = TempTrace.surface;
					pTrace->m_pEnt = TempTrace.m_pEnt;
					Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
					if (portal_trace_shrink_ray_each_query.GetBool())
					{
						queryRay.m_Delta *= pTrace->fraction;
					}
#if defined ( PORTAL_TRACE_LOGGING )
					keptType = ENTITIES;
#endif
				}
			}
			else
			{
				enginetrace->ClipRayToEntity(queryRay, fMask, enumerator.m_pHandles[counter], &TempTrace);
#if defined ( PORTAL_TRACE_LOGGING )
				s_TraceLogger.LogTrace(ENTITIES);
#endif
				bool bIsCloser = (portal_trace_shrink_ray_each_query.GetBool()) ? (TempTrace.DidHit()) : (TempTrace.fraction < pTrace->fraction);
				if ((TempTrace.startsolid && !pTrace->startsolid) || (queryRay.m_IsSwept && bIsCloser))
				{
					*pTrace = TempTrace;
					Assert(pTrace->startsolid || (pTrace->fraction == 1.0f) || (pTrace->plane.normal.LengthSqr() > 0.5f));
					if (portal_trace_shrink_ray_each_query.GetBool())
					{
						queryRay.m_Delta *= pTrace->fraction;
					}
#if defined ( PORTAL_TRACE_LOGGING )
					keptType = ENTITIES;
#endif
				}
			}
		}
	}

#if defined ( PORTAL_TRACE_LOGGING )
	if (pTrace->DidHit() && keptType >= 0 && keptType < TRACE_TYPE_COUNT)
		s_TraceLogger.LogTypeKept((eTraceType)keptType);

	s_TraceLogger.Display();
#endif
}

void UTIL_Portal_TraceRay( const CProp_Portal *pPortal, const Ray_t &ray, unsigned int fMask, const IHandleEntity *ignore, int collisionGroup, trace_t *pTrace, bool bTraceHolyWall )
{
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	UTIL_Portal_TraceRay( pPortal, ray, fMask, &traceFilter, pTrace, bTraceHolyWall );
}

//-----------------------------------------------------------------------------
// Purpose: Trace a ray 'past' a portal's surface, hitting objects in the linked portal's collision environment
// Input  : *pPortal - The portal being traced 'through'
//			&ray - The ray being traced
//			fMask - trace mask to cull results
//			*pTraceFilter - trace filter to cull results
//			*pTrace - Empty trace to return the result (value will be overwritten)
//-----------------------------------------------------------------------------
void UTIL_PortalLinked_TraceRay( const CProp_Portal *pPortal, const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace, bool bTraceHolyWall )
{
#ifdef CLIENT_DLL
	Assert( (GameRules() == NULL) || GameRules()->IsMultiplayer() );
#endif
	// Transform the specified ray to the remote portal's space
	Ray_t rayTransformed;
	UTIL_Portal_RayTransform( pPortal->MatrixThisToLinked(), ray, rayTransformed );

	AssertMsg ( ray.m_IsRay, "Ray with extents across portal tracing not implemented!" );

	const CPortalSimulator &portalSimulator = pPortal->m_PortalSimulator;
	CProp_Portal *pLinkedPortal = (CProp_Portal*)(pPortal->m_hLinkedPortal.Get());
	if( (pLinkedPortal == NULL) || (portalSimulator.IsRayInPortalHole(ray) == RIPHR_NOT_TOUCHING_HOLE) )
	{
		memset( pTrace, 0, sizeof(trace_t));
		pTrace->fraction = 1.0f;
		pTrace->fractionleftsolid = 0;

		pTrace->contents = pPortal->m_PortalSimulator.GetInternalData().Simulation.Static.SurfaceProperties.contents;
		pTrace->surface  = pPortal->m_PortalSimulator.GetInternalData().Simulation.Static.SurfaceProperties.surface;
		pTrace->m_pEnt	 = pPortal->m_PortalSimulator.GetInternalData().Simulation.Static.SurfaceProperties.pEntity;
		return;
	}
	UTIL_Portal_TraceRay( pLinkedPortal, rayTransformed, fMask, pTraceFilter, pTrace, bTraceHolyWall );

	// Transform the ray's start, end and plane back into this portal's space, 
	// because we react to the collision as it is displayed, and the image is displayed with this local portal's orientation.
	VMatrix matLinkedToThis = pLinkedPortal->MatrixThisToLinked();
	UTIL_Portal_PointTransform( matLinkedToThis, pTrace->startpos, pTrace->startpos );
	UTIL_Portal_PointTransform( matLinkedToThis, pTrace->endpos, pTrace->endpos );
	UTIL_Portal_PlaneTransform( matLinkedToThis, pTrace->plane, pTrace->plane );
}

void UTIL_PortalLinked_TraceRay( const CProp_Portal *pPortal, const Ray_t &ray, unsigned int fMask, const IHandleEntity *ignore, int collisionGroup, trace_t *pTrace, bool bTraceHolyWall )
{
	CTraceFilterSimple traceFilter( ignore, collisionGroup );
	UTIL_PortalLinked_TraceRay( pPortal, ray, fMask, &traceFilter, pTrace, bTraceHolyWall );
}

struct CollideableTraceData_t
{
	Vector vecAbsStart;
	Vector vecAbsEnd;
	CPhysCollide* pEntCollide;
	QAngle qCollisionAngle;
};

void TraceFunc_Collideable(void* pData, CPhysCollide* pCollide, const Vector& vOrigin, const QAngle& qAngle, trace_t* pTrace)
{
	physcollision->TraceCollide(((CollideableTraceData_t*)pData)->vecAbsStart, ((CollideableTraceData_t*)pData)->vecAbsEnd, ((CollideableTraceData_t*)pData)->pEntCollide, ((CollideableTraceData_t*)pData)->qCollisionAngle,
		pCollide, vOrigin, qAngle, pTrace);
}

void TraceFunc_Ray(void* pData, CPhysCollide* pCollide, const Vector& vOrigin, const QAngle& qAngle, trace_t* pTrace)
{
	physcollision->TraceBox(*(Ray_t*)pData, MASK_ALL, NULL, pCollide, vOrigin, qAngle, pTrace);
}

//-----------------------------------------------------------------------------
// Purpose: A version of trace entity which detects portals and translates the trace through portals
//-----------------------------------------------------------------------------
void UTIL_Portal_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
							 unsigned int mask, ITraceFilter *pFilter, trace_t *pTrace )
{
	if (!pEntity || !pEntity->CollisionProp())
	{
		Assert(0);
		return;
	}

#ifdef CLIENT_DLL
	Assert(pEntity->IsPlayer());

	CPortalSimulator* pPortalSimulator = NULL;
	if (pEntity->IsPlayer())
	{
		C_Prop_Portal* pPortal = ((C_Portal_Player*)pEntity)->m_hPortalEnvironment.Get();
		if (pPortal)
			pPortalSimulator = &pPortal->m_PortalSimulator;
	}
#else
	CPortalSimulator* pPortalSimulator = CPortalSimulator::GetSimulatorThatOwnsEntity(pEntity);
#endif

	memset(pTrace, 0, sizeof(trace_t));
	pTrace->fraction = 1.0f;
	pTrace->fractionleftsolid = 0;

	ICollideable* pCollision = enginetrace->GetCollideable(pEntity);

	// If main is simulating this object, trace as UTIL_TraceEntity would
	trace_t realTrace;
	QAngle qCollisionAngles = pCollision->GetCollisionAngles();
	// BUG: sweep collideable requires no angles to be passed in
	enginetrace->SweepCollideable(pCollision, vecAbsStart, vecAbsEnd, vec3_angle, mask, pFilter, &realTrace);

	// For the below box test, we need to add the tolerance onto the extents, because the underlying
	// box on plane side test doesn't use the parameter tolerance.
	float flTolerance = 0.1f;
	Vector vEntExtents = pEntity->CollisionProp()->OBBSize() * 0.5 + Vector(flTolerance, flTolerance, flTolerance);
	Vector vColCenter = realTrace.endpos + (pEntity->CollisionProp()->OBBMins() + pEntity->CollisionProp()->OBBMaxs()) * 0.5f;

	// If this entity is not simulated in a portal environment, trace as normal
	if (pPortalSimulator == NULL)
	{
		// If main is simulating this object, trace as UTIL_TraceEntity would
		*pTrace = realTrace;
	}
	else
	{
		CPhysCollide* pTempPhysCollide = (/*sv_portal_new_player_trace.GetBool() &&*/ (pEntity->GetSolid() == SOLID_BBOX)) ? NULL : physcollision->BBoxToCollide(pCollision->OBBMins(), pCollision->OBBMaxs());
		//CPhysCollide *pTempPhysCollide = physcollision->BBoxToCollide( pCollision->OBBMins(), pCollision->OBBMaxs() );
		CPhysCollide* pEntPhysCollide = pTempPhysCollide; //HACKHACK: At some point we should investigate into getting the real CPhysCollide

		void (*pTraceFunc)(void*, CPhysCollide*, const Vector&, const QAngle&, trace_t*);
		void* pTraceData;
		Ray_t entRay;
		entRay.Init(vecAbsStart, vecAbsEnd, pCollision->OBBMins(), pCollision->OBBMaxs());
		CollideableTraceData_t entCollideableData;

		if (pEntPhysCollide)
		{
			pTraceFunc = TraceFunc_Collideable;
			pTraceData = &entCollideableData;
			entCollideableData.pEntCollide = pEntPhysCollide;
			entCollideableData.qCollisionAngle = qCollisionAngles;
			entCollideableData.vecAbsStart = vecAbsStart;
			entCollideableData.vecAbsEnd = vecAbsEnd;
}
		else
		{
			pTraceFunc = TraceFunc_Ray;
			pTraceData = &entRay;
		}

		CPortalSimulator* pLinkedPortalSimulator = pPortalSimulator->GetLinkedPortalSimulator();

#if 0 // this trace for brush ents made sense at one time, but it's 'overcolliding' during portal transitions (bugzilla#25)
		if (realTrace.m_pEnt && (realTrace.m_pEnt->GetMoveType() != MOVETYPE_NONE)) //started by hitting something moving which wouldn't be detected in the following traces
		{
			float fFirstPortalFraction = 2.0f;
			CPortal_Base2D* pFirstPortal = UTIL_Portal_FirstAlongRay(entRay, fFirstPortalFraction);

			if (!pFirstPortal)
				*pTrace = realTrace;
			else
			{
				if (pFirstPortal->m_vForward.Dot(realTrace.endpos - pFirstPortal->m_ptOrigin) > 0.0f)
					*pTrace = realTrace;
			}
		}
#endif

		// We require both environments to be active in order to trace against them
		Assert(pCollision);
		if (!pCollision)
		{
			return;
		}

		// World, displacements and holy wall are stored in separate collideables
		// Traces against each and keep the closest intersection (if any)
		trace_t tempTrace;

		// Hit the world
		if (pFilter->GetTraceType() != TRACE_ENTITIES_ONLY)
		{
			if (sv_portal_trace_vs_world.GetBool())
			{
				for (int iBrushSet = 0; iBrushSet != ARRAYSIZE(pPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets); ++iBrushSet)
				{
					if (((pPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet].iSolidMask & mask) != 0) &&
						pPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet].pCollideable)
					{
						pTraceFunc(pTraceData, pPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet].pCollideable, vec3_origin, vec3_angle, &tempTrace);

						if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
						{
							*pTrace = tempTrace;
						}
					}
				}
			}

			if (pPortalSimulator->GetInternalData().Simulation.Static.World.Displacements.pCollideable &&
				sv_portal_trace_vs_world.GetBool() &&
				portal_clone_displacements.GetBool())
			{
				pTraceFunc(pTraceData, pPortalSimulator->GetInternalData().Simulation.Static.World.Displacements.pCollideable, vec3_origin, vec3_angle, &tempTrace);

				if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
				{
					*pTrace = tempTrace;
				}
			}

			//if( pPortalSimulator->GetInternalData().Simulation.Static.Wall.RemoteTransformedToLocal.Brushes.pCollideable &&
			if (pLinkedPortalSimulator &&
				sv_portal_trace_vs_world.GetBool() &&
				sv_portal_trace_vs_holywall.GetBool())
			{
				for (int iBrushSet = 0; iBrushSet != ARRAYSIZE(pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets); ++iBrushSet)
				{
					if (((pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet].iSolidMask & mask) != 0) &&
						pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet].pCollideable)
					{
						pTraceFunc(pTraceData, pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Brushes.BrushSets[iBrushSet].pCollideable, pPortalSimulator->GetInternalData().Placement.ptaap_LinkedToThis.ptOriginTransform, pPortalSimulator->GetInternalData().Placement.ptaap_LinkedToThis.qAngleTransform, &tempTrace);

						if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
						{
							*pTrace = tempTrace;
						}
					}
				}
			}

			//if( pPortalSimulator->GetInternalData().Simulation.Static.Wall.RemoteTransformedToLocal.Brushes.pCollideable &&
			if (pLinkedPortalSimulator &&
				pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Displacements.pCollideable &&
				sv_portal_trace_vs_world.GetBool() &&
				sv_portal_trace_vs_holywall.GetBool() &&
				portal_clone_displacements.GetBool())
			{
				pTraceFunc(pTraceData, pLinkedPortalSimulator->GetInternalData().Simulation.Static.World.Displacements.pCollideable, pPortalSimulator->GetInternalData().Placement.ptaap_LinkedToThis.ptOriginTransform, pPortalSimulator->GetInternalData().Placement.ptaap_LinkedToThis.qAngleTransform, &tempTrace);

				if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
				{
					*pTrace = tempTrace;
				}
			}

			if (sv_portal_trace_vs_holywall.GetBool())
			{
				for (int iBrushSet = 0; iBrushSet != ARRAYSIZE(pPortalSimulator->GetInternalData().Simulation.Static.Wall.Local.Brushes.BrushSets); ++iBrushSet)
				{
					if (((pPortalSimulator->GetInternalData().Simulation.Static.Wall.Local.Brushes.BrushSets[iBrushSet].iSolidMask & mask) != 0) &&
						pPortalSimulator->GetInternalData().Simulation.Static.Wall.Local.Brushes.BrushSets[iBrushSet].pCollideable)
					{
						pTraceFunc(pTraceData, pPortalSimulator->GetInternalData().Simulation.Static.Wall.Local.Brushes.BrushSets[iBrushSet].pCollideable, vec3_origin, vec3_angle, &tempTrace);

						if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
						{
							if (tempTrace.fraction == 0.0f)
								tempTrace.startsolid = true;

							if (tempTrace.fractionleftsolid == 1.0f)
								tempTrace.allsolid = true;

							*pTrace = tempTrace;
						}
					}
				}
			}

			if (pPortalSimulator->GetInternalData().Simulation.Static.Wall.Local.Tube.pCollideable &&
				sv_portal_trace_vs_holywall.GetBool())
			{
				pTraceFunc(pTraceData, pPortalSimulator->GetInternalData().Simulation.Static.Wall.Local.Tube.pCollideable, vec3_origin, vec3_angle, &tempTrace);

				if ((tempTrace.startsolid == false) && (tempTrace.fraction < pTrace->fraction)) //never allow something to be stuck in the tube, it's more of a last-resort guide than a real collideable
				{
					*pTrace = tempTrace;
				}
			}

			// For all brush traces, use the 'portal backbrush' surface surface contents
			// BUGBUG: Doing this is a great solution because brushes near a portal
			// will have their contents and surface properties homogenized to the brush the portal ray hit.
			if (pTrace->startsolid || (pTrace->fraction < 1.0f))
			{
				pTrace->surface = pPortalSimulator->GetInternalData().Simulation.Static.SurfaceProperties.surface;
				pTrace->contents = pPortalSimulator->GetInternalData().Simulation.Static.SurfaceProperties.contents;
				pTrace->m_pEnt = pPortalSimulator->GetInternalData().Simulation.Static.SurfaceProperties.pEntity;
			}
		}

		// Trace vs entities
		if (pFilter->GetTraceType() != TRACE_WORLD_ONLY)
		{
			if (sv_portal_trace_vs_staticprops.GetBool() && (pFilter->GetTraceType() != TRACE_ENTITIES_ONLY))
			{
				bool bFilterStaticProps = (pFilter->GetTraceType() == TRACE_EVERYTHING_FILTER_PROPS);

				//local clipped static props
				{
					int iLocalStaticCount = pPortalSimulator->GetInternalData().Simulation.Static.World.StaticProps.ClippedRepresentations.Count();
					if (iLocalStaticCount != 0 && pPortalSimulator->GetInternalData().Simulation.Static.World.StaticProps.bCollisionExists)
					{
						const PS_SD_Static_World_StaticProps_ClippedProp_t* pCurrentProp = pPortalSimulator->GetInternalData().Simulation.Static.World.StaticProps.ClippedRepresentations.Base();
						const PS_SD_Static_World_StaticProps_ClippedProp_t* pStop = pCurrentProp + iLocalStaticCount;
						Vector vTransform = vec3_origin;
						QAngle qTransform = vec3_angle;

						do
						{
							if ((!bFilterStaticProps) || pFilter->ShouldHitEntity(pCurrentProp->pSourceProp, mask))
							{
								pTraceFunc(pTraceData, pCurrentProp->pCollide, vTransform, qTransform, &tempTrace);

								if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
								{
									*pTrace = tempTrace;
									pTrace->surface.flags = pCurrentProp->iTraceSurfaceFlags;
									pTrace->surface.surfaceProps = pCurrentProp->iTraceSurfaceProps;
									pTrace->surface.name = pCurrentProp->szTraceSurfaceName;
									pTrace->contents = pCurrentProp->iTraceContents;
									pTrace->m_pEnt = pCurrentProp->pTraceEntity;
								}
							}

							++pCurrentProp;
						} while (pCurrentProp != pStop);
					}
				}

				if (pLinkedPortalSimulator && pPortalSimulator->EntityIsInPortalHole(pEntity))
				{

#ifndef CLIENT_DLL
					if (sv_use_transformed_collideables.GetBool()) //if this never gets turned off, it should be removed before release
					{
						//moving entities near the remote portal
						CBaseEntity* pEnts[1024];
						int iEntCount = pLinkedPortalSimulator->GetMoveableOwnedEntities(pEnts, 1024);

						CTransformedCollideable transformedCollideable;
						transformedCollideable.m_matTransform = pLinkedPortalSimulator->GetInternalData().Placement.matThisToLinked;
						transformedCollideable.m_matInvTransform = pLinkedPortalSimulator->GetInternalData().Placement.matLinkedToThis;
						for (int i = 0; i != iEntCount; ++i)
						{
							CBaseEntity* pRemoteEntity = pEnts[i];
							if (pRemoteEntity->GetSolid() == SOLID_NONE)
								continue;

							transformedCollideable.m_pWrappedCollideable = pRemoteEntity->GetCollideable();
							Assert(transformedCollideable.m_pWrappedCollideable != NULL);

							//enginetrace->ClipRayToCollideable( entRay, mask, &transformedCollideable, pTrace );

							enginetrace->ClipRayToCollideable(entRay, mask, &transformedCollideable, &tempTrace);
							if (tempTrace.startsolid || (tempTrace.fraction < pTrace->fraction))
							{
								*pTrace = tempTrace;
							}
						}
					}
#endif
				}
			}
		}

		if (pTrace->fraction == 1.0f)
		{
			memset(pTrace, 0, sizeof(trace_t));
			pTrace->fraction = 1.0f;
			pTrace->startpos = vecAbsStart;
			pTrace->endpos = vecAbsEnd;
		}

		if (pTempPhysCollide)
		{
			physcollision->DestroyCollide(pTempPhysCollide);
		}
	}
}

void UTIL_Portal_PointTransform( const VMatrix matThisToLinked, const Vector &ptSource, Vector &ptTransformed )
{
	ptTransformed = matThisToLinked * ptSource;
}

void UTIL_Portal_VectorTransform( const VMatrix matThisToLinked, const Vector &vSource, Vector &vTransformed )
{
	vTransformed = matThisToLinked.ApplyRotation( vSource );
}

void UTIL_Portal_AngleTransform( const VMatrix matThisToLinked, const QAngle &qSource, QAngle &qTransformed )
{
	qTransformed = TransformAnglesToWorldSpace( qSource, matThisToLinked.As3x4() );
}

void UTIL_Portal_RayTransform( const VMatrix matThisToLinked, const Ray_t &raySource, Ray_t &rayTransformed )
{
	rayTransformed = raySource;

	UTIL_Portal_PointTransform( matThisToLinked, raySource.m_Start, rayTransformed.m_Start );
	UTIL_Portal_VectorTransform( matThisToLinked, raySource.m_StartOffset, rayTransformed.m_StartOffset );
	UTIL_Portal_VectorTransform( matThisToLinked, raySource.m_Delta, rayTransformed.m_Delta );

	//BUGBUG: Extents are axis aligned, so rotating it won't necessarily give us what we're expecting
	UTIL_Portal_VectorTransform( matThisToLinked, raySource.m_Extents, rayTransformed.m_Extents );
	
	//HACKHACK: Negative extents hang in traces, make each positive because we rotated it above
	if ( rayTransformed.m_Extents.x < 0.0f )
	{
		rayTransformed.m_Extents.x = -rayTransformed.m_Extents.x;
	}
	if ( rayTransformed.m_Extents.y < 0.0f )
	{
		rayTransformed.m_Extents.y = -rayTransformed.m_Extents.y;
	}
	if ( rayTransformed.m_Extents.z < 0.0f )
	{
		rayTransformed.m_Extents.z = -rayTransformed.m_Extents.z;
	}

}

void UTIL_Portal_PlaneTransform( const VMatrix matThisToLinked, const cplane_t &planeSource, cplane_t &planeTransformed )
{
	planeTransformed = planeSource;

	Vector vTrans;
	UTIL_Portal_VectorTransform( matThisToLinked, planeSource.normal, planeTransformed.normal );
	planeTransformed.dist = planeSource.dist * DotProduct( planeTransformed.normal, planeTransformed.normal );
	planeTransformed.dist += DotProduct( planeTransformed.normal, matThisToLinked.GetTranslation( vTrans ) );
}

void UTIL_Portal_PlaneTransform( const VMatrix matThisToLinked, const VPlane &planeSource, VPlane &planeTransformed )
{
	Vector vTranformedNormal;
	float fTransformedDist;

	Vector vTrans;
	UTIL_Portal_VectorTransform( matThisToLinked, planeSource.m_Normal, vTranformedNormal );
	fTransformedDist = planeSource.m_Dist * DotProduct( vTranformedNormal, vTranformedNormal );
	fTransformedDist += DotProduct( vTranformedNormal, matThisToLinked.GetTranslation( vTrans ) );

	planeTransformed.Init( vTranformedNormal, fTransformedDist );
}

void UTIL_Portal_Triangles( const Vector &ptPortalCenter, const QAngle &qPortalAngles, Vector pvTri1[ 3 ], Vector pvTri2[ 3 ] )
{
	// Get points to make triangles
	Vector vRight, vUp;
	AngleVectors( qPortalAngles, NULL, &vRight, &vUp );

	Vector vTopEdge = vUp * PORTAL_HALF_HEIGHT;
	Vector vBottomEdge = -vTopEdge;
	Vector vRightEdge = vRight * PORTAL_HALF_WIDTH;
	Vector vLeftEdge = -vRightEdge;

	Vector vTopLeft = ptPortalCenter + vTopEdge + vLeftEdge;
	Vector vTopRight = ptPortalCenter + vTopEdge + vRightEdge;
	Vector vBottomLeft = ptPortalCenter + vBottomEdge + vLeftEdge;
	Vector vBottomRight = ptPortalCenter + vBottomEdge + vRightEdge;

	// Make triangles
	pvTri1[ 0 ] = vTopRight;
	pvTri1[ 1 ] = vTopLeft;
	pvTri1[ 2 ] = vBottomLeft;

	pvTri2[ 0 ] = vTopRight;
	pvTri2[ 1 ] = vBottomLeft;
	pvTri2[ 2 ] = vBottomRight;
}

void UTIL_Portal_Triangles( const CProp_Portal *pPortal, Vector pvTri1[ 3 ], Vector pvTri2[ 3 ] )
{
	UTIL_Portal_Triangles( pPortal->GetAbsOrigin(), pPortal->GetAbsAngles(), pvTri1, pvTri2 );
}

float UTIL_Portal_DistanceThroughPortal( const CProp_Portal *pPortal, const Vector &vPoint1, const Vector &vPoint2 )
{
	return FastSqrt( UTIL_Portal_DistanceThroughPortalSqr( pPortal, vPoint1, vPoint2 ) );
}

float UTIL_Portal_DistanceThroughPortalSqr( const CProp_Portal *pPortal, const Vector &vPoint1, const Vector &vPoint2 )
{
	if ( !pPortal || !pPortal->m_bActivated )
		return -1.0f;

	CProp_Portal *pPortalLinked = pPortal->m_hLinkedPortal;
	if ( !pPortalLinked || !pPortalLinked->m_bActivated )
		return -1.0f;

	return vPoint1.DistToSqr( pPortal->GetAbsOrigin() ) + pPortalLinked->GetAbsOrigin().DistToSqr( vPoint2 );
}

float UTIL_Portal_ShortestDistance( const Vector &vPoint1, const Vector &vPoint2, CProp_Portal **pShortestDistPortal_Out /*= NULL*/, bool bRequireStraightLine /*= false*/ )
{
	return FastSqrt( UTIL_Portal_ShortestDistanceSqr( vPoint1, vPoint2, pShortestDistPortal_Out, bRequireStraightLine ) );
}

float UTIL_Portal_ShortestDistanceSqr( const Vector &vPoint1, const Vector &vPoint2, CProp_Portal **pShortestDistPortal_Out /*= NULL*/, bool bRequireStraightLine /*= false*/ )
{
	float fMinDist = vPoint1.DistToSqr( vPoint2 );	
	
	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount == 0 )
	{
		if( pShortestDistPortal_Out )
			*pShortestDistPortal_Out = NULL;

		return fMinDist;
	}
	CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();
	CProp_Portal *pShortestDistPortal = NULL;

	for( int i = 0; i != iPortalCount; ++i )
	{
		CProp_Portal *pTempPortal = pPortals[i];
		if( pTempPortal->m_bActivated )
		{
			CProp_Portal *pLinkedPortal = pTempPortal->m_hLinkedPortal.Get();
			if( pLinkedPortal != NULL )
			{
				Vector vPoint1Transformed = pTempPortal->MatrixThisToLinked() * vPoint1;

				float fDirectDist = vPoint1Transformed.DistToSqr( vPoint2 );
				if( fDirectDist < fMinDist )
				{
					//worth investigating further
					//find out if it's a straight line through the portal, or if we have to wrap around a corner
					float fPoint1TransformedDist = pLinkedPortal->m_plane_Origin.normal.Dot( vPoint1Transformed ) - pLinkedPortal->m_plane_Origin.dist;
					float fPoint2Dist = pLinkedPortal->m_plane_Origin.normal.Dot( vPoint2 ) - pLinkedPortal->m_plane_Origin.dist;

					bool bStraightLine = true;
					if( (fPoint1TransformedDist > 0.0f) || (fPoint2Dist < 0.0f) ) //straight line through portal impossible, part of the line has to backtrack to get to the portal surface
						bStraightLine = false;

					if( bStraightLine ) //if we're not already doing some crazy wrapping, find an intersection point
					{
						float fTotalDist = fPoint2Dist - fPoint1TransformedDist; //fPoint1TransformedDist is known to be negative
						Vector ptPlaneIntersection;

						if( fTotalDist != 0.0f )
						{
							float fInvTotalDist = 1.0f / fTotalDist;
							ptPlaneIntersection = (vPoint1Transformed * (fPoint2Dist * fInvTotalDist)) + (vPoint2 * ((-fPoint1TransformedDist) * fInvTotalDist));
						}
						else
						{
							ptPlaneIntersection = vPoint1Transformed;
						}

						Vector vRight, vUp;
						pLinkedPortal->GetVectors( NULL, &vRight, &vUp );
						
						Vector ptLinkedCenter = pLinkedPortal->GetAbsOrigin();
						Vector vCenterToIntersection = ptPlaneIntersection - ptLinkedCenter;
						float fRight = vRight.Dot( vCenterToIntersection );
						float fUp = vUp.Dot( vCenterToIntersection );

						float fAbsRight = fabsf( fRight );
						float fAbsUp = fabsf( fUp );
						if( (fAbsRight > PORTAL_HALF_WIDTH) ||
							(fAbsUp > PORTAL_HALF_HEIGHT) )
							bStraightLine = false;

						if( bStraightLine == false )
						{
							if( bRequireStraightLine )
								continue;

							//find the offending extent and shorten both extents to bring it into the portal quad
							float fNormalizer;
							if( fAbsRight > PORTAL_HALF_WIDTH )
							{
								fNormalizer = fAbsRight/PORTAL_HALF_WIDTH;

								if( fAbsUp > PORTAL_HALF_HEIGHT )
								{
									float fUpNormalizer = fAbsUp/PORTAL_HALF_HEIGHT;
									if( fUpNormalizer > fNormalizer )
										fNormalizer = fUpNormalizer;
								}
							}
							else
							{
								fNormalizer = fAbsUp/PORTAL_HALF_HEIGHT;
							}

							vCenterToIntersection *= (1.0f/fNormalizer);
							ptPlaneIntersection = ptLinkedCenter + vCenterToIntersection;

							float fWrapDist = vPoint1Transformed.DistToSqr( ptPlaneIntersection ) + vPoint2.DistToSqr( ptPlaneIntersection );
							if( fWrapDist < fMinDist )
							{
								fMinDist = fWrapDist;
								pShortestDistPortal = pTempPortal;
							}
						}
						else
						{
							//it's a straight shot from point 1 to 2 through the portal
							fMinDist = fDirectDist;
							pShortestDistPortal = pTempPortal;
						}
					}
					else
					{
						if( bRequireStraightLine )
							continue;

						//do some crazy wrapped line intersection algorithm

						//for now, just do the cheap and easy solution
						float fWrapDist = vPoint1.DistToSqr( pTempPortal->GetAbsOrigin() ) + pLinkedPortal->GetAbsOrigin().DistToSqr( vPoint2 );
						if( fWrapDist < fMinDist )
						{
							fMinDist = fWrapDist;
							pShortestDistPortal = pTempPortal;
						}
					}
				}
			}
		}
	}

	return fMinDist;
}

void UTIL_Portal_AABB( const CProp_Portal *pPortal, Vector &vMin, Vector &vMax )
{
	Vector vOrigin = pPortal->GetAbsOrigin();
	QAngle qAngles = pPortal->GetAbsAngles();

	Vector vOBBForward;
	Vector vOBBRight;
	Vector vOBBUp;

	AngleVectors( qAngles, &vOBBForward, &vOBBRight, &vOBBUp );

	//scale the extents to usable sizes
	vOBBForward *= PORTAL_HALF_DEPTH;
	vOBBRight *= PORTAL_HALF_WIDTH;
	vOBBUp *= PORTAL_HALF_HEIGHT;

	vOrigin -= vOBBForward + vOBBRight + vOBBUp;

	vOBBForward *= 2.0f;
	vOBBRight *= 2.0f;
	vOBBUp *= 2.0f;

	vMin = vMax = vOrigin;

	for( int i = 1; i != 8; ++i )
	{
		Vector ptTest = vOrigin;
		if( i & (1 << 0) ) ptTest += vOBBForward;
		if( i & (1 << 1) ) ptTest += vOBBRight;
		if( i & (1 << 2) ) ptTest += vOBBUp;

		if( ptTest.x < vMin.x ) vMin.x = ptTest.x;
		if( ptTest.y < vMin.y ) vMin.y = ptTest.y;
		if( ptTest.z < vMin.z ) vMin.z = ptTest.z;
		if( ptTest.x > vMax.x ) vMax.x = ptTest.x;
		if( ptTest.y > vMax.y ) vMax.y = ptTest.y;
		if( ptTest.z > vMax.z ) vMax.z = ptTest.z;
	}
}

float UTIL_IntersectRayWithPortal( const Ray_t &ray, const CProp_Portal *pPortal )
{
	if ( !pPortal || !pPortal->m_bActivated )
	{
		return -1.0f;
	}

	Vector vForward;
	pPortal->GetVectors( &vForward, NULL, NULL );

	// Discount rays not coming from the front of the portal
	float fDot = DotProduct( vForward, ray.m_Delta );
	if ( fDot > 0.0f  )
	{
		return -1.0f;
	}

	Vector pvTri1[ 3 ], pvTri2[ 3 ];

	UTIL_Portal_Triangles( pPortal, pvTri1, pvTri2 );

	float fT;

	// Test triangle 1
	fT = IntersectRayWithTriangle( ray, pvTri1[ 0 ], pvTri1[ 1 ], pvTri1[ 2 ], false );

	// If there was an intersection return the T
	if ( fT >= 0.0f )
		return fT;

	// Return the result of collision with the other face triangle
	return IntersectRayWithTriangle( ray, pvTri2[ 0 ], pvTri2[ 1 ], pvTri2[ 2 ], false );
}

bool UTIL_IntersectRayWithPortalOBB( const CProp_Portal *pPortal, const Ray_t &ray, trace_t *pTrace )
{
	return IntersectRayWithOBB( ray, pPortal->GetAbsOrigin(), pPortal->GetAbsAngles(), CProp_Portal_Shared::vLocalMins, CProp_Portal_Shared::vLocalMaxs, 0.0f, pTrace );
}

bool UTIL_IntersectRayWithPortalOBBAsAABB( const CProp_Portal *pPortal, const Ray_t &ray, trace_t *pTrace )
{
	Vector vAABBMins, vAABBMaxs;

	UTIL_Portal_AABB( pPortal, vAABBMins, vAABBMaxs );

	return IntersectRayWithBox( ray, vAABBMins, vAABBMaxs, 0.0f, pTrace );
}

bool UTIL_IsBoxIntersectingPortal( const Vector &vecBoxCenter, const Vector &vecBoxExtents, const Vector &ptPortalCenter, const QAngle &qPortalAngles, float flTolerance )
{
	Vector pvTri1[ 3 ], pvTri2[ 3 ];

	UTIL_Portal_Triangles( ptPortalCenter, qPortalAngles, pvTri1, pvTri2 );

	cplane_t plane;

	ComputeTrianglePlane( pvTri1[ 0 ], pvTri1[ 1 ], pvTri1[ 2 ], plane.normal, plane.dist );
	plane.type = PLANE_ANYZ;
	plane.signbits = SignbitsForPlane( &plane );

	if ( IsBoxIntersectingTriangle( vecBoxCenter, vecBoxExtents, pvTri1[ 0 ], pvTri1[ 1 ], pvTri1[ 2 ], plane, flTolerance ) )
	{
		return true;
	}

	ComputeTrianglePlane( pvTri2[ 0 ], pvTri2[ 1 ], pvTri2[ 2 ], plane.normal, plane.dist );
	plane.type = PLANE_ANYZ;
	plane.signbits = SignbitsForPlane( &plane );

	return IsBoxIntersectingTriangle( vecBoxCenter, vecBoxExtents, pvTri2[ 0 ], pvTri2[ 1 ], pvTri2[ 2 ], plane, flTolerance );
}

bool UTIL_IsBoxIntersectingPortal( const Vector &vecBoxCenter, const Vector &vecBoxExtents, const CProp_Portal *pPortal, float flTolerance )
{
	if( pPortal == NULL )
		return false;

	return UTIL_IsBoxIntersectingPortal( vecBoxCenter, vecBoxExtents, pPortal->GetAbsOrigin(), pPortal->GetAbsAngles(), flTolerance );
}

CProp_Portal *UTIL_IntersectEntityExtentsWithPortal( const CBaseEntity *pEntity )
{
	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount == 0 )
		return NULL;

	Vector vMin, vMax;
	pEntity->CollisionProp()->WorldSpaceAABB( &vMin, &vMax );
	Vector ptCenter = ( vMin + vMax ) * 0.5f;
	Vector vExtents = ( vMax - vMin ) * 0.5f;

	CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();
	for( int i = 0; i != iPortalCount; ++i )
	{
		CProp_Portal *pTempPortal = pPortals[i];
		if( pTempPortal->m_bActivated && 
			(pTempPortal->m_hLinkedPortal.Get() != NULL) &&
			UTIL_IsBoxIntersectingPortal( ptCenter, vExtents, pTempPortal )	)
		{
			return pPortals[i];
		}
	}

	return NULL;
}

void UTIL_Portal_NDebugOverlay( const Vector &ptPortalCenter, const QAngle &qPortalAngles, int r, int g, int b, int a, bool noDepthTest, float duration )
{
#ifndef CLIENT_DLL
	Vector pvTri1[ 3 ], pvTri2[ 3 ];

	UTIL_Portal_Triangles( ptPortalCenter, qPortalAngles, pvTri1, pvTri2 );

	NDebugOverlay::Triangle( pvTri1[ 0 ], pvTri1[ 1 ], pvTri1[ 2 ], r, g, b, a, noDepthTest, duration );
	NDebugOverlay::Triangle( pvTri2[ 0 ], pvTri2[ 1 ], pvTri2[ 2 ], r, g, b, a, noDepthTest, duration );
#endif //#ifndef CLIENT_DLL
}

void UTIL_Portal_NDebugOverlay( const CProp_Portal *pPortal, int r, int g, int b, int a, bool noDepthTest, float duration )
{
#ifndef CLIENT_DLL
	UTIL_Portal_NDebugOverlay( pPortal->GetAbsOrigin(), pPortal->GetAbsAngles(), r, g, b, a, noDepthTest, duration );
#endif //#ifndef CLIENT_DLL
}


bool FindClosestPassableSpace( CBaseEntity *pEntity, const Vector &vIndecisivePush, unsigned int fMask ) //assumes the object is already in a mostly passable space
{
	if ( sv_use_find_closest_passable_space.GetBool() == false )
		return true;

	// Don't ever do this to entities with a move parent
	if ( pEntity->GetMoveParent() )
		return true;

#ifndef CLIENT_DLL
	ADD_DEBUG_HISTORY( HISTORY_PLAYER_DAMAGE, UTIL_VarArgs( "RUNNING FIND CLOSEST PASSABLE SPACE on %s..\n", pEntity->GetDebugName() ) );
#endif

	Vector ptExtents[8]; //ordering is going to be like 3 bits, where 0 is a min on the related axis, and 1 is a max on the same axis, axis order x y z

	float fExtentsValidation[8]; //some points are more valid than others, and this is our measure


	Vector vEntityMaxs;// = pEntity->WorldAlignMaxs();
	Vector vEntityMins;// = pEntity->WorldAlignMins();
	CCollisionProperty *pEntityCollision = pEntity->CollisionProp();
	pEntityCollision->WorldSpaceAABB( &vEntityMins, &vEntityMaxs );



	Vector ptEntityCenter = ((vEntityMins + vEntityMaxs) / 2.0f);
	vEntityMins -= ptEntityCenter;
	vEntityMaxs -= ptEntityCenter;

	Vector ptEntityOriginalCenter = ptEntityCenter;
	
	ptEntityCenter.z += 0.001f; //to satisfy m_IsSwept on first pass

	int iEntityCollisionGroup = pEntity->GetCollisionGroup();

	trace_t traces[2];
	Ray_t entRay;
	//entRay.Init( ptEntityCenter, ptEntityCenter, vEntityMins, vEntityMaxs );
	entRay.m_Extents = vEntityMaxs;
	entRay.m_IsRay = false;
	entRay.m_IsSwept = true;
	entRay.m_StartOffset = vec3_origin;

	Vector vOriginalExtents = vEntityMaxs;	

	Vector vGrowSize = vEntityMaxs / 101.0f;
	vEntityMaxs -= vGrowSize;
	vEntityMins += vGrowSize;
	
	
	Ray_t testRay;
	testRay.m_Extents = vGrowSize;
	testRay.m_IsRay = false;
	testRay.m_IsSwept = true;
	testRay.m_StartOffset = vec3_origin;



	unsigned int iFailCount;
	for( iFailCount = 0; iFailCount != 100; ++iFailCount )
	{
		entRay.m_Start = ptEntityCenter;
		entRay.m_Delta = ptEntityOriginalCenter - ptEntityCenter;

		UTIL_TraceRay( entRay, fMask, pEntity, iEntityCollisionGroup, &traces[0] );
		if( traces[0].startsolid == false )
		{
			Vector vNewPos = traces[0].endpos + (pEntity->GetAbsOrigin() - ptEntityOriginalCenter);
#ifdef CLIENT_DLL
			pEntity->SetAbsOrigin( vNewPos );
#else
			pEntity->Teleport( &vNewPos, NULL, NULL );
#endif
			return true; //current placement worked
		}

		bool bExtentInvalid[8];
		for( int i = 0; i != 8; ++i )
		{
			fExtentsValidation[i] = 0.0f;
			ptExtents[i] = ptEntityCenter;
			ptExtents[i].x += ((i & (1<<0)) ? vEntityMaxs.x : vEntityMins.x);
			ptExtents[i].y += ((i & (1<<1)) ? vEntityMaxs.y : vEntityMins.y);
			ptExtents[i].z += ((i & (1<<2)) ? vEntityMaxs.z : vEntityMins.z);

			bExtentInvalid[i] = enginetrace->PointOutsideWorld( ptExtents[i] );
		}

		unsigned int counter, counter2;
		for( counter = 0; counter != 7; ++counter )
		{
			for( counter2 = counter + 1; counter2 != 8; ++counter2 )
			{

				testRay.m_Delta = ptExtents[counter2] - ptExtents[counter];
				
				if( bExtentInvalid[counter] )
					traces[0].startsolid = true;
				else
				{
					testRay.m_Start = ptExtents[counter];
					UTIL_TraceRay( testRay, fMask, pEntity, iEntityCollisionGroup, &traces[0] );
				}

				if( bExtentInvalid[counter2] )
					traces[1].startsolid = true;
				else
				{
					testRay.m_Start = ptExtents[counter2];
					testRay.m_Delta = -testRay.m_Delta;
					UTIL_TraceRay( testRay, fMask, pEntity, iEntityCollisionGroup, &traces[1] );
				}

				float fDistance = testRay.m_Delta.Length();

				for( int i = 0; i != 2; ++i )
				{
					int iExtent = (i==0)?(counter):(counter2);

					if( traces[i].startsolid )
					{
						fExtentsValidation[iExtent] -= 100.0f;
					}
					else
					{
						fExtentsValidation[iExtent] += traces[i].fraction * fDistance;
					}
				}
			}
		}

		Vector vNewOriginDirection( 0.0f, 0.0f, 0.0f );
		float fTotalValidation = 0.0f;
		for( counter = 0; counter != 8; ++counter )
		{
			if( fExtentsValidation[counter] > 0.0f )
			{
				vNewOriginDirection += (ptExtents[counter] - ptEntityCenter) * fExtentsValidation[counter];
				fTotalValidation += fExtentsValidation[counter];
			}
		}

		if( fTotalValidation != 0.0f )
		{
			ptEntityCenter += (vNewOriginDirection / fTotalValidation);

			//increase sizing
			testRay.m_Extents += vGrowSize;
			vEntityMaxs -= vGrowSize;
			vEntityMins = -vEntityMaxs;
		}
		else
		{
			//no point was valid, apply the indecisive vector
			ptEntityCenter += vIndecisivePush;

			//reset sizing
			testRay.m_Extents = vGrowSize;
			vEntityMaxs = vOriginalExtents;
			vEntityMins = -vEntityMaxs;
		}		
	}

	// X360TBD: Hits in portal devtest
	AssertMsg( IsX360() || iFailCount != 100, "FindClosestPassableSpace() failure." );
	return false;
}

bool UTIL_Portal_EntityIsInPortalHole( const CProp_Portal *pPortal, CBaseEntity *pEntity )
{
	CCollisionProperty *pCollisionProp = pEntity->CollisionProp();
	Vector vMins = pCollisionProp->OBBMins();
	Vector vMaxs = pCollisionProp->OBBMaxs();
	Vector vForward, vUp, vRight;
	AngleVectors( pCollisionProp->GetCollisionAngles(), &vForward, &vRight, &vUp );
	Vector ptOrigin = pEntity->GetAbsOrigin();

	Vector ptOBBCenter = pEntity->GetAbsOrigin() + (vMins + vMaxs * 0.5f);
	Vector vExtents = (vMaxs - vMins) * 0.5f;

	vForward *= vExtents.x;
	vRight *= vExtents.y;
	vUp *= vExtents.z;

	Vector vPortalForward, vPortalRight, vPortalUp;
	pPortal->GetVectors( &vPortalForward, &vPortalRight, &vPortalUp );
	Vector ptPortalCenter = pPortal->GetAbsOrigin();

	return OBBHasFullyContainedIntersectionWithQuad( vForward, vRight, vUp, ptOBBCenter, 
		vPortalForward, vPortalForward.Dot( ptPortalCenter ), ptPortalCenter, 
		vPortalRight, PORTAL_HALF_WIDTH + 1.0f, vPortalUp, PORTAL_HALF_HEIGHT + 1.0f );
}


//Extends a radius through portals. For various radius effects like explosions and pushing
void ExtendRadiusThroughPortals(const Vector& vecOrigin, const QAngle& vecAngles, float flRadius, PortalRadiusExtensionVector& portalRadiusExtensions)
{
	//Always add the original point
	PortalRadiusExtension_t extension;
	extension.pPortalFrom = NULL;
	extension.pPortalTo = NULL;
	extension.vecOrigin = vecOrigin;
	extension.vecAngles = vecAngles;
	portalRadiusExtensions.AddToTail(extension);

	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if (iPortalCount != 0)
	{
		CProp_Portal** pAllPortals = CProp_Portal_Shared::AllPortals.Base();
		CUtlVector<CProp_Portal*> testedPortals; //The portals that have already been tested
		for (int i = 0; i != iPortalCount; ++i)
		{
			CProp_Portal* pPortal = pAllPortals[i];

			//If this portal exists and is linked
			if (!pPortal || !pPortal->IsActivedAndLinked())
			{
				continue;
			}

			//Check if this portal has already been tested
			int iTestedPortalsCount = testedPortals.Count();
			bool bTestThroughPortal = true;
			for (int j = 0; j < iTestedPortalsCount; ++j)
			{
				//If either this portal or it's linked partner has been tested, then don't test it again
				if (testedPortals[j] == pPortal || testedPortals[j] == pPortal->m_hLinkedPortal.Get())
				{
					bTestThroughPortal = false;
					break;
				}
			}

			//If we should test through this portal
			if (bTestThroughPortal)
			{
				//If this portal is near our radius
				float flDistSqr = pPortal->GetAbsOrigin().DistToSqr(vecOrigin);
				if (flDistSqr <= flRadius * flRadius)
				{
					//Check which portal is facing the origin.
					Vector vecOriginForward;
					AngleVectors(vecAngles, &vecOriginForward);

					Vector vecPortal1ToOrigin = pPortal->GetAbsOrigin() - vecOrigin;
					VectorNormalize(vecPortal1ToOrigin);

					Vector vecPortal2ToOrigin = pPortal->m_hLinkedPortal.Get()->GetAbsOrigin() - vecOrigin;
					VectorNormalize(vecPortal2ToOrigin);

					float flDot1 = 1.0f - abs(DotProduct(vecPortal1ToOrigin, vecOriginForward));
					float flDot2 = 1.0f - abs(DotProduct(vecPortal2ToOrigin, vecOriginForward));

					//Use the portal that is facing the origin for the transformations
					if (flDot2 < flDot1)
					{
						pPortal = pPortal->m_hLinkedPortal.Get();
					}

					VMatrix matThisToLinked = pPortal->MatrixThisToLinked();

					//Transform the center to the other side of the portal
					Vector vecTransformedOrigin;
					UTIL_Portal_PointTransform(matThisToLinked, vecOrigin, vecTransformedOrigin);

					//Transform the angles to the other side of the portal
					QAngle vecTransformedAngles;
					UTIL_Portal_AngleTransform(matThisToLinked, vecAngles, vecTransformedAngles);

					//Extend the radius through this portal
					PortalRadiusExtension_t extension;
					extension.pPortalFrom = pPortal;
					extension.pPortalTo = pPortal->m_hLinkedPortal.Get();
					extension.vecOrigin = vecTransformedOrigin;
					extension.vecAngles = vecTransformedAngles;
					portalRadiusExtensions.AddToTail(extension);

					//Add this portal to the list of tested portals
					testedPortals.AddToTail(pPortal);
				}
			}

		}// For all the portals in the level

	} //If there are portals in the level
}

void UTIL_DebugOverlay_Polyhedron(const CPolyhedron* pPolyhedron, int red, int green, int blue, bool noDepthTest, float flDuration, const matrix3x4_t* pTransform)
{
	const Vector* pPoints;
	if (pTransform)
	{
		Vector* pPointsTransform;
		pPoints = pPointsTransform = (Vector*)stackalloc(sizeof(Vector) * pPolyhedron->iVertexCount);

		for (int i = 0; i != pPolyhedron->iVertexCount; ++i)
		{
			VectorTransform(pPolyhedron->pVertices[i], *pTransform, pPointsTransform[i]);
		}
	}
	else
	{
		pPoints = pPolyhedron->pVertices;
	}

	for (int i = 0; i != pPolyhedron->iLineCount; ++i)
	{
		NDebugOverlay::Line(pPoints[pPolyhedron->pLines[i].iPointIndices[0]], pPoints[pPolyhedron->pLines[i].iPointIndices[1]], red, green, blue, noDepthTest, flDuration);
	}
}


void UTIL_DebugOverlay_CPhysCollide(const CPhysCollide* pCollide, int red, int green, int blue, bool noDepthTest, float flDuration, const matrix3x4_t* pTransform)
{
	Vector* outVerts;
	int vertCount = physcollision->CreateDebugMesh(pCollide, &outVerts);
	int triCount = vertCount / 3;

	const Vector* pPoints;
	if (pTransform)
	{
		Vector* pPointsTransform;
		pPoints = pPointsTransform = (Vector*)stackalloc(sizeof(Vector) * vertCount);

		for (int i = 0; i != vertCount; ++i)
		{
			VectorTransform(outVerts[i], *pTransform, pPointsTransform[i]);
		}
	}
	else
	{
		pPoints = outVerts;
	}

	for (int i = 0; i != triCount; ++i)
	{
		int iStartVert = (i * 3);
		NDebugOverlay::Line(pPoints[iStartVert], pPoints[iStartVert + 1], red, green, blue, noDepthTest, flDuration);
		NDebugOverlay::Line(pPoints[iStartVert + 1], pPoints[iStartVert + 2], red, green, blue, noDepthTest, flDuration);
		NDebugOverlay::Line(pPoints[iStartVert + 2], pPoints[iStartVert], red, green, blue, noDepthTest, flDuration);
	}

	physcollision->DestroyDebugMesh(vertCount, outVerts);
}


bool UTIL_IsCollideableIntersectingPhysCollide(ICollideable* pCollideable, const CPhysCollide* pCollide, const Vector& vPhysCollideOrigin, const QAngle& qPhysCollideAngles)
{
	vcollide_t* pVCollide = modelinfo->GetVCollide(pCollideable->GetCollisionModel());
	trace_t Trace;

	if (pVCollide != NULL)
	{
		Vector ptEntityPosition = pCollideable->GetCollisionOrigin();
		QAngle qEntityAngles = pCollideable->GetCollisionAngles();

		for (int i = 0; i != pVCollide->solidCount; ++i)
		{
			physcollision->TraceCollide(ptEntityPosition, ptEntityPosition, pVCollide->solids[i], qEntityAngles, pCollide, vPhysCollideOrigin, qPhysCollideAngles, &Trace);

			if (Trace.startsolid)
				return true;
		}
	}
	else
	{
		//use AABB
		Vector vMins, vMaxs, ptCenter;
		pCollideable->WorldSpaceSurroundingBounds(&vMins, &vMaxs);
		ptCenter = (vMins + vMaxs) * 0.5f;
		vMins -= ptCenter;
		vMaxs -= ptCenter;
		physcollision->TraceBox(ptCenter, ptCenter, vMins, vMaxs, pCollide, vPhysCollideOrigin, qPhysCollideAngles, &Trace);

		return Trace.startsolid;
	}

	return false;
}

#ifdef GAME_DLL

bool CBrushEntityList::EnumEntity(IHandleEntity* pHandleEntity)
{
	if (!pHandleEntity)
		return true;

	CBaseEntity* pEntity = gEntList.GetBaseEntity(pHandleEntity->GetRefEHandle());
	if (!pEntity)
		return true;

	if (pEntity->IsBSPModel())
	{
		m_BrushEntitiesToPaint.AddToTail(pEntity);
	}

	return true;
}


void UTIL_FindBrushEntitiesInSphere(CBrushEntityList& brushEnum, const Vector& vCenter, float flRadius)
{
	Vector vExtents = flRadius * Vector(1.f, 1.f, 1.f);
	enginetrace->EnumerateEntities(vCenter - vExtents, vCenter + vExtents, &brushEnum);
}

#endif

#ifdef CLIENT_DLL
void UTIL_TransformInterpolatedAngle( CInterpolatedVar< QAngle > &qInterped, matrix3x4_t matTransform, bool bSkipNewest )
{
	int iHead = qInterped.GetHead();
	if( !qInterped.IsValidIndex( iHead ) )
		return;

#ifdef _DEBUG
	float fHeadTime;
	qInterped.GetHistoryValue( iHead, fHeadTime );
#endif

	float fTime;
	QAngle *pCurrent;
	int iCurrent;

	if( bSkipNewest )
		iCurrent = qInterped.GetNext( iHead );
	else
		iCurrent = iHead;

	while( (pCurrent = qInterped.GetHistoryValue( iCurrent, fTime )) != NULL )
	{
		Assert( (fTime <= fHeadTime) || (iCurrent == iHead) ); //asserting that head is always newest

		if( fTime < gpGlobals->curtime )
			*pCurrent = TransformAnglesToWorldSpace( *pCurrent, matTransform );

		iCurrent = qInterped.GetNext( iCurrent );
		if( iCurrent == iHead )
			break;
	}

	qInterped.Interpolate( gpGlobals->curtime );
}

void UTIL_TransformInterpolatedPosition( CInterpolatedVar< Vector > &vInterped, VMatrix matTransform, bool bSkipNewest )
{
	int iHead = vInterped.GetHead();
	if( !vInterped.IsValidIndex( iHead ) )
		return;

#ifdef _DEBUG
	float fHeadTime;
	vInterped.GetHistoryValue( iHead, fHeadTime );
#endif

	float fTime;
	Vector *pCurrent;
	int iCurrent;

	if( bSkipNewest )
		iCurrent = vInterped.GetNext( iHead );
	else
		iCurrent = iHead;

	while( (pCurrent = vInterped.GetHistoryValue( iCurrent, fTime )) != NULL )
	{
		Assert( (fTime <= fHeadTime) || (iCurrent == iHead) );

		if( fTime < gpGlobals->curtime )
			*pCurrent = matTransform * (*pCurrent);

		iCurrent = vInterped.GetNext( iCurrent );
		if( iCurrent == iHead )
			break;
	}

	vInterped.Interpolate( gpGlobals->curtime );
}
#endif


#ifndef CLIENT_DLL

void CC_Debug_FixMyPosition( void )
{
	CBaseEntity *pPlayer = UTIL_GetCommandClient();

	FindClosestPassableSpace( pPlayer, vec3_origin );
}

static ConCommand debug_fixmyposition("debug_fixmyposition", CC_Debug_FixMyPosition, "Runs FindsClosestPassableSpace() on player.", FCVAR_CHEAT );
#endif
