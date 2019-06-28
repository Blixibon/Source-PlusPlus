//========= Made by The HL2CE Team, some rights reserved. =====================//
//
// Purpose: Hacked sound system code to allow dynamic precaching and loading of sound script files.
//
//=============================================================================//

#include "cbase.h"
#include "CHackedSoundEmitter.h"
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#pragma warning(disable: 4005)
#include "windows.h"
#pragma warning(default: 4005)
#include "tier0/protected_things.h"
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;
static CDllDemandLoader g_pSoundEmitterSystemDLL("SoundEmitterSystem");

typedef void(ISoundEmitterSystemBase::*AddSoundsFromFileFunc)(const char *, bool, bool, bool);

void CSoundEmitterSystemBase::AddSoundsFromFile(const char *filename, bool bPreload, bool bIsOverride, bool unknown)
{
	static AddSoundsFromFileFunc m_pFunc = nullptr;
	if(!m_pFunc) {
		union { void *ui; AddSoundsFromFileFunc uo; };
		ui = CSoundEmitterSystemBase::GetAddSoundsFromFilePtr();
		m_pFunc = uo;
	}
	if(m_pFunc)
		return (soundemitterbase->*m_pFunc)(filename, bPreload, bIsOverride, unknown);
}

void *CSoundEmitterSystemBase::GetAddSoundsFromFilePtr()
{
	static const char *signature = "\x55\x8B\xEC\x83\xEC\x20\x53\x56";
	unsigned char buffer[512] = "";
	size_t written = 0;
	size_t length = V_strlen(signature);
	for(size_t i = 0; i < length; i++) {
		if(written >= sizeof(buffer))
			break;
		buffer[written++] = signature[i];
		if(signature[i] == '\\' && signature[i + 1] == 'x') {
			if(i + 3 >= length)
				continue;
			char s_byte[3];
			int r_byte;
			s_byte[0] = signature[i + 2];
			s_byte[1] = signature[i + 3];
			s_byte[2] = '\0';
			sscanf(s_byte, "%x", &r_byte);
			buffer[written - 1] = r_byte;
			i += 3;
		}
	}
	const char *pattern = (const char *)buffer;
	const void *handle = (const void *)g_pSoundEmitterSystemDLL.GetFactory();
	uintptr_t baseAddr = 0;
	if(!handle)
		return nullptr;
	MEMORY_BASIC_INFORMATION meminfo;
	IMAGE_DOS_HEADER *dos = nullptr;
	IMAGE_NT_HEADERS *pe = nullptr;
	IMAGE_FILE_HEADER *file = nullptr;
	IMAGE_OPTIONAL_HEADER *opt = nullptr;
	if(!VirtualQuery(handle, &meminfo, sizeof(MEMORY_BASIC_INFORMATION)))
		return nullptr;
	baseAddr = reinterpret_cast<uintptr_t>(meminfo.AllocationBase);
	dos = reinterpret_cast<IMAGE_DOS_HEADER *>(baseAddr);
	pe = reinterpret_cast<IMAGE_NT_HEADERS *>(baseAddr + dos->e_lfanew);
	file = &pe->FileHeader;
	opt = &pe->OptionalHeader;
	if(dos->e_magic != IMAGE_DOS_SIGNATURE || pe->Signature != IMAGE_NT_SIGNATURE || opt->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		return nullptr;
	if(file->Machine != IMAGE_FILE_MACHINE_I386)
		return nullptr;
	if((file->Characteristics & IMAGE_FILE_DLL) == 0)
		return nullptr;
	char *ptr, *end;
	ptr = reinterpret_cast<char *>(baseAddr);
	end = ptr + opt->SizeOfImage - written;
	bool found = false;
	while(ptr < end) {
		found = true;
		for(register size_t i = 0; i < written; i++) {
			if(pattern[i] != '\x2A' && pattern[i] != ptr[i]) {
				found = false;
				break;
			}
		}
		if(found)
			return ptr;
		ptr++;
	}
	return nullptr;
}
