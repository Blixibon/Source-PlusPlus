#include "cbase.h"
#include "particle_property.h"
#include "c_entityelectric.h"
#include "iefx.h"
#include "dlight.h"

IMPLEMENT_CLIENTCLASS_DT(C_EntityElectric, DT_EntityElectric, CEntElectric)
RecvPropInt(RECVINFO(m_nShockType)),
RecvPropTime(RECVINFO(m_flStartTime)),
END_RECV_TABLE()

static const char *g_sParticleNames[MAX_SHOCK] =
{
	"tfa_lightning_model",
	"vortigaunt_zap",
	"electrical_zap"
};

ColorRGBExp32 g_clrDlightFlashColors[MAX_SHOCK] = {
	{0, 168, 255, 1},
	{184, 255, 114, 0},
	{214, 113, 13, 0}
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
	if (gpGlobals->frametime <= 0.0f || GetRenderColor().a <= 0)
		return;

	C_BaseAnimating* pAnimating = GetMoveParent() ? GetMoveParent()->GetBaseAnimating() : NULL;
	if (!pAnimating)
		return;

	if ((IsEffectActive(EF_BRIGHTLIGHT) || IsEffectActive(EF_DIMLIGHT)) && gpGlobals->curtime - m_flLastFlashTime >= 0.01f)
	{
		m_flLastFlashTime = gpGlobals->curtime;

		studiohdr_t* pStudioHdr = modelinfo->GetStudiomodel(pAnimating->GetModel());
		if (pStudioHdr)
		{
			mstudiohitboxset_t* set = pStudioHdr->pHitboxSet(pAnimating->GetHitboxSet());
			if (set)
			{
				matrix3x4_t* hitboxbones[MAXSTUDIOBONES];
				if (pAnimating->HitboxToWorldTransforms(hitboxbones))
				{
					for (int i = 0; i < 2; i++)
					{
						int iHitBox = random->RandomInt(1, set->numhitboxes) - 1;
						mstudiobbox_t* pBox = set->pHitbox(iHitBox);

						// Select a random point somewhere in the hitboxes of the entity.
						Vector vecLocalPosition, vecWorldPosition;
						vecLocalPosition.x = Lerp(random->RandomFloat(-0.1f, 1.1f), pBox->bbmin.x, pBox->bbmax.x);
						vecLocalPosition.y = Lerp(random->RandomFloat(-0.1f, 1.1f), pBox->bbmin.y, pBox->bbmax.y);
						vecLocalPosition.z = Lerp(random->RandomFloat(-0.1f, 1.1f), pBox->bbmin.z, pBox->bbmax.z);
						VectorTransform(vecLocalPosition, *hitboxbones[pBox->bone], vecWorldPosition);

						dlight_t* dl = effects->CL_AllocDlight((i == 0) ? index : -index);
						dl->origin = vecWorldPosition;
						dl->color = g_clrDlightFlashColors[m_nShockType];
						dl->radius = random->RandomFloat(200, 245);
						dl->die = gpGlobals->curtime + 0.01f;
					}
				}
			}
		}
	}

	if (gpGlobals->curtime >= m_flNextZapTime)
	{
		m_flNextZapTime = gpGlobals->curtime + RandomFloat(0.1f, 0.3f);

		// Sound
		if (C_BasePlayer::GetLocalPlayer())
		{
			soundlevel_t level = CBaseEntity::LookupSoundLevel("RagdollBoogie.Zap");
			float attenuation = SNDLVL_TO_ATTN(level);
			float maxAudible = (2 * SOUND_NORMAL_CLIP_DIST) / attenuation;
			float flDistSqr = GetAbsOrigin().DistToSqr(C_BasePlayer::GetLocalPlayer()->EarPosition());

			if (flDistSqr <= Sqr(maxAudible))
				EmitSound("RagdollBoogie.Zap");
		}

		if (pAnimating->IsRagdoll())
		{
			float flMagnitude;
			float dt = gpGlobals->curtime - m_flStartTime;
			if (dt >= 2.f || dt < 0)
			{
				flMagnitude = 0.f;
			}
			else
			{
				//flMagnitude = SimpleSplineRemapValClamped(dt, 0.0f, 2.f, 150.f, 0.0f);
				flMagnitude = 150.f;
			}

			if (flMagnitude > 0.f)
			{
				ragdoll_t* pRagdollPhys = pAnimating->m_pRagdoll->GetRagdoll();
				for (int j = 0; j < pRagdollPhys->listCount; ++j)
				{
					float flMass = pRagdollPhys->list[j].pObject->GetMass();
					float flForce = flMagnitude * flMass;

					Vector vecForce;
					vecForce = RandomVector(-flForce, flForce);
					pRagdollPhys->list[j].pObject->ApplyForceCenter(vecForce);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EntityElectric::ClientThink(void)
{
	StopEffect();
	Release();
}