#ifndef IHOLIDAY_EVENTS_H
#define IHOLIDAY_EVENTS_H
#pragma once

typedef struct
{
	char chName[32];
	int tm_mon_start;
	int tm_mday_start;

	int tm_mon_end;
	int tm_mday_end;

	bool bWrapsYear;
} holiday_t;

class IHolidayEvents
{
public:
	virtual bool IsEventActive(int iEvent) = 0;
	virtual int NumEvents() = 0;
	//Lookup an event by name
	// Returns -1 if no event
	virtual int LookupEvent(const char* pchEventName) = 0;
	virtual const holiday_t* GetEvent(int iEvent) = 0;
	virtual int GetActiveEvents(const holiday_t** ppEvents, int iMaxEvents) = 0;
};
#endif // !HOLIDAY_EVENTS_H
