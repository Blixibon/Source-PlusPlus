//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom C_PlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_laz_player_resource.h"
#include <shareddefs.h>
#include "hud.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT(C_LAZ_PlayerResource, DT_LAZPlayerResource, CLAZPlayerResource)
RecvPropArray3(RECVINFO_ARRAY(m_vPlayerColors), RecvPropVector(RECVINFO(m_vPlayerColors[0]), SPROP_NORMAL)),
END_RECV_TABLE();


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_LAZ_PlayerResource::C_LAZ_PlayerResource()
{
	m_Colors[TEAM_UNASSIGNED] = COLOR_TF_SPECTATOR;
	m_Colors[TEAM_SPECTATOR] = COLOR_TF_SPECTATOR;
	m_Colors[TF_TEAM_RED] = COLOR_RED;
	m_Colors[TF_TEAM_BLUE] = COLOR_BLUE;
	m_Colors[TF_TEAM_GREEN] = COLOR_GREEN;
	m_Colors[TF_TEAM_YELLOW] = COLOR_YELLOW;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_LAZ_PlayerResource::~C_LAZ_PlayerResource()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color C_LAZ_PlayerResource::GetPlayerColor(int iIndex, int iColor)
{ 
	if (!IsConnected(iIndex) || iColor < 0 || iColor >= NUM_PLAYER_COLORS)
		return COLOR_BLACK;

	Vector& vColor = m_vPlayerColors[(iIndex * NUM_PLAYER_COLORS) + iColor];

	//Msg("Client %f %f %f\n", m_iColors[iIndex].x, m_iColors[iIndex].y, m_iColors[iIndex].z);
	return Color(vColor.x * 255.0, vColor.y * 255.0, vColor.z * 255.0, 255);
}

Vector C_LAZ_PlayerResource::GetPlayerColorVector(int iIndex, int iColor)
{
	if (!IsConnected(iIndex) || iColor < 0 || iColor >= NUM_PLAYER_COLORS)
		return Vector(0);

	const Vector& vColor = m_vPlayerColors[(iIndex * NUM_PLAYER_COLORS) + iColor];

	return vColor;
}
