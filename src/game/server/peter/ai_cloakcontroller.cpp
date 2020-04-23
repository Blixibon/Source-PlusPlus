#include "cbase.h"
#include "ai_cloakcontroller.h"

BEGIN_SIMPLE_DATADESC(CAI_CloakController)

END_DATADESC();

void CAI_CloakController::Init(float flCloakTime, float flUncloakTime)
{
	m_flCloakTime = flCloakTime;
	m_flUncloakTime = flUncloakTime;
	m_bCloaking = false;
	m_flMalfunctionUntilTime = 0.f;
	m_flCloakChangeTime = 0.f;
}

void CAI_CloakController::SetCloaked(bool bCloaked)
{
	if (bCloaked != m_bCloaking.Get())
	{
		m_bCloaking = bCloaked;
		m_flCloakChangeTime = gpGlobals->curtime;
	}
}

void CAI_CloakController::StartMalfunction(float flErrorTime)
{
	m_flMalfunctionUntilTime = gpGlobals->curtime + flErrorTime;
}

CloakState_e CAI_CloakController::GetCloakState()
{
	if (m_flCloakChangeTime.Get() == 0.f)
		return STATE_UNCLOAKED;

	const float flFullyCloakedAtTime = m_flCloakChangeTime.Get() + m_flCloakTime.Get();
	const float flFullyUncloakedAtTime = m_flCloakChangeTime.Get() + m_flUncloakTime.Get();

	if (m_bCloaking.Get())
	{
		float flCloakPct = RemapValClamped(gpGlobals->curtime, m_flCloakChangeTime, flFullyCloakedAtTime, 0.f, 1.f);

		if (flCloakPct < 1.0f)
			return STATE_CLOAKING;
		else
		{
			return (m_flMalfunctionUntilTime.Get() > gpGlobals->curtime) ? STATE_MALFUNTION : STATE_CLOAKED;
		}
	}
	else
	{
		float flCloakPct = RemapValClamped(gpGlobals->curtime, m_flCloakChangeTime, flFullyUncloakedAtTime, 1.f, 0.f);
		if (flCloakPct > 0.f)
			return STATE_UNCLOAKING;
		else
		{
			return STATE_UNCLOAKED;
		}
	}
}
