#include "cbase.h"
#include "c_props.h"

class C_GlowProp : public CDynamicProp
{
public:
	DECLARE_CLASS(C_GlowProp, CDynamicProp);
	DECLARE_CLIENTCLASS();

	C_GlowProp();

	virtual void ApplyBoneMatrixTransform(matrix3x4_t& transform);
	virtual void GetRenderBounds(Vector& theMins, Vector& theMaxs);

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	UpdateGlowEffect( void );
	virtual void	DestroyGlowEffect( void );
	virtual void	UpdateGlowColor( void );

	// Must be available to proxy functions
	float m_flScaleX;
	float m_flScaleY;
	float m_flScaleZ;

	float m_flLerpTimeX;
	float m_flLerpTimeY;
	float m_flLerpTimeZ;

	float m_flGoalTimeX;
	float m_flGoalTimeY;
	float m_flGoalTimeZ;

	float m_flCurrentScale[3];
	bool  m_bRunningScale[3];
	float m_flTargetScale[3];
protected:
	void	CalculateScale(void);
	float	m_nCalcFrame;	// Frame the last calculation was made at

	bool				m_bGlowEnabled;
	bool				m_bOldGlowEnabled;
	CGlowObject			*m_pGlowEffect;
	Vector				m_vGlowColor;
};

namespace GlowProp
{
	void RecvProxy_ScaleX(const CRecvProxyData* pData, void* pStruct, void* pOut)
	{
		C_GlowProp* pCoreData = (C_GlowProp*)pStruct;

		pCoreData->m_flScaleX = pData->m_Value.m_Float;

		if (pCoreData->m_bRunningScale[0] == true)
		{
			pCoreData->m_flTargetScale[0] = pCoreData->m_flCurrentScale[0];
		}
	}

	void RecvProxy_ScaleY(const CRecvProxyData* pData, void* pStruct, void* pOut)
	{
		C_GlowProp* pCoreData = (C_GlowProp*)pStruct;

		pCoreData->m_flScaleY = pData->m_Value.m_Float;

		if (pCoreData->m_bRunningScale[1] == true)
		{
			pCoreData->m_flTargetScale[1] = pCoreData->m_flCurrentScale[1];
		}
	}

	void RecvProxy_ScaleZ(const CRecvProxyData* pData, void* pStruct, void* pOut)
	{
		C_GlowProp* pCoreData = (C_GlowProp*)pStruct;

		pCoreData->m_flScaleZ = pData->m_Value.m_Float;

		if (pCoreData->m_bRunningScale[2] == true)
		{
			pCoreData->m_flTargetScale[2] = pCoreData->m_flCurrentScale[2];
		}
	}
}

BEGIN_RECV_TABLE( C_GlowProp, DT_GlowProp )
	RecvPropFloat(RECVINFO(m_flScaleX), 0, GlowProp::RecvProxy_ScaleX),
	RecvPropFloat(RECVINFO(m_flScaleY), 0, GlowProp::RecvProxy_ScaleY),
	RecvPropFloat(RECVINFO(m_flScaleZ), 0, GlowProp::RecvProxy_ScaleZ),

	RecvPropFloat(RECVINFO(m_flLerpTimeX)),
	RecvPropFloat(RECVINFO(m_flLerpTimeY)),
	RecvPropFloat(RECVINFO(m_flLerpTimeZ)),

	RecvPropFloat(RECVINFO(m_flGoalTimeX)),
	RecvPropFloat(RECVINFO(m_flGoalTimeY)),
	RecvPropFloat(RECVINFO(m_flGoalTimeZ)),

	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
	RecvPropVector( RECVINFO( m_vGlowColor ) ),
END_RECV_TABLE();

C_GlowProp::C_GlowProp(void) 
{
	m_flTargetScale[0] = 1.0f;
	m_flTargetScale[1] = 1.0f;
	m_flTargetScale[2] = 1.0f;

	m_bRunningScale[0] = false;
	m_bRunningScale[1] = false;
	m_bRunningScale[2] = false;

	m_nCalcFrame = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Calculates the scake of the object once per frame
//-----------------------------------------------------------------------------
void C_GlowProp::CalculateScale(void)
{
	// Don't bother to calculate this for a second time in the same frame
	if (m_nCalcFrame == gpGlobals->framecount)
		return;

	// Mark that we cached this value for the frame
	m_nCalcFrame = gpGlobals->framecount;

	float flVal[3] = { m_flTargetScale[0], m_flTargetScale[1], m_flTargetScale[2] };
	float* flTargetScale[3] = { &m_flTargetScale[0], &m_flTargetScale[1], &m_flTargetScale[2] };
	float flScale[3] = { m_flScaleX, m_flScaleY, m_flScaleZ };
	float flLerpTime[3] = { m_flLerpTimeX, m_flLerpTimeY, m_flLerpTimeZ };
	float flGoalTime[3] = { m_flGoalTimeX, m_flGoalTimeY, m_flGoalTimeZ };
	bool* bRunning[3] = { &m_bRunningScale[0], &m_bRunningScale[1], &m_bRunningScale[2] };

	for (int i = 0; i < 3; i++)
	{
		if (*flTargetScale[i] != flScale[i])
		{
			float deltaTime = (float)(gpGlobals->curtime - flGoalTime[i]) / flLerpTime[i];
			float flRemapVal = SimpleSplineRemapValClamped(deltaTime, 0.0f, 1.0f, *flTargetScale[i], flScale[i]);

			*bRunning[i] = true;

			if (deltaTime >= 1.0f)
			{
				*flTargetScale[i] = flScale[i];
				*bRunning[i] = false;
			}

			flVal[i] = flRemapVal;
			m_flCurrentScale[i] = flVal[i];
		}
		else
		{
			m_flCurrentScale[i] = m_flTargetScale[i];
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scales the bones based on the current scales
//-----------------------------------------------------------------------------
void C_GlowProp::ApplyBoneMatrixTransform(matrix3x4_t& transform)
{
	BaseClass::ApplyBoneMatrixTransform(transform);

	// Find the scale for this frame
	CalculateScale();

	VectorScale(transform[0], m_flCurrentScale[0], transform[0]);
	VectorScale(transform[1], m_flCurrentScale[1], transform[1]);
	VectorScale(transform[2], m_flCurrentScale[2], transform[2]);

	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose: Ensures the render bounds match the scales
//-----------------------------------------------------------------------------
void C_GlowProp::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	BaseClass::GetRenderBounds(theMins, theMaxs);

	// Find the scale for this frame
	CalculateScale();

	// Extend our render bounds to encompass the scaled object
	theMins.x *= m_flCurrentScale[0];
	theMins.y *= m_flCurrentScale[1];
	theMins.z *= m_flCurrentScale[2];

	theMaxs.x *= m_flCurrentScale[0];
	theMaxs.y *= m_flCurrentScale[1];
	theMaxs.z *= m_flCurrentScale[2];

	Assert(theMins.IsValid() && theMaxs.IsValid());

}

void C_GlowProp::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bOldGlowEnabled != m_bGlowEnabled )
	{
		m_bOldGlowEnabled = m_bGlowEnabled;
		UpdateGlowEffect();
	}
	UpdateGlowColor();
}

void C_GlowProp::UpdateGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	if ( m_bGlowEnabled )
	{
		m_pGlowEffect = new CGlowObject( this, Vector( 0, 0, 0 ), 1.0, true, true );
	}
}

void C_GlowProp::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}

void C_GlowProp::UpdateGlowColor( void )
{
	if ( m_pGlowEffect )
	{
		m_pGlowEffect->SetColor(m_vGlowColor);
	}
}

IMPLEMENT_CLIENTCLASS(C_GlowProp, DT_GlowProp, CGlowProp);