//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "C_VGuiScreen.h"
#include <vgui/IVGUI.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "lazuul_gamerules.h"
#include "c_team_objectiveresource.h"
#include "teamplay_round_timer.h"

#pragma region RESPAWN_SCREEN
//-----------------------------------------------------------------------------
// Base class for all vgui screens on objects: 
//-----------------------------------------------------------------------------
class CRespawnWaveVGuiScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS( CRespawnWaveVGuiScreen, CVGuiScreenPanel );

public:
	CRespawnWaveVGuiScreen( vgui::Panel *parent, const char *panelName );

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void OnTick();

private:
	vgui::Label *m_pTime1RemainingLabel;
};


//-----------------------------------------------------------------------------
// Standard VGUI panel for objects 
//-----------------------------------------------------------------------------
DECLARE_VGUI_SCREEN_FACTORY( CRespawnWaveVGuiScreen, "respawn_wave_screen" );


//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CRespawnWaveVGuiScreen::CRespawnWaveVGuiScreen( vgui::Panel *parent, const char *panelName )
	: BaseClass( parent, panelName ) 
{
}


//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CRespawnWaveVGuiScreen::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	// Load all of the controls in
	if (!BaseClass::Init(pKeyValues, pInitData))
		return false;

	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	// Grab ahold of certain well-known controls
	// NOTE: it is valid for these controls to not exist!
	m_pTime1RemainingLabel = dynamic_cast<vgui::Label*>(FindChildByName( "RespawnTimeRemaining" ));
	
	return true;
}


//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CRespawnWaveVGuiScreen::OnTick()
{
	BaseClass::OnTick();

	if (!GetEntity())
		return;

	int nTime1Remaining = 0;
	if (g_pGameRules)
	{
		nTime1Remaining = Max(LazuulRules()->GetNextRespawnWave(GetEntity()->GetTeamNumber(), nullptr) - gpGlobals->curtime, 0.f);
	}

	char buf[32];
	if (m_pTime1RemainingLabel)
	{
		Q_snprintf( buf, 256, "%d", nTime1Remaining );
		m_pTime1RemainingLabel->SetText( buf );
	}
}
#pragma endregion

#pragma region ROUND_TIMER
class CRoundTimerVGUIScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS(CRoundTimerVGUIScreen, CVGuiScreenPanel);

public:
	CRoundTimerVGUIScreen(vgui::Panel* parent, const char* panelName);

	virtual bool Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData);
	virtual void OnTick();

private:
	vgui::Label* m_pTime1RemainingLabel;
};

//-----------------------------------------------------------------------------
// Standard VGUI panel for objects 
//-----------------------------------------------------------------------------
DECLARE_VGUI_SCREEN_FACTORY(CRoundTimerVGUIScreen, "round_timer_screen");


//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CRoundTimerVGUIScreen::CRoundTimerVGUIScreen(vgui::Panel* parent, const char* panelName)
	: BaseClass(parent, panelName)
{
}


//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CRoundTimerVGUIScreen::Init(KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData)
{
	// Load all of the controls in
	if (!BaseClass::Init(pKeyValues, pInitData))
		return false;

	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal(GetVPanel());

	// Grab ahold of certain well-known controls
	// NOTE: it is valid for these controls to not exist!
	m_pTime1RemainingLabel = dynamic_cast<vgui::Label*>(FindChildByName("RoundTimeRemaining"));
	//SetDialogVariable("roundstate", L"");
	return true;
}


//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CRoundTimerVGUIScreen::OnTick()
{
	BaseClass::OnTick();

	if (!GetEntity())
		return;

	int nTime1Remaining = 0;
	if (ObjectiveResource())
	{
		int iActiveTimer = ObjectiveResource()->GetTimerToShowInHUD();
		CTeamRoundTimer* pTimer = dynamic_cast<CTeamRoundTimer*>(ClientEntityList().GetEnt(iActiveTimer));
		// get the time remaining (in seconds)
		if (iActiveTimer != 0 && pTimer && !pTimer->IsDormant() && pTimer->ShowInHud())
		{
			//int nTotalTime = pTimer->GetTimerMaxLength();
			nTime1Remaining = pTimer->GetTimeRemaining();

			/*if (TeamplayRoundBasedRules()->IsInWaitingForPlayers())
			{
				SetDialogVariable("roundstate", L"Waiting for Players");
			}
			else if (TeamplayRoundBasedRules()->InSetup())
			{
				SetDialogVariable("roundstate", L"Setup");
			}
			else if (TeamplayRoundBasedRules()->InStalemate())
			{
				SetDialogVariable("roundstate", L"Sudden Death");
			}
			else if (TeamplayRoundBasedRules()->InOvertime())
			{
				SetDialogVariable("roundstate", L"Overtime");
			}
			else
			{
				SetDialogVariable("roundstate", L"");
			}*/
		}
	}

	char buf[32];
	if (m_pTime1RemainingLabel)
	{
		int nMinutes = 0;
		int nSeconds = 0;

		if (nTime1Remaining <= 0)
		{
			nMinutes = 0;
			nSeconds = 0;
		}
		else
		{
			nMinutes = nTime1Remaining / 60;
			nSeconds = nTime1Remaining % 60;
		}

		Q_snprintf(buf, 256, "%.2d:%.2d", nMinutes, nSeconds);
		m_pTime1RemainingLabel->SetText(buf);
	}
}
#pragma endregion