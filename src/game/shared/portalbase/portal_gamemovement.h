#ifndef PORTAL_GAMEMOVEMENT_H
#define PORTAL_GAMEMOVEMENT_H
#pragma once

#include "hl_gamemovement.h"
#if defined( CLIENT_DLL )
#include "c_portal_player.h"
#else
#include "portal_player.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: Portal specific movement code
//-----------------------------------------------------------------------------
class CPortalGameMovement : public CHL2GameMovement
{
	typedef CGameMovement BaseClass;
public:

	CPortalGameMovement();

	bool	m_bInPortalEnv;
	// Overrides
	virtual void ProcessMovement(CBasePlayer* pPlayer, CMoveData* pMove);
	virtual bool CheckJumpButton(void);

	void FunnelIntoPortal(CProp_Portal* pPortal, Vector& wishdir);

	virtual void AirAccelerate(Vector& wishdir, float wishspeed, float accel);
	virtual void AirMove(void);

	virtual void CategorizePosition(void);

	// Traces the player bbox as it is swept from start to end
	virtual void TracePlayerBBox(const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm);

	// Tests the player position
	virtual CBaseHandle	TestPlayerPosition(const Vector& pos, int collisionGroup, trace_t& pm);

	virtual void Duck(void);				// Check for a forced duck

	virtual int CheckStuck(void);

	virtual void SetGroundEntity(trace_t* pm);

private:


	CPortal_Player* GetPortalPlayer();
};

#endif