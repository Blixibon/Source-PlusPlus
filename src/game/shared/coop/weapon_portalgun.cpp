//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_portalgun.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "rumble_shared.h"

#include "prop_portal_shared.h"
#include "weapon_physcannon.h"
#include "portal_shareddefs.h"
#include "portal_placement.h"
#ifdef CLIENT_DLL
#include "c_portal_player.h"
#include "c_te_effect_dispatch.h"
#include "iviewrender_beams.h"
#include "model_types.h"
#include "fx_interpvalue.h"
#include "ClientEffectPrecacheSystem.h"
#include "bone_setup.h"
#include "c_rumble.h"
#include "c_props.h"
#else
#include "BasePropDoor.h"
#include "portal_player.h"
#include "te_effect_dispatch.h"
#include "GameInterface.h"
#include "prop_combine_ball.h"
#include "physicsshadowclone.h"
#include "triggers.h"
#include "CollisionUtils.h"
#endif // CLIENT_DLL
#include "particle_parse.h"

#include "memdbgon.h" // Allways last

enum
{
	EFFECT_NONE,
	EFFECT_READY,
	EFFECT_HOLDING,
};

#ifdef CLIENT_DLL
#define	SPRITE_SCALE 128.0f

ConVar cl_portalgun_effects_min_alpha("cl_portalgun_effects_min_alpha", "96", FCVAR_CLIENTDLL);
ConVar cl_portalgun_effects_max_alpha("cl_portalgun_effects_max_alpha", "128", FCVAR_CLIENTDLL);

ConVar cl_portalgun_effects_min_size("cl_portalgun_effects_min_size", "3.0", FCVAR_CLIENTDLL);
ConVar cl_portalgun_beam_size("cl_portalgun_beam_size", "0.04", FCVAR_CLIENTDLL);

// HACK HACK! Used to make the gun visually change when going through a cleanser!
//ConVar cl_portalgun_effects_max_size ("cl_portalgun_effects_max_size", "2.5", FCVAR_REPLICATED );


//Precahce the effects
CLIENTEFFECT_REGISTER_BEGIN(PrecacheEffectPortalgun)
CLIENTEFFECT_MATERIAL(PORTALGUN_BEAM_SPRITE)
CLIENTEFFECT_MATERIAL(PORTALGUN_BEAM_SPRITE_NOZ)
CLIENTEFFECT_MATERIAL(PORTALGUN_GLOW_SPRITE)
CLIENTEFFECT_MATERIAL(PORTALGUN_ENDCAP_SPRITE)
CLIENTEFFECT_MATERIAL(PORTALGUN_GRAV_ACTIVE_GLOW)
CLIENTEFFECT_MATERIAL(PORTALGUN_PORTAL1_FIRED_LAST_GLOW)
CLIENTEFFECT_MATERIAL(PORTALGUN_PORTAL2_FIRED_LAST_GLOW)
CLIENTEFFECT_MATERIAL(PORTALGUN_PORTAL_MUZZLE_GLOW_SPRITE)
CLIENTEFFECT_MATERIAL(PORTALGUN_PORTAL_TUBE_BEAM_SPRITE)
CLIENTEFFECT_REGISTER_END()

CPortalgunEffectBeam::CPortalgunEffectBeam(void)
	: m_pBeam(NULL),
	m_fBrightness(255.0f)
{}

CPortalgunEffectBeam::~CPortalgunEffectBeam(void)
{
	Release();
}

void CPortalgunEffectBeam::Release(void)
{
	if (m_pBeam != NULL)
	{
		m_pBeam->flags = 0;
		m_pBeam->die = gpGlobals->curtime - 1;

		m_pBeam = NULL;
	}
}

void CPortalgunEffectBeam::Init(int startAttachment, int endAttachment, CBaseEntity* pEntity, bool firstPerson)
{
	if (m_pBeam != NULL)
		return;

	BeamInfo_t beamInfo;

	beamInfo.m_pStartEnt = pEntity;
	beamInfo.m_nStartAttachment = startAttachment;
	beamInfo.m_pEndEnt = pEntity;
	beamInfo.m_nEndAttachment = endAttachment;
	beamInfo.m_nType = TE_BEAMPOINTS;
	beamInfo.m_vecStart = vec3_origin;
	beamInfo.m_vecEnd = vec3_origin;

	beamInfo.m_pszModelName = (firstPerson) ? PORTALGUN_BEAM_SPRITE_NOZ : PORTALGUN_BEAM_SPRITE;

	beamInfo.m_flHaloScale = 0.0f;
	beamInfo.m_flLife = 0.0f;

	if (firstPerson)
	{
		beamInfo.m_flWidth = 0.0f;
		beamInfo.m_flEndWidth = 2.0f;
	}
	else
	{
		beamInfo.m_flWidth = 0.5f;
		beamInfo.m_flEndWidth = 2.0f;
	}

	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 16;
	beamInfo.m_flBrightness = 128.0;
	beamInfo.m_flSpeed = 150.0f;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 30.0;
	beamInfo.m_flRed = 255.0;
	beamInfo.m_flGreen = 255.0;
	beamInfo.m_flBlue = 255.0;
	beamInfo.m_nSegments = 8;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = FBEAM_FOREVER;

	m_pBeam = beams->CreateBeamEntPoint(beamInfo);

	if (m_pBeam)
	{
		m_pBeam->m_bDrawInMainRender = false;
		m_pBeam->m_bDrawInPortalRender = false;
	}
}

void CPortalgunEffectBeam::SetVisibleViewModel(bool visible /*= true*/)
{
	if (m_pBeam == NULL)
		return;

	m_pBeam->m_bDrawInMainRender = visible;
}

int CPortalgunEffectBeam::IsVisibleViewModel(void) const
{
	if (m_pBeam == NULL)
		return false;

	return m_pBeam->m_bDrawInMainRender;
}

void CPortalgunEffectBeam::SetVisible3rdPerson(bool visible /*= true*/)
{
	if (m_pBeam == NULL)
		return;

	m_pBeam->m_bDrawInPortalRender = visible;
}

int CPortalgunEffectBeam::SetVisible3rdPerson(void) const
{
	if (m_pBeam == NULL)
		return false;

	return m_pBeam->m_bDrawInPortalRender;
}

void CPortalgunEffectBeam::SetBrightness(float fBrightness)
{
	m_fBrightness = clamp(fBrightness, 0.0f, 255.0f);
}

void CPortalgunEffectBeam::DrawBeam(void)
{
	if (m_pBeam)
		m_pBeam->DrawModel(0);
}
#endif // CLIENT_DLL

#define BLAST_SPEED_NON_PLAYER 1000.0f
#define BLAST_SPEED 3000.0f


IMPLEMENT_NETWORKCLASS_ALIASED(WeaponPortalgun, DT_WeaponPortalgun)

BEGIN_NETWORK_TABLE(CWeaponPortalgun, DT_WeaponPortalgun)
#ifndef CLIENT_DLL
SendPropBool(SENDINFO(m_bCanFirePortal1)),
SendPropBool(SENDINFO(m_bCanFirePortal2)),
SendPropInt(SENDINFO(m_iLastFiredPortal)),
SendPropBool(SENDINFO(m_bOpenProngs)),
SendPropFloat(SENDINFO(m_fCanPlacePortal1OnThisSurface)),
SendPropFloat(SENDINFO(m_fCanPlacePortal2OnThisSurface)),
SendPropFloat(SENDINFO(m_fEffectsMaxSize1)), // HACK HACK! Used to make the gun visually change when going through a cleanser!
SendPropFloat(SENDINFO(m_fEffectsMaxSize2)),
SendPropInt(SENDINFO(m_EffectState)),
SendPropInt(SENDINFO(m_iPortalLinkageGroupID), -1, SPROP_UNSIGNED),
#else
RecvPropBool(RECVINFO(m_bCanFirePortal1)),
RecvPropBool(RECVINFO(m_bCanFirePortal2)),
RecvPropInt(RECVINFO(m_iLastFiredPortal)),
RecvPropBool(RECVINFO(m_bOpenProngs)),
RecvPropFloat(RECVINFO(m_fCanPlacePortal1OnThisSurface)),
RecvPropFloat(RECVINFO(m_fCanPlacePortal2OnThisSurface)),
RecvPropFloat(RECVINFO(m_fEffectsMaxSize1)), // HACK HACK! Used to make the gun visually change when going through a cleanser!
RecvPropFloat(RECVINFO(m_fEffectsMaxSize2)),
RecvPropInt(RECVINFO(m_EffectState)),
RecvPropInt(RECVINFO(m_iPortalLinkageGroupID), SPROP_UNSIGNED),
#endif // !CLIENT_DLL

END_NETWORK_TABLE()

BEGIN_DATADESC(CWeaponPortalgun)

DEFINE_KEYFIELD(m_bCanFirePortal1, FIELD_BOOLEAN, "CanFirePortal1"),
DEFINE_KEYFIELD(m_bCanFirePortal2, FIELD_BOOLEAN, "CanFirePortal2"),
DEFINE_FIELD(m_iLastFiredPortal, FIELD_INTEGER),
DEFINE_FIELD(m_bOpenProngs, FIELD_BOOLEAN),
DEFINE_FIELD(m_fCanPlacePortal1OnThisSurface, FIELD_FLOAT),
DEFINE_FIELD(m_fCanPlacePortal2OnThisSurface, FIELD_FLOAT),
DEFINE_FIELD(m_fEffectsMaxSize1, FIELD_FLOAT),
DEFINE_FIELD(m_fEffectsMaxSize2, FIELD_FLOAT),
DEFINE_FIELD(m_EffectState, FIELD_INTEGER),
DEFINE_FIELD(m_iPortalLinkageGroupID, FIELD_CHARACTER),
#ifndef CLIENT_DLL
DEFINE_INPUTFUNC(FIELD_VOID, "ChargePortal1", InputChargePortal1),
DEFINE_INPUTFUNC(FIELD_VOID, "ChargePortal2", InputChargePortal2),
DEFINE_INPUTFUNC(FIELD_VOID, "FirePortal1", InputFirePortal1),
DEFINE_INPUTFUNC(FIELD_VOID, "FirePortal2", InputFirePortal2),
DEFINE_INPUTFUNC(FIELD_VECTOR, "FirePortalDirection1", FirePortalDirection1),
DEFINE_INPUTFUNC(FIELD_VECTOR, "FirePortalDirection2", FirePortalDirection2),

DEFINE_OUTPUT(m_OnFiredPortal1, "OnFiredPortal1"),
DEFINE_OUTPUT(m_OnFiredPortal2, "OnFiredPortal2"),

DEFINE_THINKFUNC(Think),
#endif // !CLIENT_DLL

END_DATADESC()

BEGIN_PREDICTION_DATA(CWeaponPortalgun)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_bCanFirePortal1, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bCanFirePortal2, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_iLastFiredPortal, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bOpenProngs, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_fCanPlacePortal1OnThisSurface, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_fCanPlacePortal2OnThisSurface, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_EffectState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
#endif // CLIENT_DLL

END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_portalgun, CWeaponPortalgun);
PRECACHE_WEAPON_REGISTER(weapon_portalgun);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPortalgun::CWeaponPortalgun( void )
{
	m_bReloadsSingly = true;

	// TODO: specify these in hammer instead of assuming every gun has blue chip
	m_bCanFirePortal1 = true;
	m_bCanFirePortal2 = false;

	m_iLastFiredPortal = 0;
	m_fCanPlacePortal1OnThisSurface = 1.0f;
	m_fCanPlacePortal2OnThisSurface = 1.0f;

	m_fMinRange1	= 0.0f;
	m_fMaxRange1	= MAX_TRACE_LENGTH;
	m_fMinRange2	= 0.0f;
	m_fMaxRange2	= MAX_TRACE_LENGTH;

	m_EffectState	= EFFECT_NONE;
}

void CWeaponPortalgun::Precache()
{
	BaseClass::Precache();

	PrecacheModel( PORTALGUN_BEAM_SPRITE );
	PrecacheModel( PORTALGUN_BEAM_SPRITE_NOZ );

	PrecacheModel( "models/portals/portal1.mdl" );
	PrecacheModel( "models/portals/portal2.mdl" );

	PrecacheScriptSound( "Portal.ambient_loop" );

	PrecacheScriptSound( "Portal.open_blue" );
	PrecacheScriptSound( "Portal.open_red" );
	PrecacheScriptSound( "Portal.close_blue" );
	PrecacheScriptSound( "Portal.close_red" );
	PrecacheScriptSound( "Portal.fizzle_moved" );
	PrecacheScriptSound( "Portal.fizzle_invalid_surface" );
	PrecacheScriptSound( "Weapon_Portalgun.powerup" );
	PrecacheScriptSound( "Weapon_PhysCannon.HoldSound" );
	PrecacheScriptSound("PortalPlayer.ObjectUse");
	PrecacheScriptSound("PortalPlayer.ObjectUseStop");
	PrecacheScriptSound("PortalPlayer.UseDeny");

#ifndef CLIENT_DLL
	PrecacheParticleSystem( "portal_1_projectile_stream" );
	PrecacheParticleSystem( "portal_1_projectile_stream_pedestal" );
	PrecacheParticleSystem( "portal_2_projectile_stream" );
	PrecacheParticleSystem( "portal_2_projectile_stream_pedestal" );
	PrecacheParticleSystem("portal_x_projectile_stream");
	PrecacheParticleSystem("portal_x_projectile_stream_pedestal");
	PrecacheParticleSystem( "portal_1_charge" );
	PrecacheParticleSystem( "portal_2_charge" );
#endif
}

bool CWeaponPortalgun::ShouldDrawCrosshair( void )
{
	return true;//( m_fCanPlacePortal1OnThisSurface > 0.5f || m_fCanPlacePortal2OnThisSurface > 0.5f );
}

#ifndef CLIENT_DLL
extern ConVar sv_portal_placement_debug;
#endif // !CLIENT_DLL

extern ConVar sv_portal_placement_never_fail;

void CWeaponPortalgun::Spawn(void)
{
	//Precache();

	BaseClass::Spawn();

#ifndef CLIENT_DLL
	SetThink(&CWeaponPortalgun::Think);
	SetNextThink(gpGlobals->curtime + 0.1);

	if (GameRules()->IsMultiplayer())
	{
		CBaseEntity* pOwner = GetOwner();
		if (pOwner && pOwner->IsPlayer())
			m_iPortalLinkageGroupID = pOwner->entindex();

		Assert((m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256));
	}
#endif // !CLIENT_DLL
}

#ifndef CLIENT_DLL
void CWeaponPortalgun::Activate()
{
	BaseClass::Activate();

	CreateSounds();

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer && pPlayer->GetActiveWeapon() == this)
	{
		CBaseEntity* pHeldObject = GetPlayerHeldEntity(pPlayer);
		OpenProngs((pHeldObject) ? (false) : (true));
		OpenProngs((pHeldObject) ? (true) : (false));

		if (GameRules()->IsMultiplayer())
			m_iPortalLinkageGroupID = pPlayer->entindex();

		Assert((m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256));
	}

	// HACK HACK! Used to make the gun visually change when going through a cleanser!
	m_fEffectsMaxSize1 = 4.0f;
	m_fEffectsMaxSize2 = 4.0f;
}

void CWeaponPortalgun::OnPickedUp(CBaseCombatCharacter* pNewOwner)
{
	if (GameRules()->IsMultiplayer())
	{
		if (pNewOwner && pNewOwner->IsPlayer())
			m_iPortalLinkageGroupID = pNewOwner->entindex();

		Assert((m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256));
	}

	if (pNewOwner && pNewOwner->IsPlayer())
	{
		CBasePlayer* pOwner = ToBasePlayer(pNewOwner);
		if (CanFirePortal1())
		{
			IGameEvent* pEvent = gameeventmanager->CreateEvent("portal_enabled");
			if (pEvent)
			{
				pEvent->SetInt("userid", pOwner->GetUserID());
				pEvent->SetBool("leftportal", true);
				gameeventmanager->FireEvent(pEvent);
			}
		}
		if (CanFirePortal2())
		{
			IGameEvent* pEvent = gameeventmanager->CreateEvent("portal_enabled");
			if (pEvent)
			{
				pEvent->SetInt("userid", pOwner->GetUserID());
				pEvent->SetBool("leftportal", false);
				gameeventmanager->FireEvent(pEvent);
			}
		}
	}

	BaseClass::OnPickedUp(pNewOwner);
}

void CWeaponPortalgun::CreateSounds()
{
}

void CWeaponPortalgun::StopLoopingSounds()
{
	StopSound("PortalPlayer.ObjectUse");

	BaseClass::StopLoopingSounds();
}

//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the turret is currently in
//-----------------------------------------------------------------------------
bool CWeaponPortalgun::PreThink(void)
{
	//Animate
	StudioFrameAdvance();

	//Do not interrupt current think function
	return false;
}

void CWeaponPortalgun::Think(void)
{
	//Allow descended classes a chance to do something before the think function
	if (PreThink())
		return;

	SetNextThink(gpGlobals->curtime + 0.1f);

	CPortal_Player* pPlayer = ToPortalPlayer(GetOwner());

	if (!pPlayer || pPlayer->GetActiveWeapon() != this)
	{
		m_fCanPlacePortal1OnThisSurface = 1.0f;
		m_fCanPlacePortal2OnThisSurface = 1.0f;
		return;
	}

	return;

	// Test portal placement
	m_fCanPlacePortal1OnThisSurface = ((m_bCanFirePortal1) ? (FirePortal(false, 0, 1)) : (0.0f));
	m_fCanPlacePortal2OnThisSurface = ((m_bCanFirePortal2) ? (FirePortal(true, 0, 2)) : (0.0f));

	// Draw obtained portal color chips
	int iSlot1State = ((m_bCanFirePortal1) ? (0) : (1)); // FIXME: Portal gun might have only red but not blue;
	int iSlot2State = ((m_bCanFirePortal2) ? (0) : (1));

	SetBodygroup(1, iSlot1State);
	SetBodygroup(2, iSlot2State);

	if (pPlayer->GetViewModel())
	{
		pPlayer->GetViewModel()->SetBodygroup(1, iSlot1State);
		pPlayer->GetViewModel()->SetBodygroup(2, iSlot2State);
	}

	// HACK HACK! Used to make the gun visually change when going through a cleanser!
	if (m_fEffectsMaxSize1 > 4.0f)
	{
		m_fEffectsMaxSize1 -= gpGlobals->frametime * 400.0f;
		if (m_fEffectsMaxSize1 < 4.0f)
			m_fEffectsMaxSize1 = 4.0f;
	}

	if (m_fEffectsMaxSize2 > 4.0f)
	{
		m_fEffectsMaxSize2 -= gpGlobals->frametime * 400.0f;
		if (m_fEffectsMaxSize2 < 4.0f)
			m_fEffectsMaxSize2 = 4.0f;
	}
}

void CWeaponPortalgun::OpenProngs(bool bOpenProngs)
{
	if (m_bOpenProngs == bOpenProngs)
	{
		return;
	}

	m_bOpenProngs = bOpenProngs;

	DoEffect((m_bOpenProngs) ? (EFFECT_HOLDING) : (EFFECT_READY));

	SendWeaponAnim((m_bOpenProngs) ? (ACT_VM_PICKUP) : (ACT_VM_RELEASE));

	if (!m_bOpenProngs)
		EmitSound("PortalPlayer.ObjectUseStop");
	else
		EmitSound("PortalPlayer.ObjectUse");
}

void CWeaponPortalgun::InputChargePortal1(inputdata_t& inputdata)
{
	DispatchParticleEffect("portal_1_charge", PATTACH_POINT_FOLLOW, this, "muzzle");
}

void CWeaponPortalgun::InputChargePortal2(inputdata_t& inputdata)
{
	DispatchParticleEffect("portal_2_charge", PATTACH_POINT_FOLLOW, this, "muzzle");
}

void CWeaponPortalgun::InputFirePortal1(inputdata_t& inputdata)
{
	FirePortal(false);
	m_iLastFiredPortal = 1;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		WeaponSound(SINGLE);
	}
	else
	{
		WeaponSound(SINGLE_NPC);
	}
}

void CWeaponPortalgun::InputFirePortal2(inputdata_t& inputdata)
{
	FirePortal(true);
	m_iLastFiredPortal = 2;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		WeaponSound(WPN_DOUBLE);
	}
	else
	{
		WeaponSound(DOUBLE_NPC);
	}
}

void CWeaponPortalgun::FirePortalDirection1(inputdata_t& inputdata)
{
	Vector vDirection;
	inputdata.value.Vector3D(vDirection);
	FirePortal(false, &vDirection);
	m_iLastFiredPortal = 1;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		WeaponSound(SINGLE);
	}
	else
	{
		WeaponSound(SINGLE_NPC);
	}
}

void CWeaponPortalgun::FirePortalDirection2(inputdata_t& inputdata)
{
	Vector vDirection;
	inputdata.value.Vector3D(vDirection);
	FirePortal(true, &vDirection);
	m_iLastFiredPortal = 2;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		WeaponSound(WPN_DOUBLE);
	}
	else
	{
		WeaponSound(DOUBLE_NPC);
	}
}

#else

void C_WeaponPortalgun::OnPreDataChanged(DataUpdateType_t updateType)
{
	//PreDataChanged.m_matrixThisToLinked = m_matrixThisToLinked;
	m_bOldCanFirePortal1 = m_bCanFirePortal1;
	m_bOldCanFirePortal1 = m_bCanFirePortal2;

	BaseClass::OnPreDataChanged(updateType);
}

void C_WeaponPortalgun::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if (updateType == DATA_UPDATE_CREATED)
	{
		// Start thinking (Baseclass stops it)
		SetNextClientThink(CLIENT_THINK_ALWAYS);

		{
			C_BaseAnimating::AutoAllowBoneAccess boneaccess(true, true);
			StartEffects();
		}

		DoEffect(m_EffectState);
	}

	// Update effect state when out of parity with the server
	else if (m_nOldEffectState != m_EffectState || m_bOldCanFirePortal1 != m_bCanFirePortal1 || m_bOldCanFirePortal2 != m_bCanFirePortal2)
	{
		DoEffect(m_EffectState);
		m_nOldEffectState = m_EffectState;

		m_bOldCanFirePortal1 = m_bCanFirePortal1;
		m_bOldCanFirePortal2 = m_bCanFirePortal2;
	}
}

void C_WeaponPortalgun::ClientThink(void)
{
	CPortal_Player* pPlayer = ToPortalPlayer(GetOwner());

	if (pPlayer && dynamic_cast<C_WeaponPortalgun*>(pPlayer->GetActiveWeapon()))
	{
		if (m_EffectState == EFFECT_NONE)
		{
			//DoEffect( ( GetPlayerHeldEntity( pPlayer ) ) ? ( EFFECT_HOLDING ) : ( EFFECT_READY ) );
		}

		if (m_EffectState != EFFECT_NONE)
		{
			// Showing a special color for holding is confusing... just use the last fired color -Jeep

			//if ( m_bOpenProngs )
			//{
			//	//Turn on the grav light
			//	m_Parameters[PORTALGUN_GRAVLIGHT].SetVisibleViewModel();
			//	m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].SetVisible3rdPerson();

			//	m_Parameters[PORTALGUN_PORTAL1LIGHT].SetVisibleViewModel( false );
			//	m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetVisible3rdPerson( false );
			//	m_Parameters[PORTALGUN_PORTAL2LIGHT].SetVisibleViewModel( false );
			//	m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].SetVisible3rdPerson( false );
			//}
			//else
			{
				m_Parameters[PORTALGUN_GRAVLIGHT].SetVisibleViewModel(false);
				m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].SetVisible3rdPerson(false);

				//Turn on and off the correct fired last lights
				m_Parameters[PORTALGUN_PORTAL1LIGHT].SetVisibleViewModel(m_iLastFiredPortal == 1);
				m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetVisible3rdPerson(m_iLastFiredPortal == 1);
				m_Parameters[PORTALGUN_PORTAL2LIGHT].SetVisibleViewModel(m_iLastFiredPortal == 2);
				m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].SetVisible3rdPerson(m_iLastFiredPortal == 2);
			}
		}
	}

	// Update our effects
	DoEffectIdle();

	NetworkStateChanged();
}

Vector C_WeaponPortalgun::GetEffectColor(int iPalletIndex)
{
	Color color;

	// Showing a special color for holding is confusing... just use the last fired color -Jeep
	/*if ( m_bOpenProngs )
	{
		color = UTIL_Portal_Color( 0 );
	}
	else */if (m_iLastFiredPortal == 1)
	{
		color = UTIL_Portal_Color(1, m_iPortalLinkageGroupID);
	}
	else if (m_iLastFiredPortal == 2)
	{
		color = UTIL_Portal_Color(2, m_iPortalLinkageGroupID);
	}
	else
	{
		color = Color(128, 128, 128, 255);
	}

	Vector vColor;
	vColor.x = color.r();
	vColor.y = color.g();
	vColor.z = color.b();

	return vColor;
}

extern void FormatViewModelAttachment(Vector& vOrigin, bool bInverse);

//-----------------------------------------------------------------------------
// Purpose: Gets the complete list of values needed to render an effect from an
//			effect parameter
//-----------------------------------------------------------------------------
void C_WeaponPortalgun::GetEffectParameters(EffectType_t effectID, color32& color, float& scale, IMaterial** pMaterial, Vector& vecAttachment, bool b3rdPerson)
{
	const float dt = gpGlobals->curtime;

	// Get alpha
	float alpha = m_Parameters[effectID].GetAlpha().Interp(dt);

	// Get scale
	scale = m_Parameters[effectID].GetScale().Interp(dt);

	// Get material
	*pMaterial = (IMaterial*)m_Parameters[effectID].GetMaterial();

	// Setup the color
	color.r = (int)m_Parameters[effectID].GetColor().x;
	color.g = (int)m_Parameters[effectID].GetColor().y;
	color.b = (int)m_Parameters[effectID].GetColor().z;
	color.a = (int)alpha;

	// Setup the attachment
	int		attachment = m_Parameters[effectID].GetAttachment();
	QAngle	angles;

	// Format for first-person
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (pOwner != NULL)
	{
		C_BaseAnimating* pModel;

		if (b3rdPerson)
		{
			pModel = this;
		}
		else
		{
			pModel = pOwner->GetViewModel();
		}

		pModel->GetAttachment(attachment, vecAttachment, angles);

		if (!b3rdPerson)
		{
			::FormatViewModelAttachment(vecAttachment, true);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not an effect is set to display
//-----------------------------------------------------------------------------
bool C_WeaponPortalgun::IsEffectVisible(EffectType_t effectID, bool b3rdPerson)
{
	if (b3rdPerson)
	{
		return m_Parameters[effectID].IsVisible3rdPerson();
	}
	else
	{
		return m_Parameters[effectID].IsVisibleViewModel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws the effect sprite, given an effect parameter ID
//-----------------------------------------------------------------------------
void C_WeaponPortalgun::DrawEffectSprite(EffectType_t effectID, bool b3rdPerson)
{
	color32 color;
	float scale;
	IMaterial* pMaterial;
	Vector	vecAttachment;

	// Don't draw invisible effects
	if (!IsEffectVisible(effectID, b3rdPerson))
		return;

	// Get all of our parameters
	GetEffectParameters(effectID, color, scale, &pMaterial, vecAttachment, b3rdPerson);

	// Don't render fully translucent objects
	if (color.a <= 0.0f)
		return;

	// Draw the sprite
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->Bind(pMaterial, this);
	DrawSprite(vecAttachment, scale, scale, color);
}

//-----------------------------------------------------------------------------
// Purpose: Render our third-person effects
//-----------------------------------------------------------------------------
void C_WeaponPortalgun::DrawEffects(bool b3rdPerson)
{
	for (int i = 0; i < NUM_PORTALGUN_PARAMETERS; i++)
	{
		DrawEffectSprite((EffectType_t)i, b3rdPerson);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Third-person function call to render world model
//-----------------------------------------------------------------------------
int C_WeaponPortalgun::DrawModel(int flags)
{
	// Only render these on the transparent pass
	/*if ( flags & STUDIO_TRANSPARENCY )
	{
		DrawEffects( true );
		return 1;
	}*/

	int iRetValue = BaseClass::DrawModel(flags);

	if (iRetValue)
	{
		DrawEffects(true);
	}

	return iRetValue;
}

//-----------------------------------------------------------------------------
// Purpose: First-person function call after viewmodel has been drawn
//-----------------------------------------------------------------------------
void C_WeaponPortalgun::ViewModelDrawn(C_BaseViewModel* pBaseViewModel)
{
	// Render our effects
	DrawEffects(false);

	// Pass this back up
	BaseClass::ViewModelDrawn(pBaseViewModel);
}

void UpdatePoseParameter(C_BaseAnimating* pBaseAnimating, int iPose, float fValue)
{
	pBaseAnimating->SetPoseParameter(iPose, fValue);
}

void InterpToward(float* pfCurrent, float fGoal, float fRate)
{
	if (*pfCurrent < fGoal)
	{
		*pfCurrent += fRate;

		if (*pfCurrent > fGoal)
		{
			*pfCurrent = fGoal;
		}
	}
	else if (*pfCurrent > fGoal)
	{
		*pfCurrent -= fRate;

		if (*pfCurrent < fGoal)
		{
			*pfCurrent = fGoal;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Idle effect (pulsing)
//-----------------------------------------------------------------------------
void C_WeaponPortalgun::DoEffectIdle(void)
{
	StartEffects();

	int i;

	// Turn on the glow sprites
	for (i = PORTALGUN_GLOW1; i < (PORTALGUN_GLOW1 + NUM_GLOW_SPRITES); i++)
	{
		m_Parameters[i].GetScale().InitFromCurrent(RandomFloat(0.0075f, 0.05f) * SPRITE_SCALE, 0.1f);
		m_Parameters[i].GetAlpha().SetAbsolute(RandomInt(10, 24));
	}

	// Turn on the world glow sprites
	for (i = PORTALGUN_GLOW1_WORLD; i < (PORTALGUN_GLOW1_WORLD + NUM_GLOW_SPRITES_WORLD); i++)
	{
		m_Parameters[i].GetScale().InitFromCurrent(RandomFloat(0.015f, 0.1f) * SPRITE_SCALE, 0.1f);
		m_Parameters[i].GetAlpha().SetAbsolute(RandomInt(10, 24));
	}

	// Turn on the endcap sprites
	for (i = PORTALGUN_ENDCAP1; i < (PORTALGUN_ENDCAP1 + NUM_ENDCAP_SPRITES); i++)
	{
		m_Parameters[i].GetScale().SetAbsolute(RandomFloat(1.0f, 3.0f));
		m_Parameters[i].GetAlpha().SetAbsolute(RandomInt(96, 128));
	}

	// Turn on the world endcap sprites
	for (i = PORTALGUN_ENDCAP1_WORLD; i < (PORTALGUN_ENDCAP1_WORLD + NUM_ENDCAP_SPRITES_WORLD); i++)
	{
		m_Parameters[i].GetScale().SetAbsolute(RandomFloat(2.0f, 6.0f));
		m_Parameters[i].GetAlpha().SetAbsolute(RandomInt(96, 128));
	}

	// Turn on the internal sprites
	i = PORTALGUN_MUZZLE_GLOW;

	if (m_bPulseUp)
	{
		m_fPulse += gpGlobals->frametime;
		if (m_fPulse > 1.0f)
		{
			m_fPulse = 1.0f;
			m_bPulseUp = !m_bPulseUp;
		}
	}
	else
	{
		m_fPulse -= gpGlobals->frametime;
		if (m_fPulse < 0.0f)
		{
			m_fPulse = 0.0f;
			m_bPulseUp = !m_bPulseUp;
		}
	}

	m_Parameters[i].GetScale().SetAbsolute(cl_portalgun_effects_min_size.GetFloat() + (m_fEffectsMaxSize1 - cl_portalgun_effects_min_size.GetFloat()) * m_fPulse);
	m_Parameters[i].GetAlpha().SetAbsolute(cl_portalgun_effects_min_alpha.GetInt() + (cl_portalgun_effects_max_alpha.GetInt() - cl_portalgun_effects_min_alpha.GetInt()) * m_fPulse);
	Vector colorMagSprites = GetEffectColor(i);
	m_Parameters[i].SetColor(colorMagSprites);

	// Turn on the world internal sprites
	i = PORTALGUN_MUZZLE_GLOW_WORLD;

	m_Parameters[i].GetScale().SetAbsolute(cl_portalgun_effects_min_size.GetFloat() + (m_fEffectsMaxSize1 - cl_portalgun_effects_min_size.GetFloat()) * m_fPulse);
	m_Parameters[i].GetAlpha().SetAbsolute(cl_portalgun_effects_min_alpha.GetInt() + (cl_portalgun_effects_max_alpha.GetInt() - cl_portalgun_effects_min_alpha.GetInt()) * m_fPulse);
	m_Parameters[i].SetColor(colorMagSprites);

	// Turn on the tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1; i < (PORTALGUN_TUBE_BEAM1 + NUM_TUBE_BEAM_SPRITES); i++)
	{
		m_Parameters[i].GetAlpha().SetAbsolute(cl_portalgun_effects_min_alpha.GetInt() + (cl_portalgun_effects_max_alpha.GetInt() - cl_portalgun_effects_min_alpha.GetInt()) * m_fPulse);
		m_Parameters[i].SetColor(colorMagSprites);
	}

	// Turn on the world tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1_WORLD; i < (PORTALGUN_TUBE_BEAM1_WORLD + NUM_TUBE_BEAM_SPRITES_WORLD); i++)
	{
		m_Parameters[i].GetAlpha().SetAbsolute(cl_portalgun_effects_min_alpha.GetInt() + (cl_portalgun_effects_max_alpha.GetInt() - cl_portalgun_effects_min_alpha.GetInt()) * m_fPulse);
		m_Parameters[i].SetColor(colorMagSprites);
	}
}
#endif // !CLIENT_DLL

void CWeaponPortalgun::DoEffectBlast(bool bPortal2, int iPlacedBy, const Vector& ptStart, const Vector& ptFinalPos, const QAngle& qStartAngles, float fDelay)
{
	CEffectData	fxData;
	fxData.m_vOrigin = ptStart;
	fxData.m_vStart = ptFinalPos;
	fxData.m_flScale = gpGlobals->curtime + fDelay;
	fxData.m_vAngles = qStartAngles;
	fxData.m_nColor = ((bPortal2) ? (2) : (1));
	fxData.m_nDamageType = iPlacedBy;
	fxData.m_nHitBox = m_iPortalLinkageGroupID;
	DispatchEffect("PortalBlast", fxData);
}

void CWeaponPortalgun::FirePortal1()
{
	FirePortal(false);
	m_iLastFiredPortal = 1;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		WeaponSound(SINGLE);
	}
	else
	{
		WeaponSound(SINGLE_NPC);
	}
}

void CWeaponPortalgun::FirePortal2()
{
	FirePortal(true);
	m_iLastFiredPortal = 2;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		WeaponSound(WPN_DOUBLE);
	}
	else
	{
		WeaponSound(DOUBLE_NPC);
	}
}

float CWeaponPortalgun::TraceFirePortal(bool bPortal2, const Vector& vTraceStart, const Vector& vDirection, trace_t& tr, Vector& vFinalPosition, QAngle& qFinalAngles, int iPlacedBy, bool bTest /*= false*/)
{
	CTraceFilterSimpleClassnameList baseFilter(this, COLLISION_GROUP_NONE);
	UTIL_Portal_Trace_Filter(&baseFilter);
#ifndef CLIENT_DLL
	CTraceFilterTranslateClones traceFilterPortalShot(&baseFilter);
#else
#define traceFilterPortalShot baseFilter
#endif // !CLIENT_DLL

	Ray_t rayEyeArea;
	rayEyeArea.Init(vTraceStart + vDirection * 24.0f, vTraceStart + vDirection * -24.0f);

	float fMustBeCloserThan = 2.0f;

	CProp_Portal* pNearPortal = UTIL_Portal_FirstAlongRay(rayEyeArea, fMustBeCloserThan);

	if (!pNearPortal)
	{
		// Check for portal near and infront of you
		rayEyeArea.Init(vTraceStart + vDirection * -24.0f, vTraceStart + vDirection * 48.0f);

		fMustBeCloserThan = 2.0f;

		pNearPortal = UTIL_Portal_FirstAlongRay(rayEyeArea, fMustBeCloserThan);
	}

	if (pNearPortal && pNearPortal->IsActivedAndLinked())
	{
		iPlacedBy = PORTAL_PLACED_BY_PEDESTAL;

		Vector vPortalForward;
		pNearPortal->GetVectors(&vPortalForward, 0, 0);

		if (vDirection.Dot(vPortalForward) < 0.01f)
		{
			// If shooting out of the world, fizzle
			if (!bTest)
			{
#ifndef CLIENT_DLL
				CProp_Portal* pPortal = CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2, true);

				pPortal->m_iDelayedFailure = ((pNearPortal->m_bIsPortal2) ? (PORTAL_FIZZLE_NEAR_RED) : (PORTAL_FIZZLE_NEAR_BLUE));
				VectorAngles(vPortalForward, pPortal->m_qDelayedAngles);
				pPortal->m_vDelayedPosition = pNearPortal->GetAbsOrigin();

				vFinalPosition = pPortal->m_vDelayedPosition;
				qFinalAngles = pPortal->m_qDelayedAngles;
#endif // !CLIENT_DLL


				UTIL_TraceLine(vTraceStart - vDirection * 16.0f, vTraceStart + (vDirection * m_fMaxRange1), MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr);

				return PORTAL_ANALOG_SUCCESS_NEAR;
			}

			UTIL_TraceLine(vTraceStart - vDirection * 16.0f, vTraceStart + (vDirection * m_fMaxRange1), MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr);

			return PORTAL_ANALOG_SUCCESS_OVERLAP_LINKED;
		}
	}

	// Trace to see where the portal hit
	UTIL_TraceLine(vTraceStart, vTraceStart + (vDirection * m_fMaxRange1), MASK_SHOT_PORTAL, &traceFilterPortalShot, &tr);

	if (!tr.DidHit() || tr.startsolid)
	{
		// If it didn't hit anything, fizzle
		if (!bTest)
		{
#ifndef CLIENT_DLL
			CProp_Portal* pPortal = CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2, true);

			pPortal->m_iDelayedFailure = PORTAL_FIZZLE_NONE;
			VectorAngles(-vDirection, pPortal->m_qDelayedAngles);
			pPortal->m_vDelayedPosition = tr.endpos;

			vFinalPosition = pPortal->m_vDelayedPosition;
			qFinalAngles = pPortal->m_qDelayedAngles;
#endif // !CLIENT_DLL

		}

		return PORTAL_ANALOG_SUCCESS_PASSTHROUGH_SURFACE;
	}

	// Trace to the surface to see if there's a rotating door in the way
	CBaseEntity* list[1024];

	Ray_t ray;
	ray.Init(vTraceStart, tr.endpos);

	int nCount = UTIL_EntitiesAlongRay(list, 1024, ray, 0);

	// Loop through all entities along the ray between the gun and the surface
	for (int i = 0; i < nCount; i++)
	{
#ifndef CLIENT_DLL
		// If the entity is a rotating door
		if (FClassnameIs(list[i], "prop_door_rotating"))
		{
			// Check more precise door collision
			CBasePropDoor* pRotatingDoor = static_cast<CBasePropDoor*>(list[i]);

			Ray_t rayDoor;
			rayDoor.Init(vTraceStart, vTraceStart + (vDirection * m_fMaxRange1));

			trace_t trDoor;
			pRotatingDoor->TestCollision(rayDoor, MASK_ALL, trDoor);

			if (trDoor.DidHit())
			{
				// There's a door in the way
				tr = trDoor;

				if (sv_portal_placement_debug.GetBool())
				{
					Vector vMin;
					Vector vMax;
					Vector vZero = Vector(0.0f, 0.0f, 0.0f);
					list[i]->GetCollideable()->WorldSpaceSurroundingBounds(&vMin, &vMax);
					NDebugOverlay::Box(vZero, vMin, vMax, 0, 255, 0, 128, 0.5f);
				}

				if (!bTest)
				{
					CProp_Portal* pPortal = CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2, true);

					pPortal->m_iDelayedFailure = PORTAL_FIZZLE_CANT_FIT;
					VectorAngles(tr.plane.normal, pPortal->m_qDelayedAngles);
					pPortal->m_vDelayedPosition = trDoor.endpos;

					vFinalPosition = pPortal->m_vDelayedPosition;
					qFinalAngles = pPortal->m_qDelayedAngles;
				}

				return PORTAL_ANALOG_SUCCESS_CANT_FIT;
			}
		}
		else if (FClassnameIs(list[i], "trigger_portal_cleanser"))
		{
			CBaseTrigger* pTrigger = static_cast<CBaseTrigger*>(list[i]);

			if (pTrigger && !pTrigger->m_bDisabled)
			{
				Vector vMin;
				Vector vMax;
				pTrigger->GetCollideable()->WorldSpaceSurroundingBounds(&vMin, &vMax);

				IntersectRayWithBox(ray.m_Start, ray.m_Delta, vMin, vMax, 0.0f, &tr);

				tr.plane.normal = -vDirection;

				if (!bTest)
				{
					CProp_Portal* pPortal = CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2, true);

					pPortal->m_iDelayedFailure = PORTAL_FIZZLE_CLEANSER;
					VectorAngles(tr.plane.normal, pPortal->m_qDelayedAngles);
					pPortal->m_vDelayedPosition = tr.endpos;

					vFinalPosition = pPortal->m_vDelayedPosition;
					qFinalAngles = pPortal->m_qDelayedAngles;
				}

				return PORTAL_ANALOG_SUCCESS_CLEANSER;
			}
		}
#else
		// If the entity is a rotating door
		if (FClassnameIs(list[i], "class C_BasePropDoor"))
		{
			// Check more precise door collision
			C_BasePropDoor* pRotatingDoor = static_cast<C_BasePropDoor*>(list[i]);

			Ray_t rayDoor;
			rayDoor.Init(vTraceStart, vTraceStart + (vDirection * m_fMaxRange1));

			trace_t trDoor;
			pRotatingDoor->TestCollision(rayDoor, MASK_ALL, trDoor);

			if (trDoor.DidHit())
			{
				// There's a door in the way
				tr = trDoor;

#ifndef CLIENT_DLL
				if (sv_portal_placement_debug.GetBool())
				{
					Vector vMin;
					Vector vMax;
					Vector vZero = Vector(0.0f, 0.0f, 0.0f);
					list[i]->GetCollideable()->WorldSpaceSurroundingBounds(&vMin, &vMax);
					NDebugOverlay::Box(vZero, vMin, vMax, 0, 255, 0, 128, 0.5f);
				}

				if (!bTest)
				{
					CProp_Portal* pPortal = CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2, true);

					pPortal->m_iDelayedFailure = PORTAL_FIZZLE_CANT_FIT;
					VectorAngles(tr.plane.normal, pPortal->m_qDelayedAngles);
					pPortal->m_vDelayedPosition = trDoor.endpos;

					vFinalPosition = pPortal->m_vDelayedPosition;
					qFinalAngles = pPortal->m_qDelayedAngles;
				}
#endif // !CLIENT_DLL


				return PORTAL_ANALOG_SUCCESS_CANT_FIT;
			}
		}
#endif // !CLIENT_DLL

	}

	Vector vUp(0.0f, 0.0f, 1.0f);
	if ((tr.plane.normal.x > -0.001f && tr.plane.normal.x < 0.001f) && (tr.plane.normal.y > -0.001f && tr.plane.normal.y < 0.001f))
	{
		//plane is a level floor/ceiling
		vUp = vDirection;
	}

	// Check that the placement succeed
	VectorAngles(tr.plane.normal, vUp, qFinalAngles);

	vFinalPosition = tr.endpos;
	return VerifyPortalPlacement(CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2), vFinalPosition, qFinalAngles, iPlacedBy, bTest);
}

float CWeaponPortalgun::FirePortal(bool bPortal2, Vector* pVector /*= 0*/, bool bTest /*= false*/)
{
	bool bPlayer = false;
	Vector vEye;
	Vector vDirection;
	Vector vTracerOrigin;

	CBaseEntity* pOwner = GetOwner();

	if (pOwner && pOwner->IsPlayer())
	{
		bPlayer = true;
	}

	if (bPlayer)
	{
		CPortal_Player* pPlayer = (CPortal_Player*)pOwner;

		if (!bTest && pPlayer)
		{
			IGameEvent* pEvent = gameeventmanager->CreateEvent("portal_fired");
			if (pEvent)
			{
				pEvent->SetInt("userid", pPlayer->GetUserID());
				pEvent->SetBool("leftportal", !bPortal2);
				gameeventmanager->FireEvent(pEvent);
			}
		}

		Vector forward, right, up;
		AngleVectors(pPlayer->EyeAngles(), &forward, &right, &up);
		pPlayer->EyeVectors(&vDirection, NULL, NULL);
		vEye = pPlayer->EyePosition();

		// Check if the players eye is behind the portal they're in and translate it
		VMatrix matThisToLinked;
		CProp_Portal* pPlayerPortal = pPlayer->m_hPortalEnvironment;

		if (pPlayerPortal)
		{
			Vector ptPortalCenter;
			Vector vPortalForward;

			ptPortalCenter = pPlayerPortal->GetAbsOrigin();
			pPlayerPortal->GetVectors(&vPortalForward, NULL, NULL);

			Vector vEyeToPortalCenter = ptPortalCenter - vEye;

			float fPortalDist = vPortalForward.Dot(vEyeToPortalCenter);
			if (fPortalDist > 0.0f)
			{
				// Eye is behind the portal
				matThisToLinked = pPlayerPortal->MatrixThisToLinked();
			}
			else
			{
				pPlayerPortal = NULL;
			}
		}

		if (pPlayerPortal)
		{
			UTIL_Portal_VectorTransform(matThisToLinked, forward, forward);
			UTIL_Portal_VectorTransform(matThisToLinked, right, right);
			UTIL_Portal_VectorTransform(matThisToLinked, up, up);
			UTIL_Portal_VectorTransform(matThisToLinked, vDirection, vDirection);
			UTIL_Portal_PointTransform(matThisToLinked, vEye, vEye);

			if (pVector)
			{
				UTIL_Portal_VectorTransform(matThisToLinked, *pVector, *pVector);
			}
		}

		vTracerOrigin = vEye
			+ forward * 30.0f
			+ right * 4.0f
			+ up * (-5.0f);
	}
	else
	{
		// This portalgun is not held by the player-- Fire using the muzzle attachment
		Vector vecShootOrigin;
		QAngle angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		vEye = vecShootOrigin;
		vTracerOrigin = vecShootOrigin;
		AngleVectors(angShootDir, &vDirection, NULL, NULL);
	}

	if (!bTest)
	{
		SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	}

	if (pVector)
	{
		vDirection = *pVector;
	}

	Vector vTraceStart = vEye + (vDirection * m_fMinRange1);

	Vector vFinalPosition;
	QAngle qFinalAngles;

	PortalPlacedByType ePlacedBy = (bPlayer) ? (PORTAL_PLACED_BY_PLAYER) : (PORTAL_PLACED_BY_PEDESTAL);

	trace_t tr;
	float fPlacementSuccess = TraceFirePortal(bPortal2, vTraceStart, vDirection, tr, vFinalPosition, qFinalAngles, ePlacedBy, bTest);

	if (sv_portal_placement_never_fail.GetBool())
	{
		fPlacementSuccess = 1.0f;
	}

	if (!bTest)
	{
#ifndef CLIENT_DLL
		CProp_Portal* pPortal = CProp_Portal::FindPortal(m_iPortalLinkageGroupID, bPortal2, true);

		// If it was a failure, put the effect at exactly where the player shot instead of where the portal bumped to
		if (fPlacementSuccess < 0.5f)
			vFinalPosition = tr.endpos;

		pPortal->PlacePortal(vFinalPosition, qFinalAngles, fPlacementSuccess, true);

		float fDelay = vTracerOrigin.DistTo(tr.endpos) / ((bPlayer) ? (BLAST_SPEED) : (BLAST_SPEED_NON_PLAYER));

		QAngle qFireAngles;
		VectorAngles(vDirection, qFireAngles);
		DoEffectBlast(pPortal->m_bIsPortal2, ePlacedBy, vTracerOrigin, vFinalPosition, qFireAngles, fDelay);

		pPortal->SetContextThink(&CProp_Portal::DelayedPlacementThink, gpGlobals->curtime + fDelay, s_pDelayedPlacementContext);
		pPortal->m_vDelayedPosition = vFinalPosition;
		pPortal->m_hPlacedBy = this;
#else
		// If it was a failure, put the effect at exactly where the player shot instead of where the portal bumped to
		if (fPlacementSuccess < 0.5f)
			vFinalPosition = tr.endpos;

		float fDelay = vTracerOrigin.DistTo(tr.endpos) / ((bPlayer) ? (BLAST_SPEED) : (BLAST_SPEED_NON_PLAYER));

		QAngle qFireAngles;
		VectorAngles(vDirection, qFireAngles);
		DoEffectBlast(bPortal2, ePlacedBy, vTracerOrigin, vFinalPosition, qFireAngles, fDelay);
#endif // !CLIENT_DLL

	}

	return fPlacementSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalgun::StartEffects(void)
{
#ifdef CLIENT_DLL
	int i;

	CBaseEntity* pModelView = ((GetOwner()) ? (ToBasePlayer(GetOwner())->GetViewModel()) : (0));
	CBaseEntity* pModelWorld = this;

	if (!pModelView)
	{
		pModelView = pModelWorld;
	}

	// ------------------------------------------
	// Lights
	// ------------------------------------------

	if (m_Parameters[PORTALGUN_GRAVLIGHT].GetMaterial() == NULL)
	{
		m_Parameters[PORTALGUN_GRAVLIGHT].GetScale().SetAbsolute(0.018f * SPRITE_SCALE);
		m_Parameters[PORTALGUN_GRAVLIGHT].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[PORTALGUN_GRAVLIGHT].SetAttachment(pModelView->LookupAttachment("Body_light"));
		m_Parameters[PORTALGUN_GRAVLIGHT].SetVisible(false);

		if (m_Parameters[PORTALGUN_GRAVLIGHT].SetMaterial(PORTALGUN_GRAV_ACTIVE_GLOW) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	if (m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].GetMaterial() == NULL)
	{
		m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].GetScale().SetAbsolute(0.03f * SPRITE_SCALE);
		m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].SetAttachment(pModelWorld->LookupAttachment("Body_light"));
		m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].SetVisible(false);

		if (m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].SetMaterial(PORTALGUN_GRAV_ACTIVE_GLOW) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	if (m_Parameters[PORTALGUN_PORTAL1LIGHT].GetMaterial() == NULL)
	{
		m_Parameters[PORTALGUN_PORTAL1LIGHT].GetScale().SetAbsolute(0.018f * SPRITE_SCALE);
		m_Parameters[PORTALGUN_PORTAL1LIGHT].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[PORTALGUN_PORTAL1LIGHT].SetAttachment(pModelView->LookupAttachment("Body_light"));
		m_Parameters[PORTALGUN_PORTAL1LIGHT].SetVisible(false);

		if (m_Parameters[PORTALGUN_PORTAL1LIGHT].SetMaterial(PORTALGUN_PORTAL1_FIRED_LAST_GLOW) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	if (m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].GetMaterial() == NULL)
	{
		m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].GetScale().SetAbsolute(0.03f * SPRITE_SCALE);
		m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetAttachment(pModelWorld->LookupAttachment("Body_light"));
		m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetVisible(false);

		if (m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetMaterial(PORTALGUN_PORTAL1_FIRED_LAST_GLOW) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	if (m_Parameters[PORTALGUN_PORTAL2LIGHT].GetMaterial() == NULL)
	{
		m_Parameters[PORTALGUN_PORTAL2LIGHT].GetScale().SetAbsolute(0.018f * SPRITE_SCALE);
		m_Parameters[PORTALGUN_PORTAL2LIGHT].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[PORTALGUN_PORTAL2LIGHT].SetAttachment(pModelView->LookupAttachment("Body_light"));
		m_Parameters[PORTALGUN_PORTAL2LIGHT].SetVisible(false);

		if (m_Parameters[PORTALGUN_PORTAL2LIGHT].SetMaterial(PORTALGUN_PORTAL2_FIRED_LAST_GLOW) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	if (m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].GetMaterial() == NULL)
	{
		m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].GetScale().SetAbsolute(0.03f * SPRITE_SCALE);
		m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].SetAttachment(pModelWorld->LookupAttachment("Body_light"));
		m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].SetVisible(false);

		if (m_Parameters[PORTALGUN_PORTAL2LIGHT_WORLD].SetMaterial(PORTALGUN_PORTAL2_FIRED_LAST_GLOW) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	// ------------------------------------------
	// Glows
	// ------------------------------------------

	const char* attachNamesGlow[NUM_GLOW_SPRITES] =
	{
		"Arm1_attach1",
		"Arm1_attach2",
		"Arm2_attach1",
		"Arm2_attach2",
		"Arm3_attach1",
		"Arm3_attach2"
	};

	//Create the view glow sprites
	for (i = PORTALGUN_GLOW1; i < (PORTALGUN_GLOW1 + NUM_GLOW_SPRITES); i++)
	{
		if (m_Parameters[i].GetMaterial() != NULL)
			continue;

		m_Parameters[i].GetScale().SetAbsolute(0.05f * SPRITE_SCALE);
		m_Parameters[i].GetAlpha().SetAbsolute(24.0f);

		// Different for different views
		m_Parameters[i].SetAttachment(pModelView->LookupAttachment(attachNamesGlow[i - PORTALGUN_GLOW1]));
		m_Parameters[i].SetColor(Vector(255, 128, 0));
		m_Parameters[i].SetVisible(false);

		if (m_Parameters[i].SetMaterial(PORTALGUN_GLOW_SPRITE) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	//Create the world glow sprites
	for (i = PORTALGUN_GLOW1_WORLD; i < (PORTALGUN_GLOW1_WORLD + NUM_GLOW_SPRITES_WORLD); i++)
	{
		if (m_Parameters[i].GetMaterial() != NULL)
			continue;

		m_Parameters[i].GetScale().SetAbsolute(0.1f * SPRITE_SCALE);
		m_Parameters[i].GetAlpha().SetAbsolute(24.0f);

		// Different for different views
		m_Parameters[i].SetAttachment(pModelWorld->LookupAttachment(attachNamesGlow[i - PORTALGUN_GLOW1_WORLD]));
		m_Parameters[i].SetColor(Vector(255, 128, 0));
		m_Parameters[i].SetVisible(false);

		if (m_Parameters[i].SetMaterial(PORTALGUN_GLOW_SPRITE) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	// ------------------------------------------
	// End caps
	// ------------------------------------------

	const char* attachNamesEndCap[NUM_ENDCAP_SPRITES] =
	{
		"Arm1_attach3",
		"Arm2_attach3",
		"Arm3_attach3",
	};

	//Create the endcap sprites
	for (i = PORTALGUN_ENDCAP1; i < (PORTALGUN_ENDCAP1 + NUM_ENDCAP_SPRITES); i++)
	{
		if (m_Parameters[i].GetMaterial() != NULL)
			continue;

		m_Parameters[i].GetScale().SetAbsolute(0.02f * SPRITE_SCALE);
		m_Parameters[i].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[i].SetAttachment(pModelView->LookupAttachment(attachNamesEndCap[i - PORTALGUN_ENDCAP1]));
		m_Parameters[i].SetVisible(false);

		if (m_Parameters[i].SetMaterial(PORTALGUN_ENDCAP_SPRITE) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	//Create the world endcap sprites
	for (i = PORTALGUN_ENDCAP1_WORLD; i < (PORTALGUN_ENDCAP1_WORLD + NUM_ENDCAP_SPRITES_WORLD); i++)
	{
		if (m_Parameters[i].GetMaterial() != NULL)
			continue;

		m_Parameters[i].GetScale().SetAbsolute(0.04f * SPRITE_SCALE);
		m_Parameters[i].GetAlpha().SetAbsolute(128.0f);
		m_Parameters[i].SetAttachment(pModelWorld->LookupAttachment(attachNamesEndCap[i - PORTALGUN_ENDCAP1_WORLD]));
		m_Parameters[i].SetVisible(false);

		if (m_Parameters[i].SetMaterial(PORTALGUN_ENDCAP_SPRITE) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	// ------------------------------------------
	// Internals
	// ------------------------------------------

	//Create the muzzle glow sprites
	i = PORTALGUN_MUZZLE_GLOW;

	if (m_Parameters[i].GetMaterial() == NULL)
	{
		m_Parameters[i].GetScale().SetAbsolute(0.025f * SPRITE_SCALE);
		m_Parameters[i].GetAlpha().SetAbsolute(64.0f);
		m_Parameters[i].SetAttachment(pModelView->LookupAttachment("Inside_effects"));
		m_Parameters[i].SetVisible(false);

		if (m_Parameters[i].SetMaterial(PORTALGUN_PORTAL_MUZZLE_GLOW_SPRITE) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	//Create the world muzzle glow sprites
	i = PORTALGUN_MUZZLE_GLOW_WORLD;

	if (m_Parameters[i].GetMaterial() == NULL)
	{
		m_Parameters[i].GetScale().SetAbsolute(0.025f * SPRITE_SCALE);
		m_Parameters[i].GetAlpha().SetAbsolute(64.0f);
		m_Parameters[i].SetAttachment(pModelWorld->LookupAttachment("Inside_effects"));
		m_Parameters[i].SetVisible(false);

		if (m_Parameters[i].SetMaterial(PORTALGUN_PORTAL_MUZZLE_GLOW_SPRITE) == false)
		{
			// This means the texture was not found
			Assert(0);
		}
	}

	// ------------------------------------------
	// Tube sprites
	// ------------------------------------------

	const char* attachNamesTubeBeam[NUM_TUBE_BEAM_SPRITES] =
	{
		"Beam_point1",
		"Beam_point2",
		"Beam_point3",
		"Beam_point4",
		"Beam_point5",
	};

	//Create the tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1; i < (PORTALGUN_TUBE_BEAM1 + NUM_TUBE_BEAM_SPRITES); i++)
	{
		if (m_Parameters[i].GetMaterial() == NULL)
		{
			m_Parameters[i].GetScale().SetAbsolute(cl_portalgun_beam_size.GetFloat() * SPRITE_SCALE);
			m_Parameters[i].GetAlpha().SetAbsolute(255.0f);
			m_Parameters[i].SetAttachment(pModelView->LookupAttachment(attachNamesTubeBeam[i - PORTALGUN_TUBE_BEAM1]));
			m_Parameters[i].SetVisible(false);

			if (m_Parameters[i].SetMaterial(PORTALGUN_PORTAL_TUBE_BEAM_SPRITE) == false)
			{
				// This means the texture was not found
				Assert(0);
			}
		}
	}

	//Create the world tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1_WORLD; i < (PORTALGUN_TUBE_BEAM1_WORLD + NUM_TUBE_BEAM_SPRITES_WORLD); i++)
	{
		if (m_Parameters[i].GetMaterial() == NULL)
		{
			m_Parameters[i].GetScale().SetAbsolute(cl_portalgun_beam_size.GetFloat() * SPRITE_SCALE);
			m_Parameters[i].GetAlpha().SetAbsolute(255.0f);
			m_Parameters[i].SetAttachment(pModelView->LookupAttachment(attachNamesTubeBeam[i - PORTALGUN_TUBE_BEAM1_WORLD]));
			m_Parameters[i].SetVisible(false);

			if (m_Parameters[i].SetMaterial(PORTALGUN_PORTAL_TUBE_BEAM_SPRITE) == false)
			{
				// This means the texture was not found
				Assert(0);
			}
		}
	}

	// ------------------------------------------
	// Beams
	// ------------------------------------------

	// Setup the beams
	int iBeam = 0;

	if (pModelView != pModelWorld)
	{
		m_Beams[iBeam++].Init(pModelView->LookupAttachment("Arm1_attach3"), pModelView->LookupAttachment("muzzle"), pModelView, true);
		m_Beams[iBeam++].Init(pModelView->LookupAttachment("Arm2_attach3"), pModelView->LookupAttachment("muzzle"), pModelView, true);
		m_Beams[iBeam++].Init(pModelView->LookupAttachment("Arm3_attach3"), pModelView->LookupAttachment("muzzle"), pModelView, true);
	}
	else
	{
		iBeam += 3;
	}

	m_Beams[iBeam++].Init(pModelWorld->LookupAttachment("Arm1_attach3"), pModelWorld->LookupAttachment("muzzle"), pModelWorld, false);
	m_Beams[iBeam++].Init(pModelWorld->LookupAttachment("Arm2_attach3"), pModelWorld->LookupAttachment("muzzle"), pModelWorld, false);
	m_Beams[iBeam++].Init(pModelWorld->LookupAttachment("Arm3_attach3"), pModelWorld->LookupAttachment("muzzle"), pModelWorld, false);
#endif // CLIENT_DLL

}

void CWeaponPortalgun::DestroyEffects(void)
{
#ifdef CLIENT_DLL
	// Free our beams
	for (int i = 0; i < NUM_PORTALGUN_BEAMS; ++i)
	{
		m_Beams[i].Release();
	}
#endif // CLIENT_DLL


	// Stop everything
	StopEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Ready effects
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffectReady(void)
{
#ifdef CLIENT_DLL
	int i;

	// Turn on the glow sprites
	for (i = PORTALGUN_GLOW1; i < (PORTALGUN_GLOW1 + NUM_GLOW_SPRITES); i++)
	{
		m_Parameters[i].GetScale().InitFromCurrent(0.4f * SPRITE_SCALE, 0.2f);
		m_Parameters[i].GetAlpha().InitFromCurrent(32.0f, 0.2f);
		m_Parameters[i].SetVisibleViewModel();
	}

	for (i = PORTALGUN_GLOW1_WORLD; i < (PORTALGUN_GLOW1_WORLD + NUM_GLOW_SPRITES_WORLD); i++)
	{
		m_Parameters[i].GetScale().InitFromCurrent(0.8f * SPRITE_SCALE, 0.4f);
		m_Parameters[i].GetAlpha().InitFromCurrent(32.0f, 0.2f);
		m_Parameters[i].SetVisible3rdPerson();
	}

	// Turn on the endcap sprites
	for (i = PORTALGUN_ENDCAP1; i < (PORTALGUN_ENDCAP1 + NUM_ENDCAP_SPRITES); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	// Turn on the world endcap sprites
	for (i = PORTALGUN_ENDCAP1_WORLD; i < (PORTALGUN_ENDCAP1_WORLD + NUM_ENDCAP_SPRITES_WORLD); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	// Turn on the internal sprites
	i = PORTALGUN_MUZZLE_GLOW;

	Vector colorMagSprites = GetEffectColor(i);
	m_Parameters[i].SetColor(colorMagSprites);
	m_Parameters[i].SetVisibleViewModel();

	// Turn on the world internal sprites
	i = PORTALGUN_MUZZLE_GLOW_WORLD;

	m_Parameters[i].SetColor(colorMagSprites);
	m_Parameters[i].SetVisible3rdPerson();

	// Turn on the tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1; i < (PORTALGUN_TUBE_BEAM1 + NUM_TUBE_BEAM_SPRITES); i++)
	{
		m_Parameters[i].SetColor(colorMagSprites);
		m_Parameters[i].SetVisibleViewModel();
	}

	// Turn on the world tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1_WORLD; i < (PORTALGUN_TUBE_BEAM1_WORLD + NUM_TUBE_BEAM_SPRITES_WORLD); i++)
	{
		m_Parameters[i].SetColor(colorMagSprites);
		m_Parameters[i].SetVisible3rdPerson();
	}

	// Turn off beams off
	for (i = 0; i < NUM_PORTALGUN_BEAMS; ++i)
	{
		m_Beams[i].SetVisibleViewModel(false);
		m_Beams[i].SetVisible3rdPerson(false);
	}

	CPortal_Player* pPlayer = (CPortal_Player*)GetOwner();
	if (pPlayer)
	{
		RumbleEffect(RUMBLE_PHYSCANNON_OPEN, 0, RUMBLE_FLAG_STOP);
	}
#else
	
#endif // CLIENT_DLL
}


//-----------------------------------------------------------------------------
// Holding effects
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffectHolding(void)
{
#ifdef CLIENT_DLL
	int i;

	// Turn on the glow sprites
	for (i = PORTALGUN_GLOW1; i < (PORTALGUN_GLOW1 + NUM_GLOW_SPRITES); i++)
	{
		m_Parameters[i].GetScale().InitFromCurrent(0.5f * SPRITE_SCALE, 0.2f);
		m_Parameters[i].GetAlpha().InitFromCurrent(64.0f, 0.2f);
		m_Parameters[i].SetVisibleViewModel();
	}

	for (i = PORTALGUN_GLOW1_WORLD; i < (PORTALGUN_GLOW1_WORLD + NUM_GLOW_SPRITES_WORLD); i++)
	{
		m_Parameters[i].GetScale().InitFromCurrent(1.0f * SPRITE_SCALE, 0.4f);
		m_Parameters[i].GetAlpha().InitFromCurrent(64.0f, 0.2f);
		m_Parameters[i].SetVisible3rdPerson();
	}

	// Turn on the endcap sprites
	for (i = PORTALGUN_ENDCAP1; i < (PORTALGUN_ENDCAP1 + NUM_ENDCAP_SPRITES); i++)
	{
		m_Parameters[i].SetVisibleViewModel();
	}

	// Turn on the world endcap sprites
	for (i = PORTALGUN_ENDCAP1_WORLD; i < (PORTALGUN_ENDCAP1_WORLD + NUM_ENDCAP_SPRITES_WORLD); i++)
	{
		m_Parameters[i].SetVisible3rdPerson();
	}

	// Turn on the internal sprites
	i = PORTALGUN_MUZZLE_GLOW;

	Vector colorMagSprites = GetEffectColor(i);
	m_Parameters[i].SetColor(colorMagSprites);
	m_Parameters[i].SetVisibleViewModel();

	// Turn on the world internal sprites
	i = PORTALGUN_MUZZLE_GLOW_WORLD;

	m_Parameters[i].SetColor(colorMagSprites);
	m_Parameters[i].SetVisible3rdPerson();

	// Turn on the tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1; i < (PORTALGUN_TUBE_BEAM1 + NUM_TUBE_BEAM_SPRITES); i++)
	{
		m_Parameters[i].SetColor(colorMagSprites);
		m_Parameters[i].SetVisibleViewModel();
	}

	// Turn on the world tube beam sprites
	for (i = PORTALGUN_TUBE_BEAM1; i < (PORTALGUN_TUBE_BEAM1 + NUM_TUBE_BEAM_SPRITES); i++)
	{
		m_Parameters[i].SetColor(colorMagSprites);
		m_Parameters[i].SetVisible3rdPerson();
	}

	// Set beams them visible
	for (i = 0; i < NUM_PORTALGUN_BEAMS / 2; ++i)
	{
		m_Beams[i].SetVisible3rdPerson(false);
		m_Beams[i].SetVisibleViewModel();
		m_Beams[i].SetBrightness(128.0f);
	}

	for (i; i < NUM_PORTALGUN_BEAMS; ++i)
	{
		m_Beams[i].SetVisibleViewModel(false);
		m_Beams[i].SetVisible3rdPerson();
		m_Beams[i].SetBrightness(128.0f);
	}
#else
	
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown for the weapon when it's holstered
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffectNone(void)
{
#ifdef CLIENT_DLL
	int i;

	//Turn off main glows
	m_Parameters[PORTALGUN_GRAVLIGHT].SetVisible(false);
	m_Parameters[PORTALGUN_GRAVLIGHT_WORLD].SetVisible(false);
	m_Parameters[PORTALGUN_PORTAL1LIGHT].SetVisible(false);
	m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetVisible(false);
	m_Parameters[PORTALGUN_PORTAL2LIGHT].SetVisible(false);
	m_Parameters[PORTALGUN_PORTAL1LIGHT_WORLD].SetVisible(false);

	for (i = PORTALGUN_GLOW1; i < (PORTALGUN_GLOW1 + NUM_GLOW_SPRITES); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	for (i = PORTALGUN_GLOW1_WORLD; i < (PORTALGUN_GLOW1_WORLD + NUM_GLOW_SPRITES_WORLD); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	for (i = PORTALGUN_ENDCAP1; i < (PORTALGUN_ENDCAP1 + NUM_ENDCAP_SPRITES); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	for (i = PORTALGUN_ENDCAP1_WORLD; i < (PORTALGUN_ENDCAP1_WORLD + NUM_ENDCAP_SPRITES_WORLD); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	m_Parameters[PORTALGUN_MUZZLE_GLOW].SetVisible(false);
	m_Parameters[PORTALGUN_MUZZLE_GLOW_WORLD].SetVisible(false);

	for (i = PORTALGUN_TUBE_BEAM1; i < (PORTALGUN_TUBE_BEAM1 + NUM_TUBE_BEAM_SPRITES); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	for (i = PORTALGUN_TUBE_BEAM1_WORLD; i < (PORTALGUN_TUBE_BEAM1_WORLD + NUM_TUBE_BEAM_SPRITES_WORLD); i++)
	{
		m_Parameters[i].SetVisible(false);
	}

	for (i = 0; i < NUM_PORTALGUN_BEAMS; ++i)
	{
		m_Beams[i].SetVisibleViewModel(false);
		m_Beams[i].SetVisible3rdPerson(false);
	}
#else
	
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CWeaponPortalgun::Reload( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponPortalgun::FillClip( void )
{
	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	// Add them to the clip
	if ( pOwner->GetAmmoCount( m_iPrimaryAmmoType ) > 0 )
	{
		if ( Clip1() < GetMaxClip1() )
		{
			m_iClip1++;
			pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DryFire( void )
{
	WeaponSound(EMPTY);
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponPortalgun::SetCanFirePortal1( bool bCanFire /*= true*/ )
{
	bool bOldCanFire = m_bCanFirePortal1;
	m_bCanFirePortal1 = bCanFire;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	if ( !m_bOpenProngs )
	{
		DoEffect( EFFECT_HOLDING );
		DoEffect( EFFECT_READY );
	}

	if (!bOldCanFire && bCanFire)
	{
#ifndef CLIENT_DLL
		IGameEvent* pEvent = gameeventmanager->CreateEvent("portal_enabled");
		if (pEvent)
		{
			pEvent->SetInt("userid", pOwner->GetUserID());
			pEvent->SetBool("leftportal", true);
			gameeventmanager->FireEvent(pEvent);
		}
#endif // !CLIENT_DLL

	}

	// TODO: Remove muzzle flash when there's an upgrade animation
	//pOwner->DoMuzzleFlash();

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.25f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.25f;

	// player "shoot" animation
	//pOwner->SetAnimation( PLAYER_ATTACK1 );

	pOwner->ViewPunch( QAngle( random->RandomFloat( -1, -0.5f ), random->RandomFloat( -1, 1 ), 0 ) );

	EmitSound( "Weapon_Portalgun.powerup" );
}

void CWeaponPortalgun::SetCanFirePortal2( bool bCanFire /*= true*/ )
{
	bool bOldCanFire = m_bCanFirePortal2;
	m_bCanFirePortal2 = bCanFire;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	if ( !m_bOpenProngs )
	{
		DoEffect( EFFECT_HOLDING );
		DoEffect( EFFECT_READY );
	}

	if (!bOldCanFire && bCanFire)
	{
#ifndef CLIENT_DLL
		IGameEvent* pEvent = gameeventmanager->CreateEvent("portal_enabled");
		if (pEvent)
		{
			pEvent->SetInt("userid", pOwner->GetUserID());
			pEvent->SetBool("leftportal", false);
			gameeventmanager->FireEvent(pEvent);
		}
#endif // !CLIENT_DLL

	}

	// TODO: Remove muzzle flash when there's an upgrade animation
	//pOwner->DoMuzzleFlash();

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

	// player "shoot" animation
	//pOwner->SetAnimation( PLAYER_ATTACK1 );

	pOwner->ViewPunch( QAngle( random->RandomFloat( -1, -0.5f ), random->RandomFloat( -1, 1 ), 0 ) );

	EmitSound( "Weapon_Portalgun.powerup" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponPortalgun::PrimaryAttack( void )
{
	if ( !CanFirePortal1() )
		return;

	// Only the player fires this way so we can cast
	CPortal_Player* pPlayer = ToPortalPlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

#ifndef CLIENT_DLL
	m_OnFiredPortal1.FireOutput( pPlayer, this );
#endif
	RumbleEffect(RUMBLE_PORTALGUN_LEFT, 0, RUMBLE_FLAGS_NONE);

	FirePortal1();

	pPlayer->DoMuzzleFlash();

	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;//SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;//SequenceDuration();

	pPlayer->ViewPunch( QAngle( random->RandomFloat( -1, -0.5f ), random->RandomFloat( -1, 1 ), 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CWeaponPortalgun::SecondaryAttack( void )
{
	if ( !CanFirePortal2() )
		return;

	// Only the player fires this way so we can cast
	CPortal_Player *pPlayer = ToPortalPlayer( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

#ifndef CLIENT_DLL
	m_OnFiredPortal2.FireOutput( pPlayer, this );
#endif
	RumbleEffect(RUMBLE_PORTALGUN_RIGHT, 0, RUMBLE_FLAGS_NONE);

	FirePortal2();

	pPlayer->DoMuzzleFlash();

	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	// Don't fire again until fire animation has completed
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;//SequenceDuration();
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;//SequenceDuration();

	pPlayer->ViewPunch( QAngle( random->RandomFloat( -1, -0.5f ), random->RandomFloat( -1, 1 ), 0 ) );
}

void CWeaponPortalgun::DelayAttack( float fDelay )
{
	m_flNextPrimaryAttack = gpGlobals->curtime + fDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalgun::ItemHolsterFrame( void )
{
	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Reset the timer
		m_flHolsterTime = gpGlobals->curtime;
	
		if ( GetOwner() == NULL )
			return;

		if ( m_iClip1 == GetMaxClip1() )
			return;

		// Just load the clip with no animations
		int ammoFill = min( (GetMaxClip1() - m_iClip1), GetOwner()->GetAmmoCount( GetPrimaryAmmoType() ) );
		
		GetOwner()->RemoveAmmo( ammoFill, GetPrimaryAmmoType() );
		m_iClip1 += ammoFill;
	}
}

void CWeaponPortalgun::ItemPreFrame()
{
	BaseClass::ItemPreFrame();

	// Test portal placement
	m_fCanPlacePortal1OnThisSurface = ((m_bCanFirePortal1) ? (FirePortal(false, 0, true)) : (0.0f));
	m_fCanPlacePortal2OnThisSurface = ((m_bCanFirePortal2) ? (FirePortal(true, 0, true)) : (0.0f));

	// HACK HACK! Used to make the gun visually change when going through a cleanser!
	if (m_fEffectsMaxSize1 > 4.0f)
	{
		m_fEffectsMaxSize1 -= gpGlobals->frametime * 400.0f;
		if (m_fEffectsMaxSize1 < 4.0f)
			m_fEffectsMaxSize1 = 4.0f;
	}

	if (m_fEffectsMaxSize2 > 4.0f)
	{
		m_fEffectsMaxSize2 -= gpGlobals->frametime * 400.0f;
		if (m_fEffectsMaxSize2 < 4.0f)
			m_fEffectsMaxSize2 = 4.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPortalgun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	DestroyEffects();

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponPortalgun::Deploy( void )
{
	DoEffect( EFFECT_READY );

	bool bReturn = BaseClass::Deploy();

	m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner )
	{
		pOwner->SetNextAttack( gpGlobals->curtime );

#ifndef CLIENT_DLL
		if( GameRules()->IsMultiplayer() )
		{
			m_iPortalLinkageGroupID = pOwner->entindex();

			Assert( (m_iPortalLinkageGroupID >= 0) && (m_iPortalLinkageGroupID < 256) );
		}
#endif
	}

	return bReturn;
}

void CWeaponPortalgun::WeaponIdle( void )
{
	if (m_bOpenProngs)
	{
		if (HasWeaponIdleTimeElapsed())
			SendWeaponAnim(ACT_VM_PICKUP_IDLE);
	}
	//See if we should idle high or low
	else if ( WeaponShouldBeLowered() )
	{
		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED 
			&& GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( m_flRaiseTime < gpGlobals->curtime && GetActivity() == ACT_VM_IDLE_LOWERED ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponPortalgun::StopEffects( bool stopSound )
{
	// Turn off our effect state
	DoEffect( EFFECT_NONE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : effectType - 
//-----------------------------------------------------------------------------
void CWeaponPortalgun::DoEffect( int effectType, Vector *pos )
{
	m_EffectState = effectType;

#ifdef CLIENT_DLL
	// Save predicted state
	m_nOldEffectState = m_EffectState;
#endif

	switch( effectType )
	{
	case EFFECT_READY:
		DoEffectReady();
		break;

	case EFFECT_HOLDING:
		DoEffectHolding();
		break;

	default:
	case EFFECT_NONE:
		DoEffectNone();
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Restore
//-----------------------------------------------------------------------------
void CWeaponPortalgun::OnRestore()
{
	BaseClass::OnRestore();

	// Portalgun effects disappear through level transition, so
	//  just recreate any effects here
	if ( m_EffectState != EFFECT_NONE )
	{
		DoEffect( m_EffectState, NULL );
	}
}


//-----------------------------------------------------------------------------
// On Remove
//-----------------------------------------------------------------------------
void CWeaponPortalgun::UpdateOnRemove(void)
{
	DestroyEffects();
	BaseClass::UpdateOnRemove();
}

#ifndef CLIENT_DLL
void CC_UpgradePortalGun(void)
{
	CPortal_Player* pPlayer = ToPortalPlayer(UTIL_GetCommandClient());

	CWeaponPortalgun* pPortalGun = static_cast<CWeaponPortalgun*>(pPlayer->Weapon_OwnsThisType("weapon_portalgun"));
	if (pPortalGun)
	{
		pPortalGun->SetCanFirePortal1();
		pPortalGun->SetCanFirePortal2();
	}
}

static ConCommand upgrade_portal("upgrade_portalgun", CC_UpgradePortalGun, "Equips the player with a single portal portalgun. Use twice for a dual portal portalgun.\n\tArguments:   	none ", FCVAR_CHEAT);




static void change_portalgun_linkage_id_f(const CCommand& args)
{
	if (sv_cheats->GetBool() == false) //heavy handed version since setting the concommand with FCVAR_CHEATS isn't working like I thought
		return;

	if (args.ArgC() < 2)
		return;

	unsigned char iNewID = (unsigned char)atoi(args[1]);

	CPortal_Player* pPlayer = (CPortal_Player*)UTIL_GetCommandClient();

	int iWeaponCount = pPlayer->WeaponCount();
	for (int i = 0; i != iWeaponCount; ++i)
	{
		CBaseCombatWeapon* pWeapon = pPlayer->GetWeapon(i);
		if (pWeapon == NULL)
			continue;

		if (dynamic_cast<CWeaponPortalgun*>(pWeapon) != NULL)
		{
			CWeaponPortalgun* pPortalGun = (CWeaponPortalgun*)pWeapon;
			pPortalGun->m_iPortalLinkageGroupID = iNewID;
			break;
		}
	}
}

ConCommand change_portalgun_linkage_id("change_portalgun_linkage_id", change_portalgun_linkage_id_f, "Changes the portal linkage ID for the portal gun held by the commanding player.", FCVAR_CHEAT);

#endif // !CLIENT_DLL
