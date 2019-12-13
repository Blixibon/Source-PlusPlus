//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "cdll_int.h"
#include "tmessage.h"
#include "characterset.h"
#include "cdll_int.h"
#include "UtlStringMap.h"
#include "utldict.h"
#include "utlbufferutil.h"
#include "filesystem.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MSGFILE_NAME		0
#define MSGFILE_TEXT		1
#define MAX_MESSAGES		600			// I don't know if this table will balloon like every other feature in Half-Life
										// But, for now, I've set this to a reasonable value
// Defined in other files.
static characterset_t	g_WhiteSpace;

client_textmessage_t	gMessageParms;

CUtlLinkedList<char *> gStringTableList;

//typedef CUtlDict<client_textmessage_t> messageDict_t;
//CUtlStringMap<client_textmessage_t> gMessageMap;
CUtlVectorConservative< client_textmessage_t > gMessageVector;
int giMessageBuildStart;

static void TextMessageParse( byte *pMemFile, int fileSize );

// The string "pText" is assumed to have all whitespace from both ends cut out
int IsComment( char *pText )
{
	if ( pText )
	{
		int length = strlen( pText );

		if ( length >= 2 && pText[0] == '/' && pText[1] == '/' )
			return 1;
		
		// No text?
		if ( length > 0 )
			return 0;
	}
	// No text is a comment too
	return 1;
}


// The string "pText" is assumed to have all whitespace from both ends cut out
int IsStartOfText( char *pText )
{
	if ( pText )
	{
		if ( pText[0] == '{' )
			return 1;
	}
	return 0;
}


// The string "pText" is assumed to have all whitespace from both ends cut out
int IsEndOfText( char *pText )
{
	if ( pText )
	{
		if ( pText[0] == '}' )
			return 1;
	}
	return 0;
}


#if 0
int IsWhiteSpace( char space )
{
	if ( space == ' ' || space == '\t' || space == '\r' || space == '\n' )
		return 1;
	return 0;
}
#else

#define IsWhiteSpace(space)		IN_CHARACTERSET( g_WhiteSpace, space )
#endif


const char *SkipSpace( const char *pText )
{
	if ( pText )
	{
		int pos = 0;
		while ( pText[pos] && IsWhiteSpace( pText[pos] ) )
			pos++;
		return pText + pos;
	}

	return NULL;
}


const char *SkipText( const char *pText )
{
	if ( pText )
	{
		int pos = 0;
		while ( pText[pos] && !IsWhiteSpace( pText[pos] ) )
			pos++;
		return pText + pos;
	}

	return NULL;
}


int ParseFloats( const char *pText, float *pFloat, int count )
{
	const char *pTemp = pText;
	int index = 0;

	while ( pTemp && count > 0 )
	{
		// Skip current token / float
		pTemp = SkipText( pTemp );
		// Skip any whitespace in between
		pTemp = SkipSpace( pTemp );
		if ( pTemp )
		{
			// Parse a float
			pFloat[index] = (float)atof( pTemp );
			count--;
			index++;
		}
	}

	if ( count == 0 )
		return 1;

	return 0;
}

int ParseString( char const *pText, char *buf, size_t bufsize )
{
	const char *pTemp = pText;

	// Skip current token / float
	pTemp = SkipText( pTemp );
	// Skip any whitespace in between
	pTemp = SkipSpace( pTemp );
	
	if ( pTemp )
	{
		char const *pStart = pTemp;
		pTemp = SkipText( pTemp );

		int len =  min( pTemp - pStart + 1, (int)bufsize - 1 );
		Q_strncpy( buf, pStart, len );
		buf[ len ] = 0;
		return 1;
	}

	return 0;
}


// Trims all whitespace from the front and end of a string
void TrimSpace( const char *source, char *dest )
{
	int start, end, length;

	start = 0;
	end = strlen( source );

	while ( source[start] && IsWhiteSpace( source[start] ) )
		start++;

	end--;
	while ( end > 0 && IsWhiteSpace( source[end] ) )
		end--;

	end++;

	length = end - start;
	if ( length > 0 )
		memcpy( dest, source + start, length );
	else
		length = 0;

	// Terminate the dest string
	dest[ length ] = 0;
}


int IsToken( const char *pText, const char *pTokenName )
{
	if ( !pText || !pTokenName )
		return 0;

	if ( !Q_strnicmp( pText+1, pTokenName, strlen(pTokenName) ) )
		return 1;
	
	return 0;
}

static char g_pchSkipName[ 64 ];

int ParseDirective( const char *pText )
{
	if ( pText && pText[0] == '$' )
	{
		float tempFloat[8];

		if ( IsToken( pText, "position" ) )
		{
			if ( ParseFloats( pText, tempFloat, 2 ) )
			{
				gMessageParms.x = tempFloat[0];
				gMessageParms.y = tempFloat[1];
			}
		}
		else if ( IsToken( pText, "effect" ) )
		{
			if ( ParseFloats( pText, tempFloat, 1 ) )
			{
				gMessageParms.effect = (int)tempFloat[0];
			}
		}
		else if ( IsToken( pText, "fxtime" ) )
		{
			if ( ParseFloats( pText, tempFloat, 1 ) )
			{
				gMessageParms.fxtime = tempFloat[0];
			}
		}
		else if ( IsToken( pText, "color2" ) )
		{
			if ( ParseFloats( pText, tempFloat, 3 ) )
			{
				gMessageParms.r2 = (int)tempFloat[0];
				gMessageParms.g2 = (int)tempFloat[1];
				gMessageParms.b2 = (int)tempFloat[2];
			}
		}
		else if ( IsToken( pText, "color" ) )
		{
			if ( ParseFloats( pText, tempFloat, 3 ) )
			{
				gMessageParms.r1 = (int)tempFloat[0];
				gMessageParms.g1 = (int)tempFloat[1];
				gMessageParms.b1 = (int)tempFloat[2];
			}
		}
		else if ( IsToken( pText, "fadein" ) )
		{
			if ( ParseFloats( pText, tempFloat, 1 ) )
			{
				gMessageParms.fadein = tempFloat[0];
			}
		}
		else if ( IsToken( pText, "fadeout" ) )
		{
			if ( ParseFloats( pText, tempFloat, 3 ) )
			{
				gMessageParms.fadeout = tempFloat[0];
			}
		}
		else if ( IsToken( pText, "holdtime" ) )
		{
			if ( ParseFloats( pText, tempFloat, 3 ) )
			{
				gMessageParms.holdtime = tempFloat[0];
			}
		}
		else if ( IsToken( pText, "boxsize" ) )
		{
			if ( ParseFloats( pText, tempFloat, 1 ) )
			{
				gMessageParms.bRoundedRectBackdropBox = tempFloat[0] != 0.0f;
				gMessageParms.flBoxSize = tempFloat[0];
			}
		}
		else if ( IsToken( pText, "boxcolor" ) )
		{
			if ( ParseFloats( pText, tempFloat, 4 ) )
			{
				for ( int i = 0; i < 4; ++i )
				{
					gMessageParms.boxcolor[ i ] = (byte)(int)tempFloat[ i ];
				}
			}
		}
		else if ( IsToken( pText, "clearmessage" ) )
		{
			if ( ParseString( pText, g_pchSkipName, sizeof( g_pchSkipName ) ) )
			{
				if ( !g_pchSkipName[ 0 ] || !Q_stricmp( g_pchSkipName, "0" ) )
				{
					gMessageParms.pClearMessage = NULL;
				}
				else
				{
					gMessageParms.pClearMessage = g_pchSkipName;
				}
			}
		}
		else
		{
			ConDMsg("Unknown token: %s\n", pText );
		}

		return 1;
	}
	return 0;
}

#define NAME_HEAP_SIZE 16384

bool GetLineFromBuffer(CUtlBuffer *pBuf, int *pFilePos, char *pchTextBuf, int iMaxChars)
{
	if (pBuf->GetBytesRemaining() <= 0)
		return false;

	pBuf->GetLine(pchTextBuf, iMaxChars);

	if (pFilePos)
		*pFilePos = pBuf->TellGet();

	return true;
}

char *memfgets(unsigned char *pMemFile, int fileSize, int *pFilePos, char *pBuffer, int bufferSize)
{
	int filePos = *pFilePos;
	int i, last, stop;

	// Bullet-proofing
	if (!pMemFile || !pBuffer)
		return NULL;

	if (filePos >= fileSize)
		return NULL;

	i = filePos;
	last = fileSize;

	// fgets always NULL terminates, so only read bufferSize-1 characters
	if (last - filePos > (bufferSize - 1))
		last = filePos + (bufferSize - 1);

	stop = 0;

	// Stop at the next newline (inclusive) or end of buffer
	while (i < last && !stop)
	{
		if (pMemFile[i] == '\n')
			stop = 1;
		i++;
	}


	// If we actually advanced the pointer, copy it over
	if (i != filePos)
	{
		// We read in size bytes
		int size = i - filePos;
		// copy it out
		memcpy(pBuffer, pMemFile + filePos, sizeof(unsigned char)*size);

		// If the buffer isn't full, terminate (this is always true)
		if (size < bufferSize)
			pBuffer[size] = 0;

		// Update file pointer
		*pFilePos = i;
		return pBuffer;
	}

	// No data read, bail
	return NULL;
}

void TextMessageParse( byte *pMemFile, int fileSize )
{
	char		buf[512], trim[512];
	char		*pCurrentText=0, *pNameHeap;
	char		 currentName[512], nameHeap[ NAME_HEAP_SIZE ];
	int			lastNamePos;

	int			mode = MSGFILE_NAME;	// Searching for a message name	
	int			lineNumber, filePos, lastLinePos;
	int			messageCount;

	client_textmessage_t	textMessages[ MAX_MESSAGES ];
	
	int			i, nameHeapSize, textHeapSize, nameOffset;

	lastNamePos = 0;
	lineNumber = 0;
	filePos = 0;
	lastLinePos = 0;
	messageCount = 0;

	bool bSpew = CommandLine()->FindParm( "-textmessagedebug" ) ? true : false;

	CharacterSetBuild( &g_WhiteSpace, " \r\n\t" );

	/*int iFlags = (CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER);

	CUtlBuffer memBuffer(pMemFile, fileSize, iFlags);*/


	while ( memfgets(pMemFile, fileSize, &filePos, buf, 512) )
	{

		TrimSpace( buf, trim );
		switch( mode )
		{
		case MSGFILE_NAME:
			if ( IsComment( trim ) )	// Skip comment lines
				break;
			
			if ( ParseDirective( trim ) )	// Is this a directive "$command"?, if so parse it and break
				break;

			if ( IsStartOfText( trim ) )
			{
				mode = MSGFILE_TEXT;
				pCurrentText = (char*)(pMemFile + filePos);
				break;
			}
			if ( IsEndOfText( trim ) )
			{
				ConDMsg("Unexpected '}' found, line %d\n", lineNumber );
				return;
			}
			Q_strncpy( currentName, trim, sizeof( currentName ) );
			break;
		
		case MSGFILE_TEXT:
			if ( IsEndOfText( trim ) )
			{
				int length = strlen(currentName);

				// Save name on name heap
				if ( lastNamePos + length > 8192 )
				{
					ConDMsg("Error parsing file!\n" );
					return;
				}
				Q_strcpy( nameHeap + lastNamePos, currentName );

				// Terminate text in-place in the memory file (it's temporary memory that will be deleted)
				// If the string starts with #, it's a localization string and we don't
				// want the \n on the end or the Find() lookup will fail (so subtract 2)
				/*if ( pCurrentText && pCurrentText[0] && pCurrentText[0] == '#' && lastLinePos > 1 ) 
				{
					pMemFile[ lastLinePos - 2 ] = 0;
				}
				else*/
				{
					pMemFile[ lastLinePos - 1 ] = 0;
				}

				// Save name/text on heap
				textMessages[ messageCount ] = gMessageParms;
				textMessages[ messageCount ].pName = nameHeap + lastNamePos;
				lastNamePos += strlen(currentName) + 1;
				if ( gMessageParms.pClearMessage )
				{
					Q_strncpy( nameHeap + lastNamePos, textMessages[ messageCount ].pClearMessage, Q_strlen(textMessages[ messageCount ].pClearMessage ) + 1 );
					textMessages[ messageCount ].pClearMessage = nameHeap + lastNamePos;
					lastNamePos += Q_strlen(textMessages[ messageCount ].pClearMessage ) + 1;
				}
				textMessages[ messageCount ].pMessage = pCurrentText;

				if ( bSpew )
				{
					client_textmessage_t *m = &textMessages[ messageCount ];
					Msg( "%d %s\n",
						messageCount, m->pName ? m->pName : "(null)" );
					Msg( "  effect %d, color1(%d,%d,%d,%d), color2(%d,%d,%d,%d)\n",
						m->effect, m->r1, m->g1, m->b1, m->a1, m->r2, m->g2, m->b2, m->a2 );
					Msg( "  pos %f,%f, fadein %f fadeout %f hold %f fxtime %f\n",
						m->x, m->y, m->fadein, m->fadeout, m->holdtime, m->fxtime );
					Msg( "  '%s'\n", m->pMessage ? m->pMessage : "(null)" );

					Msg( "  box %s, size %f, color(%d,%d,%d,%d)\n",
						m->bRoundedRectBackdropBox ? "yes" : "no", m->flBoxSize, m->boxcolor[ 0 ], m->boxcolor[ 1 ], m->boxcolor[ 2 ], m->boxcolor[ 3 ] );

					if ( m->pClearMessage )
					{
						Msg( "  will clear '%s'\n", m->pClearMessage );
					}
				}

				messageCount++;

				// Reset parser to search for names
				mode = MSGFILE_NAME;
				break;
			}
			if ( IsStartOfText( trim ) )
			{
				ConDMsg("Unexpected '{' found, line %d\n", lineNumber );
				return;
			}
			break;
		}
		lineNumber++;
		lastLinePos = filePos;

		if ( messageCount >= MAX_MESSAGES )
		{
			ConMsg("WARNING: TOO MANY MESSAGES IN TITLES.TXT, MAX IS %d\n", MAX_MESSAGES );
			break;
		}
	}

	ConDMsg("Parsed %d text messages\n", messageCount );
	nameHeapSize = lastNamePos;
	textHeapSize = 0;
	for ( i = 0; i < messageCount; i++ )
		textHeapSize += strlen(textMessages[i].pMessage ) + 1;


	// Must malloc because we need to be able to clear it after initialization
	void *pMessageTable = malloc( textHeapSize + nameHeapSize );
	
	// Copy Name heap
	pNameHeap = ((char *)pMessageTable);
	memcpy( pNameHeap, nameHeap, nameHeapSize );
	nameOffset = pNameHeap - nameHeap;

	// Copy text & fixup pointers
	pCurrentText = pNameHeap + nameHeapSize;

	for ( i = 0; i < messageCount; i++ )
	{
		textMessages[i].pName += nameOffset;		// Adjust name pointer (parallel buffer)
		if (textMessages[i].pClearMessage )
		{
			textMessages[i].pClearMessage += nameOffset;
		}
		Q_strcpy( pCurrentText, textMessages[i].pMessage );	// Copy text over
		textMessages[i].pMessage = pCurrentText;
		pCurrentText += strlen( pCurrentText ) + 1;
	}

#if _DEBUG
	if ( (pCurrentText - (char *)pMessageTable) != (textHeapSize + nameHeapSize) )
		ConMsg("Overflow text message buffer!!!!!\n");
#endif
	gStringTableList.AddToTail((char *)pMessageTable);
	gMessageVector.AddMultipleToTail(messageCount, textMessages);
	giMessageBuildStart =+ messageCount;
}

void TextMessageShutdown( void )
{
	gMessageVector.Purge();
	for (int i = 0; i < gStringTableList.Count(); i++)
	{
		void *pMem = gStringTableList.Element(i);
		free(pMem);
	}
	gStringTableList.Purge();
	giMessageBuildStart = 0;
}

void TextMessageInit( void )
{
	

	// Clear out any old data that's sitting around.
	TextMessageShutdown();

	//pMemFile = COM_LoadFile( "scripts/titles.txt", 5, &fileSize );

	char searchPaths[4096];
	filesystem->GetSearchPath("GAME", true, searchPaths, sizeof(searchPaths));

	for (char *path = strtok(searchPaths, ";"); path; path = strtok(NULL, ";"))
	{
		//int fileSize = 0;
		//byte *pMemFile = nullptr;

		char fullpath[MAX_PATH];
		V_ComposeFileName(path, "scripts/titles.txt", fullpath, MAX_PATH);

		FileHandle_t hFile = filesystem->Open(fullpath, "r");
		if (hFile)
		{
			//fileSize = filesystem->Size(hFile);
			CUtlBuffer buf;

			if (filesystem->ReadToBuffer(hFile, buf))
			{
				buf.PutChar(0);
				TextMessageParse((byte *)buf.Base(), buf.TellMaxPut());
			}

			filesystem->Close(hFile);
		}
	}
}

client_textmessage_t *TextMessageGet( const char *pName )
{
#if 1
	if (!Q_stricmp(pName, DEMO_MESSAGE) || !Q_stricmp(pName, NETWORK_MESSAGE1) || !Q_stricmp(pName, NETWORK_MESSAGE2) ||
		!Q_stricmp(pName, NETWORK_MESSAGE3) || !Q_stricmp(pName, NETWORK_MESSAGE4) || !Q_stricmp(pName, NETWORK_MESSAGE5) ||
		!Q_stricmp(pName, NETWORK_MESSAGE6))
	{
		return engine->TextMessageGet(pName);
	}
#else
	{
		client_textmessage_t *pEngRet = engine->TextMessageGet(pName);
		if (pEngRet)
			return pEngRet;
	}
#endif

	for ( int i = 0; i < gMessageVector.Count(); i++ )
	{
		if ( !Q_stricmp( pName, gMessageVector[i].pName ) )
			return &gMessageVector[i];
	}

	return NULL;
}