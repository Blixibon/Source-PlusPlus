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

enum LAZFlashlightType
{
	FLASHLIGHT_NONE = -1,
	FLASHLIGHT_SUIT,
	FLASHLIGHT_NVG,
	FLASHLIGHT_WEAPON,

	FLASHLIGHT_TYPE_COUNT
};

enum PlayerColors_e
{
	PLRCOLOR_CLOTHING = 0,
	PLRCOLOR_CLOTHING_GLOW,
	PLRCOLOR_WEAPON,
	PLRCOLOR_PORTAL1,
	PLRCOLOR_PORTAL2,

	NUM_PLAYER_COLORS
};

#if defined( CLIENT_DLL )
#define CLaz_Player C_Laz_Player
#endif

#endif