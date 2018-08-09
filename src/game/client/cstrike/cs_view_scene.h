//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CS_VIEW_SCENE_H
#define CS_VIEW_SCENE_H
#ifdef _WIN32
#pragma once
#endif

#include "viewrender.h"

//-----------------------------------------------------------------------------
// Purpose: Implements the interview to view rendering for the client .dll
//-----------------------------------------------------------------------------
class CCSViewRender : public CViewRender
{
public:
	CCSViewRender();

	virtual void Init( void );

	virtual void GetScreenFadeDistances( float *min, float *max );

	virtual void Render2DEffectsPreHUD( const CNewViewSetup &view );
	virtual void Render2DEffectsPostHUD( const CNewViewSetup &view );
	virtual void RenderPlayerSprites( void );

private:

	void PerformFlashbangEffect( const CNewViewSetup &view );
	void PerformNightVisionEffect( const CNewViewSetup &view );
	
	ITexture *m_pFlashTexture;
};

#endif //CS_VIEW_SCENE_H