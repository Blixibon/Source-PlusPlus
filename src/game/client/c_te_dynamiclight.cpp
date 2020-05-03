//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "c_basetempentity.h"
#include "dlight.h"
#include "iefx.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"
#include "tier0/vprof.h"
#include "peter/c_lightmanager.h"

#ifdef DEFERRED
#include "deferred\deferred_shared_common.h"
#endif // DEFERRED


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dynamic Light
//-----------------------------------------------------------------------------
class C_TEDynamicLight : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEDynamicLight, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEDynamicLight( void );
	virtual			~C_TEDynamicLight( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:
	Vector			m_vecOrigin;
	float			m_fRadius;
	int				r;
	int				g;
	int				b;
	int				exponent;
	float			m_fTime;
	float			m_fDecay;
};


//-----------------------------------------------------------------------------
// Networking 
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEDynamicLight, DT_TEDynamicLight, CTEDynamicLight)
	RecvPropVector( RECVINFO(m_vecOrigin)),
	RecvPropInt( RECVINFO(r)),
	RecvPropInt( RECVINFO(g)),
	RecvPropInt( RECVINFO(b)),
	RecvPropInt( RECVINFO(exponent)),
	RecvPropFloat( RECVINFO(m_fRadius)),
	RecvPropFloat( RECVINFO(m_fTime)),
	RecvPropFloat( RECVINFO(m_fDecay)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEDynamicLight::C_TEDynamicLight( void )
{
	m_vecOrigin.Init();
	r = 0;
	g = 0;
	b = 0;
	exponent = 0;
	m_fRadius = 0.0;
	m_fTime = 0.0;
	m_fDecay = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEDynamicLight::~C_TEDynamicLight( void )
{
}

void TE_DynamicLight( IRecipientFilter& filter, float delay,
	const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex )
{
#ifdef DEFERRED
	if (!GetLightingManager())
		return;

	def_light_temp_t *dl = new def_light_temp_t(time);
	if (!dl)
		return;

	dl->iLighttype = DEFLIGHTTYPE_POINT;
	dl->iFlags = (DEFLIGHT_COOKIE_ENABLED/*|DEFLIGHT_SHADOW_ENABLED|DEFLIGHT_VOLUMETRICS_ENABLED*/);
	dl->pos = *org;
	dl->flRadius = radius;
	dl->col_diffuse.x = TexLightToLinear(r, exponent);
	dl->col_diffuse.y = TexLightToLinear(g, exponent);
	dl->col_diffuse.z = TexLightToLinear(b, exponent);
	dl->flFalloffPower = exponent;
	//dl->decay = decay;

	GetLightingManager()->AddTempLight(dl);
#else
	dlight_t *dl = effects->CL_AllocDlight( nLightIndex );
	if ( !dl )
		return;

	dl->origin	= *org;
	dl->radius	= radius;
	dl->color.r	= r;
	dl->color.g	= g;
	dl->color.b	= b;
	dl->color.exponent	= exponent;
	dl->die		= gpGlobals->curtime + time;
	dl->decay	= decay;
#endif
	if ( ToolsEnabled() && clienttools->IsInRecordingMode() )
	{
		Color clr( r, g, b, 255 );

		KeyValues *msg = new KeyValues( "TempEntity" );

 		msg->SetInt( "te", TE_DYNAMIC_LIGHT );
 		msg->SetString( "name", "TE_DynamicLight" );
		msg->SetFloat( "time", gpGlobals->curtime );
		msg->SetFloat( "duration", time );
		msg->SetFloat( "originx", org->x );
		msg->SetFloat( "originy", org->y );
		msg->SetFloat( "originz", org->z );
		msg->SetFloat( "radius", radius );
		msg->SetFloat( "decay", decay );
		msg->SetColor( "color", clr );
 		msg->SetInt( "exponent", exponent );
 		msg->SetInt( "lightindex", nLightIndex );
		msg->SetBool("follow", false);

		ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEDynamicLight::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TEDynamicLight::PostDataUpdate" );

	CBroadcastRecipientFilter filter;
	TE_DynamicLight( filter, 0.0f, &m_vecOrigin, r, g, b, exponent, m_fRadius, m_fTime, m_fDecay, LIGHT_INDEX_TE_DYNAMIC );
}

//-----------------------------------------------------------------------------
// Purpose: Dynamic Light
//-----------------------------------------------------------------------------
class C_TEDynamicLightFollow : public C_BaseTempEntity
{
public:
	DECLARE_CLASS(C_TEDynamicLightFollow, C_BaseTempEntity);
	DECLARE_CLIENTCLASS();

	C_TEDynamicLightFollow(void);
	virtual			~C_TEDynamicLightFollow(void);

	virtual void	PostDataUpdate(DataUpdateType_t updateType);

public:
	int				m_nEntIndex;
	int				m_nAttachmentIndex;
	float			m_fRadius;
	int				r;
	int				g;
	int				b;
	int				exponent;
	float			m_fTime;
	float			m_fDecay;
};


//-----------------------------------------------------------------------------
// Networking 
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_EVENT_DT(C_TEDynamicLightFollow, DT_TEDynamicLightFollow, CTEDynamicLightFollow)
RecvPropInt(RECVINFO(m_nEntIndex)),
RecvPropInt(RECVINFO(m_nAttachmentIndex)),
RecvPropInt(RECVINFO(r)),
RecvPropInt(RECVINFO(g)),
RecvPropInt(RECVINFO(b)),
RecvPropInt(RECVINFO(exponent)),
RecvPropFloat(RECVINFO(m_fRadius)),
RecvPropFloat(RECVINFO(m_fTime)),
RecvPropFloat(RECVINFO(m_fDecay)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEDynamicLightFollow::C_TEDynamicLightFollow(void)
{
	m_nEntIndex = 0;
	m_nAttachmentIndex = 0;
	r = 0;
	g = 0;
	b = 0;
	exponent = 0;
	m_fRadius = 0.0;
	m_fTime = 0.0;
	m_fDecay = 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEDynamicLightFollow::~C_TEDynamicLightFollow(void)
{
}

void TE_DynamicLightFollow(IRecipientFilter& filter, float delay,
	int iEntIndex, int iAttachmentIndex, int r, int g, int b, int exponent, float radius, float time, float decay)
{
	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity(iEntIndex);
	LightManager()->CreateAutoFollowLight(pEnt, iAttachmentIndex, r, g, b, exponent, radius, time, decay);

	if (ToolsEnabled() && pEnt && clienttools->IsInRecordingMode())
	{
		Color clr(r, g, b, 255);

		KeyValues* msg = new KeyValues("TempEntity");

		msg->SetInt("te", TE_DYNAMIC_LIGHT);
		msg->SetString("name", "TE_DynamicLightFollow");
		msg->SetFloat("time", gpGlobals->curtime);
		msg->SetFloat("duration", time);
		msg->SetFloat("radius", radius);
		msg->SetFloat("decay", decay);
		msg->SetColor("color", clr);
		msg->SetInt("exponent", exponent);
		msg->SetInt("entindex", iEntIndex);
		msg->SetInt("attachment", iAttachmentIndex);
		msg->SetBool("follow", true);

		ToolFramework_PostToolMessage(HTOOLHANDLE_INVALID, msg);
		msg->deleteThis();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEDynamicLightFollow::PostDataUpdate(DataUpdateType_t updateType)
{
	VPROF("C_TEDynamicLight::PostDataUpdate");

	CBroadcastRecipientFilter filter;
	TE_DynamicLightFollow(filter, 0.0f, m_nEntIndex, m_nAttachmentIndex, r, g, b, exponent, m_fRadius, m_fTime, m_fDecay);
}

void TE_DynamicLight( IRecipientFilter& filter, float delay, KeyValues *pKeyValues )
{
	Vector vecOrigin;
	vecOrigin.x = pKeyValues->GetFloat( "originx" );
	vecOrigin.y = pKeyValues->GetFloat( "originy" );
	vecOrigin.z = pKeyValues->GetFloat( "originz" );
	float flDuration = pKeyValues->GetFloat( "duration" );
	Color c = pKeyValues->GetColor( "color" );
	int nExponent = pKeyValues->GetInt( "exponent" );
	float flRadius = pKeyValues->GetFloat( "radius" );
	float flDecay = pKeyValues->GetFloat( "decay" );
 	int nLightIndex = pKeyValues->GetInt( "lightindex", LIGHT_INDEX_TE_DYNAMIC );
	bool bFollow = pKeyValues->GetBool("follow");
	int iEntIndex = pKeyValues->GetInt("entindex");
	int iAttachment = pKeyValues->GetInt("attachment");

	if (bFollow)
	{
		TE_DynamicLightFollow(filter, 0.0f, iEntIndex, iAttachment, c.r(), c.g(), c.b(), nExponent, flRadius, flDuration, flDecay);
	}
	else
	{
		TE_DynamicLight(filter, 0.0f, &vecOrigin, c.r(), c.g(), c.b(), nExponent,
			flRadius, flDuration, flDecay, nLightIndex);
	}
}

