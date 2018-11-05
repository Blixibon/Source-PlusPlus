#include "cbase.h"
#include "bone_setup.h"
#include "hl2\weapon_physcannon.h"
#include "npc_personality_core.h"
#include "IEffects.h"
#include "info_darknessmode_lightsource.h"
#include "hl2_gamerules.h"


#define CORE_MODEL "models/npcs/personality_sphere/personality_sphere.mdl"
#define CORE_SKINS_MODEL "models/npcs/personality_sphere/personality_sphere_skins.mdl"

const char *NORMAL_ANIM_PREFIX = "sphere_idle";
const char *DAMAGED_ANIM_PREFIX = "damaged";
const char *PLUG_ANIM_PREFIX = "sphere_plug_idle";
//const char *PLUG_DAMAGED_ANIM_PREFIX = "sphere_plug_damaged_idle";

#define NORMAL_FRONT_ANIM "sphere_glance_front"
#define DAMAGED_FRONT_ANIM "sphere_damaged_glance_front"

const char *g_pszAnimTransitions[MAX_ANIMSTATES][MAX_ANIMSTATES] =
{
	{NULL,								"sphere_plug_attach",	"sphere_damaged_flip_to_front"},
	{"sphere_eye_pop_big",				 NULL,					NULL},
	{"sphere_damaged_flip_to_player",	 NULL,					NULL}
};


BEGIN_DATADESC(CNPC_Core)

DEFINE_KEYFIELD(m_bUseAltModel, FIELD_BOOLEAN, "altmodel"),
DEFINE_KEYFIELD(m_iModelSkin, FIELD_INTEGER, "ModelSkin"),
DEFINE_FIELD(m_iszIdleAnim, FIELD_STRING),
DEFINE_FIELD(m_iszIdealIdleAnim, FIELD_STRING),
DEFINE_FIELD(m_bPhysShadow, FIELD_BOOLEAN),
DEFINE_FIELD(m_bFlashlightOn, FIELD_BOOLEAN),
//DEFINE_FIELD(m_bFlippedToFront, FIELD_BOOLEAN),

DEFINE_FIELD(m_iAnimState, FIELD_INTEGER),
DEFINE_FIELD(m_iDesiredAnimState, FIELD_INTEGER),

DEFINE_INPUTFUNC(FIELD_STRING, "SetIdleSequence", InputSetIdleSequence),
DEFINE_INPUTFUNC(FIELD_VOID, "ClearIdleSequence", InputClearIdleSequence),
DEFINE_INPUTFUNC(FIELD_VOID, "EnableFlashlight", InputEnableFlashlight),
DEFINE_INPUTFUNC(FIELD_VOID, "DisableFlashlight", InputDisableFlashlight),

DEFINE_INPUTFUNC(FIELD_VOID, "ToggleFlashlight", InputToggleFlashlight),

END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_personality_core, CNPC_Core);

//Data-tables
IMPLEMENT_SERVERCLASS_ST(CNPC_Core, DT_NPC_Core)
SendPropBool(SENDINFO(m_bFlashlightOn)),
END_SEND_TABLE()

bool CNPC_Core::CreateVPhysics(void)
{
	IPhysicsObject *pPhys = NULL;

	if (GetMoveParent())
	{
		// Create the object in the physics system
		pPhys = VPhysicsInitShadow(false, false);
		m_bPhysShadow = true;
	}
	else
	{
		// Create the object in the physics system
		pPhys = VPhysicsInitNormal(SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false);
		m_bPhysShadow = false;
	}

	return (pPhys != NULL);
}

void CNPC_Core::ModifyOrAppendCriteria(AI_CriteriaSet& set)
{
	BaseClass::ModifyOrAppendCriteria(set);

	if (m_bUseAltModel)
	{
		set.AppendCriteria("coretype", UTIL_VarArgs("%d", m_iModelSkin));
		set.AppendCriteria("brokeneye", (m_iModelSkin == 0) ? "1" : "0");
	}
	else
	{
		set.AppendCriteria("coretype", "0");
		set.AppendCriteria("brokeneye", (m_iModelSkin == 0) ? "1" : "0");
	}
	
	if (GetPotentialSpeechTarget() && GetPotentialSpeechTarget()->ClassMatches(m_iClassname))
	{
		CNPC_Core *pCore = assert_cast<CNPC_Core *> (GetPotentialSpeechTarget());

		if (pCore->m_bUseAltModel)
		{
			set.AppendCriteria("targetcoretype", UTIL_VarArgs("%d", pCore->m_iModelSkin));
		}
		else
		{
			set.AppendCriteria("targetcoretype", "0");
		}
	}
	else
	{
		set.AppendCriteria("targetcoretype", "-1");
	}
}

void CNPC_Core::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bPhysShadow)
	{
		CBasePlayer *pPlayer = ToBasePlayer(pActivator);
		if (pPlayer)
		{
			pPlayer->PickupObject(this, false);
		}
	}

	SpeakIfAllowed(TLK_HELLO, NULL, true);
}

void CNPC_Core::PlayerPenetratingVPhysics(void)
{
	// We don't care!
}

void CNPC_Core::ModifyEmitSoundParams(EmitSound_t &params)
{
	BaseClass::ModifyEmitSoundParams(params);

	if (params.m_nChannel == CHAN_VOICE)
	{
		//params.m_nFlags |= SND_IGNORE_PHONEMES;
		if (params.m_SoundLevel == SNDLVL_NONE)
			params.m_SoundLevel = SNDLVL_TALKING;
	}
}

void CNPC_Core::Precache()
{
	BaseClass::Precache();

	PrecacheModel(CORE_MODEL);
	PrecacheModel(CORE_SKINS_MODEL);

	PrecacheScriptSound("VFX.SphereFlashlightOn");
}

void CNPC_Core::PrescheduleThink()
{
	if (m_bPhysShadow && !GetMoveParent())
	{
		VPhysicsDestroyObject();
		CreateVPhysics();
	}
	else if (GetMoveParent() && !m_bPhysShadow)
	{
		VPhysicsDestroyObject();
		CreateVPhysics();
	}

	BaseClass::PrescheduleThink();

	UpdateIdleAnimation();
}

void CNPC_Core::GatherConditions()
{
	BaseClass::GatherConditions();

	if (m_iDesiredAnimState != m_iAnimState)
		SetCondition(COND_CORE_ANIM_CHANGED);
	else
		ClearCondition(COND_CORE_ANIM_CHANGED);
}

int CNPC_Core::SelectSchedule(void)
{
	if (HasCondition(COND_CORE_ANIM_CHANGED))
		return SCHED_CORE_TRANSITION;

	return BaseClass::SelectSchedule();
}

void CNPC_Core::UpdateIdleAnimation()
{
	if (m_iAnimState != ANIM_NORMAL)
	{
		string_t iszIdle = m_iszIdealIdleAnim;

		bool bIsDamaged = (Q_stristr(m_iszIdealIdleAnim.ToCStr(), DAMAGED_ANIM_PREFIX) != NULL);

		if (m_iAnimState == ANIM_FRONT)
		{
			iszIdle = AllocPooledString(bIsDamaged ? DAMAGED_FRONT_ANIM : NORMAL_FRONT_ANIM);

			if (CapabilitiesGet() & bits_CAP_ANIMATEDFACE)
				CapabilitiesRemove(bits_CAP_ANIMATEDFACE);
		}

		if (GetIdealActivity() == ACT_IDLE && m_iszIdleAnim != iszIdle)
			ResetActivity();

		m_iszIdleAnim = iszIdle;
	}
	else
	{
		if (GetIdealActivity() == ACT_IDLE && m_iszIdleAnim != m_iszIdealIdleAnim)
			ResetActivity();

		m_iszIdleAnim = m_iszIdealIdleAnim;
		if (!(CapabilitiesGet() & bits_CAP_ANIMATEDFACE))
			CapabilitiesAdd(bits_CAP_ANIMATEDFACE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Core::Spawn(void)
{
	Precache();

	if (m_bUseAltModel)
		SetModel(CORE_SKINS_MODEL);
	else
		SetModel(CORE_MODEL);

	m_iModelSkin = m_nSkin = clamp(m_iModelSkin, 0, 3);

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags(SF_NPC_FADE_CORPSE);
	AddEffects(EF_NOSHADOW);
#endif // _XBOX

	SetHullType(HULL_SMALL_CENTERED);
	SetHullSizeNormal();

	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetMoveType(MOVETYPE_VPHYSICS);

	m_bloodColor = DONT_BLEED;
	SetViewOffset(Vector(0, 0, 1));		// Position of the eyes relative to NPC's origin.
	m_flFieldOfView = 0.2;
	m_NPCState = NPC_STATE_NONE;

	m_iHealth = 9999;

	SetNavType(NAV_NONE);

	m_takedamage = DAMAGE_NO;

	CapabilitiesAdd(bits_CAP_FRIENDLY_DMG_IMMUNE | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD);

	// This entity cannot be dissolved by the combine balls,
	// nor does it get killed by the mega physcannon.
	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL);

	m_iAnimState = m_iDesiredAnimState = ANIM_NORMAL;

	NPCInit();
	
	//m_iLookLayer = AddGestureSequence(LookupSequence("sphere_aimmatrix_eyeball"), false);

	//SetLayerNoRestore(m_iLookLayer, true);
}

void CNPC_Core::OnRestore()
{
	BaseClass::OnRestore();

	/*if (GetPlayerHoldingEntity(this) != NULL)
		m_iLookLayer = AddGestureSequence(LookupSequence("sphere_aimmatrix_neutral"), false);
	else
		m_iLookLayer = AddGestureSequence(LookupSequence("sphere_aimmatrix_eyeball"), false);*/

	//SetLayerNoRestore(m_iLookLayer, true);
}

//-----------------------------------------------------------------------------
// Start task!
//-----------------------------------------------------------------------------
void CNPC_Core::StartTask(const Task_t *pTask)
{
	//int task = pTask->iTask;
	switch (pTask->iTask)
	{
	case TASK_FACE_AWAY_FROM_SAVEPOSITION:
	case TASK_FACE_ENEMY:
	case TASK_FACE_HINTNODE:
	case TASK_FACE_IDEAL:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_PATH:
	case TASK_FACE_PLAYER:
	case TASK_FACE_REASONABLE:
	case TASK_FACE_SAVEPOSITION:
	case TASK_FACE_SCRIPT:
	case TASK_FACE_TARGET:
		TaskComplete();
		break;

	case TASK_CORE_TRANSITION:
		if (g_pszAnimTransitions[m_iAnimState][m_iDesiredAnimState] == NULL)
		{
			m_iAnimState = m_iDesiredAnimState;
			UpdateIdleAnimation();
			TaskComplete();
		}
		else
		{
			int iSequence = LookupSequence(g_pszAnimTransitions[m_iAnimState][m_iDesiredAnimState]);
			SetIdealActivity(ACT_DO_NOT_DISTURB);
			SetSequenceById(iSequence);
		}
		break;
		
	default:
		BaseClass::StartTask(pTask);
	}
}

void CNPC_Core::RunTask(const Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_CORE_TRANSITION:
		if (IsSequenceFinished())
		{
			m_iAnimState = m_iDesiredAnimState;
			UpdateIdleAnimation();
			TaskComplete();
		}
		break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

void CNPC_Core::OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason)
{
	/*if (m_iLookLayer != -1)
	{
		RemoveLayer(m_iLookLayer, 0.2, 0.2);
		m_iLookLayer = -1;
	}

	
	m_iLookLayer = AddGestureSequence(LookupSequence("sphere_aimmatrix_neutral"), false);

	SetLayerNoRestore(m_iLookLayer, true);*/

	if (reason != PUNTED_BY_CANNON)
	{
		if (/*HL2GameRules()->IsAlyxInDarknessMode() &&*/ m_bFlashlightOn)
			m_iDesiredAnimState = ANIM_FRONT;
	}
}

void CNPC_Core::OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t reason)
{
	/*if (m_iLookLayer != -1)
	{
		RemoveLayer(m_iLookLayer, 0.2, 0.2);
		m_iLookLayer = -1;
	}


	m_iLookLayer = AddGestureSequence(LookupSequence("sphere_aimmatrix_eyeball"), false);

	SetLayerNoRestore(m_iLookLayer, true);*/

	if (m_iDesiredAnimState == ANIM_FRONT)
		m_iDesiredAnimState = ANIM_NORMAL;
}


static ConVar scene_clamplookat("scene_clamplookat", "1", FCVAR_NONE, "Clamp head turns to a max of 20 degrees per think.");


void CNPC_Core::UpdateHeadControl(const Vector &vHeadTarget, float flHeadInfluence)
{
#if 1
	float flTarget;
	float flLimit;

	if (!(CapabilitiesGet() & bits_CAP_TURN_HEAD))
	{
		return;
	}

	// calc current animation head bias, movement needs to clamp accumulated with this
	QAngle angBias;
	QAngle vTargetAngles;

	int iEyes = LookupAttachment("eyes");
	int iChest = LookupAttachment("chest");
	int iForward = /*LookupAttachment("forward")*/ iEyes;

	matrix3x4_t eyesToWorld;
	matrix3x4_t forwardToWorld, worldToForward;

	if (iEyes <= 0 || iForward <= 0)
	{
		// Head control on model without "eyes" or "forward" attachment
		// Most likely this is a cheaple or a generic_actor set to a model that doesn't support head/eye turning.
		DevWarning("%s using model \"%s\" that doesn't support head turning\n", GetClassname(), STRING(GetModelName()));
		CapabilitiesRemove(bits_CAP_TURN_HEAD);
		return;
	}


	GetAttachment(iEyes, eyesToWorld);

	GetAttachment(iForward, forwardToWorld);
	MatrixInvert(forwardToWorld, worldToForward);

	// Lookup chest attachment to do compounded range limit checks
	if (iChest > 0)
	{
		matrix3x4_t chestToWorld, worldToChest;
		GetAttachment(iChest, chestToWorld);
		MatrixInvert(chestToWorld, worldToChest);
		matrix3x4_t tmpM;
		ConcatTransforms(worldToChest, eyesToWorld, tmpM);
		MatrixAngles(tmpM, angBias);

		angBias.y -= Get(m_ParameterHeadYaw);
		angBias.x -= Get(m_ParameterHeadPitch);
		angBias.z -= Get(m_ParameterHeadRoll);

		/*
		if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
		{
		// Msg("bias %f %f %f\n", angBias.x, angBias.y, angBias.z );

		Vector tmp1, tmp2;

		VectorTransform( Vector( 0, 0, 0), chestToWorld, tmp1 );
		VectorTransform( Vector( 100, 0, 0), chestToWorld, tmp2 );
		NDebugOverlay::Line( tmp1, tmp2, 0,0,255, false, 0.12 );

		VectorTransform( Vector( 0, 0, 0), eyesToWorld, tmp1 );
		VectorTransform( Vector( 100, 0, 0), eyesToWorld, tmp2 );
		NDebugOverlay::Line( tmp1, tmp2, 0,0,255, false, 0.12 );

		// NDebugOverlay::Line( EyePosition(), pEntity->EyePosition(), 0,0,255, false, 0.5);
		}
		*/
	}
	else
	{
		angBias.Init(0, 0, 0);
	}

	matrix3x4_t targetXform;
	targetXform = forwardToWorld;
	Vector vTargetDir = vHeadTarget - EyePosition();

	if (scene_clamplookat.GetBool())
	{
		// scale down pitch when the target is behind the head
		Vector vTargetLocal;
		VectorNormalize(vTargetDir);
		VectorIRotate(vTargetDir, forwardToWorld, vTargetLocal);
		vTargetLocal.z *= clamp(vTargetLocal.x, 0.1f, 1.0f);
		VectorNormalize(vTargetLocal);
		VectorRotate(vTargetLocal, forwardToWorld, vTargetDir);

		// clamp local influence when target is behind the head
		flHeadInfluence = flHeadInfluence * clamp(vTargetLocal.x * 2.0f + 2.0f, 0.0f, 1.0f);
	}

	Studio_AlignIKMatrix(targetXform, vTargetDir);

	matrix3x4_t headXform;
	ConcatTransforms(worldToForward, targetXform, headXform);
	MatrixAngles(headXform, vTargetAngles);

	// partially debounce head goal
	float s0 = 1.0 - flHeadInfluence + GetHeadDebounce() * flHeadInfluence;
	float s1 = (1.0 - s0);
	// limit velocity of head turns
	m_goalHeadCorrection.x = UTIL_Approach(m_goalHeadCorrection.x * s0 + vTargetAngles.x * s1, m_goalHeadCorrection.x, 10.0);
	m_goalHeadCorrection.y = UTIL_Approach(m_goalHeadCorrection.y * s0 + vTargetAngles.y * s1, m_goalHeadCorrection.y, 30.0);
	m_goalHeadCorrection.z = UTIL_Approach(m_goalHeadCorrection.z * s0 + vTargetAngles.z * s1, m_goalHeadCorrection.z, 10.0);

	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
	// Msg( "yaw %.1f (%f) pitch %.1f (%.1f)\n", m_goalHeadCorrection.y, vTargetAngles.y, vTargetAngles.x, m_goalHeadCorrection.x );
	// Msg( "yaw %.2f (goal %.2f) (influence %.2f) (flex %.2f)\n", flLimit, m_goalHeadCorrection.y, flHeadInfluence, Get( m_FlexweightHeadRightLeft ) );
	}
	*/

	flTarget = m_goalHeadCorrection.y + Get(m_FlexweightHeadRightLeft);
	flLimit = RangeCompressor(flTarget, -15, 15, angBias.y);
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
	Msg( "yaw  %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.y, Get( m_FlexweightHeadRightLeft ), angBias.y, Get( m_ParameterHeadYaw ) );
	}
	*/
	SetPoseParameter(m_poseAim_Yaw, -flLimit);

	flTarget = m_goalHeadCorrection.x + Get(m_FlexweightHeadUpDown);
	flLimit = RangeCompressor(flTarget, -15, 15, angBias.x);
	/*
	if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	{
	Msg( "pitch %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.x, Get( m_FlexweightHeadUpDown ), angBias.x, Get( m_ParameterHeadPitch ) );
	}
	*/
	SetPoseParameter(m_poseAim_Pitch, -flLimit);

	//flTarget = m_goalHeadCorrection.z + Get(m_FlexweightHeadTilt);
	//flLimit = ClampWithBias(m_ParameterHeadRoll, flTarget, angBias.z);
	///*
	//if ( (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) )
	//{
	//Msg( "roll  %5.1f : (%5.1f : %5.1f) %5.1f (%5.1f)\n", flLimit, m_goalHeadCorrection.z, Get( m_FlexweightHeadTilt ), angBias.z, Get( m_ParameterHeadRoll ) );
	//}
	//*/
	//Set(m_ParameterHeadRoll, flLimit);
#endif
}

//=========================================================
// SelectWeightedSequence
//=========================================================
int CNPC_Core::SelectWeightedSequence(Activity activity)
{
	if (activity == ACT_IDLE && m_iszIdleAnim != NULL_STRING)
	{
		int iSeq = LookupSequence(STRING(m_iszIdleAnim));
		if (iSeq != ACT_INVALID)
			return iSeq;
	}

	return BaseClass::SelectWeightedSequence(activity, GetSequence());
}


int CNPC_Core::SelectWeightedSequence(Activity activity, int curSequence)
{
	if (activity == ACT_IDLE && m_iszIdleAnim != NULL_STRING)
	{
		int iSeq = LookupSequence(STRING(m_iszIdleAnim));
		if (iSeq != ACT_INVALID)
			return iSeq;
	}
	
	return BaseClass::SelectWeightedSequence(activity, curSequence);
}

//=========================================================
// LookupHeaviestSequence
//
// Get sequence with highest 'weight' for this activity
//
//=========================================================
int CNPC_Core::SelectHeaviestSequence(Activity activity)
{
	if (activity == ACT_IDLE && m_iszIdleAnim != NULL_STRING)
	{
		int iSeq = LookupSequence(STRING(m_iszIdleAnim));
		if (iSeq != ACT_INVALID)
			return iSeq;
	}
	
	return BaseClass::SelectHeaviestSequence(activity);
}

void CNPC_Core::InputSetIdleSequence(inputdata_t &inputdata)
{
	m_iszIdealIdleAnim = inputdata.value.StringID();
	UpdateIdleAnimation();
}

void CNPC_Core::InputClearIdleSequence(inputdata_t &inputdata)
{
	m_iszIdealIdleAnim = NULL_STRING;
	UpdateIdleAnimation();
}

void CNPC_Core::InputEnableFlashlight(inputdata_t &inputdata)
{
	if (!m_bFlashlightOn)
	{
		Vector vecEyes, vecForward;
		GetAttachment("eyes", vecEyes, &vecForward);
		CPASAttenuationFilter filter(vecEyes, "VFX.SphereFlashlightOn");
		EmitSound(filter, entindex(), "VFX.SphereFlashlightOn", &vecEyes);
		
		g_pEffects->Sparks(vecEyes, 2, 2, &vecForward);

		AddEntityToDarknessCheck(this);

		if (VPhysicsGetObject() && VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
			m_iDesiredAnimState = ANIM_FRONT;

		m_bFlashlightOn = true;
	}
}

void CNPC_Core::InputDisableFlashlight(inputdata_t &inputdata)
{
	if (m_bFlashlightOn)
	{
		if (VPhysicsGetObject() && VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
			m_iDesiredAnimState = ANIM_NORMAL;

		m_bFlashlightOn = false;

		RemoveEntityFromDarknessCheck(this);
	}
}

void CNPC_Core::InputToggleFlashlight(inputdata_t &inputdata)
{
	if (!m_bFlashlightOn)
	{
		AcceptInput("EnableFlashlight", inputdata.pActivator, inputdata.pCaller, inputdata.value, inputdata.nOutputID);
	}
	else
	{
		AcceptInput("DisableFlashlight", inputdata.pActivator, inputdata.pCaller, inputdata.value, inputdata.nOutputID);
	}
}

void CNPC_Core::BuildScheduleTestBits(void)
{
	BaseClass::BuildScheduleTestBits();

	if (GetCurSchedule()->HasInterrupt(COND_IDLE_INTERRUPT))
	{
		SetCustomInterruptCondition(COND_CORE_ANIM_CHANGED);
	}
}

AI_BEGIN_CUSTOM_NPC(npc_personality_core, CNPC_Core)

DECLARE_TASK(TASK_CORE_TRANSITION)

DECLARE_CONDITION(COND_CORE_ANIM_CHANGED)

DEFINE_SCHEDULE
(
	SCHED_CORE_TRANSITION,

	"	Tasks"
	"		TASK_CORE_TRANSITION			0"
	""
	"	Interrupts"
	)

AI_END_CUSTOM_NPC()