/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


// ui_qmenu.c -- Quake menu framework


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "keydefs.h"
#include "BtnsBMPTable.h"

#if defined _WIN32
	#undef GetParent
	#include <winlite.h>
#elif defined __APPLE__
	#include <sys/time.h>
#elif defined __DOS__
	// nothing
#else
	#include <time.h>
#endif

#ifdef _DEBUG
void DBG_AssertFunction( bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage )
{
	if( fExpr ) return;

	char szOut[512];
	if( szMessage != NULL )
		sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage );
	else sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine );
	Host_Error( szOut );
}
#endif	// DEBUG

int colorstrcmp( const char *s1, const char *s2 )
{
	int	c1, c2;

	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}

	do {
		if( IsColorString( s1 ))
		{
			s1 += 2;
		}
		if( IsColorString( s2 ))
		{
			s2 += 2;
		}

		c1 = *s1++;
		c2 = *s2++;

		if( c1 != c2 ) return c1 < c2 ? -1 : 1;

	} while( c1 );

	// strings are equal
	return 0;
}

int colorstricmp( const char *s1, const char *s2 )
{
	int	c1, c2;

	if( s1 == NULL )
	{
		if( s2 == NULL )
			return 0;
		else return -1;
	}
	else if( s2 == NULL )
	{
		return 1;
	}

	do {
		if( IsColorString( s1 ))
		{
			s1 += 2;
		}
		if( IsColorString( s2 ))
		{
			s2 += 2;
		}

		c1 = *s1++;
		c2 = *s2++;

		if( c1 != c2 )
		{
			if( c1 >= 'a' && c1 <= 'z' ) c1 -= ('a' - 'A');
			if( c2 >= 'a' && c2 <= 'z' ) c2 -= ('a' - 'A');
			if( c1 != c2 ) return c1 < c2 ? -1 : 1;
		}
	} while( c1 );

	// strings are equal
	return 0;
}

int ColorStrlen( const char *str )
{
	const char *p;

	if( !str )
		return 0;

	int len = 0;
	p = str;
	EngFuncs::UtfProcessChar( 0 );
	while( *p )
	{
		if( IsColorString( p ))
		{
			p += 2;
			continue;
		}

		p++;
		if( EngFuncs::UtfProcessChar( (unsigned char) *p ) )
			len++;
	}

	EngFuncs::UtfProcessChar( 0 );

	return len;
}

char *StringCopy( const char *input )
{
	if( !input ) return NULL;

	char *out = new char[strlen( input ) + 1];
	strcpy( out, input );

	return out;
}

/*
============
COM_CompareSaves
============
*/
int COM_CompareSaves( const void **a, const void **b )
{
	char *file1, *file2;

	file1 = (char *)*a;
	file2 = (char *)*b;

	int bResult;

	EngFuncs::CompareFileTime( file2, file1, &bResult );

	return bResult;
}

/*
============
COM_FileBase

Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
a1ba: adapted and simplified version from QuakeSpasm
============
*/
#define COM_CheckString( string ) ( ( !string || !*string ) ? 0 : 1 )
void COM_FileBase( const char *in, char *out, size_t size )
{
	const char *dot, *slash, *s;
	size_t len;

	if( unlikely( !COM_CheckString( in ) || size <= 1 ))
	{
		out[0] = 0;
		return;
	}

	slash = in;
	dot = NULL;
	for( s = in; *s; s++ )
	{
		if( *s == '/' || *s == '\\' )
			slash = s + 1;

		if( *s == '.' )
			dot = s;
	}

	if( dot == NULL || dot < slash )
		dot = s;

	len = Q_min( size - 1, dot - slash );

	memcpy( out, slash, len );
	out[len] = 0;
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
#define MAX_KV_SIZE		128
const char *Info_ValueForKey( const char *s, const char *key )
{
	char	pkey[MAX_KV_SIZE];
	static	char value[4][MAX_KV_SIZE]; // use two buffers so compares work without stomping on each other
	static	int valueindex;
	int	count;
	char	*o;

	valueindex = (valueindex + 1) % 4;
	if( *s == '\\' ) s++;

	while( 1 )
	{
		count = 0;
		o = pkey;

		while( count < (MAX_KV_SIZE - 1) && *s != '\\' )
		{
			if( !*s ) return "";
			*o++ = *s++;
			count++;
		}

		*o = 0;
		s++;

		o = value[valueindex];
		count = 0;

		while( count < (MAX_KV_SIZE - 1) && *s && *s != '\\' )
		{
			if( !*s ) return "";
			*o++ = *s++;
			count++;
		}
		*o = 0;

		if( !strcmp( key, pkey ))
			return value[valueindex];
		if( !*s ) return "";
		s++;
	}
}


/*
===================
Key_GetKey
===================
*/
int KEY_GetKey( const char *binding )
{
	const char *b;

	if ( !binding )
		return -1;

	for ( int i = 0; i < 256; i++ )
	{
		b = EngFuncs::KEY_GetBinding( i );
		if( !b ) continue;

		if( !stricmp( binding, b ))
			return i;
	}
	return -1;
}

/*
================
UI_FadeAlpha
================
*/
int UI_FadeAlpha( int starttime, int endtime )
{
	int	time, fade_time;

	if( starttime == 0 )
	{
		return 0xFFFFFFFF;
	}

	time = ( gpGlobals->time * 1000 ) - starttime;

	if( time >= endtime )
	{
		return 0x00FFFFFF;
	}

	// fade time is 1/4 of endtime
	fade_time = endtime / 4;
	fade_time = bound( 300, fade_time, 10000 );

	int alpha;

	// fade out
	if(( endtime - time ) < fade_time )
		alpha = bound( 0, (( endtime - time ) * 1.0f / fade_time ) * 255, 255 );
	else alpha = 255;

	return PackRGBA( 255, 255, 255, alpha );
}

void UI_EnableTextInput( bool enable )
{
	uiStatic.textInput = enable;
	EngFuncs::EnableTextInput( enable );
}

CBMP* CBMP::LoadFile( const char *filename )
{
	int length = 0;
	bmp_t *bmp = (bmp_t*)EngFuncs::COM_LoadFile( filename, &length );

	// cannot load
	if( !bmp )
		return NULL;

	// too small for BMP
	if( (size_t)length < sizeof( bmp_t ))
		return NULL;

	// not a BMP
	if( bmp->id[0] != 'B' || bmp->id[1] != 'M' )
		return NULL;

	// bogus data
	if( !bmp->width || !bmp->height )
		return NULL;

	// validate all nasty data size fields
	if( length < bmp->fileSize ||
		 length < bmp->bitmapDataSize ||
		 length < bmp->bitmapDataOffset ||
		 length < bmp->bitmapHeaderSize ||
		 length < bmp->bitmapDataOffset + bmp->bitmapDataSize ||
		 length < bmp->bitmapHeaderSize + bmp->bitmapDataSize )
		return NULL;

	// will be freed in destructor
	CBMP *ret = new CBMP( bmp );

	return ret;
}

const int table_cp1251[64] = {
	0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
	0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
	0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x007F, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
	0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
	0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
	0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
	0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457
};


/*
============================
Con_UtfProcessChar

Ripped from engine.

Xash3D FWGS uses multibyte, converting it to current single-byte encoding
Converting to single-byte not necessary anymore, as UI uses custom font render which works with 32-bit chars
============================
*/
int Con_UtfProcessChar( int in )
{
	static int m = -1, k = 0; //multibyte state
	static int uc = 0; //unicode char

	if( !in )
	{
		m = -1;
		k = 0;
		uc = 0;
		return 0;
	}

	// Get character length
	if(m == -1)
	{
		uc = 0;
		if( in >= 0xF8 )
		{
			return 0;
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
		else if( in <= 0x7F )
		{
			return in; // ascii
		}
		// return 0 if we need more chars to decode one
		k = 0;
		return 0;
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
		return 0;
	}

	if( k == m )
	{
		k = m = -1;

		return uc;

		// not implemented yet
		// return '?';
	}
	return 0;
}

/*
=================
Con_UtfMoveLeft

get position of previous printful char
=================
*/
int Con_UtfMoveLeft( const char *str, int pos )
{
	int i, k = 0;
	// int j;
	Con_UtfProcessChar( 0 );
	if(pos == 1) return 0;
	for( i = 0; i < pos-1; i++ )
		if( Con_UtfProcessChar( (unsigned char)str[i] ) )
			k = i+1;
	Con_UtfProcessChar( 0 );
	return k;
}

/*
=================
Con_UtfMoveRight

get next of previous printful char
=================
*/
int Con_UtfMoveRight( const char *str, int pos, int length )
{
	int i;
	Con_UtfProcessChar( 0 );
	for( i = pos; i <= length; i++ )
	{
		if( Con_UtfProcessChar( (unsigned char)str[i] ) )
			return i+1;
	}
	Con_UtfProcessChar( 0 );

	return pos + 1;
}


bool UI::Names::CheckIsNameValid(const char *name)
{
	if( !name || !*name )
		return false;

	// exclude some default names, that may be set from engine or just come with config files
	static struct
	{
		const char *name;
		bool substring;
	} prohibitedNames[] =
	{
	{ "default", false, },
	{ "unnamed", false, },
	{ "Player", false, },
	{ "<Warrior> Player", false, },
	{ "Shinji", false, },
	{ "CSDuragiCOM", true },
	{ "Nero Claudius", true }, // *purrrt* you found a secret area!
	};

	for( size_t i = 0; i < V_ARRAYSIZE( prohibitedNames ); i++ )
	{
		if( prohibitedNames[i].substring )
		{
			if( strstr( name, prohibitedNames[i].name ) )
			{
				return false;
			}
		}
		else
		{
			if( !stricmp( name, prohibitedNames[i].name ) )
			{
				return false;
			}
		}
	}

	return true;
}

char *Q_pretifymem( float value, int digitsafterdecimal )
{
	static char	output[8][32];
	static int	current;
	const float onekb = 1024.0f;
	const float onemb = onekb * onekb;
	const char *suffix;
	char		*out = output[current];
	char		val[32], *i, *o, *dot;
	int		pos;

	current = ( current + 1 ) & ( 8 - 1 );

	// first figure out which bin to use
	if( value > onemb )
	{
		value /= onemb;
		suffix = "Mb";
	}
	else if( value > onekb )
	{
		value /= onekb;
		suffix = "Kb";
	}
	else
	{
		suffix = "bytes";
	}

	// if it's basically integral, don't do any decimals
	if( fabs( value - (int)value ) < 0.00001f || digitsafterdecimal <= 0 )
		snprintf( val, sizeof( val ), "%i %s", (int)round( value ), suffix );
	else
		snprintf( val, sizeof( val ), "%.*f %s", digitsafterdecimal, (double)value, suffix );

	// copy from in to out
	i = val;
	o = out;

	// search for decimal or if it was integral, find the space after the raw number
	dot = strchr( i, '.' );
	if( !dot ) dot = strchr( i, ' ' );

	pos = dot - i;	// compute position of dot
	pos -= 3;		// don't put a comma if it's <= 3 long

	while( *i )
	{
		// if pos is still valid then insert a comma every third digit, except if we would be
		// putting one in the first spot
		if( pos >= 0 && !( pos % 3 ))
		{
			// never in first spot
			if( o != out ) *o++ = ',';
		}

		pos--;		// count down comma position
		*o++ = *i++;	// copy rest of data as normal
	}
	*o = 0; // terminate

	return out;
}

void Com_EscapeCommand( char *newCommand, const char *oldCommand, int len )
{
	char c;
	int scripting = (int)EngFuncs::GetCvarFloat( "cmd_scripting" );

	while( (c = *oldCommand++) && len > 1 )
	{
		if( c == '"' )
		{
			*newCommand++ = '\\';
			len--;
		}

		if( scripting && c == '$')
		{
			*newCommand++ = '$';
			len--;
		}

		*newCommand++ = c;
		len--;
	}

	*newCommand++ = 0;
}
