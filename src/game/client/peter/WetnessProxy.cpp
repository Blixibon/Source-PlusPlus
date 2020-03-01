#include "cbase.h"
#include "functionproxy.h"

class CWetnessProxy : public CResultProxy
{
public:
	virtual void OnBind(void* pArg);
};

void CWetnessProxy::OnBind(void* pArg)
{
	C_BaseEntity* pEnt = BindArgToEntity(pArg);
	if (pEnt)
	{
		C_BaseAnimating* pAnim = pEnt->GetBaseAnimating();
		if (pAnim)
		{
			SetFloatResult(pAnim->GetWetness());
			return;
		}
	}

	SetFloatResult(0.f);
}

EXPOSE_INTERFACE(CWetnessProxy, IMaterialProxy, "EntityWetness" IMATERIAL_PROXY_INTERFACE_VERSION);