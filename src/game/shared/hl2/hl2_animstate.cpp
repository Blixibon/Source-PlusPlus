#include "cbase.h"
#include "hl2_animstate.h"
#include "hl2_player_shared.h"

#ifdef CLIENT_DLL
extern ConVar anim_showmainactivity;
#endif

#define ACT_DOD_FIRST ACT_DOD_DEPLOYED
#define ACT_DOD_LAST ACT_DOD_DEFUSE_TNT
#define ACT_BMMP_FIRST ACT_BMMP_IDLE
#define ACT_BMMP_LAST _ACT_BMMP_LAST

acttable_t CHL2PlayerAnimState::s_acttableMPToHL2MP[] = {
	// Sequences
	{ACT_MP_STAND_IDLE, ACT_HL2MP_IDLE, true},
	{ACT_MP_CROUCH_IDLE, ACT_HL2MP_IDLE_CROUCH, true},
	{ACT_MP_RUN, ACT_HL2MP_RUN, true},
	{ACT_MP_WALK, ACT_HL2MP_WALK, true},
	{ACT_MP_AIRWALK, ACT_HL2MP_WALK, true},
	{ACT_MP_CROUCHWALK, ACT_HL2MP_WALK_CROUCH, true},
	{ACT_MP_SPRINT, ACT_HL2MP_RUN, true},
	{ACT_MP_JUMP, ACT_HL2MP_JUMP, true},
	{ACT_MP_SWIM, ACT_HL2MP_SWIM, true},
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

MultiPlayerMovementData_t CHL2PlayerAnimState::s_MoveParams = { 150.f, 190.f, 320.f, 360.f };

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
		actTranslated = pWeapon->ActivityOverride(actTranslated, false);

		// Live TF2 does this but is doing this after the above call correct?
		actTranslated = pWeapon->GetItem()->GetActivityOverride(GetBasePlayer()->GetTeamNumber(), actTranslated);
	}
	else if (actTranslated == ACT_HL2MP_JUMP)
		actTranslated = ACT_BMMP_JUMP_START;

	return actTranslated;
}

AimType_e CHL2PlayerAnimState::GetAimType()
{
	Activity actCurrent = TranslateActivity(m_eCurrentMainSequenceActivity);
	if (actCurrent >= ACT_DOD_FIRST && actCurrent <= ACT_DOD_LAST)
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
	/*
	if ( ( m_PoseParameterData.m_iMoveYaw < 0 ) || ( m_PoseParameterData.m_iMoveScale < 0 ) )
		return false;
	*/

	return true;
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
		return false;
	}

	//airwalk more like hl2mp, we airwalk until we have 0 velocity, then it's the jump animation
	//underwater we're alright we airwalking
	/*if (!m_bJumping && !(GetBasePlayer()->GetFlags() & FL_ONGROUND) && GetBasePlayer()->GetWaterLevel() <= WL_NotInWater)
	{
		if (!m_fGroundTime)
		{
			m_fGroundTime = gpGlobals->curtime;
		}
		else if ((gpGlobals->curtime - m_fGroundTime) > 0 && GetOuterXYSpeed() < 0.5f)
		{
			m_bJumping = true;
			m_bFirstJumpFrame = false;
			m_flJumpStartTime = 0;
		}
	}*/

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
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if (gpGlobals->curtime - m_flJumpStartTime > 0.2f)
		{
			if (GetBasePlayer()->GetFlags() & FL_ONGROUND)
			{
				m_bJumping = false;
				RestartMainSequence();
			}
		}
	}
	if (m_bJumping)
	{
		idealActivity = ACT_MP_JUMP;
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CHL2PlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_MP_STAND_IDLE;

	if (HandleVaulting(idealActivity) ||
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