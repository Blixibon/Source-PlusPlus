//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "rumble_shared.h"
#include "gamestats.h"

#ifndef CLIENT_DLL
#include "ai_memory.h"
#include "soundent.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "game.h"
#include "basehlcombatweapon.h"
#else
#define CWeaponMP5K C_WeaponMP5K
#endif

#include "weapon_coop_basemachinegun.h"

#define	COMBINE_MIN_GRENADE_CLEAR_DIST 256


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



class CWeaponMP5K : public CWeaponCoopMachineGun
{
public:
	DECLARE_CLASS(CWeaponMP5K, CWeaponCoopMachineGun);

	CWeaponMP5K();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	//DECLARE_ACTTABLE();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_SMG2; }

	void	Precache(void);
	void	AddViewKick(void);

	int		GetMinBurst() { return 5; }
	int		GetMaxBurst() { return 10; }

	virtual void Equip(CBaseCombatCharacter* pOwner);
	bool	Reload(void);
	float	GetFireRate(void) { return RPM_TO_HZ(900.f); }

	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void)
	{
		if (IsIronsighted())
		{
			static Vector ironCone(0.02f);
			return ironCone;
		}

		static Vector cone;
#define SMG2_BIGGEST_SPREAD 0.14f
#define SMG2_SMALLEST_SPREAD 0.04f

		//static const Vector cone = VECTOR_CONE_5DEGREES;
		float flCone = Clamp((float)m_nShotsFired / 30.0f, 0.0f, 1.0f);
		flCone = (SMG2_SMALLEST_SPREAD * (1.0f - flCone)) + (SMG2_BIGGEST_SPREAD * flCone);
		cone.Init(flCone, flCone, flCone + 0.008);
		return cone;
	}

#ifndef CLIENT_DLL
	int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	const WeaponProficiencyInfo_t* GetProficiencyValues();
	void FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir);
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
#endif

#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif

private:
	CWeaponMP5K(const CWeaponMP5K&);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMP5K, DT_WeaponMP5K)

BEGIN_NETWORK_TABLE(CWeaponMP5K, DT_WeaponMP5K)
END_NETWORK_TABLE();

BEGIN_PREDICTION_DATA(CWeaponMP5K)
END_PREDICTION_DATA();

LINK_ENTITY_TO_CLASS(weapon_mp5k, CWeaponMP5K);
PRECACHE_WEAPON_REGISTER(weapon_mp5k);

#ifndef CLIENT_DLL
BEGIN_DATADESC(CWeaponMP5K)

END_DATADESC();
#endif

//=========================================================
CWeaponMP5K::CWeaponMP5K()
{
	m_fMinRange1 = 0;// No minimum range. 
	m_fMaxRange1 = 1400;

	m_bAltFiresUnderwater = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5K::Precache(void)
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponMP5K::Equip(CBaseCombatCharacter* pOwner)
{
#ifndef CLIENT_DLL
	if (pOwner->Classify() == CLASS_PLAYER_ALLY)
	{
		m_fMaxRange1 = 3000;
	}
	else
#endif
	{
		m_fMaxRange1 = 1400;
	}


	BaseClass::Equip(pOwner);
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t* CWeaponMP5K::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0 / 3.0, 0.75	},
		{ 5.0 / 3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT(ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5K::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir)
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0);

	pOperator->DoMuzzleFlash();
	EmitLowAmmoSound(1);
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5K::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;
	UpdateNPCShotCounter();

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5K::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	{
		Vector vecShootOrigin, vecShootDir;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
		{
			vecShootOrigin = pOperator->Weapon_ShootPosition();
		}

		UpdateNPCShotCounter();
		CAI_BaseNPC* npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

		FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
	break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponMP5K::GetPrimaryAttackActivity(void)
{
	return IsIronsighted() ? ACT_VM_PRIMARYATTACK_1 : ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponMP5K::Reload(void)
{
	bool fRet;
	//float fCacheTime = m_flNextSecondaryAttack;

	fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), Clip1() ? ACT_VM_RELOAD : ACT_VM_RELOAD_EMPTY);
	if (fRet)
	{
		WeaponSound(RELOAD);
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponMP5K::AddViewKick(void)
{
#define	EASY_DAMPEN			0.5f
#define	MAX_VERTICAL_KICK	2.0f	//Degrees
#define	SLIDE_LIMIT			4.0f	//Seconds

	//Get the view kick
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	DoMachineGunKick(pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT);
}