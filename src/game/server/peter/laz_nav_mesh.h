#ifndef LAZ_NAV_MESH_H
#define LAZ_NAV_MESH_H
#pragma once

#include "nav_mesh.h"

class CLazNavMesh : public CNavMesh
{
	DECLARE_CLASS_GAMEROOT(CLazNavMesh, CNavMesh);
public:
	virtual void AddWalkableSeeds(void);
};

#endif // !LAZ_NAV_MESH_H
