#include "cbase.h"
#include "c_npc_basecollegue.h"
#include "networkstringtabledefs.h"
#include "saverestore_stringtable.h"
#include "ragdoll_shared.h"
#include "debugoverlay_shared.h"
#include "vphysics/constraints.h"
#include "c_entitydissolve.h"
#include "c_fire_smoke.h"
#include "saverestore_utlvector.h"
#include "peter/c_entityelectric.h"
#include "vprof.h"


extern INetworkStringTable* g_pStringTableHeadShapes;
extern CStringTableSaveRestoreOps g_ParticleStringTableOPs;

C_EntityDissolve* DissolveEffect(C_BaseEntity* pTarget, float flTime);
C_EntityFlame* FireEffect(C_BaseAnimating* pTarget, C_BaseEntity* pServerFire, float* flScaleEnd, float* flTimeStart, float* flTimeEnd);
C_EntityElectric* ShockEffect(C_BaseEntity* pTarget, float flTime, int iType);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_NPC_BaseColleague::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (m_nFlexTableIndex != m_nOldFlexTableIndex)
	{
		m_nOldFlexTableIndex = m_nFlexTableIndex;
		m_FlexData.Purge();

		if (m_nFlexTableIndex > -1)
		{
			int iLen = 0;
			const void* pData = g_pStringTableHeadShapes->GetStringUserData(m_nFlexTableIndex, &iLen);
			CUtlBuffer buf(pData, iLen, CUtlBuffer::READ_ONLY);
			buf.SetBigEndian(true);

			Assert(iLen % sizeof(ManifestFlexData_t) == 0);
			int iNum = iLen / sizeof(ManifestFlexData_t);
			m_FlexData.SetSize(iNum);
			for (int i = 0; i < m_FlexData.Count(); i++)
			{
				m_FlexData[i].Unserialize(buf);
			}
		}

		m_FlexControllers.Purge();
		for (int i = 0; i < m_FlexData.Count(); i++)
		{
			LocalFlexController_t controller = FindFlexController(m_FlexData[i].cName);
			m_FlexControllers.AddToTail(controller);
		}
	}
}

class C_CollegueClientRagdoll : public C_BaseFlex, public IPVSNotify
{

public:
	C_CollegueClientRagdoll(bool bRestoring = true);
	DECLARE_CLASS(C_CollegueClientRagdoll, C_BaseFlex);
	DECLARE_DATADESC();

	// inherited from IPVSNotify
	virtual void OnPVSStatusChanged(bool bInPVS);

	virtual void Release(void);
	virtual void		ProcessSceneEvents(bool bFlexEvents);
	virtual void SetupWeights(const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights);
	virtual void ImpactTrace(trace_t* pTrace, int iDamageType, const char* pCustomImpactName);
	void ClientThink(void);
	void ReleaseRagdoll(void) { m_bReleaseRagdoll = true; }
	bool ShouldSavePhysics(void) { return true; }
	virtual void	OnSave();
	virtual void	OnRestore();
	virtual int ObjectCaps(void) { return BaseClass::ObjectCaps() | FCAP_SAVE_NON_NETWORKABLE; }
	virtual IPVSNotify* GetPVSNotifyInterface() { return this; }

	virtual void					TransferDissolveFrom(C_BaseAnimating* pSource);
	virtual void					TransferElectricsFrom(C_BaseAnimating* pSource);

	void	HandleAnimatedFriction(void);
	virtual void SUB_Remove(void);

	void	FadeOut(void);
	virtual float LastBoneChangedTime();

	bool m_bFadeOut;
	bool m_bImportant;
	float m_flEffectTime;

private:
	int m_iCurrentFriction;
	int m_iMinFriction;
	int m_iMaxFriction;
	float m_flFrictionModTime;
	float m_flFrictionTime;

	int  m_iFrictionAnimState;
	bool m_bReleaseRagdoll;

	bool m_bFadingOut;

	float m_flScaleEnd[NUM_HITBOX_FIRES];
	float m_flScaleTimeStart[NUM_HITBOX_FIRES];
	float m_flScaleTimeEnd[NUM_HITBOX_FIRES];

	CUtlVector<ManifestFlexData_t> m_FlexData;
	CUtlVector<LocalFlexController_t> m_FlexControllers;

	friend class C_NPC_BaseColleague;
};

LINK_ENTITY_TO_CLASS_CLIENTONLY(collegue_client_ragdoll, C_CollegueClientRagdoll);

BEGIN_DATADESC(C_CollegueClientRagdoll)
DEFINE_FIELD(m_bFadeOut, FIELD_BOOLEAN),
DEFINE_FIELD(m_bImportant, FIELD_BOOLEAN),
DEFINE_FIELD(m_iCurrentFriction, FIELD_INTEGER),
DEFINE_FIELD(m_iMinFriction, FIELD_INTEGER),
DEFINE_FIELD(m_iMaxFriction, FIELD_INTEGER),
DEFINE_FIELD(m_flFrictionModTime, FIELD_FLOAT),
DEFINE_FIELD(m_flFrictionTime, FIELD_TIME),
DEFINE_FIELD(m_iFrictionAnimState, FIELD_INTEGER),
DEFINE_FIELD(m_bReleaseRagdoll, FIELD_BOOLEAN),
DEFINE_FIELD(m_nBody, FIELD_INTEGER),
DEFINE_FIELD(m_nSkin, FIELD_INTEGER),
DEFINE_FIELD(m_nRenderFX, FIELD_CHARACTER),
DEFINE_FIELD(m_nRenderMode, FIELD_CHARACTER),
DEFINE_FIELD(m_clrRender, FIELD_COLOR32),
DEFINE_FIELD(m_flEffectTime, FIELD_TIME),
DEFINE_FIELD(m_bFadingOut, FIELD_BOOLEAN),
DEFINE_CUSTOM_FIELD(m_iFireEffectIndex, &g_ParticleStringTableOPs),

DEFINE_UTLVECTOR(m_FlexData, FIELD_EMBEDDED),

DEFINE_AUTO_ARRAY(m_flScaleEnd, FIELD_FLOAT),
DEFINE_AUTO_ARRAY(m_flScaleTimeStart, FIELD_FLOAT),
DEFINE_AUTO_ARRAY(m_flScaleTimeEnd, FIELD_FLOAT),
DEFINE_EMBEDDEDBYREF(m_pRagdoll),

END_DATADESC()

C_CollegueClientRagdoll::C_CollegueClientRagdoll(bool bRestoring)
{
	m_iCurrentFriction = 0;
	m_iFrictionAnimState = RAGDOLL_FRICTION_NONE;
	m_bReleaseRagdoll = false;
	m_bFadeOut = false;
	m_bFadingOut = false;
	m_bImportant = false;
	m_bNoModelParticles = false;

	SetClassname("collegue_client_ragdoll");

	if (bRestoring == true)
	{
		m_pRagdoll = new CRagdoll;
	}
}

void C_CollegueClientRagdoll::TransferDissolveFrom(C_BaseAnimating* pSource)
{
	C_BaseEntity* pChild = pSource->GetEffectEntity(ENT_EFFECT_DISSOLVE);

	if (pChild)
	{
		C_EntityDissolve* pDissolveChild = dynamic_cast<C_EntityDissolve*>(pChild);

		if (pDissolveChild)
		{
			m_flEffectTime = pDissolveChild->m_flStartTime;

			C_EntityDissolve* pDissolve = DissolveEffect(this, m_flEffectTime);

			if (pDissolve)
			{
				pDissolve->SetRenderMode(pDissolveChild->GetRenderMode());
				pDissolve->m_nRenderFX = pDissolveChild->m_nRenderFX;
				pDissolve->SetRenderColor(255, 255, 255, 255);
				pDissolveChild->SetRenderColorA(0);

				pDissolve->m_vDissolverOrigin = pDissolveChild->m_vDissolverOrigin;
				pDissolve->m_nDissolveType = pDissolveChild->m_nDissolveType;

				if (pDissolve->m_nDissolveType == ENTITY_DISSOLVE_CORE)
				{
					pDissolve->m_nMagnitude = pDissolveChild->m_nMagnitude;
					pDissolve->m_flFadeOutStart = CORE_DISSOLVE_FADE_START;
					pDissolve->m_flFadeOutModelStart = CORE_DISSOLVE_MODEL_FADE_START;
					pDissolve->m_flFadeOutModelLength = CORE_DISSOLVE_MODEL_FADE_LENGTH;
					pDissolve->m_flFadeInLength = CORE_DISSOLVE_FADEIN_LENGTH;
				}
			}
		}
	}
}

void C_CollegueClientRagdoll::TransferElectricsFrom(C_BaseAnimating* pSource)
{
	C_BaseEntity* pChild = pSource->GetEffectEntity(ENT_EFFECT_SHOCK);

	if (pChild)
	{
		C_EntityElectric* pDissolveChild = dynamic_cast<C_EntityElectric*>(pChild);

		if (pDissolveChild)
		{
			m_flEffectTime = pDissolveChild->m_flStartTime;

			C_EntityElectric* pDissolve = ShockEffect(this, m_flEffectTime, pDissolveChild->m_nShockType);

			if (pDissolve)
			{
				pDissolve->SetRenderMode(pDissolveChild->GetRenderMode());
				pDissolve->m_nRenderFX = pDissolveChild->m_nRenderFX;
				pDissolve->SetRenderColor(255, 255, 255, 255);
				if (pDissolveChild->IsEffectActive(EF_DIMLIGHT))
				{
					pDissolve->AddEffects(EF_DIMLIGHT);
				}
				if (pDissolveChild->IsEffectActive(EF_BRIGHTLIGHT))
				{
					pDissolve->AddEffects(EF_BRIGHTLIGHT);
				}

				pDissolveChild->SetRenderColorA(0);
			}
		}
	}
}

void C_CollegueClientRagdoll::OnSave(void)
{
}

void C_CollegueClientRagdoll::OnRestore(void)
{
	CStudioHdr* hdr = GetModelPtr();

	if (hdr == NULL)
	{
		const char* pModelName = STRING(GetModelName());
		SetModel(pModelName);

		hdr = GetModelPtr();

		if (hdr == NULL)
			return;
	}

	if (m_pRagdoll == NULL)
		return;

	ragdoll_t* pRagdollT = m_pRagdoll->GetRagdoll();

	if (pRagdollT == NULL || pRagdollT->list[0].pObject == NULL)
	{
		m_bReleaseRagdoll = true;
		m_pRagdoll = NULL;
		Assert(!"Attempted to restore a ragdoll without physobjects!");
		return;
	}

	if (GetFlags() & FL_DISSOLVING)
	{
		DissolveEffect(this, m_flEffectTime);
	}
	else if (GetFlags() & FL_ONFIRE)
	{
		C_EntityFlame* pFireChild = dynamic_cast<C_EntityFlame*>(GetEffectEntity(ENT_EFFECT_FIRE));
		C_EntityFlame* pNewFireChild = FireEffect(this, pFireChild, m_flScaleEnd, m_flScaleTimeStart, m_flScaleTimeEnd);

		//Set the new fire child as the new effect entity.
		SetEffectEntity(pNewFireChild, ENT_EFFECT_FIRE);
	}

	VPhysicsSetObject(NULL);
	VPhysicsSetObject(pRagdollT->list[0].pObject);

	SetupBones(NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime);

	pRagdollT->list[0].parentIndex = -1;
	pRagdollT->list[0].originParentSpace.Init();

	RagdollActivate(*pRagdollT, modelinfo->GetVCollide(GetModelIndex()), GetModelIndex(), true);
	RagdollSetupAnimatedFriction(physenv, pRagdollT, GetModelIndex());

	m_pRagdoll->BuildRagdollBounds(this);

	// UNDONE: The shadow & leaf system cleanup should probably be in C_BaseEntity::OnRestore()
	// this must be recomputed because the model was NULL when this was set up
	RemoveFromLeafSystem();
	AddToLeafSystem(RENDER_GROUP_OPAQUE_ENTITY);

	DestroyShadow();
	CreateShadow();

	SetNextClientThink(CLIENT_THINK_ALWAYS);

	if (m_bFadeOut == true)
	{
		s_RagdollLRU.MoveToTopOfLRU(this, m_bImportant);
	}

	NoteRagdollCreationTick(this);

	BaseClass::OnRestore();

	m_FlexControllers.Purge();
	for (int i = 0; i < m_FlexData.Count(); i++)
	{
		LocalFlexController_t controller = FindFlexController(m_FlexData[i].cName);
		m_FlexControllers.AddToTail(controller);
	}

	RagdollMoved();
}

void C_CollegueClientRagdoll::ImpactTrace(trace_t* pTrace, int iDamageType, const char* pCustomImpactName)
{
	VPROF("C_CollegueClientRagdoll::ImpactTrace");

	IPhysicsObject* pPhysicsObject = VPhysicsGetObject();

	if (!pPhysicsObject)
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if (iDamageType == DMG_BLAST)
	{
		dir *= 500;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter(dir);
	}
	else
	{
		Vector hitpos;

		VectorMA(pTrace->startpos, pTrace->fraction, dir, hitpos);
		VectorNormalize(dir);

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset(dir, hitpos);
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

extern ConVar g_debug_ragdoll_visualize;

void C_CollegueClientRagdoll::HandleAnimatedFriction(void)
{
	if (m_iFrictionAnimState == RAGDOLL_FRICTION_OFF)
		return;

	ragdoll_t* pRagdollT = NULL;
	int iBoneCount = 0;

	if (m_pRagdoll)
	{
		pRagdollT = m_pRagdoll->GetRagdoll();
		iBoneCount = m_pRagdoll->RagdollBoneCount();

	}

	if (pRagdollT == NULL)
		return;

	switch (m_iFrictionAnimState)
	{
	case RAGDOLL_FRICTION_NONE:
	{
		m_iMinFriction = pRagdollT->animfriction.iMinAnimatedFriction;
		m_iMaxFriction = pRagdollT->animfriction.iMaxAnimatedFriction;

		if (m_iMinFriction != 0 || m_iMaxFriction != 0)
		{
			m_iFrictionAnimState = RAGDOLL_FRICTION_IN;

			m_flFrictionModTime = pRagdollT->animfriction.flFrictionTimeIn;
			m_flFrictionTime = gpGlobals->curtime + m_flFrictionModTime;

			m_iCurrentFriction = m_iMinFriction;
		}
		else
		{
			m_iFrictionAnimState = RAGDOLL_FRICTION_OFF;
		}

		break;
	}

	case RAGDOLL_FRICTION_IN:
	{
		float flDeltaTime = (m_flFrictionTime - gpGlobals->curtime);

		m_iCurrentFriction = RemapValClamped(flDeltaTime, m_flFrictionModTime, 0, m_iMinFriction, m_iMaxFriction);

		if (flDeltaTime <= 0.0f)
		{
			m_flFrictionModTime = pRagdollT->animfriction.flFrictionTimeHold;
			m_flFrictionTime = gpGlobals->curtime + m_flFrictionModTime;
			m_iFrictionAnimState = RAGDOLL_FRICTION_HOLD;
		}
		break;
	}

	case RAGDOLL_FRICTION_HOLD:
	{
		if (m_flFrictionTime < gpGlobals->curtime)
		{
			m_flFrictionModTime = pRagdollT->animfriction.flFrictionTimeOut;
			m_flFrictionTime = gpGlobals->curtime + m_flFrictionModTime;
			m_iFrictionAnimState = RAGDOLL_FRICTION_OUT;
		}

		break;
	}

	case RAGDOLL_FRICTION_OUT:
	{
		float flDeltaTime = (m_flFrictionTime - gpGlobals->curtime);

		m_iCurrentFriction = RemapValClamped(flDeltaTime, 0, m_flFrictionModTime, m_iMinFriction, m_iMaxFriction);

		if (flDeltaTime <= 0.0f)
		{
			m_iFrictionAnimState = RAGDOLL_FRICTION_OFF;
		}

		break;
	}
	}

	for (int i = 0; i < iBoneCount; i++)
	{
		if (pRagdollT->list[i].pConstraint)
			pRagdollT->list[i].pConstraint->SetAngularMotor(0, m_iCurrentFriction);
	}

	IPhysicsObject* pPhysicsObject = VPhysicsGetObject();

	if (pPhysicsObject)
	{
		pPhysicsObject->Wake();
	}
}

extern ConVar g_ragdoll_fadespeed;
extern ConVar g_ragdoll_lvfadespeed;

void C_CollegueClientRagdoll::OnPVSStatusChanged(bool bInPVS)
{
	if (bInPVS)
	{
		CreateShadow();
	}
	else
	{
		DestroyShadow();
	}
}

void C_CollegueClientRagdoll::FadeOut(void)
{
	if (m_bFadingOut == false)
	{
		return;
	}

	int iAlpha = GetRenderColor().a;
	int iFadeSpeed = (g_RagdollLVManager.IsLowViolence()) ? g_ragdoll_lvfadespeed.GetInt() : g_ragdoll_fadespeed.GetInt();

	iAlpha = MAX(iAlpha - (iFadeSpeed * gpGlobals->frametime), 0.f);

	SetRenderMode(kRenderTransAlpha);
	SetRenderColorA(iAlpha);

	if (iAlpha == 0)
	{
		m_bReleaseRagdoll = true;
	}
}

void C_CollegueClientRagdoll::SUB_Remove(void)
{
	m_bFadingOut = true;
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_CollegueClientRagdoll::ClientThink(void)
{
	if (m_bReleaseRagdoll == true)
	{
		DestroyBoneAttachments();
		Release();
		return;
	}

	if (g_debug_ragdoll_visualize.GetBool())
	{
		Vector vMins, vMaxs;

		Vector origin = m_pRagdoll->GetRagdollOrigin();
		m_pRagdoll->GetRagdollBounds(vMins, vMaxs);

		debugoverlay->AddBoxOverlay(origin, vMins, vMaxs, QAngle(0, 0, 0), 0, 255, 0, 16, 0);
	}

	HandleAnimatedFriction();

	FadeOut();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
float C_CollegueClientRagdoll::LastBoneChangedTime()
{
	// When did this last change?
	return m_pRagdoll ? m_pRagdoll->GetLastVPhysicsUpdateTime() : -FLT_MAX;
}


//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_CollegueClientRagdoll::SetupWeights(const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights)
{
	BaseClass::SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);

	if (m_iEyeAttachment > 0)
	{
		matrix3x4_t attToWorld;
		if (GetAttachment(m_iEyeAttachment, attToWorld))
		{
			Vector local, tmp;
			local.Init(1000.0f, 0.0f, 0.0f);
			VectorTransform(local, attToWorld, tmp);
			modelrender->SetViewTarget(GetModelPtr(), GetBody(), tmp);
		}
	}
}

void C_CollegueClientRagdoll::Release(void)
{
	for (int i = 0; i < ENT_EFFECT_MAX; i++)
	{
		C_BaseEntity* pChild = GetEffectEntity(i);

		if (pChild && pChild->IsMarkedForDeletion() == false)
		{
			pChild->Release();
		}
	}

	if (GetThinkHandle() != INVALID_THINK_HANDLE)
	{
		ClientThinkList()->RemoveThinkable(GetClientHandle());
	}
	ClientEntityList().RemoveEntity(GetClientHandle());

	if (CollisionProp()->GetPartitionHandle() != PARTITION_INVALID_HANDLE)
	{
		partition->Remove(PARTITION_CLIENT_SOLID_EDICTS | PARTITION_CLIENT_RESPONSIVE_EDICTS | PARTITION_CLIENT_NON_STATIC_EDICTS, CollisionProp()->GetPartitionHandle());
	}
	RemoveFromLeafSystem();

	BaseClass::Release();
}

void C_CollegueClientRagdoll::ProcessSceneEvents(bool bFlexEvents)
{
	BaseClass::ProcessSceneEvents(bFlexEvents);

	CStudioHdr* hdr = GetModelPtr();
	if (!hdr)
	{
		return;
	}

	if (bFlexEvents)
	{
		Assert(m_FlexControllers.Count() == m_FlexData.Count());
		for (int i = 0; i < m_FlexControllers.Count(); i++)
		{
			SetFlexWeight(m_FlexControllers[i], m_FlexData[i].flValue);
		}
	}
}

C_BaseAnimating* C_NPC_BaseColleague::CreateRagdollCopy()
{
	//Adrian: We now create a separate entity that becomes this entity's ragdoll.
	//That way the server side version of this entity can go away. 
	//Plus we can hook save/restore code to these ragdolls so they don't fall on restore anymore.
	C_CollegueClientRagdoll* pRagdoll = new C_CollegueClientRagdoll(false);
	if (pRagdoll == NULL)
		return NULL;

	TermRopes();

	const model_t* model = GetModel();
	const char* pModelName = modelinfo->GetModelName(model);

	if (pRagdoll->InitializeAsClientEntity(pModelName, RENDER_GROUP_OPAQUE_ENTITY) == false)
	{
		pRagdoll->Release();
		return NULL;
	}

	// move my current model instance to the ragdoll's so decals are preserved.
	SnatchModelInstance(pRagdoll);

	// We need to take these from the entity
	pRagdoll->SetAbsOrigin(GetAbsOrigin());
	pRagdoll->SetAbsAngles(GetAbsAngles());

	pRagdoll->IgniteRagdoll(this);
	pRagdoll->TransferDissolveFrom(this);
	pRagdoll->TransferElectricsFrom(this);
	MoveBoneAttachments(pRagdoll);
	pRagdoll->InitModelEffects();

	if (AddRagdollToFadeQueue() == true)
	{
		pRagdoll->m_bImportant = ImportantRagdoll();
		s_RagdollLRU.MoveToTopOfLRU(pRagdoll, pRagdoll->m_bImportant);
		pRagdoll->m_bFadeOut = true;
	}

	m_builtRagdoll = true;
	AddEffects(EF_NODRAW);

	if (IsEffectActive(EF_NOSHADOW))
	{
		pRagdoll->AddEffects(EF_NOSHADOW);
	}

	pRagdoll->m_nRenderFX = kRenderFxRagdoll;
	pRagdoll->SetRenderMode(GetRenderMode());
	pRagdoll->SetRenderColor(GetRenderColor().r, GetRenderColor().g, GetRenderColor().b, GetRenderColor().a);

	pRagdoll->m_nBody = m_nBody;
	pRagdoll->m_nSkin = GetSkin();
	pRagdoll->m_vecForce = m_vecForce;
	pRagdoll->m_nForceBone = m_nForceBone;
	pRagdoll->CopyWetnessFrom(this);
	pRagdoll->SetNextClientThink(CLIENT_THINK_ALWAYS);

	pRagdoll->m_FlexData.AddVectorToTail(m_FlexData);
	pRagdoll->m_FlexControllers.AddVectorToTail(m_FlexControllers);

	pRagdoll->SetModelName(AllocPooledString(pModelName));
	pRagdoll->SetModelScale(GetModelScale());
	return pRagdoll;
}
