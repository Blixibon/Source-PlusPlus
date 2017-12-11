//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <windows.h>
#include "tier0/minidump.h"
#include "tools_minidump.h"

static bool g_bToolsWriteFullMinidumps = false;
static ToolsExceptionHandler g_pCustomExceptionHandler = NULL;


// --------------------------------------------------------------------------------- //
// Internal helpers.
// --------------------------------------------------------------------------------- //

static LONG __stdcall ToolsExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	// Non VMPI workers write a minidump and show a crash dialog like normal.
	int iType = 0;	// MiniDumpNormal
	if ( g_bToolsWriteFullMinidumps )
		iType = 0x1 | 0x40;	// MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory
		
	WriteMiniDumpUsingExceptionInfo( ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo, iType );
	return EXCEPTION_CONTINUE_SEARCH;
}


static LONG __stdcall ToolsExceptionFilter_Custom( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	// Run their custom handler.
	g_pCustomExceptionHandler( ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo );
	return EXCEPTION_EXECUTE_HANDLER; // (never gets here anyway)
}


// --------------------------------------------------------------------------------- //
// Interface functions.
// --------------------------------------------------------------------------------- //

void EnableFullMinidumps( bool bFull )
{
	g_bToolsWriteFullMinidumps = bFull;
}


void SetupDefaultToolsMinidumpHandler()
{
	SetUnhandledExceptionFilter( ToolsExceptionFilter );
}


void SetupToolsMinidumpHandler( ToolsExceptionHandler fn )
{
	g_pCustomExceptionHandler = fn;
	SetUnhandledExceptionFilter( ToolsExceptionFilter_Custom );
}
