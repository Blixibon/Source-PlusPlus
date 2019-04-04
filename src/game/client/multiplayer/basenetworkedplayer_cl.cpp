#include "cbase.h"
#include "basenetworkedplayer_cl.h"
#include "multiplayer/basenetworkedplayer_shared.h"
#include "basenetworkedragdoll_cl.h"
#include "prediction.h"

#undef CBaseNetworkedPlayer

// ***************** C_TEPlayerAnimEvent **********************

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO(m_nData) ),
END_RECV_TABLE()

void C_TEPlayerAnimEvent::PostDataUpdate( DataUpdateType_t updateType )
{
	C_BaseNetworkedPlayer *pPlayer = (C_BaseNetworkedPlayer*)m_hPlayer.Get();

	if ( pPlayer && !pPlayer->IsDormant() )
		pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get() , m_nData );
}

// ***************** C_BaseNetworkedPlayer **********************

BEGIN_RECV_TABLE_NOBASE( C_BaseNetworkedPlayer, DT_BaseNetworkedPlayerExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ), // RECVINFO_NAME redirects the received var to m_vecNetworkOrigin for interpolation purposes
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_BaseNetworkedPlayer, DT_BaseNetworkedPlayerNonLocalExclusive )
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin) ), // RECVINFO_NAME again
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_BaseNetworkedPlayer, DT_BaseNetworkedPlayer, CBaseNetworkedPlayer )
	RecvPropBool( RECVINFO( m_bSpawnInterpCounter ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropDataTable( "netplayer_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BaseNetworkedPlayerExclusive) ),
	RecvPropDataTable( "netplayer_nonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_BaseNetworkedPlayerNonLocalExclusive) ),
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( C_BaseNetworkedPlayer )
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

C_BaseNetworkedPlayer::C_BaseNetworkedPlayer() : m_iv_angEyeAngles( "C_BaseNetworkedPlayer::m_iv_angEyeAngles" )
{
	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );
	m_bSpawnInterpCounterCache,m_bSpawnInterpCounter = false;
	SetPredictionEligible(true);

	//MakeAnimState();
};

const QAngle& C_BaseNetworkedPlayer::EyeAngles()
{
	if( IsLocalPlayer() )
		return BaseClass::EyeAngles();
	else
		return m_angEyeAngles;
}
const QAngle& C_BaseNetworkedPlayer::GetRenderAngles()
{
	if ( IsRagdoll() || !GetAnimState())
		return vec3_angle;
	else
		return GetAnimState()->GetRenderAngles();
}

void C_BaseNetworkedPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() && prediction->InPrediction() && !prediction->IsFirstTimePredicted() )
			return;

	MDLCACHE_CRITICAL_SECTION();
	if (GetAnimState())
		GetAnimState()->DoAnimationEvent( event, nData );
}

void C_BaseNetworkedPlayer::UpdateClientSideAnimation()
{
	// Keep the model upright; pose params will handle pitch aiming.
	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	if (GetAnimState())
		GetAnimState()->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

void C_BaseNetworkedPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	// Did we just respawn?
	if ( m_bSpawnInterpCounter != m_bSpawnInterpCounterCache )
		Respawn();

	BaseClass::PostDataUpdate( updateType );
}

void C_BaseNetworkedPlayer::ReceiveMessage( int classID, bf_read &msg )
{
	int messageType = msg.ReadByte();

	switch( messageType )
	{
		case BECOME_RAGDOLL:
			
			break;
	}
}

C_BaseAnimating* C_BaseNetworkedPlayer::BecomeRagdollOnClient()
{
	return nullptr;
}

#include "view_scene.h" // for tone mapping reset

void C_BaseNetworkedPlayer::Respawn()
{
	// fix up interp
	MoveToLastReceivedPosition( true );
	ResetLatched();
	m_bSpawnInterpCounterCache = m_bSpawnInterpCounter;

	RemoveAllDecals();

	if (GetAnimState())
		GetAnimState()->ResetGestureSlots();
	
	// reset HDR
	if ( IsLocalPlayer() )
		ResetToneMapping(1.0);

	
}

IRagdoll* C_BaseNetworkedPlayer::GetRepresentativeRagdoll() const
{
	if (m_hRagdoll.Get())
	{
		C_BaseNetworkedRagdoll* pRagdoll = static_cast<C_BaseNetworkedRagdoll*>(m_hRagdoll.Get());
		if (!pRagdoll)
			return NULL;

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}