//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	Alien Controllers from HL1 now in updated form
//			by Au-heppa
//
//=============================================================================//


#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_basenpc.h"
#include "ai_basenpc_physicsflyer.h"
#include "npcevent.h"
#include "ai_basenpc_physicsflyer.h"
#include "soundenvelope.h"
#include "ai_hint.h"
#include "ai_route.h"
#include "ai_moveprobe.h"
#include "ai_squad.h"

#include "effect_dispatch_data.h" //muzzle flash

#include "prop_combine_ball.h"

#include "weapon_fireball.h"
#include "npc_aliencontroller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ConVar g_debug_aliencontroller("hlss_aliencontroller_debug","0");

ConVar sk_aliencontroller_health( "sk_aliencontroller_health", "0");
ConVar sk_aliencontroller_dmg_claw( "sk_aliencontroller_dmg_claw", "0");

ConVar aliencontroller_max_enemy_distance( "hlss_aliencontroller_max_enemy_distance", "1024");
ConVar aliencontroller_min_enemy_distance( "hlss_aliencontroller_min_enemy_distance", "512");
ConVar aliencontroller_prefered_height("hlss_aliencontroller_prefered_height","256");
ConVar aliencontroller_fly_speed("hlss_aliencontroller_fly_speed","50");

#define CONTROLLER_FLYSPEED aliencontroller_fly_speed.GetFloat()
#define CONTROLLER_FLYSPEED_MAX 180
#define CONTROLLER_FLYSPEED_MIN 50

#define CONTROLLER_THROW_SPEED 1500

#define CONTROLLER_TOO_CLOSE_TO_ATTACK 50

#define CONTROLLER_OFFSET_SWITCH_DELAY random->RandomInt(4,7)




int AE_CONTROLLER_FIREGLOW;
int AE_CONTROLLER_FIREBALL;
int AE_CONTROLLER_THROW_PHYSOBJ;
int AE_CONTROLLER_FLY;
int AE_CONTROLLER_LAND;

int AE_CONTROLLER_HAND;
int AE_CONTROLLER_SWING_SOUND;

Activity ACT_CONTROLLER_PULL_PHYSOBJ;
Activity ACT_CONTROLLER_HOLD_PHYSOBJ;
Activity ACT_CONTROLLER_THROW_PHYSOBJ;

Activity ACT_CONTROLLER_LAND;
Activity ACT_CONTROLLER_LIFTOFF;


BEGIN_DATADESC( CNPC_AlienController )

	DEFINE_FIELD( m_iFireBallAttachment,		FIELD_INTEGER),
	//DEFINE_FIELD( m_pFireGlow,					FIELD_EHANDLE),
	DEFINE_FIELD( m_iFireBallFadeIn,			FIELD_INTEGER),
	DEFINE_FIELD( m_fFireBallFadeInTime,		FIELD_TIME),
	DEFINE_FIELD( m_hFireBallTarget,			FIELD_EHANDLE),

	DEFINE_FIELD( m_iLastOffsetAngle,			FIELD_INTEGER),
	DEFINE_FIELD( m_fLastOffsetSwithcTime,		FIELD_TIME),
	DEFINE_KEYFIELD( m_fPreferedHeight,			FIELD_FLOAT, "prefered_height"),
	DEFINE_FIELD( m_fLastTurnSpeed,				FIELD_FLOAT),

	DEFINE_FIELD( m_flNextAttackTime,			FIELD_TIME ),
	DEFINE_FIELD( m_iNumberOfAttacks,			FIELD_INTEGER ),

	DEFINE_FIELD( m_bHasObject,					FIELD_BOOLEAN),
	DEFINE_FIELD( m_hPhysicsEnt,				FIELD_EHANDLE),
	DEFINE_FIELD( m_flNextSwat,					FIELD_TIME),
	DEFINE_FIELD( m_flNextSwatScan,				FIELD_TIME),
	DEFINE_FIELD( m_iContainerMoveType,			FIELD_INTEGER),

	DEFINE_FIELD( m_iPhysGunAttachment,			FIELD_INTEGER),

	DEFINE_FIELD( m_bIsFlying,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsLanding,					FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNextLandingTime,			FIELD_TIME ),

	DEFINE_FIELD( m_flNextCombineBallScan,		FIELD_TIME ),
	DEFINE_FIELD( m_hCombineBall,				FIELD_EHANDLE ),

END_DATADESC()


LINK_ENTITY_TO_CLASS(npc_aliencontroller, CNPC_AlienController);

CNPC_AlienController::CNPC_AlienController()
{
	m_iFireBallAttachment = -1;
	m_iFireBallFadeIn	  = 0;
	m_fFireBallFadeInTime = 0.f;

	m_flNextSwatScan = 0;
	m_flNextSwat	 = 0;

	m_fPreferedHeight = 128;

	m_flNextCombineBallScan = 0;
}

void CNPC_AlienController::Spawn(void)
{
	BaseClass::Spawn();

	Precache();

	SetModel( "models/Controller.mdl");

	SetHullType( HULL_HUMAN ); //HULL_HUMAN_CENTERED );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	SetMoveType( MOVETYPE_FLY );

	SetNavType( NAV_FLY );
		
	CapabilitiesAdd ( bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_SKIP_NAV_GROUND_CHECK );
	CapabilitiesAdd ( bits_CAP_MOVE_FLY );
	CapabilitiesAdd	( bits_CAP_INNATE_RANGE_ATTACK1 );

	AddEFlags( EFL_NO_DISSOLVE );

	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= sk_aliencontroller_health.GetFloat();

	m_flFieldOfView		= VIEW_FIELD_WIDE;

	AddSpawnFlags( SF_NPC_LONG_RANGE );

	m_NPCState			= NPC_STATE_NONE;

	m_iFireBallAttachment = LookupAttachment("FireBallGlow");
	m_iPhysGunAttachment  = LookupAttachment("PhysObject");

	CreateFireGlow();

	m_hFireBallTarget = NULL;

	m_hCombineBall	  = NULL;

	m_hPhysicsEnt = NULL;
	m_bHasObject  = false;

	m_iLastOffsetAngle = random->RandomInt( 45, 90 );
	m_iLastOffsetAngle = m_iLastOffsetAngle * ((-1)^(random->RandomInt(1,2)));
	m_fLastOffsetSwithcTime = 0;

	m_fLastTurnSpeed		= 0;

	m_bIsFlying				= true;
	m_bIsLanding			= false;
	m_bNextLandingTime		= 0;

	m_flNextAttackTime = 0;
	m_iNumberOfAttacks = random->RandomInt(3,6);

	NPCInit();
}

void CNPC_AlienController::Precache(void)
{
	PrecacheModel( "models/Controller.mdl" );
	PrecacheModel( CONTROLLER_FIRE_SPRITE );

	PrecacheScriptSound( "NPC_Vortigaunt.FootstepLeft" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepRight" );
	PrecacheScriptSound( "NPC_Vortigaunt.Claw" );
	PrecacheScriptSound( "NPC_Vortigaunt.Swing" );
	
	UTIL_PrecacheOther( "fireball_missile" ); 

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CNPC_AlienController::Activate()
{
	BaseClass::Activate();

	// Have to do this here because sprites do not go across level transitions
	if (!m_pFireGlow)
	{
		CreateFireGlow();
	}

}

void CNPC_AlienController::CombineBallCheck( void )
{
	CSound *pSound = GetLoudestSoundOfType( SOUND_DANGER );

	if (pSound && pSound->m_hOwner && FClassnameIs( pSound->m_hOwner, "prop_combine_ball" ) )
	{
		m_flNextCombineBallScan = gpGlobals->curtime + 4.0f;

		DevMsg("ALIENCONTROLLER: We have found a combine energy ball\n");
		CPropCombineBall *pBally = dynamic_cast<CPropCombineBall*>((CBaseEntity*)pSound->m_hOwner);

		if (pBally && pBally->VPhysicsGetObject() )
		{
			Vector forward;
			GetVectors(&forward, NULL, NULL);

			Vector vecPosition, vecDirection;
			pBally->VPhysicsGetObject()->GetPosition( &vecPosition, NULL );
			vecDirection = vecPosition - GetAbsOrigin();
			//pBally->VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );

			if (DotProduct( forward, vecDirection ) > 0.45)
			{
				//TERO: this is all the stuff we need to change in order for the Bally to behave nicely
				pBally->SetOwnerEntity( this );
				pBally->StartLifetime( 2.0f );
				pBally->SetWeaponLaunched( true );
				pBally->SetCollisionGroup( HL2COLLISION_GROUP_COMBINE_BALL_NPC );

				PhysSetGameFlags( pBally->VPhysicsGetObject(), FVPHYSICS_NO_NPC_IMPACT_DMG );
				PhysClearGameFlags( pBally->VPhysicsGetObject(), FVPHYSICS_DMG_DISSOLVE | FVPHYSICS_HEAVY_OBJECT );
				
				m_hCombineBall = pBally;

				m_flNextCombineBallScan = gpGlobals->curtime + 0.5f;

				if (GetEnemy() && HasCondition( COND_SEE_ENEMY ))
				{
					//TERO: setting speed for the Combine Ball
					vecDirection = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
					VectorNormalize(vecDirection);
					vecDirection *= 0.5;
					pBally->VPhysicsGetObject()->SetVelocity( &vecDirection, NULL );
				} else
				{
					pBally->VPhysicsGetObject()->GetVelocity( &vecDirection, NULL );
					VectorNormalize(vecDirection);
					vecDirection *= -0.5;
					pBally->VPhysicsGetObject()->SetVelocity( &vecDirection, NULL );
				}
			}
		}
	}
}

void CNPC_AlienController::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	if (m_hCombineBall)
	{
		if ( m_flNextCombineBallScan < gpGlobals->curtime )
		{
			m_flNextCombineBallScan = gpGlobals->curtime + 4.0f;

			CPropCombineBall *pBally = dynamic_cast<CPropCombineBall*>((CBaseEntity*)m_hCombineBall);

			if (pBally && pBally->VPhysicsGetObject())
			{
				if (GetEnemy() && HasCondition(COND_SEE_ENEMY) )
				{
					Vector vecEnemy = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin());
					VectorNormalize( vecEnemy );
					vecEnemy *= 1000.0f;
					pBally->VPhysicsGetObject()->SetVelocity( &vecEnemy, NULL );
				} else
				{
					Vector vecDirection;
					pBally->VPhysicsGetObject()->GetVelocity( &vecDirection, NULL );
					VectorNormalize( vecDirection );
					vecDirection *= 1000.0f;
					pBally->VPhysicsGetObject()->SetVelocity( &vecDirection, NULL );
				}
			}

			m_hCombineBall = NULL;
		}
	}
	else if ( m_flNextCombineBallScan < gpGlobals->curtime ) //HasCondition(COND_HEAR_DANGER) &&
	{
		//DevMsg("Aliencontroller: hear danger\n");
		CombineBallCheck();
	}

	if (m_fFireBallFadeInTime!=0.f)
	{
		m_iFireBallFadeIn = (int)((gpGlobals->curtime - m_fFireBallFadeInTime) * 1.5 * 255);
		m_iFireBallFadeIn = clamp(m_iFireBallFadeIn, 0, 255);

		if (m_pFireGlow)
			m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
		else
			CreateFireGlow();
	} else if (m_iFireBallFadeIn!=0)
	{
		m_iFireBallFadeIn = 0;
		m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
	}
}

void CNPC_AlienController::CreateFireGlow()
{
	m_pFireGlow = CSprite::SpriteCreate( CONTROLLER_FIRE_SPRITE, GetLocalOrigin(), FALSE );
	if (m_pFireGlow)
	{
		DevMsg("Fireglow created\n");
		m_pFireGlow->SetRenderMode(kRenderTransColor );
		m_pFireGlow->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
		m_pFireGlow->SetAttachment( this, m_iFireBallAttachment );
		//m_pFireGlow->SetBrightness( 0 );
		m_pFireGlow->SetScale( 0.4 );
		m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
	}
}


void CNPC_AlienController::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_CONTROLLER_THROW_PHYSOBJ )
	{
		DevMsg("Aliencontroller: throw physobj\n");
		ThrowObject();
		return;
	}

	if ( pEvent->event == AE_CONTROLLER_FIREGLOW )
	{
		m_iFireBallFadeIn	  = 0;
		m_fFireBallFadeInTime = gpGlobals->curtime;
		if (m_pFireGlow!=NULL)
		{
			m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
		}
 		else
		{
			CreateFireGlow();
		}
		return;
	}


	if ( pEvent->event == AE_CONTROLLER_FIREBALL )
	{

		m_iFireBallFadeIn	  = 0;
		m_fFireBallFadeInTime = 0.f;
		if (m_pFireGlow)
		{	
			m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
		}
		else
			CreateFireGlow();

		ShootFireBall();
		return;
	}

	if ( pEvent->event == AE_CONTROLLER_LAND )
	{
		m_bIsFlying = false;
		m_bIsLanding = false;
		DevMsg("Succesfully landed\n");
		ClearCondition(COND_ALIENCONTROLLER_SHOULD_LAND);
		return;
	}

	if ( pEvent->event == AE_CONTROLLER_FLY )
	{
		m_bIsFlying = true;
		m_bIsLanding = false;
		DevMsg("Seccesfully lifted off\n");
		ClearCondition(COND_ALIENCONTROLLER_SHOULD_LAND);
		return;
	}

	if ( pEvent->event == AE_CONTROLLER_HAND )
	{
		Claw();

		return;
	}

	if ( pEvent->event == AE_CONTROLLER_SWING_SOUND )
	{
		EmitSound( "NPC_Vortigaunt.Swing" );	
		return;
	}

	if ( pEvent->event == AE_NPC_LEFTFOOT )
	{
		EmitSound( "NPC_Vortigaunt.FootstepLeft", pEvent->eventtime );
		return;
	}

	if ( pEvent->event == AE_NPC_RIGHTFOOT )
	{
		EmitSound( "NPC_Vortigaunt.FootstepRight", pEvent->eventtime );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}


void CNPC_AlienController::Claw()
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 50, Vector(-10,-10,-10), Vector(10,10,10),sk_aliencontroller_dmg_claw.GetFloat(), DMG_SLASH );
	if ( pHurt )
	{
		pHurt->ViewPunch( QAngle(5,0,-18) );
		// Play a random attack hit sound
		EmitSound( "NPC_Vortigaunt.Claw" );
	}
}

int CNPC_AlienController::GetSoundInterests( void )
{
	return	SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER;
}


int CNPC_AlienController::SelectSchedule( void )
{
	if (HasCondition(COND_ALIENCONTROLLER_SHOULD_LAND))
	{
		return SCHED_ALIENCONTROLLER_LAND;
	}

	if ( m_hPhysicsEnt )
	{
		if ( m_bHasObject == true )
		{
			DevMsg("Aliencontroller: SCHED_ALIENCONTROLLER_THROW_PHYSOBJ\n");
			return SCHED_ALIENCONTROLLER_THROW_PHYSOBJ;
		} else
		{
			if( DistToPhysicsEnt() > ALIENCONTROLLER_PHYSOBJ_PULLDIST )
			{
				DevMsg("Aliencontroller: SCHED_ALIENCONTROLLER_PULL_PHYSOBJ\n");
				return SCHED_ALIENCONTROLLER_MOVE_TO_PHYSOBJ;
			}
			else
			{
				DevMsg("Aliencontroller: SCHED_ALIENCONTROLLER_PULL_PHYSOBJ\n");
				return SCHED_ALIENCONTROLLER_PULL_PHYSOBJ;
			}
		}
	}

	if (HasCondition(COND_ALIENCONTROLLER_FLY_BLOCKED))
		return SCHED_CHASE_ENEMY;

	return BaseClass::SelectSchedule();
}

void CNPC_AlienController::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask)
	{
	case TASK_ALIENCONTROLLER_LAND:
		{
			Land();
		}
		break;
	case TASK_ALIENCONTROLLER_GET_PATH_TO_PHYSOBJ:
		{
			CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;
			if( !pPhysicsEntity )
			{
				TaskFail("Physics ent NULL");
				break;
			}

			Vector vecGoalPos;
			Vector vecDir;

			vecDir = GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin();

			VectorNormalize(vecDir);
			vecDir.z = 0;

			AI_NavGoal_t goal( m_hPhysicsEnt->WorldSpaceCenter() );
			goal.pTarget = m_hPhysicsEnt;
			GetNavigator()->SetGoal( goal );

			TaskComplete();
		}
		break;

	case TASK_ALIENCONTROLLER_PULL_PHYSOBJ:
		{
			if( m_hPhysicsEnt == NULL )
			{
				// Physics Object is gone! Probably was an explosive 
				// or something else broke it.
				TaskFail("Physics ent NULL");
			}
			else if ( DistToPhysicsEnt() > ALIENCONTROLLER_PHYSOBJ_PULLDIST )
			{
				// Physics ent is no longer in range! Probably another zombie swatted it or it moved
				// for some other reason.
				TaskFail( "Physics pull item has moved" );
			}
			SetIdealActivity( (Activity)ACT_CONTROLLER_PULL_PHYSOBJ );
			
		}
		break;
	case TASK_ALIENCONTROLLER_THROW_PHYSOBJ:
		{
			if( m_hPhysicsEnt == NULL )
			{
				// Physics Object is gone! Probably was an explosive 
				// or something else broke it.
				TaskFail("Physics ent NULL");
				return;
			}
			else
			{
				if ( m_hPhysicsEnt == NULL || m_bHasObject == false )
				{
					 TaskFail( "Don't have the item!" );
					 return;
				}

				DevMsg("AlienContoller: setting activity ACT_CONTROLLER_THROW_PHYSOBJ\n");
				SetIdealActivity( (Activity)ACT_CONTROLLER_THROW_PHYSOBJ );
			}
		}
		break;
	case TASK_ALIENCONTROLLER_FIREBALL_EXTINGUISH:
		{
			m_iFireBallFadeIn = 0;

			if (m_pFireGlow)
				m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
			else
				CreateFireGlow();

			TaskComplete();
		}
		break;
	default:
		{
			BaseClass::StartTask( pTask );
		}
		break;
	}

}

void CNPC_AlienController::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_ALIENCONTROLLER_LAND:
			{
				if (!m_bIsLanding)
				{
					if (!m_bIsFlying)
						DevMsg("Landing task complete\n");
					else
						DevMsg("Lifting Off task complete\n");
					TaskComplete();
				}
			}
			break;
		case TASK_ALIENCONTROLLER_THROW_PHYSOBJ:
			{
				if( IsActivityFinished() )
				{
					DevMsg("AlienController: activity finished, task finished, yay\n");

					TaskComplete();
				}
			}
			break;
		case TASK_ALIENCONTROLLER_PULL_PHYSOBJ:
			{
				PullObject( false );
				if (IsActivityFinished() )
				{
					SetIdealActivity( (Activity)ACT_CONTROLLER_HOLD_PHYSOBJ );
				}
			}
			break;
		case TASK_ALIENCONTROLLER_FIREBALL_EXTINGUISH:
			{
				TaskComplete();
			}
			break;
		default:
			{
				BaseClass::RunTask( pTask );
			}
			break;
	}
}


Activity CNPC_AlienController::NPC_TranslateActivity( Activity eNewActivity )
{
	if (m_bIsFlying)
	{
		if (eNewActivity == ACT_IDLE || eNewActivity == ACT_GLIDE )
		{
			return ACT_FLY;
		}
	} else
	{
		if (eNewActivity == ACT_RANGE_ATTACK1)
		{
			return ACT_RANGE_ATTACK2;
		}
	}

	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_AlienController::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			m_fFireBallFadeInTime =0.f;
			if (m_pFireGlow)
				m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);
			else CreateFireGlow();

			return SCHED_COMBAT_FACE;
			break;
		}
	case SCHED_CHASE_ENEMY_FAILED:
		{
			baseType = BaseClass::TranslateSchedule(scheduleType);
			if ( baseType != SCHED_CHASE_ENEMY_FAILED )
				return baseType;

			if (HasMemory(bits_MEMORY_INCOVER))
			{
				// Occasionally run somewhere so I don't just
				// stand around forever if my enemy is stationary
				if (random->RandomInt(0,5) == 5 && (GetEnemy() && GetEnemy()->Classify()!=CLASS_MANHACK) )
				{
					return SCHED_PATROL_RUN;
				}
				else
				{
					return SCHED_IDLE_STAND;
				}
			}
			break;
		}
	case SCHED_FAIL_TAKE_COVER:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			m_fFireBallFadeInTime =0.f;
			if (m_pFireGlow)
				m_pFireGlow->SetRenderColorA(m_iFireBallFadeIn);	
			else
				CreateFireGlow();

			return SCHED_RUN_RANDOM;
			break;
		}
	case SCHED_RANGE_ATTACK1:
		{
			return SCHED_ALIENCONTROLLER_SHOOT_FIREBALL;
			break;
		}
	}
	
	return BaseClass::TranslateSchedule( scheduleType );
}


Vector CNPC_AlienController::GetPreferedAttackPoint(CBaseEntity *moveTarget)
{
	if (moveTarget)
	{
		Vector vecTarget;

		if (IsCurSchedule( SCHED_CHASE_ENEMY ))
		{
			vecTarget=moveTarget->GetAbsOrigin();
		}
		else
		{
			float flDistance = ( moveTarget->GetAbsOrigin()  - GetAbsOrigin() ).Length();

			if (flDistance > aliencontroller_max_enemy_distance.GetFloat() )
				flDistance = aliencontroller_max_enemy_distance.GetFloat();
			else if (flDistance < aliencontroller_min_enemy_distance.GetFloat() )
				flDistance = aliencontroller_min_enemy_distance.GetFloat();


			//Note: if enemy's closer to prefered, we maybe don't want to get away straight away

			//We want to change the direction we are going once in a while or if that direction is blocked
			if (HasCondition(COND_ALIENCONTROLLER_FLY_BLOCKED) || gpGlobals->curtime > m_fLastOffsetSwithcTime )
			{
				m_fLastOffsetSwithcTime = gpGlobals->curtime + CONTROLLER_OFFSET_SWITCH_DELAY;
				m_iLastOffsetAngle      = -m_iLastOffsetAngle;
			}
	
			QAngle angTarget = QAngle(0, moveTarget->GetAbsAngles().y + m_iLastOffsetAngle,0);
			AngleVectors( angTarget, &vecTarget );

			vecTarget = moveTarget->GetAbsOrigin() + vecTarget * flDistance;

		}

		vecTarget.z = moveTarget->GetAbsOrigin().z + m_fPreferedHeight; 

		if ( g_debug_aliencontroller.GetInt() == 2 )
		{
			NDebugOverlay::Box( vecTarget, GetHullMins(), GetHullMaxs(), 255, 0, 0, 0, 5 );
		}

		trace_t tr;
		AI_TraceHull( moveTarget->GetAbsOrigin(), vecTarget, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		vecTarget=tr.endpos; // - Vector(0,0, ((1-tr.fraction) * 32) );

		if ( g_debug_aliencontroller.GetInt() == 2 )
		{
			NDebugOverlay::Box( vecTarget, GetHullMins(), GetHullMaxs(), 0, 255, 0, 0, 5 );
		}

		return vecTarget;
	}

	return GetAbsOrigin();
}

bool CNPC_AlienController::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
	if (!m_bIsFlying)
		return false;

	float testSpeed;
	testSpeed = clamp(m_fLastTurnSpeed, -1,1);
	if (testSpeed == m_fLastTurnSpeed)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we are too close to ground to land or if there's enough space above us to fly
// Input  : 
// Output : bool if we should land or lift off
//-----------------------------------------------------------------------------
bool CNPC_AlienController::CheckLanding()
{
	m_bNextLandingTime = gpGlobals->curtime + 1;

	trace_t tr;

	//First check if we should keep flying or lift off
	float height = m_fPreferedHeight;
	if (height < 32) 
			height = 32;

	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,m_fPreferedHeight), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction == 1  )
	{
			NDebugOverlay::Line(GetAbsOrigin() + Vector(0,0,72), tr.endpos, 255,255,255, true, 0);
			NDebugOverlay::Box( tr.endpos, Vector(10,10,10), Vector(-10,-10,-10), 255, 255, 255, 0, 1 );
			DevMsg("Should Fly\n");
			if (m_bIsFlying)
				return false;
			else
				return true;
	}

	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-32), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction != 1 )
	{
		DevMsg("Should Land\n");

		NDebugOverlay::Line(GetAbsOrigin(), tr.endpos, 255,255,255, true, 0);
		NDebugOverlay::Box( tr.endpos, Vector(10,10,10), Vector(-10,-10,-10), 255, 255, 255, 0, 1 );
		if (m_bIsFlying)
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Lands or Lifts Off depending if we are flying or walking
// Input  : 
// Output :
//-----------------------------------------------------------------------------
void CNPC_AlienController::Land()
{
	m_bIsLanding=true;

	if (m_bIsFlying)
	{
		//We are going to walk
		SetNavType( NAV_GROUND );

		RemoveFlag( FL_FLY );
		
		CapabilitiesAdd ( bits_CAP_MOVE_GROUND );
		CapabilitiesRemove( bits_CAP_MOVE_FLY );
		SetMoveType( MOVETYPE_STEP );

		SetIdealActivity( (Activity) ACT_CONTROLLER_LAND);
		DevMsg("Is now Landing\n");
	} else
	{
		//We are gonna fly
		SetGroundEntity( NULL );

		SetNavType( NAV_FLY );

		AddFlag( FL_FLY );
		
		CapabilitiesAdd ( bits_CAP_MOVE_FLY );
		CapabilitiesRemove( bits_CAP_MOVE_GROUND );

		SetMoveType( MOVETYPE_FLY );

		SetIdealActivity( (Activity) ACT_CONTROLLER_LIFTOFF );
		DevMsg("Is now Lifting Off\n");
	}

	/*bool hasGoal= GetNavigator()->IsGoalActive();;
	Vector vecGoal = GetAbsOrigin();

	if ( hasGoal )
	{
		vecGoal = GetNavigator()->GetGoalPos();
	} else if ( GetEnemy() )
	{
		vecGoal = GetEnemy()->GetAbsOrigin();
	}

	GetNavigator()->ClearGoal();

	GetNavigator()->SetGoal( vecGoal );*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_AlienController::OverrideMove( float flInterval )
{	
	if (m_bIsLanding || HasCondition(COND_ALIENCONTROLLER_SHOULD_LAND) )
	{
		return true;
	}
	else if ( !m_hPhysicsEnt && m_bNextLandingTime < gpGlobals->curtime )
	{
		if (CheckLanding() )
		{
			SetCondition(COND_ALIENCONTROLLER_SHOULD_LAND);
			m_bNextLandingTime = gpGlobals->curtime + 6;
			return true;
		}
	} 

	if (!m_bIsFlying)
		return false;

	Vector vMoveTargetPos=GetAbsOrigin();
	CBaseEntity *pMoveTarget = NULL;

	bool bYes=false, hasGoal=false;


	if ( !GetNavigator()->IsGoalActive()  ) //|| ( GetNavigator()->GetCurWaypointFlags() | bits_WP_TO_PATHCORNER )
	{
		// Select move target 
		if ( GetTarget() != NULL )
		{
			pMoveTarget = GetTarget();
			vMoveTargetPos = pMoveTarget->GetAbsOrigin(); // + Vector(0,0,32);
			bYes=true;
		}
		else if ( GetEnemy() != NULL )
		{
			pMoveTarget = GetEnemy();
			if (GetEnemy()->Classify() == CLASS_MANHACK || GetEnemy()->Classify() == CLASS_SCANNER )
				vMoveTargetPos = pMoveTarget->GetAbsOrigin(); // - Vector(0,0,32);
			else
				vMoveTargetPos = GetPreferedAttackPoint(pMoveTarget); // + Vector(0,0,32);

			bYes=true;
		}

		if (HasCondition( COND_ALIENCONTROLLER_FLY_BLOCKED ) && bYes)
		{
			DevMsg("Controller: Condition fly blocked\n");

			//int beforeSetGoal=0;
			//Assert(beforeSetGoal);

			AI_NavGoal_t goal( GOALTYPE_LOCATION, vMoveTargetPos, ACT_FLY );
			if (GetNavigator()->SetGoal( goal ))
			{
				hasGoal=true;
			}
			ClearCondition( COND_ALIENCONTROLLER_FLY_BLOCKED );
		}
	} 

	if (GetNavigator()->IsGoalActive())
	{
		hasGoal = true;

		vMoveTargetPos = GetNavigator()->GetCurWaypointPos(); //  - Vector(0,0,32);

		NDebugOverlay::Box( GetNavigator()->GetCurWaypointPos(), Vector(10,10,10), Vector(-10,-10,-10), 255, 0, 0, 0, 1 );

		Vector vecMoveDir = ( vMoveTargetPos - GetAbsOrigin() );

		float flDistance = VectorNormalize( vecMoveDir );

		if ( flDistance < CONTROLLER_FLYSPEED * flInterval )
		{
			if ( GetNavigator()->IsGoalActive() )
			{
				if ( !GetNavigator()->CurWaypointIsGoal() )
				{
					//GetNavigator()->AdvancePath();
					
					AI_ProgressFlyPathParams_t params( MASK_NPCSOLID );
					params.bTrySimplify = false;

					GetNavigator()->ProgressFlyPath( params );

				} else 
				{
					GetNavigator()->ClearGoal();
					DevMsg("Reached goal!\n");
					hasGoal = false;
				}
			}
		}

		if ( GetHintNode() )
		{
			AIMoveTrace_t moveTrace;
			GetMoveProbe()->MoveLimit( NAV_FLY, GetAbsOrigin(), GetNavigator()->GetCurWaypointPos(), MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace );

			//See if it succeeded
			if ( IsMoveBlocked( moveTrace.fStatus ) )
			{
				Vector vNodePos = vMoveTargetPos;
				GetHintNode()->GetPosition(this, &vNodePos);
			
				GetNavigator()->SetGoal( vNodePos );

				vMoveTargetPos = vNodePos; // - Vector(0,0,32);
			}
		}
		else 
		{
			DevMsg("nay node\n");
			hasGoal = false;
		}
	}

	// See if we can fly there directly
	if ( pMoveTarget || hasGoal )
	{
		trace_t tr;
		AI_TraceHull( GetAbsOrigin(), vMoveTargetPos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		float fTargetDist = (1.0f-tr.fraction)*((GetAbsOrigin() - vMoveTargetPos).Length());
			
		if ( ( tr.m_pEnt == pMoveTarget ) || ( fTargetDist < 50 ) )
		{
			if ( g_debug_aliencontroller.GetInt() == 1 )
			{
				NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 0,255,0, true, 0);
				NDebugOverlay::Cross3D(tr.endpos, Vector(-5,-5,-5),Vector(5,5,5),0,255,0,true,0.1);
			}
		}
		else		
		{
			//HANDY DEBUG TOOL	
			if ( g_debug_aliencontroller.GetInt() == 1 )
			{
				NDebugOverlay::Line(GetLocalOrigin(), vMoveTargetPos, 255,0,0, true, 0);
				NDebugOverlay::Cross3D(tr.endpos,Vector(-5,-5,-5),Vector(5,5,5),255,0,0,true,0.1);
			}

			if ( tr.m_pEnt && tr.m_pEnt->GetCollisionGroup()  == COLLISION_GROUP_BREAKABLE_GLASS )
			{
				SetIdealActivity( (Activity) ACT_RANGE_ATTACK1 );
				m_hFireBallTarget = tr.m_pEnt;
				DevMsg("  We hit glass, we should shoot it. ");
			}
			DevMsg("  Blocked!\n");

			//AI_NavGoal_t goal( GOALTYPE_LOCATION, vMoveTargetPos, ACT_FLY );

			/*if (!hasGoal)
			{
				if (!GetNavigator()->SetGoal( vMoveTargetPos ))
				{
					DevMsg("OH SHIT\n");
				}
			}*/

			SetCondition( COND_ALIENCONTROLLER_FLY_BLOCKED );
		}

		//if (hasGoal)
			ControllerFly(flInterval, vMoveTargetPos, vMoveTargetPos );
		//else
		//	ControllerFly(flInterval, vMoveTargetPos, pMoveTarget->GetAbsOrigin());
	} 
	else 
	{
		//We don't have a target, lets slow down
		Vector vecSpeed = GetAbsVelocity();
		float flSpeed   = VectorNormalize(vecSpeed);

		if (flSpeed < 5)
			flSpeed = 0;
		else
		{
			if (flInterval > 1.0)
				flInterval = 1.0;
			flSpeed = flSpeed * (1-flInterval) * 0.7;
		}

		SetAbsVelocity( vecSpeed * flSpeed );

	}

	return true;
}

/*void CNPC_AlienController::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );
}*/


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CNPC_AlienController::ControllerFly(float flInterval, Vector vMoveTargetPos, Vector vTargetPos )
{
	Vector vecMoveDir = ( vMoveTargetPos - GetAbsOrigin() - Vector(0,0,32) );

	if (vecMoveDir.z > 32)
		vecMoveDir.z = vecMoveDir.z  * 2;

	float flDistance = VectorNormalize( vecMoveDir );

	// Calculate the speed, 0.048 is approx 1 / 1024 * 50 
	float flSpeed = flDistance * 0.078;
	flSpeed = clamp( flSpeed, CONTROLLER_FLYSPEED_MIN, CONTROLLER_FLYSPEED_MAX);

	//DevMsg("Aliencontroller.flSpeed: %f", flSpeed);

	// Look to see if we are going to hit anything.
	Vector vecDeflect;
	if ( Probe( vecMoveDir, flSpeed * flInterval * 1.2, vecDeflect ) )
	{
		vecMoveDir = vecDeflect;
		VectorNormalize( vecMoveDir );
	}

	SetAbsVelocity( vecMoveDir * flSpeed );

	//Now, lets turn to our target, if it's enemy, we turn to him, if not, we turn to the target pos
	//for enemy the vTargetPos is different than vMoveTargetPos, for others it's the same

	vecMoveDir = ( vTargetPos - GetAbsOrigin() );

	QAngle angTarget;
	VectorAngles( vecMoveDir, angTarget );

	float angleDiff, newAngleDiff;

	angleDiff = UTIL_AngleDiff( angTarget.y, GetAbsAngles().y );
	angleDiff = clamp(angleDiff, -10, 10);
	newAngleDiff = angleDiff * 10 * flInterval;
	newAngleDiff = clamp(newAngleDiff, -10, 10);
	if (angleDiff<0) angleDiff=-angleDiff;
	newAngleDiff = clamp(newAngleDiff, -angleDiff, angleDiff);

	angTarget.z = GetAbsAngles().z;
	angTarget.x = GetAbsAngles().x;
	angTarget.y = GetAbsAngles().y + newAngleDiff;

	//if ( (-1 < angleDiff) && (angleDiff < 1) )
	//{
	/*	DevMsg(" Aliencontroller.GetAbsAngles().y: %f", GetAbsAngles().y);
		DevMsg(" angTarget.y: %f", angTarget.y);
		DevMsg(" newAngleDiff: %f", newAngleDiff);
		DevMsg(" flInterval: %f\n", flInterval);*/

		m_fLastTurnSpeed = newAngleDiff;

		SetAbsAngles( angTarget );

	//} else { m_fLastTurnSpeed = 0; }

}

// Purpose: Looks ahead to see if we are going to hit something. If we are, a
//			recommended avoidance path is returned.
// Input  : vecMoveDir - 
//			flSpeed - 
//			vecDeflect - 
// Output : Returns true if we hit something and need to deflect our course,
//			false if all is well.
//-----------------------------------------------------------------------------
bool CNPC_AlienController::Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect )
{
	//
	// Look 1/2 second ahead.
	//
	trace_t tr;
	AI_TraceHull( GetAbsOrigin(), GetAbsOrigin() + vecMoveDir * flSpeed, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1.0f )
	{
		//
		// If we hit something, deflect flight path parallel to surface hit.
		//
		Vector vecUp;
		CrossProduct( vecMoveDir, tr.plane.normal, vecUp );
		CrossProduct( tr.plane.normal, vecUp, vecDeflect );
		VectorNormalize( vecDeflect );
		return true;
	}

	vecDeflect = vec3_origin;
	return false;
}



int CNPC_AlienController::RangeAttack1Conditions( float flDot, float flDist )
{
	//TERO: only because we don't have animations yet for land shooting
	if (m_bIsLanding)
	{
		return ( COND_NONE );
	}

	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}

	if ( m_flNextAttackTime > gpGlobals->curtime )
		return( COND_NONE );

	if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	return( COND_CAN_RANGE_ATTACK1 );
}

int CNPC_AlienController::MeleeAttack1Conditions( float flDot, float flDist )
{
	//TERO: only because we don't have animations yet for land shooting
	if (m_bIsLanding)
	{
		return ( COND_NONE );
	}

	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}

	if ( m_bIsFlying )
	{
		if ( !(GetEnemy()->Classify() == CLASS_MANHACK && GetEnemy()->Classify() == CLASS_SCANNER) )
		{
			return( COND_NONE );
		}
	}

	if (flDist > 50 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	return( COND_CAN_MELEE_ATTACK1 );
}



void CNPC_AlienController::ShootFireBall() 
{

	Vector vecOrigin, vecAttachmentDir;
	QAngle vecAngles, angWantedAngle;

	GetAttachment( m_iFireBallAttachment, vecOrigin, vecAngles );

	angWantedAngle = vecAngles;

	AngleVectors(vecAngles, &vecAttachmentDir);

	VectorNormalize( vecAttachmentDir );


	if (GetEnemy())
	{
		//Les count predicted position for the enemy
		Vector vEnemyForward, vForward;

		GetEnemy()->GetVectors( &vEnemyForward, NULL, NULL );
		GetVectors( &vForward, NULL, NULL );

		float flDot = DotProduct( vForward, vEnemyForward );

		if ( flDot < 0.5f )
			 flDot = 0.5f;

		Vector vecPredictedPos;

		

		//Get our likely position in two seconds
		UTIL_PredictedPosition( GetEnemy(), flDot * random->RandomFloat(0.5f, 1.1f), &vecPredictedPos );

		vecPredictedPos.z = vecPredictedPos.z + (GetEnemy()->BodyTarget(GetAbsOrigin(),true).z - GetEnemy()->GetAbsOrigin().z); 

		if ( g_debug_aliencontroller.GetInt() == 3 )
		{
			NDebugOverlay::Box( vecPredictedPos, Vector(10,10,10), Vector(-10,-10,-10), 255, 0, 0, 0, 5 );
		}

		Vector vecWantedDir = vecPredictedPos - vecOrigin;

		VectorAngles(vecWantedDir, angWantedAngle);

		//We should clamp our yaw (we are only interested on clamping the yaw since it's useless to clamp pitch or roll)
		float angleDiff   = UTIL_AngleDiff( angWantedAngle.y, vecAngles.y );
		angleDiff		  = clamp(angleDiff, -75, 75);
		angWantedAngle.y  = angWantedAngle.y + angleDiff;

		/*angleDiff		  = UTIL_AngleDiff( angWantedAngle.z, vecAngles.z );
		angleDiff		  = clamp(angleDiff, -80, 80);
		angWantedAngle.z  = angWantedAngle.z + angleDiff;

		angleDiff		  = UTIL_AngleDiff( angWantedAngle.x, vecAngles.x );
		angleDiff		  = clamp(angleDiff, -80, 80);
		angWantedAngle.x  = angWantedAngle.x + angleDiff;*/
	} 
	else if (m_hFireBallTarget)
	{
		Vector vecWantedDir = m_hFireBallTarget->GetAbsOrigin() - vecOrigin;

		VectorAngles(vecWantedDir, angWantedAngle);

		//We should clamp our yaw (we are only interested on clamping the yaw since it's useless to clamp pitch or roll)
		float angleDiff   = UTIL_AngleDiff( angWantedAngle.y, vecAngles.y );
		angleDiff		  = clamp(angleDiff, -75, 75);
		angWantedAngle.y  = angWantedAngle.y + angleDiff;

		//We have already shot it once, that should be enough, maybe a counter later 

		m_hFireBallTarget = NULL;

	}

	CFireBall *pFireBall = CFireBall::Create( vecOrigin, angWantedAngle, this );
		
	// NPCs always get a grace period
	if (pFireBall)
		pFireBall->SetGracePeriod( 0.5 );

	m_iNumberOfAttacks--;
	if (m_iNumberOfAttacks <= 0)
	{
		m_iNumberOfAttacks = random->RandomInt(3,6);
		m_flNextAttackTime = gpGlobals->curtime + random->RandomFloat( 0.5, 1.5 );
	}
}

void CNPC_AlienController::Event_Killed( const CTakeDamageInfo &info )
{
	if (m_pFireGlow)
		UTIL_Remove( m_pFireGlow );

	BaseClass::Event_Killed( info );
}



//TERO: PHYSICS THROWER STUFF COMES AFTER THIS

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_AlienController::GatherConditions( void )
{
//	ClearCondition( COND_ALIENCONTROLLER_LOCAL_MELEE_OBSTRUCTION );

	BaseClass::GatherConditions();

	//Uncomment this to ACTIVATE the phys throw stuff
	if( m_NPCState == NPC_STATE_COMBAT && !m_bIsLanding && m_bIsFlying )
	{
		// This check for !m_pPhysicsEnt prevents a crashing bug, but also
		// eliminates the zombie picking a better physics object if one happens to fall
		// between him and the object he's heading for already. 
		if( gpGlobals->curtime >= m_flNextSwatScan && (m_hPhysicsEnt == NULL) && HasCondition( COND_SEE_ENEMY ) )
		{
			DevMsg("AlienController: searching physobject...\n");


			//TERO: OI! OI! OI! PHYS GRAB SEARCH DISABLED! PUT IT BACK
			FindNearestPhysicsObject( ALIENCONTROLLER_MAX_PHYSOBJ_MASS );
			m_flNextSwatScan = gpGlobals->curtime + 5.0;
		}
	}

	/*if( (m_hPhysicsEnt != NULL) && gpGlobals->curtime >= m_flNextSwat && HasCondition( COND_SEE_ENEMY ) )
	{
		SetCondition( COND_ALIENCONTROLLER_CAN_PHYS_ATTACK );
	}
	else
	{
		ClearCondition( COND_ALIENCONTROLLER_CAN_PHYS_ATTACK );
	}*/
}

float CNPC_AlienController::DistToPhysicsEnt( void )
{
	//return ( GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin() ).Length();
	if ( m_hPhysicsEnt != NULL )
		return UTIL_DistApprox2D( GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter() );
	return ALIENCONTROLLER_PHYSOBJ_PULLDIST + 1;
}

bool CNPC_AlienController::FindNearestPhysicsObject( int iMaxMass )
{
	CBaseEntity		*pList[ ALIENCONTROLLER_PHYSICS_SEARCH_DEPTH ];
	CBaseEntity		*pNearest = NULL;
	float			flDist;
	IPhysicsObject	*pPhysObj;
	int				i;
	Vector			vecDirToEnemy;
	Vector			vecDirToObject;

	//TERO: commented out because we want swats even in assaults when the Grunt may have not seen the player yet
	if ( !GetEnemy() )
	{
		// Can't swat, or no enemy, so no swat.
		m_hPhysicsEnt = NULL;
		return false;
	}

	vecDirToEnemy = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
	float dist = VectorNormalize(vecDirToEnemy);
	vecDirToEnemy.z = 0;

	if( dist > ALIENCONTROLLER_PLAYER_MAX_SWAT_DIST )
	{
		// Player is too far away. Don't bother 
		// trying to swat anything at them until
		// they are closer.
		DevMsg("AlienController: player too far away\n");
		return false;
	}

	float flNearestDist = min( dist, ALIENCONTROLLER_FARTHEST_PHYSICS_OBJECT * 0.5 );
	Vector vecDelta( flNearestDist, flNearestDist, GetHullHeight() * 2 );
	//Vector vecGamma( -flNearestDist, -flNearestDist, GetHullHeight() );

	class CAlienControllerSwatEntitiesEnum : public CFlaggedEntitiesEnum
	{
	public:
		CAlienControllerSwatEntitiesEnum( CBaseEntity **pList, int listMax, int iMaxMass )
		 :	CFlaggedEntitiesEnum( pList, listMax, 0 ),
			m_iMaxMass( iMaxMass )
		{
		}

		virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
		{
			CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
			if ( pEntity && 
				 pEntity->VPhysicsGetObject() && 
				 pEntity->VPhysicsGetObject()->GetMass() <= m_iMaxMass && 
				 pEntity->VPhysicsGetObject()->GetMass() >= ALIENCONTROLLER_MIN_PHYSOBJ_MASS &&
				 pEntity->VPhysicsGetObject()->IsAsleep() && 
				 pEntity->VPhysicsGetObject()->IsMoveable() )
			{
				return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
			}
			return ITERATION_CONTINUE;
		}

		int m_iMaxMass;
	};

	CAlienControllerSwatEntitiesEnum swatEnum( pList, ALIENCONTROLLER_PHYSICS_SEARCH_DEPTH, iMaxMass );

	int count = UTIL_EntitiesInBox( GetEnemy()->GetAbsOrigin() - vecDelta, GetEnemy()->GetAbsOrigin() + vecDelta, &swatEnum );

	//NDebugOverlay::Box( GetEnemy()->GetAbsOrigin(), -vecDelta, vecDelta, 255, 0, 0, 0, 5 );

	// magically know where they are
	Vector vecAlienControllerKnees;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.25f ), &vecAlienControllerKnees );

	DevMsg("AlienController: objects in the box %f\n", count);

	for( i = 0 ; i < count ; i++ )
	{
		pPhysObj = pList[ i ]->VPhysicsGetObject();

		Assert( !( !pPhysObj || pPhysObj->GetMass() > iMaxMass || !pPhysObj->IsAsleep() ) );

		Vector center = pList[ i ]->WorldSpaceCenter();
		flDist = UTIL_DistApprox2D( GetAbsOrigin(), center );

		if( flDist >= flNearestDist )
			continue;

		// This object is closer... but is it between the player and the zombie?
		vecDirToObject = pList[ i ]->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize(vecDirToObject);
		vecDirToObject.z = 0;

		if( DotProduct( vecDirToEnemy, vecDirToObject ) < 0.8 )
			continue;

		//if( flDist >= UTIL_DistApprox2D( center, GetEnemy()->GetAbsOrigin() ) )
		//	continue;

		// don't swat things that are over my head.
		if( center.z > EyePosition().z )
			continue;


		//TERO: poista kommentit jos tarvit nuita!
		/*vcollide_t *pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
		
		//Vector *objMins, *objMaxs;
		//physcollision->CollideGetAABB( objMins, objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );*/

		//if ( objMaxs.z < vecAlienControllerKnees.z )
		//	continue;

		if ( !FVisible( pList[i] ) )
			continue;

		// Skip things that the enemy can't see. Do we want this as a general thing? 
		// The case for this feature is that zombies who are pursuing the player will
		// stop along the way to swat objects at the player who is around the corner or 
		// otherwise not in a place that the object has a hope of hitting. This diversion
		// makes the zombies very late (in a random fashion) getting where they are going. (sjb 1/2/06)
		if( !GetEnemy()->FVisible( pList[i] ) )
			continue;

		// Make this the last check, since it makes a string.
		// Don't swat server ragdolls! -TERO: why the hell not? -because it crashes the game, dumbass! -oh, sorry...
		if ( FClassnameIs( pList[ i ], "physics_prop_ragdoll" ) )
			continue;
			
		if ( FClassnameIs( pList[ i ], "prop_ragdoll" ) )
			continue;

		// The object must also be closer to the zombie than it is to the enemy
		pNearest = pList[ i ];
		DevMsg("Aliencontroller: found one matching object\n");
		//flNearestDist = flDist;	//Tero: commented out because we don't need the above thingie
	}

	m_hPhysicsEnt = pNearest;

	if( m_hPhysicsEnt == NULL )
	{
		return false;
	}
	else
	{
		DevMsg("AlienController: success! found a physobject\n");
		return true;
	}
}


void CNPC_AlienController::PullObject( bool bMantain )
{
	if ( m_hPhysicsEnt == NULL )
	{
		TaskFail( "Ack! No Phys Object!");
		return;
	}

	//NDebugOverlay::Line(GetLocalOrigin(), m_hPhysicsEnt->GetAbsOrigin(), 0,255,0, true, 0);

	IPhysicsObject *pPhysObj = m_hPhysicsEnt->VPhysicsGetObject();

	if ( pPhysObj == NULL )
	{
		TaskFail( "Pulling object with no Phys Object?!" );
		return;
	}


	Vector vGunPos;
	GetAttachment( m_iPhysGunAttachment, vGunPos );
	float flDistance = ( vGunPos - m_hPhysicsEnt->WorldSpaceCenter() ).Length();

	if ( bMantain == false )
	{
		if ( flDistance <= ALIENCONTROLLER_CATCH_DISTANCE )
		{
			m_hPhysicsEnt->SetOwnerEntity( this );

			GetNavigator()->StopMoving();

			m_bHasObject = true;
			TaskComplete();
			return;
		}
	}

	Vector vDir = ( vGunPos -  m_hPhysicsEnt->WorldSpaceCenter() );

	Vector vCurrentVel;
	float flCurrentVel;
	AngularImpulse vCurrentAI;

	pPhysObj->GetVelocity( &vCurrentVel, &vCurrentAI );
	flCurrentVel = vCurrentVel.Length();

	VectorNormalize( vCurrentVel );
	VectorNormalize( vDir );

	float flVelMod = ALIENCONTROLLER_PULL_VELOCITY_MOD;

	if ( bMantain == true )
		 flVelMod *= 2;

	vCurrentVel = vCurrentVel * flCurrentVel * flVelMod;

	vCurrentAI = vCurrentAI * ALIENCONTROLLER_PULL_ANGULARIMP_MOD;
	pPhysObj->SetVelocity( &vCurrentVel, &vCurrentAI );

	vDir = vDir * flDistance * (ALIENCONTROLLER_PULL_TO_GUN_VEL_MOD * 2);

	Vector vAngle( 0, 0, 0 );
	pPhysObj->AddVelocity( &vDir, &vAngle );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_AlienController::NPCThink( void )
{
	BaseClass::NPCThink();

	if ( m_hPhysicsEnt == NULL )
	{
		m_bHasObject = false;
	}
	
	if ( m_bHasObject == true )
	{
		 PullObject( true );
	}
}

void CNPC_AlienController::ThrowObject()
{
	if ( m_hPhysicsEnt )
	{
		m_bHasObject = false;

		IPhysicsObject *pPhysObj = m_hPhysicsEnt->VPhysicsGetObject();

		if ( pPhysObj )
		{
			Vector vGunPos;
			QAngle angGunAngles;

			AngularImpulse angVelocity = RandomAngularImpulse( -250 , -250 ) / pPhysObj->GetMass();
			
			GetAttachment(  m_iPhysGunAttachment, vGunPos, angGunAngles );

			pPhysObj->Wake();

			if ( pPhysObj->GetShadowController() )
			{
				m_hPhysicsEnt->SetParent( NULL );
				m_hPhysicsEnt->SetMoveType( (MoveType_t)m_iContainerMoveType );
				m_hPhysicsEnt->SetOwnerEntity( this );

				pPhysObj->RemoveShadowController();
				pPhysObj->SetPosition( m_hPhysicsEnt->GetLocalOrigin(), m_hPhysicsEnt->GetLocalAngles(), true );

				pPhysObj->RecheckCollisionFilter();
				pPhysObj->RecheckContactPoints();
			}

			if ( GetEnemy() )
			{
				//Lets count predicted position for the enemy
				Vector vEnemyForward, vForward;

				GetEnemy()->GetVectors( &vEnemyForward, NULL, NULL );
				GetVectors( &vForward, NULL, NULL );

				float flDot = DotProduct( vForward, vEnemyForward );

				if ( flDot < 0.5f )
					 flDot = 0.5f;
	
				Vector vecPredictedPos;

				//Get our likely position in two seconds
				UTIL_PredictedPosition( GetEnemy(), flDot * 1.1f, &vecPredictedPos );

				vecPredictedPos.z = vecPredictedPos.z + (GetEnemy()->BodyTarget(GetAbsOrigin(),true).z - GetEnemy()->GetAbsOrigin().z); 

				if ( g_debug_aliencontroller.GetInt() == 3 )
				{
					NDebugOverlay::Box( vecPredictedPos, Vector(10,10,10), Vector(-10,-10,-10), 255, 0, 0, 0, 5 );
				}

				Vector vecWantedDir = vecPredictedPos - vGunPos;
				QAngle angWantedAngle = angGunAngles;

				VectorAngles(vecWantedDir, angWantedAngle);

				//We should clamp our yaw (we are only interested on clamping the yaw since it's useless to clamp pitch or roll)
				float angleDiff   = UTIL_AngleDiff( angWantedAngle.y, angGunAngles.y );
				angleDiff		  = clamp(angleDiff, -75, 75);
				angWantedAngle.y  = angWantedAngle.y + angleDiff;

				AngleVectors(angWantedAngle, &vecWantedDir);

				VectorNormalize( vecWantedDir );

				vecWantedDir = vecWantedDir * CONTROLLER_THROW_SPEED;

				pPhysObj->SetVelocity( &vecWantedDir, &angVelocity );
			}
		}
		m_hPhysicsEnt->SetOwnerEntity( NULL );
		m_hPhysicsEnt = NULL;
		m_bHasObject  = false;
	}
}


int	CNPC_AlienController::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{

	return  BaseClass::OnTakeDamage_Alive( info ); //ret;
}

void CNPC_AlienController::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{

	BaseClass::TraceAttack( inputInfo, vecDir, ptr );
}


AI_BEGIN_CUSTOM_NPC( npc_aliencontroller, CNPC_AlienController )

	DECLARE_ANIMEVENT( AE_CONTROLLER_FIREGLOW )
	DECLARE_ANIMEVENT( AE_CONTROLLER_FIREBALL )
	DECLARE_ANIMEVENT( AE_CONTROLLER_THROW_PHYSOBJ )
	DECLARE_ANIMEVENT( AE_CONTROLLER_LAND )
	DECLARE_ANIMEVENT( AE_CONTROLLER_FLY )
	DECLARE_ANIMEVENT( AE_CONTROLLER_SWING_SOUND )
	DECLARE_ANIMEVENT( AE_CONTROLLER_HAND )

	DECLARE_CONDITION( COND_ALIENCONTROLLER_FLY_BLOCKED )
	DECLARE_CONDITION( COND_ALIENCONTROLLER_CAN_PHYS_ATTACK )
	DECLARE_CONDITION( COND_ALIENCONTROLLER_SHOULD_LAND )

	DECLARE_TASK(TASK_ALIENCONTROLLER_PULL_PHYSOBJ)
	DECLARE_TASK(TASK_ALIENCONTROLLER_GET_PATH_TO_PHYSOBJ)
	DECLARE_TASK(TASK_ALIENCONTROLLER_THROW_PHYSOBJ)
	DECLARE_TASK(TASK_ALIENCONTROLLER_FIREBALL_EXTINGUISH)
	DECLARE_TASK(TASK_ALIENCONTROLLER_LAND)

	DECLARE_ACTIVITY(ACT_CONTROLLER_PULL_PHYSOBJ)
	DECLARE_ACTIVITY(ACT_CONTROLLER_HOLD_PHYSOBJ)
	DECLARE_ACTIVITY(ACT_CONTROLLER_THROW_PHYSOBJ)
	DECLARE_ACTIVITY(ACT_CONTROLLER_LAND)
	DECLARE_ACTIVITY(ACT_CONTROLLER_LIFTOFF)

	//=========================================================
	// > SCHED_ALIENGRUNT_MOVE_TO_PHYSICSOBJ
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENCONTROLLER_MOVE_TO_PHYSOBJ,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ALIENCONTROLLER_GET_PATH_TO_PHYSOBJ	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ALIENCONTROLLER_PULL_PHYSOBJ	0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// Pull Physics object
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENCONTROLLER_PULL_PHYSOBJ,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ALIENCONTROLLER_PULL_PHYSOBJ		0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ALIENCONTROLLER_THROW_PHYSOBJ,

		"	Tasks"
		"		TASK_FACE_ENEMY							0.5"
		"		TASK_ALIENCONTROLLER_THROW_PHYSOBJ		0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	);


	//=========================================================
	// > SCHED_ALIENGRUNT_RANGE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENCONTROLLER_SHOOT_FIREBALL,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ALIENCONTROLLER_FIREBALL_EXTINGUISH"
		"		TASK_FACE_IDEAL					0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_WAIT						0.1" // Wait a sec before firing again
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
		"		COND_ALIENCONTROLLER_CAN_PHYS_ATTACK"
		"		COND_ALIENCONTROLLER_SHOULD_LAND"
	);

	//=========================================================
	// > SCHED_ALIENCONTROLLER_FIREBALL_EXTINGUISH
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENCONTROLLER_FIREBALL_EXTINGUISH,

		"	Tasks"
		"		TASK_ALIENCONTROLLER_FIREBALL_EXTINGUISH	0"
		""
		"	Interrupts"
	);

	//=========================================================
	// > SCHED_ALIENCONTROLLER_LAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENCONTROLLER_LAND,

		"	Tasks"
		"		TASK_ALIENCONTROLLER_LAND	0"
		""
		"	Interrupts"
	);
	
AI_END_CUSTOM_NPC()
