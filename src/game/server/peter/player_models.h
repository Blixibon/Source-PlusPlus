#ifndef PLAYER_MODELS_H
#define PLAYER_MODELS_H

#include "igamesystem.h"
#include "utlvector.h"

typedef struct bodygroup_s
{
	char szName[32];
	short body;
	short bodyMax;
} bodygroup_t;

typedef struct requirement_s
{
	struct {
		bool bSuit;
		CCopyableUtlVector<int> games;
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
	short skinMax;
	CCopyableUtlVector<bodygroup_t> bodygroups;
} rndModel_t;

typedef struct playerModel_s
{
	char szSectionID[32];
	CCopyableUtlVector<rndModel_t> models;
	char szArmModel[MAX_PATH];
	short armSkin;
	CCopyableUtlVector<bodygroup_t> armbodys;
	requirement_t reqs;
	KeyValues *kvAbilities = NULL;
	/*~playerModel_s()
	{
		if (kvAbilities != NULL)
			kvAbilities->deleteThis();
	}*/

	int iRefCount;
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

	CUtlVector<playerModel_t> GetAvailableModelsForTeam(const char *pszTeam);

	playerModel_t FindPlayerModel(const char* pszName);

	bool PlayerGrabModel(const char *pszName);
	bool PlayerReleaseModel(const char *pszName);

protected:
	bool		LoadModelsFromFile(const char* szFilename);
	CUtlVector<playerModel_t> m_Models;
	playerModel_t m_Selected;
};

CPlayerModels* PlayerModelSystem();
extern CPlayerModels* g_pPlayerModels;

#endif // !PLAYER_MODELS_H

