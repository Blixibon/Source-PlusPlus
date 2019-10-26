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

	CFlashlightEffectBase(int nEntIndex = 0);
	virtual ~CFlashlightEffectBase();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
	void TurnOn();
	void TurnOff();
	bool IsOn( void ) { return m_bIsOn;	}

	ClientShadowHandle_t GetFlashlightHandle( void ) { return m_FlashlightHandle; }
	void SetFlashlightHandle( ClientShadowHandle_t Handle ) { m_FlashlightHandle = Handle;	}
	
protected:

	virtual void UpdateLightProjection( ClientFlashlightState_t &state );

	virtual void LightOff();
	void LightOffNew();

	void UpdateLightNew(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp);

	bool m_bIsOn;
	int m_nEntIndex;
	ClientShadowHandle_t m_FlashlightHandle;

	float m_flDistMod;

	// Texture for flashlight
	CTextureReference m_FlashlightTexture;
};

class CHeadlightEffect : public CFlashlightEffectBase
{
public:
	
	CHeadlightEffect();
	~CHeadlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
};

class CSpotlightEffect : public CFlashlightEffectBase
{
public:

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
};

//class CMuzzleFlashEffect : public CFlashlightEffectBase
//{
//public:
//
//	CMuzzleFlashEffect();
//	~CMuzzleFlashEffect();
//
//	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp);
//};

#endif // FLASHLIGHTEFFECT_H
