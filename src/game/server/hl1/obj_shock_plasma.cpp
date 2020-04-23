#include "cbase.h"
#include "gamestats.h"
#include "obj_shock_plasma.h"
#include "IEffects.h"
#include "particle_parse.h"
#include "soundent.h"

#define SHOCK_IMPACT_SOUND "Weapon_Shock.Impact"

LINK_ENTITY_TO_CLASS(obj_shockroach_plasma, CShockPlasma);

BEGIN_DATADESC(CShockPlasma)
// Function Pointers
DEFINE_FUNCTION(BubbleThink),
DEFINE_FUNCTION(BoltTouch),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CShockPlasma, DT_ShockPlasma)
END_SEND_TABLE()

ConVar	plr_dmg_shock("sk_plr_dmg_shockroach","16");

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CShockPlasma::CreateVPhysics(void)
{
	// Create the object in the physics system
	IPhysicsObject *pPhys = VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);

	if (!pPhys)
		return false;

	pPhys->EnableDrag(false);
	pPhys->EnableGravity(false);
	pPhys->SetMass(1);
	pPhys->SetBuoyancyRatio(0);

	return true;
}

void CShockPlasma::Precache()
{
	PrecacheModel("models/crossbow_bolt.mdl");
	PrecacheScriptSound(SHOCK_IMPACT_SOUND);
	PrecacheParticleSystem("shockroach_projectile_trail");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CShockPlasma::BubbleThink(void)
{
	QAngle angNewAngles;

	VectorAngles(GetAbsVelocity(), angNewAngles);
	SetAbsAngles(angNewAngles);

	SetNextThink(gpGlobals->curtime + 0.1f);

	// Make danger sounds out in front of me, to scare snipers back into their hole
	CSoundEnt::InsertSound(SOUND_DANGER | SOUND_CONTEXT_DANGER_APPROACH | SOUND_CONTEXT_COMBINE_ONLY, GetAbsOrigin() + GetAbsVelocity() * 0.2, 120.0f, 0.5f, this, SOUNDENT_CHANNEL_REPEATED_DANGER);

	if (GetWaterLevel() == 0)
		return;

	UTIL_BubbleTrail(GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 8);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CShockPlasma::Spawn(void)
{
	Precache();

	SetModel("models/crossbow_bolt.mdl");
	//CreateVPhysics();
	SetMoveType(MOVETYPE_FLY);
	UTIL_SetSize(this, -Vector(0.3f, 0.3f, 0.3f), Vector(0.3f, 0.3f, 0.3f));
	SetSolid(SOLID_BBOX);
	//SetGravity(0.05f);
	AddEffects(EF_NOSHADOW);

	SetRenderMode(kRenderNone);

	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch(&CShockPlasma::BoltTouch);

	SetThink(&CShockPlasma::BubbleThink);
	SetNextThink(gpGlobals->curtime + 0.1f);

	CreateSprites();

	
}

bool CShockPlasma::CreateSprites()
{
	DispatchParticleEffect("shockroach_projectile_trail", PATTACH_ABSORIGIN_FOLLOW, this);

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CShockPlasma::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CShockPlasma::BoltTouch(CBaseEntity *pOther)
{
	if (pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS | FSOLID_TRIGGER))
	{
		// Some NPCs are triggers that can take damage (like antlion grubs). We should hit them.
		if ((pOther->m_takedamage == DAMAGE_NO) || (pOther->m_takedamage == DAMAGE_EVENTS_ONLY))
			return;
	}

	if (pOther->m_takedamage != DAMAGE_NO)
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize(vecNormalizedVel);

#if defined(HL2_EPISODIC)
		//!!!HACKHACK - specific hack for ep2_outland_10 to allow crossbow bolts to pass through her bounding box when she's crouched in front of the player
		// (the player thinks they have clear line of sight because Alyx is crouching, but her BBOx is still full-height and blocks crossbow bolts.
		if (GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->Classify() == CLASS_PLAYER_ALLY_VITAL && FStrEq(STRING(gpGlobals->mapname), "ep2_outland_10"))
		{
			// Change the owner to stop further collisions with Alyx. We do this by making her the owner.
			// The player won't get credit for this kill but at least the bolt won't magically disappear!
			SetOwnerEntity(pOther);
			return;
		}
#endif//HL2_EPISODIC

		if (GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsNPC())
		{
			CTakeDamageInfo	dmgInfo(this, GetOwnerEntity(), plr_dmg_shock.GetFloat(), DMG_SHOCK);
			dmgInfo.AdjustPlayerDamageInflictedForSkillLevel();
			CalculateMeleeDamageForce(&dmgInfo, vecNormalizedVel, tr.endpos, 0.7f);
			dmgInfo.SetDamagePosition(tr.endpos);
			pOther->DispatchTraceAttack(dmgInfo, vecNormalizedVel, &tr);

			CBasePlayer *pPlayer = ToBasePlayer(GetOwnerEntity());
			if (pPlayer)
			{
				gamestats->Event_WeaponHit(pPlayer, true, "weapon_shockrifle", dmgInfo);
			}

		}
		else
		{
			CTakeDamageInfo	dmgInfo(this, GetOwnerEntity(), 6, DMG_SHOCK | DMG_NEVERGIB);
			CalculateMeleeDamageForce(&dmgInfo, vecNormalizedVel, tr.endpos, 0.7f);
			dmgInfo.SetDamagePosition(tr.endpos);
			pOther->DispatchTraceAttack(dmgInfo, vecNormalizedVel, &tr);
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		//if (pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS)
		//	return;

		//if (!pOther->IsAlive())
		//{
		//	// We killed it! 
		//	const surfacedata_t *pdata = physprops->GetSurfaceData(tr.surface.surfaceProps);
		//	if (pdata->game.material == CHAR_TEX_GLASS)
		//	{
		//		return;
		//	}
		//}

		SetAbsVelocity(Vector(0, 0, 0));

		//VPhysicsGetObject()->EnableMotion(false);

		// play body "thwack" sound
		EmitSound(SHOCK_IMPACT_SOUND);

		//Vector vForward;

		//AngleVectors(GetAbsAngles(), &vForward);
		//VectorNormalize(vForward);

		//UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + vForward * 128, MASK_BLOCKLOS, pOther, COLLISION_GROUP_NONE, &tr2);

		//if (tr2.fraction != 1.0f)
		//{
		//	//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
		//	//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

		//	if (tr2.m_pEnt == NULL || (tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE))
		//	{
		//		CEffectData	data;

		//		data.m_vOrigin = tr2.endpos;
		//		data.m_vNormal = vForward;
		//		data.m_nEntIndex = tr2.fraction != 1.0f;

		//		DispatchEffect("BoltImpact", data);
		//	}
		//}

		SetTouch(NULL);
		SetThink(NULL);

		if (!g_pGameRules->IsMultiplayer())
		{
			UTIL_Remove(this);
		}
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if (pOther->GetMoveType() == MOVETYPE_NONE && !(tr.surface.flags & SURF_SKY))
		{
			EmitSound(SHOCK_IMPACT_SOUND);

			// if what we hit is static architecture, can stay around for a while.
			//Vector vecDir = GetAbsVelocity();
			//float speed = VectorNormalize(vecDir);

			// See if we should reflect off this surface
			//float hitDot = DotProduct(tr.plane.normal, -vecDir);

			//if ((hitDot < 0.5f) && (speed > 100))
			//{
			//	Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;

			//	QAngle reflectAngles;

			//	VectorAngles(vReflection, reflectAngles);

			//	SetLocalAngles(reflectAngles);

			//	SetAbsVelocity(vReflection * speed * 0.75f);

			//	// Start to sink faster
			//	SetGravity(1.0f);
			//}
			//else
			{
				
				UTIL_Remove(this);
				
			}

			// Shoot some sparks
			if (UTIL_PointContents(GetAbsOrigin()) != CONTENTS_WATER)
			{
				g_pEffects->Sparks(GetAbsOrigin());
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ((tr.surface.flags & SURF_SKY) == false)
			{
				UTIL_ImpactTrace(&tr, DMG_BULLET);
			}

			UTIL_Remove(this);
		}
	}

	if (g_pGameRules->IsMultiplayer())
	{
		//		SetThink( &CCrossbowBolt::ExplodeThink );
		//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

CShockPlasma *CShockPlasma::BoltCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner)
{
	// Create a new entity with CCrossbowBolt private data
	CShockPlasma *pBolt = (CShockPlasma *)CreateEntityByName("obj_shockroach_plasma");
	UTIL_SetOrigin(pBolt, vecOrigin);
	pBolt->SetAbsAngles(angAngles);
	pBolt->Spawn();
	pBolt->SetOwnerEntity(pentOwner);
	Vector vecForward;
	AngleVectors(angAngles, &vecForward);

	pBolt->SetAbsVelocity(vecForward * 2000);

	return pBolt;
}