#include "cbase.h"
#include "coop/weapon_coop_basehlcombatweapon.h"

#ifndef CLIENT_DLL
#include "ai_basenpc.h"
#include "weapon_emp.h"
#include "env_alyxemp_shared.h"
#else
#define CWeaponEmpTool C_WeaponEmpTool
#endif // !CLIENT_DLL

#include "in_buttons.h"

#define PLAYER_EMP_RADIUS 128
#ifndef CLIENT_DLL

extern float IntervalDistance(float x, float x0, float x1);

extern ConVar sv_debug_player_use;

class CWeaponEmpEffect : public CAlyxEmpEffect
{
public:
	DECLARE_CLASS(CWeaponEmpEffect, CAlyxEmpEffect);

	void	ActivateAutomatic(CBaseEntity* pAlyx, CBaseEntity* pTarget, const char* pattach = "muzzle");
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CWeaponEmpEffect::ActivateAutomatic(CBaseEntity* pAlyx, CBaseEntity* pTarget, const char* pszAttach)
{
	Assert(pAlyx->GetBaseAnimating() != NULL);

	SetParent(pAlyx, pAlyx->GetBaseAnimating()->LookupAttachment(pszAttach));
	SetLocalOrigin(vec3_origin);

	m_iState = ALYXEMP_STATE_OFF;
	SetTargetEntity(pTarget);
	SetThink(&CAlyxEmpEffect::AutomaticThink);
	SetNextThink(gpGlobals->curtime);

	m_bAutomated = true;
}

LINK_ENTITY_TO_CLASS(emptool_effect, CWeaponEmpEffect);
#endif // !CLIENT_DLL


class CWeaponEmpTool : public CWeaponCoopBaseHLCombat
{
public:
	DECLARE_CLASS(CWeaponEmpTool, CWeaponCoopBaseHLCombat);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual bool			CanBePickedUpByNPCs(void) { return false; }
	virtual int GetWeaponID(void) const { return HLSS_WEAPON_ID_EMPTOOL; }

	void PrimaryAttack();
	void Precache();
#ifndef CLIENT_DLL
	CBaseEntity* FindEmpEntity(CBasePlayer* pOwner);
	bool IsUseableEntity(CBaseEntity* pEntity, unsigned int requiredCaps);
#endif
};

LINK_ENTITY_TO_CLASS(weapon_alyxemp, CWeaponEmpTool);
PRECACHE_WEAPON_REGISTER(weapon_alyxemp);

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponEmpTool, DT_WeaponEmpTool)

BEGIN_NETWORK_TABLE(CWeaponEmpTool, DT_WeaponEmpTool)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponEmpTool)
END_PREDICTION_DATA();

void CWeaponEmpTool::Precache()
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther("emptool_effect");
#endif
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Return true if this object can be +used by the player
//-----------------------------------------------------------------------------
bool CWeaponEmpTool::IsUseableEntity(CBaseEntity *pEntity, unsigned int requiredCaps)
{
	if (pEntity)
	{
		IEMPInteractable *pTarget = dynamic_cast<IEMPInteractable *> (pEntity);
		if (pTarget)
		{
			return pTarget->EmpCanInteract(this);
		}
	}

	return false;
}

CBaseEntity *CWeaponEmpTool::FindEmpEntity(CBasePlayer *pOwner)
{
	if (!pOwner)
		return nullptr;

	Vector forward, up;
	pOwner->EyeVectors(&forward, NULL, &up);

	trace_t tr;
	// Search for objects in a sphere (tests for entities that are not solid, yet still useable)
	Vector searchCenter = pOwner->EyePosition();

	// NOTE: Some debris objects are useable too, so hit those as well
	// A button, etc. can be made out of clip brushes, make sure it's +useable via a traceline, too.
	int useableContents = MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_PLAYERCLIP;

#ifdef CSTRIKE_DLL
	useableContents = MASK_NPCSOLID_BRUSHONLY | MASK_OPAQUE_AND_NPCS;
#endif

#ifdef HL1_DLL
	useableContents = MASK_SOLID;
#endif
#ifndef CLIENT_DLL
	CBaseEntity *pFoundByTrace = NULL;
#endif

	// UNDONE: Might be faster to just fold this range into the sphere query
	CBaseEntity *pObject = NULL;

	float nearestDist = FLT_MAX;
	// try the hit entity if there is one, or the ground entity if there isn't.
	CBaseEntity *pNearest = NULL;

	const int NUM_TANGENTS = 8;
	// trace a box at successive angles down
	//							forward, 45 deg, 30 deg, 20 deg, 15 deg, 10 deg, -10, -15
	const float tangents[NUM_TANGENTS] = { 0, 1, 0.57735026919f, 0.3639702342f, 0.267949192431f, 0.1763269807f, -0.1763269807f, -0.267949192431f };
	for (int i = 0; i < NUM_TANGENTS; i++)
	{
		if (i == 0)
		{
			UTIL_TraceLine(searchCenter, searchCenter + forward * 1024, useableContents, pOwner, COLLISION_GROUP_NONE, &tr);
		}
		else
		{
			Vector down = forward - tangents[i] * up;
			VectorNormalize(down);
			UTIL_TraceHull(searchCenter, searchCenter + down * 72, -Vector(16, 16, 16), Vector(16, 16, 16), useableContents, pOwner, COLLISION_GROUP_NONE, &tr);
		}
		pObject = tr.m_pEnt;

#ifndef CLIENT_DLL
		pFoundByTrace = pObject;
#endif
		bool bUsable = IsUseableEntity(pObject, 0);
		while (pObject && !bUsable && pObject->GetMoveParent())
		{
			pObject = pObject->GetMoveParent();
			bUsable = IsUseableEntity(pObject, 0);
		}

		if (bUsable)
		{
			Vector delta = tr.endpos - tr.startpos;
			float centerZ = pOwner->CollisionProp()->WorldSpaceCenter().z;
			delta.z = IntervalDistance(tr.endpos.z, centerZ + pOwner->CollisionProp()->OBBMins().z, centerZ + pOwner->CollisionProp()->OBBMaxs().z);
			float dist = delta.Length();
			if (dist < PLAYER_EMP_RADIUS)
			{
#ifndef CLIENT_DLL

				if (sv_debug_player_use.GetBool())
				{
					NDebugOverlay::Line(searchCenter, tr.endpos, 0, 255, 0, true, 30);
					NDebugOverlay::Cross3D(tr.endpos, 16, 0, 255, 0, true, 30);
				}

				//if (pObject->MyNPCPointer() && pObject->MyNPCPointer()->IsPlayerAlly(pOwner))
				//{
				//	// If about to select an NPC, do a more thorough check to ensure
				//	// that we're selecting the right one from a group.
				//	pObject = DoubleCheckUseNPC(pObject, searchCenter, forward);
				//}
#endif
				if (sv_debug_player_use.GetBool())
				{
					Msg("Trace using: %s\n", pObject ? pObject->GetDebugName() : "no usable entity found");
				}

				pNearest = pObject;

				// if this is directly under the cursor just return it now
				if (i == 0)
					return pObject;
			}
		}
	}

	// check ground entity first
	// if you've got a useable ground entity, then shrink the cone of this search to 45 degrees
	// otherwise, search out in a 90 degree cone (hemisphere)
	/*if (GetGroundEntity() && IsUseableEntity(GetGroundEntity(), FCAP_USE_ONGROUND))
	{
		pNearest = GetGroundEntity();
	}*/
	if (pNearest)
	{
		// estimate nearest object by distance from the view vector
		Vector point;
		pNearest->CollisionProp()->CalcNearestPoint(searchCenter, &point);
		nearestDist = CalcDistanceToLine(point, searchCenter, forward);
		if (sv_debug_player_use.GetBool())
		{
			Msg("Trace found %s, dist %.2f\n", pNearest->GetClassname(), nearestDist);
		}
	}

	for (CEntitySphereQuery sphere(searchCenter, PLAYER_EMP_RADIUS); (pObject = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		if (!pObject)
			continue;

		if (!IsUseableEntity(pObject, FCAP_USE_IN_RADIUS))
			continue;

		// see if it's more roughly in front of the player than previous guess
		Vector point;
		pObject->CollisionProp()->CalcNearestPoint(searchCenter, &point);

		Vector dir = point - searchCenter;
		VectorNormalize(dir);
		float dot = DotProduct(dir, forward);

		// Need to be looking at the object more or less
		if (dot < 0.8)
			continue;

		float dist = CalcDistanceToLine(point, searchCenter, forward);

		if (sv_debug_player_use.GetBool())
		{
			Msg("Radius found %s, dist %.2f\n", pObject->GetClassname(), dist);
		}

		if (dist < nearestDist)
		{
			// Since this has purely been a radius search to this point, we now
			// make sure the object isn't behind glass or a grate.
			trace_t trCheckOccluded;
			UTIL_TraceLine(searchCenter, point, useableContents, pOwner, COLLISION_GROUP_NONE, &trCheckOccluded);

			if (trCheckOccluded.fraction == 1.0 || trCheckOccluded.m_pEnt == pObject)
			{
				pNearest = pObject;
				nearestDist = dist;
			}
		}
	}

#ifndef CLIENT_DLL
	if (!pNearest)
	{
		// Haven't found anything near the player to use, nor any NPC's at distance.
		// Check to see if the player is trying to select an NPC through a rail, fence, or other 'see-though' volume.
		trace_t trAllies;
		UTIL_TraceLine(searchCenter, searchCenter + forward * PLAYER_EMP_RADIUS, MASK_SHOT_PORTAL, pOwner, COLLISION_GROUP_NONE, &trAllies);

		if (trAllies.m_pEnt && IsUseableEntity(trAllies.m_pEnt, 0) && trAllies.m_pEnt->MyNPCPointer() && trAllies.m_pEnt->MyNPCPointer()->IsPlayerAlly(pOwner))
		{
			// This is an NPC, take it!
			pNearest = trAllies.m_pEnt;
		}
	}

	/*if (pNearest && pNearest->MyNPCPointer() && pNearest->MyNPCPointer()->IsPlayerAlly(this))
	{
		pNearest = DoubleCheckUseNPC(pNearest, searchCenter, forward);
	}*/

	if (sv_debug_player_use.GetBool())
	{
		if (!pNearest)
		{
			NDebugOverlay::Line(searchCenter, tr.endpos, 255, 0, 0, true, 30);
			NDebugOverlay::Cross3D(tr.endpos, 16, 255, 0, 0, true, 30);
		}
		else if (pNearest == pFoundByTrace)
		{
			NDebugOverlay::Line(searchCenter, tr.endpos, 0, 255, 0, true, 30);
			NDebugOverlay::Cross3D(tr.endpos, 16, 0, 255, 0, true, 30);
		}
		else
		{
			NDebugOverlay::Box(pNearest->WorldSpaceCenter(), Vector(-8, -8, -8), Vector(8, 8, 8), 0, 255, 0, true, 30);
		}
	}
#endif

	if (sv_debug_player_use.GetBool())
	{
		Msg("Radial using: %s\n", pNearest ? pNearest->GetDebugName() : "no usable entity found");
	}

	return pNearest;
}
#endif
void CWeaponEmpTool::PrimaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
		return;

	/*if (!(pOwner->m_afButtonPressed & IN_ATTACK))
		return;*/

#ifndef CLIENT_DLL
	CBaseEntity *pEnt = FindEmpEntity(pOwner);

	if (pEnt)
	{
		IEMPInteractable *pTarget = dynamic_cast<IEMPInteractable *> (pEnt);
		Assert(pTarget);

		if (pTarget && pTarget->EmpCanInteract(this))
		{
			pTarget->EmpNotifyInteraction(this);

			CWeaponEmpEffect *pEffect = (CWeaponEmpEffect *)Create("emptool_effect", GetAbsOrigin(), GetAbsAngles(), this);
			pEffect->ActivateAutomatic(this, pEnt);
		}
	}
#endif

	SendWeaponAnim(ACT_VM_FIDGET);

	m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
}