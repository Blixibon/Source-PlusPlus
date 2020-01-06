#include "cbase.h"
#include <vgui_controls/PropertyPage.h>
#include "gameui\optionsdialog.h"
#include <vgui_controls/cvartogglecheckbutton.h>
#include <vgui_controls/ComboBox.h>
#include "materialsystem/imaterialsystemhardwareconfig.h"

using namespace vgui;

class CLazOptionsSub : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(CLazOptionsSub, vgui::PropertyPage);

public:
	CLazOptionsSub(vgui::Panel* parent);
	~CLazOptionsSub();

	virtual void OnResetData();
	virtual void OnApplyChanges();

	MESSAGE_FUNC(OnCheckButtonChecked, "CheckButtonChecked")
	{
		OnControlModified();
	}
	MESSAGE_FUNC(OnControlModified, "ControlModified");
	MESSAGE_FUNC(OnTextChanged, "TextChanged")
	{
		OnControlModified();
	}
protected:
	CvarToggleCheckButton<ConVarRef> *m_pPlayerLegsCheckBox;
	ComboBox* m_pCSMComboBox;
};

void AddCustomPagesToOptions(COptionsDialog* pThis)
{
	pThis->AddPage(new CLazOptionsSub(pThis), "#Lazul_OptionsPage");
	pThis->SetBounds(0, 0, 596, 406);
	pThis->MoveToCenterOfScreen();
}

CLazOptionsSub::CLazOptionsSub(vgui::Panel* parent) : vgui::PropertyPage(parent, NULL)
{
	m_pPlayerLegsCheckBox = new CvarToggleCheckButton<ConVarRef>(
		this,
		"LegsCheckbox",
		"#GameUI_LazOptionLegs",
		"cl_legs_enable"
		);

	m_pCSMComboBox = new ComboBox(
		this,
		"CSMCombo",
		3,
		false
	);
	m_pCSMComboBox->AddItem("#gameui_disabled", NULL);
	m_pCSMComboBox->AddItem("#gameui_enabled", NULL);
	m_pCSMComboBox->AddItem("#gameui_forced", NULL);
	m_pCSMComboBox->SetEnabled(g_pMaterialSystemHardwareConfig->SupportsShaderModel_3_0());
	

	LoadControlSettings("Resource\\OptionsSubLazul.res");
}

CLazOptionsSub::~CLazOptionsSub()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLazOptionsSub::OnControlModified()
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));
}

void CLazOptionsSub::OnResetData()
{
	m_pPlayerLegsCheckBox->Reset();

	ConVarRef r_csm_enabled("r_csm_enabled");
	m_pCSMComboBox->ActivateItem(r_csm_enabled.GetInt());
}

void CLazOptionsSub::OnApplyChanges()
{
	m_pPlayerLegsCheckBox->ApplyChanges();

	ConVarRef r_csm_enabled("r_csm_enabled");
	r_csm_enabled.SetValue(m_pCSMComboBox->GetActiveItem());
}
