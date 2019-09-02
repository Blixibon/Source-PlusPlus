#ifndef LAZ_GAMEMOVE_H
#define LAZ_GAMEMOVE_H
#pragma once
#include "hl_gamemovement.h"

class CLazGameMovement : public CHL2GameMovement
{
	typedef CHL2GameMovement BaseClass;
public:
	virtual unsigned int PlayerSolidMask(bool brushOnly = false);
	virtual void	PlayerRoughLandingEffects(float fvol);
	virtual bool CheckJumpButton(void);
};

#endif // !LAZ_GAMEMOVE_H
