//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dr. Kleiner, a suave ladies man leading the fight against the evil 
//			combine while constantly having to help his idiot assistant Gordon
//
//=============================================================================//


//-----------------------------------------------------------------------------
// Generic NPC - purely for scripted sequence work.
//-----------------------------------------------------------------------------
#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include "ai_baseactor.h"
#include "props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int AE_ODELL_WELDER_ATTACHMENT;

//-----------------------------------------------------------------------------
// NPC's Anim Events Go Here
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Odell : public CAI_BaseActor
{
public:
	DECLARE_CLASS(CNPC_Odell, CAI_BaseActor);
	DECLARE_DATADESC();

	void	Spawn(void);
	void	Precache(void);
	Class_T Classify(void);
	void	HandleAnimEvent(animevent_t *pEvent);
	int		GetSoundInterests(void);

	DEFINE_CUSTOM_AI;

private:
	void CreateEmpTool(void);
	EHANDLE	m_hEmpTool;
	bool m_bShouldHaveEMP;
};

LINK_ENTITY_TO_CLASS(npc_odell, CNPC_Odell);
//LINK_ENTITY_TO_CLASS(npc_human_scientist_kleiner, CNPC_Odell);

BEGIN_DATADESC(CNPC_Odell)
DEFINE_FIELD(m_hEmpTool, FIELD_EHANDLE),
DEFINE_KEYFIELD(m_bShouldHaveEMP, FIELD_BOOLEAN, "ShouldHaveWelder"),
END_DATADESC();

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Odell::Classify(void)
{
	return	CLASS_PLAYER_ALLY_VITAL;
}


//-----------------------------------------------------------------------------
// Purpose: Create and initialized Alyx's EMP tool
//-----------------------------------------------------------------------------

void CNPC_Odell::CreateEmpTool(void)
{
	if (!m_bShouldHaveEMP || m_hEmpTool)
		return;

	m_hEmpTool = (CBaseAnimating*)CreateEntityByName("prop_dynamic");
	if (m_hEmpTool)
	{
		m_hEmpTool->SetModel("models/props_scart/welding_torch.mdl");
		m_hEmpTool->SetName(AllocPooledString("odell_weldertool"));
		int iAttachment = LookupAttachment("Welder_Holster");
		m_hEmpTool->SetParent(this, iAttachment);
		m_hEmpTool->SetOwnerEntity(this);
		m_hEmpTool->SetSolid(SOLID_NONE);
		m_hEmpTool->SetLocalOrigin(Vector(0, 0, 0));
		m_hEmpTool->SetLocalAngles(QAngle(0, 0, 0));
		m_hEmpTool->AddSpawnFlags(SF_DYNAMICPROP_NO_VPHYSICS);
		m_hEmpTool->AddEffects(EF_PARENT_ANIMATES);
		m_hEmpTool->Spawn();
	}
}

//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Odell::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->event == AE_ODELL_WELDER_ATTACHMENT)
	{
		if (!m_hEmpTool)
		{
			// Old savegame?
			CreateEmpTool();
			if (!m_hEmpTool)
				return;
		}

		int iAttachment = LookupAttachment(pEvent->options);
		m_hEmpTool->SetParent(this, iAttachment);
		

		if (!stricmp(pEvent->options, "Welder_Holster"))
		{
			m_hEmpTool->SetLocalOrigin(Vector(0, 0, 0));
			m_hEmpTool->SetLocalAngles(QAngle(0, 0, 0));
		}
		else
		{
			m_hEmpTool->SetLocalOrigin(Vector(0, 0, 0));
			m_hEmpTool->SetLocalAngles(QAngle(90, 0, 0));
		}

		return;
	}

	switch (pEvent->event)
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_Odell::GetSoundInterests(void)
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Odell::Spawn()
{
	// Allow custom model usage (mostly for monitors)
	char *szModel = (char *)STRING(GetModelName());
	if (!szModel || !*szModel)
	{
		szModel = "models/odell.mdl";
		SetModelName(AllocPooledString(szModel));
	}

	Precache();
	SetModel(szModel);

	BaseClass::Spawn();

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	SetBloodColor(BLOOD_COLOR_RED);
	m_iHealth = 20;
	m_flFieldOfView = 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD);
	CapabilitiesAdd(bits_CAP_FRIENDLY_DMG_IMMUNE);

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	CreateEmpTool();

	NPCInit();
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Odell::Precache()
{
	PrecacheModel(STRING(GetModelName()));
	PrecacheModel("models/props_scart/welding_torch.mdl");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC(npc_odell, CNPC_Odell)

DECLARE_ANIMEVENT(AE_ODELL_WELDER_ATTACHMENT);

AI_END_CUSTOM_NPC();