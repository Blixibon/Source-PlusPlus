#include "cbase.h"
#include "weapon_coop_basehlcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponMedkit C_WeaponMedkit

#include "c_te_effect_dispatch.h"
#include "c_team.h"
#else
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "soundent.h"
#include "game.h"
#include "te_effect_dispatch.h"
#include "ilagcompensationmanager.h"
#endif

#include "hl2_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponMedkit
//-----------------------------------------------------------------------------

class CWeaponMedkit : public CWeaponCoopBaseHLCombat
{
	DECLARE_CLASS(CWeaponMedkit, CWeaponCoopBaseHLCombat);
public:

	CWeaponMedkit(void);

	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	//void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	//float	WeaponAutoAimScale() { return 0.6f; }

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_MEDKIT; }
	void		WeaponIdle();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();
	// DECLARE_ACTTABLE();
};

LINK_ENTITY_TO_CLASS(weapon_medkit, CWeaponMedkit);
PRECACHE_WEAPON_REGISTER(weapon_medkit);

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMedkit, DT_WeaponMedkit)

BEGIN_NETWORK_TABLE(CWeaponMedkit, DT_WeaponMedkit)
END_NETWORK_TABLE();

BEGIN_PREDICTION_DATA(CWeaponMedkit)
END_PREDICTION_DATA();

BEGIN_DATADESC(CWeaponMedkit)
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponMedkit::CWeaponMedkit(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = true;
}

void CWeaponMedkit::WeaponIdle()
{
	BaseClass::WeaponIdle();

	if (HasWeaponIdleTimeElapsed() && GetOwner())
	{
		if (!HasAnyAmmo())
		{
#ifndef CLIENT_DLL
			GetOwner()->Weapon_Drop(this);
			UTIL_Remove(this);
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedkit::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (UsesClipsForAmmo1() && m_iClip1 <= 0)
	{
		WeaponSound(EMPTY);
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	trace_t tr;

	class CMedkitTraceFilter : public CTraceFilterEntitiesOnly
	{
	public:
		CMedkitTraceFilter(CBasePlayer *pEntity) { m_pIgnore = pEntity; }

		bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
		{
			CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);

			if (m_pIgnore == pEntity)
				return false;

			if (!pEntity->IsNPC() && !pEntity->IsPlayer())
				return false;
#ifndef CLIENT_DLL
			if (g_pGameRules->PlayerRelationship(m_pIgnore, pEntity) != GR_TEAMMATE)
				return false;
#else
			if (GetPlayersTeam(m_pIgnore) != pEntity->GetTeam())
				return false;
#endif // !CLIENT_DLL
			return true;
		}
	private:

		CBasePlayer		*m_pIgnore;
	} traceFilter(pPlayer);
#ifndef CLIENT_DLL
	lagcompensation->StartLagCompensation(pPlayer, LAG_COMPENSATE_BOUNDS);
#endif
	UTIL_TraceHull(vecSrc, vecSrc + vecAiming * 72, -Vector(16, 16, 16), Vector(16, 16, 16), MASK_SOLID, &traceFilter, &tr);
#ifndef CLIENT_DLL
	lagcompensation->FinishLagCompensation(pPlayer);
#endif
	if (!tr.DidHit() || tr.m_pEnt->HealthFraction() >= 1.0f)
	{
		WeaponSound(EMPTY);
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	CHL2_Player* pHL2 = ToHL2Player(pPlayer);
	if (pHL2)
		pHL2->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

#ifndef CLIENT_DLL
	static ConVarRef sk_healthkit("sk_healthkit");
	tr.m_pEnt->TakeHealth(sk_healthkit.GetFloat(), DMG_GENERIC);
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMedkit::SecondaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (UsesClipsForAmmo1() && m_iClip1 <= 0)
	{
		WeaponSound(EMPTY);
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	if (pPlayer->HealthFraction() >= 1.0f)
	{
		WeaponSound(EMPTY);
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	CHL2_Player* pHL2 = ToHL2Player(pPlayer);
	if (pHL2)
		pHL2->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

#ifndef CLIENT_DLL
	static ConVarRef sk_healthkit("sk_healthkit");
	pPlayer->TakeHealth(sk_healthkit.GetFloat(), DMG_GENERIC);
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}
}