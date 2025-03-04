//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2_SHAREDDEFS_H
#define HL2_SHAREDDEFS_H

#ifdef _WIN32
#pragma once
#endif

#include "const.h"


//--------------------------------------------------------------------------
// Collision groups
//--------------------------------------------------------------------------

enum
{
	HL2COLLISION_GROUP_PLASMANODE = LAST_SHARED_COLLISION_GROUP,
	HL2COLLISION_GROUP_SPIT,
	HL2COLLISION_GROUP_HOMING_MISSILE,
	HL2COLLISION_GROUP_COMBINE_BALL,

	HL2COLLISION_GROUP_FIRST_NPC,
	HL2COLLISION_GROUP_HOUNDEYE,
	HL2COLLISION_GROUP_CROW,
	HL2COLLISION_GROUP_HEADCRAB,
	HL2COLLISION_GROUP_STRIDER,
	HL2COLLISION_GROUP_GUNSHIP,
	HL2COLLISION_GROUP_ANTLION,
	HL2COLLISION_GROUP_MANHACK,
	HL2COLLISION_GROUP_LAST_NPC,
	HL2COLLISION_GROUP_COMBINE_BALL_NPC,

	LAST_HL2_COLLISION_GROUP
};


//--------------
// HL2 SPECIFIC
//--------------
#define DMG_SNIPER			0x0000000100000000Ui64	//(DMG_LASTGENERICFLAG<<1Ui64)	// This is sniper damage. Displays headshots in deathnotice.
#define DMG_MISSILEDEFENSE	0x0000000200000000Ui64	//(DMG_LASTGENERICFLAG<<2Ui64)	// The only kind of damage missiles take. (special missile defense)
#define DMG_AIRBOAT		0x0000000400000000Ull	//(DMG_LASTGENERICFLAG<<3Ull)	// Damage from a heavy weapon

#define DMG_LASTHL2 0x0000000400000000Ull

#endif // HL2_SHAREDDEFS_H
