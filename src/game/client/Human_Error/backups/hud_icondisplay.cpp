//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud_icondisplay.h"
#include "iclientmode.h"

#include <Color.h>
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>

#include "ImageFX.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudIconDisplay::CHudIconDisplay(vgui::Panel *parent, const char *name) : BaseClass(parent, name)
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_iValue = 0;
	//m_LabelText[0] = 0;
	m_Icon[0] = 0;
	m_iSecondaryValue = 0;
	m_bDisplayValue = true;
	m_bDisplaySecondaryValue = false;

	m_iMaxValue = 100;
	m_iMaxSecondaryValue = 0;

	m_bRightSide = true;

	m_bCircle	 = false;

	m_pBar			= NULL;
	m_pBarSecondary = NULL;
	m_pBase			= NULL;
}

void CHudIconDisplay::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	/*m_hNumberFont = pScheme->GetFont( "HE_HudFont" );
	m_hNumberGlowFont = pScheme->GetFont( "HE_HudFontGlow" );
	m_hSmallNumberFont = pSceheme->GetFont*/

	SetPaintBackgroundEnabled( false );
}

void CHudIconDisplay::ShowImages(bool show)
{
	if (m_pBase)
	{
		m_pBase->SetVisibleEx(show);
	}

	if (m_pBar)
	{
		m_pBar->SetVisibleEx(show);
	}

	if (m_pBarSecondary)
	{
		m_pBarSecondary->SetVisibleEx(show);
	}
}

void CHudIconDisplay::SetIcon(wchar_t *icon, int maxValue, int maxSecondaryValue )
{
	wcsncpy(m_Icon, icon, sizeof(m_Icon) / sizeof(wchar_t));
	m_Icon[(sizeof(m_Icon) / sizeof(wchar_t)) - 1] = 0;

	m_iMaxValue			 = maxValue;
	m_iMaxSecondaryValue = maxSecondaryValue;
}

//-----------------------------------------------------------------------------
// Purpose: Resets values on restore/new map
//-----------------------------------------------------------------------------
void CHudIconDisplay::Reset()
{
	m_flBlur = 0.0f;

	SetPaintBackgroundEnabled( false );

}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetDisplayValue(int value)
{
	m_iValue = value;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetSecondaryValue(int value)
{
	m_iSecondaryValue = value;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetShouldDisplayValue(bool state)
{
	m_bDisplayValue = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetShouldDisplaySecondaryValue(bool state)
{
	m_bDisplaySecondaryValue = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
/*void CHudIconDisplay::SetLabelText(const wchar_t *text)
{
	wcsncpy(m_LabelText, text, sizeof(m_LabelText) / sizeof(wchar_t));
	m_LabelText[(sizeof(m_LabelText) / sizeof(wchar_t)) - 1] = 0;
}*/

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudIconDisplay::SetDrawGap(bool state)
{
	m_bDrawGap = state;
}

//-----------------------------------------------------------------------------
// Purpose: paints a number at the specified position
//-----------------------------------------------------------------------------
void CHudIconDisplay::PaintIcons(HFont font, int y, int value, int max)
{
	//TERO: FIRST TRY
	/*int initialY = GetTall() - 32; // * 0.9; 
	int Tall	 = GetTall() * 0.8;
	int Wide	 = (surface()->GetCharacterWidth(font, '0'));
	int iconGap  = surface()->GetFontTall(font); 
	float calcX;
	float calcY;

	surface()->DrawSetTextFont(font);

	//#define width (Wide * (GetWide()/1024));
	int width = (iconGap - 4);
	//#define wide (GetWide() - 32)

	//TERO: lets calculate the rectan positions

	//
	for (int i=0; i<=value; i+=m_iValueGap)
	{
		calcY = (float) initialY - (float)(iconGap * ( (float) ((float)i/(float)m_iValueGap) + 1) );

		if (icons_xpos>0)
			calcX = (float)( width *  (sin( (float) calcY / Tall * 3.14 ) ) + Wide + x);  
		else
			calcX = (float) GetWide() - ( width *  (sin( (float) calcY / Tall * 3.14 ) ) + Wide + x);

		if (value==100)
		{
			DevMsg("initialY: %d, ", initialY);
			DevMsg("iconGap: %d, ", iconGap);
			DevMsg("m_iValueGap: %d, ", m_iValueGap );
			DevMsg("calcY: %f, ", calcY);
			DevMsg("calcX: %f, \n", calcX);
		}

		surface()->DrawSetTextPos((int)calcX, (int)calcY);
		surface()->DrawUnicodeString( m_Icon );
	}*/

	//TERO: ATTEMPT TWO
	/*int initialY = GetTall() * 0.9; 
	int Tall	 = GetTall() * 0.8;
	int Wide	 = (surface()->GetCharacterWidth(font, '0'))+ 4;
	int fontTall = surface()->GetFontTall(font);

	//if (!m_bDrawGap)
		Wide	 *= 0.6;

	float test   = (float)Tall/max;
	if (test > fontTall)
	{

		Tall = Tall * ((float)fontTall/test);
	}

	float bar	 = ((float)value/(float)max) * Tall;
	
	
	surface()->DrawSetColor(GetBgColor());
	surface()->DrawOutlinedRect(x,initialY-bar,x+Wide, initialY + fontTall);
	surface()->DrawSetColor(GetFgColor());
	surface()->DrawFilledRect(x,initialY-bar,x+Wide, initialY + fontTall );
	surface()->DrawLine(x, initialY, x+Wide, initialY);

	surface()->DrawSetTextFont(font);
	surface()->DrawSetTextPos(x+4, initialY-bar+1);
	surface()->DrawUnicodeString( m_Icon );



	if (m_bDrawGap)
	{
		int gap= Tall / max;
		for (int i=1; i<value; i++)
		{

			surface()->DrawLine(x, initialY - (i*gap), x+Wide, initialY - (i*gap));
		}
	}*/

	//TERO: THIRD TRY
	/*
	int Tall	 = GetTall();
		int Wide	 = GetWide() - (surface()->GetCharacterWidth(font, '0'));
		int fontTall = 32; //surface()->GetFontTall(font);// * ((float)Tall/40); //surface()->GetFontTall(font);
		int fontWide = surface()->GetCharacterWidth(font, '0');
	
		int ypos;

		if (!m_bDisplaySecondaryValue)
			ypos	 = (Tall / 2) - (fontTall / 2) + y;
		else
			ypos	 = (Tall / 4) - (fontTall / 2) + y;

		surface()->DrawSetTextFont(font);
		surface()->DrawSetTextPos(2, ypos - 20);
		surface()->DrawUnicodeString( m_Icon );

		float bar	 = ((float)value/(float)max) * Wide;
	
		surface()->DrawSetColor(GetBgColor());
		surface()->DrawOutlinedRect(fontWide,     ypos,     fontWide + bar,     ypos + fontTall );
		surface()->DrawOutlinedRect(fontWide + 2, ypos + 2, fontWide + bar - 2, ypos + fontTall - 2);
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawFilledRect(fontWide, ypos, fontWide + bar, ypos + fontTall);

		//Draw the lines, but only one given
		Wide-=4;
		int gap = Wide / max;
		if (gap<2)
		{
			gap=2;
		}
		for (int i=1; i<bar; i+=gap)
		{
			//surface()->DrawLine(x, initialY - (i*gap), x+Wide, initialY - i);
			surface()->DrawLine(fontWide+i, ypos+2, fontWide+i, ypos + fontTall-2);
		}

	*/

	//FOURTH TRY

	if (m_bCircle)
	{

		float bar = (float)value/max;

		bar = clamp(bar, 0,1);

		float xpos, ypos;
		float width = GetWide();
		float height = GetTall();

		float angle = -(1-bar) * 0.2; //0.785;

		//First lets draw the icon
		xpos = (float)icon_xpos - (width * 3.4);
		ypos = (float)icon_ypos  - (height/2);

		xpos = (xpos * cos(angle)) - (ypos * sin(angle)) + (width * 3.4);
		ypos = (xpos * sin(angle)) + (ypos * cos(angle)) + (height/2);

		surface()->DrawSetTextFont(font);
		surface()->DrawSetTextPos((int)xpos, (int)ypos);
		surface()->DrawUnicodeString( m_Icon );

		//Then lets draw the value
		wchar_t unicode[6];
		swprintf(unicode, L"%d", value);

		xpos = (float)digi_xpos - (width * 3.4);
		ypos = (float)digi_ypos  - (height/2);

		xpos = (xpos * cos(angle)) - (ypos * sin(angle)) + (width * 3.4);
		ypos = (xpos * sin(angle)) + (ypos * cos(angle)) + (height/2);

		surface()->DrawSetTextFont(m_hSmallNumberFont);
		surface()->DrawSetTextPos((int)xpos, (int)ypos);
		surface()->DrawUnicodeString( unicode );
	}
	else
	{

		surface()->DrawSetTextFont(font);
		surface()->DrawSetTextPos((int)icon_xpos, (int)icon_ypos);
		surface()->DrawUnicodeString( m_Icon );

		if (!m_bRightSide)
			return;

		float xpos, ypos;

		xpos = digi_ypos; //*(float)GetWide()/100.0f;
		ypos = digi_ypos; //*(float)GetTall()/100.0f;

		wchar_t unicode[6];
		if (m_bDisplaySecondaryValue)
			swprintf(unicode, L"%d/%d", value, m_iSecondaryValue);
		else
			swprintf(unicode, L"%d", value);

		// adjust the position to take into account 3 characters
		int charWidth = surface()->GetCharacterWidth(m_hSmallNumberFont, '0')*1.2;
		if (m_bRightSide && value >= 100)
		{
			xpos -= charWidth;
		}
		if (m_bRightSide && value >= 10)
		{
			xpos -= charWidth;
		}

		//DevMsg("up: %f, ", ypos);
		//DevMsg("right: %f\n", xpos);

		surface()->DrawSetTextFont(m_hSmallNumberFont);
		surface()->DrawSetTextPos((int)xpos, (int)ypos);
		surface()->DrawUnicodeString( unicode );
	}
}


void CHudIconDisplay::PaintCircleBar(HFont font, ImageFX *pBar, int value, int max)
{
	if (pBar)
	{	
		float bar = (float)value/max;

		bar = clamp(bar, 0,1);

		Vertex_t *points = pBar->GetPoints();

		float xpos, ypos;

		float width = GetWide();
		float height = GetTall();

		if (m_bCircle)
		{
			ypos = 1 - bar;

			width = width;// *0.8;
			height = height *0.8;

			points[0].m_TexCoord = Vector2D(0,	ypos);
			points[1].m_TexCoord = Vector2D(3.4,	0.5);
			points[2].m_TexCoord = Vector2D(3.4,	0.5);
			points[3].m_TexCoord = Vector2D(0,	1);

			//DevMsg("up: %f, ", ypos);
			//DevMsg("right: %f\n", xpos);

			points[0].m_Position = Vector2D(0,	height * ypos);
			points[1].m_Position = Vector2D(width * 3.4,			height/2);
			points[2].m_Position = Vector2D(width * 3.4,			height/2);
			points[3].m_Position = Vector2D(0,		height);

			float angle = 0;
			angle = -ypos * 0.2; //0.785;
			//DevMsg("Angle: %f\n", angle);

			for (int i=0; i<4; i++)
			{
				/*DevMsg("Original point %d: ", i);
				DevMsg("x = %f ", points[i].m_Position.x);
				DevMsg("y = %f\n", points[i].m_Position.y);*/

				xpos = points[i].m_Position.x - (width * 3.4);
				ypos = points[i].m_Position.y - (height/2);

				points[i].m_Position.x = (xpos * cos(angle)) - (ypos * sin(angle)) + (width * 3.4);
				points[i].m_Position.y = (xpos * sin(angle)) + (ypos * cos(angle)) + (height/2);

				/*DevMsg("Rotated point %d: ", i);
				DevMsg("x = %f ", points[i].m_Position.x);
				DevMsg("y = %f\n", points[i].m_Position.y);*/
			}
		}
		else if (m_bRightSide)
		{
			
			if (bar>0.5)
			{
				xpos = (1 - bar) * 2;
				ypos = 0;
			} else
			{
				xpos = 1;
				ypos = (1 - (bar * 2));
			}

			points[0].m_TexCoord = Vector2D(xpos,	ypos);
			points[1].m_TexCoord = Vector2D(1,		ypos);
			points[2].m_TexCoord = Vector2D(1,		1);
			points[3].m_TexCoord = Vector2D(0.5,	0.5);

			//DevMsg("up: %f, ", ypos);
			//DevMsg("right: %f\n", xpos);

			points[0].m_Position = Vector2D(width * xpos,	height * ypos);
			points[1].m_Position = Vector2D(width,			height * ypos);
			points[2].m_Position = Vector2D(width,			height);
			points[3].m_Position = Vector2D(width/2,		height/2);

		} else
		{
			if (bar>0.5)
			{
				xpos = ((bar * 2) - 1) * 0.875;
				ypos = 0;
			} else
			{
				xpos = 0;
				ypos = (1 - (bar * 2)) * 0.875;
			}

			points[0].m_TexCoord = Vector2D(0,		ypos);
			points[1].m_TexCoord = Vector2D(xpos,	ypos);
			points[2].m_TexCoord = Vector2D(0.875,	0.875);
			points[3].m_TexCoord = Vector2D(0,		0.875);

			//DevMsg("up: %f, ", ypos);
			//DevMsg("right: %f\n", xpos);

			points[0].m_Position = Vector2D(0,				height * ypos);
			points[1].m_Position = Vector2D(width * xpos,	height * ypos);
			points[2].m_Position = Vector2D(width * 0.875,	height*0.875);
			points[3].m_Position = Vector2D(0,				height*0.875);

			m_pBar->SetColor( GetFgColor() );
		}

		/*surface()->DrawLine(points[0].m_Position.x+1, points[0].m_Position.y+1, points[1].m_Position.x-1, points[1].m_Position.y+1);
		surface()->DrawLine(points[1].m_Position.x-1, points[1].m_Position.y+1, points[2].m_Position.x-1, points[2].m_Position.y-1);
		surface()->DrawLine(points[2].m_Position.x-1, points[2].m_Position.y-1, points[3].m_Position.x+1, points[3].m_Position.y-1);
		surface()->DrawLine(points[3].m_Position.x+1, points[3].m_Position.y-1, points[0].m_Position.x+1, points[0].m_Position.y-1);
		*/

		pBar->SetPoints(points, 4);
	}
}

void CHudIconDisplay::PaintCircleBase(ImageFX *pBase, int value, int max)
{
	if (!m_bCircle)
		return;

	if (pBase)
	{	
		float bar = (float)value/max;

		bar = clamp(bar, 0,1);

		Vertex_t *points = pBase->GetPoints();

		float xpos, ypos;

		float width = GetWide();
		float height = GetTall();

		ypos = 1 - bar;

		width = width;// *0.8;

		float angle = -ypos * 0.2; //0.785;

		for (int i=0; i<4; i++)
		{
			xpos = points[i].m_Position.x - (width * 3.4);
			ypos = points[i].m_Position.y - (height/2);

			points[i].m_Position.x = (xpos * cos(angle)) - (ypos * sin(angle)) + (width * 3.4);
			points[i].m_Position.y = (xpos * sin(angle)) + (ypos * cos(angle)) + (height/2);
		}

		pBase->SetPoints(points, 4);
	}
}


//-----------------------------------------------------------------------------
// Purpose: draws the text
//-----------------------------------------------------------------------------
/*void CHudIconDisplay::PaintLabel( void )
{
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(GetFgColor());
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	surface()->DrawUnicodeString( m_LabelText );
}*/

//-----------------------------------------------------------------------------
// Purpose: renders the vgui panel
//-----------------------------------------------------------------------------
void CHudIconDisplay::Paint()
{
	if (m_bDisplayValue)
	{
		PaintCircleBase(m_pBase, m_iValue, m_iMaxValue);

		PaintCircleBar(m_hNumberFont, m_pBar, m_iValue, m_iMaxValue);

		// draw our numbers
		surface()->DrawSetTextColor(GetFgColor());
		PaintIcons(m_hNumberFont, 0, m_iValue, m_iMaxValue);

		// draw the overbright blur
		for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
		{
			if (fl >= 1.0f)
			{
				PaintIcons(m_hNumberGlowFont, 0, m_iValue, m_iMaxValue);
			}
			else
			{
				// draw a percentage of the last one
				Color col = GetFgColor();
				col[3] *= fl;
				surface()->DrawSetTextColor(col);
				PaintIcons(m_hNumberGlowFont, 0, m_iValue, m_iMaxValue);
			}
		}
	}

	// total ammo
	if (m_bDisplaySecondaryValue)
	{
		surface()->DrawSetTextColor(GetFgColor());
		PaintCircleBar(m_hNumberFont, m_pBarSecondary, m_iSecondaryValue, m_iMaxSecondaryValue);
	}
}



