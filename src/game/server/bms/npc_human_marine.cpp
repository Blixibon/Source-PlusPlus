#include "cbase.h"
#include "npcevent.h"
#include "npc_basecollegue.h"
#include "ai_behavior_rappel.h"
#include "npc_headcrab.h"
#include "ai_interactions.h"
#include "ai_memory.h"
#include "ammodef.h"
#include "particle_parse.h"
#include "ai_squad.h"

#define COMBINE_MIN_CROUCH_DISTANCE		256.0

#define COMBINE_AE_KICK				( 3 )

//static int COMBINE_AE_BEGIN_ALTFIRE;
//static int COMBINE_AE_ALTFIRE;
static int AE_METROPOLICE_KICK_BALLS;

class CNPC_HumanFGrunt : public CNPC_BaseColleague
{
	DECLARE_CLASS(CNPC_HumanFGrunt, CNPC_BaseColleague);
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
public:
	virtual void Precache()
	{
		// Prevents a warning
		SelectModel();

		PrecacheScriptSound("NPC_Combine.WeaponBash");
		PrecacheScriptSound("Weapon_CombineGuard.Special1");

		PrecacheParticleSystem("precharge_fire_tp");
		PrecacheParticleSystem("charge_fire_tp");

		BaseClass::Precache();
	}

	void	Spawn(void);
	void			OnRestore();
	void	SelectModel();
	Class_T Classify(void);

	bool			CreateBehaviors();

	void			HandleAnimEvent(animevent_t *pEvent);

	int				MeleeAttack1Conditions(float flDot, float flDist); // For kick/punch

	void			StartTask(const Task_t *pTask);
	void			RunTask(const Task_t *pTask);

	virtual bool	CanAltFireEnemy(bool bUseFreeKnowledge);
	void			DelayAltFireAttack(float flDelay);
	void			DelaySquadAltFireAttack(float flDelay);

	virtual int		TranslateSchedule(int scheduleType);

	// Rappel
	virtual bool IsWaitingToRappel(void) { return m_RappelBehavior.IsWaitingToRappel(); }
	void BeginRappel() { m_RappelBehavior.BeginRappel(); }

	virtual int			GetSpecialDSP(void)
	{
		return (GetBodygroup(FindBodygroupByName("gasmask_nv")) == 1 || GetBodygroup(FindBodygroupByName("head")) == 2) ? 55 : 0;
	}

	bool	HasDefaultWeapon(string_t strWeapon) { return (!Q_strnicmp(STRING(strWeapon), "Default", 7) || !Q_strnicmp(STRING(strWeapon), "Random", 6)); }
	const char *GetDefaultWeapon()
	{
		static const char *weapons[] = {
			"weapon_glock",
			"weapon_mp5",
			"weapon_shotgun",
			"weapon_ar2",
			"weapon_mg1",
			"weapon_oicw",
			"weapon_ar1",
			"weapon_smg1",
			"weapon_357",
			"weapon_pistol",
		};

		return weapons[RandomInt(0, ARRAYSIZE(weapons)-1)];
	}

	enum
	{
		SCHED_MARINE_AR2_ALTFIRE = BaseClass::NEXT_SCHEDULE,
		NEXT_SCHEDULE,

		TASK_MARINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET = BaseClass::NEXT_TASK,
		NEXT_TASK,
	};

protected:
	void			SetKickDamage(int nDamage) { m_nKickDamage = nDamage; }

protected:
	CAI_RappelBehavior			m_RappelBehavior;

	int				m_nKickDamage;
	Vector			m_vecAltFireTarget;

	float			m_flNextGrenadeCheck;
	float			m_flNextAltFireTime;		// Elites only. Next time to begin considering alt-fire attack.

	int				m_iAR2AmmoType;
};

//LINK_ENTITY_TO_CLASS(npc_human_marine, CNPC_HumanFGrunt);
LINK_ENTITY_TO_CLASS(npc_human_fgrunt, CNPC_HumanFGrunt);

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CNPC_HumanFGrunt)
DEFINE_FIELD(m_nKickDamage, FIELD_INTEGER),
DEFINE_FIELD(m_vecAltFireTarget, FIELD_VECTOR),
DEFINE_FIELD(m_flNextGrenadeCheck, FIELD_TIME),
DEFINE_FIELD(m_flNextAltFireTime, FIELD_TIME),
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_HumanFGrunt::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

void CNPC_HumanFGrunt::OnRestore()
{
	BaseClass::OnRestore();

	m_iAR2AmmoType = GetAmmoDef()->Index("AR2");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanFGrunt::Spawn(void)
{
	Precache();

	ConVarRef health("sk_human_commander_health");
	ConVarRef kick("sk_combine_guard_kick");

	m_iHealth = health.GetInt();
	SetKickDamage(kick.GetInt());

	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	CapabilitiesAdd(bits_CAP_MOVE_JUMP);
	// Innate range attack for kicking
	CapabilitiesAdd(bits_CAP_INNATE_MELEE_ATTACK1);

	NPCInit();

	SetUse(&CNPC_BaseColleague::CommanderUse);

	m_nSkin = random->RandomInt(0, GetModelPtr()->numskinfamilies() - 1);
	
	switch (RandomInt(0, 9))
	{
	case 5:
	case 6:
	case 7:
		SetBodygroup(FindBodygroupByName("gasmask_nv"), 1);
		break;

	case 0:
	case 1:
	default:
	{
		int iRand = RandomInt(0, 3);
		SetBodygroup(FindBodygroupByName("head"), iRand);
	}
		break;
	}



	/*if (FClassnameIs(this, "npc_human_medic"))
	{
		SetBodygroup(FindBodygroupByName("helmet_medic"), 1);
		GetExpresser()->SetVoicePitch(110);
	}*/
	
	

	SetBodygroup(FindBodygroupByName("gloves"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("packs_chest"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("packs_hips"), RandomInt(0, 1));
	SetBodygroup(FindBodygroupByName("packs_thigh"), RandomInt(0, 1));

	m_iAR2AmmoType = GetAmmoDef()->Index("AR2");

	m_flNextAltFireTime = gpGlobals->curtime;
	m_flNextGrenadeCheck = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: For combine melee attack (kick/hit)
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_HumanFGrunt::MeleeAttack1Conditions(float flDot, float flDist)
{
	if (flDist > 64)
	{
		return COND_NONE; // COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NONE; // COND_NOT_FACING_ATTACK;
	}

	// Check Z
	if (GetEnemy() && fabs(GetEnemy()->GetAbsOrigin().z - GetAbsOrigin().z) > 64)
		return COND_NONE;

	if (dynamic_cast<CBaseHeadcrab *>(GetEnemy()) != NULL)
	{
		return COND_NONE;
	}

	// Make sure not trying to kick through a window or something. 
	trace_t tr;
	Vector vecSrc, vecEnd;

	vecSrc = WorldSpaceCenter();
	vecEnd = GetEnemy()->WorldSpaceCenter();

	AI_TraceLine(vecSrc, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	if (tr.m_pEnt != GetEnemy())
	{
		return COND_NONE;
	}

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_HumanFGrunt::CanAltFireEnemy(bool bUseFreeKnowledge)
{
	CBaseCombatWeapon *pWeap = GetActiveWeapon();

	if (!pWeap)
		return false;

	//Weapon must use combine ammo
	if (pWeap->GetPrimaryAmmoType() != m_iAR2AmmoType)
		return false;

	if (IsCrouching())
		return false;

	if (gpGlobals->curtime < m_flNextAltFireTime)
		return false;

	if (!GetEnemy())
		return false;

	if (gpGlobals->curtime < m_flNextGrenadeCheck)
		return false;

	CBaseEntity *pEnemy = GetEnemy();

	Vector vecTarget;

	// Determine what point we're shooting at
	if (bUseFreeKnowledge)
	{
		vecTarget = GetEnemies()->LastKnownPosition(pEnemy) + (pEnemy->GetViewOffset()*0.75);// approximates the chest
	}
	else
	{
		vecTarget = GetEnemies()->LastSeenPosition(pEnemy) + (pEnemy->GetViewOffset()*0.75);// approximates the chest
	}

	// Trace a hull about the size of the combine ball (don't shoot through grates!)
	trace_t tr;

	Vector mins(-12, -12, -12);
	Vector maxs(12, 12, 12);

	Vector vShootPosition = EyePosition();

	if (GetActiveWeapon())
	{
		GetActiveWeapon()->GetAttachment("muzzle", vShootPosition);
	}

	// Trace a hull about the size of the combine ball.
	UTIL_TraceHull(vShootPosition, vecTarget, mins, maxs, MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr);

	float flLength = (vShootPosition - vecTarget).Length();

	flLength *= tr.fraction;

	//If the ball can travel at least 65% of the distance to the player then let the NPC shoot it.
	if (tr.fraction >= 0.65 && flLength > 128.0f)
	{
		// Target is valid
		m_vecAltFireTarget = vecTarget;
		return true;
	}


	// Check again later
	m_vecAltFireTarget = vec3_origin;
	m_flNextGrenadeCheck = gpGlobals->curtime + 1.0f;
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_HumanFGrunt::DelayAltFireAttack(float flDelay)
{
	float flNextAltFire = gpGlobals->curtime + flDelay;

	if (flNextAltFire > m_flNextAltFireTime)
	{
		// Don't let this delay order preempt a previous request to wait longer.
		m_flNextAltFireTime = flNextAltFire;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_HumanFGrunt::DelaySquadAltFireAttack(float flDelay)
{
	// Make sure to delay my own alt-fire attack.
	DelayAltFireAttack(flDelay);

	AISquadIter_t iter;
	CAI_BaseNPC *pSquadmate = m_pSquad ? m_pSquad->GetFirstMember(&iter) : NULL;
	while (pSquadmate)
	{
		CNPC_HumanFGrunt *pCombine = dynamic_cast<CNPC_HumanFGrunt*>(pSquadmate);

		if (pCombine)
		{
			pCombine->DelayAltFireAttack(flDelay);
		}

		pSquadmate = m_pSquad->GetNextMember(&iter);
	}
}

//=========================================================
// start task
//=========================================================
void CNPC_HumanFGrunt::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_MARINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET:
		SetIdealActivity((Activity)(int)pTask->flTaskData);
		GetMotor()->SetIdealYawToTargetAndUpdate(m_vecAltFireTarget, AI_KEEP_YAW_SPEED);
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_HumanFGrunt::RunTask(const Task_t *pTask)
{

	switch (pTask->iTask)
	{

	case TASK_MARINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET:
		GetMotor()->SetIdealYawToTargetAndUpdate(m_vecAltFireTarget, AI_KEEP_YAW_SPEED);

		if (IsActivityFinished())
		{
			TaskComplete();
		}
		break;

	default:
	{
		BaseClass::RunTask(pTask);
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_HumanFGrunt::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
	{
		if (!IsCrouching())
		{
			if (GetEnemy() && CouldShootIfCrouching(GetEnemy()))
			{
				Crouch();
				return SCHED_COMBAT_FACE;
			}
		}

		if (HasCondition(COND_SEE_ENEMY))
		{
			return TranslateSchedule(SCHED_TAKE_COVER_FROM_ENEMY);
		}
	}
	break;
	
	case SCHED_ESTABLISH_LINE_OF_FIRE:
	{
		// always assume standing
		// Stand();

		if (CanAltFireEnemy(true) && OccupyStrategySlot(SQUAD_SLOT_SPECIAL_ATTACK))
		{
			// If an elite in the squad could fire a combine ball at the player's last known position,
			// do so!
			return SCHED_MARINE_AR2_ALTFIRE;
		}

	}
	break;

	case SCHED_RANGE_ATTACK1:
	{
		//if (HasCondition(COND_NO_PRIMARY_AMMO) || HasCondition(COND_LOW_PRIMARY_AMMO))
		//{
		//	// Ditch the strategy slot for attacking (which we just reserved!)
		//	VacateStrategySlot();
		//	return TranslateSchedule(SCHED_HIDE_AND_RELOAD);
		//}

		if (CanAltFireEnemy(true) && OccupyStrategySlot(SQUAD_SLOT_SPECIAL_ATTACK))
		{
			// Since I'm holding this squadslot, no one else can try right now. If I die before the shot 
			// goes off, I won't have affected anyone else's ability to use this attack at their nearest
			// convenience.
			return SCHED_MARINE_AR2_ALTFIRE;
		}

		if (IsCrouching() || (CrouchIsDesired() && !HasCondition(COND_HEAVY_DAMAGE)))
		{
			// See if we can crouch and shoot
			if (GetEnemy() != NULL)
			{
				float dist = (GetLocalOrigin() - GetEnemy()->GetLocalOrigin()).Length();

				// only crouch if they are relatively far away
				if (dist > COMBINE_MIN_CROUCH_DISTANCE)
				{
					// try crouching
					Crouch();

					Vector targetPos = GetEnemy()->BodyTarget(GetActiveWeapon()->GetLocalOrigin());

					// if we can't see it crouched, stand up
					if (!WeaponLOSCondition(GetLocalOrigin(), targetPos, false))
					{
						Stand();
					}
				}
			}
		}
		else
		{
			// always assume standing
			Stand();
		}

	}
	break;
	
	}

	return BaseClass::TranslateSchedule(scheduleType);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_HumanFGrunt::CreateBehaviors()
{
	AddBehavior(&m_RappelBehavior);

	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanFGrunt::SelectModel()
{
	SetModelName(AllocPooledString("models/humans/marine_choreo.mdl"));
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNPC_HumanFGrunt::HandleAnimEvent(animevent_t *pEvent)
{
	

	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if (pEvent->event == COMBINE_AE_BEGIN_ALTFIRE)
		{
			//We want it use different sounds depending on the weapon
			if (GetActiveWeapon() && FClassnameIs(GetActiveWeapon(), "weapon_ar2"))
			{
				EmitSound("Weapon_CombineGuard.Special1");
				DispatchParticleEffect("precharge_fire_tp", PATTACH_POINT_FOLLOW, GetActiveWeapon(), "muzzle");
			}
			else
			{
				EmitSound("Weapon_CombineGuard.Special1"); //We let this play by default
			}
			
		}
		else if (pEvent->event == COMBINE_AE_ALTFIRE)
		{
			if (GetActiveWeapon())
			{
				animevent_t fakeEvent;

				fakeEvent.pSource = this;
				fakeEvent.event = EVENT_WEAPON_AR2_ALTFIRE;
				GetActiveWeapon()->Operator_HandleAnimEvent(&fakeEvent, this);
				if (FClassnameIs(GetActiveWeapon(), "weapon_ar2"))
					DispatchParticleEffect("charge_fire_tp", PATTACH_POINT_FOLLOW, GetActiveWeapon(), "muzzle");

				// Stop other squad members from combine balling for a while.
				DelaySquadAltFireAttack(2.0f);

				// Stop us for longer
				DelayAltFireAttack(5.0f);

				// I'm disabling this decrementor. At the time of this change, the elites
				// don't bother to check if they have grenades anyway. This means that all
				// elites have infinite combine balls, even if the designer marks the elite
				// as having 0 grenades. By disabling this decrementor, yet enabling the code
				// that makes sure the elite has grenades in order to fire a combine ball, we
				// preserve the legacy behavior while making it possible for a designer to prevent
				// elites from shooting combine balls by setting grenades to '0' in hammer. (sjb) EP2_OUTLAND_10
				// m_iNumGrenades--;
			}

			
		}
		else if (pEvent->event == AE_METROPOLICE_KICK_BALLS)
		{
			// Does no damage, because damage is applied based upon whether the target can handle the interaction
			CBaseEntity *pHurt = CheckTraceHullAttack(70, -Vector(16, 16, 18), Vector(16, 16, 18), 0, DMG_CLUB);
			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(pHurt);
			if (pBCC)
			{
				Vector forward, up;
				AngleVectors(GetLocalAngles(), &forward, NULL, &up);

				if (!pBCC->DispatchInteraction(g_interactionCombineBash, NULL, this))
				{
					if (pBCC->IsPlayer())
					{
						pBCC->ViewPunch(QAngle(-12, -7, 0));
						pHurt->ApplyAbsVelocityImpulse(forward * 100 + up * 50);
					}

					CTakeDamageInfo info(this, this, m_nKickDamage, DMG_CLUB);
					CalculateMeleeDamageForce(&info, forward, pBCC->GetAbsOrigin());
					pBCC->TakeDamage(info);

					EmitSound("NPC_Combine.WeaponBash");
				}
			}
		}
		else
		{
			BaseClass::HandleAnimEvent(pEvent);
		}
	}
	else
	{
		switch (pEvent->event)
		{
		//case COMBINE_AE_GREN_TOSS:
		//{
		//	Vector vecSpin;
		//	vecSpin.x = random->RandomFloat(-1000.0, 1000.0);
		//	vecSpin.y = random->RandomFloat(-1000.0, 1000.0);
		//	vecSpin.z = random->RandomFloat(-1000.0, 1000.0);

		//	Vector vecStart;
		//	GetAttachment("anim_attachment_LH", vecStart);

		//	if (m_NPCState == NPC_STATE_SCRIPT)
		//	{
		//		// Use a fixed velocity for grenades thrown in scripted state.
		//		// Grenades thrown from a script do not count against grenades remaining for the AI to use.
		//		Vector forward, up, vecThrow;

		//		GetVectors(&forward, NULL, &up);
		//		vecThrow = forward * 750 + up * 175;
		//		Fraggrenade_Create(vecStart, vec3_angle, vecThrow, vecSpin, this, COMBINE_GRENADE_TIMER, true);
		//	}
		//	else
		//	{
		//		// Use the Velocity that AI gave us.
		//		Fraggrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, COMBINE_GRENADE_TIMER, true);
		//		m_iNumGrenades--;
		//	}

		//	// wait six seconds before even looking again to see if a grenade can be thrown.
		//	m_flNextGrenadeCheck = gpGlobals->curtime + 6;
		//}
		//break;

		//case COMBINE_AE_GREN_LAUNCH:
		//{
		//	EmitSound("NPC_Hgrunt.GrenadeLaunch");

		//	Vector vecSpin;
		//	vecSpin.x = random->RandomFloat(-1000.0, 1000.0);
		//	vecSpin.y = random->RandomFloat(-1000.0, 1000.0);
		//	vecSpin.z = random->RandomFloat(-1000.0, 1000.0);

		//	Vector vecStart;
		//	vecStart = Weapon_ShootPosition();

		//	Contactgrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vecSpin, this, COMBINE_GRENADE_TIMER, true);

		//	if (g_pGameRules->GetSkillLevel() >= SKILL_HARD)
		//		m_flNextGrenadeCheck = gpGlobals->curtime + random->RandomFloat(2, 5);// wait a random amount of time before shooting again
		//	else
		//		m_flNextGrenadeCheck = gpGlobals->curtime + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		//}
		//break;

		/*case COMBINE_AE_GREN_DROP:
		{
			Vector vecStart;
			GetAttachment("anim_attachment_LH", vecStart);

			Fraggrenade_Create(vecStart, vec3_angle, m_vecTossVelocity, vec3_origin, this, COMBINE_GRENADE_TIMER, true);
			m_iNumGrenades--;
		}
		break;*/

		case COMBINE_AE_KICK:
		{
			// Does no damage, because damage is applied based upon whether the target can handle the interaction
			CBaseEntity *pHurt = CheckTraceHullAttack(70, -Vector(16, 16, 18), Vector(16, 16, 18), 0, DMG_CLUB);
			CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(pHurt);
			if (pBCC)
			{
				Vector forward, up;
				AngleVectors(GetLocalAngles(), &forward, NULL, &up);

				if (!pBCC->DispatchInteraction(g_interactionCombineBash, NULL, this))
				{
					if (pBCC->IsPlayer())
					{
						pBCC->ViewPunch(QAngle(-12, -7, 0));
						pHurt->ApplyAbsVelocityImpulse(forward * 100 + up * 50);
					}

					CTakeDamageInfo info(this, this, m_nKickDamage, DMG_CLUB);
					CalculateMeleeDamageForce(&info, forward, pBCC->GetAbsOrigin());
					pBCC->TakeDamage(info);

					EmitSound("NPC_Combine.WeaponBash");
				}
			}

			break;
		}

		

		default:
			BaseClass::HandleAnimEvent(pEvent);
			break;
		}
	}
}

AI_BEGIN_CUSTOM_NPC(npc_human_fgrunt, CNPC_HumanFGrunt)
DECLARE_ANIMEVENT(AE_METROPOLICE_KICK_BALLS);
DECLARE_ANIMEVENT(COMBINE_AE_ALTFIRE);
DECLARE_ANIMEVENT(COMBINE_AE_BEGIN_ALTFIRE);

DECLARE_TASK(TASK_MARINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET);

//=========================================================
// AR2 Alt Fire Attack
//=========================================================
DEFINE_SCHEDULE
(
	SCHED_MARINE_AR2_ALTFIRE,

	"	Tasks"
	"		TASK_STOP_MOVING									0"
	//"		TASK_ANNOUNCE_ATTACK								1"
	"		TASK_MARINE_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET		ACTIVITY:ACT_COMBINE_AR2_ALTFIRE"
	""
	"	Interrupts"
)

AI_END_CUSTOM_NPC()