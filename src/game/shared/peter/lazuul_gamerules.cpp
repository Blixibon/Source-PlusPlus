#include "cbase.h"
#include "lazuul_gamerules.h"
#ifndef CLIENT_DLL
#include "globalstate.h"
#include "ai_basenpc.h"
#include "team.h"
#include "peter/gametypes.h"
#include "peter/player_models.h"
#include "team_objectiveresource.h"
#include "peter/laz_mapents.h"
#include "items.h"
#include "trigger_area_capture.h"
#include "iscorer.h"
#include "peter/laz_player.h"
#include "gamestats.h"
#include "usermessages.h"
#include "voice_gamemgr.h"
#include "NextBotManager.h"
#include "hltvdirector.h"
#else
#include "c_team_objectiveresource.h"
#include "peter/c_laz_player.h"
#endif // !CLIENT_DLL
#include "ammodef.h"
#include "weapon_physcannon.h"
#include "hl2_player_shared.h"
#include "weapon_coop_base.h"

// Controls the application of the robus radius damage model.
extern ConVar sv_robust_explosions;

// Damage scale for damage inflicted by the player on each skill level.
extern ConVar sk_dmg_inflict_scale1;
extern ConVar sk_dmg_inflict_scale2;
extern ConVar sk_dmg_inflict_scale3;

// Damage scale for damage taken by the player on each skill level.
extern ConVar sk_dmg_take_scale1;
extern ConVar sk_dmg_take_scale2;
extern ConVar sk_dmg_take_scale3;


extern ConVar sk_allow_autoaim;

// Autoaim scale
extern ConVar sk_autoaim_scale1;
extern ConVar sk_autoaim_scale2;
//extern ConVar sk_autoaim_scale3;

// Quantity scale for ammo received by the player.
extern ConVar sk_ammo_qty_scale1;
extern ConVar sk_ammo_qty_scale2;
extern ConVar sk_ammo_qty_scale3;

extern ConVar sk_plr_health_drop_time;
extern ConVar sk_plr_grenade_drop_time;

extern ConVar sk_plr_dmg_ar2;
extern ConVar sk_npc_dmg_ar2;
extern ConVar sk_max_ar2;
extern ConVar sk_max_ar2_altfire;

extern ConVar sk_plr_dmg_alyxgun;
extern ConVar sk_npc_dmg_alyxgun;
extern ConVar sk_max_alyxgun;

extern ConVar sk_plr_dmg_pistol;
extern ConVar sk_npc_dmg_pistol;
extern ConVar sk_max_pistol;

extern ConVar sk_plr_dmg_smg1;
extern ConVar sk_npc_dmg_smg1;
extern ConVar sk_max_smg1;

// FIXME: remove these
//extern ConVar sk_plr_dmg_flare_round;
//extern ConVar sk_npc_dmg_flare_round;
//extern ConVar sk_max_flare_round;

extern ConVar sk_plr_dmg_buckshot;
extern ConVar sk_npc_dmg_buckshot;
extern ConVar sk_max_buckshot;
extern ConVar sk_plr_num_shotgun_pellets;

extern ConVar sk_plr_dmg_rpg_round;
extern ConVar sk_npc_dmg_rpg_round;
extern ConVar sk_max_rpg_round;

extern ConVar sk_plr_dmg_sniper_round;
extern ConVar sk_npc_dmg_sniper_round;
extern ConVar sk_max_sniper_round;

//extern ConVar sk_max_slam;
//extern ConVar sk_max_tripwire;

//extern ConVar sk_plr_dmg_molotov;
//extern ConVar sk_npc_dmg_molotov;
//extern ConVar sk_max_molotov;

extern ConVar sk_plr_dmg_grenade;
extern ConVar sk_npc_dmg_grenade;
extern ConVar sk_max_grenade;

#ifdef HL2_EPISODIC
extern ConVar sk_max_hopwire;
extern ConVar sk_max_striderbuster;
#endif

//extern ConVar sk_plr_dmg_brickbat;
//extern ConVar sk_npc_dmg_brickbat;
//extern ConVar sk_max_brickbat;

extern ConVar sk_plr_dmg_smg1_grenade;
extern ConVar sk_npc_dmg_smg1_grenade;
extern ConVar sk_max_smg1_grenade;

extern ConVar sk_plr_dmg_357;
extern ConVar sk_npc_dmg_357;
extern ConVar sk_max_357;

extern ConVar sk_plr_dmg_crossbow;
extern ConVar sk_npc_dmg_crossbow;
extern ConVar sk_max_crossbow;

extern ConVar sk_dmg_sniper_penetrate_plr;
extern ConVar sk_dmg_sniper_penetrate_npc;

extern ConVar sk_plr_dmg_airboat;
extern ConVar sk_npc_dmg_airboat;

extern ConVar sk_max_gauss_round;

// Gunship & Dropship cannons
extern ConVar sk_npc_dmg_gunship;
extern ConVar sk_npc_dmg_gunship_to_plr;


ConVar sk_npc_dmg_9mm_bullet("sk_npc_dmg_9mm_bullet", "0", FCVAR_REPLICATED);
ConVar sk_plr_dmg_9mm_bullet("sk_plr_dmg_9mm_bullet", "0", FCVAR_REPLICATED);
ConVar sk_max_9mm_bullet("sk_max_9mm_bullet", "0", FCVAR_REPLICATED);

ConVar sk_npc_dmg_9mmAR_bullet("sk_npc_dmg_9mmAR_bullet", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_357_bullet("sk_plr_dmg_357_bullet", "0", FCVAR_REPLICATED);
ConVar sk_max_357_bullet("sk_max_357_bullet", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_mp5_grenade("sk_plr_dmg_mp5_grenade", "0", FCVAR_REPLICATED);
ConVar sk_max_mp5_grenade("sk_max_mp5_grenade", "0", FCVAR_REPLICATED);
ConVar sk_mp5_grenade_radius("sk_mp5_grenade_radius", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_rpg("sk_plr_dmg_rpg", "0", FCVAR_REPLICATED);
ConVar sk_max_rpg_rocket("sk_max_rpg_rocket", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_xbow_bolt_plr("sk_plr_dmg_xbow_bolt_plr", "0", FCVAR_REPLICATED);
ConVar sk_plr_dmg_xbow_bolt_npc("sk_plr_dmg_xbow_bolt_npc", "0", FCVAR_REPLICATED);
ConVar sk_max_xbow_bolt("sk_max_xbow_bolt", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_egon_narrow("sk_plr_dmg_egon_narrow", "0", FCVAR_REPLICATED);
ConVar sk_plr_dmg_egon_wide("sk_plr_dmg_egon_wide", "0", FCVAR_REPLICATED);
ConVar sk_max_uranium("sk_max_uranium", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_gauss("sk_plr_dmg_gauss", "0", FCVAR_REPLICATED);

ConVar sk_plr_dmg_hornet("sk_plr_dmg_hornet", "0", FCVAR_REPLICATED);
ConVar sk_npc_dmg_hornet("sk_npc_dmg_hornet", "0", FCVAR_REPLICATED);
ConVar sk_max_hornet("sk_max_hornet", "0", FCVAR_REPLICATED);

ConVar sk_max_snark("sk_max_snark", "0", FCVAR_REPLICATED);

extern ConVar sk_plr_dmg_tripmine;
ConVar sk_max_tripmine("sk_max_tripmine", "0", FCVAR_REPLICATED);

extern ConVar sk_plr_dmg_satchel;
ConVar sk_max_satchel("sk_max_satchel", "0", FCVAR_REPLICATED);

ConVar sk_npc_dmg_12mm_bullet("sk_npc_dmg_12mm_bullet", "0", FCVAR_REPLICATED);

ConVar	sk_max_slam("sk_max_slam", "15", FCVAR_REPLICATED);

#ifndef CLIENT_DLL
static bool s_bInModeChangedScope = false;
void LazGamemodeChangedCallback(IConVar* pConVar, const char* pOldValue, float flOldValue)
{
	
	if (!s_bInModeChangedScope)
	{
		s_bInModeChangedScope = true;
		ConVarRef var(pConVar);
		if (g_pGameRules)
			LazuulRules()->SetGameMode(var.GetInt());
		s_bInModeChangedScope = false;
	}
}

ConVar gamemode("laz_gamemode", "0", FCVAR_NOTIFY, "Multiplayer gamemode.", true, 0, true, LAZ_GM_COUNT - 1, LazGamemodeChangedCallback);
#endif // !CLIENT_DLL


REGISTER_GAMERULES_CLASS(CLazuul);

BEGIN_NETWORK_TABLE_NOBASE(CLazuul, DT_Lazuul)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bMegaPhysgun)),
RecvPropInt(RECVINFO(m_nGameMode)),
#else
SendPropBool(SENDINFO(m_bMegaPhysgun)),
SendPropInt(SENDINFO(m_nGameMode)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(laz_gamerules, CLazuulProxy);
IMPLEMENT_NETWORKCLASS_ALIASED(LazuulProxy, DT_LazuulProxy)

#ifdef CLIENT_DLL
void RecvProxy_Lazuul(const RecvProp* pProp, void** pOut, void* pData, int objectID)
{
	CLazuul* pRules = dynamic_cast<CLazuul*>(GameRules());
	Assert(pRules);
	*pOut = pRules;
}

BEGIN_RECV_TABLE(CLazuulProxy, DT_LazuulProxy)
RecvPropDataTable("lazuul_game_data", 0, 0, &REFERENCE_RECV_TABLE(DT_Lazuul), RecvProxy_Lazuul)
END_RECV_TABLE()

#else
void* SendProxy_Lazuul(const SendProp* pProp, const void* pStructBase, const void* pData, CSendProxyRecipients* pRecipients, int objectID)
{
	CLazuul* pRules = dynamic_cast<CLazuul*>(GameRules());
	Assert(pRules);
	pRecipients->SetAllRecipients();
	return pRules;
}

BEGIN_SEND_TABLE(CLazuulProxy, DT_LazuulProxy)
SendPropDataTable("lazuul_game_data", 0, &REFERENCE_SEND_TABLE(DT_Lazuul), SendProxy_Lazuul)
END_SEND_TABLE()

BEGIN_DATADESC(CLazuulProxy)
DEFINE_KEYFIELD(m_bModeAllowed[0], FIELD_BOOLEAN, "allowdeathmatch"),
DEFINE_KEYFIELD(m_bModeAllowed[1], FIELD_BOOLEAN, "allowcoop"),
DEFINE_KEYFIELD(m_bModeAllowed[2], FIELD_BOOLEAN, "allowversus"),
END_DATADESC()

void CLazuulProxy::Activate()
{
	BaseClass::Activate();

	LazuulRules()->SetAllowedModes(m_bModeAllowed);
}

class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity)
	{
		// Dead players can only be heard by other dead team mates but only if a match is in progress
		if (LazuulRules()->State_Get() != GR_STATE_TEAM_WIN && LazuulRules()->State_Get() != GR_STATE_GAME_OVER)
		{
			if (pTalker->IsAlive() == false)
			{
				if (pListener->IsAlive() == false)
					return (pListener->InSameTeam(pTalker));

				return false;
			}

			bProximity = true;
		}

		return true;
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;
#endif

// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
//char* sTeamNames[] =
//{
//	"Unassigned",
//	"Spectator",
//	"Combine",
//	"Rebels",
//};

//-----------------------------------------------------------------------------
	// Purpose:
	// Input  :
	// Output :
	//-----------------------------------------------------------------------------
CLazuul::CLazuul()
{
	m_bMegaPhysgun = false;
#ifndef CLIENT_DLL
	// Create the team managers
	for (int i = 0; i < TF_TEAM_COUNT; i++)
	{
		CTeam* pTeam = static_cast<CTeam*>(CreateEntityByName("team_manager"));
		pTeam->Init(g_aTeamNames[i], i);

		g_Teams.AddToTail(pTeam);
	}

	m_flLastHealthDropTime = 0.0f;
	m_flLastGrenadeDropTime = 0.0f;
#endif
}

CLazuul::~CLazuul()
{
#ifndef CLIENT_DLL
	g_Teams.Purge();
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
int CLazuul::Damage_GetTimeBased(void)
{
	if (hl2_episodic.GetBool())
	{
		int iDamage = (DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN);
		return iDamage;
	}
	else
		return BaseClass::Damage_GetTimeBased();

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output :		bool
//-----------------------------------------------------------------------------
bool CLazuul::Damage_IsTimeBased(int iDmgType)
{
	// Damage types that are time-based.
	if (hl2_episodic.GetBool())
	{
		// This makes me think EP2 should have its own rules, but they are #ifdef all over in here.
		return ((iDmgType & (DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_SLOWBURN)) != 0);
	}
	else
		return BaseClass::Damage_IsTimeBased(iDmgType);

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CLazuul::GetFarthestOwnedControlPoint(int iTeam, bool bWithSpawnpoints)
{
	int iOwnedEnd = ObjectiveResource()->GetBaseControlPointForTeam(iTeam);
	if (iOwnedEnd == -1)
		return -1;

	int iNumControlPoints = ObjectiveResource()->GetNumControlPoints();
	int iWalk = 1;
	int iEnemyEnd = iNumControlPoints - 1;
	if (iOwnedEnd != 0)
	{
		iWalk = -1;
		iEnemyEnd = 0;
	}

	// Walk towards the other side, and find the farthest owned point that has spawn points
	int iFarthestPoint = iOwnedEnd;
	for (int iPoint = iOwnedEnd; iPoint != iEnemyEnd; iPoint += iWalk)
	{
		// If we've hit a point we don't own, we're done
		if (ObjectiveResource()->GetOwningTeam(iPoint) != iTeam)
			break;

		/*if (bWithSpawnpoints && !m_bControlSpawnsPerTeam[iTeam][iPoint])
			continue;*/

		iFarthestPoint = iPoint;
	}

	return iFarthestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLazuul::TeamMayCapturePoint(int iTeam, int iPointIndex)
{
	// Is point capturing allowed at all?
	if (!PointsMayBeCaptured())
		return false;

	// If the point is explicitly locked it can't be capped.
	if (ObjectiveResource()->GetCPLocked(iPointIndex))
		return false;

	/*if (!tf_caplinear.GetBool())
		return true;*/

	// Any previous points necessary?
	int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint(iPointIndex, iTeam, 0);

	// Points set to require themselves are always cappable 
	if (iPointNeeded == iPointIndex)
		return true;

	// No required points specified? Require all previous points.
	if (iPointNeeded == -1)
	{
		if (!ObjectiveResource()->PlayingMiniRounds())
		{
			// No custom previous point, team must own all previous points
			int iFarthestPoint = GetFarthestOwnedControlPoint(iTeam, false);
			return (abs(iFarthestPoint - iPointIndex) <= 1);
		}
		else
		{
			// No custom previous point, team must own all previous points in the current mini-round
			//tagES TFTODO: need to figure out a good algorithm for this
			return true;
		}
	}

	// Loop through each previous point and see if the team owns it
	for (int iPrevPoint = 0; iPrevPoint < MAX_PREVIOUS_POINTS; iPrevPoint++)
	{
		int iPointNeeded = ObjectiveResource()->GetPreviousPointForPoint(iPointIndex, iTeam, iPrevPoint);
		if (iPointNeeded != -1)
		{
			if (ObjectiveResource()->GetOwningTeam(iPointNeeded) != iTeam)
				return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLazuul::PlayerMayCapturePoint(CBasePlayer *pPlayer, int iPointIndex, char *pszReason /* = NULL */, int iMaxReasonLength /* = 0 */)
{
	CLaz_Player *pTFPlayer = ToLazuulPlayer(pPlayer);

	if (!pTFPlayer)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLazuul::PlayerMayBlockPoint(CBasePlayer *pPlayer, int iPointIndex, char *pszReason, int iMaxReasonLength)
{
	CLaz_Player *pTFPlayer = ToLazuulPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	

	return false;
}

// ------------------------------------------------------------------------------------ //
// Shared CHalfLife2 implementation.
// ------------------------------------------------------------------------------------ //
bool CLazuul::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	// The smaller number is always first
	if (collisionGroup0 > collisionGroup1)
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0, collisionGroup1);
	}

	// Prevent the player movement from colliding with spit globs (caused the player to jump on top of globs while in water)
	if (collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == HL2COLLISION_GROUP_SPIT)
		return false;

	if ((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON)
	{
		return false;
	}

	if (!IsMultiplayer())
	{
		// HL2 treats movement and tracing against players the same, so just remap here
		if (collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT)
		{
			collisionGroup0 = COLLISION_GROUP_PLAYER;
		}

		if (collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT)
		{
			collisionGroup1 = COLLISION_GROUP_PLAYER;
		}
	}

	//If collisionGroup0 is not a player then NPC_ACTOR behaves just like an NPC.
	if (collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER)
	{
		collisionGroup1 = COLLISION_GROUP_NPC;
	}

	// This is only for the super physcannon
	if (m_bMegaPhysgun)
	{
		if (collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_PLAYER)
			return false;
	}

	if (collisionGroup0 == HL2COLLISION_GROUP_COMBINE_BALL)
	{
		if (collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL)
			return false;
	}

	if (collisionGroup0 == HL2COLLISION_GROUP_COMBINE_BALL && collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL_NPC)
		return false;

	if ((collisionGroup0 == COLLISION_GROUP_WEAPON) ||
		//(collisionGroup0 == COLLISION_GROUP_PLAYER) ||
		(collisionGroup0 == COLLISION_GROUP_PROJECTILE))
	{
		if (collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL)
			return false;
	}

	if (collisionGroup0 == COLLISION_GROUP_DEBRIS)
	{
		if (collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL)
			return true;
	}

	if (collisionGroup0 == HL2COLLISION_GROUP_HOUNDEYE && collisionGroup1 == HL2COLLISION_GROUP_HOUNDEYE)
		return false;

	if (collisionGroup0 == HL2COLLISION_GROUP_HOMING_MISSILE && collisionGroup1 == HL2COLLISION_GROUP_HOMING_MISSILE)
		return false;

	if (collisionGroup1 == HL2COLLISION_GROUP_CROW)
	{
		if (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_NPC ||
			collisionGroup0 == HL2COLLISION_GROUP_CROW)
			return false;
	}

	if ((collisionGroup0 == HL2COLLISION_GROUP_HEADCRAB) && (collisionGroup1 == HL2COLLISION_GROUP_HEADCRAB))
		return false;

	// striders don't collide with other striders
	if (collisionGroup0 == HL2COLLISION_GROUP_STRIDER && collisionGroup1 == HL2COLLISION_GROUP_STRIDER)
		return false;

	// gunships don't collide with other gunships
	if (collisionGroup0 == HL2COLLISION_GROUP_GUNSHIP && collisionGroup1 == HL2COLLISION_GROUP_GUNSHIP)
		return false;

	// weapons and NPCs don't collide
	if (collisionGroup0 == COLLISION_GROUP_WEAPON && (collisionGroup1 >= HL2COLLISION_GROUP_FIRST_NPC && collisionGroup1 <= HL2COLLISION_GROUP_LAST_NPC))
		return false;

	//players don't collide against NPC Actors.
	//I could've done this up where I check if collisionGroup0 is NOT a player but I decided to just
	//do what the other checks are doing in this function for consistency sake.
	if (collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER)
		return false;

	// In cases where NPCs are playing a script which causes them to interpenetrate while riding on another entity,
	// such as a train or elevator, you need to disable collisions between the actors so the mover can move them.
	if (collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED)
		return false;

	// Spit doesn't touch other spit
	if (collisionGroup0 == HL2COLLISION_GROUP_SPIT && collisionGroup1 == HL2COLLISION_GROUP_SPIT)
		return false;

	return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CLazuul::ShouldUseRobustRadiusDamage(CBaseEntity* pEntity)
{
#ifdef CLIENT_DLL
	return false;
#endif

	if (!sv_robust_explosions.GetBool())
		return false;

	if (!pEntity->IsNPC())
	{
		// Only NPC's
		return false;
	}

#ifndef CLIENT_DLL
	CAI_BaseNPC* pNPC = pEntity->MyNPCPointer();
	if (pNPC->CapabilitiesGet() & bits_CAP_SIMPLE_RADIUS_DAMAGE)
	{
		// This NPC only eligible for simple radius damage.
		return false;
	}
#endif//CLIENT_DLL

	return true;
}



#ifndef CLIENT_DLL

// Classnames of entities that are preserved across round restarts
static const char* s_LazPreserveEnts[] =
{
	"laz_gamerules",
	"predicted_viewmodel",
	"objective_resource",
	"team_manager",
	"player_manager",
	"info_player_teamspawn",
	"", // END Marker
};

void CLazuul::SetGameMode(int iMode)
{
	if (iMode < 0 || iMode > LAZ_GM_COUNT)
		return;

	int iGameMode = iMode;

	if (m_bitAllowedModes.IsAllClear())
	{
		iGameMode = LAZ_GM_SINGLEPLAYER;
		Warning("ERROR: Map has no valid multiplayer modes!\n");
	}
	else if (!m_bitAllowedModes.IsBitSet(iGameMode))
	{
		if (m_bitAllowedModes.IsBitSet(LAZ_GM_COOP))
			iGameMode = LAZ_GM_COOP;
		else if (m_bitAllowedModes.IsBitSet(LAZ_GM_DEATHMATCH))
			iGameMode = LAZ_GM_DEATHMATCH;
		else
			iGameMode = LAZ_GM_VERSUS;
	}

	m_nGameMode.Set(iGameMode);
	if (iGameMode >= 0)
		gamemode.SetValue(iGameMode);
}

void CLazuul::SetAllowedModes(bool bModes[])
{
	for (int i = 0; i < LAZ_GM_COUNT; i++)
	{
		m_bitAllowedModes.Set(i, bModes[i]);
	}

	s_bInModeChangedScope = true;
	SetGameMode(gamemode.GetInt());
	s_bInModeChangedScope = false;
}

//-----------------------------------------------------------------------------
// Returns whether or not Alyx cares about light levels in order to see.
//-----------------------------------------------------------------------------
bool CLazuul::IsAlyxInDarknessMode()
{
#ifdef HL2_EPISODIC
	static ConVarRef alyx_darkness_force("alyx_darkness_force");
	if (alyx_darkness_force.GetBool())
		return true;

	return (GlobalEntity_GetState("ep_alyx_darknessmode") == GLOBAL_ON);
#else
	return false;
#endif // HL2_EPISODIC
}

//-----------------------------------------------------------------------------
// This takes the long way around to see if a prop should emit a DLIGHT when it
// ignites, to avoid having Alyx-related code in props.cpp.
//-----------------------------------------------------------------------------
bool CLazuul::ShouldBurningPropsEmitLight()
{
	if (IsAlyxInDarknessMode())
		return true;

	return false;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CLazuul::WeaponShouldRespawn(CBaseCombatWeapon* pWeapon)
{
	if (pWeapon->HasSpawnFlags(SF_NORESPAWN) || !IsMultiplayer())
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
	//=========================================================
int CLazuul::ItemShouldRespawn(CItem* pItem)
{
	if (pItem->HasSpawnFlags(SF_NORESPAWN) || !IsMultiplayer())
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}

void CLazuul::Status(void(*print)(const char *fmt, ...))
{
	BaseClass::Status(print);

	if (FAllowNPCs())
		print("NPC AIs : %i of %i\n", g_AI_Manager.NumAIs(), CAI_Manager::MAX_AIS);

	print("NextBot AIs: %i\n", TheNextBots().GetNextBotCount());
}

void CLazuul::GetTaggedConVarList(KeyValues * pCvarTagList)
{
	BaseClass::GetTaggedConVarList(pCvarTagList);
}

//-----------------------------------------------------------------------------
	// Purpose: Returns how much damage the given ammo type should do to the victim
	//			when fired by the attacker.
	// Input  : pAttacker - Dude what shot the gun.
	//			pVictim - Dude what done got shot.
	//			nAmmoType - What been shot out.
	// Output : How much hurt to put on dude what done got shot (pVictim).
	//-----------------------------------------------------------------------------
float CLazuul::GetAmmoDamage(CBaseEntity* pAttacker, CBaseEntity* pVictim, int nAmmoType)
{
	float flDamage = 0.0f;
	CAmmoDef* pAmmoDef = GetAmmoDef();

	if (pAmmoDef->DamageType(nAmmoType) & DMG_SNIPER)
	{
		// If this damage is from a SNIPER, we do damage based on what the bullet
		// HITS, not who fired it. All other bullets have their damage values
		// arranged according to the owner of the bullet, not the recipient.
		if (pVictim->IsPlayer())
		{
			// Player
			flDamage = pAmmoDef->PlrDamage(nAmmoType);
		}
		else
		{
			// NPC or breakable
			flDamage = pAmmoDef->NPCDamage(nAmmoType);
		}
	}
	else
	{
		flDamage = BaseClass::GetAmmoDamage(pAttacker, pVictim, nAmmoType);
	}

	if (pAttacker && pAttacker->IsPlayer() && pVictim->IsNPC())
	{
		if (pVictim->MyCombatCharacterPointer())
		{
			// Player is shooting an NPC. Adjust the damage! This protects breakables
			// and other 'non-living' entities from being easier/harder to break
			// in different skill levels.
			flDamage = pAmmoDef->PlrDamage(nAmmoType);
			flDamage = AdjustPlayerDamageInflicted(flDamage);
		}
	}

	return flDamage;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CLazuul::AdjustPlayerDamageTaken(CTakeDamageInfo* pInfo)
{
	if (pInfo->GetDamageType() & (DMG_DROWN | DMG_CRUSH | DMG_FALL | DMG_POISON | DMG_SNIPER))
	{
		// Skill level doesn't affect these types of damage.
		return;
	}

	switch (GetSkillLevel())
	{
	case SKILL_EASY:
		pInfo->ScaleDamage(sk_dmg_take_scale1.GetFloat());
		break;

	case SKILL_MEDIUM:
		pInfo->ScaleDamage(sk_dmg_take_scale2.GetFloat());
		break;

	case SKILL_HARD:
		pInfo->ScaleDamage(sk_dmg_take_scale3.GetFloat());
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
float CLazuul::AdjustPlayerDamageInflicted(float damage)
{
	switch (GetSkillLevel())
	{
	case SKILL_EASY:
		return damage * sk_dmg_inflict_scale1.GetFloat();
		break;

	case SKILL_MEDIUM:
		return damage * sk_dmg_inflict_scale2.GetFloat();
		break;

	case SKILL_HARD:
		return damage * sk_dmg_inflict_scale3.GetFloat();
		break;

	default:
		return damage;
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CLazuul::AllowDamage(CBaseEntity* pVictim, const CTakeDamageInfo& info)
{
#ifndef CLIENT_DLL
	if ((info.GetDamageType() & DMG_CRUSH) && info.GetInflictor() && pVictim->MyNPCPointer())
	{
		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);

			if (pPlayer)
			{
				if (pVictim->MyNPCPointer()->IsPlayerAlly(pPlayer))
				{
					// A physics object has struck a player ally. Don't allow damage if it
					// came from the player's physcannon. 
					CBaseEntity* pWeapon = pPlayer->HasNamedPlayerItem("weapon_physcannon");

					if (pWeapon)
					{
						CBaseCombatWeapon* pCannon = assert_cast <CBaseCombatWeapon*>(pWeapon);

						if (pCannon)
						{
							if (PhysCannonAccountableForObject(pCannon, info.GetInflictor()))
							{
								// Antlions can always be squashed!
								if (pVictim->Classify() == CLASS_ANTLION)
									return true;

								return false;
							}
						}
					}
				}
			}
		}
	}
#endif
	return true;
}

//=========================================================
//=========================================================
int CLazuul::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if (!pPlayer || !pTarget)
		return GR_NOTTEAMMATE;

	if (pTarget->IsPlayer())
	{
		if ((*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp(GetTeamID(pPlayer), GetTeamID(pTarget)))
		{
			return GR_TEAMMATE;
		}
	}
	else if (pTarget->IsNPC())
	{
		CAI_BaseNPC *pNPC = pTarget->MyNPCPointer();
		switch (pNPC->IRelationType(pPlayer))
		{
		case D_LI:
			return GR_TEAMMATE;
			break;
		case D_HT:
		case D_FR:
			if (pNPC->CanBeAnEnemyOf(pPlayer))
			{
				return GR_NOTTEAMMATE;
				break;
			}
		default:
			return GR_NEUTRAL;
			break;
		}
	}

	return GR_NOTTEAMMATE;
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not the NPC should drop a health vial
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CLazuul::NPC_ShouldDropHealth(CBasePlayer * pRecipient)
{
	// Can only do this every so often
	if (m_flLastHealthDropTime > gpGlobals->curtime)
		return false;

	//Try to throw dynamic health
	float healthPerc = ((float)pRecipient->m_iHealth / (float)pRecipient->m_iMaxHealth);

	if (random->RandomFloat(0.0f, 1.0f) > healthPerc * 1.5f)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not the NPC should drop a health vial
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CLazuul::NPC_ShouldDropGrenade(CBasePlayer * pRecipient)
{
	// Can only do this every so often
	if (m_flLastGrenadeDropTime > gpGlobals->curtime)
		return false;

	int grenadeIndex = GetAmmoDef()->Index("grenade");
	int numGrenades = pRecipient->GetAmmoCount(grenadeIndex);

	// If we're not maxed out on grenades and we've randomly okay'd it
	if ((numGrenades < GetAmmoDef()->MaxCarry(grenadeIndex)) && (random->RandomInt(0, 2) == 0))
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Update the drop counter for health
//-----------------------------------------------------------------------------
void CLazuul::NPC_DroppedHealth(void)
{
	m_flLastHealthDropTime = gpGlobals->curtime + sk_plr_health_drop_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Update the drop counter for grenades
//-----------------------------------------------------------------------------
void CLazuul::NPC_DroppedGrenade(void)
{
	m_flLastGrenadeDropTime = gpGlobals->curtime + sk_plr_grenade_drop_time.GetFloat();
}

//=========================================================
//=========================================================
void CLazuul::PlayerSpawn(CBasePlayer* pPlayer)
{
	if (!IsMultiplayer())
		return;

	if (!pPlayer || pPlayer->IsObserver())
		return;

	bool		addDefault;
	CBaseEntity* pWeaponEntity = NULL;

	pPlayer->EquipSuit(false);

	addDefault = true;

	while ((pWeaponEntity = gEntList.FindEntityByClassname(pWeaponEntity, "info_player_equip")) != NULL)
	{
		CLazPlayerEquip* pEquip = assert_cast<CLazPlayerEquip*> (pWeaponEntity);
		
		if (!pEquip->IsDisabled() && pEquip->CanEquipTeam(pPlayer->GetTeamNumber()))
		{
			pEquip->EquipPlayer(pPlayer);
			addDefault = false;
		}
	}

	if (addDefault)
	{
		pPlayer->GiveAmmo(255, "Pistol");
		pPlayer->GiveAmmo(45, "SMG1");
		pPlayer->GiveAmmo(1, "grenade");
		pPlayer->GiveAmmo(6, "Buckshot");
		pPlayer->GiveAmmo(6, "357");

		if (pPlayer->GetTeamNumber() == TEAM_COMBINE)
		{
			pPlayer->GiveNamedItem("weapon_stunstick");
		}
		else
		{
			pPlayer->GiveNamedItem("weapon_crowbar");
		}

		pPlayer->GiveNamedItem("weapon_pistol");
		pPlayer->GiveNamedItem("weapon_smg1");
		pPlayer->GiveNamedItem("weapon_frag");
		if (!pPlayer->IsBot())
			pPlayer->GiveNamedItem("weapon_physcannon");

		const char* szDefaultWeaponName = engine->GetClientConVarValue(engine->IndexOfEdict(pPlayer->edict()), "cl_defaultweapon");

		CBaseCombatWeapon* pDefaultWeapon = pPlayer->Weapon_OwnsThisType(szDefaultWeaponName);

		if (pDefaultWeapon)
		{
			pPlayer->Weapon_Switch(pDefaultWeapon);
		}
		else
		{
			pPlayer->Weapon_Switch(pPlayer->Weapon_OwnsThisType("weapon_smg1"));
		}
	}
}

bool CLazuul::ClientCommand(CBaseEntity * pEdict, const CCommand & args)
{
	if (BaseClass::ClientCommand(pEdict, args))
		return true;

	CBasePlayer *pPlayer = (CBasePlayer *)pEdict;

	if (pPlayer->ClientCommand(args))
		return true;

	return false;
}

const char * CLazuul::GetChatFormat(bool bTeamOnly, CBasePlayer * pPlayer)
{
	if (!pPlayer)  // dedicated server output
	{
		return NULL;
	}

	CLaz_Player *pLazPlayer = ToLazuulPlayer(pPlayer);

	const char *pszFormat = NULL;

	// team only
	if (bTeamOnly == true)
	{
		if (pLazPlayer->GetTeamNumber() == TEAM_SPECTATOR)
		{
			pszFormat = "LAZ_Chat_Spec";
		}
		else
		{
			if (pLazPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN)
			{
				pszFormat = "LAZ_Chat_Team_Dead";
			}
			else
			{
				const char *chatLocation = GetChatLocation(bTeamOnly, pPlayer);
				if (chatLocation && *chatLocation)
				{
					pszFormat = "LAZ_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "LAZ_Chat_Team";
				}
			}
		}
	}
	/*else if ( pTFPlayer->m_bIsPlayerADev )
	{
		if ( pTFPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "LAZ_Chat_DevSpec";
		}
		else
		{
			if (pTFPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN)
			{
				pszFormat = "LAZ_Chat_DevDead";
			}
			else
			{
				pszFormat = "LAZ_Chat_Dev";
			}
		}
	}*/
	else
	{
		if (pLazPlayer->GetTeamNumber() == TEAM_SPECTATOR)
		{
			pszFormat = "LAZ_Chat_AllSpec";
		}
		else
		{
			if (pLazPlayer->IsAlive() == false && State_Get() != GR_STATE_TEAM_WIN)
			{
				pszFormat = "LAZ_Chat_AllDead";
			}
			else
			{
				pszFormat = "LAZ_Chat_All";
			}
		}
	}

	return pszFormat;
}

void CLazuul::MarkAchievement(IRecipientFilter & filter, char const * pchAchievementName)
{
	gamestats->Event_IncrementCountedStatistic(vec3_origin, pchAchievementName, 1.0f);

	UserMessageBegin(filter, "AchievementMapEvent");
	WRITE_STRING(pchAchievementName);
	MessageEnd();
}

bool CLazuul::FAllowNPCs(void)
{
	if (!IsMultiplayer())
		return true;

	if (IsInWaitingForPlayers() || m_iRoundState == GR_STATE_PREGAME)
		return false;

	return BaseClass::FAllowNPCs();
}

//------------------------------------------------------------------------------
	// Purpose : Initialize all default class relationships
	// Input   :
	// Output  :
	//------------------------------------------------------------------------------
void CLazuul::InitDefaultAIRelationships(void)
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i = 0; i < NUM_AI_CLASSES; i++)
	{
		for (j = 0; j < NUM_AI_CLASSES; j++)
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship((Class_T)i, (Class_T)j, D_NU, 0);
		}
	}

	// ------------------------------------------------------------
		//	> CLASS_ANTLION
		// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ANTLION, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIEN_MILITARY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIEN_MONSTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ALIENCONTROLLER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_AIR_DEFENSE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MANTARAY_TELEPORTER, D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_BARNACLE
	//
	//  In this case, the relationship D_HT indicates which characters
	//  the barnacle will try to eat.
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_BARNACLE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_EARTH_FAUNA, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ALIENCONTROLLER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_VORTIGAUNT, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ALIENCONTROLLER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSQUID
	// ------------------------------------------------------------

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_HEADCRAB, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_HOUNDEYE, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_AIR_DEFENSE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_PASSIVE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_BULLSQUID, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE_HUNTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_HEADCRAB, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_HOUNDEYE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MISSILE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_VORTIGAUNT, D_LI, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ZOMBIE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ALIENCONTROLLER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_REBEL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_BULLSQUID, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MISSILE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_VORTIGAUNT, D_LI, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_AIR_DEFENSE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_AIR_DEFENSE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_VITAL, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_AIR_DEFENSE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_PLAYER, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_GUNSHIP
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MISSILE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_AIR_DEFENSE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_AIR_DEFENSE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_HUNTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_AIR_DEFENSE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_CONSCRIPT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_VORTIGAUNT, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_FLARE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_VORTIGAUNT, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ALIENCONTROLLER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HEADCRAB
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_BULLSQUID, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_HACKED_ROLLERMINE, D_FR, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HOUNDEYE
	// ------------------------------------------------------------

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_BULLSQUID, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_AIR_DEFENSE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);


	// ------------------------------------------------------------
	//	> CLASS_MANHACK
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_HEADCRAB, D_HT, -1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_HOUNDEYE, D_HT, -1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_METROPOLICE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_METROPOLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_AIR_DEFENSE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MISSILE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_AIR_DEFENSE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_VORTIGAUNT, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIENGRUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIENCONTROLLER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BARNACLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BULLSEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CITIZEN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BEE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_AIR_DEFENSE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_BARNACLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_VORTIGAUNT, D_LI, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PROTOSNIPER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_AIR_DEFENSE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_BARNACLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_HUNTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_VORTIGAUNT, D_LI, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PROTOSNIPER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_AIR_DEFENSE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_SCANNER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MANHACK, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_METROPOLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MILITARY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_SCANNER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_STALKER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PROTOSNIPER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HACKED_ROLLERMINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIEN_PREY, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_STALKER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_VORTIGAUNT
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_CITIZEN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_VORTIGAUNT, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MILITARY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MANTARAY_TELEPORTER, D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_PROTOSNIPER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MISSILE, D_NU, 5);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_AIR_DEFENSE_HACKED, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	//
	// Hates pretty much everything equally except other earth fauna.
	// This will make the critter choose the nearest thing as its enemy.
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_NONE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_FLARE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MISSILE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_VORTIGAUNT, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_AIR_DEFENSE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_AIR_DEFENSE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_HACKED_ROLLERMINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_VORTIGAUNT, D_LI, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ZOMBIE, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_EARTH_FAUNA, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MANTARAY_TELEPORTER, D_NU, 0);

	// Human Error Classes
	// ------------------------------------------------------------
	//	> CLASS_ALIENGRUNT
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_CITIZEN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_STALKER, D_HT, 0);
	//	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_HACKED_ROLLERMINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENGRUNT, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_ALIENCONTROLLER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_MILITARY_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_VORTIGAUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_HACKED_ROLLERMINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_COMBINE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIENCONTROLLER, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	//	> CLASS_COMBINE_HACKED
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_MILITARY_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HACKED, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY_HACKED
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_CONSCRIPT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_MILITARY_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_AIR_DEFENSE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY_HACKED, CLASS_MANTARAY_TELEPORTER, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_AIR_DEFENSE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_MILITARY_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_ALIENGRUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_ALIENCONTROLLER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_COMBINE_HACKED, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_AIR_DEFENSE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_AIR_DEFENSE_HACKED, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE, CLASS_MANTARAY_TELEPORTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_AIR_DEFENSE_HACKED
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_BULLSQUID, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_COMBINE_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_COMBINE_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_CONSCRIPT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_HEADCRAB, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_HOUNDEYE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_MILITARY_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_AIR_DEFENSE_HACKED, CLASS_MANTARAY_TELEPORTER, D_LI, 0);


	// ------------------------------------------------------------
	//	> CLASS_MANTARAY_TELEPORTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_BULLSEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_BULLSQUID, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_CONSCRIPT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_HEADCRAB, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_HOUNDEYE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_MILITARY_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_ALIENGRUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_ALIENCONTROLLER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_COMBINE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_AIR_DEFENSE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_AIR_DEFENSE_HACKED, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANTARAY_TELEPORTER, CLASS_MANTARAY_TELEPORTER, D_LI, 0);


	//HALF-LIFE 1 CLASSES
	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HUMAN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIEN_PREY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIEN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIEN_MONSTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIEN_PREDATOR, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HUMAN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_INSECT, D_NU, 0);


	// ------------------------------------------------------------
	//	> CLASS_HUMAN_PASSIVE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_HUMAN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_ALIEN_PREY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_ALIEN_MILITARY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_ALIEN_PREDATOR, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_PASSIVE, CLASS_COMBINE, D_HT, 0);



	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HUMAN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ALIEN_BIOWEAPON, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_BIOWEAPON, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_INSECT, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HUMAN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_INSECT, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_VORTIGAUNT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HUMAN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_INSECT, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HUMAN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_INSECT, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_ALIEN_PREY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_ALIEN_PREY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_ALIEN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_ALIEN_MONSTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_ALIEN_PREDATOR, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREY, CLASS_SCANNER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_ALIEN_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_ALIEN_PREY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_ALIEN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_ALIEN_MONSTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_ALIEN_PREDATOR, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MILITARY, CLASS_COMBINE_GUNSHIP, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_ALIEN_MONSTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_ALIEN_PREY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_ALIEN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_ALIEN_MONSTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_ALIEN_PREDATOR, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_MACHINE_HL1, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_MONSTER, CLASS_COMBINE_GUNSHIP, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_ALIEN_PREDATOR
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_ALIEN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_ALIEN_MONSTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_ALIEN_PREDATOR, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_PREDATOR, CLASS_COMBINE_HUNTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_HUMAN_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_HUMAN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HUMAN_MILITARY, CLASS_COMBINE_HUNTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MACHINE_HL1
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_HUMAN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_ALIEN_BIOWEAPON, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_PLAYER_BIOWEAPON, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_INSECT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MACHINE_HL1, CLASS_COMBINE_HUNTER, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_ALIEN_BIOWEAPON
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_ALIEN_MILITARY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_ALIEN_PREDATOR, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_PLAYER_BIOWEAPON, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ALIEN_BIOWEAPON, CLASS_INSECT, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_BIOWEAPON
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_MACHINE_HL1, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_HUMAN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_HUMAN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_ALIEN_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_ALIEN_MONSTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_ALIEN_PREY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_ALIEN_PREDATOR, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_ALIEN_BIOWEAPON, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_BIOWEAPON, CLASS_INSECT, D_NU, 0);


	// ------------------------------------------------------------
	//	> CLASS_INSECT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_NONE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_MACHINE_HL1, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_PLAYER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_HUMAN_PASSIVE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_HUMAN_MILITARY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_ALIEN_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_ALIEN_MONSTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_ALIEN_PREY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_ALIEN_PREDATOR, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_PLAYER_ALLY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_PLAYER_ALLY_VITAL, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_ALIEN_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_PLAYER_BIOWEAPON, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_INSECT, CLASS_INSECT, D_NU, 0);
}


//------------------------------------------------------------------------------
// Purpose : Return classify text for classify type
// Input   :
// Output  :
//------------------------------------------------------------------------------
const char* CLazuul::AIClassText(int classType)
{
	switch (classType)
	{
	case CLASS_NONE:				return "CLASS_NONE";
	case CLASS_PLAYER:				return "CLASS_PLAYER";
	case CLASS_PLAYER_ALLY:			return "CLASS_PLAYER_ALLY";
	case CLASS_PLAYER_ALLY_VITAL:	return "CLASS_PLAYER_ALLY_VITAL";
	case CLASS_ANTLION:				return "CLASS_ANTLION";
	case CLASS_BARNACLE:			return "CLASS_BARNACLE";
	case CLASS_BULLSEYE:			return "CLASS_BULLSEYE";
	case CLASS_BULLSQUID:			return "CLASS_BULLSQUID";	
	case CLASS_CITIZEN_PASSIVE:		return "CLASS_CITIZEN_PASSIVE";
	case CLASS_CITIZEN_REBEL:		return "CLASS_CITIZEN_REBEL";
	case CLASS_COMBINE:				return "CLASS_COMBINE";
	case CLASS_COMBINE_GUNSHIP:		return "CLASS_COMBINE_GUNSHIP";
	case CLASS_COMBINE_HUNTER:		return "CLASS_COMBINE_HUNTER";
	case CLASS_CONSCRIPT:			return "CLASS_CONSCRIPT";
	case CLASS_HEADCRAB:			return "CLASS_HEADCRAB";
	case CLASS_HOUNDEYE:			return "CLASS_HOUNDEYE";
	case CLASS_MANHACK:				return "CLASS_MANHACK";
	case CLASS_METROPOLICE:			return "CLASS_METROPOLICE";
	case CLASS_MILITARY:			return "CLASS_MILITARY";
	case CLASS_SCANNER:				return "CLASS_SCANNER";
	case CLASS_STALKER:				return "CLASS_STALKER";
	case CLASS_VORTIGAUNT:			return "CLASS_VORTIGAUNT";
	case CLASS_ZOMBIE:				return "CLASS_ZOMBIE";
	case CLASS_PROTOSNIPER:			return "CLASS_PROTOSNIPER";
	case CLASS_MISSILE:				return "CLASS_MISSILE";
	case CLASS_FLARE:				return "CLASS_FLARE";
	case CLASS_EARTH_FAUNA:			return "CLASS_EARTH_FAUNA";
	
	// HL1
	case CLASS_ALIEN_PREY:		return "CLASS_ALIEN_PREY";
	case CLASS_ALIEN_MILITARY:	return "CLASS_ALIEN_MILITARY";
	case CLASS_ALIEN_MONSTER:	return "CLASS_ALIEN_MONSTER";
	case CLASS_ALIEN_PREDATOR:	return "CLASS_ALIEN_PREDATOR";
	case CLASS_HUMAN_MILITARY:	return "CLASS_HUMAN_MILITARY";
	case CLASS_MACHINE_HL1:			return "CLASS_MACHINE_HL1";
	case CLASS_ALIEN_BIOWEAPON:	return "CLASS_ALIEN_BIOWEAPON";
	case CLASS_PLAYER_BIOWEAPON: return "CLASS_PLAYER_BIOWEAPON";

	// Human Error
	case CLASS_ALIENGRUNT:		return "CLASS_ALIENGRUNT";
	case CLASS_ALIENCONTROLLER: return "CLASS_ALIENCONTROLLER";
	case CLASS_BEE:				return "CLASS_BEE";
	case CLASS_COMBINE_HACKED:	return "CLASS_COMBINE_HACKED";
	case CLASS_MILITARY_HACKED:	return "CLASS_MILITARY_HACKED";
	case CLASS_AIR_DEFENSE:			return "CLASS_AIR_DEFENSE";
	case CLASS_AIR_DEFENSE_HACKED:	return "CLASS_AIR_DEFENSE_HACKED";
	case CLASS_MANTARAY_TELEPORTER:	return "CLASS_MANTARAY_TELEPORTER";

	// HL2MS
	case CLASS_COMBINE_PLAYER:	return "CLASS_COMBINE_PLAYER";
	case CLASS_COMBINE_VITAL:	return "CLASS_COMBINE_VITAL";

	default:					return "MISSING CLASS in ClassifyText()";
	}
}

const char* CLazuul::GetGameDescription(void)
{
	switch (GetGameMode())
	{
	case LAZ_GM_SINGLEPLAYER:
	default:
		return "HL2 Lazuul";
		break;
	case LAZ_GM_DEATHMATCH:
		return "HL2 Lazuul: Team Deathmatch";
		break;
	case LAZ_GM_COOP:
		return "HL2 Lazuul: Co-op";
		break;
	case LAZ_GM_VERSUS:
		return "HL2 Lazuul: Versus";
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CLazuul::ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target)
{
	return sk_allow_autoaim.GetBool() != 0;
}

//---------------------------------------------------------
//---------------------------------------------------------
float CLazuul::GetAutoAimScale(CBasePlayer* pPlayer)
{
#ifdef _X360
	return 1.0f;
#else
	switch (GetSkillLevel())
	{
	case SKILL_EASY:
		return sk_autoaim_scale1.GetFloat();

	case SKILL_MEDIUM:
		return sk_autoaim_scale2.GetFloat();

	default:
		return 0.0f;
	}
#endif
}

//---------------------------------------------------------
//---------------------------------------------------------
float CLazuul::GetAmmoQuantityScale(int iAmmoIndex)
{
	switch (GetSkillLevel())
	{
	case SKILL_EASY:
		return sk_ammo_qty_scale1.GetFloat();

	case SKILL_MEDIUM:
		return sk_ammo_qty_scale2.GetFloat();

	case SKILL_HARD:
		return sk_ammo_qty_scale3.GetFloat();

	default:
		return 0.0f;
	}
}

void CLazuul::LevelInitPreEntity()
{
	// Remove this if you fix the bug in ep1 where the striders need to touch
	// triggers using their absbox instead of their bbox
	if (!Q_strnicmp(gpGlobals->mapname.ToCStr(), "ep1_", 4))
	{
		// episode 1 maps use the surrounding box trigger behavior
		CBaseEntity::sm_bAccurateTriggerBboxChecks = false;
	}
	else
	{
		CBaseEntity::sm_bAccurateTriggerBboxChecks = true;
	}

	BaseClass::LevelInitPreEntity();
}

void CLazuul::Think()
{
	BaseClass::Think();


	static ConVarRef physcannon_mega_enabled("physcannon_mega_enabled");
	if (physcannon_mega_enabled.GetBool() == true)
	{
		m_bMegaPhysgun = true;
	}
	else
	{
		// FIXME: Is there a better place for this?
		m_bMegaPhysgun = (GlobalEntity_GetState("super_phys_gun") == GLOBAL_ON);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLazuul::PlayerKilled(CBasePlayer* pVictim, const CTakeDamageInfo& info)
{
	// Find the killer & the scorer
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer* pScorer = ToBaseMultiplayerPlayer(GetDeathScorer(pKiller, pInflictor, pVictim));
	

	//find the area the player is in and see if his death causes a block
	CTriggerAreaCapture* pArea = dynamic_cast<CTriggerAreaCapture*>(gEntList.FindEntityByClassname(NULL, "trigger_capture_area"));
	while (pArea)
	{
		if (pArea->CheckIfDeathCausesBlock(ToBaseMultiplayerPlayer(pVictim), pScorer))
			break;

		pArea = dynamic_cast<CTriggerAreaCapture*>(gEntList.FindEntityByClassname(pArea, "trigger_capture_area"));
	}

	DeathNotice(pVictim, info);

	if (!m_DisableDeathPenalty)
	{
		pVictim->IncrementDeathCount(1);

		// dvsents2: uncomment when removing all FireTargets
		// variant_t value;
		// g_EventQueue.AddEvent( "game_playerdie", "Use", value, 0, pVictim, pVictim );
		FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

		// Did the player kill himself?
		if (pVictim == pScorer)
		{
			if (UseSuicidePenalty())
			{
				// Players lose a frag for killing themselves
				pVictim->IncrementFragCount(-1);
			}
		}
		else if (pScorer)
		{
			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			pScorer->IncrementFragCount(IPointsForKill(pScorer, pVictim));

			// Allow the scorer to immediately paint a decal
			pScorer->AllowImmediateDecalPainting();

			// dvsents2: uncomment when removing all FireTargets
			//variant_t value;
			//g_EventQueue.AddEvent( "game_playerkill", "Use", value, 0, pScorer, pScorer );
			FireTargets("game_playerkill", pScorer, pScorer, USE_TOGGLE, 0);
		}
		else
		{
			if (UseSuicidePenalty())
			{
				// Players lose a frag for letting the world kill them			
				pVictim->IncrementFragCount(-1);
			}
		}

		RecountTeams();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLazuul::NPCKilled(CAI_BaseNPC* pVictim, const CTakeDamageInfo& info)
{
	// Find the killer & the scorer
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBaseMultiplayerPlayer* pScorer = ToBaseMultiplayerPlayer(GetDeathScorer(pKiller, pInflictor, pVictim));

	DeathNotice(pVictim, info);

	if (pScorer)
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pScorer->IncrementFragCount(IPointsForKill(pScorer, pVictim));

		// Allow the scorer to immediately paint a decal
		pScorer->AllowImmediateDecalPainting();
	}
}

//-----------------------------------------------------------------------------
// Purpose: determine the class name of the weapon that got a kill
//-----------------------------------------------------------------------------
const char* CLazuul::GetKillingWeaponName(const CTakeDamageInfo& info, CBaseEntity* pVictim, int& iOutputID)
{
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBasePlayer* pScorer = GetDeathScorer(pKiller, pInflictor, pVictim);
	CWeaponCoopBase* pWeapon = dynamic_cast<CWeaponCoopBase*>(info.GetWeapon());
	int iWeaponID = HLSS_WEAPON_ID_NONE;

	const char* killer_weapon_name = "world";

	// Handle special kill types first.
	const char* pszCustomKill = NULL;

	switch (info.GetDamageCustom())
	{
	case TF_DMG_CUSTOM_SUICIDE:
		pszCustomKill = "world";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_HADOUKEN:
		pszCustomKill = "taunt_pyro";
		break;
	case TF_DMG_CUSTOM_BURNING_FLARE:
		pszCustomKill = "flaregun";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON:
		pszCustomKill = "taunt_heavy";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_FENCING:
		pszCustomKill = "taunt_spy";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB:
		pszCustomKill = "taunt_sniper";
		break;
	case TF_DMG_CUSTOM_TELEFRAG:
		pszCustomKill = "telefrag";
		break;
	case TF_DMG_CUSTOM_BURNING_ARROW:
		pszCustomKill = "huntsman_burning";
		break;
	case TF_DMG_CUSTOM_FLYINGBURN:
		pszCustomKill = "huntsman_flyingburn";
		break;
	case TF_DMG_CUSTOM_PUMPKIN_BOMB:
		pszCustomKill = "pumpkindeath";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_GRENADE:
		pszCustomKill = "taunt_soldier";
		break;
	case TF_DMG_CUSTOM_BASEBALL:
		pszCustomKill = "ball";
		break;
	case TF_DMG_CUSTOM_CHARGE_IMPACT:
		pszCustomKill = "demoshield";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING:
		pszCustomKill = "taunt_demoman";
		break;
	case TF_DMG_CUSTOM_DEFENSIVE_STICKY:
		pszCustomKill = "sticky_resistance";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_UBERSLICE:
		pszCustomKill = "taunt_medic";
		break;
	case TF_DMG_CUSTOM_PLAYER_SENTRY:
		pszCustomKill = "player_sentry";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH:
		pszCustomKill = "taunt_guitar_kill";
		break;
	case TF_DMG_CUSTOM_BLEEDING:
		pszCustomKill = "bleed_kill";
		break;
	case TF_DMG_CUSTOM_CARRIED_BUILDING:
		pszCustomKill = "building_carried_destroyed";
		break;
	case TF_DMG_CUSTOM_COMBO_PUNCH:
		pszCustomKill = "robot_arm_combo_kill";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL:
		pszCustomKill = "robot_arm_blender_kill";
		break;
	case TF_DMG_CUSTOM_STICKBOMB_EXPLOSION:
		pszCustomKill = "ullapool_caber_explosion";
		break;
	case TF_DMG_CUSTOM_DECAPITATION_BOSS:
		pszCustomKill = "battleaxe";
		break;
	case TF_DMG_CUSTOM_EYEBALL_ROCKET:
		pszCustomKill = "eyeball_rocket";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON:
		pszCustomKill = "armageddon";
		break;
	case TF_DMG_CUSTOM_CANNONBALL_PUSH:
		pszCustomKill = "loose_cannon_impact";
		break;
	case TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING:
		pszCustomKill = "dragons_fury_bonus";
		break;
	case TF_DMG_CUSTOM_CROC:
		pszCustomKill = "crocodile";
		break;
	case TF_DMG_CUSTOM_TAUNTATK_GASBLAST:
		pszCustomKill = "gas_blast";
		break;
	case LFE_DMG_CUSTOM_LEECHES:
		pszCustomKill = "leeches";
		break;
	case LFE_DMG_CUSTOM_JEEP:
		pszCustomKill = "jeep";
		break;
	case LFE_DMG_CUSTOM_AIRBOAT:
		pszCustomKill = "airboat";
		break;
	case LFE_DMG_CUSTOM_JALOPY:
		pszCustomKill = "jeep_episodic";
		break;
	case LFE_DMG_CUSTOM_HUNTER_FLECHETTE:
		pszCustomKill = "flechette";
		break;
	case LFE_DMG_CUSTOM_HUNTER_FLECHETTE_EXPLODE:
		pszCustomKill = "flechette_explode";
		break;
	case LFE_DMG_CUSTOM_HUNTER_CHARGE:
		pszCustomKill = "hunter_charge";
		break;
	case LFE_DMG_CUSTOM_ANTLION_WORKER_EXPLODE:
		pszCustomKill = "antworker_explosion";
		break;
	case LFE_DMG_CUSTOM_PHYSCANNON_MEGA:
		pszCustomKill = "physcannon_mega";
		break;
	case LFE_DMG_CUSTOM_PHYSCANNON_MEGA_TERTIARY:
		pszCustomKill = "physcannon_mega_tertiary";
		break;
	}

	if (pszCustomKill != NULL)
		return pszCustomKill;

	if (info.GetDamageCustom() == TF_DMG_CUSTOM_BURNING)
	{
		// Player stores last weapon that burned him so if he burns to death we know what killed him.
		if (pWeapon)
		{
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();

			//if (pInflictor && pInflictor != pScorer)
			//{
			//	CTFBaseRocket* pRocket = dynamic_cast<CTFBaseRocket*>(pInflictor);

			//	if (pRocket && pRocket->m_iDeflected)
			//	{
			//		// Fire weapon deflects go here.
			//		switch (pRocket->GetWeaponID())
			//		{
			//		case TF_WEAPON_FLAREGUN:
			//			killer_weapon_name = "deflect_flare";
			//			break;
			//		}
			//	}
			//}
		}
		else
		{
			// Default to flamethrower if no burn weapon is specified.
			killer_weapon_name = "tf_weapon_flamethrower";
			iWeaponID = HLSS_WEAPON_ID_FLAMER;
		}
	}
	else if (info.GetDamageCustom() == TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE)
	{
		// Player stores last weapon that burned him so if he burns to death we know what killed him.
		if (pWeapon)
		{
			killer_weapon_name = pWeapon->GetClassname();
			iWeaponID = pWeapon->GetWeaponID();

			//if (pInflictor && pInflictor != pScorer)
			//{
			//	CTFBaseRocket* pRocket = dynamic_cast<CTFBaseRocket*>(pInflictor);

			//	if (pRocket && pRocket->m_iDeflected)
			//	{
			//		// Fire weapon deflects go here.
			//		switch (pRocket->GetWeaponID())
			//		{
			//		case TF_WEAPON_ROCKETLAUNCHER_FIREBALL:
			//			killer_weapon_name = "dragons_fury";
			//			break;
			//		}
			//	}
			//}
		}
		else
		{
			// Default to flamethrower if no burn weapon is specified.
			killer_weapon_name = "dragons_fury";
			iWeaponID = HLSS_WEAPON_ID_FLAMER;
		}
	}
	else if (pScorer && pInflictor && (pInflictor == pScorer))
	{
		// If the inflictor is the killer, then it must be their current weapon doing the damage
		CWeaponCoopBase* pActiveWpn = dynamic_cast< CWeaponCoopBase *>(pScorer->GetActiveWeapon());
		if (pActiveWpn)
		{
			killer_weapon_name = pActiveWpn->GetClassname();
			iWeaponID = pActiveWpn->GetWeaponID();
		}
	}
	else if (pKiller && pKiller->IsNPC() && (pInflictor == pKiller))
	{
		// Similar case for NPCs.
		CAI_BaseNPC* pNPC = assert_cast<CAI_BaseNPC*>(pKiller);
		CWeaponCoopBase* pActiveWpn = dynamic_cast<CWeaponCoopBase*>(pNPC->GetActiveWeapon());
		if (pActiveWpn)
		{
			killer_weapon_name = pActiveWpn->GetClassname();
			iWeaponID = pActiveWpn->GetWeaponID();
		}
		else
		{
			killer_weapon_name = pNPC->GetClassname();
		}
	}
	else if (pInflictor)
	{
		killer_weapon_name = pInflictor->GetClassname();

		if (CWeaponCoopBase * pTFWeapon = dynamic_cast<CWeaponCoopBase*>(pInflictor))
		{
			iWeaponID = pTFWeapon->GetWeaponID();
		}
		// See if this was a deflect kill.
		//else if (CTFBaseRocket * pRocket = dynamic_cast<CTFBaseRocket*>(pInflictor))
		//{
		//	iWeaponID = pRocket->GetWeaponID();

		//	if (pRocket->m_iDeflected)
		//	{
		//		switch (pRocket->GetWeaponID())
		//		{
		//		case TF_WEAPON_ROCKETLAUNCHER:
		//			killer_weapon_name = "deflect_rocket";
		//			break;
		//		case TF_WEAPON_COMPOUND_BOW:
		//			if (info.GetDamageType() & DMG_IGNITE)
		//				killer_weapon_name = "deflect_huntsman_flyingburn";
		//			else
		//				killer_weapon_name = "deflect_arrow";
		//			break;
		//		}
		//	}
		//}
		//else if (CTFWeaponBaseGrenadeProj * pGrenade = dynamic_cast<CTFWeaponBaseGrenadeProj*>(pInflictor))
		//{
		//	iWeaponID = pGrenade->GetWeaponID();

		//	// Most grenades have their own kill icons except for pipes and stickies, those use weapon icons.
		//	if (iWeaponID == TF_WEAPON_GRENADE_DEMOMAN || iWeaponID == TF_WEAPON_GRENADE_PIPEBOMB)
		//	{
		//		CTFWeaponBase* pLauncher = dynamic_cast<CTFWeaponBase*>(pGrenade->m_hLauncher.Get());
		//		if (pLauncher)
		//		{
		//			iWeaponID = pLauncher->GetWeaponID();
		//		}
		//	}

		//	if (pGrenade->m_iDeflected)
		//	{
		//		switch (pGrenade->GetWeaponID())
		//		{
		//		case TF_WEAPON_GRENADE_PIPEBOMB:
		//			killer_weapon_name = "deflect_sticky";
		//			break;
		//		case TF_WEAPON_GRENADE_DEMOMAN:
		//			killer_weapon_name = "deflect_promode";
		//			break;
		//		}
		//	}
		//}
	}

	// Fix for HL2 pistol to prevent conflict with TF2 pistol. Must do this before stripping prefix.
	if (V_strcmp(killer_weapon_name, "weapon_pistol") == 0)
	{
		killer_weapon_name = "weapon_pistol_hl";
	}

	// strip certain prefixes from inflictor's classname
	const char* prefix[] = { "tf_weapon_grenade_", "tf_weapon_", "weapon_", "npc_", "func_", "prop_vehicle_", "prop_", "monster_" };
	for (int i = 0; i < ARRAYSIZE(prefix); i++)
	{
		// if prefix matches, advance the string pointer past the prefix
		int len = V_strlen(prefix[i]);
		if (V_strncmp(killer_weapon_name, prefix[i], len) == 0)
		{
			killer_weapon_name += len;
			break;
		}
	}

	// In case of a sentry kill change the icon according to sentry level.
	/*if (V_strcmp(killer_weapon_name, "obj_sentrygun") == 0)
	{
		CObjectSentrygun* pObject = assert_cast<CObjectSentrygun*>(pInflictor);

		if (pObject)
		{
			switch (pObject->GetUpgradeLevel())
			{
			case 2:
				killer_weapon_name = "obj_sentrygun2";
				break;
			case 3:
				killer_weapon_name = "obj_sentrygun3";
				break;
			}
			if (pObject->IsMiniBuilding())
			{
				killer_weapon_name = "obj_minisentry";
			}
		}
	}
	else*/ if (V_strcmp(killer_weapon_name, "tf_projectile_sentryrocket") == 0)
	{
		// look out for sentry rocket as weapon and map it to sentry gun, so we get the L3 sentry death icon
		killer_weapon_name = "obj_sentrygun3";
	}
	// make sure arrow kills are mapping to the huntsman
	else if (0 == V_strcmp(killer_weapon_name, "tf_projectile_arrow"))
	{
		if (info.GetDamageType() & DMG_IGNITE)
			killer_weapon_name = "huntsman_flyingburn";
		else
			killer_weapon_name = "huntsman";
	}
	// Some special cases for NPCs.
	else if (V_strcmp(killer_weapon_name, "strider") == 0)
	{
		if (info.GetDamageType() & DMG_BULLET)
		{
			killer_weapon_name = "strider_minigun";
		}
		else if (info.GetDamageType() & DMG_CRUSH)
		{
			killer_weapon_name = "strider_skewer";
		}
	}
	else if (V_strcmp(killer_weapon_name, "concussiveblast") == 0)
	{
		if (pKiller && FClassnameIs(pKiller, "npc_strider"))
		{
			killer_weapon_name = "strider_beam";
		}
	}
	else if (V_strcmp(killer_weapon_name, "vortigaunt") == 0)
	{
		if (info.GetDamageType() & DMG_SHOCK)
		{
			killer_weapon_name = "vortigaunt_beam";
		}
	}
	else if (V_strcmp(killer_weapon_name, "jeep") == 0)
	{
		if (info.GetDamageType() & DMG_SHOCK)
		{
			killer_weapon_name = "gauss";
		}
	}
	else if (V_strcmp(killer_weapon_name, "airboat") == 0)
	{
		if (info.GetDamageType() & DMG_AIRBOAT)
		{
			killer_weapon_name = "helicopter";
		}
	}
	else if (V_strcmp(killer_weapon_name, "hunter") == 0)
	{
		if (info.GetDamageType() & DMG_SLASH)
		{
			killer_weapon_name = "hunter_skewer";
		}
		else
		{
			killer_weapon_name = "hunter_pound";
		}
	}

	else if (iWeaponID)
	{
		iOutputID = iWeaponID;
	}

	return killer_weapon_name;
}

//-----------------------------------------------------------------------------
// Purpose: returns the player/NPC who assisted in the kill, or NULL if no assister
//-----------------------------------------------------------------------------
CBaseEntity* CLazuul::GetAssister(CBaseEntity * pVictim, CBaseEntity * pKiller, CBaseEntity * pInflictor)
{
	// See who has damaged the victim 2nd most recently (most recent is the killer), and if that is within a certain time window.
	// If so, give that player an assist.  (Only 1 assist granted, to single other most recent damager.)
	CBaseEntity * pRecentDamager = GetRecentDamager(pVictim, 1, TF_TIME_ASSIST_KILL);
	if (pRecentDamager && (pRecentDamager != pKiller))
		return pRecentDamager;

	// Killing entity might be specifying a scorer player
	IScorer *pScorerInterface = dynamic_cast<IScorer*>(pKiller);
	if (pScorerInterface)
	{
		CBaseEntity *pPlayer = pScorerInterface->GetAssistant();
		if (pPlayer)
			return pPlayer;
	}

	// Inflicting entity might be specifying a scoring player
	pScorerInterface = dynamic_cast<IScorer*>(pInflictor);
	if (pScorerInterface)
	{
		CBaseEntity *pPlayer = pScorerInterface->GetAssistant();
		if (pPlayer)
			return pPlayer;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns specifed recent damager, if there is one who has done damage
//			within the specified time period.  iDamager=0 returns the most recent
//			damager, iDamager=1 returns the next most recent damager.
//-----------------------------------------------------------------------------
CBaseEntity* CLazuul::GetRecentDamager(CBaseEntity * pVictim, int iDamager, float flMaxElapsed)
{
	Assert(iDamager < MAX_DAMAGER_HISTORY);

	DamagerHistory_t* pDamagerHistory = NULL;

	if (pVictim->IsPlayer())
	{
		CBasePlayer* pTFVictim = ToBasePlayer(pVictim);
		pDamagerHistory = &pTFVictim->GetDamagerHistory(iDamager);
	}
	else if (pVictim->IsNPC())
	{
		CAI_BaseNPC* pNPC = assert_cast<CAI_BaseNPC*>(pVictim);
		pDamagerHistory = &pNPC->GetDamagerHistory(iDamager);
	}

	if (pDamagerHistory)
	{
		if ((pDamagerHistory->hDamager.Get() != NULL) && (gpGlobals->curtime - pDamagerHistory->flTimeDamage <= flMaxElapsed))
		{
			return pDamagerHistory->hDamager.Get();
		}

		return NULL;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Returns who should be awarded the kill
//-----------------------------------------------------------------------------
CBasePlayer* CLazuul::GetDeathScorer(CBaseEntity * pKiller, CBaseEntity * pInflictor, CBaseEntity * pVictim)
{
	if ((pKiller == pVictim) && (pKiller == pInflictor))
	{
		// If this was an explicit suicide, see if there was a damager within a certain time window.  If so, award this as a kill to the damager.
		if (pVictim)
		{
			CBasePlayer* pRecentDamager = ToBasePlayer(GetRecentDamager(pVictim, 0, TF_TIME_SUICIDE_KILL_CREDIT));
			if (pRecentDamager)
				return pRecentDamager;
		}
	}

	return BaseClass::GetDeathScorer(pKiller, pInflictor, pVictim);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CLazuul::DeathNotice(CBasePlayer * pVictim, const CTakeDamageInfo & info)
{
	int killer_ID = 0;
	int killer_index = 0;

	const char* killer_Name = 0;
	const char* assister_Name = 0;

	// Find the killer & the scorer
	CBasePlayer* pTFPlayerVictim = ToBasePlayer(pVictim);
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBasePlayer* pScorer = GetDeathScorer(pKiller, pInflictor, pVictim);
	CBaseEntity* pAssister = GetAssister(pVictim, pKiller, pInflictor);
	CBasePlayer* pTFAssister = ToBasePlayer(pAssister);

	if (pKiller)
	{
		if (pKiller->IsNPC())
			killer_Name = pKiller->MyNPCPointer()->GetDeathNoticeNameOverride();
		if (!killer_Name)
			killer_Name = pKiller->GetClassname();
	}

	if (pAssister)
	{
		if (pAssister->IsNPC())
			assister_Name = pAssister->MyNPCPointer()->GetDeathNoticeNameOverride();
		if (!assister_Name)
			assister_Name = pAssister->GetClassname();
	}

	int iWeaponID = HLSS_WEAPON_ID_NONE;

	// Work out what killed the player, and send a message to all clients about it
	const char* killer_weapon_name = GetKillingWeaponName(info, pTFPlayerVictim, iWeaponID);
	const char* killer_weapon_log_name = NULL;

	if (iWeaponID)
	{
		CBaseCombatWeapon* pWeapon = nullptr;
		if (pScorer)
			pWeapon = pScorer->Weapon_OwnsThisID(iWeaponID);
		else if (pKiller && pKiller->MyCombatCharacterPointer())
			pWeapon = pKiller->MyCombatCharacterPointer()->Weapon_OwnsThisID(iWeaponID);

		if (pWeapon)
		{
			CEconItemDefinition* pItemDef = pWeapon->GetItem()->GetStaticData();
			if (pItemDef)
			{
				if (pItemDef->item_iconname[0])
					killer_weapon_name = pItemDef->item_iconname;

				if (pItemDef->item_logname[0])
					killer_weapon_log_name = pItemDef->item_logname;
			}
		}
	}

	if (pScorer)	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
		killer_index = pScorer->entindex();
	}
	else if (pKiller && pKiller->IsNPC())
	{
		killer_index = pKiller->entindex();
	}

	IGameEvent* event = gameeventmanager->CreateEvent("player_death");

	if (event)
	{
		event->SetInt("userid", pVictim->GetUserID());
		event->SetInt("victim_index", pVictim->entindex());
		event->SetInt("attacker", killer_ID);
		event->SetInt("attacker_index", killer_index);
		event->SetString("attacker_name", pKiller ? killer_Name : NULL);
		event->SetInt("attacker_team", pKiller ? pKiller->GetTeamNumber() : 0);
		event->SetInt("assister", pTFAssister ? pTFAssister->GetUserID() : -1);
		event->SetInt("assister_index", pAssister ? pAssister->entindex() : -1);
		event->SetString("assister_name", pAssister ? assister_Name : NULL);
		event->SetInt("assister_team", pAssister ? pAssister->GetTeamNumber() : 0);
		event->SetString("weapon", killer_weapon_name);
		event->SetInt("weapon_index", pInflictor ? pInflictor->entindex() : -1);
		event->SetString("weapon_logclassname", killer_weapon_log_name);
		event->SetInt("playerpenetratecount", info.GetPlayerPenetrationCount());
		event->SetInt("damagebits", info.GetDamageType());
		event->SetInt("customkill", info.GetDamageCustom());
		event->SetInt("priority", 7);	// HLTV event priority, not transmitted
		event->SetInt("death_flags", 0);
#if 0
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_DOMINATION)
		{
			event->SetInt("dominated", 1);
		}
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_DOMINATION)
		{
			event->SetInt("assister_dominated", 1);
		}
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_REVENGE)
		{
			event->SetInt("revenge", 1);
		}
		if (pTFPlayerVictim->GetDeathFlags() & TF_DEATH_ASSISTER_REVENGE)
		{
			event->SetInt("assister_revenge", 1);
		}
#endif

		gameeventmanager->FireEvent(event);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			*pKiller - 
//			*pInflictor - 
//-----------------------------------------------------------------------------
void CLazuul::DeathNotice(CAI_BaseNPC* pVictim, const CTakeDamageInfo& info)
{
	int killer_ID = 0;
	int killer_index = 0;

	const char* killer_Name = 0;
	const char* assister_Name = 0;
	const char* victim_Name = 0;

	// Find the killer & the scorer
	CBaseEntity* pInflictor = info.GetInflictor();
	CBaseEntity* pKiller = info.GetAttacker();
	CBasePlayer* pScorer = GetDeathScorer(pKiller, pInflictor, pVictim);
	CBaseEntity* pAssister = GetAssister(pVictim, pKiller, pInflictor);
	//CBasePlayer* pTFAssister = ToBasePlayer(pAssister);

	if (pVictim)
	{
		victim_Name = pVictim->GetDeathNoticeNameOverride();
		if (!victim_Name)
			victim_Name = pVictim->GetClassname();
	}

	if (pKiller)
	{
		if (pKiller->IsNPC())
			killer_Name = pKiller->MyNPCPointer()->GetDeathNoticeNameOverride();
		if (!killer_Name)
			killer_Name = pKiller->GetClassname();
	}

	if (pAssister)
	{
		if (pAssister->IsNPC())
			assister_Name = pAssister->MyNPCPointer()->GetDeathNoticeNameOverride();
		if (!assister_Name)
			assister_Name = pAssister->GetClassname();
	}

	int iWeaponID = HLSS_WEAPON_ID_NONE;

	// Work out what killed the player, and send a message to all clients about it
	const char* killer_weapon_name = GetKillingWeaponName(info, pVictim, iWeaponID);
	const char* killer_weapon_log_name = NULL;

	if (iWeaponID)
	{
		CBaseCombatWeapon* pWeapon = nullptr;
		if (pScorer)
			pWeapon = pScorer->Weapon_OwnsThisID(iWeaponID);
		else if (pKiller && pKiller->MyCombatCharacterPointer())
			pWeapon = pKiller->MyCombatCharacterPointer()->Weapon_OwnsThisID(iWeaponID);

		if (pWeapon)
		{
			CEconItemDefinition* pItemDef = pWeapon->GetItem()->GetStaticData();
			if (pItemDef)
			{
				if (pItemDef->item_iconname[0])
					killer_weapon_name = pItemDef->item_iconname;

				if (pItemDef->item_logname[0])
					killer_weapon_log_name = pItemDef->item_logname;
			}
		}
	}

	bool bShowDeathNotice = pVictim->ShowInDeathnotice();

	if (pScorer)	// Is the killer a client?
	{
		killer_ID = pScorer->GetUserID();
		killer_index = pScorer->entindex();
		bShowDeathNotice = true;
	}
	else if (pKiller && pKiller->IsNPC())
	{
		killer_index = pKiller->entindex();
		if (!bShowDeathNotice && pKiller->MyNPCPointer()->ShowInDeathnotice())
		{
			bShowDeathNotice = true;
		}
	}

	if (!bShowDeathNotice && pAssister)
	{
		if (pAssister->IsPlayer() || (pAssister->IsNPC() && pAssister->MyNPCPointer()->ShowInDeathnotice()))
			bShowDeathNotice = true;
	}

	IGameEvent* event = gameeventmanager->CreateEvent("npc_death");

	if (event)
	{
		event->SetInt("victim_index", pVictim->entindex());
		event->SetString("victim_name", victim_Name);
		event->SetInt("victim_team", pVictim->GetTeamNumber());
		event->SetInt("attacker_index", killer_index);
		event->SetString("attacker_name", pKiller ? killer_Name : NULL);
		event->SetInt("attacker_team", pKiller ? pKiller->GetTeamNumber() : 0);
		event->SetInt("assister_index", pAssister ? pAssister->entindex() : -1);
		event->SetString("assister_name", pAssister ? assister_Name : NULL);
		event->SetInt("assister_team", pAssister ? pAssister->GetTeamNumber() : 0);
		event->SetString("weapon", killer_weapon_name);
		event->SetInt("weapon_index", pInflictor ? pInflictor->entindex() : -1);
		event->SetString("weapon_logclassname", killer_weapon_log_name);
		event->SetInt("damagebits", info.GetDamageType());
		event->SetInt("customkill", info.GetDamageCustom());
		event->SetBool("show_notice", bShowDeathNotice); // are we allowed to make deathnotice on THIS npc?
		event->SetInt("priority", bShowDeathNotice ? 7 : 3);	// HLTV event priority, not transmitted

		gameeventmanager->FireEvent(event);
	}
}

//-----------------------------------------------------------------------------
// Purpose: create some proxy entities that we use for transmitting data */
//-----------------------------------------------------------------------------
void CLazuul::CreateStandardEntities()
{
#if 0
	// Create the player resource
	g_pPlayerResource = (CPlayerResource*)CBaseEntity::Create("tf_player_manager", vec3_origin, vec3_angle);

	// Create the objective resource
	g_pObjectiveResource = (CTFObjectiveResource*)CBaseEntity::Create("tf_objective_resource", vec3_origin, vec3_angle);
#else
	BaseClass::CreateStandardEntities();

	g_pObjectiveResource = (CBaseTeamObjectiveResource*)CBaseEntity::Create("objective_resource", vec3_origin, vec3_angle);
#endif

	//Assert(g_pObjectiveResource);

	// Create the entity that will send our data to the client.
	CBaseEntity* pEnt = CBaseEntity::Create("laz_gamerules", vec3_origin, vec3_angle);
	Assert(pEnt);
	pEnt->SetName(AllocPooledString("laz_gamerules"));
	pEnt->KeyValue("allowdeathmatch", "1");
}

void CLazuul::CleanUpMap(void)
{
	BaseClass::CleanUpMap();

	if (HLTVDirector())
	{
		HLTVDirector()->BuildCameraList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLazuul::RoundCleanupShouldIgnore(CBaseEntity* pEnt)
{
	if (FindInList(s_LazPreserveEnts, pEnt->GetClassname()))
		return true;

	return BaseClass::RoundCleanupShouldIgnore(pEnt);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CLazuul::ShouldCreateEntity(const char* pszClassName)
{
	if (FindInList(s_LazPreserveEnts, pszClassName))
		return false;

	return BaseClass::ShouldCreateEntity(pszClassName);
}

void CLazuul::SetupSpawnPointsForRound(void)
{
	CBaseEntity *pLaz = gEntList.FindEntityByClassname(nullptr, "laz_gamerules");
	CBaseEntity *pEnt = nullptr;
	while ((pEnt = gEntList.FindEntityByClassname(pEnt, "info_player_teamspawn")) != nullptr)
	{
		variant_t var;
		pEnt->AcceptInput("RoundSpawn", pLaz, pLaz, var, 0);
	}
}
#endif

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_ar2", "sk_npc_dmg_ar2", "sk_max_ar2", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("AlyxGun", DMG_BULLET, TRACER_LINE, "sk_plr_dmg_alyxgun", "sk_npc_dmg_alyxgun", "sk_max_alyxgun", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_pistol", "sk_npc_dmg_pistol", "sk_max_pistol", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_smg1", "sk_npc_dmg_smg1", "sk_max_smg1", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("357", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_357", "sk_npc_dmg_357", "sk_max_357", BULLET_IMPULSE(800, 5000), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET, TRACER_LINE, "sk_plr_dmg_crossbow", "sk_npc_dmg_crossbow", "sk_max_crossbow", BULLET_IMPULSE(800, 8000), 0);

		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, "sk_plr_dmg_buckshot", "sk_npc_dmg_buckshot", "sk_max_buckshot", BULLET_IMPULSE(400, 1200), 0);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, "sk_plr_dmg_rpg_round", "sk_npc_dmg_rpg_round", "sk_max_rpg_round", 0, 0);
		def.AddAmmoType("SMG1_Grenade", DMG_BURN, TRACER_NONE, "sk_plr_dmg_smg1_grenade", "sk_npc_dmg_smg1_grenade", "sk_max_smg1_grenade", 0, 0);
		def.AddAmmoType("SniperRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE, "sk_plr_dmg_sniper_round", "sk_npc_dmg_sniper_round", "sk_max_sniper_round", BULLET_IMPULSE(650, 6000), 0);
		def.AddAmmoType("SniperPenetratedRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE, "sk_dmg_sniper_penetrate_plr", "sk_dmg_sniper_penetrate_npc", "sk_max_sniper_round", BULLET_IMPULSE(150, 6000), 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, "sk_plr_dmg_grenade", "sk_npc_dmg_grenade", "sk_max_grenade", 0, 0);
		def.AddAmmoType("Thumper", DMG_SONIC, TRACER_NONE, 10, 10, 2, 0, 0);
		def.AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0, 0, 8, 0, 0);
		//		def.AddAmmoType("Extinguisher",		DMG_BURN,					TRACER_NONE,			0,	0, 100, 0, 0 );
		def.AddAmmoType("Battery", DMG_CLUB, TRACER_NONE, NULL, NULL, NULL, 0, 0);
		def.AddAmmoType("GaussEnergy", DMG_SHOCK, TRACER_NONE, "sk_jeep_gauss_damage", "sk_jeep_gauss_damage", "sk_max_gauss_round", BULLET_IMPULSE(650, 8000), 0); // hit like a 10kg weight at 400 in/s
		def.AddAmmoType("CombineCannon", DMG_BULLET, TRACER_LINE, "sk_npc_dmg_gunship_to_plr", "sk_npc_dmg_gunship", NULL, 1.5 * 750 * 12, 0); // hit like a 1.5kg weight at 750 ft/s
		def.AddAmmoType("AirboatGun", DMG_AIRBOAT, TRACER_LINE, "sk_plr_dmg_airboat", "sk_npc_dmg_airboat", NULL, BULLET_IMPULSE(10, 600), 0);

		//=====================================================================
		// STRIDER MINIGUN DAMAGE - Pull up a chair and I'll tell you a tale.
		//
		// When we shipped Half-Life 2 in 2004, we were unaware of a bug in
		// CAmmoDef::NPCDamage() which was returning the MaxCarry field of
		// an ammotype as the amount of damage that should be done to a NPC
		// by that type of ammo. Thankfully, the bug only affected Ammo Types 
		// that DO NOT use ConVars to specify their parameters. As you can see,
		// all of the important ammotypes use ConVars, so the effect of the bug
		// was limited. The Strider Minigun was affected, though.
		//
		// According to my perforce Archeology, we intended to ship the Strider
		// Minigun ammo type to do 15 points of damage per shot, and we did. 
		// To achieve this we, unaware of the bug, set the Strider Minigun ammo 
		// type to have a maxcarry of 15, since our observation was that the 
		// number that was there before (8) was indeed the amount of damage being
		// done to NPC's at the time. So we changed the field that was incorrectly
		// being used as the NPC Damage field.
		//
		// The bug was fixed during Episode 1's development. The result of the 
		// bug fix was that the Strider was reduced to doing 5 points of damage
		// to NPC's, since 5 is the value that was being assigned as NPC damage
		// even though the code was returning 15 up to that point.
		//
		// Now as we go to ship Orange Box, we discover that the Striders in 
		// Half-Life 2 are hugely ineffective against citizens, causing big
		// problems in maps 12 and 13. 
		//
		// In order to restore balance to HL2 without upsetting the delicate 
		// balance of ep2_outland_12, I have chosen to build Episodic binaries
		// with 5 as the Strider->NPC damage, since that's the value that has
		// been in place for all of Episode 2's development. Half-Life 2 will
		// build with 15 as the Strider->NPC damage, which is how HL2 shipped
		// originally, only this time the 15 is located in the correct field
		// now that the AmmoDef code is behaving correctly.
		//
		//=====================================================================

		def.AddAmmoType("StriderMinigunEP2", DMG_BULLET, TRACER_LINE, 5, 5, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 1.0kg weight at 750 ft/s

		def.AddAmmoType("StriderMinigun", DMG_BULLET, TRACER_LINE, 5, 15, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 1.0kg weight at 750 ft/s


		def.AddAmmoType("StriderMinigunDirect", DMG_BULLET, TRACER_LINE, 2, 2, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 1.0kg weight at 750 ft/s
		def.AddAmmoType("HelicopterGun", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_npc_dmg_helicopter_to_plr", "sk_npc_dmg_helicopter", "sk_max_smg1", BULLET_IMPULSE(400, 1225), AMMO_FORCE_DROP_IF_CARRIED | AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER);
		def.AddAmmoType("AR2AltFire", DMG_DISSOLVE, TRACER_NONE, 0, 0, "sk_max_ar2_altfire", 0, 0);
		def.AddAmmoType("slam", DMG_BURN, TRACER_NONE, "sk_plr_dmg_grenade", "sk_npc_dmg_grenade", "sk_max_slam", 0, 0);
#ifdef HL2_EPISODIC
		def.AddAmmoType("Hopwire", DMG_BLAST, TRACER_NONE, "sk_plr_dmg_grenade", "sk_npc_dmg_grenade", "sk_max_hopwire", 0, 0);
		def.AddAmmoType("CombineHeavyCannon", DMG_BULLET, TRACER_LINE, 40, 40, NULL, 10 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 10 kg weight at 750 ft/s
		def.AddAmmoType("ammo_proto1", DMG_BULLET, TRACER_LINE, 0, 0, 10, 0, 0);
#endif // HL2_EPISODIC

		// HL1
		def.AddAmmoType("9mmRound", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, "sk_plr_dmg_9mm_bullet", "sk_npc_dmg_9mm_bullet", "sk_max_9mm_bullet", BULLET_IMPULSE(500, 1325), 0);
		def.AddAmmoType("357Round", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, "sk_plr_dmg_357_bullet", "sk_npc_dmg_12mm_bullet", "sk_max_357_bullet", BULLET_IMPULSE(650, 6000), 0);
		def.AddAmmoType("BuckshotHL1", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, "sk_plr_dmg_buckshot", NULL, "sk_max_buckshot", BULLET_IMPULSE(200, 1200), 0);
		def.AddAmmoType("XBowBoltHL1", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, "sk_plr_dmg_xbow_bolt_plr", NULL, "sk_max_xbow_bolt", BULLET_IMPULSE(200, 1200), 0);
		def.AddAmmoType("MP5_Grenade", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_mp5_grenade", NULL, "sk_max_mp5_grenade", 0, 0);
		def.AddAmmoType("RPG_Rocket", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_rpg", NULL, "sk_max_rpg_rocket", 0, 0);
		def.AddAmmoType("Uranium", DMG_ENERGYBEAM, TRACER_NONE, NULL, NULL, "sk_max_uranium", 0, 0);
		def.AddAmmoType("GrenadeHL1", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_grenade", NULL, "sk_max_grenade", 0, 0);
		def.AddAmmoType("Hornet", DMG_BULLET, TRACER_NONE, "sk_plr_dmg_hornet", "sk_npc_dmg_hornet", "sk_max_hornet", BULLET_IMPULSE(100, 1200), 0);
		def.AddAmmoType("Snark", DMG_SLASH, TRACER_NONE, "sk_snark_dmg_bite", NULL, "sk_max_snark", 0, 0);
		def.AddAmmoType("TripMine", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_tripmine", NULL, "sk_max_tripmine", 0, 0);
		def.AddAmmoType("Satchel", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_satchel", NULL, "sk_max_satchel", 0, 0);

		def.AddAmmoType("12mmRound", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE_AND_WHIZ, NULL, "sk_npc_dmg_12mm_bullet", NULL, BULLET_IMPULSE(300, 1200), 0);

		// BMS
		def.AddAmmoType("9mm", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, "sk_plr_dmg_9mm_bullet", "sk_npc_dmg_9mm_bullet", "sk_max_9mm_bullet", BULLET_IMPULSE(500, 1325), 0);
		def.AddAmmoType("grenade_mp5", DMG_BURN, TRACER_NONE, "sk_plr_dmg_smg1_grenade", "sk_npc_dmg_smg1_grenade", "sk_max_smg1_grenade", 0, 0);
	}

	return &def;
}

#ifndef CLIENT_DLL
class CTeamObjectiveResource : public CBaseTeamObjectiveResource
{
	DECLARE_CLASS(CTeamObjectiveResource, CBaseTeamObjectiveResource);
public:
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn(void);

	// capabilities
	virtual int	ObjectCaps(void)
	{
		return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
	}
};

BEGIN_DATADESC(CTeamObjectiveResource)
END_DATADESC()


LINK_ENTITY_TO_CLASS(objective_resource, CTeamObjectiveResource);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamObjectiveResource::Spawn(void)
{
	BaseClass::Spawn();
}

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
		const char* pchVoice = pkvAbillites->GetString("voice", DEFAULT_VOICE);
		const char* pchSuit = pkvAbillites->GetString("suit", DEFAULT_VOICE);
		pHLMS->SetVoiceType(pchVoice, pchSuit);

		const char* pchFootSound = pkvAbillites->GetString("footsteps", DEFAULT_FEET);
		pHLMS->SetFootsteps(pchFootSound);
	}
	else
	{
		pHLMS->SetVoiceType(DEFAULT_VOICE, DEFAULT_VOICE);
		pHLMS->SetFootsteps(DEFAULT_FEET);
	}
}
#endif