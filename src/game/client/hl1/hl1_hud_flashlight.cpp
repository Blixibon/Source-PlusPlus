//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//


#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#ifdef HL1_DLL
#include "hl1_c_player.h" 
#elif HL2_EPISODIC
#include "hl2/c_basehlplayer.h"
#endif // HL1_DLL


#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>


#define MIN_ALPHA	 100	


class CHudFlashlightHL1 : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudFlashlightHL1, vgui::Panel );

public:
	CHudFlashlightHL1( const char *pElementName );

private:
	void	Paint( void );
	void	ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	CHudTexture		*icon_flash_empty;
	CHudTexture		*icon_flash_full;
	CHudTexture		*icon_flash_beam;
	Color			m_clrReddish;
};

DECLARE_HUDELEMENT( CHudFlashlightHL1 );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudFlashlightHL1::CHudFlashlightHL1( const char *pElementName ) : CHudElement( pElementName ), BaseClass(NULL, "HudFlashlightHL1")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

#ifdef HL2_EPISODIC
#define m_nFlashBattery m_HL2Local.m_flFlashBattery
#endif // HL2_EPISODIC


void CHudFlashlightHL1::Paint( void )
{
	int		r, g, b, a, nUnused;
	int		x, y;
	bool	bIsOn;
	Color	clrFlash;

#ifdef HL1_DLL
	C_HL1_Player* pPlayer = ToHL1Player(C_HL1_Player::GetLocalPlayer());
#elif HL2_EPISODIC
	C_BaseHLPlayer* pPlayer = (C_BaseHLPlayer*)C_BasePlayer::GetLocalPlayer();
#endif // HL1_DLL

	if ( !pPlayer )
		return;

	if ( !icon_flash_empty )
	{
		icon_flash_empty = gHUD.GetIcon( "flash_empty" );
	}

	if ( !icon_flash_full )
	{
		icon_flash_full = gHUD.GetIcon( "flash_full" );
	}

	if ( !icon_flash_beam )
	{
		icon_flash_beam = gHUD.GetIcon( "flash_beam" );
	}

	if ( !icon_flash_empty || !icon_flash_full || !icon_flash_beam )
	{
		return;
	}

	bIsOn = pPlayer->IsEffectActive( EF_DIMLIGHT );

	if ( bIsOn )
		a = 225;
	else
		a = MIN_ALPHA;

	if ( pPlayer->m_nFlashBattery < 20 )
	{
		m_clrReddish.GetColor( r, g, b, nUnused );
	}
	else
	{
		(gHUD.m_clrYellowish).GetColor( r, g, b, nUnused );
	}

	clrFlash.SetColor( r, g, b, a );

	y = icon_flash_empty->Height() / 2;
	x = GetWide() - ( icon_flash_empty->Width() * 1.5 );

	// Draw the flashlight casing
	icon_flash_empty->DrawSelf( x, y, clrFlash );

	if ( bIsOn )
	{  // draw the flashlight beam
		x = GetWide() - icon_flash_empty->Width() / 2;

		icon_flash_beam->DrawSelf( x, y, clrFlash );
	}

	// draw the flashlight energy level
	x = GetWide() - ( icon_flash_empty->Width() * 1.5 );

	int nOffset = icon_flash_empty->Width() * ( 1.0 - ( (float)pPlayer->m_nFlashBattery / 100.0f ) );
	if ( nOffset < icon_flash_empty->Width() )
	{
		icon_flash_full->DrawSelfCropped( x + nOffset, y, nOffset, 0, icon_flash_full->Width() - nOffset, icon_flash_full->Height(), clrFlash );
	}
}

void CHudFlashlightHL1::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	m_clrReddish = pScheme->GetColor( "Reddish", Color( 255, 16, 16, 255 ) );
}
