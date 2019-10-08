#ifndef SPP_UTILS_H
#define SPP_UTILS_H
#pragma once

#include "appframework/IAppSystem.h"

#define SPP_UTILS_INTERFACE "VGameSharedUtils001"

class IMapEditHelper
{
public:
	virtual const char* DoMapEdit(const char* pMapName, const char* pMapEntities) = 0;
};

class IGameSharedUtils : public IAppSystem
{
public:
	virtual IMapEditHelper* GetMapEditHelper() = 0;
};

#endif // !SPP_UTILS_H
