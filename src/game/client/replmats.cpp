//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "igamesystem.h"
#include "rendertexture.h"
#include "materialsystem_passtru.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace VMTLoader
{
	//-----------------------------------------------------------------------------
	// VMT parser
	//-----------------------------------------------------------------------------
	void InsertKeyValues(KeyValues& dst, KeyValues& src, bool bCheckForExistence, bool bRecursive = false)
	{
		KeyValues* pSrcVar = src.GetFirstSubKey();
		while (pSrcVar)
		{
			if (!bCheckForExistence || dst.FindKey(pSrcVar->GetName()))
			{
				switch (pSrcVar->GetDataType())
				{
				case KeyValues::TYPE_STRING:
					dst.SetString(pSrcVar->GetName(), pSrcVar->GetString());
					break;
				case KeyValues::TYPE_INT:
					dst.SetInt(pSrcVar->GetName(), pSrcVar->GetInt());
					break;
				case KeyValues::TYPE_FLOAT:
					dst.SetFloat(pSrcVar->GetName(), pSrcVar->GetFloat());
					break;
				case KeyValues::TYPE_PTR:
					dst.SetPtr(pSrcVar->GetName(), pSrcVar->GetPtr());
					break;
				case KeyValues::TYPE_NONE:
				{
					// Subkey. Recurse.
					KeyValues* pNewDest = dst.FindKey(pSrcVar->GetName(), true);
					Assert(pNewDest);
					InsertKeyValues(*pNewDest, *pSrcVar, bCheckForExistence, true);
				}
				break;
				}
			}
			pSrcVar = pSrcVar->GetNextKey();
		}

		if (bRecursive && !dst.GetFirstSubKey())
		{
			// Insert a dummy key to an empty subkey to make sure it doesn't get removed
			dst.SetInt("__vmtpatchdummy", 1);
		}

		if (bCheckForExistence)
		{
			for (KeyValues* pScan = dst.GetFirstTrueSubKey(); pScan; pScan = pScan->GetNextTrueSubKey())
			{
				KeyValues* pTmp = src.FindKey(pScan->GetName());
				if (!pTmp)
					continue;
				// make sure that this is a subkey.
				if (pTmp->GetDataType() != KeyValues::TYPE_NONE)
					continue;
				InsertKeyValues(*pScan, *pTmp, bCheckForExistence);
			}
		}
	}

	void WriteKeyValuesToFile(const char* pFileName, KeyValues& keyValues)
	{
		keyValues.SaveToFile(g_pFullFileSystem, pFileName);
	}

	void ApplyPatchKeyValues(KeyValues& keyValues, KeyValues& patchKeyValues)
	{
		KeyValues* pInsertSection = patchKeyValues.FindKey("insert");
		KeyValues* pReplaceSection = patchKeyValues.FindKey("replace");

		if (pInsertSection)
		{
			InsertKeyValues(keyValues, *pInsertSection, false);
		}

		if (pReplaceSection)
		{
			InsertKeyValues(keyValues, *pReplaceSection, true);
		}

		// Could add other commands here, like "delete", "rename", etc.
	}

	//-----------------------------------------------------------------------------
	// Adds keys from srcKeys to destKeys, overwriting any keys that are already
	// there.
	//-----------------------------------------------------------------------------
	void MergeKeyValues(KeyValues& srcKeys, KeyValues& destKeys)
	{
		for (KeyValues* pKV = srcKeys.GetFirstValue(); pKV; pKV = pKV->GetNextValue())
		{
			switch (pKV->GetDataType())
			{
			case KeyValues::TYPE_STRING:
				destKeys.SetString(pKV->GetName(), pKV->GetString());
				break;
			case KeyValues::TYPE_INT:
				destKeys.SetInt(pKV->GetName(), pKV->GetInt());
				break;
			case KeyValues::TYPE_FLOAT:
				destKeys.SetFloat(pKV->GetName(), pKV->GetFloat());
				break;
			case KeyValues::TYPE_PTR:
				destKeys.SetPtr(pKV->GetName(), pKV->GetPtr());
				break;
			}
		}
		for (KeyValues* pKV = srcKeys.GetFirstTrueSubKey(); pKV; pKV = pKV->GetNextTrueSubKey())
		{
			KeyValues* pDestKV = destKeys.FindKey(pKV->GetName(), true);
			MergeKeyValues(*pKV, *pDestKV);
		}
	}

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void AccumulatePatchKeyValues(KeyValues& srcKeyValues, KeyValues& patchKeyValues)
	{
		KeyValues* pDestInsertSection = patchKeyValues.FindKey("insert");
		if (pDestInsertSection == NULL)
		{
			pDestInsertSection = new KeyValues("insert");
			patchKeyValues.AddSubKey(pDestInsertSection);
		}

		KeyValues* pDestReplaceSection = patchKeyValues.FindKey("replace");
		if (pDestReplaceSection == NULL)
		{
			pDestReplaceSection = new KeyValues("replace");
			patchKeyValues.AddSubKey(pDestReplaceSection);
		}

		KeyValues* pSrcInsertSection = srcKeyValues.FindKey("insert");
		if (pSrcInsertSection)
		{
			MergeKeyValues(*pSrcInsertSection, *pDestInsertSection);
		}

		KeyValues* pSrcReplaceSection = srcKeyValues.FindKey("replace");
		if (pSrcReplaceSection)
		{
			MergeKeyValues(*pSrcReplaceSection, *pDestReplaceSection);
		}
	}

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	bool AccumulateRecursiveVmtPatches(KeyValues& patchKeyValuesOut, KeyValues** ppBaseKeyValuesOut, const KeyValues& keyValues, const char* pPathID, CUtlVector<FileNameHandle_t>* pIncludes)
	{
		if (pIncludes)
		{
			pIncludes->Purge();
		}

		patchKeyValuesOut.Clear();

		if (V_stricmp(keyValues.GetName(), "patch") != 0)
		{
			// Not a patch file, nothing to do
			if (ppBaseKeyValuesOut)
			{
				// flag to the caller that the passed in keyValues are in fact final non-patch values
				*ppBaseKeyValuesOut = NULL;
			}
			return true;
		}

		KeyValues* pCurrentKeyValues = keyValues.MakeCopy();

		// Recurse down through all patch files:
		int nCount = 0;
		while ((nCount < 10) && (V_stricmp(pCurrentKeyValues->GetName(), "patch") == 0))
		{
			// Accumulate the new patch keys from this file
			AccumulatePatchKeyValues(*pCurrentKeyValues, patchKeyValuesOut);

			// Load the included file
			const char* pIncludeFileName = pCurrentKeyValues->GetString("include");

			if (pIncludeFileName == NULL)
			{
				// A patch file without an include key? Not good...
				Warning("VMT patch file has no include key - invalid!\n");
				Assert(pIncludeFileName);
				break;
			}

			CUtlString includeFileName(pIncludeFileName); // copy off the string before we clear the keyvalues it lives in
			pCurrentKeyValues->Clear();
			bool bSuccess = pCurrentKeyValues->LoadFromFile(g_pFullFileSystem, includeFileName, pPathID);
			if (bSuccess)
			{
				if (pIncludes)
				{
					// Remember that we included this file for the pure server stuff.
					pIncludes->AddToTail(g_pFullFileSystem->FindOrAddFileName(includeFileName));
				}
			}
			else
			{
				pCurrentKeyValues->deleteThis();
#ifndef DEDICATED
				Warning("Failed to load $include VMT file (%s)\n", includeFileName.String());
#endif
				if (!HushAsserts())
				{
					AssertMsg(false, "Failed to load $include VMT file (%s)", includeFileName.String());
				}
				return false;
			}

			nCount++;
		}

		if (ppBaseKeyValuesOut)
		{
			*ppBaseKeyValuesOut = pCurrentKeyValues;
		}
		else
		{
			pCurrentKeyValues->deleteThis();
		}

		if (nCount >= 10)
		{
			Warning("Infinite recursion in patch file?\n");
		}
		return true;
	}

	//-----------------------------------------------------------------------------
	//-----------------------------------------------------------------------------
	void ExpandPatchFile(KeyValues& keyValues, KeyValues& patchKeyValues, const char* pPathID, CUtlVector<FileNameHandle_t>* pIncludes)
	{
		KeyValues* pNonPatchKeyValues = NULL;
		if (!patchKeyValues.IsEmpty())
		{
			pNonPatchKeyValues = keyValues.MakeCopy();
		}
		else
		{
			bool bSuccess = AccumulateRecursiveVmtPatches(patchKeyValues, &pNonPatchKeyValues, keyValues, pPathID, pIncludes);
			if (!bSuccess)
			{
				return;
			}
		}

		if (pNonPatchKeyValues != NULL)
		{
			// We're dealing with a patch file. Apply accumulated patches to final vmt
			ApplyPatchKeyValues(*pNonPatchKeyValues, patchKeyValues);
			keyValues = *pNonPatchKeyValues;
			pNonPatchKeyValues->deleteThis();
		}
	}

	bool LoadVMTFile(KeyValues& vmtKeyValues, KeyValues& patchKeyValues, const char* pMaterialName, bool bAbsolutePath, CUtlVector<FileNameHandle_t>* pIncludes)
	{
		char pFileName[MAX_PATH];
		const char* pPathID = "GAME";
		if (!bAbsolutePath)
		{
			Q_snprintf(pFileName, sizeof(pFileName), "materials/%s.vmt", pMaterialName);
		}
		else
		{
			Q_snprintf(pFileName, sizeof(pFileName), "%s.vmt", pMaterialName);
			if (pMaterialName[0] == '/' && pMaterialName[1] == '/' && pMaterialName[2] != '/')
			{
				// UNC, do full search
				pPathID = NULL;
			}
		}

		if (!vmtKeyValues.LoadFromFile(g_pFullFileSystem, pFileName, pPathID))
		{
			return false;
		}
		ExpandPatchFile(vmtKeyValues, patchKeyValues, pPathID, pIncludes);

		return true;
	}
}

static int matCount = 0;
CON_COMMAND(print_num_replaced_mats, "")
{
	ConColorMsg(COLOR_GREEN, "%d replaced materials\n", matCount);
}

bool replMatPossible = false;

//-----------------------------------------------------------------------------
// List of materials that should be replaced
//-----------------------------------------------------------------------------
static const char* const pszShaderReplaceDict[][2] = {
	///*
#ifndef DEFERRED
	"VertexLitGeneric",			"PP_VertexLitGeneric",
	//"UnlitGeneric",				"PP_UnlitGeneric",
	"LightmappedGeneric",		"PP_LightmappedGeneric",
	"WorldVertexTransition",	"PP_WorldVertexTransition",
#if defined(HL1_CLIENT_DLL) || defined(HL2_LAZUL)
	"VertexLitGeneric_DX6",		"PP_VertexLitGeneric",
#endif
#endif // !DEFERRED
	"Teeth",					"PP_Teeth",
	"EyeRefract",				"PP_EyeRefract",
	//"Cable",					"PP_Cable",
	"DepthWrite",				"PP_DepthWrite",
	"SplineRope",				"PP_SplineRope",
	"Refract",					"PP_Refract",
	"Shadow",					"PP_Shadow",
	"ShadowBuild",				"PP_ShadowBuild",
	"ShadowModel",				"PP_ShadowModel",
	"Water",					"PP_Water",
	"Core",						"PP_Core",

	"Cable",					"PP_SplineRope"
};
static const int iNumShaderReplaceDict = ARRAYSIZE(pszShaderReplaceDict);

static const char* const pszShaderReplaceProceduralMats[] = {
	"__DepthWrite00",
	"__DepthWrite01",
	"__DepthWrite10",
	"__DepthWrite11",
	"__ColorDepthWrite00",
	"__ColorDepthWrite01",
	"__ColorDepthWrite10",
	"__ColorDepthWrite11",

	"__DepthWrite000",
	"__DepthWrite010",
	"__DepthWrite100",
	"__DepthWrite110",
	"__DepthWrite001",
	"__DepthWrite011",
	"__DepthWrite101",
	"__DepthWrite111",
	"__ColorDepthWrite000",
	"__ColorDepthWrite010",
	"__ColorDepthWrite100",
	"__ColorDepthWrite110",
	"__ColorDepthWrite001",
	"__ColorDepthWrite011",
	"__ColorDepthWrite101",
	"__ColorDepthWrite111"
};
static const int iNumShaderReplaceProceduralMats = ARRAYSIZE(pszShaderReplaceProceduralMats);

#include "icommandline.h"

// Copied from cdeferred_manager_client.cpp
static void ShaderReplaceReplMat(const char* szNewShadername, IMaterial* pMat, bool bProcedural)
{
	const char* pszOldShadername = pMat->GetShaderName();
	const char* pszMatname = pMat->GetName();
	bool bPhong = false;
	bool bNeedsRefractTex = false;

	KeyValues* msg = new KeyValues(szNewShadername);

	int nParams = pMat->ShaderParamCount();
	IMaterialVar** pParams = pMat->GetShaderParams();

	char str[512];

	for (int i = 0; i < nParams; ++i)
	{
		IMaterialVar* pVar = pParams[i];
		const char* pVarName = pVar->GetName();

		if (!V_stricmp("$flags", pVarName) ||
			!V_stricmp("$flags_defined", pVarName) ||
			!V_stricmp("$flags2", pVarName) ||
			!V_stricmp("$flags_defined2", pVarName))
			continue;

		if (!Q_strncmp("$phong", pVarName, 6))
		{
			bPhong = true;
			continue;
		}

		switch (pVar->GetType())
		{
		case MATERIAL_VAR_TYPE_FLOAT:
			msg->SetFloat(pVarName, pVar->GetFloatValue());
			break;

		case MATERIAL_VAR_TYPE_INT:
			msg->SetInt(pVarName, pVar->GetIntValue());
			break;

		case MATERIAL_VAR_TYPE_STRING:
			msg->SetString(pVarName, pVar->GetStringValue());
			break;

		case MATERIAL_VAR_TYPE_FOURCC:
			//Assert( 0 ); // JDTODO
			break;

		case MATERIAL_VAR_TYPE_VECTOR:
		{
			const float* pVal = pVar->GetVecValue();
			int dim = pVar->VectorSize();
			switch (dim)
			{
			case 1:
				V_sprintf_safe(str, "[%f]", pVal[0]);
				break;
			case 2:
				V_sprintf_safe(str, "[%f %f]", pVal[0], pVal[1]);
				break;
			case 3:
				V_sprintf_safe(str, "[%f %f %f]", pVal[0], pVal[1], pVal[2]);
				break;
			case 4:
				V_sprintf_safe(str, "[%f %f %f %f]", pVal[0], pVal[1], pVal[2], pVal[3]);
				break;
			default:
				Assert(0);
				*str = 0;
			}
			msg->SetString(pVarName, str);
		}
		break;

		case MATERIAL_VAR_TYPE_MATRIX:
		{
			const float* pVal = pVar->GetMatrixValue().Base();
			V_sprintf_safe(str,
				"[%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f]",
				pVal[0], pVal[1], pVal[2], pVal[3],
				pVal[4], pVal[5], pVal[6], pVal[7],
				pVal[8], pVal[9], pVal[10], pVal[11],
				pVal[12], pVal[13], pVal[14], pVal[15]);
			msg->SetString(pVarName, str);
		}
		break;

		case MATERIAL_VAR_TYPE_TEXTURE:
			msg->SetString(pVarName, pVar->GetTextureValue()->GetName());
			if (pVar->GetTextureValue() == GetPowerOfTwoFrameBufferTexture())
				bNeedsRefractTex = true;
			break;

		case MATERIAL_VAR_TYPE_MATERIAL:
			msg->SetString(pVarName, pVar->GetMaterialValue()->GetName());
			break;
		}
	}

	bool alphaBlending = pMat->IsTranslucent() || pMat->GetMaterialVarFlag(MATERIAL_VAR_TRANSLUCENT);
	bool translucentOverride = pMat->IsAlphaTested() || pMat->GetMaterialVarFlag(MATERIAL_VAR_ALPHATEST);

	bool bDecal = pszOldShadername != NULL && Q_stristr(pszOldShadername, "decal") != NULL ||
		pszMatname != NULL && Q_stristr(pszMatname, "decal") != NULL ||
		pMat->GetMaterialVarFlag(MATERIAL_VAR_DECAL);

	if (bDecal)
		msg->SetInt("$decal", 1);

	if (alphaBlending)
		msg->SetInt("$translucent", 1);

	if (translucentOverride)
		msg->SetInt("$alphatest", 1);

	if (pMat->IsTwoSided() || pMat->GetMaterialVarFlag(MATERIAL_VAR_NOCULL))
		msg->SetInt("$nocull", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_ADDITIVE))
		msg->SetInt("$additive", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_MODEL))
		msg->SetInt("$model", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_NOFOG))
		msg->SetInt("$nofog", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_IGNOREZ))
		msg->SetInt("$ignorez", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_HALFLAMBERT))
		msg->SetInt("$halflambert", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK))
		msg->SetInt("$normalmapalphaenvmapmask", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_BASEALPHAENVMAPMASK))
		msg->SetInt("$basealphaenvmapmask", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_SELFILLUM))
		msg->SetInt("$selfillum", 1);

	if (pMat->GetMaterialVarFlag(MATERIAL_VAR_ENVMAPSPHERE))
		msg->SetInt("$envmapsphere", 1);

	if (!bProcedural && pMat->HasProxy())
	{
		CUtlVector<FileNameHandle_t> includes;
		KeyValues* pKeyValues = new KeyValues("vmt");
		KeyValues* pPatchKeyValues = new KeyValues("vmt_patches");
		VMTLoader::LoadVMTFile(*pKeyValues, *pPatchKeyValues, pMat->GetName(), false, &includes);

		//if (pKeyValues)
		{
			//if (pMat->HasProxy())
			{
				KeyValues* pkvProxies = pKeyValues->FindKey("Proxies");
				if (pkvProxies)
				{
					KeyValues* pkvCopy = pkvProxies->MakeCopy();
					msg->AddSubKey(pkvCopy);
				}
			}
		}

		pKeyValues->deleteThis();
		pPatchKeyValues->deleteThis();
	}

	if (bNeedsRefractTex)
	{
		bool bFound = false;
		IMaterialVar* pFlags2 = pMat->FindVar("$flags2", &bFound);
		if (bFound)
		{
			int iFlags2 = pFlags2->GetIntValue();
			iFlags2 |= MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE;
			msg->SetInt("$flags2", iFlags2);
		}
		IMaterialVar* pFlagsDef2 = pMat->FindVar("$flags_defined2", &bFound);
		if (bFound)
		{
			int iFlags2 = pFlagsDef2->GetIntValue();
			iFlags2 |= MATERIAL_VAR2_NEEDS_POWER_OF_TWO_FRAME_BUFFER_TEXTURE;
			msg->SetInt("$flags_defined2", iFlags2);
		}
	}

	pMat->SetShaderAndParams(msg);

	//pMat->RefreshPreservingMaterialVars();

	pMat->Refresh();

	msg->deleteThis();
}

//-----------------------------------------------------------------------------
// Overrides FindMaterial and replaces the material if the shader name is found
// in "shadernames_tocheck".
// TODO: Also override the reload material functions and update the material, 
//		 otherwise we keep reading from the replacement directory.
//-----------------------------------------------------------------------------
class CReplMaterialSystem : public CPassThruMaterialSystem
{
public:
	IMaterial* FindMaterialEx(char const* pMaterialName, const char* pTextureGroupName, int nContext, bool complain = true, const char* pComplainPrefix = NULL) OVERRIDE
	{
		return ReplaceMaterialInternal(BaseClass::FindMaterialEx(pMaterialName, pTextureGroupName, nContext, complain, pComplainPrefix));
	}

	IMaterial* FindMaterial(char const* pMaterialName, const char* pTextureGroupName, bool complain = true, const char* pComplainPrefix = NULL) OVERRIDE
	{
		return ReplaceMaterialInternal(BaseClass::FindMaterial(pMaterialName, pTextureGroupName, complain, pComplainPrefix));
	}

	IMaterial* FindProceduralMaterial(const char* pMaterialName, const char* pTextureGroupName, KeyValues* pVMTKeyValues)	OVERRIDE
	{
		if (replMatPossible)
		{
			const char* pShaderName = pVMTKeyValues->GetName();
			for (int i = 0; i < iNumShaderReplaceDict; i++)
			{
				if (Q_stristr(pShaderName, pszShaderReplaceDict[i][0]) == pShaderName)
				{
					pVMTKeyValues->SetName(pszShaderReplaceDict[i][1]);
					matCount++;
					break;
				}
			}
		}

		return ReplaceMaterialInternal(BaseClass::FindProceduralMaterial(pMaterialName, pTextureGroupName, pVMTKeyValues), true);
	}

	IMaterial* CreateMaterial(const char* pMaterialName, KeyValues* pVMTKeyValues) OVERRIDE
	{
		if (replMatPossible)
		{
			const char* pShaderName = pVMTKeyValues->GetName();
			for (int i = 0; i < iNumShaderReplaceDict; i++)
			{
				if (Q_stristr(pShaderName, pszShaderReplaceDict[i][0]) == pShaderName)
				{
					pVMTKeyValues->SetName(pszShaderReplaceDict[i][1]);
					matCount++;
					break;
				}
			}
		}

		return BaseClass::CreateMaterial(pMaterialName, pVMTKeyValues);
	}

	IMaterial* ReplaceMaterialInternal(IMaterial* pMat, bool bProcedural = false) const
	{
		if (!pMat || pMat->IsErrorMaterial() || pMat->InMaterialPage() || !replMatPossible)
			return pMat;

		const char* pShaderName = pMat->GetShaderName();
		if (pShaderName)
		{
			for (int i = 0; i < iNumShaderReplaceDict; i++)
			{
				if (Q_stristr(pShaderName, pszShaderReplaceDict[i][0]) == pShaderName)
				{
					ShaderReplaceReplMat(pszShaderReplaceDict[i][1], pMat, bProcedural);
					matCount++;
					break;
				}
			}
		}
		return pMat;
	}

	IMaterial* RestoreProceduralMaterial(IMaterial* pMat) const
	{
		if (!pMat || pMat->IsErrorMaterial() || pMat->InMaterialPage() || !replMatPossible)
			return pMat;

		const char* pShaderName = pMat->GetShaderName();
		if (pShaderName)
		{
			for (int i = 0; i < iNumShaderReplaceDict; i++)
			{
				if (Q_stristr(pShaderName, pszShaderReplaceDict[i][1]) == pShaderName)
				{
					ShaderReplaceReplMat(pszShaderReplaceDict[i][0], pMat, true);
					matCount++;
					break;
				}
			}
		}
		return pMat;
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ReplacementSystem : public CAutoGameSystem
{
public:
	ReplacementSystem() : m_pOldMaterialSystem(NULL) {}

	virtual bool Init()
	{
		Enable();
		return true;
	}
	virtual void Shutdown() { Disable(); }
	virtual void LevelShutdownPostEntity() { /*matCount = 0;*/ }

	void Enable();
	void Disable();
	bool IsEnabled() const { return m_pOldMaterialSystem != NULL; }

private:
	CReplMaterialSystem m_MatSysPassTru;
	IMaterialSystem* m_pOldMaterialSystem;
};

static ReplacementSystem s_ReplacementSystem;

void ReplacementSystem::Enable()
{
	replMatPossible = true; //CommandLine()->CheckParm("-experimental");

	if (m_pOldMaterialSystem || !replMatPossible)
		return;

	DevMsg("Enabled material replacement system\n");

	// Replace material system
	m_MatSysPassTru.InitPassThru(materials);

	m_pOldMaterialSystem = materials;
	materials = &m_MatSysPassTru;
	g_pMaterialSystem = &m_MatSysPassTru;
	engine->Mat_Stub(&m_MatSysPassTru);

	/*for (int i = 0; i < iNumShaderReplaceProceduralMats; i++)
	{
		IMaterial* pMat = m_pOldMaterialSystem->FindMaterial(pszShaderReplaceProceduralMats[i], TEXTURE_GROUP_OTHER, false);
		if (pMat)
		{
			m_MatSysPassTru.ReplaceMaterialInternal(pMat, true);
		}
	}*/
}

void ReplacementSystem::Disable()
{
	if (m_pOldMaterialSystem)
	{
		DevMsg("Disabled material replacement system\n");

		/*for (int i = 0; i < iNumShaderReplaceProceduralMats; i++)
		{
			IMaterial* pMat = m_pOldMaterialSystem->FindMaterial(pszShaderReplaceProceduralMats[i], TEXTURE_GROUP_OTHER, false);
			if (pMat)
			{
				m_MatSysPassTru.RestoreProceduralMaterial(pMat);
			}
		}*/

		materials = m_pOldMaterialSystem;
		g_pMaterialSystem = m_pOldMaterialSystem;
		engine->Mat_Stub(m_pOldMaterialSystem);
		m_pOldMaterialSystem = NULL;
	}
}

CON_COMMAND_F(toggle_replmat, "Toggles the material replacement system", FCVAR_CHEAT)
{
	if (s_ReplacementSystem.IsEnabled())
	{
		s_ReplacementSystem.Disable();
	}
	else
	{
		s_ReplacementSystem.Enable();
	}

	materials->UncacheAllMaterials();
	materials->CacheUsedMaterials();
	materials->ReloadMaterials();
}
