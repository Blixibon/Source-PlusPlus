//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"
#include "c_world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

// Cold breath defines.
#define COLDBREATH_EMIT_MIN				2.0f
#define COLDBREATH_EMIT_MAX				3.0f
#define COLDBREATH_EMIT_SCALE			0.35f
#define COLDBREATH_PARTICLE_LIFE_MIN	0.25f
#define COLDBREATH_PARTICLE_LIFE_MAX	1.0f
#define COLDBREATH_PARTICLE_LIFE_SCALE  0.75
#define COLDBREATH_PARTICLE_SIZE_MIN	1.0f
#define COLDBREATH_PARTICLE_SIZE_MAX	4.0f
#define COLDBREATH_PARTICLE_SIZE_SCALE	1.1f
#define COLDBREATH_PARTICLE_COUNT		1
#define COLDBREATH_DURATION_MIN			0.0f
#define COLDBREATH_DURATION_MAX			1.0f
#define COLDBREATH_ALPHA_MIN			0.0f
#define COLDBREATH_ALPHA_MAX			0.3f
#define COLDBREATH_ENDSCALE_MIN			0.1f
#define COLDBREATH_ENDSCALE_MAX			0.4f

static ConVar cl_coldbreath_enable("cl_coldbreath_enable", "1");

//======================================================
//
// Cold Breath Emitter - for DOD players.
//
class ColdBreathEmitter : public CSimpleEmitter
{
public:

	ColdBreathEmitter(const char *pDebugName) : CSimpleEmitter(pDebugName) {}

	static ColdBreathEmitter *Create(const char *pDebugName)
	{
		return new ColdBreathEmitter(pDebugName);
	}

	void UpdateVelocity(SimpleParticle *pParticle, float timeDelta)
	{
		// Float up when lifetime is half gone.
		pParticle->m_vecVelocity[2] -= (8.0f * timeDelta);


		// FIXME: optimize this....
		pParticle->m_vecVelocity *= ExponentialDecay(0.9, 0.03, timeDelta);
	}

	virtual	float UpdateRoll(SimpleParticle *pParticle, float timeDelta)
	{
		pParticle->m_flRoll += pParticle->m_flRollDelta * timeDelta;

		pParticle->m_flRollDelta += pParticle->m_flRollDelta * (timeDelta * -2.0f);

		//Cap the minimum roll
		if (fabs(pParticle->m_flRollDelta) < 0.5f)
		{
			pParticle->m_flRollDelta = (pParticle->m_flRollDelta > 0.0f) ? 0.5f : -0.5f;
		}

		return pParticle->m_flRoll;
	}

private:

	ColdBreathEmitter(const ColdBreathEmitter &);
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
	{
		m_iAmmo.Set( i, 0 );
	}

#ifdef GLOWS_ENABLE
	m_pGlowEffect = NULL;
	m_bGlowEnabled = false;
	m_bOldGlowEnabled = false;
	m_bClientSideGlowEnabled = false;
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
#ifdef GLOWS_ENABLE
	DestroyGlowEffect();
#endif // GLOWS_ENABLE
}

/*
//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	C_BaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( g_pGameRules->GetAmmoDef()->Index(szName) );
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

#ifdef GLOWS_ENABLE
	m_bOldGlowEnabled = m_bGlowEnabled;
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

#ifdef GLOWS_ENABLE
	if ( m_bOldGlowEnabled != m_bGlowEnabled )
	{
		UpdateGlowEffect();
	}
#endif // GLOWS_ENABLE
}

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}

#define LerpMouth(a, b) Lerp(m_flMouthOpenPct, a, b)
#define LerpMouthScalar(base, scalar) LerpMouth(base, base*scalar)

//-----------------------------------------------------------------------------
// Purpose: Create the emitter of cold breath particles
//-----------------------------------------------------------------------------
bool C_BaseCombatCharacter::CreateColdBreathEmitter(void)
{
	// Check to see if we are in a cold breath scenario.
	if (!GetClientWorldEntity()->m_bColdWorld && cl_coldbreath_enable.GetInt() < 2)
		return false;

	// Cache off the head attachment for setting up cold breath.
	//m_iHeadAttach = LookupAttachment("mouth");

	if (m_iMouthAttachment <= 0)
		return false;

	// Set cold breath to true.
	m_bColdBreathOn = true;

	// Create a cold breath emitter if one doesn't already exist.
	if (!m_hColdBreathEmitter)
	{
		m_hColdBreathEmitter = ColdBreathEmitter::Create("ColdBreath");
		if (!m_hColdBreathEmitter)
			return false;

		// Get the particle material.
		m_hColdBreathMaterial = m_hColdBreathEmitter->GetPMaterial("sprites/frostbreath");
		Assert(m_hColdBreathMaterial != INVALID_MATERIAL_HANDLE);	
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Destroy the cold breath emitter
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DestroyColdBreathEmitter(void)
{
#if 0
	if (m_hColdBreathEmitter.IsValid())
	{
		UTIL_Remove(m_hColdBreathEmitter);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::UpdateColdBreath(void)
{
	if (!cl_coldbreath_enable.GetBool())
		return;

	// Check to see if the cold breath emitter has been created.
	if (!m_hColdBreathEmitter.IsValid())
	{
		if (!CreateColdBreathEmitter())
			return;
	}

	// Cold breath updates.
	if (!m_bColdBreathOn)
		return;

	// Don't emit breath if we are dead.
	if (!IsAlive() || IsDormant())
		return;

	// Check player speed, do emit cold breath when moving quickly.
	float flSpeed = GetAbsVelocity().Length();
	if (flSpeed > 60.0f)
		return;

	if (m_flColdBreathTimeStart < gpGlobals->curtime)
	{
		// Spawn cold breath particles.
		EmitColdBreathParticles();

		// Update the timer.
		if (m_flColdBreathTimeEnd < gpGlobals->curtime)
		{
			// Check stamina and modify the time accordingly.
			/*if (m_Shared.m_flStamina < LOW_STAMINA_THRESHOLD || cl_coldbreath_forcestamina.GetBool())
			{
				m_flColdBreathTimeStart = gpGlobals->curtime + RandomFloat(COLDBREATH_EMIT_MIN * COLDBREATH_EMIT_SCALE, COLDBREATH_EMIT_MAX * COLDBREATH_EMIT_SCALE);
				float flDuration = RandomFloat(COLDBREATH_DURATION_MIN, COLDBREATH_DURATION_MAX);
				m_flColdBreathTimeEnd = m_flColdBreathTimeStart + flDuration;
			}
			else*/
			{
				m_flColdBreathTimeStart = gpGlobals->curtime + LerpMouthScalar(RandomFloat(COLDBREATH_EMIT_MIN, COLDBREATH_EMIT_MAX), COLDBREATH_EMIT_SCALE);
				float flDuration = RandomFloat(COLDBREATH_DURATION_MIN, COLDBREATH_DURATION_MAX);
				m_flColdBreathTimeEnd = m_flColdBreathTimeStart + flDuration;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::EmitColdBreathParticles(void)
{
	// Get the position to emit from - look into caching this off we are doing redundant work in the case
	// of allies (see dod_headiconmanager.cpp).
	Vector vecOrigin;
	QAngle vecAngle;
	GetAttachment(m_iMouthAttachment, vecOrigin, vecAngle);
	Vector vecForward, vecRight, vecUp;
	AngleVectors(vecAngle, &vecForward, &vecRight, &vecUp);

	//vecOrigin += (vecForward * 8.0f);

	SimpleParticle *pParticle = static_cast<SimpleParticle*>(m_hColdBreathEmitter->AddParticle(sizeof(SimpleParticle), m_hColdBreathMaterial, vecOrigin));
	if (pParticle)
	{
		pParticle->m_flLifetime = 0.0f;
		pParticle->m_flDieTime = LerpMouthScalar(RandomFloat(COLDBREATH_PARTICLE_LIFE_MIN, COLDBREATH_PARTICLE_LIFE_MAX), (float)COLDBREATH_PARTICLE_LIFE_SCALE);
		Vector vecBaseVelocity;
		Quaternion qVelocity;
		GetAttachmentVelocity(m_iMouthAttachment, vecBaseVelocity, qVelocity);

		pParticle->m_vecVelocity = vecBaseVelocity;

		/*if (m_Shared.m_flStamina < LOW_STAMINA_THRESHOLD || cl_coldbreath_forcestamina.GetBool())
		{
			pParticle->m_flDieTime *= COLDBREATH_PARTICLE_LIFE_SCALE;
		}*/

		// Add just a little movement.
		/*if (m_Shared.m_flStamina < LOW_STAMINA_THRESHOLD || cl_coldbreath_forcestamina.GetBool())
		{
			pParticle->m_vecVelocity += (vecForward * RandomFloat(10.0f, 30.0f)) + (vecRight * RandomFloat(-2.0f, 2.0f)) +
				(vecUp * RandomFloat(0.0f, 0.5f));
		}
		else*/
		float flF, flU;
		flF = RandomFloat(10.0f, LerpMouth(20.0f, 30.0f));
		flU = RandomFloat(0.0f, LerpMouth(1.5f, 0.5f)) - LerpMouth(8.0f, 0.0f);
		{
			pParticle->m_vecVelocity += (vecForward * flF) + (vecRight * RandomFloat(-2.0f, 2.0f)) +
				(vecUp * flU);
		}


		pParticle->m_uchColor[0] = 200;
		pParticle->m_uchColor[1] = 200;
		pParticle->m_uchColor[2] = 210;

		float flParticleSize = LerpMouthScalar(RandomFloat(COLDBREATH_PARTICLE_SIZE_MIN, COLDBREATH_PARTICLE_SIZE_MAX), COLDBREATH_PARTICLE_SIZE_SCALE);
		float flParticleScale = RandomFloat(COLDBREATH_ENDSCALE_MIN, COLDBREATH_ENDSCALE_MAX);
		/*if (m_Shared.m_flStamina < LOW_STAMINA_THRESHOLD || cl_coldbreath_forcestamina.GetBool())
		{
			pParticle->m_uchEndSize = flParticleSize * COLDBREATH_PARTICLE_SIZE_SCALE;
			flParticleScale *= COLDBREATH_PARTICLE_SIZE_SCALE;
		}
		else*/
		{
			pParticle->m_uchEndSize = flParticleSize;
		}
		pParticle->m_uchStartSize = (flParticleSize * flParticleScale);

		float flAlpha = RandomFloat(COLDBREATH_ALPHA_MIN, COLDBREATH_ALPHA_MAX);
		pParticle->m_uchStartAlpha = flAlpha * 255;
		pParticle->m_uchEndAlpha = 0;

		pParticle->m_flRoll = RandomInt(0, 360);
		pParticle->m_flRollDelta = RandomFloat(0.0f, 1.25f);

		pParticle->m_iFlags |= SIMPLE_PARTICLE_FLAG_WINDBLOWN;
	}
}

#ifdef GLOWS_ENABLE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::GetGlowEffectColor( float *r, float *g, float *b )
{
	*r = 0.76f;
	*g = 0.76f;
	*b = 0.76f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*
void C_BaseCombatCharacter::EnableGlowEffect( float r, float g, float b )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	m_pGlowEffect = new CGlowObject( this, Vector( r, g, b ), 1.0, true );
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect
	if ( m_bGlowEnabled || m_bClientSideGlowEnabled )
	{
		float r, g, b;
		GetGlowEffectColor( &r, &g, &b );

		m_pGlowEffect = new CGlowObject( this, Vector( r, g, b ), 1.0, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}
#endif // GLOWS_ENABLE

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
	RecvPropDataTable( "bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive) ),
	RecvPropEHandle( RECVINFO( m_hActiveWeapon ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle( RECVINFO( m_hMyWeapons[0] ) ) ),
#ifdef GLOWS_ENABLE
	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
#endif // GLOWS_ENABLE

#ifdef INVASION_CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPowerups ) ),
#endif

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_BaseCombatCharacter )

	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()
