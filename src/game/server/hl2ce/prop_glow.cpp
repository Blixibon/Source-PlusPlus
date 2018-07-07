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
	void				InputAddGlowEffect( inputdata_t &inputdata )	{ AddGlowEffect(); }
	void				InputSetGlowColor( inputdata_t &inputdata );
protected:
	CNetworkVar( bool, m_bGlowEnabled );
	CNetworkVector( m_vGlowColor );
};

LINK_ENTITY_TO_CLASS(prop_glow, CGlowProp);

BEGIN_DATADESC(CGlowProp)
	DEFINE_INPUTFUNC( FIELD_VOID, "AddGlowEffect", InputAddGlowEffect ),
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

void CGlowProp::AddGlowEffect( void )
{
	SetTransmitState( FL_EDICT_ALWAYS );
	m_bGlowEnabled.Set( true );
}

void CGlowProp::RemoveGlowEffect( void )
{
	m_bGlowEnabled.Set( false );
}

bool CGlowProp::IsGlowEffectActive( void )
{
	return m_bGlowEnabled;
}

void CGlowProp::InputSetGlowColor( inputdata_t &inputdata )
{
	m_vGlowColor.SetX(inputdata.value.Color32().r / 255.0f);
	m_vGlowColor.SetY(inputdata.value.Color32().g / 255.0f);
	m_vGlowColor.SetZ(inputdata.value.Color32().b / 255.0f);
}