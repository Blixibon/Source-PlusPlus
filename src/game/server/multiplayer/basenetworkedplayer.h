#include "cbase.h"
#include "multiplayer/multiplayer_animstate.h"
#include "multiplayer/basenetworkedplayer_shared.h"
#include "basemultiplayerplayer.h"

#define BASENETWORKEDPLAYER_H

class CBaseNetworkedPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CBaseNetworkedPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_PREDICTABLE();

	CBaseNetworkedPlayer();
	~CBaseNetworkedPlayer();

	virtual void Spawn();
	virtual void UpdateOnRemove();
	
	virtual bool		Event_Gibbed(const CTakeDamageInfo &info) { return false; }
	virtual bool BecomeRagdollOnClient(const Vector& force);

	// Death & Ragdolls.
	virtual void CreateRagdollEntity(void);
	void CreateRagdollEntity(bool bGib, bool bBurning);
	virtual void RemoveRagdollEntity();
	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual CStudioHdr *OnNewModel(void);
		
	// Create a predicted viewmodel
	virtual void CreateViewModel( int index );

	// Lag compensate when firing bullets
	virtual void FireBullets ( const FireBulletsInfo_t &info );

	// Implement CMultiPlayerAnimState
	virtual void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0);
	virtual void SetAnimation(PLAYER_ANIM playerAnim);
	virtual void PostThink();

protected:
	virtual CMultiPlayerAnimState* GetAnimState() { return nullptr; }
	const char* ragdoll_ent_name;

	CNetworkQAngle( m_angEyeAngles );
	CNetworkVar( bool, m_bSpawnInterpCounter );
	CNetworkHandle( CBaseEntity, m_hRagdoll );

	CNetworkVar(int, m_cycleLatch); // Network the cycle to clients periodically
	CountdownTimer m_cycleLatchTimer;
};

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();
	CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name ) {}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};