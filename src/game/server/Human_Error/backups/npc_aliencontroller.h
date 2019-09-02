//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Alien Controllers from HL1 now in updated form
//
//=============================================================================//


#ifndef NPC_ALIENCONTROLLER
#define NPC_ALIENCONTROLLER

#include "cbase.h"
#include "npc_talker.h"
#include "ai_basenpc.h"
#include "IEffects.h"
#include "sprite.h"
#include "ai_basenpc_physicsflyer.h"

#define ALIENCONTROLLER_MAX_PHYSOBJ_MASS				490
#define ALIENCONTROLLER_MIN_PHYSOBJ_MASS				 30
#define ALIENCONTROLLER_PLAYER_MAX_SWAT_DIST		   1536
#define ALIENCONTROLLER_PHYSOBJ_PULLDIST			   1024
#define ALIENCONTROLLER_PHYSOBJ_MOVE_TO_DIST			900
#define ALIENCONTROLLER_SWAT_DELAY						5
#define ALIENCONTROLLER_FARTHEST_PHYSICS_OBJECT		   1024 //40.0*12.0 //40.0*12.0
#define ALIENCONTROLLER_PHYSICS_SEARCH_DEPTH			100

#define ALIENCONTROLLER_CATCH_DISTANCE					48
#define ALIENCONTROLLER_PULL_VELOCITY_MOD				0.1f
#define ALIENCONTROLLER_PULL_ANGULARIMP_MOD				0.8f
#define ALIENCONTROLLER_PULL_TO_GUN_VEL_MOD				2.0f



class CNPC_AlienController : public CAI_BaseNPC //CNPCSimpleTalker
{
DECLARE_CLASS( CNPC_AlienController, CAI_BaseNPC );

public:
	CNPC_AlienController();

	Class_T			Classify( void ) { return( CLASS_ALIENCONTROLLER ); }


	void			IdleSound( void ) { }
	void			DeathSound( const CTakeDamageInfo &info ) { }
	void			AlertSound( void ) { }
	void			AttackSound( void ) { }
	void			PainSound( const CTakeDamageInfo &info ) {}

	void			Precache(void);

	void			Spawn(void);
	void			Activate();

	void			PrescheduleThink( void );

	virtual bool	CanBecomeServerRagdoll( void ) { return true;	}
	virtual float	GetReactionDelay( CBaseEntity *pEnemy ) { return 0.0; }

	// CAI_BaseNPC:
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );

	virtual float	MaxYawSpeed( void ) { return 40.0f; }

	//virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual void	HandleAnimEvent( animevent_t *pEvent );
	virtual int		GetSoundInterests( void );
	virtual int		SelectSchedule( void );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );

	virtual	Activity NPC_TranslateActivity( Activity baseAct );
	virtual	int		TranslateSchedule( int scheduleType );

	virtual bool	OverrideMove(float flInterval);
	virtual	bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	bool			Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect );

	int				RangeAttack1Conditions( float flDot, float flDist );
	int				MeleeAttack1Conditions( float flDot, float flDist );

	void			Event_Killed( const CTakeDamageInfo &info );

	void			NPCThink(void);	//for physobject pull

	
	//Controller custom

	void			ShootFireBall();
	void			CreateFireGlow();

	void			CombineBallCheck();

	Vector			GetPreferedAttackPoint(CBaseEntity *moveTarget);
	void			ControllerFly(float flInterval, Vector vMoveTargetPos, Vector vTargetPos );

	// Pulling and throwing physics objects

	int				GetSwatActivity( void );
	bool			FindNearestPhysicsObject( int iMaxMass );
	float			DistToPhysicsEnt( void );


	void			GatherConditions( void );

	void			ThrowObject( void );
	void			PullObject( bool bMantain );

	float			m_fLastTurnSpeed;

public:

	CSprite*		m_pFireGlow;

private:

	int				m_iFireBallAttachment;
	int				m_iFireBallFadeIn;
	int				m_fFireBallFadeInTime;

	EHANDLE			m_hFireBallTarget;

	int				m_iLastOffsetAngle;
	float			m_fLastOffsetSwithcTime;

	float			m_fPreferedHeight;

	float			m_flNextAttackTime;
	int				m_iNumberOfAttacks;

	EHANDLE			m_hPhysicsEnt;
//	EHANDLE			m_hObstructor;
	float			m_flNextSwat;
	float			m_flNextSwatScan;

	float			m_flNextCombineBallScan;
	EHANDLE			m_hCombineBall;

	bool			m_bHasObject;
	int				m_iPhysGunAttachment;

	bool			m_bIsFlying;
	bool			m_bIsLanding;
	float			m_bNextLandingTime;

	bool			CheckLanding();
	void			Land();
	void			Claw();

protected:

	int				m_iContainerMoveType;


private:

	
	DEFINE_CUSTOM_AI;

	
	// Custom interrupt conditions
	enum
	{
		COND_ALIENCONTROLLER_FLY_BLOCKED  = BaseClass::NEXT_CONDITION,	
		COND_ALIENCONTROLLER_CAN_PHYS_ATTACK,
		COND_ALIENCONTROLLER_SHOULD_LAND,
	};

	

	// Custom schedules
	enum
	{
		SCHED_ALIENCONTROLLER_PULL_PHYSOBJ = BaseClass::NEXT_SCHEDULE,
		SCHED_ALIENCONTROLLER_MOVE_TO_PHYSOBJ,
		SCHED_ALIENCONTROLLER_THROW_PHYSOBJ,
		SCHED_ALIENCONTROLLER_SHOOT_FIREBALL,
		SCHED_ALIENCONTROLLER_FIREBALL_EXTINGUISH,
		SCHED_ALIENCONTROLLER_LAND,
	};

	
	// Custom tasks
	enum
	{
		TASK_ALIENCONTROLLER_PULL_PHYSOBJ = BaseClass::NEXT_TASK,
		TASK_ALIENCONTROLLER_GET_PATH_TO_PHYSOBJ,
		TASK_ALIENCONTROLLER_THROW_PHYSOBJ,
		TASK_ALIENCONTROLLER_FIREBALL_EXTINGUISH,
		TASK_ALIENCONTROLLER_LAND,
	};
	

	DECLARE_DATADESC();
};


#endif //NPC_ALIENCONTROLLER