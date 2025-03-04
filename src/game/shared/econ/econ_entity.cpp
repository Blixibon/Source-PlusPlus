//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "econ_entity.h"
#include "eventlist.h"
#include "saverestore.h"

//#ifdef GAME_DLL
//#include "tf_player.h"
//#else
//#include "c_tf_player.h"
//#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(EconEntity, DT_EconEntity)

BEGIN_NETWORK_TABLE(CEconEntity, DT_EconEntity)
#ifdef CLIENT_DLL
RecvPropDataTable(RECVINFO_DT(m_Item), 0, &REFERENCE_RECV_TABLE(DT_ScriptCreatedItem)),
RecvPropDataTable(RECVINFO_DT(m_AttributeManager), 0, &REFERENCE_RECV_TABLE(DT_AttributeContainer)),
#else
SendPropDataTable(SENDINFO_DT(m_Item), &REFERENCE_SEND_TABLE(DT_ScriptCreatedItem)),
SendPropDataTable(SENDINFO_DT(m_AttributeManager), &REFERENCE_SEND_TABLE(DT_AttributeContainer)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(C_EconEntity)
DEFINE_PRED_TYPEDESCRIPTION(m_AttributeManager, CAttributeContainer),
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC(CEconEntity)
DEFINE_EMBEDDED(m_AttributeManager),
END_DATADESC();

CEconEntity::CEconEntity()
{
	m_pAttributes = this;
}

CEconEntity::~CEconEntity()
{
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_AttributeManager.OnPreDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	m_AttributeManager.OnDataChanged( updateType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		/*C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
		if ( pAttach)
		{
			pAttach->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
		}*/
	}

	BaseClass::FireEvent( origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN )
	{
		//C_ViewmodelAttachmentModel *pAttach = GetViewmodelAddon();
		//if ( pAttach)
		{
			//pAttach->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CEconEntity::ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	// NUB
}
#else
//-----------------------------------------------------------------------------
// Purpose: Saves the current object out to disk, by iterating through the objects
//			data description hierarchy
// Input  : &save - save buffer which the class data is written to
// Output : int	- 0 if the save failed, 1 on success
//-----------------------------------------------------------------------------
int CEconEntity::Save(ISave& save)
{
	//save.StartBlock("EconItemView");
	int nItemIdx = m_Item.GetItemDefIndex();
	save.WriteInt(&nItemIdx);
	m_Item.SaveAttributeList(&save);
	//save.EndBlock();

	return BaseClass::Save(save);
}


//-----------------------------------------------------------------------------
// Purpose: Restores the current object from disk, by iterating through the objects
//			data description hierarchy
// Input  : &restore - restore buffer which the class data is read from
// Output : int	- 0 if the restore failed, 1 on success
//-----------------------------------------------------------------------------
int CEconEntity::Restore(IRestore& restore)
{
	//restore.StartBlock();
	int nItemIdx = restore.ReadInt();
	m_Item.Init(nItemIdx);
	m_Item.RestoreAttributeList(&restore);
	//restore.EndBlock();

	return BaseClass::Restore(restore);
}

//-----------------------------------------------------------------------------
// handler to do stuff after you are restored
//-----------------------------------------------------------------------------
void CEconEntity::OnRestore()
{
	BaseClass::OnRestore();

	ReapplyProvision();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::SetItem( CEconItemView &newItem )
{
	m_Item = newItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CEconEntity::GetItem( void )
{
	return &m_Item;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::HasItemDefinition( void ) const
{
	return ( m_Item.GetItemDefIndex() >= 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Shortcut to get item ID.
//-----------------------------------------------------------------------------
int CEconEntity::GetItemID( void )
{
	return m_Item.GetItemDefIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Derived classes need to override this.
//-----------------------------------------------------------------------------
void CEconEntity::GiveTo( CBaseEntity *pEntity )
{
}

//-----------------------------------------------------------------------------
// Purpose: Add or remove this from owner's attribute providers list.
//-----------------------------------------------------------------------------
void CEconEntity::ReapplyProvision( void )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	CBaseEntity *pOldOwner = m_hOldOwner.Get();

	if ( pOwner != pOldOwner )
	{
		if ( pOldOwner )
		{
			m_AttributeManager.StopProvidingTo( pOldOwner );
		}

		if ( pOwner )
		{
			m_AttributeManager.ProviteTo( pOwner );
			m_hOldOwner = pOwner;
		}
		else
		{
			m_hOldOwner = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update visible bodygroups
//-----------------------------------------------------------------------------
void CEconEntity::UpdatePlayerBodygroups( void )
{
	CBaseAnimating *pPlayer = dynamic_cast < CBaseAnimating * >( GetOwnerEntity() );

	if ( !pPlayer )
	{
		return;
	}

	// bodygroup enabling/disabling
	CEconItemDefinition *pStatic = m_Item.GetStaticData();
	if ( pStatic )
	{
		EconItemVisuals *pVisuals =	pStatic->GetVisuals();
		if ( pVisuals )
		{
			for ( int i = 0; i < pPlayer->GetNumBodyGroups(); i++ )
			{
				unsigned int index = pVisuals->player_bodygroups.Find( pPlayer->GetBodygroupName( i ) );
				if ( pVisuals->player_bodygroups.IsValidIndex( index ) )
				{
					bool bTrue = pVisuals->player_bodygroups.Element( index );
					if ( bTrue )
					{
						pPlayer->SetBodygroup( i , 1 );
					}
					else
					{
						pPlayer->SetBodygroup( i , 0 );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconEntity::UpdateOnRemove( void )
{
	SetOwnerEntity( NULL );
	ReapplyProvision();
	BaseClass::UpdateOnRemove();
}
