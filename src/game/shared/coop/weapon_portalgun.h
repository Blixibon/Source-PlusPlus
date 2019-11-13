//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_PORTALGUN_SHARED_H
#define WEAPON_PORTALGUN_SHARED_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_coop_basehlcombatweapon.h"
#include "prop_portal_shared.h"
#ifdef CLIENT_DLL
#include "fx_interpvalue.h"
#include "beamdraw.h"
#include "iviewrender_beams.h"
#endif // CLIENT_DLL


#ifdef CLIENT_DLL
#define CWeaponPortalgun C_WeaponPortalgun
#endif //#ifdef CLIENT_DLL

#define PORTALGUN_BEAM_SPRITE "sprites/grav_beam.vmt"
#define PORTALGUN_BEAM_SPRITE_NOZ "sprites/grav_beam_noz.vmt"
#define PORTALGUN_GLOW_SPRITE "sprites/glow04_noz"
#define PORTALGUN_ENDCAP_SPRITE "sprites/grav_flare"
#define PORTALGUN_GRAV_ACTIVE_GLOW "sprites/grav_light"
#define PORTALGUN_PORTAL1_FIRED_LAST_GLOW "sprites/bluelight"
#define PORTALGUN_PORTAL2_FIRED_LAST_GLOW "sprites/orangelight"
#define PORTALGUN_PORTAL_MUZZLE_GLOW_SPRITE "sprites/portalgun_effects"
#define PORTALGUN_PORTAL_TUBE_BEAM_SPRITE "sprites/portalgun_effects"


#ifdef CLIENT_DLL
class CPortalgunEffect
{
public:

	CPortalgunEffect(void)
		: m_vecColor(255, 255, 255),
		m_bVisibleViewModel(true),
		m_bVisible3rdPerson(true),
		m_nAttachment(-1)
	{}

	void SetAttachment(int attachment) { m_nAttachment = attachment; }
	int	GetAttachment(void) const { return m_nAttachment; }

	void SetVisible(bool visible = true) { m_bVisibleViewModel = visible; m_bVisible3rdPerson = visible; }

	void SetVisibleViewModel(bool visible = true) { m_bVisibleViewModel = visible; }
	int IsVisibleViewModel(void) const { return m_bVisibleViewModel; }

	void SetVisible3rdPerson(bool visible = true) { m_bVisible3rdPerson = visible; }
	int IsVisible3rdPerson(void) const { return m_bVisible3rdPerson; }

	void SetColor(const Vector& color) { m_vecColor = color; }
	const Vector& GetColor(void) const { return m_vecColor; }

	bool SetMaterial(const char* materialName)
	{
		m_hMaterial.Init(materialName, TEXTURE_GROUP_CLIENT_EFFECTS);
		return (m_hMaterial != NULL);
	}

	CMaterialReference& GetMaterial(void) { return m_hMaterial; }

	CInterpolatedValue& GetAlpha(void) { return m_Alpha; }
	CInterpolatedValue& GetScale(void) { return m_Scale; }

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_PORTALGUN; }

private:
	CInterpolatedValue	m_Alpha;
	CInterpolatedValue	m_Scale;

	Vector				m_vecColor;
	bool				m_bVisibleViewModel;
	bool				m_bVisible3rdPerson;
	int					m_nAttachment;
	CMaterialReference	m_hMaterial;
};


class CPortalgunEffectBeam
{
public:
	CPortalgunEffectBeam(void);;
	~CPortalgunEffectBeam(void);

	void Release(void);

	void Init(int startAttachment, int endAttachment, CBaseEntity* pEntity, bool firstPerson);

	void SetVisibleViewModel(bool visible = true);
	int IsVisibleViewModel(void) const;

	void SetVisible3rdPerson(bool visible = true);
	int SetVisible3rdPerson(void) const;

	void SetBrightness(float fBrightness);

	void DrawBeam(void);

private:
	Beam_t* m_pBeam;

	float	m_fBrightness;
};
#endif

extern ConVar sk_auto_reload_time;

class CWeaponPortalgun : public CWeaponCoopBaseHLCombat
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS(CWeaponPortalgun, CWeaponCoopBaseHLCombat);

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

private:
	CNetworkVar(bool, m_bCanFirePortal1);	// Is able to use primary fire
	CNetworkVar(bool, m_bCanFirePortal2);	// Is able to use secondary fire
	CNetworkVar(int, m_iLastFiredPortal);	// Which portal was placed last
	CNetworkVar(bool, m_bOpenProngs);		// Which portal was placed last
	CNetworkVar(float, m_fCanPlacePortal1OnThisSurface);	// Tells the gun if it can place on the surface it's pointing at
	CNetworkVar(float, m_fCanPlacePortal2OnThisSurface);	// Tells the gun if it can place on the surface it's pointing at

public:
	unsigned char m_iPortalLinkageGroupID; //which portal linkage group this gun is tied to, usually set by mapper, or inherited from owning player's index

	// HACK HACK! Used to make the gun visually change when going through a cleanser!
	CNetworkVar(float, m_fEffectsMaxSize1);
	CNetworkVar(float, m_fEffectsMaxSize2);

public:
	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_10DEGREES;
		return cone;
	}

	virtual void Precache(void);

#ifndef CLIENT_DLL
	virtual void CreateSounds(void);
	virtual void StopLoopingSounds(void);
#endif // !CLIENT_DLL


	virtual void OnRestore(void);
	virtual void UpdateOnRemove(void);
	void Spawn(void);
	void DoEffectBlast(bool bPortal2, int iPlacedBy, const Vector& ptStart, const Vector& ptFinalPos, const QAngle& qStartAngles, float fDelay);

#ifndef CLIENT_DLL
	virtual void OnPickedUp(CBaseCombatCharacter* pNewOwner);
	virtual void Activate();
#endif // !CLIENT_DLL


	virtual bool ShouldDrawCrosshair(void);
	float GetPortal1Placablity(void) { return m_fCanPlacePortal1OnThisSurface; }
	float GetPortal2Placablity(void) { return m_fCanPlacePortal2OnThisSurface; }
	void SetLastFiredPortal(int iLastFiredPortal) { m_iLastFiredPortal = iLastFiredPortal; }
	int GetLastFiredPortal(void) { return m_iLastFiredPortal; }
	bool IsHoldingObject(void) { return m_bOpenProngs; }

	bool Reload(void);
	void FillClip(void);
	void CheckHolsterReload(void);
	void ItemHolsterFrame(void);
	void ItemPreFrame();
	bool Holster(CBaseCombatWeapon* pSwitchingTo = NULL);
	bool Deploy(void);

	void SetCanFirePortal1(bool bCanFire = true);
	void SetCanFirePortal2(bool bCanFire = true);
	float CanFirePortal1(void) { return m_bCanFirePortal1; }
	float CanFirePortal2(void) { return m_bCanFirePortal2; }

	void PrimaryAttack(void);
	void SecondaryAttack(void);

	void DelayAttack(float fDelay);

	float TraceFirePortal(bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest = false);
	float FirePortal(bool bPortal2, Vector* pVector = 0, bool bTest = false);
	void FirePortal1();
	void FirePortal2();

#ifndef CLIENT_DLL
	virtual bool PreThink(void);
	virtual void Think(void);

	void OpenProngs(bool bOpenProngs);

	void InputChargePortal1(inputdata_t& inputdata);
	void InputChargePortal2(inputdata_t& inputdata);
	void InputFirePortal1(inputdata_t& inputdata);
	void InputFirePortal2(inputdata_t& inputdata);
	void FirePortalDirection1(inputdata_t& inputdata);
	void FirePortalDirection2(inputdata_t& inputdata);

	CSoundPatch* m_pMiniGravHoldSound;

	// Outputs for portalgun
	COutputEvent m_OnFiredPortal1;		// Fires when the gun's first (blue) portal is fired
	COutputEvent m_OnFiredPortal2;		// Fires when the gun's second (red) portal is fired
#endif

	void DryFire(void);
	virtual float GetFireRate(void) { return 0.7; };
	void WeaponIdle(void);

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_PORTALGUN; }

protected:

	void	StartEffects(void);	// Initialize all sprites and beams
	void	StopEffects(bool stopSound = true);	// Hide all effects temporarily
	void	DestroyEffects(void);	// Destroy all sprites and beams

	// Portalgun effects
	void	DoEffect(int effectType, Vector* pos = NULL);

	void	DoEffectClosed(void);
	void	DoEffectReady(void);
	void	DoEffectHolding(void);
	void	DoEffectNone(void);

#ifdef CLIENT_DLL
	enum EffectType_t
	{
		PORTALGUN_GRAVLIGHT = 0,
		PORTALGUN_GRAVLIGHT_WORLD,
		PORTALGUN_PORTAL1LIGHT,
		PORTALGUN_PORTAL1LIGHT_WORLD,
		PORTALGUN_PORTAL2LIGHT,
		PORTALGUN_PORTAL2LIGHT_WORLD,

		PORTALGUN_GLOW1,	// Must be in order!
		PORTALGUN_GLOW2,
		PORTALGUN_GLOW3,
		PORTALGUN_GLOW4,
		PORTALGUN_GLOW5,
		PORTALGUN_GLOW6,

		PORTALGUN_GLOW1_WORLD,
		PORTALGUN_GLOW2_WORLD,
		PORTALGUN_GLOW3_WORLD,
		PORTALGUN_GLOW4_WORLD,
		PORTALGUN_GLOW5_WORLD,
		PORTALGUN_GLOW6_WORLD,

		PORTALGUN_ENDCAP1,	// Must be in order!
		PORTALGUN_ENDCAP2,
		PORTALGUN_ENDCAP3,

		PORTALGUN_ENDCAP1_WORLD,
		PORTALGUN_ENDCAP2_WORLD,
		PORTALGUN_ENDCAP3_WORLD,

		PORTALGUN_MUZZLE_GLOW,

		PORTALGUN_MUZZLE_GLOW_WORLD,

		PORTALGUN_TUBE_BEAM1,
		PORTALGUN_TUBE_BEAM2,
		PORTALGUN_TUBE_BEAM3,
		PORTALGUN_TUBE_BEAM4,
		PORTALGUN_TUBE_BEAM5,

		PORTALGUN_TUBE_BEAM1_WORLD,
		PORTALGUN_TUBE_BEAM2_WORLD,
		PORTALGUN_TUBE_BEAM3_WORLD,
		PORTALGUN_TUBE_BEAM4_WORLD,
		PORTALGUN_TUBE_BEAM5_WORLD,

		NUM_PORTALGUN_PARAMETERS	// Must be last!
	};

#define	NUM_GLOW_SPRITES ((C_WeaponPortalgun::PORTALGUN_GLOW6-C_WeaponPortalgun::PORTALGUN_GLOW1)+1)
#define	NUM_GLOW_SPRITES_WORLD ((C_WeaponPortalgun::PORTALGUN_GLOW6_WORLD-C_WeaponPortalgun::PORTALGUN_GLOW1_WORLD)+1)
#define NUM_ENDCAP_SPRITES ((C_WeaponPortalgun::PORTALGUN_ENDCAP3-C_WeaponPortalgun::PORTALGUN_ENDCAP1)+1)
#define NUM_ENDCAP_SPRITES_WORLD ((C_WeaponPortalgun::PORTALGUN_ENDCAP3_WORLD-C_WeaponPortalgun::PORTALGUN_ENDCAP1_WORLD)+1)
#define NUM_TUBE_BEAM_SPRITES ((C_WeaponPortalgun::PORTALGUN_TUBE_BEAM5-C_WeaponPortalgun::PORTALGUN_TUBE_BEAM1)+1)
#define NUM_TUBE_BEAM_SPRITES_WORLD ((C_WeaponPortalgun::PORTALGUN_TUBE_BEAM5_WORLD-C_WeaponPortalgun::PORTALGUN_TUBE_BEAM1_WORLD)+1)

#define	NUM_PORTALGUN_BEAMS	6

	void			DrawEffects(bool b3rdPerson);
	Vector			GetEffectColor(int iPalletIndex);
	void			GetEffectParameters(EffectType_t effectID, color32& color, float& scale, IMaterial** pMaterial, Vector& vecAttachment, bool b3rdPerson);
	void			DrawEffectSprite(EffectType_t effectID, bool b3rdPerson);
	inline bool		IsEffectVisible(EffectType_t effectID, bool b3rdPerson);
	void			UpdateElementPosition(void);

	CPortalgunEffect		m_Parameters[NUM_PORTALGUN_PARAMETERS];	// Interpolated parameters for the effects
	CPortalgunEffectBeam	m_Beams[NUM_PORTALGUN_BEAMS];				// Beams

	int				m_nOldEffectState;	// Used for parity checks
	bool			m_bOldCanFirePortal1;
	bool			m_bOldCanFirePortal2;

	bool			m_bPulseUp;
	float			m_fPulse;
#endif

	CNetworkVar(int, m_EffectState);		// Current state of the effects on the gun

public:
#ifdef CLIENT_DLL
	virtual int		DrawModel(int flags);
	virtual void	ViewModelDrawn(C_BaseViewModel* pBaseViewModel);
	virtual void	OnPreDataChanged(DataUpdateType_t updateType);
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	ClientThink(void);

	void DoEffectIdle(void);
#endif

	CWeaponPortalgun(void);

private:
	CWeaponPortalgun(const CWeaponPortalgun&);

};

#endif // WEAPON_PORTALGUN_SHARED_H
