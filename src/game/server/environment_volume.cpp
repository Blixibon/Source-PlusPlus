#include "cbase.h"
#include "environment_volume.h"
#include "fmtstr.h"
#include "collisionutils.h"
#include "filters.h"


CUtlVector< CEnvVolume* > TheEnvVolumes;
ConVar fog_volume_debug("fog_volume_debug", "0", FCVAR_CHEAT, "If enabled, prints diagnostic information about the current fog volume.\n");

//--------------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_tonemap, CEnvVolume);
LINK_ENTITY_TO_CLASS(fog_volume, CEnvVolume);
LINK_ENTITY_TO_CLASS(trigger_fog, CEnvVolume);
LINK_ENTITY_TO_CLASS(trigger_fxvolume, CEnvVolume);

BEGIN_DATADESC(CEnvVolume)
DEFINE_KEYFIELD(m_tonemapControllerName, FIELD_STRING, "TonemapName"),
DEFINE_KEYFIELD(m_fogName, FIELD_STRING, "FogName"),
DEFINE_KEYFIELD(m_colorCorrectionName, FIELD_STRING, "ColorCorrectionName"),

DEFINE_KEYFIELD(m_fog.colorPrimary, FIELD_COLOR32, "fogcolor"),
DEFINE_KEYFIELD(m_fog.colorSecondary, FIELD_COLOR32, "fogcolor2"),
DEFINE_KEYFIELD(m_fog.dirPrimary, FIELD_VECTOR, "fogdir"),
DEFINE_KEYFIELD(m_fog.enable, FIELD_BOOLEAN, "fogenable"),
DEFINE_KEYFIELD(m_fog.blend, FIELD_BOOLEAN, "fogblend"),
DEFINE_KEYFIELD(m_fog.start, FIELD_FLOAT, "fogstart"),
DEFINE_KEYFIELD(m_fog.end, FIELD_FLOAT, "fogend"),
DEFINE_KEYFIELD(m_fog.farz, FIELD_FLOAT, "farz"),

DEFINE_FIELD(m_hFogController, FIELD_EHANDLE),
DEFINE_FIELD(m_hColorCorrectionController, FIELD_EHANDLE),
DEFINE_FIELD(m_hTonemapController, FIELD_EHANDLE),
END_DATADESC();

//--------------------------------------------------------------------------------------------------------
CEnvVolume::CEnvVolume() :
	BaseClass(),
	m_bInFogVolumesList(false)
{
}


//--------------------------------------------------------------------------------------------------------
CEnvVolume::~CEnvVolume()
{
	RemoveFromGlobalList();
}

//--------------------------------------------------------------------------------------------------------
CEnvVolume* CEnvVolume::FindEnvVolumeForPosition(const Vector& position, CBaseEntity* pLooker)
{
	CEnvVolume* fogVolume = NULL;
	for (int i = 0; i < TheEnvVolumes.Count(); ++i)
	{
		fogVolume = TheEnvVolumes[i];

		Vector vecRelativeCenter;
		fogVolume->CollisionProp()->WorldToCollisionSpace(position, &vecRelativeCenter);
		if (IsBoxIntersectingSphere(fogVolume->CollisionProp()->OBBMins(), fogVolume->CollisionProp()->OBBMaxs(), vecRelativeCenter, 1.0f) && (!pLooker || !fogVolume->UsesFilter() || fogVolume->m_hFilter->PassesFilter(fogVolume, pLooker)))
		{
			break;
		}
		fogVolume = NULL;
	}

	// This doesn't work well if there are multiple players or multiple fog volume queries per frame; might want to relocate this if that's the case
	if (fog_volume_debug.GetBool())
	{
		if (fogVolume)
		{
			char fogVolumeName[256];
			fogVolume->GetKeyValue("targetname", fogVolumeName, 256);
			engine->Con_NPrintf(0, "Fog Volume \"%s\" found at position (%f %f %f)", fogVolumeName, position.x, position.y, position.z);
			engine->Con_NPrintf(1, "Fog: %s, color correct: %s, tonemap control: %s", fogVolume->m_fogName.ToCStr(), fogVolume->m_colorCorrectionName.ToCStr(), fogVolume->m_tonemapControllerName.ToCStr());
		}
		else
		{
			engine->Con_NPrintf(0, "No Fog Volume found at given position (%f %f %f)", position.x, position.y, position.z);
		}
	}

	return fogVolume;
}

//--------------------------------------------------------------------------------------------------------
void CEnvVolume::Spawn(void)
{
	AddSpawnFlags(SF_TRIGGER_ALLOW_ALL);

	BaseClass::Spawn();
	InitTrigger();
}

//--------------------------------------------------------------------------------------------------------
void CEnvVolume::StartTouch(CBaseEntity* other)
{
	if (!PassesTriggerFilters(other))
		return;

	BaseClass::StartTouch(other);

	CBaseCombatCharacter* character = other->MyCombatCharacterPointer();
	if (character)
		character->OnFogTriggerStartTouch(this);

	CBasePlayer* player = ToBasePlayer(other);
	if (player)
		player->OnTonemapTriggerStartTouch(this);
}


//--------------------------------------------------------------------------------------------------------
void CEnvVolume::EndTouch(CBaseEntity* other)
{
	if (!PassesTriggerFilters(other))
		return;

	BaseClass::EndTouch(other);

	CBaseCombatCharacter* character = other->MyCombatCharacterPointer();
	if (character)
		character->OnFogTriggerEndTouch(this);

	CBasePlayer* player = ToBasePlayer(other);
	if (player)
		player->OnTonemapTriggerEndTouch(this);
}

//--------------------------------------------------------------------------------------------------------
void CEnvVolume::AddToGlobalList()
{
	if (!m_bInFogVolumesList)
	{
		TheEnvVolumes.AddToTail(this);
		m_bInFogVolumesList = true;
	}
}


//--------------------------------------------------------------------------------------------------------
void CEnvVolume::RemoveFromGlobalList()
{
	if (m_bInFogVolumesList)
	{
		TheEnvVolumes.FindAndRemove(this);
		m_bInFogVolumesList = false;
	}
}


//----------------------------------------------------------------------------
void CEnvVolume::Enable()
{
	BaseClass::Enable();
	AddToGlobalList();
}


//----------------------------------------------------------------------------
void CEnvVolume::Disable()
{
	BaseClass::Disable();
	RemoveFromGlobalList();
}


//----------------------------------------------------------------------------
// Called when the level loads or is restored
//----------------------------------------------------------------------------
void CEnvVolume::Activate()
{
	BaseClass::Activate();

	if (m_fogName == NULL_STRING || FStrEq(STRING(m_fogName), ""))
	{
		m_hFogController = static_cast<CFogController*>(CreateNoSpawn("env_fog_controller", GetAbsOrigin(), vec3_angle));
		m_hFogController->m_fog.colorPrimary = m_fog.colorPrimary;
		m_hFogController->m_fog.colorSecondary = m_fog.colorSecondary;
		m_hFogController->m_fog.dirPrimary = m_fog.dirPrimary;
		m_hFogController->m_fog.enable = m_fog.enable;
		m_hFogController->m_fog.blend = m_fog.blend;
		m_hFogController->m_fog.start = m_fog.start;
		m_hFogController->m_fog.end = m_fog.end;
		m_hFogController->m_fog.farz = m_fog.farz;

		CFmtStr str("%s_%i_autofogcontroller", GetDebugName(), entindex());
		m_fogName = AllocPooledString(str.Get());
		m_hFogController->SetName(m_fogName);

		DispatchSpawn(m_hFogController);
	}
	else
	{
		m_hFogController = dynamic_cast<CFogController*>(gEntList.FindEntityByName(NULL, m_fogName));
		m_fog = m_hFogController->m_fog;
	}
	m_hColorCorrectionController = dynamic_cast<CColorCorrection*>(gEntList.FindEntityByName(NULL, m_colorCorrectionName));
	m_hTonemapController = gEntList.FindEntityByName(NULL, m_tonemapControllerName);

	if (!m_bDisabled)
	{
		AddToGlobalList();
	}
}