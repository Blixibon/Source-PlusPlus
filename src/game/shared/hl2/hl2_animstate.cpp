#include "hl2_animstate.h"
#include "cbase.h"
#include "hl2_animstate.h"
#include "hl2_player_shared.h"
#ifdef CLIENT_DLL
#include "iclientvehicle.h"
#include "c_vehicle_jeep.h"
#include "c_vehicle_airboat.h"
#else
#include "iservervehicle.h"
#include "vehicle_jeep.h"
#endif

#ifdef CLIENT_DLL
extern ConVar anim_showmainactivity;
#endif

extern ConVar mp_slammoveyaw;

float SnapYawTo(float flValue);

#define MOVING_MINIMUM_SPEED	0.5f

#define ACT_DOD_FIRST ACT_DOD_DEPLOYED
#define ACT_DOD_LAST ACT_DOD_DEFUSE_TNT
#define ACT_BMMP_FIRST ACT_BMMP_IDLE
#define ACT_BMMP_LAST _ACT_BMMP_LAST
#define ACT_MP_FIRST ACT_MP_STAND_IDLE
#define ACT_MP_LAST ACT_MP_PASSTIME_THROW_CANCEL

acttable_t CHL2PlayerAnimState::s_acttableMPToHL2MP[] = {
	// Sequences
	{ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE, true},
	{ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH, true},
	{ACT_MP_RUN, ACT_HL2MP_RUN, true},
	{ACT_MP_WALK, ACT_HL2MP_WALK, true},
	{ACT_MP_AIRWALK, ACT_HL2MP_WALK, true},
	{ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH, true},
	{ACT_MP_SPRINT, ACT_HL2MP_RUN_FAST, true},
	{ACT_MP_JUMP, ACT_HL2MP_JUMP, true},
	{ACT_MP_SWIM, ACT_HL2MP_SWIM, true},
	{ACT_MP_DOUBLEJUMP, ACT_AHL_DUCKJUMP, true},
	// Gestures
	{ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK, false},
	{ACT_MP_ATTACK_STAND_GRENADE, ACT_GMOD_GESTURE_ITEM_THROW, false},
	{ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK, false},
	{ACT_MP_ATTACK_CROUCH_GRENADE, ACT_GMOD_GESTURE_ITEM_THROW, false},
	{ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK, false},
	{ACT_MP_ATTACK_SWIM_GRENADE, ACT_GMOD_GESTURE_ITEM_THROW, false},
	{ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE, ACT_HL2MP_GESTURE_RANGE_ATTACK, false},
	{ACT_MP_ATTACK_AIRWALK_GRENADE, ACT_GMOD_GESTURE_ITEM_THROW, false},
	{ACT_MP_RELOAD_STAND, ACT_HL2MP_GESTURE_RELOAD, false},
	{ACT_MP_RELOAD_CROUCH, ACT_HL2MP_GESTURE_RELOAD, false},
	{ACT_MP_RELOAD_SWIM, ACT_HL2MP_GESTURE_RELOAD, false},
	{ACT_MP_RELOAD_AIRWALK, ACT_HL2MP_GESTURE_RELOAD, false},
	// Flinches
	{ACT_MP_GESTURE_FLINCH_CHEST, ACT_GESTURE_FLINCH_CHEST, false},
	{ACT_MP_GESTURE_FLINCH_HEAD, ACT_GESTURE_FLINCH_HEAD, false},
	{ACT_MP_GESTURE_FLINCH_LEFTARM, ACT_GESTURE_FLINCH_LEFTARM, false},
	{ACT_MP_GESTURE_FLINCH_RIGHTARM, ACT_GESTURE_FLINCH_RIGHTARM, false},
	{ACT_MP_GESTURE_FLINCH_LEFTLEG, ACT_GESTURE_FLINCH_LEFTLEG, false},
	{ACT_MP_GESTURE_FLINCH_RIGHTLEG, ACT_GESTURE_FLINCH_RIGHTLEG, false},
};

//MultiPlayerMovementData_t mv;
//mv.m_flBodyYawRate = 360;
//mv.m_flRunSpeed = 190;
//mv.m_flWalkSpeed = 150;
//mv.m_flSprintSpeed = 320;
//m_PlayerAnimState = new CHL2PlayerAnimState(this, mv);

MultiPlayerMovementData_t CHL2PlayerAnimState::s_MoveParams = { 80.f, 200.f, 330.f, 360.f };

Activity CHL2PlayerAnimState::TranslateActivity(Activity actDesired)
{
	Activity actTranslated = BaseClass::TranslateActivity(actDesired);

	for (int i = 0; i < ARRAYSIZE(s_acttableMPToHL2MP); i++)
	{
		const acttable_t& act = s_acttableMPToHL2MP[i];
		if (actDesired == act.baseAct)
		{
			actTranslated = (Activity)act.weaponAct;
			break;
		}
	}

	CBaseCombatWeapon* pWeapon = GetBasePlayer()->GetActiveWeapon();
	if (pWeapon)
	{
		actTranslated = pWeapon->GetItem()->GetActivityOverride(GetBasePlayer()->GetTeamNumber(), actTranslated);

		actTranslated = pWeapon->ActivityOverride(actTranslated, false);

		actTranslated = pWeapon->GetItem()->GetActivityOverride(GetBasePlayer()->GetTeamNumber(), actTranslated);
	}
	
	if (actTranslated == ACT_HL2MP_JUMP)
		actTranslated = ACT_BMMP_JUMP_START;

	return actTranslated;
}

AimType_e CHL2PlayerAnimState::GetAimType()
{
	Activity actCurrent = TranslateActivity(m_eCurrentMainSequenceActivity);
	if (actCurrent >= ACT_DOD_FIRST && actCurrent <= ACT_DOD_LAST)
		return AIM_MP;
	if (actCurrent >= ACT_MP_FIRST && actCurrent <= ACT_MP_LAST)
		return AIM_MP;
	if (actCurrent >= ACT_BMMP_FIRST && actCurrent <= ACT_BMMP_LAST)
		return AIM_BMMP;

	return AIM_HL2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2PlayerAnimState::SetupPoseParameters(CStudioHdr* pStudioHdr)
{
	// Check to see if this has already been done.
	if (m_bPoseParameterInit)
		return true;

	// Save off the pose parameter indices.
	if (!pStudioHdr)
		return false;

	MDLCACHE_CRITICAL_SECTION();
	for (int i = 0; i < pStudioHdr->GetNumPoseParameters(); i++)
	{
		GetBasePlayer()->SetPoseParameter(i, 0.0);
	}

	//m_PoseParameterData.m_bHL2Aim = true;

	// Look for the movement blenders.
	m_PoseParameterData.m_iMoveX = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "move_x");
	m_PoseParameterData.m_iMoveY = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "move_y");

	if ((m_PoseParameterData.m_iMoveX < 0) || (m_PoseParameterData.m_iMoveY < 0))
		return false;


	// Look for the aim pitch blender.
	m_PoseParameterData.m_iAimPitch = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "aim_pitch");

	if (m_PoseParameterData.m_iAimPitch < 0)
		return false;


	// Look for aim yaw blender.
	m_PoseParameterData.m_iAimYaw = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "aim_yaw");

	if (m_PoseParameterData.m_iAimYaw < 0)
		return false;

	// The rest are not-critical
	m_bPoseParameterInit = true;

	// Look for the aim pitch blender.
	m_PoseParameterData.m_iBodyPitch = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "body_pitch");

	// Look for aim yaw blender.
	m_PoseParameterData.m_iBodyYaw = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "body_yaw");

	m_PoseParameterData.m_iMoveYaw = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "move_yaw");
	m_PoseParameterData.m_iMoveScale = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "move_scale");

	m_PoseParameterData.m_iVerticalVelocity = GetBasePlayer()->LookupPoseParameter("vertical_velocity");
	m_PoseParameterData.m_iVehicleSteer = GetBasePlayer()->LookupPoseParameter("vehicle_steer");

#ifdef CLIENT_DLL
	m_headYawPoseParam = GetBasePlayer()->LookupPoseParameter( "head_yaw" );
	GetBasePlayer()->GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = GetBasePlayer()->LookupPoseParameter( "head_pitch" );
	GetBasePlayer()->GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );

	m_chestAttachment = GetBasePlayer()->LookupAttachment("chest");
#endif

	return true;
}

void CHL2PlayerAnimState::ComputePoseParam_AimYaw(CStudioHdr* pStudioHdr)
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity(vecVelocity);
	//vecVelocity += GetBasePlayer()->GetBaseVelocity();

	// Check to see if we are moving.
	bool bMoving = (vecVelocity.Length() > 1.0f) ? true : false;

	if (GetBasePlayer()->IsInAVehicle())
	{
		Vector vecEyes;
		QAngle angEyes = vec3_angle;
		CBaseAnimating* pVehicleAnim = dynamic_cast<CBaseAnimating*> (GetBasePlayer()->GetVehicle()->GetVehicleEnt());
		IVehicle* pVehicle = GetBasePlayer()->GetVehicle();

		if (pVehicle && pVehicleAnim)
		{
			int nRole = pVehicle->GetPassengerRole(GetBasePlayer());
			char pAttachmentName[32];
			Q_snprintf(pAttachmentName, sizeof(pAttachmentName), "vehicle_feet_passenger%d", nRole);
			int nFeetAttachmentIndex = pVehicleAnim->LookupAttachment(pAttachmentName);
			if (nRole == VEHICLE_ROLE_DRIVER && nFeetAttachmentIndex <= 0)
				nFeetAttachmentIndex = pVehicleAnim->LookupAttachment("vehicle_driver_feet");

			if (nFeetAttachmentIndex > 0)
				pVehicleAnim->GetAttachment(nFeetAttachmentIndex, vecEyes, angEyes);
			else
				angEyes = pVehicleAnim->GetAbsAngles();

#ifdef CLIENT_DLL
			int iVehicleSteer = pVehicleAnim->LookupPoseParameter("vehicle_steer");

			Vector vecVelocity = pVehicleAnim->GetAbsVelocity();
			Vector vecUp;
			QAngle angVehicle = pVehicleAnim->GetAbsAngles();
			AngleVectors(angVehicle, NULL, NULL, &vecUp);
			float dp = vecUp.Dot(Vector(0, 0, 1));
			float dp2 = vecUp.Dot(vecVelocity);
			float flVert = ((dp < 0 ? dp : 0) + dp2 * 0.005);
			GetBasePlayer()->SetPoseParameter(m_PoseParameterData.m_iVerticalVelocity, flVert);

			if (iVehicleSteer > -1)
			{
				float flSteer = pVehicleAnim->GetPoseParameter(iVehicleSteer);
				//float flVMin, flVMax, flPMin, flPMax;
				//pVehicleAnim->GetPoseParameterRange(iVehicleSteer, flVMin, flVMax);
				//GetBasePlayer()->GetPoseParameterRange(m_PoseParameterData.m_iVehicleSteer, flPMin, flPMax);

				//flSteer = RemapValClamped(flSteer, flVMin, flVMax, flPMin, flPMax);
				flSteer = (flSteer * 2) - 1;
				GetBasePlayer()->SetPoseParameter(m_PoseParameterData.m_iVehicleSteer, flSteer);
			}
#endif
		}

		QAngle absangles = angEyes;
		absangles.y = AngleNormalize(absangles.y);
		m_angRender = absangles;

		m_flCurrentFeetYaw = m_flGoalFeetYaw = absangles.y;
	}
	else if (GetBasePlayer()->GetMoveType() == MOVETYPE_LADDER)
	{
		QAngle absangles;
		Vector vecUp(0, 0, 1);

		VectorAngles(-GetBasePlayer()->GetLadderNormal(), vecUp, absangles);
		absangles.y = AngleNormalize(absangles.y);

		m_flGoalFeetYaw = absangles.y;
	}
	// If we are moving or are prone and undeployed.
	// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
	// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
	// and the fact that m_flEyeYaw is never propogated from the server to the client.
	// TODO: Fix this after Halloween 2014.
	else if (bMoving || m_bForceAimYaw)
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if (m_PoseParameterData.m_flLastAimTurnTime <= 0.0f)
		{
			m_flGoalFeetYaw = m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(m_flGoalFeetYaw - m_flEyeYaw);

			if (fabsf(flYawDelta) > 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/)
			{
				float flSide = (flYawDelta > 0.0f) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += (45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ * flSide);
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize(m_flGoalFeetYaw);
	if (m_flGoalFeetYaw != m_flCurrentFeetYaw)
	{
		// If you are forcing aim yaw, your code is almost definitely broken if you don't include a delay between 
		// teleporting and forcing yaw. This is due to an unfortunate interaction between the command lookback window,
		// and the fact that m_flEyeYaw is never propogated from the server to the client.
		// TODO: Fix this after Halloween 2014.
		if (m_bForceAimYaw)
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles(m_flGoalFeetYaw, /*DOD_BODYYAW_RATE*/720.0f, gpGlobals->frametime, m_flCurrentFeetYaw);
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	if (!GetBasePlayer()->IsInAVehicle())
	{
		m_angRender[PITCH] = 0.f;
		m_angRender[YAW] = m_flCurrentFeetYaw;
		m_angRender[ROLL] = 0.f;

		GetBasePlayer()->SetPoseParameter(m_PoseParameterData.m_iVerticalVelocity, 0);
		GetBasePlayer()->SetPoseParameter(m_PoseParameterData.m_iVehicleSteer, 0);
	}

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize(flAimYaw);

	// Set the aim yaw and save.
	switch (GetAimType())
	{
	case AIM_MP:
	default:
		GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iBodyYaw, -flAimYaw);
		GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iAimYaw, 0.0f);
		break;
	case AIM_HL2:
	case AIM_BMMP:
		GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw);
		GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iBodyYaw, 0.0f);
		break;
	}
	m_DebugAnimData.m_flAimYaw = flAimYaw;

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	QAngle angle = GetBasePlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetBasePlayer()->SetAbsAngles(angle);
#else
	//QAngle bodyAngles = m_angRender;
	//
	//if (m_chestAttachment > 0)
	//{
	//	Vector vec;
	//	GetBasePlayer()->GetAttachment(m_chestAttachment, vec, bodyAngles);
	//}


	//float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	//m_flLastBodyYaw = bodyAngles[YAW];


	//// Set the head's yaw.
	//float desired = AngleNormalize(m_flEyeYaw - bodyAngles[YAW]);
	//desired = clamp(desired, m_headYawMin, m_headYawMax);
	//m_flCurrentHeadYaw = ApproachAngle(desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime);

	//// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	//m_flCurrentHeadYaw = AngleNormalize(m_flCurrentHeadYaw - flBodyYawDiff);
	//desired = clamp(desired, m_headYawMin, m_headYawMax);

	//GetBasePlayer()->SetPoseParameter(m_headYawPoseParam, m_flCurrentHeadYaw);
#endif
}

void CHL2PlayerAnimState::ComputePoseParam_AimPitch(CStudioHdr* pStudioHdr)
{
	BaseClass::ComputePoseParam_AimPitch(pStudioHdr);
#ifdef CLIENT_DLL
	// Set the head's yaw.
	float desired = m_flEyePitch;
	desired = clamp(desired, m_headPitchMin, m_headPitchMax);

	m_flCurrentHeadPitch = ApproachAngle(desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime);
	m_flCurrentHeadPitch = AngleNormalize(m_flCurrentHeadPitch);
	GetBasePlayer()->SetPoseParameter(m_headPitchPoseParam, m_flCurrentHeadPitch);
#endif
}

void CHL2PlayerAnimState::ComputePoseParam_MoveYaw(CStudioHdr* pStudioHdr)
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// Get the view yaw.
	float flAngle = AngleNormalize(m_flEyeYaw);

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize(-flYaw);

	// Get the current speed the character is running.
	bool bIsMoving;
	float flSpeed = CalcMovementSpeed(&bIsMoving);

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw(0.0f, 0.0f);
	if (GetBasePlayer()->GetMoveType() == MOVETYPE_LADDER)
	{
		Vector vecVelocity;
		GetOuterAbsVelocity(vecVelocity);

		GetMovementFlags(pStudioHdr);

		if (mp_slammoveyaw.GetBool())
		{
			flYaw = SnapYawTo(flYaw);
		}

		vecCurrentMoveYaw.x = vecVelocity.Normalized().z;
		vecCurrentMoveYaw.y = 0;

		if (m_LegAnimType == LEGANIM_9WAY)
		{
			// Set the 9-way blend movement pose parameters.
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x);
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y);
		}
		else
		{
			// find what speed was actually authored
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveYaw, 0.0f);
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveScale, 1.0f);
			float flMaxSpeed = GetBasePlayer()->GetSequenceGroundSpeed(GetBasePlayer()->GetSequence());

			// scale playback
			if (flMaxSpeed > vecCurrentMoveYaw.x)
			{
				GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveScale, vecCurrentMoveYaw.x / flMaxSpeed);
			}
		}
	}
	else if (bIsMoving)
	{
		GetMovementFlags(pStudioHdr);

		if (mp_slammoveyaw.GetBool())
		{
			flYaw = SnapYawTo(flYaw);
		}

		if (m_LegAnimType == LEGANIM_9WAY)
		{
			// convert YAW back into vector
			vecCurrentMoveYaw.x = cos(DEG2RAD(flYaw));
			vecCurrentMoveYaw.y = -sin(DEG2RAD(flYaw));
			// push edges out to -1 to 1 box
			float flInvScale = MAX(fabsf(vecCurrentMoveYaw.x), fabsf(vecCurrentMoveYaw.y));
			if (flInvScale != 0.0f)
			{
				vecCurrentMoveYaw.x /= flInvScale;
				vecCurrentMoveYaw.y /= flInvScale;
			}

			// find what speed was actually authored
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x);
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y);
			float flMaxSpeed = GetBasePlayer()->GetSequenceGroundSpeed(GetBasePlayer()->GetSequence());

			// scale playback
			if (flMaxSpeed > flSpeed)
			{
				vecCurrentMoveYaw.x *= flSpeed / flMaxSpeed;
				vecCurrentMoveYaw.y *= flSpeed / flMaxSpeed;
			}

			// Set the 9-way blend movement pose parameters.
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x);
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y);
		}
		else
		{
			// find what speed was actually authored
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveYaw, flYaw);
			GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveScale, 1.0f);
			float flMaxSpeed = GetBasePlayer()->GetSequenceGroundSpeed(GetBasePlayer()->GetSequence());

			// scale playback
			if (flMaxSpeed > flSpeed)
			{
				GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveScale, flSpeed / flMaxSpeed);
			}
		}
	}
	else
	{
		// Set the 9-way blend movement pose parameters.
		GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveX, 0.0f);
		GetBasePlayer()->SetPoseParameter(pStudioHdr, m_PoseParameterData.m_iMoveY, 0.0f);
	}

	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2PlayerAnimState::HandleJumping(Activity& idealActivity)
{
	if (GetBasePlayer()->GetMoveType() == MOVETYPE_NOCLIP)
	{
		m_bJumping = false;
		m_bLongJump = false;
		return false;
	}

	//airwalk more like hl2mp, we airwalk until we have 0 velocity, then it's the jump animation
	//underwater we're alright we airwalking
	if (!m_bJumping && !(GetBasePlayer()->GetFlags() & FL_ONGROUND) && GetBasePlayer()->GetWaterLevel() <= WL_NotInWater)
	{
		if (!m_fGroundTime)
		{
			m_fGroundTime = gpGlobals->curtime;
		}
		else if ((gpGlobals->curtime - m_fGroundTime) > 0 && GetOuterXYSpeed() < MOVING_MINIMUM_SPEED)
		{
			m_bJumping = true;
			m_bFirstJumpFrame = false;
			m_flJumpStartTime = 0;
		}
	}

	if (m_bJumping)
	{
		if (m_bFirstJumpFrame)
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Check to see if we hit water and stop jumping animation.
		if (GetBasePlayer()->GetWaterLevel() >= WL_Waist)
		{
			m_bJumping = false;
			m_bLongJump = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if (gpGlobals->curtime - m_flJumpStartTime > 0.2f)
		{
			if (GetBasePlayer()->GetFlags() & FL_ONGROUND)
			{
				m_bJumping = false;
				m_bLongJump = false;
				RestartMainSequence();
				AddToGestureSlot(GESTURE_SLOT_JUMP, ACT_LAND, true);
			}
		}
	}
	if (m_bJumping)
	{
		idealActivity = m_bLongJump ? ACT_MP_DOUBLEJUMP : ACT_MP_JUMP;
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2PlayerAnimState::HandleVaulting(Activity& idealActivity)
{
	if (GetOuterXYSpeed() < 1000.f)
		return false;
	if (GetBasePlayer()->GetFlags() & FL_ONGROUND)
		return false;

	idealActivity = ACT_MP_SWIM;

	return true;
}

bool CHL2PlayerAnimState::HandleDriving(Activity& idealActivity)
{
	CBasePlayer* pPlayer = GetBasePlayer();
	if (!pPlayer->IsInAVehicle())
		return false;

#ifdef CLIENT_DLL
	IClientVehicle* pVehicle = pPlayer->GetVehicle();
#else
	IServerVehicle* pVehicle = pPlayer->GetVehicle();
#endif

	Assert(pVehicle);

	int iRole = pVehicle->GetPassengerRole(pPlayer);
	if (iRole == VEHICLE_ROLE_DRIVER)
	{
		CBaseEntity* pEnt = pVehicle->GetVehicleEnt();
		Assert(pEnt);

#ifdef CLIENT_DLL
		if (dynamic_cast<C_PropAirboat*> (pEnt) != nullptr)
			idealActivity = ACT_DRIVE_AIRBOAT;
		else if (dynamic_cast<C_PropVehicleDriveable*> (pEnt) != nullptr)
			idealActivity = ACT_DRIVE_JEEP;
		else if (Q_strcmp(modelinfo->GetModelName(pEnt->GetModel()), "models/vehicles/prisoner_pod_inner.mdl") == 0)
			idealActivity = ACT_DRIVE_POD;
		else
			idealActivity = pVehicle->IsPassengerUsingStandardWeapons(iRole) ? ACT_HL2MP_SIT : ACT_GMOD_SIT_ROLLERCOASTER;
#else
		if (FClassnameIs(pEnt, "prop_vehicle_airboat"))
			idealActivity = ACT_DRIVE_AIRBOAT;
		else if (dynamic_cast<CPropVehicleDriveable*> (pEnt) != nullptr)
			idealActivity = ACT_DRIVE_JEEP;
		else if (Q_strcmp(STRING(pEnt->GetModelName()), "models/vehicles/prisoner_pod_inner.mdl") == 0)
			idealActivity = ACT_DRIVE_POD;
		else
			idealActivity = pVehicle->IsPassengerUsingStandardWeapons(iRole) ? ACT_HL2MP_SIT : ACT_GMOD_SIT_ROLLERCOASTER;
#endif
	}
	else if (iRole == VEHICLE_ROLE_GUNNER)
	{
		idealActivity = ACT_IDLE_MANNEDGUN;
	}
	else /*if (iRole == VEHICLE_ROLE_PASSENGER)*/
	{
		idealActivity = pVehicle->IsPassengerUsingStandardWeapons(iRole) ? ACT_HL2MP_SIT : ACT_GMOD_SIT_ROLLERCOASTER;
	}

	return true;
}

bool CHL2PlayerAnimState::HandleClimbing(Activity& idealActivity)
{
	if (GetBasePlayer()->GetMoveType() == MOVETYPE_LADDER)
	{
		idealActivity = ACT_BMMP_LADDER;
		return true;
	}

	return false;
}

bool CHL2PlayerAnimState::HandleMoving(Activity& idealActivity)
{
	// In TF we run all the time now.
	float flSpeed = GetOuterXYSpeed();

	if (flSpeed > MOVING_MINIMUM_SPEED)
	{
		if (static_cast<CHL2_Player *>(GetBasePlayer())->IsSprinting() && flSpeed > m_MovementData.m_flRunSpeed)
			idealActivity = ACT_MP_SPRINT;
		else if (flSpeed > m_MovementData.m_flWalkSpeed)
			idealActivity = ACT_MP_RUN;
		else
			idealActivity = ACT_MP_WALK;
	}

	return true;
}

bool CHL2PlayerAnimState::HandleDucking(Activity& idealActivity)
{
	if (m_bInSwim && ((GetBasePlayer()->GetFlags() & FL_ONGROUND) == 0))
		return false;

#ifdef FL_ANIMDUCKING
	if (GetBasePlayer()->GetFlags() & FL_ANIMDUCKING)
#else
	if (GetBasePlayer()->GetFlags() & FL_DUCKING)
#endif // FL_ANIMDUCKING
	{
		if (GetOuterXYSpeed() > MOVING_MINIMUM_SPEED)
		{
			idealActivity = ACT_MP_CROUCHWALK;
		}
		else
		{
			idealActivity = ACT_MP_CROUCH_IDLE;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CHL2PlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	if (HandleDriving(idealActivity) ||
		HandleClimbing(idealActivity) ||
		HandleVaulting(idealActivity) ||
		HandleJumping(idealActivity) ||
		HandleDucking(idealActivity) ||
		HandleSwimming(idealActivity) ||
		HandleDying(idealActivity))
	{
		// intentionally blank
	}
	else
	{
		HandleMoving(idealActivity);
	}

	ShowDebugInfo();

	// Client specific.
#ifdef CLIENT_DLL

	if (anim_showmainactivity.GetBool())
	{
		DebugShowActivity(idealActivity);
	}

#endif

	return idealActivity;
}

void CHL2PlayerAnimState::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	switch (event)
	{
	case PLAYERANIMEVENT_DOUBLEJUMP:
	{
		// Jump.
		m_bJumping = true;
		m_bFirstJumpFrame = true;
		m_flJumpStartTime = gpGlobals->curtime;
		m_bLongJump = true;

		RestartMainSequence();

		break;
	}
	default:
		BaseClass::DoAnimationEvent(event, nData);
		break;
	}
}

CMultiPlayerAnimState *CHL2_Player::GetAnimState()
{
	return &m_AnimState;
}