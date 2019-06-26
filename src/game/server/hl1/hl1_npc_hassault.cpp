#include "cbase.h"
#include "hl1_ai_basenpc.h"
#include "hl1_shareddefs.h"
#include "ammodef.h"
#include "npcevent.h"

#define AE_GRUNT_FIRE_PISTOL 1
#define AE_GRUNT_RELOAD 2
#define AE_GRUNT_GIVE_MINIGUN 12

enum {
	WEAP_MINIGUN = 0,
	WEAP_PISTOL,
	WEAP_DEAGLE,
	WEAP_357,

	NUM_HWEAPONS
};

int g_iClipSizes[NUM_HWEAPONS] = {
	150,
	18,
	8,
	6
};

const char *g_pszHWFireSounds[NUM_HWEAPONS] = {
	"HWGrunt.Minigun",
	"Barney.FirePistol",
	"NPC_Otis.FirePistol",
	"HL1Weapon_357.Single"
};

class CNPC_HWGrunt : public CHL1BaseNPC
{
	DECLARE_CLASS(CNPC_HWGrunt, CHL1BaseNPC);
public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	void	Precache(void);
	void	Spawn(void);

	Class_T	Classify(void);

	Vector	Weapon_ShootPosition(void);
	virtual bool		InnateWeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	void	HandleAnimEvent(animevent_t *pEvent);

	void	StartTask(const Task_t *pTask);
	void	RunTask(const Task_t *pTask);

	void	PainSound(const CTakeDamageInfo &info);
	void	DeathSound(const CTakeDamageInfo &info);

	void	FireGun();

	int		TranslateSchedule(int scheduleType);
	Activity NPC_TranslateActivity(Activity eNewActivity);

	void	CheckAmmo(void);

	void	Event_Killed(const CTakeDamageInfo &info);

	void	SetWeapon(int iWeapon)
	{
		iWeapon = Clamp(iWeapon, (int)WEAP_MINIGUN, NUM_HWEAPONS - 1);
		m_iWeapon = iWeapon;
		SetBodygroup(1, iWeapon);
		m_iClipSize = g_iClipSizes[iWeapon];
		m_cAmmoLoaded = m_iClipSize;
		SetHitboxSet((iWeapon == WEAP_MINIGUN) ? 0 : 1);
	}

	enum {
		SCHED_HWGRUNT_RELOAD = BaseClass::NEXT_SCHEDULE,
		SCHED_HWGRUNT_FIRE_MINIGUN,
	};

protected:
	int		m_iWeapon;
	int		m_iAmmoTypes[NUM_HWEAPONS];
	int		m_iClipSize;
	float m_flNextPainTime;
};

LINK_ENTITY_TO_CLASS(monster_human_assault, CNPC_HWGrunt);

BEGIN_DATADESC(CNPC_HWGrunt)
DEFINE_KEYFIELD(m_iWeapon, FIELD_INTEGER, "weapon"),
DEFINE_FIELD(m_iClipSize, FIELD_INTEGER),
END_DATADESC();

void CNPC_HWGrunt::Precache(void)
{
	m_iAmmoTypes[0] = GetAmmoDef()->Index(HL1_12MM_AMMO);
	m_iAmmoTypes[1] = GetAmmoDef()->Index(HL1_PISTOL_AMMO);
	m_iAmmoTypes[2] = m_iAmmoTypes[3] = GetAmmoDef()->Index(HL1_357_AMMO);

	PrecacheModel("models/half-life/hwgrunt.mdl");

	//PrecacheScriptSound("HGrunt.Reload");
	//PrecacheScriptSound("HGrunt.GrenadeLaunch");
	PrecacheScriptSound("HWGrunt.SpinUp");
	PrecacheScriptSound("HWGrunt.SpinDown");
	PrecacheScriptSound("HGrunt.Pain");
	PrecacheScriptSound("HGrunt.Die");

	for (int i = 0; i < NUM_HWEAPONS; i++)
	{
		PrecacheScriptSound(g_pszHWFireSounds[i]);
	}

	BaseClass::Precache();
}

void CNPC_HWGrunt::Spawn(void)
{
	Precache();

	SetModel("models/half-life/hwgrunt.mdl");

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	m_bloodColor = BLOOD_COLOR_RED;
	ClearEffects();
	m_iHealth = 160;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE | bits_CAP_DOORS_GROUP | bits_CAP_MOVE_GROUND);

	CapabilitiesAdd(bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_AIM_GUN);

	m_HackedGunPos = Vector(0, 0, 55);

	BaseClass::Spawn();

	NPCInit();

	SetWeapon(m_iWeapon);
}

//-----------------------------------------------------------------------------
// Purpose: Combine needs to check ammo
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_HWGrunt::CheckAmmo(void)
{
	if (m_cAmmoLoaded <= 0)
		SetCondition(COND_NO_PRIMARY_AMMO);

}

void CNPC_HWGrunt::Event_Killed(const CTakeDamageInfo & info)
{
	Vector	vecGunPos;
	QAngle	vecGunAngles;

	GetAttachment("gun", vecGunPos, vecGunAngles);

	// If the gun would drop into a wall, spawn it at our origin
	if (UTIL_PointContents(vecGunPos) & CONTENTS_SOLID)
	{
		vecGunPos = GetAbsOrigin();
	}

	switch (m_iWeapon)
	{
	case WEAP_MINIGUN:
	default:
		break;
	case WEAP_PISTOL:
		SetBodygroup(1, NUM_HWEAPONS);
		DropItem("weapon_glock_hl1", vecGunPos, vecGunAngles);
		break;
	case WEAP_DEAGLE:
		SetBodygroup(1, NUM_HWEAPONS);
		DropItem("weapon_deagle", vecGunPos, vecGunAngles);
		break;
	case WEAP_357:
		SetBodygroup(1, NUM_HWEAPONS);
		DropItem("weapon_357_hl1", vecGunPos, vecGunAngles);
		break;
	}

	StopSound("HWGrunt.SpinUp");

	BaseClass::Event_Killed(info);
}

Class_T CNPC_HWGrunt::Classify(void)
{
	return CLASS_HUMAN_MILITARY;
}

Vector CNPC_HWGrunt::Weapon_ShootPosition(void)
{
	if (m_iWeapon == WEAP_MINIGUN)
	{
		Vector vecShootOrigin;
		if (GetAttachment("muzzle", vecShootOrigin))
			return vecShootOrigin;
	}

	return BaseClass::Weapon_ShootPosition();
}

bool CNPC_HWGrunt::InnateWeaponLOSCondition(const Vector & ownerPos, const Vector & targetPos, bool bSetConditions)
{
	// Find its relative shoot position
	Vector vecRelativeShootPosition;
	VectorSubtract(Weapon_ShootPosition(), GetAbsOrigin(), vecRelativeShootPosition);
	Vector barrelPos = ownerPos + vecRelativeShootPosition;

	trace_t tr;
	AI_TraceLine(barrelPos, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	if (tr.fraction == 1.0)
	{
		return true;
	}

	CBaseEntity	*pHitEntity = tr.m_pEnt;

	// Translate a hit vehicle into its passenger if found
	if (GetEnemy() != NULL)
	{
		CBaseCombatCharacter *pCCEnemy = GetEnemy()->MyCombatCharacterPointer();
		if (pCCEnemy != NULL && pCCEnemy->IsInAVehicle())
		{
			// Ok, player in vehicle, check if vehicle is target we're looking at, fire if it is
			// Also, check to see if the owner of the entity is the vehicle, in which case it's valid too.
			// This catches vehicles that use bone followers.
			CBaseEntity *pVehicleEnt = pCCEnemy->GetVehicleEntity();
			if (pHitEntity == pVehicleEnt || pHitEntity->GetOwnerEntity() == pVehicleEnt)
				return true;
		}
	}

	if (pHitEntity == GetEnemy())
	{
		return true;
	}
	else if (pHitEntity && pHitEntity->MyCombatCharacterPointer())
	{
		if (IRelationType(pHitEntity) == D_HT)
		{
			return true;
		}
		else if (bSetConditions)
		{
			SetCondition(COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}
	else if (bSetConditions)
	{
		SetCondition(COND_WEAPON_SIGHT_OCCLUDED);
		SetEnemyOccluder(tr.m_pEnt);
	}

	return false;
}

void CNPC_HWGrunt::HandleAnimEvent(animevent_t * pEvent)
{
	switch (pEvent->event)
	{
	case AE_GRUNT_FIRE_PISTOL:
		FireGun();
		break;
	case AE_GRUNT_GIVE_MINIGUN:
		SetWeapon(WEAP_MINIGUN);
		break;
	case AE_GRUNT_RELOAD:
		m_cAmmoLoaded = m_iClipSize;
		break;
	default:
		BaseClass::HandleAnimEvent(pEvent);
		break;
	}
}

void CNPC_HWGrunt::StartTask(const Task_t * pTask)
{
	BaseClass::StartTask(pTask);
}

void CNPC_HWGrunt::RunTask(const Task_t * pTask)
{
	BaseClass::RunTask(pTask);
}

void CNPC_HWGrunt::PainSound(const CTakeDamageInfo & info)
{
	if (gpGlobals->curtime > m_flNextPainTime)
	{
		CPASAttenuationFilter filter(this);
		EmitSound(filter, entindex(), "HGrunt.Pain");

		m_flNextPainTime = gpGlobals->curtime + 1;
	}
}

void CNPC_HWGrunt::DeathSound(const CTakeDamageInfo & info)
{
	CPASAttenuationFilter filter(this, ATTN_IDLE);
	EmitSound(filter, entindex(), "HGrunt.Die");
}

void CNPC_HWGrunt::FireGun()
{
	Vector vecShootOrigin;
	Vector vecShootDir;

	if (GetEnemy())
	{
		vecShootOrigin = Weapon_ShootPosition();
		vecShootDir = GetShootEnemyDir(vecShootOrigin);
	}
	else
	{
		int iAttachment = 0;
		if (m_iWeapon > WEAP_MINIGUN && m_iWeapon < NUM_HWEAPONS)
		{
			iAttachment = LookupAttachment(CFmtStr("muzzle%d", m_iWeapon));
		}
		else
		{
			iAttachment = LookupAttachment("muzzle");
		}

		GetAttachment(iAttachment, vecShootOrigin, &vecShootDir);
	}

	Vector forward, right, up;
	AngleVectors(GetAbsAngles(), &forward, &right, &up);

	Vector	vecShellVelocity = right * random->RandomFloat(40, 90) + up * random->RandomFloat(75, 200) + forward * random->RandomFloat(-40, 40);
	EjectShell(vecShootOrigin, vecShellVelocity, GetAbsAngles().y, 0);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, m_iAmmoTypes[m_iWeapon]); // shoot +-5 degrees

	CPASAttenuationFilter filter(vecShootOrigin, g_pszHWFireSounds[m_iWeapon]);
	EmitSound(filter, entindex(), g_pszHWFireSounds[m_iWeapon], &vecShootOrigin);

	DoMuzzleFlash();

	if (m_iWeapon > WEAP_MINIGUN)
		m_cAmmoLoaded--;// take away a bullet!
}

int CNPC_HWGrunt::TranslateSchedule(int scheduleType)
{
	switch (scheduleType)
	{
	case SCHED_RELOAD:
		return SCHED_HWGRUNT_RELOAD;
		break;
	case SCHED_RANGE_ATTACK1:
		if (m_iWeapon == WEAP_MINIGUN)
		{
			return SCHED_HWGRUNT_FIRE_MINIGUN;
			break;
		}
	default:
		return BaseClass::TranslateSchedule(scheduleType);
		break;
	}
}

Activity CNPC_HWGrunt::NPC_TranslateActivity(Activity eNewActivity)
{
	if (m_iWeapon > WEAP_MINIGUN)
	{
		switch (eNewActivity)
		{
		case ACT_IDLE:
			return ACT_IDLE_PISTOL;
			break;
		case ACT_WALK:
			return ACT_WALK_PISTOL;
			break;
		case ACT_RUN:
			return ACT_RUN_PISTOL;
			break;
		case ACT_RANGE_ATTACK1:
			return ACT_RANGE_ATTACK_PISTOL;
			break;
		case ACT_RANGE_ATTACK1_LOW:
			return ACT_RANGE_ATTACK_PISTOL_LOW;
			break;
		case ACT_RELOAD:
			return ACT_RELOAD_PISTOL;
			break;
		default:
			break;
		}
	}

	return BaseClass::NPC_TranslateActivity(eNewActivity);
}

//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC(monster_human_assault, CNPC_HWGrunt)

DEFINE_SCHEDULE(
	SCHED_HWGRUNT_RELOAD,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_RELOAD_PISTOL"
	"	"
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
)

DEFINE_SCHEDULE(
	SCHED_HWGRUNT_FIRE_MINIGUN,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_FACE_ENEMY				0"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_RANGE_ATTACK1"
	"		TASK_WAIT_FACE_ENEMY		5"
	"		TASK_WAIT_FACE_ENEMY_RANDOM	5"
	"		TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE"
	"	"
	"	Interrupts"
	"		COND_HEAVY_DAMAGE"
	"		COND_NEW_ENEMY"
	"		COND_LOST_ENEMY"
	"		COND_HEAR_DANGER"
	"		COND_ENEMY_DEAD"
	"		COND_ENEMY_OCCLUDED"
)

AI_END_CUSTOM_NPC()