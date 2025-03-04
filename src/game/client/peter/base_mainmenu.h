#ifndef TFMAINMENU_H
#define TFMAINMENU_H

#include "GameUI/IGameUI.h"
#include "vgui_controls/EditablePanel.h"
#include "steam/steam_api.h"
#include "vgui\IVGui.h"
//#include "tf_hud_statpanel.h"

enum MenuPanel //position in this enum = zpos on the screen
{
	NONE_MENU,
	BACKGROUND_MENU,
	MAIN_MENU,
	SHADEBACKGROUND_MENU, //add popup/additional menus below:		


	COUNT_MENU,
	FIRST_MENU = NONE_MENU + 1
};

//#define CURRENT_MENU (!InGame() ? MAIN_MENU : PAUSE_MENU)
#define CURRENT_MENU MAIN_MENU
#define MAINMENU_ROOT guiroot
#define AutoLayout() (!InGame() ? DefaultLayout() : GameLayout())
#define GET_MAINMENUPANEL( className )												\
	dynamic_cast<className*>(MAINMENU_ROOT->GetMenuPanel(#className))

class CTFMenuPanelBase;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFMainMenu : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE(CTFMainMenu, vgui::EditablePanel);

public:
	CTFMainMenu(vgui::VPANEL parent);
	virtual ~CTFMainMenu();
	IGameUI*	 GetGameUI();
	CTFMenuPanelBase* GetMenuPanel(int iPanel);
	CTFMenuPanelBase* GetMenuPanel(const char *name);	
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PaintBackground();
	virtual void OnCommand(const char* command);
	virtual void OnThink();
	virtual void OnTick();
	virtual void ShowPanel(MenuPanel iPanel, bool bShowSingle = false);
	virtual void HidePanel(MenuPanel iPanel);
	virtual void InvalidatePanelsLayout(bool layoutNow = false, bool reloadScheme = false);
	virtual void LaunchInvalidatePanelsLayout();
	virtual void DefaultLayout();
	virtual void GameLayout();
	virtual bool InGame();
	//virtual void SetStats(CUtlVector<ClassStats_t> &vecClassStats);
	//virtual void ShowToolTip(char* sText);
	//virtual void HideToolTip();
	//virtual void ShowItemToolTip(CEconItemDefinition *pItemData);
	//virtual void HideItemToolTip();
	//virtual void OnNotificationUpdate();
	//virtual void SetServerlistSize(int size);
	//virtual void OnServerInfoUpdate();

private:
	CUtlVector<CTFMenuPanelBase*>		m_pPanels;
	void								AddMenuPanel(CTFMenuPanelBase *m_pPanel, int iPanel);

	bool								LoadGameUI();
	bool								bInGameLayout;
	IGameUI*							gameui;
	int									m_iStopGameStartupSound;
	int									m_iStartMenuMusic;
	int									m_iUpdateLayout;

	enum MusicStatus
	{
		MUSIC_STOP,
		MUSIC_FIND,
		MUSIC_PLAY,
		MUSIC_STOP_FIND,
		MUSIC_STOP_PLAY,
	};

	char				m_pzMusicLink[64];
	int					m_nSongGuid;
	MusicStatus			m_psMusicStatus;
	CUtlVector<FileNameHandle_t> m_vecMusic;
	bool				m_bMusicStartup;
	char* GetRandomMusic();
};

extern CTFMainMenu *guiroot;

#endif // TFMAINMENU_H