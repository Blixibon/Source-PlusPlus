#include "cbase.h"
#include "items.h"
#include "utlvector.h"

class CItemWeapon : public CItem
{
	DECLARE_CLASS(CItemWeapon, CItem);
public:

	CItemWeapon() /*: m_nWeaponName(-1)*/
	{}

	void Precache();
	void Spawn();

	bool MyTouch(CBasePlayer *pPlayer);

	const char *GetWorldModel(void) const
	{
		return GetWpnData().szWorldModel;
	}

protected:
	WEAPON_FILE_INFO_HANDLE	m_hWeaponFileInfo;

	const FileWeaponInfo_t &GetWpnData(void) const
	{
		return *GetFileWeaponInfoFromHandle(m_hWeaponFileInfo);
	}

	const char *GetWeaponName();


//private:
//	int m_nWeaponName;
};

//extern CUtlStringList g_ItemWeaponNames;

#define LINK_ITEM_WEAPON(weaponclass) LINK_ENTITY_TO_CLASS(item_##weaponclass, CItemWeapon); 
	//g_ItemWeaponNames.CopyAndAddToTail(#weaponclass);