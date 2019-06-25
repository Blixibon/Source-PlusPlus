#include "cbase.h"
#include "ai_basenpc.h"
#include "ScheduleObject.h"

int CScheduleObject::GetSchedule()
{
	return m_iSchedType;
}

float CScheduleObject::GetPriority()
{
	return m_fSchedPriority;
}

void CScheduleGoalObject::CacheResult()
{
	if (m_iCachedSched >= 0)
		return;

	m_iCachedSched = ScheduleHelper::SelectScheduleObject(GoalArray, ArraySize, &m_pSlot, &m_fSchedPriority);
}

int CScheduleGoalObject::GetSchedule()
{
	CacheResult();

	if (m_iCachedSched < 0)
		return 0;

	return m_iCachedSched;
}

float CScheduleGoalObject::GetPriority()
{
	CacheResult();

	if (m_iCachedSched < 0)
		return 0.0f;

	return m_fSchedPriority * m_flPriorityMod;
}

CSquadSlot * CScheduleGoalObject::GetSlot()
{
	CacheResult();

	if (m_iCachedSched < 0)
		return nullptr;

	return m_pSlot;
}

int ScheduleHelper::SelectScheduleObject(CBaseScheduleObject **pArray, int count, CSquadSlot **ppSlot, float *pflPriority)
{
	int FinalSched = 0;
	float SchedPriority = 0.f;
	CSquadSlot *pSlot = nullptr;
	for (int v = 0; v < count; v++)
	{
		CBaseScheduleObject *GoalSched = pArray[v];
		if (GoalSched->GetCondition() && GoalSched->GetPriority() > SchedPriority)
		{
			SchedPriority = GoalSched->GetPriority();
			FinalSched = GoalSched->GetSchedule();
			pSlot = GoalSched->GetSlot();
		}
	}

	if (pflPriority)
	{
		*pflPriority = SchedPriority;
	}

	if (ppSlot)
	{
		*ppSlot = pSlot;
	}

	return FinalSched;
}

CSquadSlot::CSquadSlot(CAI_BaseNPC * pHost, int iSlot)
{
	m_pAI = pHost;
	m_iSlotEnd = m_iSlotStart = iSlot;
}

CSquadSlot::CSquadSlot(CAI_BaseNPC * pHost, int iSlot, int iSlotEnd)
{
	m_pAI = pHost;
	m_iSlotStart = iSlot;
	m_iSlotEnd = iSlotEnd;
}

bool CSquadSlot::IsAvailible()
{
	if (!m_pAI)
		return true;

	return (!m_pAI->IsStrategySlotRangeOccupied(m_iSlotStart, m_iSlotEnd) || m_pAI->HasStrategySlotRange(m_iSlotStart, m_iSlotEnd));
}

bool CSquadSlot::Occupy()
{
	if (!m_pAI)
		return true;

	return m_pAI->OccupyStrategySlotRange(m_iSlotStart, m_iSlotEnd);
}
