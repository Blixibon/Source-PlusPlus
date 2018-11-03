#include "cbase.h"
#include "gametypes.h"
#include "KeyValues.h"
#include "filesystem.h"




CGameTypeManager g_GTManager;
CGameTypeManager *g_pGameTypeSystem = &g_GTManager;


//CUtlVectorAutoPurge<char *> vecGames;

bool CGameTypeManager::Init()
{
	char *pchAreaNames[MAX_CODE_AREAS] = {
		"",
		"trainstation",
		"canals",
		"zombietown",
		"coast",
		"prison",
		"c17",
		"citadel",
		"outlands",
		"blackmesa",
		"xen",
	};

	m_vecAreaNames.CopyArray(pchAreaNames, MAX_CODE_AREAS);

	/*GameWord_t gwCodedGames[MAX_CODE_GAMETYPES] =
	{
		{ "hl1" , GAME_HL1 },
		{ "hl2" , GAME_HL2 },
		{ "ep1" , GAME_EP1 },
		{ "ep2" , GAME_EP2 },
		{ "bms" , MOD_BMS },
		{ "portal" , GAME_PORTAL}
	};*/

	char *gwCodedGames[MAX_CODE_GAMETYPES] =
	{
		"default",
		"hl1",
		"hl2",
		"ep1",
		"ep2",
		"citizen",
		"citizen2",
		"bms",
		"portal",
		"hl2beta"
	};

	m_vecGames.CopyArray(gwCodedGames, MAX_CODE_GAMETYPES);

	KeyValues *pKV = new KeyValues("MapPrefixes");

	if (pKV->LoadFromFile(filesystem, "scripts/map_prefixes.txt", "MOD"))
	{
		KeyValues *pkvPrefixes = pKV->FindKey("Prefixes");
		if (pkvPrefixes)
		{
			KeyValues *pkvNode = pkvPrefixes->GetFirstSubKey();
			while (pkvNode)
			{
				RegisterPrefix(pkvNode);

				pkvNode = pkvNode->GetNextKey();
			}
		}
		KeyValues *pkvAreas = pKV->FindKey("Areas");
		if (pkvAreas)
		{
			KeyValues *pkvNode = pkvAreas->GetFirstSubKey();
			while (pkvNode)
			{
				AreaName_t *pNewAreaDef = new AreaName_t;

				Q_strncpy(pNewAreaDef->substring, pkvNode->GetName(), sizeof(pNewAreaDef->substring));

				//int id = 0;
				const char *pchString = pkvNode->GetString();
				for (int i = 0; i < m_vecAreaNames.Count(); i++)
				{
					if (0 == Q_stricmp(m_vecAreaNames[i], pchString))
					{
						pNewAreaDef->iArea = i;
						break;
					}
				}
				m_AreaNameVector.AddToTail(pNewAreaDef);

				pkvNode = pkvNode->GetNextKey();
			}
		}
	}
	/*else
	{
		RegisterPrefix("bm_", MOD_BMS);
		RegisterPrefix("d1_", GAME_HL2);
		RegisterPrefix("d2_", GAME_HL2);
		RegisterPrefix("d3_", GAME_HL2);
		RegisterPrefix("ep1_", GAME_EP1);
		RegisterPrefix("ep2_", GAME_EP2);
		RegisterPrefix("xen_", MOD_BMS);
		RegisterPrefix("c", GAME_HL1);
		RegisterPrefix("testchmb_", GAME_PORTAL);
		RegisterPrefix("escape_", GAME_PORTAL);
		RegisterPrefix("sp_", GAME_EP2);
		RegisterPrefix("metastasis_", GAME_EP1);
	}*/

	pKV->deleteThis();

	
	Msg("CGameTypeManager: Registered %i map prefixes.\n", m_PrefixVector.Count());

	return true;
}

bool CGameTypeManager::IsMapInGame(const char *pchGame)
{
	return (0 == Q_strcmp(pchGame, m_vecGames.Element(GetCurrentGameType())));
}

int CGameTypeManager::LookupGametype(const char *pchGame)
{
	FOR_EACH_VEC(m_vecGames, i)
	{
		if (0 == Q_strcmp(pchGame, m_vecGames.Element(i)))
			return i;
	}

	return GAME_INVALID;
}

void CGameTypeManager::Reload()
{
	m_PrefixVector.PurgeAndDeleteElements();
	/*m_vecGames.Purge();

	GameWord_t gwCodedGames[MAX_CODE_GAMETYPES] =
	{
		{ "hl1", GAME_HL1 },
		{ "hl2", GAME_HL2 },
		{ "ep1", GAME_EP1 },
		{ "ep2", GAME_EP2 },
		{ "bms", MOD_BMS },
		{ "portal", GAME_PORTAL }
	};

	m_vecGames.CopyArray(gwCodedGames, MAX_CODE_GAMETYPES);*/

	KeyValues *pKV = new KeyValues("MapPrefixes");

	if (pKV->LoadFromFile(filesystem, "scripts/map_prefixes.txt", "MOD"))
	{
		KeyValues *pkvPrefixes = pKV->FindKey("Prefixes");
		if (pkvPrefixes)
		{
			KeyValues *pkvNode = pkvPrefixes->GetFirstSubKey();
			while (pkvNode)
			{
				RegisterPrefix(pkvNode);

				pkvNode = pkvNode->GetNextKey();
			}
		}
	}
	/*else
	{
		RegisterPrefix("bm_", MOD_BMS);
		RegisterPrefix("d1_", GAME_HL2);
		RegisterPrefix("d2_", GAME_HL2);
		RegisterPrefix("d3_", GAME_HL2);
		RegisterPrefix("ep1_", GAME_EP1);
		RegisterPrefix("ep2_", GAME_EP2);
		RegisterPrefix("xen_", MOD_BMS);
		RegisterPrefix("c", GAME_HL1);
		RegisterPrefix("testchmb_", GAME_PORTAL);
		RegisterPrefix("escape_", GAME_PORTAL);
		RegisterPrefix("sp_", GAME_EP2);
		RegisterPrefix("metastasis_", GAME_EP1);
	}*/

	pKV->deleteThis();

	Msg("CGameTypeManager: Registered %i map prefixes.\n", m_PrefixVector.Count());
}

//CON_COMMAND_F(gametype_reload, "Reloads TypeManager's prefixes.\n", FCVAR_CHEAT)
//{
//	g_pGameTypeSystem->Reload();
//}

CON_COMMAND(gametype_print, "Prints info\n")
{
	int iPrefixes = g_pGameTypeSystem->m_PrefixVector.Count();
	int iGames = g_pGameTypeSystem->m_vecGames.Count();
	int i = 0;
	Msg("Registered Games:\n");
	for (i = 0; i < iGames; i++)
	{
		Msg("	%i: %s\n", i, g_pGameTypeSystem->m_vecGames[i]);
	}
	Msg("Registered Prefixes:\n");
	for (i = 0; i < iPrefixes; i++)
	{
		if (g_pGameTypeSystem->m_PrefixVector[i]->Type < iGames)
			Msg("	%s: %s\n", g_pGameTypeSystem->m_PrefixVector[i]->prefix, g_pGameTypeSystem->m_vecGames[g_pGameTypeSystem->m_PrefixVector[i]->Type]);
		else
			Msg("	%s: %i\n", g_pGameTypeSystem->m_PrefixVector[i]->prefix, (int)g_pGameTypeSystem->m_PrefixVector[i]->Type);
	}
	Msg("Valid Areas:\n");
	FOR_EACH_VEC(g_pGameTypeSystem->m_vecAreaNames, i)
	{
		char *pchArea = g_pGameTypeSystem->m_vecAreaNames[i];
		if (pchArea && pchArea[0])
		{
			Msg("	%s\n", pchArea);
		}
	}
	Msg("Current Areas:\n");
	FOR_EACH_VEC(g_pGameTypeSystem->m_vecAreaNames, i)
	{
		char *pchArea = g_pGameTypeSystem->m_vecAreaNames[i];
		if (pchArea && pchArea[0])
		{
			if (g_pGameTypeSystem->IsMapInArea(i))
			{
				Msg("	%s\n", pchArea);
			}
		}
	}
}

void CGameTypeManager::RegisterPrefix(KeyValues *pkvNode)
{
	MapPrefix_t *pPrefix = new MapPrefix_t();

	if (pkvNode->GetDataType() == KeyValues::TYPE_INT)
	{
		pPrefix->Type = (GameType)pkvNode->GetInt();
		Q_strncpy(pPrefix->prefix, pkvNode->GetName(), sizeof(pPrefix->prefix));
	}
	else if (pkvNode->GetDataType() == KeyValues::TYPE_STRING || pkvNode->GetDataType() == KeyValues::TYPE_WSTRING)
	{
		Q_strncpy(pPrefix->prefix, pkvNode->GetName(), sizeof(pPrefix->prefix));

		GameType type = GAME_INVALID;
		const char *pchString = pkvNode->GetString();
		for (int i = 0; i < m_vecGames.Count(); i++)
		{
			if (0 == Q_stricmp(m_vecGames[i], pchString))
			{
				type = (GameType)i;
				break;
			}			
		}
		if (type == GAME_INVALID)
		{
			//GameWord_t gwNew;
			
			type = (GameType)m_vecGames.Count();
			
			m_vecGames.CopyAndAddToTail(pchString);
			//Q_strncpy(m_vecGames[(int)type], pchString, sizeof(m_vecGames[(int)type]));

			Msg("CGameTypeManager: Registered custom game %s with index %i.\n", m_vecGames[(int)type], (int)type);
		}

		pPrefix->Type = type;
	}
	else
	{
		delete pPrefix;
		return;
	}

	m_PrefixVector.AddToTail(pPrefix);
}



void CGameTypeManager::LevelInitPreEntity()
{
	m_bitAreas.ClearAll();
	m_iFirstArea = AREA_NONE;
	char szMapName[256];
	Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName));
	Q_strlower(szMapName);
	FOR_EACH_VEC(m_AreaNameVector, i)
	{
		if (Q_stristr(szMapName, m_AreaNameVector[i]->substring))
		{
			AddAreaToMap(m_AreaNameVector[i]->iArea);
		}
	}

	//--Find current game
	struct PrefixCandidate
	{
		int Type;
		int Priority;
	};

	CUtlVector<PrefixCandidate> vecCandidates;

	for (int i = 0; i < m_PrefixVector.Count(); i++)
	{
		MapPrefix_t *pPrefix = m_PrefixVector.Element(i);
		char *pchPrefix = pPrefix->prefix;
		int iType = (int)pPrefix->Type;
		int iSize = Q_strlen(pchPrefix);

		if (Q_strncmp(pchPrefix, szMapName, iSize) == 0)
		{
			PrefixCandidate cand;
			cand.Type = iType;
			cand.Priority = iSize;

			vecCandidates.AddToTail(cand);
		}
	}

	if (vecCandidates.Count() == 0)
	{
		m_iGameType = GAME_DEFAULT;
		return;
	}

	if (vecCandidates.Count() == 1)
	{
		m_iGameType = (GameType)vecCandidates.Head().Type;
	}
	else
	{
		GameType BestType = GAME_DEFAULT;
		int iBestPriority = 0;
		for (int i = 0; i < vecCandidates.Count(); i++)
		{
			PrefixCandidate Candidate = vecCandidates.Element(i);

			if (iBestPriority > Candidate.Priority)
				continue;

			iBestPriority = Candidate.Priority;
			BestType = (GameType)Candidate.Type;
		}

		m_iGameType = BestType;
	}
}

ConVar sv_gametype("sv_force_gametype", "-1", FCVAR_CHEAT);

GameType CGameTypeManager::GetCurrentGameType()
{
	if (sv_gametype.GetInt() >= 0 && sv_gametype.GetInt() < m_vecGames.Count())
		return (GameType)sv_gametype.GetInt();

	return m_iGameType;
}