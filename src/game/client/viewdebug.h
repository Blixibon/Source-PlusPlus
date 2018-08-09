//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef VIEWDEBUG_H
#define VIEWDEBUG_H

#if defined( _WIN32 )
#pragma once
#endif

class CNewViewSetup;

//-----------------------------------------------------------------------------
// Purpose: Implements the debugging elements of view rendering
//-----------------------------------------------------------------------------
class CDebugViewRender
{
	DECLARE_CLASS_NOBASE( CDebugViewRender );
public:
	// Draws all the debugging info
	static void	Draw3DDebuggingInfo( const CNewViewSetup &view );
	static void	Draw2DDebuggingInfo( const CNewViewSetup &view );
	static void GenerateOverdrawForTesting();
};

#endif // VIEWDEBUG_H
