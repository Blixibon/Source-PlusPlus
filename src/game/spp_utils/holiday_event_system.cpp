#include "holiday_event_system.h"
#include "KeyValues.h"
#include "filesystem.h"

bool CHolidayEventSystem::IsEventActive(int iEvent)
{
	time_t ltime = time(0);
	struct tm* today = localtime(&ltime);

	return IsEventActiveAtTime(iEvent, today);
}

int CHolidayEventSystem::LookupEvent(const char* pchEventName)
{
	int iRet = -1;
	for (int i = 0; i < m_vecHolidays.Count(); i++)
	{
		const holiday_t* pHoliday = m_vecHolidays[i];
		if (0 == Q_strcmp(pchEventName, pHoliday->chName))
		{
			iRet = i;
		}
	}
	return iRet;
}

int CHolidayEventSystem::GetActiveEvents(const holiday_t** ppEvents, int iMaxEvents)
{
	time_t ltime = time(0);
	struct tm* today = localtime(&ltime);
	int iNumActive = 0;

	for (int i = 0; i < NumEvents() && iNumActive < iMaxEvents; i++)
	{
		if (IsEventActiveAtTime(i, today))
		{
			ppEvents[iNumActive] = GetEvent(i);
			iNumActive++;
		}
	}

	return iNumActive;
}

bool CHolidayEventSystem::Init()
{
	KeyValues* pKV = new KeyValues("Holidays");

	if (pKV->LoadFromFile(g_pFullFileSystem, "scripts/holiday_events.txt", "MOD"))
	{
		for (KeyValues* pHoliday = pKV->GetFirstTrueSubKey(); pHoliday != NULL; pHoliday = pHoliday->GetNextTrueSubKey())
		{
			holiday_t* pEvent = new holiday_t;
			Q_strncpy(pEvent->chName, pHoliday->GetName(), sizeof(pEvent->chName));

			pEvent->tm_mon_start = pHoliday->GetInt("start_month", 1) - 1;
			pEvent->tm_mday_start = pHoliday->GetInt("start_day", 1);

			pEvent->tm_mon_end = pHoliday->GetInt("end_month", 12) - 1;
			pEvent->tm_mday_end = pHoliday->GetInt("end_day", 31);

			pEvent->bWrapsYear = (pEvent->tm_mon_start > pEvent->tm_mon_end);

			m_vecHolidays.AddToTail(pEvent);
		}
	}

	pKV->deleteThis();

	return true;
}

void CHolidayEventSystem::Shutdown()
{
	m_vecHolidays.PurgeAndDeleteElements();
}

bool CHolidayEventSystem::IsEventActiveAtTime(int iEvent, const tm* curTime)
{
	const holiday_t* pEvent = GetEvent(iEvent);
	if (!pEvent || !curTime)
		return false;

	int iDay = curTime->tm_mday;
	int iMonth = curTime->tm_mon;

	if ((iMonth == pEvent->tm_mon_start && iDay >= pEvent->tm_mday_start) || iMonth > pEvent->tm_mon_start || (pEvent->bWrapsYear && iMonth <= pEvent->tm_mon_end))
	{
		if ((iMonth == pEvent->tm_mon_end && iDay <= pEvent->tm_mday_end) || iMonth < pEvent->tm_mon_end)
		{
			return true;
		}
	}

	return false;
}
