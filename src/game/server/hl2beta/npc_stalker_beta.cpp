//========= Copyright � 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "soundent.h"
#include "game.h"
#include "beam_shared.h"
#include "Sprite.h"
#include "NPCEvent.h"
#include "npc_stalker_beta.h"
#include "AI_Hull.h"
#include "AI_Default.h"
#include "AI_Node.h"
#include "AI_Network.h"
#include "AI_Hint.h"
#include "AI_Link.h"
#include "AI_Waypoint.h"
#include "AI_Navigator.h"
#include "AI_SquadSlot.h"
#include "AI_Memory.h"
#include "AI_TacticalServices.h"
#include "ai_moveprobe.h"
#include "npc_Talker.h"
#include "activitylist.h"
#include "BitString.h"
#include "decals.h"
#include "player.h"
#include "entitylist.h"
#include "ndebugoverlay.h"
#include "AI_Interactions.h"
#include "Animation.h"
#include "scriptedtarget.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "world.h"

//#define		STALKER_DEBUG
#define	MIN_STALKER_FIRE_RANGE		64
#define	MAX_STALKER_FIRE_RANGE		2048
#define	STALKER_LASER_ATTACHMENT	1
#define	STALKER_TRIGGER_DIST		200	// Enemy dist. that wakes up the stalker
#define	STALKER_SENTENCE_VOLUME		(float)0.35
#define STALKER_LASER_DURATION		5
#define STALKER_LASER_RECHARGE		2
enum StalkerBeamPower_e
{
	STALKER_BEAM_LOW,
	STALKER_BEAM_MED,
	STALKER_BEAM_HIGH,
};

//Animation events
#define STALKER_AE_MELEE_HIT			1

extern ConVar	sk_stalker_health/*( "sk_stalker_health","0")*/;
extern ConVar	sk_stalker_melee_dmg/*( "sk_stalker_melee_dmg","0")*/;

extern void		SpawnBlood(Vector vecSpot, const Vector& vAttackDir, int bloodColor, float flDamage);

LINK_ENTITY_TO_CLASS( npc_stalker_beta, CNPC_BetaStalker );


//=========================================================
// Private activities.
//=========================================================
static int ACT_STALKER_WORK = 0;

//=========================================================
// Stalker schedules
//=========================================================
enum
{
	SCHED_STALKER_BETA_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_STALKER_BETA_RANGE_ATTACK,
};

//=========================================================
// Stalker Tasks
//=========================================================
enum 
{
	TASK_STALKER_BETA_ZIGZAG = LAST_SHARED_TASK,
};

// -----------------------------------------------
//	> Squad slots
// -----------------------------------------------
enum SquadSlot_T
{
	SQUAD_SLOT_BSTALK_CHASE_1	= LAST_SHARED_SQUADSLOT,
	SQUAD_SLOT_BSTALK_CHASE_2,
};

//-----------------------------------------------------------------------------

BEGIN_DATADESC(CNPC_BetaStalker)

DEFINE_KEYFIELD(m_eBeamPower, FIELD_INTEGER, "BeamPower"),
DEFINE_FIELD(m_vLaserDir, FIELD_VECTOR),
DEFINE_FIELD(m_vLaserTargetPos, FIELD_POSITION_VECTOR),
DEFINE_FIELD(m_fBeamEndTime, FIELD_FLOAT),
DEFINE_FIELD(m_fBeamRechargeTime, FIELD_FLOAT),
DEFINE_FIELD(m_fNextDamageTime, FIELD_FLOAT),
DEFINE_FIELD(m_bPlayingHitWall, FIELD_FLOAT),
DEFINE_FIELD(m_bPlayingHitFlesh, FIELD_FLOAT),
DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
DEFINE_FIELD(m_pLightGlow, FIELD_CLASSPTR),
DEFINE_FIELD(m_flNextNPCThink, FIELD_FLOAT),
DEFINE_FIELD(m_pScriptedTarget, FIELD_CLASSPTR),
DEFINE_FIELD(m_vLaserCurPos, FIELD_POSITION_VECTOR),
DEFINE_FIELD(m_flNextAttackSoundTime, FIELD_TIME),
DEFINE_FIELD(m_flNextBreatheSoundTime, FIELD_TIME),
DEFINE_FIELD(m_flNextScrambleSoundTime, FIELD_TIME),
DEFINE_FIELD(m_nextSmokeTime, FIELD_TIME),

// Function Pointers
DEFINE_THINKFUNC(StalkerThink),

END_DATADESC();



//------------------------------------------------------------------------------
// Purpose : Starts doing scripted burn to a location
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_BetaStalker::SetScriptedTarget( CScriptedTarget *pScriptedTarget )
{
	if (pScriptedTarget)
	{
		// ---------------------------------------
		//	If I don't already have a burn target
		// ---------------------------------------
		if (m_pScriptedTarget == NULL)
		{
			// Wake the guy up
			SetCondition(COND_PROVOKED);

			// If first burn target make it my current position
			m_pScriptedTarget	= pScriptedTarget;
			m_vLaserCurPos		= m_pScriptedTarget->m_vLastPosition;
		}
		else
		{
			m_pScriptedTarget	= pScriptedTarget;
		}
		m_vLaserTargetPos	= m_pScriptedTarget->GetAbsOrigin();
	}
	else
	{
		// Break him out of burn schedule
		SetCondition(COND_ENEMY_DEAD);

		m_pScriptedTarget	= NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int	CNPC_BetaStalker::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// --------------------------------------------
	//	Don't take a lot of damage from Vortigaunt
	// --------------------------------------------
	if (info.GetAttacker()->Classify() == CLASS_VORTIGAUNT)
	{
		info.ScaleDamage( 0.25 );
	}


	int ret = BaseClass::OnTakeDamage_Alive( info );

	// If player shot me make sure I'm mad at him even if I wasn't earlier
	if ( (info.GetAttacker()->GetFlags() & FL_CLIENT) )
	{
		AddClassRelationship( CLASS_PLAYER, D_HT, 0 );
	}
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CNPC_BetaStalker::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 160;
		break;
	case ACT_RUN:
	case ACT_RUN_HURT:
		return 280;
		break;
	default:
		return 160;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : int
//-----------------------------------------------------------------------------
Class_T CNPC_BetaStalker::Classify( void )
{
	return CLASS_STALKER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::Spawn( void )
{
	Precache( );

	SetModel( "models/stalker_old.mdl" );
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= DONT_BLEED;
	m_iHealth			= sk_stalker_health.GetFloat();
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1);
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1);

	m_flNextAttackSoundTime		= 0;
	m_flNextBreatheSoundTime	= 0;
	m_flNextScrambleSoundTime	= 0;
	m_nextSmokeTime = 0;
	m_bPlayingHitWall			= false;
	m_bPlayingHitFlesh			= false;

	m_fBeamEndTime				= 0;
	m_fBeamRechargeTime			= 0;
	m_fNextDamageTime			= 0;

	m_pScriptedTarget			= NULL;
	NPCInit();
	// Allow stalker to shoot beam over great distances
	m_flDistTooFar	= MAX_STALKER_FIRE_RANGE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::Precache( void )
{
	PrecacheModel("models/stalker_old.mdl");
	engine->PrecacheModel("sprites/laser.vmt");	

	PrecacheScriptSound("NPC_Stalker.AmbientLaserStart");
	PrecacheScriptSound("NPC_Stalker.BurnFlesh");
	PrecacheScriptSound("NPC_Stalker.BurnWall");
	PrecacheScriptSound("NPC_Stalker.FootstepLeft");
	PrecacheScriptSound("NPC_Stalker.FootstepRight");
	PrecacheScriptSound("NPC_Stalker.Hit");
	PrecacheScriptSound("NPC_Stalker.Ambient01");
	PrecacheScriptSound("NPC_Stalker.Scream");
	PrecacheScriptSound("NPC_Stalker.Pain");
	PrecacheScriptSound("NPC_Stalker.Die");

	engine->PrecacheModel("sprites/redglow1.vmt");
	engine->PrecacheModel("sprites/orangeglow1.vmt");
	engine->PrecacheModel("sprites/yellowglow1.vmt");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::IdleSound ( void )
{
	if (gpGlobals->curtime > m_flNextBreatheSoundTime)
	{
//		SENTENCEG_PlayRndSz( edict(), "STALKER_BREATHING", STALKER_SENTENCE_VOLUME, ATTN_NORM, 0, 100);
		m_flNextBreatheSoundTime	= gpGlobals->curtime + 2;
	}
}

void CNPC_BetaStalker::OnScheduleChange()
{
	KillAttackBeam();

	BaseClass::OnScheduleChange();
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pInflictor - 
//			pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::Event_Killed( const CTakeDamageInfo &info )
{
	KillAttackBeam();
	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::DeathSound( void )
{ 
	// Always play this sound
	SENTENCEG_PlayRndSz( edict(), "STALKER_DIE", STALKER_SENTENCE_VOLUME, SNDLVL_NORM, 0, 100);
	m_flNextScrambleSoundTime	= gpGlobals->curtime + 1.5;
	m_flNextBreatheSoundTime	= gpGlobals->curtime + 1.5;
	m_flNextAttackSoundTime		= gpGlobals->curtime + 1.5;
};

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::PainSound( void )
{ 
	// Always play this sound
	SENTENCEG_PlayRndSz( edict(), "STALKER_PAIN", STALKER_SENTENCE_VOLUME, SNDLVL_NORM, 0, 100);
	m_flNextScrambleSoundTime	= gpGlobals->curtime + 1.5;
	m_flNextBreatheSoundTime	= gpGlobals->curtime + 1.5;
	m_flNextAttackSoundTime		= gpGlobals->curtime + 1.5;
};

//-----------------------------------------------------------------------------
// Purpose: Translates squad slot positions into schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
#if 0
// @TODO (toml 07-18-03): this function is never called. Presumably what it is trying to do still needs to be done...
int CNPC_BetaStalker::GetSlotSchedule(int slotID)
{
	switch (slotID)
	{

		case SQUAD_SLOT_BSTALK_CHASE_1:
		case SQUAD_SLOT_BSTALK_CHASE_2:
			return SCHED_STALKER_BETA_CHASE_ENEMY;
			break;
	}
	return SCHED_NONE;
}
#endif


void CNPC_BetaStalker::UpdateAttackBeam( void )
{
	CBaseEntity *pEnemy = GetEnemy();
	// If not burning at a target 
	if (pEnemy)
	{
		if (gpGlobals->curtime > m_fBeamEndTime)
		{
			TaskComplete();
		}
		else 
		{
			Vector enemyLKP = GetEnemyLKP();
			m_vLaserTargetPos = enemyLKP + pEnemy->GetViewOffset();

			// Face my enemy
			GetMotor()->SetIdealYawToTargetAndUpdate( enemyLKP );

			// ---------------------------------------------
			//	Get beam end point
			// ---------------------------------------------
			Vector vecSrc = LaserStartPosition(GetAbsOrigin());
			Vector targetDir = m_vLaserTargetPos - vecSrc;
			VectorNormalize(targetDir);
			// --------------------------------------------------------
			//	If beam position and laser dir are way off, end attack
			// --------------------------------------------------------
			if ( DotProduct(targetDir,m_vLaserDir) < 0.8 )
			{
				TaskComplete();
				return;
			}

			trace_t tr;
			AI_TraceLine( vecSrc, vecSrc + m_vLaserDir * MAX_STALKER_FIRE_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
			// ---------------------------------------------
			//  If beam not long enough, stop attacking
			// ---------------------------------------------
			if (tr.fraction == 1.0)
			{
				TaskComplete();
				return;
			}


			CSoundEnt::InsertSound(SOUND_DANGER, tr.endpos, 60, 0.025, this);
		}
	}
	// Face my burn target
	else if (m_pScriptedTarget)
	{
		GetMotor()->SetIdealYawToTargetAndUpdate( m_pScriptedTarget->GetAbsOrigin() );
		// ---------------------------------------------
		//  If can't see burn target, stop attacking
		// ---------------------------------------------
		trace_t tr;
		AI_TraceLine( LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		CBaseEntity *pEntity = tr.m_pEnt;
		if (tr.fraction != 1.0 && pEntity!=m_pScriptedTarget)		
		{	
			SetDefaultFailSchedule( SCHED_ESTABLISH_LINE_OF_FIRE ); 
			TaskFail("No LOS");
		}
		else
		{
			CSoundEnt::InsertSound(SOUND_DANGER, tr.endpos, 60, 0.025, this);
		}
	}
	else
	{
		TaskFail(FAIL_NO_ENEMY);
	}
}

//=========================================================
// start task
//=========================================================
void CNPC_BetaStalker::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANNOUNCE_ATTACK:
	{
		// If enemy isn't facing me and I haven't attacked in a while
		// annouce my attack before I start wailing away
		CBaseCombatCharacter *pBCC = GetEnemyCombatCharacterPointer();

		if	(pBCC && (!pBCC->FInViewCone ( this )) &&
			 (gpGlobals->curtime - m_flLastAttackTime > 1.0) )
		{
				m_flLastAttackTime = gpGlobals->curtime;

				// Always play this sound
				SENTENCEG_PlayRndSz( edict(), "STALKER_ANNOUNCE", STALKER_SENTENCE_VOLUME, SNDLVL_NORM, 0, 100);
				m_flNextScrambleSoundTime = gpGlobals->curtime + 2;
				m_flNextBreatheSoundTime = gpGlobals->curtime + 2;

				// Wait two seconds
				m_flWaitFinished = gpGlobals->curtime + 2.0;
				SetActivity(ACT_IDLE);
		}
		break;
	}
	case TASK_STALKER_BETA_ZIGZAG:
			break;
	case TASK_RANGE_ATTACK1:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if (pEnemy)
			{
				m_vLaserTargetPos = GetEnemyLKP() + pEnemy->GetViewOffset();

				// Never hit target on first try
				Vector missPos = m_vLaserTargetPos;
				missPos.x += 80*random->RandomInt(-1,1);
				missPos.y += 80*random->RandomInt(-1,1);

				// ----------------------------------------------------------------------
				// If target is facing me and not running towards me shoot below his feet
				// so he can see the laser coming
				// ----------------------------------------------------------------------
				CBaseCombatCharacter *pBCC = ToBaseCombatCharacter(pEnemy);
				if (pBCC)
				{
					Vector targetToMe = (pBCC->GetAbsOrigin() - GetAbsOrigin());
					Vector vBCCFacing = pBCC->BodyDirection2D( );
					if ((DotProduct(vBCCFacing,targetToMe) < 0) &&
						(pBCC->GetAbsVelocity().Length() < 50))
					{
						missPos.z -= 150;
					}
					// --------------------------------------------------------
					// If facing away or running towards laser,
					// shoot above target's head 
					// --------------------------------------------------------
					else
					{
						missPos.z += 60;
					}
				}
				m_vLaserDir = missPos - LaserStartPosition(GetAbsOrigin());
				VectorNormalize(m_vLaserDir);	
			}
			// --------------------------------------
			//  Do I have a target position to burn
			// --------------------------------------
			else if (m_pScriptedTarget)
			{
				trace_t tr;
				AI_TraceLine(LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
				CBaseEntity *pEntity = tr.m_pEnt;
				if (tr.fraction != 1.0 && pEntity != m_pScriptedTarget)
				{	
					SetDefaultFailSchedule( SCHED_ESTABLISH_LINE_OF_FIRE ); 
					TaskFail("No LOS");
					return;
				}
				m_vLaserDir	= m_vLaserCurPos - LaserStartPosition(GetAbsOrigin());
				VectorNormalize(m_vLaserDir);	
			}
			else
			{
				TaskFail(FAIL_NO_ENEMY);
				return;
			}

			StartAttackBeam();
			SetActivity(ACT_RANGE_ATTACK1);
			break;
		}
	case TASK_GET_PATH_TO_ENEMY_LOS:
		{
			if ( GetEnemy() != NULL )
			{
				BaseClass:: StartTask( pTask );
				return;
			}

			Vector posLos;

			if (GetTacticalServices()->FindLos(m_vLaserCurPos, m_vLaserCurPos, MIN_STALKER_FIRE_RANGE, MAX_STALKER_FIRE_RANGE, 1.0, &posLos))
			{
				AI_NavGoal_t goal( posLos, ACT_RUN, AIN_HULL_TOLERANCE );
				GetNavigator()->SetGoal( goal );
			}
			else
			{
				TaskFail(FAIL_NO_SHOOT);
			}
			break;
		}
	case TASK_FACE_ENEMY:
		{
			if ( GetEnemy() != NULL )
			{
				BaseClass:: StartTask( pTask );
				return;
			}
			GetMotor()->SetIdealYawToTarget( m_vLaserCurPos );
			break;
		}
	default: 
		BaseClass:: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNPC_BetaStalker::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANNOUNCE_ATTACK:
	{
		// Stop waiting if enemy facing me or lost enemy
		CBaseCombatCharacter* pBCC = GetEnemyCombatCharacterPointer();
		if	(!pBCC || pBCC->FInViewCone( this ))
		{
			TaskComplete();
		}

		if ( gpGlobals->curtime >= m_flWaitFinished )
		{
			TaskComplete();
		}
		break;
	}

	case TASK_STALKER_BETA_ZIGZAG :
		{

			if (GetNavigator()->GetGoalType() == GOALTYPE_NONE)
			{
				TaskComplete();
				GetNavigator()->ClearGoal();		// Stop moving
			}
			else if (!GetNavigator()->IsGoalActive())
			{
				SetIdealActivity( GetStoppedActivity() );
			}
			else if (ValidateNavGoal())
			{
				SetIdealActivity( GetNavigator()->GetMovementActivity() );
				AddZigZagToPath();
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
		UpdateAttackBeam();
		if ( !TaskIsRunning() || HasCondition( COND_TASK_FAILED ))
		{
			KillAttackBeam();
		}
		break;

	case TASK_FACE_ENEMY:
		{
			if ( GetEnemy() != NULL )
			{
				BaseClass:: RunTask( pTask );
				return;
			}
			GetMotor()->SetIdealYawToTargetAndUpdate( m_vLaserCurPos );

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BetaStalker::SelectSchedule( void )
{
	switch	( m_NPCState )
	{
		case NPC_STATE_IDLE:
		{
			if ( HasCondition ( COND_HEAR_DANGER ) ||
				 HasCondition ( COND_HEAR_COMBAT ) ||
				 HasCondition ( COND_HEAR_WORLD  ) ||
				 HasCondition ( COND_HEAR_PLAYER ) )
			{
				return SCHED_ALERT_FACE;
			}
			// Get out of someone's way
			else if ( HasCondition ( COND_GIVE_WAY ) )
			{
				return SCHED_MOVE_AWAY;
			}
			else if (m_pScriptedTarget!=NULL)
			{
				// Check if I have a line of sight 
				trace_t tr;
				AI_TraceLine(LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
				CBaseEntity *pEntity = tr.m_pEnt;
				if (tr.fraction != 1.0 && pEntity != m_pScriptedTarget)
				{	
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
			break;
		}
		case NPC_STATE_ALERT:
		{
			if ( HasCondition( COND_ENEMY_DEAD ) && SelectWeightedSequence( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
			{
				// Scan around for new enemies
				return SCHED_ALERT_SCAN;
			}

			if ( HasCondition(COND_LIGHT_DAMAGE) ||
				 HasCondition(COND_HEAVY_DAMAGE) )
			{
				return SCHED_TAKE_COVER_FROM_ORIGIN;
			}

			else if ( HasCondition ( COND_HEAR_DANGER ) ||
					  HasCondition ( COND_HEAR_PLAYER ) ||
					  HasCondition ( COND_HEAR_WORLD  ) ||
					  HasCondition ( COND_HEAR_COMBAT ) )
			{
				return SCHED_ALERT_FACE;
			}
			else if (m_pScriptedTarget!=NULL)
			{
				// Check if I have a line of sight
				trace_t tr;
				AI_TraceLine(LaserStartPosition(GetAbsOrigin()), m_vLaserCurPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);	
				CBaseEntity *pEntity = tr.m_pEnt;
				if (tr.fraction != 1.0 && pEntity != m_pScriptedTarget)
				{	
					return SCHED_ESTABLISH_LINE_OF_FIRE;
				}
				else
				{
					return SCHED_RANGE_ATTACK1;
				}
			}
			else
			{
				return SCHED_ALERT_STAND;
			}
			break;
		}
		case NPC_STATE_COMBAT:
		{
			// -----------
			// dead enemy
			// -----------
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			// ----------------------
			// GIVE WAY
			// ----------------------
			if ( HasCondition ( COND_GIVE_WAY ) )
			{
				return SCHED_MOVE_AWAY;
			}

			// -------------------------------------------
			// If I can't range attack and not ready to beam
			// -------------------------------------------
			if ( !HasCondition ( COND_CAN_RANGE_ATTACK1 ) && gpGlobals->curtime < m_fBeamRechargeTime)
			{
				return SCHED_TAKE_COVER_FROM_ENEMY;
			}

			// -------------------------------------------
			// If I can't range attack and not ready to beam
			// -------------------------------------------
			if ( HasCondition( COND_ENEMY_TOO_FAR )				||
				 HasCondition( COND_TOO_FAR_TO_ATTACK )			||	 
				 HasCondition( COND_TOO_CLOSE_TO_ATTACK )		||
				 HasCondition( COND_ENEMY_OCCLUDED )			|| 
				 HasCondition( COND_WEAPON_SIGHT_OCCLUDED )		|| 
				 HasCondition( COND_WEAPON_BLOCKED_BY_FRIEND )	)
			{
				return SCHED_ESTABLISH_LINE_OF_FIRE;
			}

			// --------------------------------------------------------------
			//  If I can't see my enemy, I'm not facing him as I have a
			//  line of sight
			// --------------------------------------------------------------
			if ( !HasCondition( COND_SEE_ENEMY ))
			{
				return SCHED_COMBAT_FACE;
			}

			// --------------------------------------------------------------
			// If there aren't any attack slots, go to work go dormant unless
			// my enemy just hurt
			// --------------------------------------------------------------
			if (!HasCondition(COND_LOST_ENEMY) &&
				 (CBaseEntity*)GetEnemy()		  &&
				 (OccupyStrategySlotRange( SQUAD_SLOT_ATTACK1, SQUAD_SLOT_ATTACK2 ) ||
				 HasCondition(COND_LIGHT_DAMAGE)	|| 
				 HasCondition(COND_HEAVY_DAMAGE) ))
			{
				// --------------------------------------------------------------
				// If I can't attack go for it!
				// --------------------------------------------------------------
				if (HasCondition(COND_CAN_RANGE_ATTACK1))
				{
					if (gpGlobals->curtime > m_flNextAttackSoundTime)
					{
//						SENTENCEG_PlayRndSz( edict(), "STALKER_ATTACK", STALKER_SENTENCE_VOLUME, ATTN_NORM, 0, 100);
						m_flNextScrambleSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextBreatheSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextAttackSoundTime		= gpGlobals->curtime + 0.5;	
					}
					return SCHED_RANGE_ATTACK1;
				}
				else if (HasCondition(COND_CAN_MELEE_ATTACK1))
				{
					if (gpGlobals->curtime > m_flNextAttackSoundTime)
					{
//						SENTENCEG_PlayRndSz( edict(), "STALKER_ATTACK", STALKER_SENTENCE_VOLUME, ATTN_NORM, 0, 100);
						m_flNextScrambleSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextBreatheSoundTime	= gpGlobals->curtime + 0.5;
						m_flNextAttackSoundTime		= gpGlobals->curtime + 0.5;	
					}
					return SCHED_MELEE_ATTACK1;
				}
			}

			// --------------------------------------
			// Otherwise go for cover
			// --------------------------------------
			return SCHED_TAKE_COVER_FROM_ENEMY;
			break;
		}
	}

	// no special cases here, call the base class
	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BetaStalker::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_RANGE_ATTACK1:
		{
			return SCHED_STALKER_BETA_RANGE_ATTACK;
		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{	
			if (GetEnemy() != NULL)
			{
				return SCHED_TAKE_COVER_FROM_ENEMY;
			}
			else
			{
				return SCHED_IDLE_STAND;
			}
			break;
		}
	case SCHED_FAIL_TAKE_COVER:
		{
			return SCHED_RUN_RANDOM;
			break;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//------------------------------------------------------------------------------
// Purpose : Returns new target burn position.  Calculates from last burn
//			 position, target direction and current burn speed
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_BetaStalker::ScriptedBurnPosition(void)
{
	// Make sure I don't overshoot
	Vector	vTargetDir  = m_vLaserTargetPos - m_vLaserCurPos;
	float	fTargetDist	= VectorNormalize(vTargetDir);
	float	fBurnSpeed	= m_pScriptedTarget->MoveSpeed();

	if (fTargetDist < fBurnSpeed)
	{
		return m_vLaserTargetPos;
	}
	else
	{
		return m_vLaserCurPos + fBurnSpeed*vTargetDir;
	}
}

//------------------------------------------------------------------------------
// Purpose : Returns position of laser for any given position of the staler
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector CNPC_BetaStalker::LaserStartPosition(Vector vStalkerPos)
{
	// Get attachment position
	Vector vAttachPos;
	QAngle vAttachAngles;
	GetAttachment(STALKER_LASER_ATTACHMENT,vAttachPos,vAttachAngles);

	// Now convert to vStalkerPos
	vAttachPos = vAttachPos - GetAbsOrigin() + vStalkerPos;
	return vAttachPos;
}

//------------------------------------------------------------------------------
// Purpose : Calculate position of beam
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_BetaStalker::CalcBeamPosition(void)
{
	Vector targetDir = m_vLaserTargetPos - LaserStartPosition(GetAbsOrigin());
	VectorNormalize(targetDir);

	// -----------------------------------------------
	//  If I'm burning towards a burn target
	// -----------------------------------------------
	if (GetEnemy() == NULL && m_pScriptedTarget != NULL)
	{
		//  Move towards burn target at linear rate
		m_vLaserDir		  = ScriptedBurnPosition() - LaserStartPosition(GetAbsOrigin());
		VectorNormalize(m_vLaserDir);

		// If I've reached by burn target I'm done
		float fDist = (m_vLaserDir - targetDir).Length();
		if ( fDist < 0.01)
		{
			// Update scripted target
			SetScriptedTarget( m_pScriptedTarget->NextScriptedTarget());
		}
	}
	// ---------------------------------------
	//  Otherwise if burning towards an enemy
	// ---------------------------------------
	else
	{
		// ---------------------------------------
		//  Integrate towards target position
		// ---------------------------------------
		float	iRate = 0.95;
		m_vLaserDir.x = (iRate * m_vLaserDir.x + (1-iRate) * targetDir.x);
		m_vLaserDir.y = (iRate * m_vLaserDir.y + (1-iRate) * targetDir.y);
		m_vLaserDir.z = (iRate * m_vLaserDir.z + (1-iRate) * targetDir.z);
		VectorNormalize( m_vLaserDir );

		// -----------------------------------------
		// Add time-coherent noise to the position
		// Must be scaled with distance 
		// -----------------------------------------
		float fTargetDist = (GetAbsOrigin() - m_vLaserTargetPos).Length();
		float noiseScale		= atan(0.2/fTargetDist);
		float m_fNoiseModX		= 5;
		float m_fNoiseModY		= 5;
		float m_fNoiseModZ		= 5;

		m_vLaserDir.x += 5*noiseScale*sin(m_fNoiseModX * gpGlobals->curtime + m_fNoiseModX);
		m_vLaserDir.y += 5*noiseScale*sin(m_fNoiseModY * gpGlobals->curtime + m_fNoiseModY);
		m_vLaserDir.z += 5*noiseScale*sin(m_fNoiseModZ * gpGlobals->curtime + m_fNoiseModZ);
	}
}


void CNPC_BetaStalker::StartAttackBeam( void )
{
	if ( m_fBeamEndTime > gpGlobals->curtime || m_fBeamRechargeTime > gpGlobals->curtime )
	{
		// UNDONE: Debug this and fix!?!?!
//		Msg( "Stalker Beam restart!\n" );
	}
	// ---------------------------------------------
	//  If I don't have a beam yet, create one
	// ---------------------------------------------
	// UNDONE: Why would I ever have a beam already?!?!?!
	if (!m_pBeam)
	{
		Vector vecSrc = LaserStartPosition(GetAbsOrigin());
		trace_t tr;
		AI_TraceLine ( vecSrc, vecSrc + m_vLaserDir * MAX_STALKER_FIRE_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
		if ( tr.fraction >= 1.0 )
		{
			// too far
			TaskComplete();
			return;
		}

		m_pBeam = CBeam::BeamCreate( "sprites/laser.vmt", 2.0 );
		m_pBeam->PointEntInit( tr.endpos, this );
		m_pBeam->SetEndAttachment( STALKER_LASER_ATTACHMENT );  
		m_pBeam->SetBrightness( 255 );
		m_pBeam->SetNoise( 0 );

		switch (m_eBeamPower)
		{
			case STALKER_BEAM_LOW:
				m_pBeam->SetColor( 255, 0, 0 );
				m_pLightGlow = CSprite::SpriteCreate( "sprites/redglow1.vmt", GetAbsOrigin(), FALSE );
				break;
			case STALKER_BEAM_MED:
				m_pBeam->SetColor( 255, 50, 0 );
				m_pLightGlow = CSprite::SpriteCreate( "sprites/orangeglow1.vmt", GetAbsOrigin(), FALSE );
				break;
			case STALKER_BEAM_HIGH:
				m_pBeam->SetColor( 255, 150, 0 );
				m_pLightGlow = CSprite::SpriteCreate( "sprites/yellowglow1.vmt", GetAbsOrigin(), FALSE );
				break;
		}

		// ----------------------------
		// Light myself in a red glow
		// ----------------------------
		m_pLightGlow->SetTransparency( kRenderGlow, 255, 200, 200, 0, kRenderFxNoDissipation );
		m_pLightGlow->SetAttachment( this, 1 );
		m_pLightGlow->SetBrightness( 255 );
		m_pLightGlow->SetScale( 0.65 );

		CBasePlayer* pEnemyPlayer = ToBasePlayer(GetEnemy());
		CPASAttenuationFilter filter(this, "NPC_Stalker.AmbientLaserStart");
		// --------------------------------------------------------
		// Play start up sound - client should always hear this!
		// --------------------------------------------------------
		if (pEnemyPlayer)
		{
			CSingleUserRecipientFilter filter2(pEnemyPlayer);
			EmitSound(filter2, SOUND_FROM_WORLD, "NPC_Stalker.AmbientLaserStart", &pEnemyPlayer->GetAbsOrigin());

			filter.RemoveRecipient(pEnemyPlayer);
		}

		EmitSound(filter, SOUND_FROM_WORLD, "NPC_Stalker.AmbientLaserStart", &GetAbsOrigin());
	}

	SetThink(&CNPC_BetaStalker::StalkerThink);

	m_flNextNPCThink = GetNextThink();
	SetNextThink( gpGlobals->curtime );
	m_fBeamEndTime = gpGlobals->curtime + STALKER_LASER_DURATION;
}

//------------------------------------------------------------------------------
// Purpose : Update beam more often then regular NPC think so it doesn't
//			 move so jumpily over the ground
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_BetaStalker::StalkerThink(void)
{
	DrawAttackBeam();
	if (gpGlobals->curtime > m_flNextNPCThink)
	{
		NPCThink();
		m_flNextNPCThink = GetNextThink();
	}

	if ( m_pBeam )
	{
		SetNextThink( gpGlobals->curtime );
		
		// sanity check?!
		const Task_t *pTask = GetTask();
		if ( !pTask || pTask->iTask != TASK_RANGE_ATTACK1 || !TaskIsRunning() )
		{
			KillAttackBeam();
		}
	}
}

void CNPC_BetaStalker::DoSmokeEffect( const Vector &position )
{
	if ( gpGlobals->curtime > m_nextSmokeTime )
	{
		m_nextSmokeTime = gpGlobals->curtime + 0.5;
		UTIL_Smoke(position, random->RandomInt(5, 10), 10);
	}
}

//------------------------------------------------------------------------------
// Purpose : Draw attack beam and do damage / decals
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_BetaStalker::DrawAttackBeam(void)
{
	if (!m_pBeam)
		return;

	// ---------------------------------------------
	//	Get beam end point
	// ---------------------------------------------
	Vector vecSrc = LaserStartPosition(GetAbsOrigin());
	trace_t tr;
	AI_TraceLine( vecSrc, vecSrc + m_vLaserDir * MAX_STALKER_FIRE_RANGE, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	// If I have a BurnTarget
	if (GetEnemy() == NULL && m_pScriptedTarget != NULL)
	{	
		// ------------------------------------------
		//  Update scripts last position
		// ------------------------------------------
		m_vLaserCurPos = ScriptedBurnPosition();
		m_pScriptedTarget->m_vLastPosition = m_vLaserCurPos;
	}

	CalcBeamPosition();

	bool bInWater = (UTIL_PointContents ( tr.endpos ) & MASK_WATER)?true:false;
	// ---------------------------------------------
	//	Update the beam position
	// ---------------------------------------------
	m_pBeam->SetStartPos( tr.endpos );
	m_pBeam->RelinkBeam();

	// --------------------------------------------
	//  Play burn sounds
	// --------------------------------------------
	CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( tr.m_pEnt );
	if (pBCC)
	{
		if (gpGlobals->curtime > m_fNextDamageTime)
		{
			ClearMultiDamage();

			float damage = 0.0;
			switch (m_eBeamPower)
			{
				case STALKER_BEAM_LOW:
					damage = 1;
					break;
				case STALKER_BEAM_MED:
					damage = 3;
					break;
				case STALKER_BEAM_HIGH:
					damage = 10;
					break;
			}

			CTakeDamageInfo info( this, this, damage, DMG_SHOCK );
			CalculateMeleeDamageForce( &info, m_vLaserDir, tr.endpos );
			pBCC->DispatchTraceAttack( info, m_vLaserDir, &tr );
			ApplyMultiDamage();
			m_fNextDamageTime = gpGlobals->curtime + 0.1;
		}
		if (pBCC->Classify()!=CLASS_BULLSEYE)
		{
			if (!m_bPlayingHitFlesh)
			{
				CPASAttenuationFilter filter( m_pBeam,"NPC_Stalker.BurnFlesh" );
				filter.MakeReliable();

				EmitSound( filter, m_pBeam->entindex(),"NPC_Stalker.BurnFlesh" );
				m_bPlayingHitFlesh = true;
			}
			if (m_bPlayingHitWall)
			{
				StopSound( m_pBeam->entindex(), "NPC_Stalker.BurnWall" );
				m_bPlayingHitWall = false;
			}

			tr.endpos.z -= 24.0f;
			if (!bInWater)
			{
				DoSmokeEffect(tr.endpos + tr.plane.normal * 8);
			}
		}
	}
	
	if (!pBCC || pBCC->Classify()==CLASS_BULLSEYE)
	{
		if (!m_bPlayingHitWall)
		{
			CPASAttenuationFilter filter( m_pBeam, "NPC_Stalker.BurnWall" );
			filter.MakeReliable();

			EmitSound( filter, m_pBeam->entindex(), "NPC_Stalker.BurnWall" );
			m_bPlayingHitWall = true;
		}
		if (m_bPlayingHitFlesh)
		{
			StopSound(m_pBeam->entindex(), "NPC_Stalker.BurnFlesh" );
			m_bPlayingHitFlesh = false;
		}

		UTIL_DecalTrace( &tr, "RedGlowFade");
		UTIL_DecalTrace( &tr, "FadingScorch" );
		
		tr.endpos.z -= 24.0f;
		if (!bInWater)
		{
			DoSmokeEffect(tr.endpos + tr.plane.normal * 8);
		}
	}

	if (bInWater)
	{
		UTIL_Bubbles(tr.endpos-Vector(3,3,3),tr.endpos+Vector(3,3,3),10);
	}

	/*
	CBroadcastRecipientFilter filter;
	TE_DynamicLight( filter, 0.0, EyePosition(), 255, 0, 0, 5, 0.2, 0 );
	*/
}

//------------------------------------------------------------------------------
// Purpose : Draw attack beam and do damage / decals
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_BetaStalker::KillAttackBeam(void)
{
	if ( !m_pBeam )
		return;

	// Kill sound
	StopSound(m_pBeam->entindex(), "NPC_Stalker.BurnWall" );
	StopSound(m_pBeam->entindex(), "NPC_Stalker.BurnFlesh" );

	UTIL_Remove( m_pLightGlow );
	UTIL_Remove( m_pBeam);
	m_pBeam = NULL;
	m_bPlayingHitWall = false;
	m_bPlayingHitFlesh = false;

	SetThink(&CAI_BaseNPC::CallNPCThink);
	if ( m_flNextNPCThink > gpGlobals->curtime )
	{
		SetNextThink( gpGlobals->curtime + m_flNextNPCThink );
	}

	// Beam has to recharge
	m_fBeamRechargeTime = gpGlobals->curtime + STALKER_LASER_RECHARGE;
	ClearCondition( COND_CAN_RANGE_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: Override so can handle LOS to m_pScriptedTarget
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_BetaStalker::InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions )
{
	// --------------------
	// Check for occlusion
	// --------------------
	// Base class version assumes innate weapon position is at eye level
	Vector barrelPos = LaserStartPosition(ownerPos);
	trace_t tr;
	AI_TraceLine( barrelPos, targetPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	if ( tr.fraction == 1.0 )
	{
		return true;
	}

	CBaseEntity *pBE = tr.m_pEnt;
	CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pBE );
	if (pBE == GetEnemy()				|| 
		pBE == m_pScriptedTarget	)
	{
		return true;
	}
	else if (pBCC) 
	{
		if (IRelationType( pBCC ) == D_HT)
		{
			return true;
		}
		else if (bSetConditions)
		{
			SetCondition(COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}
	else if (bSetConditions)
	{
		SetCondition(COND_WEAPON_SIGHT_OCCLUDED);
		SetEnemyOccluder(pBE);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BetaStalker::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > MIN_STALKER_FIRE_RANGE)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: For innate range attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_BetaStalker::RangeAttack1Conditions( float flDot, float flDist )
{
	if (gpGlobals->curtime < m_fBeamRechargeTime)
	{
		return COND_NONE;
	}
	if (flDist <= MIN_STALKER_FIRE_RANGE)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > MAX_STALKER_FIRE_RANGE)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}
	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: Catch stalker specific messages
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_BetaStalker::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case NPC_EVENT_LEFTFOOT:
			{
				EmitSound( "NPC_Stalker.FootstepLeft" );
			}
			break;
		case NPC_EVENT_RIGHTFOOT:
			{
				EmitSound( "NPC_Stalker.FootstepRight" );
			}
			break;

		case STALKER_AE_MELEE_HIT:
		{
			CBaseEntity *pHurt;

			

			pHurt = CheckTraceHullAttack( 32, Vector(-16,-16,-16), Vector(16,16,16), sk_stalker_melee_dmg.GetFloat(), DMG_SLASH );

			if ( pHurt )
			{
				if ( pHurt->GetFlags() & (FL_NPC|FL_CLIENT) )
				{
					pHurt->ViewPunch( QAngle( 5, 0, random->RandomInt(-10,10)) );
				}
				
				// Spawn some extra blood if we hit a BCC
				CBaseCombatCharacter* pBCC = ToBaseCombatCharacter( pHurt );
				if (pBCC)
				{
					SpawnBlood(pBCC->EyePosition(), g_vecAttackDir, pBCC->BloodColor(), sk_stalker_melee_dmg.GetFloat());
				}

				// Play a attack hit sound
				EmitSound( "NPC_Stalker.Hit" );
			}
			break;	
		}
		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Tells use whether or not the NPC cares about a given type of hint node.
// Input  : sHint - 
// Output : TRUE if the NPC is interested in this hint type, FALSE if not.
//-----------------------------------------------------------------------------
bool CNPC_BetaStalker::FValidateHintType(CAI_Hint *pHint)
{
	return(pHint->HintType() == HINT_WORLD_WORK_POSITION);
}

//-----------------------------------------------------------------------------
// Purpose: Override in subclasses to associate specific hint types
//			with activities
// Input  :
// Output :
//-----------------------------------------------------------------------------
Activity CNPC_BetaStalker::GetHintActivity(short sHintType, Activity HintsActivity)
{
	if (sHintType == HINT_WORLD_WORK_POSITION)
	{
		return ( Activity )ACT_STALKER_WORK;
	}

	return BaseClass::GetHintActivity( sHintType, HintsActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Override in subclasses to give specific hint types delays
//			before they can be used again
// Input  :
// Output :
//-----------------------------------------------------------------------------
float	CNPC_BetaStalker::GetHintDelay( short sHintType )
{
	if (sHintType == HINT_WORLD_WORK_POSITION)
	{
		return 2.0;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
#define ZIG_ZAG_SIZE 3600

void CNPC_BetaStalker::AddZigZagToPath(void) 
{
	// If already on a detour don't add a zigzag
	if (GetNavigator()->GetCurWaypointFlags() & bits_WP_TO_DETOUR)
	{
		return;
	}

	// If enemy isn't facing me or occluded, don't add a zigzag
	if (HasCondition(COND_ENEMY_OCCLUDED) || !HasCondition ( COND_ENEMY_FACING_ME ))
	{
		return;
	}

	Vector waypointPos = GetNavigator()->GetCurWaypointPos();
	Vector waypointDir = (waypointPos - GetAbsOrigin());

	// If the distance to the next node is greater than ZIG_ZAG_SIZE
	// then add a random zig/zag to the path
	if (waypointDir.LengthSqr() > ZIG_ZAG_SIZE)
	{
		// Pick a random distance for the zigzag (less that sqrt(ZIG_ZAG_SIZE)
		float distance = random->RandomFloat( 30, 60 );

		// Get me a vector orthogonal to the direction of motion
		VectorNormalize( waypointDir );
		Vector vDirUp(0,0,1);
		Vector vDir;
		CrossProduct( waypointDir, vDirUp, vDir);

		// Pick a random direction (left/right) for the zigzag
		if (random->RandomInt(0,1))
		{
			vDir = -1 * vDir;
		}

		// Get zigzag position in direction of target waypoint
		Vector zigZagPos = GetAbsOrigin() + waypointDir * 60;

		// Now offset 
		zigZagPos = zigZagPos + (vDir * distance);

		// Now make sure that we can still get to the zigzag position and the waypoint
		AIMoveTrace_t moveTrace1, moveTrace2;
		GetMoveProbe()->MoveLimit( NAV_GROUND, GetAbsOrigin(), zigZagPos, MASK_NPCSOLID, NULL, &moveTrace1);
		GetMoveProbe()->MoveLimit( NAV_GROUND, zigZagPos, waypointPos, MASK_NPCSOLID, NULL, &moveTrace2);
		if ( !IsMoveBlocked( moveTrace1 ) && !IsMoveBlocked( moveTrace2 ) )
		{
			GetNavigator()->PrependWaypoint( zigZagPos, NAV_GROUND, bits_WP_TO_DETOUR );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CNPC_BetaStalker::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		return false;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		SetIdealState(NPC_STATE_IDLE);

		SetAbsVelocity( vec3_origin );
		SetMoveType( MOVETYPE_STEP );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		if ( GetFlags() & FL_ONGROUND )
		{
			RemoveFlag( FL_ONGROUND );
		}

		SetIdealState(NPC_STATE_PRONE);
		return true;
	}
	else if (interactionType == g_interactionScriptedTarget)
	{
		// If I already have a scripted target, reject the new one
		if (m_pScriptedTarget && sourceEnt)
		{
			return false;
		}
		else
		{
			SetScriptedTarget((CScriptedTarget*)sourceEnt);
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_BetaStalker::CNPC_BetaStalker(void)
{
#ifdef _DEBUG
	m_vLaserDir.Init();
	m_vLaserTargetPos.Init();
	m_vLaserCurPos.Init();
#endif
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_stalker_beta, CNPC_BetaStalker )

	DECLARE_TASK(TASK_STALKER_BETA_ZIGZAG)

	DECLARE_ACTIVITY(ACT_STALKER_WORK)

	DECLARE_SQUADSLOT(SQUAD_SLOT_BSTALK_CHASE_1)
	DECLARE_SQUADSLOT(SQUAD_SLOT_BSTALK_CHASE_2)


	//=========================================================
	// > SCHED_STALKER_BETA_RANGE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STALKER_BETA_RANGE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_FACE_ENEMY					0"
		"		TASK_RANGE_ATTACK1				0"
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
		"		COND_REPEATED_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		//"		ENEMY_OCCLUDED"	// Don't break on this.  Keep shooting at last location
	)

	//=========================================================
	// > SCHED_STALKER_BETA_CHASE_ENEMY
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_STALKER_BETA_CHASE_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY_FAILED"
		"		TASK_SET_TOLERANCE_DISTANCE		24"
		"		TASK_GET_PATH_TO_ENEMY			0"
		"		TASK_RUN_PATH					0"
		"		TASK_STALKER_BETA_ZIGZAG				0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_BETTER_WEAPON_AVAILABLE"
		"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_NPC()
