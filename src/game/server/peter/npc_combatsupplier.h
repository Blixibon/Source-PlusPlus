#ifndef NPC_COMBATSUPPLIER_H
#define NPC_COMBATSUPPLIER_H
#pragma once

#include "ai_basenpc.h"
#include "npc_playerfollower.h"

class CNPC_CombatSupplier : public CNPC_PlayerFollower
{
	DECLARE_CLASS(CNPC_CombatSupplier, CNPC_PlayerFollower);
public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	virtual void	Precache();
	virtual void	Spawn();

	virtual bool	IsAmmoResupplier() { return false; }

	void 			StartTask(const Task_t* pTask);
	void 			RunTask(const Task_t* pTask);
	void 			HandleAnimEvent(animevent_t* pEvent);
	void			TaskFail(AI_TaskFailureCode_t code);

	virtual void			GatherHealConditions();
	virtual void 			GatherConditions();

	virtual bool 			CanHeal();
	virtual bool 			ShouldHealTarget(CBaseEntity* pTarget, bool bActiveUse = false);
#if HL2_EPISODIC
	virtual bool 			ShouldHealTossTarget(CBaseEntity* pTarget, bool bActiveUse = false);
#endif
	virtual void 			Heal();

#if HL2_EPISODIC
	void			TossHealthKit(CBaseCombatCharacter* pThrowAt, const Vector& offset); // create a healthkit and throw it at someone
	void			InputForceHealthKitToss(inputdata_t& inputdata);
#endif

	void			PredictPlayerPush();
	void			BuildScheduleTestBits();
	virtual int 	SelectScheduleHeal();
	virtual int 	SelectSchedulePriorityAction();

	enum
	{
		COND_SUPPLIER_PLAYERHEALREQUEST = BaseClass::NEXT_CONDITION,
		COND_SUPPLIER_COMMANDHEAL,
		NEXT_CONDITION,

		SCHED_SUPPLIER_HEAL = BaseClass::NEXT_SCHEDULE,
#ifdef HL2_EPISODIC
		SCHED_SUPPLIER_HEAL_TOSS,
#endif
		NEXT_SCHEDULE,

		TASK_SUPPLIER_HEAL = BaseClass::NEXT_TASK,
#ifdef HL2_EPISODIC
		TASK_SUPPLIER_HEAL_TOSS,
#endif
		NEXT_TASK,
	};

protected:

	float			m_flPlayerHealTime;
	float			m_flAllyHealTime;
	float			m_flPlayerGiveAmmoTime;
	string_t		m_iszAmmoSupply;
	int				m_iAmmoAmount;

	float			m_flTimePlayerStare;	// The game time at which the player started staring at me.
	float			m_flTimeNextHealStare;	// Next time I'm allowed to heal a player who is staring at me.

	static string_t	gm_iszCurrentAmmo;
	static string_t gm_iszAutoAmmo;
};

#endif // !NPC_COMBATSUPPLIER_H
