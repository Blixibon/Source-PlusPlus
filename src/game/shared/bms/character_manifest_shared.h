#pragma once

#include "utlbuffer.h"

struct ManifestFlexData_t
{
	DECLARE_SIMPLE_DATADESC();

	char cName[16];
	float flValue;

	bool Serialize(CUtlBuffer& buf);
	bool Unserialize(CUtlBuffer& buf);
};