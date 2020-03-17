//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SAVERESTORE_STRINGTABLE_H
#define SAVERESTORE_STRINGTABLE_H

#if defined( _WIN32 )
#pragma once
#endif


#include "isaverestore.h"
#include "networkstringtabledefs.h"


//-------------------------------------

class CStringTableSaveRestoreOps : public CDefSaveRestoreOps
{
public:
	void Init( INetworkStringTable* pNetworkStringTable )
	{
		m_pStringTable = pNetworkStringTable;
	}

	// save data type interface
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		int *pStringIndex = (int *)fieldInfo.pField;
		const char *pString = m_pStringTable->GetString( *pStringIndex );
		int nLen = Q_strlen( pString ) + 1;
		pSave->WriteInt( &nLen );
		pSave->WriteString( pString );
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		int *pStringIndex = (int *)fieldInfo.pField;
		int nLen = pRestore->ReadInt();
		char *pTemp = (char *)stackalloc( nLen );
		pRestore->ReadString( pTemp, nLen, nLen );
		*pStringIndex = m_pStringTable->AddString( CBaseEntity::IsServer(), pTemp );
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		int *pStringIndex = (int *)fieldInfo.pField;
		*pStringIndex = INVALID_STRING_INDEX;
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		int *pStringIndex = (int *)fieldInfo.pField;
		return IsInvalidString(*pStringIndex);
	}

	inline bool IsInvalidString(int iString) { return (unsigned short)iString == INVALID_STRING_INDEX; }

protected:
	INetworkStringTable *m_pStringTable;
};

#endif // SAVERESTORE_STRINGTABLE_H
