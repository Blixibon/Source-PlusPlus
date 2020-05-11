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

//LINK_ENTITY_TO_CLASS(npc_human_maint, CNPC_HumanMainT);
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanMainT::SelectModel()
{
	const CharacterManifest::ManifestCharacter_t* pChar = nullptr;
	string_t iszName = GetEntityName();
	if (iszName != NULL_STRING)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(STRING(iszName));
	}

	if (!pChar)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(GetClassname());
	}

	SetModelName(AllocPooledString(CharacterManifest::GetScriptModel(pChar, "models/humans/cwork.mdl")));
	m_pCharacterDefinition = pChar;
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanEngie::SelectModel()
{
	const CharacterManifest::ManifestCharacter_t* pChar = nullptr;
	string_t iszName = GetEntityName();
	if (iszName != NULL_STRING)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(STRING(iszName));
	}

	if (!pChar)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(GetClassname());
	}

	SetModelName(AllocPooledString(CharacterManifest::GetScriptModel(pChar, "models/humans/engineer.mdl")));
	m_pCharacterDefinition = pChar;
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_HumanOffice::SelectModel()
{
	const CharacterManifest::ManifestCharacter_t* pChar = nullptr;
	string_t iszName = GetEntityName();
	if (iszName != NULL_STRING)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(STRING(iszName));
	}

	if (!pChar)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(GetClassname());
	}

	SetModelName(AllocPooledString(CharacterManifest::GetScriptModel(pChar, "models/humans/worker_clean.mdl")));
	m_pCharacterDefinition = pChar;
}