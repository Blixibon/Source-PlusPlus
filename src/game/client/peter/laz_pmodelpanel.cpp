//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "materialsystem/imaterialsystem.h"
#include "engine/ivmodelinfo.h"

#include "gamestringpool.h"
#include "model_types.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "iefx.h"
#include "dlight.h"
#include "activitylist.h"
#include "econ_wearable.h"
#include "viewrender.h"

#include "laz_pmodelpanel.h"

bool UseHWMorphModels();

extern void MaybeInvalidateLocalPlayerAnimation();
extern int g_CurrentViewID;

using namespace vgui;

DECLARE_BUILD_FACTORY( CPlayerModelPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerModelPanel::CPlayerModelPanel( vgui::Panel *pParent, const char *pName ) : vgui::EditablePanel( pParent, pName )
{
	m_nFOV = 54;
	m_bAllowOffscreen = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPlayerModelPanel::~CPlayerModelPanel()
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_nFOV = inResourceData->GetInt( "fov", 54 );
	m_bAllowOffscreen = inResourceData->GetInt( "allow_offscreen", false );

	const char* pszCube = inResourceData->GetString("envmap", "editor/cubemap");
	const char* pszCubeHDR = inResourceData->GetString("envmap_hdr", "editor/cubemap.hdr");

	InitCubeMaps(pszCube, pszCubeHDR);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerModelPanel::InitCubeMaps(const char * pszCubemap, const char * pszCubemapHDR)
{
	ITexture *pCubemapTexture;

	// Deal with the default cubemap
	if ( g_pMaterialSystemHardwareConfig->GetHDREnabled() )
	{
		pCubemapTexture = materials->FindTexture(pszCubemapHDR, NULL, true );
		m_DefaultHDREnvCubemap.Init( pCubemapTexture );
	}
	else
	{
		pCubemapTexture = materials->FindTexture(pszCubemap, NULL, true );
		m_DefaultEnvCubemap.Init( pCubemapTexture );
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPlayerModelPanel::Paint()
{
	BaseClass::Paint();

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pLocalPlayer )
		return;

	MDLCACHE_CRITICAL_SECTION();

	int i = 0;
	int x, y, w, h;

	int iSavedView = g_CurrentViewID;
	g_CurrentViewID = VIEW_VGUI3D;

	GetBounds( x, y, w, h );
	ParentLocalToScreen( x, y );

	if ( !m_bAllowOffscreen && x < 0 )
	{
		// prevent x from being pushed off the left side of the screen
		// for modes like 1280 x 1024 (prevents model from being drawn in the panel)
		x = 0;
	}

	float flWidthRatio = ((float)w / (float)h ) / ( 4.0f / 3.0f );

	CMatRenderContextPtr pRenderContext( materials );
	
	// figure out what our viewport is right now
	int viewportX, viewportY, viewportWidth, viewportHeight;
	pRenderContext->GetViewport( viewportX, viewportY, viewportWidth, viewportHeight );

	// Now draw it.
	CNewViewSetup view;
	view.x = x + viewportX; // we actually want to offset by the 
	view.y = y + viewportY; // viewport origin here because Push3DView expects global coords below
	view.width = w;
	view.height = h;

	view.m_bOrtho = false;

	// scale the FOV for aspect ratios other than 4/3
	view.fov = ScaleFOVByWidthRatio( m_nFOV, flWidthRatio );

	MaybeInvalidateLocalPlayerAnimation();

	Vector vecEyes, vecForward, vecUp;
	QAngle angEyes;
	pLocalPlayer->GetAttachment("eyes", vecEyes, angEyes);
	AngleVectors(angEyes, &vecForward, nullptr, &vecUp);

	view.origin = vecEyes + (vecForward * 32.f);
	VectorAngles(-vecForward, vecUp, view.angles);
	view.zNear = VIEW_NEARZ;
	view.zFar = 1000;

	

	// Not supported by queued material system - doesn't appear to be necessary
//	ITexture *pLocalCube = pRenderContext->GetLocalCubemap();

	if ( g_pMaterialSystemHardwareConfig->GetHDREnabled() )
	{
		pRenderContext->BindLocalCubemap( m_DefaultHDREnvCubemap );
	}
	else
	{
		pRenderContext->BindLocalCubemap( m_DefaultEnvCubemap );
	}

	pRenderContext->SetLightingOrigin( vec3_origin );
	pRenderContext->SetAmbientLight( 0.4, 0.4, 0.4 );

	static Vector white[6] = 
	{
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
		Vector( 0.4, 0.4, 0.4 ),
	};

	g_pStudioRender->SetAmbientLightColors( white );
	g_pStudioRender->SetLocalLights( 0, NULL );


	Frustum dummyFrustum;
	render->Push3DView( view, 0, NULL, dummyFrustum );

	modelrender->SuppressEngineLighting( true );
	float color[3] = { 1.0f, 1.0f, 1.0f };
	render->SetColorModulation( color );
	render->SetBlend( 1.0f );
	pLocalPlayer->DrawModel( STUDIO_RENDER );

	if (pLocalPlayer->GetActiveWeapon() && !pLocalPlayer->IsInAVehicle())
	{
		C_BaseCombatWeapon* pWeapon = pLocalPlayer->GetActiveWeapon();
		pWeapon->SetModelIndex(pWeapon->GetWorldModelIndex());
		pWeapon->DrawModel(STUDIO_RENDER);
		pWeapon->SetModelIndex(pWeapon->CalcOverrideModelIndex());
	}

	for ( i = 0 ; i < pLocalPlayer->GetNumWearables(); i++ )
	{
		if ( pLocalPlayer->GetWearable(i) )
		{
			pLocalPlayer->GetWearable(i)->DrawModel( STUDIO_RENDER );
		}
	}

	modelrender->SuppressEngineLighting( false );
	
	render->PopView( dummyFrustum );

	pRenderContext->BindLocalCubemap( NULL );

	g_CurrentViewID = iSavedView;

	/*
	vgui::surface()->DrawSetColor( Color(0,0,0,255) );
	vgui::surface()->DrawOutlinedRect( 0,0, GetWide(), GetTall() );
	*/
	
}



