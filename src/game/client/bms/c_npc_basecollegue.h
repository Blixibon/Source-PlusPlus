#ifndef C_BASECOLLEGUE_H
#define C_BASECOLLEGUE_H
#include "c_ai_basenpc.h"
#include "bms_utils.h"
#ifdef _WIN32
#pragma once  
#endif // _WIN32

#define CNPC_BaseColleague C_NPC_BaseColleague

class C_NPC_BaseColleague : public C_AI_BaseNPC
{
	DECLARE_CLASS(C_NPC_BaseColleague, C_AI_BaseNPC);
	DECLARE_NETWORKCLASS();
public:
	virtual void		ProcessSceneEvents(bool bFlexEvents);
	virtual CStudioHdr *OnNewModel(void);
	virtual void	OnDataChanged(DataUpdateType_t updateType);

protected:
	int		m_iHeadRndSeed;
	int		m_iOldRndSeed;
	LocalFlexController_t m_HeadFlxs[NUM_RND_HEAD_FLEXES];
	float m_HeadFlxWgts[NUM_RND_HEAD_FLEXES];
};

#endif // !C_BASECOLLEGUE_H