#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "functionproxy.h"
#include "proxyentity.h"
#include "ispatialpartition.h"
#include "soundenvelope.h"
#include "utlvector.h"
#include "c_team.h"
#include "view.h"

#define SHIELD_OVERLOAD_SUSTAIN_TIME 0.1f
#define SHIELD_OVERLOAD_FADE_TIME 0.3f

#define SHIELD_POWERUP_TIME 0.35f
#define SHIELD_POWERDOWN_TIME 0.45f
#define SOUNDCONTROLLER() CSoundEnvelopeController::GetController()

class C_CombineShieldWall : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_CombineShieldWall, C_BaseEntity);
	DECLARE_CLIENTCLASS();


	virtual void		ClientThink();
	virtual void		OnDataChanged(DataUpdateType_t type);
	virtual void		OnPreDataChanged(DataUpdateType_t type);

	virtual	bool		ShouldCollide(int collisionGroup, int contentsMask) const;

	float				GetPowerFraction();
	virtual bool		ShouldDraw(void);
	Vector				GetShieldColor();
	int					GetDeniedEntities(C_BaseEntity* pEntities[2]);
	float				GetOverloadPct();

	// Server to client message received
	virtual void					ReceiveMessage(int classID, bf_read& msg);

private:
	bool m_bShieldActive;
	bool m_bOldShieldActive;

	float m_flStateChangedTime;
	float m_flTimeLastDamaged;

	CSoundPatch* m_pIdleSound;
	CSoundPatch* m_pTouchSound;

	EHANDLE m_hNearbyEntities[2];
};

IMPLEMENT_CLIENTCLASS_DT(C_CombineShieldWall, DT_CombineShieldWall, CCombineShieldWall)
RecvPropBool(RECVINFO(m_bShieldActive)),
RecvPropArray(RecvPropEHandle(RECVINFO(m_hNearbyEntities[0])), m_hNearbyEntities),
END_RECV_TABLE();

void C_CombineShieldWall::ClientThink()
{
	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (pLocalPlayer)
	{
		bool bStopSound = InSameTeam(pLocalPlayer) || !pLocalPlayer->IsAlive() || !m_bShieldActive;
		if (!bStopSound)
		{
			Vector vecShieldNearest;
			CollisionProp()->CalcNearestPoint(pLocalPlayer->WorldSpaceCenter(), &vecShieldNearest);
			float flDist = pLocalPlayer->CollisionProp()->CalcDistanceFromPoint(vecShieldNearest);
			if (flDist > 2.f)
			{
				bStopSound = true;
			}
			else if (!m_pTouchSound)
			{
				CBroadcastRecipientFilter filter;
				m_pTouchSound = SOUNDCONTROLLER().SoundCreate(filter, entindex(), "Streetwar.d3_c17_07_combine_shield_touch_loop1");
				SOUNDCONTROLLER().Play(m_pTouchSound, .01f, 75.f);

				SOUNDCONTROLLER().SoundChangePitch(m_pTouchSound, 100.f, 0.2f);
				SOUNDCONTROLLER().SoundChangeVolume(m_pTouchSound, 1.f, 0.2f);
			}
		}

		if (bStopSound && m_pTouchSound)
		{
			SOUNDCONTROLLER().SoundChangePitch(m_pTouchSound, 75.f, 0.5f);
			SOUNDCONTROLLER().SoundFadeOut(m_pTouchSound, 0.5f, true);
			m_pTouchSound = NULL;
		}
	}

	UpdateVisibility();
}

void C_CombineShieldWall::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (type == DATA_UPDATE_CREATED)
	{
		m_bOldShieldActive = m_bShieldActive;
		m_flStateChangedTime = gpGlobals->curtime - SHIELD_POWERUP_TIME;
	}
	else
	{
		if (m_bOldShieldActive != m_bShieldActive)
		{
			m_flStateChangedTime = gpGlobals->curtime;
		}
	}

	if (m_bShieldActive)
	{
		if (!m_pIdleSound)
		{
			CBroadcastRecipientFilter filter;
			m_pIdleSound = SOUNDCONTROLLER().SoundCreate(filter, entindex(), "Streetwar.d3_c17_07_combine_shield_loop3");
			SOUNDCONTROLLER().Play(m_pIdleSound, .01f, 50.f);
		}

		if (!m_bOldShieldActive)
		{
			SOUNDCONTROLLER().SoundChangePitch(m_pIdleSound, 100.f, SHIELD_POWERUP_TIME);
			SOUNDCONTROLLER().SoundChangeVolume(m_pIdleSound, 1.f, SHIELD_POWERUP_TIME);
		}
	}
	else if (m_pIdleSound && m_bOldShieldActive)
	{
		SOUNDCONTROLLER().SoundChangePitch(m_pIdleSound, 50.f, SHIELD_POWERUP_TIME);
		SOUNDCONTROLLER().SoundFadeOut(m_pIdleSound, SHIELD_POWERUP_TIME, true);
		m_pIdleSound = NULL;
	}

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_CombineShieldWall::OnPreDataChanged(DataUpdateType_t type)
{
	BaseClass::OnPreDataChanged(type);

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
		return RemapValClamped(gpGlobals->curtime, m_flStateChangedTime, m_flStateChangedTime + SHIELD_POWERDOWN_TIME, 1.f, 0.f);
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
	case TF_TEAM_GREEN:
		return Vector(.063f, .973f, .063f);
		break;
	case TF_TEAM_YELLOW:
		return Vector(1.f, .749f, .161f);
		break;
	default:
		return Vector(0.5f);
		break;
	}
}

int C_CombineShieldWall::GetDeniedEntities(C_BaseEntity* pEntities[2])
{
	int iCount = 0;
	for (int i = 0; iCount < 2 && i < ARRAYSIZE(m_hNearbyEntities); i++)
	{
		if (m_hNearbyEntities[i].Get() != nullptr)
		{
			pEntities[iCount] = m_hNearbyEntities[i].Get();
			iCount++;
		}
	}

	return iCount;
}

float C_CombineShieldWall::GetOverloadPct()
{
	if (m_flTimeLastDamaged == 0.f)
		return 0.f;

	return RemapValClamped(gpGlobals->curtime, m_flTimeLastDamaged + SHIELD_OVERLOAD_SUSTAIN_TIME, m_flTimeLastDamaged + SHIELD_OVERLOAD_SUSTAIN_TIME + SHIELD_OVERLOAD_FADE_TIME, 1.f, 0.f);
}

void C_CombineShieldWall::ReceiveMessage(int classID, bf_read& msg)
{
	if (classID != GetClientClass()->m_ClassID)
	{
		BaseClass::ReceiveMessage(classID, msg);
		return;
	}

	int iMsgType = msg.ReadByte();
	if (iMsgType == 2)
	{
		m_flTimeLastDamaged = gpGlobals->curtime;
		return;
	}
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
				Vector vColor = pShield->GetShieldColor() * pShield->GetPowerFraction() * Lerp(pShield->GetOverloadPct(), 1.f, 4.f);
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
	IMaterialVar* m_pOverloadVar;
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

	m_pOverloadVar = pMaterial->FindVar("$FLOW_COLOR_INTENSITY", &bFound);
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
		m_pPowerupVar->SetFloatValue(Bias(pShield->GetPowerFraction(), 0.75f));
		m_pOverloadVar->SetFloatValue(Lerp(pShield->GetOverloadPct(), 1.f, 4.f));
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
