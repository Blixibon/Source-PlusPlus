#include "cbase.h"
#include "IEffects.h"
#include "animation.h"
#ifdef GAME_DLL
#include "peter/ienergyshield.h"
#endif

#define SHIELD_MODEL "models/combine_armshield.mdl"

#ifdef CLIENT_DLL
#define CArmShield C_ArmShield
class C_ArmShield : public C_BaseCombatCharacter
#else
class CArmShield : public CBaseCombatCharacter, public IEnergyShield
#endif
{
public:
	DECLARE_CLASS(CArmShield, CBaseCombatCharacter);
	DECLARE_NETWORKCLASS();
#ifdef GAME_DLL
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void Precache();

	virtual bool IsShieldActive()
	{
		return m_bActivated.Get();
	}

	virtual void SetShieldActive(bool bActive)
	{
		if (bActive != m_bActivated)
		{
			m_bActivated.Set(bActive);

			if (bActive)
			{
				SetHitboxSet(m_nActiveHitboxSet);
			}
			else
			{
				SetHitboxSet(0);
			}

			CollisionProp()->MarkSurroundingBoundsDirty();
		}
	}

	void	TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);

	virtual void ProcessSceneEvents(void);
#else
	virtual void		ProcessSceneEvents(bool bFlexEvents);
#endif

	virtual CStudioHdr *OnNewModel(void);

protected:
	CNetworkVar(bool, m_bActivated);

	LocalFlexController_t m_flxActive;

#ifdef GAME_DLL
	int m_nActiveHitboxSet;
#endif
};

IMPLEMENT_NETWORKCLASS_ALIASED(ArmShield, DT_ArmShield)

BEGIN_NETWORK_TABLE(CArmShield, DT_ArmShield)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bActivated)),
#else
SendPropBool(SENDINFO(m_bActivated)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(prop_arm_shield, CArmShield);

#ifdef GAME_DLL

BEGIN_DATADESC(CArmShield)
DEFINE_FIELD(m_bActivated, FIELD_BOOLEAN),
END_DATADESC()

void CArmShield::Precache()
{
	BaseClass::Precache();

	PrecacheModel(SHIELD_MODEL);
}

void CArmShield::Spawn()
{
	BaseClass::Spawn();

	SetModel(SHIELD_MODEL);

	VPhysicsInitNormal(SOLID_VPHYSICS, FSOLID_CUSTOMBOXTEST, false);

	CollisionProp()->SetSurroundingBoundsType(USE_HITBOXES);

	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetBloodColor(DONT_BLEED);
}

void CArmShield::TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator)
{
	if (ptr->hitgroup == HITGROUP_GEAR)
	{
		g_pEffects->Sparks(ptr->endpos, RoundFloatToInt(RemapValClamped(info.GetDamage(), 0.0f, 100.0f, 1.0f, 4.0f)), 1, &ptr->plane.normal);
	}

	BaseClass::TraceAttack(info, vecDir, ptr, pAccumulator);
}
#endif // GAME_DLL

CStudioHdr *CArmShield::OnNewModel()
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	if (hdr)
	{
#ifdef GAME_DLL
		m_nActiveHitboxSet = FindHitboxSetByName(hdr, "active");
#endif

		m_flxActive = FindFlexController("activated");
	}

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: Default implementation
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
#define BASE_ARGS bFlexEvents
#define FLEX_EVENTS bFlexEvents
void C_ArmShield::ProcessSceneEvents(bool bFlexEvents)
#else
#define BASE_ARGS
#define FLEX_EVENTS true
void CArmShield::ProcessSceneEvents(void)
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
		SetFlexWeight(m_flxActive, GetFlexWeight(m_flxActive) * 0.95f + m_bActivated.Get() * 0.1f);
	}
}


#ifdef GAME_DLL
extern CBaseEntity *FindPickerEntity(CBasePlayer *pPlayer);

CON_COMMAND_F(test_armshield, "Tests an armshield.", FCVAR_CHEAT)
{
	CBaseEntity *pEnt = FindPickerEntity(UTIL_GetCommandClient());

	IEnergyShield *pShield = dynamic_cast<IEnergyShield *> (pEnt);
	if (pShield)
	{
		bool bIsOn = pShield->IsShieldActive();
		pShield->SetShieldActive(!bIsOn);
	}
}
#endif // GAME_DLL
