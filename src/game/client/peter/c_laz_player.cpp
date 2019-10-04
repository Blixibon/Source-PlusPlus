#include "cbase.h"
#include "c_laz_player.h"
#include "choreoevent.h"
#include "cam_thirdperson.h"
#include "in_buttons.h"
#include "c_ai_basenpc.h"
#include "c_team.h"
#include "collisionutils.h"

static Vector TF_TAUNTCAM_HULL_MIN(-9.0f, -9.0f, -9.0f);
static Vector TF_TAUNTCAM_HULL_MAX(9.0f, 9.0f, 9.0f);

static ConVar tf_tauntcam_yaw("tf_tauntcam_yaw", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
static ConVar tf_tauntcam_pitch("tf_tauntcam_pitch", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
static ConVar tf_tauntcam_dist("tf_tauntcam_dist", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);

static ConVar cl_playermodel("cl_playermodel", "none", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Default Player Model");
static ConVar cl_defaultweapon("cl_defaultweapon", "weapon_physcannon", FCVAR_USERINFO | FCVAR_ARCHIVE, "Default Spawn Weapon");
static ConVar cl_laz_mp_suit("cl_laz_mp_suit", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "Enable suit voice in multiplayer");


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

void C_Laz_Player::ClientThink()
{
	BaseClass::ClientThink();

	if (IsLocalPlayer())
	{
		
	}
	else
	{
		// Cold breath.
		UpdateColdBreath();
	}
}

float C_Laz_Player::GetFOV( void )
{
	//Find our FOV with offset zoom value
	float flFOVOffset = C_BasePlayer::GetFOV() + GetZoom();

	// Clamp FOV in MP
	float min_fov = GetMinFOV();

	// Don't let it go too low
	flFOVOffset = MAX( min_fov, flFOVOffset );

	return flFOVOffset;
}

void C_Laz_Player::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (type == DATA_UPDATE_CREATED)
	{
		SetNextClientThink(CLIENT_THINK_ALWAYS);
	}
	else
	{
		UpdateWearables();
	}

	UpdateVisibility();
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

	if (GetActiveWeapon())
	{
		GetActiveWeapon()->UpdateVisibility();
	}
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

//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
#define TF_AVOID_MAX_RADIUS_SQR		5184.0f			// Based on player extents and max buildable extents.
#define TF_OO_AVOID_MAX_RADIUS_SQR	0.00019f

ConVar tf_max_separation_force("tf_max_separation_force", "256", FCVAR_DEVELOPMENTONLY);

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

void C_Laz_Player::AvoidPlayers(CUserCmd *pCmd)
{
	// Don't test if the player doesn't exist or is dead.
	if (IsAlive() == false)
		return;

	C_Team *pTeam = (C_Team *)GetTeam();
	if (!pTeam)
		return;

	// Up vector.
	static Vector vecUp(0.0f, 0.0f, 1.0f);

	Vector vecTFPlayerCenter = GetAbsOrigin();
	Vector vecTFPlayerMin = GetPlayerMins();
	Vector vecTFPlayerMax = GetPlayerMaxs();
	float flZHeight = vecTFPlayerMax.z - vecTFPlayerMin.z;
	vecTFPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd(vecTFPlayerMin, vecTFPlayerCenter, vecTFPlayerMin);
	VectorAdd(vecTFPlayerMax, vecTFPlayerCenter, vecTFPlayerMax);

	// Find an intersecting player or object.
	int nAvoidPlayerCount = 0;
	C_Laz_Player *pAvoidPlayerList[MAX_PLAYERS];

	C_Laz_Player *pIntersectPlayer = NULL;
	//C_AI_BaseNPC *pIntersectObject = NULL;
	float flAvoidRadius = 0.0f;

	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for (int i = 0; i < pTeam->GetNumPlayers(); ++i)
	{
		C_Laz_Player *pAvoidPlayer = static_cast<C_Laz_Player *>(pTeam->GetPlayer(i));
		if (pAvoidPlayer == NULL)
			continue;
		// Is the avoid player me?
		if (pAvoidPlayer == this)
			continue;

		// Save as list to check against for objects.
		pAvoidPlayerList[nAvoidPlayerCount] = pAvoidPlayer;
		++nAvoidPlayerCount;

		// Check to see if the avoid player is dormant.
		if (pAvoidPlayer->IsDormant())
			continue;

		// Is the avoid player solid?
		if (pAvoidPlayer->IsSolidFlagSet(FSOLID_NOT_SOLID))
			continue;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd(vecAvoidMin, vecAvoidCenter, vecAvoidMin);
		VectorAdd(vecAvoidMax, vecAvoidCenter, vecAvoidMax);

		if (IsBoxIntersectingBox(vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax))
		{
			// Need to avoid this player.
			if (!pIntersectPlayer)
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}
#if 0
	// We didn't find a player - look for objects to avoid.
	if (!pIntersectPlayer)
	{
		for (int iPlayer = 0; iPlayer < nAvoidPlayerCount; ++iPlayer)
		{
			// Stop when we found an intersecting object.
			if (pIntersectObject)
				break;


			for (int iObject = 0; iObject < pTeam->GetNumNPCs(); ++iObject)
			{
				C_AI_BaseNPC *pAvoidObject = pTeam->GetNPC(iObject);
				if (!pAvoidObject)
					continue;

				// Check to see if the object is dormant.
				if (pAvoidObject->IsDormant())
					continue;

				// Is the object solid.
				if (pAvoidObject->IsSolidFlagSet(FSOLID_NOT_SOLID))
					continue;

				// If we shouldn't avoid it, see if we intersect it.
				//if (pAvoidObject->ShouldPlayersAvoid())
				{
					vecAvoidCenter = pAvoidObject->WorldSpaceCenter();
					vecAvoidMin = pAvoidObject->WorldAlignMins();
					vecAvoidMax = pAvoidObject->WorldAlignMaxs();
					VectorAdd(vecAvoidMin, vecAvoidCenter, vecAvoidMin);
					VectorAdd(vecAvoidMax, vecAvoidCenter, vecAvoidMax);

					if (IsBoxIntersectingBox(vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax))
					{
						// Need to avoid this object.
						pIntersectObject = pAvoidObject;
						break;
					}
				}
			}
		}
	}
#endif
	// Anything to avoid?
	if (!pIntersectPlayer /*&& !pIntersectObject*/)
	{
		return;
	}

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player - they have precedence.
	if (pIntersectPlayer)
	{
		VectorSubtract(pIntersectPlayer->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta);

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}
	// Avoid a object.
	/*else
	{
		VectorSubtract(pIntersectObject->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta);

		Vector vRad = pIntersectObject->WorldAlignMaxs() - pIntersectObject->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}*/

	float flPushStrength = RemapValClamped(vecDelta.Length(), flAvoidRadius, 0, 0, tf_max_separation_force.GetInt()); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if (flPushStrength < 0.01f)
		return;

	Vector vecPush;
	if (GetAbsVelocity().Length2DSqr() > 0.1f)
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct(vecUp, vecVelocity, vecPush);
		VectorNormalize(vecPush);
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors(angView, NULL, &vecPush, NULL);
	}

	// Move away from the other player/object.
	Vector vecSeparationVelocity;
	if (vecDelta.Dot(vecPush) < 0)
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flMaxPlayerSpeed = MaxSpeed();
	float flCropFraction = 1.33333333f;

	if ((GetFlags() & FL_DUCKING) && (GetGroundEntity() != NULL))
	{
		flMaxPlayerSpeed *= flCropFraction;
	}

	float flMaxPlayerSpeedSqr = flMaxPlayerSpeed * flMaxPlayerSpeed;

	if (vecSeparationVelocity.LengthSqr() > flMaxPlayerSpeedSqr)
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale(vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity);
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors(vAngles, &currentdir, &rightdir, NULL);

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize(vDirection);

	float fwd = currentdir.Dot(vDirection);
	float rt = rightdir.Dot(vDirection);

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	pCmd->forwardmove += forward;
	pCmd->sidemove += side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if (pCmd->forwardmove > fabs(cl_forwardspeed.GetFloat()))
	{
		flForwardScale = fabs(cl_forwardspeed.GetFloat()) / pCmd->forwardmove;
	}
	else if (pCmd->forwardmove < -fabs(cl_backspeed.GetFloat()))
	{
		flForwardScale = fabs(cl_backspeed.GetFloat()) / fabs(pCmd->forwardmove);
	}

	float flSideScale = 1.0f;
	if (fabs(pCmd->sidemove) > fabs(cl_sidespeed.GetFloat()))
	{
		flSideScale = fabs(cl_sidespeed.GetFloat()) / fabs(pCmd->sidemove);
	}

	float flScale = min(flForwardScale, flSideScale);
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
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

	Vector2D vMove(pCmd->forwardmove, pCmd->sidemove);
	if (vMove.IsLengthLessThan(HL2_WALK_SPEED))
		AvoidPlayers(pCmd);

	return bNoTaunt;
}

ConVar cl_blobbyshadows("cl_blobbyshadows", "0", FCVAR_CLIENTDLL);
ShadowType_t C_Laz_Player::ShadowCastType(void)
{
	// Removed the GetPercentInvisible - should be taken care off in BindProxy now.
	if (!IsVisible() /*|| GetPercentInvisible() > 0.0f*/)
		return SHADOWS_NONE;

	if (IsEffectActive(EF_NODRAW | EF_NOSHADOW))
		return SHADOWS_NONE;

	// If in ragdoll mode.
	if (m_nRenderFX == kRenderFxRagdoll)
		return SHADOWS_NONE;

	C_BasePlayer *pLocalPlayer = GetLocalPlayer();

	// if we're first person spectating this player
	if (pLocalPlayer &&
		pLocalPlayer->GetObserverTarget() == this &&
		pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
	{
		return SHADOWS_NONE;
	}

	if (cl_blobbyshadows.GetBool())
		return SHADOWS_SIMPLE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}

float g_flFattenAmt = 4;
void C_Laz_Player::GetShadowRenderBounds(Vector &mins, Vector &maxs, ShadowType_t shadowType)
{
	if (shadowType == SHADOWS_SIMPLE)
	{
		// Don't let the render bounds change when we're using blobby shadows, or else the shadow
		// will pop and stretch.
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
	}
	else
	{
		GetRenderBounds(mins, maxs);

		// We do this because the normal bbox calculations don't take pose params into account, and 
		// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
		// the shadow will get cut off as he rotates.
		//
		// Thus, we give it some padding here.
		mins -= Vector(g_flFattenAmt, g_flFattenAmt, 0);
		maxs += Vector(g_flFattenAmt, g_flFattenAmt, 0);
	}
}


void C_Laz_Player::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	// TODO POSTSHIP - this hack/fix goes hand-in-hand with a fix in CalcSequenceBoundingBoxes in utils/studiomdl/simplify.cpp.
	// When we enable the fix in CalcSequenceBoundingBoxes, we can get rid of this.
	//
	// What we're doing right here is making sure it only uses the bbox for our lower-body sequences since,
	// with the current animations and the bug in CalcSequenceBoundingBoxes, are WAY bigger than they need to be.
	C_BaseAnimating::GetRenderBounds(theMins, theMaxs);
}


bool C_Laz_Player::GetShadowCastDirection(Vector *pDirection, ShadowType_t shadowType) const
{
	if (shadowType == SHADOWS_SIMPLE)
	{
		// Blobby shadows should sit directly underneath us.
		pDirection->Init(0, 0, -1);
		return true;
	}
	else
	{
		return BaseClass::GetShadowCastDirection(pDirection, shadowType);
	}
}

bool C_Laz_Player::ShouldReceiveProjectedTextures(int flags)
{
	Assert(flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK);

	if (IsEffectActive(EF_NODRAW))
		return false;

	if (flags & SHADOW_FLAGS_FLASHLIGHT)
	{
		return true;
	}

	return BaseClass::ShouldReceiveProjectedTextures(flags);
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
		EmitSound(filter, entindex(), "SynergyPlayer.SprintNoPower");
		return;
	}

	CPASAttenuationFilter filter(this);
	filter.UsePredictionRules();
	EmitSound(filter, entindex(), "SynergyPlayer.SprintStart");

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
