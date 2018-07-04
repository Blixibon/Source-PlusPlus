#include "cbase.h"
#include "SteamCommon.h"
#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#endif
#include "filesystem.h"
#include "fmtstr.h"
#include "content_mounter.h"
#include "icommandline.h"
#include "scenefilecache\ISceneFileCache.h"

#include "KeyValues.h"

#ifdef _WIN32
#include "winlite.h"
#endif

#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "utlvector.h"

extern ISceneFileCache *scenefilecache;

#define GAMEINFOPATH_TOKEN		"|gameinfo_path|"
#define GAMEINFOPARENT_TOKEN	"|gameinfo_parent|"
#define SHAREDCONTENTPATH_TOKEN "|content_path|"

#define PATHID_SHARED			"SHARED"

static int SortStricmp(char * const * sz1, char * const * sz2)
{
	return V_stricmp(*sz1, *sz2);
}

const char *GetGameDir()
{
	static char gamePath[256];
#ifdef GAME_DLL
	engine->GetGameDir(gamePath, 256);
#elif defined(CLIENT_DLL)
	V_strncpy(gamePath, engine->GetGameDirectory(), sizeof(gamePath));
#endif
	return gamePath;
}

template <size_t maxLenInChars> bool GetDirPath(ISteamApps* const steamApps, IFileSystem* const pFileSystem, const char *pchPathName, OUT_Z_ARRAY char(&pchBuffer)[maxLenInChars])
{
	if (!steamApps || !pFileSystem)
		return false;

	if (Q_strcmp(pchPathName, GAMEINFOPARENT_TOKEN) == 0)
	{
		const char *pchGameDir = CommandLine()->ParmValue("-game", GetGameDir());
		if (!Q_IsAbsolutePath(pchGameDir))
		{
			pFileSystem->RelativePathToFullPath(pchGameDir, "MOD", pchBuffer, maxLenInChars);
		}
		else
		{
			V_strncpy(pchBuffer, pchGameDir, maxLenInChars);
		}

		V_StripLastDir(pchBuffer, maxLenInChars);
		V_StripTrailingSlash(pchBuffer);
	}
	else if (Q_strcmp(pchPathName, SHAREDCONTENTPATH_TOKEN) == 0)
	{
		char searchPaths[MAX_PATH*2];

		pFileSystem->GetSearchPath_safe(PATHID_SHARED, false, searchPaths);
		const char* sharedPath = strtok(searchPaths, ";");

		if (sharedPath == NULL)
			return false;

		Q_snprintf(pchBuffer, maxLenInChars, "%s" /*CORRECT_PATH_SEPARATOR_S*/ "content", sharedPath);
	}
	else if (Q_strcmp(pchPathName, "sourcemods") == 0)
	{
		HKEY hSteamKey;
		LONG lResult = VCRHook_RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hSteamKey);
		if (lResult == ERROR_SUCCESS)
		{
			unsigned long lBufferSize = maxLenInChars;
			lResult = VCRHook_RegQueryValueEx(hSteamKey, "SourceModInstallPath", NULL, NULL, (byte *)pchBuffer, &lBufferSize);
			if (lResult == ERROR_SUCCESS)
			{
				VCRHook_RegCloseKey(hSteamKey);
			}
		}

		if (lResult != ERROR_SUCCESS)
		{
			char error[256];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lResult, NULL, error, sizeof(error), NULL);
			AssertMsgAlways(false, (const tchar *)CFmtStr("MountExtraContent Registry Error: %s", error));
			return false;
		}
	}
	else
	{
		const int appId = V_atoi(pchPathName);

		if (!steamApps->BIsAppInstalled(appId))
			return false;

		steamApps->GetAppInstallDir(appId, pchBuffer, maxLenInChars);
	}

	return true;
}

void AddSearchPaths(IFileSystem* const pFileSystem, const char *path, SearchPathAdd_t addType, CUtlStringList const &pathIDs)
{
	for (int i = 0; i < pathIDs.Count(); i++)
	{
		pFileSystem->AddSearchPath(path, pathIDs.Element(i), addType);
	}
}

void MountKV(IFileSystem* const pFileSystem, KeyValues *pKVModDir, SearchPathAdd_t head, const char *path)
{
	const char* modDir = pKVModDir->GetName();

	if (FStrEq(modDir, GAMEINFOPATH_TOKEN))
		modDir = V_GetFileName(GetGameDir());

	CUtlStringList pathIDs;

	KeyValues *pkvPathIDs = pKVModDir->FindKey("path_id");

	if (!pkvPathIDs)
	{
		pathIDs.CopyAndAddToTail("GAME");
	}
	else
	{
		if (pkvPathIDs->GetBool("game", true))
			pathIDs.CopyAndAddToTail("GAME");

		for (KeyValues * kvValue = pkvPathIDs->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
		{
			if (FStrEq(kvValue->GetName(), "game"))
				continue;

			pathIDs.CopyAndAddToTail(kvValue->GetName());
		}
	}

	const CFmtStr mod("%s" CORRECT_PATH_SEPARATOR_S "%s", path, modDir);
	AddSearchPaths(pFileSystem, mod, head, pathIDs);

	FOR_EACH_VALUE(pKVModDir, pPath)
	{
		const char* const keyName = pPath->GetName();
		if (FStrEq(keyName, "vpk"))
		{
			const CFmtStr file("%s" CORRECT_PATH_SEPARATOR_S "%s.vpk", mod.Get(), pPath->GetString());
			AddSearchPaths(pFileSystem, file, head, pathIDs);
		}
		else if (FStrEq(keyName, "dir"))
		{
			const CFmtStr folder("%s" CORRECT_PATH_SEPARATOR_S "%s", mod.Get(), pPath->GetString());
			AddSearchPaths(pFileSystem, folder, head, pathIDs);
		}
		else if (FStrEq(keyName, "custom"))
		{
			const CFmtStr folder("%s" CORRECT_PATH_SEPARATOR_S "%s" CORRECT_PATH_SEPARATOR_S "*", mod.Get(), pPath->GetString());
			CUtlStringList vecFullLocationPaths;
			FileFindHandle_t findHandle = NULL;
			const char *pszFoundShortName = filesystem->FindFirst(folder, &findHandle);
			if (pszFoundShortName)
			{
				do
				{

					// We only know how to mount VPK's and directories
					if (pszFoundShortName[0] != '.' && (filesystem->FindIsDirectory(findHandle) || V_stristr(pszFoundShortName, ".vpk")))
					{
						char szAbsName[MAX_PATH];
						V_ExtractFilePath(folder, szAbsName, sizeof(szAbsName));
						V_AppendSlash(szAbsName, sizeof(szAbsName));
						V_strcat_safe(szAbsName, pszFoundShortName);

						vecFullLocationPaths.CopyAndAddToTail(szAbsName);

						// Check for a common mistake
						if (
							!V_stricmp(pszFoundShortName, "materials")
							|| !V_stricmp(pszFoundShortName, "maps")
							|| !V_stricmp(pszFoundShortName, "resource")
							|| !V_stricmp(pszFoundShortName, "scripts")
							|| !V_stricmp(pszFoundShortName, "sound")
							|| !V_stricmp(pszFoundShortName, "models"))
						{

							char szReadme[MAX_PATH];
							V_ExtractFilePath(folder, szReadme, sizeof(szReadme));
							V_AppendSlash(szReadme, sizeof(szReadme));
							V_strcat_safe(szReadme, "readme.txt");

							Error(
								"Tried to add %s as a search path.\n"
								"\nThis is probably not what you intended.\n"
								"\nCheck %s for more info\n",
								szAbsName, szReadme);
						}

					}
					pszFoundShortName = filesystem->FindNext(findHandle);
				} while (pszFoundShortName);
				filesystem->FindClose(findHandle);
			}

			// Sort alphabetically.  Also note that this will put
			// all the xxx_000.vpk packs just before the corresponding
			// xxx_dir.vpk
			vecFullLocationPaths.Sort(SortStricmp);

			// Now for any _dir.vpk files, remove the _nnn.vpk ones.
			int idx = vecFullLocationPaths.Count() - 1;
			while (idx > 0)
			{
				char szTemp[MAX_PATH];
				V_strcpy_safe(szTemp, vecFullLocationPaths[idx]);
				--idx;

				char *szDirVpk = V_stristr(szTemp, "_dir.vpk");
				if (szDirVpk != NULL)
				{
					*szDirVpk = '\0';
					while (idx >= 0)
					{
						char *pszPath = vecFullLocationPaths[idx];
						if (V_stristr(pszPath, szTemp) != pszPath)
							break;
						delete pszPath;
						vecFullLocationPaths.Remove(idx);
						--idx;
					}
				}
			}

			if (head == PATH_ADD_TO_HEAD)
			{
				// Mount them.
				FOR_EACH_VEC_BACK(vecFullLocationPaths, idxLocation)
				{
					AddSearchPaths(pFileSystem, vecFullLocationPaths[idxLocation], head, pathIDs);
				}
			}
			else
			{
				// Mount them.
				FOR_EACH_VEC(vecFullLocationPaths, idxLocation)
				{
					AddSearchPaths(pFileSystem, vecFullLocationPaths[idxLocation], head, pathIDs);
				}
			}
		}
		else if (!FStrEq(keyName, "path_id"))
			Warning("Unknown key \"%s\" in mounts\n", keyName);
	}

	//const char *pchModName = V_GetFileName(modDir);
	CFmtStr localization("resource/%s", modDir);
	localization.Append("_%language%.txt");
	g_pVGuiLocalize->AddFile(localization);
}

void MountSection(IFileSystem* const pFileSystem, KeyValues *pMounts)
{
	char path[MAX_PATH];
	ISteamApps* const steamApps = steamapicontext->SteamApps();

	FOR_EACH_TRUE_SUBKEY(pMounts, pMount)
	{
		if (FStrEq(pMount->GetName(), "deps"))
			continue;

		if (!GetDirPath(steamApps, pFileSystem, pMount->GetName(), path))
			continue;

		const SearchPathAdd_t head = pMount->GetBool("head") ? PATH_ADD_TO_HEAD : PATH_ADD_TO_TAIL;
		FOR_EACH_TRUE_SUBKEY(pMount, pModDir)
		{
			MountKV(pFileSystem, pModDir, head, path);
		}
	}
}

void MountContentFile(IFileSystem* const pFileSystem, const char *pchName);

void MountFile(IFileSystem* const pFileSystem, const char *pchFile, const char *pchPathId)
{
	KeyValuesAD pMounts("Mount");
	if (pMounts->LoadFromFile(pFileSystem, pchFile, pchPathId))
	{
		KeyValues *pkvContent = pMounts->FindKey("deps");

		if (pkvContent)
		{
			for (KeyValues * kvValue = pkvContent->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
			{
				MountContentFile(pFileSystem, kvValue->GetName());
			}
		}

		MountSection(pFileSystem, pMounts);
	}
}

CUtlSymbolTable g_MountedFiles;

void MountContentFile(IFileSystem* const pFileSystem, const char *pchName)
{
	if (g_MountedFiles.Find(pchName).IsValid())
		return;

	g_MountedFiles.AddString(pchName);

	char path[MAX_PATH];
	V_sprintf_safe(path, "mountlists/%s.txt", pchName);

	MountFile(pFileSystem, path, PATHID_SHARED);
}

void MountExtraContent()
{
	KeyValuesAD gameinfo("GameInfo");
	gameinfo->LoadFromFile(filesystem, "gameinfo.txt");

	KeyValues *pkvContent = gameinfo->FindKey("content");

	if (pkvContent)
	{
		for (KeyValues * kvValue = pkvContent->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
		{
			MountContentFile(filesystem, kvValue->GetName());
		}
	}

	// Mount shared base
	MountFile(filesystem, "base_dirs.txt", PATHID_SHARED);
	
	// Mount Mod
	KeyValues *pkvMounts = gameinfo->FindKey("mount");
	if (pkvMounts)
		MountSection(filesystem, pkvMounts);

	filesystem->MarkPathIDByRequestOnly(PATHID_SHARED, true);
	filesystem->MarkPathIDByRequestOnly("SCENES", true);

	scenefilecache->Reload();
}