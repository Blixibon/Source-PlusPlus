#include "cbase.h"
#include "c_ai_basenpc.h"
#include "functionproxy.h"

class C_NPC_Hunter : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_NPC_Hunter, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();
public:
	virtual void ModifyEmitSoundParams(EmitSound_t& params);

	bool	IsVortigauntControlled() { return m_bVortControlled; }
	bool	IsEnraged() { return m_bAngry; }
protected:
	bool m_bVortControlled;
	bool m_bAngry;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Hunter, DT_Hunter, CNPC_Hunter)
RecvPropBool(RECVINFO(m_bAngry)),
RecvPropBool(RECVINFO(m_bVortControlled)),
END_RECV_TABLE();

void C_NPC_Hunter::ModifyEmitSoundParams(EmitSound_t& params)
{
	if (m_bVortControlled)
	{
		if (FStrEq(params.m_pSoundName, "NPC_Hunter.TackleAnnounce"))
			params.m_pSoundName = "NPC_VortiHunter.TackleAnnounce";
		else if (FStrEq(params.m_pSoundName, "NPC_Hunter.FoundEnemy"))
			params.m_pSoundName = "NPC_VortiHunter.FoundEnemy";
		else if (FStrEq(params.m_pSoundName, "NPC_Hunter.FoundEnemyAck"))
			params.m_pSoundName = "NPC_VortiHunter.FoundEnemyAck";
	}

	BaseClass::ModifyEmitSoundParams(params);
}

class CHunterEyeColorProxy : public CResultProxy
{
public:
	virtual void OnBind(void*);
};

void CHunterEyeColorProxy::OnBind(void* pRenderable)
{
	C_BaseEntity *pEnt = BindArgToEntity(pRenderable);
	
	if (pEnt)
	{
		C_NPC_Hunter* pHunter = dynamic_cast<C_NPC_Hunter*> (pEnt);
		if (pHunter)
		{
			static Vector red = Vector(168, 0, 0) / 255;
			static Vector blue = Vector(48, 221, 213) / 255;
			static Vector green = Vector(0, 255, 0) / 255;

			Vector vecColor = blue;
			if (pHunter->IsVortigauntControlled())
			{
				vecColor = green;
			}
			else if (pHunter->IsEnraged())
			{
				vecColor = red;
			}

			m_pResult->SetVecValue(vecColor.Base(), 3);
			return;
		}
	}

	SetFloatResult(0.5); //Gray
}

EXPOSE_INTERFACE(CHunterEyeColorProxy, IMaterialProxy, "HunterEyeColor" IMATERIAL_PROXY_INTERFACE_VERSION);