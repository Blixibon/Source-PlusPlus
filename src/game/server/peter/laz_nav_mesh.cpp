#include "cbase.h"
#include "laz_nav_mesh.h"
#include "world.h"

#include "memdbgon.h"

//--------------------------------------------------------------------------------------------------------------
/**
 * Finds the hiding spot position in a corner's area.  If the typical inset is off the nav area (small
 * hand-constructed areas), it tries to fit the position inside the area.
 */
static Vector FindPositionInArea(CNavArea *area, NavCornerType corner)
{
	int multX = 1, multY = 1;
	switch (corner)
	{
	case NORTH_WEST:
		break;
	case NORTH_EAST:
		multX = -1;
		break;
	case SOUTH_WEST:
		multY = -1;
		break;
	case SOUTH_EAST:
		multX = -1;
		multY = -1;
		break;
	}

	const float offset = 12.5f;
	Vector cornerPos = area->GetCorner(corner);

	// Try the basic inset
	Vector pos = cornerPos + Vector(offset*multX, offset*multY, 0.0f);
	if (!area->IsOverlapping(pos))
	{
		// Try pulling the Y offset to the area's center
		pos = cornerPos + Vector(offset*multX, area->GetSizeY()*0.5f*multY, 0.0f);
		if (!area->IsOverlapping(pos))
		{
			// Try pulling the X offset to the area's center
			pos = cornerPos + Vector(area->GetSizeX()*0.5f*multX, offset*multY, 0.0f);
			if (!area->IsOverlapping(pos))
			{
				// Try pulling the X and Y offsets to the area's center
				pos = cornerPos + Vector(area->GetSizeX()*0.5f*multX, area->GetSizeY()*0.5f*multY, 0.0f);
				if (!area->IsOverlapping(pos))
				{
					AssertMsg(false, "A Hiding Spot can't be placed on its area at (%.0f %.0f %.0f)", cornerPos.x, cornerPos.y, cornerPos.z);

					// Just pull the position to a small offset
					pos = cornerPos + Vector(1.0f*multX, 1.0f*multY, 0.0f);
					if (!area->IsOverlapping(pos))
					{
						// Nothing is working (degenerate area?), so just put it directly on the corner
						pos = cornerPos;
					}
				}
			}
		}
	}

	return pos;
}

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
		"info_player_counterterrorist",
		"info_teleport_destination"
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

CNavArea * CLazNavMesh::CreateArea(void) const
{
	return new CLazNavArea;
}

unsigned int CLazNavMesh::GetGenerationTraceMask(void) const
{
	return MASK_SOLID_BRUSHONLY;
}

void CLazNavArea::Save(CUtlBuffer & fileBuffer, unsigned int version) const
{
	BaseClass::Save(fileBuffer, version);

	fileBuffer.PutInt(m_iAccumulatedWorldContents);
}

NavErrorType CLazNavArea::Load(CUtlBuffer & fileBuffer, unsigned int version, unsigned int subVersion)
{
	NavErrorType eRet = BaseClass::Load(fileBuffer, version, subVersion);

	if (eRet != NAV_OK)
		return eRet;

	if (subVersion < LAZ_NAV_MESH_FIRST_VERSION)
		return NAV_OK;

	m_iAccumulatedWorldContents = fileBuffer.GetInt();

	return NAV_OK;
}

int GetWorldContents(const Vector vecPos)
{
	ICollideable *pCollide = GetWorldEntity()->GetCollideable();
	return enginetrace->GetPointContents_Collideable(pCollide, vecPos);
}

void CLazNavArea::CustomAnalysis(bool isIncremental)
{
	Vector vecCenter = GetCenter();
	vecCenter.z = GetZ(vecCenter) + HalfHumanHeight;
	float height;
	if (TheNavMesh->GetGroundHeight(vecCenter, &height))
	{
		vecCenter.z = height + HalfHumanHeight;
	}

	int iContents = GetWorldContents(vecCenter);

	for (int i = 0; i < NUM_CORNERS; ++i)
	{
		Vector pos = FindPositionInArea(this, (NavCornerType)i);
		pos.z = GetZ(pos) + HalfHumanHeight;	// players light from their centers, and we light from slightly below that, to allow for low ceilings
		float height;
		if (TheNavMesh->GetGroundHeight(pos, &height))
		{
			pos.z = height + HalfHumanHeight;	// players light from their centers, and we light from slightly below that, to allow for low ceilings
		}

		iContents |= GetWorldContents(pos);
	}

	m_iAccumulatedWorldContents = iContents;
}
