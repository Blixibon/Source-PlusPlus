#ifndef PLAYERFOLLOWER_H
#define PLAYERFOLLOWER_H

#ifdef _WIN32
#pragma once  
#endif // _WIN32

#include "npc_playercompanion.h"

#define COMMAND_POINT_CLASSNAME "info_target_command_point"

#define MAX_PLAYER_SQUAD npc_player_squad_size.GetInt()
extern ConVar npc_player_squad_size;

struct BMSSquadMemberInfo_t
{
	CAI_PlayerAlly *	pMember;
	bool			bSeesPlayer;
	float			distSq;
};

int __cdecl BMSSquadSortFunc(const BMSSquadMemberInfo_t *pLeft, const BMSSquadMemberInfo_t *pRight);

#define SetFollowerBaseUse( a ) SetBaseUse(static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a))

struct FollowerSquadCandidate_t;

class CNPC_PlayerFollower : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS(CNPC_PlayerFollower, CNPC_PlayerCompanion);
	DECLARE_DATADESC();

	virtual bool	ShouldAutosquad() { return false; }

	void 			PrescheduleThink();
	void			PostNPCInit();

	//virtual bool	CanJoinPlayerSquad() { return CanJoinPlayerSquad(nullptr); }
	virtual bool	CanJoinPlayerSquad(CBasePlayer *pPlayer = nullptr);
	void			AddToPlayerSquad(CBasePlayer *);
	void			RemoveFromPlayerSquad();
	void 			TogglePlayerSquadState(CBasePlayer *);
	virtual string_t GetPlayerSquadName() const;
	bool IsInThisPlayerSquad(CBasePlayer *pPlayer) const;

	virtual CBasePlayer *GetBestPlayer();	

	virtual CBasePlayer *GetBestPlayer() const
	{
		return const_cast<CNPC_PlayerFollower *>(this)->GetBestPlayer();
	}

	void			BuildScheduleTestBits();

	int 			SelectSchedulePriorityAction();
	int 			SelectScheduleRetrieveItem();

	void 			FixupPlayerSquad();
	void 			ClearFollowTarget();
	bool			WasInPlayerSquad() { return m_bWasInPlayerSquad; }
	void 			UpdateFollowCommandPoint();
	bool			IsFollowingCommandPoint();
	bool 			TargetOrder(CBaseEntity *pTarget, CAI_BaseNPC **Allies, int numAllies);
	void			SetSquad(CAI_Squad *pSquad);

	bool 			ShouldAlwaysThink();

	void			UpdatePlayerSquad();
	static int __cdecl PlayerSquadCandidateSortFunc(const FollowerSquadCandidate_t *, const FollowerSquadCandidate_t *);

	void			OnRestore();

	void 			CommanderUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void			SetBaseUse(USEPTR func) { m_pfnBaseUse = func; }

	bool			SpeakCommandResponse(AIConcept_t concept, const char *modifiers = NULL);

	virtual CAI_BaseNPC *	GetSquadCommandRepresentative();
	virtual bool IsCommandable() { return IsInPlayerSquad(); }

	bool HaveCommandGoal() const
	{
		if (GetCommandGoal() != vec3_invalid)
			return true;
		return false;
	}

	bool			IsCommandMoving();
	bool			ShouldAutoSummon();

	bool			ShouldAcceptGoal(CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal);
	void			OnClearGoal(CAI_BehaviorBase *pBehavior, CAI_GoalEntity *pGoal);

	void 			MoveOrder(const Vector &vecDest, CAI_BaseNPC **Allies, int numAllies);
	void			OnMoveOrder();

protected:
	CHandle<CAI_FollowGoal>	m_hSavedFollowGoalEnt;
	string_t		m_iszOriginalSquad;
	float			m_flTimeJoinedPlayerSquad;
	bool			m_bWasInPlayerSquad;
	float			m_flTimeLastCloseToPlayer;
	string_t		m_iszDenyCommandConcept;

	CSimpleSimTimer	m_AutoSummonTimer;
	Vector			m_vAutoSummonAnchor;

	static CSimpleSimTimer gm_PlayerSquadEvaluateTimer;

	//bool					m_bNotifyNavFailBlocked;
	bool					m_bNeverLeavePlayerSquad; // Don't leave the player squad unless killed, or removed via Entity I/O. 

	USEPTR			m_pfnBaseUse;

	//-----------------------------------------------------
	//	Outputs
	//-----------------------------------------------------
	COutputEvent		m_OnJoinedPlayerSquad;
	COutputEvent		m_OnLeftPlayerSquad;
	COutputEvent		m_OnFollowOrder;
	COutputEvent		m_OnStationOrder;
	COutputEvent		m_OnPlayerUse;
	//COutputEvent		m_OnNavFailBlocked;

	AIHANDLE m_hCurrentSquadRep;
	float m_flLastSquadRepTime;

protected:
	bool			m_bAutoSquadPreventDoubleAdd;
};


#endif // !PLAYERFOLLOWER_H
