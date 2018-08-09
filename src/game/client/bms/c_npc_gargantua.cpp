#include "cbase.h"
#include "c_ai_basenpc.h"
#include "particle_property.h"
#include "materialsystem/imaterial.h"
#include <KeyValues.h>
#include "materialsystem/imaterialvar.h"
#include "functionproxy.h"
#include "dlight.h"
#include "iefx.h"

#define GARG_FLAME_PARTICLE "gargantua_flame"
#define GARG_EYE_PARTICLE "gargantua_eye"

ConVar garg_dlight_radius("cl_garg_dlight_radius", "190", FCVAR_CHEAT);

Vector GetEffectCenter(CNewParticleEffect *pEffect)
{
	return VectorLerp(pEffect->GetControlPointAtCurrentTime(0), pEffect->GetControlPointAtCurrentTime(1), 0.5);
}

class C_NPC_Garg : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_Garg, C_AI_BaseNPC)
	DECLARE_NETWORKCLASS();

	virtual void	ClientThink(void);
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	UpdateOnRemove(void);
	virtual void	NotifyShouldTransmit(ShouldTransmitState_t state);

	float			GetChargePct()
	{
		if (m_flChargeEndCycle == 0.0f)
			return 0.0f;

		return RemapValClamped(GetCycle(), 0.0f, m_flChargeEndCycle, 0.0f, 1.0f);
	}

protected:

	void DestroyDlights();
	void UpdateDlights();

	float m_flLeftFireTime;
	float m_flRightFireTime;

	float m_flChargeEndCycle;

	dlight_t *m_pDLightEye;
	dlight_t *m_pDLightFire;
	bool m_bDlightFire/*, m_bDlightLeft, m_bDlightRight*/;

	CNewParticleEffect *m_hEffectEye;
	CNewParticleEffect *m_hEffectL;
	CNewParticleEffect *m_hEffectR;
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Garg, DT_NPC_Garg, CNPC_Garg)
RecvPropTime(RECVINFO(m_flLeftFireTime)),
RecvPropTime(RECVINFO(m_flRightFireTime)),
RecvPropFloat(RECVINFO(m_flChargeEndCycle)),
END_NETWORK_TABLE()

void C_NPC_Garg::UpdateOnRemove(void)
{
	if (m_hEffectL)
	{
		m_hEffectL->StopEmission();
		m_hEffectL = NULL;
	}
	if (m_hEffectR)
	{
		m_hEffectR->StopEmission();
		m_hEffectR = NULL;
	}
	if (m_hEffectEye)
	{
		m_hEffectEye->StopEmission();
		m_hEffectEye = NULL;
	}
	
	DestroyDlights();
}

void C_NPC_Garg::DestroyDlights()
{
	if (m_pDLightEye)
	{
		m_pDLightEye->die = gpGlobals->curtime;
		m_pDLightEye = NULL;
	}
	if (m_pDLightFire)
	{
		m_pDLightFire->die = gpGlobals->curtime;
		m_pDLightFire = NULL;
	}
}

void C_NPC_Garg::UpdateDlights()
{
	bool isFire = false/*, bDoRight = false, bDoLeft = false*/;
	Vector vecLightOrigin, vecLightFire;
	float flLightScale = 1.0f;

	if (m_hEffectL && m_hEffectR)
	{
		isFire = true;
		flLightScale = 2.0f;
		vecLightFire = VectorLerp(GetEffectCenter(m_hEffectL), GetEffectCenter(m_hEffectR), 0.5);
	}
	else if (m_hEffectL)
	{
		/*bDoLeft = */isFire = true;
		vecLightFire = GetEffectCenter(m_hEffectL);
	}
	else if (m_hEffectR)
	{
		/*bDoRight = */isFire = true;
		vecLightFire = GetEffectCenter(m_hEffectR);
	}

	{
		GetAttachment("eyes", vecLightOrigin);
	}

	if (m_bDlightFire != isFire)
	{
		DestroyDlights();

		m_bDlightFire = isFire;
		/*m_bDlightLeft = bDoLeft;
		m_bDlightRight = bDoRight;*/
	}

		//ColorRGBExp32 DLColor;

		/*if (isFire)
		{
			//31 114 255 255
			DLColor.r = 31;
			DLColor.g = 114;
			DLColor.b = 255;
			DLColor.exponent = 6;
		}
		else
		{
			//238 155 9 255
			DLColor.r = 238;
			DLColor.g = 155;
			DLColor.b = 9;
			DLColor.exponent = 6;
		}*/

		if (!m_pDLightEye)
		{
			ColorRGBExp32 DLColor;
			//238 155 9 255
			DLColor.r = 238;
			DLColor.g = 155;
			DLColor.b = 9;
			DLColor.exponent = 6;

				m_pDLightEye = effects->CL_AllocElight(index);

				m_pDLightEye->radius = garg_dlight_radius.GetFloat() * 0.75f;
				m_pDLightEye->color = DLColor;
		}
		if (isFire && !m_pDLightFire)
		{
			ColorRGBExp32 DLColor;
			//31 114 255 255
			DLColor.r = 31;
			DLColor.g = 114;
			DLColor.b = 255;
			DLColor.exponent = 6;

			m_pDLightFire = effects->CL_AllocDlight(-index);
			m_pDLightFire->radius = garg_dlight_radius.GetFloat() * flLightScale;
			m_pDLightFire->color = DLColor;
		}
	

	if (m_pDLightEye)
	{
		m_pDLightEye->origin = vecLightOrigin;
		m_pDLightEye->die = gpGlobals->curtime + 10.0f;
	}
	if (m_pDLightFire)
	{
		m_pDLightFire->radius = garg_dlight_radius.GetFloat() * flLightScale;
		m_pDLightFire->origin = vecLightFire;
		m_pDLightFire->die = gpGlobals->curtime + 10.0f;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Garg::NotifyShouldTransmit(ShouldTransmitState_t state)
{
	BaseClass::NotifyShouldTransmit(state);

	// Turn off
	if (state == SHOULDTRANSMIT_END)
	{
		if (m_hEffectEye)
		{
			m_hEffectEye->StopEmission();
			m_hEffectEye = NULL;
		}
		SetNextClientThink(CLIENT_THINK_NEVER);
		DestroyDlights();
	}

	// Turn on
	if (state == SHOULDTRANSMIT_START)
	{
		m_hEffectEye = ParticleProp()->Create(GARG_EYE_PARTICLE, PATTACH_POINT_FOLLOW, "eyes");
		//m_hEffectEye->SetControlPointEntity(0, this);
		SetNextClientThink(CLIENT_THINK_ALWAYS);
		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Garg::OnDataChanged(DataUpdateType_t type)
{

	if (m_flLeftFireTime > gpGlobals->curtime)
	{
		if (!m_hEffectL)
			m_hEffectL = ParticleProp()->Create(GARG_FLAME_PARTICLE, PATTACH_POINT_FOLLOW, "FireL");
	}
	else if (m_hEffectL)
	{
		m_hEffectL->StopEmission();
		m_hEffectL = NULL;
	}

	if (m_flRightFireTime > gpGlobals->curtime)
	{
		if (!m_hEffectR)
			m_hEffectR = ParticleProp()->Create(GARG_FLAME_PARTICLE, PATTACH_POINT_FOLLOW, "FireR");
	}
	else if (m_hEffectR)
	{
		m_hEffectR->StopEmission();
		m_hEffectR = NULL;
	}


	BaseClass::OnDataChanged(type);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Garg::ClientThink(void)
{
	// Don't update if our frame hasn't moved forward (paused)
	if (gpGlobals->frametime <= 0.0f)
		return;

	//UpdateDlights();

	Vector vecStart, vecDir, vecEnd;
	QAngle vecAngles;

	if (m_hEffectL)
	{
		GetAttachment("FireL", vecStart, vecAngles);
		AngleVectors(vecAngles, &vecDir);

		vecEnd = vecStart + (vecDir * 300);
		m_hEffectL->SetControlPoint(1, vecEnd);
	}

	if (m_hEffectR)
	{
		GetAttachment("FireR", vecStart, vecAngles);
		AngleVectors(vecAngles, &vecDir);

		vecEnd = vecStart + (vecDir * 300);
		m_hEffectR->SetControlPoint(1, vecEnd);
	}

	UpdateDlights();

}

class CGargChargeProxy : public CResultProxy
{
public:
	virtual void OnBind(void *pC_BaseEntity);
};

void CGargChargeProxy::OnBind(void *pC_BaseEntity)
{
	C_BaseEntity *pEnt = BindArgToEntity(pC_BaseEntity);

	C_NPC_Garg *pGarg = dynamic_cast<C_NPC_Garg *> (pEnt);

	if (!pGarg)
	{
		SetFloatResult(0.0f);
		return;
	}


	SetFloatResult(pGarg->GetChargePct());
}

EXPOSE_INTERFACE(CGargChargeProxy, IMaterialProxy, "GargCharge" IMATERIAL_PROXY_INTERFACE_VERSION);