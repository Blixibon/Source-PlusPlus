#include "cbase.h"
#include "monstermaker.h"


class xenportalshareddata_t
{
public:
	//DECLARE_SIMPLE_DATADESC();

	xenportalshareddata_t() : iSize(80)
	{

	}

	int iSize;
	//string_t sound;
	COutputEvent m_StartPortal;
	COutputEvent m_EndPortal;

	void StartPortal(CBaseEntity *pPortal)
	{
		m_StartPortal.FireOutput(pPortal, pPortal);
		m_EndPortal.FireOutput(pPortal, pPortal, 4.0f);
	}
};


static const char *s_pSpawnDelayContext = "SpawnDelayThink";

#define MESSAGE_START_FIREBALL 0
#define MESSAGE_STOP_FIREBALL 1

class CXenPortal : public CBaseNPCMaker
{
public:
	DECLARE_CLASS(CXenPortal, CBaseNPCMaker);

	CXenPortal(void);

	void Precache(void);

	virtual void MakeNPC(void);

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	string_t m_iszNPCClassname;			// classname of the NPC(s) that will be created.
	string_t m_SquadName;
	string_t m_strHintGroup;
	string_t m_spawnEquipment;
	string_t m_RelationshipString;		// Used to load up relationship keyvalues
	string_t m_ChildTargetName;

protected:

	xenportalshareddata_t m_PortalData;

	void SpawnDelayThink(void);
};

class CQSpawn
{
public:
	DECLARE_SIMPLE_DATADESC();
	CAI_BaseNPC *pAI;
	float flSpawnTime;

	CQSpawn(CAI_BaseNPC *pEnt, float Time)
	{
		pAI = pEnt;
		flSpawnTime = Time;
	}

	CQSpawn()
	{
		pAI = NULL;
		flSpawnTime = 0.0f;
	}
};

class CTemplateXenPortal : public CTemplateNPCMaker
{
public:
	DECLARE_CLASS(CTemplateXenPortal, CTemplateNPCMaker);
	DECLARE_DATADESC();

	void QueuedSpawnThink(void);

	void MakeNPC(void);
	void MakeNPCInRadius(void);
	void MakeNPCInLine(void);


protected:
	xenportalshareddata_t m_PortalData;

	typedef CQSpawn queuedSpawn_t;
	CUtlVector<queuedSpawn_t> m_DelayedSpawns;
};