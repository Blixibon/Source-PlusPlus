#include "cbase.h"
#include "multiplayer/basenetworkedplayer_gamemove.h"

#ifdef CLIENT_DLL
#include "multiplayer/basenetworkedplayer_cl.h"
#else
#include "multiplayer/basenetworkedplayer.h"
#endif

bool CNetworkedPlayerMovement::CheckJumpButton()
{
	bool HasJumped = BaseClass::CheckJumpButton();

	if (HasJumped)
		static_cast<CBaseNetworkedPlayer*>(player)->DoAnimationEvent(PLAYERANIMEVENT_JUMP);

	return HasJumped;
}