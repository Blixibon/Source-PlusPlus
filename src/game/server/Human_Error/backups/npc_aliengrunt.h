//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	AlienGrunts from HL1 now in updated form
//			by Au-heppa
//
//=============================================================================//


#ifndef NPC_ALIENGRUNT
#define NPC_ALIENGRUNT

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_basenpc.h"
#include "ai_behavior.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_talker.h"


extern int AE_AGRUNT_ATTACK_RIGHT;
extern int AE_AGRUNT_ATTACK_LEFT;
extern int AE_AGRUNT_STEP_LEFT;
extern int AE_AGRUNT_STEP_RIGHT;
extern int AE_AGRUNT_ATTACK_SCREAM;
extern int AE_AGRUNT_POUND;



#define ALIENGRUNT_MAX_PHYSOBJ_MASS				120
#define ALIENGRUNT_MIN_PHYSOBJ_MASS				 30
#define ALIENGRUNT_PLAYER_MAX_SWAT_DIST				1000
#define ALIENGRUNT_PHYSOBJ_SWATDIST				80
#define ALIENGRUNT_PHYSOBJ_MOVE_TO_DIST			48
#define ALIENGRUNT_SWAT_DELAY					2
#define ALIENGRUNT_FARTHEST_PHYSICS_OBJECT		40.0*12.0
#define ALIENGRUNT_PHYSICS_SEARCH_DEPTH			100

//=========================================================
//=========================================================
class CNPC_AlienGrunt : public CAI_PlayerAlly
{
	DECLARE_CLASS( CNPC_AlienGrunt, CAI_PlayerAlly );

public:

	void			Spawn( void );
	void			Precache( void );
	int				GetSoundInterests( void );
	Class_T			Classify( void );

	void			Claw( int iAttachment );
	void			SwatItem();

	int				RangeAttack1Conditions( float flDot, float flDist );
	int				MeleeAttack1Conditions( float flDot, float flDist );
	

	//Vector		GetShootEnemyDir( const Vector &shootOrigin, bool bNoisy = true );
	
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual float	GetReactionDelay( CBaseEntity *pEnemy ) { return 0.0; }

	virtual int		SelectSchedule ( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	void			BuildScheduleTestBits( void );

	virtual int		TranslateSchedule( int scheduleType );
	virtual	Activity NPC_TranslateActivity( Activity baseAct );

	Activity		GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	void			PlayFlinchGesture( void );

	//bool			IsValidEnemy( CBaseEntity *pEnemy );
//	bool			IsLeading( void ) { return ( GetRunningBehavior() == &m_LeadBehavior && m_LeadBehavior.HasGoal() ); }

	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );

	float			MaxYawSpeed( void );


	bool			ShouldGoToIdleState( void );

	void			HandleAnimEvent( animevent_t *pEvent );

	void			DeathSound( const CTakeDamageInfo &info ) ;
	void			PainSound( const CTakeDamageInfo &info );
	void			AlertSound( void );
	void			AttackSound( void );
	void			IdleSound( void );
	void			HideSound( void );

	void			PrescheduleThink(void);

	bool			CreateBehaviors( void );

	void			InputAssault( inputdata_t &inputdata );

	//Vector			FindNodePositionForBees(Vector startPos, Vector enemyPos);


	// Swatting physics objects
	int				GetSwatActivity( void );
	bool			FindNearestPhysicsObject( int iMaxMass );
	float			DistToPhysicsEnt( void );

	bool			OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

	void			GatherConditions( void );

private: 

	//Item Swatting, stolen from the BaseZombie
	EHANDLE			m_hPhysicsEnt;
	EHANDLE			m_hObstructor;
	float			m_flNextSwat;
	float			m_flNextSwatScan;


	void			UpdateBeehiveGunAim();
	bool			m_bShouldAim;

	void			UpdateHead();

	enum
	{
		SCHED_ALIENGRUNT_STAND = BaseClass::NEXT_SCHEDULE,
		SCHED_ALIENGRUNT_SHOOTBEES,
		SCHED_ALIENGRUNT_MELEE_ATTACK,
		SCHED_ALIENGRUNT_MOVE_SWATITEM,
		SCHED_ALIENGRUNT_SWATITEM,
		SCHED_ALIENGRUNT_ATTACKITEM,

	};

	//=========================================================
	// ALIENGRUNT Tasks 
	//=========================================================
	enum 
	{
		TASK_ALIENGRUNT_GET_PATH_TO_PHYSOBJ = BaseClass::NEXT_TASK,
		TASK_ALIENGRUNT_SWAT_ITEM,
	};

	enum 
	{
		COND_ALIENGRUNT_CAN_SWAT_ATTACK = BaseClass::NEXT_CONDITION,
		COND_ALIENGRUNT_LOCAL_MELEE_OBSTRUCTION,
	};

	void			ShootBees();

	int				m_iLeftHandAttachment;
	int				m_iRightHandAttachment;

	float			m_flNextNPCThink;

	float			m_HideSoundTime;

	CAI_AssaultBehavior	 m_AssaultBehavior;
	//CAI_LeadBehavior	 m_LeadBehavior;

protected:

	//float m_flNextMoanSound;

public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};

#endif // NPC_ALIENGRUNT