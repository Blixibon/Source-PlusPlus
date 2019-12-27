#include "cbase.h"
#include "player_models.h"
#include "KeyValues.h"
#include "gametypes.h"
#include "filesystem.h"
#include "props.h"

bool CPlayerModels::Init()
{
	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char* fileName = "scripts/player_models/*.vdf";
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "GAME", &findHandle);
	while (fileName)
	{
		char path[MAX_PATH];
		Q_snprintf(path, sizeof(path), "scripts/player_models/%s", fileName);

		LoadModelsFromFile(path);

		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);

	return true;
}

bool CPlayerModels::LoadModelsFromFile(const char* szFilename)
{
	KeyValues *pKV = new KeyValues("Models");
	if (pKV->LoadFromFile(filesystem, szFilename, "GAME"))
	{
		for (KeyValues * pkvModel = pKV->GetFirstTrueSubKey(); pkvModel != NULL; pkvModel = pkvModel->GetNextTrueSubKey())
		{
			m_Models.AddToTail();
			playerModel_t &player = m_Models.Tail();
			Q_strncpy(player.szSectionID, pkvModel->GetName(), 32);
			player.reqs.singleplayer.bSuit = pkvModel->GetBool("needs_suit", false);
			KeyValues *pkvGames = pkvModel->FindKey("games");
			if (pkvGames)
			{
				for (KeyValues * pkvGame = pkvGames->GetFirstSubKey(); pkvGame != NULL; pkvGame = pkvGame->GetNextKey())
				{
					player.reqs.singleplayer.games.AddToTail(g_pGameTypeSystem->LookupGametype(pkvGame->GetString()));
				}
			}

			Q_strncpy(player.reqs.multiplayer.szTeam, pkvModel->GetString("team"), 32);
			player.reqs.multiplayer.iMaxNum = pkvModel->GetInt("maxplayers", 0);

			KeyValues *pkvDef = pkvModel->FindKey("models");
			if (pkvDef)
			{
				for (KeyValues* pkvModelDef = pkvDef->GetFirstTrueSubKey(); pkvModelDef != NULL; pkvModelDef = pkvModelDef->GetNextTrueSubKey())
				{
					rndModel_t& hModel = player.models.Element(player.models.AddToTail());
					Q_strncpy(hModel.szModelName, pkvModelDef->GetString("name"), MAX_PATH);
					//hModel.skin = pkvModelDef->GetInt("skin");
					if (V_strrchr(pkvModelDef->GetString("skin"), ' ') != 0)
					{
						int iBodies[2];
						UTIL_StringToIntArray(iBodies, 2, pkvModelDef->GetString("skin"));
						hModel.skin = iBodies[0];
						hModel.skinMax = iBodies[1];
					}
					else
					{
						hModel.skin = hModel.skinMax = pkvModelDef->GetInt("skin");
					}

					KeyValues* pkvBodyGroups = pkvModelDef->FindKey("bodygroups");
					if (pkvBodyGroups)
					{
						for (KeyValues* pkvGroup = pkvBodyGroups->GetFirstValue(); pkvGroup != NULL; pkvGroup = pkvGroup->GetNextValue())
						{
							hModel.bodygroups.AddToTail();
							bodygroup_s& body = hModel.bodygroups.Tail();
							Q_strncpy(body.szName, pkvGroup->GetName(), 32);
							if (V_strrchr(pkvGroup->GetString(), ' ') != 0)
							{
								int iBodies[2];
								UTIL_StringToIntArray(iBodies, 2, pkvGroup->GetString());
								body.body = iBodies[0];
								body.bodyMax = iBodies[1];
							}
							else
							{
								body.body = body.bodyMax = pkvGroup->GetInt();
							}
						}
					}
				}
			}

			KeyValues *pkvHands = pkvModel->FindKey("hands");
			if (pkvHands)
			{
				Q_strncpy(player.szArmModel, pkvHands->GetString("model"), MAX_PATH);
				player.armSkin = pkvHands->GetInt("skin");
				KeyValues *pkvBodyGroups = pkvHands->FindKey("bodygroups");
				if (pkvBodyGroups)
				{
					for (KeyValues * pkvGroup = pkvBodyGroups->GetFirstSubKey(); pkvGroup != NULL; pkvGroup = pkvGroup->GetNextKey())
					{
						player.armbodys.AddToTail();
						bodygroup_s &body = player.armbodys.Tail();
						Q_strncpy(body.szName, pkvGroup->GetName(), 32);
						body.body = pkvGroup->GetInt();
					}
				}
			}

			KeyValues *pkvAbilities = pkvModel->FindKey("abilities");
			if (pkvAbilities)
			{
				player.kvAbilities = pkvAbilities->MakeCopy();
			}
			else
			{
				player.kvAbilities = nullptr;
			}
		}
	}

	pKV->deleteThis();

	return true;
}

void CPlayerModels::Shutdown()
{
	/*for (int i = 0; i < m_Models.Count(); i++)
	{
		if (m_Models[i].kvAbilities != NULL)
		{
			m_Models[i].kvAbilities->deleteThis();
			m_Models[i].kvAbilities = NULL;
		}
	}*/

	m_Models.Purge();
}

void CPlayerModels::LevelInitPreEntity()
{
	FOR_EACH_VEC(m_Models, i)
	{
		FOR_EACH_VEC(m_Models[i].models, j)
		{
			CBaseEntity::PrecacheModel(m_Models[i].models[j].szModelName);
			PropBreakablePrecacheAll(AllocPooledString(m_Models[i].models[j].szModelName));
		}
		CBaseEntity::PrecacheModel(m_Models[i].szArmModel);
		m_Models[i].iRefCount = 0;
	}
}

ConVar force_pmodel("hlms_forced_player", "", FCVAR_CHEAT, "Forces the game to use a specific player model in singleplayer.");

playerModel_t *CPlayerModels::SelectPlayerModel(int iGame, bool bSuit)
{
	int iBestIndex = 0;
	bool bBestTestedGames = false;
	bool bBestNeedsSuit = false;
	bool bForceModel = (0 != Q_strcmp(force_pmodel.GetString(), ""));

	for (int i = 0; i < (m_Models).Count(); i++)
	{
		if (bForceModel)
		{
			if (0 == Q_strcmp(m_Models[i].szSectionID, force_pmodel.GetString()))
			{
				iBestIndex = i;
				break;
			}
		}

		bool bGameValid = true;
		bool bTestedGames = false;
		if (m_Models[i].reqs.singleplayer.games.Count() > 0)
		{
			bGameValid = false;
			bTestedGames = true;
			for (int j = 0; j < m_Models[i].reqs.singleplayer.games.Count(); j++)
			{
				if (m_Models[i].reqs.singleplayer.games[j] == iGame)
					bGameValid = true;
			}
		}

		if (!bGameValid)
			continue;

		if (m_Models[i].reqs.singleplayer.bSuit && !bSuit)
			continue;

		if (bBestTestedGames)
		{
			if (!bTestedGames)
				continue;

			if (bBestNeedsSuit && !m_Models[i].reqs.singleplayer.bSuit)
				continue;
		}
		else
		{
			if (!bTestedGames)
			{
				if (bBestNeedsSuit && !m_Models[i].reqs.singleplayer.bSuit)
					continue;
			}
		}

		/*if (bBestTestedGames && !bTestedGames)
			continue;

		if (bBestTestedGames && bBestNeedsSuit && !m_Models[i].reqs.bSuit)
			continue;*/


		iBestIndex = i;
		bBestTestedGames = bTestedGames;
		bBestNeedsSuit = m_Models[i].reqs.singleplayer.bSuit;
	}

	m_Selected = m_Models[iBestIndex];

	return &m_Selected;
}

CUtlVector<playerModel_t> CPlayerModels::GetAvailableModelsForTeam(const char * pszTeam)
{
	CUtlVector<playerModel_t> candidates;

	for (int i = 0; i < (m_Models).Count(); i++)
	{
		if (FStrEq(m_Models[i].reqs.multiplayer.szTeam, pszTeam))
		{
			if (m_Models[i].reqs.multiplayer.iMaxNum == 0 || m_Models[i].iRefCount < m_Models[i].reqs.multiplayer.iMaxNum)
			{
				candidates.AddToTail(m_Models[i]);
			}
		}
	}

	return candidates;
}

playerModel_t CPlayerModels::FindPlayerModel(const char* pszName)
{
	for (auto model : m_Models)
	{
		if (FStrEq(model.szSectionID, pszName))
			return model;
	}

	return playerModel_t();
}

bool CPlayerModels::PlayerGrabModel(const char * pszName)
{
	for (int i = 0; i < (m_Models).Count(); i++)
	{
		if (FStrEq(m_Models[i].szSectionID, pszName))
		{
			m_Models[i].iRefCount++;
			if (m_Models[i].reqs.multiplayer.iMaxNum > 0)
				Assert(m_Models[i].iRefCount <= m_Models[i].reqs.multiplayer.iMaxNum);
			break;
		}
	}

	return true;
}

bool CPlayerModels::PlayerReleaseModel(const char * pszName)
{
	for (int i = 0; i < (m_Models).Count(); i++)
	{
		if (FStrEq(m_Models[i].szSectionID, pszName))
		{
			m_Models[i].iRefCount--;
			Assert(m_Models[i].iRefCount >= 0);
			break;
		}
	}

	return true;
}

CPlayerModels g_PlayerModels;
CPlayerModels* g_pPlayerModels = &g_PlayerModels;

CPlayerModels* PlayerModelSystem()
{
	return &g_PlayerModels;
}
