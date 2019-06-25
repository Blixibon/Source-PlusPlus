//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef TMESSAGE_H
#define TMESSAGE_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define DEMO_MESSAGE "__DEMOMESSAGE__"
#define NETWORK_MESSAGE1 "__NETMESSAGE__1"
#define NETWORK_MESSAGE2 "__NETMESSAGE__2"
#define NETWORK_MESSAGE3 "__NETMESSAGE__3"
#define NETWORK_MESSAGE4 "__NETMESSAGE__4"
#define NETWORK_MESSAGE5 "__NETMESSAGE__5"
#define NETWORK_MESSAGE6 "__NETMESSAGE__6"

#include "client_textmessage.h"

// text message system
void					TextMessageInit( void );
//client_textmessage_t *TextMessageGet( const char *pName );
void					TextMessageShutdown( void );

#ifdef __cplusplus
}
#endif // __cplusplus

#endif		//TMESSAGE_H
