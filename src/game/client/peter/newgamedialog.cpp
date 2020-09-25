#include "cbase.h"
#include "vgui_controls\Frame.h"
#include "vgui_controls\ListPanel.h"
#include "vgui_controls\Label.h"
#include "vgui_controls\Button.h"
#include "vgui_controls/RadioButton.h"
#include "vgui_controls/ImageList.h"
#include "vsingleplayer.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "gameui/modinfo.h"
#include "fmtstr.h"
#include "ienginevgui.h"

using namespace vgui;

#define CHAPTERIMAGE_BASE_RECT_Y 86
#define CHAPTERIMAGE_BASE_RECT_X 152

#define CHAPTERIMAGE_DESIRED_RECT_Y ((250 - 24) / 3)

static const double CHAPTERIMAGE_SCALER = ((double)CHAPTERIMAGE_DESIRED_RECT_Y / (double)CHAPTERIMAGE_BASE_RECT_Y);
static const int MAX_CHAPTERS = 32;

#define CHAPTERIMAGE_DESIRED_RECT_X (int)(CHAPTERIMAGE_BASE_RECT_X * CHAPTERIMAGE_SCALER)

static bool s_bHasOpenedDialogue = false;

// sort function used in displaying chapter list
struct chapter_t
{
	char filename[32];
	char printname[64];
	int image;
};

static int __cdecl ChapterSortFunc(ListPanel* pPanel, const ListPanelItem& item1, const ListPanelItem& item2)
{
	const char* c1 = item1.kv->GetName();
	const char* c2 = item2.kv->GetName();

	// compare chapter number first
	static int chapterlen = strlen("chapter");
	if (atoi(c1 + chapterlen) > atoi(c2 + chapterlen))
		return 1;
	else if (atoi(c1 + chapterlen) < atoi(c2 + chapterlen))
		return -1;

	// compare length second (longer string show up later in the list, eg. chapter9 before chapter9a)
	if (strlen(c1) > strlen(c2))
		return 1;
	else if (strlen(c1) < strlen(c2))
		return -1;

	// compare strings third
	return strcmp(c1, c2);
}

struct chapterarray_t
{
	chapter_t chapters[MAX_CHAPTERS];
	char gamename[32];
	int imagelist;
	int chaptercount;
};

class ChapterListPanel : public ListPanel
{
	DECLARE_CLASS_SIMPLE(ChapterListPanel, ListPanel);
public:
	ChapterListPanel(Panel* parent, const char* panelName);

	virtual void SetFont(HFont font);
};

DECLARE_BUILD_FACTORY(ChapterListPanel);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ChapterListPanel::SetFont(HFont font)
{
	Assert(font);
	if (!font)
		return;

	m_pTextImage->SetFont(font);
}

ChapterListPanel::ChapterListPanel(Panel* parent, const char* panelName) : ListPanel(parent, panelName)
{
	m_iRowHeight = CHAPTERIMAGE_DESIRED_RECT_Y;
}

class CHLMSNewGame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHLMSNewGame, vgui::Frame);
public:
	CHLMSNewGame(vgui::Panel *parent, const char *name, bool bCommentaryMode = false);
	~CHLMSNewGame();

	virtual void	OnCommand(const char *command);

	MESSAGE_FUNC_PTR(OnSelectionUpdated, "ItemSelected", panel);

protected:
	void	BuildImageList(const char *game);

	int		GetSelectedSkill()
	{
		if (m_pEasy->IsSelected())
			return SKILL_EASY;
		else if (m_pHard->IsSelected())
			return SKILL_HARD;
		else
			return SKILL_MEDIUM;
	}

	bool m_bCommentaryMode;
	vgui::ListPanel		*m_pFolderList;
	vgui::ListPanel*	m_pChapterList;

	vgui::Button 		*m_pOpenButton;
	vgui::Button 		*m_pCancelButton;
	vgui::Button* m_pTrainingButton;

	vgui::RadioButton* m_pEasy;
	vgui::RadioButton* m_pMedium;
	vgui::RadioButton* m_pHard;

	CUtlVectorAutoPurge<ImageList *> m_ImageLists;
	CUtlVector<chapterarray_t> m_ChapterArrays;

	char	m_cTrainingLevel[MAX_MAP_NAME_SAVE];
};

vgui::Frame *g_pActiveNewGamePanel = NULL;

void CreateNewGameDialog(bool bCommentary)
{
	if (g_pActiveNewGamePanel == nullptr)
	{
		g_pActiveNewGamePanel = new CHLMSNewGame(NULL, "ModNewGame", bCommentary);
		//g_pActiveNewGamePanel->MakePopup();
		g_pActiveNewGamePanel->Activate();
		g_pActiveNewGamePanel->MoveToCenterOfScreen();
		g_pActiveNewGamePanel->SetParent(enginevgui->GetPanel(PANEL_GAMEUIDLL));
		g_pActiveNewGamePanel->SetTitle(bCommentary ? "#GameUI_Commentary" : "#GameUI_NewGame", true);
	}
}

CON_COMMAND(hlms_newgame, "Open mod support newgame dialog.")
{
	bool bCommentary = false;

	if (args.ArgC() > 1)
	{
		bCommentary = (Q_strcmp("commentary", args[1]) == 0);
	}

	if (g_pActiveNewGamePanel == nullptr)
	{
		CreateNewGameDialog(bCommentary);
	}
	else
	{
		g_pActiveNewGamePanel->Close();
	}
}

CHLMSNewGame::~CHLMSNewGame()
{
	if (g_pActiveNewGamePanel == this)
		g_pActiveNewGamePanel = NULL;
}

CHLMSNewGame::CHLMSNewGame(vgui::Panel *parent, const char *name, bool bCommentaryMode) : BaseClass(parent, name)
{
	SetDeleteSelfOnClose(true);
	//SetSize(600, 296);
	SetSizeable(false);
	SetProportional(false);
	MoveToCenterOfScreen();

	SetTitle("#GameUI_NewGame", true);

	m_bCommentaryMode = bCommentaryMode;

	// list panel
	m_pFolderList = new ListPanel(this, "FolderList");
	m_pFolderList->SetMultiselectEnabled(false);
	m_pFolderList->SetIgnoreDoubleClick(true);

	m_pFolderList->AddColumnHeader(0, "game", "Game", 175, 20, 10000, ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pFolderList->SetColumnTextAlignment(0, Label::a_west);

	// list panel
	m_pChapterList = new ChapterListPanel(this, "ChapterList");
	m_pChapterList->SetMultiselectEnabled(false);

	m_pChapterList->AddColumnHeader(0, "image", "Chapter Image", CHAPTERIMAGE_DESIRED_RECT_X, CHAPTERIMAGE_DESIRED_RECT_X, CHAPTERIMAGE_DESIRED_RECT_X, ListPanel::COLUMN_FIXEDSIZE|ListPanel::COLUMN_IMAGE);
	m_pChapterList->SetColumnTextAlignment(0, Label::a_northwest);

	m_pChapterList->AddColumnHeader(1, "chapter", "Chapter", 175, 20, 10000, ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pChapterList->SetColumnTextAlignment(1, Label::a_west);
	m_pChapterList->SetSortFunc(1, ChapterSortFunc);

	m_pCancelButton = new Button(this, "CancelButton", "#FileOpenDialog_Cancel", this);
	m_pOpenButton = new Button(this, "OpenButton", "#FileOpenDialog_Open", this);
	m_pTrainingButton = new Button(this, "TrainingButton", "Training Room", this);
	
	m_pEasy = new vgui::RadioButton(this, "Easy", "#GameUI_Easy");
	m_pMedium = new vgui::RadioButton(this, "Medium", "#GameUI_Medium");
	m_pHard = new vgui::RadioButton(this, "Hard", "#GameUI_Hard");

	if (ModInfo().NoDifficulty() /*|| ModInfo().IsSinglePlayerOnly()*/)
	{
		m_pEasy->SetVisible(false);
		m_pMedium->SetVisible(false);
		m_pHard->SetVisible(false);
	}
	else
	{
		ConVarRef refSkill("skill");
		switch (refSkill.GetInt())
		{
		case 1:
			m_pEasy->SetSelected(true);
			break;
		case 2:
		default:
			m_pMedium->SetSelected(true);
			break;
		case 3:
			m_pHard->SetSelected(true);
			break;
		}
	}

	//if (IsPC())
	{
		FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
		const char* fileName = "cfg/*";
		fileName = g_pFullFileSystem->FindFirstEx(fileName, "MOD", &findHandle);
		while (fileName)
		{
			if (g_pFullFileSystem->FindIsDirectory(findHandle) && fileName[0] != '.')
			{
				bool bCreate = false;

				char szFile[MAX_PATH];
				Q_snprintf(szFile, sizeof(szFile), "cfg/%s/chapter*.cfg", fileName);
				FileFindHandle_t findHandle2 = FILESYSTEM_INVALID_FIND_HANDLE;
				if (g_pFullFileSystem->FindFirst(szFile, &findHandle2))
				{
					char cControlFile[MAX_PATH];
					V_snprintf(cControlFile, MAX_PATH, "cfg/%s/maplist.txt", fileName);
					V_FixSlashes(cControlFile);

					CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
					if (g_pFullFileSystem->ReadFile(cControlFile, "MOD", buf))
					{
						char szMapName[MAX_MAP_NAME];
						do {
							buf.GetLine(szMapName, MAX_MAP_NAME);
							// Handle comments and empty lines
							if (V_strncmp(szMapName, "//", 2) == 0 || szMapName[0] == '\n')
								continue;

							// Handle commands
							if (szMapName[0] == '@')
							{
								/*CUtlStringList params;
								V_SplitString(szMapName + 1, ":", params);

								const char* pszCommand = params[0];
								if (V_strnicmp(pszCommand, "chapter", 7) == 0)
								{
									IF_PARAMS_VALID(params, 1)
										mapData.m_iChapterIndex = atoi(params[1]);
								}
								else if (V_strnicmp(pszCommand, "location", 8) == 0)
								{
									IF_PARAMS_VALID(params, 1)
										mapData.m_PopulationPath = params[1];
								}
								else if (V_strncmp(pszCommand, "optset", 6) == 0)
								{
									IF_PARAMS_VALID(params, 2)
										mapData.GetOrCreateOptions()->SetString(params[1], params[2]);
								}
								else if (V_strncmp(pszCommand, "optclr", 6) == 0)
								{
									mapData.NukeOptions();
								}*/

								continue;
							}
							const char* pszLineEnd = V_strnchr(szMapName, '\n', MAX_MAP_NAME);
							if (pszLineEnd)
								const_cast<char*>(pszLineEnd)[0] = 0;

							char mapname[MAX_PATH];
							V_ComposeFileName("maps/", szMapName, mapname, MAX_PATH);
							V_SetExtension(mapname, ".bsp", MAX_PATH);

							if (filesystem->FileExists(mapname, "GAME") && (!m_bCommentaryMode || filesystem->FileExists(CFmtStr("maps" CORRECT_PATH_SEPARATOR_S "%s_commentary.txt", szMapName), "GAME")))
							{
								bCreate = true;
							}
						} while (buf.IsValid() && !bCreate);
					}
					else
					{
						bCreate = true;
					}
				}
				g_pFullFileSystem->FindClose(findHandle2);

				if (bCreate)
				{
					KeyValues* pKV = new KeyValues(fileName);

					char gameName[64];
					Q_snprintf(gameName, sizeof(gameName), "#%s_Game_Title", fileName);
					pKV->SetString("game", gameName);

					KeyValuesAD pkvConfig("Config");
					if (!m_bCommentaryMode && pkvConfig->LoadFromFile(g_pFullFileSystem, CFmtStr("cfg/%s/gameconfig.vdf", fileName), "MOD"))
					{
						KeyValues *pkvLevel = pkvConfig->FindKey("training_level");
						if (pkvLevel)
							pKV->AddSubKey(pkvLevel->MakeCopy());
					}

					m_pFolderList->AddItem(pKV, 0, false, false);

					pKV->deleteThis();

					BuildImageList(fileName);
				}
			}

			fileName = g_pFullFileSystem->FindNext(findHandle);
		}

		g_pFullFileSystem->FindClose(findHandle);
	}

	LoadControlSettings("resource/modsupportnewgame.res");

	m_pTrainingButton->SetEnabled(false);

	s_bHasOpenedDialogue = true;
	g_pActiveNewGamePanel = this;
}

void CHLMSNewGame::BuildImageList(const char* pchGame)
{
	chapter_t *chapters = m_ChapterArrays.Element(m_ChapterArrays.AddToHead()).chapters;
	Q_strncpy(m_ChapterArrays.Head().gamename, pchGame, sizeof(m_ChapterArrays.Head().gamename));
	m_ChapterArrays.Head().imagelist = m_ImageLists.AddToTail(new ImageList(false));
	char szFullFileName[MAX_PATH];
	int chapterIndex = 0;

	//if (IsPC())
	{
		FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
		char szFile[MAX_PATH];

		Q_snprintf(szFile, sizeof(szFile), "cfg/%s/chapter*.cfg", pchGame);

		const char* fileName = szFile;
		fileName = g_pFullFileSystem->FindFirst(fileName, &findHandle);
		while (fileName && chapterIndex < MAX_CHAPTERS)
		{
			// Only load chapter configs from the current mod's cfg dir
			// or else chapters appear that we don't want!
			Q_snprintf(szFullFileName, sizeof(szFullFileName), "cfg/%s/%s", pchGame, fileName);
			FileHandle_t f = g_pFullFileSystem->Open(szFullFileName, "rb", "MOD");
			if (f)
			{
				// don't load chapter files that are empty, used in the demo
				if (g_pFullFileSystem->Size(f) > 0)
				{
					Q_strncpy(chapters[chapterIndex].filename, fileName, sizeof(chapters[chapterIndex].filename));
					++chapterIndex;
				}
				g_pFullFileSystem->Close(f);
			}
			fileName = g_pFullFileSystem->FindNext(findHandle);
		}
	}

	m_ChapterArrays.Head().chaptercount = chapterIndex;

	// add chapters to combobox
	for (int i = 0; i < chapterIndex; i++)
	{
		const char* fileName = chapters[i].filename;
		char chapterID[32] = { 0 };
		sscanf(fileName, "chapter%s", chapterID);
		// strip the extension
		char* ext = V_stristr(chapterID, ".cfg");
		if (ext)
		{
			*ext = 0;
		}

		//char chapterName[64];
		Q_snprintf(chapters[i].printname, sizeof(chapters[i].printname), "#%s_Chapter%s_Title", pchGame, chapterID);

		Q_snprintf(szFullFileName, sizeof(szFullFileName), "chapters/%s/%s", pchGame, fileName);
		IImage* pImage = scheme()->GetImage(szFullFileName, true);
		if (!s_bHasOpenedDialogue)
		{
			int iWide, iTall;
			pImage->GetContentSize(iWide, iTall);
			pImage->SetSize(iWide * CHAPTERIMAGE_SCALER, iTall * CHAPTERIMAGE_SCALER);
		}
		chapters[i].image = m_ImageLists.Element(m_ChapterArrays.Head().imagelist)->AddImage(pImage);
	}
}

void CHLMSNewGame::OnSelectionUpdated(vgui::Panel* pSource)
{
	if (pSource == m_pFolderList)
	{
		m_pChapterList->RemoveAll();

		KeyValues* pkv = m_pFolderList->GetItem(m_pFolderList->GetSelectedItem(0));
		const char* pchGame = pkv->GetName();

		int i = 0;
		for (i = 0; i < m_ChapterArrays.Count(); i++)
		{
			if (FStrEq(pchGame, m_ChapterArrays[i].gamename))
				break;
		}

		if (i >= m_ChapterArrays.Count())
			return;

		KeyValues* pkvTrainingRoom = pkv->FindKey("training_level");
		if (pkvTrainingRoom)
		{
			const char* pchTrainingLevel = pkvTrainingRoom->GetString();

			char mapname[MAX_PATH];
			V_ComposeFileName("maps/", pchTrainingLevel, mapname, MAX_PATH);
			V_SetExtension(mapname, ".bsp", MAX_PATH);

			if (filesystem->FileExists(mapname, "GAME"))
			{
				m_pTrainingButton->SetEnabled(true);
				m_pTrainingButton->SetAsCurrentDefaultButton(1);
				V_strcpy_safe(m_cTrainingLevel, pchTrainingLevel);
			}
			else
			{
				m_pTrainingButton->SetEnabled(false);
				m_cTrainingLevel[0] = '\0';
			}
		}
		else
		{
			m_pTrainingButton->SetEnabled(false);
			m_cTrainingLevel[0] = '\0';
		}

		ImageList* pImgList = m_ImageLists[m_ChapterArrays[i].imagelist];
		chapter_t* chapters = m_ChapterArrays[i].chapters;

		m_pChapterList->SetImageList(pImgList, false);

		for (int j = 0; j < m_ChapterArrays[i].chaptercount; j++)
		{
			KeyValues* pKV = new KeyValues(chapters[j].filename);

			pKV->SetInt("image", chapters[j].image);
			pKV->SetString("chapter", chapters[j].printname);

			m_pChapterList->AddItem(pKV, 0, false, false);

			pKV->deleteThis();
		}

		m_pChapterList->SetSortColumn(1);
		m_pChapterList->SortList();
	}
	else if (pSource == m_pChapterList)
	{
		m_pOpenButton->SetAsCurrentDefaultButton(1);
		m_pTrainingButton->SetAsCurrentDefaultButton(0);
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void CHLMSNewGame::OnCommand(const char *command)
{
	if (!stricmp(command, "Play"))
	{
		if (m_pFolderList->GetSelectedItemsCount() == 1 && m_pChapterList->GetSelectedItemsCount() == 1)
		{
			KeyValues *pkvFolder = m_pFolderList->GetItem(m_pFolderList->GetSelectedItem(0));
			KeyValues* pkvChapter = m_pChapterList->GetItem(m_pChapterList->GetSelectedItem(0));

			char mapcommand[512];
			mapcommand[0] = 0;
			Q_snprintf(mapcommand, sizeof(mapcommand), "disconnect\ndeathmatch 0\ncoop 0\nmaxplayers 1\nprogress_enable\nexec \"%s/%s\"\n", pkvFolder->GetName(), pkvChapter->GetName());

			// Set commentary
			ConVarRef commentary("commentary");
			commentary.SetValue(m_bCommentaryMode);

			if (m_bCommentaryMode)
			{
				ConVarRef sv_cheats("sv_cheats");
				sv_cheats.SetValue(m_bCommentaryMode);
			}

			if (m_pEasy->IsVisible())
			{
				ConVarRef skill("skill");
				skill.SetValue(GetSelectedSkill());
			}

			//if (IsPC())
			{
				{
					// start map
					engine->ClientCmd(mapcommand); //replace with proper fading and stuff? [str]
	//				BasePanel()->FadeToBlackAndRunEngineCommand( mapcommand );
				}
			}

			Close();
		}
	}
	else if (!stricmp(command, "trainingroom"))
	{
		if (m_pTrainingButton->IsEnabled())
		{
			char mapcommand[512];
			mapcommand[0] = 0;
			Q_snprintf(mapcommand, sizeof(mapcommand), "disconnect\ndeathmatch 0\ncoop 0\nmaxplayers 1\nprogress_enable\nmap \"%s\"\n", m_cTrainingLevel);

			// Set commentary
			ConVarRef commentary("commentary");
			commentary.SetValue(m_bCommentaryMode);

			if (m_bCommentaryMode)
			{
				ConVarRef sv_cheats("sv_cheats");
				sv_cheats.SetValue(m_bCommentaryMode);
			}

			if (m_pEasy->IsVisible())
			{
				ConVarRef skill("skill");
				skill.SetValue(GetSelectedSkill());
			}

			//if (IsPC())
			{
				{
					// start map
					engine->ClientCmd(mapcommand); //replace with proper fading and stuff? [str]
	//				BasePanel()->FadeToBlackAndRunEngineCommand( mapcommand );
				}
			}

			Close();
		}
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}