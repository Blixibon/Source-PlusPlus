//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "laz_player.h"
#include "player_resource.h"
#include "laz_player_resource.h"
#include "tf_shareddefs.h"
#include <coordsize.h>

// Datatable
IMPLEMENT_SERVERCLASS_ST( CLAZPlayerResource, DT_LAZPlayerResource )
SendPropArray3(SENDINFO_ARRAY3(m_vPlayerColors), SendPropVector(SENDINFO_ARRAY(m_vPlayerColors), 12, SPROP_NORMAL)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( laz_player_manager, CLAZPlayerResource );

CLAZPlayerResource::CLAZPlayerResource( void )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLAZPlayerResource::UpdatePlayerData( void )
{
	int i;

	BaseClass::UpdatePlayerData();

	for ( i = 1 ; i <= gpGlobals->maxClients; i++ )
	{
		CLaz_Player *pPlayer = (CLaz_Player*)UTIL_PlayerByIndex( i );
		
		if ( pPlayer && pPlayer->IsConnected() )
		{			
			for (int j = 0; j < NUM_PLAYER_COLORS; j++)
			{
				m_vPlayerColors.Set(((i-1) * NUM_PLAYER_COLORS) + j, pPlayer->m_vecPlayerColors[j]);
			}
		}
	}
}

void CLAZPlayerResource::Spawn( void )
{
	int i;

	for ( i = 0; i < MAX_PLAYERS; i++ )
	{
		for (int j = 0; j < NUM_PLAYER_COLORS; j++)
		{
			m_vPlayerColors.Set((i * NUM_PLAYER_COLORS) + j, Vector(0));
		}
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CLAZPlayerResource::GetPlayerColor(int iIndex, int iColor)
{
	if (iIndex < 1 || iIndex > MAX_PLAYERS || iColor < 0 || iColor >= NUM_PLAYER_COLORS)
		return COLOR_BLACK;

	const Vector& vColor = m_vPlayerColors.Get(((iIndex-1) * NUM_PLAYER_COLORS) + iColor);

	//Msg("Client %f %f %f\n", m_iColors[iIndex].x, m_iColors[iIndex].y, m_iColors[iIndex].z);
	return Color(vColor.x * 255.0, vColor.y * 255.0, vColor.z * 255.0, 255);
}