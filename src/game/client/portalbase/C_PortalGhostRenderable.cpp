//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "C_PortalGhostRenderable.h"
#include "PortalRender.h"
#include "c_portal_player.h"
#include "model_types.h"
#include "c_basecombatweapon.h"
#include "utllinkedlist.h"

CUtlLinkedList< C_PortalGhostRenderable* > g_PortalGhostRenderables;

void InvalidateLocalPlayerGhosts()
{
	for (auto pGhost : g_PortalGhostRenderables)
	{
		if (pGhost->m_bLocalPlayer)
		{
			pGhost->InvalidateBoneCache();
		}
	}
}

C_PortalGhostRenderable::C_PortalGhostRenderable( C_Prop_Portal *pOwningPortal, C_BaseEntity *pGhostSource, RenderGroup_t sourceRenderGroup, const VMatrix &matGhostTransform, float *pSharedRenderClipPlane, bool bLocalPlayer )
: m_pGhostedRenderable( pGhostSource ), 
	m_matGhostTransform( matGhostTransform ), 
	m_pSharedRenderClipPlane( pSharedRenderClipPlane ),
	m_bLocalPlayer( bLocalPlayer ),
	m_pOwningPortal( pOwningPortal )
{
	m_bSourceIsBaseAnimating = (dynamic_cast<C_BaseAnimating *>(pGhostSource) != NULL);
	m_bSourceIsBaseWeapon = (dynamic_cast<C_BaseCombatWeapon*>(pGhostSource) != NULL);

	cl_entitylist->AddNonNetworkableEntity( GetIClientUnknown() );
	g_pClientLeafSystem->AddRenderable( this, sourceRenderGroup );
	g_PortalGhostRenderables.AddToTail(this);
}

C_PortalGhostRenderable::~C_PortalGhostRenderable( void )
{
	g_PortalGhostRenderables.FindAndRemove(this);
	m_pGhostedRenderable = NULL;
	g_pClientLeafSystem->RemoveRenderable( RenderHandle() );
	cl_entitylist->RemoveEntity( GetIClientUnknown()->GetRefEHandle() );

	DestroyModelInstance();
}

void C_PortalGhostRenderable::PerFrameUpdate( void )
{
	if( m_pGhostedRenderable )
	{
		if (!m_bSourceIsBaseWeapon)
		{
			SetModelName(m_pGhostedRenderable->GetModelName());
			SetModelIndex(m_pGhostedRenderable->GetModelIndex());
		}
		else
		{
			C_BaseCombatWeapon* pWeapon = m_pGhostedRenderable->MyCombatWeaponPointer();
			SetModelName(pWeapon->GetWorldModel());
			SetModelIndex(pWeapon->GetWorldModelIndex());
		}
		SetEffects( m_pGhostedRenderable->GetEffects() | EF_NOINTERP );		
		m_flAnimTime = m_pGhostedRenderable->m_flAnimTime;		

		if( m_bSourceIsBaseAnimating )
		{
			C_BaseAnimating *pSource = (C_BaseAnimating *)m_pGhostedRenderable;
			SetCycle( pSource->GetCycle() );
			SetSequence( pSource->GetSequence() );
			m_nBody = pSource->m_nBody;
			m_nSkin = pSource->m_nSkin;
		}

		// Set position and angles relative to the object it's ghosting
		Vector ptNewOrigin = m_matGhostTransform * m_pGhostedRenderable->GetAbsOrigin();
		QAngle qNewAngles = TransformAnglesToWorldSpace(m_pGhostedRenderable->GetAbsAngles(), m_matGhostTransform.As3x4());

		SetAbsOrigin(ptNewOrigin);
		SetAbsAngles(qNewAngles);
	}

	AddEffects( EF_NOINTERP );

	RemoveFromInterpolationList();

	g_pClientLeafSystem->RenderableChanged( RenderHandle() );
}

void C_PortalGhostRenderable::SetupWeights(const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights)
{
	if (m_pGhostedRenderable != nullptr)
		m_pGhostedRenderable->SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);
	else
		BaseClass::SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);
}

Vector const& C_PortalGhostRenderable::GetRenderOrigin( void )
{
	if( m_pGhostedRenderable == NULL )
		return m_ReferencedReturns.vRenderOrigin;

	m_ReferencedReturns.vRenderOrigin = m_matGhostTransform * m_pGhostedRenderable->GetRenderOrigin();
	return m_ReferencedReturns.vRenderOrigin;
}

QAngle const& C_PortalGhostRenderable::GetRenderAngles( void )
{
	if( m_pGhostedRenderable == NULL )
		return m_ReferencedReturns.qRenderAngle;

	m_ReferencedReturns.qRenderAngle = TransformAnglesToWorldSpace( m_pGhostedRenderable->GetRenderAngles(), m_matGhostTransform.As3x4() );
	return m_ReferencedReturns.qRenderAngle;
}

bool C_PortalGhostRenderable::SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime )
{
	if( m_pGhostedRenderable == NULL )
		return false;
	
	class CWeaponModelIndexHelper
	{
	public:
		CWeaponModelIndexHelper(bool bActive, C_BaseCombatWeapon* pWeapon)
		{
			m_bActive = bActive;
			m_pWeapon = pWeapon;
			if (m_bActive && m_pWeapon)
			{
				m_pWeapon->SetModelIndex(m_pWeapon->GetWorldModelIndex());
			}
		}
		~CWeaponModelIndexHelper()
		{
			if (m_bActive && m_pWeapon)
			{
				m_pWeapon->ValidateModelIndex();
			}
		}
	private:
		C_BaseCombatWeapon* m_pWeapon;
		bool				m_bActive;
	};
	class CPlayerBoneSetupHelper
	{
	public:
		CPlayerBoneSetupHelper(bool bActive, C_Portal_Player* pWeapon)
		{
			m_bActive = bActive;
			m_pWeapon = pWeapon;
			if (m_bActive && m_pWeapon)
			{
				m_pWeapon->SetIsSettingUpBonesForGhostAnim(true);
				m_pWeapon->InvalidateBoneCache();
			}
		}
		~CPlayerBoneSetupHelper()
		{
			if (m_bActive && m_pWeapon)
			{
				m_pWeapon->SetIsSettingUpBonesForGhostAnim(false);
				m_pWeapon->InvalidateBoneCache();
			}
		}
	private:
		C_Portal_Player* m_pWeapon;
		bool				m_bActive;
	};

	CWeaponModelIndexHelper helper(m_bSourceIsBaseWeapon, m_pGhostedRenderable->MyCombatWeaponPointer());

	CPlayerBoneSetupHelper plHelper(m_bLocalPlayer, ToPortalPlayer(m_pGhostedRenderable));

	if( m_pGhostedRenderable->SetupBones( pBoneToWorldOut, nMaxBones, boneMask, currentTime ) )
	{
		if( pBoneToWorldOut )
		{
			for( int i = 0; i != nMaxBones; ++i ) //FIXME: nMaxBones is most definitely greater than the actual number of bone transforms actually used, find the subset somehow
			{
				pBoneToWorldOut[i] = (m_matGhostTransform * pBoneToWorldOut[i]).As3x4();
			}
		}
		return true;
	}

	return false;
}

void C_PortalGhostRenderable::GetRenderBounds( Vector& mins, Vector& maxs )
{
	if( m_pGhostedRenderable == NULL )
	{
		mins = maxs = vec3_origin;
		return;
	}

	m_pGhostedRenderable->GetRenderBounds( mins, maxs );
}

void C_PortalGhostRenderable::GetRenderBoundsWorldspace( Vector& mins, Vector& maxs )
{
	if( m_pGhostedRenderable == NULL )
	{
		mins = maxs = vec3_origin;
		return;
	}

	m_pGhostedRenderable->GetRenderBoundsWorldspace( mins, maxs );
	TransformAABB( m_matGhostTransform.As3x4(), mins, maxs, mins, maxs );
}

void C_PortalGhostRenderable::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	m_pGhostedRenderable->GetShadowRenderBounds( mins, maxs, shadowType );
	TransformAABB( m_matGhostTransform.As3x4(), mins, maxs, mins, maxs );
}

/*bool C_PortalGhostRenderable::GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const
{
	if( m_pGhostedRenderable == NULL )
		return false;

	return m_pGhostedRenderable->GetShadowCastDistance( pDist, shadowType );
}

bool C_PortalGhostRenderable::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetShadowCastDirection( pDirection, shadowType ) )
	{
		if( pDirection )
			*pDirection = m_matGhostTransform.ApplyRotation( *pDirection );

		return true;
	}
	return false;
}*/

const matrix3x4_t & C_PortalGhostRenderable::RenderableToWorldTransform()
{
	if( m_pGhostedRenderable == NULL )
		return m_ReferencedReturns.matRenderableToWorldTransform;

	ConcatTransforms( m_matGhostTransform.As3x4(), m_pGhostedRenderable->RenderableToWorldTransform(), m_ReferencedReturns.matRenderableToWorldTransform );
	return m_ReferencedReturns.matRenderableToWorldTransform;
}

bool C_PortalGhostRenderable::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetAttachment( number, origin, angles ) )
	{
		origin = m_matGhostTransform * origin;
		angles = TransformAnglesToWorldSpace( angles, m_matGhostTransform.As3x4() );
		return true;
	}
	return false;
}

bool C_PortalGhostRenderable::GetAttachment( int number, matrix3x4_t &matrix )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetAttachment( number, matrix ) )
	{
		ConcatTransforms( m_matGhostTransform.As3x4(), matrix, matrix );
		return true;
	}
	return false;
}

bool C_PortalGhostRenderable::GetAttachment( int number, Vector &origin )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	if( m_pGhostedRenderable->GetAttachment( number, origin ) )
	{
		origin = m_matGhostTransform * origin;
		return true;
	}
	return false;
}

bool C_PortalGhostRenderable::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	Vector ghostVel;
	if( m_pGhostedRenderable->GetAttachmentVelocity( number, ghostVel, angleVel ) )
	{
		Vector3DMultiply( m_matGhostTransform, ghostVel, originVel );
		Vector3DMultiply( m_matGhostTransform, *(Vector*)( &angleVel ), *(Vector*)( &angleVel ) );
		return true;
	}
	return false;
}


int C_PortalGhostRenderable::DrawModel( int flags )
{
	if( m_bSourceIsBaseAnimating )
	{
		if( m_bLocalPlayer )
		{
			C_Portal_Player *pPlayer = C_Portal_Player::GetLocalPlayer();

			if ( !pPlayer->IsAlive() )
			{
				// Dead player uses a ragdoll to draw, so don't ghost the dead entity
				return 0;
			}
			else if (pPlayer->ShouldDoPortalRenderCulling() && pPlayer->InFirstPersonView())
			{
				if (g_pPortalRender->GetViewRecursionLevel() == 0)
				{
					if (pPlayer->m_bEyePositionIsTransformedByPortal)
						return 0;
				}
				else if (g_pPortalRender->GetViewRecursionLevel() == 1)
				{
					if (!pPlayer->m_bEyePositionIsTransformedByPortal)
						return 0;
				}
			}
		}

		return C_BaseAnimating::DrawModel( flags );
	}
	else
	{
		render->DrawBrushModel( m_pGhostedRenderable, 
								(model_t *)m_pGhostedRenderable->GetModel(), 
								GetRenderOrigin(), 
								GetRenderAngles(), 
								flags & STUDIO_TRANSPARENCY ? true : false );
		
		return 1;
	}

	return 0;
}

ModelInstanceHandle_t C_PortalGhostRenderable::GetModelInstance()
{
	if ( m_pGhostedRenderable && !m_bSourceIsBaseWeapon)
		return m_pGhostedRenderable->GetModelInstance();

	return BaseClass::GetModelInstance();
}




bool C_PortalGhostRenderable::IsTransparent( void )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	return m_pGhostedRenderable->IsTransparent();
}

bool C_PortalGhostRenderable::UsesPowerOfTwoFrameBufferTexture()
{
	if( m_pGhostedRenderable == NULL )
		return false;

	return m_pGhostedRenderable->UsesPowerOfTwoFrameBufferTexture();
}

/*const model_t* C_PortalGhostRenderable::GetModel( ) const
{
	if( m_pGhostedRenderable == NULL )
		return NULL;

	return m_pGhostedRenderable->GetModel();
}

int C_PortalGhostRenderable::GetBody()
{
	if( m_pGhostedRenderable == NULL )
		return 0;

	return m_pGhostedRenderable->GetBody();
}*/

void C_PortalGhostRenderable::GetColorModulation( float* color )
{
	if( m_pGhostedRenderable == NULL )
		return;

	return m_pGhostedRenderable->GetColorModulation( color );
}

/*ShadowType_t C_PortalGhostRenderable::ShadowCastType()
{
	if( m_pGhostedRenderable == NULL )
		return SHADOWS_NONE;

	return m_pGhostedRenderable->ShadowCastType();
}*/

int C_PortalGhostRenderable::LookupAttachment( const char *pAttachmentName )
{
	if( m_pGhostedRenderable == NULL )
		return -1;


	return m_pGhostedRenderable->LookupAttachment( pAttachmentName );
}

/*int C_PortalGhostRenderable::GetSkin()
{
	if( m_pGhostedRenderable == NULL )
		return -1;


	return m_pGhostedRenderable->GetSkin();
}

bool C_PortalGhostRenderable::IsTwoPass( void )
{
	if( m_pGhostedRenderable == NULL )
		return false;

	return m_pGhostedRenderable->IsTwoPass();
}*/









