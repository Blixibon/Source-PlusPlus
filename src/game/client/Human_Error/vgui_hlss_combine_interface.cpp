#include "cbase.h"
#include "vgui_controls/Frame.h"
#include "c_baseanimating.h"
#include "vgui/IImage.h"
#include "vgui/IScheme.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Label.h"
#include "game_controls/basemodel_panel.h"
#include "fmtstr.h"
#include "peter/c_laz_player.h"
#include "clientmode_shared.h"
#include "animation.h"
#include "lazuul_gamerules.h"

enum ControlSlots_e
{
	SLOT_SHIELDS = 0,
	SLOT_TURRETS,
	SLOT_MANHACKS,
	SLOT_DOORS,

	NUM_CONTROL_SLOTS
};

class C_HLSSCombineInterface : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_HLSSCombineInterface, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

	~C_HLSSCombineInterface()
	{
		if (m_pGlowEffect)
		{
			delete m_pGlowEffect;
		}
	}

	bool	m_bSlotActive[NUM_CONTROL_SLOTS];
	bool	m_bSlotIsAvailable[NUM_CONTROL_SLOTS];
	bool	m_bDoorIsGate;
	CHandle<C_Laz_Player> m_hInterfacePlayer;

	virtual void	OnDataChanged(DataUpdateType_t updateType);

protected:
	CGlowObject* m_pGlowEffect;
};

IMPLEMENT_CLIENTCLASS_DT(C_HLSSCombineInterface, DT_HLSSCombineInterface, CHLSSCombineInterface)
RecvPropArray3(RECVINFO_ARRAY(m_bSlotActive), RecvPropBool(RECVINFO(m_bSlotActive[0]))),
RecvPropEHandle(RECVINFO(m_hInterfacePlayer)),

RecvPropArray3(RECVINFO_ARRAY(m_bSlotIsAvailable), RecvPropBool(RECVINFO(m_bSlotIsAvailable[0]))),
RecvPropBool(RECVINFO(m_bDoorIsGate)),
END_RECV_TABLE();

struct SlotConfig_t
{
	ControlSlots_e nSlot;

	const char* pszModelName;
	const char* pszActiveAnimation;
	const char* pszInactiveAnimation;
	const char* pszActivateAnim;
	const char* pszDeactivateAnim;

	int	nActiveSkin;
	int nInactiveSkin;

	const char* pszLabelEnable;
	const char* pszLabelDisable;
	const char* pszInfoDescription;
};

SlotConfig_t g_ShieldSlot = { SLOT_SHIELDS, "models/props_combine/combine_fence01b.mdl", NULL, NULL, NULL, NULL, 0, 1, "#HLSS_Interface_Shield_Enable", "#HLSS_Interface_Shield_Disable", "#HLSS_Interface_Shield_Description" };
SlotConfig_t g_TurretSlot = { SLOT_TURRETS, "models/combine_turrets/ceiling_turret.mdl", "idlealert", "idle", "deploy", "retract", 0, 0, "#HLSS_Interface_Turret_Enable", "#HLSS_Interface_Turret_Disable", "#HLSS_Interface_Turret_Description" };
SlotConfig_t g_ManhackSlot = { SLOT_MANHACKS, "models/manhack.mdl", "fly", "idle", "Deploy", NULL, 0, 0, "#HLSS_Interface_Manhack_Enable", "#HLSS_Interface_Manhack_Disable", "#HLSS_Interface_Manhack_Description" };
SlotConfig_t g_DoorSlot = { SLOT_DOORS, "models/props_combine/combine_lock01.mdl", NULL, NULL, NULL, NULL, 0, 0, "#HLSS_Interface_Door_Lock", "#HLSS_Interface_Door_Unlock", "#HLSS_Interface_Door_Description" };
SlotConfig_t g_GateSlot = { SLOT_DOORS, "models/combine_gate_vehicle.mdl", "idle_closed", "idle_open", "Close", "Open", 0, 0, "#HLSS_Interface_Gate_Close", "#HLSS_Interface_Gate_Open", "#HLSS_Interface_Gate_Description" };


class CHLSSInterfacePanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHLSSInterfacePanel, vgui::Frame);
public:
	CHLSSInterfacePanel(vgui::Panel* parent);
	~CHLSSInterfacePanel();

	// Perform graphical layout of button
	virtual void PerformLayout();
	// command handling
	virtual void OnCommand(const char* command);

	void	SetInterface(C_HLSSCombineInterface* pInterface);

	C_HLSSCombineInterface* GetInterfaceEnt()
	{
		return m_hActiveInterface.Get();
	}
protected:
	// painting
	virtual void PaintBackground();

	virtual void OnCursorMoved(int x, int y);

protected:
	vgui::IImage* m_pBackgroundImage;
	vgui::Button* m_pSlotButtons[5];
	vgui::Button* m_pCancelButton;
	vgui::Label* m_pInfoTitle;
	vgui::Label* m_pInfoDescription;
	CBaseModelPanel* m_pModelPanel;

	/*struct buttonBounds_t
	{
		int x, y, width, height;
	};
	buttonBounds_t m_ButtonBounds[5];*/

	CHandle< C_HLSSCombineInterface > m_hActiveInterface;
	SlotConfig_t* m_pButtonConfig[5];
	SlotConfig_t* m_pDrawingSlot;
	CStudioHdr* m_pStudioHdr;
};

static CHLSSInterfacePanel* s_pInterfacePanel = nullptr;
CHLSSInterfacePanel* GetInterfacePanel()
{
	if (!s_pInterfacePanel)
	{
		s_pInterfacePanel = new CHLSSInterfacePanel(g_pClientMode->GetViewport());
	}

	return s_pInterfacePanel;
}

CHLSSInterfacePanel::CHLSSInterfacePanel(vgui::Panel* parent) : vgui::Frame(parent, "HLSS_CombineInterface")
{
	SetSizeable(false);

	for (int i = 0; i < 5; i++)
	{
		m_pSlotButtons[i] = new vgui::Button(this, CFmtStr("SelectSlot%i", i+1), "");
		m_pSlotButtons[i]->SetParentNeedsCursorMoveEvents(true);
		m_pSlotButtons[i]->SetDepressedSound("buttons/button9.wav");
		m_pButtonConfig[i] = nullptr;
	}

	m_pCancelButton = new vgui::Button(this, "CancelSlot", "#HLSS_Cancel");
	m_pInfoTitle = new vgui::Label(this, "HLSS_InterfaceSlotName", "");
	m_pInfoDescription = new vgui::Label(this, "HLSS_InterfaceSlotDescription", "");
	m_pModelPanel = new CBaseModelPanel(this, "ModelPanel");

	LoadControlSettings("resource/UI/HLSS_CombineInterface.res");

	m_pBackgroundImage = vgui::scheme()->GetImage("interface", true);
	m_hActiveInterface.Term();
	m_pDrawingSlot = nullptr;
	m_pStudioHdr = nullptr;

	/*for (int i = 0; i < 5; i++)
	{
		buttonBounds_t& bounds = m_ButtonBounds[i];
		m_pSlotButtons[i]->GetBounds(bounds.x, bounds.y, bounds.width, bounds.height);
	}*/
}

CHLSSInterfacePanel::~CHLSSInterfacePanel()
{
	if (s_pInterfacePanel == this)
	{
		s_pInterfacePanel = nullptr;
	}

	if (m_pStudioHdr)
	{
		delete m_pStudioHdr;
		m_pStudioHdr = nullptr;
	}
}

void CHLSSInterfacePanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

void SendSlotCommandToServer(C_HLSSCombineInterface *pInterface, ControlSlots_e nSlot, bool bNewState)
{
	KeyValues* pkvCommand = new KeyValues("EntityCommand");
	pkvCommand->SetInt("target", pInterface->entindex());
	KeyValues* pkvData = pkvCommand->FindKey("SlotChange", true);
	pkvData->SetInt("slot", nSlot);
	pkvData->SetBool("enabled", bNewState);

	engine->ServerCmdKeyValues(pkvCommand);
}

void CHLSSInterfacePanel::OnCommand(const char* command)
{
	if (m_hActiveInterface.Get())
	{
		C_HLSSCombineInterface* pInterface = m_hActiveInterface.Get();

		if (V_strcmp(command, "slot1") == 0)
		{
			SlotConfig_t* pConfig = m_pButtonConfig[0];
			if (pConfig)
			{
				bool bNewState = !pInterface->m_bSlotActive[pConfig->nSlot];
				SendSlotCommandToServer(pInterface, pConfig->nSlot, bNewState);
			}
			return;
		}
		else if (V_strcmp(command, "slot2") == 0)
		{
			SlotConfig_t* pConfig = m_pButtonConfig[1];
			if (pConfig)
			{
				bool bNewState = !pInterface->m_bSlotActive[pConfig->nSlot];
				SendSlotCommandToServer(pInterface, pConfig->nSlot, bNewState);
			}
			return;
		}
		else if (V_strcmp(command, "slot3") == 0)
		{
			SlotConfig_t* pConfig = m_pButtonConfig[2];
			if (pConfig)
			{
				bool bNewState = !pInterface->m_bSlotActive[pConfig->nSlot];
				SendSlotCommandToServer(pInterface, pConfig->nSlot, bNewState);
			}
			return;
		}
		else if (V_strcmp(command, "slot4") == 0)
		{
			SlotConfig_t* pConfig = m_pButtonConfig[3];
			if (pConfig)
			{
				bool bNewState = !pInterface->m_bSlotActive[pConfig->nSlot];
				SendSlotCommandToServer(pInterface, pConfig->nSlot, bNewState);
			}
			return;
		}
		else if (V_strcmp(command, "slot5") == 0)
		{
			SlotConfig_t* pConfig = m_pButtonConfig[4];
			if (pConfig)
			{
				bool bNewState = !pInterface->m_bSlotActive[pConfig->nSlot];
				SendSlotCommandToServer(pInterface, pConfig->nSlot, bNewState);
			}
			return;
		}
		else if (V_strcmp(command, "close") == 0)
		{
			KeyValues* pkvCommand = new KeyValues("EntityCommand");
			pkvCommand->SetInt("target", pInterface->entindex());
			/*KeyValues* pkvData =*/ pkvCommand->FindKey("Close", true);
			//pkvData->SetInt("dummy", 1);

			engine->ServerCmdKeyValues(pkvCommand);
			return;
		}
	}

	BaseClass::OnCommand(command);
}

void CHLSSInterfacePanel::SetInterface(C_HLSSCombineInterface* pInterface)
{
	if (pInterface != m_hActiveInterface.Get())
	{
		m_hActiveInterface.Set(pInterface);

		if (!pInterface)
		{
			m_pDrawingSlot = nullptr;
			m_pInfoTitle->SetText(L"");
			m_pInfoDescription->SetText(L"");

			if (m_pStudioHdr)
			{
				delete m_pStudioHdr;
				m_pStudioHdr = nullptr;
			}
			m_pModelPanel->SetMDL(MDLHANDLE_INVALID);
			return;
		}

		int iButtonIndex = 0;
		for (int i = 0; i < NUM_CONTROL_SLOTS; i++)
		{
			if (pInterface->m_bSlotIsAvailable[i])
			{
				switch (i)
				{
				case SLOT_SHIELDS:
					m_pButtonConfig[iButtonIndex] = &g_ShieldSlot;
					break;
				case SLOT_TURRETS:
					m_pButtonConfig[iButtonIndex] = &g_TurretSlot;
					break;
				case SLOT_MANHACKS:
					m_pButtonConfig[iButtonIndex] = &g_ManhackSlot;
					break;
				case SLOT_DOORS:
					m_pButtonConfig[iButtonIndex] = pInterface->m_bDoorIsGate ? &g_GateSlot : &g_DoorSlot;
					break;
				default:
					m_pButtonConfig[iButtonIndex] = nullptr;
					break;
				}

				m_pSlotButtons[iButtonIndex]->SetVisible(true);
				iButtonIndex++;
			}
		}

		for (; iButtonIndex < 5; iButtonIndex++)
		{
			m_pButtonConfig[iButtonIndex] = nullptr;
			m_pSlotButtons[iButtonIndex]->SetVisible(false);
		}

		Activate();
	}

	if (pInterface)
	{
		for (int i = 0; i < 5; i++)
		{
			if (m_pButtonConfig[i])
			{
				const char* pszText = pInterface->m_bSlotActive[m_pButtonConfig[i]->nSlot] ? m_pButtonConfig[i]->pszLabelDisable : m_pButtonConfig[i]->pszLabelEnable;
				m_pSlotButtons[i]->SetText(pszText);

				/*buttonBounds_t& bounds = m_ButtonBounds[i];
				m_pSlotButtons[i]->SetBounds(bounds.x, bounds.y, bounds.width, bounds.height);*/

				if (m_pButtonConfig[i] == m_pDrawingSlot)
				{
					m_pInfoTitle->SetText(pszText);
				}
			}
		}

		if (m_pDrawingSlot && m_pStudioHdr)
		{
			bool bSlotActive = pInterface->m_bSlotActive[m_pDrawingSlot->nSlot];
			int iSkin = bSlotActive ? m_pDrawingSlot->nActiveSkin : m_pDrawingSlot->nInactiveSkin;
			m_pModelPanel->SetSkin(iSkin);

			const char* pszAnimation = bSlotActive ? m_pDrawingSlot->pszActivateAnim : m_pDrawingSlot->pszDeactivateAnim;
			if (!pszAnimation)
				pszAnimation = bSlotActive ? m_pDrawingSlot->pszActiveAnimation : m_pDrawingSlot->pszInactiveAnimation;

			int iSequence = 0;
			if (pszAnimation)
			{
				iSequence = LookupSequence(m_pStudioHdr, pszAnimation);
			}

			m_pModelPanel->SetSequence(iSequence, true);
		}
	}
}

void CHLSSInterfacePanel::PaintBackground()
{
	if (!m_pBackgroundImage)
	{
		BaseClass::PaintBackground();
		return;
	}

	int imageWide, imageTall;
	m_pBackgroundImage->GetSize(imageWide, imageTall);

	int wide, tall;
	GetSize(wide, tall);
	m_pBackgroundImage->SetSize(wide, tall);
	m_pBackgroundImage->SetPos(0, 0);

	m_pBackgroundImage->Paint();

	m_pBackgroundImage->SetSize(imageWide, imageTall);
}

void CHLSSInterfacePanel::OnCursorMoved(int x, int y)
{
	BaseClass::OnCursorMoved(x, y);

	for (int i = 0; i < 5; i++)
	{
		if (m_pSlotButtons[i]->IsVisible() && m_pSlotButtons[i]->IsArmed() && m_pButtonConfig[i] && m_pButtonConfig[i] != m_pDrawingSlot)
		{
			m_pDrawingSlot = m_pButtonConfig[i];
			{
				bool bSlotActive = m_hActiveInterface.Get() && m_hActiveInterface->m_bSlotActive[m_pDrawingSlot->nSlot];
				m_pInfoTitle->SetText(bSlotActive ? m_pDrawingSlot->pszLabelDisable : m_pDrawingSlot->pszLabelEnable);
				m_pInfoDescription->SetText(m_pDrawingSlot->pszInfoDescription);

				m_pModelPanel->SetMDL(m_pDrawingSlot->pszModelName, m_hActiveInterface.Get());
				m_pModelPanel->LookAtMDL();
				if (m_pStudioHdr)
				{
					delete m_pStudioHdr;
					m_pStudioHdr = nullptr;
				}
				m_pStudioHdr = new CStudioHdr(m_pModelPanel->GetStudioHdr(), mdlcache);

				m_pModelPanel->SetSkin(bSlotActive ? m_pDrawingSlot->nActiveSkin : m_pDrawingSlot->nInactiveSkin);
				const char* pszIdleAnim = bSlotActive ? m_pDrawingSlot->pszActiveAnimation : m_pDrawingSlot->pszInactiveAnimation;
				int iSequence = 0;
				if (pszIdleAnim)
				{
					iSequence = LookupSequence(m_pStudioHdr, pszIdleAnim);
				}

				m_pModelPanel->SetSequence(iSequence);
			}
			break;
		}
	}
}

void C_HLSSCombineInterface::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	C_HLSSCombineInterface* pCurrentInterfact = GetInterfacePanel()->GetInterfaceEnt();
	if (m_hInterfacePlayer.Get() == C_BasePlayer::GetLocalPlayer() && (pCurrentInterfact == nullptr || pCurrentInterfact == this))
	{
		GetInterfacePanel()->SetInterface(this);
	}
	else if (pCurrentInterfact == this)
	{
		GetInterfacePanel()->SetInterface(nullptr);
		GetInterfacePanel()->Close();
	}

	if (InLocalTeam())
	{
		if (!m_pGlowEffect)
		{
			Vector vColor;
			TeamplayRoundBasedRules()->GetTeamGlowColor(GetTeamNumber(), vColor.x, vColor.y, vColor.z);
			m_pGlowEffect = new CGlowObject(this, vColor, 1.f, true, true);
		}
	}
	else if (m_pGlowEffect)
	{
		delete m_pGlowEffect;
		m_pGlowEffect = nullptr;
	}
}