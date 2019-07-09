//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "fx.h"
#include "c_te_effect_dispatch.h"
#include "basecombatweapon_shared.h"
#include "baseviewmodel_shared.h"
#include "particles_new.h"
#include "vprof.h"
#include "clientsideeffects.h"
#include "view.h"
#include "model_types.h"
#include "studio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar r_drawtracers( "r_drawtracers", "1", FCVAR_CHEAT );
ConVar r_drawtracers_firstperson( "r_drawtracers_firstperson", "1", FCVAR_ARCHIVE, "Toggle visibility of first person weapon tracers" );
ConVar r_tracermodels("r_tracermodels", "1", FCVAR_ARCHIVE);

#define	TRACER_SPEED			5000 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector GetTracerOrigin( const CEffectData &data )
{
	Vector vecStart = data.m_vStart;
	QAngle vecAngles;

	int iAttachment = data.m_nAttachmentIndex;

	// Attachment?
	if ( data.m_fFlags & TRACER_FLAG_USEATTACHMENT )
	{
		// If the entity specified is a weapon being carried by this player, use the viewmodel instead
		IClientRenderable *pRenderable = data.GetRenderable();
		if ( !pRenderable )
			return vecStart;

		C_BaseEntity *pEnt = data.GetEntity();

// This check should probably be for all multiplayer games, investigate later
#if defined( HL2MP ) || defined( TF_CLIENT_DLL ) || defined( TF_CLASSIC_CLIENT ) || defined(HL2_LAZUL)
		if ( pEnt && pEnt->IsDormant() )
			return vecStart;
#endif

		C_BaseCombatWeapon *pWpn = dynamic_cast<C_BaseCombatWeapon *>( pEnt );
		if ( pWpn && pWpn->ShouldDrawUsingViewModel() )
		{
			C_BasePlayer *player = ToBasePlayer( pWpn->GetOwner() );

			// Use GetRenderedWeaponModel() instead?
			C_BaseViewModel *pViewModel = player ? player->GetViewModel( 0 ) : NULL;
			if ( pViewModel )
			{
				// Get the viewmodel and use it instead
				pRenderable = pViewModel;
			}
		}

		// Get the attachment origin
		if ( !pRenderable->GetAttachment( iAttachment, vecStart, vecAngles ) )
		{
			DevMsg( "GetTracerOrigin: Couldn't find attachment %d on model %s\n", iAttachment, 
				modelinfo->GetModelName( pRenderable->GetModel() ) );
		}
	}

	return vecStart;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TracerCallback( const CEffectData &data )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	if ( !r_drawtracers.GetBool() )
		return;

	if ( !r_drawtracers_firstperson.GetBool() )
	{
		C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer*>( data.GetEntity() );

		if ( pPlayer && !pPlayer->ShouldDrawThisPlayer() )
			return;
	}

	// Grab the data
	const Vector& vecStart = GetTracerOrigin( data );
	float flVelocity = data.m_flScale;
	const bool bWhiz = (data.m_fFlags & TRACER_FLAG_WHIZ);
	const int iEntIndex = data.entindex();

	if ( iEntIndex && iEntIndex == player->index )
	{
		Vector	foo = data.m_vStart;
		QAngle	vangles;
		Vector	vforward, vright, vup;

		engine->GetViewAngles( vangles );
		AngleVectors( vangles, &vforward, &vright, &vup );

		VectorMA( data.m_vStart, 4, vright, foo );
		foo[2] -= 0.5f;

		FX_PlayerTracer( foo, data.m_vOrigin );
		return;
	}
	
	// Use default velocity if none specified
	if ( !flVelocity )
	{
		flVelocity = TRACER_SPEED;
	}

	// Do tracer effect
	if (!r_tracermodels.GetBool())
		FX_Tracer( vecStart, data.m_vOrigin, flVelocity, bWhiz );
	else
	{
		FX_AddTracerModel(vecStart, data.m_vOrigin, flVelocity);
		if (bWhiz)
			FX_TracerSound(vecStart, data.m_vOrigin, TRACER_TYPE_DEFAULT);
	}
}

DECLARE_CLIENT_EFFECT( "Tracer", TracerCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ParticleTracerCallback( const CEffectData &data )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	if ( !r_drawtracers.GetBool() )
		return;

	if ( !r_drawtracers_firstperson.GetBool() )
	{
		C_BasePlayer *pPlayer = dynamic_cast<C_BasePlayer*>( data.GetEntity() );

		if ( pPlayer && !pPlayer->ShouldDrawThisPlayer() )
			return;
	}

	// Grab the data
	Vector vecStart = GetTracerOrigin( data );
	const Vector& vecEnd = data.m_vOrigin;

	// Adjust view model tracers
	C_BaseEntity *pEntity = data.GetEntity();
	if ( data.entindex() && data.entindex() == player->index )
	{
		QAngle	vangles;
		Vector	vforward, vright, vup;

		engine->GetViewAngles( vangles );
		AngleVectors( vangles, &vforward, &vright, &vup );

		VectorMA( data.m_vStart, 4, vright, vecStart );
		vecStart[2] -= 0.5f;
	}

	// Create the particle effect
	QAngle vecAngles;
	Vector vecToEnd = vecEnd - vecStart;
	VectorNormalize(vecToEnd);
	VectorAngles( vecToEnd, vecAngles );
	DispatchParticleEffect( data.m_nHitBox, vecStart, vecEnd, vecAngles, pEntity );

	if ( data.m_fFlags & TRACER_FLAG_WHIZ )
	{
		FX_TracerSound( vecStart, vecEnd, TRACER_TYPE_DEFAULT );	
	}
}

DECLARE_CLIENT_EFFECT( "ParticleTracer", ParticleTracerCallback );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TracerSoundCallback( const CEffectData &data )
{
	// Do tracer effect
	FX_TracerSound( GetTracerOrigin( data ), data.m_vOrigin, data.m_fFlags );
}

DECLARE_CLIENT_EFFECT( "TracerSound", TracerSoundCallback );

// Model Tracers
#define TRACER_MODEL_LENGTH 24.2f
#define TRACER_MODEL_TIP_OFFSET 10.0f
class CFXTracerModel : public C_BaseAnimating
{
	DECLARE_CLASS(CFXTracerModel, C_BaseAnimating);

public:
	void			Init(const Vector& start, const Vector& end, float scale, float life, int iSkin, bool bCombineFX);

	virtual const Vector &			GetRenderOrigin(void);
	virtual const QAngle &			GetRenderAngles(void)
	{
		static QAngle angRot;
		VectorAngles(m_vecDirection, angRot);
		return angRot;
	}
	virtual bool					ShouldDraw(void)
	{
		return r_drawtracers.GetBool() && BaseClass::ShouldDraw();
	}

	// Should this object cast shadows?
	virtual ShadowType_t	ShadowCastType() { return SHADOWS_NONE; }

	// Should this object receive shadows?
	virtual bool			ShouldReceiveProjectedTextures(int flags)
	{
		return false;
	}

	virtual int DrawModel(int flags);
	virtual void					ClientThink();

private:
	float			m_fLife;
	Vector			m_vecOrigin, m_vecEnd, m_vecDirection;
	float			m_fStartTime;
	float			m_fScale;
	VPlane			m_ClipPlanes[2];
	bool			m_bCombineTracer;
};

void CFXTracerModel::Init(const Vector& start, const Vector& end, float scale, float life, int iSkin, bool bCombineFX)
{
	m_vecOrigin = start;
	m_vecEnd = end;
	m_vecDirection = (end - start).Normalized();
	m_fScale = scale;
	m_fLife = life;
	m_fStartTime = gpGlobals->curtime;
	m_nSkin = iSkin;
	m_bCombineTracer = bCombineFX;

	InitializeAsClientEntity("models/ryu-gi/effect_props/incendiary/bullet_tracer.mdl", RENDER_GROUP_TRANSLUCENT_ENTITY);

	SetAbsOrigin(start);
	SetAbsAngles(GetRenderAngles());

	{
		Vector normal = m_vecDirection;
		float flDist = DotProduct(normal, start);
		m_ClipPlanes[0].Init(normal, flDist);
	}
	
	{
		Vector normal = -m_vecDirection;
		float flDist = DotProduct(normal, end);
		m_ClipPlanes[1].Init(normal, flDist);
	}

	SetModelScale(scale);

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

const Vector & CFXTracerModel::GetRenderOrigin(void)
{
	float fT = (gpGlobals->curtime - m_fStartTime) / m_fLife;
	fT = Min(fT, 1.0f);

	static Vector vecRet;
	vecRet = VecLerp(m_vecOrigin, m_vecEnd + m_vecDirection*TRACER_MODEL_LENGTH, fT) - (m_vecDirection*TRACER_MODEL_TIP_OFFSET*GetModelScale());
	return vecRet;
}

int CFXTracerModel::DrawModel(int flags)
{
	VPROF_BUDGET("CFXTracerModel::DrawModel", VPROF_BUDGETGROUP_MODEL_RENDERING);
	if (!m_bReadyToDraw)
		return 0;

	int drawn = 0;

#if defined( TF_CLIENT_DLL ) || defined ( TF_CLASSIC_CLIENT )
	ValidateModelIndex();
#endif

	int iOriginalSkin = GetSkin();

	if (r_drawtracers.GetInt())
	{
		MDLCACHE_CRITICAL_SECTION();

		if (flags & (STUDIO_SHADOWDEPTHTEXTURE| STUDIO_SSAODEPTHTEXTURE))
		{
			return 0;
		}

		// Necessary for lighting blending
		//CreateModelInstance();

		CMatRenderContextPtr pRenderContext(materials);
		if (!materials->UsingFastClipping()) //do NOT change the fast clip plane mid-scene, depth problems result. Regular user clip planes are fine though
		{
			pRenderContext->PushCustomClipPlane(m_ClipPlanes[1].m_Normal.Base());
			pRenderContext->PushCustomClipPlane(m_ClipPlanes[0].m_Normal.Base());
		}

		drawn = InternalDrawModel(flags);

		if (drawn && m_bCombineTracer)
		{
			if (iOriginalSkin == 0)
				m_nSkin = 8;
			else
				m_nSkin += 7;

			SetModelScale(m_fScale * 1.4f);

			drawn = InternalDrawModel(flags);

			m_nSkin = iOriginalSkin;
			SetModelScale(m_fScale);
		}

		if (!materials->UsingFastClipping())
		{
			pRenderContext->PopCustomClipPlane();
			pRenderContext->PopCustomClipPlane();
		}
	}

	// If we're visualizing our bboxes, draw them
	DrawBBoxVisualizations();

	return drawn;
}

void CFXTracerModel::ClientThink()
{
	if (gpGlobals->curtime > m_fStartTime + m_fLife)
	{
		//SetNextClientThink(CLIENT_THINK_NEVER);
		Remove();
		return;
	}

	Assert(!GetMoveParent());

	SetLocalOrigin(GetRenderOrigin());
}


void FX_AddTracerModel(const Vector & start, const Vector & end, int velocity, int iSkin, bool bCombineFX, float flScale)
{
	VPROF_BUDGET("FX_AddTracerModel", VPROF_BUDGETGROUP_PARTICLE_RENDERING);
	//Don't make small tracers
	float dist;
	Vector dir;

	VectorSubtract(end, start, dir);
	dist = VectorNormalize(dir);

	// Don't make short tracers.
	if (dist >= TRACER_MODEL_LENGTH*0.5f)
	{
		float length = TRACER_MODEL_LENGTH;
		float life = (dist + length) / velocity;	//NOTENOTE: We want the tail to finish its run as well

		//Add it
		//FX_AddDiscreetLine(start, dir, velocity, length, dist, random->RandomFloat(0.75f, 0.9f), life, "effects/spark");
		CFXTracerModel	*t = new CFXTracerModel;
		Assert(t);

		t->Init(start, end, flScale, life, iSkin, bCombineFX);
	}
}