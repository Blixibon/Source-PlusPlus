#include "cbase.h"
#include "c_ai_basenpc.h"
#include "functionproxy.h"

class C_NPC_Houndeye : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_Houndeye, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	float GetChargePct();

	float m_flEndEnergyWaveTime;
	float m_flWaveChargeTime;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Houndeye, DT_BMHound, CNPC_BMSHoundeye)
RecvPropTime(RECVINFO(m_flEndEnergyWaveTime)),
RecvPropFloat(RECVINFO(m_flWaveChargeTime)),
END_RECV_TABLE();

float C_NPC_Houndeye::GetChargePct()
{
	if (m_flEndEnergyWaveTime == 0.0f)
		return 0.0f;

	float flStart = m_flEndEnergyWaveTime - m_flWaveChargeTime;

	return RemapValClamped(gpGlobals->curtime, flStart, m_flEndEnergyWaveTime, 0.0f, 1.0f);
}

class CHoundProxy : public CResultProxy
{
public:
	virtual void OnBind(void *);
};

void CHoundProxy::OnBind(void *pArg)
{
	C_BaseEntity *pEnt = BindArgToEntity(pArg);

	if (pEnt)
	{
		C_NPC_Houndeye *pHound = dynamic_cast<C_NPC_Houndeye *> (pEnt);
		if (pHound)
		{
			SetFloatResult(pHound->GetChargePct());
			return;
		}
	}

	SetFloatResult(0.0f);
}

EXPOSE_MATERIAL_PROXY(CHoundProxy, HoundeyeCharge);