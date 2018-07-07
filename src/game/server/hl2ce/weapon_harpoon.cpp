//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Crowbar - an old favorite
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "basebludgeonweapon.h"
#include "vstdlib/random.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "weapon_crossbow.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar    sk_plr_dmg_harpoon		( "sk_plr_dmg_harpoon","20");
ConVar    sk_npc_dmg_harpoon		( "sk_npc_dmg_harpoon","20");

#define	HARPOON_RANGE	75.0f
#define	HARPOON_REFIRE	0.4f

//-----------------------------------------------------------------------------
// CWeaponHarpoon
//-----------------------------------------------------------------------------

class CWeaponHarpoon : public CBaseHLBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponHarpoon, CBaseHLBludgeonWeapon );

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CWeaponHarpoon();

	float		GetRange( void )		{	return	HARPOON_RANGE;	}
	float		GetFireRate( void )		{	return	HARPOON_REFIRE;	}
	
	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );

	virtual int WeaponMeleeAttack1Condition( float flDot, float flDist );
	void		SecondaryAttack( void );
	virtual Activity	GetPrimaryAttackActivity( void )	{	return	ACT_VM_PRIMARYATTACK;	}
	virtual Activity	GetPrimaryAttackMissActivity( void )	{	return	ACT_VM_PRIMARYATTACK;	}
	
	bool Reload();

	// Animation event
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
};

//-----------------------------------------------------------------------------
// CWeaponHarpoon
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CWeaponHarpoon, DT_WeaponHarpoon)
END_SEND_TABLE()

#ifndef HL2MP
LINK_ENTITY_TO_CLASS( weapon_oldmanharpoon, CWeaponHarpoon );
PRECACHE_WEAPON_REGISTER( weapon_oldmanharpoon );
#endif

acttable_t CWeaponHarpoon::m_acttable[] = 
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },

	{ ACT_MP_STAND_IDLE,				ACT_DOD_STAND_AIM_SPADE,				false },
	{ ACT_MP_CROUCH_IDLE,				ACT_DOD_CROUCH_AIM_SPADE,				false },
	{ ACT_MP_RUN,						ACT_DOD_WALK_AIM_SPADE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_DOD_CROUCHWALK_AIM_SPADE,			false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_DOD_PRIMARYATTACK_SPADE,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_DOD_PRIMARYATTACK_SPADE,			false },
	//{ ACT_MP_RELOAD_STAND,				ACT_DOD_RELOAD_PISTOL,					false },
	//{ ACT_MP_RELOAD_CROUCH,				ACT_DOD_RELOAD_CROUCH_PISTOL,			false },
	{ ACT_MP_JUMP,						ACT_MP_JUMP,							false },

};

IMPLEMENT_ACTTABLE(CWeaponHarpoon);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponHarpoon::CWeaponHarpoon( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CWeaponHarpoon::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_harpoon.GetFloat();

	return sk_npc_dmg_harpoon.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponHarpoon::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}


//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
extern ConVar sk_crowbar_lead_time;

int CWeaponHarpoon::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC *pNPC	= GetOwner()->MyNPCPointer();
	CBaseEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity( );

	// Project where the enemy will be in a little while
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );
	if ((flDist > 64) && (flExtrapolatedDist > 64))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponHarpoon::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	CBaseEntity *pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if ( pEnemy )
	{
		Vector vecDelta;
		VectorSubtract( pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta );
		VectorNormalize( vecDelta );
		
		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize( vecDelta2D );
		if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
		{
			vecDirection = vecDelta;
		}
	}

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
		Vector(-16,-16,-16), Vector(36,36,36), sk_npc_dmg_harpoon.GetFloat(), DMG_CLUB, 0.75 );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponHarpoon::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

bool CWeaponHarpoon::Reload()
{
	return DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_DRAW);
}

//-----------------------------------------------------------------------------
// Harpoon throw!
//-----------------------------------------------------------------------------
void CWeaponHarpoon::SecondaryAttack()
{
	Msg("TODO: Fix harpoon throwing\n");
	m_flNextSecondaryAttack = 0.15;
	return;

	/*
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	if ( m_iPrimaryAmmoType <= 0 )
	{
		WeaponSound( EMPTY );
		ClientPrint( pOwner, HUD_PRINTCENTER, "#Valve_Hint_Harpoon_CantThrowLast" );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.15;
		return;
	}

	if (m_iClip1 <= 0)
	{
		if (!m_bFireOnEmpty)
		{
			Reload();
		}
		else
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.15;
		}

		return;
	}

	pOwner->RumbleEffect(RUMBLE_357, 0, RUMBLE_FLAG_RESTART);

	Vector vecAiming = pOwner->GetAutoaimVector(0);
	Vector vecSrc = pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles(vecAiming, angAiming);

#if defined(HL2_EPISODIC)
	// !!!HACK - the other piece of the Alyx crossbow bolt hack for Outland_10 (see ::BoltTouch() for more detail)
	if (FStrEq(STRING(gpGlobals->mapname), "ep2_outland_10"))
	{
		trace_t tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 24.0f, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr);

		if (tr.m_pEnt != NULL && tr.m_pEnt->Classify() == CLASS_PLAYER_ALLY_VITAL)
		{
			// If Alyx is right in front of the player, make sure the bolt starts outside of the player's BBOX, or the bolt
			// will instantly collide with the player after the owner of the bolt is switched to Alyx in ::BoltTouch(). We 
			// avoid this altogether by making it impossible for the bolt to collide with the player.
			vecSrc += vecAiming * 24.0f;
		}
	}
#endif

	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate(vecSrc, angAiming, pOwner, true);

	if (pOwner->GetWaterLevel() == 3)
	{
		pBolt->SetAbsVelocity(vecAiming * BOLT_WATER_VELOCITY);
	}
	else
	{
		pBolt->SetAbsVelocity(vecAiming * BOLT_AIR_VELOCITY);
	}

	m_iClip1--;

	pOwner->ViewPunch(QAngle(-2, 0, 0));

	WeaponSound(SINGLE);
	//WeaponSound(SPECIAL2);

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 200, 0.2);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	if (!m_iClip1 && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;
	*/
}
