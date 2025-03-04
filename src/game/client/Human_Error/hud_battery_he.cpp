//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// battery.cpp
//
// implementation of CHudBatteryHE class
//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "human_error/hud_icondisplay.h"
#include "iclientmode.h"

#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BAT	-1

//-----------------------------------------------------------------------------
// Purpose: Displays suit power (armor) on hud
//-----------------------------------------------------------------------------
class CHudBatteryHE : public CHudIconDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudBatteryHE, CHudIconDisplay );

	/*~CHudBatteryHE()
	{
		if (m_pBar)
		{
			m_pBar->DeletePanel();
		}
		if (m_pBase)
		{
			m_pBase->DeletePanel();
		}
	}*/

public:
	CHudBatteryHE( const char *pElementName );
	void Init( void );
	void Reset( void );
	void VidInit( void );
	void OnThink( void );
	void MsgFunc_Battery(bf_read &msg );
	bool ShouldDraw();
	
private:
	int		m_iBat;	
	int		m_iNewBat;
};

DECLARE_HUDELEMENT( CHudBatteryHE );
DECLARE_HUD_MESSAGE( CHudBatteryHE, Battery );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudBatteryHE::CHudBatteryHE( const char *pElementName ) : BaseClass(NULL, "HudSuitHE"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBatteryHE::Init( void )
{
	HOOK_HUD_MESSAGE( CHudBatteryHE, Battery);
	Reset();
	m_iBat		= INIT_BAT;
	m_iNewBat   = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBatteryHE::Reset( void )
{
	BaseClass::Reset();

	//SetLabelText(g_pVGuiLocalize->Find("#Valve_Hud_SUIT"));
	SetIcon(L"*");
	SetDisplayValue(m_iBat);
	SetLabel(L"Battery");

	/*m_bSimple = true;

	if (!m_pBase)
		m_pBase = new ImageFX( this, "sprites/metrocop_hud/hud_base", "MetrocopHud_BaseBattery" );

	if (m_pBase)
	{
		m_pBase->SetCustomPoints(true);
		m_pBase->SetVisibleEx(true);

		m_pBase->SetZPos(5);	
		PaintSimpleBar(m_pBase, 100, 100, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*0.6) + (GetWide()*0.1), 0.2f);
	}

	if (!m_pBar)
		m_pBar = new ImageFX( this, "sprites/metrocop_hud/hud_battery", "MetrocopHud_Battery" );

	if (m_pBar)
	{
		m_pBar->SetPosEx(GetWide()/2,GetTall()/2);
		m_pBar->SetImageSize(GetWide(), GetTall());
		m_pBar->SetCustomPoints(true);
		m_pBar->SetVisibleEx(true);

		m_pBar->SetZPos(6);
	}

	DevMsg("Battery hud reset\n");*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBatteryHE::VidInit( void )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudBatteryHE::ShouldDraw( void )
{
	bool bNeedsDraw = ( m_iBat != m_iNewBat || ( GetAlpha() > 0 ));

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBatteryHE::OnThink( void )
{
	if ( m_iBat == m_iNewBat )
		return;

	if ( !m_iNewBat )
	{
	 	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerZeroHE");
//		ShowImages(false);
		//SetPaintBackgroundEnabled( false );
	}
	else if ( m_iNewBat < m_iBat )
	{
		// battery power has decreased, so play the damaged animation
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitDamageTakenHE");

		// play an extra animation if we're super low
		if ( m_iNewBat < 20 )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitArmorLowHE");
		}
	}
	else
	{
		// battery power has increased (if we had no previous armor, or if we just loaded the game, don't use alert state)
		if ( m_iBat == INIT_BAT || m_iBat == 0 || m_iNewBat >= 20)
		{
//			ShowImages(true);
			SetPaintBackgroundEnabled( true );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerIncreasedAbove20HE");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitPowerIncreasedBelow20HE");
		}
	}

	m_iBat = m_iNewBat;

	SetDisplayValue(m_iBat);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBatteryHE::MsgFunc_Battery( bf_read &msg )
{
	m_iNewBat = msg.ReadShort();
}
