#pragma once

#include "portal_render_targets.h"

// externs
class IMaterialSystem;
class IMaterialSystemHardwareConfig;

class CLazRenderTargets : public CPortalRenderTargets
{
	// no networked vars
	DECLARE_CLASS_GAMEROOT(CLazRenderTargets, CPortalRenderTargets);
public:
	virtual void InitClientRenderTargets(IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig);
	virtual void ShutdownClientRenderTargets();

	ITexture* GetManhackScreenTexture();
	ITexture* GetCameraScreenTexture();

	ITexture* GetPreciseFullFrameDepthTexture();

protected:
	ITexture* CreateManhackScreenTexture(IMaterialSystem* pMaterialSystem);
	ITexture* CreateCameraScreenTexture(IMaterialSystem* pMaterialSystem);
	ITexture* CreatePreciseFullFrameDepthTexture(IMaterialSystem* pMaterialSystem);

	CTextureReference		m_ManhackScreenTexture;
	CTextureReference		m_CameraScreenTexture;
	CTextureReference		m_PreciseDepthTexture;
};

extern CLazRenderTargets* lazulrendertargets;