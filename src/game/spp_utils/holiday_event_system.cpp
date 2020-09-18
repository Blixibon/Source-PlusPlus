#include "holiday_event_system.h"
#include "KeyValues.h"
#include "filesystem.h"

IMPLEMENT_PRIVATE_SYMBOLTYPE(CHolidaySymbol);

namespace HolidaySymbol
{
	bool SymbolLessFunc(const CHolidaySymbol& a, const CHolidaySymbol& b)
	{
		return CaselessStringLessThan(a.String(), b.String());
	}
}

CScriptedHoliday::~CScriptedHoliday()
{
	for (auto pOperator : m_Operators)
		pOperator->Release();

	m_Operators.Purge();
}

bool CScriptedHoliday::IsActive(const time_t& timeCurrent)
{
	if (!m_Operators.Count())
		return false;

	for (auto pOperator : m_Operators)
	{
		if (!pOperator->IsActive(timeCurrent))
			return false;
	}

	return true;
}

time_t CScriptedHoliday::GetStartTime()
{
	CalcTimesForCache();
	return m_timeStart;
}

time_t CScriptedHoliday::GetEndTime()
{
	CalcTimesForCache();
	return m_timeEnd;
}

void CScriptedHoliday::CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd)
{
	if (!m_Operators.Count())
	{
		timeStart = 0;
		timeEnd = 0;
	}
	else
	{
		timeStart = 0;
		timeEnd = LLONG_MAX;

		for (auto pOperator : m_Operators)
		{
			time_t opTimeStart, opTimeEnd;
			pOperator->CalculateTimes(timeCurrent, opTimeStart, opTimeEnd);

			timeStart = Max(timeStart, opTimeStart);
			timeEnd = Min(timeEnd, opTimeEnd);
		}
	}
}

void CScriptedHoliday::CalcTimesForCache()
{
	if (m_bCalcdTimes)
		return;

	time_t ltime = time(0);
	CalculateTimes(ltime, m_timeStart, m_timeEnd);
	m_bCalcdTimes = true;
}

CHolidayEventSystem::CHolidayEventSystem() : m_Holidays(HolidaySymbol::SymbolLessFunc)
{
}

bool CHolidayEventSystem::IsEventActive(int iEvent)
{
	time_t ltime = time(0);

	return IsEventActiveAtTime(iEvent, ltime);
}

int CHolidayEventSystem::LookupEvent(const char* pchEventName)
{
	return m_Holidays.Find(pchEventName);
}

int CHolidayEventSystem::LookupEvent(CHolidaySymbol sEventName)
{
	return m_Holidays.Find(sEventName);
}

int CHolidayEventSystem::GetActiveEvents(IHoliday** ppEvents, int iMaxEvents)
{
	time_t ltime = time(0);
	int iNumActive = 0;

	for (int i = 0; i < NumEvents() && iNumActive < iMaxEvents; i++)
	{
		if (IsEventActiveAtTime(i, ltime))
		{
			ppEvents[iNumActive] = GetEvent(i);
			iNumActive++;
		}
	}

	return iNumActive;
}

void CHolidayEventSystem::GetActiveEventNames(CUtlVector<const char*>& vEvents)
{
	time_t ltime = time(0);

	for (int i = 0; i < NumEvents(); i++)
	{
		if (IsEventActiveAtTime(i, ltime))
		{
			vEvents.AddToTail(GetEvent(i)->GetHolidayName());
		}
	}
}

bool CHolidayEventSystem::Init()
{
	KeyValues* pKV = new KeyValues("Holidays");

	if (pKV->LoadFromFile(g_pFullFileSystem, "scripts/holiday_events.txt", "MOD"))
	{
		for (KeyValues* pkvData = pKV->GetFirstTrueSubKey(); pkvData != NULL; pkvData = pkvData->GetNextTrueSubKey())
		{
			if (m_Holidays.Find(pkvData->GetName()) != m_Holidays.InvalidIndex())
				continue;

			CScriptedHoliday* pHoliday = new CScriptedHoliday(pkvData->GetName());
			for (KeyValues* pkvOperator = pkvData->GetFirstTrueSubKey(); pkvOperator != NULL; pkvOperator = pkvOperator->GetNextTrueSubKey())
			{
				CScriptOperator* pOperator = ParseScriptOperator(pkvOperator, pkvData->GetName());
				if (pOperator)
				{
					pHoliday->m_Operators.AddToTail(pOperator);
				}
			}

			m_Holidays.Insert(pkvData->GetName(), pHoliday);
		}
	}

	pKV->deleteThis();

	return true;
}

void CHolidayEventSystem::Shutdown()
{
	m_Holidays.PurgeAndDeleteElements();
	g_CHolidaySymbolSymbolTable.RemoveAll();
}

bool CHolidayEventSystem::IsEventActiveAtTime(int iEvent, const time_t& curTime)
{
	IHoliday* pEvent = GetEvent(iEvent);
	if (!pEvent)
		return false;

	return pEvent->IsActive(curTime);
}

bool CHolidayEventSystem::IsEventActiveAtTime(CHolidaySymbol sEvent, const time_t& curTime)
{
	int iIndex = m_Holidays.Find(sEvent);
	IHoliday* pEvent = GetEvent(iIndex);
	if (!pEvent)
		return false;

	return pEvent->IsActive(curTime);
}

namespace ScriptOperators
{
	class COperatorOR : public CScriptOperator
	{
	public:
		COperatorOR(CScriptOperator* pA, CScriptOperator* pB);

		virtual bool IsActive(const time_t& timeCurrent);
		virtual void Release();
		virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);
	private:
		CScriptOperator* m_pOPS[2];
	};

	COperatorOR::COperatorOR(CScriptOperator* pA, CScriptOperator* pB)
	{
		m_pOPS[0] = pA;
		m_pOPS[1] = pB;
	}

	bool COperatorOR::IsActive(const time_t& timeCurrent)
	{
		return (m_pOPS[0]->IsActive(timeCurrent) || m_pOPS[1]->IsActive(timeCurrent));
	}

	void COperatorOR::Release()
	{
		m_pOPS[0]->Release();
		m_pOPS[1]->Release();
		CScriptOperator::Release();
	}

	void COperatorOR::CalculateTimes(const time_t& timeCurrent, time_t& outTimeStart, time_t& outTimeEnd)
	{
		time_t timeStart[2], timeEnd[2];
		m_pOPS[0]->CalculateTimes(timeCurrent, timeStart[0], timeEnd[0]);
		m_pOPS[1]->CalculateTimes(timeCurrent, timeStart[1], timeEnd[1]);

		// Check for an inactive split.
		bool bSplit = (timeStart[0] > timeEnd[1] || timeStart[1] > timeEnd[0]);
		if (bSplit)
		{
			const bool bAMin = (timeStart[0] < timeStart[1]);
			const time_t& timeStartMin = timeStart[bAMin ? 0 : 1];
			const time_t& timeEndMin = timeEnd[bAMin ? 0 : 1];
			const bool bInMin = ((timeCurrent >= timeStartMin) && (timeCurrent <= timeEndMin));
			if (bInMin)
			{
				outTimeStart = timeStartMin;
				outTimeEnd = timeEndMin;
			}
			else
			{
				outTimeStart = timeStart[bAMin ? 1 : 0];
				outTimeEnd = timeEnd[bAMin ? 1 : 0];
			}
		}
		else
		{
			outTimeStart = Min(timeStart[0], timeStart[1]);
			outTimeEnd = Max(timeEnd[0], timeEnd[1]);
		}
	}

	class COperatorGetHoliday : public CScriptOperator
	{
	public:
		COperatorGetHoliday(CHolidayEventSystem* pSystem, CHolidaySymbol sHoliday);
		virtual bool IsActive(const time_t& timeCurrent);
		virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);

	private:
		CHolidaySymbol m_Holiday;
		CHolidayEventSystem* m_pSystem;
	};

	COperatorGetHoliday::COperatorGetHoliday(CHolidayEventSystem* pSystem, CHolidaySymbol sHoliday)
	{
		m_pSystem = pSystem;
		m_Holiday = sHoliday;
	}

	bool COperatorGetHoliday::IsActive(const time_t& timeCurrent)
	{
		return m_pSystem->IsEventActiveAtTime(m_Holiday, timeCurrent);;
	}

	void COperatorGetHoliday::CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd)
	{
		int iIndex = m_pSystem->LookupEvent(m_Holiday);
		IHoliday* pEvent = m_pSystem->GetEvent(iIndex);
		if (!pEvent)
		{
			return;
		}

		assert_cast<CScriptedHoliday*> (pEvent)->CalculateTimes(timeCurrent, timeStart, timeEnd);
	}

	class COperatorCyclical : public CScriptOperator
	{
	public:
		COperatorCyclical(int iMonth, int iDay, int iYear, float fCycleLengthInDays, float fBonusTimeInDays)
			: m_fCycleLengthInDays(fCycleLengthInDays)
			, m_fBonusTimeInDays(fBonusTimeInDays)
		{
			// When is our initial interval?
			tm holiday_tm = { };
			holiday_tm.tm_mday = iDay;
			holiday_tm.tm_mon = iMonth;
			holiday_tm.tm_year = iYear - 1900; // convert to years since 1900
			m_timeInitial = _mkgmtime(&holiday_tm);
		}

		virtual bool IsActive(const time_t& timeCurrent);
		virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);
	private:
		time_t m_timeInitial;

		float m_fCycleLengthInDays;
		float m_fBonusTimeInDays;
	};

	bool COperatorCyclical::IsActive(const time_t& timeCurrent)
	{
		// Days-to-seconds conversion.
		const int iSecondsPerDay = 24 * 60 * 60;

		// Convert our cycle/buffer times to seconds.
		const int iCycleLengthInSeconds = (int)(m_fCycleLengthInDays * iSecondsPerDay);
		const int iBufferTimeInSeconds = (int)(m_fBonusTimeInDays * iSecondsPerDay);

		// How long has it been since we started this cycle?
		int iSecondsIntoCycle = (timeCurrent - m_timeInitial) % iCycleLengthInSeconds;

		// If we're within the buffer period right after the start of a cycle, we're active.
		if (iSecondsIntoCycle < iBufferTimeInSeconds)
			return true;

		// If we're within the buffer period towards the end of a cycle, we're active.
		if (iSecondsIntoCycle > iCycleLengthInSeconds - iBufferTimeInSeconds)
			return true;

		// Alas, normal mode for us.
		return false;
	}

	void COperatorCyclical::CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd)
	{
		// Days-to-seconds conversion.
		const int iSecondsPerDay = 24 * 60 * 60;

		// Convert our cycle/buffer times to seconds.
		const int iCycleLengthInSeconds = (int)(m_fCycleLengthInDays * iSecondsPerDay);
		const int iBufferTimeInSeconds = (int)(m_fBonusTimeInDays * iSecondsPerDay);

		// How long has it been since we started this cycle?
		int iSecondsIntoCycle = (timeCurrent - m_timeInitial) % iCycleLengthInSeconds;
		time_t timeCycleCenter = timeCurrent - iSecondsIntoCycle;

		// If we're within the buffer period right after the start of a cycle, we're active.
		if (iSecondsIntoCycle >= iBufferTimeInSeconds)
		{
			// Next Cycle
			timeCycleCenter += iCycleLengthInSeconds;
		}

		timeStart = timeCycleCenter - iBufferTimeInSeconds;
		timeEnd = timeCycleCenter + iBufferTimeInSeconds;
	}

	class COperatorSingleDay : public CScriptOperator
	{
	public:
		COperatorSingleDay(int iMonth, int iDay)
			: m_iMonth(iMonth)
			, m_iDay(iDay)
		{
		}

		virtual bool IsActive(const time_t& timeCurrent);
		virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);

	private:
		int m_iMonth;
		int m_iDay;
	};

	bool COperatorSingleDay::IsActive(const time_t& timeCurrent)
	{
		tm day;
		localtime_s(&day, &timeCurrent);

		return m_iMonth == day.tm_mon
			&& m_iDay == day.tm_mday;
	}

	void COperatorSingleDay::CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd)
	{
		tm day;
		localtime_s(&day, &timeCurrent);
		day.tm_mon = m_iMonth;
		day.tm_mday = m_iDay;
		day.tm_hour = day.tm_min = day.tm_sec = 0;
		timeStart = mktime(&day);
		day.tm_hour = 23;
		day.tm_min = 59;
		timeEnd = mktime(&day);
	}

	class COperatorDateRange : public CScriptOperator
	{
	public:
		COperatorDateRange(int iStartMonth, int iStartDay, int iEndMonth, int iEndDay)
			: m_iStartMonth(iStartMonth),
			m_iStartDay(iStartDay),
			m_iEndMonth(iEndMonth),
			m_iEndDay(iEndDay)
		{
			m_bWrapsYear = (m_iStartMonth > m_iEndMonth);
		}

		virtual bool IsActive(const time_t& timeCurrent);
		virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);

	private:
		int m_iStartMonth, m_iEndMonth;
		int m_iStartDay, m_iEndDay;
		bool m_bWrapsYear;
	};

	bool COperatorDateRange::IsActive(const time_t& timeCurrent)
	{
		tm today;
		localtime_s(&today, &timeCurrent);

		const int& iDay = today.tm_mday;
		const int& iMonth = today.tm_mon;

		if ((iMonth == m_iStartMonth && iDay >= m_iStartDay) || iMonth > m_iStartMonth || (m_bWrapsYear && iMonth <= m_iEndMonth))
		{
			if ((iMonth == m_iEndMonth && iDay <= m_iEndDay) || iMonth < m_iEndMonth)
			{
				return true;
			}
		}

		return false;
	}

	void COperatorDateRange::CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd)
	{
		tm today;
		localtime_s(&today, &timeCurrent);
		today.tm_hour = today.tm_min = today.tm_sec = 0;

		const int iDay = today.tm_mday;
		const int iMonth = today.tm_mon;

		if (m_bWrapsYear && ((iMonth == m_iEndMonth && iDay <= m_iEndDay) || iMonth < m_iEndMonth))
		{
			today.tm_year--;
		}

		today.tm_mon = m_iStartMonth;
		today.tm_mday = m_iStartDay;
		timeStart = mktime(&today);

		if (m_bWrapsYear)
		{
			today.tm_year++;
		}

		today.tm_mon = m_iEndMonth;
		today.tm_mday = m_iEndDay;
		today.tm_hour = 23;
		today.tm_min = 59;
		timeEnd = mktime(&today);
	}

	class COperatorWeekBased : public CScriptOperator
	{
	public:
		COperatorWeekBased(int iMonth, int iDay, int iExtraWeeks)
			: m_iMonth(iMonth)
			, m_iDay(iDay)
			, m_iExtraWeeks(iExtraWeeks)
			, m_iCachedCalculatedYear(0)
		{}

		virtual bool IsActive(const time_t& timeCurrent);
		virtual void CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd);

	private:
		void RecalculateTimeActiveInterval(int iYear);

		static const int kMonday = 1;
		static const int kFriday = 5;

		int m_iMonth;
		int m_iDay;
		int m_iExtraWeeks;

		// Filled out from RecalculateTimeActiveInterval().
		int m_iCachedCalculatedYear;

		time_t m_timeStart;
		time_t m_timeEnd;
	};

	bool COperatorWeekBased::IsActive(const time_t& timeCurrent)
	{
		tm now;
		localtime_s(&now, &timeCurrent);

		const int iCurrentYear = now.tm_year;
		if (m_iCachedCalculatedYear != iCurrentYear)
			RecalculateTimeActiveInterval(iCurrentYear);

		return timeCurrent > m_timeStart
			&& timeCurrent < m_timeEnd;
	}

	void COperatorWeekBased::CalculateTimes(const time_t& timeCurrent, time_t& timeStart, time_t& timeEnd)
	{
		tm now;
		localtime_s(&now, &timeCurrent);

		const int iCurrentYear = now.tm_year;
		if (m_iCachedCalculatedYear != iCurrentYear)
			RecalculateTimeActiveInterval(iCurrentYear);

		timeStart = m_timeStart;
		timeEnd = m_timeEnd;
	}

	void COperatorWeekBased::RecalculateTimeActiveInterval(int iYear)
	{
		// Get the date of the holiday.
		tm holiday_tm = { };
		holiday_tm.tm_mday = m_iDay;
		holiday_tm.tm_mon = m_iMonth;
		holiday_tm.tm_year = iYear;
		mktime(&holiday_tm);

		// The event starts on the first Friday at least four days prior to the holiday.
		tm start_time_tm(holiday_tm);
		start_time_tm.tm_mday -= 4;							// Move back four days.
		mktime(&start_time_tm);
		int days_offset = start_time_tm.tm_wday - kFriday;	// Find the nearest prior Friday.
		if (days_offset < 0)
			days_offset += 7;
		start_time_tm.tm_mday -= days_offset;
		time_t start_time = mktime(&start_time_tm);

		// The event ends on the first Monday after the holiday, maybe plus some additional fudge
		// time.
		tm end_time_tm(holiday_tm);
		days_offset = 7 - (end_time_tm.tm_wday - kMonday);
		if (days_offset >= 7)
			days_offset -= 7;
		end_time_tm.tm_mday += days_offset + 7 * m_iExtraWeeks;
		time_t end_time = mktime(&end_time_tm);

		m_timeStart = start_time;
		m_timeEnd = end_time;

		// We're done and our interval data is cached.
		m_iCachedCalculatedYear = iYear;
	}
}

CScriptOperator* CHolidayEventSystem::ParseScriptOperator(KeyValues* pKV, const char* pszEventName)
{
	const char* pszOperator = pKV->GetName();
	if (V_stricmp(pszOperator, "OR") == 0)
	{
		CUtlVector<CScriptOperator*> Operators;
		for (KeyValues* pkvOperator = pKV->GetFirstTrueSubKey(); pkvOperator != NULL; pkvOperator = pkvOperator->GetNextTrueSubKey())
		{
			CScriptOperator* pOperator = ParseScriptOperator(pkvOperator, pszEventName);
			if (pOperator)
			{
				Operators.AddToTail(pOperator);
			}
		}

		// Replace the below if OR supports more than two operators
		if (Operators.Count() != 2)
		{
			Warning("[HolidayParseScriptOperator] %s: OR operator takes two sub-operators!\n", pszEventName);
			Operators.PurgeAndDeleteElements();
			return nullptr;
		}
		else
		{
			return new ScriptOperators::COperatorOR(Operators[0], Operators[1]);
		}
	}
	else if (V_stricmp(pszOperator, "GET_HOLIDAY") == 0)
	{
		const char* pszHoliday = pKV->GetString("holiday", nullptr);
		if (!pszHoliday)
		{
			Warning("[HolidayParseScriptOperator] %s: GET_HOLIDAY missing parameter holiday!\n", pszEventName);
			return nullptr;
		}

		return new ScriptOperators::COperatorGetHoliday(this, pszHoliday);
	}
	else if (V_stricmp(pszOperator, "SINGLE_DAY") == 0)
	{
		int iMonth = pKV->GetInt("month");
		int iDay = pKV->GetInt("day");

		if (iMonth <= 0 || iMonth > 12)
		{
			Warning("[HolidayParseScriptOperator] %s: SINGLE_DAY parameter month out of range! (1-12)\n", pszEventName);
			return nullptr;
		}

		if (iDay <= 0 || iDay > 31)
		{
			Warning("[HolidayParseScriptOperator] %s: SINGLE_DAY parameter day out of range! (1-31)\n", pszEventName);
			return nullptr;
		}

		return new ScriptOperators::COperatorSingleDay(iMonth - 1, iDay);
	}
	else if (V_stricmp(pszOperator, "DATE_RANGE") == 0)
	{
		int tm_mon_start = pKV->GetInt("start_month", 1);
		int tm_mday_start = pKV->GetInt("start_day", 1);

		int tm_mon_end = pKV->GetInt("end_month", 12);
		int tm_mday_end = pKV->GetInt("end_day", 31);

		if (tm_mon_start <= 0 || tm_mon_start > 12)
		{
			Warning("[HolidayParseScriptOperator] %s: DATE_RANGE parameter start_month out of range! (1-12)\n", pszEventName);
			return nullptr;
		}

		if (tm_mon_end <= 0 || tm_mon_end > 12)
		{
			Warning("[HolidayParseScriptOperator] %s: DATE_RANGE parameter end_month out of range! (1-12)\n", pszEventName);
			return nullptr;
		}

		if (tm_mday_start <= 0 || tm_mday_start > 31)
		{
			Warning("[HolidayParseScriptOperator] %s: DATE_RANGE parameter start_day out of range! (1-31)\n", pszEventName);
			return nullptr;
		}

		if (tm_mday_end <= 0 || tm_mday_end > 31)
		{
			Warning("[HolidayParseScriptOperator] %s: DATE_RANGE parameter end_day out of range! (1-31)\n", pszEventName);
			return nullptr;
		}

		return new ScriptOperators::COperatorDateRange(tm_mon_start - 1, tm_mday_start, tm_mon_end - 1, tm_mday_end);
	}
	else if (V_stricmp(pszOperator, "CYCLICAL") == 0)
	{
		int iMonth = pKV->GetInt("month");
		int iDay = pKV->GetInt("day");
		int iYear = pKV->GetInt("year");

		if (iMonth <= 0 || iMonth > 12)
		{
			Warning("[HolidayParseScriptOperator] %s: CYCLICAL parameter month out of range! (1-12)\n", pszEventName);
			return nullptr;
		}

		if (iDay <= 0 || iDay > 31)
		{
			Warning("[HolidayParseScriptOperator] %s: CYCLICAL parameter day out of range! (1-31)\n", pszEventName);
			return nullptr;
		}

		if (iYear < 1900)
		{
			Warning("[HolidayParseScriptOperator] %s: CYCLICAL parameter year out of range! (1900+)\n", pszEventName);
			return nullptr;
		}

		float fCycleLengthInDays = pKV->GetFloat("cycle_length", 1.0f);
		float fBonusTimeInDays = pKV->GetFloat("bonus_time");

		return new ScriptOperators::COperatorCyclical(iMonth - 1, iDay, iYear, fCycleLengthInDays, fBonusTimeInDays);
	}
	else if (V_stricmp(pszOperator, "WEEK_BASED") == 0)
	{
		int iMonth = pKV->GetInt("month");
		int iDay = pKV->GetInt("day");
		int iExtraWeeks = pKV->GetInt("extra_weeks");

		if (iMonth <= 0 || iMonth > 12)
		{
			Warning("[HolidayParseScriptOperator] %s: WEEK_BASED parameter month out of range! (1-12)\n", pszEventName);
			return nullptr;
		}

		if (iDay <= 0 || iDay > 31)
		{
			Warning("[HolidayParseScriptOperator] %s: WEEK_BASED parameter day out of range! (1-31)\n", pszEventName);
			return nullptr;
		}

		if (iExtraWeeks < 0)
		{
			Warning("[HolidayParseScriptOperator] %s: WEEK_BASED parameter extra_weeks must be greate than or equal to zero!\n", pszEventName);
			return nullptr;
		}

		return new ScriptOperators::COperatorWeekBased(iMonth - 1, iDay, iExtraWeeks);
	}
	else
	{
		return nullptr;
	}
}
