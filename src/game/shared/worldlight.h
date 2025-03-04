﻿//========= Copyright (C) 2011, CSProMod Team, All rights reserved. =========//
//
// Purpose: provide world light related functions to the client
// 
// Written: November 2011
// Author: Saul Rennison
//
//===========================================================================//
#ifndef WORLDLIGHT_H
#define WORLDLIGHT_H

#pragma once

#include "igamesystem.h" // CAutoGameSystem

class Vector;
struct dworldlight_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CWorldLights : public CAutoGameSystemPerFrame
{
public:
	CWorldLights();
	~CWorldLights() { Clear(); }

	//-------------------------------------------------------------------------
	// Find the brightest light source at a point
	//-------------------------------------------------------------------------
	bool GetBrightestLightSource( const Vector& vecPosition, Vector& vecLightPos, Vector& vecLightBrightness ) const;

	bool GetTotalLightAtPoint(const Vector& vecPosition, Vector& vecLightBrightness) const;

	bool GetAmbientLightAtPoint(const Vector& vecPosition, Vector& vecLightAmbient) const;

	// CAutoGameSystem overrides
public:
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity() { Clear(); }

private:
	void Clear();

	typedef struct
	{
		const dworldlight_t *pLight;
		Vector intensity;
		Vector pos;
	} collectedlight_t;

	typedef struct
	{
		CompressedLightCube	cube;
		Vector pos;
	} lightCube_t;

	typedef CUtlVector<collectedlight_t> LightVec;
	typedef CUtlVector<lightCube_t> CubeVec;

	LightVec CollectLightsAtPoint(const Vector& vecPosition) const;

	int m_nWorldLights;
	dworldlight_t *m_pWorldLights;
	CUtlVector<CubeVec> m_LeafCubes;
#ifndef CLIENT_DLL
	//INetworkStringTable* m_pLightStyleTable;
#endif
};

//-----------------------------------------------------------------------------
// Singleton exposure
//-----------------------------------------------------------------------------
extern CWorldLights *g_pWorldLights;

#endif // WORLDLIGHT_H