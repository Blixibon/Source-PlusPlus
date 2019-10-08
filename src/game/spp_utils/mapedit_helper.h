#pragma once
#include "spp_utils.h"
#include "utlbuffer.h"
#include "utlstring.h"

class CMapEditHelper : public IMapEditHelper
{
public:
	CMapEditHelper() : m_bufEntities(0, 0, CUtlBuffer::TEXT_BUFFER), m_strLastMap()
	{}

	virtual const char* DoMapEdit(const char* pMapName, const char* pMapEntities);
private:
	CUtlBuffer m_bufEntities;
	CUtlString m_strLastMap;
};