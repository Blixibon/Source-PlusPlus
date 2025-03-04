//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "baseanimating.h"
#include "studio.h"
#include "physics.h"
#include "physics_saverestore.h"
#include "ai_basenpc.h"
#include "vphysics/constraints.h"
#include "datacache/imdlcache.h"
#include "bone_setup.h"
#include "physics_prop_ragdoll.h"
#include "KeyValues.h"
#include "props.h"
#include "RagdollBoogie.h"
#include "AI_Criteria.h"
#include "ragdoll_shared.h"
#include "hierarchy.h"
#include "particle_parse.h"
#ifdef HL2_LAZUL
#include "bms/character_manifest_system.h"
#endif // HL2_LAZUL


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
const char *GetMassEquivalent(float flMass);

#define RAGDOLL_VISUALIZE 0

//-----------------------------------------------------------------------------
// ThinkContext
//-----------------------------------------------------------------------------
const char *s_pFadeOutContext = "RagdollFadeOutContext";
const char *s_pDebrisContext = "DebrisContext";

const float ATTACHED_DAMPING_SCALE = 50.0f;

//-----------------------------------------------------------------------------
// Spawnflags
//-----------------------------------------------------------------------------

#define SF_FULL_DESTROY						0x0001
#define	SF_DONT_IGNITE						0x0002
#define	SF_DESTROY_BY_PIECE					0x0008
#define	SF_DISABLE_MOTION_IF_NOT_MOVING		0x0010
#define	SF_ALLOW_BULLET_DAMAGE				0x0020
#define SF_NO_SELF_COLLISION				0x0040

#define	SF_RAGDOLLPROP_DEBRIS		0x0004
#define SF_RAGDOLLPROP_USE_LRU_RETIREMENT	0x1000
#define	SF_RAGDOLLPROP_ALLOW_DISSOLVE		0x2000	// Allow this prop to be dissolved
#define	SF_RAGDOLLPROP_MOTIONDISABLED		0x4000
#define	SF_RAGDOLLPROP_ALLOW_STRETCH		0x8000
#define	SF_RAGDOLLPROP_STARTASLEEP			0x10000

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( physics_prop_ragdoll, CRagdollProp );
LINK_ENTITY_TO_CLASS( prop_ragdoll, CRagdollProp );
EXTERN_SEND_TABLE(DT_Ragdoll)

IMPLEMENT_SERVERCLASS_ST(CRagdollProp, DT_Ragdoll)
	SendPropArray	(SendPropQAngles(SENDINFO_ARRAY(m_ragAngles), 13, 0 ), m_ragAngles),
	SendPropArray	(SendPropVector(SENDINFO_ARRAY(m_ragPos), -1, SPROP_COORD ), m_ragPos),
	SendPropEHandle(SENDINFO( m_hUnragdoll ) ),
	SendPropFloat(SENDINFO(m_flBlendWeight), 8, SPROP_ROUNDDOWN, 0.0f, 1.0f ),
	SendPropInt(SENDINFO(m_nOverlaySequence), 11),
END_SEND_TABLE()

#define DEFINE_RAGDOLL_ELEMENT( i ) \
	DEFINE_FIELD( m_ragdoll.list[i].originParentSpace, FIELD_VECTOR ), \
	DEFINE_PHYSPTR( m_ragdoll.list[i].pObject ), \
	DEFINE_PHYSPTR( m_ragdoll.list[i].pConstraint ), \
	DEFINE_FIELD( m_ragdoll.list[i].parentIndex, FIELD_INTEGER )

#pragma warning( push )
#pragma warning( disable : 4838 )
BEGIN_DATADESC(CRagdollProp)
//					m_ragdoll (custom handling)
	DEFINE_AUTO_ARRAY	( m_ragdoll.boneIndex,	FIELD_INTEGER	),
	DEFINE_AUTO_ARRAY	( m_ragPos,		FIELD_POSITION_VECTOR	),
	DEFINE_AUTO_ARRAY	( m_ragAngles,	FIELD_VECTOR	),
	DEFINE_KEYFIELD(m_anglesOverrideString,	FIELD_STRING, "angleOverride" ),
	DEFINE_FIELD( m_lastUpdateTickCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_allAsleep, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hDamageEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hKiller, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartRagdollBoogie", InputStartRadgollBoogie ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMotion", InputEnableMotion ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMotion", InputDisableMotion ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",		InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",	InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "FadeAndRemove", InputFadeAndRemove ),

	DEFINE_FIELD( m_hUnragdoll, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bFirstCollisionAfterLaunch, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flBlendWeight, FIELD_FLOAT ),
	DEFINE_FIELD( m_nOverlaySequence, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_ragdollMins, FIELD_VECTOR ),
	DEFINE_AUTO_ARRAY( m_ragdollMaxs, FIELD_VECTOR ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),
	DEFINE_FIELD( m_flFadeOutStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_flFadeTime,	FIELD_FLOAT),
	DEFINE_FIELD( m_strSourceClassName, FIELD_STRING ),
	DEFINE_FIELD( m_bHasBeenPhysgunned, FIELD_BOOLEAN ),

	// think functions
	DEFINE_THINKFUNC( SetDebrisThink ),
	DEFINE_THINKFUNC( ClearFlagsThink ),
	DEFINE_THINKFUNC( FadeOutThink ),

	DEFINE_FIELD( m_ragdoll.listCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_ragdoll.allowStretch, FIELD_BOOLEAN ),
	DEFINE_PHYSPTR( m_ragdoll.pGroup ),
	DEFINE_FIELD( m_flDefaultFadeScale, FIELD_FLOAT ),

	//DEFINE_RAGDOLL_ELEMENT( 0 ),
	DEFINE_RAGDOLL_ELEMENT( 1 ),
	DEFINE_RAGDOLL_ELEMENT( 2 ),
	DEFINE_RAGDOLL_ELEMENT( 3 ),
	DEFINE_RAGDOLL_ELEMENT( 4 ),
	DEFINE_RAGDOLL_ELEMENT( 5 ),
	DEFINE_RAGDOLL_ELEMENT( 6 ),
	DEFINE_RAGDOLL_ELEMENT( 7 ),
	DEFINE_RAGDOLL_ELEMENT( 8 ),
	DEFINE_RAGDOLL_ELEMENT( 9 ),
	DEFINE_RAGDOLL_ELEMENT( 10 ),
	DEFINE_RAGDOLL_ELEMENT( 11 ),
	DEFINE_RAGDOLL_ELEMENT( 12 ),
	DEFINE_RAGDOLL_ELEMENT( 13 ),
	DEFINE_RAGDOLL_ELEMENT( 14 ),
	DEFINE_RAGDOLL_ELEMENT( 15 ),
	DEFINE_RAGDOLL_ELEMENT( 16 ),
	DEFINE_RAGDOLL_ELEMENT( 17 ),
	DEFINE_RAGDOLL_ELEMENT( 18 ),
	DEFINE_RAGDOLL_ELEMENT( 19 ),
	DEFINE_RAGDOLL_ELEMENT( 20 ),
	DEFINE_RAGDOLL_ELEMENT( 21 ),
	DEFINE_RAGDOLL_ELEMENT( 22 ),
	DEFINE_RAGDOLL_ELEMENT( 23 ),
	DEFINE_RAGDOLL_ELEMENT( 24 ),
	DEFINE_RAGDOLL_ELEMENT( 25 ),
	DEFINE_RAGDOLL_ELEMENT( 26 ),
	DEFINE_RAGDOLL_ELEMENT( 27 ),
	DEFINE_RAGDOLL_ELEMENT( 28 ),
	DEFINE_RAGDOLL_ELEMENT( 29 ),
	DEFINE_RAGDOLL_ELEMENT( 30 ),
	DEFINE_RAGDOLL_ELEMENT( 31 ),
	DEFINE_RAGDOLL_ELEMENT( 32 ),
	DEFINE_RAGDOLL_ELEMENT( 33 ),
	DEFINE_RAGDOLL_ELEMENT( 34 ),
	DEFINE_RAGDOLL_ELEMENT( 35 ),
	DEFINE_RAGDOLL_ELEMENT( 36 ),
	DEFINE_RAGDOLL_ELEMENT( 37 ),
	DEFINE_RAGDOLL_ELEMENT( 38 ),
	DEFINE_RAGDOLL_ELEMENT( 39 ),
	DEFINE_RAGDOLL_ELEMENT( 41 ),
	DEFINE_RAGDOLL_ELEMENT( 42 ),
	DEFINE_RAGDOLL_ELEMENT( 43 ),
	DEFINE_RAGDOLL_ELEMENT( 44 ),
	DEFINE_RAGDOLL_ELEMENT( 45 ),
	DEFINE_RAGDOLL_ELEMENT( 46 ),
	DEFINE_RAGDOLL_ELEMENT( 47 ),
	DEFINE_RAGDOLL_ELEMENT( 48 ),
	DEFINE_RAGDOLL_ELEMENT( 49 ),
	DEFINE_RAGDOLL_ELEMENT( 50 ),
	DEFINE_RAGDOLL_ELEMENT( 51 ),
	DEFINE_RAGDOLL_ELEMENT( 52 ),
	DEFINE_RAGDOLL_ELEMENT( 53 ),
	DEFINE_RAGDOLL_ELEMENT( 54 ),
	DEFINE_RAGDOLL_ELEMENT( 55 ),
	DEFINE_RAGDOLL_ELEMENT( 56 ),
	DEFINE_RAGDOLL_ELEMENT( 57 ),
	DEFINE_RAGDOLL_ELEMENT( 58 ),
	DEFINE_RAGDOLL_ELEMENT( 59 ),
	DEFINE_RAGDOLL_ELEMENT( 60 ),
	DEFINE_RAGDOLL_ELEMENT( 61 ),
	DEFINE_RAGDOLL_ELEMENT( 62 ),
	DEFINE_RAGDOLL_ELEMENT( 63 ),
	DEFINE_RAGDOLL_ELEMENT( 64 ),
	DEFINE_RAGDOLL_ELEMENT( 65 ),
	DEFINE_RAGDOLL_ELEMENT( 66 ),
	DEFINE_RAGDOLL_ELEMENT( 67 ),
	DEFINE_RAGDOLL_ELEMENT( 68 ),
	DEFINE_RAGDOLL_ELEMENT( 69 ),
	DEFINE_RAGDOLL_ELEMENT( 70 ),
	DEFINE_RAGDOLL_ELEMENT( 71 ),
	DEFINE_RAGDOLL_ELEMENT( 72 ),
	DEFINE_RAGDOLL_ELEMENT( 73 ),
	DEFINE_RAGDOLL_ELEMENT( 74 ),
	DEFINE_RAGDOLL_ELEMENT( 75 ),
	DEFINE_RAGDOLL_ELEMENT( 76 ),
	DEFINE_RAGDOLL_ELEMENT( 77 ),
	DEFINE_RAGDOLL_ELEMENT( 78 ),
	DEFINE_RAGDOLL_ELEMENT( 79 ),
	DEFINE_RAGDOLL_ELEMENT( 80 ),
	DEFINE_RAGDOLL_ELEMENT( 81 ),
	DEFINE_RAGDOLL_ELEMENT( 82 ),
	DEFINE_RAGDOLL_ELEMENT( 83 ),
	DEFINE_RAGDOLL_ELEMENT( 84 ),
	DEFINE_RAGDOLL_ELEMENT( 85 ),
	DEFINE_RAGDOLL_ELEMENT( 86 ),
	DEFINE_RAGDOLL_ELEMENT( 87 ),
	DEFINE_RAGDOLL_ELEMENT( 88 ),
	DEFINE_RAGDOLL_ELEMENT( 89 ),
	DEFINE_RAGDOLL_ELEMENT( 90 ),
	DEFINE_RAGDOLL_ELEMENT( 91 ),
	DEFINE_RAGDOLL_ELEMENT( 92 ),
	DEFINE_RAGDOLL_ELEMENT( 93 ),
	DEFINE_RAGDOLL_ELEMENT( 94 ),
	DEFINE_RAGDOLL_ELEMENT( 95 ),
	DEFINE_RAGDOLL_ELEMENT( 96 ),
	DEFINE_RAGDOLL_ELEMENT( 97 ),
	DEFINE_RAGDOLL_ELEMENT( 98 ),
	DEFINE_RAGDOLL_ELEMENT( 99 ),
	DEFINE_RAGDOLL_ELEMENT( 100 ),
	DEFINE_RAGDOLL_ELEMENT( 101 ),
	DEFINE_RAGDOLL_ELEMENT( 102 ),
	DEFINE_RAGDOLL_ELEMENT( 103 ),
	DEFINE_RAGDOLL_ELEMENT( 104 ),
	DEFINE_RAGDOLL_ELEMENT( 105 ),
	DEFINE_RAGDOLL_ELEMENT( 106 ),
	DEFINE_RAGDOLL_ELEMENT( 107 ),
	DEFINE_RAGDOLL_ELEMENT( 108 ),
	DEFINE_RAGDOLL_ELEMENT( 109 ),
	DEFINE_RAGDOLL_ELEMENT( 110 ),
	DEFINE_RAGDOLL_ELEMENT( 111 ),
	DEFINE_RAGDOLL_ELEMENT( 112 ),
	DEFINE_RAGDOLL_ELEMENT( 113 ),
	DEFINE_RAGDOLL_ELEMENT( 114 ),
	DEFINE_RAGDOLL_ELEMENT( 115 ),
	DEFINE_RAGDOLL_ELEMENT( 116 ),
	DEFINE_RAGDOLL_ELEMENT( 117 ),
	DEFINE_RAGDOLL_ELEMENT( 118 ),
	DEFINE_RAGDOLL_ELEMENT( 119 ),
	DEFINE_RAGDOLL_ELEMENT( 120 ),
	DEFINE_RAGDOLL_ELEMENT( 121 ),
	DEFINE_RAGDOLL_ELEMENT( 122 ),
	DEFINE_RAGDOLL_ELEMENT( 123 ),
	DEFINE_RAGDOLL_ELEMENT( 124 ),
	DEFINE_RAGDOLL_ELEMENT( 125 ),
	DEFINE_RAGDOLL_ELEMENT( 126 ),
	DEFINE_RAGDOLL_ELEMENT( 127 ),
END_DATADESC()
#pragma warning( pop )

//-----------------------------------------------------------------------------
// Disable auto fading under dx7 or when level fades are specified
//-----------------------------------------------------------------------------
void CRagdollProp::DisableAutoFade()
{
	m_flFadeScale = 0;
	m_flDefaultFadeScale = 0;
}


void CRagdollProp::Spawn( void )
{
	const CharacterManifest::ManifestCharacter_t* pChar = nullptr;
	string_t iszName = GetEntityName();
	if (iszName != NULL_STRING)
	{
		pChar = GetCharacterManifest()->FindCharacterModel(STRING(iszName));
	}

	if (pChar)
		SetModelName(AllocPooledString(CharacterManifest::GetScriptModel(pChar, STRING(GetModelName()))));

	// Starts out as the default fade scale value
	m_flDefaultFadeScale = m_flFadeScale;

	// NOTE: If this fires, then the assert or the datadesc is wrong!  (see DEFINE_RAGDOLL_ELEMENT above)
	Assert( RAGDOLL_MAX_ELEMENTS == 128 );
	Precache();
	SetModel( STRING( GetModelName() ) );

	CStudioHdr *pStudioHdr = GetModelPtr( );
	if ( pStudioHdr->flags() & STUDIOHDR_FLAGS_NO_FORCED_FADE )
	{
		DisableAutoFade();
	}
	else
	{
		m_flFadeScale = m_flDefaultFadeScale;
	}

	matrix3x4a_t pBoneToWorld[MAXSTUDIOBONES];
	BaseClass::SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING ); // FIXME: shouldn't this be a subset of the bones
	// this is useless info after the initial conditions are set
	SetAbsAngles( vec3_angle );
	int collisionGroup = (m_spawnflags & SF_RAGDOLLPROP_DEBRIS) ? COLLISION_GROUP_DEBRIS : COLLISION_GROUP_NONE;

	bool bWake;

	if( FClassnameIs(this, "prop_dynamically_destructible") )
	{
		bWake = false;
	}
	else
	{
		bWake = (m_spawnflags & SF_RAGDOLLPROP_STARTASLEEP) ? false : true;
	}

	InitRagdoll( vec3_origin, 0, vec3_origin, pBoneToWorld, pBoneToWorld, 0, collisionGroup, true, bWake );
	m_lastUpdateTickCount = 0;
	m_flBlendWeight = 0.0f;
	m_nOverlaySequence = -1;

	// Unless specified, do not allow this to be dissolved
	if ( HasSpawnFlags( SF_RAGDOLLPROP_ALLOW_DISSOLVE ) == false )
	{
		AddEFlags( EFL_NO_DISSOLVE );
	}

	if ( HasSpawnFlags(SF_RAGDOLLPROP_MOTIONDISABLED) || FClassnameIs( this, "prop_dynamically_destructible") )
	{
		DisableMotion();
	}

	if( m_bStartDisabled )
	{
		AddEffects( EF_NODRAW );
	}

	if( FClassnameIs(this, "prop_dynamically_destructible") )	//I love you, Gabe, pls hire me :-*
	{
		Vector vecFullMins, vecFullMaxs;
		vecFullMins = m_ragPos[0];
		vecFullMaxs = m_ragPos[0];
		for (int i = 0; i < m_ragdoll.listCount; i++ )
		{
			Vector mins, maxs;
			matrix3x4_t update;
			if ( !m_ragdoll.list[i].pObject )
			{
				m_ragdollMins[i].Init();
				m_ragdollMaxs[i].Init();
				continue;
			}
			m_ragdoll.list[i].pObject->GetPositionMatrix( &update );
			TransformAABB( update, m_ragdollMins[i], m_ragdollMaxs[i], mins, maxs );
			for ( int j = 0; j < 3; j++ )
			{
				if ( mins[j] < vecFullMins[j] )
				{
					vecFullMins[j] = mins[j];
				}
				if ( maxs[j] > vecFullMaxs[j] )
				{
					vecFullMaxs[j] = maxs[j];
				}
			}
		}

		SetAbsOrigin( m_ragPos[0] );
		SetAbsAngles( vec3_angle );
		const Vector &vecOrigin = CollisionProp()->GetCollisionOrigin();
		CollisionProp()->AddSolidFlags( FSOLID_FORCE_WORLD_ALIGNED );
		CollisionProp()->SetSurroundingBoundsType( USE_COLLISION_BOUNDS_NEVER_VPHYSICS );
		SetCollisionBounds( vecFullMins - vecOrigin, vecFullMaxs - vecOrigin );
	}

	if (pChar)
	{
		pChar->ApplyToModel(GetModelPtr(), m_nSkin.GetForModify(), m_nBody.GetForModify());

		for (int i = 0; i < pChar->vMergedModels.Count(); i++)
		{
			CDynamicProp* pProp = static_cast<CDynamicProp*>(CreateEntityByName("prop_dynamic_override"));
			if (pProp != NULL)
			{
				// Set the model
				pProp->SetModelName(AllocPooledString(pChar->vMergedModels[i].String()));
				pProp->SetAbsOrigin(GetAbsOrigin());
				pProp->SetOwnerEntity(this);
				DispatchSpawn(pProp);
				pProp->FollowEntity(this, true);
			}
		}
	}
}

void CRagdollProp::SetSourceClassName( const char *pClassname )
{
	m_strSourceClassName = MAKE_STRING( pClassname );
}


void CRagdollProp::OnSave( IEntitySaveUtils *pUtils )
{
	if ( !m_ragdoll.listCount )
		return;

	// Don't save ragdoll element 0, base class saves the pointer in
	// m_pPhysicsObject
	Assert( m_ragdoll.list[0].parentIndex == -1 );
	Assert( m_ragdoll.list[0].pConstraint == NULL );
	Assert( m_ragdoll.list[0].originParentSpace == vec3_origin );
	Assert( m_ragdoll.list[0].pObject != NULL );
	VPhysicsSetObject( NULL );	// squelch a warning message
	VPhysicsSetObject( m_ragdoll.list[0].pObject );	// make sure object zero is saved by CBaseEntity
	BaseClass::OnSave( pUtils );
}

void CRagdollProp::OnRestore()
{
	// rebuild element 0 since it isn't saved
	// NOTE: This breaks the rules - the pointer needs to get fixed in Restore()
	m_ragdoll.list[0].pObject = VPhysicsGetObject();
	m_ragdoll.list[0].parentIndex = -1;
	m_ragdoll.list[0].originParentSpace.Init();

	BaseClass::OnRestore();
	if ( !m_ragdoll.listCount )
		return;

	// JAY: Reset collision relationships
	RagdollSetupCollisions( m_ragdoll, modelinfo->GetVCollide( GetModelIndex() ), GetModelIndex() );
	VPhysicsUpdate( VPhysicsGetObject() );
}

void CRagdollProp::CalcRagdollSize( void )
{
	CollisionProp()->SetSurroundingBoundsType( USE_HITBOXES );
	CollisionProp()->RemoveSolidFlags( FSOLID_FORCE_WORLD_ALIGNED );
}

void CRagdollProp::UpdateOnRemove( void )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( m_ragdoll.list[i].pObject )
		{
			g_pPhysSaveRestoreManager->ForgetModel( m_ragdoll.list[i].pObject );
		}
	}

	// Set to null so that the destructor's call to DestroyObject won't destroy
	//  m_pObjects[ 0 ] twice since that's the physics object for the prop
	VPhysicsSetObject( NULL );

	RagdollDestroy( m_ragdoll );
	// Chain to base after doing our own cleanup to mimic
	//  destructor unwind order
	BaseClass::UpdateOnRemove();
}

CRagdollProp::CRagdollProp( void )
{
	m_strSourceClassName = NULL_STRING;
	m_anglesOverrideString = NULL_STRING;
	m_ragdoll.listCount = 0;
	Assert( (1<<RAGDOLL_INDEX_BITS) >=RAGDOLL_MAX_ELEMENTS );
	m_allAsleep = false;
	m_flFadeScale = 1;
	m_flDefaultFadeScale = 1;
}

CRagdollProp::~CRagdollProp( void )
{
}

void CRagdollProp::Precache( void )
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

int CRagdollProp::ObjectCaps()
{
	return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRagdollProp::InitRagdollAnimation()
{
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle( 0 );

	// put into ACT_DIERAGDOLL if it exists, otherwise use sequence 0
	int nSequence = SelectWeightedSequence( ACT_DIERAGDOLL );
	if ( nSequence < 0 )
	{
		ResetSequence( 0 );
	}
	else
	{
		ResetSequence( nSequence );
	}
}


//-----------------------------------------------------------------------------
// Response system stuff
//-----------------------------------------------------------------------------
ResponseRules::IResponseSystem *CRagdollProp::GetResponseSystem()
{
	extern ResponseRules::IResponseSystem *g_pResponseSystem;

	// Just use the general NPC response system; we often come from NPCs after all
	return g_pResponseSystem;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRagdollProp::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	BaseClass::ModifyOrAppendCriteria( set );

	if ( m_strSourceClassName != NULL_STRING )
	{
		set.RemoveCriteria( "classname" );
		set.AppendCriteria( "classname", STRING(m_strSourceClassName) );
		set.AppendCriteria( "ragdoll", "1" );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRagdollProp::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	CDefaultPlayerPickupVPhysics::OnPhysGunPickup(pPhysGunUser, reason);

	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	// Clear out the classname if we've been physgunned before
	// so that the screams, etc. don't happen. Simulate that the first
	// major punt or throw has been enough to kill him.
	if ( m_bHasBeenPhysgunned )
	{
		m_strSourceClassName = NULL_STRING;
	}
	m_bHasBeenPhysgunned = true;

	if( HasPhysgunInteraction( "onpickup", "boogie" ) )
	{
		if ( reason == PUNTED_BY_CANNON )
		{
			CRagdollBoogie::Create( this, 150, gpGlobals->curtime, 3.0f, SF_RAGDOLL_BOOGIE_ELECTRICAL );
		}
		else
		{
			CRagdollBoogie::Create( this, 150, gpGlobals->curtime, 2.0f, 0.0f );
		}
	}

	if ( HasSpawnFlags( SF_RAGDOLLPROP_USE_LRU_RETIREMENT ) )
	{
		s_RagdollLRU.MoveToTopOfLRU( this );
	}

	if ( !HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON ) )
		return;

	ragdoll_t *pRagdollPhys = GetRagdoll( );
	for ( int j = 0; j < pRagdollPhys->listCount; ++j )
	{
		pRagdollPhys->list[j].pObject->Wake();
		pRagdollPhys->list[j].pObject->EnableMotion( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRagdollProp::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	CDefaultPlayerPickupVPhysics::OnPhysGunDrop( pPhysGunUser, Reason );
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	if( HasPhysgunInteraction( "onpickup", "boogie" ) )
	{
		CRagdollBoogie::Create( this, 150, gpGlobals->curtime, 3.0f, SF_RAGDOLL_BOOGIE_ELECTRICAL );
	}

	if ( HasSpawnFlags( SF_RAGDOLLPROP_USE_LRU_RETIREMENT ) )
	{
		s_RagdollLRU.MoveToTopOfLRU( this );
	}

	// Make sure it's interactive debris for at most 5 seconds
	if ( GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
	{
		SetContextThink( &CRagdollProp::SetDebrisThink, gpGlobals->curtime + 5, s_pDebrisContext );
	}

	if ( Reason != LAUNCHED_BY_CANNON )
		return;

	if( HasPhysgunInteraction( "onlaunch", "spin_zaxis" ) )
	{
		Vector vecAverageCenter( 0, 0, 0 );

		// Get the average position, apply forces to produce a spin
		int j;
		ragdoll_t *pRagdollPhys = GetRagdoll( );
		for ( j = 0; j < pRagdollPhys->listCount; ++j )
		{
			Vector vecCenter;
			pRagdollPhys->list[j].pObject->GetPosition( &vecCenter, NULL );
			vecAverageCenter += vecCenter;
		}

		vecAverageCenter /= pRagdollPhys->listCount;

		Vector vecZAxis( 0, 0, 1 );
		for ( j = 0; j < pRagdollPhys->listCount; ++j )
		{
			Vector vecDelta;
			pRagdollPhys->list[j].pObject->GetPosition( &vecDelta, NULL );
			vecDelta -= vecAverageCenter;

			Vector vecDir;
			CrossProduct( vecZAxis, vecDelta, vecDir );
			vecDir *= 100;
			pRagdollPhys->list[j].pObject->AddVelocity( &vecDir, NULL );
		}
	}

	PhysSetGameFlags( VPhysicsGetObject(), FVPHYSICS_WAS_THROWN );
	m_bFirstCollisionAfterLaunch = true;
}


//-----------------------------------------------------------------------------
// Physics attacker
//-----------------------------------------------------------------------------
CBasePlayer *CRagdollProp::HasPhysicsAttacker( float dt )
{
	if (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime)
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRagdollProp::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	CBaseEntity *pHitEntity = pEvent->pEntities[!index];
	if ( pHitEntity == this )
		return;

	// Don't take physics damage from whoever's holding him with the physcannon.
	if ( VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD) )
	{
		if ( pHitEntity && (pHitEntity == HasPhysicsAttacker( FLT_MAX )) )
			return;
	}

	// Don't bother taking damage from the physics attacker
	if ( pHitEntity && HasPhysicsAttacker( 0.5f ) == pHitEntity )
		return;

	if( m_bFirstCollisionAfterLaunch )
	{
		HandleFirstCollisionInteractions( index, pEvent );
	}

	if ( m_takedamage != DAMAGE_NO )
	{
		int damageType = 0;
		float damage = CalculateDefaultPhysicsDamage( index, pEvent, 1.0f, true, damageType );
		if ( damage > 0 )
		{
			// Take extra damage after we're punted by the physcannon
			if ( m_bFirstCollisionAfterLaunch )
			{
				damage *= 10;
			}

			CBaseEntity *pHitEntity = pEvent->pEntities[!index];
			if ( !pHitEntity )
			{
				// hit world
				pHitEntity = GetContainingEntity( INDEXENT(0) );
			}
			Vector damagePos;
			pEvent->pInternalData->GetContactPoint( damagePos );
			Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
			if ( damageForce == vec3_origin )
			{
				// This can happen if this entity is motion disabled, and can't move.
				// Use the velocity of the entity that hit us instead.
				damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
			}

			// FIXME: this doesn't pass in who is responsible if some other entity "caused" this collision
			PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ), *pEvent, index );
		}
	}

	if ( m_bFirstCollisionAfterLaunch )
	{
		// Setup the think function to remove the flags
		SetThink( &CRagdollProp::ClearFlagsThink );
		SetNextThink( gpGlobals->curtime );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CRagdollProp::HasPhysgunInteraction( const char *pszKeyName, const char *pszValue )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		KeyValues *pkvPropData = modelKeyValues->FindKey("physgun_interactions");
		if ( pkvPropData )
		{
			char const *pszBase = pkvPropData->GetString( pszKeyName );

			if ( pszBase && pszBase[0] && !stricmp( pszBase, pszValue ) )
			{
				modelKeyValues->deleteThis();
				return true;
			}
		}
	}

	modelKeyValues->deleteThis();
	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CRagdollProp::HandleFirstCollisionInteractions( int index, gamevcollisionevent_t *pEvent )
{
	IPhysicsObject *pObj = VPhysicsGetObject();
	if ( !pObj)
		return;

	if( HasPhysgunInteraction( "onfirstimpact", "break" ) )
	{
		// Looks like it's best to break by having the object damage itself.
		CTakeDamageInfo info;

		info.SetDamage( m_iHealth );
		info.SetAttacker( this );
		info.SetInflictor( this );
		info.SetDamageType( DMG_GENERIC );

		Vector vecPosition;
		Vector vecVelocity;

		VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );
		VPhysicsGetObject()->GetPosition( &vecPosition, NULL );

		info.SetDamageForce( vecVelocity );
		info.SetDamagePosition( vecPosition );

		TakeDamage( info );
		return;
	}

	if( HasPhysgunInteraction( "onfirstimpact", "paintsplat" ) )
	{
		IPhysicsObject *pObj = VPhysicsGetObject();

		Vector vecPos;
		pObj->GetPosition( &vecPos, NULL );

		trace_t tr;
		UTIL_TraceLine( vecPos, vecPos + pEvent->preVelocity[0] * 1.5, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		switch( random->RandomInt( 1, 3 ) )
		{
		case 1:
			UTIL_DecalTrace( &tr, "PaintSplatBlue" );
			break;

		case 2:
			UTIL_DecalTrace( &tr, "PaintSplatGreen" );
			break;

		case 3:
			UTIL_DecalTrace( &tr, "PaintSplatPink" );
			break;
		}
	}

	bool bAlienBloodSplat = HasPhysgunInteraction( "onfirstimpact", "alienbloodsplat" );
	if( bAlienBloodSplat || HasPhysgunInteraction( "onfirstimpact", "bloodsplat" ) )
	{
		IPhysicsObject *pObj = VPhysicsGetObject();

		Vector vecPos;
		pObj->GetPosition( &vecPos, NULL );

		trace_t tr;
		UTIL_TraceLine( vecPos, vecPos + pEvent->preVelocity[0] * 1.5, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		UTIL_BloodDecalTrace( &tr, bAlienBloodSplat ? BLOOD_COLOR_GREEN : BLOOD_COLOR_RED );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRagdollProp::ClearFlagsThink( void )
{
	PhysClearGameFlags( VPhysicsGetObject(), FVPHYSICS_WAS_THROWN );
	m_bFirstCollisionAfterLaunch = false;
	SetThink( NULL );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AngularImpulse CRagdollProp::PhysGunLaunchAngularImpulse()
{
	if( HasPhysgunInteraction( "onlaunch", "spin_zaxis" ) )
	{
		// Don't add in random angular impulse if this object is supposed to spin in a specific way.
		AngularImpulse ang( 0, 0, 0 );
		return ang;
	}

	return CDefaultPlayerPickupVPhysics::PhysGunLaunchAngularImpulse();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : activity -
//-----------------------------------------------------------------------------
void CRagdollProp::SetOverlaySequence( Activity activity )
{
	int seq = SelectWeightedSequence( activity );
	if ( seq < 0 )
	{
		m_nOverlaySequence = -1;
	}
	else
	{
		m_nOverlaySequence = seq;
	}
}

void CRagdollProp::InitRagdoll( const Vector &forceVector, int forceBone, const Vector &forcePos, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, bool activateRagdoll, bool bWakeRagdoll )
{
	if( FClassnameIs( this, "prop_dynamically_destructible" ) && HasSpawnFlags( SF_NO_SELF_COLLISION ) )
	{
		SetCollisionGroup( COLLISION_GROUP_INTERACTIVE ); //SAVE THE FPS
	}
	else
	{
		SetCollisionGroup( collisionGroup );
	}

	// Make sure it's interactive debris for at most 5 seconds
	if ( collisionGroup == COLLISION_GROUP_INTERACTIVE_DEBRIS )
	{
		SetContextThink( &CRagdollProp::SetDebrisThink, gpGlobals->curtime + 5, s_pDebrisContext );
	}

	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	m_takedamage = DAMAGE_EVENTS_ONLY;

	ragdollparams_t params;
	params.pGameData = static_cast<void *>( static_cast<CBaseEntity *>(this) );
	params.modelIndex = GetModelIndex();
	params.pCollide = modelinfo->GetVCollide( params.modelIndex );
	params.pStudioHdr = GetModelPtr();
	params.forceVector = forceVector;
	params.forceBoneIndex = forceBone;
	params.forcePosition = forcePos;
	params.pCurrentBones = pBoneToWorld;
	params.jointFrictionScale = 1.0;
	params.allowStretch = HasSpawnFlags(SF_RAGDOLLPROP_ALLOW_STRETCH);
	params.fixedConstraints = false;
	//SNEAKY PEAKY LIKE!
	if( !FClassnameIs( this, "prop_dynamically_destructible" ) )
	{
		RagdollCreate( m_ragdoll, params, physenv );
	}
	else
	{
		RagdollCreateDestr( m_ragdoll, params, physenv );
	}

	RagdollApplyAnimationAsVelocity( m_ragdoll, pPrevBones, pBoneToWorld, dt );
	if ( m_anglesOverrideString != NULL_STRING && Q_strlen(m_anglesOverrideString.ToCStr()) > 0 )
	{
		char szToken[2048];
		const char *pStr = nexttoken_safe(szToken, STRING(m_anglesOverrideString), ',');
		// anglesOverride is index,angles,index,angles (e.g. "1, 22.5 123.0 0.0, 2, 0 0 0, 3, 0 0 180.0")
		while ( szToken[0] != 0 )
		{
			int objectIndex = atoi(szToken);
			// sanity check to make sure this token is an integer
			Assert( atof(szToken) == ((float)objectIndex) );
			pStr = nexttoken_safe(szToken, pStr, ',');
			Assert( szToken[0] );
			if ( objectIndex >= m_ragdoll.listCount )
			{
				Warning("Bad ragdoll pose in entity %s, model (%s) at %s, model changed?\n", GetDebugName(), GetModelName().ToCStr(), VecToString(GetAbsOrigin()) );
			}
			else if ( szToken[0] != 0 )
			{
				QAngle angles;
				Assert( objectIndex >= 0 && objectIndex < RAGDOLL_MAX_ELEMENTS );
				UTIL_StringToVector( angles.Base(), szToken );
				int boneIndex = m_ragdoll.boneIndex[objectIndex];
				AngleMatrix( angles, pBoneToWorld[boneIndex] );
				const ragdollelement_t &element = m_ragdoll.list[objectIndex];
				Vector out;
				if ( element.parentIndex >= 0 )
				{
					int parentBoneIndex = m_ragdoll.boneIndex[element.parentIndex];
					VectorTransform( element.originParentSpace, pBoneToWorld[parentBoneIndex], out );
				}
				else
				{
					out = GetAbsOrigin();
				}
				MatrixSetColumn( out, 3, pBoneToWorld[boneIndex] );
				element.pObject->SetPositionMatrix( pBoneToWorld[boneIndex], true );
			}
			pStr = nexttoken_safe(szToken, pStr, ',');
		}
	}

	if ( activateRagdoll )
	{
		MEM_ALLOC_CREDIT();

		if( !FClassnameIs( this, "prop_dynamically_destructible" ) )
		{
			RagdollActivate( m_ragdoll, params.pCollide, GetModelIndex(), bWakeRagdoll );
		}
		else
		{
			RagdollActivateDestr( m_ragdoll, params.pCollide, GetModelIndex(), bWakeRagdoll );
		}
	}

	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		UpdateNetworkDataFromVPhysics( m_ragdoll.list[i].pObject, i );
		g_pPhysSaveRestoreManager->AssociateModel( m_ragdoll.list[i].pObject, GetModelIndex() );
		physcollision->CollideGetAABB( &m_ragdollMins[i], &m_ragdollMaxs[i], m_ragdoll.list[i].pObject->GetCollide(), vec3_origin, vec3_angle );
	}
	VPhysicsSetObject( m_ragdoll.list[0].pObject );

	CalcRagdollSize();
}

void CRagdollProp::SetDebrisThink()
{
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	RecheckCollisionFilter();
}

void CRagdollProp::SetDamageEntity( CBaseEntity *pEntity )
{
	// Damage passing
	m_hDamageEntity = pEntity;

	// Set our takedamage to match it
	if ( pEntity )
	{
		m_takedamage = pEntity->m_takedamage;
	}
	else
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CRagdollProp::OnTakeDamage( const CTakeDamageInfo &info )
{
	// If we have a damage entity, we want to pass damage to it. Add the
	// Never Ragdoll flag, on the assumption that if the entity dies, we'll
	// actually be taking the role of its ragdoll.
	if ( m_hDamageEntity.Get() )
	{
		CTakeDamageInfo subInfo = info;
		subInfo.AddDamageType( DMG_REMOVENORAGDOLL );
		return m_hDamageEntity->OnTakeDamage( subInfo );
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: Force all the ragdoll's bone's physics objects to recheck their collision filters
//-----------------------------------------------------------------------------
void CRagdollProp::RecheckCollisionFilter( void )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		m_ragdoll.list[i].pObject->RecheckCollisionFilter();
	}
}


void CRagdollProp::TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( ptr->physicsbone >= 0 && ptr->physicsbone < m_ragdoll.listCount )
	{
		VPhysicsSwapObject( m_ragdoll.list[ptr->physicsbone].pObject );
	}
	BaseClass::TraceAttack( info, dir, ptr, pAccumulator );
}

void CRagdollProp::SetupBones( matrix3x4a_t *pBoneToWorld, int boneMask )
{
	// no ragdoll, fall through to base class
	if ( !m_ragdoll.listCount )
	{
		BaseClass::SetupBones( pBoneToWorld, boneMask );
		return;
	}

	// Not really ideal, but it'll work for now
	UpdateModelScale();

	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr *pStudioHdr = GetModelPtr( );
	bool sim[MAXSTUDIOBONES];
	memset( sim, 0, pStudioHdr->numbones() );

	int i;

	CBoneAccessor boneaccessor( pBoneToWorld );
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		// during restore this may be NULL
		if ( !m_ragdoll.list[i].pObject )
			continue;

		if ( RagdollGetBoneMatrix( m_ragdoll, boneaccessor, i ) )
		{
			sim[m_ragdoll.boneIndex[i]] = true;
		}
	}

	mstudiobone_t *pbones = pStudioHdr->pBone( 0 );
	for ( i = 0; i < pStudioHdr->numbones(); i++ )
	{
		if ( sim[i] )
			continue;

		if ( !(pStudioHdr->boneFlags(i) & boneMask) )
			continue;

		matrix3x4_t matBoneLocal;
		AngleMatrix( pbones[i].rot, pbones[i].pos, matBoneLocal );
		ConcatTransforms( pBoneToWorld[pbones[i].parent], matBoneLocal, pBoneToWorld[i]);
	}
}

bool CRagdollProp::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	//Save the perfomance!
	if( FClassnameIs(this, "prop_dynamically_destructible") )
	{
		// PERFORMANCE: Use hitboxes for rays instead of vcollides if this is a performance problem
		if ( ray.m_IsRay )
		{
			return BaseClass::TestCollision( ray, mask, trace );
		}
	}

	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
		return false;

	// Just iterate all of the elements and trace the box against each one.
	// NOTE: This is pretty expensive for small/dense characters
	trace_t tr;
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		Vector position;
		QAngle angles;

		if( m_ragdoll.list[i].pObject )
		{
			m_ragdoll.list[i].pObject->GetPosition( &position, &angles );
			physcollision->TraceBox( ray, m_ragdoll.list[i].pObject->GetCollide(), position, angles, &tr );

			if ( tr.fraction < trace.fraction )
			{
				tr.physicsbone = i;
				tr.surface.surfaceProps = m_ragdoll.list[i].pObject->GetMaterialIndex();
				trace = tr;
			}
		}
		else
		{
			DevWarning("Bogus object in Ragdoll Prop's ragdoll list!\n");
		}
	}

	if ( trace.fraction >= 1 )
	{
		return false;
	}

	return true;
}


void CRagdollProp::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	// newAngles is a relative transform for the entity
	// But a ragdoll entity has identity orientation by design
	// so we compute a relative transform here based on the previous transform
	matrix3x4_t startMatrixInv;
	MatrixInvert( EntityToWorldTransform(), startMatrixInv );
	matrix3x4_t endMatrix;
	MatrixCopy( EntityToWorldTransform(), endMatrix );
	if ( newAngles )
	{
		AngleMatrix( *newAngles, endMatrix );
	}
	if ( newPosition )
	{
		PositionMatrix( *newPosition, endMatrix );
	}
	// now endMatrix is the refernce matrix for the entity at the target position
	matrix3x4_t xform;
	ConcatTransforms( endMatrix, startMatrixInv, xform );
	// now xform is the relative transform the entity must undergo

	// we need to call the base class and it will teleport our vphysics object,
	// so set object 0 up and compute the origin/angles for its new position (base implementation has side effects)
	VPhysicsSwapObject( m_ragdoll.list[0].pObject );
	matrix3x4_t obj0source, obj0Target;
	m_ragdoll.list[0].pObject->GetPositionMatrix( &obj0source );
	ConcatTransforms( xform, obj0source, obj0Target );
	Vector obj0Pos;
	QAngle obj0Angles;
	MatrixAngles( obj0Target, obj0Angles, obj0Pos );
	BaseClass::Teleport( &obj0Pos, &obj0Angles, newVelocity );

	for ( int i = 1; i < m_ragdoll.listCount; i++ )
	{
		matrix3x4_t matrix, newMatrix;
		m_ragdoll.list[i].pObject->GetPositionMatrix( &matrix );
		ConcatTransforms( xform, matrix, newMatrix );
		m_ragdoll.list[i].pObject->SetPositionMatrix( newMatrix, true );
		UpdateNetworkDataFromVPhysics( m_ragdoll.list[i].pObject, i );
	}
	// fixup/relink object 0
	UpdateNetworkDataFromVPhysics( m_ragdoll.list[0].pObject, 0 );
}

void CRagdollProp::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( m_lastUpdateTickCount == (unsigned int)gpGlobals->tickcount )
		return;

	m_lastUpdateTickCount = gpGlobals->tickcount;
	//NetworkStateChanged();

	matrix3x4a_t boneToWorld[MAXSTUDIOBONES];
	QAngle angles;

	int i;
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		CBoneAccessor boneaccessor( boneToWorld );
		if ( RagdollGetBoneMatrix( m_ragdoll, boneaccessor, i ) )
		{
			Vector vNewPos;
			MatrixAngles( boneToWorld[m_ragdoll.boneIndex[i]], angles, vNewPos );
			m_ragPos.Set( i, vNewPos );
			m_ragAngles.Set( i, angles );
		}
		else
		{
			m_ragPos.GetForModify(i).Init();
			m_ragAngles.GetForModify(i).Init();
		}
	}

	// BUGBUG: Use the ragdollmins/maxs to do this instead of the collides
	m_allAsleep = RagdollIsAsleep( m_ragdoll );

	// Don't scream after you've come to rest
	if ( m_allAsleep )
	{
		m_strSourceClassName = NULL_STRING;
	}
	else
	{
		if ( m_ragdoll.pGroup->IsInErrorState() )
		{
			RagdollSolveSeparation( m_ragdoll, this );
		}
	}

	// Interactive debris converts back to debris when it comes to rest
	if ( m_allAsleep && GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
	{
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		RecheckCollisionFilter();
		SetContextThink( NULL, gpGlobals->curtime, s_pDebrisContext );
	}

	Vector vecFullMins, vecFullMaxs;
	vecFullMins = m_ragPos[0];
	vecFullMaxs = m_ragPos[0];
	for ( i = 0; i < m_ragdoll.listCount; i++ )
	{
		Vector mins, maxs;
		matrix3x4_t update;
		if ( !m_ragdoll.list[i].pObject )
		{
			m_ragdollMins[i].Init();
			m_ragdollMaxs[i].Init();
			continue;
		}
		m_ragdoll.list[i].pObject->GetPositionMatrix( &update );
		TransformAABB( update, m_ragdollMins[i], m_ragdollMaxs[i], mins, maxs );
		for ( int j = 0; j < 3; j++ )
		{
			if ( mins[j] < vecFullMins[j] )
			{
				vecFullMins[j] = mins[j];
			}
			if ( maxs[j] > vecFullMaxs[j] )
			{
				vecFullMaxs[j] = maxs[j];
			}
		}
	}

	SetAbsOrigin( m_ragPos[0] );
	SetAbsAngles( vec3_angle );
	const Vector &vecOrigin = CollisionProp()->GetCollisionOrigin();
	CollisionProp()->AddSolidFlags( FSOLID_FORCE_WORLD_ALIGNED );
	CollisionProp()->SetSurroundingBoundsType( USE_COLLISION_BOUNDS_NEVER_VPHYSICS );
	SetCollisionBounds( vecFullMins - vecOrigin, vecFullMaxs - vecOrigin );
	CollisionProp()->MarkSurroundingBoundsDirty();

	PhysicsTouchTriggers();
}

int CRagdollProp::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( i < listMax )
		{
			pList[i] = m_ragdoll.list[i].pObject;
		}
	}

	return m_ragdoll.listCount;
}

void CRagdollProp::UpdateNetworkDataFromVPhysics( IPhysicsObject *pPhysics, int index )
{
	Assert(index < m_ragdoll.listCount);

	QAngle angles;
	Vector vPos;
	m_ragdoll.list[index].pObject->GetPosition( &vPos, &angles );
	m_ragPos.Set( index, vPos );
	m_ragAngles.Set( index, angles );

	// move/relink if root moved
	if ( index == 0 )
	{
		SetAbsOrigin( m_ragPos[0] );
		PhysicsTouchTriggers();
	}
}


//-----------------------------------------------------------------------------
// Fade out due to the LRU telling it do
//-----------------------------------------------------------------------------
#define FADE_OUT_LENGTH 0.5f

void CRagdollProp::FadeOut( float flDelay, float fadeTime )
{
	if ( IsFading() )
		return;

	m_flFadeTime = ( fadeTime == -1 ) ? FADE_OUT_LENGTH : fadeTime;

	m_flFadeOutStartTime = gpGlobals->curtime + flDelay;
	m_flFadeScale = 0;
	SetContextThink( &CRagdollProp::FadeOutThink, gpGlobals->curtime + flDelay + 0.01f, s_pFadeOutContext );
}

bool CRagdollProp::IsFading()
{
	return ( GetNextThink( s_pFadeOutContext ) >= gpGlobals->curtime );
}

void CRagdollProp::FadeOutThink(void)
{
	float dt = gpGlobals->curtime - m_flFadeOutStartTime;
	if ( dt < 0 )
	{
		SetContextThink( &CRagdollProp::FadeOutThink, gpGlobals->curtime + 0.1, s_pFadeOutContext );
	}
	else if ( dt < m_flFadeTime )
	{
		float alpha = 1.0f - dt / m_flFadeTime;
		int nFade = (int)(alpha * 255.0f);
		m_nRenderMode = kRenderTransTexture;
		SetRenderColorA( nFade );
		NetworkStateChanged();
		SetContextThink( &CRagdollProp::FadeOutThink, gpGlobals->curtime + TICK_INTERVAL, s_pFadeOutContext );
	}
	else
	{
		// Necessary to cause it to do the appropriate death cleanup
		// Yeah, the player may have nothing to do with it, but
		// passing NULL to TakeDamage causes bad things to happen
		CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
		CTakeDamageInfo info( pPlayer, pPlayer, 10000.0, DMG_GENERIC );
		TakeDamage( info );
		UTIL_Remove( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CRagdollProp::DrawDebugTextOverlays(void)
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		if (m_ragdoll.listCount)
		{
			float mass = 0;
			for ( int i = 0; i < m_ragdoll.listCount; i++ )
			{
				if ( m_ragdoll.list[i].pObject != NULL )
				{
					mass += m_ragdoll.list[i].pObject->GetMass();
				}
			}

			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"Mass: %.2f kg / %.2f lb (%s)", mass, kg2lbs(mass), GetMassEquivalent(mass) );
			EntityText( text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}

void CRagdollProp::DrawDebugGeometryOverlays()
{
	if (m_debugOverlays & OVERLAY_BBOX_BIT)
	{
		DrawServerHitboxes();
	}
	if (m_debugOverlays & OVERLAY_PIVOT_BIT)
	{
		for ( int i = 0; i < m_ragdoll.listCount; i++ )
		{
			if ( m_ragdoll.list[i].pObject )
			{
				float mass = m_ragdoll.list[i].pObject->GetMass();
				Vector pos;
				m_ragdoll.list[i].pObject->GetPosition( &pos, NULL );
				CFmtStr str("mass %.1f", mass );
				NDebugOverlay::EntityTextAtPosition( pos, 0, str.Access(), 0, 0, 255, 0, 255 );
			}
		}
	}
	BaseClass::DrawDebugGeometryOverlays();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pOther -
//-----------------------------------------------------------------------------
void CRagdollProp::SetUnragdoll( CBaseAnimating *pOther )
{
	m_hUnragdoll = pOther;
}

//===============================================================================================================
// RagdollPropAttached
//===============================================================================================================
class CRagdollPropAttached : public CRagdollProp
{
	DECLARE_CLASS( CRagdollPropAttached, CRagdollProp );
public:

	CRagdollPropAttached()
	{
		m_bShouldDetach = false;
	}

	~CRagdollPropAttached()
	{
		physenv->DestroyConstraint( m_pAttachConstraint );
		m_pAttachConstraint = NULL;
	}

	void InitRagdollAttached( IPhysicsObject *pAttached, const Vector &forceVector, int forceBone, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, CBaseAnimating *pFollow, int boneIndexRoot, const Vector &boneLocalOrigin, int parentBoneAttach, const Vector &worldAttachOrigin );
	void DetachOnNextUpdate();
	void VPhysicsUpdate( IPhysicsObject *pPhysics );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void Detach();
	CNetworkVar( int, m_boneIndexAttached );
	CNetworkVar( int, m_ragdollAttachedObjectIndex );
	CNetworkVector( m_attachmentPointBoneSpace );
	CNetworkVector( m_attachmentPointRagdollSpace );
	bool		m_bShouldDetach;
	IPhysicsConstraint	*m_pAttachConstraint;
};

LINK_ENTITY_TO_CLASS( prop_ragdoll_attached, CRagdollPropAttached );
EXTERN_SEND_TABLE(DT_Ragdoll_Attached)

IMPLEMENT_SERVERCLASS_ST(CRagdollPropAttached, DT_Ragdoll_Attached)
	SendPropInt( SENDINFO( m_boneIndexAttached ), MAXSTUDIOBONEBITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_ragdollAttachedObjectIndex ), RAGDOLL_INDEX_BITS, SPROP_UNSIGNED ),
	SendPropVector(SENDINFO(m_attachmentPointBoneSpace), -1,  SPROP_COORD ),
	SendPropVector(SENDINFO(m_attachmentPointRagdollSpace), -1,  SPROP_COORD ),
END_SEND_TABLE()

BEGIN_DATADESC(CRagdollPropAttached)
	DEFINE_FIELD( m_boneIndexAttached,	FIELD_INTEGER ),
	DEFINE_FIELD( m_ragdollAttachedObjectIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_attachmentPointBoneSpace,	FIELD_VECTOR ),
	DEFINE_FIELD( m_attachmentPointRagdollSpace, FIELD_VECTOR ),
	DEFINE_FIELD( m_bShouldDetach, FIELD_BOOLEAN ),
	DEFINE_PHYSPTR( m_pAttachConstraint ),
END_DATADESC()


static void SyncAnimatingWithPhysics( CBaseAnimating *pAnimating )
{
	IPhysicsObject *pPhysics = pAnimating->VPhysicsGetObject();
	if ( pPhysics )
	{
		Vector pos;
		pPhysics->GetShadowPosition( &pos, NULL );
		pAnimating->SetAbsOrigin( pos );
	}
}


CBaseAnimating *CreateServerRagdollSubmodel( CBaseAnimating *pOwner, const char *pModelName, const Vector &position, const QAngle &angles, int collisionGroup )
{
	CRagdollProp *pRagdoll = (CRagdollProp *)CBaseEntity::CreateNoSpawn( "prop_ragdoll", position, angles, pOwner );
	pRagdoll->SetModelName( AllocPooledString( pModelName ) );
	pRagdoll->SetModel( STRING(pRagdoll->GetModelName()) );
	matrix3x4a_t pBoneToWorld[MAXSTUDIOBONES], pBoneToWorldNext[MAXSTUDIOBONES];
	pRagdoll->ResetSequence( 0 );

	// let bone merging do the work of copying everything over for us
	pRagdoll->SetParent( pOwner );
	pRagdoll->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );
	// HACKHACK: don't want this parent anymore
	pRagdoll->SetParent( NULL );

	memcpy( pBoneToWorldNext, pBoneToWorld, sizeof(pBoneToWorld) );

	pRagdoll->InitRagdoll( vec3_origin, -1, vec3_origin, pBoneToWorld, pBoneToWorldNext, 0.1, collisionGroup, true );
	return pRagdoll;
}


CBaseEntity *CreateServerRagdoll( CBaseAnimating *pAnimating, int forceBone, const CTakeDamageInfo &info, int collisionGroup, bool bUseLRURetirement )
{
	if ( info.GetDamageType() & (DMG_VEHICLE|DMG_CRUSH) )
	{
		// if the entity was killed by physics or a vehicle, move to the vphysics shadow position before creating the ragdoll.
		SyncAnimatingWithPhysics( pAnimating );
	}
	CRagdollProp *pRagdoll = (CRagdollProp *)CBaseEntity::CreateNoSpawn( "prop_ragdoll", pAnimating->GetAbsOrigin(), vec3_angle, NULL );
	pRagdoll->CopyAnimationDataFrom( pAnimating );
	pRagdoll->SetOwnerEntity( pAnimating );

	pRagdoll->InitRagdollAnimation();
	matrix3x4a_t pBoneToWorld[MAXSTUDIOBONES], pBoneToWorldNext[MAXSTUDIOBONES];

	float dt = 0.1f;

	// Copy over dissolve state...
	if ( pAnimating->IsEFlagSet( EFL_NO_DISSOLVE ) )
	{
		pRagdoll->AddEFlags( EFL_NO_DISSOLVE );
	}

	// NOTE: This currently is only necessary to prevent manhacks from
	// colliding with server ragdolls they kill
	pRagdoll->SetKiller( info.GetInflictor() );
	pRagdoll->SetSourceClassName( pAnimating->GetClassname() );

	// NPC_STATE_DEAD npc's will have their COND_IN_PVS cleared, so this needs to force SetupBones to happen
	unsigned short fPrevFlags = pAnimating->GetBoneCacheFlags();
	pAnimating->SetBoneCacheFlags( BCF_NO_ANIMATION_SKIP );

	// UNDONE: Extract velocity from bones via animation (like we do on the client)
	// UNDONE: For now, just move each bone by the total entity velocity if set.
	// Get Bones positions before
	// Store current cycle
	float fSequenceDuration = pAnimating->SequenceDuration( pAnimating->GetSequence() );
	float fSequenceTime = pAnimating->GetCycle() * fSequenceDuration;

	if( fSequenceTime <= dt && fSequenceTime > 0.0f )
	{
		// Avoid having negative cycle
		dt = fSequenceTime;
	}

	float fPreviousCycle = clamp(pAnimating->GetCycle()-( dt * ( 1 / fSequenceDuration ) ),0.f,1.f);
	float fCurCycle = pAnimating->GetCycle();
	// Get current bones positions
	pAnimating->SetupBones( pBoneToWorldNext, BONE_USED_BY_ANYTHING );
	// Get previous bones positions
	pAnimating->SetCycle( fPreviousCycle );
	pAnimating->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );
	// Restore current cycle
	pAnimating->SetCycle( fCurCycle );

	// Reset previous bone flags
	pAnimating->ClearBoneCacheFlags( BCF_NO_ANIMATION_SKIP );
	pAnimating->SetBoneCacheFlags( fPrevFlags );

	Vector vel = pAnimating->GetAbsVelocity();
	if( ( vel.Length() == 0 ) && ( dt > 0 ) )
	{
		// Compute animation velocity
		CStudioHdr *pstudiohdr = pAnimating->GetModelPtr();
		if ( pstudiohdr )
		{
			Vector deltaPos;
			QAngle deltaAngles;
			if (Studio_SeqMovement( pstudiohdr,
				pAnimating->GetSequence(),
				fPreviousCycle,
				pAnimating->GetCycle(),
				pAnimating->GetPoseParameterArray(),
				deltaPos,
				deltaAngles ))
			{
				VectorRotate( deltaPos, pAnimating->EntityToWorldTransform(), vel );
				vel /= dt;
			}
		}
	}

	if ( vel.LengthSqr() > 0 )
	{
		int numbones = pAnimating->GetModelPtr()->numbones();
		vel *= dt;
		for ( int i = 0; i < numbones; i++ )
		{
			Vector pos;
			MatrixGetColumn( pBoneToWorld[i], 3, pos );
			pos -= vel;
			MatrixSetColumn( pos, 3, pBoneToWorld[i] );
		}
	}

#if RAGDOLL_VISUALIZE
	pAnimating->DrawRawSkeleton( pBoneToWorld, BONE_USED_BY_ANYTHING, true, 20, false );
	pAnimating->DrawRawSkeleton( pBoneToWorldNext, BONE_USED_BY_ANYTHING, true, 20, true );
#endif
	// Is this a vehicle / NPC collision?
	if ( (info.GetDamageType() & DMG_VEHICLE) && pAnimating->MyNPCPointer() )
	{
		// init the ragdoll with no forces
		pRagdoll->InitRagdoll( vec3_origin, -1, vec3_origin, pBoneToWorld, pBoneToWorldNext, dt, collisionGroup, true );

		// apply vehicle forces
		// Get a list of bones with hitboxes below the plane of impact
		int boxList[128];
		Vector normal(0,0,-1);
		int count = pAnimating->GetHitboxesFrontside( boxList, ARRAYSIZE(boxList), normal, DotProduct( normal, info.GetDamagePosition() ) );

		// distribute force over mass of entire character
		float massScale = Studio_GetMass(pAnimating->GetModelPtr());
		massScale = clamp( massScale, 1.f, 1.e4f );
		massScale = 1.f / massScale;

		// distribute the force
		// BUGBUG: This will hit the same bone twice if it has two hitboxes!!!!
		ragdoll_t *pRagInfo = pRagdoll->GetRagdoll();
		for ( int i = 0; i < count; i++ )
		{
			int physBone = pAnimating->GetPhysicsBone( pAnimating->GetHitboxBone( boxList[i] ) );
			IPhysicsObject *pPhysics = pRagInfo->list[physBone].pObject;
			pPhysics->ApplyForceCenter( info.GetDamageForce() * pPhysics->GetMass() * massScale );
		}
	}
	else
	{
		pRagdoll->InitRagdoll( info.GetDamageForce(), forceBone, info.GetDamagePosition(), pBoneToWorld, pBoneToWorldNext, dt, collisionGroup, true );
	}

	// Are we dissolving?
	if ( pAnimating->IsDissolving() )
	{
		pRagdoll->TransferDissolveFrom( pAnimating );
	}
	else if ( bUseLRURetirement )
	{
		pRagdoll->AddSpawnFlags( SF_RAGDOLLPROP_USE_LRU_RETIREMENT );
		s_RagdollLRU.MoveToTopOfLRU( pRagdoll );
	}

	// Tracker 22598:  If we don't set the OBB mins/maxs to something valid here, then the client will have a zero sized hull
	//  for the ragdoll for one frame until Vphysics updates the real obb bounds after the first simulation frame.  Having
	//  a zero sized hull makes the ragdoll think it should be faded/alpha'd to zero for a frame, so you get a blink where
	//  the ragdoll doesn't draw initially.
	Vector mins, maxs;
	mins = pAnimating->CollisionProp()->OBBMins();
	maxs = pAnimating->CollisionProp()->OBBMaxs();
	pRagdoll->CollisionProp()->SetCollisionBounds( mins, maxs );

	return pRagdoll;
}

void CRagdollPropAttached::DetachOnNextUpdate()
{
	m_bShouldDetach = true;
}

void CRagdollPropAttached::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( m_bShouldDetach )
	{
		Detach();
		m_bShouldDetach = false;
	}
	BaseClass::VPhysicsUpdate( pPhysics );
}

void CRagdollPropAttached::Detach()
{
	SetParent(NULL);
	SetOwnerEntity( NULL );
	SetAbsAngles( vec3_angle );
	SetMoveType( MOVETYPE_VPHYSICS );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	physenv->DestroyConstraint( m_pAttachConstraint );
	m_pAttachConstraint = NULL;
	const float dampingScale = 1.0f / ATTACHED_DAMPING_SCALE;
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		float damping, rotdamping;
		m_ragdoll.list[i].pObject->GetDamping( &damping, &rotdamping );
		damping *= dampingScale;
		rotdamping *= dampingScale;
		m_ragdoll.list[i].pObject->SetDamping( &damping, &damping );
	}

	// Go non-solid
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	RecheckCollisionFilter();
}

void CRagdollPropAttached::InitRagdollAttached(
	IPhysicsObject *pAttached,
	const Vector &forceVector,
	int forceBone,
	matrix3x4_t *pPrevBones,
	matrix3x4_t *pBoneToWorld,
	float dt,
	int collisionGroup,
	CBaseAnimating *pFollow,
	int boneIndexRoot,
	const Vector &boneLocalOrigin,
	int parentBoneAttach,
	const Vector &worldAttachOrigin )
{
	int ragdollAttachedIndex = 0;
	if ( parentBoneAttach > 0 )
	{
		CStudioHdr *pStudioHdr = GetModelPtr();
		mstudiobone_t *pBone = pStudioHdr->pBone( parentBoneAttach );
		ragdollAttachedIndex = pBone->physicsbone;
	}

	InitRagdoll( forceVector, forceBone, vec3_origin, pPrevBones, pBoneToWorld, dt, collisionGroup, false );

	IPhysicsObject *pRefObject = m_ragdoll.list[ragdollAttachedIndex].pObject;

	Vector attachmentPointRagdollSpace;
	pRefObject->WorldToLocal( &attachmentPointRagdollSpace, worldAttachOrigin );

	constraint_ragdollparams_t constraint;
	constraint.Defaults();
	matrix3x4_t tmp, worldToAttached, worldToReference, constraintToWorld;

	Vector offsetWS;
	pAttached->LocalToWorld( &offsetWS, boneLocalOrigin );

	QAngle followAng = QAngle(0, pFollow->GetAbsAngles().y, 0 );
	AngleMatrix( followAng, offsetWS, constraintToWorld );

	constraint.axes[0].SetAxisFriction( -2, 2, 20 );
	constraint.axes[1].SetAxisFriction( 0, 0, 0 );
	constraint.axes[2].SetAxisFriction( -15, 15, 20 );

	// Exaggerate the bone's ability to pull the mass of the ragdoll around
	constraint.constraint.bodyMassScale[1] = 50.0f;

	pAttached->GetPositionMatrix( &tmp );
	MatrixInvert( tmp, worldToAttached );

	pRefObject->GetPositionMatrix( &tmp );
	MatrixInvert( tmp, worldToReference );

	ConcatTransforms( worldToReference, constraintToWorld, constraint.constraintToReference );
	ConcatTransforms( worldToAttached, constraintToWorld, constraint.constraintToAttached );

	// for now, just slam this to be the passed in value
	MatrixSetColumn( attachmentPointRagdollSpace, 3, constraint.constraintToReference );

	PhysDisableEntityCollisions( pAttached, m_ragdoll.list[0].pObject );
	m_pAttachConstraint = physenv->CreateRagdollConstraint( pRefObject, pAttached, m_ragdoll.pGroup, constraint );

	SetParent( pFollow );
	SetOwnerEntity( pFollow );

	RagdollActivate( m_ragdoll, modelinfo->GetVCollide( GetModelIndex() ), GetModelIndex() );

	// add a bunch of dampening to the ragdoll
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		float damping, rotdamping;
		m_ragdoll.list[i].pObject->GetDamping( &damping, &rotdamping );
		damping *= ATTACHED_DAMPING_SCALE;
		rotdamping *= ATTACHED_DAMPING_SCALE;
		m_ragdoll.list[i].pObject->SetDamping( &damping, &rotdamping );
	}

	m_boneIndexAttached = boneIndexRoot;
	m_ragdollAttachedObjectIndex = ragdollAttachedIndex;
	m_attachmentPointBoneSpace = boneLocalOrigin;

	Vector vTemp;
	MatrixGetColumn( constraint.constraintToReference, 3, vTemp );
	m_attachmentPointRagdollSpace = vTemp;
}

CRagdollProp *CreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, IPhysicsObject *pAttached, CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin )
{
	// Return immediately if the model doesn't have a vcollide
	if ( modelinfo->GetVCollide( pAnimating->GetModelIndex() ) == NULL )
		return NULL;

	CRagdollPropAttached *pRagdoll = (CRagdollPropAttached *)CBaseEntity::CreateNoSpawn( "prop_ragdoll_attached", pAnimating->GetAbsOrigin(), vec3_angle, NULL );
	pRagdoll->CopyAnimationDataFrom( pAnimating );

	pRagdoll->InitRagdollAnimation();
	matrix3x4a_t pBoneToWorld[MAXSTUDIOBONES];
	pAnimating->SetupBones( pBoneToWorld, BONE_USED_BY_ANYTHING );
	pRagdoll->InitRagdollAttached( pAttached, vecForce, forceBone, pBoneToWorld, pBoneToWorld, 0.1, collisionGroup, pParentEntity, boneAttach, boneOrigin, parentBoneAttach, originAttached );

	return pRagdoll;
}

void DetachAttachedRagdoll( CBaseEntity *pRagdollIn )
{
	CRagdollPropAttached *pRagdoll = dynamic_cast<CRagdollPropAttached *>(pRagdollIn);

	if ( pRagdoll )
	{
		pRagdoll->DetachOnNextUpdate();
	}
}

void DetachAttachedRagdollsForEntity( CBaseEntity *pRagdollParent )
{
	CUtlVector<CBaseEntity *> list;
	GetAllChildren( pRagdollParent, list );
	for ( int i = list.Count()-1; i >= 0; --i )
	{
		DetachAttachedRagdoll( list[i] );
	}
}

bool Ragdoll_IsPropRagdoll( CBaseEntity *pEntity )
{
	if ( dynamic_cast<CRagdollProp *>(pEntity) != NULL )
		return true;
	return false;
}

ragdoll_t *Ragdoll_GetRagdoll( CBaseEntity *pEntity )
{
	CRagdollProp *pProp = dynamic_cast<CRagdollProp *>(pEntity);
	if ( pProp )
		return pProp->GetRagdoll();
	return NULL;
}

void CRagdollProp::GetAngleOverrideFromCurrentState( char *pOut, int size )
{
	pOut[0] = 0;
	for ( int i = 0; i < m_ragdoll.listCount; i++ )
	{
		if ( i != 0 )
		{
			Q_strncat( pOut, ",", size, COPY_ALL_CHARACTERS );

		}
		CFmtStr str("%d,%.2f %.2f %.2f", i, m_ragAngles[i].x, m_ragAngles[i].y, m_ragAngles[i].z );
		Q_strncat( pOut, str, size, COPY_ALL_CHARACTERS );
	}
}

void CRagdollProp::DisableMotion( void )
{
	for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
	{
		IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;
		if ( pPhysicsObject != NULL )
		{
			pPhysicsObject->EnableMotion( false );
		}
	}
}

void CRagdollProp::InputStartRadgollBoogie( inputdata_t &inputdata )
{
	float duration = inputdata.value.Float();

	if( duration <= 0.0f )
	{
		duration = 5.0f;
	}

	CRagdollBoogie::Create( this, 100, gpGlobals->curtime, duration, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Enable physics motion and collision response (on by default)
//-----------------------------------------------------------------------------
void CRagdollProp::InputEnableMotion( inputdata_t &inputdata )
{
	for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
	{
		IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;
		if ( pPhysicsObject != NULL )
		{
			pPhysicsObject->EnableMotion( true );
			pPhysicsObject->Wake();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Disable any physics motion or collision response
//-----------------------------------------------------------------------------
void CRagdollProp::InputDisableMotion( inputdata_t &inputdata )
{
	DisableMotion();
}

void CRagdollProp::InputTurnOn( inputdata_t &inputdata )
{
	RemoveEffects( EF_NODRAW );
}

void CRagdollProp::InputTurnOff( inputdata_t &inputdata )
{
	AddEffects( EF_NODRAW );
}

void CRagdollProp::InputFadeAndRemove( inputdata_t &inputdata )
{
	float flFadeDuration = inputdata.value.Float();

	if( flFadeDuration == 0.0f )
		flFadeDuration = 1.0f;

	FadeOut( 0.0f, flFadeDuration );
}

void Ragdoll_GetAngleOverrideString( char *pOut, int size, CBaseEntity *pEntity )
{
	CRagdollProp *pRagdoll = dynamic_cast<CRagdollProp *>(pEntity);
	if ( pRagdoll )
	{
		pRagdoll->GetAngleOverrideFromCurrentState( pOut, size );
	}
}


class CDynamicDestrProp : public CRagdollProp
{
	DECLARE_CLASS( CDynamicDestrProp, CRagdollProp );
public:
	CDynamicDestrProp();

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	void			InputStartDestruction( inputdata_t &inputdata );

	DECLARE_DATADESC();
private:
	bool		bwehit;
	bool		busefirstlimit;
	bool		busesecondlimit;

	float		Health;
	float		PieceMass;

	int			iNumBrokenPartsLimit;
	int			iNumBrokenParts;
	int			iNumHitLimit;
	int			iNumHits;
	char		particle_name[128];
	float		flTimeToCollapse;
	COutputEvent m_OnTakeDamage;
	COutputEvent m_OnFullyDestroyed;
};
LINK_ENTITY_TO_CLASS( prop_dynamically_destructible, CDynamicDestrProp );

BEGIN_DATADESC( CDynamicDestrProp )
	DEFINE_KEYFIELD( Health, FIELD_FLOAT, "prophealth" ),
	DEFINE_KEYFIELD( PieceMass, FIELD_FLOAT, "mass" ),
	DEFINE_KEYFIELD( iNumHitLimit, FIELD_INTEGER, "numhits" ),
	DEFINE_KEYFIELD( iNumBrokenPartsLimit, FIELD_INTEGER, "numpieces" ),

	DEFINE_FIELD( iNumHits, FIELD_INTEGER ),
	DEFINE_FIELD( iNumBrokenParts, FIELD_INTEGER ),

	DEFINE_INPUTFUNC( FIELD_VOID, "StartDestruction", InputStartDestruction ),

	DEFINE_OUTPUT(m_OnTakeDamage, "OnTakeDamage"),
	DEFINE_OUTPUT(m_OnFullyDestroyed, "OnFullyDestroyed"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CDynamicDestrProp::CDynamicDestrProp()
{
	Health = 0;
	PieceMass = 0;
	iNumHitLimit = 0;
	iNumHits = 0;
	iNumBrokenPartsLimit = 0;
	iNumBrokenParts = 0;
	particle_name[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CDynamicDestrProp::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	char mat;
	float velmodifier;

	surfacedata_t *phit = physprops->GetSurfaceData( pEvent->surfaceProps[index] );
	mat = phit->game.material;

	//start burning if we are wood (btw I am defuse kit lololol, but if you are wood you should burn)
	if ( !HasSpawnFlags( SF_DONT_IGNITE ) && bwehit == true )
	{
		if( mat == 'W')
		{
			flTimeToCollapse = gpGlobals->curtime + 60 + RandomFloat(0,5);
			Ignite( 60, false, 256, true );
		}
	}

	bwehit = false;

	switch (mat)
	{
		case 'W':
			velmodifier = 1;
			V_strcpy_safe( particle_name, "dest_exp_wood_big" );
			break;
		case 'C':
			velmodifier = 2;
			V_strcpy_safe( particle_name, "dest_exp_concrete_big" );
			break;
		case 'M':
			velmodifier = 1.5;
			V_strcpy_safe( particle_name, "dest_exp_metal_big" );
			break;
		case 'Y':
			velmodifier = 0.8;
			break;
		default:
			velmodifier = 1.2;
			break;
	}

	if( IsOnFire() && gpGlobals->curtime > flTimeToCollapse )
	{
		for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
		{
			IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;

			if ( pPhysicsObject != NULL )
			{
				pPhysicsObject->EnableMotion(true);
				pPhysicsObject->Wake();
			}
		}
	}

	if ( pEvent->preVelocity[!index].Length() > 1156 * velmodifier )
	{
		for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
		{
			IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;

			if ( pPhysicsObject != NULL)
				pPhysicsObject->EnableMotion(true);
		}
	}
	if( HasSpawnFlags( SF_DISABLE_MOTION_IF_NOT_MOVING ) || HasSpawnFlags( SF_DESTROY_BY_PIECE ) )
	{
		Vector vel;
		for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
		{
			IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;

			if ( pPhysicsObject != NULL )
			{
				pPhysicsObject->GetVelocity(&vel, NULL);

				if( vel.Length() < 1 )	//disable motion if we are not moving
				{
					pPhysicsObject->EnableMotion(false);
				}
			}
		}
	}
	CBaseAnimating::VPhysicsCollision(index, pEvent);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CDynamicDestrProp::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !( info.GetDamageType() & (DMG_SONIC | DMG_BLAST ) ) && !HasSpawnFlags( SF_ALLOW_BULLET_DAMAGE ) )
		return 0;

	if( iNumHitLimit > 1 )
	{
		iNumHits += 1;
	}

	bwehit = ( info.GetDamageType() & ( DMG_BURN | DMG_SLOWBURN | DMG_BLAST ) ) != 0;

	if( Health < 1 )
	{
		Health = 1;
	}

	m_OnTakeDamage.FireOutput(this, this);

	Vector vpos;
	QAngle vangles;

	if( particle_name[0] != '\0' )	//why not?
	{
		DispatchParticleEffect( particle_name, info.GetDamagePosition(), vangles, this );
	}
	//numhits and pieces limits
	if ( HasSpawnFlags( SF_FULL_DESTROY ) || ( iNumHitLimit > 1 && iNumHits >= iNumHitLimit )
		|| ( iNumBrokenPartsLimit > 1 && iNumBrokenParts >= iNumBrokenPartsLimit ) )
	{
		for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
		{
			IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;

			if ( pPhysicsObject != NULL )
			{
				pPhysicsObject->GetPosition( &vpos, &vangles );
				pPhysicsObject->EnableMotion( true );

				if( PieceMass >= 0 )
				{
					pPhysicsObject->SetMass( PieceMass );
				}

				pPhysicsObject->ApplyForceOffset( info.GetDamageForce()*(1/(vpos - info.GetDamagePosition()).Length()), info.GetDamagePosition() );
			}
		}

		m_OnFullyDestroyed.FireOutput( this, this );
	}

	//if no limits set - use default destruction
	if ( !HasSpawnFlags( SF_DESTROY_BY_PIECE ) )
	{
		for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
		{
			IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;

			if ( pPhysicsObject != NULL )
			{
				pPhysicsObject->GetPosition(&vpos, &vangles);
				//256 - MAXIMUM_EXPLODE_RADIUS or something like that
				if( ( ( vpos - info.GetDamagePosition()).Length() < 256/Health ) )	//I am Source Engine God, Gabe, pls hire me. :) :) ;)
				{
					if( PieceMass >= 0 )
					{
						pPhysicsObject->SetMass( PieceMass );
					}

					pPhysicsObject->EnableMotion( true );
					pPhysicsObject->ApplyForceOffset( info.GetDamageForce()*(1/(vpos - info.GetDamagePosition()).Length()), info.GetDamagePosition() );

					if( iNumBrokenPartsLimit > 1 )
					{
						iNumBrokenParts += 1;
					}

				}
			}
		}
	}
	else if ( HasSpawnFlags(SF_DESTROY_BY_PIECE) )
	{
		for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
		{
			IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;

			if ( pPhysicsObject != NULL )
			{
				pPhysicsObject->GetPosition(&vpos, &vangles);

				if( ( ( vpos - info.GetDamagePosition()).Length() < 16 ) )
				{
					if( PieceMass >= 0 )
					{
						pPhysicsObject->SetMass( PieceMass );
					}

					pPhysicsObject->EnableMotion( true );
					pPhysicsObject->ApplyForceOffset( info.GetDamageForce()*(1/(vpos - info.GetDamagePosition()).Length()), info.GetDamagePosition() );

					if( iNumBrokenPartsLimit > 1 )
					{
						iNumBrokenParts += 1;
					}

					break;
				}
			}
		}
	}

	return CBaseAnimating::OnTakeDamage( info );
}

void CDynamicDestrProp::InputStartDestruction( inputdata_t &inputdata )
{
	for ( int iRagdoll = 0; iRagdoll < m_ragdoll.listCount; ++iRagdoll )
	{
		IPhysicsObject *pPhysicsObject = m_ragdoll.list[ iRagdoll ].pObject;
		if ( pPhysicsObject != NULL )
		{
			pPhysicsObject->EnableMotion( true );
			pPhysicsObject->ApplyForceCenter(vec3_origin);
		}
	}
}