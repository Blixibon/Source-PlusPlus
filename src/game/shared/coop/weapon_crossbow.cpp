//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon_shared.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "IEffects.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "beam_shared.h"
#include "rumble_shared.h"
#include "gamestats.h"
#include "decals.h"

#include "weapon_coop_basehlcombatweapon.h"

#include "hl2_player_shared.h"

#ifndef CLIENT_DLL
    #include "basecombatcharacter.h"
    #include "ai_basenpc.h"
    #include "player.h"
    #include "soundent.h"
    #include "game.h"
    #include "te_effect_dispatch.h"
    #include "ilagcompensationmanager.h"
#else
    #include "c_te_effect_dispatch.h"
    #include "input.h"
    #define CWeaponCrossbow C_WeaponCrossbow
#endif

#ifdef PORTAL
	#include "portal_util_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define BOLT_MODEL	"models/weapons/w_missile_closed.mdl"

#define BOLT_AIR_VELOCITY	2500
#define BOLT_WATER_VELOCITY	1500

#define	BOLT_SKIN_NORMAL	0
#define BOLT_SKIN_GLOW		1

#ifndef CLIENT_DLL

extern ConVar sk_plr_dmg_crossbow;
extern ConVar sk_npc_dmg_crossbow;

void TE_StickyBolt( IRecipientFilter& filter, float delay,	Vector vecDirection, const Vector *origin );

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CCrossbowBolt : public CBaseCombatCharacter
{
	DECLARE_CLASS( CCrossbowBolt, CBaseCombatCharacter );
    DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:
	CCrossbowBolt();
	~CCrossbowBolt();

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Precache( void );
	void BubbleThink( void );
	void BoltTouch( CBaseEntity *pOther );
	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
	static CCrossbowBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, const CWeaponCoopBase *pCrossbow, CBaseEntity *pentOwner = NULL );

protected:

	bool	CreateSprites( void );

	CHandle<CSprite>		m_pGlowSprite;
	//CHandle<CSpriteTrail>	m_pGlowTrail;
	CHandle<CWeaponCoopBase> m_hCrossbow;
};

LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

IMPLEMENT_SERVERCLASS_ST( CCrossbowBolt, DT_CrossbowBolt )
END_SEND_TABLE()

#ifndef CLIENT_DLL
BEGIN_DATADESC( CCrossbowBolt )
	// Function Pointers
	DEFINE_FUNCTION( BubbleThink ),
	DEFINE_FUNCTION( BoltTouch ),

	// These are recreated on reload, they don't need storage
	DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),
	DEFINE_FIELD(m_hCrossbow, FIELD_EHANDLE),

END_DATADESC()
#endif

CCrossbowBolt *CCrossbowBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, const CWeaponCoopBase* pCrossbow, CBaseEntity*pentOwner )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = (CCrossbowBolt *)CreateEntityByName( "crossbow_bolt" );
	UTIL_SetOrigin( pBolt, vecOrigin );
	pBolt->SetAbsAngles( angAngles );
	pBolt->Spawn();
	pBolt->SetOwnerEntity( pentOwner );
	pBolt->m_hCrossbow.Set(pCrossbow);

	return pBolt;
}

#ifdef CLIENT_DLL
//================================================================================
//================================================================================
void CCrossbowBolt::OnDataChanged( DataUpdateType_t type ) 
{
    BaseClass::OnDataChanged( type );

    if ( GetPredictable() && !ShouldPredict() )
        ShutdownPredictable();
}

//================================================================================
//================================================================================
bool CCrossbowBolt::ShouldPredict() 
{
    if ( m_hOwnerEntity.Get() && m_hOwnerEntity.Get() == C_BasePlayer::GetLocalPlayer() )
		return true;

	return BaseClass::ShouldPredict();
}
#endif

CCrossbowBolt::CCrossbowBolt()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCrossbowBolt::~CCrossbowBolt( void )
{
	if ( m_pGlowSprite )
	{
		UTIL_Remove( m_pGlowSprite );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCrossbowBolt::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CCrossbowBolt::PhysicsSolidMaskForEntity() const
{
	return (CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_HITBOX);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCrossbowBolt::CreateSprites( void )
{
	// Start up the eye glow
	m_pGlowSprite = CSprite::SpriteCreate( "sprites/light_glow02_noz.vmt", GetLocalOrigin(), false );

	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->FollowEntity( this );
		m_pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		m_pGlowSprite->SetScale( 0.2f );
		m_pGlowSprite->TurnOff();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::Spawn( void )
{
	Precache( );

	SetModel( "models/crossbow_bolt.mdl" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(0.3f,0.3f,0.3f), Vector(0.3f,0.3f,0.3f) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.05f );
	
	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CCrossbowBolt::BoltTouch );

	SetThink( &CCrossbowBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	CreateSprites();

	// Make us glow until we've hit the wall
	m_nSkin = BOLT_SKIN_GLOW;
}


void CCrossbowBolt::Precache( void )
{
	PrecacheModel( BOLT_MODEL );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	PrecacheModel( "models/crossbow_bolt.mdl" );

	PrecacheModel( "sprites/light_glow02_noz.vmt" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER) )
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ( ( pOther->m_takedamage == DAMAGE_NO ) || ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
			return;
	}

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

#if defined(HL2_EPISODIC)
		//!!!HACKHACK - specific hack for ep2_outland_10 to allow crossbow bolts to pass through her bounding box when she's crouched in front of the player
		// (the player thinks they have clear line of sight because Alyx is crouching, but her BBOx is still full-height and blocks crossbow bolts.
		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->Classify() == CLASS_PLAYER_ALLY_VITAL && FStrEq(STRING(gpGlobals->mapname), "ep2_outland_10") )
		{
			// Change the owner to stop further collisions with Alyx. We do this by making her the owner.
			// The player won't get credit for this kill but at least the bolt won't magically disappear!
			SetOwnerEntity( pOther );
			return;
		}
#endif//HL2_EPISODIC

		CBaseEntity* pOwner = GetOwnerEntity();
		if (m_hCrossbow.Get() && (!pOwner || !pOwner->IsPlayer()) && m_hCrossbow->GetOwner() && m_hCrossbow->GetOwner()->IsPlayer())
		{
			pOwner = m_hCrossbow->GetOwner();
		}

		if(pOwner && pOwner->IsPlayer() && pOther->IsNPC() )
		{
			CTakeDamageInfo	dmgInfo( this, pOwner, m_hCrossbow.Get(), sk_plr_dmg_crossbow.GetFloat(), DMG_NEVERGIB | DMG_SNIPER );
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );

			CBasePlayer *pPlayer = ToBasePlayer(pOwner);
			if ( pPlayer )
			{
				gamestats->Event_WeaponHit( pPlayer, true, "weapon_crossbow", dmgInfo );
			}

		}
		else
		{
			CTakeDamageInfo	dmgInfo( this, pOwner, m_hCrossbow.Get(), pOwner->IsNPC() ? sk_npc_dmg_crossbow.GetFloat() : sk_plr_dmg_crossbow.GetFloat(), DMG_BULLET | DMG_NEVERGIB | DMG_SNIPER);
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;

		if ( !pOther->IsAlive() )
		{
			// We killed it! 
			const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
			if ( pdata->game.material == CHAR_TEX_GLASS )
			{
				return;
			}
		}

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
		EmitSound( "Weapon_Crossbow.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;
			
				DispatchEffect( "BoltImpact", data );
			}
		}
		
		SetTouch( NULL );
		SetThink( NULL );

		//if ( !g_pGameRules->IsMultiplayer() )
		{
			UTIL_Remove( this );
		}
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			EmitSound( "Weapon_Crossbow.BoltHitWorld" );

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			if ( ( hitDot < 0.5f ) && ( speed > 100 ) )
			{
				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				DispatchEffect( "BoltImpact", data );
				
				UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CCrossbowBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 2.0f );

				if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}
			}
			
			// Shoot some sparks
			if ( UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}

			UTIL_Remove( this );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CCrossbowBolt::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrossbowBolt::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make danger sounds out in front of me, to scare snipers back into their hole
	CSoundEnt::InsertSound( SOUND_DANGER_SNIPERONLY, GetAbsOrigin() + GetAbsVelocity() * 0.2, 120.0f, 0.5f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );

	if ( GetWaterLevel()  == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}
#endif


//-----------------------------------------------------------------------------
// CWeaponCrossbow
//-----------------------------------------------------------------------------

class CWeaponCrossbow : public CWeaponCoopBaseHLCombat
{
	DECLARE_CLASS( CWeaponCrossbow, CWeaponCoopBaseHLCombat );
public:

	CWeaponCrossbow( void );

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_CROSSBOW; }

	virtual bool			IsSniper() { return true; }
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Deploy( void );
	virtual void	Drop( const Vector &vecVelocity );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
    #ifndef CLIENT_DLL
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual void	Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary);
	virtual void	NPC_Reload();
	virtual void	NPC_OnRangeAttack1(CAI_BaseNPC* pOperator);

	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual int	GetMinBurst() { return 1; }
	virtual int	GetMaxBurst() { return 1; }

	virtual float	GetMinRestTime(void) { return 3.0f; } // 1.5f
	virtual float	GetMaxRestTime(void) { return 3.0f; } // 2.0f

	virtual float GetFireRate(void) { return 5.0f; }

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_15DEGREES;
		if (!GetOwner() || !GetOwner()->IsNPC())
			return cone;

		static Vector NPCCone = VECTOR_CONE_5DEGREES;

		return NPCCone;
	}
    #endif
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	IsWeaponZoomed() { return m_bInZoom; }
	
	bool	ShouldDisplayHUDHint() { return true; }


	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
    //DECLARE_ACTTABLE();

private:
	
	void	StopEffects( void );
	void	SetSkin( int skinNum );
	void	CheckZoomToggle( void );
	void	FireBolt( void );
#ifndef CLIENT_DLL
	void	FireNPCBolt(CAI_BaseNPC* pOwner, Vector& vecShootOrigin, Vector& vecShootDir);
#endif
	void	ToggleZoom( void );
	
	// Various states for the crossbow's charger
	enum ChargerState_t
	{
		CHARGER_STATE_START_LOAD,
		CHARGER_STATE_START_CHARGE,
		CHARGER_STATE_READY,
		CHARGER_STATE_DISCHARGE,
		CHARGER_STATE_OFF,
	};

	void	CreateChargerEffects( void );
	void	SetChargerState( ChargerState_t state );
	void	DoLoadEffect( void );

private:
	
	// Charger effects
	ChargerState_t		m_nChargeState;
	CHandle<CSprite>	m_hChargerSprite;

    bool m_bIsInThirdPerson;

	CNetworkVar( bool, m_bInZoom );
	CNetworkVar( bool, m_bMustReload );
};

LINK_ENTITY_TO_CLASS( weapon_crossbow, CWeaponCrossbow );
PRECACHE_WEAPON_REGISTER( weapon_crossbow );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCrossbow, DT_WeaponCrossbow )

BEGIN_NETWORK_TABLE( CWeaponCrossbow, DT_WeaponCrossbow )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInZoom ) ),
	RecvPropBool( RECVINFO( m_bMustReload ) ),
#else
	SendPropBool( SENDINFO( m_bInZoom ) ),
	SendPropBool( SENDINFO( m_bMustReload ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponCrossbow )
	DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bMustReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC( CWeaponCrossbow )
	DEFINE_FIELD( m_bInZoom,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMustReload,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nChargeState,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hChargerSprite,	FIELD_EHANDLE ),
END_DATADESC()

//acttable_t	CWeaponCrossbow::m_acttable[] = 
//{
//	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_CROSSBOW,					false },
//	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_CROSSBOW,						false },
//	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_CROSSBOW,				false },
//	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_CROSSBOW,				false },
//	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },
//	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_CROSSBOW,			false },
//	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_CROSSBOW,					false },
//};

//IMPLEMENT_ACTTABLE(CWeaponCrossbow);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCrossbow::CWeaponCrossbow( void )
{
	m_bReloadsSingly	= true;
	m_bFiresUnderwater	= true;
	m_bAltFiresUnderwater = true;
	m_bInZoom			= false;
	m_bMustReload		= false;

	m_fMinRange1 = 24;
	m_fMaxRange1 = 5000;
	m_fMinRange2 = 24;
	m_fMaxRange2 = 5000;
}

#define	CROSSBOW_GLOW_SPRITE	"sprites/light_glow02_noz.vmt"
#define	CROSSBOW_GLOW_SPRITE2	"sprites/blueflare1.vmt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "crossbow_bolt" );
#endif

	PrecacheScriptSound( "Weapon_Crossbow.BoltHitBody" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltHitWorld" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltSkewer" );

	PrecacheModel( CROSSBOW_GLOW_SPRITE );
	PrecacheModel( CROSSBOW_GLOW_SPRITE2 );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::PrimaryAttack( void )
{
	/*if ( m_bInZoom && g_pGameRules->IsMultiplayer() )
	{
//		FireSniperBolt();
		FireBolt();
	}
	else*/
	{
		FireBolt();
	}

	// Signal a reload
	m_bMustReload = true;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );

	m_iPrimaryAttacks++;

#ifndef CLIENT_DLL
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		//m_iPrimaryAttacks++;
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SecondaryAttack( void )
{
	//NOTENOTE: The zooming is handled by the post/busy frames
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Reload( void )
{
	if ( BaseClass::Reload() )
	{
		m_bMustReload = false;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::CheckZoomToggle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
			ToggleZoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ItemBusyFrame( void )
{
	// Allow zoom toggling even when we're reloading
	CheckZoomToggle();
	HandleUserHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ItemPostFrame( void )
{
	// Allow zoom toggling
	CheckZoomToggle();

	if ( m_bMustReload && HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::FireBolt( void )
{
	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

#ifndef CLIENT_DLL
	pOwner->RumbleEffect( RUMBLE_357, 0, RUMBLE_FLAG_RESTART );
//#endif

	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

#if defined(HL2_EPISODIC)
	// !!!HACK - the other piece of the Alyx crossbow bolt hack for Outland_10 (see ::BoltTouch() for more detail)
	if( FStrEq(STRING(gpGlobals->mapname), "ep2_outland_10") )
	{
		trace_t tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 24.0f, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );

		if( tr.m_pEnt != NULL && tr.m_pEnt->Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			// If Alyx is right in front of the player, make sure the bolt starts outside of the player's BBOX, or the bolt
			// will instantly collide with the player after the owner of the bolt is switched to Alyx in ::BoltTouch(). We 
			// avoid this altogether by making it impossible for the bolt to collide with the player.
			vecSrc += vecAiming * 24.0f;
		}
	}
#endif

//#ifndef CLIENT_DLL
	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate( vecSrc, angAiming, this, pOwner );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_WATER_VELOCITY );
	}
	else
	{
		pBolt->SetAbsVelocity( vecAiming * BOLT_AIR_VELOCITY );
	}
#endif

	m_iClip1--;

	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

	WeaponSound( SINGLE );
	WeaponSound( SPECIAL2 );

#ifndef CLIENT_DLL
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 200, 0.2 );
#endif    

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	CHL2_Player* pHL2 = ToHL2Player(pOwner);
	if (pHL2)
		pHL2->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;

	DoLoadEffect();
	SetChargerState( CHARGER_STATE_DISCHARGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Deploy( void )
{
	if ( m_iClip1 <= 0 )
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}

	SetSkin( BOLT_SKIN_GLOW );

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	StopEffects();
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

#ifdef CLIENT_DLL
    // Jugador local
    // Cambiamos a primera persona al apuntar siempre
    /*if ( pPlayer->IsLocalPlayer() )
    {
        ConVarRef c_thirdpersonshoulder("c_thirdpersonshoulder");
        
        // Activando zoom
        if ( !m_bInZoom )
        {
            m_bIsInThirdPerson = c_thirdpersonshoulder.GetBool();
            input->CAM_ToFirstPerson();
        }
        else
        {
            if ( m_bIsInThirdPerson )
            {
                input->CAM_ToThirdPersonShoulder();
            }
        }
    }*/
#endif

	if ( m_bInZoom )
	{
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
		}
	}
	else
	{
		if ( pPlayer->SetFOV( this, 20, 0.1f ) )
		{
			m_bInZoom = true;
		}
	}
}

#define	BOLT_TIP_ATTACHMENT	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::CreateChargerEffects( void )
{
#ifndef CLIENT_DLL
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( m_hChargerSprite != NULL || !pOwner)
		return;

	m_hChargerSprite = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE, GetAbsOrigin(), false );

	if ( m_hChargerSprite )
	{
		m_hChargerSprite->SetAttachment( pOwner->GetViewModel(), BOLT_TIP_ATTACHMENT );
		m_hChargerSprite->SetTransparency( kRenderTransAdd, 255, 128, 0, 255, kRenderFxNoDissipation );
		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->SetScale( 0.1f );
		m_hChargerSprite->TurnOff();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : skinNum - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetSkin( int skinNum )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	pViewModel->m_nSkin = skinNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::DoLoadEffect( void )
{
	SetSkin( BOLT_SKIN_GLOW );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	CEffectData	data;

#ifndef CLIENT_DLL
	data.m_nEntIndex = pViewModel->entindex();
#else
    data.m_hEntity = pViewModel;
#endif

	data.m_nAttachmentIndex = 1;

	DispatchEffect( "CrossbowLoad", data );

#ifndef CLIENT_DLL
	CSprite *pBlast = CSprite::SpriteCreate( CROSSBOW_GLOW_SPRITE2, GetAbsOrigin(), false );

	if ( pBlast )
	{
		pBlast->SetAttachment( pOwner->GetViewModel(), 1 );
		pBlast->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		pBlast->SetBrightness( 128 );
		pBlast->SetScale( 0.2f );
		pBlast->FadeOutFromSpawn();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::SetChargerState( ChargerState_t state )
{
	// Make sure we're setup
	CreateChargerEffects();

	// Don't do this twice
	if ( state == m_nChargeState )
		return;

	m_nChargeState = state;

	switch( m_nChargeState )
	{
	case CHARGER_STATE_START_LOAD:
	
		WeaponSound( SPECIAL1 );
		
		// Shoot some sparks and draw a beam between the two outer points
		DoLoadEffect();
		
		break;

	case CHARGER_STATE_START_CHARGE:
		{
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 32, 0.5f );
			m_hChargerSprite->SetScale( 0.025f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_READY:
		{
			// Get fully charged
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 80, 1.0f );
			m_hChargerSprite->SetScale( 0.1f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_DISCHARGE:
		{
			SetSkin( BOLT_SKIN_NORMAL );
			
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
		}

		break;

	case CHARGER_STATE_OFF:
		{
			SetSkin( BOLT_SKIN_NORMAL );

			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
		}
		break;

	default:
		break;
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::NPC_Reload(void)
{
	BaseClass::NPC_Reload();

	m_nSkin = 0;
}

void CWeaponCrossbow::NPC_OnRangeAttack1(CAI_BaseNPC* pOperator)
{
	if (!pOperator || !m_iClip1)
		return;

	Vector vecSrc = pOperator->Weapon_ShootPosition();
	Vector vecAiming = pOperator->GetActualShootTrajectory(vecSrc);

	FireNPCBolt(pOperator, vecSrc, vecAiming);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::FireNPCBolt(CAI_BaseNPC* pOwner, Vector& vecShootOrigin, Vector& vecShootDir)
{
	Assert(pOwner);

	QAngle angAiming;
	VectorAngles(vecShootDir, angAiming);

	CCrossbowBolt* pBolt = CCrossbowBolt::BoltCreate(vecShootOrigin, angAiming, this, pOwner);

	if (pOwner->GetWaterLevel() == 3)
	{
		pBolt->SetAbsVelocity(vecShootDir * BOLT_WATER_VELOCITY);
	}
	else
	{
		pBolt->SetAbsVelocity(vecShootDir * BOLT_AIR_VELOCITY);
	}

	m_iClip1--;

	m_nSkin = 1;

	WeaponSound(SINGLE_NPC);
	WeaponSound(SPECIAL2);

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 200, 0.2);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 2.5f;

	//SetSkin(BOLT_SKIN_GLOW);
	//SetChargerState(CHARGER_STATE_DISCHARGE);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_THROW:
		SetChargerState( CHARGER_STATE_START_LOAD );
		break;

	case EVENT_WEAPON_THROW2:
		SetChargerState( CHARGER_STATE_START_CHARGE );
		break;
	
	case EVENT_WEAPON_THROW3:
		SetChargerState( CHARGER_STATE_READY );
		break;

	case EVENT_WEAPON_SMG1:
	{
		CAI_BaseNPC* pNPC = pOperator->MyNPCPointer();
		Assert(pNPC);

		NPC_OnRangeAttack1(pNPC);
		//m_bMustReload = true;
	}
	break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCBolt(pOperator->MyNPCPointer(), vecShootOrigin, vecShootDir);

	//m_bMustReload = true;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CWeaponCrossbow::SendWeaponAnim( int iActivity )
{
	int newActivity = iActivity;

	// The last shot needs a non-loaded activity
	if ( ( newActivity == ACT_VM_IDLE ) && ( m_iClip1 <= 0 ) )
	{
		newActivity = ACT_VM_FIDGET;
	}

	//For now, just set the ideal activity and be done with it
	return BaseClass::SendWeaponAnim( newActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Stop all zooming and special effects on the viewmodel
//-----------------------------------------------------------------------------
void CWeaponCrossbow::StopEffects( void )
{
	// Stop zooming
	if ( m_bInZoom )
	{
		ToggleZoom();
	}

	// Turn off our sprites
	SetChargerState( CHARGER_STATE_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCrossbow::Drop( const Vector &vecVelocity )
{
	StopEffects();
	BaseClass::Drop( vecVelocity );
}
