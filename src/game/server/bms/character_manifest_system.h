#pragma once

#include "igamesystem.h"
#include "utldict.h"
#include "utlvector.h"
#include "bitvec.h"
#include "utlsymbol.h"
#include "utlbuffer.h"
#include "bms/character_manifest_shared.h"

namespace CharacterManifest
{
	DECLARE_PRIVATE_SYMBOLTYPE(CManifestSymbol);

	struct ManifestBodyData_t
	{
		CManifestSymbol strName;
		CCopyableUtlVector<int> vValues;
	};

	struct ManifestBodyPropagate_t
	{
		CManifestSymbol srcName;
		CManifestSymbol dstName;
	};

	struct ManifestCharacter_t
	{
		CManifestSymbol strModelName;
		CCopyableUtlVector<int> vSkins;
		CCopyableUtlVector<ManifestBodyData_t> vBodyGroups;
		CCopyableUtlVector<ManifestBodyPropagate_t> vBodyGroupSync;
		CCopyableUtlVector<ManifestFlexData_t> vFlexControllers;
		CCopyableUtlVector<CManifestSymbol> vMergedModels;
		CCopyableUtlVector<CManifestSymbol> vModelTags;

		bool	ApplyToModel(CStudioHdr* pHdr, int& nSkin, int& nBody) const;
	};

	inline const char* GetScriptModel(const ManifestCharacter_t* pChar, const char* pszDefault = "models/error.mdl")
	{
		if (!pChar)
			return pszDefault;

		const char* pszModel = pChar->strModelName.String();
		if (!pszModel || V_strcmp(pszModel, "") == 0)
			return pszDefault;

		return pszModel;
	}

	CRC32_t EncodeFlexVector(CUtlVector<ManifestFlexData_t>& vData, CUtlBuffer& buf);

	class CCharacterManifestSystem : public CAutoGameSystem
	{
		DECLARE_CLASS_GAMEROOT(CCharacterManifestSystem, CAutoGameSystem);
	public:
		CCharacterManifestSystem();

		bool	Init();
		void	Shutdown();

		const ManifestCharacter_t* FindCharacterModel(const char* pszCharName) const;
		const ManifestCharacter_t* FindCharacterModel(const char* pszCharName, CUtlVector<CManifestSymbol>* pvIncludeTags, CUtlVector<CManifestSymbol>* pvExcludeTags, CUtlVector<CManifestSymbol>* pvPreferTags) const;
		void	PrecacheCharacterModels(const char* pszCharName) const;

	protected:
		void	LoadDataFromFile(const char* pszFileName);

		typedef CCopyableUtlVector<ManifestCharacter_t> characterList_t;
		typedef CUtlVector<const ManifestCharacter_t*> characterPtrList_t;
		CUtlDict<characterList_t> m_ManifestDict;
	};
}

CharacterManifest::CCharacterManifestSystem* GetCharacterManifest();