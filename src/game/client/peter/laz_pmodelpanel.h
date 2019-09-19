//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef LAZ_PLAYERMODELPANEL_H
#define LAZ_PLAYERMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include "GameEventListener.h"
#include "KeyValues.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPlayerModelPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE(CPlayerModelPanel, vgui::EditablePanel );

	CPlayerModelPanel( vgui::Panel *parent, const char *name );
	virtual ~CPlayerModelPanel();

	virtual void Paint();
	virtual void ApplySettings( KeyValues *inResourceData );

	virtual void SetFOV( int nFOV ){ m_nFOV = nFOV; }

private:
	void InitCubeMaps(const char* pszCubemap, const char* pszCubemapHDR);

public:
	int								m_nFOV;

private:

	bool	m_bAllowOffscreen;

	CTextureReference m_DefaultEnvCubemap;
	CTextureReference m_DefaultHDREnvCubemap;
};


#endif // LAZ_PLAYERMODELPANEL_H
