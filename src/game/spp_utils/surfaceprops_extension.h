#pragma once
#include "shared.h"
#include "ISurfacePropsExt.h"
#include "utlvector.h"
#include "utlsymbol.h"
#include "bitvec.h"

class CExtendedSurfaceProps : public ISPPSurfacePropsExtension
{
public:
	CExtendedSurfaceProps();

	void	Init(IInternalSharedData* pData);

	// ISPPSurfacePropsExtension
	virtual surfacedata2_t* GetSurfaceData2(int surfaceDataIndex) override;
	virtual surfacedataall_t GetSurfaceDataBoth(int surfaceDataIndex) override;

	virtual const char* GetString2(unsigned short stringTableIndex) const override;

	// IPhysicsSurfaceProps
	virtual int		ParseSurfaceData(const char* pFilename, const char* pTextfile) override;
	virtual int		SurfacePropCount(void) const override;

	virtual int		GetSurfaceIndex(const char* pSurfacePropName) const override;
	virtual void	GetPhysicsProperties(int surfaceDataIndex, float* density, float* thickness, float* friction, float* elasticity) const override;

	virtual surfacedata_t* GetSurfaceData(int surfaceDataIndex) override;
	virtual const char* GetString(unsigned short stringTableIndex) const override;


	virtual const char* GetPropName(int surfaceDataIndex) const override;

	// sets the global index table for world materials
	// UNDONE: Make this per-CPhysCollide
	virtual void	SetWorldMaterialIndexTable(int* pMapArray, int mapSize) override;

	// NOTE: Same as GetPhysicsProperties, but maybe more convenient
	virtual void	GetPhysicsParameters(int surfaceDataIndex, surfacephysicsparams_t* pParamsOut) const override;

private:
	inline IPhysicsSurfaceProps* GetBaseProps() const;
	void	CopyPhysicsProperties(surfacedata2_t* pOut, int baseIndex);

	IInternalSharedData* m_pData;

	CUtlSymbolTable m_Strings;
	//CUtlVector< int > m_PropFlags;
	CUtlVector< surfacedata2_t > m_Props;
	CVarBitVec m_PropsUsed;
	bool m_bInit;
};

inline IPhysicsSurfaceProps* CExtendedSurfaceProps::GetBaseProps() const
{
	if (!m_pData)
		return nullptr;

	return m_pData->GetEnginePointers().physicsprops;
}