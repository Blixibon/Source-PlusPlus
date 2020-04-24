//-----------------------------------------------------------------------
//	The population manager: a system to perform weighted randomization
//	of server-side game elements based on a 'tag' string in the level's
//	'world' entity.
//
//	Author: Petercov (petercov@outlook.com)
//	Created: August 22, 2018
//-----------------------------------------------------------------------

#include "cbase.h"
#include "population_manager.h"
#include "gametypes.h"
#include "utlbuffer.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "world.h"

#define SCRIPT_DIR "scripts/population/"
#define DEFAULT_TAG "default"

#define FILENAME_FORMAT SCRIPT_DIR "%s/%s/%s.txt"


int CPopulationDefinition::GetRandom()
{
	if (weighted_random.Count() > 0)
	{
		return weighted_random.Element(RandomInt(0, weighted_random.Count()-1));
	}
	else
	{
		return RandomInt(0, types.Count()-1);
	}
}

bool CPopulationDefinition::Init()
{
	g_pPopulationManager->AddDefinition(this);

	return true;
}

#pragma region UTILS
//-----------------------------------------------
// Helper functions copied from code by Valve.
//-----------------------------------------------

// Strip ' ' and '\n' characters from string.
static void StripWhitespaceChars(char *szBuffer)
{
	char *szOut = szBuffer;

	for (char *szIn = szOut; *szIn; szIn++)
	{
		if (*szIn != ' ' && *szIn != '\r')
			*szOut++ = *szIn;
	}
	*szOut = '\0';
}

void RawLoadFileIntoVector(const char *pszMapCycleFile, CUtlVector<char *> &mapList)
{
	CUtlBuffer buf;
	if (!filesystem->ReadFile(pszMapCycleFile, "GAME", buf))
		return;
	buf.PutChar(0);
	V_SplitString((char*)buf.Base(), "\n", mapList);

	for (int i = 0; i < mapList.Count(); i++)
	{
		bool bIgnore = false;

		// Strip out ' ' and '\r' chars.
		StripWhitespaceChars(mapList[i]);

		if (!Q_strncmp(mapList[i], "//", 2) || mapList[i][0] == '\0')
		{
			bIgnore = true;
		}

		if (bIgnore)
		{
			delete[] mapList[i];
			mapList.Remove(i);
			--i;
		}
	}
}

#pragma endregion

#pragma region MANAGER
//----------------------------------------------------
// The manager class. Reads population scripts and
// weights the elements of each definition instance.
//----------------------------------------------------

void CPopulationControl::LevelInitPostEntity()
{
	gEntList.RemoveListenerEntity(this);

	if (gpGlobals->eLoadType != MapLoad_NewGame)
	{
		OnEntitySpawned(GetWorldEntity());
	}
}

void CPopulationControl::OnEntitySpawned(CBaseEntity *pEntity)
{
	if (FClassnameIs(pEntity, "worldspawn"))
	{
		// Get the tag string from the world entity.
		const char *pchTag = static_cast<CWorld *>(pEntity)->GetPopulationTag();

		// If it's null, ask the gametype system for the area it got from the level's name.
		if (pchTag == nullptr)
			pchTag = g_pGameTypeSystem->GetFirstArea();

		if (FStrEq(pchTag, ""))
		{
			// We got nothing. Use the default.
			m_iszPopulationTag = AllocPooledString(DEFAULT_TAG);
		}
		else
		{
			m_iszPopulationTag = AllocPooledString(pchTag);
		}

		const char* pszPopSet = g_pGameTypeSystem->GetPopulationSet();

		// Iterate through every definition instance we know about.
		for (int i = 0; i < m_Definitions.Count(); i++)
		{
			CPopulationDefinition *pDef = m_Definitions[i];
			pDef->weighted_random.Purge(); // Purge any previous weighting.

			CFmtStrN<MAX_PATH> filename;
			filename.AppendFormat(FILENAME_FORMAT, pszPopSet, STRING(m_iszPopulationTag), pDef->chName); // Build the filename for the weighting script.

			// Check if it exists.
			if (!filesystem->FileExists(filename.Access(), "GAME"))
			{
				bool bFound = false;
				char chPopTag[MAX_PATH];
				Q_strncpy(chPopTag, STRING(m_iszPopulationTag), MAX_PATH);

				// It wasn't found, so go up the directory tree and look for it.
				while (!bFound && V_StripLastDir(chPopTag, MAX_PATH))
				{
					char chPopTagNoSlash[MAX_PATH];
					Q_strncpy(chPopTagNoSlash, chPopTag, MAX_PATH);

					V_StripTrailingSlash(chPopTagNoSlash);

					filename.Clear();
					filename.AppendFormat(FILENAME_FORMAT, pszPopSet, chPopTagNoSlash, pDef->chName);

					// Check again.
					if (filesystem->FileExists(filename.Access(), "GAME"))
						bFound = true;
				}

				// Still nothing; use the default.
				if (!bFound)
				{
					// Try the default tag
					filename.Clear();
					filename.AppendFormat(FILENAME_FORMAT, DEFAULT_TAG, STRING(m_iszPopulationTag), pDef->chName);

					// Check again.
					if (!filesystem->FileExists(filename.Access(), "GAME"))
					{
						bool bFound = false;
						char chPopTag[MAX_PATH];
						Q_strncpy(chPopTag, STRING(m_iszPopulationTag), MAX_PATH);

						// It wasn't found, so go up the directory tree and look for it.
						while (!bFound && V_StripLastDir(chPopTag, MAX_PATH))
						{
							char chPopTagNoSlash[MAX_PATH];
							Q_strncpy(chPopTagNoSlash, chPopTag, MAX_PATH);

							V_StripTrailingSlash(chPopTagNoSlash);

							filename.Clear();
							filename.AppendFormat(FILENAME_FORMAT, DEFAULT_TAG, chPopTagNoSlash, pDef->chName);

							// Check again.
							if (filesystem->FileExists(filename.Access(), "GAME"))
								bFound = true;
						}
					}
					else
					{
						bFound = true;
					}
				}

				if (!bFound)
					continue;
			}



			CUtlStringList typeList;

			RawLoadFileIntoVector(filename.Access(), typeList); // Load the file.

			if (typeList.Count() <= 0)
				continue;

			for (int j = 0; j < typeList.Count(); j++)
			{
				char *pchType = typeList[j];
				for (int k = 0; k < pDef->types.Count(); k++)
				{
					if (FStrEq(pchType, pDef->types[k]))
					{
						pDef->weighted_random.AddToTail(k);
						break;
					}
				}
			}
		}
	}
}
#pragma endregion


CPopulationControl g_PopulationManager;
CPopulationControl *g_pPopulationManager = &g_PopulationManager;