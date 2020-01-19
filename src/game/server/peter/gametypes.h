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
} NewGameDef_t;

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
	}

	void LevelInitPreEntity();
	void LevelShutdown();

	const char *RemapEntityClass(const char *pchClass);

	int GetCurrentBaseGameType();
	int GetCurrentModGameType();
	
	bool IsMapInGame(const char *pchGame); 

	int LookupGametype(const char *);
	const char* GetGameTypeName(int);

	const char* GetCurrentConfigName() { return m_symConfigName.String(); }
	int		GetSoundOverrideScripts(CUtlStringList& scripts);

	bool IsMapInArea(int iArea)
	{
		return m_bitAreas.IsBitSet(iArea);
	}

	const char *GetFirstArea()
	{
		return m_vecAreaNames.Element(m_iFirstArea);
	}

	void Reload();

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
	CUtlMap<CGTSymbol, CGTSymbol>	m_MapNameToGameConfig;
	CGTSymbol						m_symConfigName;

	CUtlVectorAutoPurge<MapPrefix_t *> m_PrefixVector;
	CUtlVector<AreaName_t *> m_AreaNameVector;
	CUtlStringList m_vecGames;
	NewGameDef_t	m_CurrentGame;

	CBitVec<MAX_CODE_AREAS> m_bitAreas;
	int						m_iFirstArea;

	CUtlSymbolTable			m_CRSymTable;
	CUtlDict<CUtlSymbol>	m_ClassRemap;

	friend static void gametype_print(const CCommand &args);

};

extern CGameTypeManager *g_pGameTypeSystem;

#endif
