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

#define GAMEINFOPATH_TOKEN		"|gameinfo_path|"
#define GAMEINFOPARENT_TOKEN	"|gameinfo_parent|"
#define SHAREDPATH_PARENT		"|shared_parent|"
#define SHAREDCONTENTPATH_TOKEN "|content_path|"

#define PATHID_SHARED			"SHARED"

extern ISceneFileCache *scenefilecache;

namespace Mounter
{
	class ISteamAppInfoFinder
	{
	public:
		// returns current app install folder for AppID, returns folder name length
		virtual uint32 GetAppInstallDir(AppId_t appID, char* pchFolder, uint32 cchFolderBufferSize) = 0;
		virtual bool BIsAppInstalled(AppId_t appID) = 0; // returns true if that app is installed (not necessarily owned)

		virtual bool GetSourceModsDir(char* pchFolder, uint32 cchFolderBufferSize) = 0;
	};

	KeyValues *g_pWritePathsToHere = nullptr;

	CUtlFilenameSymbolTable g_FileNameTable;
	/*typedef struct {
		FileNameHandle_t file;
		bool	bSuper;
	} MountFileInfo_t;*/
	CUtlMap<FileNameHandle_t, bool> g_SuperFiles(DefLessFunc(FileNameHandle_t));
	CUtlVector<FileNameHandle_t> g_FilesToMount;

	CUtlStringList g_SentenceFiles;

	CUtlVector< char *, CUtlMemory< char *, int> > *GetSentenceFiles()
	{
		return &g_SentenceFiles;
	}

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

	template <size_t maxLenInChars> bool GetDirPath(ISteamAppInfoFinder* const steamApps, IFileSystem* const pFileSystem, const char *pchPathName, OUT_Z_ARRAY char(&pchBuffer)[maxLenInChars])
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
			char searchPaths[MAX_PATH * 2];

			pFileSystem->GetSearchPath_safe(PATHID_SHARED, false, searchPaths);
			const char* sharedPath = strtok(searchPaths, ";");

			if (sharedPath == NULL)
				return false;

			Q_snprintf(pchBuffer, maxLenInChars, "%s" /*CORRECT_PATH_SEPARATOR_S*/ "content", sharedPath);
		}
		else if (Q_strcmp(pchPathName, SHAREDPATH_PARENT) == 0)
		{
			char searchPaths[MAX_PATH * 2];

			pFileSystem->GetSearchPath_safe(PATHID_SHARED, false, searchPaths);
			const char* sharedPath = strtok(searchPaths, ";");

			if (sharedPath == NULL)
				return false;

			Q_strncpy(pchBuffer, sharedPath, maxLenInChars);
			Q_StripLastDir(pchBuffer, maxLenInChars);
			Q_StripTrailingSlash(pchBuffer);
		}
		else if (Q_strcmp(pchPathName, "sourcemods") == 0)
		{
			steamApps->GetSourceModsDir(pchBuffer, maxLenInChars);
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

		if (g_pWritePathsToHere != nullptr)
		{
			CFmtStr paths("%s", pathIDs[0]);
			for (int i = 1; i < pathIDs.Count(); i++)
			{
				paths.AppendFormat("+%s", pathIDs[i]);
			}

			KeyValues *pKV = new KeyValues(paths.Access());
			pKV->SetStringValue(path);
			if (addType == PATH_ADD_TO_TAIL)
			{
				g_pWritePathsToHere->AddSubKey(pKV);
			}
			else
			{
				KeyValues *pPeer = g_pWritePathsToHere->GetFirstSubKey()->MakeCopy();
				KeyValues *pNext = g_pWritePathsToHere->GetFirstSubKey()->GetNextKey();
				pPeer->SetNextKey(pNext);
				KeyValues &kvFirst = *g_pWritePathsToHere->GetFirstSubKey();
				kvFirst.SetNextKey(nullptr);
				kvFirst = *pKV;
				kvFirst.SetNextKey(pPeer);
				pKV->deleteThis();
			}
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

		// Check for "noroot" added specifically for mounting BM:S retail.
		if (!pKVModDir->GetBool("noroot"))
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
			else if (!FStrEq(keyName, "path_id") && !FStrEq(keyName, "noroot"))
				Warning("Unknown key \"%s\" in mounts\n", keyName);
		}

		const char *pchModName = V_GetFileName(modDir);
		CFmtStr localization("resource/%s", pchModName);
		localization.Append("_%language%.txt");
		g_pVGuiLocalize->AddFile(localization);
	}

	void MountSection(ISteamAppInfoFinder* const steamApps, IFileSystem* const pFileSystem, KeyValues *pMounts)
	{
		char path[MAX_PATH];

		FOR_EACH_TRUE_SUBKEY(pMounts, pMount)
		{
			if (FStrEq(pMount->GetName(), "deps"))
				continue;

			if (FStrEq(pMount->GetName(), "supers"))
				continue;

			if (FStrEq(pMount->GetName(), "sentence_files"))
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

	void MountFiles(ISteamAppInfoFinder* const steamApps, IFileSystem* const pFileSystem)
	{
		int iCount = g_FilesToMount.Count();
		for (int i = 0; i < iCount; i++)
		{
			char path[MAX_PATH];
			g_FileNameTable.String(g_FilesToMount.Element(i), path, MAX_PATH);

			KeyValuesAD pMounts("Mount");
			if (pMounts->LoadFromFile(pFileSystem, path, PATHID_SHARED))
			{
				KeyValues *pkvSentences = pMounts->FindKey("sentence_files");
				if (pkvSentences)
				{
					for (KeyValues * kvValue = pkvSentences->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
					{
						g_SentenceFiles.CopyAndAddToTail(kvValue->GetString());
					}
				}

				MountSection(steamApps, pFileSystem, pMounts);
			}
		}

		g_SuperFiles.Purge();
		g_FilesToMount.Purge();
		g_FileNameTable.RemoveAll();
	}

	void MarkContentFile(IFileSystem* const pFileSystem, const char *pchName, bool bSuper = false);

	void MarkFile(IFileSystem* const pFileSystem, const char *pchFile, /*const char *pchPathId,*/ bool bSuper = false)
	{
		KeyValuesAD pMounts("Mount");
		if (pMounts->LoadFromFile(pFileSystem, pchFile, PATHID_SHARED))
		{
			KeyValues *pkvContent = pMounts->FindKey("deps");

			if (pkvContent)
			{
				for (KeyValues * kvValue = pkvContent->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
				{
					MarkContentFile(pFileSystem, kvValue->GetName());
				}
			}

			//MountSection(pFileSystem, pMounts);
			FileNameHandle_t hFileName = g_FileNameTable.FindOrAddFileName(pchFile);
			int iIndex = g_FilesToMount.Find(hFileName);
			int iMIndex = g_SuperFiles.Find(hFileName);
			bool bExists = g_FilesToMount.IsValidIndex(iIndex);
			bool bWasSuper = bExists ? g_SuperFiles.Element(iMIndex) : false;
			if (!bExists || bWasSuper || bSuper)
			{
				if (!bExists)
				{
					iIndex = g_FilesToMount.AddToTail(hFileName);
				}
				else
				{
					g_FilesToMount.Remove(iIndex);
					iIndex = g_FilesToMount.AddToTail(hFileName);
				}

				iMIndex = g_SuperFiles.InsertOrReplace(hFileName, bSuper);
			}

			KeyValues *pkvSuper = pMounts->FindKey("supers");

			if (pkvSuper)
			{
				for (KeyValues * kvValue = pkvSuper->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
				{
					MarkContentFile(pFileSystem, kvValue->GetName(), true);
				}
			}
		}
	}

	//CUtlSymbolTable g_MountedFiles;

	void MarkContentFile(IFileSystem* const pFileSystem, const char *pchName, bool bSuper)
	{
		/*if (g_MountedFiles.Find(pchName).IsValid())
			return;

		g_MountedFiles.AddString(pchName);*/

		char path[MAX_PATH];
		V_sprintf_safe(path, "mountlists/%s.txt", pchName);

		MarkFile(pFileSystem, path, bSuper);
	}

	class CSteamClientAppInfoFinder : public ISteamAppInfoFinder
	{
	public:
		CSteamClientAppInfoFinder(ISteamApps* pApps)
		{
			m_pSteamApps = pApps;
		}

		// returns current app install folder for AppID, returns folder name length
		virtual uint32 GetAppInstallDir(AppId_t appID, char* pchFolder, uint32 cchFolderBufferSize)
		{
			return m_pSteamApps->GetAppInstallDir(appID, pchFolder, cchFolderBufferSize);
		}
		virtual bool BIsAppInstalled(AppId_t appID)
		{
			return m_pSteamApps->BIsAppInstalled(appID);
		}

		virtual bool GetSourceModsDir(char* pchFolder, uint32 cchFolderBufferSize)
		{
#ifdef _WIN32
			HKEY hSteamKey;
			LONG lResult = VCRHook_RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hSteamKey);
			if (lResult == ERROR_SUCCESS)
			{
				unsigned long lBufferSize = cchFolderBufferSize;
				lResult = VCRHook_RegQueryValueEx(hSteamKey, "SourceModInstallPath", NULL, NULL, (byte*)pchFolder, &lBufferSize);
				if (lResult == ERROR_SUCCESS)
				{
					VCRHook_RegCloseKey(hSteamKey);
				}
			}

			if (lResult != ERROR_SUCCESS)
			{
				char error[256];
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lResult, NULL, error, sizeof(error), NULL);
				AssertMsgAlways(false, (const tchar*)CFmtStr("MountExtraContent Registry Error: %s", error));
				return false;
			}
#else
#pragma warning "Not implemented!"
			return false;
#endif

			return true;
		}

	private:
		CSteamClientAppInfoFinder() {}
		ISteamApps* m_pSteamApps;
	};

	class CScriptAppInfoFinder : public ISteamAppInfoFinder
	{
	public:
		CScriptAppInfoFinder(IFileSystem* pApps)
		{
			m_pFileSystem = pApps;

			m_pKV = new KeyValues("ServerPaths");
			m_pKV->UsesEscapeSequences(false);
			m_pKV->LoadFromFile(m_pFileSystem, "dedicated_mount_paths.txt", "shared");
		}

		~CScriptAppInfoFinder()
		{
			if (m_pKV)
				m_pKV->deleteThis();
		}

		// returns current app install folder for AppID, returns folder name length
		virtual uint32 GetAppInstallDir(AppId_t appID, char* pchFolder, uint32 cchFolderBufferSize)
		{
			char buf[32];
			V_snprintf(buf, 32, "%i", appID);
			const char* pchPath = m_pKV->GetString(buf);

			V_strncpy(pchFolder, pchPath, cchFolderBufferSize);

			return V_strlen(pchPath);
		}
		virtual bool BIsAppInstalled(AppId_t appID)
		{
			char buf[32];
			V_snprintf(buf, 32, "%i", appID);

			return (m_pKV->FindKey(buf) != nullptr);
		}
		virtual bool GetSourceModsDir(char* pchFolder, uint32 cchFolderBufferSize)
		{
			if (!m_pKV->FindKey("sourcemods"))
				return false;

			V_strncpy(pchFolder, m_pKV->GetString("sourcemods"), cchFolderBufferSize);
			return true;
		}

	private:
		CScriptAppInfoFinder() {}
		IFileSystem* m_pFileSystem;
		KeyValues* m_pKV;
	};

	void MountExtraContent()
	{
		KeyValuesAD gameinfo("GameInfo");
		gameinfo->LoadFromFile(filesystem, "gameinfo.txt");

		KeyValues *pKVHammer = nullptr;
#ifdef CLIENT_DLL
		if (CommandLine()->FindParm("-hammerprep"))
		{
			pKVHammer = gameinfo->MakeCopy();
			g_pWritePathsToHere = pKVHammer->FindKey("FileSystem", true)->FindKey("SearchPaths", true);
			Warning("[CLIENT] Writing search paths for hammer...\n");
		}
#endif

		KeyValues *pkvContent = gameinfo->FindKey("content");

		if (pkvContent)
		{
			for (KeyValues * kvValue = pkvContent->GetFirstValue(); kvValue != NULL; kvValue = kvValue->GetNextValue())
			{
				MarkContentFile(filesystem, kvValue->GetName());
			}
		}

		// Mount shared base
		MarkFile(filesystem, "base_dirs.txt");

		ISteamAppInfoFinder* steamApps = nullptr;

#if defined(GAME_DLL) || defined(CLIENT_DLL)
#ifdef GAME_DLL
		if (engine->IsDedicatedServer())
		{
			steamApps = new CScriptAppInfoFinder(filesystem);
		}
		else
#endif
		{
			steamApps = new CSteamClientAppInfoFinder(steamapicontext->SteamApps());
		}
#endif

		MountFiles(steamApps, filesystem);

		// Mount Mod
		KeyValues *pkvMounts = gameinfo->FindKey("mount");
		if (pkvMounts)
			MountSection(steamApps, filesystem, pkvMounts);

		filesystem->MarkPathIDByRequestOnly(PATHID_SHARED, true);
		filesystem->MarkPathIDByRequestOnly("SCENES", true);

		if (g_pWritePathsToHere != nullptr)
			g_pWritePathsToHere = nullptr;

		if (steamApps != nullptr)
		{
			delete steamApps;
			steamApps = nullptr;
		}

		if (pKVHammer != nullptr)
		{
			pKVHammer->SaveToFile(filesystem, "gameinfo_hammer.txt", "MOD_WRITE");
			pKVHammer->deleteThis();
			pKVHammer = nullptr;

			Warning("Finished writing search paths to 'gameinfo_hammer.txt'\n");
		}

		//scenefilecache->Reload();
	}
}