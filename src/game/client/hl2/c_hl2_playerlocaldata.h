//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#if !defined( C_HL2_PLAYERLOCALDATA_H )
#define C_HL2_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#define	MAX_SQUAD_MEMBERS	16

#include "dt_recv.h"

#include "hl2/hl_movedata.h"
#include "bitvec.h"
#include "c_ai_basenpc.h"

EXTERN_RECV_TABLE( DT_HL2Local );

typedef CBitVec<MAX_SQUAD_MEMBERS> SquadMedicBits;

class C_HL2PlayerLocalData
{
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( C_HL2PlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	C_HL2PlayerLocalData();

	float	m_flSuitPower;
	bool	m_bZooming;
	int		m_bitsActiveDevices;
	int		m_iSquadMemberCount;
	//int		m_iSquadMedicCount;
	CHandle<C_AI_BaseNPC> m_hPlayerSquad[MAX_SQUAD_MEMBERS];
	SquadMedicBits m_SquadMedicBits;
	bool	m_fSquadInFollowMode;
	bool	m_bWeaponLowered;
	EHANDLE m_hAutoAimTarget;
	Vector	m_vecAutoAimPoint;
	bool	m_bDisplayReticle;
	bool	m_bStickyAutoAim;
	bool	m_bAutoAimTarget;
#ifdef HL2_EPISODIC
	float	m_flFlashBattery;
	Vector	m_vecLocatorOrigin;
#endif

	int		GetSquadMedicCount()
	{
		int iCount = 0;
		for (int i = 0; i < m_SquadMedicBits.GetNumBits(); i++)
		{
			if (m_SquadMedicBits.IsBitSet(i))
				iCount++;
		}
		return iCount;
	}

	// Ladder related data
	EHANDLE			m_hLadder;
	LadderMove_t	m_LadderMove;
};


#endif
