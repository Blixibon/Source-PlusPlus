//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Michael S. Booth (linkedin.com/in/michaelbooth), 2003
// Iv�n Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "in\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

//#define Msg(...) Log_Msg(LOG_BOTS, __VA_ARGS__)
//#define Warning(...) Log_Warning(LOG_BOTS, __VA_ARGS__)

//================================================================================
// Macros
//================================================================================

// Time in seconds between each random "look at".
#define LOOK_RANDOM_INTERVAL ( IsAlerted() ) ? RandomInt( 3, 10 ) : RandomInt( 6, 20 )

// Time in seconds between each look at interesting places.
#define LOOK_INTERESTING_INTERVAL ( IsAlerted() ) ? RandomInt( 2, 5 )  : RandomInt( 6, 15 )

//================================================================================
// Run the expensive Update
//================================================================================
void CBotVision::Update()
{
	VPROF_BUDGET("CBotVision::Update", VPROF_BUDGETGROUP_BOTS);

	LookNavigation(); // PRIORITY_LOW

	LookAround();

	LookAtThreat(); // PRIORITY_HIGH
}

//================================================================================
// Cheap Upkeep
//================================================================================
void CBotVision::Upkeep()
{
	VPROF_BUDGET("CBotVision::Upkeep", VPROF_BUDGETGROUP_BOTS);

	Process();
}

//================================================================================
// Process the aiming system
// Author: Michael S. Booth (linkedin.com/in/michaelbooth), 2003
//================================================================================
void CBotVision::Process()
{
	VPROF_BUDGET("CBotVision::Process", VPROF_BUDGETGROUP_BOTS);

	if (!HasAimGoal())
		return;

	// We have finished aiming our target
	if (IsVisionTimeExpired()) {
		Reset();
		return;
	}

	float deltaT = m_flTickInterval;

	// TODO: Find better values?
	int speed = GetAimSpeed();
	float maxAccel = 3000.0f;
	float stiffness = GetStiffness();
	float damping = 25.0f;

	if (!IsIdle()) {
		damping = 30.0f;
	}

	QAngle viewAngles = GetHost()->EyeAngles();
	Vector lookPosition = m_vecLookGoal - GetHost()->EyePosition();

	QAngle lookAngle;
	VectorAngles(lookPosition, lookAngle);

	float lookYaw = lookAngle.y;
	float lookPitch = lookAngle.x;

	// If we are to this tolerance of being able to aim at our target we do it in an instant way.
	const float onTargetTolerance = 1.0f;

	//
	// Yaw
	//
	float angleDiffYaw = AngleNormalize(lookYaw - viewAngles.y);

	if (speed == AIM_SPEED_INSTANT || angleDiffYaw < onTargetTolerance && angleDiffYaw > -onTargetTolerance) {
		m_flLookYawVel = 0.0f;
		viewAngles.y = lookYaw;
	}
	else {
		// simple angular spring/damper
		float accel = stiffness * angleDiffYaw - damping * m_flLookYawVel;

		// limit rate
		if (accel > maxAccel)
			accel = maxAccel;
		else if (accel < -maxAccel)
			accel = -maxAccel;

		m_flLookYawVel += deltaT * accel;
		viewAngles.y += deltaT * m_flLookYawVel;
	}

	//
	// Pitch
	//
	float angleDiffPitch = lookPitch - viewAngles.x;
	angleDiffPitch = AngleNormalize(angleDiffPitch);

	/*if ( angleDiffPitch < onTargetTolerance && angleDiffPitch > -onTargetTolerance || speed == AIM_SPEED_INSTANT ) {
		m_flLookPitchVel = 0.0f;
		viewAngles.x = lookPitch;
	}
	else {*/
	// simple angular spring/damper
	// double the stiffness since pitch is only +/- 90 and yaw is +/- 180
	float accel = 2.0f * stiffness * angleDiffPitch - damping * m_flLookPitchVel;

	// limit rate
	if (accel > maxAccel)
		accel = maxAccel;
	else if (accel < -maxAccel)
		accel = -maxAccel;

	m_flLookPitchVel += deltaT * accel;
	viewAngles.x += deltaT * m_flLookPitchVel;
	//}

	// We are in tolerance
	if ((angleDiffYaw < m_flCosTolerance && angleDiffYaw > -m_flCosTolerance) && (angleDiffPitch < m_flCosTolerance && angleDiffPitch > -m_flCosTolerance)) {
		m_bAimReady = true;

		// We start the timer
		if (!m_VisionTimer.HasStarted()) {
			m_VisionTimer.Start(m_flDuration);
		}
	}
	else {
		m_bAimReady = false;
	}

	// limit range - avoid gimbal lock
	if (viewAngles.x < -89.0f) {
		viewAngles.x = -89.0f;
	}
	else if (viewAngles.x > 89.0f) {
		viewAngles.x = 89.0f;
	}

	GetHost()->SnapEyeAngles(viewAngles);
}

//================================================================================
// Sets [vecLookAt] as the ideal position to look at the specified entity
//================================================================================
void CBotVision::GetEntityBestAimPosition(CBaseEntity *pEntity, Vector &vecLookAt)
{
	vecLookAt.Invalidate();

	if (pEntity == NULL)
		return;

	CEntityMemory *memory = NULL;

	if (GetMemory()) {
		memory = GetMemory()->GetEntityMemory(pEntity);
	}

	if (memory) {
		// We have information about this entity in our memory!
		vecLookAt = memory->GetIdealPosition();
	}
	else if (pEntity->MyCombatCharacterPointer()) {
		// If it is a character, we try to aim to a hitbox
		Utils::GetHitboxPosition(pEntity, vecLookAt, GetProfile()->GetFavoriteHitbox());
	}
	else {
		vecLookAt = pEntity->WorldSpaceCenter();
	}

	float distance = vecLookAt.DistTo(GetHost()->GetAbsOrigin());

	// It is an enemy!
	if (GetDecision()->IsEnemy(pEntity) && distance > 100.0f) {
		// 
		if (GetProfile()->GetSkill() > SKILL_MEDIUM) {
			const float k = 3.0f;
			vecLookAt += k * m_flTickInterval * (pEntity->GetAbsVelocity() - GetHost()->GetAbsVelocity());
		}

		// We added a margin of error when aiming.
		if (GetProfile()->GetSkill() < SKILL_HARDEST) {
			float errorRange = 0.0f;

			if (GetProfile()->GetSkill() >= SKILL_VERY_HARD) {
				errorRange = RandomFloat(0.0f, 10.0f);
			}
			else if (GetProfile()->GetSkill() >= SKILL_HARD) {
				errorRange = RandomFloat(0.0f, 15.0f);
			}
			else if (GetProfile()->IsMedium()) {
				errorRange = RandomFloat(10.0f, 20.0f);
			}
			else {
				errorRange = RandomFloat(20.0f, 30.0f);
			}

			vecLookAt.x += RandomFloat(-errorRange, errorRange);
			vecLookAt.y += RandomFloat(-errorRange, errorRange);
			vecLookAt.z += RandomFloat(-errorRange, errorRange);
		}
	}
}

//================================================================================
// Returns ideal aiming speed
//================================================================================
int CBotVision::GetAimSpeed()
{
	int speed = GetProfile()->GetMinAimSpeed();

	if (speed == AIM_SPEED_INSTANT) {
		return AIM_SPEED_INSTANT;
	}

	// TODO: This should not be necessary.
	// (We need to predict where our target will be)
	if (GetHost()->IsMoving()) {
		++speed;
	}

	// Adrenaline?
	if (!GetProfile()->IsEasy()) {
		if (IsCombating()) {
			++speed;
		}

#ifdef INSOURCE_DLL
		if (GetHost()->IsUnderAttack()) {
			++speed;
		}
#endif
	}

	speed = clamp(speed, GetProfile()->GetMinAimSpeed(), GetProfile()->GetMaxAimSpeed());
	return speed;
}

//================================================================================
// TODO: Find better values?
//================================================================================
float CBotVision::GetStiffness()
{
	int speed = GetAimSpeed();

	switch (speed) {
		case AIM_SPEED_VERYLOW:
			return 120.0f;
			break;

		case AIM_SPEED_LOW:
			return 170.0f;
			break;

		case AIM_SPEED_NORMAL:
		default:
			return 220.0f;
			break;

		case AIM_SPEED_FAST:
			return 270.0f;
			break;

		case AIM_SPEED_VERYFAST:
			return 330.0f;
			break;

		case AIM_SPEED_INSTANT:
			return 999.0f;
			break;
	}
}

//================================================================================
// Look at the specified entity
//================================================================================
bool CBotVision::LookAt(const char *pDesc, CBaseEntity *pTarget, int priority, float duration, float cosTolerance)
{
	if (pTarget == NULL)
		return false;

	Vector vecLookAt;
	GetEntityBestAimPosition(pTarget, vecLookAt);

	return LookAt(pDesc, pTarget, vecLookAt, priority, duration, cosTolerance);
}

//================================================================================
// Look at the specified location stating that it is an entity
//================================================================================
bool CBotVision::LookAt(const char *pDesc, CBaseEntity *pTarget, const Vector &goal, int priority, float duration, float cosTolerance)
{
	if (pTarget == NULL)
		return false;

	bool success = LookAt(pDesc, goal, priority, duration, cosTolerance);

	if (success) {
		m_pLookingAt = pTarget;
	}

	return success;
}

//================================================================================
// Look at the specified location
//================================================================================
bool CBotVision::LookAt(const char *pDesc, const Vector &vecGoal, int priority, float duration, float cosTolerance)
{
	if (!vecGoal.IsValid())
		return false;

	if (m_vecLookGoal == vecGoal)
		return false;

	if (GetPriority() > priority)
		return false;

	duration = MAX(duration, 0.1f);
	cosTolerance = MAX(cosTolerance, 0.5f);

	m_vecLookGoal = vecGoal;
	m_pLookingAt = NULL;
	m_pDescription = pDesc;
	m_bAimReady = false;
	m_flDuration = duration;
	m_flCosTolerance = cosTolerance;
	m_VisionTimer.Invalidate();

	SetPriority(priority);
	return true;
}

//================================================================================
//================================================================================
void CBotVision::LookAtThreat()
{
	if (!GetMemory())
		return;

	CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

	if (memory == NULL)
		return;

	if (memory->IsLost())
		return;

	if (!GetDecision()->ShouldLookThreat())
		return;

	LookAt("Threat", memory->GetEntity(), PRIORITY_HIGH, 0.2f);
}

//================================================================================
// Look to the next position on the route
//================================================================================
void CBotVision::LookNavigation()
{
	if (!GetLocomotion())
		return;

	if (!GetLocomotion()->HasDestination())
		return;

	Vector lookAt = GetLocomotion()->GetNextSpot();
	lookAt.z = GetHost()->EyePosition().z;

	int priority = PRIORITY_LOW;

	if (GetFollow() && GetFollow()->IsFollowingActive()) {
		priority = PRIORITY_NORMAL;
	}

	LookAt("Looking Forward", lookAt, priority, 0.5f);
}

//================================================================================
// It allows us to look at an interesting or random place.
//================================================================================
void CBotVision::LookAround()
{
	if (GetMemory()) {
		int blocked = GetDataMemoryInt("BlockLookAround");

		if (blocked == 1)
			return;
	}

	// We heard a sound that represents danger
	if (HasCondition(BCOND_HEAR_COMBAT) || HasCondition(BCOND_HEAR_DANGER) || HasCondition(BCOND_HEAR_ENEMY)) {
		if (GetDecision()->ShouldLookDangerSpot()) {
			LookDanger(); // PRIORITY_HIGH
			return;
		}
	}

	if (GetPriority() > PRIORITY_NORMAL)
		return;

	// An interesting place:
	// Places where enemies can be covered or revealed.
	if (GetDecision()->ShouldLookInterestingSpot()) {
		LookInterestingSpot(); // PRIORITY_NORMAL
		return;
	}

	// Random place
	if (GetDecision()->ShouldLookRandomSpot()) {
		LookRandomSpot(); // PRIORITY_VERY_LOW
		return;
	}

	// We are in a squadron, look at a friend :)
	if (GetDecision()->ShouldLookSquadMember()) {
		LookSquadMember(); // PRIORITY_VERY_LOW
		return;
	}
}


//================================================================================
// Find an interesting place and look at that place
//================================================================================
void CBotVision::LookInterestingSpot()
{
	CBotDecision *pDecision = dynamic_cast<CBotDecision *>(GetBot()->GetDecision());
	Assert(pDecision);

	pDecision->m_IntestingAimTimer.Start(LOOK_INTERESTING_INTERVAL);

	CSpotCriteria criteria;
	criteria.SetMaxRange(1000.0f);
	criteria.SetPlayer(GetHost());
	criteria.SetTacticalMode(GetBot()->GetTacticalMode());
	criteria.SetFlags(FLAG_IGNORE_RESERVED | FLAG_INTERESTING_SPOT);

	if (!GetDecision()->CanLookNoVisibleSpots()) {
		criteria.SetFlags(FLAG_ONLY_VISIBLE);
	}

	Vector vecSpot;

	if (!Utils::GetSpotCriteria(&vecSpot, criteria))
		return;

	float duration = 2.0f;

	// We are moving, we look fast
	if (GetLocomotion() && GetLocomotion()->HasDestination()) {
		duration = 0.2f;
	}

	LookAt("Intesting Spot", vecSpot, PRIORITY_NORMAL, duration, 3.0f);
}

//================================================================================
// Look at random spot
//================================================================================
void CBotVision::LookRandomSpot()
{
	CBotDecision *pDecision = dynamic_cast<CBotDecision *>(GetBot()->GetDecision());
	Assert(pDecision);

	pDecision->m_RandomAimTimer.Start(LOOK_RANDOM_INTERVAL);

	QAngle viewAngles = GetBot()->GetUserCommand()->viewangles;
	viewAngles.x += RandomInt(-10, 10);
	viewAngles.y += RandomInt(-40, 40);

	Vector vecForward;
	AngleVectors(viewAngles, &vecForward);

	Vector vecPosition = GetHost()->EyePosition();
	LookAt("Random Spot", vecPosition + 30 * vecForward, PRIORITY_VERY_LOW, 3.0f);
}

//================================================================================
// We look at a member of our squad
//================================================================================
void CBotVision::LookSquadMember()
{
	CBasePlayer *pMember = GetHost()->GetSquad()->GetRandomMember();

	if (!pMember || pMember == GetHost()) {
		return;
	}

	LookAt("Squad Member", pMember, PRIORITY_VERY_LOW, RandomFloat(1.0f, 3.5f), 3.0f);
}

//================================================================================
// Look to a place where we heard danger/combat
//================================================================================
void CBotVision::LookDanger()
{
	CSound *pSound = GetHost()->GetBestSound(SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER);

	if (!pSound)
		return;

	Vector vecLookAt = pSound->GetSoundReactOrigin();
	vecLookAt.z = GetHost()->EyePosition().z;

	LookAt("Danger Sound", vecLookAt, PRIORITY_HIGH, RandomFloat(1.0f, 3.5f), 3.0f);
}