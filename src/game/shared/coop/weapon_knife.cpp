#include "cbase.h"
#include "weapon_coop_basehlcombatweapon.h"
#include "gamerules.h"
#include "datacache/imdlcache.h"

#if defined( CLIENT_DLL )
#include "c_te_effect_dispatch.h"
#include "c_rumble.h"
#include "rumble_shared.h"
#else
#include "ilagcompensationmanager.h"
#include "te_effect_dispatch.h"
#endif

#include "hl2_player_shared.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#if defined( CLIENT_DLL )
#define CKnife C_Knife
#endif

#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512

#define KNIFE_RANGE_SHORT 32
#define KNIFE_RANGE_LONG 48

Vector head_hull_mins(-16, -16, -18);
Vector head_hull_maxs(16, 16, 18);

// ----------------------------------------------------------------------------- //
// CKnife class definition.
// ----------------------------------------------------------------------------- //

class CKnife : public CWeaponCoopBaseHLCombat
{
public:
	DECLARE_CLASS(CKnife, CWeaponCoopBaseHLCombat);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CKnife();

	// We say yes to this so the weapon system lets us switch to it.
	virtual bool HasPrimaryAmmo();
	virtual bool CanBeSelected();

	virtual void Precache();

	void Spawn();
	bool SwingOrStab(bool bSecondary);
	void PrimaryAttack();
	void SecondaryAttack();
	void WeaponAnimation(int iAnimation);

	virtual bool Deploy();

	void WeaponIdle();

	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_KNIFE; }

	virtual bool WeaponShouldBeLowered(void) { return false; }
	virtual bool CanLower(void) { return false; }

private:
	CKnife(const CKnife&) {}
};

// ----------------------------------------------------------------------------- //
// CKnife tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED(Knife, DT_WeaponKnife)

BEGIN_NETWORK_TABLE(CKnife, DT_WeaponKnife)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CKnife)
END_PREDICTION_DATA()


LINK_ENTITY_TO_CLASS(weapon_knife, CKnife);
PRECACHE_REGISTER(weapon_knife);


// ----------------------------------------------------------------------------- //
// CKnife implementation.
// ----------------------------------------------------------------------------- //

CKnife::CKnife()
{
}


bool CKnife::HasPrimaryAmmo()
{
	return true;
}


bool CKnife::CanBeSelected()
{
	return true;
}

void CKnife::Precache()
{
	BaseClass::Precache();

	//PrecacheEffect("KnifeSlash");
}

void CKnife::Spawn()
{
	m_iClip1 = -1;
	BaseClass::Spawn();
}


//
// Different knives seem to have a slightly different deploy timing on the animation
// we hardcode the next attack to be consistent with any knife
//
#define hardcoded_knife_deploy_time 1.0f
bool CKnife::Deploy()
{
	// NOTE (wills): Knives no longer use model bodygroups to change their appearance 
	// between CT and T versions. Team-specific knives now support team-specific
	// viewmodel and world animations, so they are stored as unique models.
	// If a knife needs to look aesthetically different between CT/T teams,
	// add an asset_modifier block to the item definition to divert the whole model.

	/*
	// We need to set the model for the knife based on what team the player is on.
	// We must do this after running deploy to avoid frame locking issues.
	CCSPlayer *pPlayer = GetPlayerOwner();
	if ( pPlayer != NULL )
	{
		MDLCACHE_CRITICAL_SECTION_(g_pMDLCache);
		int bodyPartID = ( pPlayer->GetTeamNumber() == TEAM_TERRORIST ) ? 0 : 1;
		SetBodygroup( 0 , bodyPartID );
	}
	*/

	if (!BaseClass::Deploy())
		return false;

	// Fix for different knife models having different deploy times.  If it's short,
	// you just idle a bit before you attack.  If it's long, we animation-cancel the
	// deploy animation and go straight into the swing/stab after a fixed amount of
	// time.
	if (CBasePlayer* pOwner = ToBasePlayer(GetOwner()))
	{
		pOwner->SetNextAttack(gpGlobals->curtime + hardcoded_knife_deploy_time);
		CBaseViewModel* vm = pOwner->GetViewModel(m_nViewModelIndex, false);
		if (vm != NULL)
		{
			Assert(vm->ViewModelIndex() == m_nViewModelIndex);
			vm->m_nSkin = m_nSkin;
		}
	}

	return true;
}


void CKnife::WeaponAnimation(int iAnimation)
{
	/*
	int flag;
	#if defined( CLIENT_WEAPONS )
		flag = FEV_NOTHOST;
	#else
		flag = 0;
	#endif

	PLAYBACK_EVENT_FULL( flag, pPlayer->edict(), m_usKnife,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero,
		0.0,
		0.0,
		iAnimation, 2, 3, 4 );
	*/
}

void FindHullIntersection(const Vector& vecSrc, trace_t& tr, const Vector& mins, const Vector& maxs, CBaseEntity* pEntity)
{
	int			i, j, k;
	float		distance;
	Vector minmaxs[2] = { mins, maxs };
	trace_t tmpTrace;
	Vector		vecHullEnd = tr.endpos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace);
	if (tmpTrace.fraction < 1.0)
	{
		tr = tmpTrace;
		return;
	}

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 2; j++)
		{
			for (k = 0; k < 2; k++)
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace);
				if (tmpTrace.fraction < 1.0)
				{
					float thisDistance = (tmpTrace.endpos - vecSrc).Length();
					if (thisDistance < distance)
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CKnife::PrimaryAttack()
{
	CHL2_Player* pPlayer = ToHL2Player(GetOwner());
	if (pPlayer)
	{
#if !defined (CLIENT_DLL)
		// Move other players back to history positions based on local player's lag
		lagcompensation->StartLagCompensation(pPlayer, LAG_COMPENSATE_HITBOXES_ALONG_RAY, pPlayer->EyePosition(), pPlayer->EyeAngles(), KNIFE_RANGE_LONG);
#endif
		SwingOrStab(false);
#if !defined (CLIENT_DLL)
		lagcompensation->FinishLagCompensation(pPlayer);
#endif
	}
}

void CKnife::SecondaryAttack()
{
	CHL2_Player* pPlayer = ToHL2Player(GetOwner());
	if (pPlayer /*&& !pPlayer->m_bIsDefusing && !CSGameRules()->IsFreezePeriod()*/)
	{
#if !defined (CLIENT_DLL)
		// Move other players back to history positions based on local player's lag
		lagcompensation->StartLagCompensation(pPlayer, LAG_COMPENSATE_HITBOXES_ALONG_RAY, pPlayer->EyePosition(), pPlayer->EyeAngles(), KNIFE_RANGE_SHORT);
#endif
		SwingOrStab(true);
#if !defined (CLIENT_DLL)
		lagcompensation->FinishLagCompensation(pPlayer);
#endif
	}
}

#include "effect_dispatch_data.h"


void CKnife::WeaponIdle()
{
	if (HasWeaponIdleTimeElapsed())
	{
		m_bHasBeenDeployed = true;
	}

	CHL2_Player* pPlayer = ToHL2Player(GetOwner());
	if (CanSprint() && pPlayer && pPlayer->IsSprinting() && (pPlayer->GetFlags() & FL_ONGROUND) && pPlayer->GetAbsVelocity().IsLengthGreaterThan(90.f))
	{
		// Move to lowered position if we're not there yet
		if (GetActivity() != GetSprintActivity() && GetActivity() != ACT_VM_SPRINT_ENTER
			&& GetActivity() != ACT_TRANSITION)
		{
			SendWeaponAnim(GetSprintActivity());
			m_flTimeLastAction = gpGlobals->curtime;
		}
		else if (HasWeaponIdleTimeElapsed())
		{
			// Keep idling low
			SendWeaponAnim(GetSprintActivity());
			m_flTimeLastAction = gpGlobals->curtime;
		}
	}
	else
	{
		if (HasFidget() && gpGlobals->curtime - m_flTimeLastAction > 15.f && random->RandomFloat(0, 1) <= 0.9)
		{
			SendWeaponAnim(GetWpnData().iScriptedVMActivities[VM_ACTIVITY_FIDGET]);
			m_flTimeLastAction = gpGlobals->curtime;
		}
		else if (HasWeaponIdleTimeElapsed())
		{
			SendWeaponAnim(ACT_VM_IDLE);
		}
	}
}

// [tj] Hacky cheat code to control knife damage
#ifndef CLIENT_DLL
ConVar KnifeDamageScale("knife_damage_scale", "100", FCVAR_DEVELOPMENTONLY);
#endif



bool CKnife::SwingOrStab(bool bSecondary)
{
	CHL2_Player* pPlayer = ToHL2Player(GetOwner());
	if (!pPlayer)
		return false;

	// bStab: false=primary, true=secondary
	float fRange = (!bSecondary) ? KNIFE_RANGE_LONG : KNIFE_RANGE_SHORT; // knife range

	Vector vForward; AngleVectors(pPlayer->EyeAngles(), &vForward);
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecEnd = vecSrc + vForward * fRange;

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	//check for hitting glass - TODO - fix this hackiness, doesn't always line up with what FindHullIntersection returns
#ifndef CLIENT_DLL
	CTakeDamageInfo glassDamage(pPlayer, pPlayer, 42.0f, DMG_BULLET | DMG_NEVERGIB);
	TraceAttackToTriggers(glassDamage, tr.startpos, tr.endpos, vForward);
#endif

	if (tr.fraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, head_hull_mins, head_hull_maxs, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = tr.m_pEnt;
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, pPlayer);
			vecEnd = tr.endpos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	bool bDidHit = tr.fraction < 1.0f;

#ifndef CLIENT_DLL

	bool bFirstSwing = (m_flNextPrimaryAttack + 0.4) < gpGlobals->curtime;

#endif

	float fPrimDelay, fSecDelay;

	if (bSecondary)
	{
		fPrimDelay = fSecDelay = bDidHit ? 1.1f : 1.0f;
	}
	else // swing
	{
		fPrimDelay = bDidHit ? 0.5f : 0.4f;
		fSecDelay = bDidHit ? 0.5f : 0.5f;
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + fPrimDelay;
	m_flNextSecondaryAttack = gpGlobals->curtime + fSecDelay;
	SetWeaponIdleTime(gpGlobals->curtime + 2);

	bool bBackStab = false;

	if (bDidHit)
	{
		// server side damage calculations
		CBaseEntity* pEntity = tr.m_pEnt;

#ifndef CLIENT_DLL
		ClearMultiDamage();

		float flDamage = 0.f;	// set below
#endif
		if (pEntity && pEntity->IsCombatCharacter())
		{
			Vector vTragetForward;

			AngleVectors(pEntity->GetAbsAngles(), &vTragetForward);

			Vector2D vecLOS = (pEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin()).AsVector2D();
			Vector2DNormalize(vecLOS);

			float flDot = vecLOS.Dot(vTragetForward.AsVector2D());

			//Triple the damage if we are stabbing them in the back.
			if (flDot > 0.475f)
				bBackStab = true;
		}

#ifndef CLIENT_DLL
		if (bSecondary)
		{
			if (bBackStab)
			{
				flDamage = 180.0f;
			}
			else
			{
				flDamage = 65.0f;
			}
		}
		else
		{
			if (bBackStab)
			{
				flDamage = 90;
			}
			else if (bFirstSwing)
			{
				// first swing does full damage
				flDamage = 40;
			}
			else
			{
				// subsequent swings do less	
				flDamage = 25;
			}
		}

		// [tj] Hacky cheat to lower knife damage for testing
		flDamage *= (KnifeDamageScale.GetInt() / 100.0f);
#endif

		if (bSecondary)
		{
			SendWeaponAnim(bBackStab ? ACT_VM_SWINGHARD : ACT_VM_HITCENTER2);
		}
		else // swing
		{
			SendWeaponAnim(bBackStab ? ACT_VM_SWINGHIT : ACT_VM_HITCENTER);
		}

#ifndef CLIENT_DLL
		CTakeDamageInfo info(pPlayer, pPlayer, this, flDamage, DMG_SLASH | DMG_NEVERGIB, bBackStab ? TF_DMG_CUSTOM_BACKSTAB : 0);

		CalculateMeleeDamageForce(&info, vForward, tr.endpos, 1.0f / flDamage);
		pEntity->DispatchTraceAttack(info, vForward, &tr);
		ApplyMultiDamage();
#endif

		if (tr.m_pEnt)
		{
			CPASAttenuationFilter filter(this);
			filter.UsePredictionRules();

			if (tr.m_pEnt->IsCombatCharacter())
			{
				EmitSound(filter, entindex(), GetShootSound(bSecondary ? SPECIAL1 : MELEE_HIT));
			}
			else
			{
				EmitSound(filter, entindex(), GetShootSound(MELEE_HIT_WORLD));
			}
		}
#if 1
		CEffectData data;
		data.m_vOrigin = tr.endpos;
		data.m_vStart = tr.startpos;
		data.m_nSurfaceProp = tr.surface.surfaceProps;
		data.m_nDamageType = (bSecondary || bBackStab) ? DMG_BULLET : DMG_SLASH;
		data.m_nHitBox = tr.hitbox;
#ifdef CLIENT_DLL
		data.m_hEntity = tr.m_pEnt->GetRefEHandle();
#else
		data.m_nEntIndex = tr.m_pEnt->entindex();
#endif

		CPASFilter filter(data.m_vOrigin);
		filter.UsePredictionRules();

		data.m_vAngles = pPlayer->EyeAngles();
		//data.m_fFlags = 0x1;	//IMPACT_NODECAL;
		DispatchEffect("KnifeSlash", data, filter);
#else
		UTIL_ImpactTrace(&tr, (bSecondary||bBackStab) ? DMG_BULLET : DMG_SLASH);
#endif
	}
	else
	{
		// play wiff or swish sound
		CPASAttenuationFilter filter(this);
		filter.UsePredictionRules();
		EmitSound(filter, entindex(), GetShootSound(MELEE_MISS));

		if (bSecondary)
		{
			SendWeaponAnim(ACT_VM_MISSCENTER2);
		}
		else // swing
		{
			SendWeaponAnim(ACT_VM_MISSCENTER);
		}
	}

	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
	RumbleEffect(!bDidHit ? RUMBLE_CROWBAR_SWING : RUMBLE_AR2, 0, RUMBLE_FLAG_RESTART);
	NotifyWeaponAction();

	return bDidHit;
}