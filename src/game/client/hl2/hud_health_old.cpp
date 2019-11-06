/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// Health.cpp
//
// implementation of CHudHealthOld class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#define PAIN_NAME "sprites/%d_pain.vmt"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

using namespace vgui;

#include "hudelement.h"

#include "hud_bitmapnumericdisplay.h"


#define INIT_HEALTH -1

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudHealthOld : public CHudElement, public CHudBitmapNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudHealthOld, CHudBitmapNumericDisplay );

public:
	CHudHealthOld( const char *pElementName );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void			OnThink();
//	void MsgFunc_HealthOld(const char *pszName, int iSize, void *pbuf);
	void MsgFunc_Damage(bf_read& msg);

private:
	// old variables
	int		m_iHealth;
	
	float	m_fFade;
	int		m_bitsDamage;
	int		m_iGhostHealth;
};	

DECLARE_HUDELEMENT( CHudHealthOld );
DECLARE_HUD_MESSAGE( CHudHealthOld, Damage );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealthOld::CHudHealthOld( const char *pElementName ) : CHudElement( pElementName ), CHudBitmapNumericDisplay(NULL, "HudHealthOld")
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthOld::Init()
{
	HOOK_HUD_MESSAGE(CHudHealthOld, Damage);
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthOld::Reset()
{
	m_iHealth		= INIT_HEALTH;
	m_iGhostHealth	= 100;
	m_fFade			= 0;
	m_bitsDamage	= 0;

	SetLabelText(L"HEALTH");
	SetDisplayValue(m_iHealth);

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthOld::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthOld::OnThink()
{

	int x = 0;
	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
	{
		// Never below zero
		x = max( local->GetHealth(), 0 );
	}

	// Only update the fade if we've changed health
	if ( x == m_iHealth )
	{
		return;
	}

	m_iGhostHealth = m_iHealth;
	m_fFade = 200;
	m_iHealth = x;

	/*bool restored = GetGameRestored();
	if ( restored )
	{
		SetGameRestored( false );
	}*/

	if ( m_iHealth >= 20 )
	{
		// Don't flash on save/load restoration
		//if ( !restored )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedAbove20Old");
		}
	}
	else
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthIncreasedBelow20Old");
	}

	SetDisplayValue(m_iHealth);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthOld::MsgFunc_Damage(bf_read& msg)
{
	int armor = msg.ReadByte();	// armor
	int damageTaken = msg.ReadByte();	// health
	long bitsDamage = msg.ReadLong(); // damage bits
	bitsDamage; // variable still sent but not used

	Vector vecFrom;

	vecFrom.x = msg.ReadBitCoord();
	vecFrom.y = msg.ReadBitCoord();
	vecFrom.z = msg.ReadBitCoord();

	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
	{
		if ( damageTaken > 0 )
		{
			// start the animation
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthDamageTakenOld");

			// see if our health is low
			if ( m_iHealth < 20 )
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HealthLowOld");
			}
		}
	}
}