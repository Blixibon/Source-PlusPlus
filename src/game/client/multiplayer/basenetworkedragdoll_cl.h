#include "cbase.h"

class C_BaseNetworkedRagdoll : public C_BaseFlex
{
public:
	DECLARE_CLASS( C_BaseNetworkedRagdoll, C_BaseFlex);
	DECLARE_CLIENTCLASS();
	
	C_BaseNetworkedRagdoll() {}
	~C_BaseNetworkedRagdoll();

	virtual void OnDataChanged(DataUpdateType_t type);

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ClientThink(void);
	void StartFadeOut(float fDelay);
	void EndFadeOut();

	void ImpactTrace(trace_t* pTrace, int iDamageType, const char* pCustomImpactName);
	void UpdateOnRemove(void);

	bool IsRagdollVisible();
	virtual void SetupWeights(const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights);

private:

	C_BaseNetworkedRagdoll(const C_BaseNetworkedRagdoll&) {}

	void Interp_Copy(C_BaseAnimatingOverlay* pDestinationEntity);
	void CreateHL2MPRagdoll(void);
	void CreateTFGibs(void);

private:

	float m_fDeathTime;
	bool  m_bFadingOut;

	EHANDLE	m_hPlayer;
	CNetworkVector(m_vecRagdollVelocity);
	CNetworkVector(m_vecRagdollOrigin);
	bool  m_bGib;
	bool  m_bBurning;
};