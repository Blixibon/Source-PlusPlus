//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Pistol with laser
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "gamestats.h"
#include "weapon_rpg.h"
#include "util.h"
#include "Sprite.h"
#include "npcevent.h"
#include "beam_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	DEAGLE_FASTEST_REFIRE_TIME		0.1f
#define	DEAGLE_FASTEST_DRY_REFIRE_TIME	0.2f

#define	DEAGLE_ACCURACY_SHOT_PENALTY_TIME		0.2f	// Applied amount of time each shot adds to the time we must recover from
#define	DEAGLE_ACCURACY_MAXIMUM_PENALTY_TIME	1.5f	// Maximum penalty to deal out

//-----------------------------------------------------------------------------
// CWeaponDeagle
//-----------------------------------------------------------------------------

class CWeaponDeagle : public CBaseHLCombatWeapon
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CWeaponDeagle, CBaseHLCombatWeapon );

	CWeaponDeagle();
	~CWeaponDeagle();

	DECLARE_SERVERCLASS();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );
	void	PrimaryAttack( void );
	void	AddViewKick( void );
	void	DryFire( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	WeaponIdle( void );
	void	UpdatePenaltyTime( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	Activity	GetPrimaryAttackActivity( void );

	virtual bool Reload( void );

	virtual const Vector& GetBulletSpread( void )
	{		
		static Vector cone = VECTOR_CONE_5DEGREES;

		if ( GetOwner() && GetOwner()->IsNPC() )
			return cone;

		if (m_bGuiding)
			cone = vec3_origin;
		else
			cone = VECTOR_CONE_7DEGREES;

		return cone;
	}
	
	virtual int	GetMinBurst() 
	{ 
		return 1; 
	}

	virtual int	GetMaxBurst() 
	{ 
		return 3; 
	}

	virtual float GetFireRate( void ) 
	{
		if (m_bGuiding)
			return 0.95f;
		else
			return 0.3f;
	}

	DECLARE_ACTTABLE();

private:
	float	m_flSoonestPrimaryAttack;
	float	m_flLastAttackTime;
	float	m_flAccuracyPenalty;
	int		m_nNumShotsFired;

public:
	void	SuppressGuiding(bool state = true);
	void	StartGuiding(void);
	void	StopGuiding(void);
	void	ToggleGuiding(void);
	bool	IsGuiding(void);
	void	Activate(void);
	void	CreateLaserPointer(void);
	void	UpdateLaserPosition(Vector vecMuzzlePos = vec3_origin, Vector vecEndPos = vec3_origin);
	Vector	GetLaserPosition(void);
	void	StartLaserEffects(void);
	void	StopLaserEffects(void);
	void	UpdateLaserEffects(void);
	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	void	Drop(const Vector &vecVelocity);

protected:
	bool				m_bInitialStateUpdate;
	bool				m_bGuiding;
	bool				m_bHideGuiding;		//User to override the player's wish to guide under certain circumstances
	CHandle<CLaserDot>	m_hLaserDot;
	CHandle<CSprite>	m_hLaserMuzzleSprite;
	CHandle<CBeam>		m_hLaserBeam;
	bool				m_bStartGuidingWhenReloadIsFinished;
	bool				m_bWasGuidingBeforeHolster;
};


IMPLEMENT_SERVERCLASS_ST(CWeaponDeagle, DT_WeaponDeagle)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_deagle, CWeaponDeagle );
PRECACHE_WEAPON_REGISTER( weapon_deagle );

BEGIN_DATADESC( CWeaponDeagle )

	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flAccuracyPenalty,		FIELD_FLOAT ), //NOTENOTE: This is NOT tracking game time
	DEFINE_FIELD( m_nNumShotsFired,			FIELD_INTEGER ),
	
	DEFINE_FIELD( m_bInitialStateUpdate,FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bGuiding,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hLaserDot,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLaserMuzzleSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLaserBeam,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_bHideGuiding,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bStartGuidingWhenReloadIsFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWasGuidingBeforeHolster, FIELD_BOOLEAN ),

END_DATADESC()

acttable_t	CWeaponDeagle::m_acttable[] = 
{
	{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },
	{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },
	{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },
	{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },
	{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },
	{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },
	{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },
	{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },
	{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_WALK,						ACT_WALK_PISTOL,				false },
	{ ACT_RUN,						ACT_RUN_PISTOL,					false },

	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
	{ ACT_MP_STAND_IDLE,				ACT_DOD_STAND_AIM_PISTOL,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_DOD_CROUCH_IDLE_PISTOL,				false },
	{ ACT_MP_RUN,						ACT_DOD_WALK_IDLE_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,				ACT_DOD_CROUCHWALK_IDLE_PISTOL,			false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_DOD_PRIMARYATTACK_PISTOL,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_DOD_PRIMARYATTACK_PISTOL,			false },
	{ ACT_MP_RELOAD_STAND,				ACT_DOD_RELOAD_PISTOL,					false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_DOD_RELOAD_CROUCH_PISTOL,			false },
	{ ACT_MP_JUMP,						ACT_MP_JUMP,							false },
};


IMPLEMENT_ACTTABLE( CWeaponDeagle );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDeagle::CWeaponDeagle()
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= true;
}

CWeaponDeagle::~CWeaponDeagle()
{
	if (m_hLaserDot != NULL)
	{
		UTIL_Remove(m_hLaserDot);
		m_hLaserDot = NULL;
	}

	if (m_hLaserMuzzleSprite)
	{
		UTIL_Remove(m_hLaserMuzzleSprite);
	}

	if (m_hLaserBeam)
	{
		UTIL_Remove(m_hLaserBeam);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::Precache( void )
{
	BaseClass::Precache();

	// Laser dot...
	PrecacheModel( RPG_LASER_SPRITE );
	PrecacheModel( RPG_BEAM_SPRITE );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CWeaponDeagle::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_PISTOL_FIRE:
		{
			Vector vecShootOrigin, vecShootDir;
			vecShootOrigin = pOperator->Weapon_ShootPosition();

			CAI_BaseNPC *npc = pOperator->MyNPCPointer();
			ASSERT( npc != NULL );

			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );

			CSoundEnt::InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy() );

			WeaponSound( SINGLE_NPC );
			pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2 );
			pOperator->DoMuzzleFlash();
			m_iClip1 = m_iClip1 - 1;
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
void CWeaponDeagle::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );
	
	m_flSoonestPrimaryAttack	= gpGlobals->curtime + DEAGLE_FASTEST_DRY_REFIRE_TIME;
	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if ( !pOwner )
		return;

	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.5f )
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestPrimaryAttack = gpGlobals->curtime + DEAGLE_FASTEST_REFIRE_TIME;
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, GetOwner() );

	//Disorient the player
	QAngle angles = pOwner->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

	pOwner->SnapEyeAngles(angles);

	pOwner->ViewPunch(QAngle(-8, random->RandomFloat(-2, 2), 0));

	if( pOwner )
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += DEAGLE_ACCURACY_SHOT_PENALTY_TIME;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pOwner, true, GetClassname() );

	m_flTimeWeaponIdle = gpGlobals->curtime + 1.65;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Check our penalty time decay
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, DEAGLE_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CWeaponDeagle::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	if (m_bStartGuidingWhenReloadIsFinished)
	{
		StartGuiding();
		m_bStartGuidingWhenReloadIsFinished = false;
	}

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ( ( m_bInitialStateUpdate ) && ( GetActivity() != ACT_VM_DRAW ) )
	{
		m_bInitialStateUpdate = false;

		if (m_bWasGuidingBeforeHolster)
		{
			StartGuiding();
			m_bWasGuidingBeforeHolster = false;
		}
	}

	// Supress our guiding effects if we're lowered
	if ( GetIdealActivity() == ACT_VM_IDLE_LOWERED || GetIdealActivity() == ACT_VM_RELOAD )
	{
		SuppressGuiding();
	}
	else
	{
		SuppressGuiding( false );
	}

	//Player has toggled guidance state
	//Adrian: Players are not allowed to remove the laser guide in single player anymore, bye!
	//if ( g_pGameRules->IsMultiplayer() == true )
	//{
		if ( pOwner->m_afButtonPressed & IN_ATTACK2 )
		{
			ToggleGuiding();
		}
	//}

	//Move the laser
	UpdateLaserPosition();
	UpdateLaserEffects();

	//Allow a refire as fast as the player can click
//	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
//	{
//		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
//	}
//	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
//	{
//		DryFire();
//	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponDeagle::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//=========================================================
void CWeaponDeagle::WeaponIdle(void)
{
	//Idle again if we've finished
	if ( HasWeaponIdleTimeElapsed() )
	{
		float rand = random->RandomFloat(0, 1);

		//Fidget 1 out of 4 idle animations and only if we're not guiding
		if ((rand <= 0.25) && (!IsGuiding()))
			SendWeaponAnim( ACT_VM_FIDGET );
		else
			SendWeaponAnim( ACT_VM_IDLE );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponDeagle::Reload( void )
{
	Activity act;
	if (m_iClip1 == 0)
		act = ACT_VM_RELOAD;
	else
		act = ACT_GLOCK_SHOOT_RELOAD;

	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), act );
	if ( fRet )
	{
		if (IsGuiding())
		{
			StopGuiding();
			m_bStartGuidingWhenReloadIsFinished = true;
		}

		WeaponSound( RELOAD );
		m_flAccuracyPenalty = 0.0f;
	}
	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = random->RandomFloat( 0.25f, 0.5f );
	viewPunch.y = random->RandomFloat( -.6f, .6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::Activate( void )
{
	BaseClass::Activate();

	// Restore the laser pointer after transition
	if ( m_bGuiding )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		
		if ( pOwner == NULL )
			return;

		if ( pOwner->GetActiveWeapon() == this )
		{
			StartGuiding();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponDeagle::SuppressGuiding(bool state)
{
	m_bHideGuiding = state;

	if ( m_hLaserDot == NULL )
	{
		//StartGuiding();

		//STILL!?
		if ( m_hLaserDot == NULL )
			 return;
	}

	if ( state )
	{
		m_hLaserDot->TurnOff();
	}
	else
	{
		m_hLaserDot->TurnOn();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CWeaponDeagle::GetLaserPosition(void)
{
	CreateLaserPointer();

	if ( m_hLaserDot != NULL )
		return m_hLaserDot->GetAbsOrigin();

	//FIXME: The laser dot sprite is not active, this code should not be allowed!
	assert(0);
	return vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true if the rocket is being guided, false if it's dumb
//-----------------------------------------------------------------------------
bool CWeaponDeagle::IsGuiding(void)
{
	return m_bGuiding;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponDeagle::Deploy(void)
{
	m_bInitialStateUpdate = true;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponDeagle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	if (IsGuiding())
	{
		StopGuiding();
	}
	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the guiding laser
//-----------------------------------------------------------------------------
void CWeaponDeagle::StartGuiding(void)
{
	// Don't start back up if we're overriding this
	if (m_bHideGuiding)
		return;

	m_bGuiding = true;

	if (!m_bStartGuidingWhenReloadIsFinished)
		WeaponSound(SPECIAL1);

	CreateLaserPointer();
	StartLaserEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Turn off the guiding laser
//-----------------------------------------------------------------------------
void CWeaponDeagle::StopGuiding(void)
{
	m_bGuiding = false;

	if (!m_bStartGuidingWhenReloadIsFinished)
		WeaponSound(SPECIAL2);

	StopLaserEffects();

	// Kill the dot completely
	if (m_hLaserDot != NULL)
	{
		m_hLaserDot->TurnOff();
		UTIL_Remove(m_hLaserDot);
		m_hLaserDot = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the guiding laser
//-----------------------------------------------------------------------------
void CWeaponDeagle::ToggleGuiding(void)
{
	if (IsGuiding())
	{
		StopGuiding();
	}
	else
	{
		StartGuiding();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::Drop(const Vector &vecVelocity)
{
	StopGuiding();

	BaseClass::Drop(vecVelocity);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponDeagle::UpdateLaserPosition(Vector vecMuzzlePos, Vector vecEndPos)
{
	if (vecMuzzlePos == vec3_origin || vecEndPos == vec3_origin)
	{
		CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
		if (!pPlayer)
			return;

		vecMuzzlePos = pPlayer->Weapon_ShootPosition();
		Vector	forward;

		if (g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE)
		{
			forward = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
		}
		else
		{
			pPlayer->EyeVectors(&forward);
		}

		vecEndPos = vecMuzzlePos + (forward * MAX_TRACE_LENGTH);
	}

	//Move the laser dot, if active
	trace_t	tr;

	// Trace out for the endpoint
#ifdef PORTAL
	g_bBulletPortalTrace = true;
	Ray_t rayLaser;
	rayLaser.Init(vecMuzzlePos, vecEndPos);
	UTIL_Portal_TraceRay(rayLaser, (MASK_SHOT & ~CONTENTS_WINDOW), this, COLLISION_GROUP_NONE, &tr);
	g_bBulletPortalTrace = false;
#else
	UTIL_TraceLine(vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), this, COLLISION_GROUP_NONE, &tr);
#endif

	// Move the laser sprite
	if (m_hLaserDot != NULL)
	{
		Vector	laserPos = tr.endpos;
		m_hLaserDot->SetLaserPosition(laserPos, tr.plane.normal);

		if (tr.DidHitNonWorldEntity())
		{
			CBaseEntity *pHit = tr.m_pEnt;

			if ((pHit != NULL) && (pHit->m_takedamage))
			{
				m_hLaserDot->SetTargetEntity(pHit);
			}
			else
			{
				m_hLaserDot->SetTargetEntity(NULL);
			}
		}
		else
		{
			m_hLaserDot->SetTargetEntity(NULL);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::CreateLaserPointer(void)
{
	if (m_hLaserDot != NULL)
		return;

	m_hLaserDot = CLaserDot::Create(GetAbsOrigin(), GetOwnerEntity());
	m_hLaserDot->TurnOff();

	UpdateLaserPosition();
}

//-----------------------------------------------------------------------------
// Purpose: Start the effects on the viewmodel of the RPG
//-----------------------------------------------------------------------------
void CWeaponDeagle::StartLaserEffects(void)
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	CBaseViewModel *pBeamEnt = static_cast<CBaseViewModel *>(pOwner->GetViewModel());

	if ( m_hLaserBeam == NULL )
	{
		m_hLaserBeam = CBeam::BeamCreate( RPG_BEAM_SPRITE, 1.0f );
		
		if ( m_hLaserBeam == NULL )
		{
			// We were unable to create the beam
			Assert(0);
			return;
		}

		m_hLaserBeam->EntsInit( pBeamEnt, pBeamEnt );

		int	startAttachment = LookupAttachment( "laser" );
		int endAttachment	= LookupAttachment( "laser_end" );

		m_hLaserBeam->FollowEntity( pBeamEnt );
		m_hLaserBeam->SetStartAttachment( startAttachment );
		m_hLaserBeam->SetEndAttachment( endAttachment );
		m_hLaserBeam->SetNoise( 0 );
		m_hLaserBeam->SetColor( 255, 0, 0 );
		m_hLaserBeam->SetScrollRate( 0 );
		m_hLaserBeam->SetWidth( 0.5f );
		m_hLaserBeam->SetEndWidth( 0.5f );
		m_hLaserBeam->SetBrightness( 128 );
		m_hLaserBeam->SetBeamFlags( SF_BEAM_SHADEIN );
#ifdef PORTAL
		m_hLaserBeam->m_bDrawInMainRender = true;
		m_hLaserBeam->m_bDrawInPortalRender = false;
#endif
	}
	else
	{
		m_hLaserBeam->SetBrightness( 128 );
	}

	if ( m_hLaserMuzzleSprite == NULL )
	{
		m_hLaserMuzzleSprite = CSprite::SpriteCreate( RPG_LASER_SPRITE, GetAbsOrigin(), false );

		if ( m_hLaserMuzzleSprite == NULL )
		{
			// We were unable to create the sprite
			Assert(0);
			return;
		}

#ifdef PORTAL
		m_hLaserMuzzleSprite->m_bDrawInMainRender = true;
		m_hLaserMuzzleSprite->m_bDrawInPortalRender = false;
#endif

		m_hLaserMuzzleSprite->SetAttachment( pOwner->GetViewModel(), LookupAttachment( "laser" ) );
		m_hLaserMuzzleSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
		m_hLaserMuzzleSprite->SetBrightness( 255, 0.5f );
		m_hLaserMuzzleSprite->SetScale( 0.25f, 0.5f );
		m_hLaserMuzzleSprite->TurnOn();
	}
	else
	{
		m_hLaserMuzzleSprite->TurnOn();
		m_hLaserMuzzleSprite->SetScale( 0.25f, 0.25f );
		m_hLaserMuzzleSprite->SetBrightness( 255 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stop the effects on the viewmodel of the RPG
//-----------------------------------------------------------------------------
void CWeaponDeagle::StopLaserEffects(void)
{
	if ( m_hLaserBeam != NULL )
	{
		m_hLaserBeam->SetBrightness( 0 );
	}
	
	if ( m_hLaserMuzzleSprite != NULL )
	{
		m_hLaserMuzzleSprite->SetScale( 0.01f );
		m_hLaserMuzzleSprite->SetBrightness( 0, 0.5f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pulse all the effects to make them more... well, laser-like
//-----------------------------------------------------------------------------
void CWeaponDeagle::UpdateLaserEffects(void)
{
	if ( !m_bGuiding )
		return;

	if ( m_hLaserBeam != NULL )
	{
		m_hLaserBeam->SetBrightness( 128 + random->RandomInt( -8, 8 ) );
	}

	if ( m_hLaserMuzzleSprite != NULL )
	{
		m_hLaserMuzzleSprite->SetScale( 0.1f + random->RandomFloat( -0.025f, 0.025f ) );
	}
}
