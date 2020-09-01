//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "gamestats.h"
#include "weapon_coop_basehlcombatweapon.h"


#ifdef CLIENT_DLL
#define CWeaponComSniper C_WeaponComSniper

#include "c_te_effect_dispatch.h"
#include "beamdraw.h"
#include "iviewrender_beams.h"
#include "model_types.h"
#else
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "soundent.h"
#include "game.h"
#include "te_effect_dispatch.h"
#include "ilagcompensationmanager.h"
#include "ai_behavior.h"
#include "ai_hint.h"
#endif

#include "hl2_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponComSniper;

#ifndef CLIENT_DLL
// If the last time I fired at someone was between 0 and this many seconds, draw
// a bead on them much faster. (use subsequent paint time)
#define SNIPER_FASTER_ATTACK_PERIOD		3.0f

// These numbers determine the interval between shots. They used to be constants,
// but are now keyfields. HL2 backwards compatibility was maintained by supplying 
// default values in the constructor.
#if 1
// How long to aim at someone before shooting them.
#define SNIPER_PAINT_ENEMY_TIME			1.0f
// ...plus this
#define	SNIPER_PAINT_NPC_TIME_NOISE		0.75f
#else
// How long to aim at someone before shooting them.
#define SNIPER_DEFAULT_PAINT_ENEMY_TIME			1.0f
// ...plus this
#define	SNIPER_DEFAULT_PAINT_NPC_TIME_NOISE		0.75f
#endif

#define SNIPER_SUBSEQUENT_PAINT_TIME	( ( IsXbox() ) ? 1.0f : 0.4f )

#define SNIPER_FOG_PAINT_ENEMY_TIME	    0.25f
#define SNIPER_PAINT_DECOY_TIME			2.0f
#define SNIPER_PAINT_FRUSTRATED_TIME	1.0f
#define SNIPER_QUICKAIM_TIME			0.2f
#define SNIPER_PAINT_NO_SHOT_TIME		0.7f

#define SNIPER_DECOY_MAX_MASS			200.0f

// Target protection
#define SNIPER_PROTECTION_MINDIST		(1024.0*1024.0)	// Distance around protect target that sniper does priority modification in
#define SNIPER_PROTECTION_PRIORITYCAP	100.0			// Max addition to priority of an enemy right next to the protect target, falls to 0 at SNIPER_PROTECTION_MINDIST.

#define SNIPER_DECOY_RADIUS	256
#define SNIPER_NUM_DECOYS 5

class CAI_SniperBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS(CAI_SniperBehavior, CAI_SimpleBehavior);
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;
	DECLARE_DATADESC();

public:
	CAI_SniperBehavior(CWeaponComSniper *pSniper) : m_pWeapon(pSniper)
	{}

	// Identifier
	const char* GetName() { return "Sniper"; }

	// Schedule
	bool 		CanSelectSchedule();;
	void		PrescheduleThink();

	// Conditions:
	virtual void GatherConditions();
	virtual int SelectSchedule(void);
	virtual int RangeAttack1Conditions(float flDot, float flDist);
	virtual int TranslateSchedule(int scheduleType);
	virtual bool FValidateHintType(CAI_Hint* pHint);

	void AimGun(void);

	enum
	{
		SCHED_SNIPERRIFLE_SCAN = BaseClass::NEXT_SCHEDULE,
		SCHED_SNIPERRIFLE_CAMP,
		SCHED_SNIPERRIFLE_ATTACK,
		SCHED_SNIPERRIFLE_ATTACKDECOY,
		SCHED_SNIPERRIFLE_SUPPRESSED,
		SCHED_SNIPERRIFLE_FRUSTRATED_ATTACK,
		SCHED_SNIPERRIFLE_SNAPATTACK,
		SCHED_SNIPERRIFLE_NO_CLEAR_SHOT,
		SCHED_SNIPERRIFLE_MOVE_TO_VANTAGE,
	};

	// Tasks
	void		StartTask(const Task_t* pTask);
	void		RunTask(const Task_t* pTask);

	enum
	{
		TASK_SNIPERRIFLE_FRUSTRATED_ATTACK = BaseClass::NEXT_TASK,
		TASK_SNIPERRIFLE_PAINT_ENEMY,
		TASK_SNIPERRIFLE_PAINT_DECOY,
		TASK_SNIPERRIFLE_PAINT_FRUSTRATED,
		TASK_SNIPERRIFLE_ATTACK_CURSOR,
		TASK_SNIPERRIFLE_ATTACK_ENEMY,
		TASK_SNIPERRIFLE_ATTACK_DECOY,
		TASK_SNIPERRIFLE_PAINT_NO_SHOT,
	};

	enum
	{
		COND_SNIPERRIFLE_CANATTACKDECOY = BaseClass::NEXT_CONDITION,
		COND_SNIPERRIFLE_SUPPRESSED,
		COND_SNIPERRIFLE_FRUSTRATED,
		COND_SNIPERRIFLE_NO_SHOT,
	};

	float SetWait(float minWait, float maxWait = 0.0) { return GetOuter()->SetWait(minWait, maxWait); }
	void ClearWait() { GetOuter()->ClearWait(); }
	float GetWaitFinishTime() { return GetOuter()->GetWaitFinishTime(); }
	bool IsWaitFinished() { return GetOuter()->IsWaitFinished(); }
	bool IsWaitSet() { return GetOuter()->IsWaitSet(); }

	void LaserOff(void);
	void LaserOn(const Vector& vecTarget, const Vector& vecDeviance);

	void PaintTarget(const Vector& vecTarget, float flPaintTime);

protected:
	bool ShouldSnapShot(void);

	bool FireBullet(const Vector& vecTarget, bool bDirectShot);
	Vector	LeadTarget(CBaseEntity* pTarget);
	Vector  DesiredBodyTarget(CBaseEntity* pTarget);
	bool VerifyShot(CBaseEntity* pTarget);
	bool FindDecoyObject(void);

	float GetPositionParameter(float flTime, bool fLinear);

	void GetPaintAim(const Vector& vecStart, const Vector& vecGoal, float flParameter, Vector* pProgress);

	bool FindFrustratedShot(float flNoise);

	Vector GetBulletOrigin(void) { return GetOuter()->Weapon_ShootPosition(); }
	float GetBulletSpeed() { return 100000.f; }

	bool IsWeaponEquiped();
	bool IsWeaponReady();

	CWeaponComSniper* m_pWeapon;

	// This keeps track of the last spot the laser painted. For 
	// continuous sweeping that changes direction.
	Vector m_vecPaintCursor;
	float  m_flPaintTime;

	bool						m_fIsPatient;
	float						m_flPatience;
	int							m_iMisses;
	EHANDLE						m_hDecoyObject;
	Vector						m_vecDecoyObjectTarget;
	Vector						m_vecFrustratedTarget;
	Vector						m_vecPaintStart; // used to track where a sweep starts for the purpose of interpolating.

	float						m_flFrustration;
	float						m_flTimeLastAttackedPlayer;
	bool m_fSnapShot;

	// Have I warned the target that I'm pointing my laser at them?
	bool						m_bWarnedTargetEntity;

	float						m_flTimeLastShotMissed;
	bool						m_bKilledPlayer;
	bool						m_bShootZombiesInChest;		///< if true, do not try to shoot zombies in the headcrab
};

BEGIN_DATADESC(CAI_SniperBehavior)
DEFINE_FIELD(m_fIsPatient, FIELD_BOOLEAN),
DEFINE_FIELD(m_flPatience, FIELD_FLOAT),
DEFINE_FIELD(m_iMisses, FIELD_INTEGER),
DEFINE_FIELD(m_hDecoyObject, FIELD_EHANDLE),
DEFINE_FIELD(m_vecDecoyObjectTarget, FIELD_VECTOR),
DEFINE_FIELD(m_vecFrustratedTarget, FIELD_VECTOR),
DEFINE_FIELD(m_vecPaintStart, FIELD_VECTOR),
DEFINE_FIELD(m_flPaintTime, FIELD_TIME),
DEFINE_FIELD(m_vecPaintCursor, FIELD_VECTOR),
DEFINE_FIELD(m_flFrustration, FIELD_TIME),
DEFINE_FIELD(m_fSnapShot, FIELD_BOOLEAN),

DEFINE_KEYFIELD(m_bShootZombiesInChest, FIELD_BOOLEAN, "shootZombiesInChest"),

DEFINE_FIELD(m_flTimeLastAttackedPlayer, FIELD_TIME),

DEFINE_FIELD(m_bWarnedTargetEntity, FIELD_BOOLEAN),
DEFINE_FIELD(m_flTimeLastShotMissed, FIELD_TIME),
END_DATADESC();
#else
#define SNIPERRIFLE_LASER_ATTACHMENT_WORLD 3
#define SNIPERRIFLE_MUZZLE_ATTACHMENT_WORLD 1

#define SNIPERRIFLE_MUZZLE_ATTACHMENT 2
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// CWeaponComSniper
//-----------------------------------------------------------------------------

class CWeaponComSniper : public CWeaponCoopBaseHLCombat
{
	DECLARE_CLASS(CWeaponComSniper, CWeaponCoopBaseHLCombat);
public:

	CWeaponComSniper(void);

	void	PrimaryAttack(void);

	virtual bool			IsSniper() { return true; }

	float	WeaponAutoAimScale() { return 0.6f; }

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_SNIPER; }

	bool		IsLaserEnabled()
	{ 
		if (GetOwner() && GetOwner()->IsPlayer() && Clip1())
			return true;

		return m_bLaserEnabled.Get();
	}

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

#ifndef CLIENT_DLL
	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	virtual void			NPC_AttachBehavior(IBehaviorHost* pAI);
	virtual void			NPC_DetachBehavior(IBehaviorHost* pAI);
	void	FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool bUseWeaponAngles, const Vector * pShootTarget = nullptr);
	void	Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary);
	void	Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
	//bool	WeaponLOSCondition(const Vector& ownerPos, const Vector& targetPos, bool bSetConditions);
	int		WeaponRangeAttack1Condition(float flDot, float flDist) { return m_NPCBehavior.RangeAttack1Conditions(flDot, flDist); }

	void	Precache()
	{
		BaseClass::Precache();

		PrecacheModel("sprites/light_glow03.vmt");
		PrecacheModel("effects/bluelaser1.vmt");
	}
#else
	bool				IsViewModelForLocalPlayer();
	Vector			GetLaserEndPoint();

	// We need to render opaque and translucent pieces
	virtual RenderGroup_t	GetRenderGroup(void) { return RENDER_GROUP_TWOPASS; }

	virtual void	NotifyShouldTransmit(ShouldTransmitState_t state);
	virtual int		DrawModel(int flags);
	virtual void	ViewModelDrawn(C_BaseViewModel* pBaseViewModel);
	virtual bool	IsTranslucent(void);

	void			InitBeam(bool bViewModel);
	void			GetWeaponAttachment(bool bViewModel, int attachmentId, Vector& outVector, Vector* dir = NULL);
	void			DrawEffects(bool bViewModel);
	//	void			DrawLaserDot( void );

	Beam_t* m_pBeam[2];			// Laser beam temp entity
#endif // !CLIENT_DLL

protected:
#ifndef CLIENT_DLL
	CAI_SniperBehavior m_NPCBehavior;
	friend class CAI_SniperBehavior;

	void					NPC_SetLaserState(bool bEnabled) { m_bLaserEnabled = bEnabled; }
#endif // !CLIENT_DLL

	CNetworkVar(bool, m_bLaserEnabled);
};

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CWeaponComSniper);
PRECACHE_WEAPON_REGISTER(weapon_sniperrifle);

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponComSniper, DT_WeaponComSniper)

BEGIN_NETWORK_TABLE(CWeaponComSniper, DT_WeaponComSniper)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bLaserEnabled)),
#else
SendPropBool(SENDINFO(m_bLaserEnabled)),
#endif // CLIENT_DLL
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponComSniper)
END_PREDICTION_DATA()

BEGIN_DATADESC(CWeaponComSniper)
#ifndef CLIENT_DLL
DEFINE_EMBEDDED(m_NPCBehavior),
#endif
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
CWeaponComSniper::CWeaponComSniper(void)
#else
CWeaponComSniper::CWeaponComSniper(void) : m_NPCBehavior(this)
#endif
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponComSniper::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponComSniper::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary)
{
	if (bSecondary)
	{
		return;
	}
	else
	{
		// Ensure we have enough rounds in the clip
		m_iClip1++;

		FireNPCPrimaryAttack(pOperator, true);
	}
}

void CWeaponComSniper::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool bUseWeaponAngles, const Vector * pShootTarget)
{
	Vector vecShootOrigin, vecShootDir;

	CAI_BaseNPC* npc = pOperator->MyNPCPointer();
	ASSERT(npc != NULL);

	if (pShootTarget)
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = *pShootTarget - vecShootOrigin;
	}
	else if (bUseWeaponAngles)
	{
		QAngle	angShootDir;
		GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);
	}

	WeaponSound(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1);

	// NOTENOTE: This is overriden on the client-side
	// pOperator->DoMuzzleFlash();

	m_iClip1 = m_iClip1 - 1;
}
#else
extern void FormatViewModelAttachment(Vector& vOrigin, bool bInverse);
//-----------------------------------------------------------------------------
// Purpose: Returns the attachment point on either the world or viewmodel
//			This should really be worked into the CBaseCombatWeapon class!
//-----------------------------------------------------------------------------
void CWeaponComSniper::GetWeaponAttachment(bool bViewModel, int attachmentId, Vector& outVector, Vector* dir /*= NULL*/)
{
	QAngle	angles;

	if (bViewModel)
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());

		if (pOwner != NULL)
		{
			pOwner->GetViewModel()->GetAttachment(attachmentId, outVector, angles);
			::FormatViewModelAttachment(outVector, true);
		}
	}
	else
	{
		// We offset the IDs to make them correct for our world model
		BaseClass::GetAttachment(attachmentId, outVector, angles);
	}

	// Supply the direction, if requested
	if (dir != NULL)
	{
		AngleVectors(angles, dir, NULL, NULL);
	}
}

bool CWeaponComSniper::IsViewModelForLocalPlayer()
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		if (ShouldDrawUsingViewModel())
			return true;

		if (CBasePlayer::GetLocalPlayer()->GetObserverTarget() == pOwner && CBasePlayer::GetLocalPlayer()->GetObserverMode() == OBS_MODE_IN_EYE)
			return true;
	}

	return false;
}

Vector CWeaponComSniper::GetLaserEndPoint()
{
	if (GetOwner() && GetOwner()->IsPlayer())
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());
		Vector vecEye, vecForward;
		pOwner->EyePositionAndVectors(&vecEye, &vecForward, NULL, NULL);
		trace_t tr;
		UTIL_TraceLine(vecEye, vecEye + vecForward * MAX_TRACE_LENGTH, MASK_OPAQUE_AND_NPCS | CONTENTS_HITBOX, pOwner, COLLISION_GROUP_NONE, &tr);
		return tr.endpos;
	}
	else
	{
		Vector vecStart, vecForward;
		QAngle angAttachment;
		GetAttachment(SNIPERRIFLE_LASER_ATTACHMENT_WORLD, vecStart);
		GetAttachment(SNIPERRIFLE_MUZZLE_ATTACHMENT_WORLD, vecForward, angAttachment);
		AngleVectors(angAttachment, &vecForward);
		trace_t tr;
		UTIL_TraceLine(vecStart, vecStart + vecForward * MAX_TRACE_LENGTH, MASK_OPAQUE_AND_NPCS | CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr);
		return tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup our laser beam
//-----------------------------------------------------------------------------
void CWeaponComSniper::InitBeam(bool bViewModel)
{
	if (m_pBeam[bViewModel ? 1 : 0] != NULL)
		return;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	BeamInfo_t beamInfo;

	CBaseEntity* pEntity = NULL;

	if (bViewModel)
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());

		if (pOwner != NULL)
		{
			pEntity = pOwner->GetViewModel();
		}
	}
	else
	{
		pEntity = this;
	}

	beamInfo.m_pEndEnt = pEntity;
	beamInfo.m_pStartEnt = nullptr;
	beamInfo.m_nType = TE_BEAMPOINTS;
	beamInfo.m_vecStart = GetLaserEndPoint();
	beamInfo.m_vecEnd = vec3_origin;

	beamInfo.m_pszModelName = "effects/bluelaser1.vmt";

	beamInfo.m_flHaloScale = 4.0f;
	beamInfo.m_pszHaloName = "sprites/light_glow03.vmt";
	beamInfo.m_flLife = 0.0f;

	if (bViewModel)
	{
		beamInfo.m_flWidth = 2.0f;
		beamInfo.m_flEndWidth = 2.0f;
		beamInfo.m_nEndAttachment = SNIPERRIFLE_MUZZLE_ATTACHMENT;
	}
	else
	{
		beamInfo.m_flWidth = 1.0f;
		beamInfo.m_flEndWidth = 1.0f;
		beamInfo.m_nEndAttachment = SNIPERRIFLE_LASER_ATTACHMENT_WORLD;
	}

	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 0;
	beamInfo.m_flBrightness = 255.0;
	beamInfo.m_flSpeed = 0.0f;
	beamInfo.m_nStartFrame = 0.0;
	beamInfo.m_flFrameRate = 30.0;
	beamInfo.m_flRed = 0.0;
	beamInfo.m_flGreen = 100.0;
	beamInfo.m_flBlue = 255.0;
	beamInfo.m_nSegments = 4;
	beamInfo.m_bRenderable = false;
	beamInfo.m_nFlags = (FBEAM_FOREVER | FBEAM_HALOBEAM);

	m_pBeam[bViewModel ? 1 : 0] = beams->CreateBeamEntPoint(beamInfo);
}

//-----------------------------------------------------------------------------
// Purpose: Draw effects for our weapon
//-----------------------------------------------------------------------------
void CWeaponComSniper::DrawEffects(bool bViewModel)
{
	// Must be guiding and not hidden
	if (!IsLaserEnabled())
	{
		if (m_pBeam[0] != NULL)
		{
			m_pBeam[0]->brightness = 0;
		}
		if (m_pBeam[1] != NULL)
		{
			m_pBeam[1]->brightness = 0;
		}

		return;
	}

	InitBeam(bViewModel);

	int iBeamIdx = (bViewModel ? 1 : 0);

	if (m_pBeam[iBeamIdx] != NULL)
	{
		m_pBeam[iBeamIdx]->attachment[0] = GetLaserEndPoint();
		m_pBeam[iBeamIdx]->brightness = 255;
		m_pBeam[iBeamIdx]->delta = m_pBeam[iBeamIdx]->attachment[1] - m_pBeam[iBeamIdx]->attachment[0];

		/*if (bViewModel)
		{
			BeamInfo_t beamInfo;
			beamInfo.m_flFadeLength = 0.0f;
			beamInfo.m_flAmplitude = 0;
			beamInfo.m_flBrightness = 255.0;
			beamInfo.m_flSpeed = 1.0f;
			beamInfo.m_nStartFrame = 0.0;
			beamInfo.m_flFrameRate = 30.0;
			beamInfo.m_flRed = 255.0;
			beamInfo.m_flGreen = 0.0;
			beamInfo.m_flBlue = 0.0;
			beamInfo.m_nSegments = 4;

			beamInfo.m_vecStart = vecAttachment;
			GetWeaponAttachment(true, RPG_GUIDE_TARGET_ATTACHMENT, beamInfo.m_vecEnd);
			beams->UpdateBeamInfo(m_pBeam[iBeamIdx], beamInfo);
		}*/


		beams->DrawBeam(m_pBeam[iBeamIdx]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on third-person weapon drawing
//-----------------------------------------------------------------------------
int	CWeaponComSniper::DrawModel(int flags)
{
	int iRet = BaseClass::DrawModel(flags);

	// Only render these on the transparent pass
	if (iRet && flags & STUDIO_TRANSPARENCY)
	{
		DrawEffects(false);
		return 1;
	}

	// Draw the model as normal
	return iRet;
}

//-----------------------------------------------------------------------------
// Purpose: Called after first-person viewmodel is drawn
//-----------------------------------------------------------------------------
void CWeaponComSniper::ViewModelDrawn(C_BaseViewModel* pBaseViewModel)
{
	// Draw our laser effects
	DrawEffects(true);

	BaseClass::ViewModelDrawn(pBaseViewModel);
}

//-----------------------------------------------------------------------------
// Purpose: Used to determine sorting of model when drawn
//-----------------------------------------------------------------------------
bool CWeaponComSniper::IsTranslucent(void)
{
	// Must be guiding and not hidden
	if (IsLaserEnabled())
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Turns off effects when leaving the PVS
//-----------------------------------------------------------------------------
void CWeaponComSniper::NotifyShouldTransmit(ShouldTransmitState_t state)
{
	BaseClass::NotifyShouldTransmit(state);

	if (state == SHOULDTRANSMIT_END)
	{
		if (m_pBeam[0] != NULL)
		{
			m_pBeam[0]->brightness = 0.0f;
		}
		if (m_pBeam[1] != NULL)
		{
			m_pBeam[1]->brightness = 0.0f;
		}
	}
}
#endif // !CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponComSniper::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
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
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

#ifndef CLIENT_DLL
	//m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());
#endif

	m_iPrimaryAttacks++;

	WeaponSound(SINGLE);
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	CHL2_Player* pHL2 = ToHL2Player(pPlayer);
	if (pHL2)
		pHL2->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	FireBulletsInfo_t info(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType);
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets(info);


#ifndef CLIENT_DLL
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt(-1, 1);
	angles.y += random->RandomInt(-1, 1);
	angles.z = 0;

	pPlayer->SnapEyeAngles(angles);
#endif

	pPlayer->ViewPunch(QAngle(-8, random->RandomFloat(-2, 2), 0));

#ifndef CLIENT_DLL
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());
#endif

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
}

#ifndef CLIENT_DLL
void CWeaponComSniper::NPC_AttachBehavior(IBehaviorHost* pAI)
{
	NPC_SetLaserState(false);
	pAI->AddBehavior(&m_NPCBehavior);
}

void CWeaponComSniper::NPC_DetachBehavior(IBehaviorHost* pAI)
{
	pAI->RemoveBehavior(&m_NPCBehavior);
	NPC_SetLaserState(false);
}

bool CAI_SniperBehavior::CanSelectSchedule()
{
	if (!GetOuter())
		return false;

	// They don't have the weapon out?
	if (!IsWeaponEquiped())
		return false;

	// Are you alive, in a script?
	if (!GetOuter()->IsInterruptable())
		return false;

	// Commander is giving you orders?
	if (GetOuter()->HasCondition(COND_RECEIVED_ORDERS))
		return false;

	return true;
}

void CAI_SniperBehavior::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	// Suppress at the sound of danger. Incoming missiles, for example.
	if (HasCondition(COND_HEAR_DANGER))
	{
		SetCondition(COND_SNIPERRIFLE_SUPPRESSED);
	}
}

void CAI_SniperBehavior::GatherConditions()
{
	BaseClass::GatherConditions();
}

int CAI_SniperBehavior::SelectSchedule(void)
{
	if (HasCondition(COND_NO_PRIMARY_AMMO) || HasCondition(COND_LOW_PRIMARY_AMMO))
	{
		//LaserOff();
		return BaseClass::SelectSchedule();
	}

	if (GetEnemy() == NULL || HasCondition(COND_ENEMY_DEAD))
	{
		// Look for an enemy.
		GetOuter()->SetEnemy(NULL);
		m_flFrustration = gpGlobals->curtime;
		return SCHED_SNIPERRIFLE_SCAN;
	}

	if (HasCondition(COND_SNIPERRIFLE_FRUSTRATED))
	{
		return SCHED_SNIPERRIFLE_FRUSTRATED_ATTACK;
	}

	if (HasCondition(COND_SNIPERRIFLE_CANATTACKDECOY))
	{
		return SCHED_SNIPERRIFLE_ATTACKDECOY;
	}

	if (HasCondition(COND_SNIPERRIFLE_NO_SHOT))
	{
		return SCHED_SNIPERRIFLE_NO_CLEAR_SHOT;
	}

	if (HasCondition(COND_CAN_RANGE_ATTACK1))
	{
		// shoot!
		return TranslateSchedule(SCHED_RANGE_ATTACK1);
	}
	else if (gpGlobals->curtime - m_flFrustration < 6.f)
	{
		// Camp on this target
		return SCHED_SNIPERRIFLE_CAMP;
	}
	else
	{
		return SCHED_SNIPERRIFLE_MOVE_TO_VANTAGE;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_SniperBehavior::VerifyShot(CBaseEntity* pTarget)
{
	trace_t tr;

	Vector vecTarget = DesiredBodyTarget(pTarget);
	UTIL_TraceLine(GetBulletOrigin(), vecTarget, MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction != 1.0)
	{
		if (pTarget->IsPlayer())
		{
			// if the target is the player, do another trace to see if we can shoot his eyeposition. This should help 
			// improve sniper responsiveness in cases where the player is hiding his chest from the sniper with his 
			// head in full view.
			UTIL_TraceLine(GetBulletOrigin(), pTarget->EyePosition(), MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr);

			if (tr.fraction == 1.0)
			{
				return true;
			}
		}

		// Trace hit something.
		if (tr.m_pEnt)
		{
			if (tr.m_pEnt->m_takedamage == DAMAGE_YES)
			{
				// Just shoot it if I can hurt it. Probably a breakable or glass pane.
				return true;
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

bool CAI_SniperBehavior::FindDecoyObject(void)
{
#define SEARCH_DEPTH	50

	CBaseEntity* pDecoys[SNIPER_NUM_DECOYS];
	CBaseEntity* pList[SEARCH_DEPTH];
	CBaseEntity* pCurrent;
	int			count;
	int			i;
	Vector vecTarget = GetEnemy()->WorldSpaceCenter();
	Vector vecDelta;

	m_hDecoyObject = NULL;

	for (i = 0; i < SNIPER_NUM_DECOYS; i++)
	{
		pDecoys[i] = NULL;
	}

	vecDelta.x = SNIPER_DECOY_RADIUS;
	vecDelta.y = SNIPER_DECOY_RADIUS;
	vecDelta.z = SNIPER_DECOY_RADIUS;

	count = UTIL_EntitiesInBox(pList, SEARCH_DEPTH, vecTarget - vecDelta, vecTarget + vecDelta, 0);

	// Now we have the list of entities near the target. 
	// Dig through that list and build the list of decoys.
	int iIterator = 0;

	for (i = 0; i < count; i++)
	{
		pCurrent = pList[i];

		if (FClassnameIs(pCurrent, "func_breakable") || FClassnameIs(pCurrent, "prop_physics") || FClassnameIs(pCurrent, "func_physbox"))
		{
			if (!pCurrent->VPhysicsGetObject())
				continue;

			if (pCurrent->VPhysicsGetObject()->GetMass() > SNIPER_DECOY_MAX_MASS)
			{
				// Skip this very heavy object. Probably a car or dumpster.
				continue;
			}

			if (pCurrent->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
			{
				// Ah! If the player is holding something, try to shoot it!
				if (GetOuter()->FVisible(pCurrent))
				{
					m_hDecoyObject = pCurrent;
					m_vecDecoyObjectTarget = pCurrent->WorldSpaceCenter();
					return true;
				}
			}

			// This item meets criteria for a decoy object to shoot at. 

			// But have we shot at this item recently? If we HAVE, don't add it.
#if 0
			if (!HasOldDecoy(pCurrent))
#endif
			{
				pDecoys[iIterator] = pCurrent;

				if (iIterator == SNIPER_NUM_DECOYS - 1)
				{
					break;
				}
				else
				{
					iIterator++;
				}
			}
		}
	}

	if (iIterator == 0)
	{
		return false;
	}

	// try 4 times to pick a random object from the list
	// and trace to it. If the trace goes off, that's the object!

	for (i = 0; i < 4; i++)
	{
		CBaseEntity* pProspect;
		trace_t		tr;

		// Pick one of the decoys at random.
		pProspect = pDecoys[random->RandomInt(0, iIterator - 1)];

		Vector vecDecoyTarget;
		Vector vecDirToDecoy;
		Vector vecBulletOrigin;

		vecBulletOrigin = GetBulletOrigin();
		pProspect->CollisionProp()->RandomPointInBounds(Vector(.1, .1, .1), Vector(.6, .6, .6), &vecDecoyTarget);

		// When trying to trace to an object using its absmin + some fraction of its size, it's best 
		// to lengthen the trace a little beyond the object's bounding box in case it's a more complex
		// object, or not axially aligned. 
		vecDirToDecoy = vecDecoyTarget - vecBulletOrigin;
		VectorNormalize(vecDirToDecoy);


		// Right now, tracing with MASK_BLOCKLOS and checking the fraction as well as the object the trace
		// has hit makes it possible for the decoy behavior to shoot through glass. 
		UTIL_TraceLine(vecBulletOrigin, vecDecoyTarget + vecDirToDecoy * 32,
			MASK_BLOCKLOS, GetOuter(), COLLISION_GROUP_NONE, &tr);

		if (tr.m_pEnt == pProspect || tr.fraction == 1.0)
		{
			// Great! A shot will hit this object.
			m_hDecoyObject = pProspect;
			m_vecDecoyObjectTarget = tr.endpos;

			// Throw some noise in, don't always hit the center.
			Vector vecNoise;
			pProspect->CollisionProp()->RandomPointInBounds(Vector(0.25, 0.25, 0.25), Vector(0.75, 0.75, 0.75), &vecNoise);
			m_vecDecoyObjectTarget += vecNoise - pProspect->GetAbsOrigin();
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CAI_SniperBehavior::RangeAttack1Conditions(float flDot, float flDist)
{
	float fFrustration;
	fFrustration = gpGlobals->curtime - m_flFrustration;

	//Msg( "Frustration: %f\n", fFrustration );

	if (HasCondition(COND_SEE_ENEMY) && !HasCondition(COND_ENEMY_OCCLUDED))
	{
		if (VerifyShot(GetEnemy()))
		{
			// Can see the enemy, have a clear shot to his midsection
			ClearCondition(COND_SNIPERRIFLE_NO_SHOT);
		}
		else
		{
			// Can see the enemy, but can't take a shot at his midsection
			SetCondition(COND_SNIPERRIFLE_NO_SHOT);
			return COND_NONE;
		}

		if (m_fIsPatient)
		{
			// This sniper has a clear shot at the target, but can not take
			// the shot if he is being patient and the target is outside
			// of the patience radius.

			float flDist;

			flDist = (GetLocalOrigin() - GetEnemy()->GetLocalOrigin()).Length2D();

			if (flDist <= m_flPatience)
			{
				// This target is close enough to attack!
				return COND_CAN_RANGE_ATTACK1;
			}
			else
			{
				// Be patient...
				return COND_NONE;
			}
		}
		else
		{
			// Not being patient. Clear for attack.
			return COND_CAN_RANGE_ATTACK1;
		}
	}

	if (fFrustration >= 2 && !m_fIsPatient)
	{
		if (!m_hDecoyObject && FindDecoyObject())
		{
			// If I don't have a decoy, try to find one and shoot it.
			return COND_SNIPERRIFLE_CANATTACKDECOY;
		}


		if (fFrustration >= 2.5)
		{
			// Otherwise, just fire somewhere near the hiding enemy.
			return COND_SNIPERRIFLE_FRUSTRATED;
		}
	}

	return COND_NONE;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CAI_SniperBehavior::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_RANGE_ATTACK1:
		if (m_fSnapShot && ShouldSnapShot())
		{
			return SCHED_SNIPERRIFLE_SNAPATTACK;
		}

		return SCHED_SNIPERRIFLE_ATTACK;
		break;

	case SCHED_RANGE_ATTACK2:
		return SCHED_SNIPERRIFLE_ATTACKDECOY;
		break;

	case SCHED_HIDE_AND_RELOAD:
		if (IsWeaponReady())
		{
			return SCHED_RELOAD;
			break;
		}
	}
	return BaseClass::TranslateSchedule(scheduleType);
}

bool CAI_SniperBehavior::FValidateHintType(CAI_Hint* pHint)
{
	if (GetEnemy())
	{
		Vector	hintPos;
		pHint->GetPosition(GetOuter(), &hintPos);

		// Verify that we can see the target from that position
		hintPos += GetOuter()->GetViewOffset();

		trace_t	tr;
		UTIL_TraceLine(hintPos, DesiredBodyTarget(GetEnemy()), MASK_SHOT, GetOuter(), COLLISION_GROUP_NONE, &tr);

		// Check for seeing our target at the new location
		if ((tr.fraction == 1.0f) || (tr.m_pEnt == GetEnemy()))
			return false;

		return true;
	}

	return false;
}

void CAI_SniperBehavior::AimGun(void)
{
	if (IsWeaponReady())
	{
		Vector vecForward;
		vecForward = m_vecPaintCursor - GetBulletOrigin();

		GetMotor()->SetIdealYawToTargetAndUpdate(m_vecPaintCursor);
		GetOuter()->SetAim(vecForward);
		return;
	}

	BaseClass::AimGun();
}
//---------------------------------------------------------
//---------------------------------------------------------
Vector CAI_SniperBehavior::DesiredBodyTarget(CBaseEntity* pTarget)
{
	// By default, aim for the center
	Vector vecTarget = pTarget->WorldSpaceCenter();

	float flTimeSinceLastMiss = gpGlobals->curtime - m_flTimeLastShotMissed;

	if (pTarget->GetFlags() & FL_CLIENT)
	{
		if (!GetOuter()->FVisible(vecTarget))
		{
			// go to the player's eyes if his center is concealed.
			// Bump up an inch so the player's not looking straight down a beam.
			vecTarget = pTarget->EyePosition() + Vector(0, 0, 1);
		}
	}
	else
	{
		if (pTarget->Classify() == CLASS_HEADCRAB)
		{
			// Headcrabs are tiny inside their boxes.
			vecTarget = pTarget->GetAbsOrigin();
			vecTarget.z += 4.0;
		}
		else if (!m_bShootZombiesInChest && pTarget->Classify() == CLASS_ZOMBIE)
		{
			if (flTimeSinceLastMiss > 0.0f && flTimeSinceLastMiss < 4.0f && hl2_episodic.GetBool())
			{
				vecTarget = pTarget->BodyTarget(GetBulletOrigin(), false);
			}
			else
			{
				// Shoot zombies in the headcrab
				vecTarget = pTarget->HeadTarget(GetBulletOrigin());
			}
		}
		else if (pTarget->Classify() == CLASS_ANTLION)
		{
			// Shoot about a few inches above the origin. This makes it easy to hit antlions
			// even if they are on their backs.
			vecTarget = pTarget->GetAbsOrigin();
			vecTarget.z += 18.0f;
		}
		else if (pTarget->Classify() == CLASS_EARTH_FAUNA)
		{
			// Shoot birds in the center
		}
		else
		{
			// Shoot NPCs in the chest
			vecTarget.z += 8.0f;
		}
	}

	return vecTarget;
}
//---------------------------------------------------------
//---------------------------------------------------------
Vector CAI_SniperBehavior::LeadTarget(CBaseEntity* pTarget)
{
	float targetTime;
	float targetDist;
	//float adjustedShotDist;
	//float actualShotDist;
	Vector vecAdjustedShot;
	Vector vecTarget;
	trace_t tr;

	if (pTarget == NULL)
	{
		// no target
		return vec3_origin;
	}

	// Get target
	vecTarget = DesiredBodyTarget(pTarget);

	// Get bullet time to target
	targetDist = (vecTarget - GetBulletOrigin()).Length();
	targetTime = targetDist / GetBulletSpeed();

	// project target's velocity over that time. 
	Vector vecVelocity = vec3_origin;

	if (pTarget->IsPlayer() || pTarget->Classify() == CLASS_MISSILE)
	{
		// This target is a client, who has an actual velocity.
		vecVelocity = pTarget->GetSmoothedVelocity();

		// Slow the vertical velocity down a lot, or the sniper will
		// lead a jumping player by firing several feet above his head.
		// THIS may affect the sniper hitting a player that's ascending/descending
		// ladders. If so, we'll have to check for the player's ladder flag.
		if (pTarget->GetFlags() & FL_CLIENT)
		{
			vecVelocity.z *= 0.25;
		}
	}
	else
	{
		if (pTarget->MyNPCPointer() && pTarget->MyNPCPointer()->GetNavType() == NAV_FLY)
		{
			// Take a flying monster's velocity directly.
			vecVelocity = pTarget->GetAbsVelocity();
		}
		else
		{
			// Have to build a velocity vector using the character's current groundspeed.
			CBaseAnimating* pAnimating;

			pAnimating = (CBaseAnimating*)pTarget;

			Assert(pAnimating != NULL);

			QAngle vecAngle;
			vecAngle.y = pAnimating->GetSequenceMoveYaw(pAnimating->GetSequence());
			vecAngle.x = 0;
			vecAngle.z = 0;

			vecAngle.y += pTarget->GetLocalAngles().y;

			AngleVectors(vecAngle, &vecVelocity);

			vecVelocity = vecVelocity * pAnimating->m_flGroundSpeed;
		}
	}

	if (m_iMisses > 0 && !FClassnameIs(pTarget, "npc_bullseye"))
	{
		// I'm supposed to miss this shot, so aim above the target's head.
		// BUT DON'T miss bullseyes, and don't count the shot.
		vecAdjustedShot = vecTarget;
		vecAdjustedShot.z += 16;

		m_iMisses--;

		// NDebugOverlay::Cross3D(vecAdjustedShot,12.0f,255,0,0,false,1);

		return vecAdjustedShot;
	}

	vecAdjustedShot = vecTarget + (vecVelocity * targetTime);

	// if the adjusted shot falls well short of the target, take the straight shot.
	// it's not very interesting for the bullet to hit something far away from the 
	// target. (for instance, if a sign or ledge or something is between the player
	// and the sniper, and the sniper would hit this object if he tries to lead the player)

	/*
		UTIL_TraceLine( vecBulletOrigin, vecAdjustedShot, MASK_SHOT, this, &tr );

		actualShotDist = (tr.endpos - vecBulletOrigin ).Length();
		adjustedShotDist = ( vecAdjustedShot - vecBulletOrigin ).Length();

		/////////////////////////////////////////////
		// the shot taken should hit within 10% of the sniper's distance to projected target.
		// else, shoot straight. (there's some object in the way of the adjusted shot)
		/////////////////////////////////////////////
		if( actualShotDist <= adjustedShotDist * 0.9 )
		{
			vecAdjustedShot = vecTarget;
		}
	*/
	return vecAdjustedShot;
}
bool CAI_SniperBehavior::FireBullet(const Vector& vecTarget, bool bDirectShot)
{
	m_pWeapon->FireNPCPrimaryAttack(GetOuter(), false, &vecTarget);

	GetOuter()->SetLastAttackTime(gpGlobals->curtime);
	GetOuter()->ResetIdealActivity(ACT_RANGE_ATTACK1);

	// Once the sniper takes a shot, turn the patience off!
	m_fIsPatient = false;

	// Alleviate frustration, too!
	m_flFrustration = gpGlobals->curtime;

	// This may have been a snap shot.
	// Don't allow subsequent snap shots.
	m_fSnapShot = false;

	// Sniper had to be aiming here to fire here.
	// Make it the cursor.
	m_vecPaintCursor = vecTarget;

	m_hDecoyObject.Set(NULL);

	return true;
}
//---------------------------------------------------------
//---------------------------------------------------------
#define SNIPER_SNAP_SHOT_VELOCITY	125
bool CAI_SniperBehavior::ShouldSnapShot(void)
{
	if (GetEnemy()->IsPlayer())
	{
		if (GetEnemy()->GetSmoothedVelocity().Length() >= SNIPER_SNAP_SHOT_VELOCITY)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	// Right now, always snapshot at NPC's
	return true;
}
void CAI_SniperBehavior::StartTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SNIPERRIFLE_ATTACK_CURSOR:
		break;

	case TASK_SNIPERRIFLE_ATTACK_ENEMY:
		// Start task does nothing here.
		// We fall through to RunTask() which will keep trying to take
		// the shot until the weapon is ready to fire. In some rare cases,
		// the weapon may be ready to fire before the single bullet allocated
		// to the sniper has hit its target. 
		break;

	case TASK_SNIPERRIFLE_ATTACK_DECOY:
		// Don't call up to base class, it will try to set the activity.
		break;

	case TASK_SNIPERRIFLE_PAINT_ENEMY:
		// Everytime we start to paint an enemy, this is reset to false.
		m_bWarnedTargetEntity = false;

		//if (m_spawnflags & SF_SNIPER_VIEWCONE)
		//{
		//	SetWait(SNIPER_FOG_PAINT_ENEMY_TIME);

		//	// Just turn it on where it is.
		//	LaserOn(m_vecPaintCursor, vec3_origin);
		//}
		//else
		{
			if (GetEnemy()->IsPlayer())
			{
				float delay = 0;
#ifdef _XBOX
				delay += sniper_xbox_delay.GetFloat();
#endif

				if (gpGlobals->curtime - m_flTimeLastAttackedPlayer <= SNIPER_FASTER_ATTACK_PERIOD)
				{
					SetWait(SNIPER_SUBSEQUENT_PAINT_TIME + delay);
					m_flPaintTime = SNIPER_SUBSEQUENT_PAINT_TIME + delay;
				}
				else
				{
					SetWait(SNIPER_PAINT_ENEMY_TIME + delay);
					m_flPaintTime = SNIPER_PAINT_ENEMY_TIME + delay;
				}
			}
			else
			{
				m_flPaintTime = SNIPER_PAINT_ENEMY_TIME + random->RandomFloat(0, SNIPER_PAINT_NPC_TIME_NOISE);

				//if (IsFastSniper())
				//{
				//	// Get the shot off a little faster.
				//	m_flPaintTime *= 0.75f;
				//}

				SetWait(m_flPaintTime);
			}

			Vector vecCursor;

			/*if (m_spawnflags & SF_SNIPER_NOSWEEP)
			{
				LaserOn(m_vecPaintCursor, vec3_origin);
			}
			else*/
			{
				// Try to start the laser where the player can't miss seeing it!
				AngleVectors(GetEnemy()->GetLocalAngles(), &vecCursor);
				vecCursor = vecCursor * 300;
				vecCursor += GetEnemy()->EyePosition();
				LaserOn(vecCursor, Vector(16, 16, 16));
			}

		}
		break;

	case TASK_SNIPERRIFLE_PAINT_NO_SHOT:
		SetWait(SNIPER_PAINT_NO_SHOT_TIME);
		if (FindFrustratedShot(pTask->flTaskData))
		{
			LaserOff();
			LaserOn(m_vecFrustratedTarget, vec3_origin);
		}
		else
		{
			TaskFail("Frustrated shot with no enemy");
		}
		break;

	case TASK_SNIPERRIFLE_PAINT_FRUSTRATED:
		m_flPaintTime = SNIPER_PAINT_FRUSTRATED_TIME + random->RandomFloat(0, SNIPER_PAINT_FRUSTRATED_TIME);
		SetWait(m_flPaintTime);
		if (FindFrustratedShot(pTask->flTaskData))
		{
			LaserOff();
			LaserOn(m_vecFrustratedTarget, vec3_origin);
		}
		else
		{
			TaskFail("Frustrated shot with no enemy");
		}
		break;

	case TASK_SNIPERRIFLE_PAINT_DECOY:
		SetWait(pTask->flTaskData);
		LaserOn(m_vecDecoyObjectTarget, Vector(64, 64, 64));
		break;

	case TASK_SNIPERRIFLE_FRUSTRATED_ATTACK:
		//FindFrustratedShot();
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}
//---------------------------------------------------------
//---------------------------------------------------------
void CAI_SniperBehavior::RunTask(const Task_t* pTask)
{
	switch (pTask->iTask)
	{
	
	case TASK_SNIPERRIFLE_ATTACK_CURSOR:
		if (FireBullet(m_vecPaintCursor, true))
		{
			TaskComplete();
		}
		break;

	case TASK_SNIPERRIFLE_ATTACK_ENEMY:
		// Fire at enemy.
		if (FireBullet(LeadTarget(GetEnemy()), true))
		{
			// Msg("Firing at %s\n",GetEnemy()->GetEntityName().ToCStr());

			if (GetEnemy() && GetEnemy()->IsPlayer())
			{
				m_flTimeLastAttackedPlayer = gpGlobals->curtime;
			}

			TaskComplete();
		}
		else
		{
			// Msg("Firebullet %s is false\n",GetEnemy()->GetEntityName().ToCStr());
		}
		break;

	case TASK_SNIPERRIFLE_FRUSTRATED_ATTACK:
		if (FireBullet(m_vecFrustratedTarget, false))
		{
			TaskComplete();
		}
		break;

	

	case TASK_SNIPERRIFLE_PAINT_ENEMY:
		if (IsWaitFinished())
		{
			TaskComplete();
		}

		PaintTarget(LeadTarget(GetEnemy()), m_flPaintTime);
		break;

	case TASK_SNIPERRIFLE_PAINT_DECOY:
		if (IsWaitFinished())
		{
			TaskComplete();
		}

		PaintTarget(m_vecDecoyObjectTarget, pTask->flTaskData);
		break;

	case TASK_SNIPERRIFLE_PAINT_NO_SHOT:
		if (IsWaitFinished())
		{
			//HACKHACK(sjb)
			// This condition should be turned off 
			// by a task.
			ClearCondition(COND_SNIPERRIFLE_NO_SHOT);
			TaskComplete();
		}

		PaintTarget(m_vecFrustratedTarget, SNIPER_PAINT_NO_SHOT_TIME);
		break;

	case TASK_SNIPERRIFLE_PAINT_FRUSTRATED:
		if (IsWaitFinished())
		{
			TaskComplete();
		}

		PaintTarget(m_vecFrustratedTarget, m_flPaintTime);
		break;

	case TASK_SNIPERRIFLE_ATTACK_DECOY:
		// Fire at decoy
		if (m_hDecoyObject == NULL)
		{
			TaskFail("sniper: bad decoy");
			break;
		}

		if (FireBullet(m_vecDecoyObjectTarget, false))
		{
			//Msg( "Fired at decoy\n" );
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}
void CAI_SniperBehavior::LaserOff(void)
{
	m_pWeapon->NPC_SetLaserState(false);
}
void CAI_SniperBehavior::LaserOn(const Vector& vecTarget, const Vector& vecDeviance)
{
	m_pWeapon->NPC_SetLaserState(true);

	// Don't aim right at the guy right now.
	Vector vecInitialAim;

	if (vecDeviance == vec3_origin)
	{
		// Start the aim where it last left off!
		vecInitialAim = m_vecPaintCursor;
	}
	else
	{
		vecInitialAim = vecTarget;
	}

	vecInitialAim.x += random->RandomFloat(-vecDeviance.x, vecDeviance.x);
	vecInitialAim.y += random->RandomFloat(-vecDeviance.y, vecDeviance.y);
	vecInitialAim.z += random->RandomFloat(-vecDeviance.z, vecDeviance.z);

	m_vecPaintStart = vecInitialAim;
}
//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_SniperBehavior::FindFrustratedShot(float flNoise)
{
	if (!GetEnemy())
	{
		return false;
	}

	// Just pick a spot somewhere around the target.
	// Try a handful of times to pick a spot that guarantees the 
	// target will see the laser.
#define MAX_TRIES	15
	for (int i = 0; i < MAX_TRIES; i++)
	{
		Vector vecSpot = GetEnemyLKP();

		vecSpot.x += random->RandomFloat(-64, 64);
		vecSpot.y += random->RandomFloat(-64, 64);
		vecSpot.z += random->RandomFloat(-40, 40);

		// Help move the frustrated spot off the target's BBOX in X/Y space.
		if (vecSpot.x < 0)
			vecSpot.x -= 32;
		else
			vecSpot.x += 32;

		if (vecSpot.y < 0)
			vecSpot.y -= 32;
		else
			vecSpot.y += 32;

		Vector vecSrc, vecDir;

		vecSrc = GetAbsOrigin();
		vecDir = vecSpot - vecSrc;
		VectorNormalize(vecDir);

		if (GetEnemy()->FVisible(vecSpot) || i == MAX_TRIES - 1)
		{
			trace_t tr;
			AI_TraceLine(vecSrc, vecSrc + vecDir * 8192, MASK_SHOT, GetOuter(), COLLISION_GROUP_NONE, &tr);

			if (!GetEnemy()->FVisible(tr.endpos))
			{
				// Dont accept this point unless we are out of tries!
				if (i != MAX_TRIES - 1)
				{
					continue;
				}
			}
			m_vecFrustratedTarget = tr.endpos;
			break;
		}
	}

#if 0
	NDebugOverlay::Line(vecStart, tr.endpos, 0, 255, 0, true, 20);
#endif

	return true;
}
bool CAI_SniperBehavior::IsWeaponEquiped()
{
	return GetOuter() && GetOuter()->GetActiveWeapon() == m_pWeapon;
}
bool CAI_SniperBehavior::IsWeaponReady()
{
	return IsWeaponEquiped() && m_pWeapon->IsLaserEnabled();
}
//-----------------------------------------------------------------------------
// Crikey!
//-----------------------------------------------------------------------------
float CAI_SniperBehavior::GetPositionParameter(float flTime, bool fLinear)
{
	float flElapsedTime;
	float flTimeParameter;

	flElapsedTime = flTime - (GetWaitFinishTime() - gpGlobals->curtime);

	flTimeParameter = (flElapsedTime / flTime);

	if (fLinear)
	{
		return flTimeParameter;
	}
	else
	{
		return (1 + sin((M_PI * flTimeParameter) - (M_PI / 2))) / 2;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_SniperBehavior::GetPaintAim(const Vector& vecStart, const Vector& vecGoal, float flParameter, Vector* pProgress)
{
#if 0
	Vector vecDelta;

	vecDelta = vecGoal - vecStart;

	float flDist = VectorNormalize(vecDelta);

	vecDelta = vecStart + vecDelta * (flDist * flParameter);

	vecDelta = (vecDelta - GetBulletOrigin()).Normalize();

	*pProgress = vecDelta;
#else
	// Quaternions
	Vector vecIdealDir;
	QAngle vecIdealAngles;
	QAngle vecCurrentAngles;
	Vector vecCurrentDir;
	Vector vecBulletOrigin = GetBulletOrigin();

	// vecIdealDir is where the gun should be aimed when the painting
	// time is up. This can be approximate. This is only for drawing the
	// laser, not actually aiming the weapon. A large discrepancy will look
	// bad, though.
	vecIdealDir = vecGoal - vecBulletOrigin;
	VectorNormalize(vecIdealDir);

	// Now turn vecIdealDir into angles!
	VectorAngles(vecIdealDir, vecIdealAngles);

	// This is the vector of the beam's current aim.
	vecCurrentDir = vecStart - vecBulletOrigin;
	VectorNormalize(vecCurrentDir);

	// Turn this to angles, too.
	VectorAngles(vecCurrentDir, vecCurrentAngles);

	Quaternion idealQuat;
	Quaternion currentQuat;
	Quaternion aimQuat;

	AngleQuaternion(vecIdealAngles, idealQuat);
	AngleQuaternion(vecCurrentAngles, currentQuat);

	QuaternionSlerp(currentQuat, idealQuat, flParameter, aimQuat);

	QuaternionAngles(aimQuat, vecCurrentAngles);

	// Rebuild the current aim vector.
	AngleVectors(vecCurrentAngles, &vecCurrentDir);

	*pProgress = vecCurrentDir;
#endif
}
void CAI_SniperBehavior::PaintTarget(const Vector& vecTarget, float flPaintTime)
{
	Vector vecCurrentDir;
	Vector vecStart;

	// vecStart is the barrel of the gun (or the laser sight)
	vecStart = GetBulletOrigin();

	float P;

	// keep painttime from hitting 0 exactly.
	flPaintTime = MAX(flPaintTime, 0.000001f);

	P = GetPositionParameter(flPaintTime, false);

	// Vital allies are sharper about avoiding the sniper.
	if (P > 0.25f && GetEnemy() && GetEnemy()->IsNPC() && HasCondition(COND_SEE_ENEMY) && !m_bWarnedTargetEntity)
	{
		m_bWarnedTargetEntity = true;

		if (GetEnemy()->Classify() == CLASS_PLAYER_ALLY_VITAL && GetEnemy()->MyNPCPointer()->FVisible(GetOuter()))
		{
			CSoundEnt::InsertSound(SOUND_DANGER | SOUND_CONTEXT_REACT_TO_SOURCE, GetEnemy()->EarPosition(), 16, 1.0f, GetOuter());
		}
	}

	GetPaintAim(m_vecPaintStart, vecTarget, clamp(P, 0.0f, 1.0f), &vecCurrentDir);

#if 1
#define THRESHOLD 0.8f
	float flNoiseScale;

	if (P >= THRESHOLD)
	{
		flNoiseScale = 1 - (1 / (1 - THRESHOLD)) * (P - THRESHOLD);
	}
	else if (P <= 1 - THRESHOLD)
	{
		flNoiseScale = P / (1 - THRESHOLD);
	}
	else
	{
		flNoiseScale = 1;
	}

	// mult by P
	vecCurrentDir.x += flNoiseScale * (sin(3 * M_PI * gpGlobals->curtime) * 0.0006);
	vecCurrentDir.y += flNoiseScale * (sin(2 * M_PI * gpGlobals->curtime + 0.5 * M_PI) * 0.0006);
	vecCurrentDir.z += flNoiseScale * (sin(1.5 * M_PI * gpGlobals->curtime + M_PI) * 0.0006);
#endif

	trace_t tr;

	UTIL_TraceLine(vecStart, vecStart + vecCurrentDir * 8192, MASK_SHOT, GetOuter(), COLLISION_GROUP_NONE, &tr);

	m_vecPaintCursor = tr.endpos;
}

//=============================================================================
//
// Custom AI schedule data
//

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_SniperBehavior)

DECLARE_CONDITION(COND_SNIPERRIFLE_CANATTACKDECOY);
DECLARE_CONDITION(COND_SNIPERRIFLE_SUPPRESSED);
DECLARE_CONDITION(COND_SNIPERRIFLE_FRUSTRATED);
DECLARE_CONDITION(COND_SNIPERRIFLE_NO_SHOT);

DECLARE_TASK(TASK_SNIPERRIFLE_FRUSTRATED_ATTACK);
DECLARE_TASK(TASK_SNIPERRIFLE_PAINT_ENEMY);
DECLARE_TASK(TASK_SNIPERRIFLE_PAINT_DECOY);
DECLARE_TASK(TASK_SNIPERRIFLE_PAINT_FRUSTRATED);
DECLARE_TASK(TASK_SNIPERRIFLE_ATTACK_CURSOR);
DECLARE_TASK(TASK_SNIPERRIFLE_ATTACK_ENEMY);
DECLARE_TASK(TASK_SNIPERRIFLE_ATTACK_DECOY);
DECLARE_TASK(TASK_SNIPERRIFLE_PAINT_NO_SHOT);

//=========================================================
// SCAN
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_SCAN,

	"	Tasks"
	"		TASK_WAIT_INDEFINITE		0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAR_DANGER"
)

//=========================================================
// CAMP
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_CAMP,

	"	Tasks"
	"		TASK_WAIT_INDEFINITE		0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_SNIPERRIFLE_CANATTACKDECOY"
	"		COND_SNIPERRIFLE_SUPPRESSED"
	"		COND_HEAR_DANGER"
	"		COND_SNIPERRIFLE_FRUSTRATED"
)

//=========================================================
// ATTACK
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_ATTACK,

	"	Tasks"
	"		TASK_SNIPERRIFLE_PAINT_ENEMY		0"
	"		TASK_SNIPERRIFLE_ATTACK_ENEMY		0"
	"	"
	"	Interrupts"
	"		COND_ENEMY_OCCLUDED"
	"		COND_ENEMY_DEAD"
	"		COND_HEAR_DANGER"
)

//=========================================================
// ATTACK
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_SNAPATTACK,

	"	Tasks"
	"		TASK_SNIPERRIFLE_ATTACK_CURSOR	0"
	"	"
	"	Interrupts"
	"		COND_ENEMY_OCCLUDED"
	"		COND_ENEMY_DEAD"
	"		COND_NEW_ENEMY"
	"		COND_HEAR_DANGER"
)

//=========================================================
// Attack decoy
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_ATTACKDECOY,

	"	Tasks"
	"		TASK_SNIPERRIFLE_PAINT_DECOY		2.0"
	"		TASK_SNIPERRIFLE_ATTACK_DECOY		0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAR_DANGER"
	"		COND_CAN_RANGE_ATTACK1"
)

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_SUPPRESSED,

	"	Tasks"
	"		TASK_WAIT			2.0"
	"	"
	"	Interrupts"
)

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_FRUSTRATED_ATTACK,

	"	Tasks"
	"		TASK_WAIT						2.0"
	"		TASK_SNIPERRIFLE_PAINT_FRUSTRATED	0.05"
	"		TASK_SNIPERRIFLE_PAINT_FRUSTRATED	0.025"
	"		TASK_SNIPERRIFLE_PAINT_FRUSTRATED	0.0"
	"		TASK_SNIPERRIFLE_FRUSTRATED_ATTACK	0.0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_SEE_ENEMY"
	"		COND_HEAR_DANGER"
)

//=========================================================
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_NO_CLEAR_SHOT,

	"	Tasks"
	"		TASK_SNIPERRIFLE_PAINT_NO_SHOT	0.0"
	"		TASK_SNIPERRIFLE_PAINT_NO_SHOT	0.075"
	"		TASK_SNIPERRIFLE_PAINT_NO_SHOT	0.05"
	"		TASK_SNIPERRIFLE_PAINT_NO_SHOT	0.0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_CAN_RANGE_ATTACK1"
	"		COND_HEAR_DANGER"
)

DEFINE_SCHEDULE
(
	SCHED_SNIPERRIFLE_MOVE_TO_VANTAGE,

	"	Tasks"
	"		TASK_SET_FAIL_SCHEDULE		Schedule:SCHED_ESTABLISH_LINE_OF_FIRE"
	"		TASK_FIND_HINTNODE			2050.0"
	"		TASK_GET_PATH_TO_HINTNODE	0.0"
	"		TASK_RUN_PATH				0.0"
	"		TASK_WAIT_FOR_MOVEMENT		0.0"
	"		TASK_STOP_MOVING			0.0"
	"		TASK_FACE_ENEMY				0.0"
	"	"
	"	Interrupts"
	"		COND_NEW_ENEMY"
	"		COND_ENEMY_DEAD"
	"		COND_HEAR_DANGER"
)

AI_END_CUSTOM_SCHEDULE_PROVIDER()
#endif // !CLIENT_DLL
