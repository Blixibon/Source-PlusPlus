//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef ICLIENTSHADOWMGR_H
#define ICLIENTSHADOWMGR_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "icliententityinternal.h"
#include "engine/ishadowmgr.h"
#include "ivrenderview.h"
#include "toolframework/itoolentity.h"
#include "materialsystem/imaterialsystem.h"
#include "../materialsystem/stdshaders/IShaderExtension.h"

class CNewViewSetup;

//-----------------------------------------------------------------------------
// Client-side flashlight state
//-----------------------------------------------------------------------------
struct ClientFlashlightState_t : public FlashlightState_t
{
	ClientFlashlightState_t() : FlashlightState_t()
	{
		m_bVolumetric = false;
		m_flNoiseStrength = 0.8f;
		m_nNumPlanes = 64;
		m_flPlaneOffset = 0.0f;
		m_flFlashlightTime = 0.0f;
		m_flVolumetricIntensity = 1.0f;

		m_ExtData.m_bOrtho = false;
		m_ExtData.m_fOrthoLeft = -1.0f;
		m_ExtData.m_fOrthoRight = 1.0f;
		m_ExtData.m_fOrthoTop = -1.0f;
		m_ExtData.m_fOrthoBottom = 1.0f;

		m_bShadowHighRes = false;
		m_bGlobalLight = false;

		m_ExtData.m_fBrightnessScale = 1.0f;
	}

	void CopyFromOld(const FlashlightState_t& other)
	{
		*static_cast<FlashlightState_t*>(this) = other;
	}

	bool m_bVolumetric;
	float m_flNoiseStrength;
	float m_flFlashlightTime;
	int m_nNumPlanes;
	float m_flPlaneOffset;
	float m_flVolumetricIntensity;

	bool m_bShadowHighRes;
	bool m_bGlobalLight;

	flashlightDataExt_t m_ExtData;

	IMPLEMENT_OPERATOR_EQUAL(ClientFlashlightState_t);
};

//-----------------------------------------------------------------------------
// Handles to a client shadow
//-----------------------------------------------------------------------------
enum ShadowReceiver_t
{
	SHADOW_RECEIVER_BRUSH_MODEL = 0,
	SHADOW_RECEIVER_STATIC_PROP,
	SHADOW_RECEIVER_STUDIO_MODEL,
};


//-----------------------------------------------------------------------------
// The class responsible for dealing with shadows on the client side
//-----------------------------------------------------------------------------
abstract_class IClientShadowMgr : public IGameSystemPerFrame
{
public:
	// Create, destroy shadows
	virtual ClientShadowHandle_t CreateShadow( ClientEntityHandle_t entity, int flags ) = 0;
	virtual void DestroyShadow( ClientShadowHandle_t handle ) = 0;

	// Create flashlight.
	// FLASHLIGHTFIXME: need to rename all of the shadow stuff to projectedtexture and have flashlights and shadows as instances.
	virtual ClientShadowHandle_t CreateFlashlight( const ClientFlashlightState_t&lightState ) = 0;
	virtual void UpdateFlashlightState( ClientShadowHandle_t shadowHandle, const ClientFlashlightState_t&lightState ) = 0;
	virtual void DestroyFlashlight( ClientShadowHandle_t handle ) = 0;
	
	// Indicate that the shadow should be recomputed due to a change in
	// the client entity
	virtual void UpdateProjectedTexture( ClientShadowHandle_t handle, bool force = false ) = 0;

	// Used to cause shadows to be re-projected against the world.
	virtual void AddToDirtyShadowList( ClientShadowHandle_t handle, bool force = false ) = 0;
	virtual void AddToDirtyShadowList( IClientRenderable *pRenderable, bool force = false ) = 0;

	// deals with shadows being added to shadow receivers
	virtual void AddShadowToReceiver( ClientShadowHandle_t handle,
		IClientRenderable* pRenderable, ShadowReceiver_t type ) = 0;

	virtual void RemoveAllShadowsFromReceiver( 
		IClientRenderable* pRenderable, ShadowReceiver_t type ) = 0;

	// Re-renders all shadow textures for shadow casters that lie in the leaf list
	virtual void ComputeShadowTextures( const CNewViewSetup&view, int leafCount, LeafIndex_t* pLeafList ) = 0;

	// Frees shadow depth textures for use in subsequent view/frame
	virtual void UnlockAllShadowDepthTextures() = 0;
	
	// Renders the shadow texture to screen...
	virtual void RenderShadowTexture( int w, int h ) = 0;

	// Sets the shadow direction + color
	virtual void SetShadowDirection( const Vector& dir ) = 0;
	virtual const Vector &GetShadowDirection() const = 0;
	
	virtual void SetShadowColor( unsigned char r, unsigned char g, unsigned char b ) = 0;
	virtual void SetShadowDistance( float flMaxDistance ) = 0;
	virtual void SetShadowBlobbyCutoffArea( float flMinArea ) = 0;
	virtual void SetFalloffBias( ClientShadowHandle_t handle, unsigned char ucBias ) = 0;

	// Marks the render-to-texture shadow as needing to be re-rendered
	virtual void MarkRenderToTextureShadowDirty( ClientShadowHandle_t handle ) = 0;

	// Advance the frame
	virtual void AdvanceFrame() = 0;

	// Set and clear flashlight target renderable
	virtual void SetFlashlightTarget( ClientShadowHandle_t shadowHandle, EHANDLE targetEntity ) = 0;

	// Set flashlight light world flag
	virtual void SetFlashlightLightWorld( ClientShadowHandle_t shadowHandle, bool bLightWorld ) = 0;

	virtual void SetShadowsDisabled( bool bDisabled ) = 0;

	virtual void ComputeShadowDepthTextures( const CNewViewSetup &pView ) = 0;

	virtual void GetFrustumExtents( ClientShadowHandle_t handle, Vector &vecMin, Vector &vecMax ) = 0;

	virtual ShadowType_t GetActualShadowCastType( ClientShadowHandle_t handle ) const = 0;
	virtual ShadowHandle_t GetShadowHandle( ClientShadowHandle_t clienthandle ) = 0;
	virtual int GetNumShadowDepthtextures() = 0;
	virtual int GetMaxShadowDepthtextures() = 0;
	virtual CTextureReference GetShadowDepthTex( int num ) = 0;
	
	virtual ShadowHandle_t GetShadowDepthHandle( int num ) = 0;
	virtual ShadowHandle_t GetActiveDepthTextureHandle() = 0;

	// OBSOLETE
	//virtual void UpdateUberlightState( FlashlightState_t& handle, const UberlightState_t& uberlightState ) = 0;

	virtual void SetShadowFromWorldLightsEnabled(bool bEnabled) = 0;
	virtual bool IsShadowingFromWorldLights() const = 0;

	// Flashlight access
	virtual bool GetFlashlightByIndex(int iIndex, ClientFlashlightState_t* pState, VMatrix* pWorldToLight, ITexture** ppDepthTexture, ShadowHandle_t *pEngineHandle) = 0;

	virtual bool VolumetricsAvailable() = 0;
};


//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
extern IClientShadowMgr* g_pClientShadowMgr;

#endif // ICLIENTSHADOWMGR_H
