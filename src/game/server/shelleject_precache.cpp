#include "cbase.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "particle_parse.h"

void PrecacheNewShells()
{
	FileFindHandle_t findHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	const char *fileName = "scripts/animevents/*.txt";
	char szFullFileName[MAX_PATH];
	fileName = g_pFullFileSystem->FindFirstEx(fileName, "GAME", &findHandle);
	while (fileName)
	{
		if (!FStrEq(fileName, "readme.txt"))
		{
			char name[32];
			V_StripExtension(fileName, name, 32);
			Q_snprintf(szFullFileName, sizeof(szFullFileName), "scripts/animevents/%s", fileName);
			KeyValues* pKVFile = new KeyValues(name);
			if (pKVFile->LoadFromFile(filesystem, szFullFileName))
			{
				AddFileToDownloadTable(szFullFileName);
				for (KeyValues* pkvEvent = pKVFile->GetFirstTrueSubKey(); pkvEvent != NULL; pkvEvent = pkvEvent->GetNextTrueSubKey())
				{
					CBaseEntity::PrecacheModel(pkvEvent->GetString("eject_model", "models/weapons/shell.mdl"));
					PrecacheParticleSystem(pkvEvent->GetString("shell_particle_system"));
				}
			}

			pKVFile->deleteThis();
		}
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);
}