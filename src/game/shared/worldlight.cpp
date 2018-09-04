//========= Copyright (C) 2011, CSProMod Team, All rights reserved. =========//
//
// Purpose: provide world light related functions to the client
//
// As the engine provides no access to brush/model data (brushdata_t, model_t),
// we hence have no access to dworldlight_t. Therefore, we manually extract the
// world light data from the BSP itself, before entities are initialised on map
// load.
//
// To find the brightest light at a point, all world lights are iterated.
// Lights whose radii do not encompass our sample point are quickly rejected,
// as are lights which are not in our PVS, or visible from the sample point.
// If the sky light is visible from the sample point, then it shall supersede
// all other world lights.
//
// Written: November 2011
// Author: Saul Rennison
//
//===========================================================================//

#include "cbase.h"
#include "worldlight.h"
#include "bspfile.h"
#include "filesystem.h"
#include "map_load_helper.h"

#include "tier0/memdbgon.h"

//static IVEngineServer *g_pEngineServer = NULL;

#ifdef GAME_DLL
#define VarArgs UTIL_VarArgs
#endif

#ifdef CLIENT_DLL
extern int GetClusterForOrigin( const Vector& org );
extern int GetPVSForCluster( int clusterIndex, int outputpvslength, byte* outputpvs );
extern bool CheckOriginInPVS( const Vector& org, const byte* checkpvs, int checkpvssize );
#else
#define GetClusterForOrigin engine->GetClusterForOrigin
#define GetPVSForCluster engine->GetPVSForCluster
#define CheckOriginInPVS engine->CheckOriginInPVS
#endif
extern int GetLeafForOrigin(const Vector& org);

//-----------------------------------------------------------------------------
// Singleton exposure
//-----------------------------------------------------------------------------
static CWorldLights s_WorldLights;
CWorldLights* g_pWorldLights = &s_WorldLights;

//-----------------------------------------------------------------------------
// Purpose: calculate intensity ratio for a worldlight by distance
// Author: Valve Software
//-----------------------------------------------------------------------------
static float Engine_WorldLightDistanceFalloff( const dworldlight_t& wl, const Vector& delta )
{
	switch ( wl.type )
	{
	case emit_surface:
		// Cull out stuff that's too far
		if ( wl.radius != 0 )
		{
			if ( DotProduct( delta, delta ) > ( wl.radius * wl.radius ) )
				return 0.0f;
		}

		return InvRSquared( delta );

	case emit_skylight:
	case emit_skyambient:
		return 1.f;

	case emit_quakelight:
	{
		// X - r;
		const float falloff = wl.linear_attn - FastSqrt( DotProduct( delta, delta ) );
		if ( falloff < 0 )
			return 0.f;

		return falloff;
	}

	case emit_point:
	case emit_spotlight:	// directional & positional
	{
		const float dist2 = DotProduct( delta, delta );
		const float dist = FastSqrt( dist2 );

		// Cull out stuff that's too far
		if ( wl.radius != 0 && dist > wl.radius )
			return 0.f;

		float scale = 1.f;
		if (wl.type == emit_spotlight)
		{
			Vector vecLightToPoint = -delta;
			VectorNormalize(vecLightToPoint);

			float flDot = DotProduct(vecLightToPoint, wl.normal);

			scale = RemapValClamped(flDot, wl.stopdot, wl.stopdot2, 1.f, 0.f);
		}

		return (1.f / ( wl.constant_attn + wl.linear_attn * dist + wl.quadratic_attn * dist2 )) * scale;
	}
	}

	return 1.f;
}

//-----------------------------------------------------------------------------
// Purpose: initialise game system and members
//-----------------------------------------------------------------------------
CWorldLights::CWorldLights() : CAutoGameSystemPerFrame( "World lights" )
{
	m_nWorldLights = 0;
	m_pWorldLights = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: clear worldlights, free memory
//-----------------------------------------------------------------------------
void CWorldLights::Clear()
{
	m_nWorldLights = 0;

	if ( m_pWorldLights )
	{
		delete[] m_pWorldLights;
		m_pWorldLights = NULL;
	}

	m_LeafCubes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: get all world lights from the BSP
//-----------------------------------------------------------------------------
void CWorldLights::LevelInitPreEntity()
{
	CMapLoadHelper::Init(VarArgs("maps/%s.bsp", MapName()));

	// Grab the light lump and seek to it
#ifdef GAME_DLL
	CMapLoadHelper ldrLump(LUMP_WORLDLIGHTS);
	CMapLoadHelper hdrLump(LUMP_WORLDLIGHTS_HDR);
	bool bHDR = (hdrLump.LumpSize() > 0);
	const CMapLoadHelper& lightLump = bHDR ? hdrLump : ldrLump;
#else
	bool bHDR = (g_pMaterialSystemHardwareConfig->GetHDREnabled() && engine->MapHasHDRLighting());

	const CMapLoadHelper lightLump(bHDR ? LUMP_WORLDLIGHTS_HDR : LUMP_WORLDLIGHTS);
#endif

	// If we can't divide the lump data into a whole number of worldlights,
	// then the BSP format changed and we're unaware
	if ( lightLump.LumpSize() % sizeof( dworldlight_t ) )
	{
		Warning( "[%s]CWorldLights: broken world light lump with size of %d, epcected to be multiple of %u\n", (CBaseEntity::IsServer() ? "SERVER" : "CLIENT"), lightLump.LumpSize(), sizeof( dworldlight_t ) );

		// Close file
		CMapLoadHelper::Shutdown();
		return;
	}

	// Allocate memory for the worldlights
	m_nWorldLights = lightLump.LumpSize() / sizeof( dworldlight_t );
	m_pWorldLights = new dworldlight_t[m_nWorldLights];

	// Read worldlights
	V_memcpy(m_pWorldLights, lightLump.LumpBase(), lightLump.LumpSize());
	DevMsg("[%s]CWorldLights: world lights load successful (%d lights at 0x%p)\n", (CBaseEntity::IsServer() ? "SERVER" : "CLIENT"), m_nWorldLights, m_pWorldLights);

	// Find ambient lighting
	const CMapLoadHelper leafLump(LUMP_LEAFS);
	switch (leafLump.LumpVersion())
	{
	case 0:
	{
		Assert(leafLump.LumpSize() % sizeof(dleaf_version_0_t));
		int nLeaves = leafLump.LumpSize() / sizeof(dleaf_version_0_t);
		dleaf_version_0_t *pLeaf = reinterpret_cast<dleaf_version_0_t*>(leafLump.LumpBase());
		m_LeafCubes.SetCount(nLeaves);

		for (int i = 0; i < nLeaves; i++, pLeaf++)
		{
			CubeVec &lightCubes = m_LeafCubes.Element(i);

			Vector mins, maxs;
			mins.Init(pLeaf->mins[0], pLeaf->mins[1], pLeaf->mins[2]);
			maxs.Init(pLeaf->maxs[0], pLeaf->maxs[1], pLeaf->maxs[2]);

			lightCube_t cube;
			cube.cube = pLeaf->m_AmbientLighting;
			VectorLerp(mins, maxs, 0.5f, cube.pos);
			lightCubes.AddToTail(cube);
		}
	}
	break;
	case 1:
	{
		Assert(leafLump.LumpSize() % sizeof(dleaf_t));
		int nLeaves = leafLump.LumpSize() / sizeof(dleaf_t);
		dleaf_t *pLeaf = reinterpret_cast<dleaf_t*>(leafLump.LumpBase());
		m_LeafCubes.SetCount(nLeaves);

		CMapLoadHelper indexLump(bHDR ? LUMP_LEAF_AMBIENT_INDEX_HDR : LUMP_LEAF_AMBIENT_INDEX);
		int nIndices = indexLump.LumpSize() / sizeof(dleafambientindex_t);
		dleafambientindex_t *pIndices = reinterpret_cast<dleafambientindex_t*>(indexLump.LumpBase());

		Assert(nIndices == nLeaves);

		CMapLoadHelper ambientLump(bHDR ? LUMP_LEAF_AMBIENT_LIGHTING_HDR : LUMP_LEAF_AMBIENT_LIGHTING);
		//int nCubes = ambientLump.LumpSize() / sizeof(dleafambientlighting_t);
		dleafambientlighting_t *pLighting = reinterpret_cast<dleafambientlighting_t*>(ambientLump.LumpBase());

		for (int i = 0; i < nLeaves; i++, pLeaf++)
		{
			CubeVec &lightCubes = m_LeafCubes.Element(i);

			Vector mins, maxs;
			mins.Init(pLeaf->mins[0], pLeaf->mins[1], pLeaf->mins[2]);
			maxs.Init(pLeaf->maxs[0], pLeaf->maxs[1], pLeaf->maxs[2]);

			dleafambientindex_t &ambientIndex = pIndices[i];
			lightCubes.SetCount(ambientIndex.ambientSampleCount);
			for (int j = 0; j < ambientIndex.ambientSampleCount; j++)
			{
				dleafambientlighting_t &ambientLight = pLighting[ambientIndex.firstAmbientSample + j];
				lightCube_t &lightCube = lightCubes.Element(j);
				lightCube.cube = ambientLight.cube;
				lightCube.pos.x = RemapVal(ambientLight.x, 0, 255, mins.x, maxs.x);
				lightCube.pos.y = RemapVal(ambientLight.y, 0, 255, mins.y, maxs.y);
				lightCube.pos.z = RemapVal(ambientLight.z, 0, 255, mins.z, maxs.z);
			}
		}
	}
	break;
	default:
		Assert(0);
		Warning("Unknown LUMP_LEAFS version\n");
		break;
	}

	CMapLoadHelper::Shutdown();
}

CWorldLights::LightVec CWorldLights::CollectLightsAtPoint(const Vector& vecPosition) const
{
	CUtlVector<collectedlight_t> vecLights;

	if (!m_nWorldLights || !m_pWorldLights)
		return vecLights;

	// Find the size of the PVS for our current position
	const int nCluster = GetClusterForOrigin(vecPosition);
	const int nPVSSize = GetPVSForCluster(nCluster, 0, NULL);

	// Get the PVS at our position
	byte* pvs = new byte[nPVSSize];
	GetPVSForCluster(nCluster, nPVSSize, pvs);

	// Iterate through all the worldlights
	for (int i = 0; i < m_nWorldLights; ++i)
	{
		const dworldlight_t& light = m_pWorldLights[i];

		// Skip skyambient
		if (light.type == emit_skyambient)
			continue;

		// Handle sun
		if (light.type == emit_skylight)
		{
			if (light.intensity.LengthSqr() <= 0.f)
				continue;

			const Vector& pos = vecPosition - light.normal * MAX_TRACE_LENGTH;

			trace_t tr;
			UTIL_TraceLine(vecPosition, pos, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);

			if (!tr.DidHit())
				continue;

			if (!(tr.surface.flags & SURF_SKY) && !(tr.surface.flags & SURF_SKY2D))
				continue;

			collectedlight_t col;
			col.pLight = &light;
			col.intensity = light.intensity;
			col.pos = pos;
			vecLights.AddToTail(col);
			continue;
		}

		// Calculate square distance to this worldlight
		const Vector& vecDelta = light.origin - vecPosition;
		const float flDistSqr = vecDelta.LengthSqr();
		const float flRadiusSqr = light.radius * light.radius;

		// Skip lights that are out of our radius
		if (light.type == emit_spotlight)
		{
			Vector vecLightToPoint = -vecDelta;
			VectorNormalize(vecLightToPoint);

			float flDot = DotProduct(vecLightToPoint, light.normal);

			if (flDot < light.stopdot2)
				continue;
		}
		else if (flRadiusSqr > 0 && flDistSqr >= flRadiusSqr)
			continue;

		// Is it out of our PVS?
		if (!CheckOriginInPVS(light.origin, pvs, nPVSSize))
			continue;

		// Calculate intensity at our position
		float flRatio = Engine_WorldLightDistanceFalloff(light, vecDelta);
#ifdef CLIENT_DLL
		flRatio *= engine->LightStyleValue(light.style);
#endif
		Vector vecIntensity = light.intensity * flRatio;

		// Is this light more intense than the one we already found?
		if (vecIntensity.LengthSqr() <= 0.f)
			continue;

		// Can we see the light?
		trace_t tr;
		const Vector& vecAbsStart = vecPosition + Vector(0, 0, 30);
		UTIL_TraceLine(vecAbsStart, light.origin, MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr);

		if (tr.DidHit())
			continue;

		collectedlight_t col;
		col.pLight = &light;
		col.intensity = vecIntensity;
		col.pos = light.origin;
		vecLights.AddToTail(col);
	}

	delete[] pvs;

	return vecLights;
}

//-----------------------------------------------------------------------------
// Purpose: find the brightest light source at a point
//-----------------------------------------------------------------------------
bool CWorldLights::GetBrightestLightSource( const Vector& vecPosition, Vector& vecLightPos, Vector& vecLightBrightness ) const
{
	LightVec vecLights = CollectLightsAtPoint(vecPosition);

	if ( vecLights.Count() <= 0 )
		return false;

	// Default light position and brightness to zero
	vecLightBrightness.Init();
	vecLightPos.Init();

	// Iterate through all the worldlights
	for ( int i = 0; i < vecLights.Count(); ++i )
	{
		const collectedlight_t& light = vecLights[i];

		// Skip skyambient
		if ( light.pLight->type == emit_skyambient )
			continue;

		// Handle sun
		if ( light.pLight->type == emit_skylight )
		{
			if ( light.intensity.LengthSqr() <= vecLightBrightness.LengthSqr() )
				continue;

			vecLightBrightness = light.intensity;
			vecLightPos = light.pos;
			continue;
		}

		
		// Is this light more intense than the one we already found?
		if ( light.intensity.LengthSqr() <= vecLightBrightness.LengthSqr() )
			continue;

		vecLightPos = light.pos;
		vecLightBrightness = light.intensity;
	}

	return !vecLightBrightness.IsZero();
}

//-----------------------------------------------------------------------------
// Purpose: find the brightest light source at a point
//-----------------------------------------------------------------------------
bool CWorldLights::GetTotalLightAtPoint(const Vector& vecPosition, Vector& vecLightBrightness) const
{
	LightVec vecLights = CollectLightsAtPoint(vecPosition);

	if (vecLights.Count() <= 0)
		return false;

	// Default light position and brightness to zero
	vecLightBrightness.Init();

	// Iterate through all the worldlights
	for (int i = 0; i < vecLights.Count(); ++i)
	{
		const collectedlight_t& light = vecLights[i];

		// Skip skyambient
		if (light.pLight->type == emit_skyambient)
			continue;

		// Handle sun
		if (light.pLight->type == emit_skylight)
		{
			if (light.intensity.LengthSqr() <= vecLightBrightness.LengthSqr())
				continue;

			vecLightBrightness += light.intensity;
			continue;
		}


		// Is this light more intense than the one we already found?
		if (light.intensity.LengthSqr() <= vecLightBrightness.LengthSqr())
			continue;

		vecLightBrightness += light.intensity;
	}

	return !vecLightBrightness.IsZero();
}

bool CWorldLights::GetAmbientLightAtPoint(const Vector& vecPosition, Vector& vecLightAmbient) const
{
	vecLightAmbient.Init();

	int iLeaf = GetLeafForOrigin(vecPosition);
	if (iLeaf <= -1)
		return false;

	const CubeVec &lightCubes = m_LeafCubes.Element(iLeaf);
	if (lightCubes.Count() <= 0)
		return false;

	for (int c = 0; c < lightCubes.Count(); c++)
	{
		Vector vecCubeColor;
		const CompressedLightCube &lCube = lightCubes.Element(0).cube;
		for (int i = 0; i < ARRAYSIZE(lCube.m_Color); i++)
		{
			Vector vecColor;
			ColorRGBExp32ToVector(lCube.m_Color[i], vecColor);
			vecColor /= 255.f;
			vecCubeColor += vecColor;
		}
		vecCubeColor /= ARRAYSIZE(lCube.m_Color);
		vecLightAmbient += vecCubeColor;
	}

	vecLightAmbient /= lightCubes.Count();

	return true;
}