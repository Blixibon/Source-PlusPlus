#include "shader_data_system.h"
#include "callqueue.h"
#include "materialsystem/imaterialsystem.h"

void CShaderDataExtension::SetUberlightParamsForFlashlightState(int iIndex, const UberlightState_t state)
{
	CMatRenderContextPtr pRenderContext(materials);
	if (pRenderContext->GetCallQueue())
		pRenderContext->GetCallQueue()->QueueCall(this, &CShaderDataExtension::InternalSetUberlightParamsForFlashlightState, iIndex, state);
	else
		InternalSetUberlightParamsForFlashlightState(iIndex, state);
}

void CShaderDataExtension::OnFlashlightStateDestroyed(int iIndex)
{
	CMatRenderContextPtr pRenderContext(materials);
	if (pRenderContext->GetCallQueue())
		pRenderContext->GetCallQueue()->QueueCall(this, &CShaderDataExtension::InternalOnFlashlightStateDestroyed, iIndex);
	else
		InternalOnFlashlightStateDestroyed(iIndex);
}

void CShaderDataExtension::SetDepthTextureFallbackForFlashlightState(int iIndex, ITexture* pTex)
{
	CMatRenderContextPtr pRenderContext(materials);
	if (pRenderContext->GetCallQueue())
		pRenderContext->GetCallQueue()->QueueCall(this, &CShaderDataExtension::InternalSetDepthTextureFallbackForFlashlightState, iIndex, pTex);
	else
		InternalSetDepthTextureFallbackForFlashlightState(iIndex, pTex);
}

const flashlightData_t* CShaderDataExtension::GetState(const FlashlightState_t& flashlightState) const
{
	const int index = flashlightState.m_nShadowQuality >> 16;
	if (index != 0 && m_usedSlots.IsBitSet(index - 1))
	{
		return &m_dataTable[index - 1];
	}
	return NULL;
}

int CShaderDataExtension::GetFlashlightStateIndex(FlashlightState_t& flashlightState)
{
	const int index = flashlightState.m_nShadowQuality >> 16;
	if (index == 0)
	{
		return -1;
	}

	return index;
}

void CShaderDataExtension::InternalSetUberlightParamsForFlashlightState(int iIndex, const UberlightState_t uberlightState)
{
	m_dataTable[iIndex - 1].uber = uberlightState;
	m_usedSlots.Set(iIndex - 1);
}

void CShaderDataExtension::InternalOnFlashlightStateDestroyed(int iIndex)
{
	if (iIndex != 0)
	{
		m_usedSlots.Clear(iIndex - 1);
	}
}

void CShaderDataExtension::InternalSetDepthTextureFallbackForFlashlightState(int iIndex, ITexture* pDepthTex)
{
	m_dataTable[iIndex - 1].pDepth = pDepthTex;
	m_usedSlots.Set(iIndex - 1);
}
