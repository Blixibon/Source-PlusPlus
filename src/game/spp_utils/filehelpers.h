#pragma once
#include "utlvector.h"
#include "utlstring.h"

int FindFiles(char* pFileMask, bool bRecurse, CUtlVector<CUtlString>& fileList, const char *pszPathID = nullptr);