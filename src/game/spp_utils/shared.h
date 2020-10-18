#pragma once

#include "eiface.h"
#include "edict.h"
#include "cdll_int.h"
#include "vphysics_interface.h"
#include "../shared/usermessages.h"

typedef struct enginePointers_s
{
	IVEngineClient* engineclient;
	IVEngineServer* engineserver;
	IBaseClientDLL* gameclient;
	IServerGameDLL* gameserver;
	CGlobalVarsBase* gpClientGlobals;
	CGlobalVars* gpServerGlobals;

	CUserMessages* cl_usermessages;
	CUserMessages* sv_usermessages;

	IPhysicsSurfaceProps* physicsprops;

	enginePointers_s()
	{
		memset(this, 0, sizeof(enginePointers_s));
	}
} enginePointers_t;

class IInternalSharedData
{
public:
	virtual bool IsServerRunning() = 0;
	virtual bool IsClientConnected() = 0;
	virtual bool IsClientOnLocalServer() = 0;
	virtual bool IsRunningBackgroundMap() = 0;

	virtual const char* GetServerLevel() = 0;
	virtual MapLoadType_t GetServerLoadType() = 0;

	virtual const enginePointers_t& GetEnginePointers() = 0;
};

extern IInternalSharedData* internaldata;