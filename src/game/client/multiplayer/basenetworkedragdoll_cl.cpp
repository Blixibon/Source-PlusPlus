#include "cbase.h"
#include "basenetworkedragdoll_cl.h"

ConVar cl_ragdoll_physics_enable("cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics.");
ConVar cl_ragdoll_fade_time("cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL);
ConVar cl_ragdoll_forcefade("cl_ragdoll_forcefade", "0", FCVAR_CLIENTDLL);
ConVar cl_ragdoll_pronecheck_distance("cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL);

IMPLEMENT_CLIENTCLASS_DT( C_BaseNetworkedRagdoll, DT_BaseNetworkedRagdoll, CBaseNetworkedRagdoll )
RecvPropVector(RECVINFO(m_vecRagdollOrigin)),
RecvPropEHandle(RECVINFO(m_hPlayer)),
RecvPropInt(RECVINFO(m_nModelIndex)),
RecvPropInt(RECVINFO(m_nForceBone)),
RecvPropVector(RECVINFO(m_vecForce)),
RecvPropVector(RECVINFO(m_vecRagdollVelocity)),
RecvPropBool(RECVINFO(m_bGib)),
RecvPropBool(RECVINFO(m_bBurning)),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( networked_ragdoll, C_BaseNetworkedRagdoll );

C_BaseNetworkedRagdoll::~C_BaseNetworkedRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

void C_BaseNetworkedRagdoll::Interp_Copy(C_BaseAnimatingOverlay* pSourceEntity)
{
	if (!pSourceEntity)
		return;

	VarMapping_t* pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t* pDest = GetVarMapping();

	// Find all the VarMapEntry_t's that represent the same variable.
	for (int i = 0; i < pDest->m_Entries.Count(); i++)
	{
		VarMapEntry_t* pDestEntry = &pDest->m_Entries[i];
		const char* pszName = pDestEntry->watcher->GetDebugName();
		for (int j = 0; j < pSrc->m_Entries.Count(); j++)
		{
			VarMapEntry_t* pSrcEntry = &pSrc->m_Entries[j];
			if (!Q_strcmp(pSrcEntry->watcher->GetDebugName(), pszName))
			{
				pDestEntry->watcher->Copy(pSrcEntry->watcher);
				break;
			}
		}
	}
}

void C_BaseNetworkedRagdoll::ImpactTrace(trace_t* pTrace, int iDamageType, const char* pCustomImpactName)
{
	IPhysicsObject* pPhysicsObject = VPhysicsGetObject();

	if (!pPhysicsObject)
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if (iDamageType == DMG_BLAST)
	{
		dir *= 4000;  // adjust impact strenght

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

		// Blood spray!
//		FX_CS_BloodSpray( hitpos, dir, 10 );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_BaseNetworkedRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector(-1, -1, -1);	//WorldAlignMins();
	Vector vMaxs = Vector(1, 1, 1);	//WorldAlignMaxs();

	Vector origin = GetAbsOrigin();

	if (!engine->IsBoxInViewCluster(vMins + origin, vMaxs + origin))
	{
		return false;
	}
	else if (engine->CullBox(vMins + origin, vMaxs + origin))
	{
		return false;
	}

	return true;
}

void C_BaseNetworkedRagdoll::ClientThink(void)
{
	SetNextClientThink(CLIENT_THINK_ALWAYS);

	if (m_bFadingOut == true)
	{
		int iAlpha = GetRenderColor().a;
		int iFadeSpeed = 600.0f;

		iAlpha = max(iAlpha - (iFadeSpeed * gpGlobals->frametime), .0f);

		SetRenderMode(kRenderTransAlpha);
		SetRenderColorA(iAlpha);

		if (iAlpha == 0)
		{
			EndFadeOut(); // remove clientside ragdoll
		}

		return;
	}

	// if the player is looking at us, delay the fade
	if (IsRagdollVisible())
	{
		if (cl_ragdoll_forcefade.GetBool())
		{
			m_bFadingOut = true;
			float flDelay = cl_ragdoll_fade_time.GetFloat() * 0.33f;
			m_fDeathTime = gpGlobals->curtime + flDelay;

			// If we were just fully healed, remove all decals
			RemoveAllDecals();
		}

		StartFadeOut(cl_ragdoll_fade_time.GetFloat() * 0.33f);
		return;
	}

	if (m_fDeathTime > gpGlobals->curtime)
		return;

	EndFadeOut(); // remove clientside ragdoll
}

void C_BaseNetworkedRagdoll::StartFadeOut(float fDelay)
{
	if (!cl_ragdoll_forcefade.GetBool())
	{
		m_fDeathTime = gpGlobals->curtime + fDelay;
	}
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}


void C_BaseNetworkedRagdoll::EndFadeOut()
{
	SetNextClientThink(CLIENT_THINK_NEVER);
	ClearRagdoll();
	SetRenderMode(kRenderNone);
	UpdateVisibility();
}

void C_BaseNetworkedRagdoll::CreateHL2MPRagdoll(void)
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_BasePlayer* pPlayer = dynamic_cast<C_BasePlayer*>(m_hPlayer.Get());

	if (pPlayer && !pPlayer->IsDormant())
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance(this);

		VarMapping_t* varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());
		if (bRemotePlayer)
		{
			Interp_Copy(pPlayer);

			SetAbsAngles(pPlayer->GetRenderAngles());
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence(pPlayer->GetSequence());
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin(m_vecRagdollOrigin);

			SetAbsAngles(pPlayer->GetRenderAngles());

			SetAbsVelocity(m_vecRagdollVelocity);

			int iSeq = pPlayer->GetSequence();
			if (iSeq == -1)
			{
				Assert(false);	// missing walk_lower?
				iSeq = 0;
			}

			SetSequence(iSeq);	// walk_lower, basic pose
			SetCycle(0.0);

			Interp_Reset(varMap);
		}

		m_nBody = pPlayer->GetBody();
		m_nSkin = pPlayer->GetSkin();
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin(m_vecRagdollOrigin);

		SetAbsOrigin(m_vecRagdollOrigin);
		SetAbsVelocity(m_vecRagdollVelocity);

		Interp_Reset(GetVarMapping());

	}

	SetModelIndex(m_nModelIndex);

	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if (pPlayer && !pPlayer->IsDormant())
	{
		pPlayer->GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);
	}
	else
	{
		GetRagdollInitBoneArrays(boneDelta0, boneDelta1, currentBones, boneDt);
	}

	InitAsClientRagdoll(boneDelta0, boneDelta1, currentBones, boneDt);

	if (m_bBurning)
	{
		//m_flBurnEffectStartTime = gpGlobals->curtime;
		//ParticleProp()->Create("burningplayer_corpse", PATTACH_ABSORIGIN_FOLLOW);
	}

	// Fade out the ragdoll in a while
	StartFadeOut(cl_ragdoll_fade_time.GetFloat());
	SetNextClientThink(gpGlobals->curtime + cl_ragdoll_fade_time.GetFloat() * 0.33f);
}


void C_BaseNetworkedRagdoll::OnDataChanged(DataUpdateType_t type)
{
	BaseClass::OnDataChanged(type);

	if (type == DATA_UPDATE_CREATED)
	{
		CreateHL2MPRagdoll();
	}
}

IRagdoll* C_BaseNetworkedRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_BaseNetworkedRagdoll::UpdateOnRemove(void)
{
	VPhysicsSetObject(NULL);

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_BaseNetworkedRagdoll::SetupWeights(const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights)
{
	// While we're dying, we want to mimic the facial animation of the player. Once they're dead, we just stay as we are.
	if ((m_hPlayer && m_hPlayer->IsAlive()) || !m_hPlayer)
	{
		BaseClass::SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);
	}
	else if (m_hPlayer)
	{
		m_hPlayer->SetupWeights(pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights);
	}

	CStudioHdr* hdr = GetModelPtr();
	if (!hdr)
		return;

	/*int nFlexDescCount = hdr->numflexdesc();
	if (nFlexDescCount)
	{
		Assert(!pFlexDelayedWeights);
		memset(pFlexWeights, 0, nFlexWeightCount * sizeof(float));
	}*/

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