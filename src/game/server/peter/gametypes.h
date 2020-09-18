#ifndef GAMETYPES_H
#define GAMETYPES_H
#include "igamesystem.h"
#include "utlvector.h"
#include "bitvec.h"
#include "utlsymbol.h"

enum GameType
{
	GAME_INVALID = -1,

	GAME_DEFAULT = 0,
	GAME_HL1,
	GAME_HL2,
	GAME_EP1,
	GAME_EP2,
	MOD_CITIZEN,
	MOD_CITIZEN2,
	MOD_BMS,
	GAME_PORTAL,
	MOD_HL2BETA,
	MOD_HLSS,
	MOD_EZ1,

	MAX_CODE_GAMETYPES
};

enum
{
	AREA_NONE = 0,

	AREA_TRAINSTATION,
	AREA_CANALS,
	AREA_ZOMBIETOWN,
	AREA_COAST,
	AREA_PRISON,
	AREA_CITY17,
	AREA_CITADEL,
	AREA_OUTLAND,
	AREA_BMRF,
	AREA_XEN,

	MAX_CODE_AREAS
};

DECLARE_PRIVATE_SYMBOLTYPE(CGTSymbol);

typedef struct GameWord_s
{
	CGTSymbol game;
	int index;
} GameWord_t;

typedef struct MapPrefix_s
{
	CGTSymbol prefix;
	int Type;
} MapPrefix_t;

typedef struct AreaName_s
{
	CGTSymbol substring;
	int iArea;
} AreaName_t;

typedef struct NewGameDef_s
{
	int m_BaseGame;
	int m_GameMod;
	CGTSymbol m_PopSet;
	CCopyableUtlVector< FileNameHandle_t > m_SoundOverrides;

	bool	m_bExpectPortals;

	NewGameDef_s()
	{
		m_BaseGame = GAME_DEFAULT;
		m_GameMod = GAME_DEFAULT;
		m_PopSet = "default";
		m_bExpectPortals = false;
	}
} NewGameDef_t;

typedef struct NewMapData_s
{
	CGTSymbol m_GameDef;
	CGTSymbol m_PopulationPath;
	int	m_iChapterIndex; // Chapter index. -1 specifies invalid map. -2 specifies training room. -3 specifies bonus map.
	KeyValues* m_pOptions;

	KeyValues* GetOrCreateOptions()
	{
		if (!m_pOptions)
		{
			m_pOptions = new KeyValues("MapOptions");
		}

		return m_pOptions;
	}

	void	NukeOptions()
	{
		if (m_pOptions)
		{
			m_pOptions->deleteThis();
			m_pOptions = nullptr;
		}
	}

	NewMapData_s()
	{
		m_iChapterIndex = -1;
		m_pOptions = nullptr;
	}

	~NewMapData_s()
	{
		if (m_pOptions)
		{
			m_pOptions->deleteThis();
			m_pOptions = nullptr;
		}
	}

	NewMapData_s &operator=(NewMapData_s const& other)
	{
		if (m_pOptions)
		{
			m_pOptions->deleteThis();
			m_pOptions = nullptr;
		}

		m_GameDef = other.m_GameDef;
		m_PopulationPath = other.m_PopulationPath;
		m_iChapterIndex = other.m_iChapterIndex;

		if (other.m_pOptions)
		{
			m_pOptions = other.m_pOptions->MakeCopy();
		}
		else
		{
			m_pOptions = nullptr;
		}

		return *this;
	}

	NewMapData_s(NewMapData_s const& other)
	{
		*this = other;
	}
} NewMapData_t;

class CGameTypeManager /*: public CBaseGameSystem*/
{
public:
	CGameTypeManager();

	virtual char const* Name() { return "CGameTypeManager"; }

	bool Init();
	void Shutdown()
	{ 
		m_PrefixVector.PurgeAndDeleteElements();
		m_vecGames.RemoveAll();
		m_AreaNameVector.PurgeAndDeleteElements();

		m_NewGameConfigs.Purge();
		m_NewMapConfigs.Purge();
	}

	void LevelInitPreEntity();
	void LevelShutdown();

	const char *RemapEntityClass(const char *pchClass);

	int GetCurrentBaseGameType();
	int GetCurrentModGameType();
	
	bool IsMapInGame(const char *pchGame); 

	int LookupGametype(const char *);
	const char* GetGameTypeName(int);

	const char* GetCurrentConfigName() { return m_CurrentMap.m_GameDef.String(); }
	int		GetSoundOverrideScripts(CUtlStringList& scripts);
	const char* GetPopulationSet() { return m_CurrentGame.m_PopSet.String(); }
	bool WorldShouldExpectPortals() { return m_CurrentGame.m_bExpectPortals; }

	const char* GetPopulationLocation() { return m_CurrentMap.m_PopulationPath.String(); }
	const NewMapData_t& LookupMapData(const char* pszMapname) const;

	// Data access
	int   GetMapOptionInt(const char* keyName, int defaultValue = 0) const;
	uint64 GetMapOptionUint64(const char* keyName, uint64 defaultValue = 0) const;
	float GetMapOptionFloat(const char* keyName, float defaultValue = 0.0f) const;
	const char* GetMapOptionString(const char* keyName, const char* defaultValue = "") const;
	bool GetMapOptionBool(const char* keyName, bool defaultValue = false, bool* optGotDefault = NULL) const;
	bool HasMapOption(const char* keyName) const;

	void Reload();

	// Script Functions
	int   ScriptGetMapOptionInt(const char* keyName) const { return GetMapOptionInt(keyName); }
	int64 ScriptGetMapOptionInt64(const char* keyName) const { return GetMapOptionUint64(keyName); }
	float ScriptGetMapOptionFloat(const char* keyName) const { return GetMapOptionFloat(keyName); }
	const char* ScriptGetMapOptionString(const char* keyName) const { return GetMapOptionString(keyName); }
	bool ScriptGetMapOptionBool(const char* keyName) const { return GetMapOptionBool(keyName); }

	const char* ScriptGetBaseGameType() { return GetGameTypeName(GetCurrentBaseGameType()); }
	const char* ScriptGetModGameType() { return GetGameTypeName(GetCurrentModGameType()); }
	const char* ScriptRemapEntityClass(const char* pszClassname)
	{
		const char* pszNewClass = RemapEntityClass(pszClassname);
		return (pszNewClass != nullptr) ? pszNewClass : pszClassname;
	}



	static CUtlFilenameSymbolTable sm_FilenameTable;

protected:
	void RegisterPrefix(KeyValues *pkvNode);
	int	FindOrAddGameType(const char* pchGame);

	void AddAreaToMap(int iArea)
	{
		if (m_iFirstArea == AREA_NONE)
			m_iFirstArea = iArea;

		m_bitAreas.Set(iArea);
	}

	void SelectGameType();

	//static char *m_pchAreaNames[MAX_CODE_AREAS];
	CUtlVectorFixed<char *, MAX_CODE_AREAS> m_vecAreaNames;

private:
	CUtlMap<CGTSymbol, NewGameDef_t> m_NewGameConfigs;
	CUtlMap<CGTSymbol, NewMapData_t>	m_NewMapConfigs;
	//CGTSymbol						m_symConfigName;

	CUtlVectorAutoPurge<MapPrefix_t *> m_PrefixVector;
	CUtlVector<AreaName_t *> m_AreaNameVector;
	CUtlStringList m_vecGames;
	NewGameDef_t	m_CurrentGame;
	NewMapData_t	m_CurrentMap;

	CBitVec<MAX_CODE_AREAS> m_bitAreas;
	int						m_iFirstArea;

	CUtlSymbolTable			m_CRSymTable;
	CUtlDict<CUtlSymbol>	m_ClassRemap;

	friend static void gametype_print(const CCommand &args);

};

extern CGameTypeManager *g_pGameTypeSystem;

#endif
