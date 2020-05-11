//---------------------------------------------------------------
// basecollegue_shared.cpp: Client/Server shared functions for
// the CNPC_BaseColleague baseclass. This class is the base for
// the recreations of the humanoid ally NPCs from 'Black Mesa'.
// The code in this file handles the randomization of head
// shapes.
//
// Author: Petercov (petercov@outlook.com)
//---------------------------------------------------------------

#include "cbase.h"
#include "bms_utils.h"

#ifdef CLIENT_DLL
#include "c_npc_basecollegue.h"
#else
#include "npc_basecollegue.h"
#include "networkstringtable_gamedll.h"
#endif // CLIENT_DLL

IMPLEMENT_NETWORKCLASS_ALIASED(NPC_BaseColleague, DT_BaseColleague);

BEGIN_NETWORK_TABLE(CNPC_BaseColleague, DT_BaseColleague)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_nFlexTableIndex)),
#else
SendPropInt(SENDINFO(m_nFlexTableIndex), MAX_FLEXDATA_STRING_BITS),
#endif // CLIENT_DLL
END_NETWORK_TABLE();

//-----------------------------------------------------------------------------
// Purpose: initialize fast lookups when model changes
//-----------------------------------------------------------------------------

CStudioHdr *CNPC_BaseColleague::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_FlexControllers.Purge();

	if (hdr)
	{
		for (int i = 0; i < m_FlexData.Count(); i++)
		{
			LocalFlexController_t controller = FindFlexController(m_FlexData[i].cName);
			m_FlexControllers.AddToTail(controller);
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
		Assert(m_FlexControllers.Count() == m_FlexData.Count());
		for (int i = 0; i < m_FlexControllers.Count(); i++)
		{
			SetFlexWeight(m_FlexControllers[i], m_FlexData[i].flValue);
		}
	}
}

