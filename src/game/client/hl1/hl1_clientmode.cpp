//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "ivmodemanager.h"
#include "clientmode_hlnormal.h"
#include "hl1_clientmode.h"
#include "hl1_clientscoreboard.h"
#include "colorcorrectionmgr.h"

// default FOV for HL1
ConVar default_fov( "default_fov", "90", FCVAR_CHEAT );

ConVar hl1_cc("cl_halflife_color", "0.4f", FCVAR_ARCHIVE);

// The current client mode. Always ClientModeNormal in HL.
IClientMode *g_pClientMode = NULL;

class CHLModeManager : public IVModeManager
{
public:
				CHLModeManager( void );
	virtual		~CHLModeManager( void );

	virtual void	Init( void );
	virtual void	SwitchMode( bool commander, bool force );
	virtual void	OverrideView( CViewSetup *pSetup );
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
}

void CHLModeManager::SwitchMode( bool commander, bool force )
{
}

void CHLModeManager::OverrideView( CViewSetup *pSetup )
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

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
private:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual void CreateDefaultPanels( void )
	{
		CBaseViewport::CreateDefaultPanels();
	}

	virtual IViewPortPanel *CreatePanelByName( const char *szPanelName );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeHL1Normal::ClientModeHL1Normal()
{
	m_CCHandle = INVALID_CLIENT_CCHANDLE;
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeHL1Normal::~ClientModeHL1Normal()
{
	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->RemoveColorCorrection(m_CCHandle);
	}
}

void ClientModeHL1Normal::InitViewport()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

float ClientModeHL1Normal::GetViewModelFOV( void )
{
	return 90.0f;
}


int	ClientModeHL1Normal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}


ClientModeHL1Normal g_ClientModeNormal;

IClientMode *GetClientModeNormal()
{
	return &g_ClientModeNormal;
}

ClientModeHL1Normal* GetClientModeHL1Normal()
{
	Assert( dynamic_cast< ClientModeHL1Normal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeHL1Normal* >( GetClientModeNormal() );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *newmap -
//-----------------------------------------------------------------------------
void ClientModeHL1Normal::LevelInit(const char *newmap)
{
	BaseClass::LevelInit(newmap);

	/*if (m_CCHandle == INVALID_CLIENT_CCHANDLE)
	{
		m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection("scripts/colorcorrection/half-life1.raw");
	}*/
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientModeHL1Normal::LevelShutdown(void)
{
	/*if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->RemoveColorCorrection(m_CCHandle);
	}*/

	BaseClass::LevelShutdown();
}

void ClientModeHL1Normal::Update()
{
	BaseClass::Update();

	if (/*engine->IsInGame() && */m_CCHandle == INVALID_CLIENT_CCHANDLE)
	{
		m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection("scripts/colorcorrection/half-life1.raw");
	}

	/*if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->SetColorCorrectionWeight(m_CCHandle, hl1_cc.GetFloat());
	}*/
}

void ClientModeHL1Normal::OnColorCorrectionWeightsReset()
{
	BaseClass::OnColorCorrectionWeightsReset();

	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->SetColorCorrectionWeight(m_CCHandle, hl1_cc.GetFloat());
	}
}

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
	{
		newpanel = new CHL1MPClientScoreBoardDialog( this );
		return newpanel;
	}
/*	else if ( Q_strcmp(PANEL_INFO, szPanelName) == 0 )
	{
		newpanel = new CHL2MPTextWindow( this );
		return newpanel;
	}*/

	return BaseClass::CreatePanelByName( szPanelName ); 
}





