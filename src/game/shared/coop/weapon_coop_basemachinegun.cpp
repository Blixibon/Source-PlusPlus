//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "weapon_coop_basemachinegun.h"

#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "hl2_gamerules.h"
#include "hl2_player_shared.h"

#ifdef CLIENT_DLL
    #include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	//#include "c_coop_player.h"
	#include "hud_crosshair.h"
#else
   //#include "coop_player.h"
	#include "vphysics/constraints.h"
    #include "ilagcompensationmanager.h"
#endif

//================================================================================
// Comandos
//================================================================================



//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCoopMachineGun, DT_WeaponCoopMachineGun );

BEGIN_NETWORK_TABLE_NOBASE(CWeaponCoopMachineGun, DT_WeaponCoopMachineGunLocalData)
#ifndef CLIENT_DLL
SendPropInt(SENDINFO(m_nShotsFired)),
#else
RecvPropInt(RECVINFO(m_nShotsFired)),
#endif // !CLIENT_DLL
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CWeaponCoopMachineGun, DT_WeaponCoopMachineGun)
#ifndef CLIENT_DLL
SendPropDataTable("CoopMachineGunLocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_WeaponCoopMachineGunLocalData), SendProxy_SendLocalWeaponDataTable),
#else
RecvPropDataTable("CoopMachineGunLocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_WeaponCoopMachineGunLocalData)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponCoopMachineGun)
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_nShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
#endif // CLIENT_DLL
END_PREDICTION_DATA()

#ifndef CLIENT_DLL
#include "globalstate.h"
#endif

BEGIN_DATADESC(CWeaponCoopMachineGun)
DEFINE_FIELD(m_nShotsFired, FIELD_INTEGER),
DEFINE_FIELD(m_flNextSoundTime, FIELD_TIME),
#ifndef CLIENT_DLL
DEFINE_FIELD(m_flTimeLastNPCFired, FIELD_TIME),
#endif // !CLIENT_DLL
END_DATADESC();

//================================================================================
//================================================================================
void CWeaponCoopMachineGun::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Debounce the recoiling counter
	if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
	{
		m_nShotsFired = 0;
	}

	BaseClass::ItemPostFrame();
}

//================================================================================
//================================================================================
bool CWeaponCoopMachineGun::Deploy( void )
{
	m_nShotsFired = 0;
	return BaseClass::Deploy();
}

//================================================================================
//================================================================================
const Vector &CWeaponCoopMachineGun::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_3DEGREES;
	return cone;
}

//================================================================================
//================================================================================
int CWeaponCoopMachineGun::WeaponSoundRealtime( WeaponSound_t shoot_type )
{
#ifndef CLIENT_DLL
	int numBullets = 0;

	// ran out of time, clamp to current
	if (m_flNextSoundTime < gpGlobals->curtime)
	{
		m_flNextSoundTime = gpGlobals->curtime;
	}

	// make enough sound events to fill up the next estimated think interval
	float dt = clamp( m_flAnimTime - m_flPrevAnimTime, 0, 0.2 );
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}

	return numBullets;
#else
    return 0;
#endif
}

void CWeaponCoopMachineGun::UpdateNPCShotCounter()
{
#ifndef CLIENT_DLL
	if (gpGlobals->curtime - m_flTimeLastNPCFired >= GetMinRestTime())
	{
		m_nShotsFired = 0;
	}
	else
	{
		m_nShotsFired++;
	}

	m_flTimeLastNPCFired = gpGlobals->curtime;
#endif
}

extern void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip );

//================================================================================
//================================================================================
void CWeaponCoopMachineGun::DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime )
{
	#define	KICK_MIN_X			0.2f	//Degrees
	#define	KICK_MIN_Y			0.2f	//Degrees
	#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	
	//Find how far into our accuracy degradation we are
	float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset( 10 );

	//Apply this to the view angles as well
	vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
	vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
	vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

	RandomSeed( iSeed );

	//Wibble left and right
	if ( RandomInt( -1, 1 ) >= 0 )
		vecScratch.y *= -1;

	iSeed++;

	//Wobble up and down
	if ( RandomInt( -1, 1 ) >= 0 )
		vecScratch.z *= -1;

	//Clip this to our desired min/max
	UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );

	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch( vecScratch * 0.5 );
}

void CWeaponCoopMachineGun::PrimaryAttack(void)
{
	// If my clip is empty (and I use clips) start reload
	if (UsesClipsForAmmo1() && !m_iClip1)
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CHL2_Player* pPlayer = ToHL2Player(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	m_nShotsFired++;

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(GetPrimaryAttackActivity());

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	FireBulletsInfo_t info;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = static_cast<CBasePlayer*>(pPlayer)->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while (m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if (!fireRate)
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if (UsesClipsForAmmo1())
	{
		info.m_iShots = MIN(info.m_iShots, m_iClip1.Get());
		EmitLowAmmoSound(info.m_iShots);
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN(info.m_iShots, pPlayer->GetAmmoCount(m_iPrimaryAmmoType));
		pPlayer->RemoveAmmo(info.m_iShots, m_iPrimaryAmmoType);
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread(this);
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL

	int iRumblue = GetRumbleEffect();
	if (iRumblue > 0)
		RumbleEffect(iRumblue, 0, RUMBLE_FLAGS_NONE);

	pPlayer->FireBullets(info);

	m_iPrimaryAttacks++;

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	//Add our view kick in
	AddViewKick();
}