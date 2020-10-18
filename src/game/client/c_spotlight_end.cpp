//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "dlight.h"
#include "iefx.h"
#include "beamdraw.h"
#include "view.h"
#include "beam_shared.h"
#include "iclientshadowmgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//##################################################################
//
// PlasmaBeamNode - generates plasma embers
//
//##################################################################
class C_SpotlightEnd : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_SpotlightEnd, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_SpotlightEnd();

public:
	void	OnDataChanged(DataUpdateType_t updateType);
	bool	ShouldDraw();
	void	ClientThink( void );

	virtual bool ShouldInterpolate();


//	Vector	m_vSpotlightOrg;
//	Vector	m_vSpotlightDir;
	float	m_flLightScale;
	float	m_Radius;

private:
	dlight_t*	m_pDynamicLight;

	//dlight_t*	m_pModelLight;
};

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
C_SpotlightEnd::C_SpotlightEnd(void) : /*m_pModelLight(0), */m_pDynamicLight(0)
{
	m_flLightScale	= 100;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_SpotlightEnd::OnDataChanged(DataUpdateType_t updateType)
{
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool C_SpotlightEnd::ShouldDraw()
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: YWB:  This is a hack, BaseClass::Interpolate skips this entity because model == NULL
//   We could do something like model = (model_t *)0x00000001, but that's probably more evil.
// Input  : currentTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_SpotlightEnd::ShouldInterpolate()
{
	return true;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_SpotlightEnd::ClientThink(void)
{
	// If light scale is zero, don't draw light
	if ( m_flLightScale <= 0 )
		return;

	ColorRGBExp32 color;
	color.r	= m_clrRender->r * m_clrRender->a;
	color.g	= m_clrRender->g * m_clrRender->a;
	color.b	= m_clrRender->b * m_clrRender->a;
	color.exponent = 0;
	if ( color.r == 0 && color.g == 0 && color.b == 0 )
		return;

	// Deal with the environment light
	if ( !m_pDynamicLight || (m_pDynamicLight->key != index) )
	{
		m_pDynamicLight = effects->CL_AllocDlight( index );
		Assert (m_pDynamicLight);
	}

	//m_pDynamicLight->flags = DLIGHT_NO_MODEL_ILLUMINATION;
	m_pDynamicLight->radius		= m_flLightScale*3.0f;
	m_pDynamicLight->origin		= GetAbsOrigin() + Vector(0,0,5);
	m_pDynamicLight->die		= gpGlobals->curtime + 0.05f;
	m_pDynamicLight->color		= color;

	/*
	// For bumped lighting
	VectorCopy (m_vSpotlightDir,  m_pDynamicLight->m_Direction);

	// Deal with the model light
 	if ( !m_pModelLight || (m_pModelLight->key != -index) )
	{
		m_pModelLight = effects->CL_AllocDlight( -index );
		Assert (m_pModelLight);
	}

	m_pModelLight->radius = m_Radius;
	m_pModelLight->flags = DLIGHT_NO_WORLD_ILLUMINATION;
	m_pModelLight->color.r = m_clrRender->r * m_clrRender->a;
	m_pModelLight->color.g = m_clrRender->g * m_clrRender->a;
	m_pModelLight->color.b = m_clrRender->b * m_clrRender->a;
	m_pModelLight->color.exponent	= 1;
	m_pModelLight->origin		= m_vSpotlightOrg;
	m_pModelLight->m_InnerAngle = 6;
	m_pModelLight->m_OuterAngle = 8;
	m_pModelLight->die = gpGlobals->curtime + 0.05;
	VectorCopy( m_vSpotlightDir, m_pModelLight->m_Direction );
	*/

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

IMPLEMENT_CLIENTCLASS_DT(C_SpotlightEnd, DT_SpotlightEnd, CSpotlightEnd)
RecvPropFloat(RECVINFO(m_flLightScale)),
RecvPropFloat(RECVINFO(m_Radius)),
//	RecvPropVector	(RECVINFO(m_vSpotlightOrg)),
//	RecvPropVector	(RECVINFO(m_vSpotlightDir)),
END_RECV_TABLE();

class C_PointSpotlight : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_PointSpotlight, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_PointSpotlight();

	void	OnDataChanged(DataUpdateType_t updateType);
	bool	ShouldDraw();
	virtual int		DrawModel(int flags);
	void	ClientThink(void);
	virtual void	NotifyShouldTransmit(ShouldTransmitState_t state);
	void	UpdateOnRemove();

protected:
	void	DestroyBeam();
	void	DestroyFlashlight();

	bool m_bSpotlightOn;
	int m_nHaloSprite;
	int m_nBeamSprite;
	EHANDLE m_hSpotlightTarget;
	bool m_bVolumetricMode;
	bool m_bLightWorld;

	float m_flSpotlightMaxLength;
	float m_flSpotlightCurLength;
	float m_flSpotlightGoalWidth;
	float m_flHDRColorScale;
	int m_nMinDXLevel;
	int m_nNumPlanes;

	float m_flCurBeamEndWidth;
	pixelvis_handle_t	m_queryHandleHalo;

	C_Beam* m_pBeam;
	ClientShadowHandle_t m_LightHandle;

	// Texture for flashlight
	CTextureReference m_FlashlightTexture;
};

IMPLEMENT_CLIENTCLASS_DT(C_PointSpotlight, DT_PointSpotlight, CPointSpotlight)
RecvPropFloat(RECVINFO(m_flSpotlightCurLength)),
RecvPropBool(RECVINFO(m_bSpotlightOn)),

RecvPropFloat(RECVINFO(m_flSpotlightGoalWidth)),
RecvPropFloat(RECVINFO(m_flSpotlightMaxLength)),
RecvPropEHandle(RECVINFO(m_hSpotlightTarget)),

RecvPropFloat(RECVINFO(m_flHDRColorScale)),
RecvPropInt(RECVINFO(m_nMinDXLevel)),
RecvPropInt(RECVINFO(m_nNumPlanes)),
RecvPropInt(RECVINFO(m_nHaloSprite)),
RecvPropInt(RECVINFO(m_nBeamSprite)),
RecvPropBool(RECVINFO(m_bVolumetricMode)),
RecvPropBool(RECVINFO(m_bLightWorld)),
END_RECV_TABLE();

C_PointSpotlight::C_PointSpotlight()
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	m_pBeam = nullptr;
}

void C_PointSpotlight::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
}

bool C_PointSpotlight::ShouldDraw()
{
	// Some rendermodes prevent rendering
	if (GetRenderMode() == kRenderNone)
		return false;

	return m_bVolumetricMode && m_bSpotlightOn && !IsEffectActive(EF_NODRAW);
}

int C_PointSpotlight::DrawModel(int flags)
{
	if (!m_hSpotlightTarget.Get())
		return 0;

	const model_t* halosprite = modelinfo->GetModel(m_nHaloSprite);
	if (!halosprite)
		return 0;

	// HACKHACK: heuristic to estimate proxy size.  Revisit this!
	float size = 1.0f + (60 * m_flSpotlightGoalWidth / m_flCurBeamEndWidth);
	size = clamp(size, 1.0f, 8.0f);

	Vector beamDir = m_hSpotlightTarget->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize(beamDir);

	Vector localDir = CurrentViewOrigin() - GetAbsOrigin();
	VectorNormalize(localDir);

	float dotpr = DotProduct(beamDir, localDir);
	float fade;

	if (dotpr < 0.0f)
	{
		fade = 0;
	}
	else
	{
		fade = dotpr * 2.0f;
	}

	float	distToLine;
	Vector	out;

	// Find out how close we are to the "line" of the spotlight
	CalcClosestPointOnLine(CurrentViewOrigin(), GetAbsOrigin(), GetAbsOrigin() + (beamDir * 2), out, &distToLine);

	distToLine = (CurrentViewOrigin() - out).Length();

	//float scaleColor[4];
	//float dotScale = 1.0f;

	// Use beam width
	float distThreshold = m_flSpotlightGoalWidth * 4.0f;

	Vector vSource = GetAbsOrigin();
	Vector srcColor;
	const color32& color = GetRenderColor();
	srcColor.x = color.r;
	srcColor.y = color.g;
	srcColor.z = color.b;
	srcColor /= 255.f;

	pixelvis_queryparams_t params;
	params.Init(vSource, size);

	float haloFractionVisible = PixelVisibility_FractionVisible(params, &m_queryHandleHalo);
	if (fade && haloFractionVisible > 0.0f)
	{
		//NOTENOTE: This is kinda funky when moving away and to the backside -- jdw
		float haloScale = RemapVal(distToLine, distThreshold, m_flSpotlightGoalWidth * 0.5f, 1.0f, 2.0f);

		haloScale = clamp(haloScale, 1.0f, 2.0f);

		haloScale *= 60;

		float colorFade = fade * fade;
		colorFade = clamp(colorFade, 0.f, 1.f);

		float haloColor[3];
		VectorScale(srcColor.Base(), colorFade * haloFractionVisible, haloColor);

		BeamDrawHalo(halosprite, 0, kRenderGlow, vSource, haloScale, haloColor, m_flHDRColorScale);
	}

	return 1;
}

ConVar cl_spotlight_light_fov_scale("cl_spotlight_light_fov_scale", "20", FCVAR_CHEAT);
void C_PointSpotlight::ClientThink(void)
{
	if (m_bVolumetricMode && m_bSpotlightOn && m_hSpotlightTarget.Get())
	{
		// Adjust end width to keep beam width constant
		m_flCurBeamEndWidth = m_flSpotlightGoalWidth * (m_flSpotlightCurLength / m_flSpotlightMaxLength);
		m_flCurBeamEndWidth = clamp(m_flCurBeamEndWidth, 0.f, MAX_BEAM_WIDTH);

		if (g_pClientShadowMgr->VolumetricsAvailable())
		{
			DestroyBeam();

			if (!m_FlashlightTexture.IsValid())
			{
				m_FlashlightTexture.Init("effects/flashlight001", TEXTURE_GROUP_OTHER);
			}

			Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
			QAngle vecAngles;
			Vector vecToTarget = m_hSpotlightTarget->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(vecToTarget);
			VectorAngles(vecToTarget, vecAngles);
			AngleVectors(vecAngles, &vForward, &vRight, &vUp);
			Vector vecToTargetOuterRight = (m_hSpotlightTarget->GetAbsOrigin() + (vRight * m_flSpotlightGoalWidth)) - GetAbsOrigin();
			VectorNormalize(vecToTargetOuterRight);
			Vector vecToTargetOuterLeft = (m_hSpotlightTarget->GetAbsOrigin() - (vRight * m_flSpotlightGoalWidth)) - GetAbsOrigin();
			VectorNormalize(vecToTargetOuterLeft);
			float flDot = DotProduct(vecToTargetOuterLeft, vecToTargetOuterRight);

			ClientFlashlightState_t state;
			state.m_vecLightOrigin = vPos;
			BasisToQuaternion(vForward, vRight, vUp, state.m_quatOrientation);
			state.m_fVerticalFOVDegrees = state.m_fHorizontalFOVDegrees = (1.f - Clamp(flDot, 0.f, 1.f)) * 180.f * cl_spotlight_light_fov_scale.GetFloat();

			Vector srcColor;
			const color32& color = GetRenderColor();
			srcColor.x = color.r;
			srcColor.y = color.g;
			srcColor.z = color.b;
			srcColor /= 255.f;
			if (g_pMaterialSystemHardwareConfig->GetHDREnabled())
			{
				srcColor *= m_flHDRColorScale;
			}

			srcColor.CopyToArray(state.m_Color);

			state.m_fQuadraticAtten = 0.0;
			state.m_fLinearAtten = 100;
			state.m_fConstantAtten = 0.0f;

			state.m_NearZ = 7.f;
			state.m_FarZ = Min(m_flSpotlightCurLength, m_flSpotlightMaxLength);
			state.m_bEnableShadows = true;
			state.m_pSpotlightTexture = m_FlashlightTexture;
			state.m_nSpotlightTextureFrame = 0;

			state.m_bVolumetric = true;
			state.m_flVolumetricIntensity = 2.f;
			state.m_nNumPlanes = m_nNumPlanes;
			state.m_flFlashlightTime = gpGlobals->curtime;

			if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE)
			{
				m_LightHandle = g_pClientShadowMgr->CreateFlashlight(state);
			}
			else
			{
				g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, state);
			}

			g_pClientShadowMgr->SetFlashlightLightWorld(m_LightHandle, m_bLightWorld);
			g_pClientShadowMgr->UpdateProjectedTexture(m_LightHandle, true);
		}
		else
		{
			DestroyFlashlight();

			if (!m_pBeam)
			{
				m_pBeam = CBeam::BeamCreate("sprites/glow_test02.vmt", m_flSpotlightGoalWidth);
				// Set the temporary spawnflag on the beam so it doesn't save (we'll recreate it on restore)
				m_pBeam->SetHDRColorScale(m_flHDRColorScale);
				m_pBeam->SetHaloTexture(m_nHaloSprite);
				m_pBeam->SetHaloScale(0);
				m_pBeam->SetEndWidth(m_flSpotlightGoalWidth);
				m_pBeam->SetBeamFlags((FBEAM_SHADEOUT | FBEAM_NOTILE));
				m_pBeam->SetBrightness(64);
				m_pBeam->SetNoise(0);
				m_pBeam->SetMinDXLevel(m_nMinDXLevel);
				m_pBeam->EntsInit(this, m_hSpotlightTarget);
			}

			m_pBeam->SetColor(m_clrRender->r, m_clrRender->g, m_clrRender->b);

			if (m_flSpotlightCurLength > m_flSpotlightMaxLength)
			{
				m_pBeam->SetFadeLength(m_flSpotlightMaxLength);
			}
			else
			{
				m_pBeam->SetFadeLength(m_flSpotlightCurLength);
			}

			m_pBeam->SetEndWidth(m_flCurBeamEndWidth);
		}
	}
	else
	{
		DestroyBeam();
		DestroyFlashlight();
	}
}

void C_PointSpotlight::NotifyShouldTransmit(ShouldTransmitState_t state)
{
	BaseClass::NotifyShouldTransmit(state);

	switch (state)
	{
	case SHOULDTRANSMIT_START:
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
	break;

	case SHOULDTRANSMIT_END:
	{
		DestroyBeam();
		DestroyFlashlight();
		SetNextClientThink(CLIENT_THINK_NEVER);
	}
	break;

	default:
		Assert(0);
		break;
	}
}

void C_PointSpotlight::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	DestroyBeam();
	DestroyFlashlight();
}

void C_PointSpotlight::DestroyBeam()
{
	if (m_pBeam)
	{
		m_pBeam->SUB_Remove();
		m_pBeam = nullptr;
	}
}

void C_PointSpotlight::DestroyFlashlight()
{
	// Clear out the light
	if (m_LightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyFlashlight(m_LightHandle);
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}

	if (m_FlashlightTexture.IsValid())
	{
		m_FlashlightTexture.Shutdown();
	}
}
