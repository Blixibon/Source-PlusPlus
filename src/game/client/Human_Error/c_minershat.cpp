#include "cbase.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender_beams.h"
#include "beam_shared.h"
#include "estranged_system_caps.h"
#include "flashlighteffect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DLIGHT_RADIUS (100.0f)
#define DLIGHT_MINLIGHT (4.0f/255.0f)

ConVar hlss_miners_hat_elight("hlss_miners_hat_elight", "1");
//ConVar hlss_miners_hat_light_color("hlss_miners_hat_light_color", "128");
ConVar hlss_miners_hat_spotlight("hlss_miners_hat_spotlight", "1");
ConVar hlss_miners_hat_volumetric("hlss_miners_hat_spotlight_volumetrics", "0");

#define HLSS_MINERS_HAT_LIGHT_LENGTH 8.0f

class C_MinersHat : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_MinersHat, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

	C_MinersHat();

	void	OnDataChanged(DataUpdateType_t updateType);
	void	OnRestore();
	void	UpdateOnRemove();
	void	ClientThink();

	bool	m_bLight;
	bool	m_bFadeOut;
	float	m_flFadeOutSpeed;
	float	m_flFadeOutTime;

private:
	void SetupAttachments();

	int		m_iAttachment;
	dlight_t* m_pELight;
	CSpotlightEffect* m_pSpotLight;
	CBeam *m_pBeam;
};

IMPLEMENT_CLIENTCLASS_DT(C_MinersHat, DT_MinersHat, CHLSS_MinersHat)
RecvPropInt(RECVINFO(m_bLight)),
RecvPropBool(RECVINFO(m_bFadeOut)),
RecvPropFloat(RECVINFO(m_flFadeOutSpeed)),
RecvPropFloat(RECVINFO(m_flFadeOutTime)),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_MinersHat::C_MinersHat()
{
	m_bLight = false;
	m_bFadeOut = false;

	m_iAttachment = -1;

	m_pELight = NULL;
}

void C_MinersHat::OnRestore()
{
	SetupAttachments();

	BaseClass::OnRestore();
}

void C_MinersHat::SetupAttachments()
{
	if (m_iAttachment == -1)
	{
		m_iAttachment = LookupAttachment("light");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_MinersHat::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	SetupAttachments();

	// start thinking if we need to fade.
	if (m_bLight)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
	else
	{
		SetNextClientThink(CLIENT_THINK_NEVER);
	}
}

void C_MinersHat::UpdateOnRemove()
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
void C_MinersHat::ClientThink(void)
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

	float flScale = 1.0f;

	if (m_bFadeOut)
	{
		flScale = clamp((m_flFadeOutTime - gpGlobals->curtime) / m_flFadeOutSpeed, 0.0f, 1.0f);
	}

	bool bSpotLight = m_bLight && hlss_miners_hat_spotlight.GetBool() && CEstrangedSystemCaps::HasCaps(CAPS_SHADOW_DEPTHPASS);
	bool bAttemptVolumetrics = bSpotLight && hlss_miners_hat_volumetric.GetBool();


	if (m_bLight && hlss_miners_hat_elight.GetBool())
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
			m_pSpotLight = new CSpotlightEffect(bAttemptVolumetrics);

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

	float flColor = 128.0f;

	if (!bAttemptVolumetrics || !g_pClientShadowMgr->VolumetricsAvailable())
	{
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
			m_pBeam->SetBeamFlag(FBEAM_SHADEOUT | FBEAM_NOTILE);
			m_pBeam->PointsInit(effect_origin, effect_origin + (forward * 64));
			m_pBeam->SetType(BEAM_ENTPOINT);
			m_pBeam->SetStartEntity(this);
			m_pBeam->SetEndPos(effect_origin + (forward * 64));
			m_pBeam->SetStartAttachment(m_iAttachment);
			m_pBeam->SetEndAttachment(0);
			m_pBeam->RelinkBeam();
		}
		else
		{
			m_pBeam->RemoveEffects(EF_NODRAW);
			m_pBeam->SetColor(flColor * flScale, flColor * flScale, flColor * flScale);
			m_pBeam->SetBrightness(flColor * flScale);
			m_pBeam->SetEndWidth((24.0f * flScale) + 8.0f);

			m_pBeam->SetEndPos(effect_origin + (forward * 64));
			m_pBeam->RelinkBeam();
		}
	}
	else if (m_pBeam)
	{
		m_pBeam->Remove();
		m_pBeam = NULL;
	}
}