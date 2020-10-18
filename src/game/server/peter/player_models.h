#ifndef PLAYER_MODELS_H
#define PLAYER_MODELS_H

#include "igamesystem.h"
#include "utlvector.h"

namespace PlayerModels
{
	DECLARE_PRIVATE_SYMBOLTYPE(CPlayerModelSymbol);
	DECLARE_PRIVATE_FILENAME_SYMBOLTYPE(CPlayerModelFilename);

	typedef struct bodygroup_s
	{
		CPlayerModelSymbol strName;
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
			CPlayerModelSymbol strTeam;
			int iMaxNum;
		} multiplayer;
	} requirement_t;

	typedef struct rndModel_s
	{
		CPlayerModelFilename hPlayerModelName;
		short skin;
		short skinMax;
		CCopyableUtlVector<bodygroup_t> bodygroups;

		CPlayerModelFilename hArmModelName;
		short armSkin;
		CCopyableUtlVector<bodygroup_t> armbodys;
	} rndModel_t;

	typedef struct playerModel_s
	{
		CPlayerModelSymbol strModelID;
		CCopyableUtlVector<rndModel_t> models;
		requirement_t reqs;
		KeyValues* kvAbilities = NULL;
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

		playerModel_t* SelectPlayerModel(int iGame, bool bSuit);

		CUtlVector<playerModel_t> GetAvailableModelsForTeam(const char* pszTeam);

		playerModel_t FindPlayerModel(const char* pszName);

		bool PlayerGrabModel(CPlayerModelSymbol strName);
		bool PlayerReleaseModel(CPlayerModelSymbol strName);

	protected:
		bool		LoadModelsFromFile(const char* szFilename);
		CUtlVector<playerModel_t> m_Models;
		playerModel_t m_Selected;
	};

	extern CPlayerModels* g_pPlayerModels;
}

PlayerModels::CPlayerModels* PlayerModelSystem();

#endif // !PLAYER_MODELS_H

