//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef LAZ_PLAYER_RESOURCE_H
#define LAZ_PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "player_resource.h"
#include "laz_player_shared.h"

class CLAZPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CLAZPlayerResource, CPlayerResource );
	
public:
	DECLARE_SERVERCLASS();

	CLAZPlayerResource();

	virtual void UpdatePlayerData( void );
	virtual void Spawn( void );
	Color GetPlayerColor(int iIndex, int iColor);

protected:
	CNetworkArray(Vector, m_vPlayerColors, (MAX_PLAYERS + 1)* NUM_PLAYER_COLORS);
};

#endif // TF_PLAYER_RESOURCE_H
