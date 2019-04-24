#include "cbase.h"
#ifdef GAME_DLL
#include "multiplayer/basenetworkedplayer.h"
#else
#include "multiplayer/basenetworkedplayer_cl.h"
#endif

/*void CBaseNetworkedPlayer::MakeAnimState()
{
#ifdef CLIENT_DLL
	MDLCACHE_CRITICAL_SECTION();
#endif
	MultiPlayerMovementData_t mv;
	mv.m_flBodyYawRate = 360;
	mv.m_flRunSpeed = 320;
	mv.m_flWalkSpeed = 75;
	mv.m_flSprintSpeed = -1.0f;
	m_PlayerAnimState = new CMultiPlayerAnimState( this,mv );
}*/

void CBaseNetworkedPlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	switch (playerAnim)
	{
	case PLAYER_IDLE:
		break;
	case PLAYER_WALK:
		break;
	case PLAYER_JUMP:
		break;
	case PLAYER_SUPERJUMP:
		break;
	case PLAYER_DIE:
		break;
	case PLAYER_ATTACK1:
		DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);
		break;
	case PLAYER_IN_VEHICLE:
		break;
	case PLAYER_RELOAD:
		DoAnimationEvent(PLAYERANIMEVENT_RELOAD);
		break;
	case PLAYER_START_AIMING:
		break;
	case PLAYER_LEAVE_AIMING:
		break;
	default:
		break;
	}
}