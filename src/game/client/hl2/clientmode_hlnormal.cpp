//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the normal TF2 or HL2 HUD.
//
//=============================================================================
#include "cbase.h"
#include "clientmode_hlnormal.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "iinput.h"
#include "ienginevgui.h"
#ifdef HL2_LAZUL
#include "hl2mp/ui/hl2mptextwindow.h"
#include "peter/ui/lazuulclientscoreboard.h"
#endif // HL2_LAZUL


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool g_bRollingCredits;

ConVar fov_desired( "fov_desired", "75", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 75.0, true, 90.0 );

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------
vgui::HScheme g_hVGuiCombineScheme = 0;


// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeHLNormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}


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

#ifdef HL2_LAZUL
	virtual void CreateDefaultPanels(void)
	{
		AddNewPanel(CreatePanelByName(PANEL_SCOREBOARD), "PANEL_SCOREBOARD");
		AddNewPanel(CreatePanelByName(PANEL_INFO), "PANEL_INFO");
		AddNewPanel(CreatePanelByName(PANEL_SPECGUI), "PANEL_SPECGUI");
		//AddNewPanel(CreatePanelByName(PANEL_SPECMENU), "PANEL_SPECMENU");
		AddNewPanel(CreatePanelByName(PANEL_NAV_PROGRESS), "PANEL_NAV_PROGRESS");
		AddNewPanel(CreatePanelByName(PANEL_TEAM), "PANEL_TEAM");
	};

	virtual IViewPortPanel* CreatePanelByName(const char* szPanelName)
	{
		IViewPortPanel* newpanel = NULL;

		if (Q_strcmp(PANEL_SCOREBOARD, szPanelName) == 0)
		{
			newpanel = new CLazClientScoreBoardDialog(this);
			return newpanel;
		}
		else if (Q_strcmp(PANEL_INFO, szPanelName) == 0)
		{
			newpanel = new CHL2MPTextWindow(this);
			return newpanel;
		}
		else if (Q_strcmp(PANEL_SPECGUI, szPanelName) == 0)
		{
			newpanel = new CHL2MPSpectatorGUI(this);
			return newpanel;
		}


		return BaseClass::CreatePanelByName(szPanelName);
	}
#endif // HL2_LAZUL

};


//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeHLNormal::ClientModeHLNormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ClientModeHLNormal::~ClientModeHLNormal()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientModeHLNormal::Init()
{
	BaseClass::Init();

	// Load up the combine control panel scheme
	g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/CombinePanelScheme.res", "CombineScheme" );
	if (!g_hVGuiCombineScheme)
	{
		Warning( "Couldn't load combine panel scheme!\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeHLNormal::DoPostScreenSpaceEffects(const CNewViewSetup *pSetup)
{
	return BaseClass::DoPostScreenSpaceEffects(pSetup);
}

bool ClientModeHLNormal::ShouldDrawCrosshair( void )
{
	return ( g_bRollingCredits == false );
}