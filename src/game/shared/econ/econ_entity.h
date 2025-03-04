//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef ECON_ENTITY_H
#define ECON_ENTITY_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CEconEntity C_EconEntity
#endif

#include "ihasattributes.h"
#include "econ_item_view.h"
#include "attribute_manager.h"

#ifdef CLIENT_DLL
//#include "c_tf_viewmodeladdon.h"
#endif

struct wearableanimplayback_t
{
	int iStub;
};

//-----------------------------------------------------------------------------
// Purpose: BaseCombatWeapon is derived from this in live tf2.
//-----------------------------------------------------------------------------
class CEconEntity : public CBaseAnimating, public IHasAttributes
{
	DECLARE_CLASS( CEconEntity, CBaseAnimating );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

#ifdef CLIENT_DLL
	DECLARE_PREDICTABLE();
#endif

public:
	CEconEntity();
	~CEconEntity();

#ifdef CLIENT_DLL
	virtual void OnPreDataChanged( DataUpdateType_t );
	virtual void OnDataChanged( DataUpdateType_t );
	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
	//virtual C_ViewmodelAttachmentModel *GetViewmodelAddon( void ) { return NULL; }
	virtual void ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
#else
	// save/restore
	// only overload these if you have special data to serialize
	virtual int	Save(ISave& save);
	virtual int	Restore(IRestore& restore);

	// handler to reset stuff after you are restored
	// called after all entities have been loaded from all affected levels
	// called before activate
	// NOTE: Always chain to base class when implementing this!
	virtual void OnRestore();
#endif

	virtual int TranslateViewmodelHandActivity( int iActivity ) { return iActivity; }

	virtual void PlayAnimForPlaybackEvent( wearableanimplayback_t iPlayback ) {};

	virtual void SetItem( CEconItemView &newItem );
	CEconItemView *GetItem( void );
	virtual bool HasItemDefinition( void ) const;
	virtual int GetItemID( void );

	virtual void GiveTo( CBaseEntity *pEntity );

	virtual CAttributeManager *GetAttributeManager( void ) { return &m_AttributeManager; }
	virtual CAttributeContainer *GetAttributeContainer( void ) { return &m_AttributeManager; }
	virtual CBaseEntity *GetAttributeOwner( void ) { return NULL; }
	virtual void ReapplyProvision( void );

	void UpdatePlayerModelToClass( void );

	virtual void UpdatePlayerBodygroups( void );

	virtual void UpdateOnRemove( void );

protected:
	EHANDLE m_hOldOwner;
	CEconItemView m_Item;

private:
	CNetworkVarEmbedded( CAttributeContainer, m_AttributeManager );
};

#endif
