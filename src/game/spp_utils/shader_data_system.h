#pragma once
#include "../materialsystem/stdshaders/IShaderExtension.h"
#include "bitvec.h"

class CShaderDataExtension : public IShaderExtension, public IShaderExtensionInternal
{
public:
	// Inherited via IShaderExtension
	virtual void SetUberlightParamsForFlashlightState(int iIndex, const UberlightState_t) override;

	virtual void OnFlashlightStateDestroyed(int iIndex) override;

	virtual void SetDepthTextureFallbackForFlashlightState(int iIndex, ITexture*) override;

	virtual void SetOrthoDataForFlashlight(int iIndex, bool bOrtho, float flOrthoLeft, float flOrthoRight, float flOrthoTop, float flOrthoBottom) override;

	// Inherited via IShaderExtensionInternal
	virtual const flashlightData_t* GetState(const FlashlightState_t& flashlightState) const override;
private:
	int		GetFlashlightStateIndex(FlashlightState_t& flashlightState);

	void InternalSetUberlightParamsForFlashlightState(int iIndex, const UberlightState_t state);
	void InternalOnFlashlightStateDestroyed(int iIndex);
	void InternalSetDepthTextureFallbackForFlashlightState(int iIndex, ITexture* pTex);
	void InternalSetOrthoDataForFlashlight(int iIndex, bool bOrtho, float flOrthoLeft, float flOrthoRight, float flOrthoTop, float flOrthoBottom);

	flashlightData_t m_dataTable[128];
	CBitVec<128> m_usedSlots;
};