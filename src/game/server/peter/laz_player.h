#ifndef HLMS_PLAYER_H
#define HLMS_PLAYER_H
#include "cbase.h"
#include "hl2_player.h"
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

	SPECIAL_ATTACK_COUNT
};

extern INetworkStringTable *g_pStringTablePlayerFootSteps;

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

class CLaz_Player : public CHL2_Player
{
public:
	DECLARE_CLASS(CLaz_Player, CHL2_Player);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CLaz_Player();

	virtual void Precache(void);
	//virtual void CreateSounds(void);
	//virtual void StopLoopingSounds(void);
	virtual void Spawn(void);
	virtual void UpdateOnRemove();

	//virtual void PostThink(void);
	virtual void PreThink(void);

	virtual bool HandleCommand_JoinTeam(int team);

	void SetPlayerModel(void);

	Class_T Classify();

	//void UpdateWooshSounds(void);

	virtual void ChangeTeam(int iTeam);

	bool ClientCommand(const CCommand &args);

	//virtual int			OnTakeDamage(const CTakeDamageInfo &inputInfo);

	void PlayFlinch(const CTakeDamageInfo &info);

	void PainSound(const CTakeDamageInfo &info);
	//void DeathSound(const CTakeDamageInfo &info);

	virtual	bool			TestHitboxes(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);

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

	void		SetFootsteps(const char *);

	virtual void PlayStepSound(Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force);
	void	PrecacheFootStepSounds(void);
	const char *GetPlayerModelSoundPrefix(void);


	//virtual CStudioHdr *OnNewModel(void);

	// Starts a special attack.
	void					Special();

	virtual void			CheckSuitUpdate();
	virtual void			SetSuitUpdate(const char *name, int fgroup, int iNoRepeat);

	void State_Transition(LAZPlayerState newState);
	void State_Enter(LAZPlayerState newState);
	void State_Leave();
	void State_PreThink();
	CLAZPlayerStateInfo *State_LookupInfo(LAZPlayerState state);

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	virtual bool		SetObserverMode(int mode);

protected:
	//CSoundPatch		*m_pWooshSound;

	CNetworkVar(int, m_iPlayerSoundType);

	float				m_flNextPainSoundTime;

	string_t			m_iszVoiceType;
	string_t			m_iszSuitVoice;

	LAZPlayerState m_iPlayerState;
	CLAZPlayerStateInfo *m_pCurStateInfo;

	// Anim event handlers
	void OnAnimEventDeployManhack(animevent_t *pEvent);
	void OnAnimEventStartDeployManhack(void);
	void		ReleaseManhack(void);

	bool ShouldRunRateLimitedCommand(const CCommand &args);

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float, int>	m_RateLimitLastCommandTimes;

	int					GetAutoTeam(void);

	void				SetMinion(CBaseCombatCharacter *pMinion);

	void				ClearMinion()
	{
		m_hMinion.Set(NULL);
		V_memset(m_strMinionClass.GetForModify(), '\0', 32);
	}

	bool		HasMPModel()
	{
		return m_MPModel.szSectionID[0] != '\0';
	}

	playerModel_t m_MPModel;
	int				m_nSpecialAttack;
	float				m_flNextSpecialAttackTime;

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
