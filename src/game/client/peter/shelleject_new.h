#ifndef SHELLEJECT_NEW_H
#define SHELLEJECT_NEW_H
#pragma once

#include "igamesystem.h"
#include "UtlStringMap.h"
#include "utldict.h"

typedef struct scriptShell_s
{
	scriptShell_s()
	{
		iCount = 1;
		forward_speed_min = 0.f;
		forward_speed_max = 0.f;
		right_speed_min = 0.f;
		right_speed_max = 0.f;
		up_speed_min = 0.f;
		up_speed_max = 0.f;
		lifetime = 0.f;
		iSkin = 0;
		flGravityScale;
		V_memset(cModelName, 0, MAX_PATH);
		iModelIndex = -1;
	}
	// Generic variables.
	int iCount;
	float forward_speed_min;
	float forward_speed_max;
	float right_speed_min;
	float right_speed_max;
	float up_speed_min;
	float up_speed_max;
	float lifetime;
	int iSkin;
	float flGravityScale;
	char cModelName[MAX_PATH];

	int iModelIndex;
} scriptShell_t;

typedef CUtlDict<scriptShell_t> shellMap_t;

class CShellEjectScriptSystem : public CAutoGameSystem
{
public:
	CShellEjectScriptSystem() : CAutoGameSystem("CShellEjectScriptSystem")
	{}

	virtual bool Init();
	virtual void Shutdown();
	virtual void LevelInitPreEntity();

	void EjectShell(C_BaseAnimating *pEnt, const char *options);

protected:
	CUtlStringMap<shellMap_t *> m_Scripts;
};

CShellEjectScriptSystem *NewShellSystem();

#endif // !SHELLEJECT_NEW_H
