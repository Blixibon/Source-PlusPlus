#include "cbase.h"
#include "c_props.h"

class C_GlowProp : public CDynamicProp
{
public:
	DECLARE_CLASS(C_GlowProp, CDynamicProp);
	DECLARE_CLIENTCLASS();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	UpdateGlowEffect( void );
	virtual void	DestroyGlowEffect( void );
	virtual void	UpdateGlowColor( void );
protected:
	bool				m_bGlowEnabled;
	bool				m_bOldGlowEnabled;
	CGlowObject			*m_pGlowEffect;
	Vector				m_vGlowColor;
};

LINK_ENTITY_TO_CLASS(prop_glow, C_GlowProp);

BEGIN_RECV_TABLE( C_GlowProp, DT_GlowProp )
	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
	RecvPropVector( RECVINFO( m_vGlowColor ) ),
END_RECV_TABLE();

void C_GlowProp::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bOldGlowEnabled != m_bGlowEnabled )
	{
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
		m_pGlowEffect = new CGlowObject( this, Vector( 0, 0, 0 ), 1.0, true );
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