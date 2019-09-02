//=================== Half-Life 2: Short Stories Mod 2007 =====================//
//
// Purpose:	AlienGrunts from HL1 now in updated form
//			by Au-heppa
//
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "game.h"				// For skill levels
#include "globalstate.h"
#include "npc_talker.h"
#include "ai_motor.h"
#include "ai_schedule.h"
#include "scripted.h"
#include "basecombatweapon.h"
#include "soundent.h"
#include "npcevent.h"
#include "ai_hull.h"
#include "animation.h"
#include "ammodef.h"				// For DMG_CLUB
#include "Sprite.h"
#include "npc_aliengrunt.h"
#include "activitylist.h"
#include "player.h"
#include "items.h"
#include "vehicle_base.h"
//#include "basegrenade_shared.h"
#include "ai_interactions.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "physics_prop_ragdoll.h"
#include "RagdollBoogie.h"

#include "weapon_physcannon.h"
#include "weapon_bee.h"


#include "ai_hint.h"
#include "ai_network.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	ALIENGRUNT_LEFT_HAND				"lefthand"
#define	ALIENGRUNT_RIGHT_HAND				"righthand"

//#define ALIENGRUNT_SHOULD_WALK_DIST 150
#define ALIENGRUNT_TOO_CLOSE_FOR_BEES 70

static const char *AGRUNT_ATTACK = "AGRUNT_ATTACK";
static const char *AGRUNT_ALERT  = "AGRUNT_ALERT";
static const char *AGRUNT_HIDE   = "AGRUNT_HIDE";


ConVar	sk_aliengrunt_health( "sk_aliengrunt_health","0");
ConVar	sk_aliengrunt_dmg_claw( "sk_aliengrunt_dmg_claw","0");

//=========================================================
// ALIENGRUNT activities
//=========================================================
//int	ACT_ALIENGRUNT_AIM;
Activity ACT_ALIENGRUNT_TO_ACTION;
Activity ACT_ALIENGRUNT_TO_IDLE;
//Activity ACT_ALIENGRUNT_WALK_SHOOTBEES;
//int ACT_SHOOTBEES;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
int AE_ALIENGRUNT_HAND_LEFT;
int AE_ALIENGRUNT_HAND_RIGHT;
int AE_ALIENGRUNT_SHOOTBEES;
//int AE_ALIENGRUNT_SHOOTBEES_DONE;
int AE_ALIENGRUNT_SWING_SOUND;

//-----------------------------------------------------------------------------
// Interactions
//-----------------------------------------------------------------------------
//int	g_interactionALIENGRUNTStomp		= 0;
//int	g_interactionALIENGRUNTStompFail	= 0;
//int	g_interactionALIENGRUNTStompHit		= 0;
//int	g_interactionALIENGRUNTKick			= 0;
//int	g_interactionALIENGRUNTClaw			= 0;


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_AlienGrunt )

	DEFINE_FIELD( m_flNextNPCThink,		FIELD_TIME),

	DEFINE_FIELD( m_HideSoundTime,				FIELD_TIME),

	//DEFINE_FIELD( m_nextLineFireTime,		FIELD_TIME),
	DEFINE_FIELD( m_iLeftHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iRightHandAttachment,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bShouldAim,				FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flNextSwat, FIELD_TIME ),
	DEFINE_FIELD( m_flNextSwatScan, FIELD_TIME ),
	DEFINE_FIELD( m_hPhysicsEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hObstructor, FIELD_EHANDLE ),

END_DATADESC()
LINK_ENTITY_TO_CLASS( npc_aliengrunt, CNPC_AlienGrunt );


void CNPC_AlienGrunt::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	UpdateHead();
	UpdateBeehiveGunAim();
}


//TERO: YOU COULD OPTIMIZE BY PUTTING THESE TWO IN A SAME FUNCTION
void CNPC_AlienGrunt::UpdateBeehiveGunAim(void)
{
	float yaw = GetPoseParameter( "aim_yaw" );
	float pitch = GetPoseParameter( "aim_pitch" );

	// If we should be watching our enemy, turn our head
	if ( m_bShouldAim && ( GetEnemy() != NULL ) )
	{
		Vector	enemyDir = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( enemyDir );
		
		float angle = VecToYaw( BodyDirection3D() );
		float angleDiff = VecToYaw( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + yaw );

		SetPoseParameter( "aim_yaw", UTIL_Approach( yaw + angleDiff, yaw, 50 ) );

		angle = UTIL_VecToPitch( BodyDirection3D() );
		angleDiff = UTIL_VecToPitch( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + pitch );

		SetPoseParameter( "aim_pitch", UTIL_Approach( pitch + angleDiff, pitch, 50 ) );
	}
	else
	{
		// Otherwise turn the head back to its normal position
		SetPoseParameter( "aim_yaw",	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( "aim_pitch", UTIL_Approach( 0, pitch, 10 ) );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AlienGrunt::UpdateHead( void )
{
	float yaw = GetPoseParameter( "head_yaw" );
	float pitch = GetPoseParameter( "head_pitch" );

	// If we should be watching our enemy, turn our head
	if ( ( GetEnemy() != NULL ) )
	{
		Vector	enemyDir = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( enemyDir );
		
		float angle = VecToYaw( BodyDirection3D() );
		float angleDiff = VecToYaw( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + yaw );

		SetPoseParameter( "head_yaw", UTIL_Approach( yaw + angleDiff, yaw, 50 ) );

		angle = UTIL_VecToPitch( BodyDirection3D() );
		angleDiff = UTIL_VecToPitch( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + pitch );

		SetPoseParameter( "head_pitch", UTIL_Approach( pitch + angleDiff, pitch, 50 ) );
	}
	else
	{
		// Otherwise turn the head back to its normal position
		SetPoseParameter( "head_yaw",	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( "head_pitch", UTIL_Approach( 0, pitch, 10 ) );
	}
}

bool CNPC_AlienGrunt::CreateBehaviors()
{
	AddBehavior( &m_AssaultBehavior );

	//AddBehavior( &m_LeadBehavior );
	//AddBehavior( &m_FollowBehavior );
	
	return BaseClass::CreateBehaviors();
}

float CNPC_AlienGrunt::MaxYawSpeed( void )
{
	Activity eActivity = GetActivity();

	// Stay still
	if ( ( eActivity == ACT_RANGE_ATTACK1 ) ||
		 ( eActivity == ACT_ALIENGRUNT_TO_ACTION ) || 
		 ( eActivity == ACT_ALIENGRUNT_TO_IDLE ) || 
		 ( eActivity == ACT_MELEE_ATTACK1 ) )
		return 0.0f;

	switch( eActivity )
	{
	case ACT_IDLE:
		return 30.0f;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 40.0f;
		break;
	
	case ACT_RUN:
	default:
		return 30.0f;
		break;
	}
}


void CNPC_AlienGrunt::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask)
	{

	case TASK_ALIENGRUNT_GET_PATH_TO_PHYSOBJ:
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

	case TASK_ALIENGRUNT_SWAT_ITEM:
		{
			if( m_hPhysicsEnt == NULL )
			{
				// Physics Object is gone! Probably was an explosive 
				// or something else broke it.
				TaskFail("Physics ent NULL");
			}
			else if ( DistToPhysicsEnt() > ALIENGRUNT_PHYSOBJ_SWATDIST )
			{
				// Physics ent is no longer in range! Probably another zombie swatted it or it moved
				// for some other reason.
				TaskFail( "Physics swat item has moved" );
			}
			else
			{
				SetIdealActivity( (Activity)GetSwatActivity() );
			}
			break;
		}
		break;

	case TASK_RANGE_ATTACK1:
	case TASK_MELEE_ATTACK1:
		AttackSound();
	
	default:
		{
			BaseClass::StartTask( pTask );	
			break;
		}
  }
}

void CNPC_AlienGrunt::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ALIENGRUNT_SWAT_ITEM:
		if( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_RANGE_ATTACK1:
		{
			if (GetEnemy() != NULL)
			{
				if (GetEnemy()->IsPlayer())
				{
					m_flPlaybackRate = 1.5;
				}
				if (!GetEnemy()->IsAlive())
				{
					if( IsActivityFinished() )
					{
						TaskComplete();
						break;
					}
				}
				// This is along attack sequence so if the enemy
				// becomes occluded bail
				/*if (HasCondition( COND_ENEMY_OCCLUDED ))
				{
					TaskComplete();
					break;
				}*/
			}
			BaseClass::RunTask( pTask );
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			if (GetEnemy() == NULL)
			{
				if ( IsActivityFinished() )
				{
					TaskComplete();
				}
			}
			else
			{
				BaseClass::RunTask( pTask );
			}
			break;	
		}
	case TASK_MELEE_ATTACK2:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}
	/*case TASK_RANGE_ATTACK1_WARMUP:
	{
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;
	}

	case TASK_RANGE_ATTACK1_COOLDOWN:
	{
		if ( IsActivityFinished() )
		{
//			m_bExtractingBugbait = false;
			TaskComplete();
		}
		break;
	}*/

	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

int CNPC_AlienGrunt::GetSoundInterests ( void) 
{
	return	SOUND_WORLD	|
			SOUND_COMBAT	|
			SOUND_CARCASS	|
			SOUND_MEAT		|
			SOUND_GARBAGE	|
			SOUND_DANGER	|
			SOUND_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Class_T	CNPC_AlienGrunt::Classify ( void )
{
	return	CLASS_ALIENGRUNT;
}

int CNPC_AlienGrunt::RangeAttack1Conditions( float flDot, float flDist )
{
	if (GetEnemy() == NULL)
	{
		return( COND_NONE );
	}

	if ( gpGlobals->curtime < m_flNextAttack )
	{
		return( COND_NONE );
	}

	// Range attack is ineffective on manhack so never use it
	// Melee attack looks a lot better anyway
	/*if (GetEnemy()->Classify() == CLASS_MANHACK)
	{
		return( COND_NONE );
	}*/

	if ( flDist <= ALIENGRUNT_TOO_CLOSE_FOR_BEES )
	{
		return( COND_TOO_CLOSE_TO_ATTACK );
	}
	else if ( flDist > 1500 * 12 )	// 1500ft max
	{
		return( COND_TOO_FAR_TO_ATTACK );
	}
	else if ( flDot < 0.65 )
	{
		return( COND_NOT_FACING_ATTACK );
	}

	return( COND_CAN_RANGE_ATTACK1 );
}


//-----------------------------------------------------------------------------
// Purpose: For innate melee attack
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_AlienGrunt::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if (flDist > 50 )
	{
		// Translate a hit vehicle into its passenger if found
		if ( GetEnemy() != NULL )
		{
			//CBaseCombatCharacter *pCCEnemy = GetEnemy()->MyCombatCharacterPointer();
			//if ( pCCEnemy != NULL && pCCEnemy->IsInAVehicle() )
			//	return MeleeAttack1ConditionsVsEnemyInVehicle( pCCEnemy, flDot );

			// If the player is holding an object, knock it down.
			if( GetEnemy()->IsPlayer() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );

				Assert( pPlayer != NULL );

				// Is the player carrying something?
				CBaseEntity *pObject = GetPlayerHeldEntity(pPlayer);

				if( !pObject )
				{
					pObject = PhysCannonGetHeldEntity( pPlayer->GetActiveWeapon() );
				}

				if( pObject )
				{
					float flDist = pObject->WorldSpaceCenter().DistTo( WorldSpaceCenter() );

					if( flDist <= 50 )
						return COND_CAN_MELEE_ATTACK1;
				}
			}
		}
		return COND_TOO_FAR_TO_ATTACK;
	}

	if (flDot < 0.7)
	{
		ClearCondition(COND_TOO_FAR_TO_ATTACK);
		return COND_NOT_FACING_ATTACK;
	}

	// Build a cube-shaped hull, the same hull that ClawAttack() is going to use.
	Vector vecMins = GetHullMins();
	Vector vecMaxs = GetHullMaxs();
	vecMins.z = vecMins.x;
	vecMaxs.z = vecMaxs.x;

	Vector forward;
	GetVectors( &forward, NULL, NULL );

	trace_t	tr;
	CTraceFilterNav traceFilter( this, false, this, COLLISION_GROUP_NONE );
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + forward * 50, vecMins, vecMaxs, MASK_NPCSOLID, &traceFilter, &tr );

	if( tr.fraction == 1.0 || !tr.m_pEnt )
	{
		// This attack would miss completely. Trick the zombie into moving around some more.
		return COND_TOO_FAR_TO_ATTACK;
	}

	if( tr.m_pEnt == GetEnemy() || tr.m_pEnt->IsNPC() || (tr.m_pEnt->m_takedamage == DAMAGE_YES && (dynamic_cast<CBreakableProp*>(tr.m_pEnt))) )
	{
		// -Let the zombie swipe at his enemy if he's going to hit them.
		// -Also let him swipe at NPC's that happen to be between the zombie and the enemy. 
		//  This makes mobs of zombies seem more rowdy since it doesn't leave guys in the back row standing around.
		// -Also let him swipe at things that takedamage, under the assumptions that they can be broken.
		return COND_CAN_MELEE_ATTACK1;
	}

	if( tr.m_pEnt->IsBSPModel() )
	{
		// The trace hit something solid, but it's not the enemy. If this item is closer to the zombie than
		// the enemy is, treat this as an obstruction.
		Vector vecToEnemy = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();
		Vector vecTrace = tr.endpos - tr.startpos;

		if( vecTrace.Length2DSqr() < vecToEnemy.Length2DSqr() )
		{
			return COND_ALIENGRUNT_LOCAL_MELEE_OBSTRUCTION;
		}
	}

	if ( !tr.m_pEnt->IsWorld() && GetEnemy() && GetEnemy()->GetGroundEntity() == tr.m_pEnt )
	{
		//Try to swat whatever the player is standing on instead of acting like a dill.
		return COND_CAN_MELEE_ATTACK1;
	}

	// Move around some more
	return COND_TOO_FAR_TO_ATTACK;
}

void CNPC_AlienGrunt::Claw( int iAttachment)
{
	CBaseEntity *pHurt = CheckTraceHullAttack( 50, Vector(-10,-10,-10), Vector(10,10,10),sk_aliengrunt_dmg_claw.GetFloat(), DMG_SLASH );
	if ( pHurt )
	{
		pHurt->ViewPunch( QAngle(5,0,-18) );
		// Play a random attack hit sound
		EmitSound( "NPC_Vortigaunt.Claw" );
	}
}

//TERO: The swat AE_ copied from the Base Zombie
void CNPC_AlienGrunt::SwatItem()
{
	CBaseEntity *pEnemy = GetEnemy();
	if ( pEnemy )
	{
		Vector v;
		CBaseEntity *pPhysicsEntity = m_hPhysicsEnt;
		if( !pPhysicsEntity )
		{
			DevMsg( "**Alie Grunt: Missing my physics ent!!" );
			return;
		}
			
		IPhysicsObject *pPhysObj = pPhysicsEntity->VPhysicsGetObject();

		if( !pPhysObj )
		{
			DevMsg( "**Alien Grunt: No Physics Object for physics Ent!" );
			return;
		}

		EmitSound( "NPC_Vortigaunt.Claw" );
		PhysicsImpactSound( pEnemy, pPhysObj, CHAN_BODY, pPhysObj->GetMaterialIndex(), physprops->GetSurfaceIndex("flesh"), 0.5, 800 );

		Vector physicsCenter = pPhysicsEntity->WorldSpaceCenter();
		v = pEnemy->WorldSpaceCenter() - physicsCenter;
		VectorNormalize(v);

		// Send the object at 800 in/sec toward the enemy.  Add 200 in/sec up velocity to keep it
		// in the air for a second or so.
		v = v * 1000;
		v.z += 200;

		// add some spin so the object doesn't appear to just fly in a straight line
		// Also this spin will move the object slightly as it will press on whatever the object
		// is resting on.
		AngularImpulse angVelocity( random->RandomFloat(-180, 180), 20, random->RandomFloat(-360, 360) );

		pPhysObj->AddVelocity( &v, &angVelocity );

		// If we don't put the object scan time well into the future, the zombie
		// will re-select the object he just hit as it is flying away from him.
		// It will likely always be the nearest object because the zombie moved
		// close enough to it to hit it.
		m_hPhysicsEnt = NULL;

		m_flNextSwatScan = gpGlobals->curtime + ALIENGRUNT_SWAT_DELAY;

		return;
	}

}


void CNPC_AlienGrunt::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_ALIENGRUNT_HAND_LEFT )
	{
		if (IsCurSchedule( SCHED_ALIENGRUNT_SWATITEM) || IsCurSchedule(SCHED_ALIENGRUNT_MOVE_SWATITEM) || IsCurSchedule(SCHED_ALIENGRUNT_ATTACKITEM) )
			SwatItem();
		else 
			Claw( m_iLeftHandAttachment );

		return;
	}

	if ( pEvent->event == AE_ALIENGRUNT_HAND_RIGHT )
	{
		if (IsCurSchedule( SCHED_ALIENGRUNT_SWATITEM) || IsCurSchedule(SCHED_ALIENGRUNT_MOVE_SWATITEM) || IsCurSchedule(SCHED_ALIENGRUNT_ATTACKITEM) )
			SwatItem();
		else 
			Claw( m_iRightHandAttachment );

		return;
	}

	if ( pEvent->event == AE_ALIENGRUNT_SHOOTBEES )
	{

		ShootBees();

		EmitSound( "NPC_AlienGrunt.Shoot" );
		//m_bStopLoopingSounds = true;
		ApplyMultiDamage();


		m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.5, 1.5 );
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
	if ( pEvent->event == AE_ALIENGRUNT_SWING_SOUND )
	{
		EmitSound( "NPC_Vortigaunt.Swing" );	
		return;
	}
	
	BaseClass::HandleAnimEvent( pEvent );
}

Activity CNPC_AlienGrunt::NPC_TranslateActivity( Activity eNewActivity )
{
	if (eNewActivity == ACT_ALIENGRUNT_TO_ACTION || eNewActivity == ACT_RANGE_ATTACK1 )
	{
		m_bShouldAim = true;
	} 
	else
	{
		m_bShouldAim = false;
	}

	//TERO: If the Alien Grunt is close enough to his enemy, he will walk to him instead of running
	/*if (eNewActivity == ACT_RUN)  //Commented out because it doesn't look that great
	{
		if (GetEnemy()!=NULL)
		{
			float flDist=UTIL_DistApprox( GetAbsOrigin(), GetEnemy()->GetAbsOrigin());
			if (flDist<ALIENGRUNT_SHOULD_WALK_DIST)
				eNewActivity = ACT_WALK;
		}
	}*/

	if ((eNewActivity == ACT_SIGNAL3)									&& 
		(SelectWeightedSequence ( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE)	)
	{
		eNewActivity = ACT_IDLE;
	}

	if (eNewActivity == ACT_MELEE_ATTACK1)
	{
		// If enemy is low pick ATTACK2 (kick)
		if (GetEnemy() != NULL && (GetEnemy()->EyePosition().z - GetLocalOrigin().z) < 20)
		{
			return ACT_MELEE_ATTACK2;
		}
	}
	return BaseClass::NPC_TranslateActivity( eNewActivity );
}

//------------------------------------------------------------------------------
// Purpose : If I've been in alert for a while and nothing's happened, 
//			 go back to idle
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CNPC_AlienGrunt::ShouldGoToIdleState( void )
{
	if (m_flLastStateChangeTime + 10 < gpGlobals->curtime)
	{
		return true;
	}
	return false;
}

void CNPC_AlienGrunt::Spawn()
{
	Precache();
	SetModel("models/aliengrunt.mdl");

	BaseClass::Spawn();

	SetHullType(HULL_MEDIUM);
	SetHullSizeNormal();

	SetNavType( NAV_GROUND );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_GREEN;
	m_iHealth			= sk_aliengrunt_health.GetFloat();
	//SetViewOffset( Vector ( 0, 0, 64 ) );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_NPCState			= NPC_STATE_NONE;

	GetExpresser()->SetVoicePitch( random->RandomInt( 85, 110 ) );

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD | bits_CAP_MOVE_GROUND );
	CapabilitiesAdd	( bits_CAP_INNATE_RANGE_ATTACK1 );
	CapabilitiesAdd	( bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd	( bits_CAP_DOORS_GROUP );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	
	m_flEyeIntegRate		= 0.6;		// Got a big eyeball so turn it slower

	//m_bStopLoopingSounds	= false;

	m_bShouldAim = false;

	m_iLeftHandAttachment = LookupAttachment( ALIENGRUNT_LEFT_HAND );
	m_iRightHandAttachment = LookupAttachment( ALIENGRUNT_RIGHT_HAND );

	NPCInit();

	m_HideSoundTime	  = 0;

	if (m_spawnflags & SF_NPC_WAIT_TILL_SEEN || HasSpawnFlags( SF_NPC_WAIT_TILL_SEEN) )
	{
		DevMsg("WTF\n");

	}

}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNPC_AlienGrunt::Precache()
{
	PrecacheModel( "models/aliengrunt.mdl" );

	UTIL_PrecacheOther( "bee_missile" ); 

	TalkInit();


	PrecacheScriptSound( "NPC_AlienGrunt.Idle" );
	PrecacheScriptSound( "NPC_AlienGrunt.Hide1" );
	PrecacheScriptSound( "NPC_AlienGrunt.Hide2" );
	PrecacheScriptSound( "NPC_AlienGrunt.Hide3" );
	PrecacheScriptSound( "NPC_AlienGrunt.Hide4" );
	PrecacheScriptSound( "NPC_AlienGrunt.Alert" );
	PrecacheScriptSound( "NPC_AlienGrunt.Combine" );
	PrecacheScriptSound( "NPC_AlienGrunt.CombineHere" );
	PrecacheScriptSound( "NPC_AlienGrunt.GoAway" );
	PrecacheScriptSound( "NPC_AlienGrunt.GetAway" );
	PrecacheScriptSound( "NPC_AlienGrunt.Attack" );


	PrecacheScriptSound( "NPC_AlienGrunt.Pain" );
	PrecacheScriptSound( "NPC_AlienGrunt.Die" );
	PrecacheScriptSound( "NPC_AlienGrunt.Shoot" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepLeft" );
	PrecacheScriptSound( "NPC_Vortigaunt.FootstepRight" );
	PrecacheScriptSound( "NPC_Vortigaunt.Claw" );
	PrecacheScriptSound( "NPC_Vortigaunt.Swing" );

	BaseClass::Precache();
}	

int	CNPC_AlienGrunt::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// make sure friends talk about it if player hurts talkmonsters...
	/*int ret = BaseClass::OnTakeDamage_Alive( info );
	if (!IsAlive())
	{
		return ret;
	}*/

	if (  info.GetDamage() > 25 ) //( info.GetDamageType() & DMG_VEHICLE ) )
	{
			
		CBaseEntity *pAttacker = info.GetInflictor();

		if ( pAttacker && pAttacker->GetServerVehicle() )
		{
			DevMsg("Aliengrunt: should attack vehicle\n");
		}	DevMsg("No attacker or attacker not in vehicle\n");
	}

	//PainSound();

	return  BaseClass::OnTakeDamage_Alive( info ); //ret;
}


void CNPC_AlienGrunt::PainSound ( const CTakeDamageInfo &info )
{

	EmitSound( "NPC_AlienGrunt.Pain" );
}

void CNPC_AlienGrunt::DeathSound( const CTakeDamageInfo &info ) 
{
	EmitSound( "NPC_AlienGrunt.Die" );
}

void CNPC_AlienGrunt::IdleSound( void )
{

	EmitSound( "NPC_AlienGrunt.Idle" );
}

void CNPC_AlienGrunt::HideSound( void )
{
	if ( GetEnemy() != NULL && IsOkToCombatSpeak() )
	{
		if (gpGlobals->curtime < m_HideSoundTime)
			return;
	
		m_HideSoundTime = gpGlobals->curtime + random->RandomFloat(2.5, 6.75);

		Speak( AGRUNT_HIDE );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CNPC_AlienGrunt::AttackSound( void )
{
	if ( GetEnemy() != NULL && IsOkToCombatSpeak() )
	{
		Speak( AGRUNT_ATTACK );
	}
}

void CNPC_AlienGrunt::AlertSound( void )
{
	if ( GetEnemy() != NULL && IsOkToCombatSpeak() )
	{
		Speak( AGRUNT_ALERT );
	}
}

void CNPC_AlienGrunt::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{

	BaseClass::TraceAttack( inputInfo, vecDir, ptr );
}

int CNPC_AlienGrunt::TranslateSchedule( int scheduleType )
{
	int baseType;

	switch( scheduleType )
	{

	case SCHED_CHASE_ENEMY:
		if ( HasCondition( COND_ALIENGRUNT_LOCAL_MELEE_OBSTRUCTION ) && !HasCondition(COND_TASK_FAILED) && IsCurSchedule( SCHED_CHASE_ENEMY, false ) )
		{
			return SCHED_COMBAT_PATROL;
		}
		return SCHED_CHASE_ENEMY;
		break;


	case SCHED_ALIENGRUNT_SWATITEM:
		// If the object is far away, move and swat it. If it's close, just swat it.
		if( DistToPhysicsEnt() > ALIENGRUNT_PHYSOBJ_SWATDIST )
		{
			return SCHED_ALIENGRUNT_MOVE_SWATITEM;
		}
		else
		{
			return SCHED_ALIENGRUNT_SWATITEM;
		}
		break;

	case SCHED_RANGE_ATTACK1:
		{
		return SCHED_ALIENGRUNT_SHOOTBEES;
		break;
		}
	case SCHED_MELEE_ATTACK1:
		{
		return SCHED_ALIENGRUNT_MELEE_ATTACK;
		break;
		}
	case SCHED_IDLE_STAND:
		{
			// call base class default so that scientist will talk
			// when standing during idle
			baseType = BaseClass::TranslateSchedule(scheduleType);

			if (baseType == SCHED_IDLE_STAND)
			{
				// just look straight ahead
				return SCHED_ALIENGRUNT_STAND;
			}
			else
				return baseType;	
			break;

		}
	case SCHED_FAIL_ESTABLISH_LINE_OF_FIRE:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			//ClearBeams();
			//EndHandGlow();

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
				if (random->RandomInt(0,5) == 5)
				{
					return SCHED_PATROL_RUN;
				}
				else
				{
					return SCHED_ALIENGRUNT_STAND;
				}
			}
			break;
		}
	case SCHED_FAIL_TAKE_COVER:
		{
			// Fail schedule doesn't go through SelectSchedule()
			// So we have to clear beams and glow here
			//ClearBeams();
			//EndHandGlow();

			return SCHED_RUN_RANDOM;
			break;
		}
	}
	
	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------
//---------------------------------------------------------
int	CNPC_AlienGrunt::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{

	if ( IsPathTaskFailure( taskFailCode ) && 
		m_hObstructor != NULL && m_hObstructor->VPhysicsGetObject() && 
		 m_hObstructor->VPhysicsGetObject()->GetMass() < 100 )
	{
		if ( !FClassnameIs( m_hObstructor, "physics_prop_ragdoll" ) && !FClassnameIs( m_hObstructor, "prop_ragdoll" ) )
		{
			m_hPhysicsEnt = m_hObstructor;
			m_hObstructor = NULL;
			return SCHED_ALIENGRUNT_ATTACKITEM;
		}
		else
			DevMsg("AlienGrunt: yikes! we almost took a ragdoll as m_hPhysicsEnt\n");
	}

	m_hObstructor = NULL;

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Determines the best type of flinch anim to play.
//-----------------------------------------------------------------------------
Activity CNPC_AlienGrunt::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{

	return BaseClass::GetFlinchActivity( bHeavyDamage, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_AlienGrunt::PlayFlinchGesture( void )
{
	BaseClass::PlayFlinchGesture();

	// To ensure old playtested difficulty stays the same, stop cops shooting for a bit after gesture flinches
	if (m_flNextAttack + 0.5 < gpGlobals->curtime )
		m_flNextAttack = gpGlobals->curtime + random->RandomFloat( 0.5, 1.5 );
}


int CNPC_AlienGrunt::SelectSchedule( void )
{
	if (HasCondition( COND_ENEMY_OCCLUDED ))
		HideSound();

	int nSched = SelectFlinchSchedule();
	if ( nSched != SCHED_NONE )
		return nSched;

	if ( BehaviorSelectSchedule() )
		return BaseClass::SelectSchedule();

	switch( m_NPCState )
	{
	case NPC_STATE_COMBAT:
		{
		
			// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			if( HasCondition( COND_ALIENGRUNT_CAN_SWAT_ATTACK ) )
			{
				return SCHED_ALIENGRUNT_SWATITEM;
			}
			break;

			//TERO: if enemy goes behind a corner we want to shoot some bees after him
			//We need a better schedule here, something that shoots towards the last position we saw him
			/*if ( HasCondition( COND_ENEMY_OCCLUDED ) )
			{
				DevMsg("Enemy occluded, lets shoot some bees\n");
				return SCHED_ALIENGRUNT_SHOOTBEES;
			}*/

		}
		break;

	case NPC_STATE_ALERT:	
	case NPC_STATE_IDLE:

		if ( HasCondition( COND_HEAR_DANGER ) )
		{
			return SCHED_TAKE_COVER_FROM_BEST_SOUND;
		}

		/*if ( HasCondition( COND_ENEMY_DEAD ) && IsOkToCombatEmitSound() )
		{
			EmitSound( AGRUNT_KILL );
		}*/
		break;
	}	
	
	return BaseClass::SelectSchedule();
}

void CNPC_AlienGrunt::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();


	if ( ( ConditionInterruptsCurSchedule( COND_GIVE_WAY ) || 
		   IsCurSchedule(SCHED_HIDE_AND_RELOAD ) || 
		   IsCurSchedule(SCHED_RELOAD ) || 
		   IsCurSchedule(SCHED_STANDOFF ) || 
		   IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY ) || 
		   IsCurSchedule(SCHED_COMBAT_FACE ) || 
		   IsCurSchedule(SCHED_ALERT_FACE )  ||
		   IsCurSchedule(SCHED_COMBAT_STAND ) || 
		   IsCurSchedule(SCHED_ALERT_FACE_BESTSOUND) ||
		   IsCurSchedule(SCHED_ALERT_STAND) ) )
	{
		SetCustomInterruptCondition( COND_PLAYER_PUSHING );
	}
}

void CNPC_AlienGrunt::ShootBees() 
{

		Vector vecOrigin;
		QAngle vecAngles;

		int handAttachment = LookupAttachment( "Hivehand_mouth" );
		GetAttachment( handAttachment, vecOrigin, vecAngles );

	//	Vector forward;
		//GetVectors( NULL, NULL, forward );

		Vector vecDir;
		//CrossProduct( Vector( 0, 0, 1 ), forward, vecDir );
		//vecDir.z = 1.0f;
		//vecDir.x = 0;
		//vecDir.y = 0;
		AngleVectors(vecAngles, &vecDir);

		VectorNormalize( vecDir );

		Vector vecVelocity;
		VectorMultiply( vecDir, 50, vecVelocity );

		//QAngle angles;
		//VectorAngles( vecDir, angles );

		CBee *pBee = CBee::Create( vecOrigin, vecAngles, this );
		
		if (pBee)
		{
			pBee->m_hOwner = this;
			pBee->CreateNavigator();
			pBee->m_hTarget = GetEnemy();

			// NPCs always get a grace period
			pBee->SetGracePeriod( 0.5 );
		}

		//Lets create a "muzzle flash"

		UTIL_BloodDrips( vecOrigin, vecVelocity, BLOOD_COLOR_YELLOW, 1 );

		/*CEffectData data;
		data.m_nEntIndex = entindex();
		data.m_nAttachmentIndex = handAttachment;
		data.m_flScale = 1.0f;
		DispatchEffect( "ChopperMuzzleFlash", data );*/

}

/*Vector CNPC_AlienGrunt::FindNodePositionForBees(Vector startPos, Vector enemyPos)
{
	SetNavType( NAV_FLY );
	Vector vTargetDir = enemyPos - startPos; //just saving in case nodes are not found

	//we can't get there straight, lets try to find an air node
	int	iNearestNode = GetNavigator()->GetNetwork()->NearestNodeToPoint( enemyPos, true);
		
		// GetPathfinder()->NearestNodeToPoint( enemyPos );

	if ( iNearestNode != NO_NODE )
	{
		
		// Get the node and move to it
		CAI_Node *pNode = GetNavigator()->GetNetwork()->GetNode( iNearestNode );
		if ( pNode )
		{

			Vector vecNodePos = pNode->GetPosition( HULL_TINY );
			vTargetDir = vecNodePos - startPos;

			NDebugOverlay::Box( vecNodePos, Vector(10,10,10), Vector(-10,-10,-10), 255, 0, 0, 0, 5 );
			DevMsg("Aliengrunt: giving node position to bee\n");
		}
	} 
	else
		DevMsg("Aliengrunt: no air node\n");

	SetNavType( NAV_GROUND );

	return vTargetDir;
}*/

void CNPC_AlienGrunt::InputAssault( inputdata_t &inputdata )
{
	m_AssaultBehavior.SetParameters( AllocPooledString(inputdata.value.String()), CUE_DONT_WAIT, RALLY_POINT_SELECT_DEFAULT );
}


bool CNPC_AlienGrunt::OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_ENTITY && gpGlobals->curtime >= m_flNextSwat )
	{
		m_hObstructor = pMoveGoal->directTrace.pObstruction;
	}
	
	return false;
}

int CNPC_AlienGrunt::GetSwatActivity( void )
{
	Vector vecMyCenter;
	Vector vecObjCenter;

	vecMyCenter = WorldSpaceCenter();
	vecObjCenter = m_hPhysicsEnt->WorldSpaceCenter();
	float flZDiff;

	flZDiff = vecMyCenter.z - vecObjCenter.z;

		
	if( flZDiff < 0 )
	{
			return ACT_MELEE_ATTACK1;
	}
	return ACT_MELEE_ATTACK2;
}

bool CNPC_AlienGrunt::FindNearestPhysicsObject( int iMaxMass )
{
	CBaseEntity		*pList[ ALIENGRUNT_PHYSICS_SEARCH_DEPTH ];
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

	if( dist > ALIENGRUNT_PLAYER_MAX_SWAT_DIST )
	{
		// Player is too far away. Don't bother 
		// trying to swat anything at them until
		// they are closer.
		return false;
	}

	float flNearestDist = min( dist, ALIENGRUNT_FARTHEST_PHYSICS_OBJECT * 0.5 );
	Vector vecDelta( flNearestDist, flNearestDist, GetHullHeight() * 2.0 );

	class CAlienGruntSwatEntitiesEnum : public CFlaggedEntitiesEnum
	{
	public:
		CAlienGruntSwatEntitiesEnum( CBaseEntity **pList, int listMax, int iMaxMass )
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
				 pEntity->VPhysicsGetObject()->GetMass() >= ALIENGRUNT_MIN_PHYSOBJ_MASS &&
				 pEntity->VPhysicsGetObject()->IsAsleep() && 
				 pEntity->VPhysicsGetObject()->IsMoveable() )
			{
				return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
			}
			return ITERATION_CONTINUE;
		}

		int m_iMaxMass;
	};

	CAlienGruntSwatEntitiesEnum swatEnum( pList, ALIENGRUNT_PHYSICS_SEARCH_DEPTH, iMaxMass );

	int count = UTIL_EntitiesInBox( GetAbsOrigin() - vecDelta, GetAbsOrigin() + vecDelta, &swatEnum );

	// magically know where they are
	Vector vecAlienGruntKnees;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.25f ), &vecAlienGruntKnees );

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

		if( flDist >= UTIL_DistApprox2D( center, GetEnemy()->GetAbsOrigin() ) )
			continue;

		// don't swat things where the highest point is under my knees
		// NOTE: This is a rough test; a more exact test is going to occur below
		if ( (center.z + pList[i]->BoundingRadius()) < vecAlienGruntKnees.z )
			continue;

		// don't swat things that are over my head.
		if( center.z > EyePosition().z )
			continue;

		vcollide_t *pCollide = modelinfo->GetVCollide( pList[i]->GetModelIndex() );
		
		Vector objMins, objMaxs;
		physcollision->CollideGetAABB( &objMins, &objMaxs, pCollide->solids[0], pList[i]->GetAbsOrigin(), pList[i]->GetAbsAngles() );

		if ( objMaxs.z < vecAlienGruntKnees.z )
			continue;

		if ( !FVisible( pList[i] ) )
			continue;

		if ( hl2_episodic.GetBool() )
		{
			// Skip things that the enemy can't see. Do we want this as a general thing? 
			// The case for this feature is that zombies who are pursuing the player will
			// stop along the way to swat objects at the player who is around the corner or 
			// otherwise not in a place that the object has a hope of hitting. This diversion
			// makes the zombies very late (in a random fashion) getting where they are going. (sjb 1/2/06)
			if( !GetEnemy()->FVisible( pList[i] ) )
				continue;
		}

		// Make this the last check, since it makes a string.
		// Don't swat server ragdolls! -TERO: why the hell not? -because it makes the game crash, dumbass! -oh... sorry...
		if ( FClassnameIs( pList[ i ], "physics_prop_ragdoll" ) )
			continue;
			
		if ( FClassnameIs( pList[ i ], "prop_ragdoll" ) )
			continue;

		// The object must also be closer to the zombie than it is to the enemy
		pNearest = pList[ i ];
		flNearestDist = flDist;
	}

	m_hPhysicsEnt = pNearest;

	if( m_hPhysicsEnt == NULL )
	{
		return false;
	}
	else
	{
		return true;
	}
}

float CNPC_AlienGrunt::DistToPhysicsEnt( void )
{
	//return ( GetLocalOrigin() - m_hPhysicsEnt->GetLocalOrigin() ).Length();
	if ( m_hPhysicsEnt != NULL )
		return UTIL_DistApprox2D( GetAbsOrigin(), m_hPhysicsEnt->WorldSpaceCenter() );
	return ALIENGRUNT_PHYSOBJ_SWATDIST + 1;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_AlienGrunt::GatherConditions( void )
{
	ClearCondition( COND_ALIENGRUNT_LOCAL_MELEE_OBSTRUCTION );

	BaseClass::GatherConditions();

	if( m_NPCState == NPC_STATE_COMBAT  )
	{
		// This check for !m_pPhysicsEnt prevents a crashing bug, but also
		// eliminates the zombie picking a better physics object if one happens to fall
		// between him and the object he's heading for already. 
		if( gpGlobals->curtime >= m_flNextSwatScan && (m_hPhysicsEnt == NULL) )
		{
			FindNearestPhysicsObject( ALIENGRUNT_MAX_PHYSOBJ_MASS );
			m_flNextSwatScan = gpGlobals->curtime + 2.0;
		}
	}

	if( (m_hPhysicsEnt != NULL) && gpGlobals->curtime >= m_flNextSwat && HasCondition( COND_SEE_ENEMY ) )
	{
		SetCondition( COND_ALIENGRUNT_CAN_SWAT_ATTACK );
	}
	else
	{
		ClearCondition( COND_ALIENGRUNT_CAN_SWAT_ATTACK );
	}
}


//------------------------------------------------------------------------------
//
// Schedules
//
//------------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_aliengrunt, CNPC_AlienGrunt )

	DECLARE_USES_SCHEDULE_PROVIDER( CAI_AssaultBehavior )

	DECLARE_TASK( TASK_ALIENGRUNT_SWAT_ITEM )
	DECLARE_TASK( TASK_ALIENGRUNT_GET_PATH_TO_PHYSOBJ )

	DECLARE_ACTIVITY(ACT_ALIENGRUNT_TO_ACTION)
	DECLARE_ACTIVITY(ACT_ALIENGRUNT_TO_IDLE)

	DECLARE_ANIMEVENT( AE_ALIENGRUNT_HAND_LEFT )
	DECLARE_ANIMEVENT( AE_ALIENGRUNT_HAND_RIGHT )
	DECLARE_ANIMEVENT( AE_ALIENGRUNT_SHOOTBEES )
	DECLARE_ANIMEVENT( AE_ALIENGRUNT_SWING_SOUND )

	DECLARE_CONDITION( COND_ALIENGRUNT_CAN_SWAT_ATTACK )
	DECLARE_CONDITION( COND_ALIENGRUNT_LOCAL_MELEE_OBSTRUCTION )


	//=========================================================
	// > SCHED_ALIENGRUNT_RANGE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENGRUNT_SHOOTBEES,

		"	Tasks"
		"		TASK_STOP_MOVING				0" // commented out so that we can choose our anim
		"		TASK_FACE_IDEAL					0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_RANGE_ATTACK1				0"
		"		TASK_WAIT						0.1" // Wait a sec before firing again
		""
		"	Interrupts"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_HEAVY_DAMAGE"
	);

	//=========================================================
	// > SCHED_ALIENGRUNT_MELEE_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENGRUNT_MELEE_ATTACK,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FACE_ENEMY						0"
		"		TASK_MELEE_ATTACK1					ACTIVITY:ACT_MELEE_ATTACK1"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
	);

	//=========================================================
	// > SCHED_ALIENGRUNT_STAND
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENGRUNT_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WAIT							2"					// repick IDLESTAND every two seconds."
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_DANGER"
	);

	//=========================================================
	// > SCHED_ALIENGRUNT_MOVE_SWATITEM
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENGRUNT_MOVE_SWATITEM,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_ALIENGRUNT_GET_PATH_TO_PHYSOBJ	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ALIENGRUNT_SWAT_ITEM			0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	)

	//=========================================================
	// SwatItem
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENGRUNT_SWATITEM,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
		"		TASK_FACE_ENEMY					0"
		"		TASK_ALIENGRUNT_SWAT_ITEM		0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	);

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ALIENGRUNT_ATTACKITEM,

		"	Tasks"
		"		TASK_FACE_ENEMY					0"
		"		TASK_MELEE_ATTACK1				0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
	);
AI_END_CUSTOM_NPC()
