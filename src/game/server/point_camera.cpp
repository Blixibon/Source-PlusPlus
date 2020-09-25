//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "point_camera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAM_THINK_INTERVAL 0.05

// Spawnflags
#define SF_CAMERA_START_OFF				0x01

// UNDONE: Share properly with the client code!!!
#define POINT_CAMERA_MSG_SETACTIVE		1

CEntityClassList<CPointCamera> g_PointCameraList;
template <> CPointCamera *CEntityClassList<CPointCamera>::m_pClassList = NULL;

CPointCamera* GetPointCameraList()
{
	return g_PointCameraList.m_pClassList;
}

// These are already built into CBaseEntity
//	DEFINE_KEYFIELD( m_iName, FIELD_STRING, "targetname" ),
//	DEFINE_KEYFIELD( m_iParent, FIELD_STRING, "parentname" ),
//	DEFINE_KEYFIELD( m_target, FIELD_STRING, "target" ),

LINK_ENTITY_TO_CLASS( point_camera, CPointCamera );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointCamera::~CPointCamera()
{
	g_PointCameraList.Remove( this );
}

CPointCamera::CPointCamera()
{
	// Set these to opposites so that it'll be sent the first time around.
	m_bActive = false;
	m_bIsOn = false;
	
	m_bFogEnable = false;

	m_bDOFEnabled = false;
	m_flNearBlurDepth = 50.0f;
	m_flNearFocusDepth = 100.0f;
	m_flFarFocusDepth = 250.0f;
	m_flFarBlurDepth = 1000.0f;
	m_flNearBlurRadius = 0.0f;		// no near blur by default
	m_flFarBlurRadius = 5.0f;

	g_PointCameraList.Insert( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::Spawn( void )
{
	BaseClass::Spawn();

	if ( m_spawnflags & SF_CAMERA_START_OFF )
	{
		m_bIsOn = false;
	}
	else
	{
		m_bIsOn = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override ShouldTransmit since we want to be sent even though we don't have a model, etc.
//			All that matters is if we are in the pvs.
//-----------------------------------------------------------------------------
int CPointCamera::UpdateTransmitState()
{
	if ( m_bActive )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	else
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::SetActive( bool bActive )
{
	// If the mapmaker's told the camera it's off, it enforces inactive state
	if ( !m_bIsOn )
	{
		bActive = false;
	}

	if ( m_bActive != bActive )
	{
		m_bActive = bActive;
		DispatchUpdateTransmitState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputChangeFOV( inputdata_t &inputdata )
{
	// Parse the keyvalue data
	char parseString[255];

	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get FOV
	char *pszParam = strtok(parseString," ");
	if(pszParam)
	{
		m_TargetFOV = atof( pszParam );
	}
	else
	{
		// Assume no change
		m_TargetFOV = m_FOV;
	}

	// Get Time
	float flChangeTime;
	pszParam = strtok(NULL," ");
	if(pszParam)
	{
		flChangeTime = atof( pszParam );
	}
	else
	{
		// Assume 1 second.
		flChangeTime = 1.0;
	}

	m_DegreesPerSecond = ( m_TargetFOV - m_FOV ) / flChangeTime;

	SetThink( &CPointCamera::ChangeFOVThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::ChangeFOVThink( void )
{
	SetNextThink( gpGlobals->curtime + CAM_THINK_INTERVAL );

	float newFOV = m_FOV;

	newFOV += m_DegreesPerSecond * CAM_THINK_INTERVAL;

	if( m_DegreesPerSecond < 0 )
	{
		if( newFOV <= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}
	else
	{
		if( newFOV >= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}

	m_FOV = newFOV;
}

//-----------------------------------------------------------------------------
// Purpose: Turn this camera on, and turn all other cameras off
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOnAndTurnOthersOff( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "point_camera" )) != NULL)
	{
		CPointCamera *pCamera = (CPointCamera*)pEntity;
		pCamera->InputSetOff( inputdata );
	}

	// Now turn myself on
	InputSetOn( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOn( inputdata_t &inputdata )
{
	m_bIsOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOff( inputdata_t &inputdata )
{
	m_bIsOn = false;
	SetActive( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetNearBlurDepth(inputdata_t& inputdata)
{
	m_flNearBlurDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetNearFocusDepth(inputdata_t& inputdata)
{
	m_flNearFocusDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetFarFocusDepth(inputdata_t& inputdata)
{
	m_flFarFocusDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetFarBlurDepth(inputdata_t& inputdata)
{
	m_flFarBlurDepth = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetNearBlurRadius(inputdata_t& inputdata)
{
	m_flNearBlurRadius = inputdata.value.Float();
	m_bDOFEnabled = (m_flNearBlurRadius > 0.0f) || (m_flFarBlurRadius > 0.0f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetFarBlurRadius(inputdata_t& inputdata)
{
	m_flFarBlurRadius = inputdata.value.Float();
	m_bDOFEnabled = (m_flNearBlurRadius > 0.0f) || (m_flFarBlurRadius > 0.0f);
}

BEGIN_DATADESC( CPointCamera )

	// Save/restore Keyvalue fields
	DEFINE_KEYFIELD( m_FOV,			FIELD_FLOAT, "FOV" ),
	DEFINE_KEYFIELD( m_Resolution,	FIELD_FLOAT, "resolution" ),
	DEFINE_KEYFIELD( m_bFogEnable,	FIELD_BOOLEAN, "fogEnable" ),
	DEFINE_KEYFIELD( m_FogColor,	FIELD_COLOR32,	"fogColor" ),
	DEFINE_KEYFIELD( m_flFogStart,	FIELD_FLOAT, "fogStart" ),
	DEFINE_KEYFIELD( m_flFogEnd,	FIELD_FLOAT, "fogEnd" ),
	DEFINE_KEYFIELD( m_flFogMaxDensity,	FIELD_FLOAT, "fogMaxDensity" ),
	DEFINE_KEYFIELD( m_bUseScreenAspectRatio, FIELD_BOOLEAN, "UseScreenAspectRatio" ),
	DEFINE_KEYFIELD(m_bDOFEnabled, FIELD_BOOLEAN, "dof_enabled"),
	DEFINE_KEYFIELD(m_flNearBlurDepth, FIELD_FLOAT, "near_blur"),
	DEFINE_KEYFIELD(m_flNearFocusDepth, FIELD_FLOAT, "near_focus"),
	DEFINE_KEYFIELD(m_flFarFocusDepth, FIELD_FLOAT, "far_focus"),
	DEFINE_KEYFIELD(m_flFarBlurDepth, FIELD_FLOAT, "far_blur"),
	DEFINE_KEYFIELD(m_flNearBlurRadius, FIELD_FLOAT, "near_radius"),
	DEFINE_KEYFIELD(m_flFarBlurRadius, FIELD_FLOAT, "far_radius"),
	DEFINE_FIELD( m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsOn,			FIELD_BOOLEAN ),

	DEFINE_FIELD( m_TargetFOV,		FIELD_FLOAT ),
	DEFINE_FIELD( m_DegreesPerSecond, FIELD_FLOAT ),
	// This is re-set up in the constructor
	//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),

	DEFINE_FUNCTION( ChangeFOVThink ),

	// Input
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeFOV", InputChangeFOV ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOnAndTurnOthersOff", InputSetOnAndTurnOthersOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOn", InputSetOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOff", InputSetOff ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetNearBlurDepth", InputSetNearBlurDepth),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetNearFocusDepth", InputSetNearFocusDepth),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetFarFocusDepth", InputSetFarFocusDepth),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetFarBlurDepth", InputSetFarBlurDepth),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetNearBlurRadius", InputSetNearBlurRadius),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetFarBlurRadius", InputSetFarBlurRadius),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointCamera, DT_PointCamera )
	SendPropFloat( SENDINFO( m_FOV ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_Resolution ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_bFogEnable ), 1, SPROP_UNSIGNED ),	
	SendPropInt( SENDINFO_STRUCTELEM( m_FogColor ), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flFogStart ), 0, SPROP_NOSCALE ),	
	SendPropFloat( SENDINFO( m_flFogEnd ), 0, SPROP_NOSCALE ),	
	SendPropFloat( SENDINFO( m_flFogMaxDensity ), 0, SPROP_NOSCALE ),	
	SendPropInt( SENDINFO( m_bActive ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bUseScreenAspectRatio ), 1, SPROP_UNSIGNED ),

	SendPropInt(SENDINFO(m_bDOFEnabled), 1, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flNearBlurDepth), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flNearFocusDepth), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flFarFocusDepth), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flFarBlurDepth), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flNearBlurRadius), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_flFarBlurRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()
