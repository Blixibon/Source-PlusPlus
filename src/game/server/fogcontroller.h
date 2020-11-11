//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FOGCONTROLLER_H
#define FOGCONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "playernet_vars.h"
#include "igamesystem.h"
#include "GameEventListener.h"
#include "triggers.h"
#include "utldict.h"

// Spawn Flags
#define SF_FOG_MASTER		0x0001

//=============================================================================
//
// Class Fog Controller:
// Compares a set of integer inputs to the one main input
// Outputs true if they are all equivalant, false otherwise
//
class CFogController : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_CLASS( CFogController, CBaseEntity );

	CFogController();
	~CFogController();

	// Parse data from a map file
	virtual void Activate();
	virtual int UpdateTransmitState();

	// Input handlers
	void InputSetStartDist(inputdata_t &data);
	void InputSetEndDist(inputdata_t &data);
	void InputTurnOn(inputdata_t &data);
	void InputTurnOff(inputdata_t &data);
	void InputSetColor(inputdata_t &data);
	void InputSetColorSecondary(inputdata_t &data);
	void InputSetFarZ( inputdata_t &data );
	void InputSetAngles( inputdata_t &inputdata );
	void InputSetMaxDensity( inputdata_t &inputdata );

	void InputSetColorLerpTo(inputdata_t &data);
	void InputSetColorSecondaryLerpTo(inputdata_t &data);
	void InputSetStartDistLerpTo(inputdata_t &data);
	void InputSetEndDistLerpTo(inputdata_t &data);

	void InputStartFogTransition(inputdata_t &data);

	int DrawDebugTextOverlays(void);

	void SetLerpValues( void );
	void Spawn( void );

	bool IsMaster( void )					{ return HasSpawnFlags( SF_FOG_MASTER ); }

public:

	CNetworkVarEmbedded( fogparams_t, m_fog );
	bool					m_bUseAngles;
	int						m_iChangedVariables;
	string_t		m_iszFogSet;
};
#if 0
class CFogTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS(CFogTrigger, CBaseTrigger);
	DECLARE_DATADESC();

	virtual void Spawn(void);
	virtual void StartTouch(CBaseEntity *other);
	virtual void EndTouch(CBaseEntity *other);

	fogparams_t *GetFog(void)
	{
		return &m_fog;
	}

protected:
	fogparams_t	m_fog;
};
#endif
//=============================================================================
//
// Fog Controller System.
//
class CFogSystem : public CAutoGameSystem, public CGameEventListener
{
public:

	// Creation/Init.
	CFogSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = NULL;
	}

	~CFogSystem()
	{
		m_hMasterController = NULL;
	}

	virtual bool Init();
	fogparams_t* GetScriptFog(const char* pszFogname);

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FireGameEvent( IGameEvent *pEvent ) { InitMasterController(); }
	CFogController *GetMasterFogController( void )			{ return m_hMasterController; }

private:

	void InitMasterController( void );
	CHandle< CFogController > m_hMasterController;
	CUtlDict<fogparams_t> m_ScriptedFog;
};

CFogSystem *FogSystem( void );

#endif // FOGCONTROLLER_H
