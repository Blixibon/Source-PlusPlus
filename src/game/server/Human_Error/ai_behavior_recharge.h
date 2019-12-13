//=================== Half-Life 2: Short Stories Mod 2009 =====================//
//
// Purpose:	Recharge behavior
//
//=============================================================================//

#ifndef AI_BEHAVIOR_RECHARGE_H
#define AI_BEHAVIOR_RECHARGE_H
#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "ai_behavior.h"
#include "ai_goalentity.h"
#include "ai_moveshoot.h"
#include "ai_utils.h"
#include "ai_basenpc.h"

#define HLSS_RECHARGE_POINT_TOLERANCE (3.0*12.0)
#define HLSS_RECHARGE_POINT_THINK_DELAY 1.0f
#define HLSS_RECHARGE_POINT_THINK_DELAY_CHANGE 0.2f

class IAmmoCrate
{
public:
	virtual bool	IsCrateOpen() = 0;
	virtual void	OpenForNPC(CAI_BaseNPC*) = 0;
	virtual int		GetCrateAmmoType() = 0;
};

//#define HLSS_DEBUG_RECHARGE_BEHAVIOR

//=============================================================================
//=============================================================================
class CRechargePoint : public CPointEntity
{
	DECLARE_CLASS( CRechargePoint, CPointEntity );

public:
	CRechargePoint()
	{
		m_hLockedBy.Set(NULL);
		m_hLockedBy = NULL;
	//	m_bActive = false;
		m_flMaxDistance = 2048;
	}

	#define SF_HLSS_BEHAVIOR_RECHARGE_START_ACTIVE		( 1 << 0  )
	#define SF_HLSS_BEHAVIOR_RECHARGE_DONT_SPEAK		( 1 << 1  )
	#define SF_HLSS_BEHAVIOR_RECHARGE_NEEDS_AMMO		( 1 << 2  )

	void Spawn( void );
	void Activate();

	bool Lock( CAI_BaseNPC *pLocker );
	bool Unlock( CAI_BaseNPC *pUnlocker );
	bool IsLocked( void ) { return (m_hLockedBy.Get() != NULL); }

	COutputEvent	m_OnRecharge;
	virtual void 	InputActivate( inputdata_t &inputdata );
	virtual void 	InputDeactivate( inputdata_t &inputdata );

	bool			FindRechagerNPC( void );
	void			FindRechargeThink();

	CBaseEntity* GetAmmoCrateEnt() { return m_hAmmoCrate.Get(); }
	IAmmoCrate* GetIAmmoCrate() { return dynamic_cast<IAmmoCrate*>(m_hAmmoCrate.Get()); }
	int			GetAmmoType() { return m_iAmmoIndex; }

	DECLARE_DATADESC();

	string_t		m_RechargeSequenceName;

	//bool			m_bActive;

private:
	AIHANDLE		m_hLockedBy;
	float		m_flMaxDistance;

	enum SearchType_t	
	{
		ST_ENTNAME,
		ST_CLASSNAME,
		ST_UNIQUECOPS,
		ST_TEAMNUM,
	};

	string_t				m_iszActor;
	SearchType_t			m_SearchType;

	string_t				m_iszAmmoCrate;
	EHANDLE					m_hAmmoCrate;
	int						m_iAmmoIndex;
};

enum
{
	RECHARGE_SENTENCE_STARTING_RECHARGE = SENTENCE_BASE_BEHAVIOR_INDEX,
};

//=============================================================================
//=============================================================================
class CAI_RechargeBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_RechargeBehavior, CAI_SimpleBehavior );

public:
	CAI_RechargeBehavior();
	
	virtual const char *GetName() {	return "Recharge"; }

	virtual void OnRestore();

	//bool CanRunAScriptedNPCInteraction( bool bForced );

	virtual bool 	CanSelectSchedule();
	//virtual void	BeginScheduleSelection();
	//virtual void	EndScheduleSelection();
	
	bool			HasReachedRechargePoint() { return m_bHasReachedRechargePoint; }
	bool			IsMovingToAcquireAmmo() { return IsCurSchedule(SCHED_RECHARGE_ACQUIRE_AMMO, false); }
	bool			NeedsToAcquireAmmo();

	void			UpdateRechargePointDistance();

	//void GatherConditions( void );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	//void BuildScheduleTestBits();
	int TranslateSchedule( int scheduleType );
	void OnStartSchedule( int scheduleType );
	void ClearSchedule( const char *szReason );

	void InitializeBehavior();

	bool IsAllowedToDivert( void );
	bool IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint );
	float GetMaxTacticalLateralMovement( void );

	void UpdateOnRemove();

	enum
	{
		SCHED_MOVE_TO_RECHARGE_POINT = BaseClass::NEXT_SCHEDULE,		// Try to get out of the player's way
		SCHED_RECHARGE_FAILED_TO_MOVE,
		SCHED_RECHARGE_ACQUIRE_AMMO,
		NEXT_SCHEDULE,

		TASK_GET_PATH_TO_RECHARGE_POINT = BaseClass::NEXT_TASK,
		TASK_FACE_RECHARGE_POINT,
		TASK_RECHARGE,
		TASK_RECHARGE_DEFER_SCHEDULE_SELECTION,
		TASK_GET_PATH_TO_AMMO_CRATE,
		TASK_GET_AMMO,
		NEXT_TASK,


		COND_RECHARGE_CHANGED_POINT = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,

	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

private:
	//void			OnScheduleChange();
	virtual int		SelectSchedule();

public:
	CHandle<CRechargePoint> m_hRechargePoint;

	bool HasRechargePoint( void ) { return (m_hRechargePoint.Get() != NULL); }
	void			UnlockRechargePoint( void );
	void			SetRechargePoint( CRechargePoint *pRechargePoint );

	void			SetCanRecharge(bool bCanRecharge) { m_bCanRecharge = bCanRecharge; }
	bool			CanRecharge() { return m_bCanRecharge; }

	float			m_flNextTimeChange;

private:

	void			Recharge();

	bool			m_bCanRecharge;
	bool			m_bHasReachedRechargePoint;

	float			m_flTimeDeferScheduleSelection;

	//---------------------------------
	
	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_RECHARHE_H