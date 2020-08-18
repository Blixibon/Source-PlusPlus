#include "cbase.h"
#include "env_entity_electric.h"
#include "ai_basenpc.h"

IMPLEMENT_SERVERCLASS_ST(CEntElectric, DT_EntityElectric)
SendPropInt(SENDINFO(m_nShockType)),
SendPropTime(SENDINFO(m_flStartTime)),
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS(entityshock, CEntElectric);

BEGIN_DATADESC(CEntElectric)
DEFINE_THINKFUNC(RemoveThink),
DEFINE_FIELD(m_nShockType, FIELD_INTEGER),
DEFINE_FIELD(m_flStartTime, FIELD_TIME),
END_DATADESC();

void CEntElectric::Spawn()
{

	SetThink(&CEntElectric::RemoveThink);
	if (gpGlobals->curtime > m_flStartTime)
	{
		// Necessary for server-side ragdolls
		RemoveThink();
	}
	else
	{
		SetNextThink(gpGlobals->curtime + 0.01f);
	}
}

void CEntElectric::Precache()
{
	BaseClass::Precache();

	PrecacheParticleSystem("tfa_lightning_model");
	PrecacheParticleSystem("vortigaunt_zap");
	PrecacheParticleSystem("electrical_zap");
}

//-----------------------------------------------------------------------------
// Purpose: Creates a flame and attaches it to a target entity.
// Input  : pTarget - 
//-----------------------------------------------------------------------------
CEntElectric *CEntElectric::Create(CBaseEntity *pTarget, float flStartTime, int nDissolveType)
{
	CEntElectric *pDissolve = (CEntElectric *)CreateEntityByName("entityshock");

	if (pDissolve == NULL)
		return NULL;

	pDissolve->m_nShockType = nDissolveType;

	if (g_pGameRules->ShouldBurningPropsEmitLight())
		pDissolve->AddEffects(EF_DIMLIGHT);

	pDissolve->AttachToEntity(pTarget);
	pDissolve->SetStartTime(flStartTime);
	pDissolve->Spawn();

	// Send to the client even though we don't have a model
	pDissolve->AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

	return pDissolve;
}


//-----------------------------------------------------------------------------
// What type of dissolve?
//-----------------------------------------------------------------------------
CEntElectric *CEntElectric::Create(CBaseEntity *pTarget, CBaseEntity *pSource)
{
	// Look for other boogies on the ragdoll + kill them
	for (CBaseEntity *pChild = pSource->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer())
	{
		CEntElectric *pDissolve = dynamic_cast<CEntElectric*>(pChild);
		if (!pDissolve)
			continue;

		return Create(pTarget,  pDissolve->m_flStartTime, pDissolve->m_nShockType);
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Attaches the flame to an entity and moves with it
// Input  : pTarget - target entity to attach to
//-----------------------------------------------------------------------------
void CEntElectric::AttachToEntity(CBaseEntity *pTarget)
{
	// Look for other boogies on the ragdoll + kill them
	for (CBaseEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pChild->NextMovePeer())
	{
		CEntElectric *pDissolve = dynamic_cast<CEntElectric*>(pChild);
		if (!pDissolve)
			continue;

		UTIL_Remove(pDissolve);
	}

	// So our dissolver follows the entity around on the server.
	SetParent(pTarget);
	SetLocalOrigin(vec3_origin);
	SetLocalAngles(vec3_angle);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : lifetime - 
//-----------------------------------------------------------------------------
void CEntElectric::SetStartTime(float flStartTime)
{
	m_flStartTime = flStartTime;
}

void CEntElectric::RemoveThink()
{
	if (GetMoveParent())
	{
		if (GetMoveParent()->GetFlags() & FL_TRANSRAGDOLL)
		{
			SetRenderColorA(0);
			return;
		}

		//CAI_BaseNPC *pNPC = GetMoveParent()->MyNPCPointer();
		//if (pNPC && !pNPC->IsAlive())
		//{
		//	UTIL_Remove(this);
		//	// Notify the NPC that it's no longer burning!
		//	//pNPC->Extinguish();
		//	return;
		//}
	}
	else
	{
		UTIL_Remove(this);
		return;
	}

	if (gpGlobals->curtime >= m_flStartTime + 2.0f)
	{
		UTIL_Remove(this);
		return;
	}



	SetNextThink(gpGlobals->curtime + 0.1f);
}