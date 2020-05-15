#pragma once
#include "rich_presence.h"
#include "shared.h"
#include "appframework/IAppSystem.h"
#include "ServerBrowser/IServerBrowser.h"

class CRichPresense : public CBaseAppSystem< IDiscordPresence >
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect(CreateInterfaceFn factory);

	// Inherited via IDiscordPresence
	virtual void InitDiscord(const char* applicationId) override;
	virtual void ShutdownDiscord();
	virtual void SetServerPorts(int iGame, int iSpectate) override;
	virtual void ResetTimeStamp();
	// Inherited via IDiscordPresence
	virtual void RunCallbacks() override;

	void	InternalInit(IInternalSharedData* pInternalData);

	void	InitiateConnection(const char* pszServer);

private:
	//ThreadHandle_t m_UpdateThread;
	//CThreadRWLock m_Lock;
	//CInterlockedInt m_Signal;

	int m_iGamePort;
	int m_iHLTVPort;

	int64 m_liTimeStamp;

	bool	m_bStarted;

	const enginePointers_t* m_pEnginePointers;
	IInternalSharedData* m_pInternalData;

	IServerBrowser* m_pServerBrowser;
};

