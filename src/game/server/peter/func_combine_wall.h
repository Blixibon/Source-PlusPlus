#pragma once
#include "modelentities.h"
#include "laz_mapents.h"

class CCombineShieldWall : public CLazNetworkEntity< CFuncBrush >
{
public:
	DECLARE_CLASS(CCombineShieldWall, CLazNetworkEntity< CFuncBrush >);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CCombineShieldWall();
	~CCombineShieldWall();

	CCombineShieldWall* m_pNext;

	virtual void Precache();
	virtual void Spawn();
	virtual void Activate();

	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;

	virtual void TurnOff(void);
	virtual void TurnOn(void);
	virtual bool IsOn(void) const;

	virtual void NetworkPowerOn(bool bForce) { TurnOn(); }
	virtual void NetworkPowerOff(bool bForce) { TurnOff(); }
	virtual LazNetworkRole_t GetNetworkRole() { return NETROLE_SHIELDS; }

	// Team Handling
	virtual void			ChangeTeam(int iTeamNum);

	void	ActiveThink();

	static bool PointsCrossForceField(const Vector& vecStart, const Vector& vecEnd, int nTeamToIgnore = TEAM_UNASSIGNED);
private:
	void	ClearSpace();

	CNetworkVar(bool, m_bShieldActive);
	CNetworkArray(EHANDLE, m_hNearbyEntities, 2);

	CUtlVector<EHANDLE> m_hShieldProjectors;
	string_t			m_strShieldProjectors;

	Vector	m_vecLocalOuterBoundsMins;
	Vector	m_vecLocalOuterBoundsMaxs;
};