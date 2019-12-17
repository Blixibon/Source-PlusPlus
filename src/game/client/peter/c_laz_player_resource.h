//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_LAZ_PLAYERRESOURCE_H
#define C_LAZ_PLAYERRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "laz_player_shared.h"
#include "c_playerresource.h"

class C_LAZ_PlayerResource : public C_PlayerResource
{
	DECLARE_CLASS(C_LAZ_PlayerResource, C_PlayerResource );
public:
	DECLARE_CLIENTCLASS();

	C_LAZ_PlayerResource();
	virtual ~C_LAZ_PlayerResource();
	Color GetPlayerColor(int iIndex, int iColor);
	Vector GetPlayerColorVector(int iIndex, int iColor);
	
protected:

	Vector m_vPlayerColors[(MAX_PLAYERS + 1) * NUM_PLAYER_COLORS];
};


#endif // C_TF_PLAYERRESOURCE_H
