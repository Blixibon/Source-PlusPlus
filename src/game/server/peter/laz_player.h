#ifndef HLMS_PLAYER_H
#define HLMS_PLAYER_H
#include "cbase.h"
#include "hl2_player.h"
#include "portalbase/portal_player.h"
#include "laz_player_shared.h"
#include "player_models.h"

//#define MIN_FLING_SPEED 290

class CLaz_Player;

#define DEFAULT_VOICE "none"
#define DEFAULT_FEET "none"

extern ConVar	spec_freeze_time;
extern ConVar	spec_freeze_traveltime;

enum LazSpecialAttack_e
{
	LAZ_SPECIAL_NONE = -1,
	LAZ_SPECIAL_MANHACK = 0,
	LAZ_SPECIAL_OLIVIA,

	SPECIAL_ATTACK_COUNT
};

extern INetworkStringTable *g_pStringTablePlayerFootSteps;

enum LazSpecialtyStatus_e
{
	LAZ_STATUS_PLAYER = 0,
	LAZ_STATUS_ADMIN,
	LAZ_STATUS_DEVELOPER,
};

enum LazPlayerPermissions_e
{
	LAZ_PERM_VOICE_BROADCAST = (1 << 0),
	LAZ_PERM_NOCLIP = (1 << 1),
};

//=============================================================================
// >> HL2MP_Player
//=============================================================================
class CLAZPlayerStateInfo
{
public:
	LAZPlayerState m_iPlayerState;
	const char *m_pStateName;

	void (CLaz_Player::*pfnEnterState)();	// Init and deinit the state.
	void (CLaz_Player::*pfnLeaveState)();

	void (CLaz_Player::*pfnPreThink)();	// Do a PreThink() in this state.
};

class CLaz_Player : public CPortal_Player
{
public:
	DECLARE_CLASS(CLaz_Player, CPortal_Player);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CLaz_Player();

	virtual void Precache(void);
	virtual void ForceRespawn(void);
	//virtual void CreateSounds(void);
	//virtual void StopLoopingSounds(void);
	virtual void Spawn(void);
	virtual void			InitialSpawn(void);
	virtual void UpdateOnRemove();
	virtual CBaseEntity		*EntSelectSpawnPoint(void);

	virtual void DoMuzzleFlash();

	LazSpecialtyStatus_e GetPlayerPrivilegeLevel();
	int					GetPlayerPermissions();

	virtual bool	IsReadyToPlay(void)
	{
		if (m_iPlayerState == STATE_WELCOME)
			return false;

		return (GetTeamNumber() > LAST_SHARED_TEAM);
	}

	virtual float		PlayScene(const char* pszScene, float flDelay = 0.0f, AI_Response * response = NULL, IRecipientFilter * filter = NULL);

	//virtual void PostThink(void);
	virtual void PreThink(void);

	virtual bool HandleCommand_JoinTeam(int team);

	void SetPlayerModel(void);

	Class_T Classify();

	CEconEntity* GiveItemById(int iItem);

	//void UpdateWooshSounds(void);

	virtual void ChangeTeam(int iTeam);

	bool ClientCommand(const CCommand &args);

	//virtual int			OnTakeDamage(const CTakeDamageInfo &inputInfo);

	void PlayFlinch(const CTakeDamageInfo &info);
	virtual void			TraceAttack(const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator);
	void PainSound(const CTakeDamageInfo &info);
	virtual void			DeathSound(const CTakeDamageInfo &info);

	virtual	bool			TestHitboxes(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);

	virtual bool ShouldGib(const CTakeDamageInfo & info);

	virtual	void		Event_KilledOther(CBaseEntity *pVictim, const CTakeDamageInfo &info);

	void			ModifyOrAppendCriteria(AI_CriteriaSet& set);

	virtual void	DeathNotice(CBaseEntity *pVictim);

	virtual int	OnTakeDamage(const CTakeDamageInfo &inputInfo);
	virtual void Event_Killed(const CTakeDamageInfo &info);

	void		HandleAnimEvent(animevent_t *pEvent);

	//virtual void			PlayerDeathThink(void);

	void		SetVoiceType(const char *pchVoice, const char *pchSuit)
	{
		m_iszVoiceType = AllocPooledString(pchVoice);
		m_iszSuitVoice = AllocPooledString(pchSuit);
	}

	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;

	void		SetFootsteps(const char *);

	virtual void PlayStepSound(const Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force);
	void	PrecacheFootStepSounds(void);
	const char *GetPlayerModelSoundPrefix(void);

	virtual void		PlayerDeathThink(void);

	//virtual CStudioHdr *OnNewModel(void);

	bool				ChooseEnemy();

	// Starts a special attack.
	void					Special();

	CBaseEntity*		GetEnemy() { return m_hEnemy.Get(); }
	CBaseEntity*		GetEnemy() const { return m_hEnemy.Get(); }

	virtual void			CheckSuitUpdate();
	virtual void			SetSuitUpdate(const char *name, int fgroup, int iNoRepeat);

	virtual void SetAnimation(PLAYER_ANIM playerAnim);

	void State_Transition(LAZPlayerState newState);
	void State_Enter(LAZPlayerState newState);
	void State_Leave();
	void State_PreThink();
	CLAZPlayerStateInfo *State_LookupInfo(LAZPlayerState state);

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();
	void				StateEnterWELCOME(void);
	void				StateThinkWELCOME(void);
	void				StateEnterDYING(void);
	void				StateThinkDYING(void);

	int					BuildObservableEntityList(void);
	virtual int			GetNextObserverSearchStartPoint(bool bReverse); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual CBaseEntity *FindNextObserverTarget(bool bReverse);
	virtual bool		IsValidObserverTarget(CBaseEntity * target); // true, if player is allowed to see this target
	virtual bool		SetObserverTarget(CBaseEntity * target);
	virtual bool		ModeWantsSpectatorGUI(int iMode) { return (iMode != OBS_MODE_FREEZECAM && iMode != OBS_MODE_DEATHCAM); }
	void				FindInitialObserverTarget(void);
	CBaseEntity		    *FindNearestObservableTarget(Vector vecOrigin, float flMaxDist);
	virtual void		ValidateCurrentObserverTarget(void);
	virtual void			CheckObserverSettings(); // checks, if target still valid (didn't die etc)
	virtual bool		SetObserverMode(int mode);

	bool				GetResponseSceneFromConcept(int iConcept, char* chSceneBuffer, int numSceneBufferBytes);

	void	UpdateExpression(void);
	void	ClearExpression(void);

	void			InputAnswerQuestion(inputdata_t& inputdata);
	void			AnswerQuestion(CBaseCombatCharacter* pQuestioner, int iQARandomNum, bool bAnsweringHello);

	bool		HasMPModel()
	{
		return m_MPModel.szSectionID[0] != '\0';
	}

	const playerModel_t* GetMPModel()
	{
		if (!HasMPModel())
			return nullptr;

		return &m_MPModel;
	}

public:
	static EHANDLE gm_hLastRandomSpawn;
protected:
	//CSoundPatch		*m_pWooshSound;

	CNetworkVar(int, m_iPlayerSoundType);

	float				m_flNextPainSoundTime;

	string_t			m_iszVoiceType;
	string_t			m_iszSuitVoice;

	LAZPlayerState m_iPlayerState;
	CLAZPlayerStateInfo *m_pCurStateInfo;

	CSimpleSimTimer m_AnnounceAttackTimer;
	EHANDLE			m_hEnemy;

	// Anim event handlers
	void OnAnimEventDeployManhack(animevent_t *pEvent);
	void OnAnimEventStartDeployManhack(void);
	void		ReleaseManhack(void);

	bool ShouldRunRateLimitedCommand(const CCommand &args);

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float, int>	m_RateLimitLastCommandTimes;
	CUtlVector<EHANDLE>	m_hObservableEntities;

	int					GetAutoTeam(void);

	void				SetMinion(CBaseCombatCharacter *pMinion);

	void				ClearMinion()
	{
		m_hMinion.Set(NULL);
		V_memset(m_strMinionClass.GetForModify(), '\0', 32);
	}

	playerModel_t m_MPModel;
	int				m_nSpecialAttack;
	float				m_flNextSpecialAttackTime;
	bool				m_bPlayedFreezeCamSound;

	// Background expressions
	string_t			m_iszExpressionScene;
	EHANDLE				m_hExpressionSceneEnt;
	float				m_flNextRandomExpressionTime;

	int					m_iDesiredPermissions;

	CNetworkHandle(CBaseCombatCharacter, m_hMinion);
	CNetworkString(m_strMinionClass, 32);
public:
	CNetworkVar(bool, m_bHasLongJump);
};

inline CLaz_Player *ToLazuulPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<CLaz_Player*>(pEntity);
}

#endif //HLMS_PLAYER_H
