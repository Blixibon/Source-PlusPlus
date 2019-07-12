#ifndef TFMAINMENUPANEL_H
#define TFMAINMENUPANEL_H

#include "base_menupanelbase.h"
#include "steam/steam_api.h"
#include <vgui_controls/HTML.h>
#include "vgui_controls/MenuItem.h"
#include "gameui/backgroundmenubutton.h"

class CAvatarImagePanel;
class CGameMenu;

//-----------------------------------------------------------------------------
// Purpose: Panel that acts as background for button icons and help text in the UI
//-----------------------------------------------------------------------------
class CFooterPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CFooterPanel, vgui::EditablePanel);

public:
	CFooterPanel(Panel *parent, const char *panelName);
	virtual ~CFooterPanel();

	virtual void	ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void	ApplySettings(KeyValues *pResourceData);
	virtual void	Paint(void);
	virtual void	PaintBackground(void);

	// caller tags the current hint, used to assist in ownership
	void			SetHelpNameAndReset(const char *pName);
	const char		*GetHelpName();

	void			AddButtonsFromMap(vgui::Frame *pMenu);
	void			SetStandardDialogButtons();
	void			AddNewButtonLabel(const char *text, const char *icon);
	void			ShowButtonLabel(const char *name, bool show = true);
	void			SetButtonText(const char *buttonName, const char *text);
	void			ClearButtons();
	void			SetButtonGap(int nButtonGap) { m_nButtonGap = nButtonGap; }
	void			UseDefaultButtonGap() { m_nButtonGap = m_nButtonGapDefault; }

private:
	struct ButtonLabel_t
	{
		bool	bVisible;
		char	name[MAX_PATH];
		wchar_t	text[MAX_PATH];
		wchar_t	icon[2];			// icon is a single character
	};

	CUtlVector< ButtonLabel_t* > m_ButtonLabels;

	vgui::Label		*m_pSizingLabel;		// used to measure font sizes

	bool			m_bPaintBackground;		// fill the background?
	bool			m_bCenterHorizontal;	// center buttons horizontally?
	int				m_ButtonPinRight;		// if not centered, this is the distance from the right margin that we use to start drawing buttons (right to left)
	int				m_nButtonGap;			// space between buttons when drawing
	int				m_nButtonGapDefault;		// space between buttons (initial value)
	int				m_FooterTall;			// height of the footer
	int				m_ButtonOffsetFromTop;	// how far below the top the buttons should be drawn
	int				m_ButtonSeparator;		// space between the button icon and text
	int				m_TextAdjust;			// extra adjustment for the text (vertically)...text is centered on the button icon and then this value is applied

	char			m_szTextFont[64];		// font for the button text
	char			m_szButtonFont[64];		// font for the button icon
	char			m_szFGColor[64];		// foreground color (text)
	char			m_szBGColor[64];		// background color (fill color)

	vgui::HFont		m_hButtonFont;
	vgui::HFont		m_hTextFont;
	char			*m_pHelpName;
};

//-----------------------------------------------------------------------------
// Purpose: EditablePanel that can replace the GameMenuButtons in CBasePanel
//-----------------------------------------------------------------------------
class CMainMenuGameLogo : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CMainMenuGameLogo, vgui::EditablePanel);
public:
	CMainMenuGameLogo(vgui::Panel *parent, const char *name);

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	int GetOffsetX() { return m_nOffsetX; }
	int GetOffsetY() { return m_nOffsetY; }

private:
	int m_nOffsetX;
	int m_nOffsetY;
};

//-----------------------------------------------------------------------------
// Purpose: Transparent menu item designed to sit on the background ingame
//-----------------------------------------------------------------------------
class CGameMenuItem : public vgui::MenuItem
{
	DECLARE_CLASS_SIMPLE(CGameMenuItem, vgui::MenuItem);
public:
	CGameMenuItem(vgui::Menu *parent, const char *name);

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground(void);
	void SetRightAlignedText(bool state);

private:
	bool		m_bRightAligned;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFMainMenuPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFMainMenuPanel, CTFMenuPanelBase);

public:
	CTFMainMenuPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTFMainMenuPanel();
	bool Init();
	void PerformLayout();
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void OnThink();
	void OnTick();
	void Show();
	void Hide();
	void OnCommand(const char* command);
	void DefaultLayout();
	void GameLayout();
	//void SetVersionLabel();
	//void PlayMusic();
	//void OnNotificationUpdate();
	//void ShowBlogPanel(bool show);
	//void SetServerlistSize(int size);
	//void UpdateServerInfo();

	void CreateGameMenu();
	void CreateGameLogo();
	void CheckBonusBlinkState();
	void UpdateGameMenus();
	CGameMenu *RecursiveLoadGameMenu(KeyValues *datafile);

private:
	//CExLabel			*m_pVersionLabel;
	//CTFAdvButton		*m_pNotificationButton;
	//CAvatarImagePanel	*m_pProfileAvatar; 
	vgui::ImagePanel	*m_pFakeBGImage;

	int					m_iShowFakeIntro;

	CSteamID			m_SteamID;
	//CTFBlogPanel		*m_pBlogPanel;
	//CTFServerlistPanel	*m_pServerlistPanel;

	// menu logo
	CMainMenuGameLogo *m_pGameLogo;

	// menu buttons
	CUtlVector< CBackgroundMenuButton * >m_pGameMenuButtons;
	CGameMenu *m_pGameMenu;
	bool m_bPlatformMenuInitialized;
	int m_iGameMenuInset;

	struct coord {
		int x;
		int y;
	};
	CUtlVector< coord > m_iGameTitlePos;
	coord m_iGameMenuPos;
};


//class CTFBlogPanel : public CTFMenuPanelBase
//{
//	DECLARE_CLASS_SIMPLE(CTFBlogPanel, CTFMenuPanelBase);
//
//public:
//	CTFBlogPanel(vgui::Panel* parent, const char *panelName);
//	virtual ~CTFBlogPanel();
//	void PerformLayout();
//	void ApplySchemeSettings(vgui::IScheme *pScheme);
//	void LoadBlogPost(const char* URL);
//
//private:
//	vgui::HTML			*m_pHTMLPanel;
//};
//
//class CTFServerlistPanel : public CTFMenuPanelBase
//{
//	DECLARE_CLASS_SIMPLE(CTFServerlistPanel, CTFMenuPanelBase);
//
//public:
//	CTFServerlistPanel(vgui::Panel* parent, const char *panelName);
//	virtual ~CTFServerlistPanel();
//	void PerformLayout();
//	void ApplySchemeSettings(vgui::IScheme *pScheme);
//	void SetServerlistSize(int size);
//	void UpdateServerInfo();
//	void OnThink();
//	void OnCommand(const char* command);
//
//private:
//	static bool ServerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);
//	vgui::SectionedListPanel	*m_pServerList;
//	CTFAdvButton				*m_pConnectButton;
//	CTFAdvSlider				*m_pListSlider;
//	CPanelAnimationVarAliasType(int, m_iServerWidth, "server_width", "35", "proportional_int");
//	CPanelAnimationVarAliasType(int, m_iPlayersWidth, "players_width", "35", "proportional_int");
//	CPanelAnimationVarAliasType(int, m_iPingWidth, "ping_width", "23", "proportional_int");
//	CPanelAnimationVarAliasType(int, m_iMapWidth, "map_width", "23", "proportional_int");
//	CPanelAnimationVarAliasType(int, m_iScrollWidth, "scroll_width", "23", "proportional_int");
//	int		m_iSize;
//};

#endif // TFMAINMENUPANEL_H