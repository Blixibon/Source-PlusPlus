//=================== Half-Life 2: Short Stories Mod 2009 ====================================//
//
// Purpose: All Weapons derivied from CBaseCombatWeapon will have const int HLSS_GetWeaponId()
//
//============================================================================================//


#ifndef HLSS_WEAPON_ID
#define HLSS_WEAPON_ID

enum HLSS_WeaponId {
	HLSS_WEAPON_ID_NONE = 0,

	// Primary
	HLSS_WEAPON_ID_SMG1, // 0
	HLSS_WEAPON_ID_SMG2, // 0
	HLSS_WEAPON_ID_SHOTGUN, // 1
	HLSS_WEAPON_ID_AR2, // 2
	HLSS_WEAPON_ID_MG1, // 2

	// Primary 2
	HLSS_WEAPON_ID_RPG, // 0
	HLSS_WEAPON_ID_SNIPER, // 1
	HLSS_WEAPON_ID_CROSSBOW, // 2

	// Grenade
	HLSS_WEAPON_ID_FRAG, // 0
	HLSS_WEAPON_ID_BUGBAIT, // 0

	// Building
	HLSS_WEAPON_ID_TURRET, // 0
	
	// Item 1
	HLSS_WEAPON_ID_PHYSGUN, // 0

	// Item 2
	HLSS_WEAPON_ID_SLAM, // 0

	// PDA
	HLSS_WEAPON_ID_MEDKIT, // 0

	// Secondary
	HLSS_WEAPON_ID_PISTOL, // 0
	HLSS_WEAPON_ID_357, // 1
	HLSS_WEAPON_ID_ALYXGUN, // 0

	// Melee
	HLSS_WEAPON_ID_STUNSTICK, // 0
	HLSS_WEAPON_ID_CROWBAR, // 0
};

#endif