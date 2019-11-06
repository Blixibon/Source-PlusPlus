//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "hltvdirector.h"
#include "team_control_point.h"


CBaseEntity* GetCapturePointByIndex(int iCaptureIndex)
{
	CTeamControlPoint *pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname(NULL, "team_control_point");

	while (pTeamControlPoint)
	{
		if (pTeamControlPoint->GetPointIndex() == iCaptureIndex)
		{
			return pTeamControlPoint;
		}

		pTeamControlPoint = (CTeamControlPoint *)gEntList.FindEntityByClassname(pTeamControlPoint, "team_control_point");
	}

	return NULL;
}

class CLAZHLTVDirector : public CHLTVDirector, public IEntityListener
{
public:
	DECLARE_CLASS(CLAZHLTVDirector, CHLTVDirector);

	const char** GetModEvents();
	void SetHLTVServer(IHLTVServer *hltv);
	void CreateShotFromEvent(CHLTVGameEvent *event);

	virtual const char	*GetFixedCameraEntityName(void) { return "info_observer_point"; }

	virtual void	BuildCameraList(void);

	virtual void OnEntityDeleted(CBaseEntity *pEntity);
	virtual void	LevelInitPostEntity();
	virtual void	LevelShutdownPreEntity();
};

void CLAZHLTVDirector::SetHLTVServer(IHLTVServer *hltv)
{
	BaseClass::SetHLTVServer(hltv);

	if (m_pHLTVServer)
	{
		// mod specific events the director uses to find interesting shots
		ListenForGameEvent("teamplay_point_captured");
		ListenForGameEvent("teamplay_capture_blocked");
		ListenForGameEvent("teamplay_point_startcapture");
		ListenForGameEvent("teamplay_flag_event");
		ListenForGameEvent("teamplay_round_start");
		ListenForGameEvent("ctf_flag_captured");
		ListenForGameEvent("npc_death");
	}
}

void CLAZHLTVDirector::CreateShotFromEvent(CHLTVGameEvent *event)
{
	// show event at least for 2 more seconds after it occured
	const char *name = event->m_Event->GetName();

	int thera = RandomFloat() > 0.5 ? 20 : -20;

	if (!Q_strcmp("teamplay_point_startcapture", name) ||
		!Q_strcmp("teamplay_point_captured", name) ||
		!Q_strcmp("teamplay_capture_blocked", name))
	{
		CBaseEntity *pCapturePoint = GetCapturePointByIndex(event->m_Event->GetInt("cp"));

		int iCameraIndex = -1;
		float flClosest = 99999.9f;

		if (pCapturePoint)
		{
			// Does it have an associated viewpoint?
			for (int i = 0; i < m_nNumFixedCameras; i++)
			{
				CBaseEntity *pCamera = m_pFixedCameras[i];

				if (pCamera)
				{
					byte pvs[MAX_MAP_CLUSTERS / 8];
					int clusterIndex = engine->GetClusterForOrigin(pCamera->GetAbsOrigin());
					engine->GetPVSForCluster(clusterIndex, sizeof(pvs), pvs);
					bool bCameraInPVS = engine->CheckOriginInPVS(pCapturePoint->GetAbsOrigin(), pvs, sizeof(pvs));

					if (bCameraInPVS == true)
					{
						float flDistance = (pCapturePoint->GetAbsOrigin() - pCamera->GetAbsOrigin()).Length();
						if (flDistance <= flClosest)
						{
							iCameraIndex = i;
							flClosest = flDistance;
						}
					}
				}
			}
		}

		CBasePlayer *pPlayer = NULL;

		if (!Q_strcmp("teamplay_point_captured", name))
		{
			const char *pszCappers = event->m_Event->GetString("cappers");
			int nLength = Q_strlen(pszCappers);

			if (nLength > 0)
			{
				int iRandomCapper = pszCappers[RandomInt(0, nLength - 1)];
				pPlayer = UTIL_PlayerByIndex(iRandomCapper);
			}
		}
		else if (!Q_strcmp("teamplay_capture_blocked", name))
		{
			int iBlocker = event->m_Event->GetInt("blocker");
			pPlayer = UTIL_PlayerByIndex(iBlocker);
		}

		if (pPlayer)
		{
			if (iCameraIndex >= 0 && RandomFloat() > 0.66f)
			{
				StartFixedCameraShot(iCameraIndex, pPlayer->entindex());
			}
			else if (pCapturePoint)
			{
				StartChaseCameraShot(pPlayer->entindex(), pCapturePoint->entindex(), 96, 20, thera, false);
			}
			else
			{
				StartChaseCameraShot(pPlayer->entindex(), 0, 96, 20, 0, false);
			}
		}
		else if (iCameraIndex >= 0 && pCapturePoint)
		{
			// no player known for this event
			StartFixedCameraShot(iCameraIndex, pCapturePoint->entindex());
		}

		// shot 2 seconds after event
		m_nNextShotTick = min(m_nNextShotTick, (event->m_Tick + TIME_TO_TICKS(1.0)));
	}
	else if (!Q_strcmp("object_destroyed", name))
	{
		CBasePlayer *attacker = UTIL_PlayerByUserId(event->m_Event->GetInt("attacker"));
		if (attacker)
		{
			int iObjectIndex = event->m_Event->GetInt("index");
			StartChaseCameraShot(attacker->entindex(), iObjectIndex, 96, 20, thera, false);
		}
	}
	else if (!Q_strcmp("ctf_flag_captured", name))
	{
		CBasePlayer *capper = UTIL_PlayerByUserId(event->m_Event->GetInt("capper"));
		if (capper)
		{
			StartChaseCameraShot(capper->entindex(), 0, 96, 20, 0, false);
		}
	}
	else if (!Q_strcmp("teamplay_flag_event", name))
	{
		StartChaseCameraShot(event->m_Event->GetInt("player"), 0, 96, 20, 0, false);
	}
	else if (!Q_strcmp("npc_death", name))
	{
		CBaseEntity *victim = UTIL_EntityByIndex(event->m_Event->GetInt("victim_index"));
		CBaseEntity *attacker = UTIL_EntityByIndex(event->m_Event->GetInt("attacker_index"));

		if (!victim)
			return;

		if (attacker == victim || attacker == NULL)
		{
			// player killed self or by WORLD
			StartChaseCameraShot(victim->entindex(), 0, 96, 20, 0, false);
		}
		else // attacker != NULL
		{
			// if we show ineye view, show it more likely from killer
			if (RandomFloat(0, 1) > (0.7f))
			{
				::V_swap(attacker, victim);
			}

			// hurting a victim is shown as chase more often
			// view from behind over head
			// lower view point, dramatic
			// view over shoulder, randomly left or right
			StartChaseCameraShot(victim->entindex(), attacker->entindex(), 96, -20, (RandomFloat() > 0.5) ? 30 : -30, false);
		}

		// shot 2 seconds after death/hurt
		m_nNextShotTick = MIN(m_nNextShotTick, (event->m_Tick + TIME_TO_TICKS(2.0)));
	}
	else if (!Q_strcmp("teamplay_round_start", name))
	{
		StartBestFixedCameraShot(false);
	}
	else
	{

		// let baseclass create a shot
		BaseClass::CreateShotFromEvent(event);
	}
}

void CLAZHLTVDirector::BuildCameraList(void)
{
	m_nNumFixedCameras = 0;
	memset(m_pFixedCameras, 0, sizeof(m_pFixedCameras));

	CBaseEntity *pCamera = gEntList.FindEntityByClassname(NULL, GetFixedCameraEntityName());

	while (pCamera && m_nNumFixedCameras < MAX_NUM_CAMERAS)
	{
		CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, STRING(pCamera->m_target));

		if (pTarget)
		{
			// look at target if any given
			QAngle angles;
			VectorAngles(pTarget->GetAbsOrigin() - pCamera->GetAbsOrigin(), angles);
			pCamera->SetAbsAngles(angles);
		}

		m_pFixedCameras[m_nNumFixedCameras] = pCamera;

		m_nNumFixedCameras++;
		pCamera = gEntList.FindEntityByClassname(pCamera, GetFixedCameraEntityName());
	}

	//pCamera = gEntList.FindEntityByClassname(NULL, "point_viewcontrol");

	//while (pCamera && m_nNumFixedCameras < MAX_NUM_CAMERAS)
	//{
	//	CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, STRING(pCamera->m_target));

	//	if (pTarget)
	//	{
	//		// look at target if any given
	//		QAngle angles;
	//		VectorAngles(pTarget->GetAbsOrigin() - pCamera->GetAbsOrigin(), angles);
	//		pCamera->SetAbsAngles(angles);
	//	}

	//	m_pFixedCameras[m_nNumFixedCameras] = pCamera;

	//	m_nNumFixedCameras++;
	//	pCamera = gEntList.FindEntityByClassname(pCamera, "point_viewcontrol");
	//}

	//pCamera = gEntList.FindEntityByClassname(NULL, "point_camera");

	//while (pCamera && m_nNumFixedCameras < MAX_NUM_CAMERAS)
	//{
	//	CBaseEntity *pTarget = gEntList.FindEntityByName(NULL, STRING(pCamera->m_target));

	//	if (pTarget)
	//	{
	//		// look at target if any given
	//		QAngle angles;
	//		VectorAngles(pTarget->GetAbsOrigin() - pCamera->GetAbsOrigin(), angles);
	//		pCamera->SetAbsAngles(angles);
	//	}

	//	m_pFixedCameras[m_nNumFixedCameras] = pCamera;

	//	m_nNumFixedCameras++;
	//	pCamera = gEntList.FindEntityByClassname(pCamera, "point_camera");
	//}
}

void CLAZHLTVDirector::OnEntityDeleted(CBaseEntity * pEntity)
{
	/*bool bRefresh = false;

	for (int i = 0; i < m_nNumFixedCameras; i++)
	{
		if (m_pFixedCameras[i] == pEntity)
		{
			bRefresh = true;
			break;
		}
	}

	if (bRefresh)
		BuildCameraList();*/
}

void CLAZHLTVDirector::LevelInitPostEntity()
{
	BaseClass::LevelInitPostEntity();

	//gEntList.AddListenerEntity(this);
}

void CLAZHLTVDirector::LevelShutdownPreEntity()
{
	//gEntList.RemoveListenerEntity(this);
}

const char** CLAZHLTVDirector::GetModEvents()
{
	// game events relayed to spectator clients
	static const char *s_modevents[] =
	{
		"game_newmap",
		"hltv_status",
		"hltv_chat",
		"player_connect",
		"player_disconnect",
		"player_changeclass",
		"player_team",
		"player_info",
		"player_death",
		"player_chat",
		"player_spawn",
		"round_start",
		"round_end",
		"server_cvar",
		"server_spawn",
		"npc_death",

		// additional TF events:
		"controlpoint_starttouch",
		"controlpoint_endtouch",
		"teamplay_capture_broken",
		"ctf_flag_captured",
		"teamplay_broadcast_audio",
		"teamplay_capture_blocked",
		"teamplay_flag_event",
		"teamplay_game_over",
		"teamplay_point_captured",
		"teamplay_round_stalemate",
		"teamplay_round_start",
		"teamplay_round_win",
		"teamplay_timer_time_added",
		"teamplay_update_timer",
		"teamplay_win_panel",
		"tf_game_over",
		"object_destroyed",

		NULL
	};

	return s_modevents;
}

static CLAZHLTVDirector s_HLTVDirector;	// singleton

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CHLTVDirector, IHLTVDirector, INTERFACEVERSION_HLTVDIRECTOR, s_HLTVDirector);

CHLTVDirector* HLTVDirector()
{
	return &s_HLTVDirector;
}

IGameSystem* HLTVDirectorSystem()
{
	return &s_HLTVDirector;
}