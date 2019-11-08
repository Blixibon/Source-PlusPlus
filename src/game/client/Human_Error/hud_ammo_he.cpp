//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "human_error/hud_icondisplay.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"
#include "ammodef.h"
#include "hud_manhack.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ihudlcd.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int CHudAR2AltFire::m_iAmmo = 0;
bool CHudAR2AltFire::m_bHide = true;

DECLARE_HUDELEMENT( CHudAmmoHE );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudAmmoHE::CHudAmmoHE( const char *pElementName ) : BaseClass(NULL, "HudAmmoHE"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );

	m_iStunstickType = -1;
	m_iAltFireType = -1;
	m_iAlyxGunType = -1;
	m_iAlyxGunBurst = 0;
}

/*CHudAmmoHE::~CHudAmmoHE()
{
	if (m_pBar)
	{
		m_pBar->DeletePanel();
	}
	if (m_pBarSecondary)
	{
		m_pBarSecondary->DeletePanel();
	}
	if (m_pBase)
	{
		m_pBase->DeletePanel();
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAmmoHE::Init( void )
{
	m_iAmmo		= -1;
	m_iAmmo2	= -1;

	/*wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_AMMO");
	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"AMMO");
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAmmoHE::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void CHudAmmoHE::Reset()
{
//	m_bSimple = true;

	BaseClass::Reset();

	m_hCurrentActiveWeapon = NULL;
	m_hCurrentVehicle = NULL;
	m_iAmmo = 0;
	m_iAmmo2 = 0;

	UpdateAmmoDisplays();

	m_iStunstickType = -1;
	m_iAltFireType = -1;
	m_iAlyxGunType = -1;
	m_iAlyxGunBurst = 0;

	//CreateImages();
}

/*void CHudAmmoHE::CreateImages()
{
	
	if (!m_pBar)
		m_pBar = new ImageFX( this, "sprites/metrocop_hud/hud_ammo", "MetrocopHud_Clip" );

	if (m_pBar)
	{
		float width = GetWide() * 0.8f;
		float tall  = width * 0.125f;
		float ypos  = digi_ypos + (surface()->GetFontTall(m_hSmallNumberFont)) + (tall/2);

		m_pBar->SetPosEx(GetWide()/2, ypos);
		m_pBar->SetImageSize(width, tall);
		m_pBar->SetCustomPoints(true);
		m_pBar->SetVisibleEx(true);

		m_pBar->SetZPos(6);
	}

	if (!m_pBase)
		m_pBase = new ImageFX( this, "sprites/metrocop_hud/hud_base", "MetrocopHud_ClipBase" );

	if (m_pBase)
	{
		m_pBase->SetCustomPoints(true);
		m_pBase->SetVisibleEx(true);

		m_pBase->SetZPos(5);	
		PaintSimpleBar(m_pBase, 100, 100, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*0.6) + (GetWide()*0.1), 0.2f);
	}

	if (!m_pBarSecondary)
		m_pBarSecondary = new ImageFX( this, "sprites/metrocop_hud/hud_ammo", "MetrocopHud_MaxCarry" );

	if (m_pBarSecondary)
	{
		float width = GetWide() * 0.8f;
		float tall  = width / 8;
		float ypos  = digi_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*2) + tall;

		m_pBarSecondary->SetPosEx(GetWide()/2, ypos);
		m_pBarSecondary->SetImageSize(width, tall);
		m_pBarSecondary->SetCustomPoints(true);
		m_pBarSecondary->SetVisibleEx(true);

		m_pBarSecondary->SetZPos(6);
	}

	if (!m_pBase2)
		m_pBase2 = new ImageFX( this, "sprites/metrocop_hud/hud_base", "MetrocopHud_MaxCarryBase" );

	if (m_pBase)
	{
		m_pBase2->SetCustomPoints(true);
		m_pBase2->SetVisibleEx(true);

		m_pBase2->SetZPos(5);	
		PaintSimpleBar(m_pBase2, 100, 100, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*1.4f) + (GetWide()*0.1f), 0.1f);
	}
}*/

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CHudAmmoHE::UpdatePlayerAmmo( C_BasePlayer *player )
{
	//TERO: if we were in a vehicle, update the label back to normal
	if (m_iStunstickType < 0)
	{
		m_iStunstickType = GetAmmoDef()->Index( "Stunstick" );
	}
	
	if (m_iAltFireType < 0)
	{
		m_iAltFireType = GetAmmoDef()->Index( "AR2AltFire" );
	}

	if (m_iAlyxGunType < 0)
	{
		m_iAlyxGunType = GetAmmoDef()->Index( "AlyxGun" );
	}

	// Clear out the vehicle entity
	m_hCurrentVehicle = NULL;

	C_BaseCombatWeapon *wpn = GetActiveWeapon();

	if (player && (!wpn || (wpn->GetSecondaryAmmoType() != m_iAltFireType)))
	{
		CHudAR2AltFire::m_iAmmo = player->GetAmmoCount(m_iAltFireType);
		CHudAR2AltFire::m_bHide = false;
	}
	else
	{
		CHudAR2AltFire::m_bHide = true;
	}

	if ( !wpn || !player || !wpn->UsesPrimaryAmmo() ||
		 (wpn->GetPrimaryAmmoType() == m_iStunstickType && player->GetAmmoCount(wpn->GetPrimaryAmmoType()) <= 0))
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	// get the ammo in our clip
	int ammo1 = wpn->Clip1();
	int ammo2;
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
	}

	bool bLabel = true;
	bool bForceAnimation = false;

	//TERO: not the best way to do it
	if (wpn->GetPrimaryAmmoType() == m_iAlyxGunType)
	{
		bLabel = false;

		if (wpn->GetSecondaryAmmoType() == -2 && m_iAlyxGunBurst != -2 )
		{
			bForceAnimation = true;
			m_iAlyxGunBurst = -2;
			SetLabel(L"AUTO");
		}
		else if (wpn->GetSecondaryAmmoType() != -2 && m_iAlyxGunBurst != -1)
		{
			bForceAnimation = true;
			m_iAlyxGunBurst = -1;
			SetLabel(L"BURST");
		}
	}
	else
	{
		m_iAlyxGunBurst = 0;
	}

	if (wpn == m_hCurrentActiveWeapon)
	{
		// same weapon, just update counts
		SetAmmo(ammo1, bForceAnimation);
		SetAmmo2(ammo2);
	}
	else
	{

		// diferent weapon, change without triggering
		SetAmmo(ammo1, bForceAnimation);
		SetAmmo2(ammo2);

		int maxValue, maxSecondaryValue;

		// update whether or not we show the total ammo display
		if (wpn->UsesClipsForAmmo1())
		{
			SetShouldDisplaySecondaryValue(true);
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClipsHE");
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClipsHE");

			maxValue = wpn->GetMaxClip1();
			maxSecondaryValue = GetAmmoDef()->MaxCarry(wpn->GetPrimaryAmmoType());
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClipsHE");
			SetShouldDisplaySecondaryValue(false);

			maxValue = GetAmmoDef()->MaxCarry(wpn->GetPrimaryAmmoType());
			maxSecondaryValue = 0;
		}

		CHudTexture* pTex = gWR.GetAmmoIconFromWeapon(wpn->GetPrimaryAmmoType());
		wchar_t icon[2];
			icon[0] = (pTex && pTex->bRenderUsingFont) ? pTex->cCharacterInFont : L'\0';
			icon[1] = L'\0';
		SetIcon( icon );

		if (bLabel) //FClassnameIs( wpn, "weapon_alyxgun" ))
		{
			wchar_t* tempString = g_pVGuiLocalize->Find(CFmtStr("#AMMO_TITLE_%s", GetAmmoDef()->m_AmmoType[wpn->GetPrimaryAmmoType()].pName));
			if (tempString)
				SetLabel(tempString);
			else
				SetLabel(GetAmmoDef()->m_AmmoType[wpn->GetPrimaryAmmoType()].pName);
		}

//		SetDrawGap(true);

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponChangedHE");
		m_hCurrentActiveWeapon = wpn;
	}
}

void CHudAmmoHE::UpdateVehicleAmmo( C_BasePlayer *player, IClientVehicle *pVehicle )
{
	m_hCurrentActiveWeapon = NULL;
	CBaseEntity *pVehicleEnt = pVehicle->GetVehicleEnt();

	CHudAR2AltFire::m_bHide = true;

	if ( !pVehicleEnt || pVehicle->GetPrimaryAmmoType() < 0 || pVehicle->GetPrimaryAmmoType() == GetAmmoDef()->Index( "Manhack" ))
	{
		SetPaintEnabled(false);
//		ShowImages(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
//	ShowImages(true);
	SetPaintBackgroundEnabled(true);

	// get the ammo in our clip
	int ammo1 = pVehicle->GetPrimaryAmmoClip();
	int ammo2;
	if (ammo1 < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		ammo1 = pVehicle->GetPrimaryAmmoCount();
		ammo2 = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		ammo2 = pVehicle->GetPrimaryAmmoCount();
	}

	if (pVehicleEnt == m_hCurrentVehicle)
	{
		// same weapon, just update counts
		SetAmmo(ammo1, false);
		SetAmmo2(ammo2);
	}
	else
	{
		// diferent weapon, change without triggering
		SetAmmo(ammo1, true);
		SetAmmo2(ammo2);

		bool IsAPC = (pVehicle->GetPrimaryAmmoType() == GetAmmoDef()->Index( "CombineCannon" )); //FClassnameIs(pVehicleEnt, "prop_vehicle_drivable_apc");

		int maxValue, maxSecondaryValue;

		// update whether or not we show the total ammo display
		if (pVehicle->PrimaryAmmoUsesClips())
		{
			SetShouldDisplaySecondaryValue(true);
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClipsHE");
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseSecondaryAmmoHE");

			//Tero: this is our way to set the ammo for the APC, I am too lazy to figure out any other way
			if (IsAPC)
			{
				maxValue = 100;
				maxSecondaryValue = 3;
			}
			else
			{
				maxValue = 100;
				maxSecondaryValue = 100;
			}
		}
		else
		{
			//g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClipsHE");
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseSecondaryAmmoNoClipHE");
			SetShouldDisplaySecondaryValue(false);

			maxValue = GetAmmoDef()->MaxCarry(pVehicle->GetPrimaryAmmoType());
			maxSecondaryValue = 0;
		}

		SetIcon( IsAPC ? L"u" : L"*");
		wchar_t* tempString = g_pVGuiLocalize->Find(CFmtStr("#AMMO_TITLE_%s", GetAmmoDef()->m_AmmoType[pVehicle->GetPrimaryAmmoType()].pName));
		if (tempString)
			SetLabel(tempString);
		else
			SetLabel(GetAmmoDef()->m_AmmoType[pVehicle->GetPrimaryAmmoType()].pName);

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponChangedHE");
		m_hCurrentVehicle = pVehicleEnt;
	}
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CHudAmmoHE::OnThink()
{
	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: updates the ammo display counts
//-----------------------------------------------------------------------------
void CHudAmmoHE::UpdateAmmoDisplays()
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	IClientVehicle *pVehicle = player ? player->GetVehicle() : NULL;

	if ( !pVehicle )
	{
		UpdatePlayerAmmo( player );
	}
	else
	{
		UpdateVehicleAmmo( player, pVehicle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates ammo display
//-----------------------------------------------------------------------------
void CHudAmmoHE::SetAmmo(int ammo, bool playAnimation)
{
	if (ammo != m_iAmmo || playAnimation)
	{
		if (ammo == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoEmptyHE");
		}
		else if (ammo < m_iAmmo)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreasedHE");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreasedHE");
		}

		m_iAmmo = ammo;
	}

	SetDisplayValue(ammo);
}

//-----------------------------------------------------------------------------
// Purpose: Updates 2nd ammo display
//-----------------------------------------------------------------------------
void CHudAmmoHE::SetAmmo2(int ammo2)
{
	if (ammo2 != m_iAmmo2)
	{
		if (m_bIsAPC)
		{
			if (ammo2 < m_iAmmo2)
			{
				// ammo has decreased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreasedHE");
			}
			else if (ammo2 > m_iAmmo2)
			{
				// ammunition has increased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreasedHE");
			}
		}
		else
		{
			if (ammo2 == 0)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2EmptyHE");
			}
			else if (ammo2 < m_iAmmo2)
			{
				// ammo has decreased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2DecreasedHE");
			}
			else
			{
				// ammunition has increased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2IncreasedHE");
			}
		}

		m_iAmmo2 = ammo2;
	}

	SetSecondaryValue(ammo2);
}

//-----------------------------------------------------------------------------
// Purpose: Displays the secondary ammunition level
//-----------------------------------------------------------------------------
class CHudSecondaryAmmoHE : public CHudIconDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudSecondaryAmmoHE, CHudIconDisplay );

	/*~CHudSecondaryAmmoHE()
	{
		if (m_pBar)
		{
			m_pBar->DeletePanel();
		}
	}*/

public:
	CHudSecondaryAmmoHE( const char *pElementName ) : BaseClass( NULL, "HudAmmoSecondaryHE" ), CHudElement( pElementName )
	{
		m_iAmmo = -1;

		SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_WEAPONSELECTION | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
	}

	void Init( void )
	{
	}

	void VidInit( void )
	{
	}

	void SetAmmo( int ammo )
	{
		if (ammo != m_iAmmo)
		{
			if (ammo == 0)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryEmptyHE");
			}
			else if (ammo < m_iAmmo)
			{
				// ammo has decreased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryDecreasedHE");
			}
			else
			{
				// ammunition has increased
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoSecondaryIncreasedHE");
			}

			m_iAmmo = ammo;
		}
		SetDisplayValue( ammo );
	}

	void Reset()
	{
		// hud reset, update ammo state
//		m_bSimple = true;

		BaseClass::Reset();

		m_iAmmo = 0;
		m_hCurrentActiveWeapon = NULL;
		SetAlpha( 0 );
		UpdateAmmoState();

		/*if (!m_pBar)
		m_pBar = new ImageFX( this, "sprites/metrocop_hud/hud_ammo", "MetrocopHud_Secondary" );

		if (m_pBar)
		{
			float width = GetWide() * 0.8f;
			float tall  = width * 0.125f;
			float ypos  = digi_ypos + surface()->GetFontTall(m_hSmallNumberFont) + (tall/2);

			m_pBar->SetPosEx(GetWide()/2, ypos);
			m_pBar->SetImageSize(width, tall);
			m_pBar->SetCustomPoints(true);
			m_pBar->SetVisibleEx(true);

			m_pBar->SetZPos(6);
		}

		
		if (!m_pBase)
			m_pBase = new ImageFX( this, "sprites/metrocop_hud/hud_base", "MetrocopHud_SecondaryBase" );

		if (m_pBase)
		{
			m_pBase->SetCustomPoints(true);
			m_pBase->SetVisibleEx(true);

			m_pBase->SetZPos(5);	
			PaintSimpleBar(m_pBase, 100, 100, icon_ypos + (surface()->GetFontTall(m_hSmallNumberFont)*0.6) + (GetWide()*0.1), 0.2f);
		}*/
	}

protected:
	virtual void OnThink()
	{
		// set whether or not the panel draws based on if we have a weapon that supports secondary ammo
		C_BaseCombatWeapon *wpn = GetActiveWeapon();
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		IClientVehicle *pVehicle = player ? player->GetVehicle() : NULL;
		if (!wpn || !player || pVehicle)
		{
			m_hCurrentActiveWeapon = NULL;
			SetPaintEnabled(false);
//			ShowImages(false);
			SetPaintBackgroundEnabled(false);
			return;
		}
		else
		{
			SetPaintEnabled(true);
//			ShowImages(true);
			SetPaintBackgroundEnabled(true);
		}

		UpdateAmmoState();
	}

	void UpdateAmmoState()
	{
		C_BaseCombatWeapon *wpn = GetActiveWeapon();
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();

		if ( m_hCurrentActiveWeapon != wpn )
		{
			if (player && wpn)
			{
				if (wpn->UsesSecondaryAmmo())
				{
					//TERO: a bit hacky
					SetAmmo(GetAmmoDef()->MaxCarry(wpn->GetSecondaryAmmoType()));

					// we've changed to a weapon that uses secondary ammo. NoClip added by - Tero
					if (wpn->UsesClipsForAmmo1())
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesSecondaryAmmoHE");
					else
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesSecondaryAmmoNoClipHE");

					CHudTexture* pTex = gWR.GetAmmoIconFromWeapon(wpn->GetSecondaryAmmoType());
					wchar_t icon[2];
					icon[0] = (pTex && pTex->bRenderUsingFont) ? pTex->cCharacterInFont : L'\0';
					icon[1] = L'\0';
					SetIcon(icon);
					wchar_t* tempString = g_pVGuiLocalize->Find(CFmtStr("#AMMO_TITLE_%s", GetAmmoDef()->m_AmmoType[wpn->GetSecondaryAmmoType()].pName));
					if (tempString)
						SetLabel(tempString);
					else
						SetLabel(GetAmmoDef()->m_AmmoType[wpn->GetSecondaryAmmoType()].pName);
				}
				else
				{
					// we've changed away from a weapon that uses secondary ammo. NoClip added by - Tero
					if (wpn->UsesClipsForAmmo1())
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseSecondaryAmmoHE");
					else
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseSecondaryAmmoNoClipHE");
				}
			}
			m_hCurrentActiveWeapon = wpn;
		}
		else
		{
			if (player && wpn && wpn->UsesSecondaryAmmo())
			{
				SetAmmo( player->GetAmmoCount(wpn->GetSecondaryAmmoType()) );
			}
		}
	}
	
private:
	CHandle< C_BaseCombatWeapon > m_hCurrentActiveWeapon;
	int		m_iAmmo;
};

DECLARE_HUDELEMENT( CHudSecondaryAmmoHE );


DECLARE_HUDELEMENT( CHudManhackHealth );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudManhackHealth::CHudManhackHealth( const char *pElementName ) : BaseClass(NULL, "HudManhackHealth"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
//	m_iImageID = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudManhackHealth::Init( void )
{
	m_iHealth		= -1;
	
	wchar_t *tempString = g_pVGuiLocalize->Find("#HLSS_Hud_ManhackHealth");
	if (tempString)
	{
		SetLabelText(tempString);
	}
	else
	{
		SetLabelText(L"MANHACK");
	}

	//SetSecondaryLabelText(L"RADIO SIGNAL");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudManhackHealth::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void CHudManhackHealth::Reset()
{
	BaseClass::Reset();

	m_iHealth = 0;
	m_iDistance = 100;


	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	IClientVehicle *pVehicle = player ? player->GetVehicle() : NULL;

	UpdateManhackHealth( player, pVehicle );
	SetShouldDisplaySecondaryValue(true);
}


void CHudManhackHealth::UpdateManhackHealth( C_BasePlayer *player, IClientVehicle *pVehicle )
{
	CBaseEntity *pVehicleEnt=NULL;
	if (pVehicle)
		pVehicleEnt = pVehicle->GetVehicleEnt();

	if ( !pVehicleEnt || (pVehicle && pVehicle->GetPrimaryAmmoType() != GetAmmoDef()->Index( "Manhack" ) ) )
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	SetHealthAndDistance(pVehicle->GetPrimaryAmmoCount(), pVehicle->GetPrimaryAmmoClip(), true);
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CHudManhackHealth::OnThink()
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	IClientVehicle *pVehicle = player ? player->GetVehicle() : NULL;


	UpdateManhackHealth( player, pVehicle );
}


//-----------------------------------------------------------------------------
// Purpose: Updates ammo display
//-----------------------------------------------------------------------------
void CHudManhackHealth::SetHealthAndDistance(int health, int distance, bool playAnimation)
{
	if (health != m_iHealth || distance != m_iDistance)
	{
		if (health < m_iHealth || distance >= 90)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ManhackHealthDecreased");
		}
		else if ( distance < 90 && m_iDistance >= 90 )
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ManhackRadioIncreased");
		}

		m_iHealth = health;
		m_iDistance = distance;
	}


	//distance = m_iDistance + random->RandomInt(-2,2);
	distance = clamp(distance, 0, 100);
	distance = 100 - distance;

	SetDisplayValue(health);
	SetSecondaryValue(distance);
}

/*void CHudManhackHealth::Paint()
{
	

	if (m_iImageID == -1)
	{
		m_iImageID = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_iImageID, "vgui/screens/manhack_screen", true, false );
	}

	int wide, tall;
	GetSize(wide, tall);
	vgui::surface()->DrawSetTexture(m_iImageID);
	vgui::surface()->DrawTexturedRect(0, 0, wide, tall);

	BaseClass::Paint();
}*/



DECLARE_HUDELEMENT( CHudAR2AltFire );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudAR2AltFire::CHudAR2AltFire( const char *pElementName ) : BaseClass(NULL, "HudAR2AltFire"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAR2AltFire::Init( void )
{
	m_iAmmo = 0;
	m_iOldAmmo = -1;

	wchar_t *tempString = g_pVGuiLocalize->Find("#HLSS_AR2_AltFire");
	if (tempString)
	{
		SetLabel(tempString);
	}
	else
	{
		SetLabel(L"AR2 AltFire");
	}

	SetIcon(L"z" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAR2AltFire::VidInit( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void CHudAR2AltFire::Reset()
{
	BaseClass::Reset();

	m_iAmmo = 0;	
	m_iOldAmmo = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudAR2AltFire::OnThink( void )
{
	if (m_iAmmo == m_iOldAmmo && m_bHide == m_bHidden)
		return;

	if ( !m_iAmmo || m_bHide)
	{
	 	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AR2AmmoZero");
		//SetPaintBackgroundEnabled( false );
	}
	else if ( m_iAmmo > m_iOldAmmo )
	{
		SetPaintBackgroundEnabled( true );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AR2AmmoIncreased");
	}
	else
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AR2AmmoDecreased");
	}

	SetDisplayValue(m_iAmmo);

	m_iOldAmmo = m_iAmmo;
	m_bHidden = m_bHide;
}