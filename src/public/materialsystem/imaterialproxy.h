//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMATERIALPROXY_H
#define IMATERIALPROXY_H
#pragma once

#include "interface.h"

#define IMATERIAL_PROXY_INTERFACE_VERSION "_IMaterialProxy003"

class IMaterial;
class KeyValues;

abstract_class IMaterialProxy
{
public:
	virtual bool Init( IMaterial* pMaterial, KeyValues *pKeyValues ) = 0;
	virtual void OnBind( void * ) = 0;
	virtual void Release() = 0;
	virtual IMaterial *	GetMaterial() = 0;

protected:
	// no one should call this directly
	virtual ~IMaterialProxy() {}
};

#define PRXY_STRING(p) #p
//#define EXPOSE_MATERIAL_PROXY(DLLClassName, proxy) EXPOSE_INTERFACE( DLLClassName, IMaterialProxy, PRXY_STRING(proxy) IMATERIAL_PROXY_INTERFACE_VERSION );
#define EXPOSE_MATERIAL_PROXY(DLLClassName, proxy) \
	static void* __Create##DLLClassName##proxy##_interface() {return static_cast<IMaterialProxy *>( new DLLClassName );} \
	static InterfaceReg __g_Create##DLLClassName##proxy##_reg(__Create##DLLClassName##proxy##_interface, PRXY_STRING(proxy) IMATERIAL_PROXY_INTERFACE_VERSION );

#endif // IMATERIALPROXY_H
