#ifndef BASECOLLEGUE_H
#define BASECOLLEGUE_H

#include "ai_behavior_functank.h"
#include "peter/npc_playerfollower.h"
#include "bms_utils.h"

#define SF_COLLEGUE_DONT_FOLLOW			( 1 << 16 )	//65536
#define SF_COLLEAGUE_NO_IDLE_SPEAK 1048576

//-----------------------------------------------------------------------------
// Citizen expression types
//-----------------------------------------------------------------------------
enum ColleagueExpressionTypes_t
{
	COL_EXP_UNASSIGNED,	// Defaults to this, selects other in spawn.

	COL_EXP_SCARED,
	COL_EXP_NORMAL,
	COL_EXP_ANGRY,

	COL_EXP_LAST_TYPE,
};

typedef struct
{
	const char *pszCleanModel;
	const char *pszHurtModel;
} colleagueModel_t;

class CNPC_BaseColleague : public CNPC_PlayerFollower
{
public:
	DECLARE_CLASS(CNPC_BaseColleague, CNPC_PlayerFollower);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void		Precache();
	void			SelectExpressionType();

	virtual void ProcessSceneEvents(void);
	virtual CStudioHdr *OnNewModel(void);

	void		Spawn()
	{
		SetFollowerBaseUse(&CNPC_BaseColleague::UseFunc);

		BaseClass::Spawn();
	}
	

	int				DrawDebugTextOverlays(void);
	virtual const char *SelectRandomExpressionForState(NPC_STATE state);

	Disposition_t	IRelationType(CBaseEntity *pTarget);

	bool CreateBehaviors(void);
	void OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior);

	void UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool	CanJoinPlayerSquad(CBasePlayer *pPlayer = nullptr);
	

	Activity		NPC_TranslateActivity(Activity eNewActivity);

	void GatherConditions();
	

	void		ModifyOrAppendCriteria(AI_CriteriaSet& set);
	virtual bool SelectIdleSpeech(AISpeechSelection_t *pSelection);

	

	void			OnRestore()
	{
		BaseClass::OnRestore();

		if (GetModelPtr() != NULL)
		{
			for (int i = 0; i < NUM_RND_HEAD_FLEXES; i++)
			{
				m_HeadFlxs[i] = FindFlexController(g_szRandomFlexControls[i]);
			}
		}
	}

	

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	

	void	InputEnableFollow(inputdata_t &inputdata);
	void	InputDisableFollow(inputdata_t &inputdata);
	void	InputStopFollow(inputdata_t &inputdata);
	void	InputStartFollow(inputdata_t &inputdata);

	void	InputEnableIdleSpeak(inputdata_t &inputdata);
	void	InputDisableIdleSpeak(inputdata_t &inputdata);

	

	static const char *ChooseColleagueModel(colleagueModel_t mdl[]);

protected:
	CNetworkVar(int, m_iHeadRndSeed);
	LocalFlexController_t m_HeadFlxs[NUM_RND_HEAD_FLEXES];
	float m_HeadFlxWgts[NUM_RND_HEAD_FLEXES];

	ColleagueExpressionTypes_t	m_ExpressionType;
};



#endif // !BASECOLLEGUE_H

