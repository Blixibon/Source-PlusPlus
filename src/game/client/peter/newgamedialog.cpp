#include "cbase.h"
#include "vgui_controls\Frame.h"
#include "vgui_controls\ListPanel.h"
#include "vgui_controls\Label.h"
#include "vgui_controls\Button.h"
#include "vsingleplayer.h"
#include "filesystem.h"
#include "KeyValues.h"

using namespace vgui;

class CHLMSNewGame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHLMSNewGame, vgui::Frame);
public:
	CHLMSNewGame(vgui::Panel *parent, const char *name, bool bCommentaryMode = false);
	~CHLMSNewGame();

	virtual void	OnCommand(const char *command);

protected:

	bool m_bCommentaryMode;
	vgui::ListPanel		*m_pFolderList;

	vgui::Button 		*m_pOpenButton;
	vgui::Button 		*m_pCancelButton;
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
	SetSize(600, 296);
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

	m_pCancelButton = new Button(this, "CancelButton", "#FileOpenDialog_Cancel", this);
	m_pOpenButton = new Button(this, "OpenButton", "#FileOpenDialog_Open", this);

	if (IsPC())
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
			}

			fileName = g_pFullFileSystem->FindNext(findHandle);
		}
	}

	LoadControlSettings("resource/modsupportnewgame.res");

	g_pActiveNewGamePanel = this;
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