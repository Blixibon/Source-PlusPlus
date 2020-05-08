#include "cbase.h"
#include "baseanimating.h"
#include "peter/laz_mapents.h"
#include "saverestore_utlvector.h"
#include "peter/weapon_emp.h"
#include "lazuul_gamerules.h"
#include "peter/laz_player.h"
#include "te.h"

enum ControlSlots_e
{
	SLOT_SHIELDS = 0,
	SLOT_TURRETS,
	SLOT_MANHACKS,
	SLOT_DOORS,

	NUM_CONTROL_SLOTS
};

ConVar hlss_main_frame_health("hlss_main_frame_health", "2500");

#define SF_INTERFACE_NOT_NPC_HACKABLE 1
#define SF_INTERFACE_DOORS_ARE_GATES 2

class CHLSSCombineInterface : public CLazNetworkEntity< CBaseAnimating >, public ILazNetworkController, public IEMPInteractable
{
public:
	DECLARE_CLASS(CHLSSCombineInterface, CLazNetworkEntity< CBaseAnimating >);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual Class_T Classify();
	virtual void ChangeTeam(int iTeam);

	virtual void Precache();
	virtual void Spawn();
	virtual int	ObjectCaps(void)
	{
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}

	virtual int			UpdateTransmitState();
	virtual int			ShouldTransmit(const CCheckTransmitInfo* pInfo);

	virtual void			Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	virtual bool	HandleEntityCommand(CBasePlayer* pClient, KeyValues* pKeyValues);

	virtual bool EmpCanInteract(CBaseCombatWeapon* pWeapon);
	virtual void EmpNotifyInteraction(CBaseCombatWeapon* pWeapon);

	void	AddEntityToNetwork(ILazNetworkEntity* pEnt);
	void	RemoveEntityFromNetwork(ILazNetworkEntity* pEnt);

	void	SetPowerEnabled(bool bPower, bool bNetwork = false, bool bForce = false);

	void	InputToggle(inputdata_t& inputdata);
	void	InputEnable(inputdata_t& inputdata);
	void	InputDisable(inputdata_t& inputdata);

	virtual void NetworkPowerOn(bool bForce) { SetPowerEnabled(true, true, bForce); }
	virtual void NetworkPowerOff(bool bForce) { SetPowerEnabled(false, true, bForce); }
	virtual LazNetworkRole_t GetNetworkRole() { return NETROLE_SUBNETWORK; }

	void	UpdateSlot(int iSlot);
protected:
	CUtlVector<EHANDLE> m_NetworkEnts;
	bool	m_bIsDisabled;
	bool	m_bHasOwnGenerator;
	bool	m_bForcedOff;
	//bool	m_bSlotActive[NUM_CONTROL_SLOTS];
	//bool	m_bSlotIsAvailable[NUM_CONTROL_SLOTS];
	CNetworkArray(bool, m_bSlotActive, NUM_CONTROL_SLOTS);
	CNetworkArray(bool, m_bSlotIsAvailable, NUM_CONTROL_SLOTS);
	CNetworkHandle(CLaz_Player, m_hInterfacePlayer);
	CNetworkVar(bool, m_bDoorIsGate);

	COutputEvent m_ManhacksEnable;
	COutputEvent m_ManhacksDisable;

	COutputEvent m_DoorsLock;
	COutputEvent m_DoorsUnlock;

	COutputInt m_OnChangedTeam;
};

LINK_ENTITY_TO_CLASS(hlss_combine_interface, CHLSSCombineInterface);

IMPLEMENT_SERVERCLASS_ST(CHLSSCombineInterface, DT_HLSSCombineInterface)
SendPropArray3(SENDINFO_ARRAY3(m_bSlotActive), SendPropBool(SENDINFO_ARRAY(m_bSlotActive))),
SendPropEHandle(SENDINFO(m_hInterfacePlayer)),

SendPropArray3(SENDINFO_ARRAY3(m_bSlotIsAvailable), SendPropBool(SENDINFO_ARRAY(m_bSlotIsAvailable))),
SendPropBool(SENDINFO(m_bDoorIsGate)),
END_SEND_TABLE();

BEGIN_DATADESC(CHLSSCombineInterface)
DEFINE_UTLVECTOR(m_NetworkEnts, FIELD_EHANDLE),
DEFINE_KEYFIELD(m_bIsDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_FIELD(m_bForcedOff, FIELD_BOOLEAN),
DEFINE_FIELD(m_bDoorIsGate, FIELD_BOOLEAN),
DEFINE_KEYFIELD(m_bHasOwnGenerator, FIELD_BOOLEAN, "HasGenerator"),

DEFINE_KEYFIELD(m_bSlotIsAvailable[0], FIELD_BOOLEAN, "slot_shield"),
DEFINE_KEYFIELD(m_bSlotActive[0], FIELD_BOOLEAN, "shields"),
DEFINE_KEYFIELD(m_bSlotIsAvailable[1], FIELD_BOOLEAN, "slot_turrets"),
DEFINE_KEYFIELD(m_bSlotActive[1], FIELD_BOOLEAN, "turrets"),
DEFINE_KEYFIELD(m_bSlotIsAvailable[2], FIELD_BOOLEAN, "slot_manhacks"),
DEFINE_KEYFIELD(m_bSlotActive[2], FIELD_BOOLEAN, "manhacks"),
DEFINE_KEYFIELD(m_bSlotIsAvailable[3], FIELD_BOOLEAN, "slot_doors"),
DEFINE_KEYFIELD(m_bSlotActive[3], FIELD_BOOLEAN, "doors"),

DEFINE_INPUTFUNC(FIELD_VOID, "GeneratorEnabled", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "GeneratorDisabled", InputDisable),

DEFINE_OUTPUT(m_ManhacksEnable, "OnManhacksEnabled"),
DEFINE_OUTPUT(m_ManhacksDisable, "OnManhacksDisabled"),
DEFINE_OUTPUT(m_DoorsLock, "OnDoorsLocked"),
DEFINE_OUTPUT(m_DoorsUnlock, "OnDoorsUnlocked"),

DEFINE_OUTPUT(m_OnChangedTeam, "OnChangedTeam"),

DEFINE_LAZNETWORKENTITY_DATADESC(),
END_DATADESC();

Class_T CHLSSCombineInterface::Classify()
{
	return (Class_T)(CLASS_COMPUTER_NEUTRAL + GetTeamNumber());
}

void CHLSSCombineInterface::ChangeTeam(int iTeam)
{
	int iOldTeam = GetTeamNumber();

	BaseClass::ChangeTeam(iTeam);

	for (int i = 0; i < m_NetworkEnts.Count(); i++)
	{
		ILazNetworkEntity* pNet = dynamic_cast<ILazNetworkEntity*> (m_NetworkEnts.Element(i).Get());
		if (pNet && !pNet->HasFirewall())
		{
			m_NetworkEnts.Element(i)->ChangeTeam(iTeam);
		}
	}

	if (iOldTeam != iTeam)
		m_OnChangedTeam.Set(iTeam, this, this);
}

void CHLSSCombineInterface::Precache()
{
	BaseClass::Precache();

	const char* szModel = STRING(GetModelName());
	if (!szModel || !*szModel)
	{
		SetModelName(AllocPooledString("models/props_combine/combine_interface001.mdl"));
	}

	PrecacheModel(STRING(GetModelName()));
	PrecacheScriptSound("combine.door_lock");
}

void CHLSSCombineInterface::Spawn()
{
	Precache();

	BaseClass::Spawn();

	m_bDoorIsGate = HasSpawnFlags(SF_INTERFACE_DOORS_ARE_GATES);

	SetModel(STRING(GetModelName()));

	SetSolid(SOLID_VPHYSICS);
	VPhysicsInitStatic();

	if (m_bIsDisabled)
		m_nSkin = 1;
}

int CHLSSCombineInterface::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_FULLCHECK);
}

int CHLSSCombineInterface::ShouldTransmit(const CCheckTransmitInfo* pInfo)
{
	CBaseEntity* pRecipientEntity = CBaseEntity::Instance(pInfo->m_pClientEnt);

	Assert(pRecipientEntity->IsPlayer());

	CBasePlayer* pRecipientPlayer = static_cast<CBasePlayer*>(pRecipientEntity);

	if (InSameTeam(pRecipientPlayer))
	{
		return FL_EDICT_ALWAYS;
	}

	// by default do a PVS check

	return FL_EDICT_PVSCHECK;
}

void CHLSSCombineInterface::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CLaz_Player* pPlayer = ToLazuulPlayer(pActivator);
	if (!m_bIsDisabled && pPlayer && InSameTeam(pPlayer) && !m_hInterfacePlayer.Get())
	{
		m_hInterfacePlayer.Set(pPlayer);
	}
	else if (pPlayer)
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, entindex(), "combine.door_lock");
	}
}

bool CHLSSCombineInterface::HandleEntityCommand(CBasePlayer* pClient, KeyValues* pKeyValues)
{
	if (pClient == m_hInterfacePlayer.Get())
	{
		if (FStrEq(pKeyValues->GetName(), "SlotChange"))
		{
			int iSlot = pKeyValues->GetInt("slot");
			bool bEnabled = pKeyValues->GetBool("enabled");

			if (iSlot < 0 || iSlot >= NUM_CONTROL_SLOTS)
			{
				return false;
			}

			if (m_bSlotActive.Get(iSlot) != bEnabled)
			{
				if (m_bSlotIsAvailable[iSlot])
					m_bSlotActive.Set(iSlot, bEnabled);

				UpdateSlot(iSlot);
			}
			return true;
		}
		else if (FStrEq(pKeyValues->GetName(), "Close"))
		{
			m_hInterfacePlayer.Set(nullptr);
			return true;
		}
	}

	return false;
}

bool CHLSSCombineInterface::EmpCanInteract(CBaseCombatWeapon* pWeapon)
{
	int iInvaderTeam = LazuulRules()->GetTeamInRole(TEAM_ROLE_ATTACKERS);
	if (iInvaderTeam == pWeapon->GetTeamNumber() && GetTeamNumber() != iInvaderTeam)
	{
		return true;
	}

	return false;
}

void CHLSSCombineInterface::EmpNotifyInteraction(CBaseCombatWeapon* pWeapon)
{
	ChangeTeam(pWeapon->GetTeamNumber());
}

void CHLSSCombineInterface::AddEntityToNetwork(ILazNetworkEntity* pEnt)
{
	EHANDLE hEnt = pEnt->GetEntityPtr();
	if (hEnt.Get() && m_NetworkEnts.Find(hEnt) == -1)
	{
		m_NetworkEnts.AddToTail(hEnt);
	}
}
void CHLSSCombineInterface::RemoveEntityFromNetwork(ILazNetworkEntity* pEnt)
{
	EHANDLE hEnt = pEnt->GetEntityPtr();
	m_NetworkEnts.FindAndRemove(hEnt);
}

void CHLSSCombineInterface::SetPowerEnabled(bool bPower, bool bNetwork, bool bForce)
{
	bool bIsOn = !m_bIsDisabled;
	if (bIsOn == bPower)
		return;

	if (bNetwork && !bForce && m_bHasOwnGenerator)
		return;

	if (m_bForcedOff && !bForce)
		return;

	if (bPower)
	{
		for (int i = 0; i < m_NetworkEnts.Count(); i++)
		{
			ILazNetworkEntity* pNet = dynamic_cast<ILazNetworkEntity*> (m_NetworkEnts.Element(i).Get());
			if (!pNet)
				continue;

			int iRole = pNet->GetNetworkRole();
			if ((iRole != NETROLE_SHIELDS && iRole != NETROLE_TURRETS) || m_bSlotActive[iRole-1])
				pNet->NetworkPowerOn(bForce);
		}

		if (m_bSlotActive[SLOT_MANHACKS])
		{
			m_ManhacksEnable.FireOutput(this, this);
		}

		if (m_bSlotActive[SLOT_DOORS])
		{
			m_DoorsLock.FireOutput(this, this);
		}
		else
		{
			m_DoorsUnlock.FireOutput(this, this);
		}

		m_bForcedOff = false;
		m_nSkin = 0;
	}
	else
	{
		for (int i = 0; i < m_NetworkEnts.Count(); i++)
		{
			ILazNetworkEntity* pNet = dynamic_cast<ILazNetworkEntity*> (m_NetworkEnts.Element(i).Get());
			if (!pNet)
				continue;

			pNet->NetworkPowerOff(bForce);
		}

		if (m_bSlotActive[SLOT_MANHACKS])
		{
			m_ManhacksDisable.FireOutput(this, this);
		}

		m_bForcedOff = bForce;
		m_nSkin = 1;
		m_hInterfacePlayer = nullptr;
	}

	m_bIsDisabled = !bPower;
}

void CHLSSCombineInterface::InputToggle(inputdata_t& inputdata)
{
	if (m_bHasOwnGenerator)
		SetPowerEnabled(m_bIsDisabled);
}

void CHLSSCombineInterface::InputEnable(inputdata_t& inputdata)
{
	if (m_bHasOwnGenerator)
		SetPowerEnabled(true);
}

void CHLSSCombineInterface::InputDisable(inputdata_t& inputdata)
{
	if (m_bHasOwnGenerator)
		SetPowerEnabled(false);
}

void CHLSSCombineInterface::UpdateSlot(int iSlot)
{
	if (!m_bIsDisabled)
	{
		if (iSlot < SLOT_MANHACKS)
		{
			for (int i = 0; i < m_NetworkEnts.Count(); i++)
			{
				ILazNetworkEntity* pNet = dynamic_cast<ILazNetworkEntity*> (m_NetworkEnts.Element(i).Get());
				if (!pNet)
					continue;

				int iRole = pNet->GetNetworkRole();
				if (iSlot == iRole - 1)
				{
					if (m_bSlotActive[iSlot])
						pNet->NetworkPowerOn(false);
					else
						pNet->NetworkPowerOff(false);
				}
			}
		}
		else if (iSlot == SLOT_MANHACKS)
		{
			if (m_bSlotActive[iSlot])
				m_ManhacksEnable.FireOutput(this, this);
			else
				m_ManhacksDisable.FireOutput(this, this);
		}
		else if (iSlot == SLOT_DOORS)
		{
			if (m_bSlotActive[iSlot])
				m_DoorsLock.FireOutput(this, this);
			else
				m_DoorsUnlock.FireOutput(this, this);
		}
	}
}

class CHLSSCombineMainframe : public CHLSSCombineInterface, public IGameModeControllerEntity
{
public:
	DECLARE_CLASS(CHLSSCombineMainframe, CHLSSCombineInterface);
	DECLARE_DATADESC();

	virtual bool ShouldActivateMode(int iGameMode);
	virtual bool CanActivateVariant(int iGameMode, int iVariant);

	void Spawn();
	virtual bool EmpCanInteract(CBaseCombatWeapon* pWeapon) { return false; }

	// Entity killed (only fired once)
	virtual void	Event_Killed(const CTakeDamageInfo& info);

protected:
	COutputEvent	m_OnDestroyed;
};

BEGIN_DATADESC(CHLSSCombineMainframe)
DEFINE_OUTPUT(m_OnDestroyed, "OnDestroyed"),
END_DATADESC();

LINK_ENTITY_TO_CLASS(hlss_main_frame, CHLSSCombineMainframe);

bool CHLSSCombineMainframe::ShouldActivateMode(int iGameMode)
{
	return iGameMode == LAZ_GM_BASE_DEFENSE;
}

bool CHLSSCombineMainframe::CanActivateVariant(int iGameMode, int iVariant)
{
	if (iGameMode == LAZ_GM_BASE_DEFENSE)
	{
		return true;
	}

	return false;
}

void CHLSSCombineMainframe::Spawn()
{
	BaseClass::Spawn();

	SetMaxHealth(hlss_main_frame_health.GetInt());
	SetHealth(hlss_main_frame_health.GetInt());

	m_takedamage = DAMAGE_YES;
}

void CHLSSCombineMainframe::Event_Killed(const CTakeDamageInfo& info)
{
	if (info.GetAttacker())
	{
		info.GetAttacker()->Event_KilledOther(this, info);
	}

	m_takedamage = DAMAGE_NO;
	m_lifeState = LIFE_DEAD;
	m_OnDestroyed.FireOutput(this, this);
	SetPowerEnabled(false, false, true);

	if (LazuulRules()->GetGameMode() == LAZ_GM_BASE_DEFENSE)
	{
		LazuulRules()->SetWinningTeam(LazuulRules()->GetTeamInRole(TEAM_ROLE_ATTACKERS), WINREASON_ALL_POINTS_CAPTURED);
	}
}
