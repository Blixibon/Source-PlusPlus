#include "cbase.h"
#include "hl1_ai_basenpc.h"
#include "npcevent.h"
#include "particle_parse.h"
#include "saverestore_utlvector.h"

#define TOR_MODEL "models/half-life/tor.mdl"

ConVar sk_tor_health("sk_tor_health", "1600");
ConVar sk_tor_dmg_blast("sk_tor_dmg_blast", "35");
ConVar sk_tor_dmg_slash("sk_tor_dmg_slash", "28");
ConVar sk_tor_dmg_projectile("sk_tor_dmg_projectile", "12");

int AE_TOR_SETSKIN;
int AE_TOR_TRANSFORM;
int AE_TOR_MELEE_HIT;
int AE_TOR_PROJECTILE;
int AE_TOR_DISCHARGE;
int AE_TOR_SUMMON;

Activity ACT_TOR_SUMMON;
Activity ACT_TOR_CHARGE_START;
Activity ACT_TOR_CHARGE_LOOP;
Activity ACT_TOR_CHARGE_END;

class CNPC_HL1TorProjectile : public CAI_BaseNPC
{
public:
	DECLARE_CLASS(CNPC_HL1TorProjectile, CAI_BaseNPC);

	DECLARE_DATADESC();

	void Spawn(void);
	void Precache(void);

	void EXPORT HuntThink(void);
	void EXPORT KillThink(void);
	void EXPORT BounceTouch(CBaseEntity* pOther);
	void MovetoTarget(Vector vecTarget);

	float m_flSpawnTime;
	Vector m_vecIdeal;
	bool m_bBlue;
};

LINK_ENTITY_TO_CLASS(obj_tor_projectile, CNPC_HL1TorProjectile);

BEGIN_DATADESC(CNPC_HL1TorProjectile)
DEFINE_THINKFUNC(HuntThink),
DEFINE_THINKFUNC(KillThink),
DEFINE_ENTITYFUNC(BounceTouch),

DEFINE_FIELD(m_flSpawnTime, FIELD_TIME),
DEFINE_FIELD(m_vecIdeal, FIELD_VECTOR),
DEFINE_FIELD(m_bBlue, FIELD_BOOLEAN),
END_DATADESC();

void CNPC_HL1TorProjectile::Spawn(void)
{
	Precache();
	// motor
	SetMoveType(MOVETYPE_FLY);
	SetSolid(SOLID_BBOX);
	SetSize(vec3_origin, vec3_origin);

	AddEFlags(EFL_FORCE_CHECK_TRANSMIT);

	SetThink(&CNPC_HL1TorProjectile::HuntThink);
	SetTouch(&CNPC_HL1TorProjectile::BounceTouch);

	SetNextThink(gpGlobals->curtime + 0.1);

	m_flSpawnTime = gpGlobals->curtime;

	DispatchParticleEffect(m_bBlue ? "tor_projectile_blue" : "tor_projectile", PATTACH_ABSORIGIN_FOLLOW, this);
}


void CNPC_HL1TorProjectile::Precache(void)
{
	PrecacheParticleSystem("tor_projectile_blue");
	PrecacheParticleSystem("tor_projectile");
	PrecacheParticleSystem("tor_projectile_vanish_blue");
	PrecacheParticleSystem("tor_projectile_vanish");

	PrecacheScriptSound("NPC_Tor.Discharge");
}

void CNPC_HL1TorProjectile::HuntThink(void)
{
	SetNextThink(gpGlobals->curtime + 0.1);

	// check world boundaries
	if (gpGlobals->curtime - m_flSpawnTime > 8 /*|| GetEnemy() == NULL || m_hOwner == NULL*/ || !IsInWorld())
	{
		SetTouch(NULL);
		SetThink(&CNPC_HL1TorProjectile::KillThink);
		SetNextThink(gpGlobals->curtime);
		return;
	}

	if (!GetEnemy())
		return;

	MovetoTarget(GetEnemy()->GetAbsOrigin());
}

void CNPC_HL1TorProjectile::MovetoTarget(Vector vecTarget)
{
	// accelerate
	float flSpeed = m_vecIdeal.Length();
	if (flSpeed == 0)
	{
		m_vecIdeal = GetAbsVelocity();
		flSpeed = m_vecIdeal.Length();
	}

	if (flSpeed > 400)
	{
		VectorNormalize(m_vecIdeal);
		m_vecIdeal = m_vecIdeal * 400;
	}

	Vector t = vecTarget - GetAbsOrigin();
	VectorNormalize(t);
	m_vecIdeal = m_vecIdeal + t * 100;
	SetAbsVelocity(m_vecIdeal);
}

void CNPC_HL1TorProjectile::BounceTouch(CBaseEntity* pOther)
{
	CTakeDamageInfo info(this, GetOwnerEntity() ? GetOwnerEntity() : this, sk_tor_dmg_projectile.GetFloat(), DMG_SHOCK);
	if (m_bBlue)
		info.ScaleDamage(0.5f);

	if (pOther && pOther->m_takedamage != DAMAGE_NO && pOther->PassesDamageFilter(info))
	{
		trace_t tr;
		tr = GetTouchTrace();

		if (m_bBlue && pOther->IsPlayer())
		{
			float flRandX = RandomFloat(-10.f, 10.f);
			float flRandY = RandomFloat(-10.f, 10.f);

			pOther->ViewPunch(QAngle(flRandX, flRandY, 0.f));
		}

		ClearMultiDamage();
		Vector vecAttackDir = GetAbsVelocity();
		VectorNormalize(vecAttackDir);
		CalculateMeleeDamageForce(&info, vecAttackDir, tr.endpos);
		pOther->DispatchTraceAttack(info, vecAttackDir, &tr);
		ApplyMultiDamage();

		UTIL_EmitAmbientSound(GetSoundSourceIndex(), tr.endpos, "NPC_Tor.Discharge", 1, SNDLVL_NORM, 0, random->RandomInt(90, 99));
		DispatchParticleEffect(m_bBlue ? "tor_projectile_vanish_blue" : "tor_projectile_vanish", GetAbsOrigin(), vec3_angle);

		SetTouch(NULL);
		RemoveDeferred();
	}
}

void CNPC_HL1TorProjectile::KillThink(void)
{
	UTIL_Remove(this);
}

class CNPC_HL1Tor : public CHL1BaseNPC
{
public:
	DECLARE_CLASS(CNPC_HL1Tor, CHL1BaseNPC);
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	void Spawn(void);
	void Precache(void);
	Class_T	Classify(void);

	void AlertSound(void);
	void IdleSound(void);
	void PainSound(const CTakeDamageInfo& info);
	void DeathSound(const CTakeDamageInfo& info);

	virtual int OnTakeDamage_Alive(const CTakeDamageInfo& info);

	virtual void		HandleAnimEvent(animevent_t* pEvent);
	virtual int			SelectSchedule(void);

	void	DoSummonAlly();
	void	DeathNotice(CBaseEntity* pevChild);

	enum
	{
		SCHED_HL1TOR_TRANSFORM = BaseClass::NEXT_SCHEDULE,
	};

protected:
	bool m_bPhase2;
	CUtlVectorConservative<AIHANDLE> m_SummonedAllies;
};

LINK_ENTITY_TO_CLASS(monster_alien_tor, CNPC_HL1Tor);

BEGIN_DATADESC(CNPC_HL1Tor)
DEFINE_FIELD(m_bPhase2, FIELD_BOOLEAN),
DEFINE_UTLVECTOR(m_SummonedAllies, FIELD_EHANDLE),
END_DATADESC();

void CNPC_HL1Tor::Spawn(void)
{
	Precache();
	BaseClass::Spawn();

	SetModel(TOR_MODEL);

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetMoveType(MOVETYPE_STEP);

	SetHullType(HULL_MEDIUM_TALL);
	SetHullSizeNormal();

	m_bloodColor = BLOOD_COLOR_GREEN;
	m_iHealth = sk_tor_health.GetInt();

	m_flFieldOfView = VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();

	SetNavType(NAV_GROUND);

	CapabilitiesAdd(bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_DOORS_GROUP | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_MELEE_ATTACK1);

	NPCInit();

	m_bPhase2 = false;
}

void CNPC_HL1Tor::Precache(void)
{
	BaseClass::Precache();

	UTIL_PrecacheOther("obj_tor_projectile");

	PrecacheModel(TOR_MODEL);

	PrecacheScriptSound("NPC_Tor.Summoned");
	PrecacheScriptSound("NPC_Tor.Alert");
	PrecacheScriptSound("NPC_Tor.Idle");
	PrecacheScriptSound("NPC_Tor.Pain");
	PrecacheScriptSound("NPC_Tor.Death");
	PrecacheScriptSound("NPC_Tor.Melee");

	PrecacheParticleSystem("tor_transform_wave");
	PrecacheParticleSystem("tor_shockwave_blue");
	PrecacheParticleSystem("tor_shockwave");
	PrecacheParticleSystem("tor_projectile_vanish_blue");
	PrecacheParticleSystem("tor_projectile_vanish");
}

Class_T CNPC_HL1Tor::Classify(void)
{
	return CLASS_ALIEN_MILITARY;
}

void CNPC_HL1Tor::AlertSound(void)
{
	CPASAttenuationFilter filter(this);
	EmitSound(filter, entindex(), "NPC_Tor.Alert");
}

void CNPC_HL1Tor::IdleSound(void)
{
	CPASAttenuationFilter filter(this);
	EmitSound(filter, entindex(), "NPC_Tor.Idle");
}

void CNPC_HL1Tor::PainSound(const CTakeDamageInfo& info)
{
	if (random->RandomInt(0, 5) < 2)
	{
		CPASAttenuationFilter filter(this);
		EmitSound(filter, entindex(), "NPC_Tor.Pain");
	}
}

void CNPC_HL1Tor::DeathSound(const CTakeDamageInfo& info)
{
	CPASAttenuationFilter filter(this);
	EmitSound(filter, entindex(), "NPC_Tor.Death");
}

int CNPC_HL1Tor::OnTakeDamage_Alive(const CTakeDamageInfo& inputInfo)
{
	if (!m_bPhase2)
	{
		const int iHalfHealth = GetMaxHealth() / 2;
		if (GetHealth() <= iHalfHealth)
			return 0;

		const float flMaxDamage = Max(0, GetHealth() - iHalfHealth);

		CTakeDamageInfo info(inputInfo);
		info.SetDamage(Min(flMaxDamage, inputInfo.GetDamage()));
		return BaseClass::OnTakeDamage_Alive(info);
	}

	return BaseClass::OnTakeDamage_Alive(inputInfo);
}

void CNPC_HL1Tor::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if (pEvent->event == AE_TOR_SETSKIN)
		{
			m_nSkin = atoi(pEvent->options);
			return;
		}
		else if (pEvent->event == AE_TOR_TRANSFORM)
		{
			m_bPhase2 = true;
			DispatchParticleEffect("tor_transform_wave", WorldSpaceCenter(), GetAbsAngles());
			for (const auto& hAlly : m_SummonedAllies)
			{
				if (hAlly.Get() && (WorldSpaceCenter() - hAlly->WorldSpaceCenter()).IsLengthLessThan(250))
				{
					hAlly->SetHealth(hAlly->GetMaxHealth());
				}
			}
			return;
		}
		else if (pEvent->event == AE_TOR_SUMMON)
		{
			DoSummonAlly();
			return;
		}
		else if (pEvent->event == AE_TOR_MELEE_HIT)
		{
			CBaseEntity* pHurt = CheckTraceHullAttack(40, Vector(-10, -10, -10), Vector(10, 10, 10), sk_tor_dmg_slash.GetFloat(), DMG_SLASH);
			CPASAttenuationFilter filter(this);
			if (pHurt)
			{
				if (pHurt->GetFlags() & (FL_NPC | FL_CLIENT))
					pHurt->ViewPunch(QAngle(19, -4, 2));

				// Play a random attack hit sound
				CSoundParameters params;
				if (GetParametersForSound("Vortigaunt.AttackHit", params, NULL))
				{
					EmitSound_t ep(params);
					EmitSound(filter, entindex(), ep);
				}
			}
			else
			{
				// Play a random attack miss sound
				CSoundParameters params;
				if (GetParametersForSound("Vortigaunt.AttackMiss", params, NULL))
				{
					EmitSound_t ep(params);
					EmitSound(filter, entindex(), ep);
				}
			}
			return;
		}
		else if (pEvent->event == AE_TOR_PROJECTILE)
		{
			Vector vStaffTop, vForward;
			GetAttachment("staff_top", vStaffTop);
			GetVectors(&vForward, nullptr, nullptr);

			CNPC_HL1TorProjectile* pBall = (CNPC_HL1TorProjectile*)CreateNoSpawn("obj_tor_projectile", vStaffTop, vec3_angle, this);
			pBall->m_bBlue = m_bPhase2;
			DispatchSpawn(pBall, false);
			pBall->SetAbsVelocity(vForward * 50.f);
			pBall->SetEnemy(GetEnemy());
			return;
		}
	}

	BaseClass::HandleAnimEvent(pEvent);
}

int CNPC_HL1Tor::SelectSchedule(void)
{
	if (!m_bPhase2)
	{
		const int iHalfHealth = GetMaxHealth() / 2;
		if (GetHealth() <= iHalfHealth + 4)
			return SCHED_HL1TOR_TRANSFORM;
	}

	return BaseClass::SelectSchedule();
}

void CNPC_HL1Tor::DoSummonAlly()
{
	static const char* ppszSummonClasses[] = {
		"monster_alien_controller",
		"monster_alien_grunt",
		"monster_alien_slave"
	};
	int nSpawnClass = RandomInt(0, ARRAYSIZE(ppszSummonClasses) - 1);

	Vector vSummonPos;
	EntityToWorldSpace(Vector(60, 0, 10), &vSummonPos);
	if (nSpawnClass == 0)
		vSummonPos.z += 80;

	CAI_BaseNPC* pSummon = (CAI_BaseNPC*)Create(ppszSummonClasses[nSpawnClass], vSummonPos, GetAbsAngles(), this);
	pSummon->SetSquad(GetSquad());
	if (GetEnemy())
	{
		pSummon->UpdateEnemyMemory(GetEnemy(), GetEnemy()->GetAbsOrigin(), this);
	}

	m_SummonedAllies.AddToHead(pSummon);

	DispatchParticleEffect(m_bPhase2 ? "tor_shockwave_blue" : "tor_shockwave", vSummonPos + Vector(0, 0, 40), vec3_angle, pSummon);
	DispatchParticleEffect(m_bPhase2 ? "tor_projectile_vanish_blue" : "tor_projectile_vanish", vSummonPos + Vector(0, 0, 40), vec3_angle, pSummon);
}

void CNPC_HL1Tor::DeathNotice(CBaseEntity* pChild)
{
	if (pChild && pChild->IsNPC())
	{
		const AIHANDLE hChildAI = pChild->MyNPCPointer();
		m_SummonedAllies.FindAndRemove(hChildAI);
	}
}

AI_BEGIN_CUSTOM_NPC(monster_alien_tor, CNPC_HL1Tor)

DECLARE_ANIMEVENT(AE_TOR_SETSKIN);
DECLARE_ANIMEVENT(AE_TOR_TRANSFORM);
DECLARE_ANIMEVENT(AE_TOR_MELEE_HIT);
DECLARE_ANIMEVENT(AE_TOR_PROJECTILE);
DECLARE_ANIMEVENT(AE_TOR_DISCHARGE);
DECLARE_ANIMEVENT(AE_TOR_SUMMON);

DECLARE_ACTIVITY(ACT_TOR_SUMMON);
DECLARE_ACTIVITY(ACT_TOR_CHARGE_START);
DECLARE_ACTIVITY(ACT_TOR_CHARGE_LOOP);
DECLARE_ACTIVITY(ACT_TOR_CHARGE_END);

DEFINE_SCHEDULE(
	SCHED_HL1TOR_TRANSFORM,

	"	Tasks"
	"		TASK_STOP_MOVING					0"
	"		TASK_SET_ACTIVITY					ACTIVITY:ACT_VICTORY_DANCE"
	"		TASK_WAIT							1"
)

AI_END_CUSTOM_NPC();