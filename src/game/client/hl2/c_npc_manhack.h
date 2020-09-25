#pragma once

#include "c_ai_basenpc.h"

class C_NPC_Manhack : public C_AI_BaseNPC
{
public:
	C_NPC_Manhack() {}

	DECLARE_CLASS(C_NPC_Manhack, C_AI_BaseNPC);
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	// Purpose: Start the manhack's engine sound.
	virtual void OnDataChanged(DataUpdateType_t type);
	virtual void UpdateOnRemove(void);
	virtual void OnRestore();

private:
	C_NPC_Manhack(const C_NPC_Manhack&);

	// Purpose: Start + stop the manhack's engine sound.
	void SoundInit(void);
	void SoundShutdown(void);

	CSoundPatch* m_pEngineSound1;
	CSoundPatch* m_pEngineSound2;
	CSoundPatch* m_pBladeSound;

	int				m_nEnginePitch1;
	int				m_nEnginePitch2;
	float			m_flEnginePitch1Time;
	float			m_flEnginePitch2Time;
};
