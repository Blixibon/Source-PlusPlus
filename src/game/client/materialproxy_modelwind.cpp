#include "cbase.h"
#include "functionproxy.h"
#include "env_wind_shared.h"

class CModelWindProxy : public CResultProxy
{
public:
	void OnBind(void *);
	virtual bool Init(IMaterial *pMaterial, KeyValues *pKeyValues);

private:
	CFloatInput	m_Factor;
};

bool CModelWindProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	if (!CResultProxy::Init(pMaterial, pKeyValues))
		return false;

	if (m_pResult->GetType() != MATERIAL_VAR_TYPE_VECTOR || m_pResult->VectorSize() < 3)
		return false;

	if (!m_Factor.Init(pMaterial, pKeyValues, "scale", 1.f/25.f))
		return false;

	return true;
}

void CModelWindProxy::OnBind(void *pArg)
{
	IClientRenderable *pRend = (IClientRenderable *)pArg;

	if (pRend)
	{
		Vector mins, maxs;
		pRend->GetRenderBounds(mins, maxs);
		Vector vPoint = pRend->GetRenderOrigin() + Vector(0, 0, maxs.z*0.5f);

		Vector vecWind = GetWindspeedAtLocation(vPoint) * m_Factor.GetFloat();

		m_pResult->SetVecValue(vecWind.Base(), 3);
	}
	else
	{
		SetFloatResult(0);
	}
}

//EXPOSE_MATERIAL_PROXY(CLightedMouthProxy, LightedMouth);
EXPOSE_INTERFACE(CModelWindProxy, IMaterialProxy, "ModelWind" IMATERIAL_PROXY_INTERFACE_VERSION);