//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

#include "OptionsSubMouse.h"
//#include "CommandCheckButton.h"
#include "KeyToggleCheckButton.h"
#include <vgui_controls/CvarNegateCheckButton.h>
#include <vgui_controls/cvartogglecheckbutton.h>
#include <vgui_controls/CvarSlider.h>

#include "EngineInterface.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include "tier1/convar.h"
#include <stdio.h>
#include <vgui_controls/TextEntry.h>
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

COptionsSubMouse::COptionsSubMouse(vgui::Panel *parent) : PropertyPage(parent, NULL)
{
	m_pReverseMouseCheckBox = new CCvarNegateCheckButton( 
		this, 
		"ReverseMouse", 
		"#GameUI_ReverseMouse", 
		"m_pitch" );
	
	m_pMouseFilterCheckBox = new CCvarToggleCheckButton( 
		this, 
		"MouseFilter", 
		"#GameUI_MouseFilter", 
		"m_filter" );

	m_pMouseRawCheckBox = new CCvarToggleCheckButton(
		this,
		"MouseRaw",
		"#GameUI_MouseRaw",
		"m_rawinput"
	);

	m_pMouseAccelCheckBox = new CheckButton(
		this,
		"MouseAccelerationCheckbox",
		"#GameUI_MouseAcceleration"
	);

	m_pJoystickCheckBox = new CCvarToggleCheckButton( 
		this, 
		"Joystick", 
		"#GameUI_Joystick", 
		"joystick" );

	m_pJoystickSouthpawCheckBox = new CCvarToggleCheckButton( 
		this, 
		"JoystickSouthpaw", 
		"#GameUI_JoystickSouthpaw", 
		"joy_movement_stick" );

	m_pReverseJoystickCheckBox = new CCvarToggleCheckButton( 
		this, 
		"ReverseJoystick", 
		"#GameUI_ReverseJoystick", 
		"joy_inverty" );

	m_pQuickInfoCheckBox = new CCvarToggleCheckButton(
		this,
		"HudQuickInfo",
		"#GameUI_HudQuickInfo",
		"hud_quickinfo" );

	m_pMouseSensitivitySlider = new CCvarSlider( this, "Slider", "#GameUI_MouseSensitivity",
		1.0f, 20.0f, "sensitivity", true );

    m_pMouseSensitivityLabel = new TextEntry(this, "SensitivityLabel");
    m_pMouseSensitivityLabel->AddActionSignalTarget(this);

	m_pMouseAccelSlider = new CCvarSlider(this, "MouseAccelerationSlider", "#GameUI_MouseAcceleration",
		1.0f, 1.4f, "m_customaccel_exponent", true);

	m_pMouseAccelLabel = new TextEntry(this, "MouseAccelerationLabel");
	m_pMouseAccelLabel->AddActionSignalTarget(this);

	m_pJoyYawSensitivitySlider = new CCvarSlider( this, "JoystickYawSlider", "#GameUI_JoystickYawSensitivity",
		-0.5f, -7.0f, "joy_yawsensitivity", true );
	m_pJoyYawSensitivityPreLabel = new Label(this, "JoystickYawSensitivityPreLabel", "#GameUI_JoystickLookSpeedYaw" );

	m_pJoyPitchSensitivitySlider = new CCvarSlider( this, "JoystickPitchSlider", "#GameUI_JoystickPitchSensitivity",
		0.5f, 7.0f, "joy_pitchsensitivity", true );
	m_pJoyPitchSensitivityPreLabel = new Label(this, "JoystickPitchSensitivityPreLabel", "#GameUI_JoystickLookSpeedPitch" );

	LoadControlSettings("Resource\\OptionsSubMouse.res");

    //float sensitivity = engine->pfnGetCvarFloat( "sensitivity" );
	ConVarRef var( "sensitivity" );
	if ( var.IsValid() )
	{
		float sensitivity = var.GetFloat();

		char buf[64];
		Q_snprintf(buf, sizeof(buf), " %.1f", sensitivity);
		m_pMouseSensitivityLabel->SetText(buf);
	}

	ConVarRef var2("m_customaccel_exponent");
	if (var2.IsValid())
	{
		float sensitivity = var2.GetFloat();

		char buf[64];
		Q_snprintf(buf, sizeof(buf), " %.1f", sensitivity);
		m_pMouseAccelLabel->SetText(buf);
	}

	UpdateJoystickPanels();
	UpdateAccelPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubMouse::~COptionsSubMouse()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnResetData()
{
	m_pReverseMouseCheckBox->Reset();
	m_pMouseFilterCheckBox->Reset();
	m_pJoystickCheckBox->Reset();
	m_pJoystickSouthpawCheckBox->Reset();
	m_pMouseSensitivitySlider->Reset();
	m_pQuickInfoCheckBox->Reset();
	m_pReverseJoystickCheckBox->Reset();
	m_pJoyYawSensitivitySlider->Reset();
	m_pJoyPitchSensitivitySlider->Reset();
	m_pMouseRawCheckBox->Reset();

	ConVarRef m_customaccel("m_customaccel");
	m_pMouseAccelCheckBox->SetSelected(m_customaccel.GetInt() == 3);

	m_pMouseAccelSlider->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnApplyChanges()
{
	m_pReverseMouseCheckBox->ApplyChanges();
	m_pMouseFilterCheckBox->ApplyChanges();
	m_pJoystickCheckBox->ApplyChanges();
	m_pJoystickSouthpawCheckBox->ApplyChanges();
	m_pMouseSensitivitySlider->ApplyChanges();
	m_pQuickInfoCheckBox->ApplyChanges();
	m_pReverseJoystickCheckBox->ApplyChanges();
	m_pJoyYawSensitivitySlider->ApplyChanges();
	m_pJoyPitchSensitivitySlider->ApplyChanges();
	m_pMouseRawCheckBox->ApplyChanges();

	ConVarRef m_customaccel("m_customaccel");
	m_customaccel.SetValue(m_pMouseAccelCheckBox->IsSelected() ? 3 : 0);
	
	m_pMouseAccelSlider->ApplyChanges();

	engine->ClientCmd_Unrestricted( "joyadvancedupdate" );
}

//-----------------------------------------------------------------------------
// Purpose: sets background color & border
//-----------------------------------------------------------------------------
void COptionsSubMouse::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnControlModified(Panel *panel)
{
	PostActionSignal(new KeyValues("ApplyButtonEnable"));

    // the HasBeenModified() check is so that if the value is outside of the range of the
    // slider, it won't use the slider to determine the display value but leave the
    // real value that we determined in the constructor
    if (panel == m_pMouseSensitivitySlider && m_pMouseSensitivitySlider->HasBeenModified())
    {
        UpdateSensitivityLabel();
    }
	else if (panel == m_pJoystickCheckBox)
	{
		UpdateJoystickPanels();
	}
	else if (panel == m_pMouseAccelSlider && m_pMouseAccelSlider->HasBeenModified())
	{
		char buf[64];
		Q_snprintf(buf, sizeof(buf), " %.1f", m_pMouseAccelSlider->GetSliderValue());
		m_pMouseAccelLabel->SetText(buf);
	}
	else if (panel == m_pMouseAccelCheckBox)
	{
		UpdateAccelPanels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnTextChanged(Panel *panel)
{
    if (panel == m_pMouseSensitivityLabel)
    {
        char buf[64];
        m_pMouseSensitivityLabel->GetText(buf, 64);

        float fValue = (float) atof(buf);
        if (fValue >= 1.0)
        {
            m_pMouseSensitivitySlider->SetSliderValue(fValue);
            PostActionSignal(new KeyValues("ApplyButtonEnable"));
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::UpdateSensitivityLabel()
{
    char buf[64];
    Q_snprintf(buf, sizeof( buf ), " %.1f", m_pMouseSensitivitySlider->GetSliderValue());
    m_pMouseSensitivityLabel->SetText(buf);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::UpdateJoystickPanels()
{
	bool bEnabled = m_pJoystickCheckBox->IsSelected();

	m_pReverseJoystickCheckBox->SetEnabled( bEnabled );
	m_pJoystickSouthpawCheckBox->SetEnabled( bEnabled );
	m_pJoyYawSensitivitySlider->SetEnabled( bEnabled );
	m_pJoyYawSensitivityPreLabel->SetEnabled( bEnabled );
	m_pJoyPitchSensitivitySlider->SetEnabled( bEnabled );
	m_pJoyPitchSensitivityPreLabel->SetEnabled( bEnabled );
}

void COptionsSubMouse::UpdateAccelPanels()
{
	bool bEnabled = m_pMouseAccelCheckBox->IsSelected();

	m_pMouseAccelSlider->SetEnabled(bEnabled);
	m_pMouseAccelLabel->SetEnabled(bEnabled);
}
