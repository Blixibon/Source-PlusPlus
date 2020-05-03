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

	static bool PointsCrossForceField(const Vector& vecStart, const Vector& vecEnd, int nTeamToIgnore = TEAM_UNASSIGNED);
private:
	CNetworkVar(bool, m_bShieldActive);
};