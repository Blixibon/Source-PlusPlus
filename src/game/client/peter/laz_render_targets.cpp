#include "cbase.h"
#include "laz_render_targets.h"

#define MANHACK_SCREEN_MATERIAL "vgui/screens/manhack_screen"
#define CAMERA_SCREEN_MATERIAL "vgui/screens/camera_screen"

ITexture* CLazRenderTargets::CreateManhackScreenTexture(IMaterialSystem* pMaterialSystem)
{
	//	DevMsg("Creating Scope Render Target: _rt_Scope\n");
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		MANHACK_SCREEN_MATERIAL,
		128, 64, RT_SIZE_OFFSCREEN,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);

}

ITexture* CLazRenderTargets::CreateCameraScreenTexture(IMaterialSystem* pMaterialSystem)
{
	//DevMsg("Creating Camera Screen Render Target: _rt_Scope\n");
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		CAMERA_SCREEN_MATERIAL,
		128, 64, RT_SIZE_OFFSCREEN,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR);

}

ITexture* CLazRenderTargets::CreatePreciseFullFrameDepthTexture(IMaterialSystem* pMaterialSystem)
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_PreciseFullFrameDepth",
		0, 0, RT_SIZE_FULL_FRAME_BUFFER,
		IMAGE_FORMAT_R32F,
		MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		0);
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CLazRenderTargets::InitClientRenderTargets(IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig)
{
	m_ManhackScreenTexture.Init(CreateManhackScreenTexture(pMaterialSystem));
	m_CameraScreenTexture.Init(CreateCameraScreenTexture(pMaterialSystem));
	m_PreciseDepthTexture.Init(CreatePreciseFullFrameDepthTexture(pMaterialSystem));

	// Water effects & camera from the base class (standard HL2 targets) 
	BaseClass::InitClientRenderTargets(pMaterialSystem, pHardwareConfig);	//TERO: not sure if we need this
}

//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CLazRenderTargets::ShutdownClientRenderTargets()
{
	m_ManhackScreenTexture.Shutdown();
	m_CameraScreenTexture.Shutdown();
	m_PreciseDepthTexture.Shutdown();

	// Clean up standard HL2 RTs (camera and water) 
	BaseClass::ShutdownClientRenderTargets();
}

ITexture* CLazRenderTargets::GetManhackScreenTexture()
{
	return m_ManhackScreenTexture;
}

ITexture* CLazRenderTargets::GetCameraScreenTexture()
{
	return m_CameraScreenTexture;
}

ITexture* CLazRenderTargets::GetPreciseFullFrameDepthTexture()
{
	return m_PreciseDepthTexture;
}

static CLazRenderTargets g_LazRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CLazRenderTargets, IClientRenderTargets, CLIENTRENDERTARGETS_INTERFACE_VERSION, g_LazRenderTargets);
CLazRenderTargets* lazulrendertargets = &g_LazRenderTargets;
CPortalRenderTargets* portalrendertargets = lazulrendertargets;
IClientRenderTargets* g_pClientRenderTargets = lazulrendertargets;