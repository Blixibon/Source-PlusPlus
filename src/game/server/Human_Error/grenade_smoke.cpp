//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "npc_citizen17.h"
#include "Human_Error/grenade_smoke.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "particle_parse.h"
#include "particle_system.h"
#include "soundent.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SMOKE_GRENADE_BLIP_FREQUENCY			1.0f
#define SMOKE_GRENADE_BLIP_FAST_FREQUENCY	0.3f

#define SMOKE_GRENADE_GRACE_TIME_AFTER_PICKUP 0.5f
#define SMOKE_GRENADE_WARN_TIME 1.0f

const float SMOKE_GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

ConVar smoke_grenade_radius	( "sk_smoke_grenade_radius", "256");
ConVar smoke_grenade_damage( "sk_smoke_grenade_damage", "5" );

#define SMOKE_GRENADE_MODEL "models/Weapons/w_grenade.mdl"
#define SMOKE_GRENADE_DETONATE_SOUND "HeadcrabCanister.AfterLanding"	//ambient.steam01 or HeadcrabCanister.AfterLanding
#define SMOKE_GRENADE_BREAK_SOUND "Padlock.Break"

LINK_ENTITY_TO_CLASS( npc_grenade_smoke, CGrenadeSmoke );

BEGIN_DATADESC(CGrenadeSmoke)
DEFINE_FIELD(m_bStartSmoke, FIELD_BOOLEAN),

// Function Pointers
DEFINE_THINKFUNC(SmokeThink),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CGrenadeSmoke, DT_GrenadeSmoke)
SendPropBool(SENDINFO(m_bStartSmoke)),
END_SEND_TABLE();


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrenadeSmoke::~CGrenadeSmoke( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrenadeSmoke::CreateEffects(void)
{
	// Start up the eye glow
	m_pMainGlow = CSprite::SpriteCreate("sprites/blueglow1.vmt", GetLocalOrigin(), false);

	int	nAttachment = LookupAttachment("fuse");

	if (m_pMainGlow != NULL)
	{
		m_pMainGlow->FollowEntity(this);
		m_pMainGlow->SetAttachment(this, nAttachment);
		m_pMainGlow->SetTransparency(kRenderGlow, 0, 0, 255, 200, kRenderFxNoDissipation);
		m_pMainGlow->SetScale(0.2f);
		m_pMainGlow->SetGlowProxySize(4.0f);
	}

	// Start up the eye trail
	m_pGlowTrail = CSpriteTrail::SpriteTrailCreate("sprites/bluelaser1.vmt", GetLocalOrigin(), false);

	if (m_pGlowTrail != NULL)
	{
		m_pGlowTrail->FollowEntity(this);
		m_pGlowTrail->SetAttachment(this, nAttachment);
		m_pGlowTrail->SetTransparency(kRenderTransAdd, 0, 0, 255, 255, kRenderFxNone);
		m_pGlowTrail->SetStartWidth(8.0f);
		m_pGlowTrail->SetEndWidth(1.0f);
		m_pGlowTrail->SetLifeTime(0.5f);
	}
}

void CGrenadeSmoke::BlipSound()
{
	EmitSound("Grenade.Blip");

	CPVSFilter filter(GetAbsOrigin());
	//te->DynamicLight(filter, 0.f, &WorldSpaceCenter(), 255, 0, 0, 5, 64.f, 0.1f, 128.f);

	/*CEffectData data;
	data.m_flRadius = 64.f;
	data.m_flScale = 0.1f;
	data.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = LookupAttachment("fuse");
	ColorRGBExp32 color;
	color.r = 0;
	color.g = 0;
	color.b = 255;
	color.exponent = 8;
	ColorRGBExp32ToVector(color, data.m_CustomColors.m_vecColor1);

	DispatchEffect("CreateFollowLight", data, filter);*/

	te->DynamicLight(filter, 0.f, entindex(), LookupAttachment("fuse"), 0, 0, 255, 5, 64.f, 0.1f, 128.f);
}

void CGrenadeSmoke::Precache(void)
{
	BaseClass::Precache();

	PrecacheScriptSound(SMOKE_GRENADE_DETONATE_SOUND);
	PrecacheScriptSound(SMOKE_GRENADE_BREAK_SOUND);
}

void CGrenadeSmoke::Detonate()
{
	m_flDetonateTime = gpGlobals->curtime + 5.0f;

	/*Vector vecAbsOrigin = GetAbsOrigin();
	CPVSFilter filter( vecAbsOrigin );
	te->Smoke( filter, 0.0, 
		&vecAbsOrigin, g_sModelIndexSmoke,
		m_DmgRadius * 0.03,
		24 );*/

	m_bStartSmoke = true;

	EmitSound( SMOKE_GRENADE_DETONATE_SOUND );

	SetThink( &CGrenadeSmoke::SmokeThink );
	SetNextThink( gpGlobals->curtime );
}

void CGrenadeSmoke::SmokeThink()
{
	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		StopSound( SMOKE_GRENADE_DETONATE_SOUND );
		EmitSound( SMOKE_GRENADE_BREAK_SOUND );
		UTIL_Remove( this );
		return;
	}

	CBaseEntity*	pList[100];
	Vector			delta( smoke_grenade_radius.GetInt(), smoke_grenade_radius.GetInt(), smoke_grenade_radius.GetInt() );

	int count = UTIL_EntitiesInBox(pList, 100, GetAbsOrigin() - delta, GetAbsOrigin() + delta, FL_NPC | FL_CLIENT);
	
	// If the bugbait's been thrown, look for nearby targets to affect

	for ( int i = 0; i < count; i++ )
	{
		// If close enough, make citizens freak out when hit
		if ( UTIL_DistApprox( pList[i]->WorldSpaceCenter(), GetAbsOrigin() ) < smoke_grenade_radius.GetInt() )
		{
			//if ( pList[i]->IsNPC() )
			{
				pList[i]->TakeDamage(CTakeDamageInfo(this, GetThrower(), smoke_grenade_damage.GetInt(), DMG_NERVEGAS));
			}

			// Must be a citizen
			/*if ( FClassnameIs( pList[i], "npc_citizen") )
			{
				CNPC_Citizen *pCitizen = assert_cast<CNPC_Citizen *>(pList[i]);

				if ( pCitizen != NULL )
				{
					pCitizen->RunAwayFromSmoke( this );
				}
			} */
		}
	}

	//TERO: damage time

	//CheckTraceHullAttack(0, -delta, delta, smoke_grenade_damage.GetInt(), DMG_POISON, 0.0f, false );

	SetNextThink( gpGlobals->curtime + 0.5f );
}

CBaseGrenade *SmokeGrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer)
{
	return Fraggrenade_Create(position, angles, velocity, angVelocity, pOwner, timer, false, "npc_grenade_smoke");
}
