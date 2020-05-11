#include "cbase.h"
#include "character_manifest_shared.h"


BEGIN_SIMPLE_DATADESC(ManifestFlexData_t)
DEFINE_AUTO_ARRAY(cName, FIELD_CHARACTER),
DEFINE_FIELD(flValue, FIELD_FLOAT),
END_DATADESC();

bool ManifestFlexData_t::Serialize(CUtlBuffer& buf)
{
	buf.PutObjects(this);
	return buf.IsValid();
}

bool ManifestFlexData_t::Unserialize(CUtlBuffer& buf)
{
	buf.GetObjects(this);
	return buf.IsValid();
}
