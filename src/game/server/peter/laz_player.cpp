#include "cbase.h"
#include "laz_player.h"
#include "iservervehicle.h"
#include "gametypes.h"
#include "grenade_frag.h"
#include "in_buttons.h"
#include "gamestats.h"
#include "ammodef.h"
#include "bms_utils.h"
#include "ai_basenpc.h"
#include "peter\player_models.h"
#include "mp_shareddefs.h"
#include "sceneentity.h"
#include "vehicle_jeep.h"
#include "saverestore_stringtable.h"
#include "networkstringtable_gamedll.h"
#include "datacache\imdlcache.h"
#include "animation.h"
#include "lazuul_gamerules.h"
#include "team.h"
#include "gamevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define HL2MP_COMMAND_MAX_RATE 0.3

ConVar sv_forcedspecialattack("sv_laz_forcedspecial", "-1", FCVAR_CHEAT);

CStringTableSaveRestoreOps g_FootStepStringOps;

const char *g_pszSpecialAttacks[SPECIAL_ATTACK_COUNT] = {
	"manhack"
};

LINK_ENTITY_TO_CLASS(player, CLaz_Player);

BEGIN_DATADESC(CLaz_Player)
//DEFINE_SOUNDPATCH(m_pWooshSound),
DEFINE_FIELD(m_bHasLongJump, FIELD_BOOLEAN),

DEFINE_FIELD(m_flNextPainSoundTime, FIELD_TIME),
DEFINE_FIELD(m_iszVoiceType, FIELD_STRING),
DEFINE_FIELD(m_iszSuitVoice, FIELD_STRING),

DEFINE_CUSTOM_FIELD(m_iPlayerSoundType, &g_FootStepStringOps),
END_DATADESC();

IMPLEMENT_SERVERCLASS_ST(CLaz_Player, DT_Laz_Player)
SendPropInt(SENDINFO(m_bHasLongJump), 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iPlayerSoundType), MAX_FOOTSTEP_STRING_BITS + 1),
END_SEND_TABLE();

#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

void CLaz_Player::Precache(void)
{
	BaseClass::Precache();

	PrecacheFootStepSounds();
}

void CLaz_Player::Spawn(void)
{
	m_iszVoiceType = AllocPooledString(DEFAULT_VOICE);

	BaseClass::Spawn();

	if (HasMPModel())
	{
		rndModel_t *variant = &m_MPModel.models.Random();
		SetModel(variant->szModelName);
		m_nSkin = variant->skin;
		for (int i = 0; i < variant->bodygroups.Count(); i++)
		{
			int iGroup = FindBodygroupByName(variant->bodygroups[i].szName);
			SetBodygroup(iGroup, variant->bodygroups[i].body);
		}

		for (int i = 0; i < MAX_VIEWMODELS; i++)
		{
			CBaseViewModel* pHands = GetViewModel(i);
			if (pHands)
			{
				pHands->SetHandsModel(m_MPModel.szArmModel, m_MPModel.armSkin);

				for (int i = 0; i < m_MPModel.armbodys.Count(); i++)
				{
					pHands->SetHandsBodygroupByName(m_MPModel.armbodys[i].szName, m_MPModel.armbodys[i].body);
				}
			}
		}

		KeyValues* pkvAbillites = m_MPModel.kvAbilities;
		if (pkvAbillites != nullptr)
		{
			const char* pchVoice = pkvAbillites->GetString("voice", DEFAULT_VOICE);
			const char* pchSuit = pkvAbillites->GetString("suit", DEFAULT_VOICE);
			SetVoiceType(pchVoice, pchSuit);

			const char* pchFootSound = pkvAbillites->GetString("footsteps", DEFAULT_FEET);
			SetFootsteps(pchFootSound);

			m_nSpecialAttack = UTIL_StringFieldToInt(pkvAbillites->GetString("special"), g_pszSpecialAttacks, SPECIAL_ATTACK_COUNT);
		}
		else
		{
			SetVoiceType(DEFAULT_VOICE, DEFAULT_VOICE);
			SetFootsteps(DEFAULT_FEET);
			m_nSpecialAttack = -1;
		}
	}
	else
	{
		m_nSpecialAttack = -1;
	}

	if (!IsObserver())
	{
		pl.deadflag = false;
		RemoveSolidFlags(FSOLID_NOT_SOLID);

		RemoveEffects(EF_NODRAW);
	}

	if (!g_pGameRules->IsMultiplayer())
		ChangeTeam(GetAutoTeam());
}

void CLaz_Player::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if (HasMPModel())
	{
		PlayerModelSystem()->PlayerReleaseModel(m_MPModel.szSectionID);
		m_MPModel = { 0 };
	}
}

void CLaz_Player::PreThink(void)
{
	BaseClass::PreThink();
	State_PreThink();
}

void CLaz_Player::Event_KilledOther(CBaseEntity * pVictim, const CTakeDamageInfo & info)
{
	BaseClass::Event_KilledOther(pVictim, info);

	// No taunts after killing teammates.
	if (g_pGameRules->PlayerRelationship(this, pVictim) == GR_TEAMMATE)
		return;

	if (pVictim->IsNPC())
	{
		CFmtStrN<128> modifiers("playerenemy:%s", pVictim->GetClassname());
		SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_TAUNT, modifiers);
	}
}

void CLaz_Player::Event_Killed(const CTakeDamageInfo & info)
{
	BaseClass::Event_Killed(info);

	StartObserverMode(OBS_MODE_DEATHCAM);
}

void CLaz_Player::ChangeTeam(int iTeam)
{
	/*	if ( GetNextTeamChangeTime() >= gpGlobals->curtime )
		{
			char szReturnString[128];
			Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch teams again.\n", (int)(GetNextTeamChangeTime() - gpGlobals->curtime) );

			ClientPrint( this, HUD_PRINTTALK, szReturnString );
			return;
		}*/

	bool bKill = false;

	if (g_pGameRules->IsMultiplayer() == true)
	{
		// In co-op mode, you can be a spectator or a protaganist.
		if (LazuulRules()->GetGameMode() == LAZ_GM_COOP && iTeam != TEAM_SPECTATOR)
		{
			if (GetTeamNumber() != iTeam)
				iTeam = LazuulRules()->GetProtaganistTeam();
		}

		if (iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED)
		{
			bKill = true;
		}
	}

	BaseClass::ChangeTeam(iTeam);

	//m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	if (iTeam == TEAM_SPECTATOR)
	{
		RemoveAllItems(true);

		State_Transition(STATE_OBSERVER_MODE);
	}

	if (bKill == true)
	{
		CommitSuicide();
	}

	SetPlayerModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CLaz_Player::GetAutoTeam(void)
{
	if (!LazuulRules()->IsDeathmatch())
		return LazuulRules()->GetProtaganistTeam();

	int iTeam = TEAM_SPECTATOR;

	CTeam *pBlue = GetGlobalTeam(TF_TEAM_BLUE);
	CTeam *pRed = GetGlobalTeam(TF_TEAM_RED);

	{
		if (pBlue && pRed)
		{
			if (pBlue->GetNumPlayers() < pRed->GetNumPlayers())
			{
				iTeam = TF_TEAM_BLUE;
			}
			else if (pRed->GetNumPlayers() < pBlue->GetNumPlayers())
			{
				iTeam = TF_TEAM_RED;
			}
			else
			{
				iTeam = RandomInt(0, 1) ? TF_TEAM_RED : TF_TEAM_BLUE;
			}
		}
	}

	return iTeam;
}

bool CLaz_Player::HandleCommand_JoinTeam(int team)
{
	if (team == TF_TEAM_AUTOASSIGN)
		team = GetAutoTeam();

	if (!GetGlobalTeam(team) || team == 0)
	{
		Warning("HandleCommand_JoinTeam( %d ) - invalid team index.\n", team);
		return false;
	}

	if (team == TEAM_SPECTATOR)
	{
		// Prevent this is the cvar is set
		if (!mp_allowspectators.GetInt())
		{
			ClientPrint(this, HUD_PRINTCENTER, "#Cannot_Be_Spectator");
			return false;
		}

		if (GetTeamNumber() != TEAM_UNASSIGNED && !IsDead())
		{
			m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work

			CommitSuicide();

			// add 1 to frags to balance out the 1 subtracted for killing yourself
			IncrementFragCount(1);
		}

		ChangeTeam(TEAM_SPECTATOR);

		return true;
	}
	else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}

	// Switch their actual team...
	ChangeTeam(team);

	return true;
}

void CLaz_Player::SetPlayerModel(void)
{
	if (!g_pGameRules->IsMultiplayer())
		return;

	if (HasMPModel())
	{
		PlayerModelSystem()->PlayerReleaseModel(m_MPModel.szSectionID);
		m_MPModel = { 0 };
	}

	CUtlVector<playerModel_t> models = PlayerModelSystem()->GetAvailableModelsForTeam(TeamID());

	if (!models.Count())
		return;

	const char *pszModel = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "cl_playermodel");
	bool bFound = false;
	int i = 0;
	for (i = 0; i < models.Count(); i++)
	{
		if (FStrEq(pszModel, models[i].szSectionID))
		{
			bFound = true;
			break;
		}
	}

	if (bFound)
	{
		m_MPModel = models[i];
	}
	else
	{
		m_MPModel = models.Random();
	}
}

bool CLaz_Player::ClientCommand(const CCommand &args)
{
	if (FStrEq(args[0], "spectate"))
	{
		if (ShouldRunRateLimitedCommand(args))
		{
			// instantly join spectators
			HandleCommand_JoinTeam(TEAM_SPECTATOR);
		}
		return true;
	}
	else if (FStrEq(args[0], "jointeam"))
	{
		if (args.ArgC() < 2)
		{
			Warning("Player sent bad jointeam syntax\n");
		}

		if (ShouldRunRateLimitedCommand(args))
		{
			int iTeam = atoi(args[1]);
			HandleCommand_JoinTeam(iTeam);
		}
		return true;
	}
	else if (FStrEq(args[0], "joingame"))
	{
		if (ShouldRunRateLimitedCommand(args) && GetTeamNumber() < FIRST_GAME_TEAM)
		{
			int iTeam = GetAutoTeam();
			HandleCommand_JoinTeam(iTeam);
		}
		return true;
	}
	else if (FStrEq(args[0], "special_attack"))
	{
		Special();
		return true;
	}

	return BaseClass::ClientCommand(args);
}

bool CLaz_Player::ShouldRunRateLimitedCommand(const CCommand &args)
{
	int i = m_RateLimitLastCommandTimes.Find(args[0]);
	if (i == m_RateLimitLastCommandTimes.InvalidIndex())
	{
		m_RateLimitLastCommandTimes.Insert(args[0], gpGlobals->curtime);
		return true;
	}
	else if ((gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HL2MP_COMMAND_MAX_RATE)
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLaz_Player::SetObserverMode(int mode)
{
	if (mode < OBS_MODE_NONE || mode >= NUM_OBSERVER_MODES)
		return false;

	// Skip OBS_MODE_POI as we're not using that.
	if (mode == OBS_MODE_POI)
	{
		mode++;
	}

	// Skip over OBS_MODE_ROAMING for dead players
	if (GetTeamNumber() > TEAM_SPECTATOR)
	{
		if (IsDead() && (mode > OBS_MODE_FIXED) && mp_fadetoblack.GetBool())
		{
			mode = OBS_MODE_CHASE;
		}
		else if (mode == OBS_MODE_ROAMING)
		{
			mode = OBS_MODE_IN_EYE;
		}
	}

	if (m_iObserverMode > OBS_MODE_DEATHCAM)
	{
		// remember mode if we were really spectating before
		m_iObserverLastMode = m_iObserverMode;
	}

	m_iObserverMode = mode;
	//m_flLastAction = gpGlobals->curtime;

	switch (mode)
	{
	case OBS_MODE_NONE:
	case OBS_MODE_FIXED:
	case OBS_MODE_DEATHCAM:
		SetFOV(this, 0);	// Reset FOV
		SetViewOffset(vec3_origin);
		SetMoveType(MOVETYPE_NONE);
		break;

	case OBS_MODE_CHASE:
	case OBS_MODE_IN_EYE:
		// udpate FOV and viewmodels
		SetObserverTarget(m_hObserverTarget);
		SetMoveType(MOVETYPE_OBSERVER);
		break;

	case OBS_MODE_ROAMING:
		SetFOV(this, 0);	// Reset FOV
		SetObserverTarget(m_hObserverTarget);
		SetViewOffset(vec3_origin);
		SetMoveType(MOVETYPE_OBSERVER);
		break;

	case OBS_MODE_FREEZECAM:
		SetFOV(this, 0);	// Reset FOV
		SetObserverTarget(m_hObserverTarget);
		SetViewOffset(vec3_origin);
		SetMoveType(MOVETYPE_OBSERVER);
		break;
	}

	CheckObserverSettings();

	return true;
}

void CLaz_Player::PainSound(const CTakeDamageInfo & info)
{
	// Don't make sounds if we just died. DeathSound will handle that.
	if (!IsAlive())
		return;

	if (m_flNextPainSoundTime > gpGlobals->curtime)
		return;

	// Don't play falling pain sounds, they have their own system
	if (info.GetDamageType() & DMG_FALL)
		return;

	if (info.GetDamageType() & DMG_DROWN)
	{
		//EmitSound("TFPlayer.Drown");
		return;
	}

	if (info.GetDamageType() & DMG_BURN)
	{
		// Looping fire pain sound is done in CTFPlayerShared::ConditionThink
		return;
	}

	float flPainLength = 0;

	//bool bAttackerIsPlayer = (info.GetAttacker() && info.GetAttacker()->IsPlayer());

	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert(pExpresser);

	pExpresser->AllowMultipleScenes();

	// speak a pain concept here, send to everyone but the attacker
	CPASFilter filter(GetAbsOrigin());

	/*if (bAttackerIsPlayer)
	{
		filter.RemoveRecipient(ToBasePlayer(info.GetAttacker()));
	}*/



	const char *pszHitLocCriterion = "shotloc:none";

	if (LastHitGroup() == HITGROUP_LEFTLEG || LastHitGroup() == HITGROUP_RIGHTLEG)
	{
		pszHitLocCriterion = "shotloc:leg";
	}
	else if (LastHitGroup() == HITGROUP_LEFTARM || LastHitGroup() == HITGROUP_RIGHTARM)
	{
		pszHitLocCriterion = "shotloc:arm";
	}
	else if (LastHitGroup() == HITGROUP_STOMACH)
	{
		pszHitLocCriterion = "shotloc:gut";
	}

	// set up the speech modifiers
	CFmtStrN<128> modifiers("%s,damageammo:%s", pszHitLocCriterion, info.GetAmmoName());

	if (SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_PAIN, modifiers, NULL, 0, &filter))
	{
		flPainLength = max(pExpresser->GetTimeSpeechComplete() - gpGlobals->curtime, flPainLength);
	}

	/*AI_Response *pResp = SpeakConcept(MP_CONCEPT_PLAYER_PAIN);
	if (pResp)
	{
		pExpresser->SpeakDispatchResponse(g_pszMPConcepts[MP_CONCEPT_PLAYER_PAIN], pResp, &filter);
		flPainLength = max(pExpresser->GetTimeSpeechComplete() - gpGlobals->curtime, flPainLength);
	}*/

	// speak a louder pain concept to just the attacker
	/*if (bAttackerIsPlayer)
	{
		CSingleUserRecipientFilter attackerFilter(ToBasePlayer(info.GetAttacker()));
		SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_ATTACKER_PAIN, "damagecritical:1", szResponse, AI_Response::MAX_RESPONSE_NAME, &attackerFilter);
	}*/

	pExpresser->DisallowMultipleScenes();

	m_flNextPainSoundTime = gpGlobals->curtime + flPainLength;
}

void CLaz_Player::Special()
{
	if (gpGlobals->curtime < m_flNextSpecialAttackTime)
		return;

	int iSpecial = m_nSpecialAttack;
	if (sv_forcedspecialattack.GetInt() >= 0)
		iSpecial = sv_forcedspecialattack.GetInt();

	switch (iSpecial)
	{
	case LAZ_SPECIAL_MANHACK:
	{
		int iSequence = SelectWeightedSequence(ACT_METROPOLICE_DEPLOY_MANHACK);
		if (iSequence != ACT_INVALID)
		{
			m_flNextSpecialAttackTime = gpGlobals->curtime + SequenceDuration(iSequence) + 2.0f;
			SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_CAST_MONOCULOUS);
			DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_SEQUENCE, iSequence);
		}
	}
		break;
	default:
		break;
	}
}

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CLaz_Player::SetSuitUpdate(const char *name, int fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;
	bool bGroup = true;


	// Ignore suit updates if no suit
	if (!IsSuitEquipped())
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}


	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}

	if (m_iszSuitVoice == AllocPooledString(DEFAULT_VOICE))
		return;

	if (name[0] == AI_SP_SPECIFIC_SENTENCE)
	{
		name++;
		bGroup = false;
	}

	name += 3;

	char response[256];

	CFmtStr str;

	if (!bGroup)
	{
		str.Append(AI_SP_SPECIFIC_SENTENCE);
	}

	str.AppendFormat("%s%s", STRING(m_iszSuitVoice), name);

	Q_strcpy(response, str);

	// get sentence or group number
	if (response[0] == AI_SP_SPECIFIC_SENTENCE)
	{
		isentence = SENTENCEG_Lookup(response);	// Lookup sentence index (not group) by name
		if (isentence < 0)
			return;
	}
	else
	{
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(response);		// Lookup group index by name
	}



	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
		{
			// this sentence or group is already in 
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->curtime)
			{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
			}
			else
			{
				// don't play, still marked as norepeat
				return;
			}
		}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = random->RandomInt(0, CSUITNOREPEAT - 1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->curtime;
	}

	// find empty spot in queue, or overwrite last spot

	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->curtime)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->curtime + SUITFIRSTUPDATETIME;
		else
			m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME;
	}

}

Class_T CLaz_Player::Classify()
{
	if (GetTeamNumber() == TEAM_COMBINE)
		return CLASS_COMBINE_PLAYER;

	return CLASS_PLAYER;
}

int CLaz_Player::OnTakeDamage(const CTakeDamageInfo & inputInfo)
{
	CTakeDamageInfo inputInfoCopy(inputInfo);


	CBaseEntity *pAttacker = inputInfoCopy.GetAttacker();
	CBaseEntity *pInflictor = inputInfoCopy.GetInflictor();

	// Refuse damage from prop_glados_core.
	if ((pAttacker && FClassnameIs(pAttacker, "prop_glados_core")) ||
		(pInflictor && FClassnameIs(pInflictor, "prop_glados_core")))
	{
		inputInfoCopy.SetDamage(0.0f);
	}

	bool bTookDamage = (BaseClass::OnTakeDamage(inputInfoCopy) != 0);

	// Early out if the base class took no damage
	if (!bTookDamage)
	{
		/*if (bDebug)
		{
		Warning("    ABORTED: Player failed to take the damage.\n");
		}*/
		return 0;
	}

	if (IsAlive())
	{
		PlayFlinch(inputInfoCopy);
		PainSound(inputInfoCopy);
	}

	return bTookDamage;
}

void CLaz_Player::PlayFlinch(const CTakeDamageInfo & info)
{
	// Don't play flinches if we just died. 
	if (!IsAlive())
		return;

	PlayerAnimEvent_t flinchAct;

	Vector forward;
	AngleVectors(GetLocalAngles(), &forward);
	float flDot = -DotProduct(forward, g_vecAttackDir);

	switch (LastHitGroup())
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchAct = PLAYERANIMEVENT_FLINCH_HEAD;
		break;
	case HITGROUP_LEFTARM:
		flinchAct = PLAYERANIMEVENT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchAct = PLAYERANIMEVENT_FLINCH_RIGHTARM;
		break;
		case HITGROUP_LEFTLEG:
			flinchAct = PLAYERANIMEVENT_FLINCH_LEFTLEG;
			break;
		case HITGROUP_RIGHTLEG:
			flinchAct = PLAYERANIMEVENT_FLINCH_RIGHTLEG;
			break;
	case HITGROUP_STOMACH:
	case HITGROUP_CHEST:
		flinchAct = PLAYERANIMEVENT_FLINCH_CHEST;
		break;
	case HITGROUP_GEAR:
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchAct = PLAYERANIMEVENT_FLINCH_CHEST;
		break;
	}

	int iSubType = 0;
	if (flDot < 0.1f)
	{
		iSubType = 1;
	}

	if (info.GetDamageType() & DMG_CRUSH)
	{
		if (flDot > -0.2f && info.GetBaseDamage() >= 5.0f)
		{
			iSubType = 3;
		}
	}

	DoAnimationEvent(flinchAct, iSubType);
}

void CLaz_Player::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendCriteria(set);
	set.AppendCriteria("voice", STRING(m_iszVoiceType));
	set.AppendCriteria("suitvoice", STRING(m_iszSuitVoice));
}

void CLaz_Player::SetFootsteps(const char *pchPrefix)
{
	if (0 == Q_strcmp(DEFAULT_FEET, pchPrefix))
	{
		m_iPlayerSoundType = INVALID_STRING_INDEX;
	}
	else
	{
		m_iPlayerSoundType = g_pStringTablePlayerFootSteps->AddString(CBaseEntity::IsServer(), pchPrefix);
	}

	PrecacheFootStepSounds();
}

void CLaz_Player::State_Transition(LAZPlayerState newState)
{
	State_Leave();
	State_Enter(newState);
}


void CLaz_Player::State_Enter(LAZPlayerState newState)
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo(newState);

	// Initialize the new state.
	if (m_pCurStateInfo && m_pCurStateInfo->pfnEnterState)
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CLaz_Player::State_Leave()
{
	if (m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState)
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CLaz_Player::State_PreThink()
{
	if (m_pCurStateInfo && m_pCurStateInfo->pfnPreThink)
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CLAZPlayerStateInfo *CLaz_Player::State_LookupInfo(LAZPlayerState state)
{
	// This table MUST match the 
	static CLAZPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CLaz_Player::State_Enter_ACTIVE, NULL, &CLaz_Player::State_PreThink_ACTIVE },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CLaz_Player::State_Enter_OBSERVER_MODE,	NULL, &CLaz_Player::State_PreThink_OBSERVER_MODE }
	};

	for (int i = 0; i < ARRAYSIZE(playerStateInfos); i++)
	{
		if (playerStateInfos[i].m_iPlayerState == state)
			return &playerStateInfos[i];
	}

	return NULL;
}

void CLaz_Player::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;
	if (IsNetClient())
	{
		const char *pIdealMode = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "cl_spec_mode");
		if (pIdealMode)
		{
			observerMode = atoi(pIdealMode);
			if (observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING)
			{
				observerMode = m_iObserverLastMode;
			}
		}
	}
	//m_bEnterObserver = true;
	StartObserverMode(observerMode);
}

void CLaz_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert(m_takedamage == DAMAGE_NO);
	Assert(IsSolidFlagSet(FSOLID_NOT_SOLID));
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert(m_lifeState == LIFE_DEAD);
	Assert(pl.deadflag);
}


void CLaz_Player::State_Enter_ACTIVE()
{
	SetMoveType(MOVETYPE_WALK);

	// md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
	// is false, then they'll be spectating but blocking live players from moving.
	// RemoveSolidFlags( FSOLID_NOT_SOLID );

	m_Local.m_iHideHUD = 0;
}


void CLaz_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death
}