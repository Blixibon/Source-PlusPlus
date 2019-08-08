
#include "cbase.h"
#include "xenportal.h"
#include "particle_parse.h"
#include "datacache/imdlcache.h"
#include "entityapi.h"
#include "entityoutput.h"
#include "ai_basenpc.h"
#include "monstermaker.h"
#include "TemplateEntities.h"
#include "ndebugoverlay.h"
#include "mapentities.h"
#include "IEffects.h"
#include "props.h"
#include "utlvector.h"
#include "saverestore_utlvector.h"
#include "te_effect_dispatch.h"




static void DispatchActivate(CBaseEntity *pEntity)
{
	bool bAsyncAnims = mdlcache->SetAsyncLoad(MDLCACHE_ANIMBLOCK, false);
	pEntity->Activate();
	mdlcache->SetAsyncLoad(MDLCACHE_ANIMBLOCK, bAsyncAnims);
}



LINK_ENTITY_TO_CLASS(env_xen_portal, CXenPortal);

BEGIN_DATADESC(CXenPortal)

DEFINE_KEYFIELD(m_iszNPCClassname, FIELD_STRING, "NPCType"),
DEFINE_KEYFIELD(m_ChildTargetName, FIELD_STRING, "NPCTargetname"),
DEFINE_KEYFIELD(m_SquadName, FIELD_STRING, "NPCSquadName"),
DEFINE_KEYFIELD(m_spawnEquipment, FIELD_STRING, "additionalequipment"),
DEFINE_KEYFIELD(m_strHintGroup, FIELD_STRING, "NPCHintGroup"),
DEFINE_KEYFIELD(m_RelationshipString, FIELD_STRING, "Relationship"),

DEFINE_KEYFIELD(m_PortalData.iSize, FIELD_INTEGER, "size"),
DEFINE_OUTPUT(m_PortalData.m_StartPortal, "OnStartPortal"),
DEFINE_OUTPUT(m_PortalData.m_EndPortal, "OnFinishPortal"),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CXenPortal, DT_XenPortal)

END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CXenPortal::CXenPortal(void)
{
	m_spawnEquipment = NULL_STRING;
	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
}


//-----------------------------------------------------------------------------
// Purpose: Precache the target NPC
//-----------------------------------------------------------------------------
void CXenPortal::Precache(void)
{
	BaseClass::Precache();

	const char *pszNPCName = STRING(m_iszNPCClassname);
	if (!pszNPCName || !pszNPCName[0])
	{
		Warning("env_xen_portal %s has no specified NPC-to-spawn classname.\n", STRING(GetEntityName()));

		switch (m_PortalData.iSize)
		{
		default:
		case 80:
			m_iszNPCClassname = AllocPooledString("npc_alien_slave");
			break;
		case 20:
		case 40:
			m_iszNPCClassname = AllocPooledString("npc_headcrab");
			break;
		case 90:
			m_iszNPCClassname = AllocPooledString("npc_houndeye");
			break;
		case 100:
			m_iszNPCClassname = AllocPooledString("npc_alien_grunt");
			break;
		case 110:
			m_iszNPCClassname = AllocPooledString("npc_bullsquid");
			break;
		}

		const char *pszNPCName = STRING(m_iszNPCClassname);
		UTIL_PrecacheOther(pszNPCName);
	}
	else
	{
		UTIL_PrecacheOther(pszNPCName);
	}

	PrecacheParticleSystem("xen_portal");
	PrecacheParticleSystem("xen_portal_lines");
	
	PrecacheScriptSound("XenPortal.Sound");
}

void CXenPortal::SpawnDelayThink()
{
	CAI_BaseNPC	*pent = (CAI_BaseNPC*)CreateEntityByName(STRING(m_iszNPCClassname));

	if (!pent)
	{
		Warning("NULL Ent in NPCMaker!\n");
		return;
	}

	// ------------------------------------------------
	//  Intialize spawned NPC's relationships
	// ------------------------------------------------
	pent->SetRelationshipString(m_RelationshipString);

	m_OnSpawnNPC.Set(pent, pent, this);

	pent->SetAbsOrigin(GetAbsOrigin());

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles(angles);

	pent->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);

	if (m_spawnflags & SF_NPCMAKER_FADE)
	{
		pent->AddSpawnFlags(SF_NPC_FADE_CORPSE);
	}

	pent->m_spawnEquipment = m_spawnEquipment;
	pent->SetSquadName(m_SquadName);
	pent->SetHintGroup(m_strHintGroup);

	ChildPreSpawn(pent);

	DispatchSpawn(pent);
	pent->SetAbsOrigin(GetAbsOrigin() + (pent->GetAbsOrigin() - pent->WorldSpaceCenter()));
	pent->SetOwnerEntity(this);
	DispatchActivate(pent);

	if (pent->SelectHeaviestSequence(ACT_LAND) != ACT_INVALID)
	{
		pent->SetState(NPC_STATE_ALERT);
		pent->SetSchedule(SCHED_FALL_FROM_PORTAL);
	}

	if (m_ChildTargetName != NULL_STRING)
	{
		// if I have a netname (overloaded), give the child NPC that name as a targetname
		pent->SetName(m_ChildTargetName);
	}

	ChildPostSpawn(pent);

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if (IsDepleted())
		{
			m_OnAllSpawned.FireOutput(this, this);

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink(NULL);
			SetUse(NULL);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Creates the NPC.
//-----------------------------------------------------------------------------
void CXenPortal::MakeNPC(void)
{
	if (!CanMakeNPC(true))
		return;

	SetContextThink(&CXenPortal::SpawnDelayThink, gpGlobals->curtime + 0.6f, s_pSpawnDelayContext);
	//EmitSound("XenPortal.Sound");

	m_PortalData.StartPortal(this);

	m_PortalData.StartPortal(this);

	CEffectData data;

	data.m_vOrigin = GetAbsOrigin();
	data.m_flScale = m_PortalData.iSize;
	data.m_nHitBox = RandomInt(3, 5);
	data.m_flRadius = RandomFloat(200.0f, 300.0f);

	CPVSFilter filter(data.m_vOrigin);

	DispatchEffect("XenPortalIn", data, filter);

	/*EntityMessageBegin(this, true);
		WRITE_BYTE(MESSAGE_START_FIREBALL);
	MessageEnd();*/
}

LINK_ENTITY_TO_CLASS(env_xen_portal_template, CTemplateXenPortal);

BEGIN_SIMPLE_DATADESC(CQSpawn)
DEFINE_FIELD(pAI, FIELD_CLASSPTR),
DEFINE_FIELD(flSpawnTime, FIELD_TIME),
END_DATADESC()


BEGIN_DATADESC(CTemplateXenPortal)

DEFINE_KEYFIELD(m_PortalData.iSize, FIELD_INTEGER, "size"),
DEFINE_OUTPUT(m_PortalData.m_StartPortal, "OnStartPortal"),
DEFINE_OUTPUT(m_PortalData.m_EndPortal, "OnFinishPortal"),

DEFINE_THINKFUNC(QueuedSpawnThink),

DEFINE_UTLVECTOR(m_DelayedSpawns, FIELD_EMBEDDED),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTemplateXenPortal::MakeNPC(void)
{
	// If we should be using the radius spawn method instead, do so
	if (m_flRadius && HasSpawnFlags(SF_NPCMAKER_ALWAYSUSERADIUS))
	{
		MakeNPCInRadius();
		return;
	}

	if (!CanMakeNPC((m_iszDestinationGroup != NULL_STRING)))
		return;

	CNPCSpawnDestination *pDestination = NULL;
	if (m_iszDestinationGroup != NULL_STRING)
	{
		pDestination = FindSpawnDestination();
		if (!pDestination)
		{
			DevMsg(2, "%s '%s' failed to find a valid spawnpoint in destination group: '%s'\n", GetClassname(), STRING(GetEntityName()), STRING(m_iszDestinationGroup));
			return;
		}
	}

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity(pEntity, STRING(m_iszTemplateData), NULL);
	if (pEntity != NULL)
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if (!pent)
	{
		Warning("NULL Ent in NPCMaker!\n");
		return;
	}

	if (pDestination)
	{
		pent->SetAbsOrigin(pDestination->GetAbsOrigin());

		// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
		QAngle angles = pDestination->GetAbsAngles();
		angles.x = 0.0;
		angles.z = 0.0;
		pent->SetAbsAngles(angles);

		pDestination->OnSpawnedNPC(pent);
	}
	else
	{
		pent->SetAbsOrigin(GetAbsOrigin());

		// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
		QAngle angles = GetAbsAngles();
		angles.x = 0.0;
		angles.z = 0.0;
		pent->SetAbsAngles(angles);
	}

	m_OnSpawnNPC.Set(pEntity, pEntity, this);

	if (m_spawnflags & SF_NPCMAKER_FADE)
	{
		pent->AddSpawnFlags(SF_NPC_FADE_CORPSE);
	}

	pent->RemoveSpawnFlags(SF_NPC_TEMPLATE);

	if ((m_spawnflags & SF_NPCMAKER_NO_DROP) == false)
	{
		pent->RemoveSpawnFlags(SF_NPC_FALL_TO_GROUND); // don't fall, slam
	}

	/*ChildPreSpawn(pent);

	DispatchSpawn(pent);
	pent->SetOwnerEntity(this);
	DispatchActivate(pent);

	ChildPostSpawn(pent);*/

	m_PortalData.StartPortal(this);

	CEffectData data;

	data.m_vOrigin = pent->GetAbsOrigin();
	data.m_flScale = m_PortalData.iSize;
	data.m_nHitBox = RandomInt(3, 5);
	data.m_flRadius = RandomFloat(200.0f, 300.0f);

	CPVSFilter filter(data.m_vOrigin);

	DispatchEffect("XenPortalIn", data, filter);

	if (m_DelayedSpawns.Count() <= 0)
		SetContextThink(&CTemplateXenPortal::QueuedSpawnThink, gpGlobals->curtime + 0.1, s_pSpawnDelayContext);

	m_DelayedSpawns.AddToTail(queuedSpawn_t(pent, gpGlobals->curtime + 0.6f));

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if (IsDepleted())
		{
			m_OnAllSpawned.FireOutput(this, this);

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink(NULL);
			SetUse(NULL);
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CTemplateXenPortal::MakeNPCInLine(void)
{
	if (!CanMakeNPC(true))
		return;

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity(pEntity, STRING(m_iszTemplateData), NULL);
	if (pEntity != NULL)
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if (!pent)
	{
		Warning("NULL Ent in NPCMaker!\n");
		return;
	}

	m_OnSpawnNPC.Set(pEntity, pEntity, this);

	PlaceNPCInLine(pent);

	pent->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);

	pent->RemoveSpawnFlags(SF_NPC_TEMPLATE);
	/*ChildPreSpawn(pent);

	DispatchSpawn(pent);
	pent->SetOwnerEntity(this);
	DispatchActivate(pent);

	ChildPostSpawn(pent);*/

	m_PortalData.StartPortal(this);

	CEffectData data;

	data.m_vOrigin = pent->GetAbsOrigin();
	data.m_flScale = m_PortalData.iSize;
	data.m_nHitBox = RandomInt(3, 5);
	data.m_flRadius = RandomFloat(200.0f, 300.0f);

	CPVSFilter filter(data.m_vOrigin);

	DispatchEffect("XenPortalIn", data, filter);

	if (m_DelayedSpawns.Count() <= 0)
		SetContextThink(&CTemplateXenPortal::QueuedSpawnThink, gpGlobals->curtime + 0.1, s_pSpawnDelayContext);

	m_DelayedSpawns.AddToTail(queuedSpawn_t(pent, gpGlobals->curtime + 0.6f));

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if (IsDepleted())
		{
			m_OnAllSpawned.FireOutput(this, this);

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink(NULL);
			SetUse(NULL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Place NPC somewhere on the perimeter of my radius.
//-----------------------------------------------------------------------------
void CTemplateXenPortal::MakeNPCInRadius(void)
{
	if (!CanMakeNPC(true))
		return;

	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity(pEntity, STRING(m_iszTemplateData), NULL);
	if (pEntity != NULL)
	{
		pent = (CAI_BaseNPC *)pEntity;
	}

	if (!pent)
	{
		Warning("NULL Ent in NPCMaker!\n");
		return;
	}

	if (!PlaceNPCInRadius(pent))
	{
		// Failed to place the NPC. Abort
		UTIL_RemoveImmediate(pent);
		return;
	}

	m_OnSpawnNPC.Set(pEntity, pEntity, this);

	pent->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);

	pent->RemoveSpawnFlags(SF_NPC_TEMPLATE);
	/*ChildPreSpawn(pent);

	DispatchSpawn(pent);
	pent->SetOwnerEntity(this);
	DispatchActivate(pent);

	ChildPostSpawn(pent);*/

	m_PortalData.StartPortal(this);

	CEffectData data;

	data.m_vOrigin = pent->GetAbsOrigin();
	data.m_flScale = m_PortalData.iSize;
	data.m_nHitBox = RandomInt(3, 5);
	data.m_flRadius = RandomFloat(200.0f, 300.0f);

	CPVSFilter filter(data.m_vOrigin);

	DispatchEffect("XenPortalIn", data, filter);

	if (m_DelayedSpawns.Count() <= 0)
		SetContextThink(&CTemplateXenPortal::QueuedSpawnThink, gpGlobals->curtime + 0.1, s_pSpawnDelayContext);

	m_DelayedSpawns.AddToTail(queuedSpawn_t(pent, gpGlobals->curtime + 0.6f));

	m_nLiveChildren++;// count this NPC

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		m_nMaxNumNPCs--;

		if (IsDepleted())
		{
			m_OnAllSpawned.FireOutput(this, this);

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink(NULL);
			SetUse(NULL);
		}
	}
}

void CTemplateXenPortal::QueuedSpawnThink()
{
	FOR_EACH_VEC_BACK(m_DelayedSpawns, i)
	{
		queuedSpawn_t *pSpawn = &m_DelayedSpawns.Element(i);
		if (pSpawn->flSpawnTime <= gpGlobals->curtime)
		{
			CAI_BaseNPC *pent = pSpawn->pAI;

			ChildPreSpawn(pent);

			DispatchSpawn(pent);
			pent->SetAbsOrigin(GetAbsOrigin() + (pent->GetAbsOrigin() - pent->WorldSpaceCenter()));
			pent->SetOwnerEntity(this);
			DispatchActivate(pent);

			if (pent->SelectHeaviestSequence(ACT_LAND) != ACT_INVALID)
			{
				pent->SetSchedule(SCHED_FALL_FROM_PORTAL);
			}

			ChildPostSpawn(pent);

			m_DelayedSpawns.Remove(i);
		}
	}

	if (m_DelayedSpawns.Count() > 0)
		SetNextThink(gpGlobals->curtime + 0.1f, s_pSpawnDelayContext);
}