#pragma once
#include "../materialsystem/stdshaders/IShaderExtension.h"
#include "bitvec.h"
#include "utlmap.h"

class IVEngineClient;

class CShaderDataExtension : public IShaderExtension, public IShaderExtensionInternal
{
public:
	CShaderDataExtension();

	void		LevelLoad(char const* pMapName, IVEngineClient* engine);
	void		LevelClose();

	// Inherited via IShaderExtension
	//virtual void SetUberlightParamsForFlashlightState(int iIndex, const UberlightState_t) override;

	virtual void OnFlashlightStateDestroyed(int iIndex) override;

	virtual void SetDepthTextureFallbackForFlashlightState(int iIndex, ITexture*) override;

	//virtual void SetOrthoDataForFlashlight(int iIndex, bool bOrtho, float flOrthoLeft, float flOrthoRight, float flOrthoTop, float flOrthoBottom) override;

	virtual void SetFlashlightStateExtension(int iIndex, const flashlightDataExt_t &) override;

	// Inherited via IShaderExtensionInternal
	virtual const flashlightData_t* GetState(const FlashlightState_t& flashlightState) const override;
	virtual const cubemapParallaxData_t* GetCubemapParallax(ITexture* pEnvmap) override;
private:
	int		GetFlashlightStateIndex(FlashlightState_t& flashlightState);

	//void InternalSetUberlightParamsForFlashlightState(int iIndex, const UberlightState_t state);
	void InternalOnFlashlightStateDestroyed(int iIndex);
	void InternalSetDepthTextureFallbackForFlashlightState(int iIndex, ITexture* pTex);
	//void InternalSetOrthoDataForFlashlight(int iIndex, bool bOrtho, float flOrthoLeft, float flOrthoRight, float flOrthoTop, float flOrthoBottom);
	void InternalSetFlashlightStateExtension(int iIndex, const flashlightDataExt_t &);

	flashlightData_t m_dataTable[128];
	CBitVec<128> m_usedSlots;

	CUtlVector< cubemapParallaxData_t > m_ParallaxData;
	CUtlMap<ITexture*, int> m_ParallaxTextureCache;
};