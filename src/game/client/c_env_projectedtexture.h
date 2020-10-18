//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_ENVPROJECTED_TEXTURE_H
#define C_ENVPROJECTED_TEXTURE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "basetypes.h"
#include "iclientshadowmgr.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EnvProjectedTexture : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvProjectedTexture, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	void SetLightColor(byte r, byte g, byte b, byte a);
	virtual void OnDataChanged( DataUpdateType_t updateType );
	void	ShutDownLightHandle( void );

	virtual void Simulate();

	void	UpdateLight( bool bForceUpdate );

	virtual bool ShouldReceiveProjectedTextures( int flags ) { return false; }

	C_EnvProjectedTexture();
	~C_EnvProjectedTexture();

private:

	inline bool IsBBoxVisible(void);
	bool IsBBoxVisible(Vector vecExtentsMin,
		Vector vecExtentsMax);

	CTextureReference m_SpotlightTexture;

	ClientShadowHandle_t m_LightHandle;

	EHANDLE	m_hTargetEntity;

	bool	m_bState;
	bool		m_bAlwaysUpdate;
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	float		m_flBrightnessScale;
	color32		m_LightColor;
	Vector		m_CurrentLinearFloatLightColor;
	float		m_flCurrentLinearFloatLightAlpha;
	float		m_flColorTransitionTime;
	float	m_flAmbient;
	float	m_flNearZ;
	float	m_flFarZ;
	char	m_SpotlightTextureName[MAX_PATH];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
	int			m_iStyle;

	ClientFlashlightState_t	m_FlashlightState;
	bool	m_bLastUberState;

	bool m_bEnableVolumetrics;
	bool m_bEnableVolumetricsLOD;
	float m_flVolumetricsFadeDistance;
	int m_iVolumetricsQuality;
	float m_flVolumetricsMultiplier;
	float m_flVolumetricsQualityBias;

	Vector	m_vecExtentsMin;
	Vector	m_vecExtentsMax;

	static float m_flVisibleBBoxMinHeight;
};

bool C_EnvProjectedTexture::IsBBoxVisible(void)
{
	return IsBBoxVisible(GetAbsOrigin() + m_vecExtentsMin, GetAbsOrigin() + m_vecExtentsMax);
}
#endif // C_ENV_PROJECTED_TEXTURE_H
