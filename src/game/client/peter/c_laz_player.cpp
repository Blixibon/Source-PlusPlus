#include "cbase.h"
#include "c_laz_player.h"
#include "choreoevent.h"
#include "cam_thirdperson.h"
#include "in_buttons.h"

static Vector TF_TAUNTCAM_HULL_MIN(-9.0f, -9.0f, -9.0f);
static Vector TF_TAUNTCAM_HULL_MAX(9.0f, 9.0f, 9.0f);

static ConVar tf_tauntcam_yaw("tf_tauntcam_yaw", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
static ConVar tf_tauntcam_pitch("tf_tauntcam_pitch", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
static ConVar tf_tauntcam_dist("tf_tauntcam_dist", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);

static ConVar cl_playermodel("cl_playermodel", "none", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Default Player Model");
static ConVar cl_defaultweapon("cl_defaultweapon", "weapon_physcannon", FCVAR_USERINFO | FCVAR_ARCHIVE, "Default Spawn Weapon");


IMPLEMENT_NETWORKCLASS_ALIASED(Laz_Player, DT_Laz_Player);

BEGIN_RECV_TABLE(C_Laz_Player, DT_Laz_Player)
RecvPropInt(RECVINFO(m_bHasLongJump)),
RecvPropInt(RECVINFO(m_iPlayerSoundType)),

RecvPropBool(RECVINFO(m_fIsWalking)),
END_RECV_TABLE();

BEGIN_PREDICTION_DATA(C_Laz_Player)
DEFINE_PRED_FIELD(m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA();

#define	HL2_WALK_SPEED 150
#define	HL2_NORM_SPEED 190
#define	HL2_SPRINT_SPEED 320

void C_Laz_Player::PreThink(void)
{
	BaseClass::PreThink();

	HandleSpeedChanges();

	if (m_HL2Local.m_flSuitPower <= 0.0f)
	{
		if (IsSprinting())
		{
			StopSprinting();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_Laz_Player::StartSceneEvent(CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget)
{
	switch (event->GetType())
	{
	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::GESTURE:
		return StartGestureSceneEvent(info, scene, event, actor, pTarget);
	default:
		return BaseClass::StartSceneEvent(info, scene, event, actor, pTarget);
	}
}

void C_Laz_Player::CalcView(Vector & eyeOrigin, QAngle & eyeAngles, float & zNear, float & zFar, float & fov)
{
	HandleTaunting();
	BaseClass::CalcView(eyeOrigin, eyeAngles, zNear, zFar, fov);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_Laz_Player::StartGestureSceneEvent(CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget)
{
	// Get the (gesture) sequence.
	info->m_nSequence = LookupSequence(event->GetParameters());
	if (info->m_nSequence < 0)
		return false;

	// Player the (gesture) sequence.
	if (GetAnimState())
		GetAnimState()->AddVCDSequenceToGestureSlot(GESTURE_SLOT_VCD, info->m_nSequence);

	return true;
}

void C_Laz_Player::TurnOnTauntCam(void)
{
	if (!IsLocalPlayer())
		return;

	// Already in third person?
	if (g_ThirdPersonManager.WantToUseGameThirdPerson())
		return;

	// Save the old view angles.
	/*engine->GetViewAngles( m_angTauntEngViewAngles );
	prediction->GetViewAngles( m_angTauntPredViewAngles );*/

	m_TauntCameraData.m_flPitch = tf_tauntcam_pitch.GetFloat();
	m_TauntCameraData.m_flYaw = tf_tauntcam_yaw.GetFloat();
	m_TauntCameraData.m_flDist = tf_tauntcam_dist.GetFloat();
	m_TauntCameraData.m_flLag = 4.0f;
	m_TauntCameraData.m_vecHullMin = TF_TAUNTCAM_HULL_MIN;
	m_TauntCameraData.m_vecHullMax = TF_TAUNTCAM_HULL_MAX;

	QAngle vecCameraOffset(tf_tauntcam_pitch.GetFloat(), tf_tauntcam_yaw.GetFloat(), tf_tauntcam_dist.GetFloat());

	g_ThirdPersonManager.SetDesiredCameraOffset(Vector(tf_tauntcam_dist.GetFloat(), 0.0f, 0.0f));
	g_ThirdPersonManager.SetOverridingThirdPerson(true);
	::input->CAM_ToThirdPerson();
	ThirdPersonSwitch(true);

	::input->CAM_SetCameraThirdData(&m_TauntCameraData, vecCameraOffset);

}

void C_Laz_Player::TurnOffTauntCam(void)
{
	if (!IsLocalPlayer())
		return;

	/*Vector vecOffset = g_ThirdPersonManager.GetCameraOffsetAngles();

	tf_tauntcam_pitch.SetValue( vecOffset[PITCH] - m_angTauntPredViewAngles[PITCH] );
	tf_tauntcam_yaw.SetValue( vecOffset[YAW] - m_angTauntPredViewAngles[YAW] );*/

	g_ThirdPersonManager.SetOverridingThirdPerson(false);
	::input->CAM_SetCameraThirdData(NULL, vec3_angle);

	if (g_ThirdPersonManager.WantToUseGameThirdPerson())
	{
		ThirdPersonSwitch(true);
		return;
	}

	::input->CAM_ToFirstPerson();
	ThirdPersonSwitch(false);

	// Reset the old view angles.
	/*engine->SetViewAngles( m_angTauntEngViewAngles );
	prediction->SetViewAngles( m_angTauntPredViewAngles );*/

	// Force the feet to line up with the view direction post taunt.
	if (GetAnimState())
		GetAnimState()->m_bForceAimYaw = true;

	if (GetViewModel())
	{
		GetViewModel()->UpdateVisibility();
	}
}

void C_Laz_Player::HandleTaunting(void)
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	bool bUseTauntCam = false;

	if (GetAnimState() && GetAnimState()->IsPlayingCustomSequence())
		bUseTauntCam = true;

	// Clear the taunt slot.
	if (!m_bWasTaunting && (bUseTauntCam))
	{
		m_bWasTaunting = true;

		// Handle the camera for the local player.
		if (pLocalPlayer)
		{
			TurnOnTauntCam();
		}
	}

	if (m_bWasTaunting && (!bUseTauntCam))
	{
		m_bWasTaunting = false;

		// Clear the vcd slot.
		//m_PlayerAnimState->ResetGestureSlot(GESTURE_SLOT_VCD);

		// Handle the camera for the local player.
		if (pLocalPlayer)
		{
			TurnOffTauntCam();
		}
	}
}

void C_Laz_Player::UpdateOnRemove()
{
	// Stop the taunt.
	if (m_bWasTaunting)
	{
		TurnOffTauntCam();
	}

	BaseClass::UpdateOnRemove();
}

bool C_Laz_Player::CreateMove(float flInputSampleTime, CUserCmd * pCmd)
{
	static QAngle angMoveAngle(0.0f, 0.0f, 0.0f);

	bool bNoTaunt = true;
	if (InTauntCam())
	{
		// show centerprint message 
		pCmd->forwardmove = 0.0f;
		pCmd->sidemove = 0.0f;
		pCmd->upmove = 0.0f;
		//int nOldButtons = pCmd->buttons;
		pCmd->buttons = 0;
		pCmd->weaponselect = 0;

		VectorCopy(angMoveAngle, pCmd->viewangles);
		bNoTaunt = false;
	}
	else
	{
		VectorCopy(pCmd->viewangles, angMoveAngle);
	}

	BaseClass::CreateMove(flInputSampleTime, pCmd);

	//AvoidPlayers(pCmd);

	return bNoTaunt;
}

ShadowType_t C_Laz_Player::ShadowCastType(void)
{
	if (!IsVisible())
		return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

bool C_Laz_Player::ShouldReceiveProjectedTextures(int flags)
{
	return false;
}

bool C_Laz_Player::CanSprint(void)
{
	return ((!m_Local.m_bDucked && !m_Local.m_bDucking) && (GetWaterLevel() != 3));
}

void C_Laz_Player::StartSprinting(void)
{
	if (m_HL2Local.m_flSuitPower < 10)
	{
		// Don't sprint unless there's a reasonable
		// amount of suit power.
		CPASAttenuationFilter filter(this);
		filter.UsePredictionRules();
		EmitSound(filter, entindex(), "HL2Player.SprintNoPower");
		return;
	}

	CPASAttenuationFilter filter(this);
	filter.UsePredictionRules();
	EmitSound(filter, entindex(), "HL2Player.SprintStart");

	SetMaxSpeed(HL2_SPRINT_SPEED);
	m_fIsSprinting = true;
}

void C_Laz_Player::StopSprinting(void)
{
	SetMaxSpeed(HL2_NORM_SPEED);
	m_fIsSprinting = false;
}

void C_Laz_Player::HandleSpeedChanges(void)
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	if (buttonsChanged & IN_SPEED)
	{
		// The state of the sprint/run button has changed.
		if (IsSuitEquipped())
		{
			if (!(m_afButtonPressed & IN_SPEED) && IsSprinting())
			{
				StopSprinting();
			}
			else if ((m_afButtonPressed & IN_SPEED) && !IsSprinting())
			{
				if (CanSprint())
				{
					StartSprinting();
				}
				else
				{
					// Reset key, so it will be activated post whatever is suppressing it.
					m_nButtons &= ~IN_SPEED;
				}
			}
		}
	}
	else if (buttonsChanged & IN_WALK)
	{
		if (IsSuitEquipped())
		{
			// The state of the WALK button has changed.
			if (IsWalking() && !(m_afButtonPressed & IN_WALK))
			{
				StopWalking();
			}
			else if (!IsWalking() && !IsSprinting() && (m_afButtonPressed & IN_WALK) && !(m_nButtons & IN_DUCK))
			{
				StartWalking();
			}
		}
	}

	if (IsSuitEquipped() && m_fIsWalking && !(m_nButtons & IN_WALK))
		StopWalking();
}

void C_Laz_Player::StartWalking(void)
{
	SetMaxSpeed(HL2_WALK_SPEED);
	m_fIsWalking = true;
}

void C_Laz_Player::StopWalking(void)
{
	SetMaxSpeed(HL2_NORM_SPEED);
	m_fIsWalking = false;
}
