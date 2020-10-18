#ifndef POPMAN_H
#define POPMAN_H
#pragma once

#include "igamesystem.h"
#include "entitylist.h"
#include "utlvector.h"
#include "utlsymbol.h"

class CPopulationControl;
extern CPopulationControl *g_pPopulationManager;

class CBasePopulationDefinition : public CAutoGameSystem
{
protected:
	friend class CPopulationControl;
	CUtlVector<int> weighted_random;
	int iRange;
	int iIndex;

	virtual void DoWeighting(const CUtlVector<char*>& typeList) = 0;
public:
	virtual bool Init();
	virtual int GetRandom();
};

class CPopulationDefinition : public CBasePopulationDefinition
{
protected:
	char chName[32];
	CUtlVector<const char*> types;

	virtual void DoWeighting(const CUtlVector<char*>& typeList);
public:
	CPopulationDefinition(const char* pchName, const char** ppszArray, int iSize) : CBasePopulationDefinition()
	{
		//AddToAutoList(this);
		iIndex = -1;
		Q_strncpy(chName, pchName, sizeof(chName));
		types.CopyArray(ppszArray, iSize);
		iRange = iSize;
		//weighted_random.SetSize(0);
	}

	virtual char const* Name() { return chName; }
};

class CPopulationControl : public CAutoGameSystem, public IEntityListener
{
public:
	DECLARE_CLASS_GAMEROOT(CPopulationControl, CAutoGameSystem);

	CPopulationControl() : CAutoGameSystem("PopulationControl")
	{}

	virtual bool Init()
	{
		/*for (int i = 0; i < IPopulationDefinition::AutoList().Count(); i++)
		{
			CPopulationDefinition *pDef = assert_cast<CPopulationDefinition *> (IPopulationDefinition::AutoList().Element(i));
			pDef->iIndex = i;
			pDef->Init();
			m_Definitions.AddToTail(pDef);
		}

		Assert(m_Definitions.Count() == IPopulationDefinition::AutoList().Count());
*/
		return true;
	}

	void AddDefinition(CBasePopulationDefinition*pDef)
	{
		int iDX = m_Definitions.AddToTail(pDef);
		pDef->iIndex = iDX;
	}

	virtual void LevelInitPreEntity()
	{
		gEntList.AddListenerEntity(this);
	}

	virtual void LevelInitPostEntity();

	virtual void OnEntitySpawned(CBaseEntity *pEntity);

protected:
	string_t m_iszPopulationTag;
	CUtlVector<CBasePopulationDefinition*> m_Definitions;
};

#endif // !POPMAN_H

