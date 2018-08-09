#ifndef NPC_POISONZOMBIE_H
#define NPC_POISONZOMBIE_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_BaseZombie.h"

//-----------------------------------------------------------------------------
// The maximum number of headcrabs we can have riding on our back.
// NOTE: If you change this value you must also change the lookup table in Spawn!
//-----------------------------------------------------------------------------
#define MAX_CRABS	3	

class CNPC_PoisonZombie : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_CLASS(CNPC_PoisonZombie, CAI_BlendingHost<CNPC_BaseZombie>);

public:

	//
	// CBaseZombie implemenation.
	//
	virtual Vector HeadTarget(const Vector &posSrc);
	bool ShouldBecomeTorso(const CTakeDamageInfo &info, float flDamageThreshold);
	virtual bool IsChopped(const CTakeDamageInfo &info) { return false; }

	//
	// CAI_BaseNPC implementation.
	//
	virtual float MaxYawSpeed(void);

	virtual int RangeAttack1Conditions(float flDot, float flDist);
	virtual int RangeAttack2Conditions(float flDot, float flDist);

	virtual float GetClawAttackRange() const { return 70; }

	virtual void PrescheduleThink(void);
	virtual void BuildScheduleTestBits(void);
	virtual int SelectSchedule(void);
	virtual int SelectFailSchedule(int nFailedSchedule, int nFailedTask, AI_TaskFailureCode_t eTaskFailCode);
	virtual int TranslateSchedule(int scheduleType);

	virtual bool ShouldPlayIdleSound(void);

	// Thinking, including core thinking, movement, animation
	virtual void		NPCThink(void);

	//
	// CBaseAnimating implementation.
	//
	virtual void HandleAnimEvent(animevent_t *pEvent);

	//
	// CBaseEntity implementation.
	//
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetZombieModel(void);

	virtual Class_T Classify(void);
	virtual void Event_Killed(const CTakeDamageInfo &info);
	virtual int OnTakeDamage_Alive(const CTakeDamageInfo &inputInfo);

	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	void PainSound(const CTakeDamageInfo &info);
	void AlertSound(void);
	void IdleSound(void);
	void AttackSound(void);
	void AttackHitSound(void);
	void AttackMissSound(void);
	void FootstepSound(bool fRightFoot);
	void FootscuffSound(bool fRightFoot) {};

	virtual void StopLoopingSounds(void);

	// pCrab will be set to remove itself next frame.
	bool		AddCrabToNest(CBlackHeadcrab *pCrab);
	bool		CanAddToNest(void);

protected:

	virtual void MoanSound(envelopePoint_t *pEnvelope, int iEnvelopeSize);
	virtual bool MustCloseToAttack(void);

	virtual const char *GetMoanSound(int nSoundIndex);
	virtual const char *GetLegsModel(void);
	virtual const char *GetTorsoModel(void);
	virtual const char *GetHeadcrabClassname(void);
	virtual const char *GetHeadcrabModel(void);

private:

	void BreatheOffShort(void);

	void EnableCrab(int nCrab, bool bEnable, float flHealth = -1.0f);
	int RandomThrowCrab(void);
	int RandomAddCrab(void);
	void EvacuateNest(bool bExplosion, float flDamage, CBaseEntity *pAttacker);
	void HealCrabs(float flDelta);

	CSoundPatch *m_pFastBreathSound;
	CSoundPatch *m_pSlowBreathSound;

	int m_nCrabCount;				// How many headcrabs we have on our back.
	bool m_bCrabs[MAX_CRABS];		// Which crabs in particular are on our back.
	float m_flCrabHealth[MAX_CRABS];	// Health of each crab.
	float m_flNextCrabThrowTime;	// The next time we are allowed to throw a headcrab.

	float m_flNextPainSoundTime;

	bool m_bNearEnemy;

	// NOT serialized:
	int m_nThrowCrab;				// The crab we are about to throw.
};

#endif // NPC_POISONZOMBIE_H
