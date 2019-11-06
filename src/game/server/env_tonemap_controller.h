#ifndef ENV_TONEMAP_CONTROLLER_H
#define ENV_TONEMAP_CONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "GameEventListener.h"
#if 0
class CTonemapTrigger : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTonemapTrigger, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *other );
	virtual void EndTouch( CBaseEntity *other );

	CBaseEntity *GetTonemapController( void ) const;

private:
	string_t m_tonemapControllerName;
	EHANDLE m_hTonemapController;
};


//--------------------------------------------------------------------------------------------------------
inline CBaseEntity *CTonemapTrigger::GetTonemapController( void ) const
{
	return m_hTonemapController.Get();
}
#endif

//--------------------------------------------------------------------------------------------------------
// Tonemap Controller System.
class CTonemapSystem : public CAutoGameSystem, public CGameEventListener
{
public:

	// Creation/Init.
	CTonemapSystem( char const *name ) : CAutoGameSystem( name ) 
	{
		m_hMasterController = NULL;
	}

	~CTonemapSystem()
	{
		m_hMasterController = NULL;
	}

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void FireGameEvent(IGameEvent* pEvent) { InitMasterController(); }
	CBaseEntity *GetMasterTonemapController( void ) const;

private:

	void InitMasterController(void);
	EHANDLE m_hMasterController;
};


//--------------------------------------------------------------------------------------------------------
inline CBaseEntity *CTonemapSystem::GetMasterTonemapController( void ) const
{
	return m_hMasterController.Get();
}

//--------------------------------------------------------------------------------------------------------
CTonemapSystem *TheTonemapSystem( void );


#endif	// ENV_TONEMAP_CONTROLLER_H
