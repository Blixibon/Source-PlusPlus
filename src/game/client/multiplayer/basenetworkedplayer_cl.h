#include "cbase.h"
#include "multiplayer/multiplayer_animstate.h"
#include "c_basetempentity.h"
#include "props_shared.h"

class C_BaseNetworkedRagdoll;

#define CBaseNetworkedPlayer C_BaseNetworkedPlayer

class C_BaseNetworkedPlayer : public C_BasePlayer {
public:
	DECLARE_CLASS(C_BaseNetworkedPlayer, C_BasePlayer);
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	C_BaseNetworkedPlayer();

	virtual void Respawn(); // Spawn() is called once, this is called every time

	void ReceiveMessage(int classID, bf_read& msg);

	// Implement multiplayer ragdolls
	virtual C_BaseAnimating* BecomeRagdollOnClient();
	virtual IRagdoll* GetRepresentativeRagdoll() const;

	virtual bool ShouldReceiveProjectedTextures(int flags) { return true; }

	// Implement CMultiplayerAnimState
	virtual void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0);
	virtual void UpdateClientSideAnimation();
	virtual void PostDataUpdate(DataUpdateType_t updateType);
	virtual const QAngle& EyeAngles();
	virtual const QAngle& GetRenderAngles();

	virtual Vector GetObserverCamOrigin(void);
	virtual CStudioHdr *OnNewModel(void);

	// Gibs.
	void InitPlayerGibs(void);
	bool CreatePlayerGibs(const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning);
	CUtlVector<EHANDLE>		*GetSpawnedGibs(void) { return &m_hSpawnedGibs; }

	virtual void SetAnimation(PLAYER_ANIM playerAnim);

	static void RecvProxy_CycleLatch(const CRecvProxyData* pData, void* pStruct, void* pOut);

	virtual float GetServerIntendedCycle() { return m_flServerCycle; }
	virtual void SetServerIntendedCycle(float cycle) { m_flServerCycle = cycle; }

	virtual CMultiPlayerAnimState* GetAnimState() { return nullptr; }
protected:

	QAngle	m_angEyeAngles;
	bool	m_bSpawnInterpCounter;
	bool	m_bSpawnInterpCounterCache;
	CInterpolatedVar<QAngle>	m_iv_angEyeAngles;
	EHANDLE	m_hRagdoll;

	int m_cycleLatch; // The animation cycle goes out of sync very easily. Mostly from the player entering/exiting PVS. Server will frequently update us with a new one.
	float m_flServerCycle;

	// Gibs.
	CUtlVector<breakmodel_t>	m_aGibs;
	EHANDLE					m_hFirstGib;
	CUtlVector<EHANDLE>		m_hSpawnedGibs;

	friend class C_BaseNetworkedRagdoll;
};

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType );

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};