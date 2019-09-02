#include "cbase.h"
#include "laz_mapents.h"
#include "team_control_point.h"
#include "team_control_point_round.h"
#include "team_control_point_master.h"
#include "items.h"
#include "laz_player.h"
#include "econ_item_system.h"

#pragma region EQUIP
LINK_ENTITY_TO_CLASS(info_player_equip, CLazPlayerEquip);

BEGIN_DATADESC(CLazPlayerEquip)

DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_Armor, FIELD_INTEGER, "item_Armor"),

// Input functions
DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
DEFINE_INPUTFUNC(FIELD_VOID, "EquipPlayer", InputEquipPlayer),
DEFINE_INPUTFUNC(FIELD_VOID, "EquipAllPlayers", InputEquipAllPlayers),

END_DATADESC()

bool CLazPlayerEquip::KeyValue(const char* szKeyName, const char* szValue)
{
	if (!BaseClass::KeyValue(szKeyName, szValue))
	{
		int iValue = atoi(szValue);
		if (!V_strncmp(szKeyName, "weapon_", 7))
		{
			if (iValue > 0)
			{
				for (int i = 0; i < MAX_WEAPONS; i++)
				{
					if (!m_weaponNames[i])
					{
						char tmp[128];

						UTIL_StripToken(szKeyName, tmp);

						m_weaponNames[i] = AllocPooledString(tmp);
						m_weaponClipEmpty[i] = (iValue > 1);
						return true;
					}
				}
			}
		}
		else if (!V_strncmp(szKeyName, "ammo_", 5))
		{
			if (iValue > 0)
			{
				const char* pszAmmo = szKeyName + 5;

				for (int i = 0; i < MAX_AMMO_TYPES; i++)
				{
					if (!m_ammoNames[i])
					{
						m_ammoNames[i] = AllocPooledString(pszAmmo);
						m_ammoCounts[i] = iValue;
						return true;
					}
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputEnable(inputdata_t& inputdata)
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputDisable(inputdata_t& inputdata)
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputEquipPlayer(inputdata_t& inputdata)
{
	CBasePlayer* pPlayer = ToBasePlayer(inputdata.pActivator);
	if (pPlayer)
	{
		EquipPlayer(pPlayer);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazPlayerEquip::InputEquipAllPlayers(inputdata_t& inputdata)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer && CanEquipTeam(pPlayer->GetTeamNumber()))
		{
			EquipPlayer(pPlayer);
		}
	}
}

void CLazPlayerEquip::EquipPlayer(CBaseEntity* pEntity)
{
	CBasePlayer* pPlayer = ToBasePlayer(pEntity);

	if (!pPlayer)
		return;

	pPlayer->IncrementArmorValue(m_Armor, m_Armor);

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (!m_weaponNames[i])
			break;

		CBaseEntity *pSpawn = pPlayer->GiveNamedItem(STRING(m_weaponNames[i]));

		if (pSpawn && !pSpawn->IsMarkedForDeletion() && m_weaponClipEmpty[i])
		{
			CBaseCombatWeapon* pWeapon = pSpawn->MyCombatWeaponPointer();
			if (pWeapon)
			{
				pWeapon->m_iClip1 = 0;
			}
		}
	}

	for (int i = 0; i < MAX_AMMO_TYPES; i++)
	{
		if (!m_ammoNames[i])
			break;

		pPlayer->GiveAmmo(m_ammoCounts[i], STRING(m_ammoNames[i]), true);
	}
}
#pragma endregion
#pragma region SPAWN
//=============================================================================
//
// CLazTeamSpawn tables.
//
BEGIN_DATADESC(CLazTeamSpawn)

DEFINE_KEYFIELD(m_iszControlPointName, FIELD_STRING, "controlpoint"),
DEFINE_KEYFIELD(m_iszRoundBlueSpawn, FIELD_STRING, "round_bluespawn"),
DEFINE_KEYFIELD(m_iszRoundRedSpawn, FIELD_STRING, "round_redspawn"),

// Inputs.
DEFINE_INPUTFUNC(FIELD_VOID, "RoundSpawn", InputRoundSpawn),

// Outputs.

END_DATADESC();

LINK_ENTITY_TO_CLASS(info_player_teamspawn, CLazTeamSpawn);

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLazTeamSpawn::CLazTeamSpawn()
{
	gameeventmanager->AddListener(this, "teamplay_point_captured", true);
	m_iDisabled = 0;
}

CLazTeamSpawn::~CLazTeamSpawn()
{
	gameeventmanager->RemoveListener(this);
}

void CLazTeamSpawn::FireGameEvent(IGameEvent * event)
{
	if (FStrEq(event->GetName(), "teamplay_point_captured"))
	{
		int iCP = event->GetInt("cp");
		if (m_hControlPoint && m_hControlPoint->GetPointIndex() == iCP)
		{
			if (GetTeam())
				GetTeam()->RemoveSpawnpoint(this);

			ChangeTeam(m_hControlPoint->GetTeamNumber());
			UpdateTeam();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLazTeamSpawn::Activate(void)
{
	BaseClass::Activate();

	Vector mins = g_pGameRules->GetViewVectors()->m_vHullMin;
	Vector maxs = g_pGameRules->GetViewVectors()->m_vHullMax;

	trace_t trace;
	UTIL_TraceHull(GetAbsOrigin(), GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);
	bool bClear = (trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1));
	if (!bClear)
	{
		Warning("Spawnpoint at (%.2f %.2f %.2f) is not clear.\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
		// m_debugOverlays |= OVERLAY_TEXT_BIT;
	}
}

void CLazTeamSpawn::UpdateTeam()
{
	CTeam* team = GetGlobalTeam(GetTeamNumber());
	if (team && GetTeamNumber() > LAST_SHARED_TEAM && GetTeamNumber() < TF_TEAM_COUNT)
	{
		team->AddSpawnpoint(this);
	}
}

bool CLazTeamSpawn::IsValid(CBasePlayer * pPlayer)
{
	CBaseEntity *ent = NULL;
	for (CEntitySphereQuery sphere(GetAbsOrigin(), 128); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
	{
		// if ent is a client, don't spawn on 'em
		CBaseEntity *plent = ent;
		if (plent)
		{
			if (plent->IsPlayer())
			{
				CBasePlayer *pOther = ToBasePlayer(plent);
				if (pOther->IsObserver())
					continue;
			}

			if (plent->GetTeamNumber() == pPlayer->GetTeamNumber())
				continue;

			return false;
		}
	}

	return m_iDisabled == 0;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CLazTeamSpawn::DrawDebugTextOverlays(void)
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		char tempstr[512];
		Q_snprintf(tempstr, sizeof(tempstr), "TeamNumber: %d", GetTeamNumber());
		EntityText(text_offset, tempstr, 0);
		text_offset++;

		color32 teamcolor = g_aTeamColors[GetTeamNumber()];
		teamcolor.a = 0;

		if (m_iDisabled)
		{
			Q_snprintf(tempstr, sizeof(tempstr), "DISABLED");
			EntityText(text_offset, tempstr, 0);
			text_offset++;

			teamcolor.a = 255;
		}

		// Make sure it's empty
		Vector mins = g_pGameRules->GetViewVectors()->m_vHullMin;
		Vector maxs = g_pGameRules->GetViewVectors()->m_vHullMax;

		Vector vTestMins = GetAbsOrigin() + mins;
		Vector vTestMaxs = GetAbsOrigin() + maxs;

		// First test the starting origin.
		if (UTIL_IsSpaceEmpty(NULL, vTestMins, vTestMaxs))
		{
			NDebugOverlay::Box(GetAbsOrigin(), mins, maxs, teamcolor.r, teamcolor.g, teamcolor.b, teamcolor.a, 0.1);
		}
		else
		{
			NDebugOverlay::Box(GetAbsOrigin(), mins, maxs, 0, 255, 0, 0, 0.1);
		}

		if (m_hControlPoint)
		{
			NDebugOverlay::Line(GetAbsOrigin(), m_hControlPoint->GetAbsOrigin(), teamcolor.r, teamcolor.g, teamcolor.b, false, 0.1);
		}
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLazTeamSpawn::InputRoundSpawn(inputdata_t& input)
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if (m_iszControlPointName != NULL_STRING)
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hControlPoint = dynamic_cast<CTeamControlPoint*>(gEntList.FindEntityByName(NULL, m_iszControlPointName));
		if (!m_hControlPoint)
		{
			Warning("%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName));
		}
	}

	if (m_iszRoundBlueSpawn != NULL_STRING)
	{
		// We need to re-find our control point round, because they're recreated over round restarts
		m_hRoundBlueSpawn = dynamic_cast<CTeamControlPointRound*>(gEntList.FindEntityByName(NULL, m_iszRoundBlueSpawn));
		if (!m_hRoundBlueSpawn)
		{
			Warning("%s failed to find control point round named '%s'\n", GetClassname(), STRING(m_iszRoundBlueSpawn));
		}
	}

	if (m_iszRoundRedSpawn != NULL_STRING)
	{
		// We need to re-find our control point round, because they're recreated over round restarts
		m_hRoundRedSpawn = dynamic_cast<CTeamControlPointRound*>(gEntList.FindEntityByName(NULL, m_iszRoundRedSpawn));
		if (!m_hRoundRedSpawn)
		{
			Warning("%s failed to find control point round named '%s'\n", GetClassname(), STRING(m_iszRoundRedSpawn));
		}
	}

	if (GetTeam())
		GetTeam()->RemoveSpawnpoint(this);

	int iNewTeam = TEAM_UNASSIGNED;
	char cValue[8];
	if (GetKeyValue("TeamNum", cValue, 8))
		iNewTeam = atoi(cValue);

	if (pMaster)
	{
		if (m_hControlPoint)
		{
			iNewTeam = m_hControlPoint->GetTeamNumber();
		}
		else if (m_hRoundBlueSpawn || m_hRoundRedSpawn)
		{
			if (m_hRoundBlueSpawn && pMaster->GetCurrentRound() == m_hRoundBlueSpawn)
			{
				iNewTeam = TF_TEAM_BLUE;
			}
			else if (m_hRoundRedSpawn && pMaster->GetCurrentRound() == m_hRoundRedSpawn)
			{
				iNewTeam = TF_TEAM_RED;
			}
		}
	}

	if (iNewTeam > LAST_SHARED_TEAM && iNewTeam < TF_TEAM_COUNT)
	{
		ChangeTeam(iNewTeam);
		UpdateTeam();
	}
}
void CLazTeamSpawn::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	gameeventmanager->RemoveListener(this);
}
#pragma endregion

class CEconPickup : public CItem
{
public:
	DECLARE_CLASS(CEconPickup, CItem);

	void Spawn(void);
	void Precache(void);

	virtual bool MyTouch(CBasePlayer* pPlayer);

protected:
	virtual int GetItemId() { return -1; }
	CEconItemView m_Item;
};

void CEconPickup::Spawn(void)
{
	CEconItemDefinition* pItem = GetItemSchema()->GetItemDefinition(GetItemId());
	if (!pItem)
	{
		UTIL_Remove(this);
		return;
	}

	m_Item.Init(GetItemId());

	Precache();
	SetModel(m_Item.GetWorldDisplayModel());

	BaseClass::Spawn();
}

void CEconPickup::Precache(void)
{
	PrecacheModel(m_Item.GetWorldDisplayModel());
}

bool CEconPickup::MyTouch(CBasePlayer* pPlayer)
{
	CLaz_Player* pLaz = ToLazuulPlayer(pPlayer);
	return pLaz->GiveItemById(GetItemId()) != nullptr;
}

class CItemExoJump : public CEconPickup
{
public:
	DECLARE_CLASS(CItemExoJump, CEconPickup);

	void Precache(void);
	virtual int GetItemId() { return 3; }
};

LINK_ENTITY_TO_CLASS(item_exojump, CItemExoJump);

void CItemExoJump::Precache(void)
{
	BaseClass::Precache();
	PrecacheScriptSound("ExoLegs.JumpLand");
}

class CItemKevlar : public CEconPickup
{
public:
	DECLARE_CLASS(CItemKevlar, CEconPickup);

	virtual int GetItemId() { return 2; }
};

LINK_ENTITY_TO_CLASS(item_kevlar, CItemKevlar);

class CItemLongJump : public CEconPickup
{
public:
	DECLARE_CLASS(CItemLongJump, CEconPickup);

	virtual int GetItemId() { return 1; }
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);