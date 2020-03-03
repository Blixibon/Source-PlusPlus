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

	virtual void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);

	static MultiPlayerMovementData_t s_MoveParams;

protected:
	virtual bool		SetupPoseParameters(CStudioHdr* pStudioHdr);
	virtual void		ComputePoseParam_AimYaw(CStudioHdr* pStudioHdr);
	virtual void		ComputePoseParam_AimPitch(CStudioHdr* pStudioHdr);
	virtual void		ComputePoseParam_MoveYaw(CStudioHdr* pStudioHdr);
	virtual AimType_e	GetAimType();

	virtual bool HandleJumping(Activity& idealActivity);
	virtual bool HandleVaulting(Activity& idealActivity);
	virtual bool HandleDriving(Activity& idealActivity);
	virtual bool HandleClimbing(Activity& idealActivity);
	virtual bool HandleMoving(Activity& idealActivity);
	virtual bool HandleDucking(Activity& idealActivity);
private:
	static acttable_t s_acttableMPToHL2MP[];
	float	m_fGroundTime;
	bool	m_bLongJump;

#ifdef CLIENT_DLL
	int	m_headYawPoseParam;
	int	m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;

	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;

	int m_chestAttachment;
#endif
};

#endif
