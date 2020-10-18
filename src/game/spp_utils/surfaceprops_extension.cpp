#include "surfaceprops_extension.h"
#include "filesystem_helpers.h"

#define MAX_KEYVALUE	1024

const char* ParseKeyvalue(const char* pBuffer, char(&key)[MAX_KEYVALUE], char(&value)[MAX_KEYVALUE])
{
	// Make sure value is always null-terminated.
	value[0] = 0;

	pBuffer = ParseFile(pBuffer, key, NULL);

	// no value on a close brace
	if (key[0] == '}' && key[1] == 0)
	{
		value[0] = 0;
		return pBuffer;
	}

	Q_strlower(key);

	pBuffer = ParseFile(pBuffer, value, NULL);

	Q_strlower(value);

	return pBuffer;
}

CExtendedSurfaceProps::CExtendedSurfaceProps() : m_Strings(0, 32, true)
{
	m_Strings.AddString("");
	m_bInit = false;
}

void CExtendedSurfaceProps::Init(IInternalSharedData* pData)
{
	m_pData = pData;
}

surfacedata2_t* CExtendedSurfaceProps::GetSurfaceData2(int surfaceDataIndex)
{
	return &m_Props[surfaceDataIndex];
}

surfacedataall_t CExtendedSurfaceProps::GetSurfaceDataBoth(int surfaceDataIndex)
{
	surfacedataall_t data;
	data.pOld = GetSurfaceData(surfaceDataIndex);
	data.pNew = GetSurfaceData2(surfaceDataIndex);
	return data;
}

const char* CExtendedSurfaceProps::GetString2(unsigned short stringTableIndex) const
{
	return m_Strings.String(stringTableIndex);
}

void CExtendedSurfaceProps::CopyPhysicsProperties(surfacedata2_t* pOut, int baseIndex)
{
	if (m_Props.IsValidIndex(baseIndex) && m_PropsUsed.IsBitSet(baseIndex))
	{
		const surfacedata2_t& baseProp = m_Props[baseIndex];
		*pOut = baseProp;
	}
}

int CExtendedSurfaceProps::ParseSurfaceData(const char* pFilename, const char* pTextfile)
{
	int iRet = GetBaseProps()->ParseSurfaceData(pFilename, pTextfile);
	if (iRet > 0)
	{
		m_Props.SetCountNonDestructively(iRet);
		m_PropsUsed.Resize(iRet);

		const char* pText = pTextfile;

		do
		{
			char key[MAX_KEYVALUE], value[MAX_KEYVALUE];

			pText = ParseKeyvalue(pText, key, value);
			if (!strcmp(value, "{"))
			{
				int iIndex = GetSurfaceIndex(key);
				if (m_Props.IsValidIndex(iIndex))
				{
					surfacedata2_t prop;
					memset(&prop, 0, sizeof(prop));

					int iBase = iIndex;
					if (!m_PropsUsed.IsBitSet(iIndex))
					{
						iBase = GetSurfaceIndex("default");
					}

					CopyPhysicsProperties(&prop, iBase);

					do
					{
						pText = ParseKeyvalue(pText, key, value);
						if (!strcmpi(key, "}"))
						{
							m_Props[iIndex] = prop;
							m_PropsUsed.Set(iIndex);
							break;
						}
						else if (!strcmpi(key, "base"))
						{
							iBase = GetSurfaceIndex(value);
							CopyPhysicsProperties(&prop, iBase);
						}
						// sound names
						else if (!strcmpi(key, "runleft"))
						{
							prop.sounds.runStepLeft = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "runright"))
						{
							prop.sounds.runStepRight = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "walkleft"))
						{
							prop.sounds.walkStepLeft = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "walkright"))
						{
							prop.sounds.walkStepRight = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "stepleft"))
						{
							prop.sounds.runStepLeft = prop.sounds.walkStepLeft = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "stepright"))
						{
							prop.sounds.runStepRight = prop.sounds.walkStepRight = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "jump"))
						{
							prop.sounds.stepJump = m_Strings.AddString(value);
						}
						else if (!strcmpi(key, "land"))
						{
							prop.sounds.stepLand = m_Strings.AddString(value);
						}
					} while (pText);
				}
				else
				{
					do
					{
						pText = ParseKeyvalue(pText, key, value);
						if (!strcmpi(key, "}"))
						{

							break;
						}
					} while (pText);
				}
			}
		} while (pText);
	}

	return iRet;
}

int CExtendedSurfaceProps::SurfacePropCount(void) const
{
	return GetBaseProps()->SurfacePropCount();
}

int CExtendedSurfaceProps::GetSurfaceIndex(const char* pSurfacePropName) const
{
	return GetBaseProps()->GetSurfaceIndex(pSurfacePropName);
}

void CExtendedSurfaceProps::GetPhysicsProperties(int surfaceDataIndex, float* density, float* thickness, float* friction, float* elasticity) const
{
	GetBaseProps()->GetPhysicsProperties(surfaceDataIndex, density, thickness, friction, elasticity);
}

surfacedata_t* CExtendedSurfaceProps::GetSurfaceData(int surfaceDataIndex)
{
	return GetBaseProps()->GetSurfaceData(surfaceDataIndex);
}

const char* CExtendedSurfaceProps::GetString(unsigned short stringTableIndex) const
{
	return GetBaseProps()->GetString(stringTableIndex);
}

const char* CExtendedSurfaceProps::GetPropName(int surfaceDataIndex) const
{
	return GetBaseProps()->GetPropName(surfaceDataIndex);
}

void CExtendedSurfaceProps::SetWorldMaterialIndexTable(int* pMapArray, int mapSize)
{
	GetBaseProps()->SetWorldMaterialIndexTable(pMapArray, mapSize);
}

void CExtendedSurfaceProps::GetPhysicsParameters(int surfaceDataIndex, surfacephysicsparams_t* pParamsOut) const
{
	GetBaseProps()->GetPhysicsParameters(surfaceDataIndex, pParamsOut);
}
