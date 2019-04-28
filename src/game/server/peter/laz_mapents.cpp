#include "cbase.h"
#include "laz_mapents.h"
#include "team_control_point.h"
#include "team_control_point_round.h"
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

DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_iszControlPointName, FIELD_STRING, "controlpoint"),
DEFINE_KEYFIELD(m_iszRoundBlueSpawn, FIELD_STRING, "round_bluespawn"),
DEFINE_KEYFIELD(m_iszRoundRedSpawn, FIELD_STRING, "round_redspawn"),

// Inputs.
DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
DEFINE_INPUTFUNC(FIELD_VOID, "RoundSpawn", InputRoundSpawn),

// Outputs.

END_DATADESC()

LINK_ENTITY_TO_CLASS(info_player_teamspawn, CLazTeamSpawn);

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLazTeamSpawn::CLazTeamSpawn()
{
	m_bDisabled = false;
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

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazTeamSpawn::InputEnable(inputdata_t& inputdata)
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLazTeamSpawn::InputDisable(inputdata_t& inputdata)
{
	m_bDisabled = true;
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

		if (m_bDisabled)
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
}
#pragma endregion