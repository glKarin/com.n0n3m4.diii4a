/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "port.h"

#include <string.h>
#include "wrect.h" // need for cl_dll.h
#include "cl_dll.h"
#include "vgui_parser.h"
#include "unicode_strtools.h"

#include "errno.h"
#include <ctype.h>

#include "interface.h"

// evil pasta hacks
#define uint64 uint64_bruh
#define int64 int64_bruh
#include "miniutl.h"
#include "utlhashmap.h"
#undef uint64
#undef int64

static CUtlHashMap<const char *, const char *> hashed_cmds;

char *StringCopy( const char *input )
{
	if( !input ) return NULL;

	char *out = new char[strlen( input ) + 1];
	strcpy( out, input );

	return out;
}

const char *Localize( const char *szStr )
{
	if( szStr )
	{
		char *str = strdup( szStr );
		char *p = str;

		StripEndNewlineFromString( str );

		if( *p == '#' )
			p++;

		int i = hashed_cmds.Find( p );

		free( str );

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

static void Localize_AddToDictionary( const char *name, const char *lang )
{
	char filename[64], token[4096];
	char *pfile, *afile = nullptr, *pFileBuf;
	int i = 0, len;
	bool isUtf16 = false;

	snprintf( filename, sizeof( filename ), "resource/%s_%s.txt", name, lang );

	pFileBuf = reinterpret_cast<char*>( gEngfuncs.COM_LoadFile( filename, 5, &len ));

	if( !pFileBuf )
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): couldn't open file. Some strings will not be localized!.\n", filename );
		return;
	}

	// support only utf-16le
	if( pFileBuf[0] == '\xFF' && pFileBuf[1] == '\xFE' )
	{
		if( len > 3 && !pFileBuf[2] && !pFileBuf[3] )
		{
			gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): couldn't parse file. UTF-32 little endian isn't supported\n", filename );
			goto error;
		}
		isUtf16 = true;
	}
	else if( pFileBuf[0] == '\xFE' && pFileBuf[1] == '\xFF' )
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): couldn't parse file. UTF-16/UTF-32 big endian isn't supported\n", filename );
		goto error;
	}

	if( isUtf16 )
	{
		int ansiLength = len + 1;
		uchar16 *autf16 = new uchar16[len/2 + 1];

		memcpy( autf16, pFileBuf + 2, len - 1 );
		autf16[len/2-1] = 0; //null terminator

		afile = new char[ansiLength]; // save original pointer, so we can free it later

		Q_UTF16ToUTF8( autf16, afile, ansiLength, STRINGCONVERT_ASSERT_REPLACE );

		delete[] autf16;
	}
	else
	{
		afile = pFileBuf;

		// strip UTF-8 BOM
		if( afile[0] == '\xEF' && afile[1] == '\xBB' && afile[2] == '\xBF')
			afile += 3;
	}

	pfile = afile;

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	if( stricmp( token, "lang" ))
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): invalid header, got %s", filename, token );
		goto error;
	}

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	if( strcmp( token, "{" ))
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): want {, got %s", filename, token );
		goto error;
	}

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	if( stricmp( token, "Language" ))
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): want Language, got %s", filename, token );
		goto error;
	}

	// skip language actual name
	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	if( stricmp( token, "Tokens" ))
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): want Tokens, got %s", filename, token );
		goto error;
	}

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	if( strcmp( token, "{" ))
	{
		gEngfuncs.Con_Printf( "Localize_AddToDict( %s ): want { after Tokens, got %s", filename, token );
		goto error;
	}

	while( (pfile = gEngfuncs.COM_ParseFile( pfile, token )))
	{
		if( !strcmp( token, "}" ))
			break;

		char szLocString[4096];
		pfile = gEngfuncs.COM_ParseFile( pfile, szLocString );

		if( !strcmp( szLocString, "}" ))
			break;

		if( pfile )
		{
			// Con_DPrintf("New token: %s %s\n", token, szLocString );
			Dictionary_Insert( token, szLocString );
			i++;
		}
	}

error:
	if( isUtf16 && afile )
		delete[] afile;

	gEngfuncs.COM_FreeFile( pFileBuf );
}

static void Localize_InitLanguage( const char *language )
{
	const char *gamedir = gEngfuncs.pfnGetGameDirectory();

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

void Localize_Init( void )
{
	gEngfuncs.pfnClientCmd( "exec mainui.cfg\n" );

	hashed_cmds.Purge();

	// always load default language translation
	Localize_InitLanguage( "english" );

	const char *language = gEngfuncs.pfnGetCvarString( "ui_language" );

	if( language[0] && strcmp( language, "english" ))
		Localize_InitLanguage( language );
}

void Localize_Free( void )
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

void Localize_StripIndices( char *s )
{
	if ( strlen( s ) >= 3 )
	{
		for ( size_t i = 0; i < strlen( s ) - 2; i++ )
		{
			if ( s[i] == '%' && s[i + 1] == 's' && isdigit( s[i + 2] ) )
			{
				char *first = &s[i + 2];
				char *second = &s[i + 3];

				size_t len = strlen( second );

				memmove( first, second, strlen( second ) );
				first[len] = '\0'; // one character has been removed and string moved, set null terminator
			}
		}
	}
}