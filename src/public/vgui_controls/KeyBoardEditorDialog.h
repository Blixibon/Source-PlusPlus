//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef KEYBOARDEDITORDIALOG_H
#define KEYBOARDEDITORDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/ListPanel.h"
#include "vgui/ISurface.h"

namespace vgui
{

	//-----------------------------------------------------------------------------
// Purpose: panel used for inline editing of key bindings
//-----------------------------------------------------------------------------
	class CInlineEditPanel : public vgui::Panel
	{
		DECLARE_CLASS_SIMPLE(CInlineEditPanel, vgui::Panel);

	public:
		CInlineEditPanel() : vgui::Panel(NULL, "InlineEditPanel")
		{
		}
		virtual ~CInlineEditPanel() {}

		virtual void Paint()
		{
			int wide, tall;
			GetSize(wide, tall);

			// Draw a white rectangle around that cell
			vgui::surface()->DrawSetColor(63, 63, 63, 255);
			vgui::surface()->DrawFilledRect(0, 0, wide, tall);

			vgui::surface()->DrawSetColor(0, 255, 0, 255);
			vgui::surface()->DrawOutlinedRect(0, 0, wide, tall);
		}

		virtual void OnKeyCodeTyped(KeyCode code)
		{
			// forward up
			if (GetParent())
			{
				GetParent()->OnKeyCodeTyped(code);
			}
		}

		virtual void ApplySchemeSettings(IScheme* pScheme)
		{
			Panel::ApplySchemeSettings(pScheme);
			SetBorder(pScheme->GetBorder("DepressedButtonBorder"));
		}

		void OnMousePressed(vgui::MouseCode code)
		{
			// forward up mouse pressed messages to be handled by the key options
			if (GetParent())
			{
				GetParent()->OnMousePressed(code);
			}
		}
	};

	//-----------------------------------------------------------------------------
	// Purpose: Special list subclass to handle drawing of trap mode prompt on top of
	//			lists client area
	//-----------------------------------------------------------------------------
	class VControlsListPanel : public ListPanel
	{
		DECLARE_CLASS_SIMPLE(VControlsListPanel, ListPanel);

	public:
		// Construction
		VControlsListPanel(vgui::Panel* parent, const char* listName);
		virtual			~VControlsListPanel();

		// Start/end capturing
		virtual void	StartCaptureMode(vgui::HCursor hCursor = NULL);
		virtual void	EndCaptureMode(vgui::HCursor hCursor = NULL);
		virtual bool	IsCapturing();

		// Set which item should be associated with the prompt
		virtual void	SetItemOfInterest(int itemID);
		virtual int		GetItemOfInterest();

		virtual void	OnMousePressed(vgui::MouseCode code);
		virtual void	OnMouseDoublePressed(vgui::MouseCode code);

		KEYBINDING_FUNC(clearbinding, KEY_DELETE, 0, OnClearBinding, 0, 0);

	private:
		void ApplySchemeSettings(vgui::IScheme* pScheme);

		// Are we showing the prompt?
		bool			m_bCaptureMode;
		// If so, where?
		int				m_nClickRow;
		// Font to use for showing the prompt
		vgui::HFont		m_hFont;
		// panel used to edit
		CInlineEditPanel* m_pInlineEditPanel;
		int m_iMouseX, m_iMouseY;
	};

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBoardEditorPage : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CKeyBoardEditorPage, EditablePanel );

public:
	CKeyBoardEditorPage( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle );
	~CKeyBoardEditorPage();

	void	SetKeybindingsSaveFile( char const *filename, char const *pathID = 0 );

	virtual void	OnKeyCodeTyped(vgui::KeyCode code);

	virtual void	ApplySchemeSettings( IScheme *scheme );

	void			OnSaveChanges();
	void			OnRevert();
	void			OnUseDefaults();

protected:

	virtual void	OnPageHide();

	virtual void	OnCommand( char const *cmd );

	void			PopulateList();

	void			GetMappingList( Panel *panel, CUtlVector< PanelKeyBindingMap * >& maps );
	int				GetMappingCount( Panel *panel );

	void			BindKey( vgui::KeyCode code );

		// Trap row selection message
	MESSAGE_FUNC( ItemSelected, "ItemSelected" );
	MESSAGE_FUNC_INT( OnClearBinding, "ClearBinding", item );

	void			SaveMappings();
	void			UpdateCurrentMappings();
	void			RestoreMappings();
	void			ApplyMappings();

protected:
	void					AnsiText( char const *token, char *out, size_t buflen );

	Panel			*m_pPanel;
	KeyBindingContextHandle_t m_Handle;

	VControlsListPanel	*m_pList;

	struct SaveMapping_t
	{
		SaveMapping_t();
		SaveMapping_t( const SaveMapping_t& src );

		PanelKeyBindingMap		*map;
		CUtlVector< BoundKey_t > current;
		CUtlVector< BoundKey_t > original;
	};

	CUtlVector< SaveMapping_t * > m_Save;
};


//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBoardEditorSheet : public PropertySheet
{
	DECLARE_CLASS_SIMPLE( CKeyBoardEditorSheet, PropertySheet );

public:
	CKeyBoardEditorSheet( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle );
	virtual ~CKeyBoardEditorSheet() {}

	void	SetKeybindingsSaveFile( char const *filename, char const *pathID = 0 );

	void			OnSaveChanges();
	void			OnRevert();
	void			OnUseDefaults();

protected:

	vgui::PHandle			m_hPanel;
	KeyBindingContextHandle_t m_Handle;
	bool					m_bSaveToExternalFile;
	CUtlSymbol				m_SaveFileName;
	CUtlSymbol				m_SaveFilePathID;
	Color					m_clrAlteredItem;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBoardEditorDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( CKeyBoardEditorDialog, Frame );

public:
	CKeyBoardEditorDialog( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle );
	virtual ~CKeyBoardEditorDialog() {}

	void			SetKeybindingsSaveFile( char const *filename, char const *pathID = 0 );

	virtual void	OnCommand( char const *cmd );

private:
	CKeyBoardEditorSheet		*m_pKBEditor;

	Button						*m_pSave;
	Button						*m_pCancel;
	Button						*m_pRevert;
	Button						*m_pUseDefaults;
};

}

#endif // KEYBOARDEDITORDIALOG_H
