#include "cbase.h"
#include "base_mainmenu.h"
#include "base_mainmenu_interface.h"

#include "base_mainmenupanel.h"
#include "base_pausemenupanel.h"
#include "base_backgroundpanel.h"
//#include "base_loadoutpanel.h"
//#include "base_notificationpanel.h"
#include "base_shadebackgroundpanel.h"
//#include "base_optionsdialog.h"
//#include "base_quitdialogpanel.h"
//#include "base_statsummarydialog.h"
//#include "base_tooltippanel.h"
//#include "base_itemtooltippanel.h"
#include "engine/IEngineSound.h"
//#include "tf_hud_statpanel.h"
//#include "tf_notificationmanager.h"
#include "tier0/icommandline.h"
#include "fmtstr.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUIDLL("GameUI");

CTFMainMenu *guiroot = NULL;

void OverrideMainMenu()
{
	if (!MainMenu->GetPanel())
	{
		MainMenu->Create(NULL);
	}
	if (guiroot->GetGameUI())
	{
		guiroot->GetGameUI()->SetMainMenuOverride(guiroot->GetVPanel());
		return;
	}
}

CON_COMMAND(tf2c_mainmenu_reload, "Reload Main Menu")
{
	MAINMENU_ROOT->InvalidatePanelsLayout(true, true);
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFMainMenu::CTFMainMenu(VPANEL parent) : vgui::EditablePanel(NULL, "MainMenu")
{
	SetParent(parent);

	guiroot = this;
	gameui = NULL;
	LoadGameUI();
	SetScheme("ClientScheme");

	SetDragEnabled(false);
	SetShowDragHelper(false);
	SetProportional(true);
	SetVisible(true);

	int width, height;
	surface()->GetScreenSize(width, height);
	SetSize(width, height);
	SetPos(0, 0);

	m_pPanels.SetSize(COUNT_MENU);
	AddMenuPanel(new CTFMainMenuPanel(this, "CTFMainMenuPanel"), MAIN_MENU);
	//AddMenuPanel(new CTFPauseMenuPanel(this, "CTFPauseMenuPanel"), PAUSE_MENU);
	AddMenuPanel(new CTFBackgroundPanel(this, "CTFBackgroundPanel"), BACKGROUND_MENU);
	//AddMenuPanel(new CTFLoadoutPanel(this, "CTFLoadoutPanel"), LOADOUT_MENU);
	//AddMenuPanel(new CTFNotificationPanel(this, "CTFNotificationPanel"), NOTIFICATION_MENU);
	AddMenuPanel(new CTFShadeBackgroundPanel(this, "CTFShadeBackgroundPanel"), SHADEBACKGROUND_MENU);
	//AddMenuPanel(new CTFQuitDialogPanel(this, "CTFQuitDialogPanel"), QUIT_MENU);
	//AddMenuPanel(new CTFOptionsDialog(this, "CTFOptionsDialog"), OPTIONSDIALOG_MENU);
	//AddMenuPanel(new CTFStatsSummaryDialog(this, "CTFStatsSummaryDialog"), STATSUMMARY_MENU);
	//AddMenuPanel(new CTFToolTipPanel(this, "CTFToolTipPanel"), TOOLTIP_MENU);
	//AddMenuPanel(new CTFItemToolTipPanel(this, "CTFItemToolTipPanel"), ITEMTOOLTIP_MENU);

	ShowPanel(MAIN_MENU);
	//ShowPanel(PAUSE_MENU);
	ShowPanel(BACKGROUND_MENU);
	HidePanel(SHADEBACKGROUND_MENU);
	//HidePanel(LOADOUT_MENU);
	//HidePanel(NOTIFICATION_MENU);
	//HidePanel(QUIT_MENU);
	//HidePanel(OPTIONSDIALOG_MENU);
	//HidePanel(STATSUMMARY_MENU);
	//HidePanel(TOOLTIP_MENU);
	//HidePanel(ITEMTOOLTIP_MENU);
	
	bInGameLayout = false;
	m_iStopGameStartupSound = 2;
	m_iUpdateLayout = 1;

	m_psMusicStatus = MUSIC_FIND;
	m_pzMusicLink[0] = '\0';
	m_nSongGuid = 0;

	vgui::ivgui()->AddTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFMainMenu::~CTFMainMenu()
{
	m_pPanels.RemoveAll();
	gameui = NULL;
	g_GameUIDLL.Unload();
}

void CTFMainMenu::AddMenuPanel(CTFMenuPanelBase *m_pPanel, int iPanel)
{
	m_pPanels[iPanel] = m_pPanel;
	m_pPanel->SetZPos(iPanel);
}

CTFMenuPanelBase* CTFMainMenu::GetMenuPanel(int iPanel)
{
	return m_pPanels[iPanel];
}

CTFMenuPanelBase* CTFMainMenu::GetMenuPanel(const char *name)
{
	for (int i = FIRST_MENU; i < COUNT_MENU; i++)
	{
		CTFMenuPanelBase* pMenu = GetMenuPanel(i);
		if (pMenu && (Q_strcmp(pMenu->GetName(), name) == 0))
		{
			return pMenu;
		}
	}
	return NULL;
}

void CTFMainMenu::ShowPanel(MenuPanel iPanel, bool bShowSingle /*= false*/)
{
	GetMenuPanel(iPanel)->SetShowSingle(bShowSingle);
	GetMenuPanel(iPanel)->Show();
	if (bShowSingle)
	{
		GetMenuPanel(CURRENT_MENU)->Hide();
	}
}

void CTFMainMenu::HidePanel(MenuPanel iPanel)
{
	GetMenuPanel(iPanel)->Hide();
}

IGameUI *CTFMainMenu::GetGameUI()
{
	if (!gameui)
	{
		if (!LoadGameUI())
			return NULL;
	}

	return gameui;
}

bool CTFMainMenu::LoadGameUI()
{
	if (!gameui)
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if (gameUIFactory)
		{
			gameui = (IGameUI *)gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL);
			if (!gameui)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

char* CTFMainMenu::GetRandomMusic()
{
	int iCount = m_vecMusic.Count();

	if (iCount <= 0)
	{
		bool bLooping = !m_bMusicStartup;
		if (m_bMusicStartup)
		{
			const char* fileName = "sound/music/menu_startup.res";
			FileHandle_t f = g_pFullFileSystem->Open(fileName, "r", "GAME");
			if (f)
			{
				// don't load chapter files that are empty, used in the demo
				if (g_pFullFileSystem->Size(f) > 0)
				{
					CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
					KeyValuesAD KVFile("MenuMusic");
					if (g_pFullFileSystem->ReadToBuffer(f, buf) && KVFile->LoadFromBuffer(fileName, buf))
					{
						for (KeyValues* kvValue = KVFile->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
						{
							if (g_pFullFileSystem->FileExists(kvValue->GetString(), "GAME"))
							{
								FileNameHandle_t fName = g_pFullFileSystem->FindOrAddFileName(kvValue->GetString());
								m_vecMusic.AddToTail(fName);
								++iCount;
							}
						}
					}
				}
				g_pFullFileSystem->Close(f);
			}
			else
			{
				bLooping = true;
			}

			m_bMusicStartup = false;
		}
		
		if (bLooping)
		{
			char szFullFileName[MAX_PATH];
			FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;

			const char* fileName = "sound/music/menu_loop_*.res";
			fileName = g_pFullFileSystem->FindFirst(fileName, &findHandle);
			while (fileName)
			{
				// Only load chapter configs from the current mod's cfg dir
				// or else chapters appear that we don't want!
				Q_snprintf(szFullFileName, sizeof(szFullFileName), "sound/music/%s", fileName);
				FileHandle_t f = g_pFullFileSystem->Open(szFullFileName, "r", "GAME");
				if (f)
				{
					// don't load chapter files that are empty, used in the demo
					if (g_pFullFileSystem->Size(f) > 0)
					{
						CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
						KeyValuesAD KVFile("MenuMusic");
						if (g_pFullFileSystem->ReadToBuffer(f, buf) && KVFile->LoadFromBuffer(szFullFileName, buf))
						{
							for (KeyValues* kvValue = KVFile->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
							{
								if (g_pFullFileSystem->FileExists(kvValue->GetString(), "GAME"))
								{
									FileNameHandle_t fName = g_pFullFileSystem->FindOrAddFileName(kvValue->GetString());
									m_vecMusic.AddToTail(fName);
									++iCount;
								}
							}
						}
					}
					g_pFullFileSystem->Close(f);
				}
				fileName = g_pFullFileSystem->FindNext(findHandle);
			}
		}
	}

	static char szResult[MAX_PATH];
	
	if (iCount > 0)
	{
		FileNameHandle_t fChosen = m_vecMusic.Random();
		m_vecMusic.FindAndRemove(fChosen);
		g_pFullFileSystem->String(fChosen, szResult, MAX_PATH);
	}

	return V_stristr(szResult, "music");
}


void CTFMainMenu::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

void CTFMainMenu::PerformLayout()
{
	BaseClass::PerformLayout();
};

void CTFMainMenu::OnCommand(const char* command)
{
	gameui->SendMainMenuCommand(command);
}

void CTFMainMenu::InvalidatePanelsLayout(bool layoutNow, bool reloadScheme)
{	
	for (int i = FIRST_MENU; i < COUNT_MENU; i++)
	{
		if (GetMenuPanel(i))
		{
			bool bVisible = GetMenuPanel(i)->IsVisible();
			GetMenuPanel(i)->InvalidateLayout(layoutNow, reloadScheme);
			GetMenuPanel(i)->SetVisible(bVisible);
		}
	}	
	AutoLayout();
}

void CTFMainMenu::LaunchInvalidatePanelsLayout()
{
	m_iUpdateLayout = 4;
}

void CTFMainMenu::OnTick()
{
	BaseClass::OnTick();
	if (!engine->IsDrawingLoadingImage() && !IsVisible())
	{
		SetVisible(true);
	} 
	else if (engine->IsDrawingLoadingImage() && IsVisible())
	{
		SetVisible(false);
	}
	if (!InGame() && bInGameLayout)
	{
		DefaultLayout();
		bInGameLayout = false;
	}
	else if (InGame() && !bInGameLayout)
	{
		GameLayout();
		bInGameLayout = true;
	}
	if (m_iStopGameStartupSound > 0)
	{
		m_iStopGameStartupSound--;
		if (!m_iStopGameStartupSound)
		{
			enginesound->NotifyBeginMoviePlayback();
			m_bMusicStartup = true;
		}
	}
	else
	{
		if (!bInGameLayout)
		{
			if ((m_psMusicStatus == MUSIC_FIND || m_psMusicStatus == MUSIC_STOP_FIND) && !enginesound->IsSoundStillPlaying(m_nSongGuid))
			{
				Q_strncpy(m_pzMusicLink, GetRandomMusic(), sizeof(m_pzMusicLink));
				m_psMusicStatus = MUSIC_PLAY;
			}
			else if ((m_psMusicStatus == MUSIC_PLAY || m_psMusicStatus == MUSIC_STOP_PLAY)&& m_pzMusicLink[0] != '\0')
			{
				enginesound->StopSoundByGuid(m_nSongGuid);
				enginesound->EmitAmbientSound(CFmtStr("*#%s", m_pzMusicLink), 1.0f, PITCH_NORM, 0);			
				m_nSongGuid = enginesound->GetGuidForLastSoundEmitted();
				m_psMusicStatus = MUSIC_FIND;
			}
		}
		else if (m_psMusicStatus == MUSIC_FIND)
		{
			enginesound->StopSoundByGuid(m_nSongGuid);
			m_psMusicStatus = (m_nSongGuid == 0 ? MUSIC_STOP_FIND : MUSIC_STOP_PLAY);
		}
	}
	if (m_iUpdateLayout > 0)
	{
		m_iUpdateLayout--;
		if (!m_iUpdateLayout)
		{
			InvalidatePanelsLayout(true, true);
		}
	}
};

void CTFMainMenu::OnThink()
{
	BaseClass::OnThink();
};


void CTFMainMenu::DefaultLayout()
{
	//set all panels to default layout
	for (int i = FIRST_MENU; i < COUNT_MENU; i++)
	{
		if (GetMenuPanel(i))
			GetMenuPanel(i)->DefaultLayout();
	}		
};

void CTFMainMenu::GameLayout()
{
	//set all panels to game layout
	for (int i = FIRST_MENU; i < COUNT_MENU; i++)
	{
		if (GetMenuPanel(i))
			GetMenuPanel(i)->GameLayout();
	}
};

void CTFMainMenu::PaintBackground()
{
	SetPaintBackgroundType(0);
	BaseClass::PaintBackground();
}

bool CTFMainMenu::InGame()
{
	/*C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer && IsVisible())
	{
		return true;
	}
	else 
	{
		return false;
	}*/

	return engine->IsInGame() && !engine->IsLevelMainMenuBackground();
}


