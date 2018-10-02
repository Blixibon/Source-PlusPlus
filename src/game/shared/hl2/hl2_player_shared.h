//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL2_PLAYER_SHARED_H
#define HL2_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Shared header file for players
#if defined( CLIENT_DLL )
#define CHL2_Player C_BaseHLPlayer	//FIXME: Lovely naming job between server and client here...
#include "c_basehlplayer.h"
#else
#include "hl2_player.h"
#endif

//-----------------------------------------------------------------------------
// Converts an entity to a player
//-----------------------------------------------------------------------------
inline CHL2_Player *ToHL2Player(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<CHL2_Player *>(pEntity);
}

//-----------------------------------------------------------------------------
// Converts an entity to a player
//-----------------------------------------------------------------------------
inline const CHL2_Player *ToHL2Player(const CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
		return NULL;

	return dynamic_cast<const CHL2_Player *>(pEntity);
}

#endif // HL2_PLAYER_SHARED_H
