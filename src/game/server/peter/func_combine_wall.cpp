#include "cbase.h"
#include "func_combine_wall.h"

CEntityClassList<CCombineShieldWall> g_ShieldWallList;
template <> CCombineShieldWall* CEntityClassList<CCombineShieldWall>::m_pClassList = NULL;

BEGIN_DATADESC(CCombineShieldWall)
DEFINE_FIELD(m_bShieldActive, FIELD_BOOLEAN),
END_DATADESC();

IMPLEMENT_SERVERCLASS_ST(CCombineShieldWall, DT_CombineShieldWall)
SendPropBool(SENDINFO(m_bShieldActive)),
END_SEND_TABLE();

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
}

void CCombineShieldWall::Spawn()
{
	m_iSolidity = BRUSHSOLID_TOGGLE;

	Precache();
	BaseClass::Spawn();
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
}


bool CCombineShieldWall::IsOn(void) const
{
	return m_bShieldActive.Get();
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
