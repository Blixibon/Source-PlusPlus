#ifndef LAZ_ENTS_H
#define LAZ_ENTS_H
#pragma once

//#define MAX_EQUIP		32
#include "team_spawnpoint.h"
#include "GameEventListener.h"

enum LazNetworkRole_t
{
	NETROLE_GENERIC = 0,
	NETROLE_SHIELDS,
	NETROLE_TURRETS,
	NETROLE_SUBNETWORK,

	NETROLE_COUNT
};

class ILazNetworkEntity
{
public:
	virtual void NetworkPowerOn(bool bForce) = 0;
	virtual void NetworkPowerOff(bool bForce) = 0;
	virtual bool HasFirewall() = 0;
	virtual LazNetworkRole_t GetNetworkRole() = 0;

	virtual CBaseEntity *GetEntityPtr() = 0;
};

class ILazNetworkController
{
public:
	virtual void	AddEntityToNetwork(ILazNetworkEntity* pEnt) = 0;
	virtual void	RemoveEntityFromNetwork(ILazNetworkEntity* pEnt) = 0;
};

#define	DEFINE_LAZNETWORKENTITY_DATADESC() \
	DEFINE_FIELD(m_hNetworkController, FIELD_EHANDLE),	\
	DEFINE_KEYFIELD(m_strControllerName, FIELD_STRING, "NetworkController")

template <class BASE_ENTITY>
class CLazNetworkEntity : public BASE_ENTITY, public ILazNetworkEntity
{
public:
	DECLARE_CLASS(CLazNetworkEntity, BASE_ENTITY);

	virtual void Activate();
	virtual void UpdateOnRemove();

	virtual void NetworkPowerOn(bool bForce) { return; }
	virtual void NetworkPowerOff(bool bForce) { return; }
	virtual LazNetworkRole_t GetNetworkRole() { return NETROLE_GENERIC; }
	virtual bool HasFirewall() { return false; }

	virtual CBaseEntity* GetEntityPtr() { return this; }
protected:
	EHANDLE m_hNetworkController;
	string_t	m_strControllerName;
};

template<class BASE_ENTITY>
inline void CLazNetworkEntity<BASE_ENTITY>::Activate()
{
	if (m_strControllerName != NULL_STRING && !m_hNetworkController.Get())
	{
		CBaseEntity* pEnt = gEntList.FindEntityByName(nullptr, m_strControllerName, this);
		if (!pEnt)
		{
			Warning("%s was unable to find network controller named %s!\n", GetDebugName(), STRING(m_strControllerName));
		}
		else
		{
			ILazNetworkController* pNetwork = dynamic_cast<ILazNetworkController*> (pEnt);
			if (pNetwork)
			{
				m_hNetworkController.Set(pEnt);
				pNetwork->AddEntityToNetwork(this);
			}
			else
			{
				Warning("%s found %s, but it was not a network controller!\n", GetDebugName(), STRING(m_strControllerName));
			}
		}
	}

	BaseClass::Activate();
}

template<class BASE_ENTITY>
inline void CLazNetworkEntity<BASE_ENTITY>::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	if (m_hNetworkController.Get())
	{
		ILazNetworkController* pNetwork = dynamic_cast<ILazNetworkController*> (m_hNetworkController.Get());
		pNetwork->RemoveEntityFromNetwork(this);
		m_hNetworkController.Term();
	}
}

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
	virtual bool	IsValid(CBasePlayer *pPlayer);

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
