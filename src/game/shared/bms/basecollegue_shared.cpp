#include "cbase.h"
#include "bms_utils.h"

#ifdef CLIENT_DLL
#include "c_npc_basecollegue.h"
#else
#include "npc_basecollegue.h"
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Helper function get get determinisitc random values for shared/prediction code
// Input  : seedvalue - 
//			*module - 
//			line - 
// Output : static int
//-----------------------------------------------------------------------------
static int SeedFileLineHash(int seedvalue, const char *sharedname, int additionalSeed)
{
	CRC32_t retval;

	CRC32_Init(&retval);

	CRC32_ProcessBuffer(&retval, (void *)&seedvalue, sizeof(int));
	CRC32_ProcessBuffer(&retval, (void *)&additionalSeed, sizeof(int));
	CRC32_ProcessBuffer(&retval, (void *)sharedname, Q_strlen(sharedname));

	CRC32_Final(&retval);

	return (int)(retval);
}

CUniformRandomStream faceRandom;

IMPLEMENT_NETWORKCLASS_ALIASED(NPC_BaseColleague, DT_BaseColleague);

BEGIN_NETWORK_TABLE(CNPC_BaseColleague, DT_BaseColleague)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iHeadRndSeed)),
#else
SendPropInt(SENDINFO(m_iHeadRndSeed)),
#endif // CLIENT_DLL
END_NETWORK_TABLE();

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_BaseColleague::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (m_iHeadRndSeed != m_iOldRndSeed)
	{
		m_iOldRndSeed = m_iHeadRndSeed;

		for (int i = 0; i < NUM_RND_HEAD_FLEXES; i++)
		{
			LocalFlexController_t controller = m_HeadFlxs[i];
			if (controller >= 0)
			{
				int iSeed = SeedFileLineHash(m_iHeadRndSeed, "", controller);
				faceRandom.SetSeed(iSeed);

				m_HeadFlxWgts[i] = faceRandom.RandomFloat();
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: initialize fast lookups when model changes
//-----------------------------------------------------------------------------

CStudioHdr *CNPC_BaseColleague::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iHeadRndSeed = 0;

	if (hdr)
	{
#ifndef CLIENT_DLL
		int iSeed = SeedFileLineHash(entindex(), hdr->pszName(), gpGlobals->curtime);
		m_iHeadRndSeed = iSeed;

		DevMsg(2, "ENT %d %s seed: %d\n", entindex(), (IsServer() ? "SERVER" : "CLIENT"), iSeed);

		for (int i = 0; i < NUM_RND_HEAD_FLEXES; i++)
		{
			LocalFlexController_t controller = m_HeadFlxs[i];
			if (controller >= 0)
			{
				int iSeed = SeedFileLineHash(m_iHeadRndSeed, "", controller);
				faceRandom.SetSeed(iSeed);

				m_HeadFlxWgts[i] = faceRandom.RandomFloat();
			}
		}
#endif


		for (int i = 0; i < NUM_RND_HEAD_FLEXES; i++)
		{
			m_HeadFlxs[i] = FindFlexController(g_szRandomFlexControls[i]);
		}
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Default implementation
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
#define BASE_ARGS bFlexEvents
#define FLEX_EVENTS bFlexEvents
void C_NPC_BaseColleague::ProcessSceneEvents(bool bFlexEvents)
#else
#define BASE_ARGS
#define FLEX_EVENTS true
void CNPC_BaseColleague::ProcessSceneEvents(void)
#endif // CLIENT_DLL
{
	BaseClass::ProcessSceneEvents(BASE_ARGS);

	CStudioHdr *hdr = GetModelPtr();
	if (!hdr)
	{
		return;
	}

	

	if (FLEX_EVENTS)
	{
		for (int i = 0; i < NUM_RND_HEAD_FLEXES; i++)
		{
			LocalFlexController_t controller = m_HeadFlxs[i];
			if (controller >= 0)
			{
				SetFlexWeight(controller, m_HeadFlxWgts[i]);
			}
		}
	}
}

