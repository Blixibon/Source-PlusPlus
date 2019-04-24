//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL1_SHAREDDEFS_H
#define HL1_SHAREDDEFS_H
#ifdef _WIN32
#pragma once
#endif


//--------------
// HL1 SPECIFIC
//--------------
#define DMG_MISSILEDEFENSE	(DMG_LASTGENERICFLAG<<2)	// The only kind of damage missiles take. (special missile defense)

#if defined(HL1_DLL) || defined(HL2_LAZUL)
#define HL1_PISTOL_AMMO "9mmRound"
#define HL1_357_AMMO "357Round"
#define HL1_12MM_AMMO "12mmRound"
#elif defined( HL2_DLL )
#define HL1_PISTOL_AMMO "Pistol"
#define HL1_357_AMMO "357"
#define HL1_12MM_AMMO "AR2"
#endif


#endif // HL1_SHAREDDEFS_H
