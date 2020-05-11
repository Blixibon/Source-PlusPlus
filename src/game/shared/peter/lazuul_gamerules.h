#ifndef LAZUUL_GAMERULES_H
#define LAZUUL_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "teamplayroundbased_gamerules.h"
#include "hl2_gamerules.h"
#include "tf_shareddefs.h"
#include "utlmultilist.h"

#ifdef CLIENT_DLL
#define CLazuul C_Lazuul
#define CLazuulProxy C_LazuulProxy
#endif // CLIENT_DLL

//enum
//{
//	TEAM_COMBINE = FIRST_GAME_TEAM,
//	TEAM_REBELS,
//};

enum LazFaction_t
{
	FACTION_NONE = 0,
	FACTION_GOODGUYS,
	FACTION_COMBINE,
	FACTION_HOSTILEFAUNA,
	FACTION_ZOMBIES,
	FACTION_MARINES,
	FACTION_XENIANS,
	FACTION_ROGUE_COMPUTERS,

	LAZ_FACTION_COUNT
};

enum {
	LAZ_GM_SINGLEPLAYER = -1,
	LAZ_GM_DEATHMATCH = 0,
	LAZ_GM_CAMPAIGN,
	LAZ_GM_BASE_DEFENSE,

	LAZ_GM_COUNT
};

enum BaseDefenseModes_e
{
	LAZ_BD_ASSAULT = 0,
	LAZ_BD_TRAITOR,
};

#ifndef CLIENT_DLL
class IGameModeControllerEntity
{
public:
	virtual bool ShouldActivateMode(int iGameMode) = 0;
	virtual bool CanActivateVariant(int iGameMode, int iVariant) = 0;
};

class CLaz_Player;

class ITeamRespawnWaveHandler
{
public:
	virtual bool RespawnPlayer(CBasePlayer* pPlayer) = 0;
	virtual bool IsCurrentlyHandlingPlayer(CBasePlayer* pPlayer) = 0;
	virtual bool IsEnabled() = 0;
	virtual bool CanSpawnPlayersNow() = 0;
};
#endif
class CLazuulProxy : public CTeamplayRoundBasedRulesProxy
{
public:
	DECLARE_CLASS(CLazuulProxy, CTeamplayRoundBasedRulesProxy);
	DECLARE_NETWORKCLASS();
};

class CLazuul : public CTeamplayRoundBasedRules, public IHalfLife2
{
	DECLARE_CLASS(CLazuul, CTeamplayRoundBasedRules)
	DECLARE_NETWORKCLASS_NOBASE()
public:
	CLazuul();
	virtual ~CLazuul();

	// Damage Query Overrides.
	virtual bool			Damage_IsTimeBased(int64 iDmgType);
	// TEMP:
	virtual int64				Damage_GetTimeBased(void);

	int				GetFarthestOwnedControlPoint(int iTeam, bool bWithSpawnpoints);
	virtual bool	TeamMayCapturePoint(int iTeam, int iPointIndex);
	virtual bool	PlayerMayCapturePoint(CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0);
	virtual bool	PlayerMayBlockPoint(CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0);

	virtual bool			ShouldCollide(int collisionGroup0, int collisionGroup1);
	virtual bool			ShouldUseRobustRadiusDamage(CBaseEntity* pEntity);

	virtual bool	IsHolidayActive(EHoliday holiday);

	virtual bool	IsConnectedUserInfoChangeAllowed(CBasePlayer *pPlayer) { return true; }
#ifndef CLIENT_DLL
	void			AddRespawnWaveHandler(ITeamRespawnWaveHandler* pHandler, int iTeam);
	void			RemoveRespawnWaveHandler(ITeamRespawnWaveHandler* pHandler);

	virtual LazFaction_t GetFactionForClass(Class_T nClass);

	virtual void ClientCommandKeyValues(edict_t* pEntity, KeyValues* pKeyValues);

	virtual void CreateCustomNetworkStringTables(void);

	virtual bool			ShouldAutoAim(CBasePlayer* pPlayer, edict_t* target);
	virtual float			GetAutoAimScale(CBasePlayer* pPlayer);
	virtual float			GetAmmoQuantityScale(int iAmmoIndex);
	virtual void			LevelInitPreEntity();
	virtual void			LevelInitPostEntity();

	// Speaking, vcds, voice commands.
	virtual void	InitCustomResponseRulesDicts();
	virtual void	ShutdownCustomResponseRulesDicts();

	virtual float FlPlayerFallDamage(CBasePlayer* pPlayer);

	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);

	virtual void PlayerSpawn(CBasePlayer* pPlayer);
	virtual bool			ClientCommand(CBaseEntity *pEdict, const CCommand &args);

	// Location name shown in chat
	virtual const char* GetChatLocation(bool bTeamOnly, CBasePlayer* pPlayer);

	// VGUI format string for chat, if desired
	virtual const char *GetChatFormat(bool bTeamOnly, CBasePlayer *pPlayer);

	virtual void CleanUpMap(void);
	virtual void GetMapEditVariants(CUtlStringList& vecList);

	// Game Achievements (server version)
	virtual void MarkAchievement(IRecipientFilter& filter, char const *pchAchievementName);

	// NPCs
	virtual bool FAllowNPCs(void);

	// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn(CBaseCombatWeapon* pWeapon);
	//virtual float FlWeaponRespawnTime(CBaseCombatWeapon* pWeapon);
	//virtual float FlWeaponTryRespawn(CBaseCombatWeapon* pWeapon);
	//virtual Vector VecWeaponRespawnSpot(CBaseCombatWeapon* pWeapon);

	// Item spawn/respawn control
	virtual int ItemShouldRespawn(CItem* pItem);
	//virtual float FlItemRespawnTime(CItem* pItem);
	//virtual Vector VecItemRespawnSpot(CItem* pItem);
	//virtual QAngle VecItemRespawnAngles(CItem* pItem);

	// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime(void);	// how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime(void);		// how long until a depleted HealthCharger recharges itself?

	virtual void Status(void(*print) (const char *fmt, ...));

	virtual void GetTaggedConVarList(KeyValues *pCvarTagList);
#else
	int		GetGameForMap() { return m_iMapGameType; }
	int		GetModForMap() { return m_iMapModType; }

	virtual void	GetTeamGlowColor(int nTeam, float& r, float& g, float& b);
#endif

	bool	MegaPhyscannonActive(void) { return m_bMegaPhysgun; }

	bool	IsMultiplayer() { return (gpGlobals->maxClients > 1); }
	bool	IsCoop()
	{
		int iMode = GetGameMode();
		return (iMode == LAZ_GM_CAMPAIGN /*|| iMode == LAZ_GM_VERSUS*/);
	}
	bool	IsDeathmatch()
	{
		int iMode = GetGameMode();
		return (iMode == LAZ_GM_DEATHMATCH || iMode == LAZ_GM_BASE_DEFENSE);
	}

	int		GetGameMode()
	{
		if (!IsMultiplayer())
			return LAZ_GM_SINGLEPLAYER;
		return m_nGameMode;
	}

	int		GetGameModeSubType()
	{
		if (!IsMultiplayer())
			return 0;
		return m_nGameModeVariant;
	}

	int		GetProtaganistTeam();
	int		GetAntagonistTeam();

	int		GetTeamInRole(int iRole);

	int		GetNumTeams();

#ifndef CLIENT_DLL
	bool			Player_CanDoTeamChange(int iOldTeam, int iNewTeam);

	virtual float			GetAmmoDamage(CBaseEntity* pAttacker, CBaseEntity* pVictim, int nAmmoType);

	virtual bool IsAlyxInDarknessMode();
	virtual void			Think(void);

	virtual void			InitDefaultAIRelationships(void);
	virtual const char* AIClassText(int classType);
	virtual const char* GetGameDescription(void);

	// Override this to prevent removal of game specific entities that need to persist
	virtual bool	RoundCleanupShouldIgnore(CBaseEntity* pEnt);
	virtual bool	ShouldCreateEntity(const char* pszClassName);

	// Setup spawn points for the current round before it starts
	virtual void	SetupSpawnPointsForRound(void);

	// Called when a new round is being initialized
	virtual void	SetupOnRoundStart(void);

	// Called when a new round is off and running
	virtual void	SetupOnRoundRunning(void);

	// Sets up g_pPlayerResource.
	virtual void CreateStandardEntities();

	// Can only set on server
	void	SetGameMode(int iMode);

	virtual bool			ShouldBurningPropsEmitLight();

	bool AllowDamage(CBaseEntity* pVictim, const CTakeDamageInfo& info);

	virtual void PlayerKilled(CBasePlayer* pVictim, const CTakeDamageInfo& info);
	virtual void NPCKilled(CAI_BaseNPC* pVictim, const CTakeDamageInfo& info);
	virtual void DeathNotice(CBasePlayer* pVictim, const CTakeDamageInfo& info);
	virtual void DeathNotice(CAI_BaseNPC* pVictim, const CTakeDamageInfo& info);
	virtual CBasePlayer* GetDeathScorer(CBaseEntity* pKiller, CBaseEntity* pInflictor, CBaseEntity* pVictim);

	const char* GetKillingWeaponName(const CTakeDamageInfo& info, CBaseEntity* pVictim, int& iWeaponID);
	CBaseEntity* GetAssister(CBaseEntity* pVictim, CBaseEntity* pKiller, CBaseEntity* pInflictor);
	CBaseEntity* GetRecentDamager(CBaseEntity* pVictim, int iDamager, float flMaxElapsed);

	virtual CBaseEntity* GetPlayerSpawnSpot(CBasePlayer* pPlayer);
	bool	IsPlayerRespawningByHandler(CBasePlayer* pPlayer);

	bool	NPC_ShouldDropGrenade(CBasePlayer* pRecipient);
	bool	NPC_ShouldDropHealth(CBasePlayer* pRecipient);
	void	NPC_DroppedHealth(void);
	void	NPC_DroppedGrenade(void);

	ResponseRules::IResponseSystem* GetPlayerResponseSystem() { return m_pPlayerResponseSystem; }
protected:
	void AdjustPlayerDamageTaken(CTakeDamageInfo* pInfo);
	float AdjustPlayerDamageInflicted(float damage);

	virtual void RespawnTeam(int iTeam);
private:

	float	m_flLastHealthDropTime;
	float	m_flLastGrenadeDropTime;

	ResponseRules::IResponseSystem* m_pPlayerResponseSystem;
	
	typedef CUtlLinkedList<ITeamRespawnWaveHandler*> TeamHandlerList_t;
	TeamHandlerList_t m_TeamSpawnHandlers[TF_TEAM_COUNT];
#endif
public:

	const char* GetGameConfigName();

private:
	// Rules change for the mega physgun
	CNetworkVar(bool, m_bMegaPhysgun);
	CNetworkVar(int, m_nGameMode);
	CNetworkVar(int, m_nGameModeVariant);
	CNetworkVar(int, m_iMapGameType);
	CNetworkVar(int, m_iMapModType);
#ifndef CLIENT_DLL
	CNetworkVar(string_t, m_iszGameConfig);
#else
	char	m_szGameConfig[64];
#endif // !CLIENT_DLL

};

inline CLazuul* LazuulRules()
{
	return assert_cast<CLazuul*> (g_pGameRules);
}

#endif
