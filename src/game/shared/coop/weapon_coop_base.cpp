//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "weapon_coop_base.h"

#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "hl2_gamerules.h"
#include "hlss_weapon_id.h"

#ifdef CLIENT_DLL
extern IVModelInfoClient* modelinfo;
#else
extern IVModelInfo* modelinfo;
#endif

#ifdef CLIENT_DLL
    #include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	#include "hud_crosshair.h"
#else
	#include "vphysics/constraints.h"
    #include "ilagcompensationmanager.h"
#endif

#include "hl2_player_shared.h"

//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCoopBase, DT_WeaponCoopBase );

BEGIN_NETWORK_TABLE( CWeaponCoopBase, DT_WeaponCoopBase )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCoopBase ) 
END_PREDICTION_DATA()

BEGIN_DATADESC( CWeaponCoopBase )
END_DATADESC()

//================================================================================
// Constructor
//================================================================================
CWeaponCoopBase::CWeaponCoopBase()
{
    SetPredictionEligible( true );
    AddSolidFlags( FSOLID_TRIGGER );
}

#ifdef CLIENT_DLL
//================================================================================
//================================================================================
void CWeaponCoopBase::OnDataChanged( DataUpdateType_t type ) 
{
    BaseClass::OnDataChanged( type );

    if ( GetPredictable() && !ShouldPredict() )
        ShutdownPredictable();
}

//================================================================================
//================================================================================
bool CWeaponCoopBase::ShouldPredict() 
{
    if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
		return true;

	return BaseClass::ShouldPredict();
}
#endif

//================================================================================
//================================================================================
void CWeaponCoopBase::Spawn() 
{
    // Baseclass
    BaseClass::Spawn();

    // Set this here to allow players to shoot dropped weapons
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

    BaseClass::ObjectCaps();
}

//================================================================================
//================================================================================
void CWeaponCoopBase::WeaponSound( WeaponSound_t sound_type, float soundtime ) 
{
#ifdef CLIENT_DLL
		// If we have some sounds from the weapon classname.txt file, play a random one of them
		const char *shootsound = GetShootSound( sound_type );

		if ( !shootsound || !shootsound[0] )
			return;

		CBroadcastRecipientFilter filter; // this is client side only

		if ( !te->CanPredict() )
			return;
				
		CBaseEntity::EmitSound( filter, GetOwner()->entindex(), shootsound, &GetOwner()->GetAbsOrigin() ); 
#else
		BaseClass::WeaponSound( sound_type, soundtime );
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CWeaponCoopBase::GetWeaponID(void) const
{
	Assert(false);
	return HLSS_WEAPON_ID_NONE;
}

//================================================================================
//================================================================================
void CWeaponCoopBase::PrimaryAttack() 
{
    // If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CHL2_Player *pPlayer = ToHL2Player( GetOwner() );

	if (!pPlayer)
	{
		return;
	}

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition();	
	info.m_vecDirShooting = static_cast<CBasePlayer *>(pPlayer)->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = MIN( info.m_iShots, m_iClip1.Get() );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL



	pPlayer->FireBullets( info );



	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	//Add our view kick in
	AddViewKick();
}

//================================================================================
//================================================================================
#ifdef CLIENT_DLL
void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip )
{
	QAngle	final = in + punch;

	//Clip each component
	for ( int i = 0; i < 3; i++ )
	{
		if ( final[i] > clip[i] )
		{
			final[i] = clip[i];
		}
		else if ( final[i] < -clip[i] )
		{
			final[i] = -clip[i];
		}

		//Return the result
		in[i] = final[i] - punch[i];
	}
}
#endif

ConVar mp_forceactivityset("mp_forceactivityset", "-1", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY);

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CWeaponCoopBase::GetActivityWeaponRole(void)
{
	int iWeaponRole = TF_WPN_TYPE_PRIMARY;

	switch (GetWeaponID())
	{
	case HLSS_WEAPON_ID_SMG1:
	case HLSS_WEAPON_ID_SMG2:
	case HLSS_WEAPON_ID_AR2:
	case HLSS_WEAPON_ID_MG1:
	case HLSS_WEAPON_ID_SHOTGUN:
	default:
		iWeaponRole = TF_WPN_TYPE_PRIMARY;
		break;
	case HLSS_WEAPON_ID_PISTOL:
	case HLSS_WEAPON_ID_357:
	case HLSS_WEAPON_ID_ALYXGUN:
	case HLSS_WEAPON_ID_DEAGLE:
		iWeaponRole = TF_WPN_TYPE_SECONDARY;
		break;
	case HLSS_WEAPON_ID_CROSSBOW:
	case HLSS_WEAPON_ID_RPG:
	case HLSS_WEAPON_ID_SNIPER:
	case HLSS_WEAPON_ID_EGON:
	case HLSS_WEAPON_ID_GAUSS:
		iWeaponRole = TF_WPN_TYPE_PRIMARY2;
		break;
	case HLSS_WEAPON_ID_TURRET:
		iWeaponRole = TF_WPN_TYPE_BUILDING;
		break;
	case HLSS_WEAPON_ID_PHYSGUN:
	case HLSS_WEAPON_ID_HIVEHAND:
		iWeaponRole = TF_WPN_TYPE_ITEM1;
		break;
	case HLSS_WEAPON_ID_SLAM:
	case HLSS_WEAPON_ID_SATCHEL:
	case HLSS_WEAPON_ID_TRIPMINE:
		iWeaponRole = TF_WPN_TYPE_ITEM2;
		break;
	case HLSS_WEAPON_ID_BUGBAIT:
	case HLSS_WEAPON_ID_FRAG:
	case HLSS_WEAPON_ID_SNARK:
		iWeaponRole = TF_WPN_TYPE_GRENADE;
		break;
	case HLSS_WEAPON_ID_MEDKIT:
		iWeaponRole = TF_WPN_TYPE_PDA;
		break;
	case HLSS_WEAPON_ID_CROWBAR:
	case HLSS_WEAPON_ID_STUNSTICK:
	case HLSS_WEAPON_ID_LEADPIPE:
		iWeaponRole = TF_WPN_TYPE_MELEE;
		break;
	}

	if (HasItemDefinition())
	{
		int iSchemaRole = m_Item.GetAnimationSlot();
		if (iSchemaRole >= 0)
			iWeaponRole = iSchemaRole;
	}

	if (mp_forceactivityset.GetInt() >= 0)
	{
		iWeaponRole = mp_forceactivityset.GetInt();
	}

	return iWeaponRole;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CWeaponCoopBase::GetActivityWeaponVariant(void)
{
	int iWeaponVariant = 0;

	switch (GetWeaponID())
	{
	case HLSS_WEAPON_ID_PISTOL:
	case HLSS_WEAPON_ID_ALYXGUN:
	case HLSS_WEAPON_ID_STUNSTICK:
	case HLSS_WEAPON_ID_CROWBAR:
	case HLSS_WEAPON_ID_MEDKIT:
	case HLSS_WEAPON_ID_SLAM:
	case HLSS_WEAPON_ID_PHYSGUN:
	case HLSS_WEAPON_ID_TURRET:
	case HLSS_WEAPON_ID_BUGBAIT:
	case HLSS_WEAPON_ID_FRAG:
	case HLSS_WEAPON_ID_RPG:
	case HLSS_WEAPON_ID_SMG1:
	case HLSS_WEAPON_ID_SMG2:
	case HLSS_WEAPON_ID_LEADPIPE:
	default:
		iWeaponVariant = 0;
		break;
	case HLSS_WEAPON_ID_SHOTGUN:
	case HLSS_WEAPON_ID_SNIPER:
	case HLSS_WEAPON_ID_357:
	case HLSS_WEAPON_ID_SNARK:
	case HLSS_WEAPON_ID_HIVEHAND:
	case HLSS_WEAPON_ID_TRIPMINE:
		iWeaponVariant = 1;
		break;
	case HLSS_WEAPON_ID_AR2:
	case HLSS_WEAPON_ID_MG1:
	case HLSS_WEAPON_ID_CROSSBOW:
	case HLSS_WEAPON_ID_SATCHEL:
	case HLSS_WEAPON_ID_DEAGLE:
		iWeaponVariant = 2;
		break;
	case HLSS_WEAPON_ID_EGON:
		iWeaponVariant = 3;
		break;
	case HLSS_WEAPON_ID_GAUSS:
		iWeaponVariant = 4;
		break;
	}

	if (HasItemDefinition())
	{
		int iSchemaRole = m_Item.GetAnimationVariant();
		if (iSchemaRole >= 0)
			iWeaponVariant = iSchemaRole;
	}

	return iWeaponVariant;
}

acttable_t CWeaponCoopBase::s_acttableSMG1[] =
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
		{ ACT_RUN,						ACT_RUN_RIFLE,					true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
		{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
		{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },

		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

		{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SMG1,					false },
		{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SMG1,						false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SMG1,				false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SMG1,				false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SMG1,			false },
		{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SMG1,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_SMG1,					false},
		{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SMG1,					false },
};
acttable_t CWeaponCoopBase::s_acttablePistol[] =
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

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_PISTOL,					false},
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};
acttable_t CWeaponCoopBase::s_acttableMelee[] =
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },

	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_MELEE,					false},
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE,					false },
};
acttable_t CWeaponCoopBase::s_acttableCrossbow[] =
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
		{ ACT_RUN,						ACT_RUN_RIFLE,						true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR1,	true },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_AR2_LOW,		true },
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,				false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,			false },

		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_CROSSBOW,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_CROSSBOW,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_CROSSBOW,				false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_CROSSBOW,				false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_CROSSBOW,			false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_CROSSBOW,					false},
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_CROSSBOW,					false },
};
acttable_t CWeaponCoopBase::s_acttableShotgun[] =
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
		{ ACT_RUN,						ACT_RUN_RIFLE,						true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
	
		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

		{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SHOTGUN,					false },
		{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SHOTGUN,					false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
		{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_SHOTGUN,					false},
		{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
};
acttable_t CWeaponCoopBase::s_acttableGrenade[] =
{
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_THROW,					true },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE,		false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE,	false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY,		false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN,				false },//always aims

	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_GRENADE,					false},
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_GRENADE,					false },
};
acttable_t CWeaponCoopBase::s_acttableRPG[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_RPG, true },
	{ ACT_IDLE_RELAXED,				ACT_IDLE_RPG_RELAXED,			true },
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_ANGRY_RPG,				true },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_RPG,				true },
	{ ACT_IDLE,						ACT_IDLE_RPG,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_RPG,				true },
	{ ACT_WALK,						ACT_WALK_RPG,					true },
	{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RPG,			true },
	{ ACT_RUN,						ACT_RUN_RPG,					true },
	{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RPG,				true },
	{ ACT_COVER_LOW,				ACT_COVER_LOW_RPG,				true },

	{ ACT_ARM,						ACT_ARM_RIFLE,					false},
	{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_RPG,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_RPG,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_RPG,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_RPG,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_RPG,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_RPG,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_RPG,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_RPG,					false},
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_RPG,				false },
};
acttable_t CWeaponCoopBase::s_acttablePhysgun[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PHYSGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PHYSGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PHYSGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PHYSGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_PHYSGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PHYSGUN, false },
	{ ACT_HL2MP_SWIM, ACT_HL2MP_SWIM_PHYSGUN, false },
	{ ACT_HL2MP_WALK, ACT_HL2MP_WALK_PHYSGUN, false },
};
acttable_t CWeaponCoopBase::s_acttableSlam[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SLAM,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SLAM,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SLAM,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SLAM,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SLAM,		false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_SLAM,					false},
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SLAM,					false },
};
acttable_t CWeaponCoopBase::s_acttableMelee2[] =
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },

	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_MELEE2,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_MELEE2,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE2,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE2,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE2,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE2,			false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE2,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_MELEE2,					false},
};
acttable_t CWeaponCoopBase::s_acttablePython[] =
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

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_REVOLVER,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_REVOLVER,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_REVOLVER,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_REVOLVER,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_REVOLVER,					false},
};
acttable_t CWeaponCoopBase::s_acttableAR2[] =
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
		{ ACT_RUN,						ACT_RUN_RIFLE,					true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
		{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
		{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
		//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },

		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

			{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,					false },
			{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,					false },
			{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,			false },
			{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,			false },
			{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
			{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,		false },
			{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_AR2,					false},
			{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_AR2,				false },
};
// BMS
acttable_t CWeaponCoopBase::s_acttableMP5[] =
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
		{ ACT_RUN,						ACT_RUN_RIFLE,					true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
		{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
		{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },

		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

		{ ACT_HL2MP_IDLE,					ACT_BMMP_IDLE_MP5,					false },
		{ ACT_HL2MP_RUN,					ACT_BMMP_RUN_MP5,						false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_BMMP_IDLE_CROUCH_MP5,				false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_BMMP_WALK_CROUCH_MP5,				false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_BMMP_GESTURE_RANGE_ATTACK_MP5,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_BMMP_GESTURE_RELOAD_MP5,			false },
		{ ACT_HL2MP_JUMP,					ACT_BMMP_JUMP_START_MP5,					false },
		{ACT_HL2MP_SWIM,					ACT_BMMP_SWIM_MP5,					false},
		{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SMG1,					false },
};
acttable_t CWeaponCoopBase::s_acttableGlock[] =
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

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

	{ ACT_HL2MP_IDLE,					ACT_BMMP_IDLE_GLOCK,					false },
	{ ACT_HL2MP_RUN,					ACT_BMMP_RUN_GLOCK,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_BMMP_IDLE_CROUCH_GLOCK,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_BMMP_WALK_CROUCH_GLOCK,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_BMMP_GESTURE_RANGE_ATTACK_GLOCK,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_BMMP_GESTURE_RELOAD_GLOCK,		false },
	{ ACT_HL2MP_JUMP,					ACT_BMMP_JUMP_START_GLOCK,					false },
		{ACT_HL2MP_SWIM,					ACT_BMMP_SWIM_GLOCK,					false},
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};
acttable_t CWeaponCoopBase::s_acttableTau[] =
{
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
	{ ACT_WALK,						ACT_WALK_RIFLE,						true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
		{ ACT_RUN,						ACT_RUN_RIFLE,						true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },

		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

		{ ACT_HL2MP_IDLE,					ACT_BMMP_IDLE_TAU,					false },
		{ ACT_HL2MP_RUN,					ACT_BMMP_RUN_TAU,					false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_BMMP_IDLE_CROUCH_TAU,			false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_BMMP_WALK_CROUCH_TAU,			false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_BMMP_GESTURE_RANGE_ATTACK_TAU,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_BMMP_GESTURE_RELOAD_TAU,		false },
		{ ACT_HL2MP_JUMP,					ACT_BMMP_JUMP_START_TAU,					false },
		{ACT_HL2MP_SWIM,					ACT_BMMP_SWIM_TAU,					false},
		{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
};
acttable_t CWeaponCoopBase::s_acttableGluon[] =
{
	{ ACT_HL2MP_IDLE, ACT_BMMP_IDLE_GLUON, false },
	{ ACT_HL2MP_RUN, ACT_BMMP_RUN_GLUON, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_BMMP_IDLE_CROUCH_GLUON, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_BMMP_WALK_CROUCH_GLUON, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_BMMP_GESTURE_RANGE_ATTACK_GLUON, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_BMMP_GESTURE_RELOAD_GLUON, false },
	{ ACT_HL2MP_JUMP, ACT_BMMP_JUMP_START_GLUON, false },
	{ ACT_HL2MP_SWIM, ACT_BMMP_SWIM_GLUON, false },
	{ ACT_HL2MP_WALK, ACT_BMMP_RUN_GLUON, false },
};
acttable_t CWeaponCoopBase::s_acttableHiveHand[] =
{
	{ ACT_HL2MP_IDLE, ACT_BMMP_IDLE_HIVEHAND, false },
	{ ACT_HL2MP_RUN, ACT_BMMP_RUN_HIVEHAND, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_BMMP_IDLE_CROUCH_HIVEHAND, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_BMMP_WALK_CROUCH_HIVEHAND, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_BMMP_GESTURE_RANGE_ATTACK_HIVEHAND, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_BMMP_GESTURE_RELOAD_HIVEHAND, false },
	{ ACT_HL2MP_JUMP, ACT_BMMP_JUMP_START_HIVEHAND, false },
	{ ACT_HL2MP_SWIM, ACT_BMMP_SWIM_HIVEHAND, false },
	{ ACT_HL2MP_WALK, ACT_BMMP_RUN_HIVEHAND, false },
};
acttable_t CWeaponCoopBase::s_acttableSatchel[] =
{
	{ ACT_HL2MP_IDLE, ACT_BMMP_IDLE_SATCHEL, false },
	{ ACT_HL2MP_RUN, ACT_BMMP_RUN_SATCHEL, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_BMMP_IDLE_CROUCH_SATCHEL, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_BMMP_WALK_CROUCH_SATCHEL, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_BMMP_GESTURE_RANGE_ATTACK_SATCHEL, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_BMMP_GESTURE_RELOAD_SATCHEL, false },
	{ ACT_HL2MP_JUMP, ACT_BMMP_JUMP_START_SATCHEL, false },
	{ ACT_HL2MP_SWIM, ACT_BMMP_SWIM_SATCHEL, false },
	{ ACT_HL2MP_WALK, ACT_BMMP_RUN_SATCHEL, false },
};
acttable_t CWeaponCoopBase::s_acttableSnark[] =
{
	{ ACT_HL2MP_IDLE, ACT_BMMP_IDLE_SNARK, false },
	{ ACT_HL2MP_RUN, ACT_BMMP_RUN_SNARK, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_BMMP_IDLE_CROUCH_SNARK, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_BMMP_WALK_CROUCH_SNARK, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_BMMP_GESTURE_RANGE_ATTACK_SNARK, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_BMMP_GESTURE_RELOAD_SNARK, false },
	{ ACT_HL2MP_JUMP, ACT_BMMP_JUMP_START_SNARK, false },
	{ ACT_HL2MP_SWIM, ACT_BMMP_SWIM_SNARK, false },
	{ ACT_HL2MP_WALK, ACT_BMMP_RUN_SNARK, false },
};
acttable_t CWeaponCoopBase::s_acttableTripmine[] =
{
	{ ACT_HL2MP_IDLE, ACT_BMMP_IDLE_TRIPMINE, false },
	{ ACT_HL2MP_RUN, ACT_BMMP_RUN_TRIPMINE, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_BMMP_IDLE_CROUCH_TRIPMINE, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_BMMP_WALK_CROUCH_TRIPMINE, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_BMMP_GESTURE_RANGE_ATTACK_TRIPMINE, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_BMMP_GESTURE_RELOAD_TRIPMINE, false },
	{ ACT_HL2MP_JUMP, ACT_BMMP_JUMP_START_TRIPMINE, false },
	{ ACT_HL2MP_SWIM, ACT_BMMP_SWIM_TRIPMINE, false },
	{ ACT_HL2MP_WALK, ACT_BMMP_RUN_TRIPMINE, false },
};
// Lazul
acttable_t CWeaponCoopBase::s_acttableDeagle[] =
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

	// Readiness activities (not aiming)
	{ ACT_IDLE_RELAXED,				ACT_IDLE_PISTOL,				false },//never aims
	{ ACT_IDLE_STIMULATED,			ACT_IDLE_STIMULATED,			false },
	{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_STEALTH,				ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_RELAXED,				ACT_WALK,						false },//never aims
	{ ACT_WALK_STIMULATED,			ACT_WALK_STIMULATED,			false },
	{ ACT_WALK_AGITATED,			ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_STEALTH,				ACT_WALK_STEALTH_PISTOL,		false },

	{ ACT_RUN_RELAXED,				ACT_RUN,						false },//never aims
	{ ACT_RUN_STIMULATED,			ACT_RUN_STIMULATED,				false },
	{ ACT_RUN_AGITATED,				ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_STEALTH,				ACT_RUN_STEALTH_PISTOL,			false },

	// Readiness activities (aiming)
	{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_PISTOL,				false },//never aims	
	{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_PISTOL,			false },
	{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_PISTOL,			false },//always aims
	{ ACT_IDLE_AIM_STEALTH,			ACT_IDLE_STEALTH_PISTOL,		false },

	{ ACT_WALK_AIM_RELAXED,			ACT_WALK,						false },//never aims
	{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_PISTOL,			false },
	{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_PISTOL,			false },//always aims
	{ ACT_WALK_AIM_STEALTH,			ACT_WALK_AIM_STEALTH_PISTOL,	false },//always aims

	{ ACT_RUN_AIM_RELAXED,			ACT_RUN,						false },//never aims
	{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_PISTOL,				false },
	{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_PISTOL,				false },//always aims
	{ ACT_RUN_AIM_STEALTH,			ACT_RUN_AIM_STEALTH_PISTOL,		false },//always aims
	//End readiness activities

	// Crouch activities
	{ ACT_CROUCHIDLE_STIMULATED,	ACT_CROUCHIDLE_STIMULATED,		false },
	{ ACT_CROUCHIDLE_AIM_STIMULATED,ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims
	{ ACT_CROUCHIDLE_AGITATED,		ACT_RANGE_AIM_PISTOL_LOW,		false },//always aims

	// Readiness translations
	{ ACT_READINESS_RELAXED_TO_STIMULATED, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED, false },
	{ ACT_READINESS_RELAXED_TO_STIMULATED_WALK, ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK, false },
	{ ACT_READINESS_AGITATED_TO_STIMULATED, ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED, false },
	{ ACT_READINESS_STIMULATED_TO_RELAXED, ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED, false },

	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_REVOLVER,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_REVOLVER,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_REVOLVER,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_REVOLVER,					false},
};

acttable_t *CWeaponCoopBase::ActivityList(int &iActivityCount)
{
	int iWeaponRole = GetActivityWeaponRole();
	int iWeaponVariant = GetActivityWeaponVariant();

	acttable_t *pTable;

	switch (iWeaponRole)
	{
	case TF_WPN_TYPE_PRIMARY:
	default:
	{
		switch (iWeaponVariant)
		{
		case 0:
		default:
			pTable = s_acttableSMG1;
			iActivityCount = ARRAYSIZE(s_acttableSMG1);
			break;
		case 1:
			pTable = s_acttableShotgun;
			iActivityCount = ARRAYSIZE(s_acttableShotgun);
			break;
		case 2:
			pTable = s_acttableAR2;
			iActivityCount = ARRAYSIZE(s_acttableAR2);
			break;
		case 3:
			pTable = s_acttableMP5;
			iActivityCount = ARRAYSIZE(s_acttableMP5);
			break;
		}
	}
		break;
	case TF_WPN_TYPE_SECONDARY:
		switch (iWeaponVariant)
		{
		case 0:
		default:
			pTable = s_acttablePistol;
			iActivityCount = ARRAYSIZE(s_acttablePistol);
			break;
		case 1:
			pTable = s_acttablePython;
			iActivityCount = ARRAYSIZE(s_acttablePython);
			break;
		case 2:
			pTable = s_acttableDeagle;
			iActivityCount = ARRAYSIZE(s_acttableDeagle);
			break;
		}
		break;
	case TF_WPN_TYPE_MELEE:
		pTable = s_acttableMelee;
		iActivityCount = ARRAYSIZE(s_acttableMelee);
		break;
	case TF_WPN_TYPE_GRENADE:
		switch (iWeaponVariant)
		{
		default:
		case 0:
			pTable = s_acttableGrenade;
			iActivityCount = ARRAYSIZE(s_acttableGrenade);
			break;
		case 1:
			pTable = s_acttableSnark;
			iActivityCount = ARRAYSIZE(s_acttableSnark);
			break;
		}
		break;
	/*case TF_WPN_TYPE_BUILDING:
		pTable = s_acttableBuilding;
		iActivityCount = ARRAYSIZE(s_acttableBuilding);
		break;*/
	case TF_WPN_TYPE_PDA:
		pTable = s_acttableSlam;
		iActivityCount = ARRAYSIZE(s_acttableSlam);
		break;
	case TF_WPN_TYPE_ITEM1:
		switch (iWeaponVariant)
		{
		default:
		case 0:
			pTable = s_acttablePhysgun;
			iActivityCount = ARRAYSIZE(s_acttablePhysgun);
			break;
		case 1:
			pTable = s_acttableHiveHand;
			iActivityCount = ARRAYSIZE(s_acttableHiveHand);
			break;
		}
		break;
	case TF_WPN_TYPE_ITEM2:
		switch (iWeaponVariant)
		{
		default:
		case 0:
			pTable = s_acttableSlam;
			iActivityCount = ARRAYSIZE(s_acttableSlam);
			break;
		case 1:
			pTable = s_acttableTripmine;
			iActivityCount = ARRAYSIZE(s_acttableTripmine);
			break;
		case 2:
			pTable = s_acttableSatchel;
			iActivityCount = ARRAYSIZE(s_acttableSatchel);
			break;
		}
		break;
	case TF_WPN_TYPE_MELEE_ALLCLASS:
		pTable = s_acttableMelee2;
		iActivityCount = ARRAYSIZE(s_acttableMelee2);
		break;
	/*case TF_WPN_TYPE_SECONDARY2:
		pTable = s_acttableSecondary2;
		iActivityCount = ARRAYSIZE(s_acttableSecondary2);
		break;*/
	case TF_WPN_TYPE_PRIMARY2:
		switch (iWeaponVariant)
		{
		case 0:
		default:
			pTable = s_acttableRPG;
			iActivityCount = ARRAYSIZE(s_acttableRPG);
			break;
		/*case 1:
			pTable = s_acttableShotgun;
			iActivityCount = ARRAYSIZE(s_acttableShotgun);
			break;*/
		case 2:
			pTable = s_acttableCrossbow;
			iActivityCount = ARRAYSIZE(s_acttableCrossbow);
			break;
		case 3:
			pTable = s_acttableGluon;
			iActivityCount = ARRAYSIZE(s_acttableGluon);
			break;
		case 4:
			pTable = s_acttableTau;
			iActivityCount = ARRAYSIZE(s_acttableTau);
			break;
		}
		break;
	}

	return pTable;
}