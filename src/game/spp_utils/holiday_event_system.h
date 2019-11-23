#ifndef HOLIDAY_EVENTS_H
#define HOLIDAY_EVENTS_H
#pragma once
#include "holiday_events.h"
#include "utlvector.h"
#include <time.h>

class CHolidayEventSystem : public IHolidayEvents
{
public:

	virtual bool IsEventActive(int iEvent);
	virtual int NumEvents() { return m_vecHolidays.Count(); }
	//Lookup an event by name
	// Returns -1 if no event
	virtual int LookupEvent(const char* pchEventName);
	virtual const holiday_t* GetEvent(int iEvent)
	{
		if (iEvent >= NumEvents() || iEvent < 0)
			return nullptr;
		return m_vecHolidays.Element(iEvent);
	}

	virtual int GetActiveEvents(const holiday_t** ppEvents, int iMaxEvents);

	bool Init();
	void Shutdown();

	bool IsEventActiveAtTime(int iEvent, const tm* curTime);

protected:
	CUtlVector<holiday_t*> m_vecHolidays;
};

#endif // !HOLIDAY_EVENTS_H
