#ifndef POPMAN_H
#define POPMAN_H
#pragma once

#include "igamesystem.h"
#include "entitylist.h"
#include "utlvector.h"
#include "utlsymbol.h"

class CPopulationControl;
extern CPopulationControl *g_pPopulationManager;

class CPopulationDefinition : public CAutoGameSystem
{
protected:
	friend class CPopulationControl;
	char chName[32];
	CUtlVector<const char *> types;
	//CUtlSymbolTable types;
	CUtlVector<int> weighted_random;
	int iIndex;
public:
	CPopulationDefinition(const char *pchName, const char **ppszArray, int iSize) : CAutoGameSystem()
	{
		//AddToAutoList(this);
		iIndex = -1;
		Q_strncpy(chName, pchName, sizeof(chName));
		types.CopyArray(ppszArray, iSize);
		//weighted_random.SetSize(0);
	}

	virtual void PostInit();

	virtual char const *Name() { return chName; }

	virtual int GetRandom();
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

	void AddDefinition(CPopulationDefinition *pDef)
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
	CUtlVector<CPopulationDefinition *> m_Definitions;
};

#endif // !POPMAN_H

