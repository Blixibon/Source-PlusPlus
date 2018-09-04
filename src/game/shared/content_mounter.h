#ifndef CONTENT_MOUNTER_H
#define CONTENT_MOUNTER_H
namespace Mounter
{
	void MountExtraContent();
	CUtlVector< char *, CUtlMemory< char *, int> > *GetSentenceFiles();
}

#endif