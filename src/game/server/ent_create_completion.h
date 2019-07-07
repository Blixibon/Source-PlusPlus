#ifndef ENT_CREATE_COMPLETION_H
#define ENT_CREATE_COMPLETION_H
#pragma once
#include "convar.h"
#include "mapentities.h"

class GDclass;

class CEntCreateCompletionFunctor : public ICommandCompletionCallback
{
public:
	CEntCreateCompletionFunctor(const char *pszCommand, IMapEntityFilter *pFilter = nullptr);

	virtual int CommandCompletionCallback(const char *partial, CUtlVector< CUtlString > &commands);
protected:
	int EntCreate_KVCompletion(GDclass *pClass, const char *partial, CUtlVector< CUtlString > &commands);

	IMapEntityFilter *m_pFilter;
	char m_cCommand[COMMAND_COMPLETION_ITEM_LENGTH];
};

#endif // !ENT_CREATE_COMPLETION_H
