//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "particles_simple.h"
#include "citadel_effects_shared.h"
#include "particles_attractor.h"
#include "iefx.h"
#include "dlight.h"
#include "ClientEffectPrecacheSystem.h"
#include "c_te_effect_dispatch.h"
#include "fx_quad.h"
#include "engine/IVDebugOverlay.h"

#include "iefx.h"
#include "dlight.h"
#include "c_ai_basenpc.h"

// For material proxy
#include "ProxyEntity.h"
#include "materialsystem/IMaterial.h"
#include "materialsystem/IMaterialVar.h"
#include "colorcorrectionmgr.h"

#include "glow_outline_effect.h"

#include "iinput.h"
#include "IGameUIFuncs.h"
#include "inputsystem/iinputsystem.h"
#include "vgui/ILocalize.h"
#include "tier2/tier2.h"
#include "tier3/tier3.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hlss_olivia_light("hlss_olivia_light", "20.0f");
ConVar hlss_olivia_light_front("hlss_olivia_light_front", "2.0f");
ConVar hlss_olivia_light_radius("hlss_olivia_light_radius", "100.0f");
ConVar hlss_olivia_light_radius_front("hlss_olivia_light_radius_front", "100.0f");
//ConVar hlss_olivia_light_distance("hlss_olivia_light_distance", "25");

#define DLIGHT_RADIUS (hlss_olivia_light_radius.GetFloat())
#define DLIGHT_RADIUS_FRONT (hlss_olivia_light_radius_front.GetFloat())
#define DLIGHT_MINLIGHT (hlss_olivia_light.GetFloat()/255.0f)
#define DLIGHT_MINLIGHT_FRONT (hlss_olivia_light_front.GetFloat()/255.0f)
#define DLIGHT_DISTANCE 25.0f //(hlss_olivia_light_distance.GetFloat())
#define DLIGHT_VECTOR_OFFSET Vector(0,0,20)

class C_NPC_Olivia : public C_AI_BaseNPC
{
	DECLARE_CLASS( C_NPC_Olivia, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

public:

	C_NPC_Olivia();
	~C_NPC_Olivia();

	virtual void	ClientThink( void );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	OnRestore( void );
	virtual void	UpdateOnRemove( void );
//	virtual void	ReceiveMessage( int classID, bf_read &msg );

	void			UpdateParticleEffects();

	bool	m_bDandelions;
	bool	m_bSmoking;
	bool	m_bOliviaLight;
	bool	m_bOliviaColorCorrection;
	int		m_nSmokeAttachment;
	int		m_nEyesAttachment;
	float	m_flAppearedTime;

	//dlight_t						*m_pDLight;
	dlight_t						*m_pDLight;
	dlight_t						*m_pELight;

private:
	
	CNewParticleEffect				*m_hDandelions;
	CNewParticleEffect				*m_hSmoke;

	ClientCCHandle_t m_CCHandle;

	CGlowObject *m_pGlowEffect;
	bool m_bGlowEnabled;
};

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Olivia, DT_NPC_Olivia, CNPC_Olivia )
	RecvPropBool( RECVINFO(m_bDandelions) ),
	RecvPropBool( RECVINFO(m_bSmoking) ),
	RecvPropBool( RECVINFO(m_bOliviaLight) ),
	RecvPropBool( RECVINFO(m_bOliviaColorCorrection) ),
	RecvPropInt( RECVINFO(m_nSmokeAttachment) ),
	RecvPropFloat( RECVINFO(m_flAppearedTime) ),
END_RECV_TABLE()

C_NPC_Olivia::C_NPC_Olivia()
{
	m_bDandelions = false;
	m_bSmoking = false;
	m_hDandelions = NULL;
	m_hSmoke = NULL;
	m_nSmokeAttachment = -1;
	m_nEyesAttachment = -1;
	m_bOliviaLight = false;
	m_bOliviaColorCorrection = false;

	m_pDLight = NULL;
	m_pELight = NULL;

	m_CCHandle = INVALID_CLIENT_CCHANDLE;

	m_pGlowEffect = nullptr;
	m_bGlowEnabled = false;
}

C_NPC_Olivia::~C_NPC_Olivia()
{
	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
	}

	if (m_pGlowEffect)
	{
		delete m_pGlowEffect;
		m_pGlowEffect = nullptr;
	}
}

void C_NPC_Olivia::ClientThink()
{
	// Don't update if our frame hasn't moved forward (paused)
	if ( gpGlobals->frametime <= 0.0f )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	//bool m_bOliviaColorCorrection = true;

	float flScale = 0.0f;

	if ( pPlayer && (m_bOliviaColorCorrection || m_bOliviaLight) && !IsDormant())
	{
		Vector vecForward;
		pPlayer->EyeVectors(&vecForward, NULL, NULL);

		Vector vecPlayer = (WorldSpaceCenter() - pPlayer->WorldSpaceCenter());
		float flDist = VectorNormalize(vecPlayer);
		float flDot = DotProduct(vecPlayer, vecForward);

		flScale = RemapValClamped(flDot, 0.1, 0.9, 0.0f, 1.0f) * RemapValClamped(flDist, 128.0f, 512.0f, 1.0f, 0.0f);
		//flScale *= clamp(gpGlobals->curtime - m_flAppearedTime, 0.0f, 1.0f);
	}

	if (pPlayer && m_bOliviaColorCorrection && !IsDormant())
	{
		if ( m_CCHandle == INVALID_CLIENT_CCHANDLE )
		{

			m_CCHandle = g_pColorCorrectionMgr->AddColorCorrection( "correction/olivia.raw" );
			SetNextClientThink( ( m_CCHandle != INVALID_CLIENT_CCHANDLE ) ? CLIENT_THINK_ALWAYS : CLIENT_THINK_NEVER );
		}

		g_pColorCorrectionMgr->SetColorCorrectionWeight( m_CCHandle, flScale );
	}
	else
	{
		if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
		{
			g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
			m_CCHandle = INVALID_CLIENT_CCHANDLE;
		}
	}

	if (pPlayer && m_bOliviaLight && !IsDormant())
	{
		

		Vector vecPlayer = EyePosition() - pPlayer->EyePosition();
		VectorNormalize(vecPlayer);
		vecPlayer.z *= 0.5f;

		Vector effect_origin = DLIGHT_VECTOR_OFFSET + WorldSpaceCenter() + (vecPlayer * DLIGHT_DISTANCE); 

		//debugoverlay->AddBoxOverlay( effect_origin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,0,0, 0, 0.1 );

		/*if (!m_pDLight)
		{
			m_pDLight = effects->CL_AllocDlight( entindex() );
			m_pDLight->die		= FLT_MAX;
		}

		if (m_pDLight)
		{
			m_pDLight->origin	= effect_origin;
			m_pDLight->radius	= DLIGHT_RADIUS; //random->RandomFloat( 245.0f, 256.0f );
			m_pDLight->minlight = DLIGHT_MINLIGHT * flScale;
			m_pDLight->color.r = 255 * flScale;
			m_pDLight->color.g = 255 * flScale;
			m_pDLight->color.b = 255 * flScale;
		}*/

		if (!m_pDLight)
		{
			m_pDLight = effects->CL_AllocDlight( entindex() );
			m_pDLight->die		= FLT_MAX;
		}

		if (m_pDLight)
		{
			m_pDLight->origin	= effect_origin;
			m_pDLight->radius	= DLIGHT_RADIUS;
			m_pDLight->minlight = DLIGHT_MINLIGHT * flScale;
			m_pDLight->color.r = 255 * flScale;
			m_pDLight->color.g = 255 * flScale;
			m_pDLight->color.b = 255 * flScale;
		}

		if (m_nEyesAttachment != -1)
		{
			Vector vecEyes;
			QAngle angEyes;
			GetAttachment(m_nEyesAttachment, vecEyes, angEyes);

			effect_origin = vecEyes - Vector(0,0,4) - (vecPlayer * DLIGHT_DISTANCE); 
		}

		if (!m_pELight)
		{
			m_pELight = effects->CL_AllocElight( entindex() );
			m_pELight->die		= FLT_MAX;
		}

		//debugoverlay->AddBoxOverlay( effect_origin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,0, 0, 0.1 );

		if (m_pELight)
		{
			m_pELight->origin	= effect_origin;
			m_pELight->radius	= DLIGHT_RADIUS_FRONT;
			m_pELight->minlight = DLIGHT_MINLIGHT_FRONT * flScale;
			m_pELight->color.r = 255 * flScale;
			m_pELight->color.g = 255 * flScale;
			m_pELight->color.b = 255 * flScale;
		}
	}
	else
	{
		/*if (m_pDLight)
		{
			m_pDLight->die = gpGlobals->curtime;
		}*/

		if (m_pDLight)
		{
			m_pDLight->die = gpGlobals->curtime;
			m_pDLight = NULL;
		}

		if (m_pELight)
		{
			m_pELight->die = gpGlobals->curtime;
			m_pELight = NULL;
		}
	}
	

	if (!m_bOliviaLight && !m_bOliviaColorCorrection)
	{
		SetNextClientThink( CLIENT_THINK_NEVER );
	}

	if (m_bGlowEnabled != m_bOliviaLight)
	{
		if (m_bOliviaLight)
		{
			//255, 255, 196, 64
			Vector vecColor(255, 255, 196);
			vecColor /= 255;
			if (!m_pGlowEffect)
				m_pGlowEffect = new CGlowObject(this, vecColor, 1.0f, false, true);
		}
		else
		{
			if (m_pGlowEffect)
			{
				delete m_pGlowEffect;
				m_pGlowEffect = nullptr;
			}
		}

		m_bGlowEnabled = m_bOliviaLight;
	}

	if (m_bGlowEnabled)
	{
		/*float flAlpha = 64 * flScale;
		flAlpha /= 255;*/
		m_pGlowEffect->SetAlpha(flScale);
	}

	UpdateParticleEffects();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_Olivia::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	//UpdateParticleEffects();

	if ( updateType == DATA_UPDATE_CREATED )
	{
		m_nEyesAttachment = LookupAttachment("eyes");
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_NPC_Olivia::OnRestore( void )
{
	BaseClass::OnRestore();

	m_nEyesAttachment = LookupAttachment("eyes");
	
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_NPC_Olivia::UpdateParticleEffects()
{
	if (m_bDandelions && !IsDormant())
	{
		if (!m_hDandelions)
		{

			// Place a beam between the two points //m_pEnt->
			CNewParticleEffect *pDandelions = ParticleProp()->Create( "dandelions_olivia", PATTACH_ABSORIGIN );
			if ( pDandelions )
			{
				m_hDandelions = pDandelions;
			}
		}
	}
	else
	{
		if ( m_hDandelions )
		{
			m_hDandelions->StopEmission();
			m_hDandelions = NULL;
		}
	}

	if (m_bSmoking && !IsDormant())
	{
		if (!m_hSmoke)
		{
			if (m_nSmokeAttachment > 0)
			{
				/*Vector	vecOrigin;
				QAngle	vecAngles;

				GetAttachment( m_nSmokeAttachment, vecOrigin, vecAngles );*/

				// Place a beam between the two points //m_pEnt->
				CNewParticleEffect *pSmoke = ParticleProp()->Create( "he_cigarette", PATTACH_POINT_FOLLOW, m_nSmokeAttachment );
				if ( pSmoke )
				{
					//pSmoke->SetControlPoint( 0, vecOrigin );
					m_hSmoke = pSmoke;
				}
			}
		}
	}
	else
	{
		if ( m_hSmoke )
		{
			m_hSmoke->StopEmission();
			m_hSmoke = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Receive messages from the server
//-----------------------------------------------------------------------------
/*void C_NPC_Olivia::ReceiveMessage( int classID, bf_read &msg )
{
	// Is the message for a sub-class?
	if ( classID != GetClientClass()->m_ClassID )
	{
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}


	int messageType = msg.ReadByte();

	switch( messageType)
	{
	case 0:
		{
			if (!m_hEffect)
			{
				// Find our attachment point
				unsigned char nAttachment = msg.ReadByte();
			
				// Get our attachment position
				Vector vecStart;
				QAngle vecAngles;
				GetAttachment( nAttachment, vecStart, vecAngles );

				// Place a beam between the two points //m_pEnt->
				CNewParticleEffect *pEffect = ParticleProp()->Create( "dandelions_olivia", PATTACH_ABSORIGIN, nAttachment );
				if ( pEffect )
				{
					pEffect->SetControlPoint( 0, vecStart );

					m_hEffect = pEffect;
				}
			}
		}
		break;
	default:
		{
			if ( m_hEffect )
			{
				m_hEffect->StopEmission();
				m_hEffect = NULL;
			}
		}
		break;
	}
}*/


void C_NPC_Olivia::UpdateOnRemove( void )
{
	if ( m_hDandelions )
	{
		m_hDandelions->StopEmission();
		m_hDandelions = NULL;
	}
	if ( m_hSmoke )
	{
		m_hSmoke->StopEmission();
		m_hSmoke = NULL;
	}

	/*if (m_pDLight)
	{
		m_pDLight->die = gpGlobals->curtime;
	}*/

	if (m_pDLight)
	{
		m_pDLight->die = gpGlobals->curtime;
		m_pDLight= NULL;
	}

	if (m_pELight)
	{
		m_pELight->die = gpGlobals->curtime;
		m_pELight = NULL;
	}

	if (m_CCHandle != INVALID_CLIENT_CCHANDLE)
	{
		g_pColorCorrectionMgr->RemoveColorCorrection( m_CCHandle );
		m_CCHandle = INVALID_CLIENT_CCHANDLE;
	}

	BaseClass::UpdateOnRemove();
}

class C_OliviaTarget : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_OliviaTarget, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

	C_OliviaTarget();

	virtual void	ClientThink(void);
	virtual void	OnDataChanged(DataUpdateType_t updateType);

protected:
	void	RegenerateDisplayText();

	char m_cDescription[DT_MAX_STRING_BUFFERSIZE];
	char m_cDisplayText[256];
};

//void RecvProxy_Description(const CRecvProxyData* pData, void* pStruct, void* pOut)
//{
//	char* pStrOut = (char*)pOut;
//	if (pData->m_pRecvProp->m_StringBufferSize <= 0)
//	{
//		return;
//	}
//
//	for (int i = 0; i < pData->m_pRecvProp->m_StringBufferSize; i++)
//	{
//		pStrOut[i] = pData->m_Value.m_pString[i];
//		if (pStrOut[i] == '/')
//		{
//			pStrOut[i] == '\n';
//		}
//
//		if (pStrOut[i] == 0)
//			break;
//	}
//
//	pStrOut[pData->m_pRecvProp->m_StringBufferSize - 1] = 0;
//}

IMPLEMENT_CLIENTCLASS_DT(C_OliviaTarget, DT_OliviaTarget, COliviaTarget)
RecvPropString(RECVINFO_NAME(m_cDescription, m_iszDescription)),
END_RECV_TABLE();

C_OliviaTarget::C_OliviaTarget()
{
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK | ENTCLIENTFLAG_DISABLEJIGGLEBONES;
}

void C_OliviaTarget::ClientThink(void)
{
	if (!IsDormant() && C_BasePlayer::GetLocalPlayer() && WorldSpaceCenter().DistToSqr(C_BasePlayer::GetLocalPlayer()->EyePosition()) <= Sqr(300.f))
	{
		char token[256];
		int iLine = 0;
		for (const char* p = nexttoken(token, m_cDisplayText, '/'); p && token[0]; p = nexttoken(token, p, '/'))
		{
			V_StrTrim(token);
			debugoverlay->AddEntityTextOverlay(entindex(), iLine++, gpGlobals->frametime, 200, 200, 200, 255, token);
		}
	}
}

void C_OliviaTarget::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	RegenerateDisplayText();

	SetNextClientThink(CLIENT_THINK_ALWAYS);
}

void C_OliviaTarget::RegenerateDisplayText()
{
	V_memset(m_cDisplayText, '\0', sizeof(m_cDisplayText));

	const char* binding = "special_attack";
	char szKeyBuff[256];
	if (::input->EnableJoystickMode())
	{
		int iNumBinds = 0;

		char szBuff[512];

		for (int iCode = JOYSTICK_FIRST; iCode <= JOYSTICK_LAST; ++iCode)
		{
			ButtonCode_t code = static_cast<ButtonCode_t>(iCode);

			bool bUseThisKey = false;

			// Only check against bind name if we haven't already forced this binding to be used
			const char* pBinding = gameuifuncs->GetBindingForButtonCode(code);

			if (!pBinding)
				continue;

			bUseThisKey = (Q_stricmp(pBinding, binding) == 0);

			//if (!bUseThisKey &&
			//	(Q_stricmp(pBinding, "+duck") == 0 || Q_stricmp(pBinding, "toggle_duck") == 0) &&
			//	(Q_stricmp(binding, "+duck") == 0 || Q_stricmp(binding, "toggle_duck") == 0))
			//{
			//	// +duck and toggle_duck are interchangable
			//	bUseThisKey = true;
			//}

			//if (!bUseThisKey &&
			//	(Q_stricmp(pBinding, "+zoom") == 0 || Q_stricmp(pBinding, "toggle_zoom") == 0) &&
			//	(Q_stricmp(binding, "+zoom") == 0 || Q_stricmp(binding, "toggle_zoom") == 0))
			//{
			//	// +zoom and toggle_zoom are interchangable
			//	bUseThisKey = true;
			//}

			// Don't use this bind in out list
			if (!bUseThisKey)
				continue;

			// Turn localized string into icon character
			Q_snprintf(szBuff, sizeof(szBuff), "#GameUI_KeyNames_%s", g_pInputSystem->ButtonCodeToString(static_cast<ButtonCode_t>(iCode)));
			const char* pszButtonName = g_pVGuiLocalize->FindAsUTF8(szBuff);
			if (pszButtonName)
			{
				V_strcpy_safe(szKeyBuff, pszButtonName);
				++iNumBinds;
			}
		}

		if (iNumBinds == 0)
		{
			V_strcpy_safe(szKeyBuff, g_pVGuiLocalize->FindAsUTF8("#GameUI_Icons_NONE"));
		}
	}
	else
	{
		const char* key = engine->Key_LookupBinding(*binding == '+' ? binding + 1 : binding);
		if (!key)
		{
			V_strcpy_safe(szKeyBuff, g_pVGuiLocalize->FindAsUTF8("#GameUI_Icons_NONE"));
		}
		else
			V_strcpy_safe(szKeyBuff, key);
	}

	V_sprintf_safe(m_cDisplayText, g_pVGuiLocalize->FindAsUTF8("#Olivia_Ask"), szKeyBuff);

	const char* pszDescription = g_pVGuiLocalize->FindAsUTF8(m_cDescription);
	if (pszDescription)
	{
		V_strcat_safe(m_cDisplayText, pszDescription);
	}
	else
	{
		V_strcat_safe(m_cDisplayText, m_cDescription);
	}
}
