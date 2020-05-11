#include "cbase.h"
#include "ai_squad.h"
#include "bms_utils.h"
#include "sceneentity.h"
#include "npc_basecollegue.h"
#include "saverestore_utlvector.h"
#include "networkstringtable_gamedll.h"

using namespace ResponseRules;

#define ALYX_FEAR_ZOMBIE_DIST_SQR	Square(60)

//-----------------------------------------------------------------------------
// Citizen expressions for the citizen expression types
//-----------------------------------------------------------------------------
#define STATES_WITH_EXPRESSIONS		3		// Idle, Alert, Combat
#define EXPRESSIONS_PER_STATE		1

extern char *szExpressionTypes[COL_EXP_LAST_TYPE]/* =
{
	"Unassigned",
	"Scared",
	"Normal",
	"Angry"
}*/;

struct colleague_expression_list_t
{
	char *szExpressions[EXPRESSIONS_PER_STATE];
};
// Scared
colleague_expression_list_t ScaredExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ "scenes/Expressions/colleague_scared_idle_01.vcd" },
	{ "scenes/Expressions/colleague_scared_alert_01.vcd" },
	{ "scenes/Expressions/colleague_scared_combat_01.vcd" },
};
// Normal
colleague_expression_list_t NormalExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ "scenes/Expressions/colleague_normal_idle_01.vcd" },
	{ "scenes/Expressions/colleague_normal_alert_01.vcd" },
	{ "scenes/Expressions/colleague_normal_combat_01.vcd" },
};
// Angry
colleague_expression_list_t AngryExpressions[STATES_WITH_EXPRESSIONS] =
{
	{ "scenes/Expressions/colleague_angry_idle_01.vcd" },
	{ "scenes/Expressions/colleague_angry_alert_01.vcd" },
	{ "scenes/Expressions/colleague_angry_combat_01.vcd" },
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_BaseColleague)

DEFINE_UTLVECTOR(m_FlexData, FIELD_EMBEDDED),

DEFINE_USEFUNC(UseFunc),
DEFINE_KEYFIELD(m_ExpressionType, FIELD_INTEGER, "expressiontype"),
DEFINE_INPUTFUNC(FIELD_VOID, "EnableFollow", InputEnableFollow),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableFollow", InputDisableFollow),
DEFINE_INPUTFUNC(FIELD_VOID, "CeaseFollowing", InputStopFollow),
DEFINE_INPUTFUNC(FIELD_VOID, "ForceFollowUntilToldNotTo", InputStartFollow),
DEFINE_INPUTFUNC(FIELD_VOID, "EnableGeneralIdles", InputEnableIdleSpeak),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableGeneralIdles", InputDisableIdleSpeak),
END_DATADESC();

void CNPC_BaseColleague::Precache()
{
	BaseClass::Precache();
	SelectExpressionType();

	for (int i = 0; i < STATES_WITH_EXPRESSIONS; i++)
	{
		for (int j = 0; j < ARRAYSIZE(ScaredExpressions[i].szExpressions); j++)
		{
			PrecacheInstancedScene(ScaredExpressions[i].szExpressions[j]);
		}
		for (int j = 0; j < ARRAYSIZE(NormalExpressions[i].szExpressions); j++)
		{
			PrecacheInstancedScene(NormalExpressions[i].szExpressions[j]);
		}
		for (int j = 0; j < ARRAYSIZE(AngryExpressions[i].szExpressions); j++)
		{
			PrecacheInstancedScene(AngryExpressions[i].szExpressions[j]);
		}
	}

	GetCharacterManifest()->PrecacheCharacterModels(GetClassname());

	m_pInstancedResponseSystem = GetAlternateResponseSystem("scripts/talker/response_rules_bms.txt");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_BaseColleague::SelectExpressionType()
{
	// If we've got a mapmaker assigned type, leave it alone
	if (m_ExpressionType != COL_EXP_UNASSIGNED)
		return;

	if (IsPreDisaster())
	{
		m_ExpressionType = COL_EXP_NORMAL;
		return;
	}

	m_ExpressionType = (ColleagueExpressionTypes_t)RandomInt(COL_EXP_SCARED, COL_EXP_ANGRY);
}

//-----------------------------------------------------------------------------
// Purpose: Return a random expression for the specified state to play over 
//			the state's expression loop.
//-----------------------------------------------------------------------------
const char *CNPC_BaseColleague::SelectRandomExpressionForState(NPC_STATE state)
{
	// Hacky remap of NPC states to expression states that we care about
	int iExpressionState = 0;
	switch (state)
	{
	case NPC_STATE_IDLE:
		iExpressionState = 0;
		break;

	case NPC_STATE_ALERT:
		iExpressionState = 1;
		break;

	case NPC_STATE_COMBAT:
		iExpressionState = 2;
		break;

	default:
		// An NPC state we don't have expressions for
		return NULL;
	}

	// Now pick the right one for our expression type
	switch (m_ExpressionType)
	{
	case COL_EXP_SCARED:
	{
		int iRandom = RandomInt(0, ARRAYSIZE(ScaredExpressions[iExpressionState].szExpressions) - 1);
		return ScaredExpressions[iExpressionState].szExpressions[iRandom];
	}

	case COL_EXP_NORMAL:
	{
		int iRandom = RandomInt(0, ARRAYSIZE(NormalExpressions[iExpressionState].szExpressions) - 1);
		return NormalExpressions[iExpressionState].szExpressions[iRandom];
	}

	case COL_EXP_ANGRY:
	{
		int iRandom = RandomInt(0, ARRAYSIZE(AngryExpressions[iExpressionState].szExpressions) - 1);
		return AngryExpressions[iExpressionState].szExpressions[iRandom];
	}

	default:
		break;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Disposition_t CNPC_BaseColleague::IRelationType(CBaseEntity *pTarget)
{
	Disposition_t disposition = BaseClass::IRelationType(pTarget);

	if (pTarget == NULL)
		return disposition;

	//if (pTarget->Classify() == CLASS_ANTLION)
	//{
	//	if (disposition == D_HT)
	//	{
	//		// If Alyx hates this antlion (default relationship), make her fear it, if it is very close.
	//		if (GetAbsOrigin().DistToSqr(pTarget->GetAbsOrigin()) < ALYX_FEAR_ANTLION_DIST_SQR)
	//		{
	//			disposition = D_FR;
	//		}

	//		// Fall through...
	//	}
	//}
	if (pTarget->Classify() == CLASS_ZOMBIE && disposition == D_HT && GetActiveWeapon() && (CapabilitiesGet() & bits_CAP_RANGE_ATTACK_GROUP))
	{
		if (GetAbsOrigin().DistToSqr(pTarget->GetAbsOrigin()) < ALYX_FEAR_ZOMBIE_DIST_SQR)
		{
			// Be afraid of a zombie that's near if I'm not allowed to dodge. This will make Alyx back away.
			return D_FR;
		}
	}
	

	return disposition;
}

void CNPC_BaseColleague::SetupModelFromManifest()
{
	for (int i = 0; i < m_pCharacterDefinition->vBodyGroups.Count(); i++)
	{
		auto& body = m_pCharacterDefinition->vBodyGroups[i];
		int iGroup = FindBodygroupByName(body.strName.String());
		if (iGroup >= 0)
			SetBodygroup(iGroup, body.vValues.Random());
	}

	m_nSkin = m_pCharacterDefinition->vSkins.Random();

	for (int i = 0; i < m_pCharacterDefinition->vMergedModels.Count(); i++)
	{
		CPhysicsProp* pProp = static_cast<CPhysicsProp*>(CreateEntityByName("prop_physics_override"));
		if (pProp != NULL)
		{
			// Set the model
			pProp->SetModelName(AllocPooledString(m_pCharacterDefinition->vMergedModels[i].String()));
			DispatchSpawn(pProp);
			pProp->VPhysicsDestroyObject();
			pProp->FollowEntity(this, true);
			m_AttachedEntities.AddToTail(pProp);
		}
	}

	m_FlexData.Purge();
	m_FlexControllers.Purge();

	for (int i = 0; i < m_pCharacterDefinition->vFlexControllers.Count(); i++)
	{
		m_FlexData.AddToTail(m_pCharacterDefinition->vFlexControllers[i]);
		LocalFlexController_t controller = FindFlexController(m_pCharacterDefinition->vFlexControllers[i].cName);
		m_FlexControllers.AddToTail(controller);
	}

	CUtlBuffer buf;
	buf.SetBigEndian(true);
	CRC32_t crc = CharacterManifest::EncodeFlexVector(m_FlexData, buf);

	if (crc)
	{
		CFmtStr str("%.8x", crc);
		m_nFlexTableIndex = g_pStringTableHeadShapes->AddString(true, str.Access(), buf.TellPut(), buf.Base());
	}
	else
	{
		m_nFlexTableIndex = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_BaseColleague::DrawDebugTextOverlays(void)
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		char tempstr[512];

		Q_snprintf(tempstr, sizeof(tempstr), "Expression type: %s", szExpressionTypes[m_ExpressionType]);
		EntityText(text_offset, tempstr, 0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_BaseColleague::CanJoinPlayerSquad(CBasePlayer *pPlayer)
{
	if (!BaseClass::CanJoinPlayerSquad(pPlayer))
		return false;

	if (IsPreDisaster())
		return false;

	if (HasSpawnFlags(SF_COLLEGUE_DONT_FOLLOW))
		return false;

	return true;
}

bool CNPC_BaseColleague::IgnorePlayerPushing(void)
{
	if (GlobalEntity_GetState("predisaster") == GLOBAL_ON)
		return true;

	return BaseClass::IgnorePlayerPushing();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_BaseColleague::UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	bool bSpoke = false;
	bool bSemaphore = m_bDontUseSemaphore;
	m_bDontUseSemaphore = true;

	if (IsAllowedToSpeak(TLK_VITALIDLE, true))
	{
		AI_Response pResp;
		if (SpeakFindResponse(pResp, TLK_VITALIDLE) && pResp.GetType() != ResponseRules::RESPONSE_NONE)
		{
			bSpoke = SpeakDispatchResponse(TLK_VITALIDLE, &pResp);
		}
	}

	if (!bSpoke)
	{
		SpeakIfAllowed(TLK_USE, (const char *)0, true);		
	}
	m_bDontUseSemaphore = bSemaphore;

	//m_OnPlayerUse.FireOutput(pActivator, pCaller);
}

void CNPC_BaseColleague::GatherConditions()
{
	BaseClass::GatherConditions();

	// Handle speech AI. Don't do AI speech if we're in scripts unless permitted by the EnableSpeakWhileScripting input.
	if (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT ||
		((m_NPCState == NPC_STATE_SCRIPT) && CanSpeakWhileScripting()))
	{
		DoCustomSpeechAI();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override base class activiites
//-----------------------------------------------------------------------------
Activity CNPC_BaseColleague::NPC_TranslateActivity(Activity activity)
{
	/*if (activity == ACT_MELEE_ATTACK1)
	{
		return ACT_MELEE_ATTACK_SWING;
	}*/

	// !!!HACK - Citizens don't have the required animations for shotguns, 
	// so trick them into using the rifle counterparts for now (sjb)
	if (!HaveSequenceForActivity(ACT_RUN_AIM_SHOTGUN))
	{
		if (activity == ACT_RUN_AIM_SHOTGUN)
			return ACT_RUN_AIM_RIFLE;
		if (activity == ACT_WALK_AIM_SHOTGUN)
			return ACT_WALK_AIM_RIFLE;
		if (activity == ACT_IDLE_ANGRY_SHOTGUN)
			return ACT_IDLE_ANGRY_SMG1;
		if (activity == ACT_RANGE_ATTACK_SHOTGUN_LOW)
			return ACT_RANGE_ATTACK_SMG1_LOW;
	}

	return BaseClass::NPC_TranslateActivity(activity);
}



bool CNPC_BaseColleague::CreateBehaviors(void)
{
	AddBehavior(&m_FuncTankBehavior);

	return BaseClass::CreateBehaviors();
}

void CNPC_BaseColleague::OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior)
{
	if (pNewBehavior == &m_FuncTankBehavior)
	{
		m_bReadinessCapable = false;
	}
	else if (pOldBehavior == &m_FuncTankBehavior)
	{
		m_bReadinessCapable = IsReadinessCapable();
	}

	BaseClass::OnChangeRunningBehavior(pOldBehavior, pNewBehavior);
}

void CNPC_BaseColleague::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendCriteria(set);

	if (GlobalEntity_GetIndex("predisaster") == -1)
	{
		set.AppendCriteria("predisaster", "0");
	}

	if (!HasSpawnFlags(SF_COLLEAGUE_NO_IDLE_SPEAK))
		set.AppendCriteria("allowgeneralidles", "1");
	else
		set.AppendCriteria("allowgeneralidles", "0");
	set.AppendCriteria("noquestion", "0");
	set.AppendCriteria("noanswer", "0");
	set.AppendCriteria("provoked", HasMemory(bits_MEMORY_PROVOKED) ? "1" : "0");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CNPC_BaseColleague::SelectIdleSpeech(AISpeechSelection_t *pSelection)
{
	if (!IsOkToSpeak(SPEECH_IDLE))
		return false;

	CBasePlayer *pTarget = assert_cast<CBasePlayer *>(FindSpeechTarget(AIST_PLAYERS | AIST_FACING_TARGET));
	if (pTarget)
	{
		if (SelectSpeechResponse(TLK_HELLO, NULL, pTarget, pSelection))
			return true;

		if (pTarget->WorldSpaceCenter().DistToSqr(WorldSpaceCenter()) < Square(256))
		{
			int chance = (GetTimePlayerStaring() > 3) ? 2 : 10;
			if (ShouldSpeakRandom(TLK_VITALIDLE, chance) && SelectSpeechResponse(TLK_VITALIDLE, NULL, pTarget, pSelection) && pSelection->Response.GetType() != RESPONSE_NONE)
				return true;
		}

		if (GetTimePlayerStaring() > 6 && !IsMoving())
		{
			if (SelectSpeechResponse(TLK_STARE, NULL, pTarget, pSelection))
				return true;
		}

		int chance = (IsMoving()) ? 20 : 2;
		if (ShouldSpeakRandom(TLK_IDLE, chance) && SelectSpeechResponse(TLK_IDLE, NULL, pTarget, pSelection))
			return true;
	}
	return false;
}

void CNPC_BaseColleague::OnRestore()
{
	BaseClass::OnRestore();

	m_FlexControllers.Purge();
	for (int i = 0; i < m_FlexData.Count(); i++)
	{
		LocalFlexController_t controller = FindFlexController(m_FlexData[i].cName);
		m_FlexControllers.AddToTail(controller);
	}

	CUtlBuffer buf;
	buf.SetBigEndian(true);
	CRC32_t crc = CharacterManifest::EncodeFlexVector(m_FlexData, buf);

	if (crc)
	{
		CFmtStr str("%.8x", crc);
		m_nFlexTableIndex = g_pStringTableHeadShapes->AddString(true, str.Access(), buf.TellPut(), buf.Base());
	}
	else
	{
		m_nFlexTableIndex = -1;
	}
}

void CNPC_BaseColleague::InputEnableFollow(inputdata_t &inputdata)
{
	RemoveSpawnFlags(SF_COLLEGUE_DONT_FOLLOW);
}

void CNPC_BaseColleague::InputDisableFollow(inputdata_t &inputdata)
{
	InputStopFollow(inputdata);

	AddSpawnFlags(SF_COLLEGUE_DONT_FOLLOW);
}

void CNPC_BaseColleague::InputStopFollow(inputdata_t &inputdata)
{
	if (IsInPlayerSquad())
	{
		RemoveFromPlayerSquad();
		SpeakIfAllowed(TLK_STOPFOLLOW);
	}
}

void CNPC_BaseColleague::InputStartFollow(inputdata_t &inputdata)
{
	RemoveSpawnFlags(SF_COLLEGUE_DONT_FOLLOW);

	if (!IsInPlayerSquad())
	{
		AddToPlayerSquad(GetBestPlayer());
		SpeakIfAllowed(TLK_STARTFOLLOW);
	}
}

void CNPC_BaseColleague::InputEnableIdleSpeak(inputdata_t &inputdata)
{
	RemoveSpawnFlags(SF_COLLEAGUE_NO_IDLE_SPEAK);
}

void CNPC_BaseColleague::InputDisableIdleSpeak(inputdata_t &inputdata)
{
	AddSpawnFlags(SF_COLLEAGUE_NO_IDLE_SPEAK);
}