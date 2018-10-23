//============ Copyright (c) Valve Corporation, All rights reserved. ============
//
// Functionality to render object motion blur
//
//===============================================================================

#include "cbase.h"
#include "object_motion_blur_effect.h"
#include "model_types.h"

int ScreenTransform(const Vector& point, Vector& screen);

ConVar mat_object_motion_blur_enable( "mat_object_motion_blur_enable", "0" );
ConVar mat_object_motion_blur_model_scale( "mat_object_motion_blur_model_scale", "1.2" );

CObjectMotionBlurManager g_ObjectMotionBlurManager;

int CObjectMotionBlurManager::RegisterObject( C_BaseAnimating *pEntity, float flVelocityScale )
{
	int nIndex;
	if ( m_nFirstFreeSlot == ObjectMotionBlurDefinition_t::END_OF_FREE_LIST )
	{
		nIndex = m_ObjectMotionBlurDefinitions.AddToTail();
	}
	else
	{
		nIndex = m_nFirstFreeSlot;
		m_nFirstFreeSlot = m_ObjectMotionBlurDefinitions[nIndex].m_nNextFreeSlot;
	}
	m_ObjectMotionBlurDefinitions[nIndex].m_pEntity = pEntity;
	m_ObjectMotionBlurDefinitions[nIndex].m_flVelocityScale = flVelocityScale;
	m_ObjectMotionBlurDefinitions[nIndex].m_nNextFreeSlot = ObjectMotionBlurDefinition_t::ENTRY_IN_USE;

	return nIndex;
}

void CObjectMotionBlurManager::UnregisterObject( int nObjectHandle )
{
	Assert( !m_ObjectMotionBlurDefinitions[nObjectHandle].IsUnused() );

	m_ObjectMotionBlurDefinitions[nObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
	m_ObjectMotionBlurDefinitions[nObjectHandle].m_pEntity = NULL;
	m_nFirstFreeSlot = nObjectHandle;
}

void CObjectMotionBlurManager::DrawObjects()
{
	for ( int i = 0; i < m_ObjectMotionBlurDefinitions.Count(); ++ i )
	{
		if ( m_ObjectMotionBlurDefinitions[i].ShouldDraw() )
		{
			m_ObjectMotionBlurDefinitions[i].DrawModel();
		}
	}
}

int CObjectMotionBlurManager::GetDrawableObjectCount()
{
	int nCount = 0;
	for ( int i = 0; i < m_ObjectMotionBlurDefinitions.Count(); ++ i )
	{
		if ( m_ObjectMotionBlurDefinitions[i].ShouldDraw() )
		{
			nCount++;
		}
	}
	return nCount;
}

float ScreenRescale(float flIn)
{
	return 0.5f * (1.0f + flIn);
}

void CObjectMotionBlurManager::ObjectMotionBlurDefinition_t::DrawModel()
{
#if 1
	Vector v3DVelocity, vOrigin;
	vOrigin = m_pEntity->GetRenderOrigin();
	m_pEntity->EstimateAbsVelocity(v3DVelocity);
	Vector v3DVelocityProj = vOrigin + v3DVelocity;
	Vector v2DStart, v2DEnd;
	Vector2D v2DVelocity;
	ScreenTransform(vOrigin, v2DStart);
	ScreenTransform(v3DVelocityProj, v2DEnd);
	v2DVelocity = v2DEnd.AsVector2D() - v2DStart.AsVector2D();

	float flR = ( m_flVelocityScale * ScreenRescale(v2DVelocity.x) );
	float flG = ( m_flVelocityScale * ScreenRescale(v2DVelocity.y) );
#else
	Vector vVelocity;
	m_pEntity->EstimateAbsVelocity(vVelocity);
	float flR = (m_flVelocityScale * vVelocity.x + 128.0f) / 256.0f;
	float flG = (m_flVelocityScale * vVelocity.y + 128.0f) / 256.0f;
#endif

	float flColor[3] = { flR, flG, 0.0f };
	render->SetColorModulation( flColor );

	C_BaseEntity *pAttachment;
	//RenderableInstance_t instance;
	//instance.m_nAlpha = 255;
	
	if ( mat_object_motion_blur_model_scale.GetFloat() != 1.0f )
	{
		float flBaseScale = m_pEntity->GetModelScale();
		m_pEntity->SetModelScale( flBaseScale * mat_object_motion_blur_model_scale.GetFloat() );
		m_pEntity->InvalidateBoneCache();

		m_pEntity->DrawModel( STUDIO_RENDER );
		pAttachment = m_pEntity->FirstMoveChild();

		while ( pAttachment != NULL )
		{
			if ( pAttachment->ShouldDraw() )
			{
				pAttachment->DrawModel( STUDIO_RENDER );
			}
			pAttachment = pAttachment->NextMovePeer();
		}

		m_pEntity->SetModelScale( flBaseScale );
		m_pEntity->InvalidateBoneCache();
	}

	flColor[2] = 1.0f;
	render->SetColorModulation( flColor );

	m_pEntity->DrawModel( STUDIO_RENDER );
	pAttachment = m_pEntity->FirstMoveChild();

	while ( pAttachment != NULL )
	{
		if ( pAttachment->ShouldDraw() )
		{
			pAttachment->DrawModel( STUDIO_RENDER );
		}
		pAttachment = pAttachment->NextMovePeer();
	}
}