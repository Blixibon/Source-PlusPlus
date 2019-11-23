#ifndef SPP_UTILS_H
#define SPP_UTILS_H
#pragma once

#include "appframework/IAppSystem.h"
#include "utlvector.h"

class IHolidayEvents;

#define SPP_UTILS_INTERFACE "VGameSharedUtils002"

class IGameSharedUtils : public IAppSystem
{
public:
	virtual const char* DoMapEdit(const char* pMapName, const char* pMapEntities, CUtlVector<char *> &vecVariants) = 0;
	virtual IHolidayEvents* GetEventSystem() = 0;
};

#endif // !SPP_UTILS_H
