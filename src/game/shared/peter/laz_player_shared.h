//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef LAZ_PLAYER_SHARED_H
#define LAZ_PLAYER_SHARED_H
#pragma once

#define HL2MP_PUSHAWAY_THINK_INTERVAL		(1.0f / 20.0f)

enum LAZPlayerState
{
	// Happily running around in the game.
	STATE_ACTIVE = 0,
	STATE_WELCOME,
	STATE_OBSERVER_MODE,		// Noclipping around, watching players, etc.
	STATE_DYING,
	NUM_PLAYER_STATES
};


#if defined( CLIENT_DLL )
#define CLaz_Player C_Laz_Player
#endif

#endif