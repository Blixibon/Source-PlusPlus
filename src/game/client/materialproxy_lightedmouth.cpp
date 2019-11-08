#include "cbase.h"
#include "functionproxy.h"
#include "c_ai_basenpc.h"

class CLightedMouthProxy : public CResultProxy
{
public:
	void OnBind(void *);
};

void CLightedMouthProxy::OnBind(void *pArg)
{
	C_BaseEntity *pEnt = BindArgToEntity(pArg);

	if (pEnt && pEnt->GetBaseAnimating())
	{
		float value = 0.0f;

		pEnt->GetBaseAnimating()->GetMouth()->ActivateEnvelope();
		value = pEnt->GetBaseAnimating()->GetMouthOpenPct();
		
		value = (1.0 - value);

		SetFloatResult(value);
	}
	else
	{
		SetFloatResult(0);
	}
}

//EXPOSE_MATERIAL_PROXY(CLightedMouthProxy, LightedMouth);
EXPOSE_INTERFACE(CLightedMouthProxy, IMaterialProxy, "LightedMouth" IMATERIAL_PROXY_INTERFACE_VERSION);