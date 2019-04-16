#ifndef LAZ_ENTS_H
#define LAZ_ENTS_H
#pragma once

#define MAX_EQUIP		32

class CLazPlayerEquip : public CServerOnlyPointEntity
{
	DECLARE_CLASS(CLazPlayerEquip, CServerOnlyPointEntity)
	DECLARE_DATADESC()
public:
	bool		KeyValue(const char* szKeyName, const char* szValue);

	bool IsDisabled(void) { return m_bDisabled; }
	void SetDisabled(bool bDisabled) { m_bDisabled = bDisabled; }

	void		EquipPlayer(CBaseEntity* pPlayer);

	bool		CanEquipTeam(int iTeam)
	{
		return (GetTeamNumber() == TEAM_UNASSIGNED || GetTeamNumber() == iTeam);
	}

	// Inputs/Outputs.
	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);
	void InputEquipPlayer(inputdata_t& inputdata);
	void InputEquipAllPlayers(inputdata_t& inputdata);
private:
	bool		m_bDisabled;

	string_t	m_weaponNames[MAX_EQUIP];
	bool		m_weaponClipEmpty[MAX_EQUIP];
	int			m_Armor;

	string_t	m_ammoNames[MAX_AMMO_TYPES];
	bool		m_ammoCounts[MAX_AMMO_TYPES];
};

#endif
