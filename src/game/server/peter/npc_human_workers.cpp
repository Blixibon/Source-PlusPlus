#include "cbase.h"
#include "npc_basecollegue.h"

class CNPC_HumanMainT : public CNPC_BaseColleague
{
	DECLARE_CLASS(CNPC_HumanMainT, CNPC_BaseColleague);
public:

	void	Spawn(void);
	void	SelectModel();
	Class_T Classify(void);
};

LINK_ENTITY_TO_CLASS(npc_human_maint, CNPC_HumanMainT);
LINK_ENTITY_TO_CLASS(npc_human_maintenance, CNPC_HumanMainT);

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_HumanMainT::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanMainT::Spawn(void)
{
	//Precache();

	m_iHealth = 80;

	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);
	//CapabilitiesRemove(bits_CAP_USE_WEAPONS);

	NPCInit();

	SetUse(&CNPC_BaseColleague::CommanderUse);

	//m_nSkin = gm_iLastChosenSkin;
	int iHelmet = FindBodygroupByName("helmet");
	SetBodygroup(iHelmet, RandomInt(0, GetBodygroupCount(iHelmet) - 1));
}

colleagueModel_t g_mainTModel[] =
{
	{ "models/humans/cwork.mdl",	"models/humans/cwork_hurt.mdl" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanMainT::SelectModel()
{
	/*if (RandomFloat() >= 0.75f)
	SetModelName(AllocPooledString(MSCI_MODEL2));
	else
	SetModelName(AllocPooledString(MSCI_MODEL));*/

	SetModelName(AllocPooledString(ChooseColleagueModel(g_mainTModel, 2, m_nSkin.GetForModify())));
}

class CNPC_HumanEngie : public CNPC_BaseColleague
{
	DECLARE_CLASS(CNPC_HumanEngie, CNPC_BaseColleague);
public:

	void	Spawn(void);
	void	SelectModel();
	Class_T Classify(void);
};

LINK_ENTITY_TO_CLASS(npc_human_engineer, CNPC_HumanEngie);

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_HumanEngie::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanEngie::Spawn(void)
{
	//Precache();

	m_iHealth = 80;

	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);
	//CapabilitiesRemove(bits_CAP_USE_WEAPONS);

	NPCInit();

	SetUse(&CNPC_BaseColleague::CommanderUse);

	//m_nSkin = gm_iLastChosenSkin;
	int iHelmet = FindBodygroupByName("helmet");
	SetBodygroup(iHelmet, RandomInt(0, GetBodygroupCount(iHelmet) - 1));
}

colleagueModel_t g_engieModel[] =
{
	{ "models/humans/engineer.mdl",	"models/humans/engineer_hurt.mdl" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanEngie::SelectModel()
{
	/*if (RandomFloat() >= 0.75f)
	SetModelName(AllocPooledString(MSCI_MODEL2));
	else
	SetModelName(AllocPooledString(MSCI_MODEL));*/

	SetModelName(AllocPooledString(ChooseColleagueModel(g_engieModel, 2, m_nSkin.GetForModify())));
}

class CNPC_HumanOffice : public CNPC_BaseColleague
{
	DECLARE_CLASS(CNPC_HumanOffice, CNPC_BaseColleague);
public:

	void	Spawn(void);
	void	SelectModel();
	Class_T Classify(void);
};

LINK_ENTITY_TO_CLASS(npc_human_worker, CNPC_HumanOffice);

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_HumanOffice::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanOffice::Spawn(void)
{
	//Precache();

	m_iHealth = 80;

	BaseClass::Spawn();

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);
	//CapabilitiesRemove(bits_CAP_USE_WEAPONS);

	NPCInit();

	SetUse(&CNPC_BaseColleague::CommanderUse);

	//m_nSkin = gm_iLastChosenSkin;
	int iGlasses = FindBodygroupByName("glasses");
	SetBodygroup(iGlasses, RandomInt(0, GetBodygroupCount(iGlasses)-1));
}

colleagueModel_t g_officeModel[] =
{
	{ "models/humans/worker_clean.mdl",	"models/humans/worker.mdl" },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanOffice::SelectModel()
{
	/*if (RandomFloat() >= 0.75f)
	SetModelName(AllocPooledString(MSCI_MODEL2));
	else
	SetModelName(AllocPooledString(MSCI_MODEL));*/

	SetModelName(AllocPooledString(ChooseColleagueModel(g_officeModel, 2, m_nSkin.GetForModify())));
}