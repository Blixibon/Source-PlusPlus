#ifndef VIEWRENDER_DEFERRED_H
#define VIEWRENDER_DEFERRED_H

#pragma once

#include "viewrender.h"
#include "deferred/deferred_shared_common.h"

class CDeferredViewRender : public CViewRender
{
	DECLARE_CLASS( CDeferredViewRender, CViewRender );
public:
	CDeferredViewRender();
	virtual			~CDeferredViewRender( void ) {}

	virtual void	Shutdown( void );

	virtual void	RenderView( const CNewViewSetup &view, int nClearFlags, int whatToDraw );

	void			LevelInit( void );

	void			ResetCascadeDelay();

	void			ViewDrawSceneDeferred( const CNewViewSetup &view, int nClearFlags, view_id_t viewID,
										   bool bDrawViewModel );

	void			ViewDrawGBuffer( const CNewViewSetup &view, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible,
									 bool bDrawViewModel );
	void			ViewDrawComposite( const CNewViewSetup &view, bool &bDrew3dSkybox, SkyboxVisibility_t &nSkyboxVisible,
									   int nClearFlags, view_id_t viewID, bool bDrawViewModel );

	void			ViewCombineDeferredShading( const CNewViewSetup &view, view_id_t viewID );
	void			ViewOutputDeferredShading( const CNewViewSetup &view );

	void			DrawSkyboxComposite( const CNewViewSetup &view, const bool &bDrew3dSkybox );
	void			DrawWorldComposite( const CNewViewSetup &view, int nClearFlags, bool bDrawSkybox );

	void			DrawLightShadowView( const CNewViewSetup &view, int iDesiredShadowmap, def_light_t *l );
protected:
	void			DrawViewModels( const CNewViewSetup &view, bool drawViewmodel, bool bGBuffer );

private:

	void ProcessDeferredGlobals( const CNewViewSetup &view );

	void PerformLighting( const CNewViewSetup &view );

	void BeginRadiosity( const CNewViewSetup &view );
	void UpdateRadiosityPosition();
	void PerformRadiosityGlobal( int iRadiosityCascade, const CNewViewSetup &view );
	void EndRadiosity( const CNewViewSetup &view );
	void DebugRadiosity( const CNewViewSetup &view );

	void RenderCascadedShadows( const CNewViewSetup &view, bool bEnableRadiosity );

	float m_flRenderDelay[SHADOW_NUM_CASCADES];

	IMesh *GetRadiosityScreenGrid( const int iCascade );
	IMesh *CreateRadiosityScreenGrid( const Vector2D &vecViewportBase, float flWorldStepSize );

	Vector m_vecRadiosityOrigin[2];
	IMesh *m_pMesh_RadiosityScreenGrid[2];
	CUtlVector< IMesh* > m_hRadiosityDebugMeshList[2];
};


#endif // VIEWRENDER_DEFERRED_H