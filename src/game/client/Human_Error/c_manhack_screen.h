//=============================================================================//
//
// Purpose: Ask: This is a screen we'll use for the Manhack
//
//=============================================================================//
#include "cbase.h"

#include <vgui_controls/EditablePanel.h>
#include <vgui/IVGUI.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "clientmode_hlnormal.h"
#include "rendertexture.h"
#include "view_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_Manhack;

//-----------------------------------------------------------------------------
//
// In-game vgui panel which shows the RPG's ammo count
//
//-----------------------------------------------------------------------------
class CManhackScreen : public vgui::EditablePanel
{
    DECLARE_CLASS( CManhackScreen, vgui::EditablePanel);

public:
    CManhackScreen( vgui::Panel *parent, const char *panelName );
	~CManhackScreen();

	void		 SetManhackData(int iDistance, int iCount, C_NPC_Manhack* pManhack);
	void		 SetManhackView(const CNewViewSetup& view) { m_ManhackView = view; }
	virtual void PaintBackground();
private:
    vgui::Label *m_pManhackCount;
	vgui::Label *m_pManhackDistance;
	vgui::Label *m_pManhackOnline;
	int			m_iManhackDistance;

	C_NPC_Manhack*	m_pManhack;
	CNewViewSetup	m_ManhackView;
};

extern CManhackScreen	*GetManhackScreen();