//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "weapon_coop_base.h"

#include "in_buttons.h"
#include "takedamageinfo.h"
#include "ammodef.h"
#include "hl2_gamerules.h"
#include "hlss_weapon_id.h"
#include "rumble_shared.h"
#include "bobvars.h"

#ifdef CLIENT_DLL
extern IVModelInfoClient* modelinfo;
#else
extern IVModelInfo* modelinfo;
#endif

#ifdef CLIENT_DLL
    #include "vgui/ISurface.h"
	#include "vgui_controls/Controls.h"
	#include "hud_crosshair.h"
#include "c_rumble.h"
#include "prediction.h"
#include "bone_setup.h"
#else
	#include "vphysics/constraints.h"
    #include "ilagcompensationmanager.h"
#endif

#include "hl2_player_shared.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CCoopWeaponData;
}

//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCoopBase, DT_WeaponCoopBase );

//-----------------------------------------------------------------------------
// Purpose: Propagation data for weapons. Only sent when a player's holding it.
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE(CWeaponCoopBase, DT_WeaponCoopLocalData)
#if !defined( CLIENT_DLL )
SendPropInt(SENDINFO(m_iPrimaryAttacks)),
SendPropInt(SENDINFO(m_iSecondaryAttacks)),
#else
RecvPropInt(RECVINFO(m_iPrimaryAttacks)),
RecvPropInt(RECVINFO(m_iSecondaryAttacks)),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CWeaponCoopBase, DT_WeaponCoopBase )
#ifndef CLIENT_DLL
SendPropDataTable("CoopLocalWeaponData", 0, &REFERENCE_SEND_TABLE(DT_WeaponCoopLocalData), SendProxy_SendLocalWeaponDataTable),
#else
RecvPropDataTable("CoopLocalWeaponData", 0, 0, &REFERENCE_RECV_TABLE(DT_WeaponCoopLocalData)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponCoopBase )
#ifdef CLIENT_DLL
DEFINE_PRED_FIELD(m_iPrimaryAttacks, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_iSecondaryAttacks, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

BEGIN_DATADESC( CWeaponCoopBase )
DEFINE_FIELD(m_iPrimaryAttacks, FIELD_INTEGER),
DEFINE_FIELD(m_iSecondaryAttacks, FIELD_INTEGER),
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

extern float	g_lateralBob;
extern float	g_verticalBob;

#define	HL2_BOB_CYCLE_MIN	1.0f
#define	HL2_BOB_CYCLE_MAX	0.45f
#define	HL2_BOB			0.002f
#define	HL2_BOB_UP		0.5f
#if 0
//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CWeaponCoopBase::CalcPortalViewmodelBob(void)
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;

	CBasePlayer* player = ToBasePlayer(GetOwner());
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ((!gpGlobals->frametime) || (player == NULL))
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	// Note: we use paint code for this so when player move on speed paint, gun bob faster (Bank)
	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length();
	const float flMaxSpeed = 320.f;

	speed = clamp(speed, -flMaxSpeed, flMaxSpeed);

	float bob_offset = RemapVal(speed, 0, flMaxSpeed, 0.0f, 1.0f);

	////Find the speed of the player
	//float speed = player->GetLocalVelocity().Length2D();

	////FIXME: This maximum speed value must come from the server.
	////		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	//speed = clamp( speed, -320, 320 );

	//float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	bobtime += (gpGlobals->curtime - lastbobtime) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime / HL2_BOB_CYCLE_MAX) * HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;

	if (cycle < HL2_BOB_UP)
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - HL2_BOB_UP) / (1.0 - HL2_BOB_UP);
	}

	g_verticalBob = speed * 0.005f;
	g_verticalBob = g_verticalBob * 0.3 + g_verticalBob * 0.7 * sin(cycle);

	g_verticalBob = clamp(g_verticalBob, -7.0f, 4.0f);

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime / HL2_BOB_CYCLE_MAX * 2) * HL2_BOB_CYCLE_MAX * 2;
	cycle /= HL2_BOB_CYCLE_MAX * 2;

	if (cycle < HL2_BOB_UP)
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - HL2_BOB_UP) / (1.0 - HL2_BOB_UP);
	}

	g_lateralBob = speed * 0.005f;
	g_lateralBob = g_lateralBob * 0.3 + g_lateralBob * 0.7 * sin(cycle);
	g_lateralBob = clamp(g_lateralBob, -7.0f, 4.0f);

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}
#endif
void CWeaponCoopBase::AddPortalViewmodelBob(CBaseViewModel* viewmodel, Vector& origin, QAngle& angles)
{
	Vector	forward, right, up;
	AngleVectors(angles, &forward, &right, &up);

	CalcHL2ViewmodelBob();

	// Apply bob, but scaled down to 40%
	VectorMA(origin, g_verticalBob * 0.1f, forward, origin);

	// Z bob a bit more
	origin += g_verticalBob * 0.1f * up;

	//move left and right
	VectorMA(origin, g_lateralBob * 0.8f, right, origin);

	//roll, pitch, yaw
	float rollAngle = g_verticalBob * 0.5f;
	VMatrix rotMatrix;
	Vector rotAxis = CrossProduct(right, up).Normalized();

	MatrixBuildRotationAboutAxis(rotMatrix, rotAxis, rollAngle);
	up = rotMatrix * up;
	forward = rotMatrix * forward;
	right = rotMatrix * right;

	float pitchAngle = -g_verticalBob * 0.4f;
	rotAxis = right;
	MatrixBuildRotationAboutAxis(rotMatrix, rotAxis, pitchAngle);
	up = rotMatrix * up;
	forward = rotMatrix * forward;

	float yawAngle = -g_lateralBob * 0.3f;
	rotAxis = up;
	MatrixBuildRotationAboutAxis(rotMatrix, rotAxis, yawAngle);
	forward = rotMatrix * forward;

	VectorAngles(forward, up, angles);

	//// Apply bob, but scaled down to 40%
	//VectorMA( origin, g_verticalBob * 0.1f, forward, origin );
	//
	//// Z bob a bit more
	//origin[2] += g_verticalBob * 0.1f;
	//
	//// bob the angles
	//angles[ ROLL ]	+= g_verticalBob * 0.5f;
	//angles[ PITCH ]	-= g_verticalBob * 0.4f;

	//angles[ YAW ]	-= g_lateralBob  * 0.3f;

	//VectorMA( origin, g_lateralBob * 0.8f, right, origin );
}

float CWeaponCoopBase::CalcHL2ViewmodelBob(void)
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;

	CBasePlayer* player = ToBasePlayer(GetOwner());
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ((!gpGlobals->frametime) || (player == NULL))
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	speed = clamp(speed, -320, 320);

	float bob_offset = RemapVal(speed, 0, 320, 0.0f, 1.0f);

	bobtime += (gpGlobals->curtime - lastbobtime) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime / HL2_BOB_CYCLE_MAX) * HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;

	if (cycle < HL2_BOB_UP)
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - HL2_BOB_UP) / (1.0 - HL2_BOB_UP);
	}

	g_verticalBob = speed * 0.005f;
	g_verticalBob = g_verticalBob * 0.3 + g_verticalBob * 0.7 * sin(cycle);

	g_verticalBob = clamp(g_verticalBob, -7.0f, 4.0f);

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime / HL2_BOB_CYCLE_MAX * 2) * HL2_BOB_CYCLE_MAX * 2;
	cycle /= HL2_BOB_CYCLE_MAX * 2;

	if (cycle < HL2_BOB_UP)
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - HL2_BOB_UP) / (1.0 - HL2_BOB_UP);
	}

	g_lateralBob = speed * 0.005f;
	g_lateralBob = g_lateralBob * 0.3 + g_lateralBob * 0.7 * sin(cycle);
	g_lateralBob = clamp(g_lateralBob, -7.0f, 4.0f);

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CWeaponCoopBase::AddHL2ViewmodelBob(CBaseViewModel* viewmodel, Vector& origin, QAngle& angles)
{
	/*if (viewmodel->GetSequenceActivity(viewmodel->GetSequence()) == GetSprintActivity())
		return;*/

	Vector	forward, right;
	AngleVectors(angles, &forward, &right, NULL);

	CalcHL2ViewmodelBob();

	// Apply bob, but scaled down to 40%
	VectorMA(origin, g_verticalBob * 0.1f, forward, origin);

	// Z bob a bit more
	origin[2] += g_verticalBob * 0.1f;

	// bob the angles
	angles[ROLL] += g_verticalBob * 0.5f;
	angles[PITCH] -= g_verticalBob * 0.4f;

	angles[YAW] -= g_lateralBob * 0.3f;

	VectorMA(origin, g_lateralBob * 0.8f, right, origin);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float g_bob;
float CWeaponCoopBase::CalcHL1ViewmodelBob(void)
{
	static  float bob;
	static   float bobtime;
	static   float lastbobtime;
	float   cycle;

	CBasePlayer* player = ToBasePlayer(GetOwner());

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ((!gpGlobals->frametime) || (player == NULL))
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	lastbobtime = gpGlobals->curtime;

	bobtime += gpGlobals->frametime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime / cl_bobcycle.GetFloat()) * cl_bobcycle.GetFloat();
	cycle /= cl_bobcycle.GetFloat();

	if (cycle < cl_bobup.GetFloat())
	{
		cycle = M_PI * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI + M_PI * (cycle - cl_bobup.GetFloat()) / (1.0 - cl_bobup.GetFloat());
	}

	//Find the speed of the player
	Vector2D vel = player->GetLocalVelocity().AsVector2D();

	bob = sqrt(vel[0] * vel[0] + vel[1] * vel[1]) * cl_bob.GetFloat();
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);
	bob = min(bob, 4.f);
	bob = max(bob, -7.f);

	g_bob = bob;

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
float v_idlescale = 0.0f;
//float aBob = 0.0f;
void CWeaponCoopBase::AddHL1ViewmodelBob(CBaseViewModel* viewmodel, Vector& origin, QAngle& angles)
{
	Vector   forward, right, up;
	QAngle oldAngles = angles;
	AngleVectors(angles, &forward, &right, &up);

	CalcHL1ViewmodelBob();
	static float time2 = 0.0f;
	time2 += gpGlobals->frametime;

	//angles[ROLL] += v_idlescale * sin(time2*0.5) * 0.1;
	//angles[PITCH] += v_idlescale * sin(time2 * 1) * 0.3;
	//angles[YAW] += v_idlescale * sin(time2 * 2) * 0.3;
	//aBob += g_bob;
	// Apply bob, but scaled down to 40%
	for (int i = 0; i < 3; i++)
	{
		origin[i] += g_bob * 0.4 * forward[i];
	}

	//origin[2] += g_bob;

	// throw in a little tilt.
	angles[YAW] -= g_bob * 0.5f;
	angles[ROLL] -= g_bob * 1.0f;
	angles[PITCH] -= g_bob * 0.3f;

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	origin[2] -= 1;
}

// return true if this vector is (0,0,0) within tolerance
bool IsZero(const QAngle &angle, float tolerance = 0.01f)
{
	return (angle.x > -tolerance && angle.x < tolerance&&
		angle.y > -tolerance && angle.y < tolerance&&
		angle.z > -tolerance && angle.z < tolerance);
}

int CWeaponCoopBase::CWeaponMergeCache::GetParentBone(CStudioHdr* pHdr, const char* pszName, boneextradata_t& extraData)
{
	const auto &MergeMod = GetWeapon()->GetCoopWpnData().m_BonemergeMod;
	int iIndex = MergeMod.Find(pszName);
	if (MergeMod.IsValidIndex(iIndex))
	{
		const auto& data = MergeMod.Element(iIndex);
		if (!data.vecOffsetPos.IsZero() || !IsZero(data.angOffsetRot))
		{
			extraData.m_iFlags |= BONE_FLAG_OFFSET_MATRIX;
			AngleMatrix(data.angOffsetRot, data.vecOffsetPos, extraData.m_matOffset);
		}

		return Studio_BoneIndexByName(pHdr, data.szParentBone);
	}

	return Studio_BoneIndexByName(pHdr, pszName);
}

void CWeaponCoopBase::CalcBoneMerge(CStudioHdr* hdr, int boneMask, CBoneBitList& boneComputed)
{
	bool boneMerge = IsEffectActive(EF_BONEMERGE);
	if (boneMerge || m_pBoneMergeCache)
	{
		if (boneMerge)
		{
			if (!m_pBoneMergeCache)
			{
				m_pBoneMergeCache = new CWeaponMergeCache;
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
	// If we have some sounds from the weapon classname.txt file, play a random one of them
	const char* shootsound = GetShootSound(sound_type);
	const char* nonownershootsound = nullptr;

	if (sound_type == SINGLE || sound_type == WPN_DOUBLE || sound_type == RELOAD)
	{
		nonownershootsound = GetShootSound(sound_type + 1);
	}

	if (!shootsound || !shootsound[0])
		return;

	if (!nonownershootsound || !nonownershootsound[0])
		nonownershootsound = shootsound;

#ifdef CLIENT_DLL
		CBroadcastRecipientFilter filter; // this is client side only

		if ( !te->CanPredict() )
			return;
				
		CBaseEntity::EmitSound( filter, GetOwner()->entindex(), shootsound, &GetOwner()->GetAbsOrigin() ); 
#else
	CSoundParameters params;

	if (!GetParametersForSound(shootsound, params, NULL))
		return;

	if (params.play_to_owner_only)
	{
		// Am I only to play to my owner?
		if (GetOwner() && GetOwner()->IsPlayer())
		{
			CSingleUserRecipientFilter filter(ToBasePlayer(GetOwner()));
			if (IsPredicted() && CBaseEntity::GetPredictionPlayer())
			{
				filter.UsePredictionRules();
			}
			EmitSound(filter, GetOwner()->entindex(), shootsound, NULL, soundtime);
		}
	}
	else
	{
		// Play weapon sound from the owner
		if (GetOwner())
		{
			CPASAttenuationFilter filter(GetOwner(), params.soundlevel);
			if (IsPredicted() && CBaseEntity::GetPredictionPlayer())
			{
				filter.UsePredictionRules();
			}

			if (GetOwner()->IsPlayer())
			{
				filter.RemoveRecipient(ToBasePlayer(GetOwner()));

				CSingleUserRecipientFilter filter2(ToBasePlayer(GetOwner()));
				if (IsPredicted() && CBaseEntity::GetPredictionPlayer())
				{
					filter2.UsePredictionRules();
				}
				EmitSound(filter2, GetOwner()->entindex(), shootsound, NULL, soundtime);
			}

			EmitSound(filter, GetOwner()->entindex(), nonownershootsound, NULL, soundtime);

#if !defined( CLIENT_DLL )
			if (sound_type == EMPTY)
			{
				CSoundEnt::InsertSound(SOUND_COMBAT, GetOwner()->GetAbsOrigin(), SOUNDENT_VOLUME_EMPTY, 0.2, GetOwner());
			}
#endif
		}
		// If no owner play from the weapon (this is used for thrown items)
		else
		{
			CPASAttenuationFilter filter(this, params.soundlevel);
			if (IsPredicted() && CBaseEntity::GetPredictionPlayer())
			{
				filter.UsePredictionRules();
			}
			EmitSound(filter, entindex(), nonownershootsound, NULL, soundtime);
		}
	}
#endif
}

const CCoopWeaponData &CWeaponCoopBase::GetCoopWpnData() const
{
	const CCoopWeaponData& data = static_cast<const CCoopWeaponData &> (GetWpnData());
	return data;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CWeaponCoopBase::GetWeaponID(void) const
{
	Assert(false);
	return HLSS_WEAPON_ID_NONE;
}

WeaponClass_t CWeaponCoopBase::WeaponClassify()
{
	switch (GetWeaponID())
	{
	case HLSS_WEAPON_ID_SMG1:
	case HLSS_WEAPON_ID_SMG2:
	case HLSS_WEAPON_ID_AR2:
	case HLSS_WEAPON_ID_MP5_BMS:
	case HLSS_WEAPON_ID_MG1:
	case HLSS_WEAPON_ID_CROSSBOW:
		return WEPCLASS_RIFLE;
		break;
	case HLSS_WEAPON_ID_SHOTGUN:
		return WEPCLASS_SHOTGUN;
		break;
	case HLSS_WEAPON_ID_RPG:
	case HLSS_WEAPON_ID_RPG_BMS:
		return WEPCLASS_HEAVY;
		break;
	case HLSS_WEAPON_ID_PISTOL:
	case HLSS_WEAPON_ID_357:
	case HLSS_WEAPON_ID_ALYXGUN:
	case HLSS_WEAPON_ID_DEAGLE:
	case HLSS_WEAPON_ID_GLOCK_BMS:
		return WEPCLASS_HANDGUN;
		break;
	case HLSS_WEAPON_ID_STUNSTICK:
	case HLSS_WEAPON_ID_CROWBAR:
	case HLSS_WEAPON_ID_LEADPIPE:
	case HLSS_WEAPON_ID_CROWBAR_BMS:
		return WEPCLASS_MELEE;
		break;
	default:
		return WEPCLASS_INVALID;
		break;
	}
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

	int iRumblue = GetRumbleEffect();
	if (iRumblue > 0)
		RumbleEffect(iRumblue, 0, RUMBLE_FLAGS_NONE);

	pPlayer->FireBullets( info );

	m_iPrimaryAttacks++;

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
	case HLSS_WEAPON_ID_MP5_BMS:
	default:
		iWeaponRole = TF_WPN_TYPE_PRIMARY;
		break;
	case HLSS_WEAPON_ID_PISTOL:
	case HLSS_WEAPON_ID_357:
	case HLSS_WEAPON_ID_ALYXGUN:
	case HLSS_WEAPON_ID_DEAGLE:
	case HLSS_WEAPON_ID_GLOCK_BMS:
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
	case HLSS_WEAPON_ID_PORTALGUN:
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
	case HLSS_WEAPON_ID_EMPTOOL:
		iWeaponRole = TF_WPN_TYPE_PDA;
		break;
	case HLSS_WEAPON_ID_CROWBAR:
	case HLSS_WEAPON_ID_STUNSTICK:
	case HLSS_WEAPON_ID_LEADPIPE:
	case HLSS_WEAPON_ID_CROWBAR_BMS:
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
	case HLSS_WEAPON_ID_EMPTOOL:
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
	case HLSS_WEAPON_ID_CROWBAR_BMS:
	case HLSS_WEAPON_ID_PORTALGUN:
		iWeaponVariant = 2;
		break;
	case HLSS_WEAPON_ID_EGON:
	case HLSS_WEAPON_ID_MP5_BMS:
	case HLSS_WEAPON_ID_GLOCK_BMS:
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_SMG1,						false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SMG1,				false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SMG1,				false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SMG1,			false },
		{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SMG1,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_SMG1,					false}, 
		{ACT_HL2MP_SIT, ACT_HL2MP_SIT_SMG1, false},
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PASSIVE,					false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_PISTOL,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_PISTOL,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_PISTOL, false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_GLOCK,	false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_MELEE,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_MELEE,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_MELEE, false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_CROSSBOW,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_CROSSBOW,				false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_CROSSBOW,				false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_CROSSBOW,			false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_CROSSBOW,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_CROSSBOW, false},
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_CROSSBOW,					false },
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PASSIVE,					false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_SHOTGUN,						false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SHOTGUN,			false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SHOTGUN,		false },
		{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_SHOTGUN,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_SHOTGUN,					false}, 
		{ACT_HL2MP_SIT, ACT_HL2MP_SIT_SHOTGUN, false},
		{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SHOTGUN,				false },
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PASSIVE,					false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_GRENADE,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_GRENADE,					false},
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_GRENADE, false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_RPG,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_RPG,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_RPG,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_RPG,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_RPG,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_RPG,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_RPG,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_RPG, false},
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PASSIVE,					false},
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
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_PHYSGUN, false},
	{ ACT_HL2MP_WALK, ACT_HL2MP_WALK_PHYSGUN, false },
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PHYSGUN,					false},
};
acttable_t CWeaponCoopBase::s_acttableSlam[] =
{
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_SLAM,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_SLAM,					false },
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_SLAM,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_SLAM,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_SLAM,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_SLAM,		false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_SLAM,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_SLAM, false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_MELEE2,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_MELEE2,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_MELEE2,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE2,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_MELEE2,			false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_MELEE2,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_MELEE2,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_MELEE2, false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_REVOLVER,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_REVOLVER,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_REVOLVER,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_REVOLVER,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_REVOLVER, false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_GLOCK,	false},
};
acttable_t CWeaponCoopBase::s_acttableAR2[] =
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
	{ ACT_RELOAD,					ACT_RELOAD_AR2,				true },
	{ ACT_IDLE,						ACT_IDLE_AR2,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_AR2,			false },

	{ ACT_WALK,						ACT_WALK_AR2,					true },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_AR2_RELAXED,			false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_AR2_STIMULATED,		false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_AR2,			false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_AR2_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_AR2_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_AR2,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_AR2_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_AR2_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_AR2_RELAXED,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_AR2_STIMULATED,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_AR2,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_AR2_RELAXED,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_AR2_STIMULATED,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_AR2,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_AR2_RELAXED,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_AR2_STIMULATED,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
	//End readiness activities

		{ ACT_WALK_AIM,					ACT_WALK_AIM_AR2,				true },
		{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
		{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
		{ ACT_RUN,						ACT_RUN_AR2,					true },
		{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
		{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
		{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
		{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
		{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_AR2_LOW,		false },
		{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_AR2,		true },
		//	{ ACT_RANGE_ATTACK2, ACT_RANGE_ATTACK_AR2_GRENADE, true },

		{ ACT_ARM,						ACT_ARM_RIFLE,					false},
		{ ACT_DISARM,					ACT_DISARM_RIFLE,				false},

			{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_AR2,					false },
			{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_AR2,					false },
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_AR2,						false },
			{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_AR2,			false },
			{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_AR2,			false },
			{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2,	false },
			{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_AR2,		false },
			{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_AR2,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_AR2,					false}, 
		{ACT_HL2MP_SIT, ACT_HL2MP_SIT_AR2, false},
			{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_AR2,				false },
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PASSIVE,					false},
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
		{ ACT_HL2MP_WALK,					ACT_BMMP_RUN_MP5,						false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_BMMP_IDLE_CROUCH_MP5,				false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_BMMP_WALK_CROUCH_MP5,				false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_BMMP_GESTURE_RANGE_ATTACK_MP5,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_BMMP_GESTURE_RELOAD_MP5,			false },
		{ ACT_HL2MP_JUMP,					ACT_BMMP_JUMP_START_MP5,					false },
		{ACT_HL2MP_SWIM,					ACT_BMMP_SWIM_MP5,					false},
		{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_MP5,	false},
		{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_SMG1,					false },
		{ACT_HL2MP_RUN_FAST,				ACT_HL2MP_RUN_PASSIVE,					false},
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
		{ ACT_HL2MP_WALK,					ACT_BMMP_RUN_GLOCK,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_BMMP_IDLE_CROUCH_GLOCK,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_BMMP_WALK_CROUCH_GLOCK,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_BMMP_GESTURE_RANGE_ATTACK_GLOCK,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_BMMP_GESTURE_RELOAD_GLOCK,		false },
	{ ACT_HL2MP_JUMP,					ACT_BMMP_JUMP_START_GLOCK,					false },
		{ACT_HL2MP_SWIM,					ACT_BMMP_SWIM_GLOCK,					false},
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_GLOCK,	false},
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
		{ ACT_HL2MP_WALK,					ACT_BMMP_RUN_TAU,						false },
		{ ACT_HL2MP_IDLE_CROUCH,			ACT_BMMP_IDLE_CROUCH_TAU,			false },
		{ ACT_HL2MP_WALK_CROUCH,			ACT_BMMP_WALK_CROUCH_TAU,			false },
		{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_BMMP_GESTURE_RANGE_ATTACK_TAU,	false },
		{ ACT_HL2MP_GESTURE_RELOAD,			ACT_BMMP_GESTURE_RELOAD_TAU,		false },
		{ ACT_HL2MP_JUMP,					ACT_BMMP_JUMP_START_TAU,					false },
		{ACT_HL2MP_SWIM,					ACT_BMMP_SWIM_TAU,					false},
		{ACT_HL2MP_RUN_FAST,				ACT_BMMP_RUN_TAU,					false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_TAU,	false},
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
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_GLUON,	false},
		{ACT_HL2MP_RUN_FAST,				ACT_BMMP_RUN_GLUON,					false},
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
		{ACT_HL2MP_RUN_FAST,				ACT_BMMP_RUN_HIVEHAND,					false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_HIVEHAND,	false},
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
		{ACT_HL2MP_RUN_FAST,				ACT_BMMP_RUN_SATCHEL,					false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_SATCHEL,	false},
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
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_SNARK,	false},
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
		{ACT_HL2MP_RUN_FAST,				ACT_BMMP_RUN_TRIPMINE,					false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_TRIPMINE,	false},
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
		{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_REVOLVER,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_REVOLVER,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_REVOLVER,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_REVOLVER,					false },
		{ACT_HL2MP_SWIM,					ACT_HL2MP_SWIM_REVOLVER,					false}, 
	{ACT_HL2MP_SIT, ACT_HL2MP_SIT_REVOLVER, false},
	{ACT_BMMP_LADDER,	ACT_BMMP_LADDER_GLOCK,	false},
};

acttable_t CWeaponCoopBase::s_acttablePortalgun[] =
{
	{ ACT_HL2MP_IDLE, ACT_MP_STAND_PRIMARY, false },
	{ ACT_HL2MP_RUN, ACT_MP_RUN_PRIMARY, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_MP_CROUCH_PRIMARY, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_MP_CROUCHWALK_PRIMARY, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_MP_ATTACK_STAND_PRIMARYFIRE, false },
	{ ACT_HL2MP_JUMP, ACT_MP_JUMP_START_PRIMARY, false },
	{ ACT_HL2MP_SWIM, ACT_HL2MP_SWIM_CROSSBOW, false },
	{ ACT_HL2MP_WALK, ACT_MP_RUN_PRIMARY, false },
		{ACT_HL2MP_RUN_FAST,				ACT_MP_RUN_PRIMARY,					false},
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
		case 3:
			pTable = s_acttableGlock;
			iActivityCount = ARRAYSIZE(s_acttableGlock);
			break;
		}
		break;
	case TF_WPN_TYPE_MELEE:
		switch (iWeaponVariant)
		{
		case 0:
		default:
			pTable = s_acttableMelee;
			iActivityCount = ARRAYSIZE(s_acttableMelee);
			break;
		}
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
		case 2:
			pTable = s_acttablePortalgun;
			iActivityCount = ARRAYSIZE(s_acttablePortalgun);
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
		case 1:
			pTable = s_acttableAR2;
			iActivityCount = ARRAYSIZE(s_acttableAR2);
			break;
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

#ifndef CLIENT_DLL
// Allows Weapon_BackupActivity() to access the SMG1's activity table.
acttable_t* GetSMG1Acttable()
{
	return CWeaponCoopBase::s_acttableSMG1;
}

int GetSMG1ActtableCount()
{
	return ARRAYSIZE(CWeaponCoopBase::s_acttableSMG1);
}
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCoopBase::Equip(CBaseCombatCharacter *pOwner)
{
	BaseClass::Equip(pOwner);

	// Add it to attribute providers list.
	ReapplyProvision();
}

void CWeaponCoopBase::RumbleEffect(unsigned char effectIndex, unsigned char rumbleData, unsigned char rumbleFlags)
{
#ifndef CLIENT_DLL
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer || gpGlobals->maxClients > 1)
		return;

	pPlayer->RumbleEffect(effectIndex, rumbleData, rumbleFlags);
#else
	if (prediction->IsFirstTimePredicted())
		::RumbleEffect(effectIndex, rumbleData, rumbleFlags);
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCoopBase::ReapplyProvision(void)
{
	int iProvideOnActive = 0;
	CALL_ATTRIB_HOOK_INT(iProvideOnActive, provide_on_active);
	if (!iProvideOnActive || m_iState == WEAPON_IS_ACTIVE)
	{
		BaseClass::ReapplyProvision();
	}
	else
	{
		// Weapon not active, remove it from providers list.
		GetAttributeContainer()->StopProvidingTo(GetOwner());
		m_hOldOwner = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponCoopBase::OnActiveStateChanged(int iOldState)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner)
	{
		int iProvideOnActive = 0;
		CALL_ATTRIB_HOOK_INT(iProvideOnActive, provide_on_active);

		// If set to only provide attributes while active, update the status now.
		if (iProvideOnActive)
		{
			ReapplyProvision();
		}
	}

	BaseClass::OnActiveStateChanged(iOldState);
}

CCoopWeaponData::CCoopWeaponData()
{
	m_iViewmodelBobMode = BOBMODE_HL2;
}

void CCoopWeaponData::Parse(KeyValues* pKeyValuesData, const char* szWeaponName)
{
	FileWeaponInfo_t::Parse(pKeyValuesData, szWeaponName);

	m_iViewmodelBobMode = pKeyValuesData->GetInt("viewmodel_bobmode", BOBMODE_HL2);

#ifdef CLIENT_DLL
	KeyValues* pkvMergeMod = pKeyValuesData->FindKey("worldmodel_bonemerge");
	if (pkvMergeMod)
	{
		for (KeyValues* pBoneDef = pkvMergeMod->GetFirstTrueSubKey(); pBoneDef; pBoneDef = pBoneDef->GetNextTrueSubKey())
		{
			int iIndex = m_BonemergeMod.Find(pBoneDef->GetName());
			if (!m_BonemergeMod.IsValidIndex(iIndex))
				iIndex = m_BonemergeMod.Insert(pBoneDef->GetName());

			if (!m_BonemergeMod.IsValidIndex(iIndex))
				continue;

			auto& newData = m_BonemergeMod.Element(iIndex);
			V_strcpy_safe(newData.szParentBone, pBoneDef->GetString("parent"));
			UTIL_StringToVector(newData.vecOffsetPos.Base(), pBoneDef->GetString("offsetpos", "0 0 0"));
			UTIL_StringToVector(newData.angOffsetRot.Base(), pBoneDef->GetString("offsetang", "0 0 0"));

		}
	}
#endif // CLIENT_DLL
}
