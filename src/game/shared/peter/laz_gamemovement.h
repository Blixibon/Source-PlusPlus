#ifndef LAZ_GAMEMOVE_H
#define LAZ_GAMEMOVE_H
#pragma once
#include "portal_gamemovement.h"
#include "laz_player_shared.h"

class CLazGameMovement : public CPortalGameMovement
{
	typedef CPortalGameMovement BaseClass;
public:
	CLazGameMovement();

	virtual unsigned int PlayerSolidMask(bool brushOnly = false);
	virtual void ProcessMovement(CBasePlayer* pBasePlayer, CMoveData* pMove);
	virtual void	PlayerRoughLandingEffects(float fvol, bool bLateral);
	virtual bool CheckJumpButton(void);
	virtual Vector GetPlayerViewOffset(bool ducked) const;

protected:
	CLaz_Player* m_pLazPlayer;
};

#endif // !LAZ_GAMEMOVE_H
