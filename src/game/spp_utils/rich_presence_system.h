#pragma once
#include "rich_presence.h"
#include "shared.h"

class CRichPresense : public IDiscordPresence
{
public:
	// Inherited via IDiscordPresence
	virtual void InitDiscord(const char* applicationId) override;
	virtual void ShutdownDiscord();
	virtual void SetServerPorts(int iGame, int iSpectate) override;
	virtual void ResetTimeStamp();
	// Inherited via IDiscordPresence
	virtual void RunCallbacks() override;

	static unsigned UpdateThread(void* pParams);

	void	InternalInit(IInternalSharedData* pInternalData);

private:
	ThreadHandle_t m_UpdateThread;
	CThreadRWLock m_Lock;
	CInterlockedInt m_Signal;

	int m_iGamePort;
	int m_iHLTVPort;

	int64 m_liTimeStamp;

	bool	m_bStarted;

	const enginePointers_t* m_pEnginePointers;
	IInternalSharedData* m_pInternalData;
};

