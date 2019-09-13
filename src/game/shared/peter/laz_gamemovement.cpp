#include "cbase.h"
#include "laz_gamemovement.h"
#include "in_buttons.h"
#include "movevars_shared.h"
#ifdef CLIENT_DLL
#include "c_recipientfilter.h"
#include "peter/c_laz_player.h"
#else
#include "peter/laz_player.h"
#endif

#define PLAYER_LONGJUMP_SPEED 350 // how fast we longjump

extern ConVar xc_uncrouch_on_jump;

unsigned int CLazGameMovement::PlayerSolidMask(bool brushOnly)
{
	unsigned int uMask = 0;

	if (player)
	{
		switch (player->GetTeamNumber())
		{
		case TF_TEAM_RED:
			uMask = CONTENTS_COMBINETEAM;
			break;

		case TF_TEAM_BLUE:
			uMask = CONTENTS_REDTEAM;
			break;
		}
	}

	return (uMask | BaseClass::PlayerSolidMask(brushOnly));
}

void CLazGameMovement::PlayerRoughLandingEffects(float fvol)
{
	BaseClass::PlayerRoughLandingEffects(fvol);

	int iPlaySound = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(player, iPlaySound, play_rough_landing_sound);

	if (iPlaySound > 0 && fvol >= 1.0)
	{
		// Play the future shoes sound
		string_t strSound = AllocPooledString_StaticConstantStringPointer("PortalPlayer.FallRecover");
		CALL_ATTRIB_HOOK_STRING_ON_OTHER(player, strSound, rough_landing_sound);

		CSoundParameters params;
		if (CBaseEntity::GetParametersForSound(STRING(strSound), params, NULL))
		{
			EmitSound_t ep(params);
			ep.m_nPitch = 125.0f - player->m_Local.m_flFallVelocity * 0.03f;					// lower pitch the harder they land
			ep.m_flVolume = min(player->m_Local.m_flFallVelocity * 0.00075f - 0.38f, 1.0f);	// louder the harder they land

			CPASAttenuationFilter filter(player, ep.m_SoundLevel);
			if (gpGlobals->maxClients > 1)
				filter.UsePredictionRules();

			CBaseEntity::EmitSound(filter, player->entindex(), ep);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CLazGameMovement::CheckJumpButton(void)
{
	if (player->pl.deadflag)
	{
		mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
		return false;
	}

	// See if we are waterjumping.  If so, decrement count and return.
	if (player->m_flWaterJumpTime)
	{
		player->m_flWaterJumpTime -= gpGlobals->frametime;
		if (player->m_flWaterJumpTime < 0)
			player->m_flWaterJumpTime = 0;

		return false;
	}

	// If we are in the water most of the way...
	if (player->GetWaterLevel() >= 2)
	{
		// swimming, not jumping
		SetGroundEntity(NULL);

		if (player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
			mv->m_vecVelocity[2] = 100;
		else if (player->GetWaterType() == CONTENTS_SLIME)
			mv->m_vecVelocity[2] = 80;

		// play swiming sound
		if (player->m_flSwimSoundTime <= 0)
		{
			// Don't play sound again for 1 second
			player->m_flSwimSoundTime = 1000;
			PlaySwimSound();
		}

		return false;
	}

	// No more effect
	if (player->GetGroundEntity() == NULL)
	{
		mv->m_nOldButtons |= IN_JUMP;
		return false;		// in air, so no effect
	}

	// Don't allow jumping when the player is in a stasis field.
	if (!hl2_episodic.GetBool() && player->m_Local.m_bSlowMovement)
		return false;

	if (mv->m_nOldButtons & IN_JUMP)
		return false;		// don't pogo stick

	// Cannot jump will in the unduck transition.
	if (player->m_Local.m_bDucking && (player->GetFlags() & FL_DUCKING))
		return false;

	// Still updating the eye position.
	if (player->m_Local.m_flDuckJumpTime > 0.0f)
		return false;


	// In the air now.
	SetGroundEntity(NULL);

	player->PlayStepSound(mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true);

	MoveHelper()->PlayerSetAnimation(PLAYER_JUMP);

	float flGroundFactor = 1.0f;
	if (player->m_pSurfaceData)
	{
		flGroundFactor = player->m_pSurfaceData->game.jumpFactor;
	}

	float flMul;
	{
		float flJumpMod = 1.f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, flJumpMod, mod_jump_height);
		flMul = sqrtf(2 * GetCurrentGravity() * GAMEMOVEMENT_JUMP_HEIGHT * flJumpMod);
	}

	bool bLongJump = false;

	// Acclerate upward
	// If we are ducking...
	float startz = mv->m_vecVelocity[2];
	if ((player->m_Local.m_bDucking) || (player->GetFlags() & FL_DUCKING))
	{
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )

		// Adjust for super long jump module
		// UNDONE -- note this should be based on forward angles, not current velocity.
		int iEnableLongJump = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, iEnableLongJump, enable_long_jump);

		if (iEnableLongJump > 0 &&
			(mv->m_nButtons & IN_DUCK) &&
			(player->m_Local.m_flDucktime > 0) &&
			mv->m_vecVelocity.Length() > 50)
		{
			player->m_Local.m_vecPunchAngle.Set(PITCH, -5);

			mv->m_vecVelocity = m_vecForward * PLAYER_LONGJUMP_SPEED * 1.6;
			mv->m_vecVelocity.z = flMul;
			bLongJump = true;
		}
		else
		{
			mv->m_vecVelocity[2] = flGroundFactor * flMul;  // 2 * gravity * height
		}
	}
	else
	{
		mv->m_vecVelocity[2] += flGroundFactor * flMul;  // 2 * gravity * height
	}

	// Add a little forward velocity based on your current forward velocity - if you are not sprinting.
#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
	if (!bLongJump && gpGlobals->maxClients == 1)
	{
		CHLMoveData* pMoveData = (CHLMoveData*)mv;
		Vector vecForward;
		AngleVectors(mv->m_vecViewAngles, &vecForward);
		vecForward.z = 0;
		VectorNormalize(vecForward);

		// We give a certain percentage of the current forward movement as a bonus to the jump speed.  That bonus is clipped
		// to not accumulate over time.
		float flSpeedBoostPerc = (!pMoveData->m_bIsSprinting && !player->m_Local.m_bDucked) ? 0.5f : 0.1f;
		float flSpeedAddition = fabsf(mv->m_flForwardMove * flSpeedBoostPerc);
		float flMaxSpeed = mv->m_flMaxSpeed + (mv->m_flMaxSpeed * flSpeedBoostPerc);
		float flNewSpeed = (flSpeedAddition + mv->m_vecVelocity.Length2D());

		// If we're over the maximum, we want to only boost as much as will get us to the goal speed
		if (flNewSpeed > flMaxSpeed)
		{
			flSpeedAddition -= flNewSpeed - flMaxSpeed;
		}

		if (mv->m_flForwardMove < 0.0f)
			flSpeedAddition *= -1.0f;

		// Add it on
		VectorAdd((vecForward * flSpeedAddition), mv->m_vecVelocity, mv->m_vecVelocity);
	}
#endif

	FinishGravity();

	mv->m_outJumpVel.z += mv->m_vecVelocity[2] - startz;
	mv->m_outStepHeight += 0.15f;

	OnJump(mv->m_outJumpVel.z);

	// Set jump time.
	if (gpGlobals->maxClients == 1)
	{
		player->m_Local.m_flJumpTime = GAMEMOVEMENT_JUMP_TIME;
		player->m_Local.m_bInDuckJump = true;
	}

#if defined( HL2_DLL )

	if (xc_uncrouch_on_jump.GetBool())
	{
		// Uncrouch when jumping
		if (player->GetToggledDuckState())
		{
			player->ToggleDuck();
		}
	}
#endif

	CLaz_Player *pLaz = static_cast<CLaz_Player*>(player);
	if (bLongJump)
		pLaz->DoAnimationEvent(PLAYERANIMEVENT_DOUBLEJUMP);
	else
		pLaz->DoAnimationEvent(PLAYERANIMEVENT_JUMP);

	// Flag that we jumped.
	mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
	return true;
}

// Expose our interface.
static CLazGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = (IGameMovement *)&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement);