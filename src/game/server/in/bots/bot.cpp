//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_player.h"

#include "in_gamerules.h"
#include "players_system.h"
#include "in_utils.h"
#else
#include "in\in_utils.h"
#include "basemultiplayerplayer.h"
#endif

#include "bots\bot_defs.h"
#include "bots\squad_manager.h"
#include "bots\bot_manager.h"

#include "nav.h"
#include "nav_mesh.h"
#include "nav_area.h"

#include "fmtstr.h"
#include "in_buttons.h"

#include "ai_hint.h"
#include "movehelper_server.h"

#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

//#define Msg(...) Log_Msg(LOG_BOTS, __VA_ARGS__)
//#define Warning(...) Log_Warning(LOG_BOTS, __VA_ARGS__)

//================================================================================
// Commands
//================================================================================

DECLARE_DEBUG_CMD(bot_frozen, "0", "")
DECLARE_DEBUG_CMD(bot_crouch, "0", "")
DECLARE_DEBUG_CMD(bot_flashlight, "0", "")
DECLARE_DEBUG_CMD(bot_mimic, "0", "")
DECLARE_DEBUG_CMD(bot_aim_player, "0", "")
DECLARE_DEBUG_CMD(bot_primary_attack, "0", "")

DECLARE_DEBUG_CMD(bot_sendcmd, "", "Forces bots to send the specified command.");
DECLARE_DEBUG_CMD(bot_team, "0", "Force all bots created with bot_add to change to the specified team")

DECLARE_DEBUG_CMD(bot_notarget, "0", "")
DECLARE_DEBUG_CMD(bot_god, "0", "")
DECLARE_DEBUG_CMD(bot_buddha, "0", "")
DECLARE_DEBUG_CMD(bot_dont_attack, "0", "")

DECLARE_DEBUG_CMD(bot_debug, "0", "")
DECLARE_DEBUG_CMD(bot_debug_locomotion, "0", "")
DECLARE_DEBUG_CMD(bot_debug_jump, "0", "")
DECLARE_DEBUG_CMD(bot_debug_memory, "0", "")

DECLARE_DEBUG_CMD(bot_debug_cmd, "0", "")
DECLARE_DEBUG_CMD(bot_debug_conditions, "0", "")
DECLARE_DEBUG_CMD(bot_debug_desires, "0", "")
DECLARE_DEBUG_CMD(bot_debug_max_msgs, "10", "")
DECLARE_DEBUG_CMD(bot_debug_data_memory, "0", "")

DECLARE_DEBUG_CMD(bot_optimize, "0", "")
DECLARE_SERVER_CMD(bot_far_distance, "2500", "");

//================================================================================
// Macros
//================================================================================

// Only a utility macro to check if the current state is finished
#define STATE_FINISHED m_iStateTimer.HasStarted() && m_iStateTimer.IsElapsed()

//================================================================================
// It allows to create a bot with the name and position specified.
//================================================================================
CBasePlayer *CreateBot(const char *pName, const Vector *vecPosition, const QAngle *angles)
{
	// Select a random name
	if (pName == NULL) {
		pName = g_charBotNames[RandomInt(0, ARRAYSIZE(g_charBotNames) - 1)];
		pName = UTIL_VarArgs("%s Bot", pName);
	}

	edict_t *pSoul = engine->CreateFakeClient(pName);

	if (pSoul == NULL) {
		Warning("There was a problem creating a bot. Maybe there is no more space for players on the server.\n");
		return NULL;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pSoul);
	Assert(pPlayer);

	if (pPlayer == NULL) {
		return NULL;
	}

	pPlayer->ClearFlags();
	pPlayer->AddFlag(FL_CLIENT | FL_FAKECLIENT);

	// This is where we implement the AI
	pPlayer->SetUpBot();
	Assert(pPlayer->GetBotController());

	if (pPlayer->GetBotController() == NULL) {
		Warning("There was a problem creating a bot. The player was created but the controller could not be created.\n");
		pPlayer->Kick();
		return NULL;
	}

	pPlayer->Spawn();

	if (vecPosition) {
		pPlayer->Teleport(vecPosition, angles, NULL);
	}

	++g_botID;
	return pPlayer;
}

//================================================================================
//================================================================================
CBot::CBot(CBasePlayer *parent) : BaseClass(parent)
{
}

//================================================================================
// Called every spawn (duh)
//================================================================================
void CBot::Spawn()
{
	Assert(GetProfile());

	m_nComponents.Purge();
	m_nSchedules.Purge();

	SetUpComponents();
	SetUpSchedules();

	FOR_EACH_COMPONENT
	{
		m_nComponents[it]->Reset();
	}

	m_cmd = NULL;
	m_lastCmd = NULL;

	m_iState = STATE_IDLE;
	m_iTacticalMode = TACTICAL_MODE_NONE;
	m_iStateTimer.Invalidate();

	m_nActiveSchedule = NULL;
	m_nConditions.ClearAll();

	m_iRepeatedDamageTimes = 0;
	m_flDamageAccumulated = 0.0f;

	m_bConditionsBlocked = false;

	if (GetMemory()) {
		GetMemory()->UpdateDataMemory(MEMORY_SPAWN_POSITION, GetHost()->GetAbsOrigin());
	}
}

//================================================================================
// Called every frame
//================================================================================
void CBot::Update()
{
	Upkeep();

	if (CanRunAI()) {
		ResetCommand();
		RunAI();
	}

	DebugDisplay();

	PlayerMove(m_cmd);
}

//================================================================================
// Create a fresh command, ready to be filled with inputs
//================================================================================
void CBot::ResetCommand()
{
	m_cmd = new CUserCmd();
	m_cmd->viewangles = GetHost()->EyeAngles();
}

//================================================================================
// Simulates all input as if it were a player
//================================================================================
void CBot::PlayerMove(CUserCmd *cmd)
{
	VPROF_BUDGET("CBot::PlayerMove", VPROF_BUDGETGROUP_BOTS);

	if (!cmd)
		return;

	m_lastCmd = cmd;

	// This is not necessary if the player is a human
	if (!GetHost()->IsBot())
		return;

#ifdef INSOURCE_DLL
	PostClientMessagesSent();
#else
	GetHost()->RemoveEffects(EF_NOINTERP);
#endif

	// Save off the CUserCmd to execute later
	GetHost()->ProcessUsercmds(cmd, 1, 1, 0, false);
}

//================================================================================
// Returns if we can process the expensive AI
//================================================================================
bool CBot::CanRunAI()
{
	if (bot_frozen.GetBool())
		return false;

	if (bot_mimic.GetInt() > 0)
		return false;

	if (!GetHost()->IsAlive())
		return false;

	if (GetHost()->IsMarkedForDeletion())
		return false;

	if (IsPanicked())
		return false;

	AssertMsg(GetDecision(), "Bot without decision component!");

	if (!GetDecision())
		return false;

	if (GetLocomotion() && TheNavMesh->GetNavAreaCount() == 0)
		return false;

	if (GetFollow() && GetFollow()->IsFollowingBot()) {
		CBasePlayer *pLeader = ToInPlayer(GetFollow()->GetEntity());

		if (pLeader && pLeader->GetBotController()) {
			return pLeader->GetBotController()->CanRunAI();
		}
	}

#ifdef INSOURCE_DLL
	if (!GetHost()->IsActive())
		return false;

	if (GetPerformance() == BOT_PERFORMANCE_PVS) {
		CHumanPVSFilter filter(GetAbsOrigin());

		if (filter.GetRecipientCount() == 0)
			return false;
	}
#endif

	if (((gpGlobals->tickcount + GetHost()->entindex()) % 2) == 0)
		return true;

	return false;
}

//================================================================================
// Cheap AI for Upkeep
//================================================================================
void CBot::Upkeep()
{
	VPROF_BUDGET("CBot::Upkeep", VPROF_BUDGETGROUP_BOTS);

	m_flStartTime = Plat_FloatTime();

	// We reset our alert status
	if (STATE_FINISHED) {
		CleanState();
	}

	// We just want to mimic a human
	if (bot_mimic.GetInt() > 0) {
		MimicThink(bot_mimic.GetInt());
		return;
	}

	// Component's Upkeep
	UpdateComponents(true);
}

//================================================================================
// Run the expensive AI of the Bot.
//================================================================================
void CBot::RunAI()
{
	VPROF_BUDGET("CBot::RunAI", VPROF_BUDGETGROUP_BOTS);

	ClearConditions();

	BlockConditions();

	ApplyDebugCommands();

	GatherConditions();

	UnblockConditions();

	UpdateComponents();

	RunCustomAI();

	UpdateSchedule();
}

//================================================================================
// Run custom AI
//================================================================================
void CBot::RunCustomAI()
{
	VPROF_BUDGET("CBot::RunCustomAI", VPROF_BUDGETGROUP_BOTS);

	// Update the list of safe places to cover
	if (GetDecision()->ShouldUpdateCoverSpots()) {
		GetDecision()->UpdateCoverSpots();
	}

	// We change to the best weapon for this situation
	// TODO: A better place to put this.
	GetDecision()->SwitchToBestWeapon();
}

//================================================================================
//================================================================================
void CBot::UpdateComponents(bool upkeep)
{
	float startTime;

	FOR_EACH_COMPONENT
	{
		IBotComponent *pComponent = m_nComponents[it];

		if (pComponent == GetMemory())
			continue;

		if (upkeep) {
			pComponent->Upkeep();
		}
		else {
			startTime = Plat_FloatTime();
			pComponent->Update();
			pComponent->SetUpdateCost((Plat_FloatTime() - startTime) * 1000.0f);
		}
	}
}

void CBot::OnActorEmoted(CBaseCombatCharacter* emoter, int emote)
{
#ifdef HL2_LAZUL
	const float RangeSqr = (256 * 256);
	CBaseMultiplayerPlayer* pThem = ToBaseMultiplayerPlayer(emoter);
	CBaseMultiplayerPlayer* pUs = ToBaseMultiplayerPlayer(GetHost());
	if (pThem && pUs && emote == MP_CONCEPT_PLAYER_FOLLOW && pThem->InSameTeam(pUs))
	{
		IBotFollow* pFollow = GetFollow();
		if (pFollow)
		{
			if (!pFollow->IsFollowingHuman() && pThem->WorldSpaceCenter().DistToSqr(pUs->WorldSpaceCenter()) <= RangeSqr)
			{
				pFollow->Start(pThem);
				pUs->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_LEAD);
			}
		}
	}
#endif
}

//================================================================================
// Applies actions for debugging
//================================================================================
void CBot::ApplyDebugCommands()
{
	// God mode
	if (bot_god.GetBool()) GetHost()->AddFlag(FL_GODMODE);
	else GetHost()->RemoveFlag(FL_GODMODE);

	// Enemies cant see us
	if (bot_notarget.GetBool()) GetHost()->AddFlag(FL_NOTARGET);
	else GetHost()->RemoveFlag(FL_NOTARGET);

	// Buddha
	if (bot_buddha.GetBool()) GetHost()->m_debugOverlays = GetHost()->m_debugOverlays | OVERLAY_BUDDHA_MODE;
	else GetHost()->m_debugOverlays = GetHost()->m_debugOverlays & ~OVERLAY_BUDDHA_MODE;

	// Forced Crouch
	if (bot_crouch.GetBool()) {
		InjectButton(IN_DUCK);
	}

	// Forced primary attack
	if (bot_primary_attack.GetBool()) {
		InjectButton(IN_ATTACK);
	}

	// Forced flashlight
	if (bot_flashlight.GetBool()) {
		if (!GetHost()->FlashlightIsOn()) {
			GetHost()->FlashlightTurnOn();
		}
	}
	else {
		if (GetHost()->FlashlightIsOn()) {
			GetHost()->FlashlightTurnOff();
		}
	}

	if (GetVision()) {
		// We aim at the host
		if (bot_aim_player.GetBool()) {
			CBasePlayer *pPlayer = UTIL_GetListenServerHost();

			if (pPlayer) {
				GetVision()->LookAt("bot_aim_player", pPlayer->EyePosition(), PRIORITY_UNINTERRUPTABLE, 1.0f);
			}
		}
	}
}

//================================================================================
// Imitates all inputs of the specified player.
//================================================================================
void CBot::MimicThink(int playerIndex)
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex(playerIndex);

	if (!pPlayer)
		return;

	if (!pPlayer->GetLastUserCommand())
		return;

	const CUserCmd *playercmd = pPlayer->GetLastUserCommand();

	m_cmd = new CUserCmd();
	//m_cmd->command_number = playercmd->command_number;
	//m_cmd->tick_count = playercmd->tick_count;
	m_cmd->viewangles = playercmd->viewangles;
	m_cmd->forwardmove = playercmd->forwardmove;
	m_cmd->sidemove = playercmd->sidemove;
	m_cmd->upmove = playercmd->upmove;
	m_cmd->buttons = playercmd->buttons;
	m_cmd->impulse = playercmd->impulse;
	m_cmd->weaponselect = playercmd->weaponselect;
	m_cmd->weaponsubtype = playercmd->weaponsubtype;
	//m_cmd->random_seed = playercmd->random_seed;
	m_cmd->mousedx = playercmd->mousedx;
	m_cmd->mousedy = playercmd->mousedy;

	GetHost()->SnapEyeAngles(m_cmd->viewangles);

	/*CFmtStr msg;
	DebugScreenText(msg.sprintf("command_number: %i", GetUserCommand()->command_number));
	DebugScreenText(msg.sprintf("tick_count: %i", GetUserCommand()->tick_count));
	DebugScreenText(msg.sprintf("viewangles: %.2f, %.2f", GetUserCommand()->viewangles.x, GetUserCommand()->viewangles.y));
	DebugScreenText(msg.sprintf("forwardmove: %.2f", GetUserCommand()->forwardmove));
	DebugScreenText(msg.sprintf("sidemove: %.2f", GetUserCommand()->sidemove));
	DebugScreenText(msg.sprintf("upmove: %.2f", GetUserCommand()->upmove));
	DebugScreenText(msg.sprintf("buttons:%i", (int)GetUserCommand()->buttons));
	DebugScreenText(msg.sprintf("impulse: %i", (int)GetUserCommand()->impulse));
	DebugScreenText(msg.sprintf("weaponselect: %i", GetUserCommand()->weaponselect));
	DebugScreenText(msg.sprintf("weaponsubtype: %i", GetUserCommand()->weaponsubtype));
	DebugScreenText(msg.sprintf("random_seed: %i", GetUserCommand()->random_seed));
	DebugScreenText(msg.sprintf("mousedx: %i", (int)GetUserCommand()->mousedx));
	DebugScreenText(msg.sprintf("mousedy: %i", (int)GetUserCommand()->mousedy));
	DebugScreenText(msg.sprintf("hasbeenpredicted: %i", (int)GetUserCommand()->hasbeenpredicted));*/
}

//================================================================================
// Kick the player
//================================================================================
void CBot::Kick()
{
#ifdef INSOURCE_DLL
	GetHost()->Kick();
#else
	engine->ServerCommand(UTIL_VarArgs("kickid %i\n", GetHost()->GetPlayerInfo()->GetUserID()));
#endif
}

//================================================================================
// Move the bot to the desired direction by injecting the command during the current frame.
//================================================================================
void CBot::InjectMovement(NavRelativeDirType direction)
{
	if (!GetUserCommand())
		return;

	switch (direction) {
	case FORWARD:
	default:
		GetUserCommand()->forwardmove = 450.0f;
		InjectButton(IN_FORWARD);
		break;

	case UP:
		GetUserCommand()->upmove = 450.0f;
		break;

	case DOWN:
		GetUserCommand()->upmove = -450.0f;
		break;

	case BACKWARD:
		GetUserCommand()->forwardmove = -450.0f;
		InjectButton(IN_BACK);
		break;

	case LEFT:
		GetUserCommand()->sidemove = -450.0f;
		InjectButton(IN_LEFT);
		break;

	case RIGHT:
		GetUserCommand()->sidemove = 450.0f;
		InjectButton(IN_RIGHT);
		break;
	}
}

//================================================================================
// Injects the command to have pressed a button during the current frame.
//================================================================================
void CBot::InjectButton(int btn)
{
	if (!GetUserCommand())
		return;

	GetUserCommand()->buttons |= btn;
}

//================================================================================
// It allows the Bot to own another player. That is, the control is exchanged.
// TODO: It does not work as it should! And for now I have no idea how to fix it.
//================================================================================
void CBot::Possess(CBasePlayer *pOther)
{
#ifdef INSOURCE_DLL
	GetHost()->Possess(pOther);
#endif
}

//================================================================================
// Returns if the host is watching us in spectator mode.
// It should only be used to debug.
//================================================================================
bool CBot::IsLocalPlayerWatchingMe()
{
#ifdef INSOURCE_DLL
	return GetHost()->IsLocalPlayerWatchingMe();
#else
	if (engine->IsDedicatedServer())
		return false;

	CBasePlayer *pPlayer = UTIL_GetListenServerHost();

	if (!pPlayer)
		return false;

	if (pPlayer->IsAlive() || !pPlayer->IsObserver())
		return false;

	if (pPlayer->GetObserverMode() != OBS_MODE_IN_EYE && pPlayer->GetObserverMode() != OBS_MODE_CHASE)
		return false;

	if (pPlayer->GetObserverTarget() != GetHost())
		return false;

	return true;
#endif
}

//================================================================================

CON_COMMAND_F(bot_add, "Adds a specified number of generic bots", FCVAR_SERVER)
{
	// Look at -count.
	int count = args.FindArgInt("-count", 1);
	count = clamp(count, 1, 16);

	// Ok, spawn all the bots.
	while (--count >= 0) {
		CBasePlayer *pPlayer = CreateBot(NULL, NULL, NULL);
		Assert(pPlayer);

		if (pPlayer) {
			if (bot_team.GetInt() > 0) {
				pPlayer->ChangeTeam(bot_team.GetInt());
			}
		}
	}
}

CON_COMMAND_F(bot_kick, "Kick all bots on the server", FCVAR_SERVER)
{
	for (int it = 0; it <= gpGlobals->maxClients; ++it) {
		CBasePlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(it));

		if (!pPlayer || !pPlayer->IsAlive())
			continue;

		if (!pPlayer->IsBot())
			continue;

		pPlayer->GetBotController()->Kick();
	}
}

CON_COMMAND_F(bot_debug_follow, "It causes all Bots to start following the host", FCVAR_SERVER)
{
	CBasePlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

	if (!pOwner)
		return;

	for (int it = 0; it <= gpGlobals->maxClients; ++it) {
		CBasePlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(it));

		if (!pPlayer || !pPlayer->IsAlive())
			continue;

		if (!pPlayer->IsBot())
			continue;

		IBot *pBot = pPlayer->GetBotController();

		if (pBot->GetFollow()) {
			pBot->GetFollow()->Start(pOwner);
		}
	}
}

CON_COMMAND_F(bot_debug_stop_follow, "Causes all Bots to stop following", FCVAR_SERVER)
{
	for (int it = 0; it <= gpGlobals->maxClients; ++it) {
		CBasePlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(it));

		if (!pPlayer || !pPlayer->IsAlive())
			continue;

		if (!pPlayer->IsBot())
			continue;

		IBot *pBot = pPlayer->GetBotController();

		if (pBot->GetFollow()) {
			pBot->GetFollow()->Stop();
		}
	}
}

CON_COMMAND_F(bot_debug_drive_random, "Orders all bots to move at random sites", FCVAR_SERVER)
{
	if (TheNavAreas.Count() == 0)
		return;

	for (int it = 0; it <= gpGlobals->maxClients; ++it) {
		CBasePlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(it));

		if (!pPlayer || !pPlayer->IsAlive())
			continue;

		if (!pPlayer->IsBot())
			continue;

		IBot *pBot = pPlayer->GetBotController();

		if (!pBot->GetLocomotion())
			continue;

		Vector vecFrom(pPlayer->GetAbsOrigin());
		CNavArea *pArea = NULL;

		while (true) {
			pArea = TheNavAreas[RandomInt(0, TheNavAreas.Count() - 1)];

			if (pArea == NULL)
				continue;

			Vector vecGoal(pArea->GetCenter());

			if (!pBot->GetLocomotion()->IsTraversable(vecFrom, vecGoal))
				continue;

			pBot->GetLocomotion()->DriveTo("bot_debug_drive_random", pArea);
			break;
		}
	}
}

CON_COMMAND_F(bot_debug_drive_player, "Command all bots to move to host location", FCVAR_SERVER)
{
	CBasePlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

	if (!pOwner)
		return;

	CNavArea *pArea = pOwner->GetLastKnownArea();

	if (!pArea)
		return;

	for (int it = 0; it <= gpGlobals->maxClients; ++it) {
		CBasePlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(it));

		if (!pPlayer || !pPlayer->IsAlive())
			continue;

		if (!pPlayer->IsBot())
			continue;

		IBot *pBot = pPlayer->GetBotController();

		if (!pBot->GetLocomotion())
			continue;

		pBot->GetLocomotion()->DriveTo("bot_debug_drive_player", pArea);
	}
}

/*
CON_COMMAND_F( bot_possess, "", FCVAR_SERVER )
{
CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

if ( !pOwner )
return;

if ( args.ArgC() != 2 )
{
Warning( "bot_possess <client index>\n" );
return;
}

int iBotClient = atoi( args[1] );
int iBotEnt = iBotClient + 1;

if ( iBotClient < 0 ||
iBotClient >= gpGlobals->maxClients ||
pOwner->entindex() == iBotEnt )
{
Warning( "bot_possess <client index>\n" );
return;
}

CPlayer *pBot = ToInPlayer(CBasePlayer::Instance( iBotEnt ));

if ( !pBot )
return;

if ( !pBot->GetAI() )
return;

DevWarning("Possesing %s!! \n", pBot->GetPlayerName());
pBot->GetAI()->Possess( pOwner );
}*/