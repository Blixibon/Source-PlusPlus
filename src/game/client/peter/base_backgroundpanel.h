#ifndef TFMAINMENUBACKGROUNDPANEL_H
#define TFMAINMENUBACKGROUNDPANEL_H

#include "vgui_controls/Panel.h"
#include "base_menupanelbase.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFBackgroundPanel : public CTFMenuPanelBase
{
	DECLARE_CLASS_SIMPLE(CTFBackgroundPanel, CTFMenuPanelBase);

public:
	CTFBackgroundPanel(vgui::Panel* parent, const char *panelName);
	virtual ~CTFBackgroundPanel();
	bool Init();
	void PerformLayout();
	void ApplySchemeSettings(vgui::IScheme *pScheme);
	void OnThink();
	void OnTick();
	void OnCommand(const char* command);
	void DefaultLayout();
	void GameLayout();
	MESSAGE_FUNC(VideoReplay, "IntroFinished");
	void VideoUpdate();

private:
	CTFVideoPanel		*m_pVideo;
	char				m_pzVideoLink[64];
	char*				GetRandomVideo(bool bWidescreen);
	CUtlVector<FileNameHandle_t> m_vecMovies;
};

#endif // TFMAINMENUBACKGROUNDPANEL_H