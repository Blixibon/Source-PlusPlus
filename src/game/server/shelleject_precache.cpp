#include "cbase.h"
#include "filesystem.h"
#include "KeyValues.h"

void PrecacheNewShells()
{
	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char *fileName = "scripts/animevents/*.txt";
	char szFullFileName[MAX_PATH];
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "GAME", &findHandle);
	while (fileName)
	{
		char name[32];
		V_StripExtension(fileName, name, 32);
		Q_snprintf(szFullFileName, sizeof(szFullFileName), "scripts/animevents/%s", fileName);
		KeyValues *pKVFile = new KeyValues(name);
		if (pKVFile->LoadFromFile(filesystem, szFullFileName))
		{
			engine->PrecacheGeneric(szFullFileName);
			for (KeyValues * pkvEvent = pKVFile->GetFirstTrueSubKey(); pkvEvent != NULL; pkvEvent = pkvEvent->GetNextTrueSubKey())
			{
				CBaseEntity::PrecacheModel(pkvEvent->GetString("eject_model", "models/weapons/shell.mdl"));
			}
		}

		pKVFile->deleteThis();
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}
}