//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_PORTALGHOSTRENDERABLE_H
#define C_PORTALGHOSTRENDERABLE_H

#ifdef _WIN32
#pragma once
#endif

//#include "iclientrenderable.h"
#include "c_baseanimating.h"

class C_PortalGhostRenderable : public C_BaseAnimating//IClientRenderable, public IClientUnknown
{
	typedef C_BaseAnimating BaseClass;
public:
	C_BaseEntity *m_pGhostedRenderable; //the renderable we're transforming and re-rendering
	
	VMatrix m_matGhostTransform;
	float *m_pSharedRenderClipPlane; //shared by all portal ghost renderables within the same portal
	bool m_bLocalPlayer; //special draw rules for the local player
	bool m_bSourceIsBaseAnimating;
	bool m_bSourceIsBaseWeapon;
	C_Prop_Portal *m_pOwningPortal;

	struct
	{
		Vector vRenderOrigin;
		QAngle qRenderAngle;
		matrix3x4_t matRenderableToWorldTransform;
	} m_ReferencedReturns; //when returning a reference, it has to actually exist somewhere

	C_PortalGhostRenderable( C_Prop_Portal *pOwningPortal, C_BaseEntity *pGhostSource, RenderGroup_t sourceRenderGroup, const VMatrix &matGhostTransform, float *pSharedRenderClipPlane, bool bLocalPlayer );
	virtual ~C_PortalGhostRenderable( void );

	void PerFrameUpdate( void ); //called once per frame for misc updating

	virtual bool OnInternalDrawModel(ClientModelRenderInfo_t* pInfo)
	{
		if (m_pGhostedRenderable && m_bSourceIsBaseAnimating)
		{
			C_BaseAnimating* pAnim = m_pGhostedRenderable->GetBaseAnimating();
			CStudioHdr* pHdr = pAnim->GetModelPtr();
			if (pHdr)
			{
				matrix3x4_t matTransform;
				int iAttachment = pHdr->IllumPositionAttachmentIndex();
				if (iAttachment > 0)
				{
					pAnim->GetAttachment(iAttachment, matTransform);
				}
				else
				{
					matTransform = pAnim->EntityToWorldTransform();
				}

				static Vector s_vecLOrigin;
				VectorTransform(pHdr->illumposition(), matTransform, s_vecLOrigin);
				pInfo->pLightingOrigin = &s_vecLOrigin;
			}
		}

		return true;
	}

	virtual void SetupWeights(const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights);

	virtual float	GetHL1MouthOpenPct()
	{
		if (m_pGhostedRenderable && m_bSourceIsBaseAnimating)
		{
			C_BaseAnimating* pAnim = m_pGhostedRenderable->GetBaseAnimating();
			return pAnim->GetHL1MouthOpenPct();
		}
		else
		{
			return BaseClass::GetHL1MouthOpenPct();
		}
	}

	virtual float	GetHL2MouthOpenPct() { 
		if (m_pGhostedRenderable && m_bSourceIsBaseAnimating)
		{
			C_BaseAnimating* pAnim = m_pGhostedRenderable->GetBaseAnimating();
			return pAnim->GetHL2MouthOpenPct();
		}
		else
		{
			return BaseClass::GetHL2MouthOpenPct();
		}
	}

	virtual float	GetVisibleMouthOpenPct() { 
		if (m_pGhostedRenderable && m_bSourceIsBaseAnimating)
		{
			C_BaseAnimating* pAnim = m_pGhostedRenderable->GetBaseAnimating();
			return pAnim->GetVisibleMouthOpenPct();
		}
		else
		{
			return BaseClass::GetVisibleMouthOpenPct();
		}
	}

	virtual float	GetMouthOpenPct()
	{
		if (m_pGhostedRenderable && m_bSourceIsBaseAnimating)
		{
			C_BaseAnimating* pAnim = m_pGhostedRenderable->GetBaseAnimating();
			return pAnim->GetMouthOpenPct();
		}
		else
		{
			return BaseClass::GetMouthOpenPct();
		}
	}

	// Data accessors
	virtual Vector const&			GetRenderOrigin( void );
	virtual QAngle const&			GetRenderAngles( void );
	virtual bool					ShouldDraw( void ) { return true; }

	// Call this to get the current bone transforms for the model.
	// currentTime parameter will affect interpolation
	// nMaxBones specifies how many matrices pBoneToWorldOut can hold. (Should be greater than or
	// equal to studiohdr_t::numbones. Use MAXSTUDIOBONES to be safe.)
	virtual bool	SetupBones( matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds( Vector& mins, Vector& maxs );

	// returns the bounds as an AABB in worldspace
	virtual void	GetRenderBoundsWorldspace( Vector& mins, Vector& maxs );

	// These normally call through to GetRenderAngles/GetRenderBounds, but some entities custom implement them.
	virtual void	GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );

	// These methods return true if we want a per-renderable shadow cast direction + distance
	//virtual bool	GetShadowCastDistance( float *pDist, ShadowType_t shadowType ) const;
	//virtual bool	GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const;

	// Returns the transform from RenderOrigin/RenderAngles to world
	virtual const matrix3x4_t &RenderableToWorldTransform();

	// Attachments
	virtual	bool GetAttachment( int number, Vector &origin, QAngle &angles );
	virtual bool GetAttachment( int number, matrix3x4_t &matrix );
	virtual bool GetAttachment( int number, Vector &origin );
	virtual bool GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel );

	// Rendering clip plane, should be 4 floats, return value of NULL indicates a disabled render clip plane
	virtual float *GetRenderClipPlane( void ) { return m_pSharedRenderClipPlane; };

	virtual int	DrawModel( int flags );

	// Get the model instance of the ghosted model so that decals will properly draw across portals
	virtual ModelInstanceHandle_t GetModelInstance();



	//------------------------------------------
	//IClientRenderable - Trivial or redirection
	//------------------------------------------
	virtual IClientUnknown*			GetIClientUnknown() { return this; };
	virtual bool					IsTransparent( void );
	virtual bool					UsesPowerOfTwoFrameBufferTexture();
	//virtual ClientShadowHandle_t	GetShadowHandle() const { return m_hShadowHandle; };
	//virtual ClientRenderHandle_t&	RenderHandle() { return m_hRenderHandle; };
	//virtual const model_t*			GetModel( ) const;
	//virtual int						GetBody();
	//virtual void					ComputeFxBlend( ) { return m_pGhostedRenderable->ComputeFxBlend(); };
	//virtual int						GetFxBlend( void ) { return m_pGhostedRenderable->GetFxBlend(); };
	virtual void					GetColorModulation( float* color );
	//virtual bool					LODTest() { return true; };
	//virtual void					SetupWeights( void ) { NULL; };
	//virtual void					DoAnimationEvents( void ) { NULL; }; //TODO: find out if there's something we should be doing with this
	//virtual IPVSNotify*				GetPVSNotifyInterface() { return NULL; };
	//virtual bool					ShouldReceiveProjectedTextures( int flags ) { return false; };//{ return m_pGhostedRenderable->ShouldReceiveProjectedTextures( flags ); };
	//virtual bool					IsShadowDirty( ) { return m_bDirtyShadow; };
	//virtual void					MarkShadowDirty( bool bDirty ) { m_bDirtyShadow = bDirty; };
	//virtual IClientRenderable *		GetShadowParent() { return NULL; };
	//virtual IClientRenderable *		FirstShadowChild() { return NULL; };
	//virtual IClientRenderable *		NextShadowPeer() { return NULL; };
	//virtual ShadowType_t			ShadowCastType();
	//virtual void					CreateModelInstance() { NULL; };
	//virtual ModelInstanceHandle_t	GetModelInstance() { return m_pGhostedRenderable->GetModelInstance(); }; //TODO: find out if sharing an instance causes bugs
	virtual int						LookupAttachment( const char *pAttachmentName );
	//virtual int						GetSkin();
	//virtual bool					IsTwoPass( void );
	//virtual void					OnThreadedDrawSetup() { NULL; };

	//IHandleEntity
	//virtual void					SetRefEHandle( const CBaseHandle &handle ) { m_RefEHandle = handle; };
	//virtual const					CBaseHandle& GetRefEHandle() const { return m_RefEHandle; };

	//IClientUnknown
	virtual ICollideable*			GetCollideable() { return NULL; };
	virtual IClientNetworkable*		GetClientNetworkable() { return NULL; };
	virtual IClientRenderable*		GetClientRenderable() { return this; };
	virtual IClientEntity*			GetIClientEntity() { return NULL; };
	virtual C_BaseEntity*			GetBaseEntity() { return NULL; };
	virtual IClientThinkable*		GetClientThinkable() { return NULL; };
};

#endif //#ifndef C_PORTALGHOSTRENDERABLE_H