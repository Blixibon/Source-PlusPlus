//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"
#include "c_env_projectedtexture.h"
#include "view_scene.h"
#include "viewrender.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float C_EnvProjectedTexture::m_flVisibleBBoxMinHeight = -FLT_MAX;

ConVar mat_slopescaledepthbias_shadowmap( "mat_slopescaledepthbias_shadowmap", "4", FCVAR_CHEAT );
ConVar mat_depthbias_shadowmap( "mat_depthbias_shadowmap", "0.00001", FCVAR_CHEAT );

IMPLEMENT_CLIENTCLASS_DT( C_EnvProjectedTexture, DT_EnvProjectedTexture, CEnvProjectedTexture )
	RecvPropEHandle( RECVINFO( m_hTargetEntity ) ),
	RecvPropBool( RECVINFO( m_bState ) ),
	RecvPropBool(RECVINFO(m_bAlwaysUpdate)),
	RecvPropFloat( RECVINFO( m_flLightFOV ) ),
	RecvPropBool( RECVINFO( m_bEnableShadows ) ),
	RecvPropBool( RECVINFO( m_bLightOnlyTarget ) ),
	RecvPropBool( RECVINFO( m_bLightWorld ) ),
	RecvPropBool( RECVINFO( m_bCameraSpace ) ),
	RecvPropFloat(RECVINFO(m_flBrightnessScale)),
	RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_Int32ToColor32),
	RecvPropFloat(RECVINFO(m_flColorTransitionTime)),
	RecvPropFloat( RECVINFO( m_flAmbient ) ),
	RecvPropString( RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt( RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat( RECVINFO( m_flNearZ ) ),
	RecvPropFloat( RECVINFO( m_flFarZ ) ),
	RecvPropInt( RECVINFO( m_nShadowQuality ) ),
	RecvPropInt(RECVINFO(m_iStyle)),

	RecvPropBool(	 RECVINFO( m_bEnableVolumetrics ) ),
	RecvPropBool(	 RECVINFO( m_bEnableVolumetricsLOD ) ),
	RecvPropFloat(	 RECVINFO( m_flVolumetricsFadeDistance ) ),
	RecvPropInt(	 RECVINFO( m_iVolumetricsQuality ) ),
	RecvPropFloat(	 RECVINFO( m_flVolumetricsQualityBias ) ),
	RecvPropFloat(	 RECVINFO( m_flVolumetricsMultiplier ) ),

	RecvPropBool( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_bEnabled, m_bUberlight ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fNearEdge, m_fNearEdge ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fFarEdge, m_fFarEdge ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fCutOn, m_fCutOn ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fCutOff, m_fCutOff ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fShearx, m_fShearx ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fSheary, m_fSheary ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fWidth, m_fWidth ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fWedge, m_fWedge ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fHeight, m_fHeight ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fHedge, m_fHedge ) ),
	RecvPropFloat( RECVINFO_NAME(m_FlashlightState.m_UberlightState.m_fRoundness, m_fRoundness ) ),
END_RECV_TABLE()

C_EnvProjectedTexture::C_EnvProjectedTexture( void )
	: m_bEnableVolumetrics( false )
	, m_flVolumetricsQualityBias( 1.0f )
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_bLastUberState = false;
}

C_EnvProjectedTexture::~C_EnvProjectedTexture( void )
{
	//Assert( m_pVolmetricMesh == NULL );

	ShutDownLightHandle();
}

void C_EnvProjectedTexture::ShutDownLightHandle( void )
{
	// Clear out the light
	if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_LightHandle );
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void C_EnvProjectedTexture::SetLightColor(byte r, byte g, byte b, byte a)
{
	m_LightColor.r = r;
	m_LightColor.g = g;
	m_LightColor.b = b;
	m_LightColor.a = a;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : updateType -
//-----------------------------------------------------------------------------
void C_EnvProjectedTexture::OnDataChanged( DataUpdateType_t updateType )
{
	bool bTextureChanged = (!m_SpotlightTexture.IsValid() || !FStrEq(m_SpotlightTextureName, m_SpotlightTexture->GetName()));
	bool bUberStateChanged = m_bLastUberState != m_FlashlightState.m_UberlightState.m_bEnabled;
	m_bLastUberState = m_FlashlightState.m_UberlightState.m_bEnabled;

	if ( updateType == DATA_UPDATE_CREATED || bTextureChanged || bUberStateChanged)
	{
		m_SpotlightTexture.Init(m_FlashlightState.m_UberlightState.m_bEnabled ? "white" : m_SpotlightTextureName, TEXTURE_GROUP_OTHER, true );
	}

	UpdateLight( true );

	BaseClass::OnDataChanged( updateType );
}

void C_EnvProjectedTexture::UpdateLight( bool bForceUpdate )
{
	if ( m_bState == false )
	{
		if ( m_LightHandle != CLIENTSHADOW_INVALID_HANDLE )
		{
			ShutDownLightHandle();
		}
		return;
	}

	Vector vLinearFloatLightColor(m_LightColor.r, m_LightColor.g, m_LightColor.b);
	float flLinearFloatLightAlpha = m_LightColor.a;

	if (m_bAlwaysUpdate)
	{
		bForceUpdate = true;
	}

	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
		m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);

		bForceUpdate = true;
	}

	if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE || m_hTargetEntity != NULL || GetMoveParent() != NULL || bForceUpdate)
	{
		Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
		if (m_hTargetEntity != NULL)
		{
			if (m_bCameraSpace)
			{
				const QAngle& angles = GetLocalAngles();

				C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
				if (pPlayer)
				{
					const QAngle playerAngles = pPlayer->GetAbsAngles();

					Vector vPlayerForward, vPlayerRight, vPlayerUp;
					AngleVectors(playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp);

					matrix3x4_t	mRotMatrix;
					AngleMatrix(angles, mRotMatrix);

					VectorITransform(vPlayerForward, mRotMatrix, vForward);
					VectorITransform(vPlayerRight, mRotMatrix, vRight);
					VectorITransform(vPlayerUp, mRotMatrix, vUp);

					float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
					vPos = m_hTargetEntity->GetAbsOrigin() - vForward * dist;

					VectorNormalize(vForward);
					VectorNormalize(vRight);
					VectorNormalize(vUp);
				}
			}
			else
			{
				// VXP: Fixing targeting
				Vector vecToTarget;
				QAngle vecAngles;
				if (m_hTargetEntity == NULL)
				{
					vecAngles = GetAbsAngles();
				}
				else
				{
					vecToTarget = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
					VectorAngles(vecToTarget, vecAngles);
				}
				AngleVectors(vecAngles, &vForward, &vRight, &vUp);
			}
		}
		else
		{
			AngleVectors(GetAbsAngles(), &vForward, &vRight, &vUp);
		}


		m_FlashlightState.m_fHorizontalFOVDegrees = m_flLightFOV;
		m_FlashlightState.m_fVerticalFOVDegrees = m_flLightFOV;

		m_FlashlightState.m_vecLightOrigin = vPos;
		BasisToQuaternion(vForward, vRight, vUp, m_FlashlightState.m_quatOrientation);

		float flAlpha = m_flCurrentLinearFloatLightAlpha * (1.0f / 255.0f);

		// Get the current light style value to throttle the brightness by
		flAlpha *= engine->LightStyleValue(m_iStyle);

		m_FlashlightState.m_fQuadraticAtten = 0.0;
		m_FlashlightState.m_fLinearAtten = 100;
		m_FlashlightState.m_fConstantAtten = 0.0f;
		m_FlashlightState.m_Color[0] = m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * flAlpha;
		m_FlashlightState.m_Color[1] = m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * flAlpha;
		m_FlashlightState.m_Color[2] = m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * flAlpha;
		m_FlashlightState.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
		m_FlashlightState.m_NearZ = m_flNearZ;
		m_FlashlightState.m_FarZ = m_flFarZ;
		m_FlashlightState.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
		m_FlashlightState.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();
		m_FlashlightState.m_bEnableShadows = m_bEnableShadows;
		m_FlashlightState.m_pSpotlightTexture = m_SpotlightTexture;
		m_FlashlightState.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

		m_FlashlightState.m_bShadowHighRes = (m_nShadowQuality > 0); // Allow entity to affect shadow quality

		m_FlashlightState.m_bVolumetric = m_bEnableVolumetrics;
		m_FlashlightState.m_flVolumetricIntensity = m_flVolumetricsMultiplier;
		m_FlashlightState.m_nNumPlanes = m_iVolumetricsQuality;
		m_FlashlightState.m_flFlashlightTime = gpGlobals->curtime;

		if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE)
		{
			m_LightHandle = g_pClientShadowMgr->CreateFlashlight(m_FlashlightState);
		}
		else
		{
			g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, m_FlashlightState);
		}

		g_pClientShadowMgr->GetFrustumExtents(m_LightHandle, m_vecExtentsMin, m_vecExtentsMax);

		m_vecExtentsMin = m_vecExtentsMin - GetAbsOrigin();
		m_vecExtentsMax = m_vecExtentsMax - GetAbsOrigin();
	}

	if ( m_bLightOnlyTarget )
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, m_hTargetEntity );
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget( m_LightHandle, NULL );
	}

	g_pClientShadowMgr->SetFlashlightLightWorld( m_LightHandle, m_bLightWorld );

	g_pClientShadowMgr->UpdateProjectedTexture( m_LightHandle, true );

}

void C_EnvProjectedTexture::Simulate( void )
{
	UpdateLight( m_bAlwaysUpdate );

	BaseClass::Simulate();
}

bool C_EnvProjectedTexture::IsBBoxVisible(Vector vecExtentsMin, Vector vecExtentsMax)
{
	// Z position clamped to the min height (but must be less than the max)
	float flVisibleBBoxMinHeight = MIN(vecExtentsMax.z - 1.0f, m_flVisibleBBoxMinHeight);
	vecExtentsMin.z = MAX(vecExtentsMin.z, flVisibleBBoxMinHeight);

	// Check if the bbox is in the view
	return !engine->CullBox(vecExtentsMin, vecExtentsMax);
}