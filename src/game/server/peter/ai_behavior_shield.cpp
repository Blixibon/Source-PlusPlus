#include "cbase.h"
#include "ienergyshield.h"
#include "ai_behavior_shield.h"

Activity ACT_SHIELD_IDLE;

void CAI_ShieldingBehavior::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_WAIT_SHIELDING:
	{
		ChainStartTask(TASK_FACE_ENEMY);
		break;
	}
	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

void CAI_ShieldingBehavior::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_WAIT_SHIELDING:
	{
		ChainRunTask(TASK_FACE_ENEMY);
		break;
	}
	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_ShieldingBehavior)
{
	DECLARE_ACTIVITY(ACT_SHIELD_IDLE)

	DECLARE_TASK(TASK_START_SHIELDING)
	DECLARE_TASK(TASK_FIND_SHIELD_POSITION)
	DECLARE_TASK(TASK_WAIT_SHIELDING)

	DECLARE_CONDITION(COND_ENEMY_BEHIND)
	DECLARE_CONDITION(COND_SQUAD_DEAD)
	
	AI_END_CUSTOM_SCHEDULE_PROVIDER()
}