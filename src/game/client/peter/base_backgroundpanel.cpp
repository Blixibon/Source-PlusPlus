#include "cbase.h"
#include "base_backgroundpanel.h"
#include "base_mainmenupanel.h"
#include "base_mainmenu.h"

using namespace vgui;
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEFAULT_RATIO_WIDE 1920.0 / 1080.0
#define DEFAULT_RATIO 1024.0 / 768.0

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFBackgroundPanel::CTFBackgroundPanel(vgui::Panel* parent, const char *panelName) : CTFMenuPanelBase(parent, panelName)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFBackgroundPanel::~CTFBackgroundPanel()
{

}

bool CTFBackgroundPanel::Init()
{
	BaseClass::Init();

	m_pVideo = NULL;
	bInMenu = true;
	bInGame = false;
	return true;
}

void CTFBackgroundPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("resource/UI/main_menu/BackgroundPanel.res");
	m_pVideo = dynamic_cast<CTFVideoPanel *>(FindChildByName("BackgroundVideo"));

	if (m_pVideo)
	{
		int width, height;
		surface()->GetScreenSize(width, height);

		float fRatio = (float)width / (float)height;
		bool bWidescreen = (fRatio < 1.5 ? false : true);

		Q_strncpy(m_pzVideoLink, GetRandomVideo(bWidescreen), sizeof(m_pzVideoLink));
		float iRatio = (bWidescreen ? DEFAULT_RATIO_WIDE : DEFAULT_RATIO);
		int iWide = (float)height * iRatio + 4;
		m_pVideo->SetBounds(-1, -1, iWide, iWide);
	}
}

void CTFBackgroundPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	AutoLayout();
};


void CTFBackgroundPanel::OnCommand(const char* command)
{
	BaseClass::OnCommand(command);
}

void CTFBackgroundPanel::VideoReplay()
{
	if (!m_pVideo)
		return;

	int width, height;
	surface()->GetScreenSize(width, height);

	float fRatio = (float)width / (float)height;
	bool bWidescreen = (fRatio < 1.5 ? false : true);

	Q_strncpy(m_pzVideoLink, GetRandomVideo(bWidescreen), sizeof(m_pzVideoLink));

	VideoUpdate();
}

void CTFBackgroundPanel::VideoUpdate()
{
	if (!m_pVideo)
		return;
	
	if (IsVisible() && m_pzVideoLink[0] != '\0' && !bInGameLayout)
	{
		m_pVideo->Activate();
		m_pVideo->BeginPlaybackNoAudio(m_pzVideoLink);
	}
	else
	{
		m_pVideo->Shutdown();
	}
}

void CTFBackgroundPanel::OnTick()
{
	BaseClass::OnTick();
};

void CTFBackgroundPanel::OnThink()
{
	BaseClass::OnThink();
};

void CTFBackgroundPanel::DefaultLayout()
{
	BaseClass::DefaultLayout();
	VideoUpdate();
};

void CTFBackgroundPanel::GameLayout()
{
	BaseClass::GameLayout();
	VideoUpdate();
};

char* CTFBackgroundPanel::GetRandomVideo(bool bWidescreen)
{
	char szFullFileName[MAX_PATH];
	int iCount = 0;

	CUtlVector<FileNameHandle_t> vecMovies;

	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	

	const char *fileName = "media/mainmenu_*.res";
	fileName = g_pFullFileSystem->FindFirst(fileName, &findHandle);
	while (fileName)
	{
		// Only load chapter configs from the current mod's cfg dir
		// or else chapters appear that we don't want!
		Q_snprintf(szFullFileName, sizeof(szFullFileName), "media/%s", fileName);
		FileHandle_t f = g_pFullFileSystem->Open(szFullFileName, "r", "GAME");
		if (f)
		{
			// don't load chapter files that are empty, used in the demo
			if (g_pFullFileSystem->Size(f) > 0)
			{
				CUtlBuffer buf(0,0, CUtlBuffer::TEXT_BUFFER);
				KeyValuesAD KVFile("MenuMovies");
				if (g_pFullFileSystem->ReadToBuffer(f, buf) && KVFile->LoadFromBuffer(szFullFileName, buf))
				{
					for (KeyValues * kvValue = KVFile->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
					{
						if (g_pFullFileSystem->FileExists(kvValue->GetString(),"GAME"))
						{
							FileNameHandle_t fName = g_pFullFileSystem->FindOrAddFileName(kvValue->GetString());
							vecMovies.AddToTail(fName);
							++iCount;
						}
					}
				}
			}
			g_pFullFileSystem->Close(f);
		}
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	static char szResult[MAX_PATH];
	
	if (iCount > 0)
	{
		FileNameHandle_t fChosen = vecMovies.Random();
		g_pFullFileSystem->String(fChosen, szResult, MAX_PATH);
	}

	return szResult;
}