//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
//			some of the code normally available in games like L4D2 or Valve's original VScript DLL.
//			Instead, that code is recreated here, shared between server and client.
//
//			It also contains other functions unique to Mapbase.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "responserules/matchers.h"
#include "takedamageinfo.h"
#include "tier0/icommandline.h"
#include "GameEventListener.h"
#include "utlbuffer.h"
#include "KeyValues.h"
#include "filesystem.h"

#ifndef CLIENT_DLL
#include "globalstate.h"
#include "vscript_server.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptNetPropManager
{
public:

#ifdef CLIENT_DLL
	RecvProp *RecurseTable( RecvTable *pTable, const char *pszPropName )
#else
	SendProp *RecurseTable( SendTable *pTable, const char *pszPropName )
#endif
	{
#ifdef CLIENT_DLL
		RecvProp *pProp = NULL;
#else
		SendProp *pProp = NULL;
#endif
		for (int i = 0; i < pTable->GetNumProps(); i++)
		{
			pProp = pTable->GetProp( i );
			if (pProp->GetType() == DPT_DataTable)
			{
				pProp = RecurseTable(pProp->GetDataTable(), pszPropName);
				if (pProp)
					return pProp;
			}
			else
			{
				if (FStrEq( pProp->GetName(), pszPropName ))
					return pProp;
			}
		}

		return NULL;
	}

#ifdef CLIENT_DLL
	RecvProp *RecurseNetworkClass( ClientClass *pClass, const char *pszPropName )
#else
	SendProp *RecurseNetworkClass( ServerClass *pClass, const char *pszPropName )
#endif
	{
#ifdef CLIENT_DLL
		RecvProp *pProp = RecurseTable( pClass->m_pRecvTable, pszPropName );
#else
		SendProp *pProp = RecurseTable( pClass->m_pTable, pszPropName );
#endif
		if (pProp)
			return pProp;

		if (pClass->m_pNext)
			return RecurseNetworkClass( pClass->m_pNext, pszPropName );
		else
			return NULL;
	}

#ifdef CLIENT_DLL
	RecvProp *GetPropByName( CBaseEntity *pEnt, const char *pszPropName )
	{
		if (pEnt)
		{
			return RecurseNetworkClass( pEnt->GetClientClass(), pszPropName );
		}

		return NULL;
	}
#else
	SendProp *GetPropByName( CBaseEntity *pEnt, const char *pszPropName )
	{
		if (pEnt)
		{
			return RecurseNetworkClass( pEnt->GetServerClass(), pszPropName );
		}

		return NULL;
	}
#endif

	int GetPropArraySize( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp)
		{
			// TODO: Is this what this function wants?
			return pProp->GetNumElements();
		}

		return -1;
	}

	#define GetPropFunc( name, varType, propType, defaultval ) \
	varType name( HSCRIPT hEnt, const char *pszPropName ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			return *(varType*)((char *)pEnt + pProp->GetOffset()); \
		} \
		return defaultval; \
	} \

	#define GetPropFuncArray( name, varType, propType, defaultval ) \
	varType name( HSCRIPT hEnt, const char *pszPropName, int iArrayElement ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			return ((varType*)((char *)pEnt + pProp->GetOffset()))[iArrayElement]; \
		} \
		return defaultval; \
	} \

	GetPropFunc( GetPropFloat, float, DPT_Float, -1 );
	GetPropFuncArray( GetPropFloatArray, float, DPT_Float, -1 );
	GetPropFunc( GetPropInt, int, DPT_Int, -1 );
	GetPropFuncArray( GetPropIntArray, int, DPT_Int, -1 );
	GetPropFunc( GetPropVector, Vector, DPT_Vector, vec3_invalid );
	GetPropFuncArray( GetPropVectorArray, Vector, DPT_Vector, vec3_invalid );

	HSCRIPT GetPropEntity( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return ToHScript( *(CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset()) );
		}

		return NULL;
	}

	HSCRIPT GetPropEntityArray( HSCRIPT hEnt, const char *pszPropName, int iArrayElement )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return ToHScript( ((CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset()))[iArrayElement] );
		}

		return NULL;
	}

	const char *GetPropString( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return (const char*)((char *)pEnt + pProp->GetOffset());
		}

		return NULL;
	}

	const char *GetPropStringArray( HSCRIPT hEnt, const char *pszPropName, int iArrayElement )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			return ((const char**)((char *)pEnt + pProp->GetOffset()))[iArrayElement];
		}

		return NULL;
	}

	const char *GetPropType( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp)
		{
			switch (pProp->GetType())
			{
			case DPT_Int:		return "integer";
			case DPT_Float:		return "float";
			case DPT_Vector:	return "vector";
			case DPT_VectorXY:	return "vector2d";
			case DPT_String:	return "string";
			case DPT_Array:		return "array";
			case DPT_DataTable:	return "datatable";
			}
		}

		return NULL;
	}

	bool HasProp( HSCRIPT hEnt, const char *pszPropName )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		return GetPropByName( pEnt, pszPropName ) != NULL;
	}

	#define SetPropFunc( name, varType, propType ) \
	void name( HSCRIPT hEnt, const char *pszPropName, varType value ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			*(varType*)((char *)pEnt + pProp->GetOffset()) = value; \
		} \
	} \

	#define SetPropFuncArray( name, varType, propType ) \
	void name( HSCRIPT hEnt, const char *pszPropName, varType value, int iArrayElement ) \
	{ \
		CBaseEntity *pEnt = ToEnt( hEnt ); \
		auto *pProp = GetPropByName( pEnt, pszPropName ); \
		if (pProp && pProp->GetType() == propType) \
		{ \
			((varType*)((char *)pEnt + pProp->GetOffset()))[iArrayElement] = value; \
		} \
	} \

	SetPropFunc( SetPropFloat, float, DPT_Float );
	SetPropFuncArray( SetPropFloatArray, float, DPT_Float );
	SetPropFunc( SetPropInt, int, DPT_Int );
	SetPropFuncArray( SetPropIntArray, int, DPT_Int );
	SetPropFunc( SetPropVector, Vector, DPT_Vector );
	SetPropFuncArray( SetPropVectorArray, Vector, DPT_Vector );
	SetPropFunc( SetPropString, const char*, DPT_String );
	SetPropFuncArray( SetPropStringArray, const char*, DPT_String );

	void SetPropEntity( HSCRIPT hEnt, const char *pszPropName, HSCRIPT value )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			*((CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset())) = ToEnt(value);
		}
	}

	HSCRIPT SetPropEntityArray( HSCRIPT hEnt, const char *pszPropName, HSCRIPT value, int iArrayElement )
	{
		CBaseEntity *pEnt = ToEnt( hEnt );
		auto *pProp = GetPropByName( pEnt, pszPropName );
		if (pProp && pProp->GetType() == DPT_Int)
		{
			((CHandle<CBaseEntity>*)((char *)pEnt + pProp->GetOffset()))[iArrayElement] = ToEnt(value);
		}

		return NULL;
	}

private:
} g_ScriptNetPropManager;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptNetPropManager, "CNetPropManager", SCRIPT_SINGLETON "Allows reading and updating the network properties of an entity." )
	DEFINE_SCRIPTFUNC( GetPropArraySize, "Returns the size of an netprop array, or -1." )
	DEFINE_SCRIPTFUNC( GetPropEntity, "Reads an EHANDLE valued netprop (21 bit integer). Returns the script handle of the entity." )
	DEFINE_SCRIPTFUNC( GetPropEntityArray, "Reads an EHANDLE valued netprop (21 bit integer) from an array. Returns the script handle of the entity." )
	DEFINE_SCRIPTFUNC( GetPropFloat, "Reads a float valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropFloatArray, "Reads a float valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropInt, "Reads an integer valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropIntArray, "Reads an integer valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropString, "Reads a string valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropStringArray, "Reads a string valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropVector, "Reads a 3D vector valued netprop." )
	DEFINE_SCRIPTFUNC( GetPropVectorArray, "Reads a 3D vector valued netprop from an array." )
	DEFINE_SCRIPTFUNC( GetPropType, "Returns the name of the netprop type as a string." )
	DEFINE_SCRIPTFUNC( HasProp, "Checks if a netprop exists." )
	DEFINE_SCRIPTFUNC( SetPropEntity, "Sets an EHANDLE valued netprop (21 bit integer) to reference the specified entity." )
	DEFINE_SCRIPTFUNC( SetPropEntityArray, "Sets an EHANDLE valued netprop (21 bit integer) from an array to reference the specified entity." )
	DEFINE_SCRIPTFUNC( SetPropFloat, "Sets a netprop to the specified float." )
	DEFINE_SCRIPTFUNC( SetPropFloatArray, "Sets a netprop from an array to the specified float." )
	DEFINE_SCRIPTFUNC( SetPropInt, "Sets a netprop to the specified integer." )
	DEFINE_SCRIPTFUNC( SetPropIntArray, "Sets a netprop from an array to the specified integer." )
	DEFINE_SCRIPTFUNC( SetPropString, "Sets a netprop to the specified string." )
	DEFINE_SCRIPTFUNC( SetPropStringArray, "Sets a netprop from an array to the specified string." )
	DEFINE_SCRIPTFUNC( SetPropVector, "Sets a netprop to the specified vector." )
	DEFINE_SCRIPTFUNC( SetPropVectorArray, "Sets a netprop from an array to the specified vector." )
END_SCRIPTDESC();

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptConvarLookup
{
public:

#ifndef CLIENT_DLL
	const char *GetClientConvarValue( const char *pszConVar, int entindex )
	{
		return engine->GetClientConVarValue( entindex, pszConVar );
	}
#endif

	const char *GetStr( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetString();
	}

	float GetFloat( const char *pszConVar )
	{
		ConVarRef cvar( pszConVar );
		return cvar.GetFloat();
	}

	void SetValue( const char *pszConVar, const char *pszValue )
	{
		ConVarRef cvar( pszConVar );
		if (!cvar.IsValid())
			return;

		// FCVAR_NOT_CONNECTED can be used to protect specific convars from nefarious interference
		if (cvar.IsFlagSet(FCVAR_NOT_CONNECTED))
			return;

		cvar.SetValue( pszValue );
	}

private:
} g_ScriptConvarLookup;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarLookup, "CConvars", SCRIPT_SINGLETON "Provides an interface for getting and setting convars." )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( GetClientConvarValue, "Returns the convar value for the entindex as a string. Only works with client convars with the FCVAR_USERINFO flag." )
#endif
	DEFINE_SCRIPTFUNC( GetStr, "Returns the convar as a string. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( GetFloat, "Returns the convar as a float. May return null if no such convar." )
	DEFINE_SCRIPTFUNC( SetValue, "Sets the value of the convar. Supported types are bool, int, float, string." )
END_SCRIPTDESC();

#ifndef CLIENT_DLL
extern ConVar sv_script_think_interval;

void AddThinkToEnt( HSCRIPT entity, const char *pszFuncName )
{
	CBaseEntity *pEntity = ToEnt( entity );
	if (!pEntity)
		return;

	if (pszFuncName == NULL || pszFuncName[0] == '\0')
		pEntity->m_iszScriptThinkFunction = NULL_STRING;
	else
		pEntity->m_iszScriptThinkFunction = AllocPooledString(pszFuncName);

	pEntity->SetContextThink( &CBaseEntity::ScriptThink, gpGlobals->curtime + sv_script_think_interval.GetFloat(), "ScriptThink" );
}

HSCRIPT EntIndexToHScript( int index )
{
	return ToHScript( UTIL_EntityByIndex( index ) );
}

void ParseScriptTableKeyValues( CBaseEntity *pEntity, HSCRIPT hKV )
{
	int nIterator = -1;
	ScriptVariant_t varKey, varValue;
	while ((nIterator = g_pScriptVM->GetKeyValue( hKV, nIterator, &varKey, &varValue )) != -1)
	{
		switch (varValue.m_type)
		{
			case FIELD_CSTRING:		pEntity->KeyValue( varKey.m_pszString, varValue.m_pszString ); break;
			case FIELD_FLOAT:		pEntity->KeyValue( varKey.m_pszString, varValue.m_float ); break;
			case FIELD_VECTOR:		pEntity->KeyValue( varKey.m_pszString, *varValue.m_pVector ); break;
		}

		g_pScriptVM->ReleaseValue( varKey );
		g_pScriptVM->ReleaseValue( varValue );
	}
}

void PrecacheEntityFromTable( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return;
	}

	// This is similar to UTIL_PrecacheOther(), but we can't check if we can only precache it once.
	// Probably for the best anyway, as similar classes can still have different precachable properties.
	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if (!pEntity)
	{
		Assert( !"PrecacheEntityFromTable: only works for CBaseEntities" );
		return;
	}

	ParseScriptTableKeyValues( pEntity, hKV );

	pEntity->Precache();

	UTIL_RemoveImmediate( pEntity );
}

HSCRIPT SpawnEntityFromTable( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromTable: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	ParseScriptTableKeyValues( pEntity, hKV );

	DispatchSpawn( pEntity );

	return ToHScript( pEntity );
}
#endif

//-----------------------------------------------------------------------------
// Mapbase-specific functions start here
//-----------------------------------------------------------------------------

static void ScriptColorPrint( int r, int g, int b, const char *pszMsg )
{
	const Color clr(r, g, b, 255);
	ConColorMsg( clr, "%s", pszMsg );
}

static void ScriptColorPrintL( int r, int g, int b, const char *pszMsg )
{
	const Color clr(r, g, b, 255);
	ConColorMsg( clr, "%s\n", pszMsg );
}

#ifndef CLIENT_DLL
HSCRIPT SpawnEntityFromKeyValues( const char *pszClassname, HSCRIPT hKV )
{
	if ( IsEntityCreationAllowedInScripts() == false )
	{
		Warning( "VScript error: A script attempted to create an entity mid-game. Due to the server's settings, entity creation from scripts is only allowed during map init.\n" );
		return NULL;
	}

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Assert( !"SpawnEntityFromKeyValues: only works for CBaseEntities" );
		return NULL;
	}

	gEntList.NotifyCreateEntity( pEntity );

	CScriptKeyValues *pScriptKV = hKV ? HScriptToClass<CScriptKeyValues>( hKV ) : NULL;
	if (pScriptKV)
	{
		KeyValues *pKV = pScriptKV->m_pKeyValues;
		for (pKV = pKV->GetFirstSubKey(); pKV != NULL; pKV = pKV->GetNextKey())
		{
			pEntity->KeyValue( pKV->GetName(), pKV->GetString() );
		}
	}

	DispatchSpawn( pEntity );

	return ToHScript( pEntity );
}

void ScriptDispatchSpawn( HSCRIPT hEntity )
{
	CBaseEntity *pEntity = ToEnt( hEntity );
	if (pEntity)
	{
		DispatchSpawn( pEntity );
	}
}
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static HSCRIPT CreateDamageInfo( HSCRIPT hInflictor, HSCRIPT hAttacker, const Vector &vecForce, const Vector &vecDamagePos, float flDamage, int64 iDamageType )
{
	// The script is responsible for deleting this via DestroyDamageInfo().
	CTakeDamageInfo *damageInfo = new CTakeDamageInfo(ToEnt(hInflictor), ToEnt(hAttacker), flDamage, iDamageType);
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( damageInfo );

	damageInfo->SetDamagePosition( vecDamagePos );
	damageInfo->SetDamageForce( vecForce );

	return hScript;
}

static void DestroyDamageInfo( HSCRIPT hDamageInfo )
{
	if (hDamageInfo)
	{
		CTakeDamageInfo *pInfo = (CTakeDamageInfo*)g_pScriptVM->GetInstanceValue( hDamageInfo, GetScriptDescForClass( CTakeDamageInfo ) );
		if (pInfo)
		{
			g_pScriptVM->RemoveInstance( hDamageInfo );
			delete pInfo;
		}
	}
}

void ScriptCalculateExplosiveDamageForce( HSCRIPT info, const Vector &vecDir, const Vector &vecForceOrigin, float flScale ) { CalculateExplosiveDamageForce( HScriptToClass<CTakeDamageInfo>(info), vecDir, vecForceOrigin, flScale ); }
void ScriptCalculateBulletDamageForce( HSCRIPT info, int iBulletType, const Vector &vecBulletDir, const Vector &vecForceOrigin, float flScale ) { CalculateBulletDamageForce( HScriptToClass<CTakeDamageInfo>(info), iBulletType, vecBulletDir, vecForceOrigin, flScale ); }
void ScriptCalculateMeleeDamageForce( HSCRIPT info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale ) { CalculateMeleeDamageForce( HScriptToClass<CTakeDamageInfo>( info ), vecMeleeDir, vecForceOrigin, flScale ); }
void ScriptGuessDamageForce( HSCRIPT info, const Vector &vecForceDir, const Vector &vecForceOrigin, float flScale ) { GuessDamageForce( HScriptToClass<CTakeDamageInfo>( info ), vecForceDir, vecForceOrigin, flScale ); }

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CTraceInfoAccessor
{
public:

	// CGrameTrace stuff
	bool DidHitWorld() const { return m_tr.DidHitWorld(); }
	bool DidHitNonWorldEntity() const { return m_tr.DidHitNonWorldEntity(); }
	int GetEntityIndex() const { return m_tr.GetEntityIndex(); }
	bool DidHit() const { return m_tr.DidHit(); }

	float FractionLeftSolid() const { return m_tr.fractionleftsolid; }
	int HitGroup() const { return m_tr.hitgroup; }
	int PhysicsBone() const { return m_tr.physicsbone; }

	HSCRIPT Entity() const { return ToHScript(m_tr.m_pEnt); }

	int HitBox() const { return m_tr.hitbox; }

	// CBaseTrace stuff
	bool IsDispSurface() { return m_tr.IsDispSurface(); }
	bool IsDispSurfaceWalkable() { return m_tr.IsDispSurfaceWalkable(); }
	bool IsDispSurfaceBuildable() { return m_tr.IsDispSurfaceBuildable(); }
	bool IsDispSurfaceProp1() { return m_tr.IsDispSurfaceProp1(); }
	bool IsDispSurfaceProp2() { return m_tr.IsDispSurfaceProp2(); }

	Vector StartPos() const { return m_tr.startpos; }
	Vector EndPos() const { return m_tr.endpos; }

	float Fraction() const { return m_tr.fraction; }

	int Contents() const { return m_tr.contents; }
	int DispFlags() const { return m_tr.dispFlags; }

	bool AllSolid() const { return m_tr.allsolid; }
	bool StartSolid() const { return m_tr.startsolid; }

	trace_t &GetTrace() { return m_tr; }
	void Destroy() { delete this; }

private:
	trace_t m_tr;

};

BEGIN_SCRIPTDESC_ROOT_NAMED( CTraceInfoAccessor, "CGameTrace", "Handle for accessing trace_t info." )
	DEFINE_SCRIPTFUNC( DidHitWorld, "Returns whether the trace hit the world entity or not." )
	DEFINE_SCRIPTFUNC( DidHitNonWorldEntity, "Returns whether the trace hit something other than the world entity." )
	DEFINE_SCRIPTFUNC( GetEntityIndex, "Returns the index of whatever entity this trace hit." )
	DEFINE_SCRIPTFUNC( DidHit, "Returns whether the trace hit anything." )

	DEFINE_SCRIPTFUNC( FractionLeftSolid, "If this trace started within a solid, this is the point in the trace's fraction at which it left that solid." )
	DEFINE_SCRIPTFUNC( HitGroup, "Returns the specific hit group this trace hit if it hit an entity." )
	DEFINE_SCRIPTFUNC( PhysicsBone, "Returns the physics bone this trace hit if it hit an entity." )
	DEFINE_SCRIPTFUNC( Entity, "Returns the entity this trace has hit." )
	DEFINE_SCRIPTFUNC( HitBox, "Returns the hitbox of the entity this trace has hit. If it hit the world entity, this returns the static prop index." )

	DEFINE_SCRIPTFUNC( IsDispSurface, "Returns whether this trace hit a displacement." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceWalkable, "Returns whether DISPSURF_FLAG_WALKABLE is ticked on the displacement this trace hit." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceBuildable, "Returns whether DISPSURF_FLAG_BUILDABLE is ticked on the displacement this trace hit." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceProp1, "Returns whether DISPSURF_FLAG_SURFPROP1 is ticked on the displacement this trace hit." )
	DEFINE_SCRIPTFUNC( IsDispSurfaceProp2, "Returns whether DISPSURF_FLAG_SURFPROP2 is ticked on the displacement this trace hit." )

	DEFINE_SCRIPTFUNC( StartPos, "Gets the trace's start position." )
	DEFINE_SCRIPTFUNC( EndPos, "Gets the trace's end position." )

	DEFINE_SCRIPTFUNC( Fraction, "Gets the fraction of the trace completed. For example, if the trace stopped exactly halfway to the end position, this would be 0.5." )
	DEFINE_SCRIPTFUNC( Contents, "Gets the contents of the surface the trace has hit." )
	DEFINE_SCRIPTFUNC( DispFlags, "Gets the displacement flags of the surface the trace has hit." )

	DEFINE_SCRIPTFUNC( AllSolid, "Returns whether the trace is completely within a solid." )
	DEFINE_SCRIPTFUNC( StartSolid, "Returns whether the trace started within a solid." )

	DEFINE_SCRIPTFUNC( Destroy, "Deletes this instance. Important for preventing memory leaks." )
END_SCRIPTDESC();

static HSCRIPT ScriptTraceLineComplex( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore, int iMask, int iCollisionGroup )
{
	// The script is responsible for deleting this via Destroy().
	CTraceInfoAccessor *traceInfo = new CTraceInfoAccessor();
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( traceInfo );

	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, iMask, pLooker, iCollisionGroup, &traceInfo->GetTrace());
	return hScript;
}

static HSCRIPT ScriptTraceHullComplex( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, const Vector &hullMax,
	HSCRIPT entIgnore, int iMask, int iCollisionGroup )
{
	// The script is responsible for deleting this via Destroy().
	CTraceInfoAccessor *traceInfo = new CTraceInfoAccessor();
	HSCRIPT hScript = g_pScriptVM->RegisterInstance( traceInfo );

	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceHull( vecStart, vecEnd, hullMin, hullMax, iMask, pLooker, iCollisionGroup, &traceInfo->GetTrace());
	return hScript;
}

bool ScriptMatcherMatch( const char *pszQuery, const char *szValue ) { return Matcher_Match( pszQuery, szValue ); }

//=============================================================================
//=============================================================================

class CScriptGameEventListener : public IGameEventListener, public CAutoGameSystem
{
public:
	CScriptGameEventListener() : m_bActive(false) {}
	~CScriptGameEventListener()
	{
		StopListeningForEvent();
	}

	intptr_t ListenToGameEvent(const char* szEvent, HSCRIPT hFunc, const char* szContext);
	void StopListeningForEvent();

public:
	static bool StopListeningToGameEvent(intptr_t listener);
	static void StopListeningToAllGameEvents(const char* szContext);

public:
	virtual void FireGameEvent(KeyValues* event);
	void LevelShutdownPreEntity();

private:
	bool m_bActive;
	const char* m_pszContext;
	HSCRIPT m_hCallback;

	static const char* FindContext(const char* szContext, CScriptGameEventListener* pIgnore = NULL);
	//inline const char *GetContext( CScriptGameEventListener *p );
	//inline const char *GetContext();

public:
	static void DumpEventListeners();
	static void WriteEventData(KeyValues* event, HSCRIPT hTable);

private:
	static CUtlVectorAutoPurge< CScriptGameEventListener* > s_GameEventListeners;

};

CUtlVectorAutoPurge< CScriptGameEventListener* > CScriptGameEventListener::s_GameEventListeners;

#if 0
#ifdef CLIENT_DLL
CON_COMMAND_F(cl_dump_script_game_event_listeners, "Dump all game event listeners created from script.", FCVAR_CHEAT)
{
	CScriptGameEventListener::DumpEventListeners();
}
#else // GAME_DLL
CON_COMMAND_F(dump_script_game_event_listeners, "Dump all game event listeners created from script.", FCVAR_CHEAT)
{
	CScriptGameEventListener::DumpEventListeners();
}
#endif // CLIENT_DLL
#endif

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CScriptGameEventListener::DumpEventListeners()
{
	Msg("--- Script game event listener dump start\n");
	FOR_EACH_VEC(s_GameEventListeners, i)
	{
		Msg(" %d   (0x%p) %d : %s\n", i, s_GameEventListeners[i],
			s_GameEventListeners[i],
			s_GameEventListeners[i]->m_pszContext ? s_GameEventListeners[i]->m_pszContext : "");
	}
	Msg("--- Script game event listener dump end\n");
}

void CScriptGameEventListener::FireGameEvent(KeyValues* event)
{
	ScriptVariant_t hTable;
	g_pScriptVM->CreateTable(hTable);

	WriteEventData(event, hTable);
	g_pScriptVM->SetValue(hTable, "game_event_listener", reinterpret_cast<intptr_t>(this)); // POINTER_TO_INT
	// g_pScriptVM->SetValue( hTable, "game_event_name", event->GetName() );
	g_pScriptVM->ExecuteFunction(m_hCallback, &hTable, 1, NULL, NULL, true);
	g_pScriptVM->ReleaseScript(hTable);
}

void CScriptGameEventListener::LevelShutdownPreEntity()
{
	s_GameEventListeners.FindAndFastRemove(this);
	delete this;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CScriptGameEventListener::WriteEventData(KeyValues* event, HSCRIPT hTable)
{
	for (KeyValues* sub = event->GetFirstSubKey(); sub; sub = sub->GetNextKey())
	{
		const char* szKey = sub->GetName();
		switch (sub->GetDataType())
		{
		case KeyValues::TYPE_STRING: g_pScriptVM->SetValue(hTable, szKey, event->GetString(szKey)); break;
		case KeyValues::TYPE_INT:    g_pScriptVM->SetValue(hTable, szKey, event->GetInt(szKey)); break;
		case KeyValues::TYPE_FLOAT:  g_pScriptVM->SetValue(hTable, szKey, event->GetFloat(szKey)); break;
		case KeyValues::TYPE_UINT64: g_pScriptVM->SetValue(hTable, szKey, event->GetUint64(szKey)); break;
			// default: DevWarning( 2, "CScriptGameEventListener::WriteEventData: unknown data type '%d' on key '%s' in event '%s'\n", sub->GetDataType(), szKey, szEvent );
		}
	}
}
//-----------------------------------------------------------------------------
// Find if context is in use by others; used to alloc/dealloc only when required.
// Returns allocated pointer to string
// Expects non-NULL context input
//-----------------------------------------------------------------------------
const char* CScriptGameEventListener::FindContext(const char* szContext, CScriptGameEventListener* pIgnore)
{
	for (int i = s_GameEventListeners.Count(); i--; )
	{
		CScriptGameEventListener* pCur = s_GameEventListeners[i];
		if (pCur != pIgnore)
		{
			if (pCur->m_pszContext && !V_stricmp(szContext, pCur->m_pszContext))
			{
				return pCur->m_pszContext;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
intptr_t CScriptGameEventListener::ListenToGameEvent(const char* szEvent, HSCRIPT hFunc, const char* szContext)
{
	m_bActive = true;

	char* psz;

	if (szContext && *szContext)
	{
		psz = const_cast<char*>(FindContext(szContext));
		if (!psz)
		{
			int len = V_strlen(szContext) + 1;
			if (len > 1)
			{
				int size = min(len, 256); // arbitrary clamp
				psz = new char[size];
				V_strncpy(psz, szContext, size);
			}
		}
	}
	else
	{
		psz = NULL;
	}

	m_pszContext = psz;
	m_hCallback = hFunc;

	if (gameeventmanagerOLD)
#ifdef CLIENT_DLL
		gameeventmanagerOLD->AddListener(this, szEvent, false);
#else
		gameeventmanagerOLD->AddListener(this, szEvent, true);
#endif
	s_GameEventListeners.AddToTail(this);

	return reinterpret_cast<intptr_t>(this); // POINTER_TO_INT
}

//-----------------------------------------------------------------------------
// Free stuff. Called from the destructor, does not remove itself from the listener list.
//-----------------------------------------------------------------------------
void CScriptGameEventListener::StopListeningForEvent()
{
	if (!m_bActive)
		return;

	if (g_pScriptVM)
	{
		g_pScriptVM->ReleaseScript(m_hCallback);
	}
	else if (m_hCallback)
	{
		AssertMsg(0, "LEAK (0x%p)\n", (void*)m_hCallback);
	}

	if (m_pszContext)
	{
		if (!FindContext(m_pszContext, this))
		{
			delete[] m_pszContext;
		}

		m_pszContext = NULL;
	}

	m_hCallback = NULL;

	if (gameeventmanagerOLD)
		gameeventmanagerOLD->RemoveListener(this);

	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Stop the specified event listener.
//-----------------------------------------------------------------------------
bool CScriptGameEventListener::StopListeningToGameEvent(intptr_t listener)
{
	CScriptGameEventListener* p = reinterpret_cast<CScriptGameEventListener*>(listener); // INT_TO_POINTER	

	bool bRemoved = s_GameEventListeners.FindAndFastRemove(p);
	if (bRemoved)
	{
		delete p;
	}

	return bRemoved;
}

//-----------------------------------------------------------------------------
// Stops listening to all events within a context.
//-----------------------------------------------------------------------------
void CScriptGameEventListener::StopListeningToAllGameEvents(const char* szContext)
{
	if (szContext)
	{
		if (*szContext)
		{
			// Iterate from the end so they can be safely removed as they are deleted
			for (int i = s_GameEventListeners.Count(); i--; )
			{
				CScriptGameEventListener* pCur = s_GameEventListeners[i];
				if (pCur->m_pszContext && !V_stricmp(szContext, pCur->m_pszContext))
				{
					s_GameEventListeners.Remove(i); // keep list order
					delete pCur;
				}
			}
		}
		else // empty (NULL) context
		{
			for (int i = s_GameEventListeners.Count(); i--; )
			{
				CScriptGameEventListener* pCur = s_GameEventListeners[i];
				if (!pCur->m_pszContext)
				{
					s_GameEventListeners.Remove(i);
					delete pCur;
				}
			}
		}
	}
#if 0
	if (!szContext)
	{
		for (int i = s_GameEventListeners.Count(); i--; )
			delete s_GameEventListeners[i];
		s_GameEventListeners.Purge();
	}
#endif
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static int ListenToGameEvent(const char* szEvent, HSCRIPT hFunc, const char* szContext)
{
	CScriptGameEventListener* p = new CScriptGameEventListener();
	return p->ListenToGameEvent(szEvent, hFunc, szContext);
}

static void FireGameEvent(const char* szEvent, HSCRIPT hTable)
{
	IGameEvent* event = gameeventmanager->CreateEvent(szEvent);
	if (event)
	{
		ScriptVariant_t key, val;
		int nIterator = -1;
		while ((nIterator = g_pScriptVM->GetKeyValue(hTable, nIterator, &key, &val)) != -1)
		{
			switch (val.m_type)
			{
			case FIELD_FLOAT:   event->SetFloat(key.m_pszString, val.m_float); break;
			case FIELD_INTEGER: event->SetInt(key.m_pszString, val.m_int); break;
			case FIELD_BOOLEAN: event->SetBool(key.m_pszString, val.m_bool); break;
			case FIELD_CSTRING: event->SetString(key.m_pszString, val.m_pszString); break;
			}

			g_pScriptVM->ReleaseValue(key);
			g_pScriptVM->ReleaseValue(val);
		}

#ifdef CLIENT_DLL
		gameeventmanager->FireEventClientSide(event);
#else
		gameeventmanager->FireEvent(event);
#endif
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Copy of FireGameEvent, server only with no broadcast to clients.
//-----------------------------------------------------------------------------
static void FireGameEventLocal(const char* szEvent, HSCRIPT hTable)
{
	IGameEvent* event = gameeventmanager->CreateEvent(szEvent);
	if (event)
	{
		ScriptVariant_t key, val;
		int nIterator = -1;
		while ((nIterator = g_pScriptVM->GetKeyValue(hTable, nIterator, &key, &val)) != -1)
		{
			switch (val.m_type)
			{
			case FIELD_FLOAT:   event->SetFloat(key.m_pszString, val.m_float); break;
			case FIELD_INTEGER: event->SetInt(key.m_pszString, val.m_int); break;
			case FIELD_BOOLEAN: event->SetBool(key.m_pszString, val.m_bool); break;
			case FIELD_CSTRING: event->SetString(key.m_pszString, val.m_pszString); break;
			}

			g_pScriptVM->ReleaseValue(key);
			g_pScriptVM->ReleaseValue(val);
		}

		gameeventmanager->FireEvent(event, true);
	}
}
#endif // !CLIENT_DLL

class CGlobalSys
{
public:
	const char* ScriptGetCommandLine()
	{
		return CommandLine()->GetCmdLine();
	}

	bool CommandLineCheck(const char* name)
	{
		return !!CommandLine()->FindParm(name);
	}

	const char* CommandLineCheckStr(const char* name)
	{
		return CommandLine()->ParmValue(name);
	}

	float CommandLineCheckFloat(const char* name)
	{
		return CommandLine()->ParmValue(name, 0);
	}

	int CommandLineCheckInt(const char* name)
	{
		return CommandLine()->ParmValue(name, 0);
	}
} g_ScriptGlobalSys;

BEGIN_SCRIPTDESC_ROOT_NAMED(CGlobalSys, "CGlobalSys", SCRIPT_SINGLETON "GlobalSys")
DEFINE_SCRIPTFUNC_NAMED(ScriptGetCommandLine, "GetCommandLine", "returns the command line")
DEFINE_SCRIPTFUNC(CommandLineCheck, "returns true if the command line param was used, otherwise false.")
DEFINE_SCRIPTFUNC(CommandLineCheckStr, "returns the command line param as a string.")
DEFINE_SCRIPTFUNC(CommandLineCheckFloat, "returns the command line param as a float.")
DEFINE_SCRIPTFUNC(CommandLineCheckInt, "returns the command line param as an int.")
END_SCRIPTDESC();

class CScriptSaveRestoreUtil : public CAutoGameSystem
{
public:
	static void SaveTable(const char* szId, HSCRIPT hTable);
	static void RestoreTable(const char* szId, HSCRIPT hTable);
	static void ClearSavedTable(const char* szId);

	// IGameSystem interface
public:
	void OnSave()
	{
		if (g_pScriptVM)
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction("OnSave");
			if (hFunc)
			{
				g_pScriptVM->Call(hFunc);
			}
		}
	}

	void OnRestore()
	{
		if (g_pScriptVM)
		{
			HSCRIPT hFunc = g_pScriptVM->LookupFunction("OnRestore");
			if (hFunc)
			{
				g_pScriptVM->Call(hFunc);
			}
		}
	}

	void Shutdown()
	{
		FOR_EACH_VEC(m_aKeyValues, i)
			m_aKeyValues[i]->deleteThis();
		m_aKeyValues.Purge();
		m_aContext.PurgeAndDeleteElements();
	}

private:
	static int GetIndexForContext(const char* szId);

	// indices must match, always remove keeping order
	static CUtlStringList m_aContext;
	static CUtlVector<KeyValues*> m_aKeyValues;

} g_ScriptSaveRestoreUtil;

CUtlStringList CScriptSaveRestoreUtil::m_aContext;
CUtlVector<KeyValues*> CScriptSaveRestoreUtil::m_aKeyValues;

int CScriptSaveRestoreUtil::GetIndexForContext(const char* szId)
{
	int idx = -1;
	FOR_EACH_VEC(m_aContext, i)
	{
		if (!V_stricmp(szId, m_aContext[i]))
		{
			idx = i;
			break;
		}
	}
	return idx;
}

//-----------------------------------------------------------------------------
// Store a table with primitive values that will persist across level transitions and save loads.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::SaveTable(const char* szId, HSCRIPT hTable)
{
	int idx = GetIndexForContext(szId);

	KeyValues* pKV;

	if (idx == -1)
	{
		pKV = new KeyValues("ScriptSavedTable");
		m_aKeyValues.AddToTail(pKV);

		if (V_strlen(szId) > 255) // arbitrary clamp
		{
			char c[256];
			V_strncpy(c, szId, sizeof(c));
			m_aContext.CopyAndAddToTail(c);
		}
		else
		{
			m_aContext.CopyAndAddToTail(szId);
		}
	}
	else
	{
		pKV = m_aKeyValues[idx];
		pKV->Clear();
	}

	ScriptVariant_t key, val;
	int nIterator = -1;
	while ((nIterator = g_pScriptVM->GetKeyValue(hTable, nIterator, &key, &val)) != -1)
	{
		switch (val.m_type)
		{
		case FIELD_FLOAT:   pKV->SetFloat(key.m_pszString, val.m_float); break;
		case FIELD_INTEGER: pKV->SetInt(key.m_pszString, val.m_int); break;
		case FIELD_BOOLEAN: pKV->SetBool(key.m_pszString, val.m_bool); break;
		case FIELD_CSTRING: pKV->SetString(key.m_pszString, val.m_pszString); break;
		}

		g_pScriptVM->ReleaseValue(key);
		g_pScriptVM->ReleaseValue(val);
	}
}

//-----------------------------------------------------------------------------
// Retrieves a table from storage. Write into input table.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::RestoreTable(const char* szId, HSCRIPT hTable)
{
	int idx = GetIndexForContext(szId);

	KeyValues* pKV;

	if (idx == -1)
	{
		// DevWarning( 2, "RestoreTable could not find saved table with context '%s'\n", szId );
		return;
	}
	else
	{
		pKV = m_aKeyValues[idx];
	}

	for (KeyValues *key = pKV->GetFirstSubKey(); key != nullptr; key = key->GetNextKey())
	{
		switch (key->GetDataType())
		{
		case KeyValues::TYPE_STRING: g_pScriptVM->SetValue(hTable, key->GetName(), key->GetString()); break;
		case KeyValues::TYPE_INT:    g_pScriptVM->SetValue(hTable, key->GetName(), key->GetInt()); break;
		case KeyValues::TYPE_FLOAT:  g_pScriptVM->SetValue(hTable, key->GetName(), key->GetFloat()); break;
		}
	}
}

//-----------------------------------------------------------------------------
// Remove a saved table.
//-----------------------------------------------------------------------------
void CScriptSaveRestoreUtil::ClearSavedTable(const char* szId)
{
	int idx = GetIndexForContext(szId);

	if (idx == -1)
	{
		// DevWarning( 2, "ClearSavedTable could not find saved table with context '%s'\n", szId );
		return;
	}

	m_aKeyValues[idx]->deleteThis();
	m_aKeyValues.Remove(idx);

	delete[] m_aContext[idx];
	m_aContext.Remove(idx);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
#define SCRIPT_MAX_FILE_READ_SIZE  (16 * 1024)			// 16KB
#define SCRIPT_MAX_FILE_WRITE_SIZE SCRIPT_MAX_FILE_READ_SIZE //(64 * 1024 * 1024)	// 64MB
#define SCRIPT_RW_PATH_ID "MOD_WRITE"
#define SCRIPT_RW_FULL_PATH_FMT "ems/%s"

class CScriptReadWriteFile : public CAutoGameSystem
{
public:
	static bool ScriptFileWrite(const char* szFile, const char* szInput);
	static const char* ScriptFileRead(const char* szFile);
	//static const char *CRC32_Checksum( const char *szFilename );

	void LevelShutdownPostEntity()
	{
		if (m_pszReturnReadFile)
		{
			delete[] m_pszReturnReadFile;
			m_pszReturnReadFile = NULL;
		}

		//if ( m_pszReturnCRC32 )
		//{
		//	delete[] m_pszReturnCRC32;
		//	m_pszReturnCRC32 = NULL;
		//}
	}

private:
	static const char* m_pszReturnReadFile;
	//static const char *m_pszReturnCRC32;

} g_ScriptReadWrite;

const char* CScriptReadWriteFile::m_pszReturnReadFile = NULL;
//const char *CScriptReadWriteFile::m_pszReturnCRC32 = NULL;

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CScriptReadWriteFile::ScriptFileWrite(const char* szFile, const char* szInput)
{
	size_t len = strlen(szInput);
	if (len > SCRIPT_MAX_FILE_WRITE_SIZE)
	{
		DevWarning(2, "Input is too large for a ScriptFileWrite ( %s / %d KB )\n", V_pretifymem(len, 2, true), (SCRIPT_MAX_FILE_WRITE_SIZE >> 10));
		return false;
	}

	char pszFullName[MAX_PATH];
	V_snprintf(pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile);

	if (!V_RemoveDotSlashes(pszFullName, CORRECT_PATH_SEPARATOR, true))
	{
		DevWarning(2, "Invalid file location : %s\n", szFile);
		return false;
	}

	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	buf.PutString(szInput);

	int nSize = V_strlen(pszFullName) + 1;
	char* pszDir = (char*)stackalloc(nSize);
	V_memcpy(pszDir, pszFullName, nSize);
	V_StripFilename(pszDir);

	g_pFullFileSystem->CreateDirHierarchy(pszDir, SCRIPT_RW_PATH_ID);
	bool res = g_pFullFileSystem->WriteFile(pszFullName, SCRIPT_RW_PATH_ID, buf);
	buf.Purge();
	return res;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
const char* CScriptReadWriteFile::ScriptFileRead(const char* szFile)
{
	char pszFullName[MAX_PATH];
	V_snprintf(pszFullName, sizeof(pszFullName), SCRIPT_RW_FULL_PATH_FMT, szFile);

	if (!V_RemoveDotSlashes(pszFullName, CORRECT_PATH_SEPARATOR, true))
	{
		DevWarning(2, "Invalid file location : %s\n", szFile);
		return NULL;
	}

	unsigned int size = g_pFullFileSystem->Size(pszFullName, SCRIPT_RW_PATH_ID);
	if (size >= SCRIPT_MAX_FILE_READ_SIZE)
	{
		DevWarning(2, "File '%s' (from '%s') is too large for a ScriptFileRead ( %s / %u KB )\n", pszFullName, szFile, V_pretifymem(size, 2, true), (SCRIPT_MAX_FILE_READ_SIZE >> 10));
		return NULL;
	}

	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	if (!g_pFullFileSystem->ReadFile(pszFullName, SCRIPT_RW_PATH_ID, buf, SCRIPT_MAX_FILE_READ_SIZE))
	{
		return NULL;
	}

	// first time calling, allocate
	if (!m_pszReturnReadFile)
		m_pszReturnReadFile = new char[SCRIPT_MAX_FILE_READ_SIZE];

	V_strncpy(const_cast<char*>(m_pszReturnReadFile), (const char*)buf.Base(), buf.Size());
	buf.Purge();
	return m_pszReturnReadFile;
}

//-----------------------------------------------------------------------------
// Get the checksum of any file. Can be used to check the existence or validity of a file.
// Returns unsigned int as hex string.
//-----------------------------------------------------------------------------
/*
const char *CScriptReadWriteFile::CRC32_Checksum( const char *szFilename )
{
	CUtlBuffer buf( 0, 0, CUtlBuffer::READ_ONLY );
	if ( !g_pFullFileSystem->ReadFile( szFilename, NULL, buf ) )
		return NULL;
	// first time calling, allocate
	if ( !m_pszReturnCRC32 )
		m_pszReturnCRC32 = new char[9]; // 'FFFFFFFF\0'
	V_snprintf( const_cast<char*>(m_pszReturnCRC32), 9, "%X", CRC32_ProcessSingleBuffer( buf.Base(), buf.Size()-1 ) );
	buf.Purge();
	return m_pszReturnCRC32;
}
*/
#undef SCRIPT_MAX_FILE_READ_SIZE
#undef SCRIPT_MAX_FILE_WRITE_SIZE
#undef SCRIPT_RW_PATH_ID
#undef SCRIPT_RW_FULL_PATH_FMT

//=============================================================================
//=============================================================================

void RegisterSharedScriptFunctions()
{
	// 
	// Due to this being a custom integration of VScript based on the Alien Swarm SDK, we don't have access to
	// some of the code normally available in games like L4D2 or Valve's original VScript DLL.
	// Instead, that code is recreated here, shared between server and client.
	// 
	ScriptRegisterFunction( g_pScriptVM, RandomFloat, "Generate a random floating point number within a range, inclusive." );
	ScriptRegisterFunction( g_pScriptVM, RandomInt, "Generate a random integer within a range, inclusive." );

	ScriptRegisterFunction( g_pScriptVM, AngleDiff, "Returns the number of degrees difference between two yaw angles." );

#ifndef CLIENT_DLL
	ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::BoxDirection, "DebugDrawBoxDirection", "Draw a debug forward box" );
	ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Text, "DebugDrawText", "Draw a debug overlay text" );

	ScriptRegisterFunction( g_pScriptVM, AddThinkToEnt, "This will put a think function onto an entity, or pass null to remove it. This is NOT chained, so be careful." );
	ScriptRegisterFunction( g_pScriptVM, EntIndexToHScript, "Returns the script handle for the given entity index." );
	ScriptRegisterFunction( g_pScriptVM, PrecacheEntityFromTable, "Precache an entity from KeyValues in a table." );
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromTable, "Native function for entity spawning." );
#endif

	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptSaveRestoreUtil::SaveTable, "SaveTable", "Store a table with primitive values that will persist across level transitions and save loads.");
	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptSaveRestoreUtil::RestoreTable, "RestoreTable", "Retrieves a table from storage. Write into input table.");
	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptSaveRestoreUtil::ClearSavedTable, "ClearSavedTable", "Removes the table with the given context.");
	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptReadWriteFile::ScriptFileWrite, "StringToFile", "Stores the string into the file");
	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptReadWriteFile::ScriptFileRead, "FileToString", "Returns the string from the file, null if no file or file is too big.");

	ScriptRegisterFunction(g_pScriptVM, ListenToGameEvent, "Register as a listener for a game event from script.");
	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptGameEventListener::StopListeningToGameEvent, "StopListeningToGameEvent", "Stop the specified event listener.");
	ScriptRegisterFunctionNamed(g_pScriptVM, CScriptGameEventListener::StopListeningToAllGameEvents, "StopListeningToAllGameEvents", "Stop listening to all game events within a specific context.");
	ScriptRegisterFunction(g_pScriptVM, FireGameEvent, "Fire a game event.");
#ifndef CLIENT_DLL
	ScriptRegisterFunction(g_pScriptVM, FireGameEventLocal, "Fire a game event without broadcasting to the client.");
#endif

	g_pScriptVM->RegisterInstance( &g_ScriptConvarLookup, "Convars" );
	g_pScriptVM->RegisterInstance( &g_ScriptNetPropManager, "NetProps" );
	g_pScriptVM->RegisterInstance(&g_ScriptGlobalSys, "GlobalSys");

	// Functions unique to Mapbase
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptColorPrint, "printc", "Version of print() which takes a color before the message." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptColorPrintL, "printcl", "Version of printl() which takes a color before the message." );

#ifndef CLIENT_DLL
	ScriptRegisterFunction( g_pScriptVM, SpawnEntityFromKeyValues, "Spawns an entity with the keyvalues in a CScriptKeyValues handle." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptDispatchSpawn, "DispatchSpawn", "Spawns an unspawned entity." );
#endif

	ScriptRegisterFunction( g_pScriptVM, CreateDamageInfo, "Creates damage info." );
	ScriptRegisterFunction( g_pScriptVM, DestroyDamageInfo, "Destroys damage info." );
	ScriptRegisterFunction( g_pScriptVM, ImpulseScale, "Returns an impulse scale required to push an object." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalculateExplosiveDamageForce, "CalculateExplosiveDamageForce", "Fill out a damage info handle with a damage force for an explosive." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalculateBulletDamageForce, "CalculateBulletDamageForce", "Fill out a damage info handle with a damage force for a bullet impact." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCalculateMeleeDamageForce, "CalculateMeleeDamageForce", "Fill out a damage info handle with a damage force for a melee impact." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptGuessDamageForce, "GuessDamageForce", "Try and guess the physics force to use." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLineComplex, "TraceLineComplex", "Complex version of TraceLine which takes 2 points, an ent to ignore, a trace mask, and a collision group. Returns a handle which can access all trace info." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceHullComplex, "TraceHullComplex", "Takes 2 points, min/max hull bounds, an ent to ignore, a trace mask, and a collision group to trace to a point using a hull. Returns a handle which can access all trace info." );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMatcherMatch, "Matcher_Match", "Compares a string to a query using Mapbase's matcher system, supporting wildcards, RS matchers, etc." );
	ScriptRegisterFunction( g_pScriptVM, Matcher_NamesMatch, "Compares a string to a query using Mapbase's matcher system using wildcards only." );
	ScriptRegisterFunction( g_pScriptVM, AppearsToBeANumber, "Checks if the given string appears to be a number." );
}

CUtlVector<IGameScriptPersistable*> g_ScriptPersistableList;