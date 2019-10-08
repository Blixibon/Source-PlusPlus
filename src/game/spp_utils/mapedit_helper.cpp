#include "mapedit_helper.h"
#include "filesystem.h"
#include "checksum_crc.h"
#include "fmtstr.h"
#include "KeyValues.h"
#include "utlvector.h"

#include "memdbgon.h"

#pragma region HELPERS
// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024

typedef struct
{
	char key[MAX_KEY];
	char value[MAX_VALUE];
} KeyValue_t;

typedef struct
{
	KeyValue_t **keyValues;
	int kvCount;
} Entity_t;

// The loaded map
typedef struct Map_
{
	CUtlVector<Entity_t*> entities;
	//int entitiesCount;
	char *entitiesBuffer = NULL;
	int entitiesBufferSize = 0;
	~Map_()
	{
		if (entitiesBuffer)
			free(entitiesBuffer);

		for (int i = 0; i < entities.Count(); i++)
		{
			if (entities[i]->keyValues)
				delete[] entities[i]->keyValues;
		}

		entities.PurgeAndDeleteElements();
	}
} Map_t;

// Misc useful
bool StrEq( const char *sz1, const char *sz2 )
{
	return ( sz1 == sz2 || _stricmp( sz1, sz2 ) == 0 );
}

bool WStrEq( wchar_t *wcs1, wchar_t *wcs2 )
{
	return ( wcs1 == wcs2 || wcscmp( wcs1, wcs2 ) == 0 );
}

bool StrContains( const char *sz1, const char *sz2 )
{
	return ( strstr( sz1, sz2 ) != NULL );
}

bool WStrContains( wchar_t *wcs1, wchar_t *wcs2 )
{
	return ( wcsstr( wcs1, wcs2 ) != NULL );
}

const char *Entity_KvGetValue( Entity_t *ent, const char *key )
{
	if ( ent )
	{
		for ( int i = 0; i < ent->kvCount; i++ )
		{
			KeyValue_t *kv = ent->keyValues[i];
			if ( !kv )
				continue;

			if ( StrEq( kv->key, key ) )
				return kv->value;
		}
	}

	return "";
}

const char *Entity_KvGetString( Entity_t *ent, const char *key )
{
	// Same as GetValue, but here for the sake of readability
	return Entity_KvGetValue( ent, key );
}

KeyValue_t *Entity_KvCreate( Entity_t *ent, const char *key, const char *value )
{
	if ( !ent )
		return NULL;

	KeyValue_t *kv = (KeyValue_t *)malloc( sizeof( KeyValue_t ) );
	strncpy( kv->key, key, MAX_KEY );
	strncpy( kv->value, value, MAX_VALUE );

	ent->kvCount++;
	ent->keyValues = (KeyValue_t **)realloc( ent->keyValues, ent->kvCount * sizeof( KeyValue_t* ) );
	ent->keyValues[ent->kvCount - 1] = kv;
	return kv;
}

void Entity_KvSetValue( Entity_t *ent, const char *key, const char *value )
{
	if ( !ent )
		return;

	BOOL found = FALSE;

	for ( int i = 0; i < ent->kvCount; i++ )
	{
		KeyValue_t *kv = ent->keyValues[i];
		if ( !kv )
			continue;

		if ( StrEq( kv->key, key ) )
		{
			strncpy( kv->value, value, MAX_VALUE );
			found = TRUE;
		}
	}

	if ( !found )
	{
		// not found, create new
		Entity_KvCreate( ent, key, value );
	}
}

void Entity_KvSetString( Entity_t *ent, const char *key, const char *value )
{
	// Same as SetValue, but here for the sake of readability
	Entity_KvSetValue( ent, key, value );
}

int GetLineFromString( const char *in, char *out, int outSize )
{
	strncpy( out, in, outSize );

	int len = strlen( in ) + 1;
	for ( int i = 0; i < len; i++ )
	{
		if ( in[i] == '\n' )
		{
			out[i] = '\0';
			break;
		}
	}

	int retLen = strlen( out ) - 1;
	return retLen;
}

bool MapEdit_ExtractEnts(Map_t *pMap, const char *pszEntBuffer)
{
	DevMsg( "Extracting entities...\n" );
	
	// extract ents from the ASCII buffer
	char line[256];
	int len = strlen( pszEntBuffer );
	int cur = 0;
	BOOL endBracketFound = FALSE;

	Entity_t **ents = NULL;
	Entity_t *curEnt = NULL;
	int entCount = 0;

	int kvCount = 0;

	while ( cur < len )
	{
		// get line
		int lineLen = GetLineFromString( &pszEntBuffer[cur], line, sizeof( line ) );

		if ( line[0] == '\0' || line[0] == '\n' )
		{
			cur++;
			continue;
		}

		// do stuff with the line
		if ( line[0] == '{' )
		{
			if ( curEnt )
				free( curEnt );
			
			// new entity
			curEnt = new Entity_t;
			curEnt->keyValues = NULL;

			endBracketFound = FALSE;
		}
		else if ( line[0] == '}' )
		{
			// put entity in list
			if ( curEnt )
			{
				entCount++;
				ents = (Entity_t**)realloc( ents, entCount * sizeof( Entity_t* ) );
				ents[entCount - 1] = curEnt;
				curEnt = NULL;
				kvCount = 0;
			}

			endBracketFound = TRUE;
		}
		else if ( !endBracketFound && curEnt )
		{
			// get keyvalue
			KeyValue_t *kv = new KeyValue_t;
			int set = 0;
			int start = -1;
			int end = -1;

			// loop through line to find quotes
			for ( int i = 0; i < lineLen + 1; i++ )
			{
				if ( line[i] == '"' )
				{
					if ( start == -1 ) // first '"'
					{
						start = i + 1;
					}
					else // second '"'
					{
						end = i;

						char *ptr;
						if ( set == 0 )
						{
							// key
							ptr = kv->key;
							set = 1;
						}
						else
						{
							// value
							ptr = kv->value;
							set = 2;
						}

						// slice string
						int len = strlen( line );
						int loc = 0;
						for ( int j = start; j < len; j++ )
						{
							if ( j == end )
							{
								ptr[loc] = '\0';
								break;
							}

							ptr[loc] = line[j];
							loc++;
						}

						ptr[loc] = '\0';

						if ( set == 2 )
						{
							// finished
							break;
						}
						else if ( set == 1 )
						{
							// key set
							start = -1;
							end = -1;
						}
					}
				}
			}

			// add kv to list
			kvCount++;
			curEnt->keyValues = (KeyValue_t**)realloc( curEnt->keyValues, kvCount * sizeof( KeyValue_t* ) );
			curEnt->keyValues[kvCount - 1] = kv;
			curEnt->kvCount = kvCount;
		}

		// into next
		cur += lineLen + 1;
	}

	pMap->entities.CopyArray(ents, entCount);

	DevMsg( "%d entities extracted\n", entCount );
	return TRUE;
}

BOOL MapEdit_BuildEntBuffer(Map_t *pMap)
{
	DevMsg( "Building entity buffer...\n" );

	if (pMap->entitiesBuffer)
		free(pMap->entitiesBuffer);

	int totalSize = 0;
	pMap->entitiesBuffer = NULL;

	int cur = -1;
	for ( int i = 0; i < pMap->entities.Count(); i++ )
	{
		Entity_t *ent = pMap->entities[i];
		if ( !ent )
			continue;

		// start bracket
		totalSize += 2;
		pMap->entitiesBuffer = (char *)realloc( pMap->entitiesBuffer, totalSize );
		pMap->entitiesBuffer[++cur] = '{';
		pMap->entitiesBuffer[++cur] = '\n';

		for ( int j = 0; j < ent->kvCount; j++ )
		{
			KeyValue_t *kv = ent->keyValues[j];
			if ( !kv )
				continue;

			char line[256];
			sprintf_s( line, sizeof( line ), "\"%s\" \"%s\"", kv->key, kv->value );

			totalSize += strlen( line ) + 1;
			pMap->entitiesBuffer = (char *)realloc( pMap->entitiesBuffer, totalSize );

			// append
			char *dest = &pMap->entitiesBuffer[++cur];
			const char *src = line;
			int len = strlen( src );

			while ( len-- )
			{
				if ( cur >= totalSize - 1 )
					break;

				*dest++ = *src++;
				cur++;
			}

			pMap->entitiesBuffer[cur] = '\n';
		}

		// end bracket
		totalSize += 2;
		pMap->entitiesBuffer = (char *)realloc( pMap->entitiesBuffer, totalSize );
		pMap->entitiesBuffer[++cur] = '}';
		pMap->entitiesBuffer[++cur] = '\n';
	}

	pMap->entitiesBufferSize = totalSize;

	return TRUE;
}
#pragma endregion

enum
{
	COMMAND_DELETE = 0,
	COMMAND_EDIT
};

typedef struct edtCommand_s
{
	int	iCommand;
	KeyValues* pkvFilter;
	KeyValues* pkvData;
} edtCommand_t;

void MapEdit_RunCommandList(Map_t* pMap, CUtlVector<edtCommand_t>* pCommandList)
{
	CUtlVector<Entity_t*> deletionList;

	for (int i = 0; i < pMap->entities.Count(); i++)
	{
		Entity_t* pEnt = pMap->entities[i];
		for (int j = 0; j < pCommandList->Count(); j++)
		{
			const edtCommand_t& cmd = pCommandList->Element(j);
			const char* pchTestKey = cmd.pkvFilter->GetName();
			const char* pchTestValue = cmd.pkvFilter->GetString();

			const char* pchValue = Entity_KvGetString(pEnt, pchTestKey);
			if (StrEq(pchTestValue, pchValue))
			{
				switch (cmd.iCommand)
				{
				case COMMAND_DELETE:
					deletionList.AddToTail(pEnt);
					break;
				case COMMAND_EDIT:
				{
					for (KeyValues* pkvValue = cmd.pkvData->GetFirstValue(); pkvValue != NULL; pkvValue = pkvValue->GetNextValue())
					{
						Entity_KvSetString(pEnt, pkvValue->GetName(), pkvValue->GetString());
					}
				}
				break;
				default:
					break;
				}
			}
		}
	}

	for (int i = 0; i < deletionList.Count(); i++)
	{
		pMap->entities.FindAndRemove(deletionList[i]);
		free(deletionList[i]->keyValues);
		delete deletionList[i];
	}
}

const char* CMapEditHelper::DoMapEdit(const char* pMapName, const char* pMapEntities)
{
	if (m_strLastMap.IsEqual_CaseInsensitive(pMapName))
		return m_bufEntities.String();

	m_bufEntities.Clear();
	m_strLastMap.Set(pMapName);

	CFmtStr edtPath("maps/%s.edt", pMapName);
	if (g_pFullFileSystem->FileExists(edtPath, "GAME"))
	{
		CUtlBuffer fileBuf;
		g_pFullFileSystem->ReadFile(edtPath, "GAME", fileBuf);

		CRC32_t calcCRC;
		CRC32_Init(&calcCRC);
		CRC32_ProcessBuffer(&calcCRC, pMapEntities, V_strlen(pMapEntities));
		CRC32_ProcessBuffer(&calcCRC, fileBuf.Base(), fileBuf.TellPut());
		CRC32_Final(&calcCRC);

		CFmtStr cachePath("maps/edt_cache/%s.ent", pMapName);
		if (g_pFullFileSystem->FileExists(cachePath, "GAME"))
		{
			FileHandle_t h = g_pFullFileSystem->Open(cachePath, "rb", "GAME");
			CRC32_t testCRC;
			g_pFullFileSystem->Read(&testCRC, sizeof(CRC32_t), h);
			if (calcCRC == testCRC)
			{
				int iRemainingSize = g_pFullFileSystem->Size(h) - g_pFullFileSystem->Tell(h);
				m_bufEntities.SetBufferType(false, false);
				m_bufEntities.EnsureCapacity(iRemainingSize);
				g_pFullFileSystem->Read(m_bufEntities.Base(), iRemainingSize, h);
				m_bufEntities.SetBufferType(true, false);
				g_pFullFileSystem->Close(h);

				return m_bufEntities.String();
			}
			
			g_pFullFileSystem->Close(h);
		}

		
		Map_t map;
		MapEdit_ExtractEnts(&map, pMapEntities);

		KeyValuesAD KV(pMapName);
		if (KV->LoadFromFile(g_pFullFileSystem, edtPath, "GAME"))
		{
			CUtlVector<edtCommand_t> vecCommands;
			CUtlBuffer creationBuf(0, 0, CUtlBuffer::TEXT_BUFFER);
			for (KeyValues* pkvSub = KV->GetFirstTrueSubKey(); pkvSub != NULL; pkvSub = pkvSub->GetNextTrueSubKey())
			{
				const char* pchName = pkvSub->GetName();
				if (StrEq(pchName, "delete"))
				{
					edtCommand_t cmd;
					cmd.iCommand = COMMAND_DELETE;
					cmd.pkvFilter = pkvSub->GetFirstSubKey();
					cmd.pkvData = nullptr;
					vecCommands.AddToTail(cmd);
				}
				else if (StrEq(pchName, "edit"))
				{
					edtCommand_t cmd;
					cmd.iCommand = COMMAND_EDIT;
					cmd.pkvFilter = pkvSub->GetFirstSubKey();
					cmd.pkvData = pkvSub->FindKey("values");
					vecCommands.AddToTail(cmd);
				}
				else if (StrEq(pchName, "create"))
				{
					creationBuf.PutChar('{');
					creationBuf.PutChar('\n');
					for (KeyValues* pkvValue = pkvSub->GetFirstValue(); pkvValue != NULL; pkvValue = pkvValue->GetNextValue())
					{
						creationBuf.PutChar('"');
						creationBuf.PutString(pkvValue->GetName());
						creationBuf.PutChar('"');
						creationBuf.PutChar(' ');
						creationBuf.PutChar('"');
						creationBuf.PutString(pkvValue->GetString());
						creationBuf.PutChar('"');
						creationBuf.PutChar('\n');
					}
					creationBuf.PutChar('}');
					creationBuf.PutChar('\n');
				}
			}

			MapEdit_RunCommandList(&map, &vecCommands);
			MapEdit_BuildEntBuffer(&map);

			m_bufEntities.Put(map.entitiesBuffer, map.entitiesBufferSize);
			m_bufEntities.Put(creationBuf.Base(), creationBuf.TellPut());

			free(map.entitiesBuffer);
			map.entitiesBuffer = nullptr;

			g_pFullFileSystem->CreateDirHierarchy("maps/edt_cache", "DEFAULT_WRITE_PATH");
			FileHandle_t h = g_pFullFileSystem->Open(cachePath, "wb", "DEFAULT_WRITE_PATH");
			g_pFullFileSystem->Write(&calcCRC, sizeof(CRC32_t), h);
			g_pFullFileSystem->Write(m_bufEntities.Base(), m_bufEntities.TellMaxPut(), h);
			g_pFullFileSystem->Close(h);

			return m_bufEntities.String();
		}
	}

	m_bufEntities.CopyBuffer(pMapEntities, V_strlen(pMapEntities));
	return m_bufEntities.String();
}
