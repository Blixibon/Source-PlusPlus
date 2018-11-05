#include "cbase.h"
#include "base_mainmenupanel.h"
#include "base_mainmenu.h"
//#include "controls/tf_advbutton.h"
//#include "controls/tf_advslider.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_controls/ImagePanel.h"
//#include "tf_notificationmanager.h"
//#include "c_sdkversionchecker.h"
#include "engine/IEngineSound.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/KeyRepeat.h"
#include "IGameUIFuncs.h"
#include "icommandline.h"
//#include "vgui_avatarimage.h"
#include "gameui/modinfo.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/ControllerMap.h"
#include "fmtstr.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAIN_MENU_INDENT_X360 10

bool IsConsoleUI(void)
{
	ConVarRef var("gameui_xbox");
	return (var.IsValid() && var.GetBool());
}

static CBackgroundMenuButton* CreateMenuButton(vgui::Panel *parent, const char *panelName, const wchar_t *panelText)
{
	CBackgroundMenuButton *pButton = new CBackgroundMenuButton(parent, panelName);
	pButton->SetProportional(true);
	pButton->SetCommand("OpenGameMenu");
	pButton->SetText(panelText);

	return pButton;
}

//-----------------------------------------------------------------------------
// Purpose: General purpose 1 of N menu
//-----------------------------------------------------------------------------
class CGameMenu : public vgui::Menu
{
	DECLARE_CLASS_SIMPLE(CGameMenu, vgui::Menu);

public:
	CGameMenu(vgui::Panel *parent, const char *name) : BaseClass(parent, name)
	{
		if (IsConsoleUI())
		{
			// shows graphic button hints
			m_pConsoleFooter = new CFooterPanel(parent, "MainMenuFooter");

			int iFixedWidth = 245;

#ifdef _X360
			// In low def we need a smaller highlight
			XVIDEO_MODE videoMode;
			XGetVideoMode(&videoMode);
			if (!videoMode.fIsHiDef)
			{
				iFixedWidth = 240;
			}
			else
			{
				iFixedWidth = 350;
			}
#endif

			SetFixedWidth(iFixedWidth);
		}
		else
		{
			m_pConsoleFooter = NULL;
		}
	}

	virtual void ApplySchemeSettings(IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		// make fully transparent
		SetMenuItemHeight(atoi(pScheme->GetResourceString("MainMenu.MenuItemHeight")));
		SetBgColor(Color(0, 0, 0, 0));
		SetBorder(NULL);
	}

	virtual void LayoutMenuBorder()
	{
	}

	virtual void SetVisible(bool state)
	{
		// force to be always visible
		BaseClass::SetVisible(true);
		// move us to the back instead of going invisible
		if (!state)
		{
			ipanel()->MoveToBack(GetVPanel());
		}
	}

	virtual int AddMenuItem(const char *itemName, const char *itemText, const char *command, Panel *target, KeyValues *userData = NULL)
	{
		MenuItem *item = new CGameMenuItem(this, itemName);
		item->AddActionSignalTarget(target);
		item->SetCommand(command);
		item->SetText(itemText);
		item->SetUserData(userData);
		return BaseClass::AddMenuItem(item);
	}

	virtual int AddMenuItem(const char *itemName, const char *itemText, KeyValues *command, Panel *target, KeyValues *userData = NULL)
	{
		CGameMenuItem *item = new CGameMenuItem(this, itemName);
		item->AddActionSignalTarget(target);
		item->SetCommand(command);
		item->SetText(itemText);
		item->SetRightAlignedText(true);
		item->SetUserData(userData);
		return BaseClass::AddMenuItem(item);
	}

	virtual void SetMenuItemBlinkingState(const char *itemName, bool state)
	{
		for (int i = 0; i < GetChildCount(); i++)
		{
			Panel *child = GetChild(i);
			MenuItem *menuItem = dynamic_cast<MenuItem *>(child);
			if (menuItem)
			{
				if (Q_strcmp(menuItem->GetCommand()->GetString("command", ""), itemName) == 0)
				{
					menuItem->SetBlink(state);
				}
			}
		}
		InvalidateLayout();
	}

	virtual void OnCommand(const char *command)
	{
		m_KeyRepeat.Reset();

		if (!stricmp(command, "Open"))
		{
			MoveToFront();
			RequestFocus();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	virtual void OnKeyCodePressed(KeyCode code)
	{
		if (IsX360())
		{
			if (GetAlpha() != 255)
			{
				SetEnabled(false);
				// inhibit key activity during transitions
				return;
			}

			SetEnabled(true);

			if (code == KEY_XBUTTON_B || code == KEY_XBUTTON_START)
			{
				if (MAINMENU_ROOT->InGame())
				{
					GetParent()->OnCommand("ResumeGame");
				}
				return;
			}
		}

		m_KeyRepeat.KeyDown(code);

		BaseClass::OnKeyCodePressed(code);

		// HACK: Allow F key bindings to operate even here
		if (IsPC() && code >= KEY_F1 && code <= KEY_F12)
		{
			// See if there is a binding for the FKey
			const char *binding = gameuifuncs->GetBindingForButtonCode(code);
			if (binding && binding[0])
			{
				// submit the entry as a console commmand
				char szCommand[256];
				Q_strncpy(szCommand, binding, sizeof(szCommand));
				engine->ClientCmd_Unrestricted(szCommand);
			}
		}
	}

	void OnKeyCodeReleased(vgui::KeyCode code)
	{
		m_KeyRepeat.KeyUp(code);

		BaseClass::OnKeyCodeReleased(code);
	}

	void OnThink()
	{
		vgui::KeyCode code = m_KeyRepeat.KeyRepeated();
		if (code)
		{
			OnKeyCodeTyped(code);
		}

		BaseClass::OnThink();
	}

	virtual void OnKillFocus()
	{
		BaseClass::OnKillFocus();

		// force us to the rear when we lose focus (so it looks like the menu is always on the background)
		surface()->MovePopupToBack(GetVPanel());

		m_KeyRepeat.Reset();
	}

	void ShowFooter(bool bShow)
	{
		if (m_pConsoleFooter)
		{
			m_pConsoleFooter->SetVisible(bShow);
		}
	}

	void UpdateMenuItemState(bool isInGame, bool isMultiplayer)
	{
		bool isSteam = IsPC() && (CommandLine()->FindParm("-steam") != 0);
		bool bIsConsoleUI = IsConsoleUI();

		// disabled save button if we're not in a game
		for (int i = 0; i < GetChildCount(); i++)
		{
			Panel *child = GetChild(i);
			MenuItem *menuItem = dynamic_cast<MenuItem *>(child);
			if (menuItem)
			{
				bool shouldBeVisible = true;
				// filter the visibility
				KeyValues *kv = menuItem->GetUserData();
				if (!kv)
					continue;

				if (!isInGame && kv->GetInt("OnlyInGame"))
				{
					shouldBeVisible = false;
				}
				else if (isMultiplayer && kv->GetInt("notmulti"))
				{
					shouldBeVisible = false;
				}
				else if (isInGame && !isMultiplayer && kv->GetInt("notsingle"))
				{
					shouldBeVisible = false;
				}
				else if (isSteam && kv->GetInt("notsteam"))
				{
					shouldBeVisible = false;
				}
				else if (!bIsConsoleUI && kv->GetInt("ConsoleOnly"))
				{
					shouldBeVisible = false;
				}

				menuItem->SetVisible(shouldBeVisible);
			}
		}

		if (!isInGame)
		{
			// Sort them into their original order
			for (int j = 0; j < GetChildCount() - 2; j++)
			{
				MoveMenuItem(j, j + 1);
			}
		}
		else
		{
			// Sort them into their in game order
			for (int i = 0; i < GetChildCount(); i++)
			{
				for (int j = i; j < GetChildCount() - 2; j++)
				{
					int iID1 = GetMenuID(j);
					int iID2 = GetMenuID(j + 1);

					MenuItem *menuItem1 = GetMenuItem(iID1);
					MenuItem *menuItem2 = GetMenuItem(iID2);

					KeyValues *kv1 = menuItem1->GetUserData();
					KeyValues *kv2 = menuItem2->GetUserData();

					if (kv1->GetInt("InGameOrder") > kv2->GetInt("InGameOrder"))
						MoveMenuItem(iID2, iID1);
				}
			}
		}

		InvalidateLayout();

		if (m_pConsoleFooter)
		{
			// update the console footer
			const char *pHelpName;
			if (!isInGame)
				pHelpName = "MainMenu";
			else
				pHelpName = "GameMenu";

			if (!m_pConsoleFooter->GetHelpName() || V_stricmp(pHelpName, m_pConsoleFooter->GetHelpName()))
			{
				// game menu must re-establish its own help once it becomes re-active
				m_pConsoleFooter->SetHelpNameAndReset(pHelpName);
				m_pConsoleFooter->AddNewButtonLabel("#GameUI_Action", "#GameUI_Icons_A_BUTTON");
				if (isInGame)
				{
					m_pConsoleFooter->AddNewButtonLabel("#GameUI_Close", "#GameUI_Icons_B_BUTTON");
				}
			}
		}
	}

private:
	CFooterPanel *m_pConsoleFooter;
	vgui::CKeyRepeatHandler	m_KeyRepeat;
};

CGameMenuItem::CGameMenuItem(vgui::Menu *parent, const char *name) : BaseClass(parent, name, "GameMenuItem")
{
	m_bRightAligned = false;
}

void CGameMenuItem::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	// make fully transparent
	SetFgColor(GetSchemeColor("MainMenu.TextColor", pScheme));
	SetBgColor(Color(0, 0, 0, 0));
	SetDefaultColor(GetSchemeColor("MainMenu.TextColor", pScheme), Color(0, 0, 0, 0));
	SetArmedColor(GetSchemeColor("MainMenu.ArmedTextColor", pScheme), Color(0, 0, 0, 0));
	SetDepressedColor(GetSchemeColor("MainMenu.DepressedTextColor", pScheme), Color(0, 0, 0, 0));
	SetContentAlignment(Label::a_west);
	SetBorder(NULL);
	SetDefaultBorder(NULL);
	SetDepressedBorder(NULL);
	SetKeyFocusBorder(NULL);

	vgui::HFont hMainMenuFont = pScheme->GetFont("MainMenuFont", IsProportional());

	if (hMainMenuFont)
	{
		SetFont(hMainMenuFont);
	}
	else
	{
		SetFont(pScheme->GetFont("MenuLarge", IsProportional()));
	}
	SetTextInset(0, 0);
	SetArmedSound("UI/buttonrollover.wav");
	SetDepressedSound("UI/buttonclick.wav");
	SetReleasedSound("UI/buttonclickrelease.wav");
	SetButtonActivationType(Button::ACTIVATE_ONPRESSED);

	if (IsConsoleUI())
	{
		SetArmedColor(GetSchemeColor("MainMenu.ArmedTextColor", pScheme), GetSchemeColor("Button.ArmedBgColor", pScheme));
		SetTextInset(MAIN_MENU_INDENT_X360, 0);
	}

	if (m_bRightAligned)
	{
		SetContentAlignment(Label::a_east);
	}
}

void CGameMenuItem::PaintBackground()
{
	if (!IsConsoleUI())
	{
		BaseClass::PaintBackground();
	}
	else
	{
		if (!IsArmed() || !IsVisible() || GetParent()->GetAlpha() < 32)
			return;

		int wide, tall;
		GetSize(wide, tall);

		DrawBoxFade(0, 0, wide, tall, GetButtonBgColor(), 1.0f, 255, 0, true);
		DrawBoxFade(2, 2, wide - 4, tall - 4, Color(0, 0, 0, 96), 1.0f, 255, 0, true);
	}
}

void CGameMenuItem::SetRightAlignedText(bool state)
{
	m_bRightAligned = state;
}

//-----------------------------------------------------------------------------
// Purpose: xbox UI panel that displays button icons and help text for all menus
//-----------------------------------------------------------------------------
CFooterPanel::CFooterPanel(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	SetVisible(true);
	SetAlpha(0);
	m_pHelpName = NULL;

	m_pSizingLabel = new vgui::Label(this, "SizingLabel", "");
	m_pSizingLabel->SetVisible(false);

	m_nButtonGap = 32;
	m_nButtonGapDefault = 32;
	m_ButtonPinRight = 100;
	m_FooterTall = 80;

	int wide, tall;
	surface()->GetScreenSize(wide, tall);

	if (tall <= 480)
	{
		m_FooterTall = 60;
	}

	m_ButtonOffsetFromTop = 0;
	m_ButtonSeparator = 4;
	m_TextAdjust = 0;

	m_bPaintBackground = false;
	m_bCenterHorizontal = false;

	m_szButtonFont[0] = '\0';
	m_szTextFont[0] = '\0';
	m_szFGColor[0] = '\0';
	m_szBGColor[0] = '\0';
}

CFooterPanel::~CFooterPanel()
{
	SetHelpNameAndReset(NULL);

	delete m_pSizingLabel;
}

//-----------------------------------------------------------------------------
// Purpose: apply scheme settings
//-----------------------------------------------------------------------------
void CFooterPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hButtonFont = pScheme->GetFont((m_szButtonFont[0] != '\0') ? m_szButtonFont : "GameUIButtons");
	m_hTextFont = pScheme->GetFont((m_szTextFont[0] != '\0') ? m_szTextFont : "MenuLarge");

	SetFgColor(pScheme->GetColor(m_szFGColor, Color(255, 255, 255, 255)));
	SetBgColor(pScheme->GetColor(m_szBGColor, Color(0, 0, 0, 255)));

	int x, y, w, h;
	GetParent()->GetBounds(x, y, w, h);
	SetBounds(x, h - m_FooterTall, w, m_FooterTall);
}

//-----------------------------------------------------------------------------
// Purpose: apply settings
//-----------------------------------------------------------------------------
void CFooterPanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	// gap between hints
	m_nButtonGap = inResourceData->GetInt("buttongap", 32);
	m_nButtonGapDefault = m_nButtonGap;
	m_ButtonPinRight = inResourceData->GetInt("button_pin_right", 100);
	m_FooterTall = inResourceData->GetInt("tall", 80);
	m_ButtonOffsetFromTop = inResourceData->GetInt("buttonoffsety", 0);
	m_ButtonSeparator = inResourceData->GetInt("button_separator", 4);
	m_TextAdjust = inResourceData->GetInt("textadjust", 0);

	m_bCenterHorizontal = (inResourceData->GetInt("center", 0) == 1);
	m_bPaintBackground = (inResourceData->GetInt("paintbackground", 0) == 1);

	// fonts for text and button
	Q_strncpy(m_szTextFont, inResourceData->GetString("fonttext", "MenuLarge"), sizeof(m_szTextFont));
	Q_strncpy(m_szButtonFont, inResourceData->GetString("fontbutton", "GameUIButtons"), sizeof(m_szButtonFont));

	// fg and bg colors
	Q_strncpy(m_szFGColor, inResourceData->GetString("fgcolor", "White"), sizeof(m_szFGColor));
	Q_strncpy(m_szBGColor, inResourceData->GetString("bgcolor", "Black"), sizeof(m_szBGColor));

	for (KeyValues *pButton = inResourceData->GetFirstSubKey(); pButton != NULL; pButton = pButton->GetNextKey())
	{
		const char *pName = pButton->GetName();

		if (!Q_stricmp(pName, "button"))
		{
			// Add a button to the footer
			const char *pText = pButton->GetString("text", "NULL");
			const char *pIcon = pButton->GetString("icon", "NULL");
			AddNewButtonLabel(pText, pIcon);
		}
	}

	InvalidateLayout(false, true); // force ApplySchemeSettings to run
}

//-----------------------------------------------------------------------------
// Purpose: adds button icons and help text to the footer panel when activating a menu
//-----------------------------------------------------------------------------
void CFooterPanel::AddButtonsFromMap(vgui::Frame *pMenu)
{
	SetHelpNameAndReset(pMenu->GetName());

	CControllerMap *pMap = dynamic_cast<CControllerMap*>(pMenu->FindChildByName("ControllerMap"));
	if (pMap)
	{
		int buttonCt = pMap->NumButtons();
		for (int i = 0; i < buttonCt; ++i)
		{
			const char *pText = pMap->GetBindingText(i);
			if (pText)
			{
				AddNewButtonLabel(pText, pMap->GetBindingIcon(i));
			}
		}
	}
}

void CFooterPanel::SetStandardDialogButtons()
{
	SetHelpNameAndReset("Dialog");
	AddNewButtonLabel("#GameUI_Action", "#GameUI_Icons_A_BUTTON");
	AddNewButtonLabel("#GameUI_Close", "#GameUI_Icons_B_BUTTON");
}

//-----------------------------------------------------------------------------
// Purpose: Caller must tag the button layout. May support reserved names
// to provide stock help layouts trivially.
//-----------------------------------------------------------------------------
void CFooterPanel::SetHelpNameAndReset(const char *pName)
{
	if (m_pHelpName)
	{
		free(m_pHelpName);
		m_pHelpName = NULL;
	}

	if (pName)
	{
		m_pHelpName = strdup(pName);
	}

	ClearButtons();
}

//-----------------------------------------------------------------------------
// Purpose: Caller must tag the button layout
//-----------------------------------------------------------------------------
const char *CFooterPanel::GetHelpName()
{
	return m_pHelpName;
}

void CFooterPanel::ClearButtons(void)
{
	m_ButtonLabels.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: creates a new button label with icon and text
//-----------------------------------------------------------------------------
void CFooterPanel::AddNewButtonLabel(const char *text, const char *icon)
{
	ButtonLabel_t *button = new ButtonLabel_t;

	Q_strncpy(button->name, text, MAX_PATH);
	button->bVisible = true;

	// Button icons are a single character
	wchar_t *pIcon = g_pVGuiLocalize->Find(icon);
	if (pIcon)
	{
		button->icon[0] = pIcon[0];
		button->icon[1] = '\0';
	}
	else
	{
		button->icon[0] = '\0';
	}

	// Set the help text
	wchar_t *pText = g_pVGuiLocalize->Find(text);
	if (pText)
	{
		wcsncpy(button->text, pText, wcslen(pText) + 1);
	}
	else
	{
		button->text[0] = '\0';
	}

	m_ButtonLabels.AddToTail(button);
}

//-----------------------------------------------------------------------------
// Purpose: Shows/Hides a button label
//-----------------------------------------------------------------------------
void CFooterPanel::ShowButtonLabel(const char *name, bool show)
{
	for (int i = 0; i < m_ButtonLabels.Count(); ++i)
	{
		if (!Q_stricmp(m_ButtonLabels[i]->name, name))
		{
			m_ButtonLabels[i]->bVisible = show;
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Changes a button's text
//-----------------------------------------------------------------------------
void CFooterPanel::SetButtonText(const char *buttonName, const char *text)
{
	for (int i = 0; i < m_ButtonLabels.Count(); ++i)
	{
		if (!Q_stricmp(m_ButtonLabels[i]->name, buttonName))
		{
			wchar_t *wtext = g_pVGuiLocalize->Find(text);
			if (text)
			{
				wcsncpy(m_ButtonLabels[i]->text, wtext, wcslen(wtext) + 1);
			}
			else
			{
				m_ButtonLabels[i]->text[0] = '\0';
			}
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Footer panel background rendering
//-----------------------------------------------------------------------------
void CFooterPanel::PaintBackground(void)
{
	if (!m_bPaintBackground)
		return;

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: Footer panel rendering
//-----------------------------------------------------------------------------
void CFooterPanel::Paint(void)
{
	// inset from right edge
	int wide = GetWide();
	int right = wide - m_ButtonPinRight;

	// center the text within the button
	int buttonHeight = vgui::surface()->GetFontTall(m_hButtonFont);
	int fontHeight = vgui::surface()->GetFontTall(m_hTextFont);
	int textY = (buttonHeight - fontHeight) / 2 + m_TextAdjust;

	if (textY < 0)
	{
		textY = 0;
	}

	int y = m_ButtonOffsetFromTop;

	if (!m_bCenterHorizontal)
	{
		// draw the buttons, right to left
		int x = right;

		for (int i = 0; i < m_ButtonLabels.Count(); ++i)
		{
			ButtonLabel_t *pButton = m_ButtonLabels[i];
			if (!pButton->bVisible)
				continue;

			// Get the string length
			m_pSizingLabel->SetFont(m_hTextFont);
			m_pSizingLabel->SetText(pButton->text);
			m_pSizingLabel->SizeToContents();

			int iTextWidth = m_pSizingLabel->GetWide();

			if (iTextWidth == 0)
				x += m_nButtonGap;	// There's no text, so remove the gap between buttons
			else
				x -= iTextWidth;

			// Draw the string
			vgui::surface()->DrawSetTextFont(m_hTextFont);
			vgui::surface()->DrawSetTextColor(GetFgColor());
			vgui::surface()->DrawSetTextPos(x, y + textY);
			vgui::surface()->DrawPrintText(pButton->text, wcslen(pButton->text));

			// Draw the button
			// back up button width and a little extra to leave a gap between button and text
			x -= (vgui::surface()->GetCharacterWidth(m_hButtonFont, pButton->icon[0]) + m_ButtonSeparator);
			vgui::surface()->DrawSetTextFont(m_hButtonFont);
			vgui::surface()->DrawSetTextColor(255, 255, 255, 255);
			vgui::surface()->DrawSetTextPos(x, y);
			vgui::surface()->DrawPrintText(pButton->icon, 1);

			// back up to next string
			x -= m_nButtonGap;
		}
	}
	else
	{
		// center the buttons (as a group)
		int x = wide / 2;
		int totalWidth = 0;
		int i = 0;
		int nButtonCount = 0;

		// need to loop through and figure out how wide our buttons and text are (with gaps between) so we can offset from the center
		for (i = 0; i < m_ButtonLabels.Count(); ++i)
		{
			ButtonLabel_t *pButton = m_ButtonLabels[i];
			if (!pButton->bVisible)
				continue;

			// Get the string length
			m_pSizingLabel->SetFont(m_hTextFont);
			m_pSizingLabel->SetText(pButton->text);
			m_pSizingLabel->SizeToContents();

			totalWidth += vgui::surface()->GetCharacterWidth(m_hButtonFont, pButton->icon[0]);
			totalWidth += m_ButtonSeparator;
			totalWidth += m_pSizingLabel->GetWide();

			nButtonCount++; // keep track of how many active buttons we'll be drawing
		}

		totalWidth += (nButtonCount - 1) * m_nButtonGap; // add in the gaps between the buttons
		x -= (totalWidth / 2);

		for (i = 0; i < m_ButtonLabels.Count(); ++i)
		{
			ButtonLabel_t *pButton = m_ButtonLabels[i];
			if (!pButton->bVisible)
				continue;

			// Get the string length
			m_pSizingLabel->SetFont(m_hTextFont);
			m_pSizingLabel->SetText(pButton->text);
			m_pSizingLabel->SizeToContents();

			int iTextWidth = m_pSizingLabel->GetWide();

			// Draw the icon
			vgui::surface()->DrawSetTextFont(m_hButtonFont);
			vgui::surface()->DrawSetTextColor(255, 255, 255, 255);
			vgui::surface()->DrawSetTextPos(x, y);
			vgui::surface()->DrawPrintText(pButton->icon, 1);
			x += vgui::surface()->GetCharacterWidth(m_hButtonFont, pButton->icon[0]) + m_ButtonSeparator;

			// Draw the string
			vgui::surface()->DrawSetTextFont(m_hTextFont);
			vgui::surface()->DrawSetTextColor(GetFgColor());
			vgui::surface()->DrawSetTextPos(x, y + textY);
			vgui::surface()->DrawPrintText(pButton->text, wcslen(pButton->text));

			x += iTextWidth + m_nButtonGap;
		}
	}
}

DECLARE_BUILD_FACTORY(CFooterPanel);

//-----------------------------------------------------------------------------
// Purpose: Editable panel that can replace the GameMenuButtons in CBasePanel
//-----------------------------------------------------------------------------
CMainMenuGameLogo::CMainMenuGameLogo(vgui::Panel *parent, const char *name) : vgui::EditablePanel(parent, name)
{
	m_nOffsetX = 0;
	m_nOffsetY = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMainMenuGameLogo::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	m_nOffsetX = inResourceData->GetInt("offsetX", 0);
	m_nOffsetY = inResourceData->GetInt("offsetY", 0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMainMenuGameLogo::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("Resource/GameLogo.res");
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMainMenuPanel::CTFMainMenuPanel(vgui::Panel* parent, const char *panelName) : CTFMenuPanelBase(parent, panelName)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMainMenuPanel::~CTFMainMenuPanel()
{

}

bool CTFMainMenuPanel::Init()
{
	BaseClass::Init();

	ModInfo().LoadCurrentGameInfo();

	if (steamapicontext->SteamUser())
	{
		m_SteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	m_iShowFakeIntro = 4;
	/*m_pVersionLabel = NULL;
	m_pNotificationButton = NULL;
	m_pProfileAvatar = NULL;*/
	m_pFakeBGImage = NULL;
	/*m_pBlogPanel = new CTFBlogPanel(this, "BlogPanel");
	m_pServerlistPanel = new CTFServerlistPanel(this, "ServerlistPanel");*/

	bInMenu = true;
	bInGame = true;

	m_pGameMenuButtons.AddToTail(CreateMenuButton(this, "GameMenuButton", ModInfo().GetGameTitle()));
	m_pGameMenuButtons.AddToTail(CreateMenuButton(this, "GameMenuButton2", ModInfo().GetGameTitle2()));
#ifdef CS_BETA
	if (!ModInfo().NoCrosshair()) // hack to not show the BETA for HL2 or HL1Port
	{
		m_pGameMenuButtons.AddToTail(CreateMenuButton(this, "BetaButton", L"BETA"));
	}
#endif // CS_BETA

	m_pGameMenu = NULL;
	m_pGameLogo = NULL;


	CreateGameMenu();
	CreateGameLogo();

	return true;
}


void CTFMainMenuPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	int i = 0;

	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/UI/main_menu/BaseMainMenuPanel.res");
	/*m_pVersionLabel = dynamic_cast<CExLabel *>(FindChildByName("VersionLabel"));
	m_pNotificationButton = dynamic_cast<CTFAdvButton *>(FindChildByName("NotificationButton"));
	m_pProfileAvatar = dynamic_cast<CAvatarImagePanel *>(FindChildByName("AvatarImage"));*/
	m_pFakeBGImage = dynamic_cast<vgui::ImagePanel *>(FindChildByName("FakeBGImage"));

	//SetVersionLabel();

	m_iGameMenuInset = atoi(pScheme->GetResourceString("MainMenu.Inset"));
	m_iGameMenuInset *= 2;

	IScheme *pClientScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("ClientScheme"));
	CUtlVector< Color > buttonColor;
	if (pClientScheme)
	{
		m_iGameTitlePos.RemoveAll();
		for (i = 0; i < m_pGameMenuButtons.Count(); ++i)
		{
			m_pGameMenuButtons[i]->SetFont(pClientScheme->GetFont("ClientTitleFont", true));
			m_iGameTitlePos.AddToTail(coord());
			m_iGameTitlePos[i].x = atoi(pClientScheme->GetResourceString(CFmtStr("Main.Title%d.X", i + 1)));
			m_iGameTitlePos[i].x = scheme()->GetProportionalScaledValue(m_iGameTitlePos[i].x);
			m_iGameTitlePos[i].y = atoi(pClientScheme->GetResourceString(CFmtStr("Main.Title%d.Y", i + 1)));
			m_iGameTitlePos[i].y = scheme()->GetProportionalScaledValue(m_iGameTitlePos[i].y);

			if (IsConsoleUI())
				m_iGameTitlePos[i].x += MAIN_MENU_INDENT_X360;

			buttonColor.AddToTail(pClientScheme->GetColor(CFmtStr("Main.Title%d.Color", i + 1), Color(255, 255, 255, 255)));
		}
#ifdef CS_BETA
		if (!ModInfo().NoCrosshair()) // hack to not show the BETA for HL2 or HL1Port
		{
			m_pGameMenuButtons[m_pGameMenuButtons.Count() - 1]->SetFont(pClientScheme->GetFont("BetaFont", true));
		}
#endif // CS_BETA

		m_iGameMenuPos.x = atoi(pClientScheme->GetResourceString("Main.Menu.X"));
		m_iGameMenuPos.x = scheme()->GetProportionalScaledValue(m_iGameMenuPos.x);
		m_iGameMenuPos.y = atoi(pClientScheme->GetResourceString("Main.Menu.Y"));
		m_iGameMenuPos.y = scheme()->GetProportionalScaledValue(m_iGameMenuPos.y);

		m_iGameMenuInset = atoi(pClientScheme->GetResourceString("Main.BottomBorder"));
		m_iGameMenuInset = scheme()->GetProportionalScaledValue(m_iGameMenuInset);
	}
	else
	{
		for (i = 0; i < m_pGameMenuButtons.Count(); ++i)
		{
			m_pGameMenuButtons[i]->SetFont(pScheme->GetFont("TitleFont"));
			buttonColor.AddToTail(Color(255, 255, 255, 255));
		}
	}

	for (i = 0; i < m_pGameMenuButtons.Count(); ++i)
	{
		m_pGameMenuButtons[i]->SetDefaultColor(buttonColor[i], Color(0, 0, 0, 0));
		m_pGameMenuButtons[i]->SetArmedColor(buttonColor[i], Color(0, 0, 0, 0));
		m_pGameMenuButtons[i]->SetDepressedColor(buttonColor[i], Color(0, 0, 0, 0));
	}
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMainMenuPanel::CreateGameMenu()
{
	// load settings from config file
	KeyValues *datafile = new KeyValues("GameMenu");
	datafile->UsesEscapeSequences(true);	// VGUI uses escape sequences
	if (datafile->LoadFromFile(g_pFullFileSystem, "Resource/GameMenu.res"))
	{
		m_pGameMenu = RecursiveLoadGameMenu(datafile);
	}

	if (!m_pGameMenu)
	{
		Error("Could not load file Resource/GameMenu.res");
	}
	else
	{
		// start invisible
		SETUP_PANEL(m_pGameMenu);
		//m_pGameMenu->SetAlpha(0);
	}

	datafile->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMainMenuPanel::CreateGameLogo()
{
	if (ModInfo().UseGameLogo())
	{
		m_pGameLogo = new CMainMenuGameLogo(this, "GameLogo");

		if (m_pGameLogo)
		{
			SETUP_PANEL(m_pGameLogo);
			m_pGameLogo->InvalidateLayout(true, true);

			// start invisible
			m_pGameLogo->SetAlpha(0);
		}
	}
	else
	{
		m_pGameLogo = NULL;
	}
}

void CTFMainMenuPanel::CheckBonusBlinkState()
{
#ifdef _X360
	// On 360 if we have a storage device at this point and try to read the bonus data it can't find the bonus file!
	return;
#endif

	//if (BonusMapsDatabase()->GetBlink())
	//{
	//	if (GameUI().IsConsoleUI())
	//		SetMenuItemBlinkingState("OpenNewGameDialog", true);	// Consoles integrate bonus maps menu into the new game menu
	//	else
	//		SetMenuItemBlinkingState("OpenBonusMapsDialog", true);
	//}
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if menu items need to be enabled/disabled
//-----------------------------------------------------------------------------
void CTFMainMenuPanel::UpdateGameMenus()
{
	// check our current state
	bool isInGame = MAINMENU_ROOT->InGame();
	bool isMulti = isInGame && (engine->GetMaxClients() > 1);

	// iterate all the menu items
	m_pGameMenu->UpdateMenuItemState(isInGame, isMulti);

	// position the menu
	InvalidateLayout();
	m_pGameMenu->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: sets up the game menu from the keyvalues
//			the game menu is hierarchial, so this is recursive
//-----------------------------------------------------------------------------
CGameMenu *CTFMainMenuPanel::RecursiveLoadGameMenu(KeyValues *datafile)
{
	CGameMenu *menu = new CGameMenu(this, datafile->GetName());

	// loop through all the data adding items to the menu
	for (KeyValues *dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		const char *label = dat->GetString("label", "<unknown>");
		const char *cmd = dat->GetString("command", NULL);
		const char *name = dat->GetString("name", label);

		//		if ( !Q_stricmp( cmd, "OpenFriendsDialog" ) && bSteamCommunityFriendsVersion )
		//			continue;

		menu->AddMenuItem(name, label, cmd, this, dat);
	}

	return menu;
}

void CTFMainMenuPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	/*if (m_pProfileAvatar)
	{
		m_pProfileAvatar->SetPlayer(m_SteamID, k_EAvatarSize64x64);
		m_pProfileAvatar->SetShouldDrawFriendIcon(false);
	}*/

	char szNickName[64];
	Q_snprintf(szNickName, sizeof(szNickName),
		(steamapicontext->SteamFriends() ? steamapicontext->SteamFriends()->GetPersonaName() : "Unknown"));
	SetDialogVariable("nickname", szNickName);

	//ShowBlogPanel(tf2c_mainmenu_showblog.GetBool());
	
	AutoLayout();

	if (m_iShowFakeIntro > 0)
	{
		char szBGName[128];
		engine->GetMainMenuBackgroundName(szBGName, sizeof(szBGName));
		char szImage[128];
		Q_snprintf(szImage, sizeof(szImage), "../console/%s", szBGName);
		int width, height;
		surface()->GetScreenSize(width, height);
		float fRatio = (float)width / (float)height;
		bool bWidescreen = (fRatio < 1.5 ? false : true);
		if (bWidescreen)
			Q_strcat(szImage, "_widescreen", sizeof(szImage));
		m_pFakeBGImage->SetImage(szImage);
		m_pFakeBGImage->SetVisible(true);
		m_pFakeBGImage->SetAlpha(255);
	}

	// Get the screen size
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	// Get the size of the menu
	int menuWide, menuTall;
	m_pGameMenu->GetSize(menuWide, menuTall);

	int idealMenuY = m_iGameMenuPos.y;
	if (idealMenuY + menuTall + m_iGameMenuInset > tall)
	{
		idealMenuY = tall - menuTall - m_iGameMenuInset;
	}

	int yDiff = idealMenuY - m_iGameMenuPos.y;

	for (int i = 0; i < m_pGameMenuButtons.Count(); ++i)
	{
		// Get the size of the logo text
		//int textWide, textTall;
		m_pGameMenuButtons[i]->SizeToContents();
		//vgui::surface()->GetTextSize( m_pGameMenuButtons[i]->GetFont(), ModInfo().GetGameTitle(), textWide, textTall );

		// place menu buttons above middle of screen
		m_pGameMenuButtons[i]->SetPos(m_iGameTitlePos[i].x, m_iGameTitlePos[i].y + yDiff);
		//m_pGameMenuButtons[i]->SetSize(textWide + 4, textTall + 4);
	}

	if (m_pGameLogo)
	{
		// move the logo to sit right on top of the menu
		m_pGameLogo->SetPos(m_iGameMenuPos.x + m_pGameLogo->GetOffsetX(), idealMenuY - m_pGameLogo->GetTall() + m_pGameLogo->GetOffsetY());
	}

	// position self along middle of screen
	if (IsConsoleUI())
	{
		int posx, posy;
		m_pGameMenu->GetPos(posx, posy);
		m_iGameMenuPos.x = posx;
	}
	m_pGameMenu->SetPos(m_iGameMenuPos.x, idealMenuY);

	UpdateGameMenus();
};



void CTFMainMenuPanel::OnCommand(const char* command)
{
	if (!Q_strcmp(command, "OpenNewGameDialog"))
	{

	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CTFMainMenuPanel::OnTick()
{
	BaseClass::OnTick();

	
};

void CTFMainMenuPanel::OnThink()
{
	BaseClass::OnThink();

	if (m_iShowFakeIntro > 0)
	{
		m_iShowFakeIntro--;
		if (m_iShowFakeIntro == 0)
		{
			vgui::GetAnimationController()->RunAnimationCommand(m_pFakeBGImage, "Alpha", 0, 1.0f, 0.5f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
		}
	}	
	if (m_pFakeBGImage->IsVisible() && m_pFakeBGImage->GetAlpha() == 0)
	{
		m_pFakeBGImage->SetVisible(false);
	}
};

void CTFMainMenuPanel::Show()
{
	BaseClass::Show();
	vgui::GetAnimationController()->RunAnimationCommand(this, "Alpha", 255, 0.0f, 0.5f, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE);
};

void CTFMainMenuPanel::Hide()
{
	BaseClass::Hide();
	vgui::GetAnimationController()->RunAnimationCommand(this, "Alpha", 0, 0.0f, 0.1f, vgui::AnimationController::INTERPOLATOR_LINEAR);
};


void CTFMainMenuPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();
	//ShowBlogPanel(tf2c_mainmenu_showblog.GetBool());
};

void CTFMainMenuPanel::GameLayout()
{
	BaseClass::GameLayout();
};


