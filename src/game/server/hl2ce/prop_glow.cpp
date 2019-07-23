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
};

LINK_ENTITY_TO_CLASS(prop_dynamic_glow, CGlowProp);

BEGIN_DATADESC(CGlowProp)
	DEFINE_KEYFIELD(m_bGlowEnabled, FIELD_BOOLEAN, "glowenabled"),
	DEFINE_FIELD(m_vGlowColor, FIELD_VECTOR),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetGlowEnabled", InputEnableGlow),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetGlowDisabled", InputDisableGlow),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CGlowProp, DT_GlowProp)
	SendPropBool( SENDINFO( m_bGlowEnabled ) ),
	SendPropVector( SENDINFO( m_vGlowColor ) ),
END_SEND_TABLE();

CGlowProp::CGlowProp()
{
	m_bGlowEnabled.Set( false );
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
	return m_bGlowEnabled;
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