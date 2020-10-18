#include "cbase.h"
#include "c_ai_basenpc.h"
#include "flashlighteffect.h"
#include "dlight.h"
#include "r_efx.h"

//-----------------------------------------------------------------------------
// Attachment points
//-----------------------------------------------------------------------------
#define SCANNER_ATTACHMENT_LIGHT	"light"

ConVar scanner_light_dist("cl_scanner_projected_light_dist", "5", FCVAR_CHEAT);

#define LIGHT_DIST scanner_light_dist.GetFloat()

class C_NPC_CScanner : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_CScanner, C_AI_BaseNPC);
	DECLARE_NETWORKCLASS();

	C_NPC_CScanner()
	{
		m_pSpotLight = nullptr;
	}
	~C_NPC_CScanner();

	void AddEntity(void);

	virtual bool	GetShadowCastDistance(float *pDist, ShadowType_t shadowType) const;

	EHANDLE m_hSpotlightTarget;
protected:
	CSpotlightEffect *m_pSpotLight;
	dlight_t *m_pDLight;
	//bool m_bClientSpotlight;
	//Vector m_NetSpotlightDir;
	
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_CScanner, DT_NPC_CScanner, CNPC_CScanner)
RecvPropEHandle(RECVINFO(m_hSpotlightTarget)),
END_NETWORK_TABLE()

C_NPC_CScanner::~C_NPC_CScanner()
{
	delete m_pSpotLight;
}

bool C_NPC_CScanner::GetShadowCastDistance(float *pDist, ShadowType_t shadowType) const
{
	if (!BaseClass::GetShadowCastDistance(pDist, shadowType) && shadowType == SHADOWS_RENDER_TO_TEXTURE)
	{
		float flTemp = *pDist;
		flTemp *= 4.f;
		*pDist = Min(flTemp, 1024.f);
	}

	return true;
}

void C_NPC_CScanner::AddEntity()
{
	BaseClass::AddEntity();


	if (m_hSpotlightTarget.Get() != NULL)
	{
		if (m_pSpotLight == nullptr)
		{
			// Turned on the headlight; create it.
			m_pSpotLight = new CSpotlightEffect(true, 0.8f);

			if (m_pSpotLight == nullptr)
				return;

			m_pSpotLight->TurnOn();

			m_pDLight = effects->CL_AllocElight(index);
			m_pDLight->radius = 40;
			m_pDLight->color.r = 255;
			m_pDLight->color.g = 255;
			m_pDLight->color.b = 255;
			m_pDLight->color.exponent = 4;
		}

		m_pDLight->die = gpGlobals->curtime + 9999.0f;

		// The headlight is emitted from an attachment point so that it can move
		// as we turn the handlebars.
		int nHeadlightIndex = LookupAttachment(SCANNER_ATTACHMENT_LIGHT);

		Vector vecLightPos;
		QAngle angLightDir;
		GetAttachment(nHeadlightIndex, vecLightPos, angLightDir);

		Vector vecLightDir, vecLightRight, vecLightUp, vecScannerDir;
		AngleVectors(angLightDir, &vecLightDir);
		AngleVectors(GetAbsAngles(), &vecScannerDir);

		m_pDLight->origin = vecLightPos;
		m_pDLight->m_Direction = vecScannerDir;

		vecLightPos += ( vecScannerDir * LIGHT_DIST);

		vecLightDir = m_hSpotlightTarget->GetLocalOrigin() - vecLightPos;
		VectorNormalize(vecLightDir);

		VectorVectors(vecLightDir, vecLightRight, vecLightUp);

		// Update the light with the new position and direction.		
		m_pSpotLight->UpdateLight(vecLightPos, vecLightDir, vecLightRight, vecLightUp, 1000, 1.0f);
	}
	else if (m_pSpotLight)
	{
		// Turned off the headlight; delete it.
		delete m_pSpotLight;
		m_pSpotLight = NULL;
		m_pDLight->radius = 0;
		m_pDLight->die = gpGlobals->curtime + 0.01f;
	}

}

