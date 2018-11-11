//---------------------------------------------------------
//	npc_gargantua.cpp: The 'Black Mesa: Source' compatible
//	'Gargantua' NPC. Created base on an analysis of it's
//	model and observations of it's behavior in 'BM:S'.
//
//	Author: Petercov (petercov@outlook.com)
//	Snippets: 
//--------------------------------------------------------

#include "cbase.h"
#include "beam_shared.h"
#include "Sprite.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#include "shake.h"
#include "IEffects.h"
#include "ai_baseactor.h"
#include "rope.h"
#include "physics_bone_follower.h"
#include "props.h"
#include "ai_memory.h"
#include "ai_hint.h"
#include "animation.h"
#include "bone_setup.h"

#define SF_SHAKE_EVERYONE	0x0001		// Don't check radius
#define SF_SHAKE_INAIR		0x0004		// Shake players in air
#define SF_SHAKE_PHYSICS	0x0008		// Shake physically (not just camera)
#define SF_SHAKE_ROPES		0x0010		// Shake ropes too.
#define SF_SHAKE_NO_VIEW	0x0020		// DON'T shake the view (only ropes and/or physics objects)
#define SF_SHAKE_NO_RUMBLE	0x0040		// DON'T Rumble the XBox Controller

// Garg animation events
//#define GARG_AE_SLASH_LEFT			1
//#define GARG_AE_BEAM_ATTACK_RIGHT	2		// No longer used
//#define GARG_AE_LEFT_FOOT			3
//#define GARG_AE_RIGHT_FOOT			4
//#define GARG_AE_STOMP				5
//#define GARG_AE_BREATHE				6

const float GARG_ATTACKDIST = 90.0;

// Gargantua is immune to any damage but this
#define GARG_DAMAGE					(DMG_ENERGYBEAM|DMG_BLAST|DMG_ACID)
#define GARG_MODEL					"models/xenians/garg.mdl"
//#define GARG_EYE_SPRITE_NAME		"sprites/gargeye1.vmt"
//#define GARG_BEAM_SPRITE_NAME		"sprites/xbeam3.vmt"
//#define GARG_BEAM_SPRITE2			"sprites/xbeam3.vmt"
//#define GARG_STOMP_SPRITE_NAME		"sprites/gargeye1.vmt"
//#define GARG_STOMP_BUZZ_SOUND		"weapons/mine_charge.wav"
//#define GARG_FLAME_LENGTH			330
//#define GARG_GIB_MODEL				"models/metalplategibs.mdl"

ConVar sk_gargantua_melee_dmg("sk_gargantua_melee_dmg", "0");
ConVar sk_gargantua_flame_dmg("sk_gargantua_flame_dmg", "0");
//ConVar garg_fire_time("sk_gargantua_fire_time", "0.8");
ConVar garg_ally("sv_garg_ally", "0", FCVAR_CHEAT);

ConVar debug_garg("g_debug_garg", "0", FCVAR_CHEAT);

//#define GARG_FIRE_TIME garg_fire_time.GetFloat()

int AE_GARG_REMOVE;
int AE_GARG_FLAME_LEFT;
int AE_GARG_FLAME_RIGHT;
int AE_MELEE_ATTACK_LEFT;
int AE_MELEE_ATTACK_RIGHT;
Activity ACT_ROAR;
Activity powerup_roomexit;
Activity powerup_ending;
Activity powerup_tramholding01;
Activity powerup_tramholding02;
Activity powerup_tramholding03;
Activity powerup_tramholding04;

/*
mstudioseqdesc_t *pSeq = &hdr->pSeqdesc( iSeq );

// Now read out all the sound events with their timing
for ( int iEvent=0; iEvent < (int)pSeq->numevents; iEvent++ )
{
mstudioevent_t *pEvent = pSeq->pEvent( iEvent );
*/

class CNPC_Garg : public CAI_BaseActor
{
	DECLARE_CLASS(CNPC_Garg, CAI_BaseActor)
public:
	DECLARE_NETWORKCLASS();
	void Spawn();
	void Precache();

	Class_T Classify() { return garg_ally.GetBool() ? CLASS_VORTIGAUNT : CLASS_ANTLION; }

	/*virtual void		StartTask(const Task_t *pTask);
	virtual void		RunTask(const Task_t *pTask);*/

	void		HandleAnimEvent(animevent_t *pEvent);

	// TraceAttack
	virtual void TraceAttack(const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);

	// This is what you should call to apply damage to an entity.
	//void TakeDamage(const CTakeDamageInfo &inputInfo);

	virtual bool			CanBecomeServerRagdoll(void) { return false; }

	virtual int		OnTakeDamage_Alive(const CTakeDamageInfo &info);

	virtual float		InnateRange1MinRange(void) { return 110; }
	virtual float		InnateRange1MaxRange(void) { return 310; }
	void UpdateOnRemove(void);

	bool FValidateHintType(CAI_Hint *pHint);


	virtual void		OnChangeActivity(Activity eNewActivity);

	enum
	{
		SCHED_GARG_ROAR = BaseClass::NEXT_SCHEDULE,
		SCHED_GARG_SUPPRESS,

		NEXT_SCHEDULE,

		TASK_GARG_SUPPRESS = BaseClass::NEXT_TASK,

		NEXT_TASK,
	};

	// Tasks
	virtual void StartTask(const Task_t *pTask);
	virtual void RunTask(const Task_t *pTask);

	virtual int		TranslateSchedule(int scheduleType);
	int			SelectGargSchedule(bool bFailed = false);

	virtual void		IdleSound(void)
	{
		EmitSound("npc_sounds_gargantua.Idle");
	}

	virtual void		AimGun();

	int RangeAttack1Conditions(float flDot, float flDist)
	{
		/*if (MeleeAttack1Conditions(flDot, flDist) == COND_CAN_MELEE_ATTACK1)
			return 0;*/

		if (flDist < InnateRange1MinRange())
		{
			return COND_TOO_CLOSE_TO_ATTACK;
		}
		else if (flDist > InnateRange1MaxRange())
		{
			return COND_TOO_FAR_TO_ATTACK;
		}
		else if (flDot < 0.5)
		{
			return COND_NOT_FACING_ATTACK;
		}

		return COND_CAN_RANGE_ATTACK1;
	}

	int MeleeAttack1Conditions(float flDot, float flDist)
	{
		if (flDist > GARG_ATTACKDIST)
		{
			return COND_TOO_FAR_TO_ATTACK;
		}
		else if (flDot < 0.5)
		{
			return COND_NOT_FACING_ATTACK;
		}
		else if (GetEnemy() == NULL)
		{
			return 0;
		}

		// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
		//if (GetEnemy()->GetFlags() & FL_ONGROUND)
		{
			return COND_CAN_MELEE_ATTACK1;
		}
		return 0;
	}

	int					SelectSchedule();

	//Vector Weapon_ShootPosition()
	//{
	//	//Vector forward;
	//	//Vector vecShootPos;

	//	/*if (m_iLeftHandAttachment)
	//	{
	//		Vector vecLeft, vecRight;
	//		GetAttachment(m_iLeftHandAttachment, vecLeft);
	//		GetAttachment(m_iRightHandAttachment, vecRight);

	//		vecShootPos = VectorLerp(vecLeft, vecRight, 0.5f);
	//	}*/
	//	if (GetEnemy() && EnemyDistance(GetEnemy()) > InnateRange1MinRange())
	//	{
	//		Vector vecLeft, vecRight;

	//		GetAttachment(m_iLeftHandAttachment, vecLeft);
	//		GetAttachment(m_iRightHandAttachment, vecRight);

	//		return VectorLerp(vecLeft, vecRight, 0.5);
	//	}

	//	return BaseClass::Weapon_ShootPosition();
	//}

	// Thinking, including core thinking, movement, animation
	virtual void		NPCThink(void);

	int GetSoundInterests(void)
	{
		return	SOUND_WORLD |
			SOUND_COMBAT |
			SOUND_CARCASS |
			SOUND_MEAT |
			SOUND_GARBAGE |
			//SOUND_DANGER |
			SOUND_PLAYER;
	}

	float MaxYawSpeed(void)
	{
		int ys;

		switch (GetActivity())
		{
		case ACT_IDLE:
			ys = 15;
			break;
		case ACT_TURN_LEFT:
		case ACT_TURN_RIGHT:
			ys = 30;
			break;
		case ACT_WALK:
		case ACT_RUN:
			ys = 60;
			break;

		default:
			ys = 60;
			break;
		}

		return ys;
	}

	//---------------------------------------------------------
	//---------------------------------------------------------
	bool CreateVPhysics()
	{
		// The strider has bone followers for every solid part of its body, 
		// so there's no reason for the bounding box to be solid.
		BaseClass::CreateVPhysics();

		//if (!m_bDisableBoneFollowers)
		/*{
			InitBoneFollowers();
		}*/

		return true;
	}

	void			PainSound(const CTakeDamageInfo &info);

	virtual bool		ShouldProbeCollideAgainstEntity(CBaseEntity *pEntity);

	DEFINE_CUSTOM_AI
	DECLARE_DATADESC()
protected:

	CNetworkVar(float, m_flLeftFireTime);
	CNetworkVar(float, m_flRightFireTime);

	CNetworkVar(float, m_flChargeEndCycle);

	bool m_bFireOn;
	
	float m_flNextRoarTime;

	//Adrian: Let's do it the right way!
	int				m_iLeftHandAttachment;
	int				m_iRightHandAttachment;

	void DoFlameDamage(int iAttachment);
	void InitBoneFollowers();

	//EHANDLE m_hShake;

	private:

		// Contained Bone Follower manager
		CBoneFollowerManager m_BoneFollowerManager;
};

IMPLEMENT_SERVERCLASS_ST(CNPC_Garg, DT_NPC_Garg)
SendPropTime(SENDINFO(m_flLeftFireTime)),
SendPropTime(SENDINFO(m_flRightFireTime)),
SendPropFloat(SENDINFO(m_flChargeEndCycle), ANIMATION_CYCLE_BITS, SPROP_ROUNDDOWN, 0.0f, 1.0f),
END_NETWORK_TABLE()

BEGIN_DATADESC(CNPC_Garg)

//DEFINE_FIELD(m_hShake,FIELD_EHANDLE),
DEFINE_FIELD(m_flLeftFireTime,FIELD_TIME),
DEFINE_FIELD(m_flRightFireTime, FIELD_TIME),
DEFINE_FIELD(m_flNextRoarTime, FIELD_TIME),
DEFINE_FIELD(m_iLeftHandAttachment, FIELD_INTEGER),
DEFINE_FIELD(m_iRightHandAttachment, FIELD_INTEGER),
DEFINE_FIELD(m_flChargeEndCycle, FIELD_FLOAT),
DEFINE_EMBEDDED(m_BoneFollowerManager),

END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_gargantua, CNPC_Garg);

AI_BEGIN_CUSTOM_NPC(npc_gargantua, CNPC_Garg)

DECLARE_ACTIVITY(ACT_ROAR)
DECLARE_ACTIVITY(powerup_roomexit)
DECLARE_ACTIVITY(powerup_ending)
DECLARE_ACTIVITY(powerup_tramholding01)
DECLARE_ACTIVITY(powerup_tramholding02)
DECLARE_ACTIVITY(powerup_tramholding03)
DECLARE_ACTIVITY(powerup_tramholding04)

DECLARE_ANIMEVENT(AE_GARG_REMOVE)
DECLARE_ANIMEVENT(AE_GARG_FLAME_LEFT)
DECLARE_ANIMEVENT(AE_GARG_FLAME_RIGHT)
DECLARE_ANIMEVENT(AE_MELEE_ATTACK_LEFT)
DECLARE_ANIMEVENT(AE_MELEE_ATTACK_RIGHT)

DECLARE_TASK(TASK_GARG_SUPPRESS)


DEFINE_SCHEDULE(
	SCHED_GARG_ROAR,

	"	Tasks"
	"		TASK_STOP_MOVING		0"
	"		TASK_PLAY_SEQUENCE_FACE_ENEMY		ACTIVITY:ACT_ROAR "
	//"		TASK_SOUND_WAKE			0"
	//"		TASK_FACE_IDEAL			0"
	"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE_ANGRY "
	""
	"	Interrupts"
	)

	DEFINE_SCHEDULE(
		SCHED_GARG_SUPPRESS,
		
		"	Tasks"
		"		TASK_GET_PATH_TO_HINTNODE	0"
		"		TASK_RUN_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_HINTNODE			0"
		"		TASK_LOCK_HINTNODE			0"
		"		TASK_REMEMBER				Memory:LOCKED_HINT"
		"		TASK_GARG_SUPPRESS			0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_LOST_ENEMY"
		"		COND_REPEATED_DAMAGE"
		"		COND_CAN_RANGE_ATTACK1"
		)

AI_END_CUSTOM_NPC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Garg::OnChangeActivity(Activity eNewActivity)
{
	BaseClass::OnChangeActivity(eNewActivity);

	float flCycle = 0.0f;

	mstudioseqdesc_t *pSeq = &GetModelPtr()->pSeqdesc(GetSequence());

	// Now read out all the sound events with their timing
	for (int iEvent = 0; iEvent < (int)pSeq->numevents; iEvent++)
	{
		mstudioevent_t *pEvent = pSeq->pEvent(iEvent);

		if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
		{
			if (pEvent->event == AE_GARG_REMOVE)
			{
				flCycle = pEvent->cycle;
				break;
			}
		}
	}

	m_flChargeEndCycle = flCycle;
}

void CNPC_Garg::AimGun()
{
	if (GetEnemy() && EnemyDistance(GetEnemy()) > InnateRange1MinRange())
		BaseClass::AimGun();
	else
		RelaxAim();
}

void CNPC_Garg::PainSound(const CTakeDamageInfo &info)
{
	if (FOkToMakeSound(SOUND_PRIORITY_HIGH))
	{
		EmitSound("npc_sounds_gargantua.Pain");
		JustMadeSound(SOUND_PRIORITY_HIGH, 2.5f);
	}
}

bool CNPC_Garg::FValidateHintType(CAI_Hint *pHint)
{
	if (pHint->HintType() == HINT_BMS_MINIBOSS)
		return true;

	return false;
}

int CNPC_Garg::SelectSchedule()
{
	if (HasCondition(COND_NEW_ENEMY))
	{
		CBaseEntity *pEnemy = GetEnemy();
		bool bFirstContact = false;
		float flTimeSinceFirstSeen = gpGlobals->curtime - GetEnemies()->FirstTimeSeen(pEnemy);

		if (flTimeSinceFirstSeen < 3.0f)
			bFirstContact = true; 

		if (bFirstContact && gpGlobals->curtime >= m_flNextRoarTime)
		{
			m_flNextRoarTime = gpGlobals->curtime + 10.0f;

			return SCHED_GARG_ROAR;
		}
	}

	int iSched = SelectGargSchedule();
	if (iSched != SCHED_NONE)
		return iSched;

	if (HasCondition(COND_CAN_MELEE_ATTACK1))
	{
		return SCHED_MELEE_ATTACK1;
	}

	if (m_NPCState == NPC_STATE_IDLE || m_NPCState == NPC_STATE_ALERT)
	{
		return SCHED_PATROL_WALK;
	}
	
	return BaseClass::SelectSchedule();
}

int CNPC_Garg::SelectGargSchedule(bool bFailed)
{
	if (GetEnemy() && HasCondition(COND_ENEMY_UNREACHABLE))
	{
		CHintCriteria criteria;
		criteria.SetGroup(GetHintGroup());
		criteria.SetHintType(HINT_BMS_MINIBOSS);
		criteria.SetFlag(bits_HINT_NODE_NEAREST | bits_HINT_NODE_USE_GROUP /*| bits_HINT_HAS_LOS_TO_PLAYER*/);
		criteria.AddIncludePosition(GetEnemy()->GetAbsOrigin(), 512);
		SetHintNode(CAI_HintManager::FindHint(this, GetEnemy()->GetAbsOrigin(), criteria));

		if (GetHintNode())
		{
			return SCHED_GARG_SUPPRESS;
		}
	}

	if (bFailed)
	{
		if (gpGlobals->curtime >= m_flNextRoarTime)
		{
			m_flNextRoarTime = gpGlobals->curtime + 10.0f;

			return SCHED_GARG_ROAR;
		}
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_Garg::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
	{
		int iSched = SelectGargSchedule(true);
		if (iSched != SCHED_NONE)
			return TranslateSchedule(iSched);
	}
	}


	return BaseClass::TranslateSchedule(scheduleType);
}

void CNPC_Garg::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_GARG_SUPPRESS:
		ChainStartTask(TASK_PLAY_HINT_ACTIVITY);
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}

void CNPC_Garg::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_GARG_SUPPRESS:
	{
		if (!GetHintNode())
		{
			TaskFail(FAIL_NO_HINT_NODE);
		}

		// Put a debugging check in here
		if (GetHintNode()->User() != this)
		{
			DevMsg("Hint node (%s) being used by non-owner!\n", GetHintNode()->GetDebugName());
		}

		if (GetIdealActivity() != ACT_DO_NOT_DISTURB && GetActivity() != ACT_TRANSITION && m_translatedActivity != ACT_IDLE && IsActivityFinished())
		{
			int iSequence;

			// FIXME: this doesn't reissue a translation, so if the idle activity translation changes over time, it'll never get reset
			if (SequenceLoops())
			{
				// animation does loop, which means we're playing subtle idle. Might need to fidget.
				iSequence = SelectWeightedSequence(m_translatedActivity);
			}
			else
			{
				// animation that just ended doesn't loop! That means we just finished a fidget
				// and should return to our heaviest weighted idle (the subtle one)
				iSequence = SelectHeaviestSequence(m_translatedActivity);
			}
			if (iSequence != ACTIVITY_NOT_AVAILABLE)
			{
				ResetSequence(iSequence); // Set to new anim (if it's there)

										  //Adrian: Basically everywhere else in the AI code this variable gets set to whatever our sequence is.
										  //But here it doesn't and not setting it causes any animation set through here to be stomped by the 
										  //ideal sequence before it has a chance of playing out (since there's code that reselects the ideal 
										  //sequence if it doesn't match the current one).
				//if (hl2_episodic.GetBool())
				{
					m_nIdealSequence = iSequence;
				}
			}
		}

		break;
	}

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CNPC_Garg::ShouldProbeCollideAgainstEntity(CBaseEntity *pEntity)
{
	if (pEntity->GetMoveType() == MOVETYPE_VPHYSICS)
	{
		//if (ai_test_moveprobe_ignoresmall.GetBool() && IsNavigationUrgent())
		{
			IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();

			if (pPhysics->IsMoveable() && !pPhysics->IsHinged())
				return false;
		}
	}

	return BaseClass::ShouldProbeCollideAgainstEntity(pEntity);
}

void CNPC_Garg::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->event == AE_GARG_REMOVE)
	{
		AngularImpulse	angVelocity;
		QAngleToAngularImpulse(GetLocalAngularVelocity(), angVelocity);
		breakablepropparams_t params(GetAbsOrigin(), GetAbsAngles(), GetAbsVelocity(), angVelocity);
		params.impactEnergyScale = 1.0f;
		params.defBurstScale = 10.0f;
		params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE_DEBRIS;
		PropBreakableCreateAll(GetModelIndex(), VPhysicsGetObject(), params, this, -1, true, true);
		UTIL_Remove(this);
	}
	else if (pEvent->event == AE_GARG_FLAME_LEFT)
	{
		/*if (m_NPCState == NPC_STATE_SCRIPT)
		{
			if (m_flLeftFireTime > gpGlobals->curtime)
				m_flLeftFireTime = gpGlobals->curtime;
			else
				m_flLeftFireTime = gpGlobals->curtime + 10;
		}*/
		/*else*/
		{
			m_flLeftFireTime = gpGlobals->curtime + atof(pEvent->options);
		}
	}
	else if (pEvent->event == AE_GARG_FLAME_RIGHT)
	{
		

		/*if ()
		{
			if (m_flRightFireTime > gpGlobals->curtime)
				m_flRightFireTime = gpGlobals->curtime;
			else
				m_flRightFireTime = gpGlobals->curtime + 10;
		}
		else*/
		{
			m_flRightFireTime = gpGlobals->curtime + atof(pEvent->options);
		}
	}
	else if (pEvent->event == AE_MELEE_ATTACK_LEFT || pEvent->event == AE_MELEE_ATTACK_RIGHT)
	{
		// If only a length is given assume we want to trace in our facing direction
		Vector forward;
		AngleVectors(GetAbsAngles(), &forward);
		Vector vStart = GetAbsOrigin();
		Vector maxs = Vector(64, 64, 64);

		// The ideal place to start the trace is in the center of the attacker's bounding box.
		// however, we need to make sure there's enough clearance. Some of the smaller monsters aren't 
		// as big as the hull we try to trace with. (SJB)
		float flVerticalOffset = WorldAlignSize().z * 0.35;

		if (flVerticalOffset < maxs.z)
		{
			// There isn't enough room to trace this hull, it's going to drag the ground.
			// so make the vertical offset just enough to clear the ground.
			flVerticalOffset = maxs.z + 1.0;
		}

		vStart.z += flVerticalOffset;
		Vector vEnd = vStart + (forward * (GARG_ATTACKDIST + 10.0));

		CBaseEntity *pHurt = CheckTraceHullAttack(vStart, vEnd, -maxs, maxs, sk_gargantua_melee_dmg.GetInt(), DMG_CLUB, 5.0f);

		if (pHurt)
		{
			pHurt->EmitSound("npc_sounds_gargantua.Hit");

			Vector forward, up, forup;
			AngleVectors(GetAbsAngles(), &forward, NULL, &up);
			forup = (up + forward);
			VectorNormalize(forup);
			pHurt->ApplyAbsVelocityImpulse(260 * forup);
			pHurt->ApplyAbsVelocityImpulse(60 * forward);
			pHurt->SetGroundEntity(NULL);
		}

	}
	else if (pEvent->event == AE_NPC_LEFTFOOT || pEvent->event == AE_NPC_RIGHTFOOT)
	{
		{
			//variant_t eVar;
			//m_hShake->AcceptInput("StartShake", this, this, eVar, 0);
			UTIL_ScreenShake(GetAbsOrigin(), 4.0, 3.0, 1.0, 750, SHAKE_START);
			CRopeKeyframe::ShakeRopes(GetAbsOrigin(), 750, 100);
			EmitSound("npc_sounds_gargantua.Footstep");
		}
		return;
	}
	else
	{
		switch (pEvent->event)
		{
		case NPC_EVENT_LEFTFOOT:
		case NPC_EVENT_RIGHTFOOT:
			{
				//variant_t eVar;
				//m_hShake->AcceptInput("StartShake", this, this, eVar, 0);
				EmitSound("Garg.Footstep");
			}
			break;

		default:
			BaseClass::HandleAnimEvent(pEvent);
			break;
		}
		
	}
}

void CNPC_Garg::NPCThink()
{
	BaseClass::NPCThink();

	{
		bool bOldFireOn = m_bFireOn;

		if (m_flLeftFireTime > gpGlobals->curtime || m_flRightFireTime > gpGlobals->curtime)
		{
			m_bFireOn = true;
		}
		else
		{
			m_bFireOn = false;
		}

		if (m_bFireOn != bOldFireOn)
		{
			if (m_bFireOn)
			{
				EmitSound("npc_sounds_gargantua.FlameRun");
			}
			else
			{
				StopSound("npc_sounds_gargantua.FlameRun");
				EmitSound("npc_sounds_gargantua.FlameOff");
			}
		}
	}

	if (m_flLeftFireTime > gpGlobals->curtime)
	{
		DoFlameDamage(m_iLeftHandAttachment);
	}

	if (m_flRightFireTime > gpGlobals->curtime)
	{
		DoFlameDamage(m_iRightHandAttachment);
	}

	// update follower bones
	//m_BoneFollowerManager.UpdateBoneFollowers(this);
	
}

void CNPC_Garg::DoFlameDamage(int iAttachment)
{
	Vector vecPos, vecForward, vecEnd;
	const Vector maxs(16, 16, 16);
	QAngle vecAngles;
	CTakeDamageInfo info(this, this, sk_gargantua_flame_dmg.GetFloat(), DMG_BURN);

	GetAttachment(iAttachment, vecPos, vecAngles);
	AngleVectors(vecAngles, &vecForward);
	vecEnd = vecPos + (vecForward * 300);

	CBaseEntity *pList[32];

	Ray_t ray;

	ray.Init(vecPos, vecEnd, -maxs, maxs);

	int iFound = UTIL_EntitiesAlongRay(pList, ARRAYSIZE(pList), ray, FL_NPC | FL_CLIENT);

	if (debug_garg.GetBool())
		NDebugOverlay::SweptBox(vecPos, vecEnd, -maxs, maxs, vec3_angle, 255, 100, 0, 100, 0.1f);

	for (int i = 0; i < iFound; i++)
	{
		CBaseEntity *pEnt = pList[i];

		if (pEnt == this)
			continue;

		if (pEnt->m_takedamage == DAMAGE_NO)
			continue;

		if (!pEnt->IsAlive())
			continue;

		pEnt->TakeDamage(info);

		CBaseAnimating *pAnim = pEnt->GetBaseAnimating();
		if (pAnim && IRelationType(pAnim) == D_HT)
			pAnim->Ignite(6);
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Garg::InitBoneFollowers(void)
{
	// Don't do this if we're already loaded
	if (m_BoneFollowerManager.GetNumBoneFollowers() != 0)
		return;

	KeyValues *modelKeyValues = new KeyValues("");
	if (modelKeyValues->LoadFromBuffer(modelinfo->GetModelName(GetModel()), modelinfo->GetModelKeyValueText(GetModel())))
	{
		// Do we have a bone follower section?
		KeyValues *pkvBoneFollowers = modelKeyValues->FindKey("bone_followers");
		if (pkvBoneFollowers)
		{
			// Loop through the list and create the bone followers
			KeyValues *pBone = pkvBoneFollowers->GetFirstSubKey();
			while (pBone)
			{
				// Add it to the list
				const char *pBoneName = pBone->GetString();
				m_BoneFollowerManager.AddBoneFollower(this, pBoneName);

				pBone = pBone->GetNextKey();
			}
		}

		modelKeyValues->deleteThis();
	}

	// if we got here, we don't have a bone follower section, but if we have a ragdoll
	// go ahead and create default bone followers for it
	if (m_BoneFollowerManager.GetNumBoneFollowers() == 0)
	{
		vcollide_t *pCollide = modelinfo->GetVCollide(GetModelIndex());
		if (pCollide && pCollide->solidCount > 1)
		{
			CreateBoneFollowersFromRagdoll(this, &m_BoneFollowerManager, pCollide);
		}
	}

	// Init our followers
	//m_BoneFollowerManager.InitBoneFollowers(this, ARRAYSIZE(pFollowerBoneNames), pFollowerBoneNames);
}

void CNPC_Garg::UpdateOnRemove()
{
	//m_BoneFollowerManager.DestroyBoneFollowers();

	BaseClass::UpdateOnRemove();
}

void CNPC_Garg::Precache()
{
	PrecacheModel(GARG_MODEL);
	PropBreakablePrecacheAll(AllocPooledString(GARG_MODEL));

	PrecacheScriptSound("npc_sounds_gargantua.Footstep");
	PrecacheScriptSound("npc_sounds_gargantua.Hit");
	PrecacheScriptSound("npc_sounds_gargantua.Idle");
	PrecacheScriptSound("npc_sounds_gargantua.Pain");
	PrecacheScriptSound("npc_sounds_gargantua.FlameOff");
	PrecacheScriptSound("npc_sounds_gargantua.FlameRun");
	PrecacheParticleSystem("gargantua_flame");
	PrecacheParticleSystem("gargantua_eye");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Garg::Spawn(void)
{
	Precache();

	SetModel(GARG_MODEL);
	SetHullType(HULL_LARGE_TALL);
	SetHullSizeNormal();

	EnableServerIK();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE/*|FSOLID_NOT_SOLID*/ | FSOLID_CUSTOMRAYTEST);
	SetMoveType(MOVETYPE_STEP);
	AddEFlags(EFL_NO_DISSOLVE|EFL_NO_MEGAPHYSCANNON_RAGDOLL);
	SetBloodColor(DONT_BLEED);
	m_iHealth = 800;
	m_flFieldOfView = VIEW_FIELD_FULL;
	m_NPCState = NPC_STATE_NONE;

	Vector vecSurroundingMins(-80, -80, 0);
	Vector vecSurroundingMaxs(80, 80, 225);
	CollisionProp()->SetSurroundingBoundsType(USE_SPECIFIED_BOUNDS, &vecSurroundingMins, &vecSurroundingMaxs);

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_MOVE_GROUND |  bits_CAP_SQUAD | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK1);
	CapabilitiesAdd(bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE | bits_CAP_AIM_GUN);
	//CapabilitiesAdd( bits_CAP_DUCK );
	//UTIL_ScreenShake(pev->origin, 4.0, 3.0, 1.0, 750);

	SetBoneCacheFlags(BCF_NO_ANIMATION_SKIP);
	
	m_HackedGunPos.y = 30;
	m_HackedGunPos.z = 100;

	//Weapon_ShootPosition()
	NPCInit();

	m_iLeftHandAttachment = LookupAttachment("FireL");
	m_iRightHandAttachment = LookupAttachment("FireR");

	m_flNextRoarTime = gpGlobals->curtime;

}

//-----------------------------------------------------------------------------
// TraceAttack
//-----------------------------------------------------------------------------
void CNPC_Garg::TraceAttack(const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
{
	CTakeDamageInfo info = inputInfo;
	int bitsDamageType = info.GetDamageType();
	UpdateEnemyMemory(info.GetAttacker(), info.GetAttacker()->GetAbsOrigin());

	bitsDamageType &= GARG_DAMAGE;

	if ( bitsDamageType == 0)
	{
		if ((info.GetDamageType() & DMG_BULLET) && (m_flLastDamageTime != gpGlobals->curtime || (RandomInt(0, 100) < 20)))
		{
			g_pEffects->Ricochet(ptr->endpos, ptr->plane.normal);
			m_flLastDamageTime = gpGlobals->curtime;
		}
		info.SetDamage(0);
	}
	
	BaseClass::TraceAttack(info, vecDir, ptr, pAccumulator);
}

int CNPC_Garg::OnTakeDamage_Alive(const CTakeDamageInfo &info)
{

	UpdateEnemyMemory(info.GetAttacker(), info.GetAttacker()->GetAbsOrigin());

	if (info.GetDamageType() == DMG_GENERIC)
		return BaseClass::OnTakeDamage_Alive(info);

	// half damage
	CTakeDamageInfo subInfo = info;

	//if (IsAlive())
	{
		if (!(info.GetDamageType() & GARG_DAMAGE))
			subInfo.ScaleDamage(0);
		if (info.GetDamageType() & DMG_BLAST)
			SetCondition(COND_LIGHT_DAMAGE);
	}

	return BaseClass::OnTakeDamage_Alive(subInfo);
}