#ifndef BASECOLLEGUE_H
#define BASECOLLEGUE_H

#include "ai_behavior_functank.h"
#include "peter/npc_combatsupplier.h"
#include "bms_utils.h"
#include "character_manifest_system.h"

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

class CNPC_BaseColleague : public CNPC_CombatSupplier
{
public:
	DECLARE_CLASS(CNPC_BaseColleague, CNPC_CombatSupplier);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void		Precache();
	void			SelectExpressionType();

	virtual void ProcessSceneEvents(void);
	virtual CStudioHdr *OnNewModel(void);

	virtual const char* GetGrenadeClassThrown() { return "npc_grenade_frag_bms"; }

	void		Spawn()
	{
		SetFollowerBaseUse(&CNPC_BaseColleague::UseFunc);

		BaseClass::Spawn();

		if (m_pCharacterDefinition)
		{
			SetupModelFromManifest();
			m_pCharacterDefinition = nullptr;
		}
	}
	
	void			SetupModelFromManifest();

	int				DrawDebugTextOverlays(void);
	virtual const char *SelectRandomExpressionForState(NPC_STATE state);

	Disposition_t	IRelationType(CBaseEntity *pTarget);

	bool CreateBehaviors(void);
	void OnChangeRunningBehavior(CAI_BehaviorBase *pOldBehavior, CAI_BehaviorBase *pNewBehavior);

	virtual void UseFunc(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual bool	CanJoinPlayerSquad(CBasePlayer *pPlayer = nullptr);
	
	virtual bool	IgnorePlayerPushing(void);

	Activity		NPC_TranslateActivity(Activity eNewActivity);

	void GatherConditions();
	

	void		ModifyOrAppendCriteria(AI_CriteriaSet& set);
	virtual bool SelectIdleSpeech(AISpeechSelection_t *pSelection);

	

	void			OnRestore();

	

	CAI_FuncTankBehavior		m_FuncTankBehavior;
	

	void	InputEnableFollow(inputdata_t &inputdata);
	void	InputDisableFollow(inputdata_t &inputdata);
	void	InputStopFollow(inputdata_t &inputdata);
	void	InputStartFollow(inputdata_t &inputdata);

	void	InputEnableIdleSpeak(inputdata_t &inputdata);
	void	InputDisableIdleSpeak(inputdata_t &inputdata);

	virtual ResponseRules::IResponseSystem* GetResponseSystem() { return m_pInstancedResponseSystem; }

protected:
	CUtlVector<ManifestFlexData_t> m_FlexData;
	CUtlVector<LocalFlexController_t> m_FlexControllers;
	CNetworkVar(int, m_nFlexTableIndex);

	ColleagueExpressionTypes_t	m_ExpressionType;

	const CharacterManifest::ManifestCharacter_t* m_pCharacterDefinition; // Only valid during spawn

	ResponseRules::IResponseSystem* m_pInstancedResponseSystem;
};



#endif // !BASECOLLEGUE_H

