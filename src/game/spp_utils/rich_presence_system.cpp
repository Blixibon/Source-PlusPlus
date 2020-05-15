#include "shared.h"
#include "rich_presence.h"
#include "rich_presence_system.h"
#include "inetchannel.h"
#include "netadr.h"
#include "inetchannelinfo.h"
#include <time.h>
#include "fmtstr.h"
#include "discord/discord_rpc.h"
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "iserver.h"

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_ServerBrowser("ServerBrowser");

static CRichPresense* s_pRichPresence = nullptr;

//-----------------------------------------------------------------------------
// Discord RPC handlers
//-----------------------------------------------------------------------------
static void HandleDiscordReady(const DiscordUser* connectedUser)
{
	if (!connectedUser)
		return;

	DevMsg("Discord: Connected to user %s#%s - %s\n",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId);
}

static void HandleDiscordDisconnected(int errcode, const char* message)
{
	DevMsg("Discord: Disconnected (%d: %s)\n", errcode, message);
}

static void HandleDiscordError(int errcode, const char* message)
{
	DevMsg("Discord: Error (%d: %s)\n", errcode, message);
}

static void HandleDiscordJoin(const char* secret)
{
	if (s_pRichPresence)
	{
		s_pRichPresence->InitiateConnection(secret);
	}
}

static void HandleDiscordSpectate(const char* secret)
{
	if (s_pRichPresence)
	{
		s_pRichPresence->InitiateConnection(secret);
	}
}

static void HandleDiscordJoinRequest(const DiscordUser* request)
{
	// Not implemented
}

bool CRichPresense::Connect(CreateInterfaceFn factory)
{
	CreateInterfaceFn ServerBrowserFactory = g_ServerBrowser.GetFactory();
	if (!ServerBrowserFactory)
		return false;

	m_pServerBrowser = (IServerBrowser *)ServerBrowserFactory(SERVERBROWSER_INTERFACE_VERSION, NULL);
	if (!m_pServerBrowser)
		return false;

	return true;
}

void CRichPresense::InitDiscord(const char* applicationId)
{
	if (m_bStarted)
		return;

	// Discord RPC
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = HandleDiscordReady;
	handlers.disconnected = HandleDiscordDisconnected;
	handlers.errored = HandleDiscordError;
	handlers.joinGame = HandleDiscordJoin;
	handlers.spectateGame = HandleDiscordSpectate;
	handlers.joinRequest = HandleDiscordJoinRequest;

	char appid[255];
	sprintf(appid, "%d", m_pEnginePointers->engineclient->GetAppID());
	Discord_Initialize(applicationId, &handlers, 0, appid);
	ResetTimeStamp();

	//m_Signal = 1;
	//m_UpdateThread = CreateSimpleThread(CRichPresense::UpdateThread, this);

	m_bStarted = true;
}

void CRichPresense::ShutdownDiscord()
{
	if (!m_bStarted)
		return;

	//m_Signal = 0;
	//ThreadJoin(m_UpdateThread);
	//ReleaseThreadHandle(m_UpdateThread);

	Discord_Shutdown();

	m_bStarted = false;
}

void CRichPresense::SetServerPorts(int iGame, int iSpectate)
{
	//m_Lock.LockForWrite();
	m_iGamePort = iGame;
	m_iHLTVPort = iSpectate;
	//m_Lock.UnlockWrite();
}

void CRichPresense::ResetTimeStamp()
{
	//m_Lock.LockForWrite();
	m_liTimeStamp = time(0);
	//m_Lock.UnlockWrite();
}

void CRichPresense::InternalInit(IInternalSharedData* pInternalData)
{
	m_pEnginePointers = &pInternalData->GetEnginePointers();
	m_pInternalData = pInternalData;
}

void CRichPresense::InitiateConnection(const char* pszServer)
{
	netadr_t address(pszServer);
	if (address.IsValid())
	{
		m_pServerBrowser->JoinGame(address.GetIPHostByteOrder(), address.GetPort(), "");
	}
}

void CRichPresense::RunCallbacks()
{
	if (m_bStarted)
	{
		IVEngineClient* engineclient = m_pEnginePointers->engineclient;
		IServerGameDLL* gameserver = m_pEnginePointers->gameserver;
		IServer* internalserver = m_pEnginePointers->engineserver->GetIServer();

		DiscordRichPresence data;
		memset(&data, 0, sizeof(data));
		CUtlString dynStrings[5];

		if ((engineclient->IsConnected() || m_pInternalData->IsServerRunning()) && !engineclient->IsLevelMainMenuBackground())
		{
			dynStrings[3].SetLength(64);
			V_StripExtension(V_UnqualifiedFileName(engineclient->GetLevelName()), dynStrings[3].GetForModify(), 64);

			if (engineclient->GetMaxClients() > 1)
			{
				if (!engineclient->IsInGame())
				{
					data.state = "Loading";
					data.details = "Multiplayer";
				}
				else
				{
					INetChannelInfo* pNet = engineclient->GetNetChannelInfo();
					if (pNet)
					{
						dynStrings[0].Format("ip %s", pNet->GetAddress());
						data.partyId = dynStrings[0].Get();
						netadr_t addr(pNet->GetAddress());
						if (!pNet->IsLoopback())
						{
							if (!engineclient->IsHLTV())
							{
								dynStrings[1].Format("%s:%i", addr.ToString(true), m_iGamePort);
								data.joinSecret = dynStrings[1].Get();
							}
							if (m_iHLTVPort > 0)
							{
								dynStrings[2].Format("%s:%i", addr.ToString(true), m_iHLTVPort);
								data.spectateSecret = dynStrings[2].Get();
							}
						}
					}

					data.startTimestamp = m_liTimeStamp;
					data.partyMax = engineclient->GetMaxClients();

					data.state = dynStrings[3].Get();
					data.details = engineclient->IsHLTV() ? "Spectating" : "Multiplayer";
				}
			}
			else
			{
				if (!engineclient->IsInGame() || engineclient->IsDrawingLoadingImage())
				{
					if (m_pInternalData->IsServerRunning() && !internalserver->IsMultiplayer())
					{
						data.details = "Singleplayer";

						dynStrings[4].Format("Loading %s", m_pInternalData->GetServerLevel());
						data.state = dynStrings[4].Get();
					}
					else
					{
						data.state = "Connecting";
						data.details = "Multiplayer";
					}
				}
				else
				{
					data.details = "Singleplayer";
					char cMapTitle[64];
					gameserver->GetTitleName(dynStrings[3].Get(), cMapTitle, 64);
					const char* pchLocalized = g_pVGuiLocalize->FindAsUTF8(cMapTitle);
					if (pchLocalized)
						dynStrings[4].Set(pchLocalized);
					else
						dynStrings[4].Set(cMapTitle);

					data.state = dynStrings[4].Get();
				}
			}
		}
		else
		{
			data.state = "";
			data.details = "Main Menu";
		}

		Discord_UpdatePresence(&data);

		s_pRichPresence = this;
		Discord_RunCallbacks();
	}
}
