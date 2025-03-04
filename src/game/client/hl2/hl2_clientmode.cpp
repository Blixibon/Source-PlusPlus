//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode_hlnormal.h"
#include "panelmetaclassmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// default FOV for HL2
ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

#define SCREEN_FOLDER		"scripts/vgui_screens"

class CHLModeManager : public IVModeManager
{
public:
				CHLModeManager( void );
	virtual		~CHLModeManager( void );

	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force );
	virtual void	OverrideView( CNewViewSetup *pSetup );
	virtual void	CreateMove( float flInputSampleTime, CUserCmd *cmd );
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
};

CHLModeManager::CHLModeManager( void )
{
}

CHLModeManager::~CHLModeManager( void )
{
}

void CHLModeManager::Init( void )
{
	g_pClientMode = GetClientModeNormal();
	//PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char* fileName = SCREEN_FOLDER "/*.txt";
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "GAME", &findHandle);
	while (fileName)
	{
		if (!g_pFullFileSystem->FindIsDirectory(findHandle))
		{
			char cScreenFile[MAX_PATH];
			V_ComposeFileName(SCREEN_FOLDER, fileName, cScreenFile, MAX_PATH);
			PanelMetaClassMgr()->LoadMetaClassDefinitionFile(cScreenFile);
		}

		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);
}

void CHLModeManager::SwitchMode( bool commander, bool force )
{
}

void CHLModeManager::OverrideView( CNewViewSetup *pSetup )
{
}

void CHLModeManager::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
}

void CHLModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );
}

void CHLModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();
}


static CHLModeManager g_HLModeManager;
IVModeManager *modemanager = &g_HLModeManager;

