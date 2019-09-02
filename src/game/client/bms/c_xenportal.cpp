#include "cbase.h"
#include "particles_simple.h"
#include "citadel_effects_shared.h"
#include "particles_attractor.h"
#include "iefx.h"
#include "dlight.h"
#include "clienteffectprecachesystem.h"
#include "c_te_effect_dispatch.h"
#include "particle_property.h"
#include "engine\IEngineSound.h"

#define MESSAGE_START_FIREBALL 0
#define MESSAGE_STOP_FIREBALL 1

bool DoPortalBeamFx(Vector origin, int iBeams = 3, float flRadius = 250.0f, float flScale = 25.0f, QAngle angles = vec3_angle)
{
	/*if (!GetClientWorldEntity())
		return false;

	if (!pEnt)
		pEnt = GetClientWorldEntity();*/

	

	//DispatchParticleEffect("xen_portal", origin, angles, pEnt);

	CPASAttenuationFilter filter(origin);
	CBaseEntity::EmitSound(filter, SOUND_FROM_WORLD, "XenPortal.Sound", &origin);

	CSmartPtr<CNewParticleEffect> pEffect = NULL;

	//pEffect = pEnt->ParticleProp()->Create("xen_portal_lines", PATTACH_CUSTOMORIGIN, 0, origin);
	pEffect = CNewParticleEffect::Create(NULL, "xen_portal");
	if (pEffect->IsValid())
	{
		pEffect->SetControlPoint(0, origin);
		pEffect->SetControlPoint(2, Vector(flScale, 0,0));
		pEffect->SetControlPoint(3, Vector(flScale, 0, 0));
	}

	// Send out beams around us
	int iNumBeamsAround = (iBeams * 2) / 3; // (2/3 of the beams are placed around in a circle)
	int iNumRandomBeams = iBeams - iNumBeamsAround;
	int iTotalBeams = iNumBeamsAround + iNumRandomBeams;
	float flYawOffset = RandomFloat(0, 360);
	for (int i = 0; i < iTotalBeams; i++)
	{
		// Make a couple of tries at it
		int iTries = -1;
		Vector vecForward;
		trace_t tr;
		do
		{
			iTries++;

			// Some beams are deliberatly aimed around the point, the rest are random.
			if (i < iNumBeamsAround)
			{
				QAngle vecTemp = angles;
				vecTemp[YAW] += anglemod(flYawOffset + ((360 / iTotalBeams) * i));
				AngleVectors(vecTemp, &vecForward);

				// Randomly angle it up or down
				vecForward.z = RandomFloat(-1, 1);
			}
			else
			{
				vecForward = RandomVector(-1, 1);
			}
			VectorNormalize(vecForward);

			float flRad = flRadius;
			UTIL_TraceLine(origin, origin + (vecForward * flRad), (CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_WATER | CONTENTS_SLIME), NULL, COLLISION_GROUP_NONE, &tr);
		} while (tr.fraction >= 1.0 && iTries < 3);

		Vector vecEnd = tr.endpos - (vecForward * 8);

		//CNewParticleEffect *pEffect;
		CSmartPtr<CNewParticleEffect> pEffect = NULL;

		//pEffect = pEnt->ParticleProp()->Create("xen_portal_lines", PATTACH_CUSTOMORIGIN, 0, origin);
		pEffect = CNewParticleEffect::Create(NULL, "xen_portal_lines");
		if (pEffect->IsValid())
		{
			pEffect->SetControlPoint(0, origin);
			pEffect->SetControlPoint(1, vecEnd);
		}

	}

	dlight_t *pLight = effects->CL_AllocDlight(0);
	pLight->origin = origin;
	pLight->color.r = 64;
	pLight->color.g = 255;
	pLight->color.b = 64;
	pLight->color.exponent = 3;
	pLight->radius = flRadius;
	pLight->die = gpGlobals->curtime + 3.6f;

	return true;
}

void XenPortalCallback(const CEffectData &data)
{
	//CPASAttenuationFilter filter(data.m_vOrigin);
	//CBaseEntity::EmitSound(filter, data.entindex(), "XenPortal.Sound", &data.m_vOrigin);
	DoPortalBeamFx(data.m_vOrigin, data.m_nHitBox, data.m_flRadius, data.m_flScale);
}

DECLARE_CLIENT_EFFECT("XenPortalIn", XenPortalCallback);

void XenPortalOutCallback(const CEffectData &data)
{
	CPASAttenuationFilter filter(data.m_vOrigin);
	CBaseEntity::EmitSound(filter, SOUND_FROM_WORLD, "VFX.TeleportOut", &data.m_vOrigin);
}

DECLARE_CLIENT_EFFECT("XenPortalOut", XenPortalOutCallback);

class C_BaseXenPortal : public C_BaseEntity
{
	DECLARE_CLASS(C_BaseXenPortal, C_BaseEntity);

public:
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	ClientThink(void);
	virtual void	ReceiveMessage(int classID, bf_read &msg);

	virtual int		GetXenPortalClassID() { return GetClientClass()->m_ClassID; }

protected:

	bool m_bIsLighting;
	float m_flNextLight;

	bool DoBeamFx(int iBeams = 3, float flMinRad = 150, float flMaxRad = 300);

};

class C_XenPortal : public C_BaseXenPortal
{
	DECLARE_CLASS(C_XenPortal, C_BaseXenPortal);
	DECLARE_CLIENTCLASS();

public:

	virtual int		GetXenPortalClassID() { return GetClientClass()->m_ClassID; }

};

IMPLEMENT_CLIENTCLASS_DT(C_XenPortal, DT_XenPortal, CXenPortal)

END_RECV_TABLE()


void C_BaseXenPortal::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	// start thinking if we need to fade.
	if (m_bIsLighting)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseXenPortal::ClientThink(void)
{
	// Don't update if our frame hasn't moved forward (paused)
	if (gpGlobals->frametime <= 0.0f)
		return;

	if (m_bIsLighting)
	{
		

		if (m_flNextLight <= gpGlobals->curtime)
		{
			float life = 0.5f;

			dlight_t *dl = effects->CL_AllocDlight(index);
			dl->origin = GetAbsOrigin();
			dl->color.r = 64;
			dl->color.g = 255;
			dl->color.b = 64;
			dl->color.exponent = 3;
			dl->radius = random->RandomFloat(100, 225);
			//dl->minlight = DLIGHT_MINLIGHT;
			dl->decay = dl->radius / life;
			dl->die = gpGlobals->curtime + life;
			

			m_flNextLight = gpGlobals->curtime + 0.1f;
		}

	}

}

//-----------------------------------------------------------------------------
// Purpose: Receive messages from the server
//-----------------------------------------------------------------------------
void C_BaseXenPortal::ReceiveMessage(int classID, bf_read &msg)
{
	//// Is the message for a sub-class?
	//if (classID != GetXenPortalClassID())
	//{
	//	BaseClass::ReceiveMessage(classID, msg);
	//	return;
	//}

	int messageType = msg.ReadByte();
	switch (messageType)
	{
	case MESSAGE_START_FIREBALL:
	{
		//m_bIsLighting = true;
		//SetNextClientThink(CLIENT_THINK_ALWAYS);

		
		DoBeamFx();
	}
	break;
	}

}

bool C_BaseXenPortal::DoBeamFx(int iBeams, float flMinRad, float flMaxRad)
{

	return DoPortalBeamFx(GetAbsOrigin(), RandomInt(3, 5), RandomFloat(flMinRad, flMaxRad), 60);

}