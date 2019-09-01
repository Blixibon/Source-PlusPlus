#ifndef CONCOMMANDREPLACE_H
#define CONCOMMANDREPLACE_H
#pragma once

#include "convar.h"
#include "utlvector.h"
struct CConCommandReplacement {

	const char* cmdName;
	const FnCommandCallback_t replaceFn;

};
struct ConCommandInfo_t {
	ConCommand* m_pCommand;
	union {
		FnCommandCallbackVoid_t m_fnCommandCallbackV1;
		FnCommandCallback_t m_fnCommandCallback{ nullptr };
		ICommandCallback * m_pCommandCallback;
	};
	bool m_bUsingNewCommandCallback;
	bool m_bUsingCommandCallbackInterface;
	constexpr bool operator=(const ConCommand*const eqCmd) const { return (eqCmd == m_pCommand); }
	~ConCommandInfo_t();
};
class CConCommandReplace {
public:
	CConCommandReplace() {  };
	CConCommandReplace(const CConCommandReplacement* list, const uint iNumReplacements);
	~CConCommandReplace();

	bool ReplaceConCommands(const CConCommandReplacement* list, const uint iNumReplacements);
	bool ReplaceConCommand(const char* cmdName, const FnCommandCallback_t);
	bool ReplaceConCommand(ConCommand* replaceCmd, const FnCommandCallback_t replaceFn);

	bool RestoreConCommand(const char* cmdName);
	bool RestoreConCommand(ConCommand* restoreCmd);

private:
	CUtlVector<ConCommandInfo_t*> m_OverridenConCommands{};
};

extern CConCommandReplace g_ConCommandReplace;
#endif