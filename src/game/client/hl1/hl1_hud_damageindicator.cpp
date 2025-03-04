//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include "vguimatsurface/IMatSystemSurface.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMesh.h"
#include "materialsystem/imaterialvar.h"
#include "ieffects.h"
#include "hudelement.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: HDU Damage indication
//-----------------------------------------------------------------------------
class CHudDamageIndicatorHL1 : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDamageIndicatorHL1, vgui::Panel );

public:
	CHudDamageIndicatorHL1( const char *pElementName );

	void	Init( void );
	void	Reset( void );
	bool	ShouldDraw( void );

	// Handler for our message
	void	MsgFunc_Damage(bf_read &msg);

private:
	void	Paint();
	void	ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	void	CalcDamageDirection( const Vector &vecFrom );
	void	DrawDamageIndicatorFront( float flFade );
	void	DrawDamageIndicatorRear( float flFade );
	void	DrawDamageIndicatorLeft( float flFade );
	void	DrawDamageIndicatorRight( float flFade );

private:
	float	m_flAttackFront;
	float	m_flAttackRear;
	float	m_flAttackLeft;
	float	m_flAttackRight;

	Color	m_clrIndicator;

	CHudTexture	*icon_up;
	CHudTexture	*icon_down;
	CHudTexture	*icon_left;
	CHudTexture	*icon_right;
};

DECLARE_HUDELEMENT( CHudDamageIndicatorHL1 );
DECLARE_HUD_MESSAGE( CHudDamageIndicatorHL1, Damage );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudDamageIndicatorHL1::CHudDamageIndicatorHL1( const char *pElementName ) : CHudElement( pElementName ), BaseClass(NULL, "HudDamageIndicatorHL1")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudDamageIndicatorHL1::Reset( void )
{
	m_flAttackFront	= 0.0;
	m_flAttackRear	= 0.0;
	m_flAttackRight	= 0.0;
	m_flAttackLeft	= 0.0;

	m_clrIndicator.SetColor( 250, 0, 0, 255 );
}

void CHudDamageIndicatorHL1::Init( void )
{
	HOOK_HUD_MESSAGE( CHudDamageIndicatorHL1, Damage );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudDamageIndicatorHL1::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	if ( ( m_flAttackFront <= 0.0 ) && ( m_flAttackRear <= 0.0 ) && ( m_flAttackLeft <= 0.0 ) && ( m_flAttackRight <= 0.0 ) )
		return false;

	return true;
}

void CHudDamageIndicatorHL1::DrawDamageIndicatorFront( float flFade )
{
	if ( m_flAttackFront > 0.4 )
	{
		if ( !icon_up )
		{
			icon_up = gHUD.GetIcon( "pain_up" );
		}

		if ( !icon_up )
		{
			return;
		}

		int	x = ( ScreenWidth() / 2 ) - icon_up->Width() / 2;
		int	y = ( ScreenHeight() / 2 ) - icon_up->Height() * 3;
		icon_up->DrawSelf( x, y, m_clrIndicator );

		m_flAttackFront = max( 0.f, m_flAttackFront - flFade );
	}
	else
	{
		m_flAttackFront = 0.0;
	}
}

void CHudDamageIndicatorHL1::DrawDamageIndicatorRear( float flFade )
{
	if ( m_flAttackRear > 0.4 )
	{
		if ( !icon_down )
		{
			icon_down = gHUD.GetIcon( "pain_down" );
		}

		if ( !icon_down )
		{
			return;
		}

		int	x = ( ScreenWidth() / 2 ) - icon_down->Width() / 2;
		int	y = ( ScreenHeight() / 2 ) + icon_down->Height() * 2;
		icon_down->DrawSelf( x, y, m_clrIndicator );

		m_flAttackRear = max( 0.f, m_flAttackRear - flFade );
	}
	else
	{
		m_flAttackRear = 0.0;
	}
}


void CHudDamageIndicatorHL1::DrawDamageIndicatorLeft( float flFade )
{
	if ( m_flAttackLeft > 0.4 )
	{
		if ( !icon_left )
		{
			icon_left = gHUD.GetIcon( "pain_left" );
		}

		if ( !icon_left )
		{
			return;
		}

		int	x = ( ScreenWidth() / 2 ) - icon_left->Width() * 3;
		int	y = ( ScreenHeight() / 2 ) - icon_left->Height() / 2;
		icon_left->DrawSelf( x, y, m_clrIndicator );

		m_flAttackLeft = max( 0.f, m_flAttackLeft - flFade );
	}
	else
	{
		m_flAttackLeft = 0.0;
	}
}


void CHudDamageIndicatorHL1::DrawDamageIndicatorRight( float flFade )
{
	if ( m_flAttackRight > 0.4 )
	{
		if ( !icon_right )
		{
			icon_right = gHUD.GetIcon( "pain_right" );
		}

		if ( !icon_right )
		{
			return;
		}

		int	x = ( ScreenWidth() / 2 ) + icon_right->Width() * 2;
		int	y = ( ScreenHeight() / 2 ) - icon_right->Height() / 2;
		icon_right->DrawSelf( x, y, m_clrIndicator );

		m_flAttackRight = max( 0.f, m_flAttackRight - flFade );
	}
	else
	{
		m_flAttackRight = 0.0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Paints the damage display
//-----------------------------------------------------------------------------
void CHudDamageIndicatorHL1::Paint()
{
	// draw damage indicators
	float flFade = gpGlobals->frametime * 2;
	DrawDamageIndicatorFront( flFade );
	DrawDamageIndicatorRear( flFade );
	DrawDamageIndicatorLeft( flFade );
	DrawDamageIndicatorRight( flFade);
}

//-----------------------------------------------------------------------------
// Purpose: Message handler for Damage message
//-----------------------------------------------------------------------------
void CHudDamageIndicatorHL1::MsgFunc_Damage( bf_read &msg )
{
	int armor		=	msg.ReadByte();	// armor
	int damageTaken	=	msg.ReadByte();	// health
						msg.ReadLongLong();	// damage bits, ignore
	Vector vecFrom;
	vecFrom.x		=	msg.ReadFloat();
	vecFrom.y		=	msg.ReadFloat();
	vecFrom.z		=	msg.ReadFloat();

	if ( damageTaken > 0 || armor > 0 )
	{
		CalcDamageDirection( vecFrom );
	}
}

void CHudDamageIndicatorHL1::CalcDamageDirection( const Vector &vecFrom )
{
	if ( vecFrom == vec3_origin )
	{
		m_flAttackFront	= 0.0;
		m_flAttackRear	= 0.0;
		m_flAttackRight	= 0.0;
		m_flAttackLeft	= 0.0;

		return;
	}

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
	{
		return;
	}

	Vector vecDelta = ( vecFrom - pLocalPlayer->GetRenderOrigin() );

	if ( vecDelta.Length() <= 50 )
	{
		m_flAttackFront	= 1.0;
		m_flAttackRear	= 1.0;
		m_flAttackRight	= 1.0;
		m_flAttackLeft	= 1.0;

		return;
	}

	VectorNormalize( vecDelta );

	Vector forward;
	Vector right;
	AngleVectors( MainViewAngles(), &forward, &right, NULL );

	float flFront	= DotProduct( vecDelta, forward );
	float flSide	= DotProduct( vecDelta, right );

	if ( flFront > 0 )
	{
		if ( flFront > 0.3 )
			m_flAttackFront = max( m_flAttackFront, flFront );
	}
	else
	{
		float f = fabsf( flFront );
		if ( f > 0.3 )
			m_flAttackRear = max( m_flAttackRear, f );
	}

	if ( flSide > 0 )
	{
		if ( flSide > 0.3 )
			m_flAttackRight = max( m_flAttackRight, flSide );
	}
	else
	{
		float f = fabsf( flSide );
		if ( f > 0.3 )
			m_flAttackLeft = max( m_flAttackLeft, f );
	}
}


//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudDamageIndicatorHL1::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	int wide, tall;
	GetHudSize(wide, tall);
	SetSize(wide, tall);
}
