#include "cbase.h"
#include "population_manager.h"
#include "gametypes.h"
#include "utlbuffer.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "world.h"

#define SCRIPT_DIR "scripts/population/"
#define DEFAULT_TAG "default"

#define FILENAME_FORMAT SCRIPT_DIR "%s/%s.txt"


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

void CPopulationDefinition::PostInit()
{
	g_pPopulationManager->AddDefinition(this);
}

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
		const char *pchTag = static_cast<CWorld *>(pEntity)->GetPopulationTag();
		if (pchTag == nullptr)
			pchTag = g_pGameTypeSystem->GetFirstArea();

		if (FStrEq(pchTag, ""))
		{
			m_iszPopulationTag = AllocPooledString(DEFAULT_TAG);
		}
		else
		{
			m_iszPopulationTag = AllocPooledString(pchTag);
		}

		for (int i = 0; i < m_Definitions.Count(); i++)
		{
			CPopulationDefinition *pDef = m_Definitions[i];
			pDef->weighted_random.Purge();

			CFmtStrN<MAX_PATH> filename;
			filename.AppendFormat(FILENAME_FORMAT, STRING(m_iszPopulationTag), pDef->chName);


			if (!filesystem->FileExists(filename.Access(), "GAME"))
			{
				bool bFound = false;
				char chPopTag[MAX_PATH];
				Q_strncpy(chPopTag, STRING(m_iszPopulationTag), MAX_PATH);
				
				while (!bFound && V_StripLastDir(chPopTag, MAX_PATH))
				{
					char chPopTagNoSlash[MAX_PATH];
					Q_strncpy(chPopTagNoSlash, chPopTagNoSlash, MAX_PATH);

					V_StripTrailingSlash(chPopTagNoSlash);

					filename.Clear();
					filename.AppendFormat(FILENAME_FORMAT, chPopTagNoSlash, pDef->chName);

					// Check again.
					if (filesystem->FileExists(filename.Access(), "GAME"))
						bFound = true;
				}
				
				if (!bFound)
				{
					// Try the default tag
					filename.Clear();
					filename.AppendFormat(FILENAME_FORMAT, DEFAULT_TAG, pDef->chName);

					// Check again.
					if (filesystem->FileExists(filename.Access(), "GAME"))
						bFound = true;
				}

				if (!bFound)
					continue;
			}

			

			CUtlStringList typeList;

			RawLoadFileIntoVector(filename.Access(), typeList);

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

CPopulationControl g_PopulationManager;
CPopulationControl *g_pPopulationManager = &g_PopulationManager;