//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client side view model implementation. Responsible for drawing
//			the view model.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseviewmodel.h"
#include "model_types.h"
#include "hud.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "view.h"
#include "mathlib/vmatrix.h"
#include "cl_animevent.h"
#include "eventlist.h"
#include "tools/bonelist.h"
#include <KeyValues.h>
#include "hltvcamera.h"
#include "bone_setup.h"
#if defined( TF_CLIENT_DLL ) || defined ( TF_CLASSIC_CLIENT )
	#include "tf_weaponbase.h"
#endif

#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#endif

// NVNT haptics system interface
#include "haptics/ihaptics.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined(CSTRIKE_DLL) || defined(HL2_LAZUL)
	ConVar cl_righthand( "cl_righthand", "1", FCVAR_ARCHIVE, "Use right-handed view models." );
#endif

#if defined( TF_CLIENT_DLL ) || defined ( TF_CLASSIC_CLIENT )
	ConVar cl_flipviewmodels( "cl_flipviewmodels", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_NOT_CONNECTED, "Flip view models." );
#endif

void PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg );

void FormatViewModelAttachment( Vector &vOrigin, bool bInverse )
{
	// Presumably, SetUpView has been called so we know our FOV and render origin.
	const CNewViewSetup *pViewSetup = view->GetPlayerViewSetup();
	
	float worldx = tan( pViewSetup->fov * M_PI/360.0 );
	float viewx = tan( pViewSetup->fovViewmodel * M_PI/360.0 );

	// aspect ratio cancels out, so only need one factor
	// the difference between the screen coordinates of the 2 systems is the ratio
	// of the coefficients of the projection matrices (tan (fov/2) is that coefficient)
	// NOTE: viewx was coming in as 0 when folks set their viewmodel_fov to 0 and show their weapon.
	float factorX = viewx ? ( worldx / viewx ) : 0.0f;
	float factorY = factorX;
	
	// Get the coordinates in the viewer's space.
	Vector tmp = vOrigin - pViewSetup->origin;
	Vector vTransformed( MainViewRight().Dot( tmp ), MainViewUp().Dot( tmp ), MainViewForward().Dot( tmp ) );

	// Now squash X and Y.
	if ( bInverse )
	{
		if ( factorX != 0 && factorY != 0 )
		{
			vTransformed.x /= factorX;
			vTransformed.y /= factorY;
		}
		else
		{
			vTransformed.x = 0.0f;
			vTransformed.y = 0.0f;
		}
	}
	else
	{
		vTransformed.x *= factorX;
		vTransformed.y *= factorY;
	}



	// Transform back to world space.
	Vector vOut = (MainViewRight() * vTransformed.x) + (MainViewUp() * vTransformed.y) + (MainViewForward() * vTransformed.z);
	vOrigin = pViewSetup->origin + vOut;
}

class C_ViewHands : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_ViewHands, C_BaseAnimating);
	//DECLARE_NETWORKCLASS();

	virtual RenderGroup_t	GetRenderGroup();

	/*virtual void					SpawnClientEntity(void)
	{
		AddEffects(EF_NODRAW);
	}*/

	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType()
	{
		return SHADOWS_NONE;
	}

	virtual float	GetWetness()
	{
		if (m_pViewModel)
			return m_pViewModel->GetWetness();
		return BaseClass::GetWetness();
	}

	virtual bool ShouldDraw();
	virtual int DrawModel(int flags);

	virtual bool OnInternalDrawModel(ClientModelRenderInfo_t *pInfo)
	{
		if (m_pViewModel)
		{
			CStudioHdr *pHdr = m_pViewModel->GetModelPtr();
			if (pHdr)
			{
				matrix3x4_t matTransform;
				int iAttachment = pHdr->IllumPositionAttachmentIndex();
				if (iAttachment > 0)
				{
					m_pViewModel->GetAttachment(iAttachment, matTransform);
				}
				else
				{
					matTransform = m_pViewModel->EntityToWorldTransform();
				}

				static Vector s_vecLOrigin;
				VectorTransform(pHdr->illumposition(), matTransform, s_vecLOrigin);
				pInfo->pLightingOrigin = &s_vecLOrigin;
			}
		}

		return true;
	}

	// Determine the color modulation amount
	virtual void	GetColorModulation(float* color)
	{
		Assert(color);
		if (m_pViewModel)
		{
			m_pViewModel->GetColorModulation(color);
			return;
		}
		BaseClass::GetColorModulation(color);
	}

	// Accessors for color.
	const color32 GetRenderColor() const
	{
		if (m_pViewModel)
			return m_pViewModel->GetRenderColor();

		return BaseClass::GetRenderColor();
	}
	
	virtual void CalcBoneMerge(CStudioHdr *hdr, int boneMask, CBoneBitList &boneComputed);
#if 0
	void DrawHands(C_BaseViewModel *pModel, int iFlags)
	{
		if (!pModel || (iFlags & STUDIO_RENDER) == 0)
			return;

		int iSkin, iBody;
		int iIndex = pModel->GetHandModelData(iSkin, iBody);

		if (iIndex <= -1)
			return;

		//RemoveEffects(EF_NODRAW);

		//m_pViewModel = pModel;
			
		/*SetModelByIndex(iIndex);
		m_nSkin = iSkin;
		m_nBody = iBody;*/

		/*FollowEntity(pModel, true);
		InvalidateBoneCache();*/

		InternalDrawModel(iFlags);

		//StopFollowingEntity();
		//SetModelByIndex(-1);

		//m_pViewModel = nullptr;
		//AddEffects(EF_NODRAW);
	}
#endif
	virtual bool			ShouldReceiveProjectedTextures(int flags)
	{
		if (m_pViewModel)
			return m_pViewModel->ShouldReceiveProjectedTextures(flags);

		return BaseClass::ShouldReceiveProjectedTextures(flags);
	}

	virtual int				InternalDrawModel(int flags);
	virtual void ApplyBoneMatrixTransform(matrix3x4_t& transform);

	CBaseViewModel *m_pViewModel;

	class CHandsMergeCache : public CBoneMergeCache
	{
	protected:
		virtual int	GetParentBone(CStudioHdr *pHdr, const char *pszName, boneextradata_t& extraData);
	};
};


bool C_ViewHands::ShouldDraw()
{
	if (m_pViewModel)
		return m_pViewModel->ShouldDraw();

	return BaseClass::ShouldDraw();
}

int C_ViewHands::DrawModel(int flags)
{
	//if (view->GetCurrentlyDrawingEntity() != m_pViewModel)
	//	return 0;

	return BaseClass::DrawModel(flags);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_ViewHands::GetRenderGroup()
{
	return RENDER_GROUP_VIEW_MODEL_OPAQUE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_ViewHands::InternalDrawModel(int flags)
{
	CMatRenderContextPtr pRenderContext(materials);
	if (m_pViewModel && m_pViewModel->ShouldFlipViewModel())
		pRenderContext->CullMode(MATERIAL_CULLMODE_CW);

	int ret = BaseClass::InternalDrawModel(flags);

	pRenderContext->CullMode(MATERIAL_CULLMODE_CCW);

	return ret;
}

void C_ViewHands::ApplyBoneMatrixTransform(matrix3x4_t& transform)
{
	if (m_pViewModel && m_pViewModel->ShouldFlipViewModel())
	{
		matrix3x4_t viewMatrix, viewMatrixInverse;

		// We could get MATERIAL_VIEW here, but this is called sometimes before the renderer
		// has set that matrix. Luckily, this is called AFTER the CViewSetup has been initialized.
		const CViewSetup *pSetup = view->GetPlayerViewSetup();
		AngleMatrix(pSetup->angles, pSetup->origin, viewMatrixInverse);
		MatrixInvert(viewMatrixInverse, viewMatrix);

		// Transform into view space.
		matrix3x4_t temp, temp2;
		ConcatTransforms(viewMatrix, transform, temp);

		// Flip it along X.

		// (This is the slower way to do it, and it equates to negating the top row).
		//matrix3x4_t mScale;
		//SetIdentityMatrix( mScale );
		//mScale[0][0] = 1;
		//mScale[1][1] = -1;
		//mScale[2][2] = 1;
		//ConcatTransforms( mScale, temp, temp2 );
		temp[1][0] = -temp[1][0];
		temp[1][1] = -temp[1][1];
		temp[1][2] = -temp[1][2];
		temp[1][3] = -temp[1][3];

		// Transform back out of view space.
		ConcatTransforms(viewMatrixInverse, temp, transform);
	}
}

int C_ViewHands::CHandsMergeCache::GetParentBone(CStudioHdr *pHdr, const char *pszName, boneextradata_t& extraData)
{
	int iBone = CBoneMergeCache::GetParentBone(pHdr, pszName, extraData);
	if (iBone < 0)
	{
		char name[128];
		if (Studio_BoneIndexByName(pHdr, "v_weapon") >= 0 && V_StrSubst(pszName, "ValveBiped", "v_weapon", name, ARRAYSIZE(name), false))
		{
			iBone = Studio_BoneIndexByName(pHdr, name);
		}
	}

	return iBone;
}

void C_ViewHands::CalcBoneMerge(CStudioHdr * hdr, int boneMask, CBoneBitList & boneComputed)
{
	bool boneMerge = IsEffectActive(EF_BONEMERGE);
	if (boneMerge || m_pBoneMergeCache)
	{
		if (boneMerge)
		{
			if (!m_pBoneMergeCache)
			{
				m_pBoneMergeCache = new CHandsMergeCache;
				m_pBoneMergeCache->Init(this);
			}
			m_pBoneMergeCache->MergeMatchingBones(boneMask, boneComputed);
		}
		else
		{
			delete m_pBoneMergeCache;
			m_pBoneMergeCache = NULL;
		}
	}
}

LINK_ENTITY_TO_CLASS_CLIENTONLY(viewhands, C_ViewHands);

//static C_ViewHands *s_pViewHands[MAX_VIEWMODELS] = { nullptr };
//
//class CViewHandsHandler : public CAutoGameSystem
//{
//public:
//	CViewHandsHandler() : CAutoGameSystem("ViewHandsHandler")
//	{}
//
//	void LevelShutdownPostEntity()
//	{
//		for (int i = 0; i < MAX_VIEWMODELS; i++)
//		{
//			s_pViewHands[i] = nullptr;
//		}
//	}
//
//	void LevelInitPostEntity()
//	{
//		for (int i = 0; i < MAX_VIEWMODELS; i++)
//		{
//			s_pViewHands[i] = (C_ViewHands *)CreateEntityByName("viewhands");
//			s_pViewHands[i]->InitializeAsClientEntity(NULL, RENDER_GROUP_VIEW_MODEL_OPAQUE);
//		}
//	}
//};
//
//CViewHandsHandler g_ViewHandsCreator;

void C_BaseViewModel::FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld )
{
	Vector vecOrigin;
	MatrixPosition( attachmentToWorld, vecOrigin );
	::FormatViewModelAttachment( vecOrigin, false );
	PositionMatrix( vecOrigin, attachmentToWorld );
}


bool C_BaseViewModel::IsViewModel() const
{
	return true;
}

void C_BaseViewModel::UncorrectViewModelAttachment( Vector &vOrigin )
{
	// Unformat the attachment.
	::FormatViewModelAttachment( vOrigin, true );
}


//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void C_BaseViewModel::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	// We override sound requests so that we can play them locally on the owning player
	if ( ( event == AE_CL_PLAYSOUND ) || ( event == CL_EVENT_SOUND ) )
	{
		// Only do this if we're owned by someone
		if ( GetOwner() != NULL )
		{
			CLocalPlayerFilter filter;
			EmitSound( filter, GetOwner()->GetSoundSourceIndex(), options, &GetAbsOrigin() );
			return;
		}
	}
	else if (event == AE_VM_BODYGROUP_SET)
	{
		int value;
		char token[256];
		char szBodygroupName[256];

		const char* p = options;

		// Bodygroup Name
		p = nexttoken(token, p, ' ');
		Q_strncpy(szBodygroupName, token, sizeof(szBodygroupName));

		// Get the desired value
		p = nexttoken(token, p, ' ');
		value = token[0] ? atoi(token) : 0;

		int index = FindBodygroupByName(szBodygroupName);
		if (index >= 0)
		{
			SetBodygroup(index, value);
		}

		return;
	}
	else if (event == AE_VM_DISABLE_BODYGROUP)
	{
		int index = FindBodygroupByName(options);
		if (index >= 0)
		{
			SetBodygroup(index, 0);
		}
		return;
	}
	else if (event == AE_VM_ENABLE_BODYGROUP)
	{
		int index = FindBodygroupByName(options);
		if (index >= 0)
		{
			SetBodygroup(index, 1);
		}
		return;
	}

	// Otherwise pass the event to our associated weapon
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		// NVNT notify the haptics system of our viewmodel's event
		if ( haptics )
			haptics->ProcessHapticEvent(4,"Weapons",pWeapon->GetName(),"AnimationEvents",VarArgs("%i",event));

		bool bResult = pWeapon->OnFireEvent( this, origin, angles, event, options );
		if ( !bResult )
		{
			BaseClass::FireEvent( origin, angles, event, options );
		}
	}
}

bool C_BaseViewModel::Interpolate( float currentTime )
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	// Make sure we reset our animation information if we've switch sequences
	UpdateAnimationParity();

	bool bret = BaseClass::Interpolate( currentTime );

	// Hack to extrapolate cycle counter for view model
	float elapsed_time = currentTime - m_flAnimTime;
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	// Predicted viewmodels have fixed up interval
	if ( GetPredictable() || IsClientCreated() )
	{
		Assert( pPlayer );
		float curtime = pPlayer ? pPlayer->GetFinalPredictedTime() : gpGlobals->curtime;
		elapsed_time = curtime - m_flAnimTime;
		// Adjust for interpolated partial frame
		if ( !engine->IsPaused() )
		{
			elapsed_time += ( gpGlobals->interpolation_amount * TICK_INTERVAL );
		}
	}

	// Prediction errors?	
	if ( elapsed_time < 0 )
	{
		elapsed_time = 0;
	}

	float dt = elapsed_time * GetSequenceCycleRate( pStudioHdr, GetSequence() ) * GetPlaybackRate();
	if ( dt >= 1.0f )
	{
		if ( !IsSequenceLooping( GetSequence() ) )
		{
			dt = 0.999f;
		}
		else
		{
			dt = fmod( dt, 1.0f );
		}
	}

	SetCycle( dt );
	return bret;
}


bool C_BaseViewModel::ShouldFlipViewModel()
{
#if defined(CSTRIKE_DLL) || defined(HL2_LAZUL)
	// If cl_righthand is set, then we want them all right-handed.
	CBaseCombatWeapon *pWeapon = m_hWeapon.Get();
	if ( pWeapon )
	{
		const FileWeaponInfo_t *pInfo = &pWeapon->GetWpnData();
		return pInfo->m_bAllowFlipping && pInfo->m_bBuiltRightHanded != cl_righthand.GetBool();
	}
#endif

#if defined( TF_CLIENT_DLL ) || defined ( TF_CLASSIC_CLIENT )
	CBaseCombatWeapon *pWeapon = m_hWeapon.Get();
	if ( pWeapon )
	{
		return pWeapon->m_bFlipViewModel != cl_flipviewmodels.GetBool();
	}
#endif

	return false;
}


void C_BaseViewModel::ApplyBoneMatrixTransform( matrix3x4_t& transform )
{
	if ( ShouldFlipViewModel() )
	{
		matrix3x4_t viewMatrix, viewMatrixInverse;

		// We could get MATERIAL_VIEW here, but this is called sometimes before the renderer
		// has set that matrix. Luckily, this is called AFTER the CNewViewSetup has been initialized.
		const CNewViewSetup *pSetup = view->GetPlayerViewSetup();
		AngleMatrix( pSetup->angles, pSetup->origin, viewMatrixInverse );
		MatrixInvert( viewMatrixInverse, viewMatrix );

		// Transform into view space.
		matrix3x4_t temp, temp2;
		ConcatTransforms( viewMatrix, transform, temp );
		
		// Flip it along X.
		
		// (This is the slower way to do it, and it equates to negating the top row).
		//matrix3x4_t mScale;
		//SetIdentityMatrix( mScale );
		//mScale[0][0] = 1;
		//mScale[1][1] = -1;
		//mScale[2][2] = 1;
		//ConcatTransforms( mScale, temp, temp2 );
		temp[1][0] = -temp[1][0];
		temp[1][1] = -temp[1][1];
		temp[1][2] = -temp[1][2];
		temp[1][3] = -temp[1][3];

		// Transform back out of view space.
		ConcatTransforms( viewMatrixInverse, temp, transform );
	}
}

//-----------------------------------------------------------------------------
// Purpose: check if weapon viewmodel should be drawn
//-----------------------------------------------------------------------------
bool C_BaseViewModel::ShouldDraw()
{
	if ( engine->IsHLTV() )
	{
		return ( HLTVCamera()->GetMode() == OBS_MODE_IN_EYE &&
				 HLTVCamera()->GetPrimaryTarget() == GetOwner()	);
	}
#if defined( REPLAY_ENABLED )
	else if ( g_pEngineClientReplay->IsPlayingReplayDemo() )
	{
		return ( ReplayCamera()->GetMode() == OBS_MODE_IN_EYE &&
				 ReplayCamera()->GetPrimaryTarget() == GetOwner() );
	}
#endif
	else
	{
		return BaseClass::ShouldDraw();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the weapon. Draw the Viewmodel if the weapon's being carried
//			by this player, otherwise draw the worldmodel.
//-----------------------------------------------------------------------------
int C_BaseViewModel::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	if ( flags & STUDIO_RENDER )
	{
		// Determine blending amount and tell engine
		float blend = (float)( GetFxBlend() / 255.0f );

		// Totally gone
		if ( blend <= 0.0f )
			return 0;

		// Tell engine
		render->SetBlend( blend );

		float color[3];
		GetColorModulation( color );
		render->SetColorModulation(	color );
	}
		
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	int ret;
	// If the local player's overriding the viewmodel rendering, let him do it
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		ret = pPlayer->DrawOverriddenViewmodel( this, flags );
	}
	else if ( pWeapon && pWeapon->IsOverridingViewmodel() )
	{
		ret = pWeapon->DrawOverriddenViewmodel( this, flags );
	}
	else
	{
		ret = BaseClass::DrawModel( flags );
	}

	// Now that we've rendered, reset the animation restart flag
	if ( flags & STUDIO_RENDER )
	{
		if ( m_nOldAnimationParity != m_nAnimationParity )
		{
			m_nOldAnimationParity = m_nAnimationParity;
		}
		// Tell the weapon itself that we've rendered, in case it wants to do something
		if ( pWeapon )
		{
			pWeapon->ViewModelDrawn( this );
		}
	}

#ifdef TF_CLIENT_DLL
	CTFWeaponBase* pTFWeapon = dynamic_cast<CTFWeaponBase*>( pWeapon );
	if ( ( flags & STUDIO_RENDER ) && pTFWeapon && pTFWeapon->m_viewmodelStatTrakAddon )
	{
		pTFWeapon->m_viewmodelStatTrakAddon->RemoveEffects( EF_NODRAW );
		pTFWeapon->m_viewmodelStatTrakAddon->DrawModel( flags );
		pTFWeapon->m_viewmodelStatTrakAddon->AddEffects( EF_NODRAW );
	}
#endif

	return ret;
}

void CBaseViewModel::CreateHandsModel()
{
	if (!m_pHands)
	{
		C_ViewHands* pHands = (C_ViewHands*)CreateEntityByName("viewhands");
		pHands->InitializeAsClientEntityByIndex(m_iHandsModelIndex.Get(), RENDER_GROUP_VIEW_MODEL_OPAQUE);
		pHands->SetOwnerEntity(this);
		pHands->m_pViewModel = this;
		pHands->FollowEntity(this, true);
		m_pHands = pHands;
	}
}

CStudioHdr* C_BaseViewModel::OnNewModel(void)
{
	CStudioHdr* hdr = BaseClass::OnNewModel();

	if (hdr && m_iHandsModelIndex.Get() > -1)
	{
		CreateHandsModel();

		/*if (LookupBone("ValveBiped.Bip01_R_Hand") < 0)
		{
			if (!m_pCSGO)
			{
				m_pCSGO = (C_ViewHands*)CreateEntityByName("viewhands");
				m_pCSGO->InitializeAsClientEntity("models/weapons/tfa_csgo/c_hands_translator.mdl", RENDER_GROUP_OTHER);
				m_pCSGO->SetOwnerEntity(this);
				m_pCSGO->m_pViewModel = this;
				m_pCSGO->FollowEntity(this, true);
			}

			m_pHands->FollowEntity(m_pCSGO, true);
		}
		else*/
		{
			m_pHands->FollowEntity(this, true);

			/*if (m_pCSGO)
			{
				m_pCSGO->SUB_Remove();
				m_pCSGO = nullptr;
			}*/
		}
	}
	else
	{
		if (m_pHands)
		{
			m_pHands->SUB_Remove();
			m_pHands = nullptr;
		}

		/*if (m_pCSGO)
		{
			m_pCSGO->SUB_Remove();
			m_pCSGO = nullptr;
		}*/
	}

	return hdr;
}

int C_BaseViewModel::GetHandModelData(int& iSkin, int& iBody)
{
	if (GetWeapon())
	{
		CBaseCombatWeapon* pWeap = GetWeapon();
		if (!pWeap->GetWpnData().bViewModelUseArms)
			return -1;
	}

	iSkin = m_iHandsSkin.Get();
	iBody = m_iHandsBody.Get();

	return m_iHandsModelIndex.Get();
}

C_BaseAnimating* C_BaseViewModel::GetHandsModel()
{
	return m_pHands;
}

float C_BaseViewModel::GetWetness()
{
	C_BasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
		return pOwner->GetWetness();

	return BaseClass::GetWetness();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_BaseViewModel::InternalDrawModel( int flags )
{
	CMatRenderContextPtr pRenderContext( materials );
	if ( ShouldFlipViewModel() )
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );

	int ret = BaseClass::InternalDrawModel( flags );

	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );

	// Now draw the arms model
	//if (ret && m_pHands != nullptr)
	//	m_pHands->DrawHands(this, flags);

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Called by the player when the player's overriding the viewmodel drawing. Avoids infinite recursion.
//-----------------------------------------------------------------------------
int C_BaseViewModel::DrawOverriddenViewmodel( int flags )
{
	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int C_BaseViewModel::GetFxBlend( void )
{
	// See if the local player wants to override the viewmodel's rendering
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		pPlayer->ComputeFxBlend();
		return pPlayer->GetFxBlend();
	}

	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->IsOverridingViewmodel() )
	{
		pWeapon->ComputeFxBlend();
		return pWeapon->GetFxBlend();
	}

	return BaseClass::GetFxBlend();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseViewModel::IsTransparent( void )
{
	// See if the local player wants to override the viewmodel's rendering
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		return pPlayer->ViewModel_IsTransparent();
	}

	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->IsOverridingViewmodel() )
		return pWeapon->ViewModel_IsTransparent();

	return BaseClass::IsTransparent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_BaseViewModel::UsesPowerOfTwoFrameBufferTexture( void )
{
	// See if the local player wants to override the viewmodel's rendering
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsOverridingViewmodel() )
	{
		return pPlayer->ViewModel_IsUsingFBTexture();
	}

	C_BaseCombatWeapon *pWeapon = GetOwningWeapon();
	if ( pWeapon && pWeapon->IsOverridingViewmodel() )
	{
		return pWeapon->ViewModel_IsUsingFBTexture();
	}

	return BaseClass::UsesPowerOfTwoFrameBufferTexture();
}

//-----------------------------------------------------------------------------
// Purpose: If the animation parity of the weapon has changed, we reset cycle to avoid popping
//-----------------------------------------------------------------------------
void C_BaseViewModel::UpdateAnimationParity( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	
	// If we're predicting, then we don't use animation parity because we change the animations on the clientside
	// while predicting. When not predicting, only the server changes the animations, so a parity mismatch
	// tells us if we need to reset the animation.
	if ( m_nOldAnimationParity != m_nAnimationParity && !GetPredictable() )
	{
		float curtime = (pPlayer && IsIntermediateDataAllocated()) ? pPlayer->GetFinalPredictedTime() : gpGlobals->curtime;
		// FIXME: this is bad
		// Simulate a networked m_flAnimTime and m_flCycle
		// FIXME:  Do we need the magic 0.1?
		SetCycle( 0.0f ); // GetSequenceCycleRate( GetSequence() ) * 0.1;
		m_flAnimTime = curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update global map state based on data received
// Input  : bnewentity - 
//-----------------------------------------------------------------------------
void C_BaseViewModel::OnDataChanged( DataUpdateType_t updateType )
{
	SetPredictionEligible( true );
	BaseClass::OnDataChanged(updateType);

	if (GetModelPtr())
	{
		int iSkin, iBody;
		int iIndex = GetHandModelData(iSkin, iBody);

		if (iIndex != -1)
		{
			CreateHandsModel();

			m_pHands->SetModelByIndex(iIndex);
			m_pHands->m_nBody = iBody;
			m_pHands->m_nSkin = iSkin;

			m_pHands->FollowEntity(this, true);
		}
	}
}

void C_BaseViewModel::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate(updateType);
	OnLatchInterpolatedVariables( LATCH_ANIMATION_VAR );
}


//-----------------------------------------------------------------------------
// Purpose: Add entity to visible view models list
//-----------------------------------------------------------------------------
void C_BaseViewModel::AddEntity( void )
{
	// Server says don't interpolate this frame, so set previous info to new info.
	if ( IsNoInterpolationFrame() )
	{
		ResetLatched();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseViewModel::GetBoneControllers(float controllers[MAXSTUDIOBONECTRLS])
{
	BaseClass::GetBoneControllers( controllers );

	// Tell the weapon itself that we've rendered, in case it wants to do something
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->GetViewmodelBoneControllers( this, controllers );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : RenderGroup_t
//-----------------------------------------------------------------------------
RenderGroup_t C_BaseViewModel::GetRenderGroup()
{
	return RENDER_GROUP_VIEW_MODEL_OPAQUE;
}

