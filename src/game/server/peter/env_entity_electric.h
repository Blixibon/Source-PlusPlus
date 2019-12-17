#ifndef ENT_ELECTRIC_H
#define ENT_ELECTRIC_H

enum
{
	SHOCK_GENERIC = 0,
	SHOCK_VORTIGAUNT,
	SHOCK_GAUSSENERGY,

	MAX_SHOCK
};

class CEntElectric : public CBaseEntity
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_CLASS(CEntElectric, CBaseEntity);

	static CEntElectric	*Create(CBaseEntity *pTarget, float flStartTime, int nShockType = SHOCK_GENERIC);
	static CEntElectric	*Create(CBaseEntity *pTarget, CBaseEntity *pSource);

	void Spawn();
	virtual void Precache();

	void	AttachToEntity(CBaseEntity *pTarget);
	void	SetStartTime(float flStartTime);

	DECLARE_DATADESC();

	void RemoveThink();

	CNetworkVar(int, m_nShockType);
	CNetworkVar(float, m_flStartTime);
};


#endif // !ENT_ELECTRIC_H

