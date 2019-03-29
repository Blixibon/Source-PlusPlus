#ifndef AI_BEHAVIOR_SHIELD_H
#define AI_BEHAVIOR_SHIELD_H
#pragma once

#include "ai_behavior.h"

class CAI_ShieldingBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS(CAI_ShieldingBehavior, CAI_SimpleBehavior);
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
	DECLARE_DATADESC();
public:

	// Tasks
	void		StartTask(const Task_t *pTask);
	void		RunTask(const Task_t *pTask);

	enum
	{
		// Schedules
		SCHED_PLANT_SHIELD = BaseClass::NEXT_SCHEDULE,
		NEXT_SCHEDULE,

		// Tasks
		TASK_FIND_SHIELD_POSITION = BaseClass::NEXT_TASK,
		TASK_START_SHIELDING,
		TASK_WAIT_SHIELDING,
		NEXT_TASK,

		// Conditions
		COND_ENEMY_BEHIND = BaseClass::NEXT_CONDITION,
		COND_SQUAD_DEAD,
		NEXT_CONDITION
	};
};

#endif
