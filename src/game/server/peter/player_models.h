#ifndef PLAYER_MODELS_H
#define PLAYER_MODELS_H

#include "igamesystem.h"
#include "utlvector.h"

typedef struct bodygroup_s
{
	char szName[32];
	short body;
} bodygroup_t;

typedef struct requirement_s
{
	struct {
		bool bSuit;
		CUtlVector<int> games;
	} singleplayer;

	struct {
		char szTeam[32];
		int iMaxNum;
	} multiplayer;
} requirement_t;

typedef struct rndModel_s
{
	char szModelName[MAX_PATH];
	short skin;
	CUtlVector<bodygroup_t> bodygroups;
} rndModel_t;

typedef struct playerModel_s
{
	char szSectionID[32];
	CUtlVector<rndModel_t> models;
	char szArmModel[MAX_PATH];
	short armSkin;
	CUtlVector<bodygroup_t> armbodys;
	requirement_t reqs;
	KeyValues *kvAbilities = NULL;
	/*~playerModel_s()
	{
		if (kvAbilities != NULL)
			kvAbilities->deleteThis();
	}*/
} playerModel_t;

class CPlayerModels : public CBaseGameSystem
{
	DECLARE_CLASS_GAMEROOT(CPlayerModels, CBaseGameSystem);
public:
	CPlayerModels() : CBaseGameSystem()
	{}

	virtual char const* Name() { return "CPlayerModels"; }

	virtual bool Init();
	virtual void LevelInitPreEntity();
	virtual void Shutdown();

	playerModel_t *SelectPlayerModel(int iGame, bool bSuit);

protected:
	bool		LoadModelsFromFile(const char* szFilename);
	CUtlVector<playerModel_t> m_Models;
	playerModel_t m_Selected;
};

CPlayerModels* PlayerModelSystem();
extern CPlayerModels* g_pPlayerModels;

#endif // !PLAYER_MODELS_H

