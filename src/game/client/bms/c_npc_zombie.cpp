#include "cbase.h"
#include "c_ai_basenpc.h"
#include "dlight.h"
#include "iefx.h"
#include "flashlighteffect.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "peter/c_lightmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_ZombieGuard : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_ZombieGuard, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t type);
	virtual void ClientThink();

	~C_NPC_ZombieGuard()
	{
		if (m_pLight.IsValid() && !m_bStartedDecay)
			m_pLight->StartDecay();
	}
protected:
	CNewParticleEffect *m_pDroolFX;
	CSmartPtr<CManagedLight> m_pLight;
	bool	m_bStartedDecay;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_ZombieGuard, DT_ZombieGuard, CNPC_ZombieGuard)

END_RECV_TABLE()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_ZombieGuard::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (type == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);

		// get my particles
		CParticleProperty * pProp = ParticleProp();

		m_pDroolFX = pProp->Create("npc_zombie_security_mouth_fx", PATTACH_ABSORIGIN_FOLLOW);
		if (m_pDroolFX)
			pProp->AddControlPoint(m_pDroolFX, 1, this, PATTACH_POINT_FOLLOW, "mouth", Vector(3, 0, -1));
	}

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_ZombieGuard::ClientThink()
{
	//37 255 63 255
	//142 251 0 255

	// update the dlight. (always done because clienthink only exists for cavernguard)
	if (!m_pLight.IsValid())
	{
		m_pLight = LightManager()->CreateLight("ZombieMouth");
	}

	if (m_pLight.IsValid())
	{
		Vector vecMouth;
		GetAttachment("mouth", vecMouth);

		if (m_pRagdollEnt != NULL)
		{
			m_pRagdollEnt->GetAttachment("mouth", vecMouth);
			ParticleProp()->AddControlPoint(m_pDroolFX, 1, m_pRagdollEnt, PATTACH_POINT_FOLLOW, "mouth", Vector(3, 0, -1));

			if (!m_bStartedDecay)
			{
				m_pLight->StartDecay();
				m_bStartedDecay = true;
			}
		}

		m_pLight->UpdateOrigin(vecMouth);
	}

	BaseClass::ClientThink();
}

class C_NPC_ZombieHEV : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_ZombieHEV, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t type);
	virtual void ClientThink();
	virtual CStudioHdr* OnNewModel();
	virtual void UpdateOnRemove();


protected:
	CSpotlightEffect* m_pFlashlight;
	CBeam* m_pBeam;
	dlight_t* m_pELight;
	int		m_iAttachment;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_ZombieHEV, DT_ZombieHEV, CNPC_ZombieHEV)

END_RECV_TABLE()

void C_NPC_ZombieHEV::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (type == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
}

void C_NPC_ZombieHEV::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
	}

	if (m_pFlashlight)
	{
		// Turned off the headlight; delete it.
		delete m_pFlashlight;
		m_pFlashlight = NULL;
	}

	if (m_pBeam)
	{
		m_pBeam->Remove();
		m_pBeam = NULL;
	}
}

void C_NPC_ZombieHEV::ClientThink()
{
	bool bLight = true;
	float flScale = 1.0f;

	Vector vecDir, vecUp, vecSide, vecOrigin;
	QAngle angAngles;
	GetAttachment(m_iAttachment, vecOrigin, angAngles);
	if (m_pRagdollEnt != NULL)
	{
		m_pRagdollEnt->GetAttachment(m_iAttachment, vecOrigin, angAngles);
	}

	AngleVectors(angAngles, &vecDir, &vecSide, &vecUp);

	if (bLight)
	{
		if (!m_pELight)
		{
			m_pELight = effects->CL_AllocElight(entindex());

			//m_pELight->minlight = DLIGHT_MINLIGHT;
			m_pELight->die = FLT_MAX;
		}

		if (m_pELight)
		{
			m_pELight->origin = vecOrigin;
			m_pELight->radius = 100.0f * flScale;
			m_pELight->color.r = 16.0f; //hlss_miners_hat_light_color.GetFloat();
			m_pELight->color.g = 16.0f; //hlss_miners_hat_light_color.GetFloat();
			m_pELight->color.b = 16.0f; //hlss_miners_hat_light_color.GetFloat();
		}
	}
	else if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
		m_pELight = nullptr;
	}

	if (bLight)
	{
		if (!m_pFlashlight)
		{
			// Turned on the headlight; create it.
			m_pFlashlight = new CSpotlightEffect(false);

			if (m_pFlashlight)
				m_pFlashlight->TurnOn();
		}

		if (m_pFlashlight)
		{
			m_pFlashlight->UpdateLight(vecOrigin, vecDir, vecSide, vecUp, 100, flScale);
		}
	}
	else if (m_pFlashlight)
	{
		// Turned off the headlight; delete it.
		delete m_pFlashlight;
		m_pFlashlight = NULL;
	}

	if (bLight)
	{
		float flColor = 128.0f;

		if (!m_pBeam)
		{
			m_pBeam = CBeam::BeamCreate("sprites/glow_test02.vmt", 0.2);
			m_pBeam->SetColor(flColor * flScale, flColor * flScale, flColor * flScale);
			m_pBeam->SetBrightness(flColor * flScale);
			m_pBeam->SetNoise(0);
			m_pBeam->SetWidth(8.0f);	// On low end TVs these lasers are very hard to see at a distance
			m_pBeam->SetEndWidth((32.0f * flScale) + 8.0f);
			m_pBeam->SetScrollRate(0);
			m_pBeam->SetFadeLength(0);
			m_pBeam->SetHaloTexture(PrecacheModel("sprites/light_glow03.vmt"));
			m_pBeam->SetHaloScale(6.0f);
			m_pBeam->SetCollisionGroup(COLLISION_GROUP_NONE);
			m_pBeam->SetBeamFlag(FBEAM_SHADEOUT | FBEAM_NOTILE);
			m_pBeam->PointsInit(vecOrigin, vecOrigin + (vecDir * 40));
			m_pBeam->SetType(BEAM_ENTPOINT);
			m_pBeam->SetStartEntity(this);
			m_pBeam->SetEndPos(vecOrigin + (vecDir * 64));
			m_pBeam->SetStartAttachment(m_iAttachment);
			m_pBeam->SetEndAttachment(0);
			m_pBeam->RelinkBeam();
		}
		else
		{
			m_pBeam->RemoveEffects(EF_NODRAW);
			m_pBeam->SetColor(flColor * flScale, flColor * flScale, flColor * flScale);
			m_pBeam->SetBrightness(flColor * flScale);
			m_pBeam->SetEndWidth((32.f * flScale) + 8.0f);

			m_pBeam->SetEndPos(vecOrigin + (vecDir * 40));
			m_pBeam->RelinkBeam();
		}
	}
	else if (m_pBeam)
	{
		m_pBeam->Remove();
		m_pBeam = NULL;
	}
}

CStudioHdr* C_NPC_ZombieHEV::OnNewModel()
{
	CStudioHdr* pHdr = BaseClass::OnNewModel();
	if (pHdr)
	{
		m_iAttachment = LookupAttachment("flashlight");
	}

	return pHdr;
}
