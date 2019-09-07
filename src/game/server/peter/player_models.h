#ifndef PLAYER_MODELS_H
#define PLAYER_MODELS_H

#include "igamesystem.h"
#include "utlvector.h"
#include "utlbuffer.h"

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
		byte iMaxNum;
	} multiplayer;
} requirement_t;

typedef struct rndModel_s
{
	char szModelName[MAX_PATH];
	short skin;
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
	//KeyValues *kvAbilities = NULL;
	CUtlBuffer bufKVAbilities;
	bool bKVAbilities = true;

	playerModel_s &operator=(const playerModel_s&);
	playerModel_s(const playerModel_s& other) { *this = other; }
	playerModel_s()
	{
		Clear();
	}

	void Clear()
	{
		szSectionID[0] = 0;
		szArmModel[0] = 0;
		armSkin = 0;
		bKVAbilities = true;
		iRefCount = 0;

		models.Purge();
		armbodys.Purge();
		bufKVAbilities.Clear();

		reqs = { 0 };
	}

	// Caller is responsible for deletion
	KeyValues* GetAbilities();
	KeyValues* GetAbilities() const { return const_cast<playerModel_s*> (this)->GetAbilities(); }

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

