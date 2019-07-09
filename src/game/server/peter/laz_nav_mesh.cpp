#include "cbase.h"
#include "laz_nav_mesh.h"

#include "memdbgon.h"

CNavMesh *NavMeshFactory(void)
{
	return new CLazNavMesh;
}

void CLazNavMesh::AddWalkableSeeds(void)
{
	const char *ppszSpawnPoints[] = {
		"info_player_start",
		"info_player_teamspawn",
		"info_player_deathmatch",
		"info_player_coop",
		"info_player_rebels",
		"info_player_combine",
		"info_player_axis",
		"info_player_allies",
		"info_player_terrorist",
		"info_player_counterterrorist"
	};

	for (CBaseEntity *pEnt = gEntList.FirstEnt(); pEnt != nullptr; pEnt = gEntList.NextEnt(pEnt))
	{
		bool bValid = false;
		for (int i = 0; i < ARRAYSIZE(ppszSpawnPoints); i++)
		{
			if (pEnt->ClassMatches(ppszSpawnPoints[i]))
				bValid = true;
		}

		if (bValid)
		{
			// snap it to the sampling grid
			Vector pos = pEnt->GetAbsOrigin();
			pos.x = SnapToGrid(pos.x);
			pos.y = SnapToGrid(pos.y);

			Vector normal;
			if (FindGroundForNode(&pos, &normal))
			{
				AddWalkableSeed(pos, normal);
			}
		}
	}
}
