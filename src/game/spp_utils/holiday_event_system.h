#ifndef HOLIDAY_EVENTS_H
#define HOLIDAY_EVENTS_H
#pragma once
#include "holiday_events.h"
#include "utlmap.h"
#include "utlvector.h"
#include <time.h>
#include "utlsymbol.h"
#include "KeyValues.h"

DECLARE_PRIVATE_SYMBOLTYPE(CHolidaySymbol);

class CHolidayEventSystem;

class CScriptOperator
{
public:
	virtual void Release() { delete this; }

	virtual bool IsActive(const time_t& timeCurrent) = 0;
	// Calculate the next time range we are active.
	virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd) = 0;
};

class CScriptedHoliday : public IHoliday
{
public:
	CScriptedHoliday(const char* pszName) : m_bCalcdTimes(false)
	{
		m_Name = pszName;
	}

	~CScriptedHoliday();

	virtual const char* GetHolidayName() const { return m_Name.String(); }
	virtual bool IsActive(const time_t& timeCurrent);
	virtual time_t GetStartTime();
	virtual time_t GetEndTime();

	// Calculate the next time range we are active.
	void	CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);
protected:
	friend class CHolidayEventSystem;

	void	CalcTimesForCache();

	CHolidaySymbol m_Name;
	CUtlVector<CScriptOperator*> m_Operators;
	time_t m_timeStart, m_timeEnd;
	bool m_bCalcdTimes;
};

class CHolidayEventSystem : public IHolidayEvents
{
public:
	CHolidayEventSystem();

	virtual bool IsEventActive(int iEvent);
	virtual int NumEvents() { return m_Holidays.Count(); }
	//Lookup an event by name
	// Returns -1 if no event
	virtual int LookupEvent(const char* pchEventName);
	virtual IHoliday* GetEvent(int iEvent)
	{
		if (iEvent >= NumEvents() || iEvent < 0)
			return nullptr;
		return m_Holidays.Element(iEvent);
	}

	virtual int GetActiveEvents(IHoliday** ppEvents, int iMaxEvents);

	bool Init();
	void Shutdown();

	bool IsEventActiveAtTime(int iEvent, const time_t& curTime);
	bool IsEventActiveAtTime(CHolidaySymbol sEvent, const time_t& curTime);
	int LookupEvent(CHolidaySymbol sEventName);

	CScriptOperator* ParseScriptOperator(KeyValues* pKV, const char *pszEventName);

	void	GetActiveEventNames(CUtlVector<const char*>& vEvents);

protected:
	CUtlMap<CHolidaySymbol, CScriptedHoliday*> m_Holidays;
};

#endif // !HOLIDAY_EVENTS_H
