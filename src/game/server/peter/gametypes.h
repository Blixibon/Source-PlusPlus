#ifndef GAMETYPES_H
#define GAMETYPES_H
#include "igamesystem.h"
#include "utlvector.h"
#include "bitvec.h"

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

typedef struct GameWord_s
{
	char game[16];
	GameType index;
} GameWord_t;

typedef struct MapPrefix_s
{
	char prefix[64];
	GameType Type;
} MapPrefix_t;

typedef struct AreaName_s
{
	char substring[32];
	int iArea;
} AreaName_t;

class CGameTypeManager : public CAutoGameSystem
{
public:
	CGameTypeManager() : CAutoGameSystem("CGameTypeManager")
	{
	}

	bool Init();
	void Shutdown()
	{ 
		m_PrefixVector.PurgeAndDeleteElements();
		m_vecGames.RemoveAll();
		m_AreaNameVector.PurgeAndDeleteElements();
	}

	void LevelInitPreEntity();

	GameType GetCurrentGameType();
	
	bool IsMapInGame(const char *pchGame); 

	int LookupGametype(const char *);

	bool IsMapInArea(int iArea)
	{
		return m_bitAreas.IsBitSet(iArea);
	}

	const char *GetFirstArea()
	{
		return m_vecAreaNames.Element(m_iFirstArea);
	}

	void Reload();

protected:
	void RegisterPrefix(KeyValues *pkvNode);

	void AddAreaToMap(int iArea)
	{
		if (m_iFirstArea == AREA_NONE)
			m_iFirstArea = iArea;

		m_bitAreas.Set(iArea);
	}

	//static char *m_pchAreaNames[MAX_CODE_AREAS];
	CUtlVectorFixed<char *, MAX_CODE_AREAS> m_vecAreaNames;

private:
	CUtlVectorAutoPurge<MapPrefix_t *> m_PrefixVector;
	CUtlVector<AreaName_t *> m_AreaNameVector;
	CUtlStringList m_vecGames;
	GameType m_iGameType;

	CBitVec<MAX_CODE_AREAS> m_bitAreas;
	int						m_iFirstArea;

	friend static void gametype_print(const CCommand &args);

};

extern CGameTypeManager *g_pGameTypeSystem;

#endif
