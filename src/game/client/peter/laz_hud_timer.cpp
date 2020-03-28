#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "hud_basetimer.h"
#include "teamplay_round_timer.h"
#include "c_team_objectiveresource.h"
#include "teamplayroundbased_gamerules.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/IVGui.h"

class CLazHudTimer : public CHudElement, public CHudBaseTimer
{
	DECLARE_CLASS_SIMPLE(CLazHudTimer, CHudBaseTimer);

public:
	CLazHudTimer(const char* pElementName);

	virtual void				Init(void);
	virtual void				Reset(void);
	virtual void				OnTick();
	bool ShouldDraw();
	virtual void FireGameEvent(IGameEvent* event);

protected:
	bool m_bHasTimer;
	int m_iCurrentTimer;
};

DECLARE_HUDELEMENT(CLazHudTimer);

CLazHudTimer::CLazHudTimer(const char* pElementName) : BaseClass(NULL, "HudTimerLaz"), CHudElement(pElementName)
{
	SetHiddenBits(HIDEHUD_MISCSTATUS);
}

void CLazHudTimer::Init(void)
{
	vgui::ivgui()->AddTickSignal(GetVPanel(), 200);

	ListenForGameEvent("teamplay_timer_time_added");
}

void CLazHudTimer::Reset(void)
{
	BaseClass::Reset();

	m_bHasTimer = false;
	m_iCurrentTimer = 0;
}

void CLazHudTimer::FireGameEvent(IGameEvent* event)
{
	const char* name = event->GetName();
	if (m_bHasTimer && V_stricmp("teamplay_timer_time_added", name) == 0)
	{
		int iTimer = event->GetInt("timer");
		if (iTimer == m_iCurrentTimer)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("TimerTimeAdded");
		}
	}
}

bool CLazHudTimer::ShouldDraw()
{
	bool bNeedsDraw = false;

	C_BaseTeamObjectiveResource* pObjective = ObjectiveResource();
	if (!pObjective)
		return false;

	bNeedsDraw = m_bHasTimer;

	return (bNeedsDraw && CHudElement::ShouldDraw());
}

void CLazHudTimer::OnTick(void)
{
	C_BaseTeamObjectiveResource* pObjective = ObjectiveResource();

	if (pObjective)
	{
		int iActiveTimer = pObjective->GetTimerToShowInHUD();
		CTeamRoundTimer* pTimer = dynamic_cast<CTeamRoundTimer*>(ClientEntityList().GetEnt(iActiveTimer));
		// get the time remaining (in seconds)
		if (iActiveTimer != 0 && pTimer && !pTimer->IsDormant() && pTimer->ShowInHud())
		{
			//int nTotalTime = pTimer->GetTimerMaxLength();
			int nTimeRemaining = pTimer->GetTimeRemaining();

			// set our label
			int nMinutes = 0;
			int nSeconds = 0;

			if (nTimeRemaining <= 0)
			{
				nMinutes = 0;
				nSeconds = 0;
			}
			else
			{
				nMinutes = nTimeRemaining / 60;
				nSeconds = nTimeRemaining % 60;
			}

			SetMinutes(nMinutes);
			SetSeconds(nSeconds);

			m_iCurrentTimer = iActiveTimer;
			m_bHasTimer = true;

			if (TeamplayRoundBasedRules()->IsInWaitingForPlayers())
			{
				SetLabelText(L"Waiting for Players");
			}
			else if (TeamplayRoundBasedRules()->InSetup())
			{
				SetLabelText(L"Setup");
			}
			else if (TeamplayRoundBasedRules()->InStalemate())
			{
				SetLabelText(L"Sudden Death");
			}
			else if (TeamplayRoundBasedRules()->InOvertime())
			{
				SetLabelText(L"Overtime");
			}
			else
			{
				SetLabelText(L"");
			}

			return;
		}
	}

	SetMinutes(0);
	SetSeconds(0);
	SetLabelText(L"");
	m_bHasTimer = false;
	m_iCurrentTimer = 0;
}
