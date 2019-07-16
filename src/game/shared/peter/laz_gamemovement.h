#ifndef LAZ_GAMEMOVE_H
#define LAZ_GAMEMOVE_H
#pragma once
#include "hl_gamemovement.h"

class CLazGameMovement : public CHL2GameMovement
{
	typedef CHL2GameMovement BaseClass;
public:
	virtual unsigned int PlayerSolidMask(bool brushOnly = false);
};

#endif // !LAZ_GAMEMOVE_H
