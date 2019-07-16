#include "cbase.h"
#include "laz_gamemovement.h"

unsigned int CLazGameMovement::PlayerSolidMask(bool brushOnly)
{
	unsigned int uMask = 0;

	if (player)
	{
		switch (player->GetTeamNumber())
		{
		case TF_TEAM_RED:
			uMask = CONTENTS_COMBINETEAM;
			break;

		case TF_TEAM_BLUE:
			uMask = CONTENTS_REDTEAM;
			break;
		}
	}

	return (uMask | BaseClass::PlayerSolidMask(brushOnly));
}

// Expose our interface.
static CLazGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = (IGameMovement *)&g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement);