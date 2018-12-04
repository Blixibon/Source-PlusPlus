//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_SPOTLIGHT_H
#define AI_SPOTLIGHT_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_component.h"

class CBeam;
class CSprite;
class SmokeTrail;
class CSpotlightEnd;


//-----------------------------------------------------------------------------
// Parameters for how the scanner relates to citizens.
//-----------------------------------------------------------------------------
enum AISPOT_FLAGS
{
	AI_SPOTLIGHT_NO_DLIGHTS = 0x1,
	AI_SPOTLIGHT_ENABLE_PROJECTED = 0x2,
};


class CAI_Spotlight : public CAI_Component
{
	DECLARE_SIMPLE_DATADESC();
	DECLARE_CLASS_NOBASE( CAI_Spotlight );
	DECLARE_EMBEDDED_NETWORKVAR();

public:
	CAI_Spotlight();
	~CAI_Spotlight();

	void Init( CAI_BaseNPC *pOuter, int nFlags, float flConstraintAngle, float flMaxLength );

	// Create, destroy the spotlight
	void SpotlightCreate( int nAttachment, const Vector &vecInitialDir );
	void SpotlightDestroy(void);

	// Controls the spotlight target position + direction
	void SetSpotlightTargetPos( const Vector &vSpotlightTargetPos );
	void SetSpotlightTargetDirection( const Vector &vSpotlightTargetDir );

	// Updates the spotlight. Call every frame!
	void Update(void);

private:
	void Precache(void);
	void CreateSpotlightEntities( void );
	void UpdateSpotlightDirection( void );
	void UpdateSpotlightEndpoint( void );

	// Constrain to cone, returns true if it was constrained
	bool ConstrainToCone( Vector *pDirection );

	// Computes the spotlight endpoint
	void ComputeEndpoint( const Vector &vecStartPoint, Vector *pEndPoint );

private:
	CHandle<CBeam>	m_hSpotlight;
	CNetworkHandle(CSpotlightEnd, m_hSpotlightTarget);

	Vector			m_vSpotlightTargetPos;
	Vector			m_vSpotlightDir;
	float			m_flSpotlightCurLength;
	float			m_flSpotlightMaxLength;
	float			m_flConstraintAngle;
	int				m_nHaloSprite;
	CNetworkVar(int, m_nSpotlightAttachment);
	CNetworkVar(int, m_nFlags);
	Quaternion		m_vAngularVelocity;
};

EXTERN_SEND_TABLE(DT_AISpotlight);
#endif // AI_SPOTLIGHT_H


