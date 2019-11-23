#pragma once
#include "spp_utils.h"
#include "utlbuffer.h"
#include "utlstring.h"
#include "utlvector.h"

class CMapEditHelper
{
public:
	CMapEditHelper() : m_bufEntities(0, 0, CUtlBuffer::TEXT_BUFFER), m_strLastMap()
	{}

	virtual const char* DoMapEdit(const char* pMapName, const char* pMapEntities, CUtlVector<char*>& vecVariants);
private:
	CUtlBuffer m_bufEntities;
	CUtlString m_strLastMap;
	CUtlStringList m_slstLastVariants;
};