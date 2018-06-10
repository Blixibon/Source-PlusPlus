#include "cbase.h"
#include "SteamCommon.h"
#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#include "particlemgr.h"
#endif
#include "filesystem.h"
#include "fmtstr.h"
#include "content_mounter.h"
#include "icommandline.h"
#include "scenefilecache\ISceneFileCache.h"

#ifdef _WIN32
#include "winlite.h"
#endif

#include "tier3/tier3.h"
#include "vgui/ILocalize.h"

extern ISceneFileCache *scenefilecache;

#define GAMEINFOPATH_TOKEN		"|gameinfo_path|"
#define BASESOURCEPATHS_TOKEN	"|all_source_engine_paths|"

const char *GetGameDir()
{
	static char gamePath[256];
#ifdef GAME_DLL
	engine->GetGameDir(gamePath, 256);
#else
	V_strncpy(gamePath, engine->GetGameDirectory(), sizeof(gamePath));
#endif
	return gamePath;
}

static int SortStricmp(char * const * sz1, char * const * sz2)
{
	return V_stricmp(*sz1, *sz2);
}

void MountExtraContent()
{
	KeyValuesAD gameinfo("GameInfo");
	gameinfo->LoadFromFile(filesystem, "gameinfo.txt");


	char path[MAX_PATH];
	ISteamApps* const steamApps = steamapicontext->SteamApps();
	if ( KeyValues* pMounts = gameinfo->FindKey( "mount" ) )
	{
		FOR_EACH_TRUE_SUBKEY( pMounts, pMount )
		{
			if (Q_strcmp(pMount->GetName(), "gameinfoparent") == 0)
			{
				const char *pchGameDir = CommandLine()->ParmValue("-game", GetGameDir());
				if (!Q_IsAbsolutePath(pchGameDir))
				{
					g_pFullFileSystem->RelativePathToFullPath_safe(pchGameDir, "MOD", path);
				}
				else
				{
					V_strncpy(path, pchGameDir, sizeof(path));
				}

				V_StripLastDir(path, sizeof(path));
				V_StripTrailingSlash(path);
			}
			else if (Q_strcmp(pMount->GetName(), "sourcemods") == 0)
			{
				HKEY hSteamKey;
				LONG lResult = VCRHook_RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hSteamKey);
				if (lResult == ERROR_SUCCESS)
				{
					unsigned long lBufferSize = sizeof(path);
					lResult = VCRHook_RegQueryValueEx(hSteamKey, "SourceModInstallPath", NULL, NULL, (byte *)path, &lBufferSize);
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
				}
			}
			else
			{
				const int appId = V_atoi(pMount->GetName());

				if (!steamApps->BIsAppInstalled(appId))
					continue;

				steamApps->GetAppInstallDir(appId, path, sizeof(path));
			}
			const SearchPathAdd_t head = pMount->GetBool( "head" ) ? PATH_ADD_TO_HEAD : PATH_ADD_TO_TAIL;
			FOR_EACH_TRUE_SUBKEY( pMount, pModDir )
			{
				const char* modDir = pModDir->GetName();

				if (FStrEq(modDir, GAMEINFOPATH_TOKEN))
					modDir = V_GetFileName(GetGameDir());

				const CFmtStr mod( "%s" CORRECT_PATH_SEPARATOR_S "%s", path, modDir );
				filesystem->AddSearchPath( mod, "GAME", head );
				filesystem->AddSearchPath( mod, "MOD", head );
				FOR_EACH_VALUE( pModDir, pPath )
				{
					const char* const keyName = pPath->GetName();
					if ( FStrEq( keyName, "vpk" ) )
					{
						const CFmtStr file( "%s" CORRECT_PATH_SEPARATOR_S "%s.vpk", mod.Get(), pPath->GetString() );
						filesystem->AddSearchPath( file, "GAME", head );
						filesystem->AddSearchPath( file, "MOD", head );
					}
					else if ( FStrEq( keyName, "dir" ) )
					{
						const CFmtStr folder( "%s" CORRECT_PATH_SEPARATOR_S "%s", mod.Get(), pPath->GetString() );
						filesystem->AddSearchPath( folder, "GAME", head );
						filesystem->AddSearchPath( folder, "MOD", head );
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
								filesystem->AddSearchPath(vecFullLocationPaths[idxLocation], "GAME", head);
								filesystem->AddSearchPath(vecFullLocationPaths[idxLocation], "MOD", head);
							}
						}
						else
						{
							// Mount them.
							FOR_EACH_VEC(vecFullLocationPaths, idxLocation)
							{
								filesystem->AddSearchPath(vecFullLocationPaths[idxLocation], "GAME", head);
								filesystem->AddSearchPath(vecFullLocationPaths[idxLocation], "MOD", head);
							}
						}
					}
					else
						Warning( "Unknown key \"%s\" in mounts\n", keyName );
				}

				//const char *pchModName = V_GetFileName(modDir);
				CFmtStr localization( "resource/%s", modDir);
				localization.Append( "_%language%.txt" );
				g_pVGuiLocalize->AddFile( localization );
			}
		}
	}

#ifdef CLIENT_DLL
	//Re-init the particle manager
	ParticleMgr()->Init(MAX_TOTAL_PARTICLES, materials);
#endif
	scenefilecache->Reload();
}