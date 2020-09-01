#include "cbase.h"
#include "character_manifest_system.h"
#include "checksum_crc.h"
#include "animation.h"

using namespace CharacterManifest;

#define CHARACTER_MANIFEST_FILE "scripts/character_manifest.txt"

IMPLEMENT_PRIVATE_SYMBOLTYPE(CManifestSymbol);

CharacterManifest::CCharacterManifestSystem::CCharacterManifestSystem() : CAutoGameSystem("CharacterManifestSystem")
{
}

bool CharacterManifest::CCharacterManifestSystem::Init()
{
	KeyValues* manifest = new KeyValues(CHARACTER_MANIFEST_FILE);
	if (manifest->LoadFromFile(filesystem, CHARACTER_MANIFEST_FILE, "MOD"))
	{
		for (KeyValues* sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey())
		{
			if (!Q_stricmp(sub->GetName(), "file"))
			{
				LoadDataFromFile(sub->GetString());
				continue;
			}

			Warning("CCharacterManifestSystem::Init:  Manifest '%s' with bogus file type '%s', expecting 'file'\n",
				CHARACTER_MANIFEST_FILE, sub->GetName());
		}
	}

	manifest->deleteThis();

	return true;
}

void CharacterManifest::CCharacterManifestSystem::Shutdown()
{
	m_ManifestDict.Purge();
}

const ManifestCharacter_t* CharacterManifest::CCharacterManifestSystem::FindCharacterModel(const char* pszCharName) const
{
	int iElement = m_ManifestDict.Find(pszCharName);
	if (m_ManifestDict.IsValidIndex(iElement))
	{
		auto& List = m_ManifestDict.Element(iElement);
		if (List.Count())
		{
			return &List.Random();
		}
	}

	return nullptr;
}

const ManifestCharacter_t* CharacterManifest::CCharacterManifestSystem::FindCharacterModel(const char* pszCharName, CUtlVector<CManifestSymbol>* pvIncludeTags, CUtlVector<CManifestSymbol>* pvExcludeTags, CUtlVector<CManifestSymbol>* pvPreferTags) const
{
	int iElement = m_ManifestDict.Find(pszCharName);
	if (m_ManifestDict.IsValidIndex(iElement))
	{
		auto& List = m_ManifestDict.Element(iElement);
		if (List.Count())
		{
			characterPtrList_t vFilteredList;
			characterPtrList_t vFilteredListHigh;
			for (auto& Model : List)
			{
				bool bValid = (Model.vModelTags.Count() > 0) || !pvIncludeTags || pvIncludeTags->Count() == 0;
				bool bLowPriority = true;
				for (auto& tag : Model.vModelTags)
				{
					if (pvIncludeTags && pvIncludeTags->Count())
					{
						int iIndex = pvIncludeTags->Find(tag);
						if (iIndex == pvIncludeTags->InvalidIndex())
						{
							bValid = false;
							break;
						}
					}

					if (pvExcludeTags && pvExcludeTags->Count())
					{
						int iIndex = pvExcludeTags->Find(tag);
						if (iIndex != pvExcludeTags->InvalidIndex())
						{
							bValid = false;
							break;
						}
					}

					if (pvPreferTags && pvPreferTags->Count())
					{
						int iIndex = pvPreferTags->Find(tag);
						if (iIndex != pvPreferTags->InvalidIndex())
						{
							bLowPriority = false;
						}
					}
				}

				if (bValid)
				{
					vFilteredList.AddToTail(&Model);
					if (!bLowPriority)
						vFilteredListHigh.AddToTail(&Model);
				}
			}

			if (vFilteredListHigh.Count())
			{
				return vFilteredListHigh.Random();
			}
			else if (vFilteredList.Count())
			{
				return vFilteredList.Random();
			}
		}
	}

	return nullptr;
}

void CharacterManifest::CCharacterManifestSystem::PrecacheCharacterModels(const char* pszCharName) const
{
	int iElement = m_ManifestDict.Find(pszCharName);
	if (m_ManifestDict.IsValidIndex(iElement))
	{
		auto& List = m_ManifestDict.Element(iElement);
		for (int i = 0; i < List.Count(); i++)
		{
			auto& model = List[i];
			CBaseEntity::PrecacheModel(GetScriptModel(&model));
		}
	}
}

void CharacterManifest::CCharacterManifestSystem::LoadDataFromFile(const char* pszFileName)
{
	KeyValuesAD pKV("character_manifest");
	if (pKV->LoadFromFile(filesystem, pszFileName, "GAME"))
	{
		for (KeyValues* pkvCharDef = pKV->GetFirstTrueSubKey(); pkvCharDef; pkvCharDef = pkvCharDef->GetNextTrueSubKey())
		{
			const char* pszName = pkvCharDef->GetName();
			int iElement = m_ManifestDict.Find(pszName);
			if (!m_ManifestDict.IsValidIndex(iElement))
			{
				iElement = m_ManifestDict.Insert(pszName);
			}

			if (!m_ManifestDict.IsValidIndex(iElement))
				continue;

			auto& List = m_ManifestDict.Element(iElement);
			auto& data = List[List.AddToTail()];

			data.strModelName = pkvCharDef->GetString("model");
			const char* pszSkin = pkvCharDef->GetString("skin", nullptr);
			if (pszSkin)
			{
				const char* pchToken = pszSkin;
				char		szToken[8];

				pchToken = nexttoken(szToken, pchToken, ',');

				while (pchToken)
				{
					if (szToken[0])
					{
						CUtlStringList strings;
						V_SplitString(szToken, ":", strings);
						if (strings.Count() == 2)
						{
							for (int i = atoi(strings[0]); i <= atoi(strings[1]); i++)
							{
								data.vSkins.AddToTail(i);
							}
						}
						else
						{
							int iSkin = atoi(szToken);
							data.vSkins.AddToTail(iSkin);
						}
					}

					pchToken = nexttoken(szToken, pchToken, ',');
				}
			}

			KeyValues* pkvBodyGroups = pkvCharDef->FindKey("bodygroup_data");
			if (pkvBodyGroups)
			{
				for (KeyValues* pkvGroup = pkvBodyGroups->GetFirstValue(); pkvGroup; pkvGroup = pkvGroup->GetNextValue())
				{
					auto& Body = data.vBodyGroups[data.vBodyGroups.AddToTail()];
					Body.strName = pkvGroup->GetName();
					const char* pchToken = pkvGroup->GetString();
					char		szToken[8];

					pchToken = nexttoken(szToken, pchToken, ',');

					while (pchToken)
					{
						if (szToken[0])
						{
							CUtlStringList strings;
							V_SplitString(szToken, ":", strings);
							if (strings.Count() == 2)
							{
								for (int i = atoi(strings[0]); i <= atoi(strings[1]); i++)
								{
									Body.vValues.AddToTail(i);
								}
							}
							else
							{
								int iBody = atoi(szToken);
								Body.vValues.AddToTail(iBody);
							}
						}

						pchToken = nexttoken(szToken, pchToken, ',');
					}
				}
			}

			KeyValues* pkvBodySync = pkvCharDef->FindKey("bodygroup_sync");
			if (pkvBodySync)
			{
				for (KeyValues* pkvSync = pkvBodySync->GetFirstValue(); pkvSync; pkvSync = pkvSync->GetNextValue())
				{
					auto& bodySync = data.vBodyGroupSync[data.vBodyGroupSync.AddToTail()];
					bodySync.srcName = pkvSync->GetName();
					bodySync.dstName = pkvSync->GetString();
				}
			}

			KeyValues* pkvFlexData = pkvCharDef->FindKey("flex_data");
			if (pkvFlexData)
			{
				for (KeyValues* pkvControl = pkvFlexData->GetFirstValue(); pkvControl; pkvControl = pkvControl->GetNextValue())
				{
					auto& flex = data.vFlexControllers[data.vFlexControllers.AddToTail()];
					V_strncpy(flex.cName, pkvControl->GetName(), 16);
					flex.flValue = pkvControl->GetFloat();
				}
			}

			KeyValues* pkvMergeData = pkvCharDef->FindKey("bonemerge_data");
			if (pkvMergeData)
			{
				for (KeyValues* pkvMergeModel = pkvMergeData->GetFirstSubKey(); pkvMergeModel; pkvMergeModel = pkvMergeModel->GetNextKey())
				{
					data.vMergedModels.AddToTail(pkvMergeModel->GetName());
				}
			}

			KeyValues* pkvModelTags = pkvCharDef->FindKey("tags");
			if (pkvModelTags)
			{
				for (KeyValues* pkvTag = pkvModelTags->GetFirstSubKey(); pkvTag; pkvTag = pkvTag->GetNextKey())
				{
					data.vModelTags.AddToTail(pkvTag->GetName());
				}
			}
		}
	}
}

CCharacterManifestSystem g_ManifestSystem;
CharacterManifest::CCharacterManifestSystem* GetCharacterManifest()
{
	return &g_ManifestSystem;
}

CRC32_t CharacterManifest::EncodeFlexVector(CUtlVector<ManifestFlexData_t>& vData, CUtlBuffer& buf)
{
	CRC32_t crc;
	CRC32_Init(&crc);
	for (int i = 0; i < vData.Count(); i++)
	{
		auto& element = vData.Element(i);
		element.Serialize(buf);
		CRC32_ProcessBuffer(&crc, &element, sizeof(element));
	}
	CRC32_Final(&crc);
	return crc;
}

bool CharacterManifest::ManifestCharacter_t::ApplyToModel(CStudioHdr* pHdr, int& nSkin, int& nBody) const
{
	if (!pHdr)
		return false;

	for (int i = 0; i < vBodyGroups.Count(); i++)
	{
		auto& body = vBodyGroups[i];
		int iGroup = FindBodygroupByName(pHdr, body.strName.String());
		if (iGroup >= 0)
			SetBodygroup(pHdr, nBody,iGroup, body.vValues.Random());
	}

	for (int i = 0; i < vBodyGroupSync.Count(); i++)
	{
		auto& sync = vBodyGroupSync[i];
		int iSrcGroup = FindBodygroupByName(pHdr, sync.srcName.String());
		int iDstGroup = FindBodygroupByName(pHdr, sync.dstName.String());
		if (iSrcGroup >= 0 && iDstGroup >= 0)
			SetBodygroup(pHdr, nBody, iDstGroup, GetBodygroup(pHdr, nBody, iSrcGroup));
	}

	if (vSkins.Count())
	{
		if (vSkins[0] < 0)
		{
			nSkin = RandomInt(0, pHdr->numskinfamilies() - 1);
		}
		else
		{
			nSkin = vSkins.Random();
		}
	}

	return true;
}
