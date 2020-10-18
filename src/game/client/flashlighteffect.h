//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FLASHLIGHTEFFECT_H
#define FLASHLIGHTEFFECT_H
#ifdef _WIN32
#pragma once
#endif

struct dlight_t;


class CFlashlightEffectBase
{
public:

	CFlashlightEffectBase(bool bLocalPlayer = false);
	virtual ~CFlashlightEffectBase();

	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance, float flScale = 1.0f) = 0;
	virtual void TurnOn();
	virtual void TurnOff();
	bool IsOn( void ) { return m_bIsOn;	}

	ClientShadowHandle_t GetFlashlightHandle( void ) { return m_FlashlightHandle; }
	void SetFlashlightHandle( ClientShadowHandle_t Handle ) { m_FlashlightHandle = Handle;	}
	
protected:

	void InitSpotlightTexture(const char* pszLightTexture);

	virtual void UpdateLightProjection( ClientFlashlightState_t &state );

	virtual void LightOff();

	bool m_bIsOn;
	bool m_bIsLocalPlayerLight;
	ClientShadowHandle_t m_FlashlightHandle;

	float m_flDistMod;

	// Texture for flashlight
	CTextureReference m_FlashlightTexture;
};

class CFlashlightEffect : public CFlashlightEffectBase
{
public:
	CFlashlightEffect(int iEntIndex, bool bIsNVG);
	~CFlashlightEffect();

	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance, float flScale = 1.0f)
	{
		UpdateLightNew(vecPos, vecDir, vecRight, vecUp);
	}

	void UpdateLightNew(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp);
protected:
	int m_nEntIndex;
	bool m_bIsNVG;
};

class CHeadlightEffect : public CFlashlightEffectBase
{
public:
	
	CHeadlightEffect();
	~CHeadlightEffect();

	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance, float flScale = 1.0f);
};

class CSpotlightEffect : public CFlashlightEffectBase
{
public:

	CSpotlightEffect(bool bVolumetric, float flVolumeIntensity = 1.f);
	CSpotlightEffect();
	~CSpotlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance, float flScale = 1.0f);

	Vector GetPosition()
	{
		return m_vecOrigin;
	}

protected:

	virtual void LightOff();
	void LightOffOld();

	void UpdateLightNew(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, float flScale);
	void UpdateLightOld(const Vector &vecPos, const Vector &vecDir, int nDistance, float flScale);

	dlight_t*	m_pDynamicLight;
	dlight_t*	m_pSpotlightEnd;

	Vector m_vecOrigin;
	bool	m_bVolumetric;
	float	m_flVolumeIntensity;
};

void ProjectMuzzleFlashLight(ClientEntityHandle_t hEntity, int attachmentIndex, ColorRGBExp32 clrColor);
#endif // FLASHLIGHTEFFECT_H
