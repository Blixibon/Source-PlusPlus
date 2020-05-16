#include "cbase.h"
#include "func_combine_wall.h"
#include "team.h"
#include "saverestore_utlvector.h"
#include "props.h"

#define SHIELD_LIST_UPDATE_INTERVAL 0.5f

CEntityClassList<CCombineShieldWall> g_ShieldWallList;
template <> CCombineShieldWall* CEntityClassList<CCombineShieldWall>::m_pClassList = NULL;

class CPropCombineWallProjector : public CDynamicProp
{
public:
	DECLARE_CLASS(CPropCombineWallProjector, CDynamicProp);
	DECLARE_DATADESC();

	CPropCombineWallProjector()
	{
		SetSolid(SOLID_VPHYSICS);
	}

	void	PowerOn();
	void	PowerOff();

	virtual void			ChangeTeam(int iTeamNum);
protected:
	string_t	m_strAnimActivate;
	string_t	m_strAnimDeactivate;
	string_t	m_strIdleActive;
	string_t	m_strIdleInactive;

	int			m_iSkinInactive;
	int			m_iTeamSkins[TF_TEAM_COUNT];

	bool		m_bDisabled;
};

LINK_ENTITY_TO_CLASS(prop_shield_wall_projector, CPropCombineWallProjector);

BEGIN_DATADESC(CPropCombineWallProjector)
DEFINE_KEYFIELD(m_strAnimActivate, FIELD_STRING, "anim_activate"),
DEFINE_KEYFIELD(m_strAnimDeactivate, FIELD_STRING, "anim_deactivate"),
DEFINE_KEYFIELD(m_strIdleActive, FIELD_STRING, "idle_active"),
DEFINE_KEYFIELD(m_strIdleInactive, FIELD_STRING, "idle_inactive"),

DEFINE_KEYFIELD(m_iSkinInactive, FIELD_INTEGER, "skin_inactive"),
DEFINE_KEYFIELD(m_iTeamSkins[2], FIELD_INTEGER, "skin_blueteam"),
DEFINE_KEYFIELD(m_iTeamSkins[3], FIELD_INTEGER, "skin_redteam"),
DEFINE_KEYFIELD(m_iTeamSkins[4], FIELD_INTEGER, "skin_greenteam"),
DEFINE_KEYFIELD(m_iTeamSkins[5], FIELD_INTEGER, "skin_yellowteam"),

DEFINE_FIELD(m_bDisabled, FIELD_BOOLEAN),
END_DATADESC();

void CPropCombineWallProjector::PowerOn()
{
	if (!m_bDisabled)
		return;

	m_bDisabled = false;
	m_nSkin = m_iTeamSkins[GetTeamNumber()];

	if (m_strAnimActivate != NULL_STRING)
	{
		PropSetAnim(STRING(m_strAnimActivate));
	}

	if (m_strIdleActive != NULL_STRING)
	{
		m_iszDefaultAnim = m_strIdleActive;
	}
}

void CPropCombineWallProjector::PowerOff()
{
	if (m_bDisabled)
		return;

	m_bDisabled = true;
	m_nSkin = m_iSkinInactive;

	if (m_strAnimDeactivate != NULL_STRING)
	{
		PropSetAnim(STRING(m_strAnimDeactivate));
	}

	if (m_strIdleInactive != NULL_STRING)
	{
		m_iszDefaultAnim = m_strIdleInactive;
	}
}

void CPropCombineWallProjector::ChangeTeam(int iTeamNum)
{
	int iOldTeam = GetTeamNumber();

	BaseClass::ChangeTeam(iTeamNum);

	if (iOldTeam != iTeamNum && !m_bDisabled)
	{
		m_nSkin = m_iTeamSkins[iTeamNum];
	}
}

BEGIN_DATADESC(CCombineShieldWall)
DEFINE_FIELD(m_bShieldActive, FIELD_BOOLEAN),
DEFINE_FIELD(m_vecLocalOuterBoundsMins, FIELD_VECTOR),
DEFINE_FIELD(m_vecLocalOuterBoundsMaxs, FIELD_VECTOR),
DEFINE_UTLVECTOR(m_hShieldProjectors, FIELD_EHANDLE),
DEFINE_KEYFIELD(m_strShieldProjectors, FIELD_STRING, "projector_props"),

DEFINE_THINKFUNC(ActiveThink),

DEFINE_LAZNETWORKENTITY_DATADESC(),
END_DATADESC();

IMPLEMENT_SERVERCLASS_ST(CCombineShieldWall, DT_CombineShieldWall)
SendPropBool(SENDINFO(m_bShieldActive)),
SendPropArray(SendPropEHandle(SENDINFO_ARRAY(m_hNearbyEntities)), m_hNearbyEntities),
END_SEND_TABLE();

LINK_ENTITY_TO_CLASS(func_combine_wall, CCombineShieldWall);

class CWallEnumerator : public IPartitionEnumerator
{
public:
	CWallEnumerator(CBaseEntity* pEnt)
	{
		m_pOuter = pEnt;
	}

	virtual IterationRetval_t EnumElement(IHandleEntity* pHandleEntity);
	void	Sort();

	CUtlVector<EHANDLE> m_listNearbyEntities;
	CBaseEntity* m_pOuter;

private:
	static CCollisionProperty* gm_pSortCollideable;
	static int __cdecl NearestEntitySort(const EHANDLE* pLeft, const EHANDLE* pRight);
};

IterationRetval_t CWallEnumerator::EnumElement(IHandleEntity* pHandleEntity)
{
	if (pHandleEntity)
	{
		CBaseEntity* pEntity = gEntList.GetBaseEntity(pHandleEntity->GetRefEHandle());
		if (pEntity)
		{
			if (pEntity == m_pOuter)
				return ITERATION_CONTINUE;

			if (m_pOuter->InSameTeam(pEntity) || pEntity->GetTeamNumber() == TEAM_SPECTATOR)
				return ITERATION_CONTINUE;

			if (!pEntity->IsPlayer() && !pEntity->IsNPC())
				return ITERATION_CONTINUE;

			m_listNearbyEntities.AddToTail(pEntity);
		}
	}

	return ITERATION_CONTINUE;
}

void CWallEnumerator::Sort()
{
	gm_pSortCollideable = m_pOuter->CollisionProp();
	m_listNearbyEntities.Sort(CWallEnumerator::NearestEntitySort);
	gm_pSortCollideable = nullptr;
}

CCollisionProperty* CWallEnumerator::gm_pSortCollideable = nullptr;

int __cdecl CWallEnumerator::NearestEntitySort(const EHANDLE* pLeft, const EHANDLE* pRight)
{
	if (pLeft->Get() && !pRight->Get())
		return -1;

	if (!pLeft->Get() && pRight->Get())
		return 1;

	if (!pLeft->Get() && !pRight->Get())
		return 0;

	float flDist2Left = gm_pSortCollideable->CalcDistanceFromPoint(pLeft->Get()->WorldSpaceCenter());
	float flDist2Right = gm_pSortCollideable->CalcDistanceFromPoint(pRight->Get()->WorldSpaceCenter());

	if (flDist2Left < flDist2Right)
		return -1;

	if (flDist2Right < flDist2Left)
		return 1;

	if (pLeft->Get()->IsPlayer() && !pRight->Get()->IsPlayer())
		return -1;

	if (!pLeft->Get()->IsPlayer() && pRight->Get()->IsPlayer())
		return 1;

	return 0;
}

CCombineShieldWall::CCombineShieldWall()
{
	g_ShieldWallList.Insert(this);
}

CCombineShieldWall::~CCombineShieldWall()
{
	g_ShieldWallList.Remove(this);
}

void CCombineShieldWall::Precache()
{
	PrecacheScriptSound("outland_10.shieldwall_off");
	PrecacheScriptSound("outland_10.shieldwall_on");
	PrecacheScriptSound("Streetwar.d3_c17_07_combine_shield_loop3");
	PrecacheScriptSound("Streetwar.d3_c17_07_combine_shield_touch_loop1");
	PrecacheScriptSound("NPC_RollerMine.Reprogram");
	PrecacheScriptSound("AlyxEMP.Discharge");
}

void CCombineShieldWall::Spawn()
{
	m_iSolidity = BRUSHSOLID_TOGGLE;

	Precache();
	BaseClass::Spawn();

	m_takedamage = DAMAGE_EVENTS_ONLY;

	AddSolidFlags(FSOLID_TRIGGER);
	CollisionProp()->UseTriggerBounds(true, 1.f);

	const Vector& delta = CollisionProp()->OBBSize();
	int iMin = 0;
	for (int i = 1; i < 3; i++)
	{
		// Get the maximum value.
		if (delta[i] < delta[iMin])
		{
			iMin = i;
		}
	}

	Vector vAdd(4.f);
	vAdd[iMin] = 64.f;

	m_vecLocalOuterBoundsMaxs = CollisionProp()->OBBMaxs() + vAdd;
	m_vecLocalOuterBoundsMins = CollisionProp()->OBBMins() - vAdd;

	if (!m_iDisabled)
	{
		m_bShieldActive = true;

		SetThink(&CCombineShieldWall::ActiveThink);
		SetNextThink(gpGlobals->curtime + 0.1f);
	}
	else
	{
		AddSolidFlags(FSOLID_NOT_SOLID);
	}
}

void CCombineShieldWall::Activate()
{
	BaseClass::Activate();

	if (m_strShieldProjectors != NULL_STRING && !m_hShieldProjectors.Count())
	{
		CBaseEntity* pEnt = gEntList.FindEntityByName(nullptr, m_strShieldProjectors, this);
		for (; pEnt != nullptr; pEnt = gEntList.FindEntityByName(pEnt, m_strShieldProjectors, this))
		{
			CPropCombineWallProjector* pProp = dynamic_cast<CPropCombineWallProjector*> (pEnt);
			if (!pProp)
				continue;

			pProp->ChangeTeam(GetTeamNumber());
			if (IsOn())
			{
				pProp->PowerOn();
			}
			else
			{
				pProp->PowerOff();
			}

			m_hShieldProjectors.AddToTail(pProp);
		}
	}
}

bool CCombineShieldWall::ShouldCollide(int collisionGroup, int contentsMask) const
{
	if (collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT ||
		collisionGroup == COLLISION_GROUP_NPC ||
		collisionGroup == COLLISION_GROUP_PLAYER)
	{
		switch (GetTeamNumber())
		{
		case TF_TEAM_RED:
			if (!(contentsMask & CONTENTS_REBELTEAM))
				return false;
			break;

		case TF_TEAM_BLUE:
			if (!(contentsMask & CONTENTS_COMBINETEAM))
				return false;
			break;

		case TF_TEAM_GREEN:
			if (!(contentsMask & CONTENTS_GREENTEAM))
				return false;
			break;

		case TF_TEAM_YELLOW:
			if (!(contentsMask & CONTENTS_YELLOWTEAM))
				return false;
			break;
		}
	}

	return BaseClass::ShouldCollide(collisionGroup, contentsMask);
}

//-----------------------------------------------------------------------------
// Purpose: Hides the brush.
//-----------------------------------------------------------------------------
void CCombineShieldWall::TurnOff(void)
{
	if (!IsOn())
		return;

	//if (m_iSolidity != BRUSHSOLID_ALWAYS)
	{
		AddSolidFlags(FSOLID_NOT_SOLID);
	}

	m_bShieldActive = false;
	m_iDisabled = TRUE;
	EmitSound("outland_10.shieldwall_off");

	SetThink(NULL);

	for (int i = 0; i < m_hShieldProjectors.Count(); i++)
	{
		CPropCombineWallProjector* pProp = dynamic_cast<CPropCombineWallProjector*> (m_hShieldProjectors[i].Get());
		if (pProp)
		{
			pProp->PowerOff();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Shows the brush.
//-----------------------------------------------------------------------------
void CCombineShieldWall::TurnOn(void)
{
	if (IsOn())
		return;

	//if (m_iSolidity != BRUSHSOLID_NEVER)
	{
		RemoveSolidFlags(FSOLID_NOT_SOLID);
	}

	m_bShieldActive = true;
	m_iDisabled = FALSE;
	EmitSound("outland_10.shieldwall_on");

	SetThink(&CCombineShieldWall::ActiveThink);
	SetNextThink(gpGlobals->curtime);

	ClearSpace();

	for (int i = 0; i < m_hShieldProjectors.Count(); i++)
	{
		CPropCombineWallProjector* pProp = dynamic_cast<CPropCombineWallProjector*> (m_hShieldProjectors[i].Get());
		if (pProp)
		{
			pProp->PowerOn();
		}
	}
}


bool CCombineShieldWall::IsOn(void) const
{
	return m_bShieldActive.Get();
}

void CCombineShieldWall::ChangeTeam(int iTeamNum)
{
	int iOldTeam = GetTeamNumber();

	BaseClass::ChangeTeam(iTeamNum);

	for (int i = 0; i < m_hShieldProjectors.Count(); i++)
	{
		CBaseEntity* pEntity = m_hShieldProjectors[i].Get();
		if (pEntity)
			pEntity->ChangeTeam(iTeamNum);
	}

	if (IsOn() && iOldTeam != iTeamNum)
	{
		ClearSpace();
		SetNextThink(gpGlobals->curtime);

		CPASAttenuationFilter filter(this, "NPC_RollerMine.Reprogram");
		filter.RemoveRecipientsNotOnTeam(GetGlobalTeam(iTeamNum));

		EmitSound(filter, entindex(), "NPC_RollerMine.Reprogram");

		CPASAttenuationFilter filter2(this, "AlyxEMP.Discharge");
		//filter2.RemoveRecipientsByTeam(GetGlobalTeam(iTeamNum));

		EmitSound(filter2, entindex(), "AlyxEMP.Discharge");
	}
}

int CCombineShieldWall::OnTakeDamage(const CTakeDamageInfo& info)
{
	int iRet = BaseClass::OnTakeDamage(info);
	if (iRet)
	{
		EntityMessageBegin(this);
		WRITE_BYTE(2);
		MessageEnd();
	}
	
	return iRet;
}

void CCombineShieldWall::ActiveThink()
{
	Vector vecMins, vecMaxs;
	CollisionProp()->CollisionToWorldSpace(m_vecLocalOuterBoundsMins, &vecMins);
	CollisionProp()->CollisionToWorldSpace(m_vecLocalOuterBoundsMaxs, &vecMaxs);

	CWallEnumerator functor(this);
	partition->EnumerateElementsInBox(PARTITION_ENGINE_NON_STATIC_EDICTS|PARTITION_ENGINE_SOLID_EDICTS, vecMins, vecMaxs, true, &functor);
	functor.Sort();

	int i = 0;
	for (; i < 2 && i < functor.m_listNearbyEntities.Count(); i++)
	{
		m_hNearbyEntities.Set(i, functor.m_listNearbyEntities[i]);
	}
	for (; i < 2; i++)
	{
		m_hNearbyEntities.Set(i, NULL);
	}

	SetNextThink(gpGlobals->curtime + SHIELD_LIST_UPDATE_INTERVAL);
}

bool CCombineShieldWall::PointsCrossForceField(const Vector& vecStart, const Vector& vecEnd, int nTeamToIgnore)
{
	// Setup the ray.
	Ray_t ray;
	ray.Init(vecStart, vecEnd);

	for (CCombineShieldWall *pEntity = g_ShieldWallList.m_pClassList; pEntity != nullptr; pEntity = pEntity->m_pNext)
	{
		if (!pEntity->IsOn())
			continue;

		if (pEntity->GetTeamNumber() == nTeamToIgnore && nTeamToIgnore != TEAM_UNASSIGNED)
			continue;

		trace_t trace;
		enginetrace->ClipRayToEntity(ray, MASK_ALL, pEntity, &trace);
		if (trace.fraction < 1.0f)
		{
			return true;
		}
	}

	return false;
}

void CCombineShieldWall::ClearSpace()
{
	touchlink_t* root = (touchlink_t*)GetDataObject(TOUCHLINK);
	if (root)
	{
		for (touchlink_t* link = root->nextLink; link != root; link = link->nextLink)
		{
			CBaseEntity* pTouch = link->entityTouched;
			if (pTouch && !InSameTeam(pTouch) && (pTouch->IsPlayer() || pTouch->IsNPC()))
			{
				CTakeDamageInfo info(this, this, pTouch->GetMaxHealth() * 2.f, DMG_CRUSH | DMG_DISSOLVE | DMG_DIRECT | DMG_NEVERGIB);
				pTouch->TakeDamage(info);
			}
		}
	}
}
