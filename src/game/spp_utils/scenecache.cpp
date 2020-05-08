#include "scenecache.h"
#include "filesystem.h"
#include "filehelpers.h"
#include "utlbuffer.h"
#include "tier3/scenetokenprocessor.h"
#include "lzmaDecoder.h"
#include "lzss.h"
#include "fmtstr.h"
#include "scenefilecache/SceneImageFile.h"
#include "shared.h"

//-----------------------------------------------------------------------------
// Binary compiled VCDs get their strings from a pool
//-----------------------------------------------------------------------------
class CChoreoStringPool : public IChoreoStringPool
{
public:
	CChoreoStringPool(SceneImageHeader_t *pHeader) : m_pHeader(pHeader)
	{}

	short FindOrAddString(const char* pString)
	{
		// huh?, no compilation at run time, only fetches
		Assert(0);
		return -1;
	}

	bool GetString(short stringId, char* buff, int buffSize)
	{
		// fetch from compiled pool
		const char* pString = m_pHeader->String(stringId);
		if (!pString)
		{
			V_strncpy(buff, "", buffSize);
			return false;
		}
		V_strncpy(buff, pString, buffSize);
		return true;
	}

protected:
	SceneImageHeader_t* m_pHeader;
};

CLZSS CSceneFileCache::compressor;

void DoLoadScenes(CSceneFileCache* pThis, MsgFunc_t print)
{
	pThis->m_ScenesLoaded = 0;
	pThis->m_PersistantStrings.m_StringPool.RemoveAll();
	pThis->m_SceneTable.Purge();

	CUtlVector<CUtlString>	vcdFileList;
	FindFiles("scenes/*.vcd", true, vcdFileList, "SCENES");
	pThis->m_SceneTable.EnsureCapacity(vcdFileList.Count());
	for (auto& file : vcdFileList)
	{
		CUtlBuffer buf;
		if (g_pFullFileSystem->ReadFile(file.Get(), "SCENES", buf))
		{
			char szCleanName[MAX_PATH];
			V_strncpy(szCleanName, file.Get(), sizeof(szCleanName));
			V_strlower(szCleanName);
			V_FixSlashes(szCleanName, '\\');
			CRC32_t crcFilename = CRC32_ProcessSingleBuffer(szCleanName, strlen(szCleanName));

			CRC32_t crcSource;
			CRC32_Init(&crcSource);
			CRC32_ProcessBuffer(&crcSource, buf.Base(), buf.TellMaxPut());
			CRC32_Final(&crcSource);

			SetTokenProcessorBuffer((char*)buf.Base());
			CChoreoScene* pCacheScene = ChoreoLoadScene(szCleanName, nullptr, GetTokenProcessor(), nullptr);

			pThis->AddSceneFile(crcFilename, crcSource, pCacheScene);
			delete pCacheScene;
		}
	}
	vcdFileList.Purge();

	int iFileScenes = pThis->m_SceneTable.Count();
	if (print)
		print("Loaded %d scenes from filesystem.\n", iFileScenes);

	int iBufferSize = g_pFullFileSystem->GetSearchPath("GAME", true, nullptr, 0);
	char* pszSearchPaths = (char*)stackalloc(iBufferSize);
	g_pFullFileSystem->GetSearchPath("GAME", true, pszSearchPaths, iBufferSize);
	const char* pSceneImageName = IsX360() ? "scenes/scenes.360.image" : "scenes/scenes.image";
	for (char* path = strtok(pszSearchPaths, ";"); path; path = strtok(NULL, ";"))
	{
		char imagePath[MAX_PATH];
		V_ComposeFileName(path, pSceneImageName, imagePath, MAX_PATH);
		if (g_pFullFileSystem->FileExists(imagePath, "GAME"))
		{
			CUtlBuffer bufImageFile;
			if (g_pFullFileSystem->ReadFile(imagePath, "GAME", bufImageFile))
			{
				SceneImageHeader_t* pHeader = (SceneImageHeader_t*)bufImageFile.Base();
				if (pHeader->nId != SCENE_IMAGE_ID ||
					pHeader->nVersion != SCENE_IMAGE_VERSION)
				{
					continue;
				}
				SceneImageEntry_t* pEntries = (SceneImageEntry_t*)((byte*)pHeader + pHeader->nSceneEntryOffset);
				CChoreoStringPool imagePool(pHeader);

				for (int i = 0; i < pHeader->nNumScenes; i++)
				{
					CUtlBuffer entryBuffer;

					unsigned char* pData = (unsigned char*)pHeader + pEntries[i].nDataOffset;
					bool bIsCompressed;
					bIsCompressed = CLZMA::IsCompressed(pData);
					if (bIsCompressed)
					{
						int originalSize = CLZMA::GetActualSize(pData);

						unsigned char* pOutputData = (unsigned char*)malloc(originalSize);
						CLZMA::Uncompress(pData, pOutputData);
						entryBuffer.Put(pOutputData, originalSize);
						free(pOutputData);
					}
					else
					{
						entryBuffer.Put(pData, pEntries[i].nDataLength);
					}

					CRC32_t crcFilename = pEntries[i].crcFilename;
					unsigned short sIDX = pThis->m_SceneTable.Find(crcFilename);
					// Don't add if we already have it
					if (!pThis->m_SceneTable.IsValidIndex(sIDX))
					{
						CChoreoScene* pImageScene = new CChoreoScene(nullptr);
						CRC32_t crcSource;
						if (CChoreoScene::GetCRCFromBinaryBuffer(entryBuffer, crcSource) && pImageScene->RestoreFromBinaryBuffer(entryBuffer, CFmtStr("scenes\\%.8x.vcd", crcFilename), &imagePool))
						{
							pThis->AddSceneFile(crcFilename, crcSource, pImageScene);
						}

						delete pImageScene;
					}
				}
			}
		}
	}

	if (print)
		print("Loaded %d scenes from images.\n", pThis->m_SceneTable.Count() - iFileScenes);

	for (CSceneFileCache::SceneIndex_t i = 0; i < pThis->m_SceneTable.Count(); i++)
	{
		pThis->m_SceneTable[i].sceneId = (int)i;
	}

	if (print)
		print("Loaded %d scenes in total.\n", pThis->m_SceneTable.Count());

	pThis->m_ScenesLoaded = 1;
}

unsigned LoadInThread(void* pParams)
{
	CSceneFileCache* pThis = static_cast<CSceneFileCache*> (pParams);
	pThis->m_ThreadActive = 1;
	DoLoadScenes(pThis, Msg);
	pThis->m_ThreadActive = 0;

	return 0;
}

InitReturnVal_t CSceneFileCache::Init()
{
	Reload();

	return INIT_OK;
}

size_t CSceneFileCache::GetSceneBufferSize(char const* filename)
{
	CSceneFileCache::SceneIndex_t IDX = GetSceneIndex(filename);
	if (m_SceneTable.IsValidIndex(IDX))
	{
		auto& data = m_SceneTable.Element(IDX);
		return data.uncompressedSize;
	}

	return 0;
}

bool CSceneFileCache::GetSceneData(char const* filename, byte* buf, size_t bufsize)
{
	CSceneFileCache::SceneIndex_t IDX = GetSceneIndex(filename);
	if (m_SceneTable.IsValidIndex(IDX))
	{
		auto &data = m_SceneTable.Element(IDX);
		const byte* pData = (byte *)data.binary.Get();
		if (compressor.IsCompressed(pData))
		{
			return compressor.SafeUncompress(pData, buf, bufsize) != 0;
		}
		else
		{
			V_memcpy(buf, pData, Min(bufsize, data.uncompressedSize));
		}

		return true;
	}

	return false;
}

bool CSceneFileCache::GetSceneCachedData(char const* pFilename, SceneCachedData_t* pData)
{
	CSceneFileCache::SceneIndex_t IDX = GetSceneIndex(pFilename);
	if (m_SceneTable.IsValidIndex(IDX))
	{
		*pData = m_SceneTable.Element(IDX);
		return true;
	}

	return false;
}

short CSceneFileCache::GetSceneCachedSound(int iScene, int iSound)
{
	BlockForLoad();

	CSceneFileCache::SceneIndex_t IDX = (CSceneFileCache::SceneIndex_t)iScene;
	if (m_SceneTable.IsValidIndex(IDX))
	{
		auto& data = m_SceneTable.Element(IDX);
		if (iSound < 0 || iSound >= data.numSounds)
			return UTL_INVAL_SYMBOL;

		return data.sounds[iSound];
	}

	return UTL_INVAL_SYMBOL;
}

const char* CSceneFileCache::GetSceneString(short stringId)
{
	BlockForLoad();

	return m_PersistantStrings.m_StringPool.String(stringId);
}

void CSceneFileCache::Reload()
{
	BlockForLoad();

	//if (internaldata->IsClientConnected() || internaldata->IsServerRunning())
	//{
		DoLoadScenes(this, Msg);
	//}
	//else
	//{
	//	m_ScenesLoaded = 0;
	//	m_ThreadActive = 1;
	//	m_LoaderThread = CreateSimpleThread(LoadInThread, this);
	//}
}

void CSceneFileCache::BlockForLoad()
{
	if (m_LoaderThread)
	{
		if (!m_ScenesLoaded)
		{
			ThreadJoin(m_LoaderThread);
			ReleaseThreadHandle(m_LoaderThread);
			m_LoaderThread = 0;
		}
		else
		{
			ThreadDetach(m_LoaderThread);
			ReleaseThreadHandle(m_LoaderThread);
			m_LoaderThread = 0;
		}
	}
}

CSceneFileCache::SceneIndex_t CSceneFileCache::GetSceneIndex(char const* pFilename)
{
	BlockForLoad();

	char szCleanName[MAX_PATH];
	V_strncpy(szCleanName, pFilename, sizeof(szCleanName));
	V_strlower(szCleanName);
	V_FixSlashes(szCleanName, '\\');
	CRC32_t crcFilename = CRC32_ProcessSingleBuffer(szCleanName, strlen(szCleanName));

	CSceneFileCache::SceneIndex_t IDX = m_SceneTable.Find(crcFilename);
	if (!m_SceneTable.IsValidIndex(IDX))
	{
		return m_SceneTable.InvalidIndex();
	}

	return IDX;
}

//-----------------------------------------------------------------------------
// Helper for crawling events to determine sounds
//-----------------------------------------------------------------------------
void FindSoundsInEvent(CChoreoEvent* pEvent, IChoreoStringPool *pStringpool, CUtlVector< short >& soundList)
{
	if (!pEvent || pEvent->GetType() != CChoreoEvent::SPEAK)
		return;

	unsigned short stringId = pStringpool->FindOrAddString(pEvent->GetParameters());
	if (soundList.Find(stringId) == soundList.InvalidIndex())
	{
		soundList.AddToTail(stringId);
	}

	if (pEvent->GetCloseCaptionType() == CChoreoEvent::CC_MASTER)
	{
		char tok[CChoreoEvent::MAX_CCTOKEN_STRING];
		if (pEvent->GetPlaybackCloseCaptionToken(tok, sizeof(tok)))
		{
			stringId = pStringpool->FindOrAddString(tok);
			if (soundList.Find(stringId) == soundList.InvalidIndex())
			{
				soundList.AddToTail(stringId);
			}
		}
	}
}

void CSceneFileCache::AddSceneFile(CRC32_t crcFilename, CRC32_t crcSource, CChoreoScene* pScene)
{
	unsigned short sIDX = m_SceneTable.Insert(crcFilename);
	if (!m_SceneTable.IsValidIndex(sIDX))
		return;

	auto& data = m_SceneTable.Element(sIDX);

	pScene->SetFileName(CFmtStr("scenes\\%.8x.vcd", crcFilename));

	// Walk all events looking for SPEAK events
	CChoreoEvent* pEvent;
	for (int i = 0; i < pScene->GetNumEvents(); ++i)
	{
		pEvent = pScene->GetEvent(i);
		FindSoundsInEvent(pEvent, &m_PersistantStrings, data.sounds);
	}
	data.numSounds = data.sounds.Count();
	data.msecs = RoundFloatToUnsignedLong(pScene->FindStopTime() * 1000.0f + 0.5f);

	CUtlBuffer binBuf;
	pScene->SaveToBinaryBuffer(binBuf, crcSource, &m_PersistantStrings);

	data.uncompressedSize = binBuf.TellMaxPut();

	unsigned int iCompressedSize = 0;
	byte* pCompressed = compressor.Compress((const byte*)binBuf.Base(), binBuf.TellMaxPut(), &iCompressedSize);
	if (pCompressed)
	{
		data.binary.Set(pCompressed, iCompressedSize);
		free(pCompressed);
	}
	else
	{
		data.binary.Set(binBuf.Base(), binBuf.TellMaxPut());
	}
}
