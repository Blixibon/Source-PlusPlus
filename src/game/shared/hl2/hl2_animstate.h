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
	virtual Activity CalcMainActivity();

	static MultiPlayerMovementData_t s_MoveParams;

protected:
	virtual bool		SetupPoseParameters(CStudioHdr* pStudioHdr);
	virtual AimType_e	GetAimType();

	virtual bool HandleJumping(Activity& idealActivity);
	virtual bool HandleVaulting(Activity& idealActivity);
private:
	static acttable_t s_acttableMPToHL2MP[];
	float	m_fGroundTime;
};

#endif
