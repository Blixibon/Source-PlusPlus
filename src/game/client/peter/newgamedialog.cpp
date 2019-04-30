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

using namespace vgui;

static const int MAX_CHAPTERS = 32;

// sort function used in displaying chapter list
struct chapter_t
{
	char filename[32];
	char printname[64];
	int image;
};

static int __cdecl ChapterSortFunc(const void* elem1, const void* elem2)
{
	chapter_t* c1 = (chapter_t*)elem1;
	chapter_t* c2 = (chapter_t*)elem2;

	// compare chapter number first
	static int chapterlen = strlen("chapter");
	if (atoi(c1->filename + chapterlen) > atoi(c2->filename + chapterlen))
		return 1;
	else if (atoi(c1->filename + chapterlen) < atoi(c2->filename + chapterlen))
		return -1;

	// compare length second (longer string show up later in the list, eg. chapter9 before chapter9a)
	if (strlen(c1->filename) > strlen(c2->filename))
		return 1;
	else if (strlen(c1->filename) < strlen(c2->filename))
		return -1;

	// compare strings third
	return strcmp(c1->filename, c2->filename);
}

struct chapterarray_t
{
	chapter_t chapters[MAX_CHAPTERS];
	char gamename[32];
	int imagelist;
	int chaptercount;
};

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

	bool m_bCommentaryMode;
	vgui::ListPanel		*m_pFolderList;
	vgui::ListPanel*	m_pChapterList;

	vgui::Button 		*m_pOpenButton;
	vgui::Button 		*m_pCancelButton;

	vgui::RadioButton* m_pEasy;
	vgui::RadioButton* m_pMedium;
	vgui::RadioButton* m_pHard;

	CUtlVectorAutoPurge<ImageList *> m_ImageLists;
	CUtlVector<chapterarray_t> m_ChapterArrays;
};

vgui::Frame *g_pActiveNewGamePanel = NULL;

void CreateNewGameDialog(bool bCommentary)
{
	if (g_pActiveNewGamePanel == nullptr)
	{
		g_pActiveNewGamePanel = new CHLMSNewGame(NULL, "ModNewGame", bCommentary);
		//g_pActiveNewGamePanel->MakePopup();
		//g_pActiveNewGamePanel->Activate();
		g_pActiveNewGamePanel->DoModal();
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

	m_pFolderList->AddColumnHeader(0, "game", "Game", 175, 20, 10000, ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pFolderList->SetColumnTextAlignment(0, Label::a_west);

	// list panel
	m_pChapterList = new ListPanel(this, "ChapterList");
	m_pChapterList->SetMultiselectEnabled(false);

	m_pChapterList->AddColumnHeader(0, "image", "Chapter Image", 175, 20, 10000, ListPanel::COLUMN_RESIZEWITHWINDOW|ListPanel::COLUMN_IMAGE);
	m_pChapterList->SetColumnTextAlignment(0, Label::a_center);

	m_pChapterList->AddColumnHeader(1, "chapter", "Chapter", 175, 20, 10000, ListPanel::COLUMN_RESIZEWITHWINDOW);
	m_pChapterList->SetColumnTextAlignment(1, Label::a_west);

	m_pCancelButton = new Button(this, "CancelButton", "#FileOpenDialog_Cancel", this);
	m_pOpenButton = new Button(this, "OpenButton", "#FileOpenDialog_Open", this);
	
	m_pEasy = new vgui::RadioButton(this, "Easy", "#GameUI_Easy");
	m_pMedium = new vgui::RadioButton(this, "Medium", "#GameUI_Medium");
	m_pHard = new vgui::RadioButton(this, "Hard", "#GameUI_Hard");

	if (ModInfo().NoDifficulty() || ModInfo().IsSinglePlayerOnly())
	{
		m_pEasy->SetVisible(false);
		m_pMedium->SetVisible(false);
		m_pHard->SetVisible(false);
	}
	else
	{
		m_pMedium->SetSelected(true);
	}

	//if (IsPC())
	{
		FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
		const char *fileName = "cfg/*";
		fileName = g_pFullFileSystem->FindFirstEx(fileName, "MOD", &findHandle);
		while (fileName)
		{
			if (g_pFullFileSystem->FindIsDirectory(findHandle) && fileName[0] != '.')
			{
				KeyValues *pKV = new KeyValues(fileName);

				char gameName[64];
				Q_snprintf(gameName, sizeof(gameName), "#%s_Game_Title", fileName);
				pKV->SetString("game", gameName);

				m_pFolderList->AddItem(pKV, 0, false, false);

				pKV->deleteThis();

				BuildImageList(fileName);
			}

			fileName = g_pFullFileSystem->FindNext(findHandle);
		}
	}

	LoadControlSettings("resource/modsupportnewgame.res");

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
		IImage* pImage = scheme()->GetImage(szFullFileName, false);
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
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void CHLMSNewGame::OnCommand(const char *command)
{
	if (!stricmp(command, "Play"))
	{
		if (m_pFolderList->GetSelectedItemsCount() == 1)
		{
			KeyValues *pkv = m_pFolderList->GetItem(m_pFolderList->GetSelectedItem(0));

			CNewGameDialog *pDialog = new CNewGameDialog(NULL, "NewGameDialog", pkv->GetName(), m_bCommentaryMode);
			Assert(pDialog);

			Close();

			//pDialog->Activate();
			//g_pActiveNewGamePanel = pDialog;
			pDialog->DoModal();

			pDialog->SetTitle(pkv->GetString("game"), true);
			
		}
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}