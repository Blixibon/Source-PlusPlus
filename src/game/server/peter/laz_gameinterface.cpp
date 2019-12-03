//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"
#include "utlvector.h"
#include "gametypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CServerGameClients::GetPlayerLimits(int& minplayers, int& maxplayers, int& defaultMaxPlayers) const
{
	minplayers = defaultMaxPlayers = 1;
	maxplayers = MAX_PLAYERS;
}

class CBlackMesaWeaponsFilter : public IMapEntityFilter
{
public:
	virtual bool ShouldCreateEntity(const char* pClassname)
	{
		// Create everything but the world
		return Q_strnicmp(pClassname, "item_weapon", 11) == 0;
	}

	virtual CBaseEntity* CreateNextEntity(const char* pClassname, char* pClassnameWriteBack, int iMaxWrite)
	{
		CFmtStr strClass("%s_bms", pClassname + 5);
		if (iMaxWrite > 0)
			Q_strncpy(pClassnameWriteBack, strClass, iMaxWrite);

		CBaseEntity *pEntity = CreateEntityByName(strClass);
		if (!pEntity)
			return nullptr;

		m_vecCreated.AddToTail(pEntity);
		return pEntity;
	}

	CUtlVector<EHANDLE> m_vecCreated;
};

// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

void CServerGameDLL::LevelInit_ParseAllEntities(const char* pMapEntities)
{
	if (g_pGameTypeSystem->GetCurrentGameType() == MOD_BMS)
	{
		CBlackMesaWeaponsFilter filter;
		MapEntity_ParseAllEntities(pMapEntities, &filter);

		// Black Mesa to Half-Life 2 spawnflag fixup
		for (const EHANDLE& hEntity : filter.m_vecCreated)
		{
			bool bMotionDisable = hEntity->HasSpawnFlags(2);
			hEntity->ClearSpawnFlags();
			if (bMotionDisable)
				hEntity->AddSpawnFlags(1);
		}
	}
}
