//========= Made by The HL2CE Team, some rights reserved. =====================//
//
// Purpose: Hacked sound system code to allow dynamic precaching and loading of sound script files.
//
//=============================================================================//
//=============================================================================//
#ifndef _SOUNDEMITTERHACK_H_INCLUDED_
#define _SOUNDEMITTERHACK_H_INCLUDED_

#pragma once

#include <soundemittersystem/isoundemittersystembase.h>

extern ISoundEmitterSystemBase *soundemitterbase;

abstract_class CSoundEmitterSystemBase : public ISoundEmitterSystemBase
{
public:
	static void AddSoundsFromFile(const char *filename, bool bPreload=true, bool bIsOverride=false, bool unknown=true);

private:
	static void *GetAddSoundsFromFilePtr();
};

#endif

//Note for programmers: To load a sound script using this, put this somewhere in your code, include this header file, and then use this pointer and this 

//CSoundEmitterSystemBase *pAddSoundFiles = reinterpret_cast<CSoundEmitterSystemBase *>(soundemitterbase); 
//pAddSoundFiles->AddSoundsFromFile("file", true, false, true); 
