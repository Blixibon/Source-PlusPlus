#include "cbase.h"
#include "ai_basenpc_flyer.h"
#include "hl2_shareddefs.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Route.h"
#include	"AI_Navigator.h"
#include	"ai_moveprobe.h"
#include "npcevent.h"



#define CONTROLLER_MODEL "models/xenians/controller.mdl"

#define SOUND_ALIENCONTROLLER_PAIN "npc_sounds_alien_controller.Pain"
#define SOUND_ALIENCONTROLLER_DIE "npc_sounds_alien_controller.Die"
#define SOUND_ALIENCONTROLLER_ATTACK "npc_sounds_alien_controller.Attack"
#define SOUND_ALIENCONTROLLER_ALERT "npc_sounds_alien_controller.Alert"
#define SOUND_ALIENCONTROLLER_IDLE "npc_sounds_alien_controller.Idle"

ConVar sk_aliencontroller_health("sk_controller_health", "0", FCVAR_NONE);


Activity ACT_CONTROLLER_ATTACK_HANDS;
Activity ACT_CONTROLLER_ATTACK_LIFT;
Activity ACT_CONTROLLER_ATTACK_THROW;

int AE_CONTROLLER_LIFT_OBJ;
int AE_CONTROLLER_THROW_OBJ;
int AE_CONTROLLER_GLOW_START;
int AE_CONTROLLER_GLOW_STOP;


class CBMSControllerNavigator : public CAI_Navigator
{
	typedef CAI_Navigator BaseClass;
public:
	CBMSControllerNavigator(CAI_BaseNPC *pOuter)
		: BaseClass(pOuter)
	{
	}

	bool ActivityIsLocomotive(Activity activity) { return true; }
};

class CBMSNPC_AlienController : public CAI_BaseFlyingBot
{
public:
	DECLARE_CLASS(CBMSNPC_AlienController, CAI_BaseFlyingBot);
	DEFINE_CUSTOM_AI;

	void Spawn();
	void Precache();
	void Activate();

	//void PostNPCInit();

	void			StartTask(const Task_t *pTask);

	bool OverridePathMove(float flInterval);
	bool OverrideMove(float flInterval);

	void MoveToTarget(float flInterval, const Vector &vecMoveTarget);

	// Thinking, including core thinking, movement, animation
	virtual void		NPCThink(void);

	void HandleAnimEvent(animevent_t *pEvent);

	CAI_Navigator *CreateNavigator()
	{
		return new CBMSControllerNavigator(this);
	}

	Class_T Classify()
	{
		return CLASS_ANTLION;
	}

protected:

	float MinGroundDist(void)
	{
		return 10.0f;
	}

	void Stop(void)
	{
		SetIdealActivity(GetStoppedActivity());
		LimitSpeed(0, 10);
	}

	Vector m_velocity;
};

LINK_ENTITY_TO_CLASS(npc_alien_controller, CBMSNPC_AlienController);

void CBMSNPC_AlienController::Activate()
{
	BaseClass::Activate();

	//RestartGesture(ACT_FLYZ, true, false);
	//int iFlyz = FindGestureLayer(ACT_FLYZ);

	//SetLayerLooping(iFlyz, true);
}

void CBMSNPC_AlienController::Precache()
{
	BaseClass::Precache();

	PrecacheModel(CONTROLLER_MODEL);
	PrecacheScriptSound(SOUND_ALIENCONTROLLER_ATTACK);
}

void CBMSNPC_AlienController::Spawn(void)
{
	Precache();

	SetModel(CONTROLLER_MODEL);

	BaseClass::Spawn();

	SetHullType(HULL_HUMAN); //HULL_WIDE_HUMAN SetHullType( HULL_HUMAN_CENTERED );
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	//SetCollisionBounds(ALIENCONTROLLER_HULL_MINS, ALIENCONTROLLER_HULL_MAXS);

	AddSolidFlags(FSOLID_NOT_STANDABLE);

	CapabilitiesClear();

	CapabilitiesAdd(bits_CAP_SQUAD | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD); // bits_CAP_ANIMATEDFACE |
	CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK1);
	CapabilitiesAdd(bits_CAP_MOVE_SHOOT);
	CapabilitiesAdd(bits_CAP_NO_HIT_SQUADMATES);

	SetNavType(NAV_FLY);

	AddFlag(FL_FLY);

	CapabilitiesAdd(bits_CAP_MOVE_FLY);

	SetMoveType(MOVETYPE_STEP);


	m_bloodColor = BLOOD_COLOR_GREEN;
	m_iHealth = sk_aliencontroller_health.GetFloat();

	m_flFieldOfView = VIEW_FIELD_FULL;

	//AddSpawnFlags(SF_NPC_LONG_RANGE);

	//m_NPCState			= NPC_STATE_NONE;

	

#ifdef HLSS_USE_COMBINE_BALL_DEFENSE
	m_hCombineBall = NULL;
#endif

	

#ifdef HLSS_CONTROLLER_TELEKINESIS
	//TELEKINESIS
	for (int i = 0; i<ALIENCONTROLLER_NUMBER_OF_TELEKINESIS_OBJECTS; i++)
	{
		m_hPhysicsEnt.Set(i, NULL);// = NULL;
	}
	m_iNumberOfPhysiscsEnts = 0;

	m_flNextTelekinesisThrow = 0;
	m_flNextTelekinesisScan = gpGlobals->curtime + ALIENCONTROLLER_TELEKINESIS_INITIAL_SCAN_DELAY;
	m_flNextTelekinesisCancel = 0;
#endif

	NPCInit();
}

void CBMSNPC_AlienController::HandleAnimEvent(animevent_t *pEvent)
{
	if (pEvent->event == AE_CONTROLLER_GLOW_START)
	{

	}
	else
		BaseClass::HandleAnimEvent(pEvent);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBMSNPC_AlienController::StartTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	// Skip as done via bone controller
		case TASK_FACE_ENEMY:
		{
			CAI_BaseNPC::StartTask(pTask);
			break;
		}
		// Activity is just idle (have no run)
		case TASK_RUN_PATH:
		{
			GetNavigator()->SetMovementActivity(ACT_FLY);
			TaskComplete();
			break;
		}

		case TASK_RANGE_ATTACK1:
		{
			SetLastAttackTime(gpGlobals->curtime);
			ResetIdealActivity(ACT_CONTROLLER_ATTACK_HANDS);
			EmitSound(SOUND_ALIENCONTROLLER_ATTACK);
			break;
		}

		default:
		{
			BaseClass::StartTask(pTask);
		}
	}
}

void CBMSNPC_AlienController::NPCThink(void)
{
	BaseClass::NPCThink();

#if 0
	Vector vecAbsVeloc = GetAbsVelocity();
	//if (vecAbsVeloc.LengthSqr() > 0)
	{
		QAngle angVel = GetLocalAngularVelocity();

		Vector forward, right, up;
		AngleVectors(GetLocalAngles() + GetLocalAngularVelocity() * 2, &forward, &right, &up);

		float flDist = DotProduct(vecAbsVeloc, forward);

		// float flSlip = DotProduct( GetAbsVelocity(), right );
		float flSlip = -DotProduct(vecAbsVeloc, right);

		// fly sideways
		if (flSlip > 0)
		{
			if (GetLocalAngles().z > -15 && angVel.z > -15)
				angVel.z -= 4;
			else
				angVel.z += 2;
		}
		else
		{
			if (GetLocalAngles().z < 15 && angVel.z < 15)
				angVel.z += 4;
			else
				angVel.z -= 2;
		}

		float flSpeed = vecAbsVeloc.Length();
		float flDir = DotProduct(Vector(forward.x, forward.y, 0), Vector(vecAbsVeloc.x, vecAbsVeloc.y, 0));
		if (flDir < 0)
		{
			flSpeed = -flSpeed;
		}

		// pitch forward or back to get to target
		//-----------------------------------------
		// Pitch is reversed since Half-Life! (sjb)
		//-----------------------------------------
		if (flDist > 0 && flSpeed < m_flGroundSpeed /* && flSpeed < flDist */ && GetLocalAngles().x + angVel.x < 40)
		{
			// ALERT( at_console, "F " );
			// lean forward
			angVel.x += 12.0;
		}
		else if (flDist < 0 && flSpeed > -50 && GetLocalAngles().x + angVel.x  > -20)
		{
			// ALERT( at_console, "B " );
			// lean backward
			angVel.x -= 12.0;
		}
		else if (GetLocalAngles().x + angVel.x < 0)
		{
			// ALERT( at_console, "f " );
			angVel.x += 4.0;
		}
		else if (GetLocalAngles().x + angVel.x > 0)
		{
			// ALERT( at_console, "b " );
			angVel.x -= 4.0;
		}

		SetLocalAngularVelocity(angVel);
	}
#endif
}

bool CBMSNPC_AlienController::OverridePathMove(float flInterval)
{
	CBaseEntity *pMoveTarget = (GetTarget()) ? GetTarget() : GetEnemy();
	Vector waypointDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();

	float flWaypointDist = waypointDir.Length2D();
	VectorNormalize(waypointDir);

	// cut corner?
	if (flWaypointDist < 128)
	{
		if (m_flGroundSpeed > 100)
			m_flGroundSpeed -= 40;
	}
	else
	{
		if (m_flGroundSpeed < 400)
			m_flGroundSpeed += 10;
	}

	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * waypointDir * 0.5;
	SetAbsVelocity(m_velocity);

	// -----------------------------------------------------------------
	// Check route is blocked
	// ------------------------------------------------------------------
	Vector checkPos = GetLocalOrigin() + (waypointDir * (m_flGroundSpeed * flInterval));

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit(NAV_FLY, GetLocalOrigin(), checkPos, MASK_NPCSOLID | CONTENTS_WATER,
		pMoveTarget, &moveTrace);
	if (IsMoveBlocked(moveTrace))
	{
		TaskFail(FAIL_NO_ROUTE_BLOCKED);
		GetNavigator()->ClearGoal();
		return true;
	}

	// ----------------------------------------------

	Vector lastPatrolDir = GetNavigator()->GetCurWaypointPos() - GetLocalOrigin();

	if (ProgressFlyPath(flInterval, pMoveTarget, MASK_NPCSOLID, false, 64) == AINPP_COMPLETE)
	{
		{
			m_vLastPatrolDir = lastPatrolDir;
			VectorNormalize(m_vLastPatrolDir);
		}
		return true;
	}

	m_vCurrentVelocity += VelocityToAvoidObstacles(flInterval);

	return false;
}

bool CBMSNPC_AlienController::OverrideMove(float flInterval)
{
	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = 100;
	}

	// ----------------------------------------------
	//	Select move target 
	// ----------------------------------------------
	CBaseEntity *pMoveTarget = NULL;
	if (GetTarget() != NULL)
	{
		pMoveTarget = GetTarget();
	}
	else if (GetEnemy() != NULL)
	{
		pMoveTarget = GetEnemy();
	}

	// ----------------------------------------------
	//	Select move target position 
	// ----------------------------------------------
	Vector vMoveTargetPos(0, 0, 0);
	if (GetTarget())
	{
		vMoveTargetPos = GetTarget()->GetAbsOrigin();
	}
	else if (GetEnemy() != NULL)
	{
		vMoveTargetPos = GetEnemy()->GetAbsOrigin();
	}

	// -----------------------------------------
	//  See if we can fly there directly
	// -----------------------------------------
	if (pMoveTarget /*|| HaveInspectTarget()*/)
	{
		trace_t tr;

		if (pMoveTarget)
		{
			UTIL_TraceEntity(this, GetAbsOrigin(), vMoveTargetPos,
				MASK_NPCSOLID_BRUSHONLY, pMoveTarget, GetCollisionGroup(), &tr);
		}
		else
		{
			UTIL_TraceEntity(this, GetAbsOrigin(), vMoveTargetPos, MASK_NPCSOLID_BRUSHONLY, &tr);
		}
		/*
		float fTargetDist = (1-tr.fraction)*(GetAbsOrigin() - vMoveTargetPos).Length();
		if (fTargetDist > 50)
		{
		//SetCondition( COND_SCANNER_FLY_BLOCKED );
		}
		else
		{
		//SetCondition( COND_SCANNER_FLY_CLEAR );
		}
		*/
	}

	

	//int iFlyz = FindGestureLayer(ACT_FLYZ);

	//// -----------------------------------------------------------------
	//// If I have a route, keep it updated and move toward target
	//// ------------------------------------------------------------------
	//if (GetNavigator()->IsGoalActive())
	//{
	//	if (!OverridePathMove(flInterval))
	//	{
	//		
	//		float flZVel = GetAbsVelocity().z;
	//		if (flZVel < 0)
	//			flZVel *= -1;
	//		float flLayerPct = RemapValClamped(flZVel, 0, 70, 0, 1);
	//		SetLayerWeight(iFlyz, flLayerPct);

	//		//return true;
	//	}
	//}
	//else
	//{
	//	//do nothing
	//	Stop();
	//	SetLayerWeight(iFlyz, 0);
	//	//TaskComplete();
	//}

	//SetCurrentVelocity(GetAbsVelocity());

	return true;
}

void CBMSNPC_AlienController::MoveToTarget(float flInterval, const Vector &vecMoveTarget)
{
	const float	myAccel = 300.0;
	const float	myDecay = 9.0;

	//TurnHeadToTarget( flInterval, MoveTarget );
	MoveToLocation(flInterval, vecMoveTarget, myAccel, (2 * myAccel), myDecay);
}

AI_BEGIN_CUSTOM_NPC(npc_alien_controller, CBMSNPC_AlienController);

DECLARE_ACTIVITY(ACT_CONTROLLER_ATTACK_HANDS)
DECLARE_ACTIVITY(ACT_CONTROLLER_ATTACK_LIFT)
DECLARE_ACTIVITY(ACT_CONTROLLER_ATTACK_THROW)

DECLARE_ANIMEVENT(AE_CONTROLLER_GLOW_START)
DECLARE_ANIMEVENT(AE_CONTROLLER_GLOW_STOP)
DECLARE_ANIMEVENT(AE_CONTROLLER_LIFT_OBJ)
DECLARE_ANIMEVENT(AE_CONTROLLER_THROW_OBJ)

AI_END_CUSTOM_NPC();