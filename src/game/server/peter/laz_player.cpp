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
#include "npcevent.h"
#include "npc_manhack.h"
#include "ai_squad.h"
#include "world.h"
#include "team_objectiveresource.h"
#include "team_control_point_master.h"
#include "npc_playerfollower.h"
#include "econ_item_system.h"
#include "econ_wearable.h"
#include "viewport_panel_names.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace ResponseRules;

#define HL2MP_COMMAND_MAX_RATE 0.3

extern ISoundEmitterSystemBase *soundemitterbase;
extern CBaseEntity *FindPlayerStart(const char *pszClassName);
extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
int UTIL_LazEmitGroupnameSuit(CBasePlayer *entity, const char *groupname);

ConVar sv_forcedspecialattack("sv_laz_forcedspecial", "-1", FCVAR_CHEAT);

CStringTableSaveRestoreOps g_FootStepStringOps;

const char *g_pszSpecialAttacks[SPECIAL_ATTACK_COUNT] = {
	"manhack",
	"olivia"
};

const char* g_pszFlashlightTypes[FLASHLIGHT_TYPE_COUNT] = {
	"suit",
	"nvg",
	"weapon"
};

const char* g_pszFlashLightSounds[FLASHLIGHT_TYPE_COUNT] = {
	"HL2Player.FlashLight%s",
	"HL2Player.NightVision%s",
	"HL2Player.FlashLight%s"
};

LINK_ENTITY_TO_CLASS(info_player_combine, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_rebel, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_terrorist, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_counterterrorist, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_allies, CPointEntity);
LINK_ENTITY_TO_CLASS(info_player_axis, CPointEntity);

LINK_ENTITY_TO_CLASS(player, CLaz_Player);

BEGIN_DATADESC(CLaz_Player)
//DEFINE_SOUNDPATCH(m_pWooshSound),
DEFINE_FIELD(m_bHasLongJump, FIELD_BOOLEAN),

DEFINE_FIELD(m_flNextPainSoundTime, FIELD_TIME),
DEFINE_FIELD(m_iszVoiceType, FIELD_STRING),
DEFINE_FIELD(m_iszSuitVoice, FIELD_STRING),

DEFINE_EMBEDDED(m_AnnounceAttackTimer),

DEFINE_CUSTOM_FIELD(m_iPlayerSoundType, &g_FootStepStringOps),
DEFINE_INPUTFUNC(FIELD_INTEGER, "AnswerQuestion", InputAnswerQuestion),

DEFINE_FIELD(m_iszExpressionScene, FIELD_STRING),
DEFINE_FIELD(m_hExpressionSceneEnt, FIELD_EHANDLE),
DEFINE_FIELD(m_flNextRandomExpressionTime, FIELD_TIME),

DEFINE_FIELD(m_nFlashlightType, FIELD_INTEGER),
DEFINE_FIELD(m_flEyeHeightOverride, FIELD_FLOAT),
END_DATADESC();

IMPLEMENT_SERVERCLASS_ST(CLaz_Player, DT_Laz_Player)
SendPropInt(SENDINFO(m_bHasLongJump), 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iPlayerSoundType), MAX_FOOTSTEP_STRING_BITS + 1),
SendPropInt(SENDINFO(m_nFlashlightType)),
SendPropFloat(SENDINFO(m_flEyeHeightOverride)),
END_SEND_TABLE();

#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

void UTIL_UpdatePlayerModel(CHL2_Player* pPlayer)
{
	// This code is only for singleplayer
	if (g_pGameRules->IsMultiplayer())
		return;

	if (!pPlayer || pPlayer->GetHealth() <= 0 || !pPlayer->IsAlive())
		return;




	//pHands->NetworkStateChanged();

	playerModel_t* modelType = PlayerModelSystem()->SelectPlayerModel(g_pGameTypeSystem->GetCurrentGameType(), pPlayer->IsSuitEquipped());

	pPlayer->SetModel(modelType->models.Head().szModelName);
	pPlayer->m_nSkin = modelType->models.Head().skin;
	for (int i = 0; i < modelType->models.Head().bodygroups.Count(); i++)
	{
		int iGroup = pPlayer->FindBodygroupByName(modelType->models.Head().bodygroups[i].szName);
		pPlayer->SetBodygroup(iGroup, modelType->models.Head().bodygroups[i].body);
	}

	for (int i = 0; i < MAX_VIEWMODELS; i++)
	{
		CBaseViewModel* pHands = pPlayer->GetViewModel(i);
		if (pHands)
		{
			pHands->SetHandsModel(modelType->szArmModel, modelType->armSkin);

			for (int i = 0; i < modelType->armbodys.Count(); i++)
			{
				pHands->SetHandsBodygroupByName(modelType->armbodys[i].szName, modelType->armbodys[i].body);
			}
		}
	}

	CLaz_Player* pHLMS = assert_cast<CLaz_Player*> (pPlayer);

	KeyValues* pkvAbillites = modelType->kvAbilities;
	if (pkvAbillites != nullptr)
	{
		const char* pchVoice = pkvAbillites->GetString("voice", DEFAULT_ABILITY);
		const char* pchSuit = pkvAbillites->GetString("suit", DEFAULT_ABILITY);
		pHLMS->SetVoiceType(pchVoice, pchSuit);

		const char* pchFootSound = pkvAbillites->GetString("footsteps", DEFAULT_ABILITY);
		pHLMS->SetFootsteps(pchFootSound);

		const char* pchClassname = pkvAbillites->GetString("response_class", DEFAULT_ABILITY);
		pHLMS->SetResponseClassname(pchClassname);

		pHLMS->m_nFlashlightType = UTIL_StringFieldToInt(pkvAbillites->GetString("flashlight"), g_pszFlashlightTypes, FLASHLIGHT_TYPE_COUNT);

		pHLMS->m_flEyeHeightOverride = pkvAbillites->GetFloat("view_height", -1.f);
	}
	else
	{
		pHLMS->SetVoiceType(DEFAULT_ABILITY, DEFAULT_ABILITY);
		pHLMS->SetFootsteps(DEFAULT_ABILITY);
		pHLMS->SetResponseClassname(DEFAULT_ABILITY);
		pHLMS->m_nFlashlightType = FLASHLIGHT_SUIT;
	}
}

EHANDLE CLaz_Player::gm_hLastRandomSpawn = nullptr;
string_t		CLaz_Player::gm_iszDefaultAbility = NULL_STRING;
HSOUNDSCRIPTHANDLE CLaz_Player::gm_hsFlashLightSoundHandles[] = { SOUNDEMITTER_INVALID_HANDLE };
int			CLaz_Player::gm_iGordonFreemanModel = -1;

extern CSuitPowerDevice SuitDeviceFlashlight;
bool Flashlight_UseLegacyVersion(void);

CLaz_Player::CLaz_Player()
{
	m_nSpecialAttack = -1;
	m_iPlayerSoundType = INVALID_STRING_INDEX;
	m_nFlashlightType = FLASHLIGHT_SUIT;
}

void CLaz_Player::Precache(void)
{
	BaseClass::Precache();

	gm_iszDefaultAbility = AllocPooledString_StaticConstantStringPointer(DEFAULT_ABILITY);
	gm_iGordonFreemanModel = PrecacheModel("models/sirgibs/ragdolls/gordon_survivor_player.mdl");

	PrecacheFootStepSounds();
	PrecacheScriptSound("TFPlayer.FreezeCam");
	PrecacheScriptSound("JumpLand.HighVelocityImpact");
	PrecacheScriptSound("PortalPlayer.FallRecover");

	for (int i = 0; i < FLASHLIGHT_TYPE_COUNT; i++)
	{
		gm_hsFlashLightSoundHandles[i] = PrecacheScriptSound(CFmtStr(g_pszFlashLightSounds[i], "On"));
		gm_hsFlashLightSoundHandles[i+ FLASHLIGHT_TYPE_COUNT] = PrecacheScriptSound(CFmtStr(g_pszFlashLightSounds[i], "Off"));
	}
}

//-----------------------------------------------------------------------------
// Purpose: Force this player to immediately respawn
//-----------------------------------------------------------------------------
void CLaz_Player::ForceRespawn(void)
{
	RemoveAllItems(true);

	// Reset ground state for airwalk animations
	SetGroundEntity(NULL);

	// Stop any firing that was taking place before respawn.
	m_nButtons = 0;

	State_Transition(STATE_ACTIVE);
	Spawn();
}

void CLaz_Player::Spawn(void)
{
	m_iszVoiceType = gm_iszDefaultAbility;

	if (!g_pGameRules->IsMultiplayer())
		ChangeTeam(GetAutoTeam());

	BaseClass::Spawn();

	if (GetTeamNumber() == TEAM_ZOMBIES)
		SetBloodColor(BLOOD_COLOR_ZOMBIE);
	else
		SetBloodColor(BLOOD_COLOR_RED);

	// Kind of lame, but CBasePlayer::Spawn resets a lot of the state that we initially want on.
	// So if we're in the welcome state, call its enter function to reset 
	if (m_iPlayerState == STATE_WELCOME)
	{
		StateEnterWELCOME();
	}

	// If they were dead, then they're respawning. Put them in the active state.
	if (m_iPlayerState == STATE_DYING)
	{
		State_Transition(STATE_ACTIVE);
	}

	ClearExpression();

	if (HasMPModel())
	{
		rndModel_t *variant = &m_MPModel.models.Random();
		SetModel(variant->szModelName);
		m_nSkin = RandomInt(variant->skin, variant->skinMax);
		for (int i = 0; i < variant->bodygroups.Count(); i++)
		{
			int iGroup = FindBodygroupByName(variant->bodygroups[i].szName);
			SetBodygroup(iGroup, RandomInt(variant->bodygroups[i].body, variant->bodygroups[i].bodyMax));
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
			const char* pchVoice = pkvAbillites->GetString("voice", DEFAULT_ABILITY);
			const char* pchSuit = pkvAbillites->GetString("suit", DEFAULT_ABILITY);
			SetVoiceType(pchVoice, pchSuit);

			const char* pchFootSound = pkvAbillites->GetString("footsteps", DEFAULT_ABILITY);
			SetFootsteps(pchFootSound);

			const char* pchClassname = pkvAbillites->GetString("response_class", DEFAULT_ABILITY);
			SetResponseClassname(pchClassname);

			m_nSpecialAttack = UTIL_StringFieldToInt(pkvAbillites->GetString("special"), g_pszSpecialAttacks, SPECIAL_ATTACK_COUNT);
			m_nFlashlightType = UTIL_StringFieldToInt(pkvAbillites->GetString("flashlight"), g_pszFlashlightTypes, FLASHLIGHT_TYPE_COUNT);

			m_flEyeHeightOverride = pkvAbillites->GetFloat("view_height", -1.f);
		}
		else
		{
			SetVoiceType(DEFAULT_ABILITY, DEFAULT_ABILITY);
			SetFootsteps(DEFAULT_ABILITY);
			SetResponseClassname(DEFAULT_ABILITY);
			m_nSpecialAttack = -1;
			m_nFlashlightType = FLASHLIGHT_NONE;
			m_flEyeHeightOverride = -1.f;
		}
	}
	else
	{
		m_nSpecialAttack = -1;
	}

	UpdatePlayerColor();

	if (!IsObserver())
	{
		pl.deadflag = false;
		RemoveSolidFlags(FSOLID_NOT_SOLID);

		RemoveEffects(EF_NODRAW);

		SetViewOffset(GetPlayerEyeHeight());
	}
}

void CLaz_Player::InitialSpawn(void)
{
	BaseClass::InitialSpawn();

	if (gpGlobals->maxClients > 1)
		State_Transition(STATE_WELCOME);
	else
		State_Transition(STATE_ACTIVE);
}

void CLaz_Player::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();
	ClearExpression();

	if (HasMPModel())
	{
		PlayerModelSystem()->PlayerReleaseModel(m_MPModel.szSectionID);
		m_MPModel = { 0 };
	}
}

//--------------------------------------------------------
//	EntSelectSpawnPoint()
//	In singleplayer, looks for info_player_start.
//	In multiplayer, looks for info_player_teamspawn.
//	If no such entities exist, looks for HL2:DM, CS:S,
//	and DoD:S spawnpoints, in that order.
//	If still unable to find the spawnpoints, looks for
//	info_player_deathmatch and info_player_start.
//--------------------------------------------------------
CBaseEntity * CLaz_Player::EntSelectSpawnPoint(void)
{
	CBaseEntity *pSpot;
	CBaseEntity *pSpotFinder;
	edict_t		*player;

	player = edict();

	pSpot = gm_hLastRandomSpawn.Get();
	pSpotFinder = NULL;

	int iTeamSpawnTeam = RandomInt(TEAM_COMBINE, TEAM_REBELS);

	if (!g_pGameRules->IsMultiplayer())
	{
		// If startspot is set, (re)spawn there.
		if (!gpGlobals->startspot || !strlen(STRING(gpGlobals->startspot)))
		{
			pSpot = FindPlayerStart("info_player_start");
			if (pSpot)
				goto ReturnSpot;
		}
		else
		{
			pSpot = gEntList.FindEntityByName(NULL, gpGlobals->startspot);
			if (pSpot)
				goto ReturnSpot;
		}
	}
	else
	{
		if (GetTeamNumber() > LAST_SHARED_TEAM)
		{
			CTeam *pTeam = GetTeam();
			if (pTeam)
			{
				pSpot = pTeam->SpawnPlayer(this);
				if (pSpot)
					goto ReturnSpot;
			}

			iTeamSpawnTeam = GetTeamNumber();
		}
	}


	if (iTeamSpawnTeam == TEAM_REBELS)
	{
		pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_rebel");
	}
	else if (iTeamSpawnTeam == TEAM_COMBINE)
	{
		pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_combine");
	}

	if (pSpotFinder)
	{
		goto SelectRandomSpot;
	}
	else
	{
		if (iTeamSpawnTeam == TF_TEAM_RED)
		{
			pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_terrorist");
		}
		else if (iTeamSpawnTeam == TF_TEAM_BLUE)
		{
			pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_counterterrorist");
		}

		if (pSpotFinder)
		{
			goto SelectRandomSpot;
		}
		else
		{
			if (iTeamSpawnTeam == TF_TEAM_BLUE)
			{
				pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_axis");
			}
			else if (iTeamSpawnTeam == TF_TEAM_RED)
			{
				pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_allies");
			}

			if (pSpotFinder)
			{
				goto SelectRandomSpot;
			}
			else
			{
				pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_deathmatch");

				if (pSpotFinder)
				{
					goto SelectRandomSpot;
				}
				else
				{
					pSpotFinder = gEntList.FindEntityByClassname(pSpot, "info_player_start");

					if (pSpotFinder)
					{
						goto SelectRandomSpot;
					}
					else
					{
						pSpot = gEntList.FindEntityByName(NULL, gpGlobals->startspot);
						if (pSpot)
							goto ReturnSpot;
					}
				}
			}
		}
	}

SelectRandomSpot:
	if (!pSpotFinder)
		goto ReturnSpot;

	// Randomize the start spot
	for (int i = random->RandomInt(1, 5); i > 0; i--)
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpotFinder->GetClassname());
	if (!pSpot)  // skip over the null point
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpotFinder->GetClassname());

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		if (pSpot)
		{
			// check if pSpot is valid
			if (g_pGameRules->IsSpawnPointValid(pSpot, this))
			{
				if (pSpot->GetLocalOrigin() == vec3_origin)
				{
					pSpot = gEntList.FindEntityByClassname(pSpot, pSpotFinder->GetClassname());
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpotFinder->GetClassname());
	} while (pSpot != pFirstSpot); // loop if we're not back to the start

								   // we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if (pSpot)
	{
		CBaseEntity *ent = NULL;
		for (CEntitySphereQuery sphere(pSpot->GetAbsOrigin(), 128); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
		{
			// if ent is a client, kill em (unless they are ourselves)
			if (ent->IsPlayer() && !(ent->edict() == player) && !ent->IsInTeam(GetTeam()))
				ent->TakeDamage(CTakeDamageInfo(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC));
		}
		goto ReturnSpot;
	}

ReturnSpot:
	if (!pSpot)
	{
		Warning("PutClientInServer: no player spawn on level. Navigation mesh generation will not work!\n");
		return CBaseEntity::Instance(INDEXENT(0));
	}

	gm_hLastRandomSpawn = pSpot;
	return pSpot;
}

bool CLaz_Player::ShouldRegenerateHealth()
{
	return g_pGameTypeSystem->GetCurrentGameType() == GAME_PORTAL;
}

void CLaz_Player::DoMuzzleFlash()
{
	BaseClass::DoMuzzleFlash();

	if (GetEnemy())
	{
		if (GetActiveWeapon() && m_AnnounceAttackTimer.Expired())
		{
			if (SpeakConceptIfAllowed(MP_CONCEPT_FIREWEAPON, CFmtStr("attacking_with_weapon:%s", GetActiveWeapon()->GetClassname())))
			{
				m_AnnounceAttackTimer.Set(10, 30);
			}
		}
	}
}

void CLaz_Player::ModifyOrAppendPlayerCriteria(AI_CriteriaSet& set, bool bEnemy)
{
	BaseClass::ModifyOrAppendPlayerCriteria(set, bEnemy);

	bool bFreeman = (GetModelIndex() == gm_iGordonFreemanModel);
	bool bSuit = IsSuitEquipped() && m_iszSuitVoice != gm_iszDefaultAbility; // Will give zero for rebels and zombies

	if (!bEnemy)
	{
		set.AppendCriteria("playerfreeman", bFreeman ? "1" : "0");
		set.AppendCriteria("playersuit", bSuit ? "1" : "0");
	}
	else
	{
		set.AppendCriteria("enemyplayerfreeman", bFreeman ? "1" : "0");
		set.AppendCriteria("enemyplayersuit", bSuit ? "1" : "0");
	}
}

LazSpecialtyStatus_e CLaz_Player::GetPlayerPrivilegeLevel()
{
	CSteamID thisID;
	if (GetSteamID(&thisID))
	{
		if (thisID.GetAccountID() == CSteamID(76561198086666630Ui64).GetAccountID())
			return LAZ_STATUS_DEVELOPER;
	}

	if (!engine->IsDedicatedServer() && UTIL_GetListenServerHost() == this)
		return LAZ_STATUS_ADMIN;

	return LAZ_STATUS_PLAYER;
}

int CLaz_Player::GetPlayerPermissions()
{
	int iPermissionMask = 0;
	if (GetPlayerPrivilegeLevel() >= LAZ_STATUS_ADMIN)
	{
		iPermissionMask |= LAZ_PERM_VOICE_BROADCAST | LAZ_PERM_NOCLIP | LAZ_PERM_FORCE_MODEL;
	}

	return (m_iDesiredPermissions & iPermissionMask);
}

float CLaz_Player::PlayScene(const char* pszScene, float flDelay, AI_Response* response, IRecipientFilter* filter)
{
	return InstancedScriptedScene(this, pszScene, NULL, flDelay, false, response, true, filter);
}

void CLaz_Player::PreThink(void)
{
	BaseClass::PreThink();
	State_PreThink();

	if (!g_pGameRules->IsMultiplayer())
		UpdatePlayerColor();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CLaz_Player::ShouldGib(const CTakeDamageInfo &info)
{
	if (info.GetDamageType() & DMG_NEVERGIB)
		return false;

	if (info.GetDamageType() & DMG_ALWAYSGIB)
		return true;

	if (info.GetDamageCustom() == TF_DMG_CUSTOM_TELEFRAG)
		return true;

	if (g_pGameRules->Damage_ShouldGibCorpse(info.GetDamageType()))
	{
		if (m_iHealth < -2)
			return true;
	}

	return false;
}

void CLaz_Player::Event_KilledOther(CBaseEntity * pVictim, const CTakeDamageInfo & info)
{
	BaseClass::Event_KilledOther(pVictim, info);

	// No taunts after killing teammates.
	if (g_pGameRules->PlayerRelationship(this, pVictim) == GR_TEAMMATE)
		return;

	if (pVictim->IsNPC() || pVictim->IsPlayer())
	{
		CFmtStrN<128> modifiers("playerenemy:%s", pVictim->GetResponseClassname(this));
		SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_TAUNTS, modifiers);
	}
}

void CLaz_Player::Event_Killed(const CTakeDamageInfo & info)
{
	// Release the manhack if we're in the middle of deploying him
	if (m_hMinion && m_hMinion->IsAlive() && m_hMinion->Classify() == CLASS_MANHACK)
	{
		ReleaseManhack();
	}

	// show killer in death cam mode
	if (info.GetAttacker() && (info.GetAttacker()->IsPlayer() || info.GetAttacker()->IsNPC()) && info.GetAttacker() != (CBaseEntity*)this)
	{
		m_hObserverTarget.Set(info.GetAttacker());
		SetFOV(this, 0); // reset
	}
	else
		m_hObserverTarget.Set(NULL);

	BaseClass::Event_Killed(info);

	ClearExpression();

	State_Transition(STATE_DYING);	// Transition into the dying state.
	m_hEnemy.Term();
}

void CLaz_Player::HandleAnimEvent(animevent_t * pEvent)
{
	if ((pEvent->type & AE_TYPE_NEWEVENTSYSTEM) && (pEvent->type & AE_TYPE_SERVER))
	{
		if (pEvent->event == AE_METROPOLICE_START_DEPLOY)
		{
			OnAnimEventStartDeployManhack();
			return;
		}

		if (pEvent->event == AE_METROPOLICE_DEPLOY_MANHACK)
		{
			OnAnimEventDeployManhack(pEvent);
			return;
		}
	}

	BaseClass::HandleAnimEvent(pEvent);
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

	//bool bKill = false;

	int iOldTeam = GetTeamNumber();

	// if this is our current team, just abort
	if (iTeam == iOldTeam)
		return;

	if (g_pGameRules->IsMultiplayer() == true)
	{
		// In co-op mode, you can be a spectator or a protaganist.
		if (LazuulRules()->GetGameMode() == LAZ_GM_COOP && iTeam != TEAM_SPECTATOR)
		{
			if (GetTeamNumber() != iTeam)
				iTeam = LazuulRules()->GetProtaganistTeam();
		}
	}

	BaseClass::ChangeTeam(iTeam);

	//m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	if (iTeam == TEAM_SPECTATOR)
	{
		RemoveAllItems(true);

		State_Transition(STATE_OBSERVER_MODE);
	}
	else // active player
	{
		if (!IsDead() && (iOldTeam >= FIRST_GAME_TEAM))
		{
			// Kill player if switching teams while alive
			CommitSuicide(false, true);
		}
		else if (IsDead() && iOldTeam < FIRST_GAME_TEAM)
		{
			SetObserverMode(OBS_MODE_CHASE);
			//HandleFadeToBlack();
		}

		if (iTeam >= FIRST_GAME_TEAM && m_iPlayerState == STATE_WELCOME)
			State_Transition(STATE_OBSERVER_MODE);
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

void CLaz_Player::SetMinion(CBaseCombatCharacter * pMinion)
{
	m_hMinion.Set(pMinion);
	const char *pchClassName = (pMinion->IsNPC()) ? pMinion->MyNPCPointer()->GetDeathNoticeNameOverride() : nullptr;
	if (!pchClassName)
		pchClassName = pMinion->GetClassname();

	V_strncpy(m_strMinionClass.GetForModify(), pchClassName, 32);
}

bool CLaz_Player::HandleCommand_JoinTeam(int team)
{
	if (!g_pGameRules->IsMultiplayer())
		return false;

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

		//if (GetTeamNumber() != TEAM_UNASSIGNED && !IsDead())
		//{
		//	m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work

		//	CommitSuicide();

		//	// add 1 to frags to balance out the 1 subtracted for killing yourself
		//	IncrementFragCount(1);
		//}

		ChangeTeam(TEAM_SPECTATOR);

		return true;
	}
	/*else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}*/

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

	if (GetPlayerPermissions() & LAZ_PERM_FORCE_MODEL)
	{
		const char* pszModel = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "cl_playermodel");
		playerModel_t model = PlayerModelSystem()->FindPlayerModel(pszModel);
		if (model.szSectionID[0])
		{
			m_MPModel = model;
			PlayerModelSystem()->PlayerGrabModel(m_MPModel.szSectionID);
			return;
		}
	}

	CUtlVector<playerModel_t> models = PlayerModelSystem()->GetAvailableModelsForTeam(TeamID());

	if (!models.Count())
		return;

	bool bFound = false;
	int i = 0;
	if (!IsFakeClient())
	{
		const char *pszModel = engine->GetClientConVarValue(engine->IndexOfEdict(edict()), "cl_playermodel");
		for (i = 0; i < models.Count(); i++)
		{
			if (FStrEq(pszModel, models[i].szSectionID))
			{
				bFound = true;
				break;
			}
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

	PlayerModelSystem()->PlayerGrabModel(m_MPModel.szSectionID);
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
			ClientPrint(this, HUD_PRINTCONSOLE, "Player sent bad jointeam syntax\n");
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
	else if (FStrEq(args[0], "laz_perm_toggle"))
	{
		if (GetPlayerPrivilegeLevel() > LAZ_STATUS_PLAYER)
		{
			int iPerms = atoi(args[1]);
			m_iDesiredPermissions ^= iPerms;
			return true;
		}
		else
		{
			return false;
		}
	}

	return BaseClass::ClientCommand(args);
}

void CLaz_Player::OnAnimEventDeployManhack(animevent_t * pEvent)
{
	// Let it go
	ReleaseManhack();

	if (!m_hMinion)
		return;

	Vector forward, right;
	GetVectors(&forward, &right, NULL);

	IPhysicsObject *pPhysObj = m_hMinion->VPhysicsGetObject();

	if (pPhysObj)
	{
		Vector	yawOff = right * random->RandomFloat(-1.0f, 1.0f);

		Vector	forceVel = (forward + yawOff * 16.0f) + Vector(0, 0, 250);
		Vector	forceAng = vec3_origin;

		// Give us velocity
		pPhysObj->AddVelocity(&forceVel, &forceAng);
	}
}

void CLaz_Player::OnAnimEventStartDeployManhack(void)
{
	//Assert(m_iManhacks);

	/*if (m_iManhacks <= 0)
	{
		DevMsg("Error: Throwing manhack but out of manhacks!\n");
		return;
	}*/

	//m_iManhacks--;

	// Turn off the manhack on our body
	/*if (m_iManhacks <= 0)
	{
		SetBodygroup(METROPOLICE_BODYGROUP_MANHACK, false);
	}*/

	// Create the manhack to throw
	CNPC_Manhack *pManhack = (CNPC_Manhack *)CreateEntityByName("npc_manhack");

	Vector	vecOrigin;
	QAngle	vecAngles;

	int handAttachment = LookupAttachment("anim_attachment_LH");
	GetAttachment(handAttachment, vecOrigin, vecAngles);

	pManhack->SetLocalOrigin(vecOrigin);
	pManhack->SetLocalAngles(vecAngles);
	pManhack->AddSpawnFlags((SF_MANHACK_PACKED_UP | SF_MANHACK_CARRIED | SF_NPC_WAIT_FOR_SCRIPT));

	pManhack->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	pManhack->Spawn();
	pManhack->SetDeployingPlayer(this);

	// Make us move with his hand until we're deployed
	pManhack->SetParent(this, handAttachment);

	SetMinion(pManhack);
}

void CLaz_Player::ReleaseManhack(void)
{
	CNPC_Manhack *pManhack = dynamic_cast<CNPC_Manhack *> (m_hMinion.Get());

	if (!pManhack)
		return;

	// Make us physical
	pManhack->RemoveSpawnFlags(SF_MANHACK_CARRIED);
	pManhack->CreateVPhysics();

	// Release us
	pManhack->RemoveSolidFlags(FSOLID_NOT_SOLID);
	pManhack->SetMoveType(MOVETYPE_VPHYSICS);
	pManhack->SetParent(NULL);

	// Make us active
	pManhack->RemoveSpawnFlags(SF_NPC_WAIT_FOR_SCRIPT);
	pManhack->ClearSchedule("Manhack released by metropolice");
	
	trace_t tr;
	CTraceFilterWorldAndPropsOnly filter;
	UTIL_TraceEntity(pManhack, WorldSpaceCenter(), pManhack->GetAbsOrigin(), pManhack->PhysicsSolidMaskForEntity(), &filter, &tr);
	if (tr.startsolid || tr.DidHit())
	{
		CTakeDamageInfo info(GetWorldEntity(), GetWorldEntity(), 100.f, DMG_CRUSH);
		pManhack->TakeDamage(info);
		return;
	}

	// Place him into our squad so we can communicate
	/*if (GetPlayerSquad())
	{
		GetPlayerSquad()->AddToSquad(pManhack);
	}*/
}

void CLaz_Player::UpdatePlayerColor(void)
{
	if (!IsFakeClient())
	{
		for (int i = 0; i < NUM_PLAYER_COLORS; i++)
		{
			CFmtStr str("cl_laz_player_color%i", i);
			const char* pszValue = engine->GetClientConVarValue(entindex(), str.Access());
			UTIL_StringToVector(m_vecPlayerColors[i].Base(), pszValue);
		}
	}
	else
	{
		for (int i = 0; i < NUM_PLAYER_COLORS; i++)
			m_vecPlayerColors[i] = RandomVector(0.f, 1.f);
	}
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

//-----------------------------------------------------------------------------
// Purpose: Get response scene corresponding to concept
//-----------------------------------------------------------------------------
bool CLaz_Player::GetResponseSceneFromConcept(int iConcept, char* chSceneBuffer, int numSceneBufferBytes)
{
	AI_Response result;
	bool bResult = SpeakConcept(result, iConcept);
	if (bResult)
	{
		const char* szResponse = result.GetResponsePtr();
		Q_strncpy(chSceneBuffer, szResponse, numSceneBufferBytes);
	}
	return bResult;
}

void CLaz_Player::UpdateExpression(void)
{
	char szScene[MAX_PATH];
	if (!GetResponseSceneFromConcept(MP_CONCEPT_PLAYER_EXPRESSION, szScene, sizeof(szScene)))
	{
		ClearExpression();
		m_flNextRandomExpressionTime = gpGlobals->curtime + RandomFloat(30, 40);
		return;
	}

	// Ignore updates that choose the same scene
	if (m_iszExpressionScene != NULL_STRING && stricmp(STRING(m_iszExpressionScene), szScene) == 0)
	{
		m_flNextRandomExpressionTime = gpGlobals->curtime + 1.0f;
		return;
	}

	if (m_hExpressionSceneEnt)
	{
		ClearExpression();
	}

	m_iszExpressionScene = AllocPooledString(szScene);
	float flDuration = InstancedScriptedScene(this, szScene, &m_hExpressionSceneEnt, 0.0, true, NULL, true);
	m_flNextRandomExpressionTime = gpGlobals->curtime + flDuration;
}

void CLaz_Player::ClearExpression(void)
{
	if (m_hExpressionSceneEnt != NULL)
	{
		StopScriptedScene(this, m_hExpressionSceneEnt);
	}
	m_iszExpressionScene = NULL_STRING;
	m_flNextRandomExpressionTime = gpGlobals->curtime;
}

void CLaz_Player::InputAnswerQuestion(inputdata_t& inputdata)
{
	AnswerQuestion(dynamic_cast<CBaseCombatCharacter*>(inputdata.pActivator), inputdata.value.Int(), false);
}

void CLaz_Player::AnswerQuestion(CBaseCombatCharacter* pQuestioner, int iQARandomNum, bool bAnsweringHello)
{
	// Original questioner may have died
	if (!pQuestioner)
		return;

	AI_Response selection;
	static ConVarRef rr_debug_qa("rr_debug_qa");

	// Use the random number that the questioner used to determine his Question (so we can match answers via response rules)
	//m_iQARandomNumber = iQARandomNum;

	// The activator is the person we're responding to
	if (SpeakFindResponse(selection, "TLK_PLAYER_ANSWER", CFmtStr("speechtarget:%s", pQuestioner->GetClassname())))
	{
		if (rr_debug_qa.GetBool())
		{
			if (bAnsweringHello)
			{
				Warning("Q&A: '%s' answered the Hello from '%s'\n", GetDebugName(), pQuestioner->GetDebugName());
			}
			else
			{
				Warning("Q&A: '%s' answered the Question from '%s'\n", GetDebugName(), pQuestioner->GetDebugName());
			}
		}

		SpeakDispatchResponse("TLK_PLAYER_ANSWER", &selection);
	}
	else if (rr_debug_qa.GetBool())
	{
		Warning("Q&A: '%s' couldn't answer '%s'\n", GetDebugName(), pQuestioner->GetDebugName());
	}
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

	CMultiplayer_Expresser *pExpresser = GetMultiplayerExpresser();
	Assert(pExpresser);

	if (!pExpresser->CanSpeakAfterMyself())
		return;

	float flPainLength = 0;
	AI_Response dummy;
	bool bAttackerIsPlayer = (info.GetAttacker() && info.GetAttacker()->IsPlayer() && SpeakConcept(dummy, MP_CONCEPT_PLAYER_ATTACKER_PAIN));

	pExpresser->AllowMultipleScenes();

	// speak a pain concept here, send to everyone but the attacker
	CPASFilter filter(GetAbsOrigin());

	if (bAttackerIsPlayer)
	{
		filter.RemoveRecipient(ToBasePlayer(info.GetAttacker()));
	}

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

	//speak a louder pain concept to just the attacker
	if (bAttackerIsPlayer)
	{
		CSingleUserRecipientFilter attackerFilter(ToBasePlayer(info.GetAttacker()));
		SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_ATTACKER_PAIN, CFmtStr("damagecritical:1,%s", modifiers.Access()), nullptr, 0U, &attackerFilter);
	}

	pExpresser->DisallowMultipleScenes();

	m_flNextPainSoundTime = gpGlobals->curtime + flPainLength;
}

void CLaz_Player::DeathSound(const CTakeDamageInfo & info)
{
	// Did we die from falling?
	if (m_bitsDamageType & DMG_FALL)
	{
		// They died in the fall. Play a splat sound.
		EmitSound("Player.FallGib");
	}
	else
	{
		AI_Response resp;
		SpeakConcept(resp, MP_CONCEPT_DIED);
		const char *szResponse = resp.GetResponsePtr();
		soundlevel_t soundlevel = resp.GetSoundLevel();
		switch (resp.GetType())
		{
		default:
		case RESPONSE_NONE:
			break;

		case RESPONSE_SPEAK:
			{
				EmitSound(szResponse);
			}
			break;

		case RESPONSE_SENTENCE:
			GetExpresser()->SpeakRawSentence(szResponse, 0, VOL_NORM, soundlevel);
			break;

		case RESPONSE_SCENE:
			PlayScene(szResponse, 0, &resp);
			break;
		}
	}

	// play one of the suit death alarms
	if (IsSuitEquipped() && !(m_iszSuitVoice == gm_iszDefaultAbility))
	{
		UTIL_LazEmitGroupnameSuit(this, CFmtStr("%s_DEAD", STRING(m_iszSuitVoice)));
	}

	GetExpresser()->ForceNotSpeaking();
}

bool CLaz_Player::ChooseEnemy()
{
	trace_t tr;
	CTraceFilterNoNPCsOrPlayer filter1(this, COLLISION_GROUP_NONE);
	CTraceFilterLOS filter2(this, COLLISION_GROUP_NONE);
	CTraceFilterChain filter(&filter1, &filter2);
	Vector vecEyes, vecForward;
	EyePositionAndVectors(&vecEyes, &vecForward, nullptr, nullptr);
	UTIL_TraceLine(vecEyes, vecEyes + vecForward * MAX_TRACE_LENGTH, MASK_OPAQUE, &filter, &tr);

	Ray_t ray;
	ray.Init(vecEyes, tr.endpos, -Vector(64), Vector(64));

	CBaseEntity *list[256];
	float flNearestDist = MAX_TRACE_LENGTH;
	CBaseEntity *pNearest = NULL;

	if (m_hEnemy.Get() && m_hEnemy->IsAlive())
	{
		pNearest = m_hEnemy;
		Vector los = (m_hEnemy->WorldSpaceCenter() - vecEyes);
		flNearestDist = VectorNormalize(los);
	}

	int count = UTIL_EntitiesAlongRay(list, 256, ray, FL_CLIENT | FL_NPC);
	for (int i = 0; i < count; i++)
	{
		if (!list[i]->IsAlive() || list[i] == this)
			continue;

		if (IRelationType(list[i]) > D_FR)
			continue;
		
		// Closer than other objects
		Vector los = (list[i]->WorldSpaceCenter() - vecEyes);
		float flDist = VectorNormalize(los);
		if (flDist >= flNearestDist)
			continue;

		if (IsAbleToSee(list[i], CBaseCombatCharacter::USE_FOV))
		{
			flNearestDist = flDist;
			pNearest = list[i];
		}
	}

	if (pNearest)
	{
		m_hEnemy.Set(pNearest);
		return true;
	}
	else
	{
		m_hEnemy.Term();
	}

	return false;
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
		if (m_hMinion && m_hMinion->IsAlive())
			return;

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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLaz_Player::FlashlightTurnOn(void)
{
	if (m_bFlashlightDisabled)
		return;

	if (Flashlight_UseLegacyVersion() && m_nFlashlightType == FLASHLIGHT_SUIT)
	{
		if (!SuitPower_AddDevice(SuitDeviceFlashlight))
			return;
	}

	if (m_nFlashlightType == FLASHLIGHT_NONE || (m_nFlashlightType == FLASHLIGHT_SUIT && !IsSuitEquipped()))
		return;


	AddEffects(EF_DIMLIGHT);
	EmitSound(CFmtStr(g_pszFlashLightSounds[m_nFlashlightType], "On"), gm_hsFlashLightSoundHandles[m_nFlashlightType]);

	variant_t flashlighton;
	flashlighton.SetFloat(m_HL2Local.m_flSuitPower / 100.0f);
	FirePlayerProxyOutput("OnFlashlightOn", flashlighton, this, this);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLaz_Player::FlashlightTurnOff(void)
{
	if (Flashlight_UseLegacyVersion() && m_nFlashlightType == FLASHLIGHT_SUIT)
	{
		if (!SuitPower_RemoveDevice(SuitDeviceFlashlight))
			return;
	}
	else
		SuitPower_RemoveDevice(SuitDeviceFlashlight);

	RemoveEffects(EF_DIMLIGHT);
	EmitSound(CFmtStr(g_pszFlashLightSounds[m_nFlashlightType], "Off"), gm_hsFlashLightSoundHandles[m_nFlashlightType + FLASHLIGHT_TYPE_COUNT]);

	variant_t flashlightoff;
	flashlightoff.SetFloat(m_HL2Local.m_flSuitPower / 100.0f);
	FirePlayerProxyOutput("OnFlashlightOff", flashlightoff, this, this);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define FLASHLIGHT_RANGE	Square(600)
bool CLaz_Player::IsIlluminatedByFlashlight(CBaseEntity* pEntity, float* flReturnDot)
{
	// Other people can't see your nightvision
	if (!FlashlightIsOn() || m_nFlashlightType == FLASHLIGHT_NVG)
		return false;

	if (pEntity->Classify() == CLASS_BARNACLE && pEntity->GetEnemy() == this)
	{
		// As long as my flashlight is on, the barnacle that's pulling me in is considered illuminated.
		// This is because players often shine their flashlights at Alyx when they are in a barnacle's 
		// grasp, and wonder why Alyx isn't helping. Alyx isn't helping because the light isn't pointed
		// at the barnacle. This will allow Alyx to see the barnacle no matter which way the light is pointed.
		return true;
	}

	// Within 50 feet?
	float flDistSqr = GetAbsOrigin().DistToSqr(pEntity->GetAbsOrigin());
	if (flDistSqr > FLASHLIGHT_RANGE)
		return false;

	// Within 45 degrees?
	Vector vecSpot = pEntity->WorldSpaceCenter();
	Vector los;

	// If the eyeposition is too close, move it back. Solves problems
	// caused by the player being too close the target.
	if (flDistSqr < (128 * 128))
	{
		Vector vecForward;
		EyeVectors(&vecForward);
		Vector vecMovedEyePos = EyePosition() - (vecForward * 128);
		los = (vecSpot - vecMovedEyePos);
	}
	else
	{
		los = (vecSpot - EyePosition());
	}

	VectorNormalize(los);
	Vector facingDir = EyeDirection3D();
	float flDot = DotProduct(los, facingDir);

	if (flReturnDot)
	{
		*flReturnDot = flDot;
	}

	if (flDot < 0.92387f)
		return false;

	if (!FVisible(pEntity))
		return false;

	return true;
}

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

int UTIL_LazEmitGroupnameSuit(CBasePlayer *entity, const char *groupname)
{
	float fvol;
	int pitch = PITCH_NORM;
	int sentenceIndex = -1;

	fvol = 0.5f;
	if (random->RandomInt(0, 1))
		pitch = random->RandomInt(0, 6) + 98;

	CAI_TimedSemaphore *pSemaphore = (entity->GetTeamNumber() == LazuulRules()->GetProtaganistTeam()) ? &g_AIFriendliesTalkSemaphore : &g_AIFoesTalkSemaphore;

	// If friendlies are talking, reduce the volume of the suit
	if (!pSemaphore->IsAvailable(nullptr))
	{
		fvol *= 0.3;
	}

	if (fvol > 0.05)
	{
		char name[64];
		int ipick;
		int isentenceg;

		name[0] = 0;

		isentenceg = engine->SentenceGroupIndexFromName(groupname);
		if (isentenceg < 0)
		{
			Warning("No such sentence group %s\n", groupname);
			return -1;
		}

		ipick = engine->SentenceGroupPick(isentenceg, name, sizeof(name));
		if (ipick >= 0 && name[0])
		{
			sentenceIndex = SENTENCEG_Lookup(name);
			CSingleUserRecipientFilter filter(entity);
			CBaseEntity::EmitSentenceByIndex(filter, ENTINDEX(entity), CHAN_STATIC, sentenceIndex, fvol, SNDLVL_NORM, 0, pitch);
		}
	}

	return sentenceIndex;
}

void UTIL_LazEmitSoundSuit(CBasePlayer *entity, const char *sample)
{
	float fvol;
	int pitch = PITCH_NORM;

	fvol = 0.5f;
	if (random->RandomInt(0, 1))
		pitch = random->RandomInt(0, 6) + 98;

	CAI_TimedSemaphore *pSemaphore = (entity->GetTeamNumber() == LazuulRules()->GetProtaganistTeam()) ? &g_AIFriendliesTalkSemaphore : &g_AIFoesTalkSemaphore;

	// If friendlies are talking, reduce the volume of the suit
	if (!pSemaphore->IsAvailable(nullptr))
	{
		fvol *= 0.3;
	}

	if (fvol > 0.05)
	{
		CSingleUserRecipientFilter filter(entity);
		filter.MakeReliable();

		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = sample;
		ep.m_flVolume = fvol;
		ep.m_SoundLevel = SNDLVL_NORM;
		ep.m_nPitch = pitch;

		CBaseEntity::EmitSound(filter, ENTINDEX(entity), ep);
	}
}

void CLaz_Player::CheckSuitUpdate()
{
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	// Ignore suit updates if no suit
	if (!IsSuitEquipped())
		return;

	if (IsBot())
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
	{
		const char *pchVar = engine->GetClientConVarValue(entindex(), "cl_laz_mp_suit");
		// don't bother updating HEV voice in multiplayer.
		if (atoi(pchVar) <= 0)
			return;
	}

	if (gpGlobals->curtime >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		// play a sentence off of the end of the queue
		for (int i = 0; i < CSUITPLAYLIST; i++)
		{
			if ((isentence = m_rgSuitPlayList[isearch]) != 0)
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;
			if (isentence > 0)
			{
				// play sentence number

				char sentence[512];
				V_sprintf_safe(sentence, "!%s", engine->SentenceNameFromIndex(isentence));
				UTIL_LazEmitSoundSuit(this, sentence);
			}
			else
			{
				// play sentence group
				UTIL_EmitGroupIDSuit(edict(), -isentence);
			}
			m_flSuitUpdate = gpGlobals->curtime + SUITUPDATETIME;
		}
		else
			// queue is empty, don't check
			m_flSuitUpdate = 0;
	}
}


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

	if (IsBot())
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		const char *pchVar = engine->GetClientConVarValue(entindex(), "cl_laz_mp_suit");
		// don't bother updating HEV voice in multiplayer.
		if (atoi(pchVar) <= 0)
			return;
	}


	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}

	if (m_iszSuitVoice == gm_iszDefaultAbility)
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

void CLaz_Player::SetAnimation(PLAYER_ANIM playerAnim)
{
	BaseClass::SetAnimation(playerAnim);

	if (playerAnim == PLAYER_RELOAD && GetEnemy())
	{
		SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_RELOAD);
	}
}

void CLaz_Player::PlayerDeathThink(void)
{
}

Class_T CLaz_Player::Classify()
{
	if (GetTeam())
		return g_aTeamPlayerClasses[GetTeamNumber()];

	return CLASS_PLAYER;
}

CEconEntity* CLaz_Player::GiveItemById(int iItem)
{
	for (int i = 0; i < m_hMyWearables.Count(); i++)
	{
		if (m_hMyWearables[i]->GetItemID() == iItem)
			return nullptr;
	}

	CEconItemDefinition* pItem = GetItemSchema()->GetItemDefinition(iItem);
	if (!pItem)
		return nullptr;

	CBaseEntity* pEnt = CreateNoSpawn(pItem->item_class, GetAbsOrigin(), vec3_angle);
	if (!pEnt)
		return nullptr;

	CEconEntity* pEcon = assert_cast<CEconEntity*> (pEnt);
	CEconItemView eItem(iItem);
	pEcon->SetItem(eItem);
	pEcon->Spawn();
	pEcon->GiveTo(this);

	return pEcon;
}

int CLaz_Player::OnTakeDamage(const CTakeDamageInfo & inputInfo)
{
	CTakeDamageInfo inputInfoCopy(inputInfo);

	if (inputInfoCopy.GetDamageType() & DMG_BULLET)
	{
		float flScale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT(flScale, mult_dmgtaken_from_bullets);
		inputInfoCopy.ScaleDamage(flScale);
	}

	if (inputInfoCopy.GetDamageType() & DMG_BLAST)
	{
		float flScale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT(flScale, mult_dmgtaken_from_explosions);
		inputInfoCopy.ScaleDamage(flScale);
	}

	if (inputInfoCopy.GetDamageType() & DMG_BURN)
	{
		float flScale = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT(flScale, mult_dmgtaken_from_fire);
		inputInfoCopy.ScaleDamage(flScale);
	}

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

void CLaz_Player::TraceAttack(const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator)
{
	CTakeDamageInfo subInfo = info;
	if (ptr->hitgroup == HITGROUP_HEAD && info.GetDamageType() & DMG_SNIPER)
	{
		subInfo.SetDamageCustom(TF_DMG_CUSTOM_HEADSHOT);
	}

	BaseClass::TraceAttack(subInfo, vecDir, ptr, pAccumulator);
}

void CLaz_Player::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendCriteria(set);

	set.AppendCriteria("voice", STRING(m_iszVoiceType));
	set.AppendCriteria("suitvoice", STRING(m_iszSuitVoice));

	gender_t gender = soundemitterbase->GetActorGender(STRING(GetModelName()));
	set.AppendCriteria("femalemodel", (gender == GENDER_FEMALE) ? "1" : "0");

	if (GetEnemy())
	{
		CBaseEntity* pEnemy = GetEnemy();
		set.AppendCriteria("playerenemy", pEnemy->GetResponseClassname(this));
		set.AppendCriteria("playerenemyclass", g_pGameRules->AIClassText(pEnemy->Classify()));
		float healthfrac = 0.0f;
		if (pEnemy->GetMaxHealth() > 0)
		{
			healthfrac = (float)pEnemy->GetHealth() / (float)pEnemy->GetMaxHealth();
		}

		set.AppendCriteria("playerenemyhealthfrac", CFmtStr("%.3f", healthfrac));
	}
	else
	{
		set.AppendCriteria("playerenemy", "none");
	}
}

const char* CLaz_Player::GetResponseClassname(CBaseEntity* pCaller)
{
	if (pCaller != this)
	{
		if (m_iszResponseClassname != gm_iszDefaultAbility && m_iszResponseClassname != NULL_STRING)
		{
			return STRING(m_iszResponseClassname);
		}
		else if (m_iszVoiceType != gm_iszDefaultAbility && m_iszVoiceType != NULL_STRING)
		{
			static CFmtStr str;
			str.Clear();
			str.AppendFormat("npc_%s", STRING(m_iszVoiceType));
			return str.Access();
		}
	}

	return BaseClass::GetResponseClassname(pCaller);
}

void CLaz_Player::DeathNotice(CBaseEntity * pVictim)
{
	if (pVictim == m_hMinion.Get())
		ClearMinion();
}

void CLaz_Player::SetFootsteps(const char *pchPrefix)
{
	if (0 == Q_strcmp(DEFAULT_ABILITY, pchPrefix))
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
		{ STATE_WELCOME,				"TF_STATE_WELCOME",				&CLaz_Player::StateEnterWELCOME,				NULL,	&CLaz_Player::StateThinkWELCOME },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CLaz_Player::State_Enter_OBSERVER_MODE,	NULL, &CLaz_Player::State_PreThink_OBSERVER_MODE },
		{ STATE_DYING,				"TF_STATE_DYING",				&CLaz_Player::StateEnterDYING,				NULL,	&CLaz_Player::StateThinkDYING },
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
	if (!IsFakeClient())
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
	AddFlag(FL_NOTARGET);

	CUtlVector<CHandle<CEconWearable > > vecWearables;
	vecWearables.AddVectorToTail(m_hMyWearables);
	for (int i = 0; i < vecWearables.Count(); i++)
	{
		RemoveWearable(vecWearables[i]);
		UTIL_Remove(vecWearables[i].Get());
	}
}

void CLaz_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert(m_takedamage == DAMAGE_NO);
	Assert(IsSolidFlagSet(FSOLID_NOT_SOLID));
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert(m_lifeState == LIFE_DEAD || m_lifeState == LIFE_RESPAWNABLE);
	Assert(pl.deadflag);
}

void CLaz_Player::StateEnterWELCOME(void)
{
	//PickWelcomeObserverPoint();

	StartObserverMode(OBS_MODE_FIXED);

	// Important to set MOVETYPE_NONE or our physics object will fall while we're sitting at one of the intro cameras.
	SetMoveType(MOVETYPE_NONE);
	AddSolidFlags(FSOLID_NOT_SOLID);
	AddEffects(EF_NODRAW | EF_NOSHADOW);

	//PhysObjectSleep();

	if (gpGlobals->eLoadType == MapLoad_Background)
	{
		//m_bSeenRoundInfo = true;

		ChangeTeam(TEAM_SPECTATOR);
	}
	/*else if ((TFGameRules() && TFGameRules()->IsLoadingBugBaitReport()))
	{
		m_bSeenRoundInfo = true;

		ChangeTeam(TF_TEAM_BLUE);
		SetDesiredPlayerClassIndex(TF_CLASS_SCOUT);
		ForceRespawn();
	}
	else if (IsInCommentaryMode())
	{
		m_bSeenRoundInfo = true;
	}*/
	else
	{
		//if (!IsX360())
		//{
		//	KeyValues *data = new KeyValues("data");
		//	data->SetString("title", "#TF_Welcome");	// info panel title
		//	data->SetString("type", "1");				// show userdata from stringtable entry
		//	data->SetString("msg", "motd");			// use this stringtable entry
		//	data->SetString("cmd", "mapinfo");		// exec this command if panel closed

		//	ShowViewPortPanel(PANEL_INFO, true, data);

		//	data->deleteThis();
		//}
		//else
		//{
		//	ShowViewPortPanel(PANEL_MAPINFO, true);
		//}

		//m_bSeenRoundInfo = false;

		ShowViewPortPanel(PANEL_TEAM, true);
	}

	//m_bIsIdle = false;
}

void CLaz_Player::StateThinkWELCOME(void)
{
	if (IsBot() && GetTeamNumber() < FIRST_GAME_TEAM)
	{
		int iTeam = GetAutoTeam();
		HandleCommand_JoinTeam(iTeam);
	}
}

void CLaz_Player::StateEnterDYING(void)
{
	SetMoveType(MOVETYPE_NONE);
	AddSolidFlags(FSOLID_NOT_SOLID);
}

void CLaz_Player::StateThinkDYING(void)
{
	// If we have a ragdoll, it's time to go to deathcam
	if (/*!m_bAbortFreezeCam &&*/ m_hRagdoll &&
		(m_lifeState == LIFE_DYING || m_lifeState == LIFE_DEAD) &&
		GetObserverMode() != OBS_MODE_FREEZECAM)
	{
		if (GetObserverMode() != OBS_MODE_DEATHCAM)
		{
			StartObserverMode(OBS_MODE_DEATHCAM);	// go to observer mode
		}
		//RemoveEffects(EF_NODRAW | EF_NOSHADOW);	// still draw player body
	}

	float flTimeInFreeze = spec_freeze_traveltime.GetFloat() + spec_freeze_time.GetFloat();
	float flFreezeEnd = (m_flDeathTime + TF_DEATH_ANIMATION_TIME + flTimeInFreeze);
	if (!m_bPlayedFreezeCamSound  && GetObserverTarget() && GetObserverTarget() != this)
	{
		// Start the sound so that it ends at the freezecam lock on time
		float flFreezeSoundLength = 0.3;
		float flFreezeSoundTime = (m_flDeathTime + TF_DEATH_ANIMATION_TIME) + spec_freeze_traveltime.GetFloat() - flFreezeSoundLength;
		if (gpGlobals->curtime >= flFreezeSoundTime)
		{
			CSingleUserRecipientFilter filter(this);
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "TFPlayer.FreezeCam";
			EmitSound(filter, entindex(), params);

			m_bPlayedFreezeCamSound = true;
		}
	}

	if (gpGlobals->curtime >= (m_flDeathTime + TF_DEATH_ANIMATION_TIME))	// allow x seconds death animation / death cam
	{
		if (GetObserverTarget() && GetObserverTarget() != this)
		{
			if (/*!m_bAbortFreezeCam &&*/ gpGlobals->curtime < flFreezeEnd)
			{
				if (GetObserverMode() != OBS_MODE_FREEZECAM)
				{
					StartObserverMode(OBS_MODE_FREEZECAM);
					//PhysObjectSleep();
				}
				return;
			}
		}

		if (GetObserverMode() == OBS_MODE_FREEZECAM)
		{
			// If we're in freezecam, and we want out, abort.  (only if server is not using mp_fadetoblack)
			//if (m_bAbortFreezeCam && !mp_fadetoblack.GetBool())
			//{
			//	if (m_hObserverTarget == NULL)
			//	{
			//		// find a new observer target
			//		CheckObserverSettings();
			//	}

			//	FindInitialObserverTarget();
			//	SetObserverMode(OBS_MODE_CHASE);
			//	ShowViewPortPanel("specgui", ModeWantsSpectatorGUI(OBS_MODE_CHASE));
			//}
		}

		// Don't allow anyone to respawn until freeze time is over, even if they're not
		// in freezecam. This prevents players skipping freezecam to spawn faster.
		if (gpGlobals->curtime < flFreezeEnd)
			return;

		if (gpGlobals->maxClients <= 1)
		{
			::respawn(this, false);
			return;
		}

		m_lifeState = LIFE_RESPAWNABLE;

		StopAnimation();

		AddEffects(EF_NOINTERP);

		if (GetMoveType() != MOVETYPE_NONE && (GetFlags() & FL_ONGROUND))
			SetMoveType(MOVETYPE_NONE);

		State_Transition(STATE_OBSERVER_MODE);
	}
}


void CLaz_Player::State_Enter_ACTIVE()
{
	SetMoveType(MOVETYPE_WALK);
	RemoveEffects(EF_NODRAW | EF_NOSHADOW);
	RemoveSolidFlags(FSOLID_NOT_SOLID);
	RemoveFlag(FL_NOTARGET);
	m_Local.m_iHideHUD = 0;
	//PhysObjectWake();
	if (GetPlayerSquad())
	{
		CAI_Squad *pSquad = GetPlayerSquad();
		for (int i = 0; i < pSquad->NumMembers(); i++)
		{
			CNPC_PlayerFollower *pNPC = dynamic_cast<CNPC_PlayerFollower *> (pSquad->GetMember(i));
			if (pNPC)
			{
				CAI_BaseNPC *pAlly[1] = { pNPC };
				pNPC->TargetOrder(this, pAlly, 1);
			}
		}
	}
}


void CLaz_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death

	CBaseEntity *pOldEnemy = GetEnemy();
	if (ChooseEnemy() && !pOldEnemy)
	{
		SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_BATTLECRY);
		if (m_AnnounceAttackTimer.Expired())
		{
			// Always delay when an encounter begins
			m_AnnounceAttackTimer.Set(4, 8);
		}
	}

	// Time to finish the current random expression? Or time to pick a new one?
	if (IsAlive() && /*m_flNextRandomExpressionTime >= 0 &&*/ gpGlobals->curtime > m_flNextRandomExpressionTime)
	{
		UpdateExpression();
	}
}

class CObserverPoint : public CPointEntity
{
	DECLARE_CLASS(CObserverPoint, CPointEntity);
public:
	DECLARE_DATADESC();

	virtual void Activate(void)
	{
		BaseClass::Activate();

		if (m_iszAssociateTeamEntityName != NULL_STRING)
		{
			m_hAssociatedTeamEntity = gEntList.FindEntityByName(NULL, m_iszAssociateTeamEntityName);
			if (!m_hAssociatedTeamEntity)
			{
				Warning("info_observer_point (%s) couldn't find associated team entity named '%s'\n", GetDebugName(), STRING(m_iszAssociateTeamEntityName));
			}
		}
	}

	bool CanUseObserverPoint(CLaz_Player *pPlayer)
	{
		if (m_bDisabled)
			return false;

		if (m_hAssociatedTeamEntity && (mp_forcecamera.GetInt() == OBS_ALLOW_TEAM))
		{
			// If we don't own the associated team entity, we can't use this point
			if (m_hAssociatedTeamEntity->GetTeamNumber() != pPlayer->GetTeamNumber() && pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM)
				return false;
		}

		// Only spectate observer points on control points in the current miniround
		if (g_pObjectiveResource->PlayingMiniRounds() && m_hAssociatedTeamEntity)
		{
			CTeamControlPoint *pPoint = dynamic_cast<CTeamControlPoint*>(m_hAssociatedTeamEntity.Get());
			if (pPoint)
			{
				bool bInRound = g_pObjectiveResource->IsInMiniRound(pPoint->GetPointIndex());
				if (!bInRound)
					return false;
			}
		}

		return true;
	}

	virtual int UpdateTransmitState()
	{
		return SetTransmitState(FL_EDICT_ALWAYS);
	}

	void InputEnable(inputdata_t &inputdata)
	{
		m_bDisabled = false;
	}
	void InputDisable(inputdata_t &inputdata)
	{
		m_bDisabled = true;
	}
	bool IsDefaultWelcome(void) { return m_bDefaultWelcome; }

public:
	bool		m_bDisabled;
	bool		m_bDefaultWelcome;
	EHANDLE		m_hAssociatedTeamEntity;
	string_t	m_iszAssociateTeamEntityName;
	float		m_flFOV;
};

BEGIN_DATADESC(CObserverPoint)
DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_bDefaultWelcome, FIELD_BOOLEAN, "defaultwelcome"),
DEFINE_KEYFIELD(m_iszAssociateTeamEntityName, FIELD_STRING, "associated_team_entity"),
DEFINE_KEYFIELD(m_flFOV, FIELD_FLOAT, "fov"),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
END_DATADESC()

LINK_ENTITY_TO_CLASS(info_observer_point, CObserverPoint);

//-----------------------------------------------------------------------------
// Purpose: Builds a list of entities that this player can observe.
//			Returns the index into the list of the player's current observer target.
//-----------------------------------------------------------------------------
int CLaz_Player::BuildObservableEntityList(void)
{
	m_hObservableEntities.Purge();
	int iCurrentIndex = -1;

	// Add all the map-placed observer points
	CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname(NULL, "info_observer_point");
	while (pObserverPoint)
	{
		m_hObservableEntities.AddToTail(pObserverPoint);

		if (m_hObserverTarget.Get() == pObserverPoint)
		{
			iCurrentIndex = (m_hObservableEntities.Count() - 1);
		}

		pObserverPoint = gEntList.FindEntityByClassname(pObserverPoint, "info_observer_point");
	}

	// Add all the players
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer)
		{
			m_hObservableEntities.AddToTail(pPlayer);

			if (m_hObserverTarget.Get() == pPlayer)
			{
				iCurrentIndex = (m_hObservableEntities.Count() - 1);
			}
		}
	}

	// Add all npcs
	int iNumNPCs = g_AI_Manager.NumAIs();
	for (int i = 0; i < iNumNPCs; i++)
	{
		CAI_BaseNPC *pNPC = g_AI_Manager.AccessAIs()[i];
		if (pNPC)
		{
			m_hObservableEntities.AddToTail(pNPC);

			if (m_hObserverTarget.Get() == pNPC)
			{
				iCurrentIndex = (m_hObservableEntities.Count() - 1);
			}
		}
	}

	return iCurrentIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CLaz_Player::GetNextObserverSearchStartPoint(bool bReverse)
{
	int iDir = bReverse ? -1 : 1;
	int startIndex = BuildObservableEntityList();
	int iMax = m_hObservableEntities.Count() - 1;

	startIndex += iDir;
	if (startIndex > iMax)
		startIndex = 0;
	else if (startIndex < 0)
		startIndex = iMax;

	return startIndex;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CLaz_Player::FindNextObserverTarget(bool bReverse)
{
	int startIndex = GetNextObserverSearchStartPoint(bReverse);

	int	currentIndex = startIndex;
	int iDir = bReverse ? -1 : 1;

	int iMax = m_hObservableEntities.Count() - 1;

	// Make sure the current index is within the max. Can happen if we were previously
	// spectating an object which has been destroyed.
	if (startIndex > iMax)
	{
		currentIndex = startIndex = 1;
	}

	do
	{
		CBaseEntity *nextTarget = m_hObservableEntities[currentIndex];

		if (IsValidObserverTarget(nextTarget))
			return nextTarget;

		currentIndex += iDir;

		// Loop through the entities
		if (currentIndex > iMax)
		{
			currentIndex = 0;
		}
		else if (currentIndex < 0)
		{
			currentIndex = iMax;
		}
	} while (currentIndex != startIndex);

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLaz_Player::IsValidObserverTarget(CBaseEntity * target)
{
	if (target && !target->IsPlayer())
	{
		CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
		if (pObsPoint && !pObsPoint->CanUseObserverPoint(this))
			return false;

		if (target->IsNPC())
		{
			CAI_BaseNPC *pAI = target->MyNPCPointer();
			bool bImportant = false;
			if (pAI == m_hMinion.Get())
				bImportant = true;
			else if (pAI->ShowInDeathnotice())
				bImportant = true;
			else if (pAI->IsInPlayerSquad())
			{
				CNPC_PlayerFollower *pFollower = dynamic_cast<CNPC_PlayerFollower *> (pAI);
				if (pFollower && pFollower->IsInThisPlayerSquad(this))
					bImportant = true;
			}

			// Only spectate important npcs
			if (!bImportant)
				return false;

			// Don't spectate sleeping npcs
			if (pAI->GetSleepState() != AISS_AWAKE)
				return false;

			if (pAI->GetEffects() & EF_NODRAW)
				return false;

			// Don't spectate burrowed npcs
			if (pAI->GetFlags() & FL_NOTARGET)
				return false;

			// Don't spectate npcs waiting for a script
			if (pAI->IsCurSchedule(SCHED_WAIT_FOR_SCRIPT, false))
				return false;
		}

		if (GetTeamNumber() == TEAM_SPECTATOR)
			return true;

		switch (mp_forcecamera.GetInt())
		{
		case OBS_ALLOW_ALL:	break;
		case OBS_ALLOW_TEAM:	if (target->GetTeamNumber() != TEAM_UNASSIGNED && GetTeamNumber() != target->GetTeamNumber())
			return false;
			break;
		case OBS_ALLOW_NONE:	return false;
		}

		return true;
	}

	return BaseClass::IsValidObserverTarget(target);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLaz_Player::SetObserverTarget(CBaseEntity *target)
{
	ClearZoomOwner();
	SetFOV(this, 0);

	if (!BaseClass::SetObserverTarget(target))
		return false;

	CObserverPoint *pObsPoint = dynamic_cast<CObserverPoint *>(target);
	if (pObsPoint)
	{
		SetViewOffset(vec3_origin);
		JumptoPosition(target->GetAbsOrigin(), target->EyeAngles());
		SetFOV(pObsPoint, pObsPoint->m_flFOV);
	}
	/*else if (target->IsBaseObject() || target->IsNPC())
	{
		if (m_iObserverMode == OBS_MODE_IN_EYE)
		{
			m_iObserverMode = OBS_MODE_CHASE;
		}
	}*/

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest team member within the distance of the origin.
//			Favor players who are the same class.
//-----------------------------------------------------------------------------
CBaseEntity *CLaz_Player::FindNearestObservableTarget(Vector vecOrigin, float flMaxDist)
{
	BuildObservableEntityList();
	CBaseEntity *pReturnTarget = NULL;
	bool bFoundClass = false;
	float flCurDistSqr = (flMaxDist * flMaxDist);
	int iNumPlayers = m_hObservableEntities.Count();

	for (int i = 0; i < iNumPlayers; i++)
	{
		CBaseEntity *pTarget = m_hObservableEntities.Element(i);

		if (!pTarget)
			continue;

		if (!IsValidObserverTarget(pTarget))
			continue;

		float flDistSqr = (pTarget->GetAbsOrigin() - vecOrigin).LengthSqr();

		if (flDistSqr < flCurDistSqr)
		{
			// If we've found a player matching our class already, this guy needs
			// to be a matching class and closer to boot.
			//if (!bFoundClass || pPlayer->IsPlayerClass(GetPlayerClass()->GetClassIndex()))
			{
				pReturnTarget = pTarget;
				flCurDistSqr = flDistSqr;
				bFoundClass = true;
			}
		}
	}

	return pReturnTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaz_Player::FindInitialObserverTarget(void)
{
	// If we're on a team (i.e. not a pure observer), try and find
	// a target that'll give the player the most useful information.
	if (GetTeamNumber() >= FIRST_GAME_TEAM)
	{
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
		if (pMaster)
		{
			// Has our forward cap point been contested recently?
			int iFarthestPoint = LazuulRules()->GetFarthestOwnedControlPoint(GetTeamNumber(), false);
			if (iFarthestPoint != -1)
			{
				float flTime = pMaster->PointLastContestedAt(iFarthestPoint);
				if (flTime != -1 && flTime > (gpGlobals->curtime - 30))
				{
					// Does it have an associated viewpoint?
					CBaseEntity *pObserverPoint = gEntList.FindEntityByClassname(NULL, "info_observer_point");
					while (pObserverPoint)
					{
						CObserverPoint *pObsPoint = assert_cast<CObserverPoint *>(pObserverPoint);
						if (pObsPoint && pObsPoint->m_hAssociatedTeamEntity == pMaster->GetControlPoint(iFarthestPoint))
						{
							if (IsValidObserverTarget(pObsPoint))
							{
								m_hObserverTarget.Set(pObsPoint);
								return;
							}
						}

						pObserverPoint = gEntList.FindEntityByClassname(pObserverPoint, "info_observer_point");
					}
				}
			}

			// Has the point beyond our farthest been contested lately?
			iFarthestPoint += (ObjectiveResource()->GetBaseControlPointForTeam(GetTeamNumber()) == 0 ? 1 : -1);
			if (iFarthestPoint >= 0 && iFarthestPoint < MAX_CONTROL_POINTS)
			{
				float flTime = pMaster->PointLastContestedAt(iFarthestPoint);
				if (flTime != -1 && flTime > (gpGlobals->curtime - 30))
				{
					// Try and find a player near that cap point
					CBaseEntity *pCapPoint = pMaster->GetControlPoint(iFarthestPoint);
					if (pCapPoint)
					{
						CBaseEntity *pTarget = FindNearestObservableTarget(pCapPoint->GetAbsOrigin(), 1500);
						if (pTarget)
						{
							m_hObserverTarget.Set(pTarget);
							return;
						}
					}
				}
			}
		}
	}

	if (m_hMinion.Get())
	{
		m_hObserverTarget = m_hMinion;
		return;
	}

	// Find the nearest guy near myself
	CBaseEntity *pTarget = FindNearestObservableTarget(GetAbsOrigin(), FLT_MAX);
	if (pTarget)
	{
		m_hObserverTarget.Set(pTarget);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLaz_Player::ValidateCurrentObserverTarget(void)
{
	// If our current target is a dead player who's gibbed / died, refind as if 
	// we were finding our initial target, so we end up somewhere useful.
	if (m_hObserverTarget && m_hObserverTarget->IsPlayer())
	{
		CBasePlayer *player = ToBasePlayer(m_hObserverTarget);

		if (player->m_lifeState == LIFE_DEAD || player->m_lifeState == LIFE_DYING)
		{
			// Once we're past the pause after death, find a new target
			if ((player->GetDeathTime() + DEATH_ANIMATION_TIME) < gpGlobals->curtime)
			{
				FindInitialObserverTarget();
			}

			return;
		}
	}

	if (m_hObserverTarget && (m_hObserverTarget->IsBaseObject() || m_hObserverTarget->IsNPC()))
	{
		if (m_iObserverMode == OBS_MODE_IN_EYE)
		{
			m_iObserverMode = OBS_MODE_CHASE;
			SetObserverTarget(m_hObserverTarget);
			SetMoveType(MOVETYPE_OBSERVER);
			CheckObserverSettings();
			//ForceObserverMode( OBS_MODE_CHASE ); // We'll leave this in in case something screws up
		}
	}

	BaseClass::ValidateCurrentObserverTarget();
}

void CLaz_Player::CheckObserverSettings()
{
	BaseClass::CheckObserverSettings();
}

CON_COMMAND_F(laz_player_set_voice, "Set the voicetype of the player", FCVAR_DEVELOPMENTONLY|FCVAR_CHEAT)
{
	if (args.ArgC() < 2)
		return;

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
		return;

	CLaz_Player *pLaz = ToLazuulPlayer(pPlayer);
	if (!pLaz)
		return;

	pLaz->SetVoiceType(args[1], args[2]);
}