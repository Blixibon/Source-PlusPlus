//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_npc_manhack.h"
#include "soundenvelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MANHACK_GLOW_SPRITE	"sprites/glow1.vmt"

//-----------------------------------------------------------------------------
// Save/restore
//-----------------------------------------------------------------------------
BEGIN_DATADESC(C_NPC_Manhack)

//	DEFINE_SOUNDPATCH( m_pEngineSound1 ),
//	DEFINE_SOUNDPATCH( m_pEngineSound2 ),
//	DEFINE_SOUNDPATCH( m_pBladeSound ),

//	DEFINE_FIELD( m_nEnginePitch1, FIELD_INTEGER ),
//	DEFINE_FIELD( m_nEnginePitch2, FIELD_INTEGER ),
//	DEFINE_FIELD( m_flEnginePitch1Time, FIELD_FLOAT ),
//	DEFINE_FIELD( m_flEnginePitch2Time, FIELD_FLOAT ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT(C_NPC_Manhack, DT_NPC_Manhack, CNPC_Manhack)
RecvPropIntWithMinusOneFlag(RECVINFO(m_nEnginePitch1)),
RecvPropFloat(RECVINFO(m_flEnginePitch1Time)),
RecvPropIntWithMinusOneFlag(RECVINFO(m_nEnginePitch2)),
RecvPropFloat(RECVINFO(m_flEnginePitch2Time)),

RecvPropInt(RECVINFO(m_nEyeState), SPROP_UNSIGNED),
RecvPropBool(RECVINFO(m_bIsControlled)),
RecvPropEHandle(RECVINFO(m_hOwningPlayer)),
RecvPropEHandle(RECVINFO(m_hHUDTarget)),
END_RECV_TABLE();



C_NPC_Manhack::C_NPC_Manhack()
{
}

//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void C_NPC_Manhack::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if (( m_nEnginePitch1 < 0 ) || ( m_nEnginePitch2 < 0 ) )
	{
		SoundShutdown();
	}
	else
	{
		SoundInit();
		if ( m_pEngineSound1 && m_pEngineSound2 )
		{
			float dt = ( m_flEnginePitch1Time >= gpGlobals->curtime ) ? m_flEnginePitch1Time - gpGlobals->curtime : 0.0f;
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound1, m_nEnginePitch1, dt );
			dt = ( m_flEnginePitch2Time >= gpGlobals->curtime ) ? m_flEnginePitch2Time - gpGlobals->curtime : 0.0f;
			CSoundEnvelopeController::GetController().SoundChangePitch( m_pEngineSound2, m_nEnginePitch2, dt );
		}
	}

	if (type == DATA_UPDATE_CREATED || m_nLastEyeState != m_nEyeState)
	{
		m_nLastEyeState = m_nEyeState;
		SetEyeState(m_nEyeState);
	}

	if (type == DATA_UPDATE_CREATED || m_bLastIsControlledByLocalPlayer != IsControlledByLocalPlayer())
	{
		m_bLastIsControlledByLocalPlayer = IsControlledByLocalPlayer();
		UpdateVisibility();

		if (!m_bLastIsControlledByLocalPlayer)
		{
			StartEye();
			SetEyeState(m_nEyeState);
		}
		else
		{
			KillEye();
		}
	}
}


//-----------------------------------------------------------------------------
// Restore
//-----------------------------------------------------------------------------
void C_NPC_Manhack::OnRestore()
{
	BaseClass::OnRestore();
	SoundInit();
}

bool C_NPC_Manhack::ShouldDraw()
{
	if (m_bLastIsControlledByLocalPlayer)
		return false;

	return BaseClass::ShouldDraw();
}

void C_NPC_Manhack::SupressGlows(bool bSupress)
{
	if (m_pEyeGlow)
	{
		m_pEyeGlow->SetSupressDraw(bSupress);
	}

	if (m_pLightGlow)
	{
		m_pLightGlow->SetSupressDraw(bSupress);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void C_NPC_Manhack::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
	SoundShutdown();
	KillEye();
}


//-----------------------------------------------------------------------------
// Purpose: Start the manhack's engine sound.
//-----------------------------------------------------------------------------
void C_NPC_Manhack::SoundInit( void )
{
	if (( m_nEnginePitch1 < 0 ) || ( m_nEnginePitch2 < 0 ) )
		return;

	// play an engine start sound!!
	CPASAttenuationFilter filter( this );

	// Bring up the engine looping sound.
	if( !m_pEngineSound1 )
	{
		m_pEngineSound1 = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.EngineSound1" );
		CSoundEnvelopeController::GetController().Play( m_pEngineSound1, 0.0, m_nEnginePitch1 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pEngineSound1, 0.7, 2.0 );
	}

	if( !m_pEngineSound2 )
	{
		m_pEngineSound2 = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.EngineSound2" );
		CSoundEnvelopeController::GetController().Play( m_pEngineSound2, 0.0, m_nEnginePitch2 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pEngineSound2, 0.7, 2.0 );
	}

	if( !m_pBladeSound )
	{
		m_pBladeSound = CSoundEnvelopeController::GetController().SoundCreate( filter, entindex(), "NPC_Manhack.BladeSound" );
		CSoundEnvelopeController::GetController().Play( m_pBladeSound, 0.0, m_nEnginePitch1 );
		CSoundEnvelopeController::GetController().SoundChangeVolume( m_pBladeSound, 0.7, 2.0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_NPC_Manhack::SoundShutdown(void)
{
	// Kill the engine!
	if ( m_pEngineSound1 )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pEngineSound1 );
		m_pEngineSound1 = NULL;
	}

	// Kill the engine!
	if ( m_pEngineSound2 )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pEngineSound2 );
		m_pEngineSound2 = NULL;
	}

	// Kill the blade!
	if ( m_pBladeSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pBladeSound );
		m_pBladeSound = NULL;
	}
}

void C_NPC_Manhack::SetEyeState(int state)
{
	switch (state)
	{
	case MANHACK_EYE_STATE_STUNNED:
	{
		if (m_pEyeGlow)
		{
			//Toggle our state
			m_pEyeGlow->SetColor(255, 128, 0);
			m_pEyeGlow->SetScale(0.15f, 0.1f);
			m_pEyeGlow->SetBrightness(164, 0.1f);
			m_pEyeGlow->m_nRenderFX = kRenderFxStrobeFast;
		}

		if (m_pLightGlow)
		{
			m_pLightGlow->SetColor(255, 128, 0);
			m_pLightGlow->SetScale(0.15f, 0.1f);
			m_pLightGlow->SetBrightness(164, 0.1f);
			m_pLightGlow->m_nRenderFX = kRenderFxStrobeFast;
		}
		break;
	}

	case MANHACK_EYE_STATE_CHARGE:
	{
		if (m_pEyeGlow)
		{
			//Toggle our state
			/*if (m_bHackedByAlyx)
			{
				m_pEyeGlow->SetColor(0, 255, 0);
			}
			else*/
			{
				m_pEyeGlow->SetColor(255, 0, 0);
			}

			m_pEyeGlow->SetScale(0.25f, 0.5f);
			m_pEyeGlow->SetBrightness(164, 0.1f);
			m_pEyeGlow->m_nRenderFX = kRenderFxNone;
		}

		if (m_pLightGlow)
		{
			/*if (m_bHackedByAlyx)
			{
				m_pLightGlow->SetColor(0, 255, 0);
			}
			else*/
			{
				m_pLightGlow->SetColor(255, 0, 0);
			}

			m_pLightGlow->SetScale(0.25f, 0.5f);
			m_pLightGlow->SetBrightness(164, 0.1f);
			m_pLightGlow->m_nRenderFX = kRenderFxNone;
		}

		break;
	}
	case MANHACK_EYE_STATE_CHASE:
	case MANHACK_EYE_STATE_IDLE:
#ifdef EZ2
		if (!m_bHackedByAlyx)
#endif
		{
			if (gpGlobals->maxClients > 1 && m_hOwningPlayer.Get() == C_BasePlayer::GetLocalPlayer())
			{
				if (m_pEyeGlow)
				{
					//Toggle our state
					m_pEyeGlow->SetColor(0, 255, 0);
					m_pEyeGlow->SetScale(0.25f, 0.5f);
					m_pEyeGlow->SetBrightness(164, 0.1f);
					m_pEyeGlow->m_nRenderFX = kRenderFxNone;
				}

				if (m_pLightGlow)
				{
					m_pLightGlow->SetColor(0, 255, 0);
					m_pLightGlow->SetScale(0.25f, 0.5f);
					m_pLightGlow->SetBrightness(164, 0.1f);
					m_pLightGlow->m_nRenderFX = kRenderFxNone;
				}
			}
			else
			{
				if (m_pEyeGlow)
				{
					//Toggle our state
					m_pEyeGlow->SetColor(0, 255, 255);
					m_pEyeGlow->SetScale(0.25f, 0.5f);
					m_pEyeGlow->SetBrightness(164, 0.1f);
					m_pEyeGlow->m_nRenderFX = kRenderFxNone;
				}

				if (m_pLightGlow)
				{
					m_pLightGlow->SetColor(0, 255, 255);
					m_pLightGlow->SetScale(0.25f, 0.5f);
					m_pLightGlow->SetBrightness(164, 0.1f);
					m_pLightGlow->m_nRenderFX = kRenderFxNone;
				}
			}
			break;
		}
	default:
		if (m_pEyeGlow)
			m_pEyeGlow->m_nRenderFX = kRenderFxNone;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Manhack::StartEye(void)
{
	//Create our Eye sprite
	if (m_pEyeGlow == NULL)
	{
		m_pEyeGlow = new C_ManhackSprite;
		m_pEyeGlow->SpriteInit(MANHACK_GLOW_SPRITE, GetLocalOrigin());
		m_pEyeGlow->SetAttachment(this, LookupAttachment("Eye"));
		m_pEyeGlow->SetSupressDraw(false);

		/*if (m_bHackedByAlyx)
		{
			m_pEyeGlow->SetTransparency(kRenderTransAdd, 0, 255, 0, 128, kRenderFxNoDissipation);
			m_pEyeGlow->SetColor(0, 255, 0);
		}
		else*/
		{
			m_pEyeGlow->SetTransparency(kRenderTransAdd, 0, 255, 255, 128, kRenderFxNoDissipation);
			m_pEyeGlow->SetColor(0, 255, 255);
		}

		m_pEyeGlow->SetBrightness(164, 0.1f);
		m_pEyeGlow->SetScale(0.25f, 0.1f);
	}

	//Create our light sprite
	if (m_pLightGlow == NULL)
	{
		m_pLightGlow = new C_ManhackSprite;
		m_pLightGlow->SpriteInit(MANHACK_GLOW_SPRITE, GetLocalOrigin());
		m_pLightGlow->SetAttachment(this, LookupAttachment("Light"));
		m_pLightGlow->SetSupressDraw(false);

		/*if (m_bHackedByAlyx)
		{
			m_pLightGlow->SetTransparency(kRenderTransAdd, 0, 255, 0, 128, kRenderFxNoDissipation);
			m_pLightGlow->SetColor(0, 255, 0);
		}
		else*/
		{
			m_pLightGlow->SetTransparency(kRenderTransAdd, 0, 255, 255, 128, kRenderFxNoDissipation);
			m_pLightGlow->SetColor(0, 255, 255);
		}

		m_pLightGlow->SetBrightness(164, 0.1f);
		m_pLightGlow->SetScale(0.25f, 0.1f);
	}
}

void C_NPC_Manhack::KillEye(void)
{
	if (m_pEyeGlow)
	{
		m_pEyeGlow->Remove();
		m_pEyeGlow = NULL;
	}

	if (m_pLightGlow)
	{
		m_pLightGlow->Remove();
		m_pLightGlow = NULL;
	}
}

bool C_NPC_Manhack::IsControlledByLocalPlayer()
{
	return (m_bIsControlled && m_hOwningPlayer->InFirstPersonView());
}
