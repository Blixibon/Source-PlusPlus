//=============================================================================//
//
// Purpose: Ask: This is a screen we'll use for the Manhack
//
//=============================================================================//
#include "cbase.h"

#include "C_VGuiScreen.h"
#include <vgui/IVGUI.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "clientmode_hlnormal.h"
#include "tne_RenderTargets.h"
#include "rendertexture.h"
#include "view_shared.h"
#include "c_manhack_screen.h"
#include "ammodef.h"
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CManhackScreen *s_ManhackScreen = NULL;

CManhackScreen *GetManhackScreen()
{
	if (!s_ManhackScreen)
	{
		s_ManhackScreen = new CManhackScreen(NULL, "Manhack_Screen");
		s_ManhackScreen->SetParent(enginevgui->GetPanel(PANEL_ROOT));
		s_ManhackScreen->SetProportional(false);
	}

	return s_ManhackScreen;
}


//-----------------------------------------------------------------------------
// Constructor:
//-----------------------------------------------------------------------------
CManhackScreen::CManhackScreen( vgui::Panel *parent, const char *panelName )
    : BaseClass( parent, panelName, g_hVGuiCombineScheme )
{
	LoadControlSettings("scripts/Human_Error_screens/ManhackViewModel.res");

	SetBgColor(Color(0, 0, 0, 255));

	m_pManhackCount = dynamic_cast<vgui::Label*>(FindChildByName("ManhackCountReadout"));
	m_pManhackDistance = dynamic_cast<vgui::Label*>(FindChildByName("ManhackDistanceReadout"));
	m_pManhackOnline = dynamic_cast<vgui::Label*>(FindChildByName("ManhacksOnlineLabel"));
	m_iManhackDistance = 100;

	if (m_pManhackOnline)
	{
		m_pManhackOnline->SetFgColor(Color(255, 0, 0, 255));
	}
}

CManhackScreen::~CManhackScreen()
{
	s_ManhackScreen = NULL;
}
#if 0
//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
bool CManhackScreen::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	s_ManhackScreen = this;

    // Load all of the controls in
    if ( !BaseClass::Init(pKeyValues, pInitData) )
        return false;

    // Make sure we get ticked...
    vgui::ivgui()->AddTickSignal( GetVPanel() );

    // Ask: Here we find a pointer to our AmmoCountReadout Label and store it in m_pAmmoCount
    m_pManhackCount		= dynamic_cast<vgui::Label*>(FindChildByName( "ManhackCountReadout" ));
	m_pManhackDistance	= dynamic_cast<vgui::Label*>(FindChildByName( "ManhackDistanceReadout" )); 
	m_pManhackOnline	= dynamic_cast<vgui::Label*>(FindChildByName( "ManhacksOnlineLabel" ));
	m_iManhackDistance = 100;

	if (m_pManhackOnline)
	{
		m_pManhackOnline->SetFgColor(Color(255,0,0,255));
	}

	SetVisible( false );

    return true;
}
#endif
void CManhackScreen::SetManhackData(int iDistance, int iCount)
{
	m_iManhackDistance = 100 - iDistance;
	m_iManhackDistance = clamp( m_iManhackDistance, 0, 100);

	SetPaintBackgroundEnabled((iCount > 0) ? false : true);

	if (m_pManhackOnline)
	{
		if (iCount > 0)
		{
			m_pManhackOnline->SetText("ONLINE");
			m_pManhackOnline->SetFgColor(Color(0, 255, 0, 255));
		}
		else
		{
			m_pManhackOnline->SetText("OFFLINE");
			m_pManhackOnline->SetFgColor(Color(255, 0, 0, 255));
		}
	}

	// If our Label exist
	if (m_pManhackCount)
	{
		char buf[32];
		Q_snprintf(buf, sizeof(buf), "%d", iCount);
		// Set the Labels text to the number of missiles we have left.
		m_pManhackCount->SetText(buf);
		m_pManhackCount->SetFgColor(Color(255, 255, 255, 255));
	}

	if (m_pManhackDistance)
	{
		char buf[32];
		Q_snprintf(buf, sizeof(buf), "%d", m_iManhackDistance);
		// Set the Labels text to the number of missiles we have left.

		int xpos, ypos;
		m_pManhackDistance->GetPos(xpos, ypos);

		int charWidth = vgui::surface()->GetCharacterWidth(m_pManhackDistance->GetFont(), '0');

		xpos = 100;

		if (m_iManhackDistance >= 10)
			xpos -= charWidth;

		if (m_iManhackDistance >= 100)
			xpos -= charWidth;

		m_pManhackDistance->SetPos(xpos, ypos);

		m_pManhackDistance->SetText(buf);
		m_pManhackDistance->SetFgColor(Color(255, 255, 255, 255));
	}
}




