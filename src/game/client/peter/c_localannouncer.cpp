#include "cbase.h"
#include "c_localannouncer.h"
#include "igamesystem.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "GameEventListener.h"
#include "utldict.h"
#include "c_team.h"
#include "eiface.h"
#include "engine/IEngineSound.h"
#include "hud_macros.h"

#define ANNOUNCER_SCRIPT "scripts/announcers.vdf"

void AnnouncerChangedCallback(IConVar* var, const char* pOldValue, float flOldValue);
ConVar cl_clientannouncer("cl_playerannouncer", "team", FCVAR_ARCHIVE, "Which announcer to use. Use \"team\" for team-based announcer.", AnnouncerChangedCallback);

const char* g_pszAnnouncementTypes[NUM_ANNOUNCEMENTS] = {
	"round_timer_1min",
	"round_timer_30sec",
	"round_timer_10sec",
	"round_timer_5sec",
	"round_timer_4sec",
	"round_timer_3sec",
	"round_timer_2sec",
	"round_timer_1sec",
	"round_timer_over",

	"setup_timer_60sec",
	"setup_timer_30sec",
	"setup_timer_10sec",
	"setup_timer_5sec",
	"setup_timer_4sec",
	"setup_timer_3sec",
	"setup_timer_2sec",
	"setup_timer_1sec",
	"setup_timer_over",

	"round_time_added",
	"round_time_added_loser",
	"round_time_added_winner",

	"round_timer_15min",
	"round_timer_10min",
	"round_timer_5min",
	"round_timer_4min",
	"round_timer_3min",
	"round_timer_2min",
	//"round_timer_1min",
};

int GetAnnouncementTypeFromString(const char* pszType)
{
	for (int i = 0; i < NUM_ANNOUNCEMENTS; i++)
	{
		if (V_stricmp(pszType, g_pszAnnouncementTypes[i]) == 0)
			return i;
	}

	return ANNOUNCE_INVALID;
}

typedef struct announcement_s
{
	HSOUNDSCRIPTHANDLE hSound;
	char chSound[MAX_PATH];

	announcement_s()
	{
		hSound = SOUNDEMITTER_INVALID_HANDLE;
		V_memset(chSound, 0, MAX_PATH);
	}
} announcement_t;

typedef struct annoucerType_s
{
	//char chName[32];
	announcement_t lines[NUM_ANNOUNCEMENTS];
} announcerType_t;

class CLocalAnnouncer :
	public CAutoGameSystem, public CGameEventListener, public ILocalAnnouncer
{
public:
	CLocalAnnouncer() : CAutoGameSystem("LocalAnnouncerSystem")
	{}

	bool	Init();
	void	LevelInitPreEntity();
	void    DispatchAnnouncement(int iAnnouncement);
	void	SelectAnnouncer();
	virtual void FireGameEvent(IGameEvent* event);

	void MsgFunc_TeamplayAnnouncement(bf_read& buf);
protected:
	CUtlDict<announcerType_t> m_Announcers;
	int	m_iCurrentAnnouncer;
};

CLocalAnnouncer g_LocalAnnouncer;

void AnnouncerChangedCallback(IConVar* var, const char* pOldValue, float flOldValue)
{
	g_LocalAnnouncer.SelectAnnouncer();
}

ILocalAnnouncer* GetAnnouncer()
{
	return &g_LocalAnnouncer;
}

DECLARE_MESSAGE(g_LocalAnnouncer, TeamplayAnnouncement);

bool CLocalAnnouncer::Init()
{
	ListenForGameEvent("localplayer_changeteam");
	usermessages->HookMessage("TeamplayAnnouncement", __MsgFunc_g_LocalAnnouncer_TeamplayAnnouncement);

	KeyValuesAD pKV("Announcers");
	if (pKV->LoadFromFile(filesystem, ANNOUNCER_SCRIPT, "GAME"))
	{
		for (KeyValues* pkvAnnouncer = pKV->GetFirstTrueSubKey(); pkvAnnouncer != nullptr; pkvAnnouncer = pkvAnnouncer->GetNextTrueSubKey())
		{
			int iAnnouncer = m_Announcers.Insert(pkvAnnouncer->GetName());
			announcerType_t& announcer = m_Announcers[iAnnouncer];
			//V_strcpy_safe(announcer.chName, pkvAnnouncer->GetName());

			for (KeyValues* pkvValue = pkvAnnouncer->GetFirstValue(); pkvValue != nullptr; pkvValue = pkvValue->GetNextValue())
			{
				int iType = GetAnnouncementTypeFromString(pkvValue->GetName());
				if (iType == ANNOUNCE_INVALID)
					continue;

				V_strcpy_safe(announcer.lines[iType].chSound, pkvValue->GetString());
			}
		}
	}

	return true;
}

void CLocalAnnouncer::LevelInitPreEntity()
{
	for (unsigned int i = 0; i < m_Announcers.Count(); i++)
	{
		announcerType_t& announcer = m_Announcers.Element(i);
		for (int j = 0; j < NUM_ANNOUNCEMENTS; j++)
		{
			announcement_t& line = announcer.lines[j];
			if (line.chSound)
				line.hSound = C_BaseEntity::PrecacheScriptSound(line.chSound);
		}
	}
}

void CLocalAnnouncer::FireGameEvent(IGameEvent* event)
{
	const char* name = event->GetName();
	if (0 == Q_strcmp(name, "localplayer_changeteam"))
	{
		SelectAnnouncer();
	}
}

void CLocalAnnouncer::MsgFunc_TeamplayAnnouncement(bf_read& buf)
{
	long lAnnouncement = buf.ReadLong();

	DispatchAnnouncement(lAnnouncement);
}

void CLocalAnnouncer::SelectAnnouncer()
{
	if (engine->IsInGame())
	{
		const char* pszAnnouncer = cl_clientannouncer.GetString();
		if (V_stricmp(pszAnnouncer, "team") == 0)
		{
			pszAnnouncer = GetLocalTeam() ? GetLocalTeam()->Get_Name() : "Default";
		}

		int iAnnouncer = m_Announcers.Find(pszAnnouncer);
		if (m_Announcers.IsValidIndex(iAnnouncer))
		{
			m_iCurrentAnnouncer = iAnnouncer;
			return;
		}
	}

	m_iCurrentAnnouncer = m_Announcers.InvalidIndex();
}

void CLocalAnnouncer::DispatchAnnouncement(int iAnnouncement)
{
	if (m_Announcers.IsValidIndex(m_iCurrentAnnouncer) && iAnnouncement > ANNOUNCE_INVALID && iAnnouncement < NUM_ANNOUNCEMENTS)
	{
		announcerType_t& announcer = m_Announcers.Element(m_iCurrentAnnouncer);
		announcement_t& line = announcer.lines[iAnnouncement];

		if (line.chSound[0] == '\0')
			return;

		EmitSound_t ep;
		CSoundParameters params;
		if (line.hSound != SOUNDEMITTER_INVALID_HANDLE && C_BaseEntity::GetParametersForSound(line.chSound, line.hSound, params, "models/error.mdl"))
		{
			ep = EmitSound_t(params);
		}
		else
		{
			ep.m_pSoundName = line.chSound;
			ep.m_flVolume = VOL_NORM;
			ep.m_nPitch = PITCH_NORM;
			ep.m_nFlags = SND_NOFLAGS;
			ep.m_bEmitCloseCaption = false;
		}

		ep.m_nChannel = CHAN_VOICE2;
		ep.m_SoundLevel = SNDLVL_NONE;

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound(filter, SOUND_FROM_WORLD, ep);
	}
}