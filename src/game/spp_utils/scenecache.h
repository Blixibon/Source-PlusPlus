#pragma once

#include "scenefilecache/ISceneFileCache.h"
#include "tier2/tier2.h"
#include "../shared/choreoscene.h"
#include "checksum_crc.h"
#include "utlbinaryblock.h"
#include "utlmap.h"

class CLZSS;
typedef void (*MsgFunc_t) (PRINTF_FORMAT_STRING const char* fmt, ...);

class CSceneFileCache : public CTier0AppSystem<ISceneFileCache>
{
public:
	CSceneFileCache() : CTier0AppSystem<ISceneFileCache>(false), m_SceneTable(DefLessFunc(CRC32_t))
	{}

	// Init, shutdown
	virtual InitReturnVal_t Init();

	// Inherited via ISceneFileCache
	virtual size_t GetSceneBufferSize(char const* filename) override;
	virtual bool GetSceneData(char const* filename, byte* buf, size_t bufsize) override;
	virtual bool GetSceneCachedData(char const* pFilename, SceneCachedData_t* pData) override;
	virtual bool GetSceneCachedData_V1(char const* pFilename, SceneCachedDataV1_t* pData) override;
	virtual short GetSceneCachedSound(int iScene, int iSound) override;
	virtual const char* GetSceneString(short stringId) override;
	virtual void Reload() override;

	void	AddSceneFile(CRC32_t crcFilename, CRC32_t crcSource, CChoreoScene* pScene);

	CInterlockedInt m_ThreadActive;
protected:
	void	BlockForLoad();
	static CLZSS compressor;

	struct internalSceneData_t : public SceneCachedData_t
	{
		CUtlBinaryBlock binary;
		CCopyableUtlVector<short> sounds;
		size_t	uncompressedSize;
	};

	typedef unsigned short SceneIndex_t;

	SceneIndex_t	GetSceneIndex(char const* pFilename);

	class CCacheStringPool : public IChoreoStringPool
	{
	public:
		virtual short FindOrAddString(const char* pString)
		{
			return m_StringPool.AddString(pString);
		}

		virtual bool	GetString(short stringId, char* buff, int buffSize)
		{
			if (stringId < 0 || (int)stringId >= m_StringPool.GetNumStrings())
			{
				V_strncpy(buff, "", buffSize);
				return false;
			}

			const char* pszString = m_StringPool.String(stringId);
			if (!pszString)
			{
				V_strncpy(buff, "", buffSize);
				return false;
			}

			V_strncpy(buff, pszString, buffSize);
			return true;
		}

		bool TableHasRoom()
		{
			return m_StringPool.GetNumStrings() < USHRT_MAX - 100;
		}

		CUtlSymbolTable m_StringPool;
	};

	CCacheStringPool m_PersistantStrings;
	CUtlMap<CRC32_t, internalSceneData_t, SceneIndex_t> m_SceneTable;
	CInterlockedInt m_ScenesLoaded;
	ThreadHandle_t m_LoaderThread;

	friend void DoLoadScenes(CSceneFileCache* pThis, MsgFunc_t print);
};