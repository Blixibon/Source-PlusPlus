#ifndef LAZ_ENTS_H
#define LAZ_ENTS_H
#pragma once

//#define MAX_EQUIP		32
#include "team_spawnpoint.h"
#include "GameEventListener.h"

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

	string_t	m_weaponNames[MAX_WEAPONS];
	bool		m_weaponClipEmpty[MAX_WEAPONS];
	int			m_Armor;

	string_t	m_ammoNames[MAX_AMMO_TYPES];
	bool		m_ammoCounts[MAX_AMMO_TYPES];
};

class CTeamControlPoint;
class CTeamControlPointRound;

//=============================================================================
//
// TF team spawning entity.
//

class CLazTeamSpawn : public CTeamSpawnPoint, public IGameEventListener2
{
public:
	DECLARE_CLASS(CLazTeamSpawn, CTeamSpawnPoint);

	CLazTeamSpawn();
	~CLazTeamSpawn();

	// FireEvent is called by EventManager if event just occured
	// KeyValue memory will be freed by manager if not needed anymore
	virtual void FireGameEvent(IGameEvent *event);

	void Activate(void);
	virtual void	UpdateTeam();

	void InputRoundSpawn(inputdata_t& inputdata);
	void UpdateOnRemove();

	int DrawDebugTextOverlays(void);

	CHandle<CTeamControlPoint> GetControlPoint(void) { return m_hControlPoint; }
	CHandle<CTeamControlPointRound> GetRoundCombineSpawn(void) { return m_hRoundBlueSpawn; }
	CHandle<CTeamControlPointRound> GetRoundRebelSpawn(void) { return m_hRoundRedSpawn; }

private:

	string_t						m_iszControlPointName;
	string_t						m_iszRoundBlueSpawn;
	string_t						m_iszRoundRedSpawn;

	CHandle<CTeamControlPoint>		m_hControlPoint;
	CHandle<CTeamControlPointRound>	m_hRoundBlueSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundRedSpawn;

	DECLARE_DATADESC();
};

#endif
