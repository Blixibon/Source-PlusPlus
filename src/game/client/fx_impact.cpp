//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//
#include "cbase.h"
#include "decals.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "fx.h"
#include "fx_impact.h"
#include "view.h"
#if defined(TF_CLIENT_DLL) || defined(TF_CLASSIC_CLIENT)
#include "cdll_util.h"
#endif
#include "engine/IStaticPropMgr.h"
#include "c_impact_effects.h"
#include "c_splash.h"
#include "particlemgr.h"
#include "model_types.h"
#include "iefx.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar  r_drawflecks( "r_drawflecks", "1" );
static ConVar  r_impacts_alt_orientation("r_impacts_alt_orientation", "1");
extern ConVar r_drawmodeldecals;

ConVar asw_burst_pipe_chance("asw_burst_pipe_chance", "0.25f");

ImpactSoundRouteFn g_pImpactSoundRouteFn = NULL;

//==========================================================================================================================
// RAGDOLL ENUMERATOR
//==========================================================================================================================
CRagdollEnumerator::CRagdollEnumerator( Ray_t& shot, int iDamageType )
{
	m_rayShot = shot;
	m_iDamageType = iDamageType;
	m_bHit = false;
}

IterationRetval_t CRagdollEnumerator::EnumElement( IHandleEntity *pHandleEntity )
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
	if ( pEnt == NULL )
		return ITERATION_CONTINUE;

	C_BaseAnimating *pModel = static_cast< C_BaseAnimating * >( pEnt );

	// If the ragdoll was created on this tick, then the forces were already applied on the server
	if ( pModel == NULL || WasRagdollCreatedOnCurrentTick( pEnt ) )
		return ITERATION_CONTINUE;

	IPhysicsObject *pPhysicsObject = pModel->VPhysicsGetObject();
	if ( pPhysicsObject == NULL )
		return ITERATION_CONTINUE;

	trace_t tr;
	enginetrace->ClipRayToEntity( m_rayShot, MASK_SHOT, pModel, &tr );

	if ( tr.fraction < 1.0 )
	{
		pModel->ImpactTrace( &tr, m_iDamageType, NULL );
		m_bHit = true;

		//FIXME: Yes?  No?
		return ITERATION_STOP;
	}

	return ITERATION_CONTINUE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool FX_AffectRagdolls( Vector vecOrigin, Vector vecStart, int iDamageType )
{
	// don't do this when lots of ragdolls are simulating
	if ( s_RagdollLRU.CountRagdolls(true) > 1 )
		return false;
	Ray_t shotRay;
	shotRay.Init( vecStart, vecOrigin );

	CRagdollEnumerator ragdollEnum( shotRay, iDamageType );
	partition->EnumerateElementsAlongRay( PARTITION_CLIENT_RESPONSIVE_EDICTS, shotRay, false, &ragdollEnum );

	return ragdollEnum.Hit();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void RagdollImpactCallback( const CEffectData &data )
{
	FX_AffectRagdolls( data.m_vOrigin, data.m_vStart, data.m_nDamageType );
}

DECLARE_CLIENT_EFFECT( "RagdollImpact", RagdollImpactCallback );

void ImpactDecalOriented(const Vector& vecOrigin, const Vector& vecStart, int iMaterial, int iDamageType, int iHitbox, C_BaseEntity* pEntity, const trace_t& tr, QAngle angles)
{
	// Setup our shot information
	Vector shotDir = vecOrigin - vecStart;
	Vector traceExt;
	VectorMA(vecStart, VectorNormalize(shotDir) + 8.0f, shotDir, traceExt);

	// vector perpendicular to the slash direction
	// so we can align the slash decal to that
	Vector vecPerp, vecUp;
	AngleVectors(angles, NULL, &vecPerp, &vecUp);

	const char* pchDecalName = GetImpactDecal(pEntity, iMaterial, iDamageType);
	int decalNumber = decalsystem->GetDecalIndexForName(pchDecalName);
	if (decalNumber == -1)
		return;

	bool bSkipDecal = false;

#if defined(TF_CLIENT_DLL) || defined(TF_CLASSIC_CLIENT)
	// Don't show blood decals if we're filtering them out (Pyro Goggles)
	if (IsLocalPlayerUsingVisionFilterFlags(TF_VISION_FILTER_PYRO) || UTIL_IsLowViolence())
	{
		if (V_strstr(pchDecalName, "Flesh"))
		{
			bSkipDecal = true;
		}
	}
#endif

	if (!bSkipDecal)
	{
		int modeltype = modelinfo->GetModelType(pEntity->GetModel());
		IClientRenderable* pRenderable = pEntity;
		ICollideable* pCollide = pEntity->CollisionProp();
		if ((pEntity->entindex() == 0) && (iHitbox != 0))
		{
			pCollide = staticpropmgr->GetStaticPropByIndex(iHitbox - 1);
			pRenderable = pCollide->GetIClientUnknown()->GetClientRenderable();
			modeltype = mod_studio;
		}

		switch (modeltype)
		{
		case mod_studio:
			{
				Ray_t ray;
				ray.Init(vecStart, traceExt);
			
				// Choose a new ray along which to project the decal based on
				// surface normal. This prevents decal skewing
				bool noPokethru = false;
				if ((pCollide->GetSolid() == SOLID_VPHYSICS) && !tr.startsolid && !tr.allsolid)
				{
					Vector temp;
					VectorSubtract(tr.endpos, tr.plane.normal, temp);
					ray.Init(tr.endpos, temp);
					noPokethru = true;
				}

				modelrender->AddDecal(pRenderable->GetModelInstance(), ray, vecUp, decalNumber, pRenderable->GetBody(), noPokethru);
			}
			break;

		case mod_brush:
			effects->DecalShoot(decalNumber, pEntity->entindex(), pRenderable->GetModel(), pRenderable->GetRenderOrigin(), pRenderable->GetRenderAngles(), vecOrigin, &vecPerp, 0);
			break;

		default:
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Impact( const Vector &vecOrigin, const Vector &vecStart, int iMaterial, int iDamageType, int iHitbox, C_BaseEntity *pEntity, trace_t &tr, int nFlags, int maxLODToDecal )
{
	VPROF( "Impact" );

	Assert ( pEntity );

	// Clear out the trace
	memset( &tr, 0, sizeof(trace_t));
	tr.fraction = 1.0f;

	// Setup our shot information
	Vector shotDir = vecOrigin - vecStart;
	Vector traceExt;
	VectorMA( vecStart, VectorNormalize( shotDir ) + 8.0f, shotDir, traceExt );

	// Attempt to hit ragdolls
	
	bool bHitRagdoll = false;
	
	if ( !pEntity->IsClientCreated() )
	{
		bHitRagdoll = FX_AffectRagdolls( vecOrigin, vecStart, iDamageType );
	}

	if ( (nFlags & IMPACT_NODECAL) == 0 )
	{
		const char *pchDecalName = GetImpactDecal( pEntity, iMaterial, iDamageType );
		int decalNumber = decalsystem->GetDecalIndexForName( pchDecalName );
		if ( decalNumber == -1 )
			return false;

		bool bSkipDecal = false;

#if defined(TF_CLIENT_DLL) || defined(TF_CLASSIC_CLIENT)
		// Don't show blood decals if we're filtering them out (Pyro Goggles)
		if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) || UTIL_IsLowViolence() )
		{
			if ( V_strstr( pchDecalName, "Flesh" ) )
			{
				bSkipDecal = true;
			}
		}
#endif

		if ( !bSkipDecal )
		{
			if ( (pEntity->entindex() == 0) && (iHitbox != 0) )
			{
				staticpropmgr->AddDecalToStaticProp( vecStart, traceExt, iHitbox - 1, decalNumber, true, tr );
			}
			else if ( pEntity )
			{
				// Here we deal with decals on entities.
				pEntity->AddDecal( vecStart, traceExt, vecOrigin, iHitbox, decalNumber, true, tr, maxLODToDecal );
			}
		}
	}
	else
	{
		// Perform the trace ourselves
		Ray_t ray;
		ray.Init( vecStart, traceExt );

		if ( (pEntity->entindex() == 0) && (iHitbox != 0) )
		{
			// Special case for world entity with hitbox (that's a static prop)
			ICollideable *pCollideable = staticpropmgr->GetStaticPropByIndex( iHitbox - 1 ); 
			enginetrace->ClipRayToCollideable( ray, MASK_SHOT, pCollideable, &tr );
		}
		else
		{
			if ( !pEntity )
				return false;

			enginetrace->ClipRayToEntity( ray, MASK_SHOT, pEntity, &tr );
		}
	}

	// If we found the surface, emit debris flecks
	if ( tr.fraction == 1.0f || ( bHitRagdoll && (nFlags & IMPACT_REPORT_RAGDOLL_IMPACTS) == 0 ) )
		return false;

	return true;
}

//------------------------------------------------------------------------------
// Purpose : Create leak effect if material requests it
// Input   :
// Output  :
//------------------------------------------------------------------------------
void LeakEffect(trace_t& tr)
{
	if (RandomFloat() > asw_burst_pipe_chance.GetFloat())
		return;

	Vector			diffuseColor, baseColor;
	Vector			vTraceDir = (tr.endpos - tr.startpos);
	VectorNormalize(vTraceDir);
	Vector			vTraceStart = tr.endpos - 0.1 * vTraceDir;
	Vector			vTraceEnd = tr.endpos + 0.1 * vTraceDir;
	IMaterial* pTraceMaterial = engine->TraceLineMaterialAndLighting(vTraceStart, vTraceEnd, diffuseColor, baseColor);

	if (!pTraceMaterial)
		return;

	bool			found;
	IMaterialVar* pLeakVar = pTraceMaterial->FindVar("$leakamount", &found, false);
	if (!found)
		return;

	C_Splash* pLeak = new C_Splash();
	if (!pLeak)
		return;

	// VXP: Fix for crash when player shoots at leakable texture
//	ClientEntityList().AddNonNetworkableEntity( pLeak->GetIClientUnknown() ); // VXP: Commented because causes crash at game shutdown
	// VXP: Taken from c_fire_smoke.cpp
/*	m_Partition = partition->CreateHandle( pLeak->GetIClientUnknown() );
	view->AddVisibleEntity( pLeak );*/

	IMaterialVar* pLeakColorVar = pTraceMaterial->FindVar("$leakcolor", &found);
	if (found)
	{
		Vector color;
		pLeakColorVar->GetVecValue(color.Base(), 3);
		pLeak->m_vStartColor = pLeak->m_vEndColor = color;
	}

	IMaterialVar* pLeakNoiseVar = pTraceMaterial->FindVar("$leaknoise", &found);
	if (found)
	{
		pLeak->m_flNoise = pLeakNoiseVar->GetFloatValue();
	}

	IMaterialVar* pLeakForceVar = pTraceMaterial->FindVar("$leakforce", &found);
	if (found)
	{
		float flForce = pLeakForceVar->GetFloatValue();
		pLeak->m_flSpeed = flForce;
		pLeak->m_flSpeedRange = pLeak->m_flNoise * flForce;
	}

	pLeak->m_flSpawnRate = pLeakVar->GetFloatValue();;
	pLeak->m_flParticleLifetime = 10;
	pLeak->m_flWidthMin = 1;
	pLeak->m_flWidthMax = 5;
	pLeak->SetLocalOrigin(tr.endpos);

	QAngle angles;
	VectorAngles(tr.plane.normal, angles);
	pLeak->SetLocalAngles(angles);

	pLeak->Start(ParticleMgr(), NULL);
	pLeak->m_flStopEmitTime = gpGlobals->curtime + 5.0;
	pLeak->SetNextClientThink(gpGlobals->curtime + 20.0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char const *GetImpactDecal( C_BaseEntity *pEntity, int iMaterial, int iDamageType )
{
	char const *decalName;
	if ( !pEntity )
	{
		decalName = "Impact.Concrete";
	}
	else
	{
		decalName = pEntity->DamageDecal( iDamageType, iMaterial );
	}

	// See if we need to offset the decal for material type
	return decalsystem->TranslateDecalForGameMaterial( decalName, iMaterial );
}

//-----------------------------------------------------------------------------
// Purpose: Perform custom effects based on the Decal index
//-----------------------------------------------------------------------------
static ConVar cl_new_impact_effects( "cl_new_impact_effects", "0" );

struct ImpactEffect_t
{
	const char *m_pName;
	const char *m_pNameNoFlecks;
};

static ImpactEffect_t s_pImpactEffect[26] = 
{
	{ "impact_antlion",		NULL },							// CHAR_TEX_ANTLION
	{ NULL,					NULL },							// CHAR_TEX_BLOODYFLESH
	{ "impact_concrete",	"impact_concrete_noflecks" },	// CHAR_TEX_CONCRETE
	{ "impact_dirt",		NULL },							// CHAR_TEX_DIRT
	{ NULL,					NULL },							// CHAR_TEX_EGGSHEL
	{ NULL,					NULL },							// CHAR_TEX_FLESH
	{ "impact_metal",		"impact_metal" },				// CHAR_TEX_GRATE
	{ NULL,					NULL },							// CHAR_TEX_ALIENFLESH
	{ NULL,					NULL },							// CHAR_TEX_CLIP
	{ NULL,					NULL },							// CHAR_TEX_UNUSED
	{ NULL,					NULL },							// CHAR_TEX_UNUSED
	{ "impact_plastic",		"impact_plastic" },				// CHAR_TEX_PLASTIC
	{ "impact_metal",		"impact_metal" },				// CHAR_TEX_METAL
	{ "impact_sand",		"impact_sand" },				// CHAR_TEX_SAND
	{ "impact_leaves",		"impact_leaves" },				// CHAR_TEX_FOLIAGE
	{ "impact_computer",	"impact_computer" },			// CHAR_TEX_COMPUTER
	{ NULL,					NULL },							// CHAR_TEX_UNUSED
	{ NULL,					NULL },							// CHAR_TEX_UNUSED
	{ "impact_wet",			"impact_wet" },					// CHAR_TEX_SLOSH
	{ "impact_concrete",	"impact_concrete_noflecks" },	// CHAR_TEX_TILE
	{ NULL,					NULL },							// CHAR_TEX_UNUSED
	{ "impact_metal",		NULL },							// CHAR_TEX_VENT
	{ "impact_wood",		"impact_wood_noflecks" },		// CHAR_TEX_WOOD
	{ NULL,					NULL },							// CHAR_TEX_UNUSED
	{ "impact_glass",		NULL },							// CHAR_TEX_GLASS
	{ "warp_shield_impact", NULL },							// CHAR_TEX_WARPSHIELD
};

static ImpactEffect_t s_pImpactEffect2[12] =
{
	{ "impact_clay",		"impact_clay" },		// CHAR_TEX_CLAY
	{ "impact_plaster",		"impact_plaster" },		// CHAR_TEX_PLASTER
	{ "impact_rock",		"impact_rock" },		// CHAR_TEX_ROCK
	{ "impact_rubber",		"impact_rubber" },		// CHAR_TEX_RUBBER
	{ "impact_sheetrock",	"impact_sheetrock" },	// CHAR_TEX_SHEETROCK
	{ "impact_cloth",		"impact_cloth" },		// CHAR_TEX_CLOTH
	{ "impact_carpet",		"impact_carpet" },		// CHAR_TEX_CARPET
	{ "impact_paper",		"impact_paper" },		// CHAR_TEX_PAPER
	{ "impact_upholstery",	"impact_upholstery" },	// CHAR_TEX_UPHOLSTERY
	{ "impact_puddle",		"impact_puddle" },		// CHAR_TEX_PUDDLE
	{ "impact_metal",		"impact_metal" },		// CHAR_TEX_STEAM_PIPE
	{ "impact_sandbarrel",	"impact_sandbarrel" },	// CHAR_TEX_SANDBARREL
};

const char* s_pszBurstPipeEffects[] =
{
	"impact_steam",
	"impact_steam_small",
	"impact_steam_short"
};

void FX_ASW_Potential_Burst_Pipe(const Vector& vecImpactPoint, const Vector& vecReflect, const Vector& vecShotBackward, const Vector& vecNormal)
{
	if (RandomFloat() > asw_burst_pipe_chance.GetFloat())
		return;

	const char* szEffectName = s_pszBurstPipeEffects[RandomInt(0, NELEMS(s_pszBurstPipeEffects) - 1)];
	CSmartPtr<CNewParticleEffect> pSteamEffect = CNewParticleEffect::Create(NULL, szEffectName);
	if (pSteamEffect.IsValid())
	{
		Vector vecImpactY, vecImpactZ;
		VectorVectors(vecNormal, vecImpactY, vecImpactZ);
		vecImpactY *= -1.0f;

		pSteamEffect->SetControlPoint(0, vecImpactPoint);
		pSteamEffect->SetControlPointOrientation(0, vecImpactZ, vecImpactY, vecNormal);
	}
}

static void SetImpactControlPoint( CNewParticleEffect *pEffect, int nPoint, const Vector &vecImpactPoint, const Vector &vecForward, C_BaseEntity *pEntity )
{
	Vector vecImpactY, vecImpactZ;
	VectorVectors(vecForward, vecImpactY, vecImpactZ);
	vecImpactY *= -1.0f;

	pEffect->SetControlPoint(nPoint, vecImpactPoint);

	if (r_impacts_alt_orientation.GetBool())
		pEffect->SetControlPointOrientation(nPoint, vecImpactZ, vecImpactY, vecForward);
	else
		pEffect->SetControlPointOrientation(nPoint, vecForward, vecImpactY, vecImpactZ);
	pEffect->SetControlPointEntity(nPoint, pEntity);
}

static bool PerformNewCustomEffects( const Vector &vecOrigin, trace_t &tr, const Vector &shotDir, int iMaterial, int iScale, int nFlags )
{
	bool bNoFlecks = !r_drawflecks.GetBool();
	if ( !bNoFlecks )
	{
		bNoFlecks = ( ( nFlags & FLAGS_CUSTIOM_EFFECTS_NOFLECKS ) != 0  );
	}

	// Compute the impact effect name
	ImpactEffect_t* pEffectList;
	//int* pEffectIndex;
	int nOffset;
	if (iMaterial >= FIRST_L4D_CHAR_TEX && iMaterial <= LAST_L4D_CHAR_TEX)
	{
		pEffectList = s_pImpactEffect2;
		nOffset = 1;
		//pEffectIndex = s_pImpactEffect2Index[iMaterial - nOffset];
	}
	else if ((iMaterial >= FIRST_CHAR_TEX) && (iMaterial <= LAST_CHAR_TEX))
	{
		pEffectList = s_pImpactEffect;
		nOffset = FIRST_CHAR_TEX;
		//pEffectIndex = s_pImpactEffectIndex[iMaterial - nOffset];
	}
	else
	{
		DevMsg("Invalid surface property.  Double-check surfaceproperties_manifest.txt\n");
		return false;
	}

	const ImpactEffect_t& effect = pEffectList[iMaterial - nOffset];

	const char* pImpactName = effect.m_pName;
	//int nEffectIndex = pEffectIndex[0];
	if (bNoFlecks && effect.m_pNameNoFlecks)
	{
		pImpactName = effect.m_pNameNoFlecks;
		//nEffectIndex = pEffectIndex[1];
	}
	if ( !pImpactName )
		return false;

	Vector	vecReflect;
	VectorMA(shotDir, -2.0f * DotProduct(shotDir, tr.plane.normal), tr.plane.normal, vecReflect);

	Vector vecShotBackward;
	VectorMultiply(shotDir, -1.0f, vecShotBackward);

	const Vector& vecImpactPoint = (tr.fraction != 1.0f) ? tr.endpos : vecOrigin;
	//Assert( VectorsAreEqual( vecOrigin, tr.endpos, 1e-1 ) );

	if (iMaterial == CHAR_TEX_STEAM_PIPE)
	{
		FX_ASW_Potential_Burst_Pipe(vecImpactPoint, vecReflect, vecShotBackward, tr.plane.normal);
	}

	CSmartPtr<CNewParticleEffect> pEffect = CNewParticleEffect::Create( NULL, pImpactName );
	if ( !pEffect->IsValid() )
		return false;

	SetImpactControlPoint( pEffect.GetObject(), 0, vecImpactPoint, tr.plane.normal, tr.m_pEnt ); 
	SetImpactControlPoint( pEffect.GetObject(), 1, vecImpactPoint, vecReflect,		tr.m_pEnt ); 
	SetImpactControlPoint( pEffect.GetObject(), 2, vecImpactPoint, vecShotBackward,	tr.m_pEnt ); 
	pEffect->SetControlPoint( 3, Vector( iScale, iScale, iScale ) );
	if ( pEffect->m_pDef->ReadsControlPoint( 4 ) )
	{
		Vector vecColor;
		GetColorForSurface( &tr, &vecColor );
		pEffect->SetControlPoint( 4, vecColor );
	}
	return true;
}

void PerformCustomEffects( const Vector &vecOrigin, trace_t &tr, const Vector &shotDir, int iMaterial, int iScale, int nFlags )
{
	// Throw out the effect if any of these are true
	if ( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) )
		return;

	//---------------------------
	// Do leak effect
	//---------------------------
	LeakEffect(tr);

	if ( cl_new_impact_effects.GetBool() && PerformNewCustomEffects( vecOrigin, tr, shotDir, iMaterial, iScale, nFlags ) )
		return;

	bool bNoFlecks = !r_drawflecks.GetBool();
	if ( !bNoFlecks )
	{
		bNoFlecks = ( ( nFlags & FLAGS_CUSTIOM_EFFECTS_NOFLECKS ) != 0  );
	}

	if (iMaterial >= FIRST_L4D_CHAR_TEX && iMaterial <= LAST_L4D_CHAR_TEX)
	{
		PerformNewCustomEffects(vecOrigin, tr, shotDir, iMaterial, iScale, nFlags);
	}
	// Cement and wood have dust and flecks
	else if ( ( iMaterial == CHAR_TEX_CONCRETE ) || ( iMaterial == CHAR_TEX_TILE ) )
	{
		FX_DebrisFlecks( vecOrigin, &tr, iMaterial, iScale, bNoFlecks );
	}
	else if ( iMaterial == CHAR_TEX_WOOD )
	{
		FX_DebrisFlecks( vecOrigin, &tr, iMaterial, iScale, bNoFlecks );
	}
	else if ( ( iMaterial == CHAR_TEX_DIRT ) || ( iMaterial == CHAR_TEX_SAND ) )
	{
		FX_DustImpact( vecOrigin, &tr, iScale );
	}
	else if ( iMaterial == CHAR_TEX_ANTLION )
	{
		FX_AntlionImpact( vecOrigin, &tr );
	}
	else if ( ( iMaterial == CHAR_TEX_METAL ) || ( iMaterial == CHAR_TEX_VENT ) )
	{
		const Vector reflect = shotDir + ( tr.plane.normal * ( shotDir.Dot( tr.plane.normal ) * -2.0f ) ) + RandomVector( -0.2f, 0.2f );

		FX_MetalSpark( vecOrigin, reflect, tr.plane.normal, iScale );
	}
	else if ( iMaterial == CHAR_TEX_COMPUTER )
	{
		Vector	offset = vecOrigin + ( tr.plane.normal * 1.0f );
		
		g_pEffects->Sparks( offset );
	}
	else if ( iMaterial == CHAR_TEX_WARPSHIELD )
	{
		QAngle vecAngles;
		VectorAngles( -shotDir, vecAngles );
		DispatchParticleEffect( "warp_shield_impact", vecOrigin, vecAngles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a sound for an impact. If tr contains a valid hit, use that. 
//			If not, use the passed in origin & surface.
//-----------------------------------------------------------------------------
void PlayImpactSound( CBaseEntity *pEntity, trace_t &tr, const Vector &vecServerOrigin, int nServerSurfaceProp )
{
	VPROF( "PlayImpactSound" );
	Vector vecOrigin;

	// If the client-side trace hit a different entity than the server, or
	// the server didn't specify a surfaceprop, then use the client-side trace 
	// material if it's valid.
	if ( tr.DidHit() && (pEntity != tr.m_pEnt || nServerSurfaceProp == 0) )
	{
		nServerSurfaceProp = tr.surface.surfaceProps;
	}
	surfacedata_t* const pdata = physprops->GetSurfaceData( nServerSurfaceProp );
	if ( tr.fraction < 1.0 )
	{
		vecOrigin = tr.endpos;
	}
	else
	{
		vecOrigin = vecServerOrigin;
	}

	// Now play the esound
	if ( pdata->sounds.bulletImpact )
	{
		const char *pbulletImpactSoundName = physprops->GetString( pdata->sounds.bulletImpact );
		
		if ( g_pImpactSoundRouteFn )
		{
			g_pImpactSoundRouteFn( pbulletImpactSoundName, vecOrigin );
		}
		else
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound( filter, NULL, pbulletImpactSoundName, pdata->soundhandles.bulletImpact, &vecOrigin );
		}

		return;
	}

#ifdef _DEBUG
	Msg("***ERROR: PlayImpactSound() on a surface with 0 bulletImpactCount!\n");
#endif //_DEBUG
}


void SetImpactSoundRoute( ImpactSoundRouteFn fn )
{
	g_pImpactSoundRouteFn = fn;
}


//-----------------------------------------------------------------------------
// Purpose: Pull the impact data out
// Input  : &data - 
//			*vecOrigin - 
//			*vecAngles - 
//			*iMaterial - 
//			*iDamageType - 
//			*iHitbox - 
//			*iEntIndex - 
//-----------------------------------------------------------------------------
C_BaseEntity *ParseImpactData( const CEffectData &data, Vector *vecOrigin, Vector *vecStart, 
	Vector *vecShotDir, short &nSurfaceProp, int &iMaterial, int &iDamageType, int &iHitbox )
{
	C_BaseEntity *pEntity = data.GetEntity( );
	*vecOrigin = data.m_vOrigin;
	*vecStart = data.m_vStart;
	nSurfaceProp = data.m_nSurfaceProp;
	iDamageType = data.m_nDamageType;
	iHitbox = data.m_nHitBox;

	*vecShotDir = (*vecOrigin - *vecStart);
	VectorNormalize( *vecShotDir );

	// Get the material from the surfaceprop
	surfacedata_t *psurfaceData = physprops->GetSurfaceData( data.m_nSurfaceProp );
	iMaterial = psurfaceData->game.material;

	return pEntity;
}

