#include "cbase.h"
#include "fmtstr.h"
#include "ent_create_completion.h"
#include "fgdlib/fgdlib.h"
#include "igamesystem.h"
#include "filesystem.h"

class CGameDataLoader : public CAutoGameSystem
{
public:
	CGameDataLoader() : CAutoGameSystem("FGDLoader")
	{}

	bool Init()
	{
		GDSetMessageFunc(DevWarning);

		KeyValuesAD kvGameInfo("GameInfo");
		if (kvGameInfo->LoadFromFile(filesystem, "gameinfo.txt", "MOD"))
		{
			const char *pszFileName = kvGameInfo->GetString("GameData", nullptr);
			if (pszFileName)
			{
				const char *ppszRelativePaths[] = {
					".",
					"..",
					"../bin",
				};

				char filePath[MAX_PATH];
				char gamedir[MAX_PATH];
				engine->GetGameDir(gamedir, MAX_PATH);

				bool bFound = false;
				for (int i = 0; i < ARRAYSIZE(ppszRelativePaths); i++)
				{
					char searchpath[MAX_PATH];
					V_ComposeFileName(gamedir, ppszRelativePaths[i], searchpath, MAX_PATH);
					V_ComposeFileName(searchpath, pszFileName, filePath, MAX_PATH);
					if (filesystem->FileExists(filePath))
					{
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					filesystem->RelativePathToFullPath_safe(pszFileName, "EXECUTABLE_PATH", filePath, FILTER_CULLPACK);
					if (filesystem->FileExists(filePath))
						bFound = true;
				}

				if (bFound)
				{
					m_FGD.Load(filePath);
				}
			}
		}

		return true;
	}

	GameData *GetData() { return &m_FGD; }

private:
	GameData m_FGD;
};

CGameDataLoader g_GameDataLoader;

CEntCreateCompletionFunctor::CEntCreateCompletionFunctor(const char * pszCommand, IMapEntityFilter * pFilter)
{
	m_pFilter = pFilter;
	V_strncpy(m_cCommand, pszCommand, COMMAND_COMPLETION_ITEM_LENGTH);
}

int CEntCreateCompletionFunctor::CommandCompletionCallback(const char * partial, CUtlVector<CUtlString>& commands)
{
	int current = 0;

	const char *cmdname = m_cCommand;
	char *substring = NULL;
	int substringLen = 0;
	if (Q_strstr(partial, cmdname))
	{
		substring = (char *)partial + strlen(cmdname) + 1;
		substringLen = Q_strlen(substring);
	}

	char *space = Q_strstr(substring, " ");
	if (space)
	{
		int iLen = space - substring;
		CUtlString classname(substring, iLen);
		GDclass *pClass = g_GameDataLoader.GetData()->ClassForName(classname.Get());
		if (pClass)
			return EntCreate_KVCompletion(pClass, partial, commands);
	}

	for (int i = EntityFactoryDictionary()->CountFactories() - 1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i--)
	{
		const char *pSoundName = EntityFactoryDictionary()->GetFactoryName(i);
		if (pSoundName)
		{
			if (!m_pFilter || m_pFilter->ShouldCreateEntity(pSoundName))
			{
				if (!substring || !Q_strncasecmp(pSoundName, substring, substringLen))
				{
					CUtlString command;
					command = CFmtStr("%s %s", cmdname, pSoundName);
					commands.AddToTail(command);
					current++;
				}
			}
		}
	}

	return current;
}

int CEntCreateCompletionFunctor::EntCreate_KVCompletion(GDclass *pClass, const char * partial, CUtlVector<CUtlString>& commands)
{
	int current = 0;

	const char *cmdname = m_cCommand;
	char *substring = (char *)partial;
	if (Q_strstr(partial, pClass->GetName()))
	{
		substring = (char *)Q_strstr(partial, pClass->GetName()) + strlen(pClass->GetName()) + 1;
	}

	CParameterList tokens;
	V_SplitString(substring, " ", tokens);
	if (substring[strlen(substring) - 1] == ' ')
	{
		char *pchSpace = new char[1];
		pchSpace[0] = 0;
		tokens.AddToTail(pchSpace);
	}

	if (tokens.Count() > 0)
	{
		bool bSearchKeys = (0 == (tokens.Count() % 2)) ? false : true;

		if (bSearchKeys)
		{
			substring = tokens.Tail();
			int substringLen = V_strlen(substring);
			for (int i = pClass->GetVariableCount() - 1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i--)
			{
				GDinputvariable *pVar = pClass->GetVariableAt(i);
				if (!pVar->IsReadOnly() && (substring[0] == 0 || !Q_strncasecmp(pVar->GetName(), substring, substringLen)))
				{
					CUtlString command;
					command = CFmtStr("%s %s", cmdname, pClass->GetName());
					for (int j = 0; j < tokens.Count() - 1; j++)
					{
						command.Append(CFmtStr(" %s", tokens[j]));
					}
					command.Append(CFmtStr(" %s", pVar->GetName()));
					commands.AddToTail(command);
					current++;
				}
			}
		}
		else
		{
			substring = tokens.Tail();
			int substringLen = V_strlen(substring);
			GDinputvariable *pVar = pClass->VarForName(tokens[tokens.Count() - 2]);
			if (pVar && pVar->GetType() == ivChoices)
			{
				for (int i = pVar->GetChoiceCount() - 1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i--)
				{
					if (substring[0] == 0 || !Q_strncasecmp(pVar->ItemValueForString(pVar->GetChoiceCaption(i)), substring, substringLen))
					{
						CUtlString command;
						command = CFmtStr("%s %s", cmdname, pClass->GetName());
						for (int j = 0; j < tokens.Count() - 2; j++)
						{
							command.Append(CFmtStr(" %s", tokens[j]));
						}
						command.Append(CFmtStr(" %s", pVar->GetName()));
						command.Append(CFmtStr(" %s", pVar->ItemValueForString(pVar->GetChoiceCaption(i))));
						commands.AddToTail(command);
						current++;
					}
				}
			}
		}
	}
	else
	{
		for (int i = pClass->GetVariableCount() - 1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i--)
		{
			GDinputvariable *pVar = pClass->GetVariableAt(i);
			if (!pVar->IsReadOnly())
			{
				CUtlString command;
				command = CFmtStr("%s %s %s", cmdname, pClass->GetName(), pVar->GetName());
				commands.AddToTail(command);
				current++;
			}
		}
	}

	tokens.PurgeAndDeleteElements();
	return current;
}
