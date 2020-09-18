#include "cbase.h"
#include "c_ai_basenpc.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "beamdraw.h"
//#include "estranged_system_caps.h"
#include "flashlighteffect.h"

#define DLIGHT_RADIUS (100.0f)
#define DLIGHT_MINLIGHT (4.0f/255.0f)

class C_NPC_Core : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_Core, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	C_NPC_Core() : m_iAttachment(-1)
	{}

	void	OnDataChanged(DataUpdateType_t updateType);
	void	UpdateOnRemove();
	void	ClientThink();

protected:
	bool	m_bFlashlightOn;

private:
	void SetupAttachments();

	int		m_iAttachment;
	dlight_t *m_pELight;
	CSpotlightEffect *m_pSpotLight;
	CBeam *m_pBeam;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Core, DT_NPC_Core, CNPC_Core)
RecvPropBool(RECVINFO(m_bFlashlightOn)),
END_RECV_TABLE()

void C_NPC_Core::SetupAttachments()
{
	if (m_iAttachment == -1)
	{
		m_iAttachment = LookupAttachment("eyes");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_Core::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		CMouthInfo *pMouth = GetMouth();
		pMouth->ActivateEnvelope();
	}

	SetupAttachments();

	// start thinking if we need to fade.
	if (m_bFlashlightOn && !IsEffectActive(EF_NODRAW))
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
	else
	{
		SetNextClientThink(CLIENT_THINK_NEVER);

		if (m_pELight)
		{
			m_pELight->die = gpGlobals->curtime;
			m_pELight = NULL;
		}

		if (m_pSpotLight)
		{
			// Turned off the headlight; delete it.
			delete m_pSpotLight;
			m_pSpotLight = NULL;
		}

		if (m_pBeam)
		{
			m_pBeam->Remove();
			m_pBeam = NULL;
		}
	}
}

void C_NPC_Core::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
	}

	if (m_pSpotLight)
	{
		// Turned off the headlight; delete it.
		delete m_pSpotLight;
		m_pSpotLight = NULL;
	}

	if (m_pBeam)
	{
		m_pBeam->Remove();
		m_pBeam = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Randomly adds extra effects
//-----------------------------------------------------------------------------
void C_NPC_Core::ClientThink(void)
{
	if (IsEffectActive(EF_NODRAW))
		return;

	if (gpGlobals->frametime == 0.0f)
		return;

	if (m_iAttachment == -1)
		return;

	Vector effect_origin;
	QAngle effect_angles;

	GetAttachment(m_iAttachment, effect_origin, effect_angles);

	Vector forward;
	AngleVectors(effect_angles, &forward);

	//CMouthInfo *pMouth = GetMouth();
	////pMouth->ActivateEnvelope();

	/*float value = GetMouthOpenPct();

	if (value > 1.0)
		value = 1.0;

	value = (1.0 - value);

	float flScale = value;*/
	const float flScale = 1.0f;

	

	bool bSpotLight = m_bFlashlightOn && g_pMaterialSystem->SupportsShadowDepthTextures();



	if (m_bFlashlightOn)
	{
		if (!m_pELight)
		{

			m_pELight = effects->CL_AllocElight(entindex());

			m_pELight->minlight = DLIGHT_MINLIGHT;
			m_pELight->die = FLT_MAX;
		}

		if (m_pELight)
		{
			m_pELight->origin = effect_origin;
			m_pELight->radius = 100.0f * flScale;
			m_pELight->color.r = 16.0f; //hlss_miners_hat_light_color.GetFloat();
			m_pELight->color.g = 16.0f; //hlss_miners_hat_light_color.GetFloat();
			m_pELight->color.b = 16.0f; //hlss_miners_hat_light_color.GetFloat();
		}
	}
	else if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
	}

	if (bSpotLight)
	{
		if (!m_pSpotLight)
		{
			// Turned on the headlight; create it.
			m_pSpotLight = new CSpotlightEffect();

			if (m_pSpotLight)
				m_pSpotLight->TurnOn();
		}
		if (m_pSpotLight)
		{
			Vector vecDir, vecUp, vecSide;
			AngleVectors(effect_angles, &vecDir, &vecSide, &vecUp);
			m_pSpotLight->UpdateLight(effect_origin, vecDir, vecSide, vecUp, 100, flScale);
		}
	}
	else if (m_pSpotLight)
	{
		// Turned off the headlight; delete it.
		delete m_pSpotLight;
		m_pSpotLight = NULL;
	}


	// Inner beams
	BeamInfo_t beamInfo;

	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_pStartEnt = this;
	beamInfo.m_nStartAttachment = m_iAttachment;

	beamInfo.m_vecEnd = effect_origin + (forward * 64);
	beamInfo.m_pEndEnt = NULL;
	beamInfo.m_nEndAttachment = -1;

	float flColor = 128.0f;

	beamInfo.m_pszModelName = "sprites/glow_test02.vmt";
	beamInfo.m_pszHaloName = "sprites/light_glow03.vmt";
	beamInfo.m_flHaloScale = 6.0f;
	beamInfo.m_flLife = 0.0f;
	beamInfo.m_flWidth = 8.0f; //random->RandomFloat( 1.0f, 2.0f );
	beamInfo.m_flEndWidth = (45.f * flScale) + 8.0f;
	//beamInfo.m_flFadeLength = HLSS_MINERS_HAT_LIGHT_LENGTH;
	beamInfo.m_flAmplitude = 0.0f; //random->RandomFloat( 16, 32 );
	beamInfo.m_flBrightness = flColor * flScale;
	beamInfo.m_flSpeed = 0.0;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 1.0f;

	beamInfo.m_flRed = flColor * flScale;
	beamInfo.m_flGreen = flColor * flScale;
	beamInfo.m_flBlue = flColor * flScale;

	beamInfo.m_nSegments = 8;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = FBEAM_SHADEOUT | FBEAM_NOTILE | FBEAM_HALOBEAM;

	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate("sprites/glow_test02.vmt", 0.2);
		m_pBeam->SetColor(flColor * flScale, flColor * flScale, flColor * flScale);
		m_pBeam->SetBrightness(flColor * flScale);
		m_pBeam->SetNoise(0);
		m_pBeam->SetWidth(8.0f);	// On low end TVs these lasers are very hard to see at a distance
		m_pBeam->SetEndWidth((24.0f * flScale) + 8.0f);
		m_pBeam->SetScrollRate(0);
		m_pBeam->SetFadeLength(0);
		m_pBeam->SetHaloTexture(PrecacheModel("sprites/light_glow03.vmt"));
		m_pBeam->SetHaloScale(6.0f);
		m_pBeam->SetCollisionGroup(COLLISION_GROUP_NONE);
		m_pBeam->PointsInit(effect_origin, effect_origin + (forward * 64));
		m_pBeam->SetBeamFlag(FBEAM_SHADEOUT | FBEAM_NOTILE | FBEAM_HALOBEAM);
		m_pBeam->SetStartEntity(this);
	}
	else
	{
		m_pBeam->RemoveEffects(EF_NODRAW);
		m_pBeam->SetColor(flColor * flScale, flColor * flScale, flColor * flScale);
		m_pBeam->SetBrightness(flColor * flScale);
		//m_pBeam->SetNoise(0);
		//m_pBeam->SetWidth(8.0f);	// On low end TVs these lasers are very hard to see at a distance
		m_pBeam->SetEndWidth((24.0f * flScale) + 8.0f);
		//m_pBeam->SetScrollRate(0);
		//m_pBeam->SetFadeLength(0);
		//m_pBeam->SetHaloTexture(PrecacheModel("sprites/light_glow03.vmt"));
		//m_pBeam->SetHaloScale(6.0f);
		//m_pBeam->SetCollisionGroup(COLLISION_GROUP_NONE);
		m_pBeam->PointsInit(effect_origin, effect_origin + (forward * 64));
		//m_pBeam->SetBeamFlag(FBEAM_SHADEOUT | FBEAM_NOTILE | FBEAM_HALOBEAM);
		//m_pBeam->SetStartEntity(this);
	}
}