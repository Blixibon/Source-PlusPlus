#include "shader_data_system.h"
#include "callqueue.h"
#include "materialsystem/imaterialsystem.h"
#include "cdll_int.h"
#include "utlbuffer.h"
#include "gamebspfile.h"
#include "materialsystem/itexture.h"

CShaderDataExtension::CShaderDataExtension() : m_ParallaxTextureCache(DefLessFunc(ITexture *))
{
}

void CShaderDataExtension::LevelLoad(char const* pMapName, IVEngineClient* engine)
{
	int iVersion = engine->GameLumpVersion(GAMELUMP_CUBEMAP_PARALLAX);
	if (iVersion == GAMELUMP_CUBEMAP_PARALLAX_VERSION)
	{
		size_t nSize = engine->GameLumpSize(GAMELUMP_CUBEMAP_PARALLAX);
		if (nSize > 0)
		{
			int iCount = nSize / sizeof(CubemapParallaxLump_t);
			CubemapParallaxLump_t* pData = new CubemapParallaxLump_t[iCount];
			if (engine->LoadGameLump(GAMELUMP_CUBEMAP_PARALLAX, pData, nSize))
			{
				for (int i = 0; i < iCount; i++)
				{
					auto& data = m_ParallaxData[m_ParallaxData.AddToHead()];
					data.matOBB = pData[i].parallaxOBB;
					data.vecOrigin.Init(pData[i].x, pData[i].y, pData[i].z);
				}
			}

			delete[] pData;
		}
	}
}

void CShaderDataExtension::LevelClose()
{
	m_ParallaxData.Purge();
	m_ParallaxTextureCache.Purge();
}

const cubemapParallaxData_t* CShaderDataExtension::GetCubemapParallax(ITexture* pEnvmap)
{
	if (!m_ParallaxData.Count())
		return nullptr;

	int iIndex = -1;
	if (!IsErrorTexture(pEnvmap))
	{
		unsigned short sMap = m_ParallaxTextureCache.Find(pEnvmap);
		if (m_ParallaxTextureCache.IsValidIndex(sMap))
		{
			iIndex = m_ParallaxTextureCache.Element(sMap);
		}
		else
		{
			int x, y, z;
			const char* pszTextureName = V_UnqualifiedFileName(pEnvmap->GetName());
			if (sscanf(pszTextureName, "c%d_%d_%d", &x, &y, &z) == 3)
			{
				Vector vecTest(x, y, z);
				for (int i = 0; i < m_ParallaxData.Count(); i++)
				{
					if (VectorsAreEqual(vecTest, m_ParallaxData[i].vecOrigin, FLT_EPSILON))
					{
						iIndex = i;
						break;
					}
				}
			}

			m_ParallaxTextureCache.Insert(pEnvmap, iIndex);
		}
	}

	if (iIndex < 0)
		return nullptr;

	return &m_ParallaxData[iIndex];
}

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

void CShaderDataExtension::SetOrthoDataForFlashlight(int iIndex, bool bOrtho, float flOrthoLeft, float flOrthoRight, float flOrthoTop, float flOrthoBottom)
{
	CMatRenderContextPtr pRenderContext(materials);
	if (pRenderContext->GetCallQueue())
		pRenderContext->GetCallQueue()->QueueCall(this, &CShaderDataExtension::InternalSetOrthoDataForFlashlight, iIndex, bOrtho, flOrthoLeft, flOrthoRight, flOrthoTop, flOrthoBottom);
	else
		InternalSetOrthoDataForFlashlight(iIndex, bOrtho, flOrthoLeft, flOrthoRight, flOrthoTop, flOrthoBottom);
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

void CShaderDataExtension::InternalSetOrthoDataForFlashlight(int iIndex, bool bOrtho, float flOrthoLeft, float flOrthoRight, float flOrthoTop, float flOrthoBottom)
{
	m_usedSlots.Set(iIndex - 1);
	flashlightData_t &data = m_dataTable[iIndex - 1];
	data.m_bOrtho = bOrtho;
	data.m_fOrthoLeft = flOrthoLeft;
	data.m_fOrthoRight = flOrthoRight;
	data.m_fOrthoTop = flOrthoTop;
	data.m_fOrthoBottom = flOrthoBottom;
}