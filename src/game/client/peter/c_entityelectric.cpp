#include "cbase.h"
#include "particle_property.h"
#include "c_entityelectric.h"

IMPLEMENT_CLIENTCLASS_DT(C_EntityElectric, DT_EntityElectric, CEntElectric)
RecvPropInt(RECVINFO(m_nShockType)),
RecvPropTime(RECVINFO(m_flStartTime)),
END_RECV_TABLE()

static const char *g_sParticleNames[MAX_SHOCK] =
{
	"tfa_lightning_model",
	"vortigaunt_zap",
	"aliencontroller_zap"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityElectric::C_EntityElectric(void) :
	m_hEffect(NULL)
{
	m_hOldAttached = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_EntityElectric::~C_EntityElectric(void)
{
	StopEffect();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityElectric::StopEffect(void)
{
	if (m_hEffect)
	{
		ParticleProp()->StopEmission(m_hEffect, true);
		//m_hEffect->SetControlPointEntity(0, NULL);
		//m_hEffect->SetControlPointEntity(1, NULL);
		m_hEffect = NULL;
	}

	if (GetMoveParent())
	{
		//m_hEntAttached->RemoveFlag(FL_ONFIRE);
		GetMoveParent()->SetEffectEntity(NULL, ENT_EFFECT_SHOCK);
		//m_hEntAttached->StopSound("General.BurningFlesh");
		//m_hEntAttached->StopSound("General.BurningObject");


		//m_hEntAttached = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityElectric::UpdateOnRemove(void)
{
	StopEffect();
	BaseClass::UpdateOnRemove();
}

void C_EntityElectric::CreateEffect(void)
{
	if (m_hEffect)
	{
		ParticleProp()->StopEmission(m_hEffect, true);
		m_hEffect->SetControlPointEntity(0, NULL);
		m_hEffect->SetControlPointEntity(1, NULL);
		m_hEffect = NULL;
	}


	m_hEffect = ParticleProp()->Create(g_sParticleNames[m_nShockType], PATTACH_ABSORIGIN);


	if (m_hEffect)
	{
		C_BaseEntity *pEntity = GetMoveParent();
		m_hOldAttached = GetMoveParent();

		ParticleProp()->AddControlPoint(m_hEffect, 0, pEntity, PATTACH_ABSORIGIN_FOLLOW);
		ParticleProp()->AddControlPoint(m_hEffect, 1, pEntity, PATTACH_ABSORIGIN_FOLLOW);
		m_hEffect->SetControlPoint(0, GetAbsOrigin());
		m_hEffect->SetControlPoint(1, GetAbsOrigin());
		m_hEffect->SetControlPointEntity(0, pEntity);
		m_hEffect->SetControlPointEntity(1, pEntity);

		float flTimeDelta = gpGlobals->curtime - m_flStartTime;
		if (flTimeDelta > 0.01f)
		{
			m_hEffect->SkipToTime(flTimeDelta);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityElectric::OnDataChanged(DataUpdateType_t updateType)
{
	if (updateType == DATA_UPDATE_CREATED)
	{
		CreateEffect();
	}

	// FIXME: This is a bit of a shady path
	if (updateType == DATA_UPDATE_DATATABLE_CHANGED)
	{
		// If our owner changed, then recreate the effect
		if (GetMoveParent() != m_hOldAttached)
		{
			//CreateEffect();

			if (m_hEffect)
			{
				C_BaseEntity *pEntity = GetMoveParent();
				m_hOldAttached = GetMoveParent();

				ParticleProp()->AddControlPoint(m_hEffect, 1, pEntity, PATTACH_ABSORIGIN_FOLLOW);
				m_hEffect->SetControlPoint(0, GetAbsOrigin());
				m_hEffect->SetControlPoint(1, GetAbsOrigin());
				m_hEffect->SetControlPointEntity(0, pEntity);
				m_hEffect->SetControlPointEntity(1, pEntity);
			}
		}

		if (GetMoveParent()->GetBaseAnimating() && GetMoveParent()->GetBaseAnimating()->IsAboutToRagdoll())
		{
			StopEffect();
		}
	}

	BaseClass::OnDataChanged(updateType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityElectric::Simulate(void)
{
	if (gpGlobals->frametime <= 0.0f)
		return;

//#ifdef HL2_EPISODIC 
//
//	if (IsEffectActive(EF_BRIGHTLIGHT) || IsEffectActive(EF_DIMLIGHT))
//	{
//		dlight_t *dl = effects->CL_AllocDlight(index);
//		dl->origin = GetAbsOrigin();
//		dl->origin[2] += 16;
//		dl->color.r = 254;
//		dl->color.g = 174;
//		dl->color.b = 10;
//		dl->radius = random->RandomFloat(400, 431);
//		dl->die = gpGlobals->curtime + 0.001;
//	}
//
//#endif // HL2_EPISODIC 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityElectric::ClientThink(void)
{
	StopEffect();
	Release();
}