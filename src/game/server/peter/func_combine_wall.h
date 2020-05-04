#pragma once
#include "modelentities.h"



class CCombineShieldWall : public CFuncBrush
{
public:
	DECLARE_CLASS(CCombineShieldWall, CFuncBrush);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CCombineShieldWall();
	~CCombineShieldWall();

	CCombineShieldWall* m_pNext;

	virtual void Precache();
	virtual void Spawn();

	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;

	virtual void TurnOff(void);
	virtual void TurnOn(void);
	virtual bool IsOn(void) const;

	// Team Handling
	virtual void			ChangeTeam(int iTeamNum);

	void	ActiveThink();

	static bool PointsCrossForceField(const Vector& vecStart, const Vector& vecEnd, int nTeamToIgnore = TEAM_UNASSIGNED);
private:
	void	ClearSpace();

	CNetworkVar(bool, m_bShieldActive);
	CNetworkArray(EHANDLE, m_hNearbyEntities, 2);

	Vector	m_vecLocalOuterBoundsMins;
	Vector	m_vecLocalOuterBoundsMaxs;
};