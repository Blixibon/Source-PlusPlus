#ifndef C_BASECOLLEGUE_H
#define C_BASECOLLEGUE_H
#include "c_ai_basenpc.h"
#include "bms_utils.h"
#include "character_manifest_shared.h"
#ifdef _WIN32
#pragma once  
#endif // _WIN32

#define CNPC_BaseColleague C_NPC_BaseColleague

class C_NPC_BaseColleague : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_NPC_BaseColleague, C_AI_BaseNPC);
	DECLARE_NETWORKCLASS();
public:
	C_NPC_BaseColleague()
	{
		m_nOldFlexTableIndex = -1;
	}

	virtual void		ProcessSceneEvents(bool bFlexEvents);
	virtual CStudioHdr *OnNewModel(void);
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual C_BaseAnimating* CreateRagdollCopy();

protected:
	CUtlVector<ManifestFlexData_t> m_FlexData;
	CUtlVector<LocalFlexController_t> m_FlexControllers;
	int m_nFlexTableIndex;
	int m_nOldFlexTableIndex;
};

#endif // !C_BASECOLLEGUE_H