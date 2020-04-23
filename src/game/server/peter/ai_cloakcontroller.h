#pragma once

#include "ai_component.h"

enum CloakState_e
{
	STATE_UNCLOAKED = 0,
	STATE_CLOAKING,
	STATE_CLOAKED,
	STATE_UNCLOAKING,
	STATE_MALFUNTION,
};

class CAI_CloakController : public CAI_Component
{
	DECLARE_CLASS(CAI_CloakController, CAI_Component);
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_SIMPLE_DATADESC();
public:
	void	Init(float flCloakTime, float flUncloakTime);
	void	SetCloaked(bool bCloaked);
	void	StartMalfunction(float flErrorTime);
	CloakState_e GetCloakState();

private:
	CNetworkVar(float, m_flCloakTime);
	CNetworkVar(float, m_flUncloakTime);
	CNetworkVar(bool, m_bCloaking);
	CNetworkVar(float, m_flCloakChangeTime);
	CNetworkVar(float, m_flMalfunctionUntilTime);
};