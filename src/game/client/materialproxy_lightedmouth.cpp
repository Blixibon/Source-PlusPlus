#include "cbase.h"
#include "functionproxy.h"
#include "c_ai_basenpc.h"
#ifdef PORTAL
#include "C_PortalGhostRenderable.h"
#endif // PORTAL


class CLightedMouthProxy : public CResultProxy
{
public:
	void OnBind(void *);
};

void CLightedMouthProxy::OnBind(void *pArg)
{
	IClientRenderable* pRend = (IClientRenderable*)pArg;

	C_BaseEntity* pEnt = nullptr;
#ifdef PORTAL
	C_PortalGhostRenderable* pGhostAnim = dynamic_cast<C_PortalGhostRenderable*> (pRend);
	if (pGhostAnim)
		pEnt = pGhostAnim->m_pGhostedRenderable;
	else
#endif
		pEnt = BindArgToEntity(pRend);

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

EXPOSE_MATERIAL_PROXY(CLightedMouthProxy, LightedMouth);
//EXPOSE_INTERFACE(CLightedMouthProxy, IMaterialProxy, "LightedMouth" IMATERIAL_PROXY_INTERFACE_VERSION);