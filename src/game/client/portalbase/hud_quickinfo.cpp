//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "c_portal_player.h"
#include "weapon_portalgun.h"
#include "igameuifuncs.h"
#include "hud_crosshair.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	HEALTH_WARNING_THRESHOLD	25


static ConVar	hud_quickinfo( "hud_quickinfo", "1", FCVAR_ARCHIVE );
static ConVar	hud_quickinfo_swap( "hud_quickinfo_swap", "0", FCVAR_ARCHIVE );

extern ConVar crosshair;

#define QUICKINFO_EVENT_DURATION	1.0f
#define	QUICKINFO_BRIGHTNESS_FULL	255
#define	QUICKINFO_BRIGHTNESS_DIM	64
#define	QUICKINFO_FADE_IN_TIME		0.5f
#define QUICKINFO_FADE_OUT_TIME		2.0f

/*
==================================================
CHUDQuickInfo 
==================================================
*/

using namespace vgui;

class CHUDQuickInfo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHUDQuickInfo, vgui::Panel );
public:
	CHUDQuickInfo( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void OnThink();
	virtual void Paint();
	
	virtual void ApplySchemeSettings( IScheme *scheme );
private:
	
	void	DrawWarning( int x, int y, CHudTexture *icon, float &time );
	void	UpdateEventTime( void );
	bool	EventTimeElapsed( void );

	int		m_lastAmmo;
	int		m_lastHealth;

	float	m_ammoFade;
	float	m_healthFade;

	bool	m_warnAmmo;
	bool	m_warnHealth;

	bool	m_bFadedOut;

	bool	m_bDimmed;			// Whether or not we are dimmed down
	float	m_flLastEventTime;
	
	float	m_fLastPlacedAlpha[2];
	bool	m_bLastPlacedAlphaCountingUp[2];

	CHudTexture	*m_icon_c;

	CHudTexture	* m_icon_prbi;	// right portal bracket
	CHudTexture	* m_icon_plbi;	// left portal bracket

	CHudTexture	* m_icon_prbv;		// right portal bracket, full
	CHudTexture	* m_icon_plbv;		// left portal bracket, full
	CHudTexture	* m_icon_plp;	// last placed portal

	CHudTexture* m_icon_rbn;	// right bracket
	CHudTexture* m_icon_lbn;	// left bracket

	CHudTexture* m_icon_rb;		// right bracket, full
	CHudTexture* m_icon_lb;		// left bracket, full
	CHudTexture* m_icon_rbe;	// right bracket, empty
	CHudTexture* m_icon_lbe;	// left bracket, empty
};

DECLARE_HUDELEMENT( CHUDQuickInfo );

CHUDQuickInfo::CHUDQuickInfo( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HUDQuickInfo" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits(HIDEHUD_CROSSHAIR | HIDEHUD_PLAYERDEAD);

	m_fLastPlacedAlpha[0] = m_fLastPlacedAlpha[1] = 80;
	m_bLastPlacedAlphaCountingUp[0] = m_bLastPlacedAlphaCountingUp[1] = true;
}

void CHUDQuickInfo::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
	SetForceStereoRenderToFrameBuffer(true);
}


void CHUDQuickInfo::Init( void )
{
	m_ammoFade = 0.0f;
	m_healthFade = 0.0f;

	m_lastAmmo = 0;
	m_lastHealth = 100;

	m_warnAmmo = false;
	m_warnHealth = false;

	m_bFadedOut = false;
	m_bDimmed = false;
	m_flLastEventTime = 0.0f;
}


void CHUDQuickInfo::VidInit( void )
{
	Init();

	m_icon_c = gHUD.GetIcon( "crosshair" );

	if ( IsX360() )
	{
		m_icon_prbv = gHUD.GetIcon( "portal_crosshair_right_valid_x360" );
		m_icon_plbv = gHUD.GetIcon( "portal_crosshair_left_valid_x360" );
		m_icon_plp = gHUD.GetIcon( "portal_crosshair_last_placed_x360" );
		m_icon_prbi = gHUD.GetIcon( "portal_crosshair_right_invalid_x360" );
		m_icon_plbi = gHUD.GetIcon( "portal_crosshair_left_invalid_x360" );
	}
	else
	{
		m_icon_prbv = gHUD.GetIcon( "portal_crosshair_right_valid" );
		m_icon_plbv = gHUD.GetIcon( "portal_crosshair_left_valid" );
		m_icon_plp = gHUD.GetIcon( "portal_crosshair_last_placed" );
		m_icon_prbi = gHUD.GetIcon( "portal_crosshair_right_invalid" );
		m_icon_plbi = gHUD.GetIcon( "portal_crosshair_left_invalid" );
	}

	m_icon_rb = gHUD.GetIcon("crosshair_right_full");
	m_icon_lb = gHUD.GetIcon("crosshair_left_full");
	m_icon_rbe = gHUD.GetIcon("crosshair_right_empty");
	m_icon_lbe = gHUD.GetIcon("crosshair_left_empty");
	m_icon_rbn = gHUD.GetIcon("crosshair_right");
	m_icon_lbn = gHUD.GetIcon("crosshair_left");
}


void CHUDQuickInfo::DrawWarning( int x, int y, CHudTexture *icon, float &time )
{
	float scale	= (int)( fabsf(sin(gpGlobals->curtime*8.0f)) * 128.0);

	// Only fade out at the low point of our blink
	if ( time <= (gpGlobals->frametime * 200.0f) )
	{
		if ( scale < 40 )
		{
			time = 0.0f;
			return;
		}
		else
		{
			// Counteract the offset below to survive another frame
			time += (gpGlobals->frametime * 200.0f);
		}
	}
	
	// Update our time
	time -= (gpGlobals->frametime * 200.0f);
	Color caution = gHUD.m_clrCaution;
	caution[3] = scale * 255;

	icon->DrawSelf( x, y, caution );
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHUDQuickInfo::ShouldDraw( void )
{
	if ( !m_icon_c || !m_icon_rb || !m_icon_rbe || !m_icon_lb || !m_icon_lbe || !m_icon_plbi || !m_icon_plbv || !m_icon_plp || !m_icon_prbi || !m_icon_prbv)
		return false;

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( player == NULL )
		return false;

	if ( !crosshair.GetBool() )
		return false;

	return ( CHudElement::ShouldDraw() && !engine->IsDrawingLoadingImage() );
}

//-----------------------------------------------------------------------------
// Purpose: Checks if the hud element needs to fade out
//-----------------------------------------------------------------------------
void CHUDQuickInfo::OnThink()
{
	BaseClass::OnThink();

	C_BasePlayer* player = C_BasePlayer::GetLocalPlayer();
	if (player == NULL)
		return;

	// see if we should fade in/out
	bool bFadeOut = player->IsZoomed();

	// check if the state has changed
	if (m_bFadedOut != bFadeOut)
	{
		m_bFadedOut = bFadeOut;

		m_bDimmed = false;

		if (bFadeOut)
		{
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", 0.0f, 0.0f, 0.25f, vgui::AnimationController::INTERPOLATOR_LINEAR);
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", QUICKINFO_BRIGHTNESS_FULL, 0.0f, QUICKINFO_FADE_IN_TIME, vgui::AnimationController::INTERPOLATOR_LINEAR);
		}
	}
	else if (!m_bFadedOut)
	{
		// If we're dormant, fade out
		if (EventTimeElapsed())
		{
			if (!m_bDimmed)
			{
				m_bDimmed = true;
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", QUICKINFO_BRIGHTNESS_DIM, 0.0f, QUICKINFO_FADE_OUT_TIME, vgui::AnimationController::INTERPOLATOR_LINEAR);
			}
		}
		else if (m_bDimmed)
		{
			// Fade back up, we're active
			m_bDimmed = false;
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand(this, "Alpha", QUICKINFO_BRIGHTNESS_FULL, 0.0f, QUICKINFO_FADE_IN_TIME, vgui::AnimationController::INTERPOLATOR_LINEAR);
		}
	}
}

void CHUDQuickInfo::Paint()
{
	C_Portal_Player *pPortalPlayer = (C_Portal_Player*)( C_BasePlayer::GetLocalPlayer() );
	if ( pPortalPlayer == NULL )
		return;

	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon == NULL )
		return;

	C_WeaponPortalgun* pPortalgun = dynamic_cast<C_WeaponPortalgun*>(pWeapon);

	float fX, fY;
	bool bBehindCamera = false;
	CHudCrosshair::GetDrawPosition(&fX, &fY, &bBehindCamera);

	// if the crosshair is behind the camera, don't draw it
	if (bBehindCamera)
		return;

	int		xCenter = (int)fX;
	int		yCenter = (int)fY - m_icon_lb->Height() / 2;

	float	scalar = pPortalgun ? 1.0f : (138.0f / 255.0f);

	int	health = pPortalPlayer->GetHealth();
	int	ammo = pWeapon->Clip1();

	if (pPortalPlayer->IsSuitEquipped())
	{
		// Check our health for a warning
		if (health != m_lastHealth)
		{
			UpdateEventTime();
			m_lastHealth = health;

			if (health <= HEALTH_WARNING_THRESHOLD)
			{
				if (m_warnHealth == false)
				{
					m_healthFade = 255;
					m_warnHealth = true;

					CLocalPlayerFilter filter;
					C_BaseEntity::EmitSound(filter, SOUND_FROM_LOCAL_PLAYER, "HUDQuickInfo.LowHealth");
				}
			}
			else
			{
				m_warnHealth = false;
			}
		}

		// Check our ammo for a warning
		if (ammo != m_lastAmmo)
		{
			UpdateEventTime();
			m_lastAmmo = ammo;

			// Find how far through the current clip we are
			float ammoPerc = (float)ammo / (float)pWeapon->GetMaxClip1();

			// Warn if we're below a certain percentage of our clip's size
			if ((pWeapon->GetMaxClip1() > 1) && (ammoPerc <= (1.0f - CLIP_PERC_THRESHOLD)))
			{
				if (m_warnAmmo == false)
				{
					m_ammoFade = 255;
					m_warnAmmo = true;

					CLocalPlayerFilter filter;
					C_BaseEntity::EmitSound(filter, SOUND_FROM_LOCAL_PLAYER, "HUDQuickInfo.LowAmmo");
				}
			}
			else
			{
				m_warnAmmo = false;
			}
		}
	}

	Color clrNormal = gHUD.m_clrNormal;
	clrNormal[3] = 255;
	m_icon_c->DrawSelf( xCenter, yCenter, clrNormal );

	if (IsX360())
	{
		// Because the fixed reticle draws on half-texels, this rather unsightly hack really helps
		// center the appearance of the quickinfo on 360 displays.
		xCenter += 1;
	}

	if (!hud_quickinfo.GetInt())
		return;

	bool bPortalPlacability[2];

	if ( pPortalgun )
	{
		bPortalPlacability[0] = pPortalgun->GetPortal1Placablity() > 0.5f;
		bPortalPlacability[1] = pPortalgun->GetPortal2Placablity() > 0.5f;
	}

	if (!pPortalgun || (!pPortalgun->CanFirePortal1() && !pPortalgun->CanFirePortal2()))
	{
		// no portalgun or we can't fire either portal, draw the hl2 quickinfo
		//clrNormal[3] = 196;
		int	sinScale = (int)(fabsf(sin(gpGlobals->curtime * 8.0f)) * 128.0f);

		// Update our health
		if (m_healthFade > 0.0f)
		{
			DrawWarning(xCenter - (m_icon_lb->Width() * 2), yCenter, m_icon_lb, m_healthFade);
		}
		else
		{
			float healthPerc = (float)health / 100.0f;
			healthPerc = clamp(healthPerc, 0.0f, 1.0f);

			Color healthColor = m_warnHealth ? gHUD.m_clrCaution : gHUD.m_clrNormal;

			if (m_warnHealth)
			{
				healthColor[3] = 255 * sinScale;
			}
			else
			{
				healthColor[3] = 255 * scalar;
			}

			gHUD.DrawIconProgressBar(xCenter - (m_icon_lb->Width() * 2), yCenter, m_icon_lb, m_icon_lbe, (1.0f - healthPerc), healthColor, CHud::HUDPB_VERTICAL);
		}

		// Update our ammo
		if (m_ammoFade > 0.0f)
		{
			DrawWarning(xCenter + m_icon_rb->Width(), yCenter, m_icon_rb, m_ammoFade);
		}
		else
		{
			float ammoPerc;

			if (pWeapon->GetMaxClip1() <= 0)
			{
				ammoPerc = 0.0f;
			}
			else
			{
				ammoPerc = 1.0f - ((float)ammo / (float)pWeapon->GetMaxClip1());
				ammoPerc = clamp(ammoPerc, 0.0f, 1.0f);
			}

			Color ammoColor = m_warnAmmo ? gHUD.m_clrCaution : gHUD.m_clrNormal;

			if (m_warnAmmo)
			{
				ammoColor[3] = 255 * sinScale;
			}
			else
			{
				ammoColor[3] = 255 * scalar;
			}

			gHUD.DrawIconProgressBar(xCenter + m_icon_rb->Width(), yCenter, m_icon_rb, m_icon_rbe, ammoPerc, ammoColor, CHud::HUDPB_VERTICAL);
		}
		return;
	}
	else
	{
		yCenter = (int)fY - (m_icon_plbv->Height() / 2);
	}

	UpdateEventTime();

	const unsigned char iAlphaStart = 150;	   

	Color portal1Color = UTIL_Portal_Color( 1, pPortalgun->m_iPortalLinkageGroupID);
	Color portal2Color = UTIL_Portal_Color( 2, pPortalgun->m_iPortalLinkageGroupID);

	portal1Color[ 3 ] = iAlphaStart;
	portal2Color[ 3 ] = iAlphaStart;

	const int iBaseLastPlacedAlpha = 128;
	Color lastPlaced1Color = Color( portal1Color[0], portal1Color[1], portal1Color[2], iBaseLastPlacedAlpha );
	Color lastPlaced2Color = Color( portal2Color[0], portal2Color[1], portal2Color[2], iBaseLastPlacedAlpha );

	const float fLastPlacedAlphaLerpSpeed = 300.0f;

	
	float fLeftPlaceBarFill = 0.0f;
	float fRightPlaceBarFill = 0.0f;

	if ( pPortalgun->CanFirePortal1() && pPortalgun->CanFirePortal2() )
	{
		int iDrawLastPlaced = 0;

		//do last placed indicator effects
		if ( pPortalgun->GetLastFiredPortal() == 1 )
		{
			iDrawLastPlaced = 0;
			fLeftPlaceBarFill = 1.0f;
		}
		else if ( pPortalgun->GetLastFiredPortal() == 2 )
		{
			iDrawLastPlaced = 1;
			fRightPlaceBarFill = 1.0f;			
		}

		if( m_bLastPlacedAlphaCountingUp[iDrawLastPlaced] )
		{
			m_fLastPlacedAlpha[iDrawLastPlaced] += gpGlobals->absoluteframetime * fLastPlacedAlphaLerpSpeed * 2.0f;
			if( m_fLastPlacedAlpha[iDrawLastPlaced] > 255.0f )
			{
				m_bLastPlacedAlphaCountingUp[iDrawLastPlaced] = false;
				m_fLastPlacedAlpha[iDrawLastPlaced] = 255.0f - (m_fLastPlacedAlpha[iDrawLastPlaced] - 255.0f);
			}
		}
		else
		{
			m_fLastPlacedAlpha[iDrawLastPlaced] -= gpGlobals->absoluteframetime * fLastPlacedAlphaLerpSpeed;
			if( m_fLastPlacedAlpha[iDrawLastPlaced] < (float)iBaseLastPlacedAlpha )
			{
				m_fLastPlacedAlpha[iDrawLastPlaced] = (float)iBaseLastPlacedAlpha;
			}
		}

		//reset the last placed indicator on the other side
		m_fLastPlacedAlpha[1 - iDrawLastPlaced] -= gpGlobals->absoluteframetime * fLastPlacedAlphaLerpSpeed;
		if( m_fLastPlacedAlpha[1 - iDrawLastPlaced] < 0.0f )
		{
			m_fLastPlacedAlpha[1 - iDrawLastPlaced] = 0.0f;
		}
		m_bLastPlacedAlphaCountingUp[1 - iDrawLastPlaced] = true;

		if ( pPortalgun->GetLastFiredPortal() != 0 )
		{
			lastPlaced1Color[3] = m_fLastPlacedAlpha[0];
			lastPlaced2Color[3] = m_fLastPlacedAlpha[1];
		}
		else
		{
			lastPlaced1Color[3] = 0.0f;
			lastPlaced2Color[3] = 0.0f;
		}
	}
	//can't fire both portals, and we want the crosshair to remain somewhat symmetrical without being confusing
	else if ( !pPortalgun->CanFirePortal1() )
	{
		// clone portal2 info to portal 1
		portal1Color = portal2Color;
		lastPlaced1Color[3] = 0.0f;
		lastPlaced2Color[3] = 0.0f;
		bPortalPlacability[0] = bPortalPlacability[1];
	}
	else if ( !pPortalgun->CanFirePortal2() )
	{
		// clone portal1 info to portal 2
		portal2Color = portal1Color;
		lastPlaced1Color[3] = 0.0f;
		lastPlaced2Color[3] = 0.0f;
		bPortalPlacability[1] = bPortalPlacability[0];
	}

	if ( pPortalgun->IsHoldingObject() )
	{
		// Change the middle to orange 
		portal1Color = portal2Color = UTIL_Portal_Color( 0 );
		bPortalPlacability[0] = bPortalPlacability[1] = false;
	}
	
	if ( !hud_quickinfo_swap.GetBool() )
	{
		if ( bPortalPlacability[0] )
			m_icon_plbv->DrawSelf(xCenter - (m_icon_plbv->Width() * 0.64f ), yCenter - ( m_icon_prbv->Height() * 0.17f ), portal1Color);
		else
			m_icon_plbi->DrawSelf(xCenter - (m_icon_plbi->Width() * 0.64f ), yCenter - ( m_icon_prbv->Height() * 0.17f ), portal1Color);

		if ( bPortalPlacability[1] )
			m_icon_prbv->DrawSelf(xCenter + (m_icon_prbv->Width() * -0.35f ), yCenter + (m_icon_prbv->Height() * 0.17f ), portal2Color);
		else
			m_icon_prbi->DrawSelf(xCenter + (m_icon_prbi->Width() * -0.35f ), yCenter + (m_icon_prbv->Height() * 0.17f ), portal2Color);

		//last placed portal indicator
		m_icon_plp->DrawSelf( xCenter - (m_icon_plp->Width() * 1.85f), yCenter, lastPlaced1Color );
		m_icon_plp->DrawSelf( xCenter + (m_icon_plp->Width() * 0.75f), yCenter, lastPlaced2Color );
	}
	else
	{
		if ( bPortalPlacability[1] )
			m_icon_plbv->DrawSelf(xCenter - (m_icon_plbv->Width() * 0.64f ), yCenter - ( m_icon_prbv->Height() * 0.17f ), portal2Color);
		else
			m_icon_plbi->DrawSelf(xCenter - (m_icon_plbi->Width() * 0.64f ), yCenter - ( m_icon_prbv->Height() * 0.17f ), portal2Color);

		if ( bPortalPlacability[0] )
			m_icon_prbv->DrawSelf(xCenter + (m_icon_prbv->Width() * -0.35f ), yCenter + (m_icon_prbv->Height() * 0.17f ), portal1Color);
		else
			m_icon_prbi->DrawSelf(xCenter + ( m_icon_prbi->Width() * -0.35f ), yCenter + (m_icon_prbv->Height() * 0.17f ), portal1Color);

		//last placed portal indicator
		m_icon_plp->DrawSelf( xCenter - (m_icon_plp->Width() * 1.85f), yCenter, lastPlaced2Color );
		m_icon_plp->DrawSelf( xCenter + (m_icon_plp->Width() * 0.75f), yCenter, lastPlaced1Color );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHUDQuickInfo::UpdateEventTime( void )
{
	m_flLastEventTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHUDQuickInfo::EventTimeElapsed( void )
{
	if (( gpGlobals->curtime - m_flLastEventTime ) > QUICKINFO_EVENT_DURATION )
		return true;

	return false;
}

