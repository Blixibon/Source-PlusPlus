#include "cbase.h"
#include "c_ai_spotlight.h"
#include "flashlighteffect.h"

BEGIN_RECV_TABLE_NOBASE(C_AI_Spotlight, DT_AISpotlight)
RecvPropEHandle(RECVINFO(m_hSpotlightTarget)),
RecvPropInt(RECVINFO(m_nSpotlightAttachment)),
RecvPropInt(RECVINFO(m_nFlags)),
END_RECV_TABLE()

C_AI_Spotlight::~C_AI_Spotlight()
{
	if (m_pSpotLight != nullptr)
		delete m_pSpotLight;
}

void C_AI_Spotlight::ClientUpdate(C_BaseAnimating *pOwner)
{
	if (pOwner && (m_nFlags & AI_SPOTLIGHT_ENABLE_PROJECTED) &&m_hSpotlightTarget.Get() != NULL)
	{
		if (!m_pSpotLight)
		{
			// Turned on the headlight; create it.
			m_pSpotLight = new CSpotlightEffect();

			if (!m_pSpotLight)
				return;

			m_pSpotLight->TurnOn();


		}

		//m_pDLight->die = gpGlobals->curtime + 9999.0f;

		

		Vector vecLightPos;
		QAngle angLightDir;
		pOwner->GetAttachment(m_nSpotlightAttachment, vecLightPos, angLightDir);

		Vector vecLightDir, vecLightRight, vecLightUp;
		AngleVectors(angLightDir, &vecLightDir);
		//AngleVectors(GetAbsAngles(), &vecScannerDir);

		/*m_pDLight->origin = vecLightPos;
		m_pDLight->m_Direction = vecScannerDir;

		vecLightPos += (vecScannerDir * LIGHT_DIST);*/

		vecLightDir = m_hSpotlightTarget->GetLocalOrigin() - vecLightPos;
		VectorNormalize(vecLightDir);

		VectorVectors(vecLightDir, vecLightRight, vecLightUp);

		// Update the light with the new position and direction.		
		m_pSpotLight->UpdateLight(vecLightPos, vecLightDir, vecLightRight, vecLightUp, 1000);
	}
	else if (m_pSpotLight)
	{
		// Turned off the headlight; delete it.
		delete m_pSpotLight;
		m_pSpotLight = NULL;
	}
}