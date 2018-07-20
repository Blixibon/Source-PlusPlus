#include "cbase.h"
#include "gamerules.h"
#include "item_weapon_bms.h"
#include "weapon_parse.h"
#include "basecombatweapon.h"

//CUtlStringList g_ItemWeaponNames;
//const CUtlStringList *g_pItemWeaponNames = &VecItemWeaponNames;

void CItemWeapon::Precache()
{
	BaseClass::Precache();


	if (ReadWeaponDataFromFileForSlot(filesystem, GetWeaponName(), &m_hWeaponFileInfo, g_pGameRules->GetEncryptionKey()))
	{
		if (GetWorldModel() && GetWorldModel()[0])
		{
			CBaseEntity::PrecacheModel(GetWorldModel());
		}
	}
}

const char *CItemWeapon::GetWeaponName()
{
	return GetClassname() + 5;
}

void CItemWeapon::Spawn()
{
	Precache();


	if (!GetWeaponName() || !GetWeaponName()[0])
	{
		UTIL_Remove(this);
		return;
	}

	if (GetWorldModel() && GetWorldModel()[0])
	{
		SetModel(GetWorldModel());
	}

	BaseClass::Spawn();
}

bool CItemWeapon::MyTouch(CBasePlayer *pPlayer)
{
	// Can't pick up dissolving weapons
	if (IsDissolving())
		return false;

	// if it's not a player, ignore
	if (!pPlayer)
		return false;

	CBaseCombatWeapon *pWep = (CBaseCombatWeapon *)Create(GetWeaponName(), GetAbsOrigin(), GetAbsAngles());

	if (pPlayer->BumpWeapon(pWep))
	{
		pWep->OnPickedUp(pPlayer);
		return true;
	}
	else
	{
		UTIL_RemoveImmediate(pWep);
		return false;
	}
}

LINK_ITEM_WEAPON(weapon_357);