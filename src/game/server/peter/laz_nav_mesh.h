#ifndef LAZ_NAV_MESH_H
#define LAZ_NAV_MESH_H
#pragma once

#include "nav_mesh.h"
#include "nav_area.h"

enum {
	LAZ_NAV_MESH_FIRST_VERSION = 1,
};
#define LAZ_NAV_MESH_VERSION LAZ_NAV_MESH_FIRST_VERSION

class CLazNavMesh : public CNavMesh
{
	DECLARE_CLASS_GAMEROOT(CLazNavMesh, CNavMesh);
public:
	virtual void AddWalkableSeeds(void);
	virtual CNavArea *CreateArea(void) const;							// CNavArea factory
	virtual unsigned int GetGenerationTraceMask(void) const;			// return the mask used by traces when generating the mesh
	virtual unsigned int GetSubVersionNumber(void) const { return LAZ_NAV_MESH_VERSION; }
};

class CLazNavArea : public CNavArea
{
	DECLARE_CLASS(CLazNavArea, CNavArea);
public:
	virtual void Save(CUtlBuffer &fileBuffer, unsigned int version) const;	// (EXTEND)
	virtual NavErrorType Load(CUtlBuffer &fileBuffer, unsigned int version, unsigned int subVersion);		// (EXTEND)

	virtual void CustomAnalysis(bool isIncremental = false);	// for game-specific analysis

	int		GetAreaContents(void) { return m_iAccumulatedWorldContents; }

protected:
	int m_iAccumulatedWorldContents;
};

#endif // !LAZ_NAV_MESH_H
