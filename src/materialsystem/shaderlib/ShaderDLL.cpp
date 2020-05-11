//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "shaderlib/ShaderDLL.h"
#include "materialsystem/IShader.h"
#include "tier1/utlvector.h"
#include "tier0/dbg.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/materialsystem_config.h"
#include "IShaderSystem.h"
#include "materialsystem/ishaderapi.h"
#include "shaderlib_cvar.h"
#include "mathlib/mathlib.h"
#include "tier1/tier1.h"
#include "spp_utils/spp_utils.h"
#include "filesystem.h"
#include "filesystem_init.h"
#include "../stdshaders/IShaderExtension.h"

#if defined( _WIN32 ) && !defined( _X360 )
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// The standard implementation of CShaderDLL
//-----------------------------------------------------------------------------
class CShaderDLL : public IShaderDLLInternal, public IShaderDLL
{
public:
	CShaderDLL();

	// methods of IShaderDLL
	virtual bool Connect( CreateInterfaceFn factory );
	virtual void Disconnect();
	virtual int ShaderCount() const;
	virtual IShader *GetShader( int nShader );

	// methods of IShaderDLLInternal
	virtual bool Connect( CreateInterfaceFn factory, bool bIsMaterialSystem );
	virtual void Disconnect( bool bIsMaterialSystem );
	virtual void InsertShader( IShader *pShader );

private:
	CUtlVector< IShader * >	m_ShaderList;
};


//-----------------------------------------------------------------------------
// Global interfaces/structures
//-----------------------------------------------------------------------------
IMaterialSystemHardwareConfig* g_pHardwareConfig;
const MaterialSystem_Config_t *g_pConfig;
IFileSystem* filesystem = NULL;
IShaderExtensionInternal *g_pShaderExtension = nullptr;


//-----------------------------------------------------------------------------
// Interfaces/structures local to shaderlib
//-----------------------------------------------------------------------------
IShaderSystem* g_pSLShaderSystem;
CSysModule* filesystem_module = NULL;
CSysModule* spp_utils_module = NULL;
IGameSharedUtils* spp_utils = NULL;


// Pattern necessary because shaders register themselves in global constructors
static CShaderDLL *s_pShaderDLL;

static bool Sys_GetExecutableName(char* out, int len)
{
#if defined( _WIN32 )
	if (!::GetModuleFileName((HINSTANCE)GetModuleHandle(NULL), out, len))
	{
		return false;
	}
#else
	if (CommandLine()->GetParm(0))
	{
		Q_MakeAbsolutePath(out, len, CommandLine()->GetParm(0));
	}
	else
	{
		return false;
	}
#endif

	return true;
}


//-----------------------------------------------------------------------------
// Global accessor
//-----------------------------------------------------------------------------
IShaderDLL *GetShaderDLL()
{
	// Pattern necessary because shaders register themselves in global constructors
	if ( !s_pShaderDLL )
	{
		s_pShaderDLL = new CShaderDLL;
	}

	return s_pShaderDLL;
}

IShaderDLLInternal *GetShaderDLLInternal()
{
	// Pattern necessary because shaders register themselves in global constructors
	if ( !s_pShaderDLL )
	{
		s_pShaderDLL = new CShaderDLL;
	}

	return static_cast<IShaderDLLInternal*>( s_pShaderDLL );
}

//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
EXPOSE_INTERFACE_FN( (InstantiateInterfaceFn)GetShaderDLLInternal, IShaderDLLInternal, SHADER_DLL_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
CShaderDLL::CShaderDLL()
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
}
#ifdef _WIN32
#define EXE_STRING ".exe"
#elif __linux__
#define EXE_STRING "_linux"
#else
#define EXE_STRING
#endif // _WIN32


//-----------------------------------------------------------------------------
// Connect, disconnect...
//-----------------------------------------------------------------------------
bool CShaderDLL::Connect( CreateInterfaceFn factory, bool bIsMaterialSystem )
{
	g_pHardwareConfig =  (IMaterialSystemHardwareConfig*)factory( MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, NULL );
	g_pConfig = (const MaterialSystem_Config_t*)factory( MATERIALSYSTEM_CONFIG_VERSION, NULL );
	g_pSLShaderSystem =  (IShaderSystem*)factory( SHADERSYSTEM_INTERFACE_VERSION, NULL );

	char cExeName[MAX_PATH];
	if ( !bIsMaterialSystem && Sys_GetExecutableName(cExeName, MAX_PATH) && V_stricmp(V_GetFileName(cExeName), "srcds" EXE_STRING) != 0)
	{
		ConnectTier1Libraries( &factory, 1 );
  		InitShaderLibCVars( factory );

		char cFileSystem[MAX_PATH];
		bool bSteam;
		FSReturnCode_t ret = FileSystem_GetFileSystemDLLName(cFileSystem, MAX_PATH, bSteam);
		if (ret == FS_OK && Sys_LoadInterface(cFileSystem, FILESYSTEM_INTERFACE_VERSION, &filesystem_module, (void**)&filesystem))
		{
			if ((spp_utils_module = filesystem->LoadModule("spp_utils" DLL_EXT_STRING, "sharedbin", false)) == NULL)
				return false;

			if ((spp_utils = (IGameSharedUtils*)Sys_GetFactory(spp_utils_module)(SPP_UTILS_INTERFACE, NULL)) == NULL)
				return false;

			spp_utils->ShaderInit();
			g_pShaderExtension = (IShaderExtensionInternal*)spp_utils->QueryInterface(SHADEREXTENSIONINTERNAL_INTERFACE_VERSION);
		}
	}

	return ( g_pConfig != NULL ) && (g_pHardwareConfig != NULL) && ( g_pSLShaderSystem != NULL );
}

void CShaderDLL::Disconnect( bool bIsMaterialSystem )
{
	if ( !bIsMaterialSystem )
	{
		ConVar_Unregister();
		DisconnectTier1Libraries();

		if (spp_utils)
		{
			spp_utils->ShaderShutdown();
			if (spp_utils->GetRefCount() <= 0)
			{
				filesystem->UnloadModule(spp_utils_module);
			}
		}

		spp_utils = nullptr;
		g_pShaderExtension = nullptr;
	}

	g_pHardwareConfig = NULL;
	g_pConfig = NULL;
	g_pSLShaderSystem = NULL;
}

bool CShaderDLL::Connect( CreateInterfaceFn factory )
{
	return Connect( factory, false );
}

void CShaderDLL::Disconnect()
{
	Disconnect( false );
}


//-----------------------------------------------------------------------------
// Iterates over all shaders
//-----------------------------------------------------------------------------
int CShaderDLL::ShaderCount() const
{
	return m_ShaderList.Count();
}

IShader *CShaderDLL::GetShader( int nShader ) 
{
	if ( ( nShader < 0 ) || ( nShader >= m_ShaderList.Count() ) )
		return NULL;

	return m_ShaderList[nShader];
}


//-----------------------------------------------------------------------------
// Adds to the shader lists
//-----------------------------------------------------------------------------
void CShaderDLL::InsertShader( IShader *pShader )
{
	Assert( pShader );
	m_ShaderList.AddToTail( pShader );
}

