#include "cbase.h"
#include "ConCommandReplace.h"
#include "icvar.h"
#include "icommandline.h"

ConCommandInfo_t::~ConCommandInfo_t() {
	if (m_bUsingCommandCallbackInterface) {
		m_pCommand->m_bUsingCommandCallbackInterface = true;
		m_pCommand->m_bUsingNewCommandCallback = false;
		m_pCommand->m_pCommandCallback = m_pCommandCallback;
	}
	else if (m_bUsingNewCommandCallback) {
		m_pCommand->m_bUsingCommandCallbackInterface = false;
		m_pCommand->m_bUsingNewCommandCallback = true;
		m_pCommand->m_fnCommandCallback = m_fnCommandCallback;
	}
	else {
		m_pCommand->m_bUsingCommandCallbackInterface = false;
		m_pCommand->m_bUsingNewCommandCallback = false;
		m_pCommand->m_fnCommandCallbackV1 = m_fnCommandCallbackV1;
	}
}

CConCommandReplace::CConCommandReplace(const CConCommandReplacement* list, const uint iNumReplacements) {
	ReplaceConCommands(list, iNumReplacements);
}
CConCommandReplace::~CConCommandReplace() {
	// Cleanup!
	m_OverridenConCommands.RemoveAll();
}
bool CConCommandReplace::ReplaceConCommands(const CConCommandReplacement* list, const uint iNumReplacements) {
	int fail{0};
	for (uint i = 0; i < iNumReplacements; i++) {
		if (!ReplaceConCommand(list[i].cmdName, list[i].replaceFn)) fail++; // so much fail
	}
	if (fail) {
		Warning("\tFailed to replace %i ConCommands!\n", fail);
		return false;
	}
	return true;
}
bool CConCommandReplace::ReplaceConCommand(const char* cmdName, const FnCommandCallback_t replaceFn) {
	Assert(g_pCVar && cvar);
	ConCommand* theCommand = cvar->FindCommand(cmdName);
	if (!theCommand || !theCommand->IsCommand()) {
		Warning("\tCould not find ConCommand \"%s\" to replace, or that ConCommand was not valid.", cmdName);
		return false;
	}
	else {
		return ReplaceConCommand(theCommand, replaceFn);
	}
}
bool CConCommandReplace::ReplaceConCommand(ConCommand* replaceCmd, const FnCommandCallback_t replaceFn) {
	Assert(g_pCVar && cvar);
	char szNoRepNameBuffer[128];
	sprintf(szNoRepNameBuffer, "-sec-no-replace-%s", replaceCmd->GetName());
	if (!CommandLine()->FindParm(szNoRepNameBuffer)) {
		ConCommandInfo_t* commandInfo = new ConCommandInfo_t;
		commandInfo->m_pCommand = replaceCmd;
		commandInfo->m_bUsingCommandCallbackInterface = replaceCmd->m_bUsingCommandCallbackInterface;
		commandInfo->m_bUsingNewCommandCallback = replaceCmd->m_bUsingNewCommandCallback;
		if (replaceCmd->m_bUsingCommandCallbackInterface) {
			commandInfo->m_pCommandCallback = replaceCmd->m_pCommandCallback;
		}
		else if (replaceCmd->m_bUsingNewCommandCallback) {
			commandInfo->m_fnCommandCallback = replaceCmd->m_fnCommandCallback;
		}
		else {
			commandInfo->m_fnCommandCallbackV1 = replaceCmd->m_fnCommandCallbackV1;
		}
		replaceCmd->m_bUsingCommandCallbackInterface = false;
		replaceCmd->m_bUsingNewCommandCallback = true;
		replaceCmd->m_fnCommandCallback = replaceFn;
		Assert(g_pMemAlloc);
		m_OverridenConCommands.AddToTail(commandInfo);
		Msg("\tOverridden ConCommand \"%s\". New address: %p\n", replaceCmd->GetName(), (void*)replaceFn);
		return true;
	}
	else {
		Warning("\tSkipping replacement of ConCommand \"%s\" because of command-line argument \"%s\".\n", replaceCmd->GetName(), szNoRepNameBuffer);
		return false;
	}
}
bool CConCommandReplace::RestoreConCommand(const char* cmdName) {
	Assert(g_pCVar && cvar);
	ConCommand* theCommand = cvar->FindCommand(cmdName);
	if (!theCommand || !theCommand->IsCommand()) {
		Warning("\tCould not find ConCommand \"%s\" to restore, or that ConCommand was not valid.\n", cmdName);
		return false;
	}
	else {
		return RestoreConCommand(theCommand);
	}
}
bool CConCommandReplace::RestoreConCommand(ConCommand* replaceCmd) {
	Assert(g_pCVar && cvar);
	ConCommandInfo_t tempInfo;
	tempInfo.m_pCommand = replaceCmd;
	return m_OverridenConCommands.FindAndRemove(&tempInfo);
}

// GLOBAL CConCommandReplace
CConCommandReplace g_ConCommandReplace;

