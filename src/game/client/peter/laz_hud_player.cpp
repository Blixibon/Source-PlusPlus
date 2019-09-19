#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include "clientmode.h"
#include "gamerules.h"

//-----------------------------------------------------------------------------
// Purpose: Shows the sprint power bar
//-----------------------------------------------------------------------------
class CHudPlayerInfo : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudPlayerInfo, vgui::EditablePanel);

public:
	CHudPlayerInfo(const char* pElementName);
	bool			ShouldDraw(void);
};

DECLARE_HUDELEMENT(CHudPlayerInfo);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudPlayerInfo::CHudPlayerInfo(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudPlayerInfo")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits(HIDEHUD_MISCSTATUS);

	LoadControlSettings("resource/UI/LazHudPlayerInfo.res");
	InvalidateLayout();
}

bool CHudPlayerInfo::ShouldDraw(void)
{
	bool bNeedsDraw = false;

	if (!g_pGameRules)
		return false;

	// Draw in multiplayer, if we are on a team, and not waiting for respawn
	bNeedsDraw = (g_pGameRules->IsMultiplayer() && GetLocalPlayerTeam() > LAST_SHARED_TEAM && !IsLocalPlayerSpectator());

	return (bNeedsDraw && CHudElement::ShouldDraw());
}
