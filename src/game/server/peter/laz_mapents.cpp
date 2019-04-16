#include "cbase.h"
#include "laz_mapents.h"

LINK_ENTITY_TO_CLASS(info_player_equip, CLazPlayerEquip);

BEGIN_DATADESC(CLazPlayerEquip)

DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_Armor, FIELD_INTEGER, "item_Armor"),

END_DATADESC()

bool CLazPlayerEquip::KeyValue(const char* szKeyName, const char* szValue)
{
	if (!BaseClass::KeyValue(szKeyName, szValue))
	{
		int iValue = atoi(szValue);
		if (!V_strncmp(szKeyName, "weapon_", 7))
		{
			if (iValue > 0)
			{
				for (int i = 0; i < MAX_EQUIP; i++)
				{
					if (!m_weaponNames[i])
					{
						char tmp[128];

						UTIL_StripToken(szKeyName, tmp);

						m_weaponNames[i] = AllocPooledString(tmp);
						m_weaponClipEmpty[i] = (iValue > 1);
						return true;
					}
				}
			}
		}
		else if (!V_strncmp(szKeyName, "ammo_", 5))
		{
			if (iValue > 0)
			{
				const char* pszAmmo = szKeyName + 5;

				for (int i = 0; i < MAX_AMMO_TYPES; i++)
				{
					if (!m_ammoNames[i])
					{
						m_ammoNames[i] = AllocPooledString(pszAmmo);
						m_ammoCounts[i] = iValue;
						return true;
					}
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputEnable(inputdata_t& inputdata)
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputDisable(inputdata_t& inputdata)
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputEquipPlayer(inputdata_t& inputdata)
{
	CBasePlayer* pPlayer = ToBasePlayer(inputdata.pActivator);
	if (pPlayer && CanEquipTeam(pPlayer->GetTeamNumber()))
	{
		EquipPlayer(pPlayer);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputEquipAllPlayers(inputdata_t& inputdata)
{
	m_bDisabled = true;
}

void CLazPlayerEquip::EquipPlayer(CBaseEntity* pEntity)
{
	CBasePlayer* pPlayer = ToBasePlayer(pEntity);

	if (!pPlayer)
		return;

	pPlayer->IncrementArmorValue(m_Armor, 100);

	for (int i = 0; i < MAX_EQUIP; i++)
	{
		if (!m_weaponNames[i])
			break;

		CBaseEntity *pSpawn = pPlayer->GiveNamedItem(STRING(m_weaponNames[i]));

		if (pSpawn && !pSpawn->IsMarkedForDeletion() && m_weaponClipEmpty[i])
		{
			CBaseCombatWeapon* pWeapon = pSpawn->MyCombatWeaponPointer();
			if (pWeapon)
			{
				pWeapon->m_iClip1 = 0;
			}
		}
	}

	for (int i = 0; i < MAX_AMMO_TYPES; i++)
	{
		if (!m_ammoNames[i])
			break;

		pPlayer->GiveAmmo(m_ammoCounts[i], STRING(m_ammoNames[i]), true);
	}
}