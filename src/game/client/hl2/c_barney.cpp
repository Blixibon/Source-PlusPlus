//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_ai_basenpc.h"
#include "functionproxy.h"
#include "toolframework_client.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_Character : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS(C_NPC_Character, C_AI_BaseNPC);

	C_NPC_Character();
	~C_NPC_Character();

	// fast class list
	ThisClass* m_pNext;

	virtual bool IsAlyx() { return false; }
};

C_EntityClassList<C_NPC_Character> g_CharacterList;
template <> C_NPC_Character* C_EntityClassList<C_NPC_Character>::m_pClassList = NULL;

C_NPC_Character::C_NPC_Character()
{
	m_pNext = nullptr;
	g_CharacterList.Insert(this);
}


C_NPC_Character::~C_NPC_Character()
{
	g_CharacterList.Remove(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_Barney : public C_NPC_Character
{
public:
	DECLARE_CLASS( C_Barney, C_NPC_Character);
	DECLARE_CLIENTCLASS();

					C_Barney();
	virtual			~C_Barney();

private:
	C_Barney( const C_Barney & ); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_Barney, DT_NPC_Barney, CNPC_Barney)
END_RECV_TABLE();

C_Barney::C_Barney()
{
}


C_Barney::~C_Barney()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_NPC_Alyx : public C_NPC_Character
{
public:
	DECLARE_CLASS(C_NPC_Alyx, C_NPC_Character);
	DECLARE_CLIENTCLASS();

	C_NPC_Alyx();
	virtual			~C_NPC_Alyx();

	virtual bool IsAlyx() { return true; }

private:
	C_NPC_Alyx(const C_NPC_Alyx&); // not defined, not accessible
};

IMPLEMENT_CLIENTCLASS_DT(C_NPC_Alyx, DT_NPC_Alyx, CNPC_Alyx)
END_RECV_TABLE();

C_NPC_Alyx::C_NPC_Alyx()
{
}


C_NPC_Alyx::~C_NPC_Alyx()
{
}

//-----------------------------------------------------------------------------
// Returns the player position
//-----------------------------------------------------------------------------
class CCharacterPositionProxy : public CResultProxy
{
public:
	bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	void OnBind(void* pC_BaseEntity);

private:
	float	m_Factor;
	IMaterialVar* m_pHasCharacterVar;
};

bool CCharacterPositionProxy::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	if (!CResultProxy::Init(pMaterial, pKeyValues))
		return false;

	char const* pResult = pKeyValues->GetString("foundVar");
	if (!pResult)
		return false;

	bool foundVar;
	m_pHasCharacterVar = pMaterial->FindVar(pResult, &foundVar, true);
	if (!foundVar)
		return false;

	m_Factor = pKeyValues->GetFloat("scale", 0.005);
	return true;
}

void CCharacterPositionProxy::OnBind(void* pC_BaseEntity)
{
	C_NPC_Character* pCurrent = g_CharacterList.m_pClassList;

	C_NPC_Character* pBest = nullptr;
	float flBestDistSqr = FLT_MAX;
	while (pCurrent != nullptr)
	{
		if (pCurrent->IsAlyx())
		{
			pBest = pCurrent;
			break;
		}
		else if (pCurrent->WorldSpaceCenter().DistToSqr(CurrentViewOrigin()) < flBestDistSqr)
		{
			pBest = pCurrent;
			flBestDistSqr = pCurrent->WorldSpaceCenter().DistToSqr(CurrentViewOrigin());
		}

		pCurrent = pCurrent->m_pNext;
	}

	if (pBest)
	{
		// This is actually a vector...
		Assert(m_pResult);
		Vector res;
		VectorMultiply(pBest->WorldSpaceCenter(), m_Factor, res);
		m_pResult->SetVecValue(res.Base(), 3);
		m_pHasCharacterVar->SetIntValue(1);
	}
	else
	{
		m_pHasCharacterVar->SetIntValue(0);
	}

	if (ToolsEnabled())
	{
		ToolFramework_RecordMaterialParams(GetMaterial());
	}
}

EXPOSE_INTERFACE(CCharacterPositionProxy, IMaterialProxy, "MainCharacterPosition" IMATERIAL_PROXY_INTERFACE_VERSION);
