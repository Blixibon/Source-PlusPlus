#include "spp_utils.h"
#include "shared.h"
#include "mapedit_helper.h"
#include "holiday_event_system.h"
#include "rich_presence_system.h"
#include "scenecache.h"
#include "shader_data_system.h"
#include "tier3/tier3.h"

//IVEngineClient* engineclient = nullptr;
//IVEngineServer* engineserver = nullptr;
//IBaseClientDLL* gameclient = nullptr;
//IServerGameDLL* gameserver = nullptr;
//CGlobalVarsBase* gpClientGlobals = nullptr;
//CGlobalVars* gpServerGlobals = nullptr;

// This is to ensure a dependency exists between the vscript library and the game DLLs
extern int vscript_token;
int vscript_token_hack = vscript_token;

class CGameSharedUtils : public CTier3AppSystem<IGameSharedUtils>, public IInternalSharedData
{
	typedef CTier3AppSystem<IGameSharedUtils> BaseClass;
public:
	CGameSharedUtils() : BaseClass(true)
	{
		m_iConnectCount = 0;
		m_iShaderCount = 0;
		m_bInitialized = false;
	}

	virtual const int	GetRefCount() const
	{
		return m_iConnectCount + m_iShaderCount;
	}

	virtual void	ShaderInit();
	virtual void	ShaderShutdown();

	virtual bool Connect( CreateInterfaceFn factory ) 
	{
		bool bRet = true;
		if (m_iConnectCount == 0)
		{
			bRet = BaseClass::Connect(factory) && m_SceneCache.Connect(factory) && m_Holidays.Init() && m_Presence.Connect(factory);
			m_Presence.InternalInit(this);
		}

		m_iConnectCount++;
		return bRet;
	}

	virtual void Disconnect()
	{
		m_iConnectCount--;
		if (m_iConnectCount != 0)
			return;

		m_Holidays.Shutdown();
		m_SceneCache.Disconnect();
		m_Presence.Disconnect();
		BaseClass::Disconnect();
	}

	virtual InitReturnVal_t Init()
	{
		if (!m_bInitialized)
		{
			InitReturnVal_t nRetVal = BaseClass::Init();
			if (nRetVal != INIT_OK)
				return nRetVal;

			nRetVal = m_SceneCache.Init();
			if (nRetVal != INIT_OK)
				return nRetVal;

			m_bInitialized = true;
		}

		return INIT_OK;
	}

	virtual void Shutdown()
	{
		if (m_bInitialized)
		{
			m_SceneCache.Shutdown();
			BaseClass::Shutdown();
			m_bInitialized = false;
		}
	}

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void* QueryInterface(const char* pInterfaceName);

	virtual const char* DoMapEdit(const char* pMapName, const char* pMapEntities, CUtlVector<char*>& vecVariants);
	virtual IHolidayEvents* GetEventSystem() { return &m_Holidays; }
	virtual IDiscordPresence* GetRichPresence() { return &m_Presence; }

	// Inherited via IGameSharedUtils
	virtual bool InitClient(CreateInterfaceFn engineFactory, CreateInterfaceFn clientFactory, CGlobalVarsBase* pGlobals) override
	{
		m_EnginePointers.engineclient = (IVEngineClient*)engineFactory(VENGINE_CLIENT_INTERFACE_VERSION, NULL);
		m_EnginePointers.gameclient = (IBaseClientDLL*)clientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);
		m_EnginePointers.gpClientGlobals = pGlobals;
		return true;
	}
	virtual bool InitServer(CreateInterfaceFn engineFactory, CreateInterfaceFn gameFactory, CGlobalVars* pGlobals) override
	{
		m_EnginePointers.engineserver = (IVEngineServer*)engineFactory(INTERFACEVERSION_VENGINESERVER, NULL);
		m_EnginePointers.gameserver = (IServerGameDLL*)gameFactory(INTERFACEVERSION_SERVERGAMEDLL, NULL);
		m_EnginePointers.gpServerGlobals = pGlobals;
		return true;
	}

	virtual bool ServerLevelInit(const char* pMapName, char const* pOldLevel, bool loadGame, bool background) override
	{
		m_ServerInfo.m_bRunning = true;
		m_ServerInfo.m_strMapName.Set(pMapName);
		if (loadGame)
		{
			if (pOldLevel)
			{
				m_ServerInfo.m_eMapLoad = MapLoad_Transition;
			}
			else
			{
				m_ServerInfo.m_eMapLoad = MapLoad_LoadGame;
			}
		}
		else
		{
			if (background)
			{
				m_ServerInfo.m_eMapLoad = MapLoad_Background;
			}
			else
			{
				m_ServerInfo.m_eMapLoad = MapLoad_NewGame;
			}
		}

		return true;
	}
	virtual void ServerLevelShutdown() override
	{
		m_ServerInfo.m_bRunning = false;
	}
	virtual bool ClientLevelInit(char const* pMapName) override
	{
		return true;
	}
	virtual void ClientLevelShutdown() override
	{
	}

private:
	int m_iConnectCount;
	int m_iShaderCount;
	bool m_bInitialized;
	CMapEditHelper m_Helper;
	CHolidayEventSystem m_Holidays;
	CRichPresense m_Presence;
	CSceneFileCache m_SceneCache;
	CShaderDataExtension m_ShaderExtension;

	typedef struct {
		bool m_bRunning;

		CUtlString m_strMapName;
		MapLoadType_t m_eMapLoad;
	} ServerData_t;
	ServerData_t m_ServerInfo;

	enginePointers_t m_EnginePointers;
public:


	// Inherited via IInternalSharedData
	virtual bool IsServerRunning() override
	{
		return m_ServerInfo.m_bRunning;
	}

	virtual bool IsClientConnected() override
	{
		return m_EnginePointers.engineclient && m_EnginePointers.engineclient->IsConnected();
	}

	virtual bool IsClientOnLocalServer() override
	{
		return IsServerRunning() && IsClientConnected();
	}

	virtual bool IsRunningBackgroundMap() override
	{
		return IsServerRunning() && GetServerLoadType() == MapLoad_Background;
	}

	virtual const char* GetServerLevel() override
	{
		return m_ServerInfo.m_strMapName.Get();
	}

	virtual MapLoadType_t GetServerLoadType() override
	{
		return m_ServerInfo.m_eMapLoad;
	}

	// Inherited via IInternalSharedData
	virtual const enginePointers_t& GetEnginePointers() override
	{
		return m_EnginePointers;
	}

};

CGameSharedUtils g_SharedUtils;
IInternalSharedData* internaldata = &g_SharedUtils;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameSharedUtils, IGameSharedUtils, SPP_UTILS_INTERFACE, g_SharedUtils);

void CGameSharedUtils::ShaderInit()
{
	m_iShaderCount++;
}

void CGameSharedUtils::ShaderShutdown()
{
	m_iShaderCount--;
}

void* CGameSharedUtils::QueryInterface(const char* pInterfaceName)
{
	if (V_strcmp(SCENE_FILE_CACHE_INTERFACE_VERSION, pInterfaceName) == 0)
	{
		return static_cast<ISceneFileCache*> (&m_SceneCache);
	}
	else if (V_strcmp(SHADEREXTENSION_INTERFACE_VERSION, pInterfaceName) == 0)
	{
		return static_cast<IShaderExtension*> (&m_ShaderExtension);
	}
	else if (V_strcmp(SHADEREXTENSIONINTERNAL_INTERFACE_VERSION, pInterfaceName) == 0)
	{
		return static_cast<IShaderExtensionInternal*> (&m_ShaderExtension);
	}
	else
	{
		return Sys_GetFactoryThis()(pInterfaceName, NULL);
	}
}

const char* CGameSharedUtils::DoMapEdit(const char* pMapName, const char* pMapEntities, CUtlVector<char*>& vecVariants)
{
	return m_Helper.DoMapEdit(pMapName, pMapEntities, vecVariants);
}
