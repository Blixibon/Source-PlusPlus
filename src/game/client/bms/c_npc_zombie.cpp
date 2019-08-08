#include "cbase.h"
#include "c_ai_basenpc.h"
#include "dlight.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_ZombieGuard : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_ZombieGuard, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t type);
	virtual void ClientThink();


protected:
	CNewParticleEffect *m_pDroolFX;
	dlight_t *m_dlight;

	float m_flTimeLastFlicker;
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
	if (!m_dlight)
	{
		m_dlight = effects->CL_AllocElight(index);
		m_dlight->color.r = 37;
		m_dlight->color.g = 255;
		m_dlight->color.b = 63;
		m_dlight->radius = 30;
		m_dlight->decay = 0;
		m_dlight->minlight = 128.0 / 256.0f;
	}

	Vector vecMouth;
	GetAttachment("mouth", vecMouth);

	if (m_pRagdollEnt != NULL)
	{
		m_pRagdollEnt->GetAttachment("mouth", vecMouth);
		ParticleProp()->AddControlPoint(m_pDroolFX, 1, m_pRagdollEnt, PATTACH_POINT_FOLLOW, "mouth", Vector(3, 0, -1));

		if (m_dlight->decay == 0)
			m_dlight->decay = m_dlight->radius / 1.75f;
	}

	m_dlight->origin = vecMouth;
	m_dlight->die = gpGlobals->curtime + 0.1f;

	if ((gpGlobals->curtime - m_flTimeLastFlicker) >= 0.1f)
	{
		Vector color1(37, 255, 63);
		Vector color2(142, 251, 0);
		Vector color = Lerp(RandomFloat(), color1, color2);
		VectorToColorRGBExp32(color, m_dlight->color);

		if (m_pRagdollEnt == NULL)
			m_dlight->radius = RandomFloat(20.0f, 30.0f);

		m_flTimeLastFlicker = gpGlobals->curtime;
	}

	BaseClass::ClientThink();
}