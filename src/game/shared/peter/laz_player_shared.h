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

#define NUMBER_OF_CONTROLLABLE_MANHACKS 3

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

enum LazSpecialStepSounds_e
{
	FOOTSTEP_SOUND_HL1 = -2,
};

enum LazMovementConfig_e
{
	MOVECFG_HL2 = -1,
	MOVECFG_HL1 = 0,

	NUM_MOVEMENT_CONFIGS
};

struct LazSpeedData_t
{
	float flNormSpeed;
	float flSlowSpeed;
	float flFastSpeed;
	bool bMainIsFast;
};

struct HL1Foot_t
{
	const char* m_pNameLeft;
	const char* m_pNameRight;
};

extern HL1Foot_t s_pHL1FootSounds[26];

inline bool IsInvalidString(int iString) { return (unsigned short)iString == ((unsigned short)-1) || iString < -1; }

#if defined( CLIENT_DLL )
#define CLaz_Player C_Laz_Player
#include "peter/c_laz_player.h"
#else
#include "peter/laz_player.h"
#endif

#endif