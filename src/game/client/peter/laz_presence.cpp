#include "cbase.h"
#include "igamesystem.h"
#include "GameEventListener.h"
#include "gamerules.h"
#include "igameresources.h"
#include "hltvcamera.h"
#include "spp_utils/spp_utils.h"
#include "spp_utils/rich_presence.h"

#include "memdbgon.h"

#define DISCORD_UPDATE_RATE 10.0f

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

	IDiscordPresence* m_pDiscord;
};

//CLazPresence g_LazPresence;

bool CLazPresence::Init()
{
	ListenForGameEvent("server_spawn");
	m_pDiscord = spp_utils->GetRichPresence();
	m_pDiscord->InitDiscord("648788826394984459");

	return true;
}

void CLazPresence::Shutdown()
{
	m_pDiscord->ShutdownDiscord();
}

void CLazPresence::LevelInitPreEntity()
{
	
}

void CLazPresence::LevelShutdownPreEntity()
{
	m_pDiscord->ResetTimeStamp();
}

void CLazPresence::FireGameEvent(IGameEvent* event)
{
	const char* pEventName = event->GetName();

	// when we are changing levels and 
	if (0 == Q_strcmp(pEventName, "server_spawn"))
	{
		m_pDiscord->ResetTimeStamp();
	}
}

void CLazPresence::Update(float frametime)
{
	if (GameRules())
		m_pDiscord->SetServerPorts(GameRules()->m_nServerPort, GameRules()->m_nHLTVPort);

	m_pDiscord->RunCallbacks();
}
