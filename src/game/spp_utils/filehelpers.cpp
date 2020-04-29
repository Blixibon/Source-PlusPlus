#include "filehelpers.h"
#include "filesystem.h"

int GetFileList(const char* pDirPath, const char* pPattern, CUtlVector<CUtlString>& fileList, const char* pszPathID)
{
	char	sourcePath[MAX_PATH];
	char	fullPath[MAX_PATH];
	bool	bFindDirs;

	fileList.Purge();

	strcpy(sourcePath, pDirPath);
	int len = (int)strlen(sourcePath);
	if (!len)
	{
		strcpy(sourcePath, ".\\");
	}
	else if (sourcePath[len - 1] != '\\')
	{
		sourcePath[len] = '\\';
		sourcePath[len + 1] = '\0';
	}

	strcpy(fullPath, sourcePath);
	if (pPattern[0] == '\\' && pPattern[1] == '\0')
	{
		// find directories only
		bFindDirs = true;
		strcat(fullPath, "*");
	}
	else
	{
		// find files, use provided pattern
		bFindDirs = false;
		strcat(fullPath, pPattern);
	}

	FileFindHandle_t hFind = FILESYSTEM_INVALID_FIND_HANDLE;
	const char* pszFile = g_pFullFileSystem->FindFirstEx(fullPath, pszPathID, &hFind);

	for (; pszFile; pszFile = g_pFullFileSystem->FindNext(hFind))
	{
		// dos attribute complexities i.e. _A_NORMAL is 0
		if (bFindDirs)
		{
			// skip non dirs
			if (!g_pFullFileSystem->FindIsDirectory(hFind))
				continue;
		}
		else
		{
			// skip dirs
			if (g_pFullFileSystem->FindIsDirectory(hFind))
				continue;
		}

		if (!stricmp(pszFile, "."))
			continue;

		if (!stricmp(pszFile, ".."))
			continue;

		char fileName[MAX_PATH];
		strcpy(fileName, sourcePath);
		strcat(fileName, pszFile);

		int j = fileList.AddToTail();
		fileList[j].Set(fileName);
	}

	g_pFullFileSystem->FindClose(hFind);

	return fileList.Count();
}

void RecurseFileTree_r(const char* pDirPath, int depth, CUtlVector< CUtlString >& dirList, const char* pszPathID)
{
	// recurse from source directory, get directories only
	CUtlVector< CUtlString > fileList;
	int dirCount = GetFileList(pDirPath, "\\", fileList, pszPathID);
	if (!dirCount)
	{
		// add directory name to search tree
		int j = dirList.AddToTail();
		dirList[j].Set(pDirPath);
		return;
	}

	for (int i = 0; i < dirCount; i++)
	{
		// form new path name, recurse into
		RecurseFileTree_r(fileList[i].Get(), depth + 1, dirList, pszPathID);
	}

	int j = dirList.AddToTail();
	dirList[j].Set(pDirPath);
}

int FindFiles(char* pFileMask, bool bRecurse, CUtlVector<CUtlString>& fileList, const char* pszPathID)
{
	char	dirPath[MAX_PATH];
	char	pattern[MAX_PATH];
	char	extension[MAX_PATH];

	// get path only
	strcpy(dirPath, pFileMask);
	V_StripFilename(dirPath);

	// get pattern only
	V_FileBase(pFileMask, pattern, sizeof(pattern));
	V_ExtractFileExtension(pFileMask, extension, sizeof(extension));
	if (extension[0])
	{
		strcat(pattern, ".");
		strcat(pattern, extension);
	}

	if (!bRecurse)
	{
		GetFileList(dirPath, pattern, fileList, pszPathID);
	}
	else
	{
		// recurse and get the tree
		CUtlVector< CUtlString > tempList;
		CUtlVector< CUtlString > dirList;
		RecurseFileTree_r(dirPath, 0, dirList, pszPathID);
		for (int i = 0; i < dirList.Count(); i++)
		{
			// iterate each directory found
			tempList.Purge();
			tempList.EnsureCapacity(dirList.Count());

			GetFileList(dirList[i].String(), pattern, tempList, pszPathID);

			int start = fileList.AddMultipleToTail(tempList.Count());
			for (int j = 0; j < tempList.Count(); j++)
			{
				fileList[start + j] = tempList[j];
			}
		}
	}

	return fileList.Count();
}
