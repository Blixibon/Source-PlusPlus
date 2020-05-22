//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hud locator element, helps direct the player to objects in the world
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>
#include "hud.h"
#include "hud_suitpower.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "c_laz_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LOCATOR_MATERIAL_JALOPY				"vgui/icons/icon_jalopy"
#define LOCATOR_MATERIAL_MAGNUSSEN_RDU		"vgui/icons/icon_buster2"	// Striderbuster
#define LOCATOR_MATERIAL_DOG				"vgui/icons/icon_dog2"		// Dog
#define LOCATOR_MATERIAL_ALLY_INSTALLATION	"vgui/icons/icon_base2"		// Ally base

//TE120--
#define LOCATOR_MATERIAL_GENERIC		"vgui/icons/icon_lambda2"
#define LOCATOR_MATERIAL_AMMO			"vgui/icons/icon_ammo"
#define LOCATOR_MATERIAL_HEALTH			"vgui/icons/icon_health"
#define LOCATOR_MATERIAL_LARGE_ENEMY	"vgui/icons/icon_strider2"
//TE120--
#define LOCATOR_MATERIAL_BIG_TICK		"vgui/icons/tick_long"
#define LOCATOR_MATERIAL_SMALL_TICK		"vgui/icons/tick_short"
//TE120--
#define LOCATOR_MATERIAL_BIG_TICK_N		"vgui/icons/tick_long_n"
#define LOCATOR_MATERIAL_RADIATION		"vgui/icons/icon_radiation"
//TE120--

ConVar hud_locator_alpha( "hud_locator_alpha", "230" );
ConVar hud_locator_fov("hud_locator_fov", "350" );

//-----------------------------------------------------------------------------
// Purpose: Shows positions of objects relative to the player.
//-----------------------------------------------------------------------------
class CHudLocator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudLocator, vgui::Panel );

public:
	CHudLocator( const char *pElementName );
	virtual ~CHudLocator( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	void VidInit( void );
	bool ShouldDraw();

protected:
	void FillRect( int x, int y, int w, int h );
	float LocatorXPositionForYawDiff( float yawDiff );
	void DrawGraduations( float flYawPlayerFacing );
	virtual void Paint();

private:
	void Reset( void );

	int m_textureID_IconJalopy;
	int m_textureID_IconGeneric;
	int m_textureID_IconAmmo;
	int m_textureID_IconHealth;
	int m_textureID_IconBigEnemy;
	int m_textureID_IconBigTick;
	int m_textureID_IconSmallTick;
	int m_textureID_IconBigTickN;
	int m_textureID_IconRadiation;
	int	m_textureID_IconBuster;
	int	m_textureID_IconDog;
	int	m_textureID_IconBase;

	Vector			m_vecLocation;
};	

using namespace vgui;

#ifdef HL2_EPISODIC
DECLARE_HUDELEMENT( CHudLocator );
#endif 

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudLocator::CHudLocator( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudLocator" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_textureID_IconJalopy = -1;
	m_textureID_IconBuster = -1;
	m_textureID_IconDog = -1;
	m_textureID_IconBase = -1;
	//TE120--
	m_textureID_IconGeneric = -1;
	m_textureID_IconAmmo = -1;
	m_textureID_IconHealth = -1;
	m_textureID_IconBigEnemy = -1;
	//TE120--
	m_textureID_IconSmallTick = -1;
	m_textureID_IconBigTick = -1;
	//TE120--
	m_textureID_IconBigTickN = -1;
	m_textureID_IconRadiation = -1;
}

CHudLocator::~CHudLocator( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pScheme - 
//-----------------------------------------------------------------------------
void CHudLocator::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHudLocator::VidInit( void )
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CHudLocator::ShouldDraw( void )
{
	C_Laz_Player* pPlayer = ToLazuulPlayer(C_BasePlayer::GetLocalPlayer());
	if ( !pPlayer || pPlayer->IsObserver())
		return false;

	if (pPlayer->m_LazLocal.m_flLocatorRange <= 0.f)
	{
		if (pPlayer->GetVehicle())
			return false;

		if (pPlayer->m_HL2Local.m_vecLocatorOrigin == vec3_invalid)
			return false;
	}
	
	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Start with our background off
//-----------------------------------------------------------------------------
void CHudLocator::Reset( void )
{
	m_vecLocation = Vector( 0, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Make it a bit more convenient to do a filled rect.
//-----------------------------------------------------------------------------
void CHudLocator::FillRect( int x, int y, int w, int h )
{
	int panel_x, panel_y, panel_w, panel_h;
	GetBounds( panel_x, panel_y, panel_w, panel_h );
	vgui::surface()->DrawFilledRect( x, y, x+w, y+h );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float CHudLocator::LocatorXPositionForYawDiff( float yawDiff )
{
	float fov = hud_locator_fov.GetFloat() / 2;
	float remappedAngle = RemapVal( yawDiff, -fov, fov, -90, 90 );
	float cosine = sin(DEG2RAD(remappedAngle));
	int element_wide = GetWide();
	
	float position = (element_wide>>1) + ((element_wide>>1) * cosine);

	return position;
}

//-----------------------------------------------------------------------------
// Draw the tickmarks on the locator
//-----------------------------------------------------------------------------
#define NUM_GRADUATIONS	16.0f
void CHudLocator::DrawGraduations( float flYawPlayerFacing )
{
	int icon_wide, icon_tall;
	int xPos, yPos;
	float fov = hud_locator_fov.GetFloat() / 2;

	if( m_textureID_IconBigTick == -1 )
	{
		m_textureID_IconBigTick = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_IconBigTick, LOCATOR_MATERIAL_BIG_TICK, true, false );
	}

	if( m_textureID_IconSmallTick == -1 )
	{
		m_textureID_IconSmallTick = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_IconSmallTick, LOCATOR_MATERIAL_SMALL_TICK, true, false );
	}

	if (m_textureID_IconBigTickN == -1)
	{
		//TE120--
		m_textureID_IconBigTickN = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconBigTickN, LOCATOR_MATERIAL_BIG_TICK_N, true, false);
		//TE120--
	}

	int element_tall = GetTall();		// Height of the VGUI element

	surface()->DrawSetColor(GetFgColor());

	// Tick Icons

	const float angleStep = 360.0f / NUM_GRADUATIONS;
	bool tallLine = true;

	for( float angle = -180 ; angle <= 180 ; angle += angleStep )
	{
		yPos = (element_tall>>1);

		if( tallLine )
		{
			//TE120--
			if (angle == 90)
			{
				vgui::surface()->DrawSetTexture(m_textureID_IconBigTickN);
				vgui::surface()->DrawGetTextureSize(m_textureID_IconBigTickN, icon_wide, icon_tall);
			}
			else
			{
				vgui::surface()->DrawSetTexture(m_textureID_IconBigTick);
				vgui::surface()->DrawGetTextureSize(m_textureID_IconBigTick, icon_wide, icon_tall);
			}
			//TE120--
			tallLine = false;
		}
		else
		{
			vgui::surface()->DrawSetTexture( m_textureID_IconSmallTick );
			vgui::surface()->DrawGetTextureSize( m_textureID_IconSmallTick, icon_wide, icon_tall );
			tallLine = true;
		}

		float flDiff = UTIL_AngleDiff( flYawPlayerFacing, angle );

		if( fabsf(flDiff) > fov )
			continue;

		float xPosition = LocatorXPositionForYawDiff( flDiff );

		xPos = (int)xPosition;
		xPos -= (icon_wide>>1);

		vgui::surface()->DrawTexturedRect(xPos, yPos, xPos+icon_wide, yPos+icon_tall);
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws the locator icons on the VGUI element.
//-----------------------------------------------------------------------------
void CHudLocator::Paint()
{
#ifdef HL2_EPISODIC

	if( m_textureID_IconJalopy == -1 )
	{
		m_textureID_IconJalopy = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_IconJalopy, LOCATOR_MATERIAL_JALOPY, true, false );
	}

	//TE120--
	if (m_textureID_IconGeneric == -1)
	{
		m_textureID_IconGeneric = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconGeneric, LOCATOR_MATERIAL_GENERIC, true, false);

		m_textureID_IconAmmo = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconAmmo, LOCATOR_MATERIAL_AMMO, true, false);

		m_textureID_IconHealth = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconHealth, LOCATOR_MATERIAL_HEALTH, true, false);

		m_textureID_IconBigEnemy = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconBigEnemy, LOCATOR_MATERIAL_LARGE_ENEMY, true, false);

		m_textureID_IconRadiation = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconRadiation, LOCATOR_MATERIAL_RADIATION, true, false);

		m_textureID_IconBuster = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconBuster, LOCATOR_MATERIAL_MAGNUSSEN_RDU, true, false);

		m_textureID_IconDog = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconDog, LOCATOR_MATERIAL_DOG, true, false);

		m_textureID_IconBase = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile(m_textureID_IconBase, LOCATOR_MATERIAL_ALLY_INSTALLATION, true, false);
	}
	//TE120--

	int alpha = hud_locator_alpha.GetInt();

	SetAlpha( alpha );

	C_Laz_Player* pPlayer = ToLazuulPlayer(C_BasePlayer::GetLocalPlayer());
	if ( !pPlayer )
		return;

	bool bHasEP2JalopyTarget = (pPlayer->m_HL2Local.m_vecLocatorOrigin != vec3_invalid);

	int element_tall = GetTall();		// Height of the VGUI element

	float fov = (hud_locator_fov.GetFloat()) / 2.0f;

	// Compute the relative position of objects we're tracking
	// We'll need the player's yaw for comparison.
	float flYawPlayerForward = pPlayer->EyeAngles().y;

	DrawGraduations(flYawPlayerForward);

	float flRangeEnd = pPlayer->m_LazLocal.m_flLocatorRange;
	float flRangeStart = flRangeEnd / 8.f;

	// Draw icons, if any
	for (int i = 0; i < pPlayer->m_LazLocal.m_iNumLocatorContacts; i++)
	{
		C_BaseEntity* pEnt = pPlayer->m_LazLocal.m_hLocatorEntities[i];
		Vector vecLocation = vec3_invalid;

		if (pPlayer->m_LazLocal.m_vLocatorPositions[i] != vec3_invalid)
		{
			vecLocation = pPlayer->m_LazLocal.m_vLocatorPositions[i];
		}
		else if (pEnt)
		{
			vecLocation = pEnt->WorldSpaceCenter();
		}

		if (vecLocation != vec3_invalid)
		{
			Vector vecToLocation = vecLocation - pPlayer->GetAbsOrigin();
			QAngle locationAngles;

			VectorAngles(vecToLocation, locationAngles);
			float yawDiff = UTIL_AngleDiff(flYawPlayerForward, locationAngles.y);
			bool bObjectInFOV = (yawDiff > -fov && yawDiff < fov);

			// Draw the icons!
			int icon_wide, icon_tall;
			int xPos, yPos;

			if (pPlayer->m_LazLocal.m_iLocatorContactType[i] == LOCATOR_CONTACT_PLAYER_VEHICLE && (bHasEP2JalopyTarget || pPlayer->GetVehicle()))
			{
				bObjectInFOV = false;
			}

			if (bObjectInFOV)
			{
				Color clrDrawIcon(255, 255, 255, 255);
				Vector vecPos = vecLocation;
				float x_diff = vecPos.x - pPlayer->GetAbsOrigin().x;
				float y_diff = vecPos.y - pPlayer->GetAbsOrigin().y;
				float flDist = sqrt(((x_diff) * (x_diff)+(y_diff) * (y_diff)));

				if (flDist <= flRangeEnd)
				{
					// The object's location maps to a valid position along the tape, so draw an icon.
					float tapePosition = LocatorXPositionForYawDiff(yawDiff);
					// Msg("tapePosition: %f\n", tapePosition);
					pPlayer->m_LazLocal.m_flTapePos[i] = tapePosition;

					// derive a scale for the locator icon
					yawDiff = fabs(yawDiff);
					float scale = 0.55f;
					float xscale = RemapValClamped(yawDiff, (fov / 4), fov, 1.0f, 0.25f);

					switch (pPlayer->m_LazLocal.m_iLocatorContactType[i])
					{
					case LOCATOR_CONTACT_GENERIC:
						vgui::surface()->DrawSetTexture(m_textureID_IconGeneric);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconGeneric, icon_wide, icon_tall);
						clrDrawIcon = GetFgColor();
						break;
					case LOCATOR_CONTACT_AMMO:
						vgui::surface()->DrawSetTexture(m_textureID_IconAmmo);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconAmmo, icon_wide, icon_tall);
						clrDrawIcon = GetFgColor();
						break;
					case LOCATOR_CONTACT_HEALTH:
						vgui::surface()->DrawSetTexture(m_textureID_IconHealth);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconHealth, icon_wide, icon_tall);
						clrDrawIcon = GetFgColor();
						break;
					case LOCATOR_CONTACT_LARGE_ENEMY:
						vgui::surface()->DrawSetTexture(m_textureID_IconBigEnemy);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconBigEnemy, icon_wide, icon_tall);
						break;
					case LOCATOR_CONTACT_RADIATION:
						vgui::surface()->DrawSetTexture(m_textureID_IconRadiation);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconRadiation, icon_wide, icon_tall);
						clrDrawIcon = GetFgColor();
						break;
					case LOCATOR_CONTACT_MAGNUSSEN_RDU:
						vgui::surface()->DrawSetTexture(m_textureID_IconBuster);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconBuster, icon_wide, icon_tall);
						break;
					case LOCATOR_CONTACT_DOG:
						vgui::surface()->DrawSetTexture(m_textureID_IconDog);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconDog, icon_wide, icon_tall);
						clrDrawIcon = GetFgColor();
						break;
					case LOCATOR_CONTACT_ALLY_INSTALLATION:
						vgui::surface()->DrawSetTexture(m_textureID_IconBase);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconBase, icon_wide, icon_tall);
						break;
					case LOCATOR_CONTACT_PLAYER_VEHICLE:
						vgui::surface()->DrawSetTexture(m_textureID_IconJalopy);
						vgui::surface()->DrawGetTextureSize(m_textureID_IconJalopy, icon_wide, icon_tall);
						clrDrawIcon = GetFgColor();
						break;

					default:
						break;
					}

					float flIconWide = ((float)element_tall * 1.25f);
					float flIconTall = ((float)element_tall * 1.25f);

					// Scale the icon based on distance
					float flDistScale = 1.0f;
					if (flDist > flRangeStart)
					{
						flDistScale = RemapValClamped(flDist, flRangeStart, flRangeEnd, 1.0f, 0.5f);
					}

					// Put back into ints
					icon_wide = (int)flIconWide;
					icon_tall = (int)flIconTall;

					// Positon Scale
					icon_wide *= xscale;

					// Distance Scale Icons
					icon_wide *= flDistScale;
					icon_tall *= flDistScale;

					// Global Scale Icons
					icon_wide *= scale;
					icon_tall *= scale;
					//Msg("yawDiff:%f  xPos:%d  scale:%f\n", yawDiff, xPos, scale );

					// Center the icon around its position.
					xPos = (int)tapePosition;
					xPos -= (icon_wide >> 1);
					yPos = (element_tall >> 1) - (icon_tall >> 1);

					// If this overlaps the last drawn items reduce opacity
					float fMostOverlapDist = 14.0f;
					if (pPlayer->m_LazLocal.m_iLocatorContactType[i] != LOCATOR_CONTACT_LARGE_ENEMY)
					{
						for (int j = i - 1; j >= 0; j--)
						{
							{
								if (pPlayer->m_LazLocal.m_flTapePos[j] > 0)
								{
									float fDiff = abs(pPlayer->m_LazLocal.m_flTapePos[j] - tapePosition);
									if (fMostOverlapDist > fDiff)
										fMostOverlapDist = fDiff;
								}
							}
						}
					}

					// Msg("fMostOverlapDist: %f\n", fMostOverlapDist );
					if (fMostOverlapDist < 14.0f)
					{
						int numOpacity = (int)(32.0f + fMostOverlapDist * 9.6f);
						// Msg("numOpacity: %d\n", numOpacity );

						vgui::surface()->DrawSetColor(clrDrawIcon.r(), clrDrawIcon.g(), clrDrawIcon.b(), numOpacity);
					}
					else
					{
						vgui::surface()->DrawSetColor(clrDrawIcon);
					}

					//Msg("Drawing at %f %f\n", x, y );
					vgui::surface()->DrawTexturedRect(xPos, yPos - 7, xPos + icon_wide, yPos + icon_tall - 7);
				}
			}
			else
			{
				pPlayer->m_LazLocal.m_flTapePos[i] = -1.0f;
			}
		}
	}

	if (bHasEP2JalopyTarget && !pPlayer->GetVehicle())
	{
		Vector vecLocation = pPlayer->m_HL2Local.m_vecLocatorOrigin;

		Vector vecToLocation = vecLocation - pPlayer->GetAbsOrigin();
		QAngle locationAngles;

		VectorAngles(vecToLocation, locationAngles);
		float yawDiff = UTIL_AngleDiff(flYawPlayerForward, locationAngles.y);
		bool bObjectInFOV = (yawDiff > -fov && yawDiff < fov);

		// Draw the icons!
		int icon_wide, icon_tall;
		int xPos, yPos;
		surface()->DrawSetColor(GetFgColor());


		if (bObjectInFOV)
		{
			// The object's location maps to a valid position along the tape, so draw an icon.
			float tapePosition = LocatorXPositionForYawDiff(yawDiff);

			// derive a scale for the locator icon
			yawDiff = fabs(yawDiff);
			float scale = 1.0f;
			scale = RemapValClamped(yawDiff, (fov / 4), fov, 1.0f, 0.25f);

			vgui::surface()->DrawSetTexture(m_textureID_IconJalopy);
			vgui::surface()->DrawGetTextureSize(m_textureID_IconJalopy, icon_wide, icon_tall);

			float flIconWide = ((float)element_tall * 1.25f);
			float flIconTall = ((float)element_tall * 1.25f);

			// Scale the icon as desired...

			// Put back into ints
			icon_wide = (int)flIconWide;
			icon_tall = (int)flIconTall;

			icon_wide *= scale;

			//Msg("yawDiff:%f  xPos:%d  scale:%f\n", yawDiff, xPos, scale );

			// Center the icon around its position.
			xPos = (int)tapePosition;
			xPos -= (icon_wide >> 1);
			yPos = (element_tall >> 1) - (icon_tall >> 1);

			//Msg("Drawing at %f %f\n", x, y );
			vgui::surface()->DrawTexturedRect(xPos, yPos, xPos + icon_wide, yPos + icon_tall);
		}
	}

#endif // HL2_EPISODIC
}


