//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Manhack vehicle, see weapon_manhack
//			CONTROL. WE HAVE IT.
//
//=============================================================================//

#include "cbase.h"
#include "movevars_shared.h"			
#include "ammodef.h"
#include <vgui_controls/Controls.h>
#include <Color.h>

//hud:
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>
#include "view_scene.h"

//#include "Human_Error\hlss_target_hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void HLSS_DrawTargetHud(Vector vecOrigin, C_BasePlayer *pPlayer, C_BaseEntity *pTarget)
{
	MDLCACHE_CRITICAL_SECTION();

	CHudTexture *pIconUpper = gHUD.GetIcon( "manhack_upper"  );
	CHudTexture *pIconLower = gHUD.GetIcon( "manhack_lower" );

	if (!pIconUpper || !pIconLower)
		return;

	if( !pPlayer || !pTarget)
		return;

	pIconUpper->EffectiveHeight(0.25f);
	pIconUpper->EffectiveWidth(0.25f);
	pIconLower->EffectiveHeight(0.25f);
	pIconLower->EffectiveWidth(0.25f);

	float flUpperX = pIconUpper->Width();
	float flUpperY = pIconUpper->Height();
	float flLowerX = pIconLower->Width();
	float flLowerY = pIconLower->Height();

	float x = ScreenWidth()/2;
	float y = ScreenHeight()/2;

	Vector vecStart = pTarget->WorldSpaceCenter();

	//Vector vecSize = m_hTarget->World //WorldAlignSize();
	Vector vecTest, screen;

	pTarget->GetRenderBounds(vecTest, screen);
 
	float height = screen.z - vecTest.z; //vecSize.z;
		
	Vector vecPlayerRight;
	pPlayer->GetVectors(NULL, &vecPlayerRight, NULL);


	//TERO: lets fix the width

	Vector vecEnemyForward, vecEnemy;

	pTarget->GetVectors(&vecEnemyForward, NULL, NULL);
	vecEnemy = (vecOrigin - vecStart);
	VectorNormalize(vecEnemy);

	float flDot = abs(DotProduct(vecEnemy, vecEnemyForward));

	float width = ((screen.y - vecTest.y) * flDot) + ((screen.x - vecTest.x) * (1.0f - flDot));
	//float width = (vecSize.y * flDot) + (vecSize.x * (1.0f - flDot));*/

	vecTest = vecStart + Vector(0, 0, height/2) - (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x1 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y1 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart + Vector(0, 0, height/2) + (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x2 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y2 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart - Vector(0, 0, height/2) - (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x3 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y3 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart - Vector(0, 0, height/2) + (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x4 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y4 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vgui::surface()->DrawSetColor(255, 0, 0, 255);

	vgui::surface()->DrawLine( x1 + flUpperX, y1, x2, y2);
	vgui::surface()->DrawLine( x1, y1 + flUpperY, x3, y3);
	vgui::surface()->DrawLine( x2, y2, x4, y4 - flLowerY);
	vgui::surface()->DrawLine( x3, y3, x4 - flLowerX, y4);

	Color color(255,255,255,255);

	pIconUpper->DrawSelf( x1, y1, color);
	pIconLower->DrawSelf( x4 - flLowerX, y4 - flLowerY, color);
}

void HLSS_DrawTargetHud(Vector vecOrigin, C_BasePlayer* pPlayer, C_BaseEntity* pTarget, int iEnemyType)
{
	//MDLCACHE_CRITICAL_SECTION();

	CHudTexture* pIconUpper = gHUD.GetIcon("manhack_upper");
	CHudTexture* pIconLower = gHUD.GetIcon("manhack_lower");

	if (!pIconUpper || !pIconLower)
		return;

	if (!pPlayer || !pTarget)
		return;

	float flBaseScale = 0.25f;

	pIconUpper->EffectiveHeight(flBaseScale);
	pIconUpper->EffectiveWidth(flBaseScale);
	pIconLower->EffectiveHeight(flBaseScale);
	pIconLower->EffectiveWidth(flBaseScale);

	float flUpperX = pIconUpper->Width();
	float flUpperY = pIconUpper->Height();
	float flLowerX = pIconLower->Width();
	float flLowerY = pIconLower->Height();

	float x = ScreenWidth() / 2;
	float y = ScreenHeight() / 2;

	Vector vecStart = pTarget->GetAbsOrigin(); //WorldSpaceCenter();

	//Vector vecSize = m_hTarget->World //WorldAlignSize();
	Vector vecTest, screen;

	pTarget->GetRenderBounds(vecTest, screen);

	float height = screen.z - vecTest.z; //vecSize.z;

	Vector vecPlayerRight;
	pPlayer->GetVectors(NULL, &vecPlayerRight, NULL);

	vecPlayerRight.z = 0;

	//TERO: lets fix the width

	Vector vecEnemyForward, vecEnemy;

	pTarget->GetVectors(&vecEnemyForward, NULL, NULL);
	vecEnemy = (vecOrigin - vecStart);
	VectorNormalize(vecEnemy);

	float flDot = fabsf(DotProduct(vecEnemy, vecEnemyForward));

	float width = ((screen.y - vecTest.y) * flDot) + ((screen.x - vecTest.x) * (1.0f - flDot)) + 2.0f; //TERO: add 2.0f to make it slighlty large than the NPC
	//float width = (vecSize.y * flDot) + (vecSize.x * (1.0f - flDot));*/

	vecTest = vecStart + Vector(0, 0, height + 1.0f) - (vecPlayerRight * width * 0.5f);
	ScreenTransform(vecTest, screen);
	float x1 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y1 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	/*vecTest = vecStart + Vector(0, 0, height/2) + (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x2 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y2 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	vecTest = vecStart - Vector(0, 0, height/2) - (vecPlayerRight * width * 0.5f);
	ScreenTransform( vecTest, screen );
	float x3 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y3 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);*/

	vecTest = vecStart - Vector(0, 0, 1) + (vecPlayerRight * width * 0.5f); //Vector(0,0,1) to make it slighlty larger than the NPC
	ScreenTransform(vecTest, screen);
	float x4 = x + (0.5 * screen[0] * ScreenWidth() + 0.5);
	float y4 = y - (0.5 * screen[1] * ScreenHeight() + 0.5);

	float flScaleUpper = min((x4 - x1) / flUpperX, (y4 - y1) / flUpperY);
	float flScaleLower = min((x4 - x1) / flLowerX, (y4 - y1) / flLowerY);

	//TERO: make sure the icons are not clipping
	/*if ((x2 - x1) >= flUpperX &&
		(y3 - y1) >= flUpperY &&
		(x4 - x3) >= flLowerX &&
		(y4 - y2) >= flLowerY)*/

		//DevMsg("flScaleUpper %f, flScaleLower %f\n", flScaleUpper, flScaleLower);

	if (flScaleUpper >= 0.16f && flScaleLower >= 0.16f)
	{
		if (flScaleUpper > 1.0f)
			flScaleUpper = 1.0f;

		if (flScaleLower > 1.0f)
			flScaleLower = 1.0f;

		Color color(0, 0, 0, 64);

		vgui::surface()->DrawSetColor(color);
		vgui::surface()->DrawFilledRect(x1, y1, x4, y4);

		/*int px[4] = { x1, x4, x4, x1 };
		int py[4] = { y1, y1, y4, y4 };

		vgui::surface()->DrawPolyLine( px, py, 4);*/

		Color color2(255, 255, 255, 196);

		pIconUpper->DrawSelf(x1, y1, (flUpperX * flScaleUpper), flUpperY * flScaleUpper, color2);
		pIconLower->DrawSelf(x4 - (flLowerX * flScaleLower), y4 - (flLowerY * flScaleLower), (flLowerX * flScaleLower), (flLowerY * flScaleLower), color2);

		vgui::HScheme scheme = vgui::scheme()->GetScheme("ClientScheme");
		vgui::HFont m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont(MAKE_STRING("Default"));

		wchar_t* unicode = L" "; //[14];

		int length = 8;

		switch (iEnemyType)
		{
		case 1:
			unicode = L"ANTI-CITIZEN.";
			length = 13;
			//swprintf(unicode, L"ANTI-CITIZE");	
			break;
		case 2:
			unicode = L"XENOFORM.";
			length = 10;
			break;
		case 3:
			unicode = L"INFECTION.";
			length = 10;
			break;
		case 4:
			unicode = L"HACKED.";
			length = 7;
			break;
		default:
			length = 0;
			break;
		}

		/*int length = 14;
		for (int i=0; i<14; i++)
		{
			if (unicode[i] == L'')
			{
				length = i;
				break;
			}
		}*/

		//TERO: 8 is the length of "HOSTILE." 
		length = max(length, 8) + 1;

		float ypos = vgui::surface()->GetFontTall(m_hFont) * 1.1f;
		float xpos = (vgui::surface()->GetCharacterWidth(m_hFont, '0') * length) + (flLowerX * flScaleLower);

		if ((x4 - x1) >= xpos && (y4 - y1) >= ((2.0f * ypos) + (flUpperY * flScaleUpper)))
		{
			vgui::surface()->DrawSetTextFont(m_hFont);
			vgui::surface()->DrawSetTextColor(0, 220, 255, 196);

			x1 += vgui::surface()->GetCharacterWidth(m_hFont, '0');

			if (unicode[0] != L'\0')
			{
				vgui::surface()->DrawSetTextPos(x1, y4 - (ypos * 2.0f));
				vgui::surface()->DrawUnicodeString(unicode);
			}

			vgui::surface()->DrawSetTextPos(x1, y4 - ypos);
			vgui::surface()->DrawUnicodeString(L"HOSTILE.");
		}
	}
}