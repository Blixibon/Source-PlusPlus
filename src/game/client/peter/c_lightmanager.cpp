#include "cbase.h"
#include "c_lightmanager.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "view.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"

bool CDLightManager::Init()
{
	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char* fileName = "scripts/lightscripts/*.txt";
	char szFullFileName[MAX_PATH];
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "GAME", &findHandle);
	while (fileName)
	{
		//char name[32];
		//V_StripExtension(fileName, name, 32);
		Q_snprintf(szFullFileName, sizeof(szFullFileName), "scripts/lightscripts/%s", fileName);
		KeyValues* pKVFile = new KeyValues("LightScript");
		if (pKVFile->LoadFromFile(filesystem, szFullFileName))
		{
			for (KeyValues* pkvEvent = pKVFile->GetFirstTrueSubKey(); pkvEvent != NULL; pkvEvent = pkvEvent->GetNextTrueSubKey())
			{
				lighttype_t light;
				light.color = pkvEvent->GetColor("color");
				light.flRadius = pkvEvent->GetFloat("radius", 128.f);
				light.iLightStyle = pkvEvent->GetInt("style", 0);
				light.flSustainTime = pkvEvent->GetFloat("time_sustain");
				light.flDecayTime = pkvEvent->GetFloat("time_decay");
				
				m_LightTypes.Insert(pkvEvent->GetName(), light);
			}
		}

		pKVFile->deleteThis();
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	return true;
}

int SortActiveLights(const CSmartPtr<CManagedLight>*pLeft, const CSmartPtr<CManagedLight>*pRight)
{
	const Vector &vecMain = MainViewOrigin();
	float flLeft = vecMain.DistToSqr(pLeft->GetObject()->GetOrigin());
	float flRight = vecMain.DistToSqr(pRight->GetObject()->GetOrigin());
	
	return (flLeft < flRight) ? -1 : 1;
}

void CDLightManager::Update(float frametime)
{
	for (CSmartPtr<CManagedLight> pLight : m_ActiveLights)
	{
		pLight->Update();
	}

	for (int i = m_ActiveLights.Count() - 1; i >= 0; i--)
	{
		if (m_ActiveLights[i]->IsDead())
			m_ActiveLights.Remove(i);
	}

	m_ActiveLights.Sort(SortActiveLights);

	int i = 0;
	for (i = 0; i < m_ActiveLights.Count() && i < ARRAYSIZE(m_pDLights); i++)
	{
		if (!m_pDLights[i] || m_pDLights[i]->die <= gpGlobals->curtime)
			m_pDLights[i] = effects->CL_AllocDlight(LIGHT_INDEX_MANAGED_LIGHT + i);

		m_ActiveLights[i]->CopyIntoLight(m_pDLights[i]);
		//m_pDLights[i]->key = LIGHT_INDEX_MANAGED_LIGHT + i;
	}

	for (int j = i; j < m_ActiveLights.Count(); j++)
	{
		m_ActiveLights[i]->CopyIntoLight(nullptr);
	}

	for (; i < ARRAYSIZE(m_pDLights); i++)
	{
		if (m_pDLights[i])
		{
			m_pDLights[i]->die = gpGlobals->curtime;
			m_pDLights[i] = nullptr;
		}
	}
}

CManagedLight* CDLightManager::CreateLight(const char* pszLightType, const Vector* pOrigin)
{
	int iLightType = m_LightTypes.Find(pszLightType);
	if (m_LightTypes.IsValidIndex(iLightType))
	{
		auto lightdef = m_LightTypes.Element(iLightType);
		CSmartPtr< CManagedLight > pNewLight = new CManagedLight;

		//V_memset(&pNewLight->m_DLightData, 0, sizeof(dlight_t));
		pNewLight->m_DeathTimes.range = lightdef.flDecayTime;
		pNewLight->m_DLightData.m_DataCopy.style = lightdef.iLightStyle;
		pNewLight->m_DLightData.m_DataCopy.flags = 0;
		pNewLight->m_DLightData.m_DataCopy.radius = pNewLight->m_flOriginalRadius = lightdef.flRadius;
		if (lightdef.flSustainTime > 0.f)
			pNewLight->m_DeathTimes.start = gpGlobals->curtime + lightdef.flSustainTime;

		Vector vecColor = Vector(lightdef.color.r(), lightdef.color.g(), lightdef.color.b());
		vecColor *= lightdef.color.a() / 255.f;

		VectorToColorRGBExp32(vecColor, pNewLight->m_DLightData.m_DataCopy.color);
		if (pOrigin)
			pNewLight->UpdateOrigin(*pOrigin);

		pNewLight->SetRadiusScale(1.0f);
		m_ActiveLights.AddToTail(pNewLight);

		return pNewLight.GetObject();
	}

	return nullptr;
}

void CDLightManager::CreateAutoFollowLight(const CEffectData& data)
{
	if (data.GetEntity())
	{
		CAutoFollowLight *pNewLight = new CAutoFollowLight;

		//V_memset(&pNewLight->m_DLightData, 0, sizeof(dlight_t));
		pNewLight->m_DeathTimes.range = data.m_flScale;
		pNewLight->m_DLightData.m_DataCopy.flags = 0;
		pNewLight->m_DLightData.m_DataCopy.radius = pNewLight->m_flOriginalRadius = data.m_flRadius;
		pNewLight->m_DeathTimes.start = gpGlobals->curtime + data.m_flMagnitude;

		union ColorInt
		{
			int m_iInt;
			ColorRGBExp32 m_Color;
		} color2Int;

		color2Int.m_iInt = data.m_nMaterial;
		pNewLight->m_DLightData.m_DataCopy.color = color2Int.m_Color;

		pNewLight->m_hFollowEntity = data.m_hEntity;
		pNewLight->m_iAttachmentIndex = data.m_nAttachmentIndex;

		pNewLight->SetRadiusScale(1.0f);
		m_ActiveLights.AddToTail(pNewLight);
	}
}

void CDLightManager::CreateAutoFollowLight(C_BaseEntity* pEntity, int iAttachmentIndex, int r, int g, int b, int exponent, float radius, float time, float decay)
{
	if (pEntity)
	{
		CAutoFollowLight* pNewLight = new CAutoFollowLight;

		//V_memset(&pNewLight->m_DLightData, 0, sizeof(dlight_t));
		pNewLight->m_DeathTimes.range = time;
		pNewLight->m_DLightData.m_DataCopy.flags = 0;
		pNewLight->m_DLightData.m_DataCopy.radius = pNewLight->m_flOriginalRadius = radius;
		pNewLight->m_DeathTimes.start = gpGlobals->curtime;

		pNewLight->m_DLightData.m_DataCopy.color.r = r;
		pNewLight->m_DLightData.m_DataCopy.color.g = g;
		pNewLight->m_DLightData.m_DataCopy.color.b = b;
		pNewLight->m_DLightData.m_DataCopy.color.exponent = exponent;

		pNewLight->m_hFollowEntity.Set(pEntity);
		pNewLight->m_iAttachmentIndex = iAttachmentIndex;

		pNewLight->SetRadiusScale(1.0f);
		m_ActiveLights.AddToTail(pNewLight);
	}
}

void CManagedLight::UpdateOrigin(Vector vecOrigin)
{
	m_DLightData.m_DataCopy.origin = vecOrigin;
}

void CManagedLight::Release()
{
	m_iRefCount--;
	if (m_iRefCount <= 0)
		delete this;
}

void CManagedLight::CopyIntoLight(dlight_t* pLight)
{
	m_DLightData.Update(pLight);
}

void CManagedLight::StartDecay()
{
	m_DeathTimes.start = gpGlobals->curtime;
}

void CManagedLight::Update()
{
	if (m_DeathTimes.start > 0 && gpGlobals->curtime >= m_DeathTimes.start)
	{
		m_DLightData.m_DataCopy.die = m_DeathTimes.start + m_DeathTimes.range;
		m_DLightData.m_DataCopy.decay = (m_flOriginalRadius * m_flRadiusScale) / m_DeathTimes.range;
	}
	else
	{
		m_DLightData.m_DataCopy.die = FLT_MAX;
		m_DLightData.m_DataCopy.decay = 0.f;
		m_DLightData.m_DataCopy.radius = (m_flOriginalRadius * m_flRadiusScale);
	}
}

CDLightManager g_LightManager;

CDLightManager* LightManager()
{
	return &g_LightManager;
}

CManagedLight::CLightData::CLightData()
{
	m_pLight = nullptr;
	memset(&m_DataCopy, 0, sizeof(dlight_t));
}

void CManagedLight::CLightData::Update(dlight_t* pLight)
{
	if (pLight != m_pLight)
	{
		m_pLight = pLight;
		if (pLight)
		{
			m_pLight->color = m_DataCopy.color;
			m_flLastRadius = m_pLight->radius = m_DataCopy.radius;
			m_pLight->style = m_DataCopy.style;
			m_pLight->origin = m_DataCopy.origin;
			m_pLight->die = m_DataCopy.die;
			m_DataCopy.key = m_pLight->key;
		}
	}
	else if (m_pLight)
	{
		m_pLight->origin = m_DataCopy.origin;
		m_pLight->die = m_DataCopy.die;
		if (m_DataCopy.decay < m_pLight->decay || m_flLastRadius != m_DataCopy.radius)
			m_flLastRadius = m_pLight->radius = m_DataCopy.radius;
		m_pLight->decay = m_DataCopy.decay;
	}
}

void CAutoFollowLight::Update()
{
	CManagedLight::Update();

	if (m_hFollowEntity.Get())
	{
		Vector vecOrigin = m_hFollowEntity->GetAbsOrigin();
		if (m_iAttachmentIndex > 0)
			m_hFollowEntity->GetAttachment(m_iAttachmentIndex, vecOrigin);

		UpdateOrigin(vecOrigin);
	}
}

void CreateFollowLightCallback(const CEffectData& data)
{
	g_LightManager.CreateAutoFollowLight(data);
}

DECLARE_CLIENT_EFFECT("CreateFollowLight", CreateFollowLightCallback);