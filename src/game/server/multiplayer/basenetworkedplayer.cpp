#include "cbase.h"
#include "basenetworkedplayer.h"
#include "predicted_viewmodel.h"
#include "ilagcompensationmanager.h"
#include "basenetworkedragdoll.h"
#include "studio.h"

#define CYCLELATCH_UPDATE_INTERVAL	0.2f

//* **************** CTEPlayerAnimEvent* *********************

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEPlayerAnimEvent, DT_TEPlayerAnimEvent)
SendPropEHandle(SENDINFO(m_hPlayer)),
SendPropInt(SENDINFO(m_iEvent), Q_log2(PLAYERANIMEVENT_COUNT) + 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_nData), ANIMATION_SEQUENCE_BITS),
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer* pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );
	if (gpGlobals->maxClients > 1 && event < PLAYERANIMEVENT_DIE)
		filter.RemoveRecipient(pPlayer);

	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

//* *************** CBaseNetworkedPlayer *******************



//-----------------------------------------------------------------------------
// Purpose: Filters updates to a variable so that only non-local players see
// the changes.  This is so we can send a low-res origin to non-local players
// while sending a hi-res one to the local player.
// Input  : *pVarData - 
//			*pOut - 
//			objectID - 
//-----------------------------------------------------------------------------

void* SendProxy_SendNonLocalDataTable(const SendProp* pProp, const void* pStruct, const void* pVarData, CSendProxyRecipients* pRecipients, int objectID)
{
	pRecipients->SetAllRecipients();
	pRecipients->ClearRecipient(objectID - 1);
	return (void*)pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER(SendProxy_SendNonLocalDataTable);

extern void SendProxy_Origin( const SendProp* pProp, const void* pStruct, const void* pData, DVariant* pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CBaseNetworkedPlayer, DT_BaseNetworkedPlayerExclusive )
	// send a hi-res origin to the local player for use in prediction
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CBaseNetworkedPlayer, DT_BaseNetworkedPlayerNonLocalExclusive )
	SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
	SendPropFloat( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 8, SPROP_CHANGES_OFTEN, -90.0f, 90.0f ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 10, SPROP_CHANGES_OFTEN ),
	// Only need to latch cycle for other players
	// If you increase the number of bits networked, make sure to also modify the code below and in the client class.
	SendPropInt(SENDINFO(m_cycleLatch), 4, SPROP_UNSIGNED),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CBaseNetworkedPlayer, DT_BaseNetworkedPlayer )
	// These aren't needed either because we use client-side animation, or because they are being moved to the local/non-local table.
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),	
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
	SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),
	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),
	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
	
	// New props
	SendPropBool( SENDINFO( m_bSpawnInterpCounter) ),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),

	// Data that only gets sent to the local player.
	SendPropDataTable( "netplayer_localdata", 0, &REFERENCE_SEND_TABLE(DT_BaseNetworkedPlayerExclusive), SendProxy_SendLocalDataTable ),
	// Data that gets sent to all other players
	SendPropDataTable( "netplayer_nonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_BaseNetworkedPlayerNonLocalExclusive), SendProxy_SendNonLocalDataTable ),
END_NETWORK_TABLE()


//* ****************** FUNCTIONS* *********************


CBaseNetworkedPlayer::CBaseNetworkedPlayer() {
	UseClientSideAnimation();
	SetPredictionEligible(true);
	m_bSpawnInterpCounter = false;
	m_angEyeAngles.Init();
	ragdoll_ent_name = "networked_ragdoll";

	m_cycleLatch = 0;
	m_cycleLatchTimer.Invalidate();
	
	//MakeAnimState();
}
CBaseNetworkedPlayer::~CBaseNetworkedPlayer() {
	/*if (m_pPlayerAnimState)
		m_pPlayerAnimState->Release();	*/
}

void CBaseNetworkedPlayer::UpdateOnRemove()
{
	RemoveRagdollEntity();
	BaseClass::UpdateOnRemove();
}

void CBaseNetworkedPlayer::Spawn()
{
	BaseClass::Spawn();

	m_cycleLatchTimer.Start(CYCLELATCH_UPDATE_INTERVAL);

	// used to not interp players when they spawn
	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;

	if (GetAnimState())
		GetAnimState()->ClearAnimationState();
	
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	RemoveRagdollEntity();
}

void CBaseNetworkedPlayer::CreateViewModel( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel(index) )
		return;

	CPredictedViewModel* vm = (CPredictedViewModel*)CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

void CBaseNetworkedPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if (GetAnimState())
		GetAnimState()->DoAnimationEvent( event, nData ); // execute on server
	TE_PlayerAnimEvent( this, event, nData );	// transmit to clients
}

void CBaseNetworkedPlayer::PostThink()
{
	BaseClass::PostThink();

	// Keep the model upright; pose params will handle pitch aiming.
	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles(angles);

	m_angEyeAngles = EyeAngles();

	if (GetAnimState())
		GetAnimState()->Update(m_angEyeAngles[YAW], m_angEyeAngles[PITCH]);

	if (IsAlive() && m_cycleLatchTimer.IsElapsed())
	{
		m_cycleLatchTimer.Start(CYCLELATCH_UPDATE_INTERVAL);
		// Compress the cycle into 4 bits. Can represent 0.0625 in steps which is enough.
		m_cycleLatch.GetForModify() = 16 * GetCycle();
	}
}

void CBaseNetworkedPlayer::FireBullets ( const FireBulletsInfo_t &info )
{
	QAngle angles;
	VectorAngles(info.m_vecDirShooting, angles);
	lagcompensation->StartLagCompensation( this, LAG_COMPENSATE_HITBOXES_ALONG_RAY, info.m_vecSrc, angles, info.m_flDistance );
		BaseClass::FireBullets(info);
	lagcompensation->FinishLagCompensation( this );
}

void CBaseNetworkedPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	bool bBurning = ((info.GetDamageType() & (DMG_BURN | DMG_BLAST | DMG_PLASMA | DMG_ENERGYBEAM)) != 0);
	bool bGib = ShouldGib(info);

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	CreateRagdollEntity(bGib, bBurning);

	// ...and employ a minor hack to stop CBaseCombatCharacter creating its own
	const_cast<CTakeDamageInfo*>(&info)->AddDamageType(DMG_REMOVENORAGDOLL);

	BaseClass::Event_Killed( info );

	if (info.GetDamageType() & DMG_DISSOLVE)
	{
		if (m_hRagdoll)
		{
			m_hRagdoll->GetBaseAnimating()->Dissolve(NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL);
		}
	}

	//RemoveEffects(EF_NODRAW);	// still draw player body
}

CStudioHdr * CBaseNetworkedPlayer::OnNewModel(void)
{
	CStudioHdr *pHdr = BaseClass::OnNewModel();
	if (pHdr)
	{
		if (GetAnimState())
			GetAnimState()->OnNewModel();

		/*int i;
		for (i = 0; i < pHdr->GetNumPoseParameters(); i++)
		{
			SetPoseParameter(pHdr, i, 0.0);
		}*/
	}

	return pHdr;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseNetworkedPlayer::CreateRagdollEntity(void)
{
	CreateRagdollEntity(false, false);
}

void CBaseNetworkedPlayer::CreateRagdollEntity(bool bGib, bool bBurning)
{
	RemoveRagdollEntity();

	// If we already have a ragdoll, don't make another one.
	CBaseNetworkedRagdoll* pRagdoll = dynamic_cast<CBaseNetworkedRagdoll*>(m_hRagdoll.Get());

	if (!pRagdoll)
	{
		// create a new one
		pRagdoll = dynamic_cast<CBaseNetworkedRagdoll*>(CreateEntityByName(ragdoll_ent_name));
	}

	if (pRagdoll)
	{
		pRagdoll->m_hPlayer = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetAbsVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = m_vecForce;
		pRagdoll->m_bGib = bGib;
		pRagdoll->m_bBurning = bBurning;
		pRagdoll->SetAbsOrigin(GetAbsOrigin());
	}

	//AddEffects(EF_NODRAW | EF_NOSHADOW);

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

void CBaseNetworkedPlayer::RemoveRagdollEntity()
{
	if (m_hRagdoll)
	{
		UTIL_RemoveImmediate(m_hRagdoll);
		m_hRagdoll = NULL;
	}
}

bool CBaseNetworkedPlayer::BecomeRagdollOnClient(const Vector& force)
{
	return true;
}