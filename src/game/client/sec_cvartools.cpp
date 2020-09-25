#include "cbase.h"
#include "ConVar.h"
#include "icvar.h"
#include <time.h>
#include "ConCommandReplace.h"
#include "utlrbtree.h"
#include "filesystem.h"
#include "convar_serverbounded.h"
#include "gameui/modinfo.h"
#if 0
static unsigned long int unregisteredUnlocked{ 0 };
static unsigned long int developmentOnlyUnlocked{ 0 };
static unsigned long int hiddenUnlocked{ 0 };
static unsigned long int conVarsUnlocked{ 0 };
static unsigned long int conCommandsUnlocked{ 0 };

static void unlockCVars() {
	if (conVarsUnlocked + conCommandsUnlocked == 0) {
		Msg("Unlocking all ConVars and ConCommands...\n");
		ICvar::Iterator iter(cvar);

		for (iter.SetFirst(); iter.IsValid(); iter.Next()) {
			ConCommandBase * base{ iter.Get() };
			bool bIsConVar = false;
			if (!(base->IsCommand())) {
				ConVar *const baseCVar{ reinterpret_cast<ConVar *const>(base) };
				base = baseCVar->m_pParent;
				baseCVar->m_bHasMax = false;
				baseCVar->m_bHasMin = true;
				bIsConVar = true;
			}

			base->m_bRegistered = true;
			static constexpr const int32 badflags{
			FCVAR_UNREGISTERED | FCVAR_DEVELOPMENTONLY |
			FCVAR_HIDDEN | FCVAR_PROTECTED | FCVAR_SPONLY |
			FCVAR_PRINTABLEONLY | FCVAR_UNLOGGED |
			FCVAR_NEVER_AS_STRING | FCVAR_CHEAT |
			FCVAR_DONTRECORD | FCVAR_NOT_CONNECTED |
			FCVAR_SERVER_CANNOT_QUERY | FCVAR_DEMO
			};

			static constexpr const int32 goodflags{
				FCVAR_SERVER_CAN_EXECUTE |
				FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_ACCESSIBLE_FROM_THREADS
			};
			static const int32 realflags = base->GetFlags();
			base->AddFlags(goodflags);
			if (realflags & ~badflags) {
				if (base->IsFlagSet(FCVAR_UNREGISTERED)) {
					++unregisteredUnlocked;
				}
				if (base->IsFlagSet(FCVAR_DEVELOPMENTONLY)) {
					++developmentOnlyUnlocked;
				}
				if (base->IsFlagSet(FCVAR_HIDDEN)) {
					++hiddenUnlocked;
				}
				base->RemoveFlags(badflags);
				if (bIsConVar) {
					++conVarsUnlocked;
				}
				else {
					++conCommandsUnlocked;
				}
			}
		}
		ECLog("\nUnlocked %lu ConVars and %lu ConCommands.\n", conVarsUnlocked, conCommandsUnlocked);
		ECLog("\tRegistered %lu previously-unregistered ConVars and ConCommands\n", unregisteredUnlocked);
		ECLog("\tUnlocked %lu development-only ConVars and ConCommands\n", developmentOnlyUnlocked);
		ECLog("\tUnhid %lu hidden ConVars and ConCommands\n\n", hiddenUnlocked);
	}
	else {
		ECLog("All ConVars and ConCommands are already unlocked!\n");
	}
	
}

static ConCommand sec_unlockallcvars("sec_unlockallcvars", unlockCVars, "Unlocks all console commands and variables that were compiled into the game.", FCVAR_NONE, nullptr);
#endif
static ConVar sec_printdescription_offsets("sec_printdescription_offsets", "0", FCVAR_ARCHIVE, "Displays the DLL identifier for all ConCommandBases. For ConCommands, displays the address of the callback and its type.");
struct PrintConVarFlags_t
{
	int flag;
	const char *desc;
};
static PrintConVarFlags_t g_PrintConVarFlags[] =
{
	{ FCVAR_UNREGISTERED, "unregistered" },
	{ FCVAR_DEVELOPMENTONLY, "developmentonly" },
	{ FCVAR_GAMEDLL, "game" },
	{ FCVAR_CLIENTDLL, "client" },
	{ FCVAR_HIDDEN, "hidden" },
	{ FCVAR_PROTECTED, "protected" },
	{ FCVAR_SPONLY, "sponly" },
	{ FCVAR_ARCHIVE, "archive" },
	{ FCVAR_NOTIFY, "notify" },
	{ FCVAR_USERINFO, "userinfo" },
	{ FCVAR_PRINTABLEONLY, "printableonly" },
	{ FCVAR_UNLOGGED, "unlogged" },
	{ FCVAR_NEVER_AS_STRING, "never_as_string" },
	{ FCVAR_REPLICATED, "replicated" },
	{ FCVAR_CHEAT, "cheat" },
	//{ FCVAR_SS, "ss" },
	{ FCVAR_DEMO, "demo" },
	{ FCVAR_DONTRECORD, "dontrecord" },
	//{ FCVAR_SS_ADDED, "ss_added" },
	//{ FCVAR_RELEASE, "release" },
	{ FCVAR_RELOAD_MATERIALS, "reload_materials" },
	{ FCVAR_RELOAD_TEXTURES, "reload_textures" },
	{ FCVAR_NOT_CONNECTED, "not_connected" },
	{ FCVAR_MATERIAL_SYSTEM_THREAD, "material_system_thread" },
	{ FCVAR_ARCHIVE_XBOX, "archive_xbox" },
	{ FCVAR_ACCESSIBLE_FROM_THREADS, "accessible_from_threads" },
	{ FCVAR_SERVER_CAN_EXECUTE, "server_can_execute" },
	{ FCVAR_SERVER_CANNOT_QUERY, "server_cannot_query" },
	{ FCVAR_CLIENTCMD_CAN_EXECUTE, "clientcmd_can_execute" }


};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_AppendFlags(const ConCommandBase *var, char *buf, size_t bufsize)
{
	for (int i = 0; i < ARRAYSIZE(g_PrintConVarFlags); ++i)
	{
		const PrintConVarFlags_t &info = g_PrintConVarFlags[i];
		if (var->IsFlagSet(info.flag))
		{
			char append[128];
			snprintf(append, sizeof(append), " %s;", info.desc);
			V_strncat(buf, append, bufsize, COPY_ALL_CHARACTERS);
		}
	}
}

void AppendPrintf(char *buf, size_t bufsize, char const *fmt, ...)
{
	char scratch[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(scratch, sizeof(scratch) - 1, fmt, argptr);
	va_end(argptr);
	scratch[sizeof(scratch) - 1] = 0;
	V_strncat(buf, scratch, bufsize, COPY_ALL_CHARACTERS);
}

static const Color gold(224, 172, 43, 255);
static const Color blue(42, 181, 224, 255);
static const Color grey(188, 188, 188, 255);


static bool ConCommandBaseLessFunc(const ConCommandBase* const &lhs, const ConCommandBase* const &rhs)
{
	const char *left = lhs->GetName();
	const char *right = rhs->GetName();

	if (*left == '-' || *left == '+')
		left++;
	if (*right == '-' || *right == '+')
		right++;

	return (_stricmp(left, right) < 0);
}
// I used to specify an initial size here, but doing that requires g_pMemAlloc to be accessed before it's actually initialized. Instead, it's grown to 512 entries in SEC_Console_replace.
static CUtlRBTree<const ConCommandBase*> ConCommandBaseTree(ConCommandBaseLessFunc);

static void UpdateCCBTree() {
	// Purge the current list. 
	ConCommandBaseTree.Purge();
	// Loop through cvars...
	ICvar::Iterator iter(cvar);
	const ConCommandBase* curVar; // optimization
	for (iter.SetFirst(); iter.IsValid(); iter.Next())
	{
		curVar = iter.Get();
		ConCommandBaseTree.Insert(curVar);
	}
	Msg("Updated SEC CCB Tree.\n");
}

static ConCommand sec_updateccbtree("sec_updateccbtree", UpdateCCBTree, "SEC uses a CUtlRBTree of ConCommandBases (CCBs) to accelerate CCB search speed and alphabetize results. This command updates that list in the case that any new CCBs are registered.", FCVAR_NONE, nullptr);
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define CVLog(fHandle, ...)  if(fHandle){ g_pFullFileSystem->FPrintf(fHandle, __VA_ARGS__); } else { ECLog(__VA_ARGS__); }
void SEC_ConVar_PrintDescription(const ConCommandBase *pVar, const FileNameHandle_t& fHandle = NULL)
{

	Assert(pVar);

	// Print name
	ConColorMsg(gold, "\"%s\"", pVar->GetName());
	// Awful formatting hack for nice alignment!
	unsigned int lenName = strlen(pVar->GetName()) + 2;
	char awfulFormattingHack[6];
	sprintf_s(awfulFormattingHack, 6, "%%-%is", 60 - lenName);

	// Print additional info. There's not any for ConCommands, but formatting must be maintained.
	ConVarRef useAddress{ "sec_printdescription_offsets" };
	if (!pVar->IsCommand()) {
		ConVar *cvVar = (ConVar *)pVar;
		float fMin, fMax;
		char pStrAdd[256];
		*pStrAdd = 0;

		// Handle bounded and NEVER_AS_STRING CVars
		const ConVar_ServerBounded *pBounded = dynamic_cast<const ConVar_ServerBounded*>(cvVar);
		if (cvVar->IsFlagSet(FCVAR_NEVER_AS_STRING) || pBounded) {
			int intVal = pBounded ? pBounded->GetInt() : cvVar->GetInt();
			float floatVal = pBounded ? pBounded->GetInt() : cvVar->GetFloat();
			if (fabsf(floatVal - intVal) > FLT_EPSILON) {
				AppendPrintf(pStrAdd, sizeof(pStrAdd), " = %d", floatVal);
				// scan in default
				float defaultFloat = atof(cvVar->GetDefault());
				if (fabsf(defaultFloat - floatVal) > FLT_EPSILON) {
					AppendPrintf(pStrAdd, sizeof(pStrAdd), " ( def: %.3f )", defaultFloat);
				}
			}
			else {
				AppendPrintf(pStrAdd, sizeof(pStrAdd), " = %i", intVal);
				int defaultInt = atoi(cvVar->GetDefault());
				if (defaultInt != intVal) {
					AppendPrintf(pStrAdd, sizeof(pStrAdd), " ( def: %i )", defaultInt);
				}
			}

		}
		else {
			AppendPrintf(pStrAdd, sizeof(pStrAdd), " = \"%s\"", cvVar->GetString());
			if (strcmp(cvVar->GetString(), cvVar->GetDefault())) {
				AppendPrintf(pStrAdd, sizeof(pStrAdd), " ( def: \"%s\" )", cvVar->GetDefault());
			}
		}
		// Mins/Maxes, note there won't be any if all CVars are unlocked with sec_unlock_all_cvars...
		bool bMin = cvVar->GetMin(fMin);
		bool bMax = cvVar->GetMax(fMax);

		if (bMin && bMax) {
			AppendPrintf(pStrAdd, sizeof(pStrAdd), " ( min = %.3f | max = %.3f )", fMin, fMax);
		}
		else if (bMin) {
			AppendPrintf(pStrAdd, sizeof(pStrAdd), " ( min = %.3f )", fMin);
		}
		else if (bMax) {
			AppendPrintf(pStrAdd, sizeof(pStrAdd), " ( max = %.3f )", fMax);
		}

		// If its bounded and clamped, show that
		if (pBounded && fabs(pBounded->GetFloat() - cvVar->GetFloat()) > 0.0001f)
		{
			AppendPrintf(pStrAdd, sizeof(pStrAdd), " [ bounded: %.3f server clamped to %.3f ]", cvVar->GetFloat(), pBounded->GetFloat());
		}
		else if (pBounded) {
			AppendPrintf(pStrAdd, sizeof(pStrAdd), " [ bounded ]");
		}


		Msg(awfulFormattingHack, pStrAdd);
		
	}
	else {

		Msg(awfulFormattingHack, " ");
		
	}

	
	// Help string, if one is registered - 
	const char *pStrHelp;
	pStrHelp = pVar->GetHelpText();
	if (pStrHelp && *pStrHelp)
	{

		ConColorMsg(grey, " - %s", pStrHelp);

	}
	// And, of course, we need a newline.
	Msg("\n");
	// Print flags, if we have any.
	if (pVar->m_nFlags) {
		char pStrFlags[128]{ 0 };
		ConVar_AppendFlags(pVar, pStrFlags, sizeof(pStrFlags));

		ConColorMsg(blue, "\t( Flags:%s )\n", pStrFlags);
		
	}
	// Debug info? Don't print this to a file, it's volatile and depends on the exact way the components were loaded into memory.
	if (useAddress.GetBool()) {
		if (pVar->IsCommand()) {
			ConCommand *ccVar = (ConCommand *)pVar;
			void* addr;
			const char* ifcCB_str = "Interface";
			const char* newCB_str = "Callback";
			const char* oldCB_str = "Callback V1";
			const char* cb_type;
			if (ccVar->m_bUsingCommandCallbackInterface) {
				addr = (void*)ccVar->m_pCommandCallback;
				cb_type = ifcCB_str;
			}
			else if (ccVar->m_bUsingNewCommandCallback) {
				addr = (void*)ccVar->m_fnCommandCallback;
				cb_type = newCB_str;
			}
			else {
				addr = (void*)ccVar->m_fnCommandCallbackV1;
				cb_type = oldCB_str;
			}
			ConColorMsg(grey, "\t( Address (%s): %p ; DLL Identifier: %i )\n", cb_type, addr, pVar->GetDLLIdentifier());
		}
		else {
			if (useAddress.GetBool()) {
				// For ConVars, just show what DLL it came from
				ConColorMsg(grey, "\t( DLL Identifier: %i )\n", pVar->GetDLLIdentifier());
			}
		}
	}

}

// Replace's Valve's AWFUL find!!!

void NewFind(const CCommand &args)
{
	const char *search;
	if (args.ArgC() != 2)
	{
		Msg("Usage:  find <string>\n");
		return;
	}

	// Get substring to find
	search = args.Arg(1);

	// Loop through vars and print out findings

	FOR_EACH_RBTREE(ConCommandBaseTree, idex){
		const ConCommandBase *var{ ConCommandBaseTree.Element(idex) };
		if (!V_stristr(var->GetName(), search) &&
			!V_stristr(var->GetHelpText(), search))
			continue;

		SEC_ConVar_PrintDescription(var);
	}
}

void NewHelp(const CCommand &args) {
	const char *search;
	const ConCommandBase *var;

	if (args.ArgC() != 2)
	{
		Msg("Usage:  help <cvarname>\n");
		return;
	}

	// Get name of var to find
	search = args.Arg(1);

	// Search for it
	var = (ConCommandBase*)g_pCVar->FindCommandBase(search);
	if (!var)
	{
		Msg("help:  no cvar or command named %s\n", search);
		return;
	}

	// Show info
	SEC_ConVar_PrintDescription(var);
}

void NewCvarlist(const CCommand &args) {
	//const char* empty_string{ "" };
	const ConCommandBase* curVar;		
	const char *partial{nullptr};
	const int argc = args.ArgC();
	FileNameHandle_t fHandle{NULL};
	if (argc == 2) { // command and partial.
		if (*(args.Arg(1)) == '?') {
			Msg("cvarlist: [log logfile] [ partial ]\n");
			return;
		}
		partial = args.Arg(1);
	}
	else if (argc >= 3) { // logfile and maybe partial

		char arg1lower[256];
		sprintf(arg1lower, "%s", args.Arg(1));
		V_strlower(arg1lower);
		if (!strcmp(arg1lower, "log")) {
			fHandle = g_pFullFileSystem->Open(args.Arg(2), "wb");
			if (!fHandle) {
				Msg("Failed to open log file \"%s\" for write access\n", args.Arg(2));
				return;
			}
		}
		else {
			Msg("cvarlist: [log logfile] [ partial ]\n");
			return;
		}
		if (argc >= 4) { // partial
			partial = args.Arg(3);
		}
	}

	time_t rawtime;
	time(&rawtime);
	char timeBuf[30];
	ctime_s(timeBuf, sizeof(timeBuf), &rawtime);
	g_pFullFileSystem->FPrintf(fHandle, "-------- ConVar & ConCommand List --------\n\tGame: %s\n\tList Generated: %s\n", "MOD", timeBuf);
	if (partial && partial[0]) {
		g_pFullFileSystem->FPrintf(fHandle, "\tNeedle: %s\n", partial);
	}
	g_pFullFileSystem->FPrintf(fHandle, "\tTotal ConVars & ConCommands Listed: %i\n\n", ConCommandBaseTree.Count());
	for (unsigned short i = ConCommandBaseTree.FirstInorder(); i != ConCommandBaseTree.InvalidIndex(); i = ConCommandBaseTree.NextInorder(i))
	{
		curVar = ConCommandBaseTree.Element(i);
		if (partial && !V_stristr(partial, curVar->GetName())) continue;
		SEC_ConVar_PrintDescription(curVar);
	}
	g_pFullFileSystem->FPrintf(fHandle, "--------------- End List ---------------\n");
	if (fHandle) {
		Msg("List written to file \"%s\"\n", args.Arg(2));
		g_pFullFileSystem->Close(fHandle);
	}
}

void NewFindflags(const CCommand &args) {
	if (args.ArgC() < 2)
	{
		Msg("Usage:  findflags <string>\n");
		ConColorMsg(grey, "Available flags to search for: \n");

		for (int i = 0; i < ARRAYSIZE(g_PrintConVarFlags); i++)
		{
			ConColorMsg(blue, "   - %s\n", g_PrintConVarFlags[i].desc);
		}
		return;
	}

	// Get substring to find
	const char *search = args.Arg(1);
	const ConCommandBase *curVar;
	FOR_EACH_RBTREE(ConCommandBaseTree, idex) {
		curVar = ConCommandBaseTree.Element(idex);
		for (unsigned int i = 0; i < ARRAYSIZE(g_PrintConVarFlags); i++) {
			if (!curVar->IsFlagSet(g_PrintConVarFlags[i].flag))
				continue;

			if (!V_stristr(g_PrintConVarFlags[i].desc, search))
				continue;

			SEC_ConVar_PrintDescription(curVar);

		}
	}
}

static constexpr const CConCommandReplacement SECReplaceList[] =
{
	{ "find", NewFind },
	{ "findflags", NewFindflags },
	{ "help", NewHelp },
	//{ "cvarlist", NewCvarlist },
};
static constexpr const int NUM_SEC_CONCOMMAND_REPLACEMENTS = sizeof(SECReplaceList) / sizeof(CConCommandReplacement);

//extern void NewStartupMenu_f(const CCommand& args);
void SEC_Console_Replace() {
	g_ConCommandReplace.ReplaceConCommands(SECReplaceList, NUM_SEC_CONCOMMAND_REPLACEMENTS);

	//if (ModInfo().HasMultipleSPCampaigns())
	//	g_ConCommandReplace.ReplaceConCommand("startupmenu", NewStartupMenu_f);

	// Build our alphabetical ConCommand tree. We can assume that there aren't going to be any new ConCommands registered... if there are, we should call sec_updateccbtree.
	UpdateCCBTree();
}