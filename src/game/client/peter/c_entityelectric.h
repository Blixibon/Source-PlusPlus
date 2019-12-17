//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef C_ENT_ELECTRIC_H
#define C_ENT_ELECTRIC_H

enum
{
	SHOCK_GENERIC = 0,
	SHOCK_VORTIGAUNT,
	SHOCK_GAUSSENERGY,

	MAX_SHOCK
};

class C_EntityElectric : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS(C_EntityElectric, C_BaseEntity);

	C_EntityElectric(void);
	~C_EntityElectric(void);

	virtual void	Simulate(void);
	virtual void	UpdateOnRemove(void);
	virtual void	OnDataChanged(DataUpdateType_t updateType);
	virtual void	ClientThink(void);

	CNewParticleEffect *m_hEffect;
	//EHANDLE				m_hEntAttached;		// The entity that we are burning (attached to).
	EHANDLE				m_hOldAttached;

	int					m_nShockType;
	float				m_flStartTime;

protected:

	void	CreateEffect(void);
	void	StopEffect(void);
};

#endif
