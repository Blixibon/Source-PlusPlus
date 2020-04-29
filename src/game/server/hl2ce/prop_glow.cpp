#include "cbase.h"
#include "props.h"

class CGlowProp : public CDynamicProp
{
public:
	DECLARE_CLASS(CGlowProp, CDynamicProp);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC()
	CGlowProp();

	void				AddGlowEffect( void );
	void				RemoveGlowEffect( void );
	bool				IsGlowEffectActive( void );
	void				SetGlowColor(color32 clr);
	void				InputSetGlowColor( inputdata_t &inputdata );
	void				InputEnableGlow(inputdata_t &inputdata) { AddGlowEffect(); }
	void				InputDisableGlow(inputdata_t &inputdata) { RemoveGlowEffect(); }

	bool KeyValue(const char *szKeyName, const char *szValue);
	virtual int			UpdateTransmitState();
protected:
	CNetworkVar( bool, m_bGlowEnabled );
	CNetworkVector( m_vGlowColor );

	CNetworkVar(float, m_flScaleX);
	CNetworkVar(float, m_flScaleY);
	CNetworkVar(float, m_flScaleZ);

	CNetworkVar(float, m_flLerpTimeX);
	CNetworkVar(float, m_flLerpTimeY);
	CNetworkVar(float, m_flLerpTimeZ);

	CNetworkVar(float, m_flGoalTimeX);
	CNetworkVar(float, m_flGoalTimeY);
	CNetworkVar(float, m_flGoalTimeZ);

	void InputSetScaleX(inputdata_t& inputdata);
	void InputSetScaleY(inputdata_t& inputdata);
	void InputSetScaleZ(inputdata_t& inputdata);
};

LINK_ENTITY_TO_CLASS(prop_dynamic_glow, CGlowProp);
LINK_ENTITY_TO_CLASS(prop_dynamic_special, CGlowProp);

BEGIN_DATADESC(CGlowProp)
	DEFINE_KEYFIELD(m_bGlowEnabled, FIELD_BOOLEAN, "glowenabled"),
	DEFINE_FIELD(m_vGlowColor, FIELD_VECTOR),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetGlowEnabled", InputEnableGlow),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetGlowDisabled", InputDisableGlow),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),

	DEFINE_INPUTFUNC(FIELD_VECTOR, "SetScaleX", InputSetScaleX),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "SetScaleY", InputSetScaleY),
	DEFINE_INPUTFUNC(FIELD_VECTOR, "SetScaleZ", InputSetScaleZ),

	DEFINE_FIELD(m_flScaleX, FIELD_FLOAT),
	DEFINE_FIELD(m_flScaleY, FIELD_FLOAT),
	DEFINE_FIELD(m_flScaleZ, FIELD_FLOAT),

	DEFINE_FIELD(m_flLerpTimeX, FIELD_FLOAT),
	DEFINE_FIELD(m_flLerpTimeY, FIELD_FLOAT),
	DEFINE_FIELD(m_flLerpTimeZ, FIELD_FLOAT),

	DEFINE_FIELD(m_flGoalTimeX, FIELD_FLOAT),
	DEFINE_FIELD(m_flGoalTimeY, FIELD_FLOAT),
	DEFINE_FIELD(m_flGoalTimeZ, FIELD_FLOAT),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CGlowProp, DT_GlowProp)
SendPropFloat(SENDINFO(m_flScaleX), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO(m_flScaleY), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO(m_flScaleZ), 0, SPROP_NOSCALE),

SendPropFloat(SENDINFO(m_flLerpTimeX), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO(m_flLerpTimeY), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO(m_flLerpTimeZ), 0, SPROP_NOSCALE),

SendPropFloat(SENDINFO(m_flGoalTimeX), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO(m_flGoalTimeY), 0, SPROP_NOSCALE),
SendPropFloat(SENDINFO(m_flGoalTimeZ), 0, SPROP_NOSCALE),

	SendPropBool( SENDINFO( m_bGlowEnabled ) ),
	SendPropVector( SENDINFO( m_vGlowColor ) ),
END_SEND_TABLE();

CGlowProp::CGlowProp()
{
	m_bGlowEnabled.Set( false );

	m_flScaleX = 1.0f;
	m_flScaleY = 1.0f;
	m_flScaleZ = 1.0f;
}

bool CGlowProp::KeyValue(const char * szKeyName, const char * szValue)
{
	if (FStrEq(szKeyName, "glowcolor"))
	{
		color32 clr;
		UTIL_StringToColor32(&clr, szValue);
		SetGlowColor(clr);
		return true;
	}

	return BaseClass::KeyValue(szKeyName, szValue);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CGlowProp::UpdateTransmitState()
{
	if (IsGlowEffectActive())
	{
		return SetTransmitState(FL_EDICT_ALWAYS);
	}

	return BaseClass::UpdateTransmitState();
}

void CGlowProp::AddGlowEffect( void )
{
	m_bGlowEnabled.Set( true );
	DispatchUpdateTransmitState();
}

void CGlowProp::RemoveGlowEffect( void )
{
	m_bGlowEnabled.Set( false );
	DispatchUpdateTransmitState();
}

bool CGlowProp::IsGlowEffectActive( void )
{
	return m_bGlowEnabled.Get();
}

void CGlowProp::SetGlowColor(color32 clr)
{
	m_vGlowColor.SetX(clr.r / 255.0f);
	m_vGlowColor.SetY(clr.g / 255.0f);
	m_vGlowColor.SetZ(clr.b / 255.0f);
}

void CGlowProp::InputSetGlowColor( inputdata_t &inputdata )
{
	SetGlowColor(inputdata.value.Color32());
}

void CGlowProp::InputSetScaleX(inputdata_t& inputdata)
{
	Vector vecScale;
	inputdata.value.Vector3D(vecScale);

	m_flScaleX = vecScale.x;
	m_flLerpTimeX = vecScale.y;
	m_flGoalTimeX = gpGlobals->curtime;
}

void CGlowProp::InputSetScaleY(inputdata_t& inputdata)
{
	Vector vecScale;
	inputdata.value.Vector3D(vecScale);

	m_flScaleY = vecScale.x;
	m_flLerpTimeY = vecScale.y;
	m_flGoalTimeY = gpGlobals->curtime;
}

void CGlowProp::InputSetScaleZ(inputdata_t& inputdata)
{
	Vector vecScale;
	inputdata.value.Vector3D(vecScale);

	m_flScaleZ = vecScale.x;
	m_flLerpTimeZ = vecScale.y;
	m_flGoalTimeZ = gpGlobals->curtime;
}