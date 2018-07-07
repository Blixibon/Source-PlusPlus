//=============================================================================//
//
// Purpose: difficulty test ent
//
//=============================================================================//

#include "cbase.h"
#include "gamerules.h"

class CLogicDifficultyTest : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicDifficultyTest , CLogicalEntity );
	DECLARE_DATADESC();

	CLogicDifficultyTest ( void ) {}

	void InputTestDifficulty( inputdata_t &inputData );

private:

	COutputEvent	m_onEasy;	
	COutputEvent	m_onNormal;	
	COutputEvent	m_onHard;	
	//COutputEvent	m_onDeathWish;
};

LINK_ENTITY_TO_CLASS( logic_difficultytest, CLogicDifficultyTest  );

BEGIN_DATADESC( CLogicDifficultyTest  )

	DEFINE_INPUTFUNC( FIELD_VOID, "TestDifficulty", InputTestDifficulty ),

	DEFINE_OUTPUT(m_onEasy, "OnEasy"),
	DEFINE_OUTPUT(m_onNormal, "OnNormal"),
	DEFINE_OUTPUT(m_onHard, "OnHard"),
	//DEFINE_OUTPUT(m_onDeathWish, "OnExpert"),

END_DATADESC()

void CLogicDifficultyTest::InputTestDifficulty( inputdata_t &inputData )
{
	DevMsg("Testing difficulty!\n");
	CGameRules *pRules = g_pGameRules;

	switch (pRules->GetSkillLevel())
	{
	default: DevMsg("Error, unable to tell what difficulty level we are!\n"); break;
	case SKILL_EASY: m_onEasy.FireOutput(inputData.pActivator, this); DevMsg("Difficulty is easy!\n");  break;
	case SKILL_MEDIUM: m_onNormal.FireOutput(inputData.pActivator, this); DevMsg("Difficulty is normal!\n"); break;
	case SKILL_HARD: m_onHard.FireOutput(inputData.pActivator, this); DevMsg("Difficulty is hard!\n"); break;
	//case SKILL_DEATHWISH: m_onDeathWish.FireOutput(inputData.pActivator, this);  DevMsg("Difficulty is expert!\n"); break;
	}
	DevMsg("Done testing!\n");
}