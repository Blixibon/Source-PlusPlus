#ifndef SCHED_OBJ_H
#define SCHED_OBJ_H

#pragma once

class CSquadSlot
{
public:
	CSquadSlot(CAI_BaseNPC *pHost, int iSlot);
	CSquadSlot(CAI_BaseNPC *pHost, int iSlot, int iSlotEnd);
	CSquadSlot()
	{
		m_pAI = nullptr;
	}

	bool IsAvailible();
	bool Occupy();
protected:
	CAI_BaseNPC *m_pAI;
	int m_iSlotStart, m_iSlotEnd;
};

#define SQUAD_SLOT(slot) CSquadSlot(this, slot)
#define SQUAD_SLOT_RANGE(slot, slot2) CSquadSlot(this, slot, slot2)

class CBaseScheduleObject
{
public:
	bool m_bCondition;

	virtual int GetSchedule() = 0;
	virtual float GetPriority() = 0;
	virtual bool GetCondition() { return m_bCondition; }
	virtual CSquadSlot *GetSlot() = 0;
};

class CScheduleObject : public CBaseScheduleObject
{
public:
	int m_iSchedType;
	CSquadSlot m_Slot;
	float m_fSchedPriority;

	// Inherited via CBaseScheduleObject
	virtual int GetSchedule() override;
	virtual float GetPriority() override;
	virtual CSquadSlot *GetSlot() override
	{
		return &m_Slot;
	}
	virtual bool GetCondition() override
	{
		if (!m_Slot.IsAvailible())
			return false;

		return m_bCondition;
	}
};

class CScheduleGoalObject : public CBaseScheduleObject
{
public:
	CScheduleGoalObject(bool bCond, CBaseScheduleObject **pArray, int count, float flPRMod)
	{
		GoalArray = pArray;
		ArraySize = count;
		m_bCondition = bCond;
		m_iCachedSched = -1;
		m_fSchedPriority = 0.f;
		m_flPriorityMod = flPRMod;
	}

protected:
	CBaseScheduleObject **GoalArray;
	int ArraySize;
	float m_flPriorityMod;

	int m_iCachedSched;
	float m_fSchedPriority;
	CSquadSlot *m_pSlot;

	void	CacheResult();
public:
	// Inherited via CBaseScheduleObject
	virtual int GetSchedule() override;
	virtual float GetPriority() override;
	virtual CSquadSlot *GetSlot() override;
};

namespace ScheduleHelper
{
	int		SelectScheduleObject(CBaseScheduleObject **pArray, int count, CSquadSlot **ppSlot, float *pflPriority = nullptr);
}

#endif