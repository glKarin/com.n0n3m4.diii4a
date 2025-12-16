/*
MenuStrings.cpp - custom menu strings
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <ctype.h>
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "MenuStrings.h"
#include "utlhashmap.h"
#include "generichash.h"
#include "utflib.h"

#define EMPTY_STRINGS_1 ""
#define EMPTY_STRINGS_2 EMPTY_STRINGS_1, EMPTY_STRINGS_1
#define EMPTY_STRINGS_5 EMPTY_STRINGS_2, EMPTY_STRINGS_2, EMPTY_STRINGS_1
#define EMPTY_STRINGS_10 EMPTY_STRINGS_5, EMPTY_STRINGS_5
#define EMPTY_STRINGS_20 EMPTY_STRINGS_10, EMPTY_STRINGS_10
#define EMPTY_STRINGS_50 EMPTY_STRINGS_20, EMPTY_STRINGS_20, EMPTY_STRINGS_10
#define EMPTY_STRINGS_100 EMPTY_STRINGS_50, EMPTY_STRINGS_50

static CUtlHashMap<const char *, const char *> hashed_cmds;

const char *MenuStrings[IDS_LAST] =
{
EMPTY_STRINGS_100, // 0..99
EMPTY_STRINGS_50, // 100..149
EMPTY_STRINGS_20, // 150..169
EMPTY_STRINGS_10, // 170..179
EMPTY_STRINGS_5, // 180..184
EMPTY_STRINGS_2, // 185..186
EMPTY_STRINGS_1, // 187
"Return to game.", // 188
"Start a new game.", // 189
EMPTY_STRINGS_1,	// 190
"Load a previously saved game.", // 191
"Load a saved game, save the current game.", // 192
"Change game settings, configure controls", // 193
EMPTY_STRINGS_20, // 194..213
EMPTY_STRINGS_20, // 214..233
"Starting a Hazard Course will exit\nany current game, OK to exit?", // 234
EMPTY_STRINGS_5, // 235..239
"Starting a new game will exit\nany current game, OK to exit?",	// 240
EMPTY_STRINGS_5, // 241..245
EMPTY_STRINGS_2, // 246..247
EMPTY_STRINGS_2, // 248..249
EMPTY_STRINGS_100, // 250..349
EMPTY_STRINGS_50, // 350..399
"Find more about Valve's product lineup",	// 400
EMPTY_STRINGS_1, // 401
"http://store.steampowered.com/app/70/", // 402
EMPTY_STRINGS_5, // 403..407
EMPTY_STRINGS_2, // 408..409
EMPTY_STRINGS_100, // 410..509
EMPTY_STRINGS_20, // 510..529
"Select a custom game",	// 530
EMPTY_STRINGS_5, // 531..535
EMPTY_STRINGS_2, // 536..537
EMPTY_STRINGS_2, // 538..539
EMPTY_STRINGS_50, // 540..589
EMPTY_STRINGS_10, // 590..599
};

const char *L( const char *szStr ) // L means Localize!
{
	if( szStr )
	{
		if( *szStr == '#' )
			szStr++;

		int i = hashed_cmds.Find( szStr );

		if( i != hashed_cmds.InvalidIndex() )
			return hashed_cmds[i];
	}

	return szStr;
}

static void Dictionary_Insert( const char *key, const char *value )
{
	int i = hashed_cmds.Find( key );

	// don't allow dupes, delete older strings
	if( i != hashed_cmds.InvalidIndex() )
	{
		const char *old = hashed_cmds[i];

		hashed_cmds[i] = StringCopy( value );

		delete[] old;
	}
	else
	{
		const char *first = StringCopy( key );
		const char *second = StringCopy( value );

		hashed_cmds.Insert( first, second );
	}
}

static void UI_InitAliasStrings( void )
{
	const struct
	{
		const char *defAliasString;
		int idx;
	} aliasStrings[] =
	{
	{ "Quit %s without\nsaving current game?", IDS_MAIN_QUITPROMPTINGAME },
	{ "Learn how to play %s", IDS_MAIN_TRAININGHELP },
	{ "Play %s on the 'easy' skill setting", IDS_NEWGAME_EASYHELP },
	{ "Play %s on the 'medium' skill setting", IDS_NEWGAME_MEDIUMHELP },
	{ "Play %s on the 'difficult' skill setting", IDS_NEWGAME_DIFFICULTHELP },
	{ "Quit playing %s", IDS_MAIN_QUITHELP },
	{ "Search for %s servers, configure character", IDS_MAIN_MULTIPLAYERHELP },
	};

	for( int i = 0; i < V_ARRAYSIZE( aliasStrings ); i++ )
	{
		if( MenuStrings[aliasStrings[i].idx][0]) // check if not initialized by strings.lst
			continue;

		char token[64];
		snprintf( token, sizeof( token ), "StringsList_%d", aliasStrings[i].idx );

		const char *fmt_str = L( token );
		if( fmt_str == token ) // not found
			fmt_str = aliasStrings[i].defAliasString;

		CUtlString fmt( fmt_str );
		fmt.Replace( "%s", gMenu.m_gameinfo.title );
		MenuStrings[aliasStrings[i].idx] = fmt.DetachRawPtr();

		Dictionary_Insert( token, MenuStrings[aliasStrings[i].idx] );
	}
}

int UTFToCP1251( char *out, const char *instr, int len, int maxoutlen )
{
	int m = -1, k = 0, uc = 0;
	const char *inbegin = instr;
	char *outbegin = out;

	while( *instr && (out - outbegin) < maxoutlen )
	{
		int in = *instr & 255;
		if( instr - inbegin >= len )
		{
			*out = 0;
			return out - outbegin;
		}

		// Get character length
		if(m == -1)
		{
			uc = 0;
			if( in >= 0xF8 )
			{
				instr++;
				continue;
			}
			else if( in >= 0xF0 )
			{
				uc = in & 0x07;
				m = 3;
			}
			else if( in >= 0xE0 )
			{
				uc = in & 0x0F;
				m = 2;
			}
			else if( in >= 0xC0 )
			{
				uc = in & 0x1F;
				m = 1;
			}
			else if( in <= 0x7F)
			{
				if( in == '\\' && *(instr + 1) != '\\'  )
				{
					instr++;
					continue;
				}
				*out++ = *instr++;
				continue;
			}
			// we need more chars to decode one
			k=0;
			instr++;
			continue;
		}
		// get more chars
		else if( k <= m )
		{
			uc <<= 6;
			uc += in & 0x3F;
			k++;
		}
		if( in > 0xBF || m < 0 )
		{
			m = -1;
			instr++;
			continue;
		}


		if( k == m )
		{
			k = m = -1;
			// cp1252 hackish translate
			if( uc > 0x7F && uc <= 0xFF )
			{
				*out++ = uc;
				instr++;
				continue;
			}

			// cp1251
			if( uc >= 0x0410 && uc <= 0x042F ) // Cyrillic Capital Letters
			{
				*out++ = uc - 0x410 + 0xC0;
				instr++;
				continue;
			}
			else if( uc >= 0x0430 && uc <= 0x044F ) // Cyrillic Small Letters
			{
				*out++ = uc - 0x430 + 0xE0;
				instr++;
				continue;
			}
			else
			{
				int i;
				for( i = 0; i < 64; i++ )
				{
					if( table_cp1251[i] == uc )
					{
						*out++ = i + 0x80;
						instr++;
						break;
					}
				}
				continue;
			}

			*out++ = *instr++;
			continue;
		}
		instr++;
	}
	*out = 0;
	return out - outbegin;
}

static uint Localize_ProcessString( char *dst, const char *src )
{
	const char *p;
	uint i = 0;

	p = src;

	while( *p )
	{
		if( *p == '\\' )
		{
			char replace = 0;

			switch( p[1] )
			{
			case '\\': replace = '\\'; break;
			case 'n': replace = '\n'; break;
			}

			if( replace )
			{
				if( dst )
					dst[i] = replace;
				i++;
				p += 2;
				continue;
			}
		}

		if( dst )
			dst[i] = *p;
		i++;
		p++;
	}

	// null terminator
	if( dst )
		dst[i] = '\0';
	i++;

	return i;
}

static void ByteSwapUTF16File( uint16_t *head, size_t len )
{
	for( size_t i = 0; i < len; i++ )
		head[i] = Swap16( head[i] );
}

static void Localize_AddToDictionary( const char *name, const char *lang )
{
	char filename[64], token[4096];
	char *pfile, *afile = nullptr, *pFileBuf;
	int i = 0, buflen, charlen;
	bool isUtf16 = false;

	snprintf( filename, sizeof( filename ), "resource/%s_%s.txt", name, lang );

	pFileBuf = reinterpret_cast<char*>( EngFuncs::COM_LoadFile( filename, &buflen ));

	if( !pFileBuf )
	{
		Con_Printf( "Localize_AddToDict( %s ): couldn't open file. Some strings will not be localized!.\n", filename );
		return;
	}

	if( pFileBuf[0] == '\xFF' && pFileBuf[1] == '\xFE' )
	{
		if( buflen > 3 && !pFileBuf[2] && !pFileBuf[3] )
		{
			Con_Printf( "Localize_AddToDict( %s ): couldn't parse file. UTF-32 little endian isn't supported\n", filename );
			goto error;
		}

		charlen = buflen / 2 - 1;
		isUtf16 = true;

#if XASH_BIG_ENDIAN
		ByteSwapUTF16File( (uint16_t *)pFileBuf, charlen );
#endif
	}
	else if( pFileBuf[0] == '\xFE' && pFileBuf[1] == '\xFF' )
	{
		if( buflen > 3 && !pFileBuf[2] && !pFileBuf[3] )
		{
			Con_Printf( "Localize_AddToDict( %s ): couldn't parse file. UTF-32 big endian isn't supported\n", filename );
			goto error;
		}

		charlen = buflen / 2 - 1;
		isUtf16 = true;

#if XASH_LITTLE_ENDIAN
		ByteSwapUTF16File( (uint16_t *)pFileBuf, charlen );
#endif
	}

	if( isUtf16 )
	{
		size_t utf8len = Q_UTF16ToUTF8( NULL, 0, (const uint16_t *)&pFileBuf[2], charlen );

		// Con_Printf( "size in utf16: %zu (%zu), size in utf8: %zu\n", buflen, charlen, utf8len );

		afile = new char[utf8len + 1]; // save original pointer, so we can free it later

		Q_UTF16ToUTF8( afile, utf8len + 1, (const uint16_t *)&pFileBuf[2], charlen );
	}
	else
	{
		afile = pFileBuf;

		// strip UTF-8 BOM
		if( afile[0] == '\xEF' && afile[1] == '\xBB' && afile[2] == '\xBF')
			afile += 3;
	}

	pfile = afile;

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));

	if( stricmp( token, "lang" ))
	{
		Con_Printf( "Localize_AddToDict( %s ): invalid header, got %s", filename, token );
		goto error;
	}

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );

	if( strcmp( token, "{" ))
	{
		Con_Printf( "Localize_AddToDict( %s ): want {, got %s", filename, token );
		goto error;
	}

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );

	if( stricmp( token, "Language" ))
	{
		Con_Printf( "Localize_AddToDict( %s ): want Language, got %s", filename, token );
		goto error;
	}

	// skip language actual name
	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );

	if( stricmp( token, "Tokens" ))
	{
		Con_Printf( "Localize_AddToDict( %s ): want Tokens, got %s", filename, token );
		goto error;
	}

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );

	if( strcmp( token, "{" ))
	{
		Con_Printf( "Localize_AddToDict( %s ): want { after Tokens, got %s", filename, token );
		goto error;
	}

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))))
	{
		if( !strcmp( token, "}" ))
			break;

		char szLocString[4096];
		pfile = EngFuncs::COM_ParseFile( pfile, szLocString, sizeof( szLocString ));

		if( !strcmp( szLocString, "}" ))
			break;

		if( pfile )
		{
			// Con_DPrintf("New token: %s %s\n", token, szLocString );
			Localize_ProcessString( token, token );
			Localize_ProcessString( szLocString, szLocString );
			Dictionary_Insert( token, szLocString );
			i++;
		}
	}

error:
	if( isUtf16 && afile )
		delete[] afile;

	EngFuncs::COM_FreeFile( pFileBuf );
}

static void Localize_InitLanguage( const char *language )
{
	const char *gamedir = gMenu.m_gameinfo.gamefolder;

	// if gamedir isn't gameui, then load standard gameui strings
	if( strcmp( gamedir, "gameui" ))
		Localize_AddToDictionary( "gameui", language );

	// if gamedir isn't valve, then load standard HL1 strings
	if( strcmp( gamedir, "valve" ))
		Localize_AddToDictionary( "valve",  language );

	// if gamedir isn't mainui, then load standard mainui strings
	if( strcmp( gamedir, "mainui" ))
		Localize_AddToDictionary( "mainui", language );

	// mod strings override default ones
	Localize_AddToDictionary( gamedir,  language );
}

static void Localize_Init( void )
{
	EngFuncs::ClientCmd( true, "exec mainui.cfg\n" );

	hashed_cmds.Purge();

	// strings.lst first
	for( int i = 0; i < IDS_LAST; i++ )
	{
		if( !MenuStrings[i][0] )
			continue;

		char buf[256];

		snprintf( buf, sizeof( buf ), "StringsList_%i", i );

		Dictionary_Insert( buf, MenuStrings[i] );
	}

	// always load default language translation
	Localize_InitLanguage( "english" );

	const char *language = EngFuncs::GetCvarString( "ui_language" );

	if( language[0] && strcmp( language, "english" ))
		Localize_InitLanguage( language );

	// strings.lst compatible aliasstrings then
	UI_InitAliasStrings ();
}

static void Localize_Free( void )
{
	FOR_EACH_HASHMAP( hashed_cmds, i )
	{
		const char *first = hashed_cmds.Key( i );
		const char *second = hashed_cmds.Element( i );

		delete[] (char*)first;
		delete[] (char*)second;
	}

	hashed_cmds.Purge();
}

void UI_LoadCustomStrings( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/strings.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int string_num;

	if( !afile )
		goto localize_init;

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) != NULL )
	{
		if( isdigit( token[0] ))
		{
			string_num = atoi( token );

			// check for bad stiringnum
			if( string_num < 0 ) continue;
			if( string_num > ( IDS_LAST - 1 ))
				continue;
		}
		else continue; // invalid declaration ?

		// parse new string
		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));
		MenuStrings[string_num] = StringCopy( token ); // replace default string with custom
	}

	EngFuncs::COM_FreeFile( afile );

localize_init:
	Localize_Init();
}

void UI_FreeCustomStrings( void )
{
	Localize_Free();
}
