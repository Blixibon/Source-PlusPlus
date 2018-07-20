#ifndef NPC_CORE_H
#define NPC_CORE_H

#include "ai_playerally.h"
#include "player_pickup.h"

//npc_personality_core

enum
{
	ANIM_NORMAL = 0,
	ANIM_PLUG,
	ANIM_FRONT,

	MAX_ANIMSTATES
};

class CNPC_Core : public CAI_PlayerAlly, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS(CNPC_Core, CAI_PlayerAlly);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DEFINE_CUSTOM_AI;

	CNPC_Core()
	{
	}

	void Spawn();
	void Precache();

	virtual void	OnRestore();

	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void StartTask(const Task_t *pTask);
	void	PrescheduleThink(void);
	void	GatherConditions(void);
	void	BuildScheduleTestBits(void);
	int		SelectSchedule(void);
	virtual void		RunTask(const Task_t *pTask);

	void					UpdateHeadControl(const Vector &vHeadTarget, float flHeadInfluence);

	int		SelectWeightedSequence(Activity activity);
	int		SelectWeightedSequence(Activity activity, int curSequence);
	int		SelectHeaviestSequence(Activity activity);

	virtual bool			HasPreferredCarryAnglesForPlayer(CBasePlayer *pPlayer) { return true; }
	virtual QAngle			PreferredCarryAngles(void) { return QAngle(0, 90, 0); }

	virtual bool			OnAttemptPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON) { return (reason == PUNTED_BY_CANNON || !m_bPhysShadow); }
	virtual void			OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON);
	virtual void			OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t reason);

	Class_T Classify() { return CLASS_PLAYER_ALLY_VITAL; }

	virtual bool 		CreateVPhysics();
	bool	ShouldSavePhysics() { return true; }

	virtual bool			CanBecomeServerRagdoll(void) { return false; }

	virtual void	PlayerPenetratingVPhysics(void);
	void		ModifyOrAppendCriteria(AI_CriteriaSet& set);
	void		ModifyEmitSoundParams(EmitSound_t &params);

	void		InputSetIdleSequence(inputdata_t &inputdata);
	void		InputClearIdleSequence(inputdata_t &inputdata);

	void		InputEnableFlashlight(inputdata_t &inputdata);
	void		InputDisableFlashlight(inputdata_t &inputdata);
	void		InputToggleFlashlight(inputdata_t &inputdata);

	enum
	{
		ANIM_NORMAL = 0,
		ANIM_PLUG,
		ANIM_FRONT,

		MAX_ANIMSTATES
	};

	enum
	{
		TASK_CORE_TRANSITION = BaseClass::NEXT_TASK,
		NEXT_TASK,

		SCHED_CORE_TRANSITION = BaseClass::NEXT_SCHEDULE,
		NEXT_SCHEDULE,

		COND_CORE_ANIM_CHANGED = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
	};


protected:
	void	UpdateIdleAnimation();


	bool m_bUseAltModel;
	int m_iModelSkin;

	//int m_iLookLayer;

	string_t m_iszIdealIdleAnim;
	string_t m_iszIdleAnim;
	
	bool m_bPhysShadow;

	//bool m_bFlippedToFront;

	CNetworkVar(bool, m_bFlashlightOn);

	int m_iAnimState;
	int m_iDesiredAnimState;
};



#endif // !NPC_CORE_H

