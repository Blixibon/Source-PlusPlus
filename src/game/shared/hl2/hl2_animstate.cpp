#include "cbase.h"
#include "hl2_animstate.h"
#include "hl2_player_shared.h"

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
		actTranslated = GetBasePlayer()->GetActiveWeapon()->ActivityOverride(actTranslated, false);

		// Live TF2 does this but is doing this after the above call correct?
		actTranslated = pWeapon->GetItem()->GetActivityOverride(GetBasePlayer()->GetTeamNumber(), actTranslated);
	}
	else if (actTranslated == ACT_HL2MP_JUMP)
		actTranslated = ACT_HL2MP_JUMP_SLAM;

	return actTranslated;
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

	m_PoseParameterData.m_bHL2Aim = true;

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

	m_PoseParameterData.m_iMoveYaw = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "move_yaw");
	m_PoseParameterData.m_iMoveScale = GetBasePlayer()->LookupPoseParameter(pStudioHdr, "move_scale");
	/*
	if ( ( m_PoseParameterData.m_iMoveYaw < 0 ) || ( m_PoseParameterData.m_iMoveScale < 0 ) )
		return false;
	*/

	return true;
}

CMultiPlayerAnimState *CHL2_Player::GetAnimState()
{
	return &m_AnimState;
}