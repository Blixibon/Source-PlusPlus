#ifndef LAZ_GAMEMOVE_H
#define LAZ_GAMEMOVE_H
#pragma once
#include "portal_gamemovement.h"

class CLazGameMovement : public CPortalGameMovement
{
	typedef CPortalGameMovement BaseClass;
public:
	virtual unsigned int PlayerSolidMask(bool brushOnly = false);
	virtual void	PlayerRoughLandingEffects(float fvol);
	virtual bool CheckJumpButton(void);
};

#endif // !LAZ_GAMEMOVE_H
