#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "functionproxy.h"
#include "proxyentity.h"
#include "ispatialpartition.h"
#include "soundenvelope.h"
#include "utlvector.h"
#include "c_team.h"
#include "view.h"

#define SHIELD_POWERUP_TIME 0.35f
#define SHIELD_LIST_UPDATE_INTERVAL 0.5f
#define SOUNDCONTROLLER() CSoundEnvelopeController::GetController()

class C_CombineShieldWall : public C_BaseEntity, public IPartitionEnumerator
{
public:
	DECLARE_CLASS(C_CombineShieldWall, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement(IHandleEntity* pHandleEntity);

	virtual void		ClientThink();
	virtual void		OnDataChanged(DataUpdateType_t type);
	virtual void		OnPreDataChanged(DataUpdateType_t type);

	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;

	float				GetPowerFraction();
	virtual bool		ShouldDraw(void);
	Vector				GetShieldColor();
	int					GetDeniedEntities(C_BaseEntity* pEntities[2]);

private:
	bool m_bShieldActive;
	bool m_bOldShieldActive;

	Vector	m_vecLocalOuterBoundsMins;
	Vector	m_vecLocalOuterBoundsMaxs;

	float m_flStateChangedTime;

	CSoundPatch* m_pIdleSound;

	CUtlVector< EHANDLE > m_listNearbyEntities;
	float	m_flLastListUpdateTime;
};

IMPLEMENT_CLIENTCLASS_DT(C_CombineShieldWall, DT_CombineShieldWall, CCombineShieldWall)
RecvPropBool(RECVINFO(m_bShieldActive)),
END_RECV_TABLE();

IterationRetval_t C_CombineShieldWall::EnumElement(IHandleEntity* pHandleEntity)
{
	IClientEntity* pClientEntity = cl_entitylist->GetClientEntityFromHandle(pHandleEntity->GetRefEHandle());
	C_BaseEntity* pEntity = pClientEntity ? pClientEntity->GetBaseEntity() : NULL;
	if (pEntity)
	{
		if (pEntity == this)
			return ITERATION_CONTINUE;

		if (!pEntity->IsServerEntity())
			return ITERATION_CONTINUE;

		if (InSameTeam(pEntity))
			return ITERATION_CONTINUE;

		if (!pEntity->IsPlayer() && !pEntity->IsNPC())
			return ITERATION_CONTINUE;

		m_listNearbyEntities.AddToTail(pEntity);
	}

	return ITERATION_CONTINUE;
}

static CCollisionProperty* g_pSortCollideable = nullptr;

int __cdecl NearestEntitySort(const EHANDLE* pLeft, const EHANDLE* pRight)
{
	if (pLeft->Get() && !pRight->Get())
		return -1;

	if (!pLeft->Get() && pRight->Get())
		return 1;

	if (!pLeft->Get() && !pRight->Get())
		return 0;

	float flDist2Left = g_pSortCollideable->CalcDistanceFromPoint(pLeft->Get()->WorldSpaceCenter());
	float flDist2Right = g_pSortCollideable->CalcDistanceFromPoint(pRight->Get()->WorldSpaceCenter());

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

void C_CombineShieldWall::ClientThink()
{
	if (m_bShieldActive && WorldSpaceCenter().DistToSqr(MainViewOrigin()) <= Sqr(1024.f) && gpGlobals->curtime >= m_flLastListUpdateTime + SHIELD_LIST_UPDATE_INTERVAL)
	{
		m_flLastListUpdateTime = gpGlobals->curtime;
		m_listNearbyEntities.Purge();

		Vector vecMins, vecMaxs;
		CollisionProp()->CollisionToWorldSpace(m_vecLocalOuterBoundsMins, &vecMins);
		CollisionProp()->CollisionToWorldSpace(m_vecLocalOuterBoundsMaxs, &vecMaxs);

		partition->EnumerateElementsInBox(PARTITION_CLIENT_SOLID_EDICTS, vecMins, vecMaxs, false, this);
		g_pSortCollideable = CollisionProp();
		m_listNearbyEntities.Sort(NearestEntitySort);
		g_pSortCollideable = nullptr;
	}
}

void C_CombineShieldWall::OnDataChanged(DataUpdateType_t type)
{
	if (type == DATA_UPDATE_CREATED)
	{
		const Vector &delta = CollisionProp()->OBBSize();
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

		m_bOldShieldActive = m_bShieldActive;
		m_flStateChangedTime = gpGlobals->curtime - SHIELD_POWERUP_TIME;
		m_flLastListUpdateTime = gpGlobals->curtime;
	}
	else
	{
		if (m_bOldShieldActive != m_bShieldActive)
		{
			m_flStateChangedTime = gpGlobals->curtime;
			if (!m_bShieldActive)
			{
				m_listNearbyEntities.Purge();
			}
		}
	}

	if (m_bShieldActive)
	{
		if (!m_pIdleSound)
		{
			CBroadcastRecipientFilter filter;
			m_pIdleSound = SOUNDCONTROLLER().SoundCreate(filter, entindex(), "Streetwar.d3_c17_07_combine_shield_loop3");
			SOUNDCONTROLLER().Play(m_pIdleSound, 1.f, 100.f);
		}
		else if (!m_bOldShieldActive)
		{
			SOUNDCONTROLLER().SoundChangePitch(m_pIdleSound, 100.f, SHIELD_POWERUP_TIME);
			SOUNDCONTROLLER().SoundChangeVolume(m_pIdleSound, 1.f, SHIELD_POWERUP_TIME);
		}
	}
	else if (m_pIdleSound && m_bOldShieldActive)
	{
		SOUNDCONTROLLER().SoundChangePitch(m_pIdleSound, 50.f, SHIELD_POWERUP_TIME);
		SOUNDCONTROLLER().SoundChangeVolume(m_pIdleSound, 0.f, SHIELD_POWERUP_TIME);
	}

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_CombineShieldWall::OnPreDataChanged(DataUpdateType_t type)
{
	if (type == DATA_UPDATE_DATATABLE_CHANGED)
	{
		m_bOldShieldActive = m_bShieldActive;
	}
}

bool C_CombineShieldWall::ShouldCollide(int collisionGroup, int contentsMask) const
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

float C_CombineShieldWall::GetPowerFraction()
{
	if (m_bShieldActive)
	{
		return RemapValClamped(gpGlobals->curtime, m_flStateChangedTime, m_flStateChangedTime + SHIELD_POWERUP_TIME, 0.f, 1.f);
	}
	else
	{
		return RemapValClamped(gpGlobals->curtime, m_flStateChangedTime, m_flStateChangedTime + SHIELD_POWERUP_TIME, 1.f, 0.f);
	}
}

bool C_CombineShieldWall::ShouldDraw(void)
{
	if (GetPowerFraction() <= 0.f)
	{
		return false;
	}

	return BaseClass::ShouldDraw();
}

Vector C_CombineShieldWall::GetShieldColor()
{
	switch (GetTeamNumber())
	{
	case TEAM_COMBINE:
		return Vector(.025f, .78f, .75f);
		break;
	case TEAM_REBELS:
		return Vector(.973f, .063f, .063f);
		break;
	default:
		return Vector(0.5f);
		break;
	}
}

int C_CombineShieldWall::GetDeniedEntities(C_BaseEntity* pEntities[2])
{
	int iCount = 0;
	for (int i = 0; iCount < 2 && i < m_listNearbyEntities.Count(); i++)
	{
		if (m_listNearbyEntities[i].Get() != nullptr)
		{
			pEntities[iCount] = m_listNearbyEntities[i].Get();
			iCount++;
		}
	}

	return iCount;
}

class CShieldBasicColorProxy : public CResultProxy
{
public:
	virtual void OnBind(void*pArg)
	{
		C_BaseEntity* pEnt = BindArgToEntity(pArg);
		if (pEnt)
		{
			C_CombineShieldWall* pShield = dynamic_cast<C_CombineShieldWall*> (pEnt);
			if (pShield)
			{
				Vector vColor = pShield->GetShieldColor() * pShield->GetPowerFraction();
				m_pResult->SetVecValue(vColor.Base(), 3);
				return;
			}
		}

		SetFloatResult(0.5f);
	}
};

EXPOSE_MATERIAL_PROXY(CShieldBasicColorProxy, CombineShieldBasic);

class CShieldVortexProxy : public CEntityMaterialProxy
{
public:
	virtual bool Init(IMaterial* pMaterial, KeyValues* pKeyValues);
	virtual IMaterial* GetMaterial() { return m_pPowerupVar->GetOwningMaterial(); }
	virtual void OnBind(C_BaseEntity* pBaseEntity);

private:
	IMaterialVar* m_pPowerupVar;
	IMaterialVar* m_pShieldColorVar;
	IMaterialVar* m_pVortexColorVar;
	IMaterialVar* m_pVortexActiveVars[2];
	IMaterialVar* m_pVortexPositionVars[2];
};

EXPOSE_MATERIAL_PROXY(CShieldVortexProxy, CombineShieldVortex);

bool CShieldVortexProxy::Init(IMaterial* pMaterial, KeyValues* pKeyValues)
{
	bool bFound = false;

	m_pPowerupVar = pMaterial->FindVar("$POWERUP", &bFound);
	if (!bFound)
		return false;

	m_pShieldColorVar = pMaterial->FindVar("$FLOW_COLOR", &bFound);
	if (!bFound)
		return false;

	m_pVortexColorVar = pMaterial->FindVar("$FLOW_VORTEX_COLOR", &bFound);
	if (!bFound)
		return false;

	m_pVortexActiveVars[0] = pMaterial->FindVar("$FLOW_VORTEX1", &bFound);
	if (!bFound)
		return false;

	m_pVortexActiveVars[1] = pMaterial->FindVar("$FLOW_VORTEX2", &bFound);
	if (!bFound)
		return false;

	m_pVortexPositionVars[0] = pMaterial->FindVar("$FLOW_VORTEX_POS1", &bFound);
	if (!bFound)
		return false;

	m_pVortexPositionVars[1] = pMaterial->FindVar("$FLOW_VORTEX_POS2", &bFound);
	if (!bFound)
		return false;

	return true;
}

void CShieldVortexProxy::OnBind(C_BaseEntity* pBaseEntity)
{
	C_CombineShieldWall* pShield = dynamic_cast<C_CombineShieldWall*> (pBaseEntity);
	if (pShield)
	{
		m_pPowerupVar->SetFloatValue(pShield->GetPowerFraction());
		Vector vecColor = pShield->GetShieldColor();
		m_pShieldColorVar->SetVecValue(vecColor.Base(), 3);
		vecColor += 0.1f;
		m_pVortexColorVar->SetVecValue(vecColor.Base(), 3);

		C_BaseEntity* pEnts[2];
		int iNum = pShield->GetDeniedEntities(pEnts);
		int i = 0;
		for (; i < iNum; i++)
		{
			m_pVortexActiveVars[i]->SetIntValue(1);
			m_pVortexPositionVars[i]->SetVecValue(pEnts[i]->WorldSpaceCenter().Base(), 3);
		}

		for (; i < 2; i++)
		{
			m_pVortexActiveVars[i]->SetIntValue(0);
		}
	}
}
