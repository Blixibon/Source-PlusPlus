#ifndef MAP_LOAD_HELPER_H
#define MAP_LOAD_HELPER_H
#pragma once  

#include "filesystem.h"
#include "bspfile.h"
#include "lzmaDecoder.h"
#include "lumpfiles.h"

class CMapLoadHelper
{
public:
	CMapLoadHelper(int lumpToLoad)
	{
		if (s_MapFileHandle == FILESYSTEM_INVALID_HANDLE)
		{
			Warning("Can't load map from invalid handle!!!");
			return;
		}

		if (lumpToLoad < 0 || lumpToLoad >= HEADER_LUMPS)
		{
			Warning("Can't load lump %i, range is 0 to %i!!!", lumpToLoad, HEADER_LUMPS - 1);
		}

		m_pData = NULL;
		m_pRawData = NULL;
		FileHandle_t hFile = s_MapFileHandle;
		int iLumpFile = 0;

		// Check lump files
		for (int i = (MAX_LUMPFILES - 1); i >= 0; i--)
		{
			if (s_LumpFileHandles[i] != FILESYSTEM_INVALID_HANDLE && s_LumpHeaders[i].lumpLength > 0)
			{
				if (s_LumpHeaders[i].lumpID == lumpToLoad)
				{
					hFile = s_LumpFileHandles[i];
					iLumpFile = i;
					break;
				}
			}
		}

		// Load raw lump from disk
		if (hFile != s_MapFileHandle)
		{
			m_nLumpSize = s_LumpHeaders[iLumpFile].lumpLength;
			m_nLumpUncompressedSize = 0;
			m_nLumpOffset = s_LumpHeaders[iLumpFile].lumpOffset;
			m_nLumpVersion = s_LumpHeaders[iLumpFile].lumpVersion;
		}
		else
		{
			lump_t* lump = &s_MapHeader.lumps[lumpToLoad];

			m_nLumpSize = lump->filelen;
			m_nLumpUncompressedSize = lump->uncompressedSize;
			m_nLumpOffset = lump->fileofs;
			m_nLumpVersion = lump->version;
		}

		if (!m_nLumpSize)
			return;	// this lump has no data

		unsigned nOffsetAlign, nSizeAlign, nBufferAlign;
		filesystem->GetOptimalIOConstraints(hFile, &nOffsetAlign, &nSizeAlign, &nBufferAlign);

		const bool bTryOptimal = (m_nLumpOffset % 4 == 0); // Don't return badly aligned data
		unsigned int alignedOffset = m_nLumpOffset;
		unsigned int alignedBytesToRead = ((m_nLumpSize) ? m_nLumpSize : 1);

		if (bTryOptimal)
		{
			alignedOffset = AlignValue((alignedOffset - nOffsetAlign) + 1, nOffsetAlign);
			alignedBytesToRead = AlignValue((m_nLumpOffset - alignedOffset) + alignedBytesToRead, nSizeAlign);
		}

		m_pRawData = static_cast<byte*>(filesystem->AllocOptimalReadBuffer(hFile, alignedBytesToRead, alignedOffset));
		if (!m_pRawData)
		{
			Warning("Can't load lump %i, allocation of %i bytes failed!!!", lumpToLoad, m_nLumpSize + 1);
		}

		filesystem->Seek(hFile, alignedOffset, FILESYSTEM_SEEK_HEAD);
		filesystem->ReadEx(m_pRawData, alignedBytesToRead, alignedBytesToRead, hFile);

		if (m_nLumpUncompressedSize)
		{
			if (CLZMA::IsCompressed(m_pRawData) && m_nLumpUncompressedSize == static_cast<int>(CLZMA::GetActualSize(m_pRawData)))
			{
				m_pData = static_cast<byte*>(MemAlloc_AllocAligned(m_nLumpUncompressedSize, 4));
				const int outSize = CLZMA::Uncompress(m_pRawData, m_pData);
				if (outSize != m_nLumpUncompressedSize)
				{
					Warning("Decompressed size differs from header, BSP may be corrupt\n");
				}
			}
			else
			{
				Assert(0);
				Warning("Unsupported BSP: Unrecognized compressed lump\n");
			}
		}
		else
		{
			m_pData = m_pRawData + (m_nLumpOffset - alignedOffset);
		}
	}

	~CMapLoadHelper()
	{
		if (m_nLumpUncompressedSize)
		{
			MemAlloc_FreeAligned(m_pData);
		}
		if (m_pRawData)
		{
			filesystem->FreeOptimalReadBuffer(m_pRawData);
		}
	}

	byte* LumpBase() const
	{
		return m_pData;
	}

	int	LumpSize() const
	{
		return m_nLumpUncompressedSize ? m_nLumpUncompressedSize : m_nLumpSize;
	}

	int	LumpOffset() const
	{
		return m_nLumpOffset;
	}

	int	LumpVersion() const
	{
		return m_nLumpVersion;
	}

	// Global setup/shutdown
	static void	Init(const char* loadname)
	{
		if (++s_nMapLoadRecursion > 1)
		{
			return;
		}

		s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
		V_memset(&s_MapHeader, 0, sizeof(s_MapHeader));
		V_memset(s_LumpFileHandles, (int)FILESYSTEM_INVALID_HANDLE, sizeof(s_LumpFileHandles));
		V_memset(s_LumpHeaders, 0, sizeof(s_LumpHeaders));

		V_strcpy_safe(s_szLoadName, loadname);
		s_MapFileHandle = filesystem->OpenEx(loadname, "rb");
		if (s_MapFileHandle == FILESYSTEM_INVALID_HANDLE)
		{
			Warning("CMapLoadHelper::Init, unable to open %s\n", loadname);
			return;
		}

		filesystem->Read(&s_MapHeader, sizeof(dheader_t), s_MapFileHandle);
		if (s_MapHeader.ident != IDBSPHEADER)
		{
			filesystem->Close(s_MapFileHandle);
			s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
			Warning("CMapLoadHelper::Init, map %s has wrong identifier\n", loadname);
			return;
		}

		if (s_MapHeader.version < MINBSPVERSION || s_MapHeader.version > BSPVERSION)
		{
			filesystem->Close(s_MapFileHandle);
			s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
			Warning("CMapLoadHelper::Init, map %s has wrong version (%i when expecting %i)\n", loadname,
				s_MapHeader.version, BSPVERSION);
		}

		for (int i = 0; i < MAX_LUMPFILES; i++)
		{
			char lumpPath[MAX_PATH];
			GenerateLumpFileName(s_szLoadName, lumpPath, MAX_PATH, i);
			s_LumpFileHandles[i] = filesystem->OpenEx(lumpPath, "rb");
			if (s_LumpFileHandles[i] == FILESYSTEM_INVALID_HANDLE)
			{
				if (filesystem->FileExists(lumpPath))
					Warning("CMapLoadHelper::Init, unable to open %s\n", lumpPath);
				continue;
			}

			filesystem->Read(&s_LumpHeaders[i], sizeof(lumpfileheader_t), s_LumpFileHandles[i]);
			if (s_LumpHeaders[i].lumpID < 0 || s_LumpHeaders[i].lumpID >= HEADER_LUMPS)
			{
				filesystem->Close(s_LumpFileHandles[i]);
				s_LumpFileHandles[i] = FILESYSTEM_INVALID_HANDLE;
				Warning("CMapLoadHelper::Init, lump file %s has invalid identifier\n", lumpPath);
				continue;
			}

			if (s_LumpHeaders[i].mapRevision <= s_MapHeader.mapRevision)
			{
				filesystem->Close(s_LumpFileHandles[i]);
				s_LumpFileHandles[i] = FILESYSTEM_INVALID_HANDLE;
				Warning("CMapLoadHelper::Init, lump file %s has same or older revision as bsp!\n", lumpPath);
				continue;
			}
		}
	}

	static void	Shutdown()
	{
		if (--s_nMapLoadRecursion > 0)
		{
			return;
		}

		if (s_MapFileHandle != FILESYSTEM_INVALID_HANDLE)
		{
			filesystem->Close(s_MapFileHandle);
			s_MapFileHandle = FILESYSTEM_INVALID_HANDLE;
		}

		for (int i = 0; i < MAX_LUMPFILES; i++)
		{
			if (s_LumpFileHandles[i] != FILESYSTEM_INVALID_HANDLE)
			{
				filesystem->Close(s_LumpFileHandles[i]);
				s_LumpFileHandles[i] = FILESYSTEM_INVALID_HANDLE;
			}
		}

		s_szLoadName[0] = 0;
		V_memset(&s_MapHeader, 0, sizeof(s_MapHeader));
		V_memset(s_LumpHeaders, 0, sizeof(s_LumpHeaders));
	}

private:
	int					m_nLumpSize;
	int					m_nLumpUncompressedSize;
	int					m_nLumpOffset;
	int					m_nLumpVersion;
	byte*				m_pRawData;
	byte*				m_pData;

	static dheader_t		s_MapHeader;
	static FileHandle_t		s_MapFileHandle;
	static lumpfileheader_t	s_LumpHeaders[MAX_LUMPFILES];
	static FileHandle_t		s_LumpFileHandles[MAX_LUMPFILES];
	static char				s_szLoadName[64];
	static int				s_nMapLoadRecursion;
};

#endif // !MAP_LOAD_HELPER_H

