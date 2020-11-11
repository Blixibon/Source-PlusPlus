#include "cbase.h"
#include "gametypes.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "utlbuffer.h"


IMPLEMENT_PRIVATE_SYMBOLTYPE(CGTSymbol);

bool GTSLessFunc(const CGTSymbol&a, const CGTSymbol&b)
{
	return CaselessStringLessThan(a.String(), b.String());
}

CGameTypeManager g_GTManager;

CGameTypeManager* GameTypeSystem()
{
	return &g_GTManager;
}

CUtlFilenameSymbolTable CGameTypeManager::sm_FilenameTable;

BEGIN_SCRIPTDESC_ROOT(CGameTypeManager, SCRIPT_SINGLETON "Information about the currently running map")
DEFINE_SCRIPTFUNC(HasMapOption, "Given a key name, return if that option exists")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetMapOptionInt, "GetMapOptionInt", "Given a key name, return the integer value")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetMapOptionInt64, "GetMapOptionInt64", "Given a key name, return the 64-bit integer value")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetMapOptionFloat, "GetMapOptionFloat", "Given a key name, return the float value")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetMapOptionString, "GetMapOptionString", "Given a key name, return the string value")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetMapOptionBool, "GetMapOptionBool", "Given a key name, return the boolean value")

DEFINE_SCRIPTFUNC_NAMED(ScriptGetBaseGameType, "GetBaseGameType", "Returns the name of the base gametype")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetModGameType, "GetModGameType", "Returns the name of the mod gametype")
DEFINE_SCRIPTFUNC(GetPopulationLocation, "Returns the current population tag")

DEFINE_SCRIPTFUNC_NAMED(ScriptRemapEntityClass, "RemapEntityClass", "Returns the mod-specific classname to use")
END_SCRIPTDESC();

CGameTypeManager::CGameTypeManager() : m_NewMapConfigs(GTSLessFunc), m_NewGameConfigs(GTSLessFunc)
{
}

#define IF_PARAMS_VALID(params, i) if (params.Count() < i + 1) { Warning("Command must have %d parameters.\n", i); } else

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
		"hl2beta",
		"human",
		"entropy"
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

				pNewAreaDef->substring = pkvNode->GetName();

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

	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char* fileName = "cfg/*";
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "MOD", &findHandle);
	while (fileName)
	{
		if (g_pFullFileSystem->FindIsDirectory(findHandle) && fileName[0] != '.')
		{
			char cMapList[MAX_PATH];
			V_snprintf(cMapList, MAX_PATH, "cfg/%s/maplist.txt", fileName);
			V_FixSlashes(cMapList);

			char cControlFile[MAX_PATH];
			V_snprintf(cControlFile, MAX_PATH, "cfg/%s/gameconfig.vdf", fileName);
			V_FixSlashes(cControlFile);

			KeyValuesAD pKVRoot("Config");
			CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
			if (g_pFullFileSystem->ReadFile(cMapList, "MOD", buf) && pKVRoot->LoadFromFile(filesystem, cControlFile, "MOD"))
			{
				NewGameDef_t GameDef;
				GameDef.m_BaseGame = FindOrAddGameType(pKVRoot->GetString("game_base", "hl2"));
				GameDef.m_GameMod = FindOrAddGameType(pKVRoot->GetString("game_mod", "hl2"));
				GameDef.m_PopSet = pKVRoot->GetString("population_set", "default");
				GameDef.m_SkillConfig = pKVRoot->GetString("skill_config", nullptr);
				GameDef.m_bExpectPortals = pKVRoot->GetBool("portals");
				KeyValues* pkvSoundScripts = pKVRoot->FindKey("sound_overrides");
				if (pkvSoundScripts)
				{
					for (KeyValues* pkvScript = pkvSoundScripts->GetFirstValue(); pkvScript != nullptr; pkvScript = pkvScript->GetNextValue())
					{
						FileNameHandle_t hFilename = sm_FilenameTable.FindOrAddFileName(pkvScript->GetString());
						GameDef.m_SoundOverrides.AddToTail(hFilename);
					}
				}

				CGTSymbol GameDefName(fileName);
				m_NewGameConfigs.InsertOrReplace(GameDefName, GameDef);

				NewMapData_t mapData;
				mapData.m_GameDef = GameDefName;

				char szMapName[MAX_MAP_NAME];
				do {
					buf.GetLine(szMapName, MAX_MAP_NAME);

					// Handle comments and empty lines
					if (V_strncmp(szMapName, "//", 2) == 0 || szMapName[0] == '\n')
						continue;

					const char* pszLineEnd = V_strnchr(szMapName, '\n', MAX_MAP_NAME);
					if (pszLineEnd)
						const_cast<char*>(pszLineEnd)[0] = 0;

					// Handle commands
					if (szMapName[0] == '@')
					{
						CUtlStringList params;
						V_SplitString(szMapName + 1, ":", params);

						const char* pszCommand = params[0];						
						if (V_strnicmp(pszCommand, "chapter", 7) == 0)
						{
							IF_PARAMS_VALID(params, 1)
								mapData.m_iChapterIndex = atoi(params[1]);
						}
						else if (V_strnicmp(pszCommand, "location", 8) == 0)
						{
							IF_PARAMS_VALID(params, 1)
								mapData.m_PopulationPath = params[1];
						}
						else if (V_strncmp(pszCommand, "optset", 6) == 0)
						{
							IF_PARAMS_VALID(params, 2)
								mapData.GetOrCreateOptions()->SetString(params[1], params[2]);
						}
						else if (V_strncmp(pszCommand, "optclr", 6) == 0)
						{
							mapData.NukeOptions();
						}

						continue;
					}

					m_NewMapConfigs.InsertOrReplace(CGTSymbol(szMapName), mapData);
				} while (buf.IsValid());
			}
		}

		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);
	
	Msg("CGameTypeManager: Registered %i map prefixes.\n", m_PrefixVector.Count());

	return true;
}

bool CGameTypeManager::IsMapInGame(const char *pchGame)
{
	return (0 == Q_strcmp(pchGame, m_vecGames.Element(GetCurrentBaseGameType())));
}

const char* CGameTypeManager::GetGameTypeName(int iType)
{
	if (iType <= GAME_INVALID || iType >= m_vecGames.Count())
		return nullptr;

	return m_vecGames[iType];
}

int CGameTypeManager::GetSoundOverrideScripts(CUtlStringList& scripts)
{
	int iCount = m_CurrentGame.m_SoundOverrides.Count();
	char szBuf[MAX_PATH];
	for (int i = 0; i < iCount; i++)
	{
		FileNameHandle_t hFileName = m_CurrentGame.m_SoundOverrides.Element(i);
		sm_FilenameTable.String(hFileName, szBuf, MAX_PATH);
		scripts.CopyAndAddToTail(szBuf);
	}

	return iCount;
}

void CGameTypeManager::AppendSkillCommand(OUT_Z_CAP(maxLenInChars) char* pDest, int maxLenInChars)
{
	if (m_CurrentGame.m_SkillConfig.IsValid())
	{
		const char *pchLineEnd = V_strnchr(pDest, '\n', maxLenInChars);
		if (pchLineEnd)
			*const_cast<char *> (pchLineEnd) = 0;
		
		CFmtStr str(";exec %s\n", m_CurrentGame.m_SkillConfig.String());
		V_strcat(pDest, str.Access(), maxLenInChars);
	}
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

const NewMapData_t& CGameTypeManager::LookupMapData(const char* pszMapname) const
{
	int iIndex = m_NewMapConfigs.Find(pszMapname);
	if (!m_NewMapConfigs.IsValidIndex(iIndex))
	{
		static NewMapData_t staticData;
		staticData.m_iChapterIndex = -1;
		return staticData;
	}

	return m_NewMapConfigs.Element(iIndex);
}

int CGameTypeManager::GetMapOptionInt(const char* keyName, int defaultValue) const
{
	if (!m_CurrentMap.m_pOptions)
	{
		return defaultValue;
	}

	return m_CurrentMap.m_pOptions->GetInt(keyName, defaultValue);
}

uint64 CGameTypeManager::GetMapOptionUint64(const char* keyName, uint64 defaultValue) const
{
	if (!m_CurrentMap.m_pOptions)
	{
		return defaultValue;
	}

	return m_CurrentMap.m_pOptions->GetUint64(keyName, defaultValue);
}

float CGameTypeManager::GetMapOptionFloat(const char* keyName, float defaultValue) const
{
	if (!m_CurrentMap.m_pOptions)
	{
		return defaultValue;
	}

	return m_CurrentMap.m_pOptions->GetFloat(keyName, defaultValue);
}

const char* CGameTypeManager::GetMapOptionString(const char* keyName, const char* defaultValue) const
{
	if (!m_CurrentMap.m_pOptions)
	{
		return defaultValue;
	}

	return m_CurrentMap.m_pOptions->GetString(keyName, defaultValue);
}

bool CGameTypeManager::GetMapOptionBool(const char* keyName, bool defaultValue, bool* optGotDefault) const
{
	if (!m_CurrentMap.m_pOptions)
	{
		if (optGotDefault)
			*optGotDefault = true;

		return defaultValue;
	}

	return m_CurrentMap.m_pOptions->GetBool(keyName, defaultValue, optGotDefault);
}

bool CGameTypeManager::HasMapOption(const char* keyName) const
{
	if (!m_CurrentMap.m_pOptions)
	{
		return false;
	}

	return (m_CurrentMap.m_pOptions->FindKey(keyName, false) != nullptr);
}

void CGameTypeManager::Reload()
{
	Shutdown();
	Init();
}

//CON_COMMAND_F(gametype_reload, "Reloads TypeManager's prefixes.\n", FCVAR_CHEAT)
//{
//	g_pGameTypeSystem->Reload();
//}

CON_COMMAND(gametype_print, "Prints info\n")
{
	int iPrefixes = GameTypeSystem()->m_PrefixVector.Count();
	int iGames = GameTypeSystem()->m_vecGames.Count();
	int i = 0;
	Msg("Registered Games:\n");
	for (i = 0; i < iGames; i++)
	{
		Msg("	%i: %s\n", i, GameTypeSystem()->m_vecGames[i]);
	}
	Msg("Registered Prefixes:\n");
	for (i = 0; i < iPrefixes; i++)
	{
		if (GameTypeSystem()->m_PrefixVector[i]->Type < iGames)
			Msg("	%s: %s\n", GameTypeSystem()->m_PrefixVector[i]->prefix.String(), GameTypeSystem()->m_vecGames[GameTypeSystem()->m_PrefixVector[i]->Type]);
		else
			Msg("	%s: %i\n", GameTypeSystem()->m_PrefixVector[i]->prefix.String(), (int)GameTypeSystem()->m_PrefixVector[i]->Type);
	}
}

void CGameTypeManager::RegisterPrefix(KeyValues *pkvNode)
{
	MapPrefix_t *pPrefix = new MapPrefix_t();

	if (pkvNode->GetDataType() == KeyValues::TYPE_INT)
	{
		pPrefix->Type = (GameType)pkvNode->GetInt();
		pPrefix->prefix = pkvNode->GetName();
	}
	else if (pkvNode->GetDataType() == KeyValues::TYPE_STRING || pkvNode->GetDataType() == KeyValues::TYPE_WSTRING)
	{
		pPrefix->prefix = pkvNode->GetName();

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

int CGameTypeManager::FindOrAddGameType(const char* pchGame)
{
	int type = GAME_INVALID;
	for (int i = 0; i < m_vecGames.Count(); i++)
	{
		if (0 == Q_stricmp(m_vecGames[i], pchGame))
		{
			type = i;
			break;
		}
	}
	if (type == GAME_INVALID)
	{
		//GameWord_t gwNew;

		type = m_vecGames.Count();

		m_vecGames.CopyAndAddToTail(pchGame);
		//Q_strncpy(m_vecGames[(int)type], pchString, sizeof(m_vecGames[(int)type]));

		Msg("CGameTypeManager: Registered custom game %s with index %i.\n", m_vecGames[type], type);
	}

	return type;
}

void CGameTypeManager::SelectGameType()
{
	m_bitAreas.ClearAll();
	m_iFirstArea = AREA_NONE;
	char szMapName[256];
	Q_strncpy(szMapName, STRING(gpGlobals->mapname), sizeof(szMapName));
	Q_strlower(szMapName);
	FOR_EACH_VEC(m_AreaNameVector, i)
	{
		if (Q_stristr(szMapName, m_AreaNameVector[i]->substring.String()))
		{
			AddAreaToMap(m_AreaNameVector[i]->iArea);
		}
	}

	//m_symConfigName = "default";

	CGTSymbol symMapName(szMapName);
	unsigned short usIDX = m_NewMapConfigs.Find(symMapName);
	if (m_NewMapConfigs.IsValidIndex(usIDX))
	{
		m_CurrentMap = m_NewMapConfigs.Element(usIDX);
		unsigned short usIDX2 = m_NewGameConfigs.Find(m_CurrentMap.m_GameDef);
		if (!m_NewGameConfigs.IsValidIndex(usIDX2))
		{
			m_CurrentGame = NewGameDef_t();
			return;
		}
		else
		{
			m_CurrentGame = m_NewGameConfigs.Element(usIDX2);
		}
	}
	else
	{
		//--Find current game
		struct PrefixCandidate
		{
			int Type;
			int Priority;
		};

		CUtlVector<PrefixCandidate> vecCandidates;

		for (int i = 0; i < m_PrefixVector.Count(); i++)
		{
			MapPrefix_t* pPrefix = m_PrefixVector.Element(i);
			const char* pchPrefix = pPrefix->prefix.String();
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

		m_CurrentGame = NewGameDef_t();
		m_CurrentMap = NewMapData_t();
		m_CurrentMap.m_GameDef = "default";
		m_CurrentMap.m_PopulationPath = m_vecAreaNames.Element(m_iFirstArea);
		m_CurrentMap.m_iChapterIndex = -1;

		if (vecCandidates.Count() == 1)
		{
			m_CurrentGame.m_BaseGame = m_CurrentGame.m_GameMod = (GameType)vecCandidates.Head().Type;
		}
		else if (vecCandidates.Count() > 0)
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

			m_CurrentGame.m_BaseGame = m_CurrentGame.m_GameMod = BestType;
		}
	}

	char szMapadd[128];
	Q_snprintf(szMapadd, sizeof(szMapadd), "maps/%s.spp", STRING(gpGlobals->mapname));
	AddFileToDownloadTable(szMapadd);
	KeyValues* pMapAdd = new KeyValues("MapData");
	if (pMapAdd->LoadFromFile(filesystem, szMapadd, "GAME"))
	{
		KeyValues* pkvMapOptions = m_CurrentMap.GetOrCreateOptions();
		FOR_EACH_VALUE(pMapAdd, pkvOption)
		{
			pkvMapOptions->SetString(pkvOption->GetName(), pkvOption->GetString());
		}
	}
	pMapAdd->deleteThis();
}



void CGameTypeManager::LevelInitPreEntity()
{
	SelectGameType();

	CFmtStrN<MAX_PATH> path("scripts/classremaps/%s.vdf", m_vecGames.Element(GetCurrentBaseGameType()));
	KeyValuesAD pKV("classremaps");
	if (pKV->LoadFromFile(filesystem, path.Access()))
	{
		for (KeyValues *pkvClass = pKV->GetFirstValue(); pkvClass != nullptr; pkvClass = pkvClass->GetNextValue())
		{
			CUtlSymbol &str = m_CRSymTable.AddString(pkvClass->GetString());
			m_ClassRemap.Insert(pkvClass->GetName(), str);
		}
	}

	if (GetCurrentBaseGameType() != GetCurrentModGameType())
	{
		CFmtStrN<MAX_PATH> path2("scripts/classremaps/%s.vdf", m_vecGames.Element(GetCurrentModGameType()));
		KeyValuesAD pKV2("classremaps");
		if (pKV2->LoadFromFile(filesystem, path2.Access()))
		{
			for (KeyValues* pkvClass = pKV2->GetFirstValue(); pkvClass != nullptr; pkvClass = pkvClass->GetNextValue())
			{
				CUtlSymbol& str = m_CRSymTable.AddString(pkvClass->GetString());
				m_ClassRemap.Insert(pkvClass->GetName(), str);
			}
		}
	}
}

void CGameTypeManager::LevelShutdown()
{
	m_ClassRemap.Purge();
	m_CRSymTable.RemoveAll();
	m_CurrentMap.NukeOptions();
}

const char * CGameTypeManager::RemapEntityClass(const char * pchClass)
{
	int iClass = m_ClassRemap.Find(pchClass);
	if (!m_ClassRemap.IsValidIndex(iClass))
	{
		return nullptr;
	}

	CUtlSymbol sym = m_ClassRemap.Element(iClass);
	return m_CRSymTable.String(sym);
}

ConVar sv_gametype("sv_force_gametype", "-1", FCVAR_CHEAT);

int CGameTypeManager::GetCurrentBaseGameType()
{
	if (sv_gametype.GetInt() >= 0 && sv_gametype.GetInt() < m_vecGames.Count())
		return (GameType)sv_gametype.GetInt();

	return m_CurrentGame.m_BaseGame;
}

int CGameTypeManager::GetCurrentModGameType()
{
	if (sv_gametype.GetInt() >= 0 && sv_gametype.GetInt() < m_vecGames.Count())
		return (GameType)sv_gametype.GetInt();

	return m_CurrentGame.m_GameMod;
}