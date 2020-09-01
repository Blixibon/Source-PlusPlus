//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "animation.h"
#include "baseviewmodel.h"
#include "player.h"
#include <KeyValues.h>
#include "studio.h"
#include "vguiscreen.h"
#include "saverestore_utlvector.h"
#include "hltvdirector.h"
#include "npcevent.h"
#include "eventlist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SendProxy_AnimTime( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_SequenceChanged( const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );

//-----------------------------------------------------------------------------
// Purpose: Save Data for Base Weapon object
//-----------------------------------------------------------------------------// 
BEGIN_DATADESC( CBaseViewModel )

	DEFINE_FIELD( m_hOwner, FIELD_EHANDLE ),

// Client only
//	DEFINE_FIELD( m_LagAnglesHistory, CInterpolatedVar < QAngle > ),
//	DEFINE_FIELD( m_vLagAngles, FIELD_VECTOR ),

	DEFINE_FIELD( m_nViewModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_flTimeWeaponIdle, FIELD_FLOAT ),
	DEFINE_FIELD( m_nAnimationParity, FIELD_INTEGER ),

	// Client only
//	DEFINE_FIELD( m_nOldAnimationParity, FIELD_INTEGER ),

	DEFINE_FIELD( m_vecLastFacing, FIELD_VECTOR ),
	DEFINE_FIELD( m_hWeapon, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_hScreens, FIELD_EHANDLE ),

// Read from weapons file
//	DEFINE_FIELD( m_sVMName, FIELD_STRING ),
//	DEFINE_FIELD( m_sAnimationPrefix, FIELD_STRING ),

// ---------------------------------------------------------------------

// Don't save these, init to 0 and regenerate
//	DEFINE_FIELD( m_Activity, FIELD_INTEGER ),


DEFINE_FIELD(m_iHandsModelIndex, FIELD_MODELINDEX),
DEFINE_FIELD(m_iHandsSkin, FIELD_INTEGER),
DEFINE_FIELD(m_iHandsBody, FIELD_INTEGER),

END_DATADESC()

int CBaseViewModel::UpdateTransmitState()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}

	return SetTransmitState( FL_EDICT_FULLCHECK );
}

int CBaseViewModel::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// check if receipient owns this weapon viewmodel
	CBasePlayer *pOwner = ToBasePlayer( m_hOwner );

	if ( pOwner && pOwner->edict() == pInfo->m_pClientEnt )
	{
		return FL_EDICT_ALWAYS;
	}

	// check if recipient spectates the own of this viewmodel
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	if ( pRecipientEntity->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pRecipientEntity );
#ifndef _XBOX
		if ( pPlayer->IsHLTV() || pPlayer->IsReplay() )
		{
			// if this is the HLTV client, transmit all viewmodels in our PVS
			return FL_EDICT_PVSCHECK;
		}
#endif
		if ( (pPlayer->GetObserverMode() == OBS_MODE_IN_EYE)  && (pPlayer->GetObserverTarget() == pOwner) )
		{
			return FL_EDICT_ALWAYS;
		}
	}

	// Don't send to anyone else except the local player or his spectators
	return FL_EDICT_DONTSEND;
}

void CBaseViewModel::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Force our screens to be sent too.
	for ( int i=0; i < m_hScreens.Count(); i++ )
	{
		CVGuiScreen *pScreen = m_hScreens[i].Get();
		if ( pScreen )
			pScreen->SetTransmit( pInfo, bAlways );
	}
}

void CBaseViewModel::SetHandsModel(const char *pchModelName, int iSkin)
{
	MDLCACHE_CRITICAL_SECTION();

	// delete exiting studio model container
	UnlockHandHdr();
	delete m_pHandsStudioHdr;
	m_pHandsStudioHdr = NULL;

	int iModel = modelinfo->GetModelIndex(pchModelName);
	if (iModel > 0 && iModel != m_iHandsModelIndex)
		m_iHandsModelIndex = iModel;

	if (iSkin != m_iHandsSkin)
		m_iHandsSkin = iSkin;

	m_iHandsBody = 0;

	LockHandHdr();
}

void CBaseViewModel::SetHandsBodygroupByName(const char *pchGroup, int iValue)
{
	int iGroup = ::FindBodygroupByName(m_pHandsStudioHdr, pchGroup);

	if (iGroup <= -1)
		return;

	::SetBodygroup(m_pHandsStudioHdr, m_iHandsBody.GetForModify(), iGroup, iValue);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CBaseViewModel::Restore(IRestore &restore)
{
	int result = BaseClass::Restore(restore);
	LockHandHdr();
	return result;
}

void CBaseViewModel::LockHandHdr()
{
	const model_t *mdl = modelinfo->GetModel(m_iHandsModelIndex);
	if (mdl)
	{
		MDLHandle_t hStudioHdr = modelinfo->GetCacheHandle(mdl);
		if (hStudioHdr != MDLHANDLE_INVALID)
		{
			const studiohdr_t *pStudioHdr = mdlcache->LockStudioHdr(hStudioHdr);
			CStudioHdr *pStudioHdrContainer = NULL;
			if (!m_pHandsStudioHdr)
			{
				if (pStudioHdr)
				{
					pStudioHdrContainer = new CStudioHdr;
					pStudioHdrContainer->Init(pStudioHdr, mdlcache);
				}
			}
			else
			{
				pStudioHdrContainer = m_pHandsStudioHdr;
			}

			Assert((pStudioHdr == NULL && pStudioHdrContainer == NULL) || pStudioHdrContainer->GetRenderHdr() == pStudioHdr);

			if (pStudioHdrContainer && pStudioHdrContainer->GetVirtualModel())
			{
				MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(pStudioHdrContainer->GetRenderHdr()->virtualModel) & 0xffff;
				mdlcache->LockStudioHdr(hVirtualModel);
			}
			m_pHandsStudioHdr = pStudioHdrContainer; // must be last to ensure virtual model correctly set up
		}
	}
}

void CBaseViewModel::UnlockHandHdr()
{
	if (m_pHandsStudioHdr)
	{
		const model_t *mdl = GetModel();
		if (mdl)
		{
			mdlcache->UnlockStudioHdr(modelinfo->GetCacheHandle(mdl));
			if (m_pHandsStudioHdr->GetVirtualModel())
			{
				MDLHandle_t hVirtualModel = (MDLHandle_t)(int)(m_pHandsStudioHdr->GetRenderHdr()->virtualModel) & 0xffff;
				mdlcache->UnlockStudioHdr(hVirtualModel);
			}
		}
	}
}

void CBaseViewModel::HandleAnimEvent(animevent_t* pEvent)
{
	if (pEvent->type & AE_TYPE_NEWEVENTSYSTEM)
	{
		if (pEvent->event == AE_VM_BODYGROUP_SET)
		{
			int value;
			char token[256];
			char szBodygroupName[256];

			const char* p = pEvent->options;

			// Bodygroup Name
			p = nexttoken(token, p, ' ');
			Q_strncpy(szBodygroupName, token, sizeof(szBodygroupName));

			// Get the desired value
			p = nexttoken(token, p, ' ');
			value = token[0] ? atoi(token) : 0;

			int index = FindBodygroupByName(szBodygroupName);
			if (index >= 0)
			{
				SetBodygroup(index, value);
			}

			return;
		}
		else if (pEvent->event == AE_VM_DISABLE_BODYGROUP)
		{
			int index = FindBodygroupByName(pEvent->options);
			if (index >= 0)
			{
				SetBodygroup(index, 0);
			}
			return;
		}
		else if (pEvent->event == AE_VM_ENABLE_BODYGROUP)
		{
			int index = FindBodygroupByName(pEvent->options);
			if (index >= 0)
			{
				SetBodygroup(index, 1);
			}
			return;
		}
	}

	BaseClass::HandleAnimEvent(pEvent);
}
