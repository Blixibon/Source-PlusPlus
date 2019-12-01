#include "cbase.h"
#include "discord/discord_rpc.h"
#include "igamesystem.h"
#include "GameEventListener.h"
#include "gamerules.h"
#include "fmtstr.h"
#include "inetchannelinfo.h"
#include <time.h>
#include "igameresources.h"
#include "hltvcamera.h"
#include "inetchannel.h"
#include "netadr.h"

#include "memdbgon.h"

#define DISCORD_UPDATE_RATE 10.0f

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
	CFmtStr command("connect %s", secret);

	engine->ClientCmd(command);
}

static void HandleDiscordSpectate(const char* secret)
{
	CFmtStr command("connect %s", secret);

	engine->ClientCmd(command);
}

static void HandleDiscordJoinRequest(const DiscordUser* request)
{
	// Not implemented
}

class CLazPresence : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CLazPresence() : CAutoGameSystemPerFrame("LazPresence")
	{}

	bool Init();
	void Shutdown();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPreEntity();

	virtual void FireGameEvent(IGameEvent* event);

	void Update(float frametime);

	void UpdateDiscord(const char* pchLevelName);

	char m_szMapName[64];
};

CLazPresence g_LazPresence;

bool CLazPresence::Init()
{
	ListenForGameEvent("server_spawn");

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
	sprintf(appid, "%d", engine->GetAppID());
	Discord_Initialize("648788826394984459", &handlers, 0, appid);

	return true;
}

void CLazPresence::Shutdown()
{
	Discord_Shutdown();
}

void CLazPresence::LevelInitPreEntity()
{
	V_StripExtension(V_UnqualifiedFileName(engine->GetLevelName()), m_szMapName, 64);
}

void CLazPresence::LevelShutdownPreEntity()
{
	V_memset(m_szMapName, 0, 64);
}

void CLazPresence::FireGameEvent(IGameEvent* event)
{
	const char* pEventName = event->GetName();

	// when we are changing levels and 
	if (0 == Q_strcmp(pEventName, "server_spawn"))
	{
		const char* pMapName = event->GetString("mapname");

		V_strncpy(m_szMapName, pMapName, 64);
	}
}

static int64_t startTimestamp = time(0);

void CLazPresence::Update(float frametime)
{
	UpdateDiscord(m_szMapName);
	Discord_RunCallbacks();
}

void CLazPresence::UpdateDiscord(const char* pchLevelName)
{
	DiscordRichPresence data;
	memset(&data, 0, sizeof(data));
	CUtlString dynStrings[4];

	if (engine->IsConnected() && !engine->IsLevelMainMenuBackground())
	{
		if (gpGlobals->maxClients > 1)
		{
			if (!engine->IsInGame())
			{
				data.state = "Connecting";
				data.details = "Multiplayer";
			}
			else
			{
				INetChannelInfo* pNet = engine->GetNetChannelInfo();
				if (pNet)
				{
					dynStrings[0].Format("ip %s", pNet->GetAddress());
					data.partyId = dynStrings[0].Get();
					netadr_t addr(pNet->GetAddress());
					if (!addr.IsLoopback())
					{
						if (!engine->IsHLTV())
						{
							dynStrings[1].Format("%s:%i", addr.ToString(true), GameRules()->m_nServerPort.Get());
							data.joinSecret = dynStrings[1].Get();
						}
						if (GameRules()->m_nHLTVPort > 0)
						{
							dynStrings[2].Format("%s:%i", addr.ToString(true), GameRules()->m_nHLTVPort.Get());
							data.spectateSecret = dynStrings[2].Get();
						}
					}
				}

				data.startTimestamp = startTimestamp;
				data.partyMax = engine->GetMaxClients();
				/*if (engine->IsHLTV())
				{
					data.partySize
				}
				else*/
				{
					for (int i = 1; i < MAX_PLAYERS; i++)
					{
						if (GameResources() && GameResources()->IsConnected(i))
							data.partySize++;
					}
				}

				data.state = pchLevelName;
				data.details = engine->IsHLTV() ? "Spectating" : "Multiplayer";
			}
		}
		else
		{
			if (!engine->IsInGame() || engine->IsDrawingLoadingImage())
			{
				data.state = "Loading";
			}
			else
			{
				data.details = "Singleplayer";
				data.state = pchLevelName;
			}
		}
	}
	else
	{
		data.state = "";
		data.details = "Main Menu";
	}

	Discord_UpdatePresence(&data);
}
