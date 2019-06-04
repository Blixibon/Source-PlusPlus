#ifndef CONTENT_MOUNTER_H
#define CONTENT_MOUNTER_H
namespace Mounter
{
	void MountExtraContent();
	bool RegenSentenceFile(IFileSystem* const pFileSystem, const char *pszFileName);
}

#endif