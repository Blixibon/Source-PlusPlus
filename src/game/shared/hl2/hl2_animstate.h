//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef HL2ANIMSTATE_H
#define HL2ANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "multiplayer/multiplayer_animstate.h"

class CHL2PlayerAnimState : public CMultiPlayerAnimState
{
	DECLARE_CLASS(CHL2PlayerAnimState, CMultiPlayerAnimState)
public:
	CHL2PlayerAnimState(CBasePlayer* pPlayer, MultiPlayerMovementData_t& movementData) : BaseClass(pPlayer, movementData)
	{}

	CHL2PlayerAnimState(CBasePlayer* pPlayer) : BaseClass(pPlayer, s_MoveParams)
	{}

	virtual Activity TranslateActivity(Activity actDesired) OVERRIDE;

	static MultiPlayerMovementData_t s_MoveParams;

protected:
	virtual bool		SetupPoseParameters(CStudioHdr* pStudioHdr);
private:
	static acttable_t s_acttableMPToHL2MP[];
};

#endif
