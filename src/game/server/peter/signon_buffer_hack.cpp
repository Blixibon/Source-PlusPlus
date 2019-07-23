#include "cbase.h"
#include "eiface.h"
#include <iserver.h>

#include "memdbgon.h"

namespace SIGNON_BUFFER_HACK
{
#if IsWindows()
#define INDEXOF_ISMULTIPLAYER 24
#define INDEXOF_CLEAR 40
#define MAX_INDEX 64
#elif IsLinux()
#define INDEXOF_ISMULTIPLAYER 25
#define INDEXOF_CLEAR 41
#define MAX_INDEX 65
#endif

	typedef bool(*IsMultiplayerProto) (void*);
	typedef void(*ClearProto) (void*);

	static bool s_bInClearScope = false;

	IsMultiplayerProto oIsMultiplayer;
	ClearProto oClear;

	uintptr_t** gameServer_VT = nullptr;
	uintptr_t* original_gs_vt = nullptr;
	uintptr_t* new_gs_vt = nullptr;

	bool hIsMultiplayer(void *thisptr)
	{
		if (s_bInClearScope)
			return true;
		else
			return oIsMultiplayer(thisptr);
	}

	void hClear(void *thisptr)
	{
		s_bInClearScope = true;
		oClear(thisptr);
		s_bInClearScope = false;
	}

	void InitBufferHack()
	{
		if (gameServer_VT != nullptr)
			return;

		IServer *pServer = engine->GetIServer(); // This is where the fun begins. This lets us in.

		gameServer_VT = reinterpret_cast<uintptr_t**>(pServer);
		original_gs_vt = *gameServer_VT;

		size_t total_functions = 0;

		while ((uintptr_t*)(*gameServer_VT)[total_functions]) {
			total_functions++;
		}

		Assert(total_functions == MAX_INDEX + 1);

		oIsMultiplayer = reinterpret_cast<IsMultiplayerProto>(original_gs_vt[INDEXOF_ISMULTIPLAYER]);
		oClear = reinterpret_cast<ClearProto>(original_gs_vt[INDEXOF_CLEAR]);

		new_gs_vt = new uintptr_t[total_functions];
		memcpy(new_gs_vt, original_gs_vt, (sizeof(uintptr_t) * total_functions));

		new_gs_vt[INDEXOF_ISMULTIPLAYER] = reinterpret_cast<uintptr_t>(hIsMultiplayer);
		new_gs_vt[INDEXOF_CLEAR] = reinterpret_cast<uintptr_t>(hClear);

		*gameServer_VT = new_gs_vt;
	}

	void ShutdownBufferHack()
	{
		if (gameServer_VT == nullptr)
			return;

		*gameServer_VT = original_gs_vt;
		delete[] new_gs_vt;

		gameServer_VT = nullptr;
		new_gs_vt = nullptr;
		original_gs_vt = nullptr;
	}
}