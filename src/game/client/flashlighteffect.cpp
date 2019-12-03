//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "flashlighteffect.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

#ifdef HL2_CLIENT_DLL
#include "c_basehlplayer.h"
#endif // HL2_CLIENT_DLL

#if defined( _X360 )
extern ConVar r_flashlightdepthres;
#else
extern ConVar r_flashlightdepthres;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_flashlightdepthtexture;

static ConVar r_swingflashlight( "r_swingflashlight", "1", FCVAR_CHEAT );
static ConVar r_flashlightlockposition( "r_flashlightlockposition", "0", FCVAR_CHEAT );
static ConVar r_flashlightfov( "r_flashlightfov", "45.0", FCVAR_CHEAT );
static ConVar r_nvgfov("r_nvgfov", "100", FCVAR_CHEAT); // Breadman - Changed for NVG effect
static ConVar r_flashlightoffsetx( "r_flashlightoffsetx", "10.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsety( "r_flashlightoffsety", "-20.0", FCVAR_CHEAT );
static ConVar r_flashlightoffsetz( "r_flashlightoffsetz", "24.0", FCVAR_CHEAT );
static ConVar r_flashlightnear( "r_flashlightnear", "4.0", FCVAR_CHEAT );
static ConVar r_flashlightfar( "r_flashlightfar", "750.0", FCVAR_CHEAT );
static ConVar r_flashlightconstant( "r_flashlightconstant", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightlinear( "r_flashlightlinear", "100.0", FCVAR_CHEAT );
static ConVar r_flashlightquadratic( "r_flashlightquadratic", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightvisualizetrace( "r_flashlightvisualizetrace", "0", FCVAR_CHEAT );
static ConVar r_flashlightambient( "r_flashlightambient", "0.0", FCVAR_CHEAT );
static ConVar r_flashlightshadowatten( "r_flashlightshadowatten", "0.35", FCVAR_CHEAT );
static ConVar r_flashlightladderdist( "r_flashlightladderdist", "40.0", FCVAR_CHEAT );
static ConVar mat_slopescaledepthbias_shadowmap( "mat_slopescaledepthbias_shadowmap", "16", FCVAR_CHEAT );
static ConVar mat_depthbias_shadowmap(	"mat_depthbias_shadowmap", "0.0005", FCVAR_CHEAT  );

CUtlVector< ClientShadowHandle_t > g_hFlashlightHandle;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nEntIndex - The m_nEntIndex of the client entity that is creating us.
//			vecPos - The position of the light emitter.
//			vecDir - The direction of the light emission.
//-----------------------------------------------------------------------------
CFlashlightEffectBase::CFlashlightEffectBase(bool bLocalPlayer)
{
	m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;

	m_bIsOn = false;
	m_bIsLocalPlayerLight = bLocalPlayer;

	/*if ( g_pMaterialSystemHardwareConfig->SupportsBorderColor() )
	{
		m_FlashlightTexture.Init( "effects/flashlight_border", TEXTURE_GROUP_OTHER, true );
	}
	else
	{
		m_FlashlightTexture.Init( "effects/flashlight001", TEXTURE_GROUP_OTHER, true );
	}*/

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFlashlightEffectBase::~CFlashlightEffectBase()
{
	LightOff();
}

void CFlashlightEffectBase::InitSpotlightTexture(const char* pszLightTexture)
{
	m_FlashlightTexture.Init(pszLightTexture, TEXTURE_GROUP_OTHER, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffectBase::TurnOn()
{
	m_bIsOn = true;
	m_flDistMod = 1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffectBase::TurnOff()
{
	if (m_bIsOn)
	{
		m_bIsOn = false;
		LightOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffectBase::UpdateLightProjection(ClientFlashlightState_t& state)
{
	if (m_FlashlightHandle == CLIENTSHADOW_INVALID_HANDLE)
	{
		m_FlashlightHandle = g_pClientShadowMgr->CreateFlashlight(state);
		if (m_bIsLocalPlayerLight)
			g_hFlashlightHandle.AddToTail(m_FlashlightHandle);
	}
	else
	{
		if (!r_flashlightlockposition.GetBool())
		{
			g_pClientShadowMgr->UpdateFlashlightState(m_FlashlightHandle, state);
		}
	}

	g_pClientShadowMgr->UpdateProjectedTexture(m_FlashlightHandle, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFlashlightEffectBase::LightOff()
{
#ifndef NO_TOOLFRAMEWORK
	if (clienttools->IsInRecordingMode())
	{
		KeyValues* msg = new KeyValues("FlashlightState");
		msg->SetFloat("time", gpGlobals->curtime);
		msg->SetInt("flashlightHandle", m_FlashlightHandle);
		msg->SetPtr("flashlightState", NULL);
		ToolFramework_PostToolMessage(HTOOLHANDLE_INVALID, msg);
		msg->deleteThis();
	}
#endif

	// Clear out the light
	if (m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		if (m_bIsLocalPlayerLight)
			g_hFlashlightHandle.FindAndRemove(m_FlashlightHandle);

		g_pClientShadowMgr->DestroyFlashlight(m_FlashlightHandle);
		m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

// Custom trace filter that skips the player and the view model.
// If we don't do this, we'll end up having the light right in front of us all
// the time.
class CTraceFilterSkipPlayerAndViewModel : public CTraceFilter
{
public:
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		// Test against the vehicle too?
		// FLASHLIGHTFIXME: how do you know that you are actually inside of the vehicle?
		C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
		if ( !pEntity )
			return true;

		if ( ( dynamic_cast<C_BaseViewModel *>( pEntity ) != NULL ) ||
			 pEntity->IsPlayer() ||
			 pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
			 pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		{
			return false;
		}

		return true;
	}
};

CFlashlightEffect::CFlashlightEffect(int iEntIndex, bool bIsNVG) : CFlashlightEffectBase(true)
{
	m_nEntIndex = iEntIndex;
	m_bIsNVG = bIsNVG;

	if (bIsNVG)
	{
		if (g_pMaterialSystemHardwareConfig->SupportsBorderColor())
		{
			InitSpotlightTexture("effects/Ez_MetroVision_border");
		}
		else
		{
			InitSpotlightTexture("effects/Ez_MetroVision");
		}
	}
	else
	{
		if (g_pMaterialSystemHardwareConfig->SupportsBorderColor())
		{
			InitSpotlightTexture("effects/flashlight_border");
		}
		else
		{
			InitSpotlightTexture("effects/flashlight001");
		}
	}
}

CFlashlightEffect::~CFlashlightEffect()
{
}

//-----------------------------------------------------------------------------
// Purpose: Do the headlight
//-----------------------------------------------------------------------------
void CFlashlightEffect::UpdateLightNew(const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
	VPROF_BUDGET( "CFlashlightEffectBase::UpdateLightNew", VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

	ClientFlashlightState_t state;
	float flFov;

	if (m_bIsNVG)
	{
		flFov = r_nvgfov.GetFloat();
	}
	else
	{
		flFov = r_flashlightfov.GetFloat();
	}

	C_BasePlayer* pl = UTIL_PlayerByIndex( m_nEntIndex );
	
	// We will lock some of the flashlight params if player is on a ladder, to prevent oscillations due to the trace-rays
	bool bPlayerOnLadder = pl && pl->GetMoveType() == MOVETYPE_LADDER;

	const float flEpsilon = 0.1f;			// Offset flashlight position along vecUp
	const float flDistCutoff = 128.0f;
	const float flDistDrag = 0.2;

	CTraceFilterSkipPlayerAndViewModel traceFilter;
	float flOffsetY = r_flashlightoffsety.GetFloat();

	if( r_swingflashlight.GetBool() )
	{
		// This projects the view direction backwards, attempting to raise the vertical
		// offset of the flashlight, but only when the player is looking down.
		Vector vecSwingLight = vecPos + vecForward * -12.0f;
		if( vecSwingLight.z > vecPos.z )
		{
			flOffsetY += (vecSwingLight.z - vecPos.z);
		}
	}

	Vector vOrigin = vecPos + flOffsetY * vecUp;

	// Not on ladder...trace a hull
	if ( !bPlayerOnLadder ) 
	{
		trace_t pmOriginTrace;
		UTIL_TraceHull( vecPos, vOrigin, Vector(-4, -4, -4), Vector(4, 4, 4), MASK_SOLID & ~(CONTENTS_HITBOX), &traceFilter, &pmOriginTrace );

		if ( pmOriginTrace.DidHit() )
		{
			vOrigin = vecPos;
		}
	}
	else // on ladder...skip the above hull trace
	{
		vOrigin = vecPos;
	}

	// Now do a trace along the flashlight direction to ensure there is nothing within range to pull back from
	int iMask = MASK_OPAQUE_AND_NPCS;
	iMask &= ~CONTENTS_HITBOX;
	iMask |= CONTENTS_WINDOW;

	Vector vTarget = vecPos + vecForward * r_flashlightfar.GetFloat();

	// Work with these local copies of the basis for the rest of the function
	Vector vDir   = vTarget - vOrigin;
	Vector vRight = vecRight;
	Vector vUp    = vecUp;
	VectorNormalize( vDir   );
	VectorNormalize( vRight );
	VectorNormalize( vUp    );

	// Orthonormalize the basis, since the flashlight texture projection will require this later...
	vUp -= DotProduct( vDir, vUp ) * vDir;
	VectorNormalize( vUp );
	vRight -= DotProduct( vDir, vRight ) * vDir;
	VectorNormalize( vRight );
	vRight -= DotProduct( vUp, vRight ) * vUp;
	VectorNormalize( vRight );

	AssertFloatEquals( DotProduct( vDir, vRight ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( vDir, vUp    ), 0.0f, 1e-3 );
	AssertFloatEquals( DotProduct( vRight, vUp  ), 0.0f, 1e-3 );

	trace_t pmDirectionTrace;
	UTIL_TraceHull( vOrigin, vTarget, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), iMask, &traceFilter, &pmDirectionTrace );

	if ( r_flashlightvisualizetrace.GetBool() == true )
	{
		debugoverlay->AddBoxOverlay( pmDirectionTrace.endpos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), QAngle( 0, 0, 0 ), 0, 0, 255, 16, 0 );
		debugoverlay->AddLineOverlay( vOrigin, pmDirectionTrace.endpos, 255, 0, 0, false, 0 );
	}

	float flDist = (pmDirectionTrace.endpos - vOrigin).Length();
	if ( flDist < flDistCutoff )
	{
		// We have an intersection with our cutoff range
		// Determine how far to pull back, then trace to see if we are clear
		float flPullBackDist = bPlayerOnLadder ? r_flashlightladderdist.GetFloat() : flDistCutoff - flDist;	// Fixed pull-back distance if on ladder
		m_flDistMod = Lerp( flDistDrag, m_flDistMod, flPullBackDist );
		
		if ( !bPlayerOnLadder )
		{
			trace_t pmBackTrace;
			UTIL_TraceHull( vOrigin, vOrigin - vDir*(flPullBackDist-flEpsilon), Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), iMask, &traceFilter, &pmBackTrace );
			if( pmBackTrace.DidHit() )
			{
				// We have an intersection behind us as well, so limit our m_flDistMod
				float flMaxDist = (pmBackTrace.endpos - vOrigin).Length() - flEpsilon;
				if( m_flDistMod > flMaxDist )
					m_flDistMod = flMaxDist;
			}
		}
	}
	else
	{
		m_flDistMod = Lerp( flDistDrag, m_flDistMod, 0.0f );
	}
	vOrigin = vOrigin - vDir * m_flDistMod;

	state.m_vecLightOrigin = vOrigin;

	BasisToQuaternion( vDir, vRight, vUp, state.m_quatOrientation );

	state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();

	bool bFlicker = false;

#ifdef HL2_EPISODIC
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		float flBatteryPower = ( pPlayer->m_HL2Local.m_flFlashBattery >= 0.0f ) ? ( pPlayer->m_HL2Local.m_flFlashBattery ) : pPlayer->m_HL2Local.m_flSuitPower;
		if ( flBatteryPower <= 10.0f )
		{
			float flScale;
			if ( flBatteryPower >= 0.0f )
			{	
				flScale = ( flBatteryPower <= 4.5f ) ? SimpleSplineRemapVal( flBatteryPower, 4.5f, 0.0f, 1.0f, 0.0f ) : 1.0f;
			}
			else
			{
				flScale = SimpleSplineRemapVal( flBatteryPower, 10.0f, 4.8f, 1.0f, 0.0f );
			}
			
			flScale = clamp( flScale, 0.0f, 1.0f );

			if ( flScale < 0.35f )
			{
				float flFlicker = cosf( gpGlobals->curtime * 6.0f ) * sinf( gpGlobals->curtime * 15.0f );
				
				if ( flFlicker > 0.25f && flFlicker < 0.75f )
				{
					// On
					state.m_fLinearAtten = r_flashlightlinear.GetFloat() * flScale;
				}
				else
				{
					// Off
					state.m_fLinearAtten = 0.0f;
				}
			}
			else
			{
				float flNoise = cosf( gpGlobals->curtime * 7.0f ) * sinf( gpGlobals->curtime * 25.0f );
				state.m_fLinearAtten = r_flashlightlinear.GetFloat() * flScale + 1.5f * flNoise;
			}

			state.m_fHorizontalFOVDegrees = flFov - ( 16.0f * (1.0f-flScale) );
			state.m_fVerticalFOVDegrees = flFov - ( 16.0f * (1.0f-flScale) );
			
			bFlicker = true;
		}
	}
#endif // HL2_EPISODIC

	if ( bFlicker == false )
	{
		state.m_fLinearAtten = r_flashlightlinear.GetFloat();
		state.m_fHorizontalFOVDegrees = flFov;
		state.m_fVerticalFOVDegrees = flFov;
	}

	state.m_fConstantAtten = r_flashlightconstant.GetFloat();
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = r_flashlightambient.GetFloat();
	state.m_NearZ = r_flashlightnear.GetFloat() + m_flDistMod;	// Push near plane out so that we don't clip the world when the flashlight pulls back 
	state.m_FarZ = r_flashlightfar.GetFloat();
	state.m_bEnableShadows = r_flashlightdepthtexture.GetBool();
	state.m_flShadowMapResolution = r_flashlightdepthres.GetInt();

	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;

	state.m_flShadowAtten = r_flashlightshadowatten.GetFloat();
	state.m_flShadowSlopeScaleDepthBias = mat_slopescaledepthbias_shadowmap.GetFloat();
	state.m_flShadowDepthBias = mat_depthbias_shadowmap.GetFloat();

	UpdateLightProjection(state);

#ifndef NO_TOOLFRAMEWORK
	if ( clienttools->IsInRecordingMode() )
	{
		KeyValues *msg = new KeyValues( "FlashlightState" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetInt( "entindex", m_nEntIndex );
		msg->SetInt( "flashlightHandle", m_FlashlightHandle );
		msg->SetPtr( "flashlightState", &state );
		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
#endif
}

CHeadlightEffect::CHeadlightEffect() 
{
	if (g_pMaterialSystemHardwareConfig->SupportsBorderColor())
	{
		InitSpotlightTexture("effects/flashlight_border");
	}
	else
	{
		InitSpotlightTexture("effects/flashlight001");
	}
}

CHeadlightEffect::~CHeadlightEffect()
{
	
}

void CHeadlightEffect::UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance, float flScale)
{
	if ( IsOn() == false )
		 return;

	ClientFlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion( basisX, basisY, basisZ, state.m_quatOrientation );
		
	state.m_vecLightOrigin = vecPos;

	state.m_fHorizontalFOVDegrees = 45.0f;
	state.m_fVerticalFOVDegrees = 30.0f;
	state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
	state.m_fLinearAtten = r_flashlightlinear.GetFloat();
	state.m_fConstantAtten = r_flashlightconstant.GetFloat();
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = r_flashlightambient.GetFloat();
	state.m_NearZ = r_flashlightnear.GetFloat();
	state.m_FarZ = r_flashlightfar.GetFloat();
	state.m_bEnableShadows = true;
	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;
	
	UpdateLightProjection( state );
}

class CSpotlightLODSystem : public CAutoGameSystemPerFrame
{
	DECLARE_CLASS_GAMEROOT(CSpotlightLODSystem, CAutoGameSystemPerFrame);
public:
	CSpotlightLODSystem() : CAutoGameSystemPerFrame("CSpotlightLOD")
	{}

	// Gets called each frame
	virtual void Update(float frametime)
	{
		if (frametime == 0.0f)
			return;

		if (m_Spotlights.Count() > 0)
			m_Spotlights.Sort(ThisClass::SpotlightSort);
	}

	virtual void LevelShutdownPostEntity()
	{
		m_Spotlights.Purge();
	}

	void AddSpotlight(CSpotlightEffect *pLight)
	{
		m_Spotlights.AddToTail(pLight);
	}

	void RemoveSpotlight(CSpotlightEffect *pLight)
	{
		m_Spotlights.FindAndRemove(pLight);
	}

	typedef CSpotlightEffect * SpotPtr;

	static int SpotlightSort(const SpotPtr *p1, const SpotPtr *p2);

	bool IsSpotlightHighLOD(CSpotlightEffect *pLight);

protected:
	CUtlVector<SpotPtr> m_Spotlights;
};

CSpotlightLODSystem g_SpotlightLod;



int CSpotlightLODSystem::SpotlightSort(const SpotPtr *p1, const SpotPtr *p2)
{
	const Vector vecView = MainViewOrigin();

	SpotPtr pLeft = *p1;
	float flDist2Left = vecView.DistToSqr(pLeft->GetPosition());

	SpotPtr pRight = *p2;
	float flDist2Right = vecView.DistToSqr(pRight->GetPosition());

	if (flDist2Left < flDist2Right)
		return -1;

	if (flDist2Right < flDist2Left)
		return 1;

	return 0;
}

bool CSpotlightLODSystem::IsSpotlightHighLOD(CSpotlightEffect *pLight)
{
	/*float flDistSqr = pLight->GetPosition().DistToSqr(MainViewOrigin());
	if (flDistSqr > Sqr(r_flashlightfar.GetInt()*1.2f))
		return false;*/

	int iMaxSpotlights = Floor2Int(g_pClientShadowMgr->GetMaxShadowDepthtextures() * 0.25f);
	iMaxSpotlights = Max(iMaxSpotlights, 1);

	int iIndex = m_Spotlights.Find(pLight);

	return (iIndex < iMaxSpotlights) ? true : false;
}

CSpotlightEffect::CSpotlightEffect()
{
	m_pDynamicLight = nullptr;
	m_pSpotlightEnd = nullptr;

	m_vecOrigin = vec3_origin;

	g_SpotlightLod.AddSpotlight(this);

	if (g_pMaterialSystemHardwareConfig->SupportsBorderColor())
	{
		InitSpotlightTexture("effects/flashlight_border");
	}
	else
	{
		InitSpotlightTexture("effects/flashlight001");
	}
}

CSpotlightEffect::~CSpotlightEffect()
{
	g_SpotlightLod.RemoveSpotlight(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSpotlightEffect::LightOffOld()
{
	if (m_pDynamicLight )
	{
		m_pDynamicLight->die = gpGlobals->curtime;
		m_pDynamicLight = NULL;
	}

	if (m_pSpotlightEnd )
	{
		m_pSpotlightEnd->die = gpGlobals->curtime;
		m_pSpotlightEnd = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSpotlightEffect::LightOff()
{
	LightOffOld();
	CFlashlightEffectBase::LightOff();
}

//-----------------------------------------------------------------------------
// Purpose: Do the headlight
//-----------------------------------------------------------------------------
void CSpotlightEffect::UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance, float flScale)
{
	if (IsOn() == false)
		return;

	nDistance = r_flashlightfar.GetInt();

	Vector end;
	end = vecPos + nDistance * vecDir;

	// Trace a line outward, skipping the player model and the view model.
	trace_t pm;
	CTraceFilterSkipPlayerAndViewModel traceFilter;
	C_BaseEntity::PushEnableAbsRecomputations(false);	 // HACK don't recompute positions while doing RayTrace
	UTIL_TraceHull(vecPos, end, Vector(-4, -4, -4), Vector(4, 4, 4), MASK_ALL, &traceFilter, &pm);
	C_BaseEntity::PopEnableAbsRecomputations();
	VectorCopy(pm.endpos, m_vecOrigin);

	if( g_SpotlightLod.IsSpotlightHighLOD(this) )
	{
		UpdateLightNew( vecPos, vecDir, vecRight, vecUp, flScale );
	}
	else
	{
		UpdateLightOld( vecPos, vecDir, nDistance, flScale);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do the headlight
//-----------------------------------------------------------------------------
void CSpotlightEffect::UpdateLightOld(const Vector &vecPos, const Vector &vecDir, int nDistance, float flScale)
{

	Vector end;
	end = vecPos + nDistance * vecDir;

	// Trace a line outward, skipping the player model and the view model.
	trace_t pm;
	CTraceFilterSkipPlayerAndViewModel traceFilter;
	C_BaseEntity::PushEnableAbsRecomputations(false);	 // HACK don't recompute positions while doing RayTrace
	UTIL_TraceLine(vecPos, end, MASK_ALL, &traceFilter, &pm);
	C_BaseEntity::PopEnableAbsRecomputations();

	float falloff = pm.fraction * nDistance;

	if (falloff < 500)
		falloff = 1.0;
	else
		falloff = 500.0 / falloff;

	falloff *= falloff;

	// Adjust end width to keep beam width constant
	float flNewWidth = 80 * pm.fraction;
	flNewWidth = Max(10.f, flNewWidth);
	flNewWidth *= 3;

	flScale = Clamp(flScale, 0.0f, 1.0f);

	float flBaseFov = r_flashlightfov.GetFloat() /** 1.25f*/;
	float flBaseRadius = flNewWidth;
	float flColorScale = 1.0f;

	if (flScale < 1.0f)
	{
		if (flScale < 0.35f)
		{
			float flFlicker = cosf(gpGlobals->curtime * 6.0f) * sinf(gpGlobals->curtime * 15.0f);

			if (flFlicker > 0.25f && flFlicker < 0.75f)
			{
				// On
				flColorScale = flScale;
			}
			else
			{
				// Off
				flColorScale = 0.0f;
			}
		}
		else
		{
			float flNoise = cosf(gpGlobals->curtime * 7.0f) * sinf(gpGlobals->curtime * 25.0f);
			flColorScale = flScale + 1.5f * flNoise;
		}

		flBaseFov = (r_flashlightfov.GetFloat() /** 1.25f*/) - (16.0f * (1.0f - flScale));
		flNewWidth = flBaseRadius - (16.0f * (1.0f - flScale));
	}

	ColorRGBExp32 clrLight;
	clrLight.r = clrLight.g = clrLight.b = 255 * flColorScale;
	clrLight.exponent = 0;

	/*Vector clrLight;
	clrLight.Init(flColorScale, flColorScale, flColorScale);*/
#if 0
	// Deal with the model light
	if (!m_pDynamicLight || (m_pDynamicLight->key != m_nEntIndex))
	{
#if DLIGHT_NO_WORLD_USES_ELIGHT
		m_pDynamicLight = ShouldBeElight() != 0
			? effects->CL_AllocElight(m_nEntIndex)
			: effects->CL_AllocDlight(m_nEntIndex);
#else
		m_pDynamicLight = effects->CL_AllocElight(m_nEntIndex);
#endif
		Assert(m_pDynamicLight);
		//m_pDynamicLight->minlight = 0;
	}

	//m_pDynamicLight->style = m_LightStyle;
	m_pDynamicLight->radius = nDistance;
	m_pDynamicLight->flags = DLIGHT_NO_WORLD_ILLUMINATION;

	//VectorToColorRGBExp32(clrLight, m_pDynamicLight->color);
	m_pDynamicLight->color = clrLight;
	m_pDynamicLight->origin = vecPos;
	m_pDynamicLight->m_InnerAngle = flBaseFov * 0.85f;
	m_pDynamicLight->m_OuterAngle = flBaseFov;
	m_pDynamicLight->die = gpGlobals->curtime + 1e6;

	// For bumped lighting
	VectorCopy(vecDir, m_pDynamicLight->m_Direction);
#endif

	if (!m_pSpotlightEnd)
	{
		// Set up the environment light
		m_pSpotlightEnd = effects->CL_AllocDlight(0);
	}

	// For bumped lighting
	VectorCopy(vecDir, m_pSpotlightEnd->m_Direction);

	VectorCopy(pm.endpos, m_pSpotlightEnd->origin);

	//m_pSpotlightEnd->flags = DLIGHT_NO_MODEL_ILLUMINATION;
	m_pSpotlightEnd->radius = flNewWidth;
	//VectorToColorRGBExp32(clrLight * falloff, m_pSpotlightEnd->color);
	m_pSpotlightEnd->color = clrLight;
	m_pSpotlightEnd->color.r *= falloff;
	m_pSpotlightEnd->color.g *= falloff;
	m_pSpotlightEnd->color.b *= falloff;

	// Make it live for a bit
	m_pSpotlightEnd->die = gpGlobals->curtime + 0.2f;

	// Update list of surfaces we influence
	render->TouchLight(m_pSpotlightEnd);

	// kill the new flashlight if we have one
	CFlashlightEffectBase::LightOff();
}

void CSpotlightEffect::UpdateLightNew(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, float flScale)
{
	

	/*if (r_dynamic_shadow_mode.GetInt() < 2)
		return;*/

	ClientFlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion(basisX, basisY, basisZ, state.m_quatOrientation);

	state.m_vecLightOrigin = vecPos;

	//state.m_fHorizontalFOVDegrees = 45.0f;
	//state.m_fVerticalFOVDegrees = 45.0f;
	state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
	//state.m_fLinearAtten = r_flashlightlinear.GetFloat();
	state.m_fConstantAtten = 0.0f;
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = r_flashlightambient.GetFloat();
	state.m_NearZ = r_flashlightnear.GetFloat();
	state.m_FarZ = r_flashlightfar.GetFloat()/* * 1.5f*/;
	state.m_bEnableShadows = true;
	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;

	bool bFlicker = false;

	flScale = Clamp(flScale, 0.0f, 1.0f);

	float flBaseFov = r_flashlightfov.GetFloat() * 1.25f;

	if (flScale < 1.0f)
	{
		if (flScale < 0.35f)
		{
			float flFlicker = cosf(gpGlobals->curtime * 6.0f) * sinf(gpGlobals->curtime * 15.0f);

			if (flFlicker > 0.25f && flFlicker < 0.75f)
			{
				// On
				state.m_fLinearAtten = r_flashlightlinear.GetFloat() * flScale;
			}
			else
			{
				// Off
				state.m_fLinearAtten = 0.0f;
			}
		}
		else
		{
			float flNoise = cosf(gpGlobals->curtime * 7.0f) * sinf(gpGlobals->curtime * 25.0f);
			state.m_fLinearAtten = r_flashlightlinear.GetFloat() * flScale + 1.5f * flNoise;
		}

		state.m_fHorizontalFOVDegrees = flBaseFov - (16.0f * (1.0f - flScale));
		state.m_fVerticalFOVDegrees = flBaseFov - (16.0f * (1.0f - flScale));

		bFlicker = true;
	}

	LightOffOld();

	if (bFlicker == false)
	{
		state.m_fLinearAtten = r_flashlightlinear.GetFloat();
		//state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
		state.m_fHorizontalFOVDegrees = flBaseFov;
		state.m_fVerticalFOVDegrees = flBaseFov;
	}

	UpdateLightProjection(state);
}

class CMuzzleFlashEffect : public CFlashlightEffectBase
{
public:

	CMuzzleFlashEffect(ColorRGBExp32 clrColor);
	~CMuzzleFlashEffect();

	virtual void UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance, float flScale = 1.0f);

	float m_flCreationTime;
	int m_iCreationFrame;

protected:
	Vector m_vecColor;
};

CMuzzleFlashEffect::CMuzzleFlashEffect(ColorRGBExp32 clrColor) : CFlashlightEffectBase(true)
{
	m_iCreationFrame = gpGlobals->framecount;
	m_flCreationTime = gpGlobals->curtime;

	ColorRGBExp32ToVector(clrColor, m_vecColor);
	m_vecColor /= 255.f;

	InitSpotlightTexture("effects/muzzleflash_light");
}

CMuzzleFlashEffect::~CMuzzleFlashEffect()
{
	LightOff();
}

void CMuzzleFlashEffect::UpdateLight(const Vector& vecPos, const Vector& vecDir, const Vector& vecRight, const Vector& vecUp, int nDistance, float flScale)
{
	if (IsOn() == false)
		return;

	ClientFlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion(basisX, basisY, basisZ, state.m_quatOrientation);

	state.m_vecLightOrigin = vecPos;

	state.m_fHorizontalFOVDegrees = 100.f;
	state.m_fVerticalFOVDegrees = 100.f;
	state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
	state.m_fLinearAtten = r_flashlightlinear.GetFloat();
	state.m_fConstantAtten = r_flashlightconstant.GetFloat();
	
	m_vecColor.CopyToArray(state.m_Color);

	state.m_Color[3] = r_flashlightambient.GetFloat();
	state.m_NearZ = r_flashlightnear.GetFloat();
	state.m_FarZ = r_flashlightfar.GetFloat();
	state.m_bEnableShadows = true;
	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;

	UpdateLightProjection(state);
}

class CMuzzleFlashSystem : public CAutoGameSystemPerFrame
{
public:
	CMuzzleFlashSystem() : CAutoGameSystemPerFrame("MuzzleFlashSystem")
	{}

	// Gets called each frame
	virtual void Update(float frametime);
	virtual void LevelShutdownPreEntity();

	void		AddLight(CMuzzleFlashEffect* pLight, ClientEntityHandle_t hEntity, int attachmentIndex);

protected:
	typedef struct {
		CMuzzleFlashEffect* pLight;
		EHANDLE hEntity;
		int iAttachment;
	} muzzleFlash_t;

	CUtlVector<muzzleFlash_t> m_Flashes;
};

extern void FormatViewModelAttachment(Vector& vOrigin, bool bInverse);

void CMuzzleFlashSystem::Update(float frametime)
{
	if (engine->IsInGame() && C_BasePlayer::GetLocalPlayer())
	{
		for (int i = m_Flashes.Count() - 1; i >= 0; i--)
		{
			muzzleFlash_t& flash = m_Flashes[i];
			if (flash.pLight && flash.hEntity && (gpGlobals->curtime - flash.pLight->m_flCreationTime <= 0.066f || gpGlobals->framecount - flash.pLight->m_iCreationFrame <= 2))
			{
				C_BaseEntity* pEnt = flash.hEntity.Get();
				matrix3x4_t matAttachment;
				if (pEnt->GetAttachment(flash.iAttachment, matAttachment))
				{
					Vector vecOrigin, vecForward, vecLeft, vecUp;
					MatrixGetColumn(matAttachment, 0, vecForward);
					MatrixGetColumn(matAttachment, 1, vecLeft);
					MatrixGetColumn(matAttachment, 2, vecUp);
					MatrixGetTranslation(matAttachment, vecOrigin);

					flash.pLight->UpdateLight(vecOrigin, vecForward, -vecLeft, vecUp, 100);
				}
			}
			else
			{
				if (flash.pLight)
				{
					flash.pLight->TurnOff();
					delete flash.pLight;
				}

				m_Flashes.FastRemove(i);
			}
		}
	}
}

void CMuzzleFlashSystem::LevelShutdownPreEntity()
{
	for (int i = m_Flashes.Count() - 1; i >= 0; i--)
	{
		muzzleFlash_t& flash = m_Flashes[i];

		if (flash.pLight)
		{
			flash.pLight->TurnOff();
			delete flash.pLight;
		}
	}

	m_Flashes.Purge();
}

void CMuzzleFlashSystem::AddLight(CMuzzleFlashEffect* pLight, ClientEntityHandle_t hEntity, int attachmentIndex)
{
	if (pLight && hEntity.IsValid())
	{
		muzzleFlash_t flash;
		flash.pLight = pLight;
		flash.hEntity = hEntity;
		flash.iAttachment = attachmentIndex;

		C_BaseEntity* pEnt = flash.hEntity.Get();
		matrix3x4_t matAttachment;
		pEnt->GetAttachment(flash.iAttachment, matAttachment);
		Vector vecOrigin, vecForward, vecLeft, vecUp;
		MatrixGetColumn(matAttachment, 0, vecForward);
		MatrixGetColumn(matAttachment, 1, vecLeft);
		MatrixGetColumn(matAttachment, 2, vecUp);
		MatrixGetTranslation(matAttachment, vecOrigin);

		flash.pLight->TurnOn();
		flash.pLight->UpdateLight(vecOrigin, vecForward, -vecLeft, vecUp, 100);

		m_Flashes.AddToTail(flash);
	}
}

CMuzzleFlashSystem g_MuzzleFlashSystem;

void ProjectMuzzleFlashLight(ClientEntityHandle_t hEntity, int attachmentIndex, ColorRGBExp32 clrColor)
{
	if (hEntity.IsValid())
	{
		CMuzzleFlashEffect* pEffect = new CMuzzleFlashEffect(clrColor);
		g_MuzzleFlashSystem.AddLight(pEffect, hEntity, attachmentIndex);
	}
}

