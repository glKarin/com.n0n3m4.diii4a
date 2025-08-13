/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

#ifdef GAMEDLL
#include "../game/g_local.h"
#endif

// ^[0-9a-zA-Z]
qboolean Q_IsColorString(const char *p) {
	if (!p)
		return qfalse;

	if (p[0] != Q_COLOR_ESCAPE)
		return qfalse;

	if (p[1] == 0)
		return qfalse;

	// isalnum expects a signed integer in the range -1 (EOF) to 255, or it might assert on undefined behaviour
	// a dereferenced char pointer has the range -128 to 127, so we just need to rangecheck the negative part
	if (p[1] < 0)
		return qfalse;

	if (isalnum(p[1]) == 0)
		return qfalse;

	return qtrue;
}

/*
============
Com_Clamp
============
*/
float Com_Clamp( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath( char *pathname ) {
	char    *last;

	last = pathname;
	while ( *pathname )
	{
		if ( *pathname == '/' ) {
			last = pathname + 1;
		}
		pathname++;
	}
	return last;
}

/*
============
COM_GetExtension
============
*/
const char *COM_GetExtension( const char *name )
{
	const char *dot = strrchr(name, '.'), *slash;
	if (dot && (!(slash = strrchr(name, '/')) || slash < dot))
		return dot + 1;
	else
		return "";
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out, int destsize )
{
	const char *dot = strrchr(in, '.'), *slash;

	if (dot && (!(slash = strrchr(in, '/')) || slash < dot))
		destsize = (destsize < dot-in+1 ? destsize : dot-in+1);

	if ( in == out && destsize > 1 )
		out[destsize-1] = '\0';
	else
		Q_strncpyz(out, in, destsize);
}

/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
qboolean COM_CompareExtension(const char *in, const char *ext)
{
	int inlen, extlen;
	
	inlen = strlen(in);
	extlen = strlen(ext);
	
	if(extlen <= inlen)
	{
		in += inlen - extlen;
		
		if(!Q_stricmp(in, ext))
			return qtrue;
	}
	
	return qfalse;
}

/*
==================
COM_DefaultExtension

if path doesn't have an extension, then append
 the specified one (which should include the .)
==================
*/
void COM_DefaultExtension( char *path, int maxSize, const char *extension )
{
	const char *dot = strrchr(path, '.'), *slash;
	if (dot && (!(slash = strrchr(path, '/')) || slash < dot))
		return;
	else
		Q_strcat(path, maxSize, extension);
}

/*
============
COM_StripFilename
============
*/
void COM_StripFilename( char *in, char *out ) {
	char *end;
	Q_strncpyz( out, in, strlen( in ) );
	end = COM_SkipPath( out );
	*end = 0;
}

//============================================================================
/*
==================
COM_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
qboolean COM_BitCheck( const int array[], int bitNum ) {
	int i;

	i = 0;
	while ( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	return ( ( array[i] & ( 1 << bitNum ) ) != 0 );  // (SA) heh, whoops. :)
}

/*
==================
COM_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitSet( int array[], int bitNum ) {
	int i;

	i = 0;
	while ( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	array[i] |= ( 1 << bitNum );
}

/*
==================
COM_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitClear( int array[], int bitNum ) {
	int i;

	i = 0;
	while ( bitNum > 31 ) {
		i++;
		bitNum -= 32;
	}

	array[i] &= ~( 1 << bitNum );
}
//============================================================================

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
/*
// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
static short ( *_BigShort )( short l );
static short ( *_LittleShort )( short l );
static int ( *_BigLong )( int l );
static int ( *_LittleLong )( int l );
static qint64 ( *_BigLong64 )( qint64 l );
static qint64 ( *_LittleLong64 )( qint64 l );
static float ( *_BigFloat )( float l );
static float ( *_LittleFloat )( float l );

short   BigShort( short l ) {return _BigShort( l );}
short   LittleShort( short l ) {return _LittleShort( l );}
int     BigLong( int l ) {return _BigLong( l );}
int     LittleLong( int l ) {return _LittleLong( l );}
qint64  BigLong64( qint64 l ) {return _BigLong64( l );}
qint64  LittleLong64( qint64 l ) {return _LittleLong64( l );}
float   BigFloat( float l ) {return _BigFloat( l );}
float   LittleFloat( float l ) {return _LittleFloat( l );}
*/

void CopyShortSwap(void *dest, void *src)
{
	byte *to = dest, *from = src;

	to[0] = from[1];
	to[1] = from[0];
}

void CopyLongSwap(void *dest, void *src)
{
	byte *to = dest, *from = src;

	to[0] = from[3];
	to[1] = from[2];
	to[2] = from[1];
	to[3] = from[0];
}

short   ShortSwap( short l ) {
	byte b1,b2;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

short   ShortNoSwap( short l ) {
	return l;
}

int    LongSwap( int l ) {
	byte b1,b2,b3,b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( (int)b1 << 24 ) + ( (int)b2 << 16 ) + ( (int)b3 << 8 ) + b4;
}

int LongNoSwap( int l ) {
	return l;
}

qint64 Long64Swap( qint64 ll ) {
	qint64 result;

	result.b0 = ll.b7;
	result.b1 = ll.b6;
	result.b2 = ll.b5;
	result.b3 = ll.b4;
	result.b4 = ll.b3;
	result.b5 = ll.b2;
	result.b6 = ll.b1;
	result.b7 = ll.b0;

	return result;
}

qint64 Long64NoSwap( qint64 ll ) {
	return ll;
}

float FloatSwap (const float *f) {
	floatint_t out;

	out.f = *f;
	out.ui = LongSwap(out.ui);

	return out.f;
}

float FloatNoSwap (const float *f)
{
	return *f;
}


/*
============================================================================

PARSING

============================================================================
*/

static char com_token[MAX_TOKEN_CHARS];
static char com_parsename[MAX_TOKEN_CHARS];
static int com_lines;
static int com_tokenline;

static int backup_lines;
static char    *backup_text;

/*
================
COM_BeginParseSession
================
*/
void COM_BeginParseSession( const char *name ) {
	com_lines = 1;
	com_tokenline = 0;
	Com_sprintf( com_parsename, sizeof( com_parsename ), "%s", name );
}

/*
================
COM_RestoreParseSession
================
*/
void COM_RestoreParseSession( char **data_p ) {
	com_lines = backup_lines;
	*data_p = backup_text;
}

/*
================
COM_SetCurrentParseLine
================
*/
void COM_SetCurrentParseLine( int line ) {
	com_lines = line;
}

/*
================
COM_GetCurrentParseLine
================
*/
int COM_GetCurrentParseLine( void ) {
	if ( com_tokenline )
	{
		return com_tokenline;
	}

	return com_lines;
}

/*
================
COM_Parse
================
*/
char *COM_Parse( char **data_p ) {
	return COM_ParseExt( data_p, qtrue );
}

/*
================
COM_ParseError
================
*/
void COM_ParseError( char *format, ... ) {
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	Q_vsnprintf (string, sizeof(string), format, argptr);
	va_end( argptr );

	Com_Printf("ERROR: %s, line %d: %s\n", com_parsename, COM_GetCurrentParseLine(), string);
}

/*
================
COM_ParseWarning
================
*/
void COM_ParseWarning( char *format, ... ) {
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof ( string ), format, argptr );
	va_end( argptr );

	Com_Printf("WARNING: %s, line %d: %s\n", com_parsename, COM_GetCurrentParseLine(), string);
}

/*
==============
SkipWhitespace

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
char *SkipWhitespace( char *data, qboolean *hasNewLines ) {
	int c;

	while ( ( c = *data ) <= ' ' ) {
		if ( c == '\n' ) {
			com_lines++;
			*hasNewLines = qtrue;
		} else if ( !c )   {
			return NULL;
		}
		data++;
	}

	return data;
}

/*
==============
SkipWhitespace2

Parse a token out of a string
Will never return NULL, just empty strings
==============
*/
char *SkipWhitespace2( char *data ) {
	int c;

	while ( ( c = *data ) <= ' ' ) {
		if ( c == '\n' ) {
			com_lines++;
			return data;
		} else if ( c == '\t' ) {
			return data;
		} else if ( !c ) {
			return NULL;
		}
		data++;
	}

	return data;
}

int COM_Compress( char *data_p ) {
	char *in, *out;
	int c;
	qboolean newline = qfalse, whitespace = qfalse;

	in = out = data_p;
	if (in) {
		while ((c = *in) != 0) {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' ) {
				while (*in && *in != '\n') {
					in++;
				}
			// skip /* */ comments
			} else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) ) 
					in++;
				if ( *in ) 
					in += 2;
				// record when we hit a newline
			} else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
				// record when we hit whitespace
			} else if ( c == ' ' || c == '\t') {
				whitespace = qtrue;
				in++;
				// an actual token
			} else {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline) {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				} if (whitespace) {
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"') {
					*out++ = c;
					in++;
					while (1) {
						c = *in;
						if (c && c != '"') {
							*out++ = c;
							in++;
						} else {
							break;
						}
					}
					if (c == '"') {
						*out++ = c;
						in++;
					}
				} else {
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}

#ifdef GAMEDLL
/*
================
COM_Eval
================
*/
qboolean COM_Eval( char *cvarname, char *condition, char *cvarvalue ) {
	qboolean copy = qtrue;

	int cvar = trap_Cvar_VariableIntegerValue( cvarname );

	int value = atoi( cvarvalue );

//        Com_Printf("%d ---- %d\n", cvar, value);

	// based on the condition, set copy to false or true
	if ( !Q_strcasecmp( condition, "==" ) ) {
		if ( cvar == value ) {
			copy = qtrue;
		} else {
			copy = qfalse;
		}
	} else if ( !Q_strcasecmp( condition, "!=" ) ) {
		if ( cvar != value ) {
			copy = qtrue;
		} else {
			copy = qfalse;
		}
	} else if ( !Q_strcasecmp( condition, "<=" ) ) {
		if ( cvar <= value ) {
			copy = qtrue;
		} else {
			copy = qfalse;
		}
	} else if ( !Q_strcasecmp( condition, ">=" ) ) {
		if ( cvar >= value ) {
			copy = qtrue;
		} else {
			copy = qfalse;
		}
	} else if ( !Q_strcasecmp( condition, "<" ) ) {
		if ( cvar < value ) {
			copy = qtrue;
		} else {
			copy = qfalse;
		}
	} else if ( !Q_strcasecmp( condition, ">" ) ) {
		if ( cvar > value ) {
			copy = qtrue;
		} else {
			copy = qfalse;
		}
	} else {
		Com_Error( ERR_DROP, "COM_Eval() Error (line %d): Unknown condition, must be ==, !=, <=,  >=, < or >.\n", COM_GetCurrentParseLine() );
	}

	return copy;
}
#endif

/*
================
COM_ParseExt
================
*/
char *COM_ParseExt( char **data_p, qboolean allowLineBreaks ) {
	int c = 0, len;
	qboolean hasNewLines = qfalse;
	char *data;
#ifdef GAMEDLL
	qboolean ignore = qfalse;     // used for #if statements
#endif

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	com_tokenline = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	// RF, backup the session data so we can unget easily
	backup_lines = com_lines;
	backup_text = *data_p;

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );
		if ( !data ) {
			*data_p = NULL;
			return com_token;
		}
		if ( hasNewLines && !allowLineBreaks ) {
			*data_p = data;
			return com_token;
		}

		c = *data;

#ifdef GAMEDLL
		// #if cvar == value #else #endif
		if ( c == '#' && data[1] == 'i' && data[2] == 'f' ) {
			char cvarname[256];
			char value[256];
			char condition[256];
			int i = 0;

			data += 3;
			// skip the witespace
			while ( *data && *data <= ' ' )
				data++;
			// get the cvar name
			while ( *data && *data > ' ' ) {
				cvarname[i++] = *data;
				data++;
			}
			cvarname[i] = '\0';
			//Com_Printf("->%s\n", cvarname);
			i = 0;

			// skip the witespace
			while ( *data && *data <= ' ' )
				data++;

			// get the condition
			while ( *data && *data > ' ' ) {
				condition[i++] = *data;
				data++;
			}
			condition[i] = '\0';
			//Com_Printf("->%s\n", condition);
			i = 0;

			// skip the witespace
			while ( *data && *data <= ' ' )
				data++;

			// get the value
			while ( *data && *data > ' ' ) {
				value[i++] = *data;
				data++;
			}
			value[i] = '\0';
			//Com_Printf("%d %d %d\n", strlen(cvarname), strlen(condition), strlen(value));
			//Com_Printf("--> #if %s %s %s\n", cvarname, condition, value);
			//Com_Printf("->%s\n", value);

			// now evaluate these
			ignore = !COM_Eval( cvarname, condition, value );
			//ignore = qtrue;
			continue;
		} else if ( c == '#' && data[1] == 'e' && data[2] == 'n' && data[3] == 'd' && data[4] == 'i' && data[5] == 'f' )           {
			data += 6;
			//Com_Printf("--> #endif\n");
			ignore = qfalse;
			continue;
		} else if ( c == '#' && data[1] == 'e' && data[2] == 'l' && data[3] == 's' && data[4] == 'e' )           {
			data += 5;
			//Com_Printf("--> #else\n");
			ignore = !ignore;
			continue;
		}

		// ignore #if / # else section
		if ( ignore ) {
			//Com_Printf("%c", c);
			data++;
			continue;
		}
#endif

		// skip double slash comments
		if ( c == '/' && data[1] == '/' ) {
			data += 2;
			while ( *data && *data != '\n' ) {
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[1] == '*' ) {
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				if ( *data == '\n' )
				{
					com_lines++;
				}
				data++;
			}
			if ( *data ) {
				data += 2;
			}
		} else
		{
			break;
		}
	}

	// token starts on this line
	com_tokenline = com_lines;

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c ) {
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if ( c == '\n' )
			{
				com_lines++;
			}
			if ( len < MAX_TOKEN_CHARS - 1 ) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS - 1 ) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
================
COM_Parse2
================
*/
char *COM_Parse2( char **data_p ) {
	int c = 0, len;
	char *data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	// RF, backup the session data so we can unget easily
	backup_lines = com_lines;
	backup_text = *data_p;

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace2( data );
		if ( !data ) {
			*data_p = NULL;
			return com_token;
		}

		c = *data;
		if ( c == '/' && data[1] == '/' ) {
		} else if ( c == '/' && data[1] == '*' )   {
		} else {
			break;
		}
	}

	// handle quoted strings
	if ( c == '\"' ) {
		data++;
		while ( 1 )
		{
			c = *data++;
			if ( c == '\"' || !c ) {
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if ( len < MAX_TOKEN_CHARS ) {
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS ) {
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if ( c == '\n' ) {
			com_lines++;
		}
	} while ( c > ' ' );

	if ( len == MAX_TOKEN_CHARS ) {
//              Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = '\0';

	*data_p = ( char * ) data;
	return com_token;
}

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( char **buf_p, char *match ) {
	char    *token;

	token = COM_Parse( buf_p );
	if ( strcmp( token, match ) ) {
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}

/*
=================
SkipBracedSection

The next token should be an open brace or set depth to 1 if already parsed it.
Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
qboolean SkipBracedSection (char **program, int depth) {
	char			*token;

	do {
		token = COM_ParseExt( program, qtrue );
		if( token[1] == 0 ) {
			if( token[0] == '{' ) {
				depth++;
			}
			else if( token[0] == '}' ) {
				depth--;
			}
		}
	} while( depth && *program );

	return ( depth == 0 );
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine( char **data ) {
	char    *p;
	int c;

	p = *data;

	if ( !*p )
		return;

	while ( ( c = *p++ ) != 0 ) {
		if ( c == '\n' ) {
			com_lines++;
			break;
		}
	}

	*data = p;
}

/*
================
Parse1DMatrix
================
*/
void Parse1DMatrix( char **buf_p, int x, float *m ) {
	char    *token;
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0 ; i < x ; i++ ) {
		token = COM_Parse( buf_p );
		m[i] = atof( token );
	}

	COM_MatchToken( buf_p, ")" );
}

/*
================
Parse2DMatrix
================
*/
void Parse2DMatrix( char **buf_p, int y, int x, float *m ) {
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0 ; i < y ; i++ ) {
		Parse1DMatrix( buf_p, x, m + i * x );
	}

	COM_MatchToken( buf_p, ")" );
}

/*
================
Parse3DMatrix
================
*/
void Parse3DMatrix( char **buf_p, int z, int y, int x, float *m ) {
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0 ; i < z ; i++ ) {
		Parse2DMatrix( buf_p, y, x, m + i * x * y );
	}

	COM_MatchToken( buf_p, ")" );
}

/*
===================
Com_HexStrToInt
===================
*/
int Com_HexStrToInt( const char *str )
{
	if ( !str )
		return -1;

	// check for hex code
	if( str[ 0 ] == '0' && str[ 1 ] == 'x' && str[ 2 ] != '\0' )
	{
		int i, n = 0, len = strlen( str );

		for( i = 2; i < len; i++ )
		{
			char digit;

			n *= 16;

			digit = tolower( str[ i ] );

			if( digit >= '0' && digit <= '9' )
				digit -= '0';
			else if( digit >= 'a' && digit <= 'f' )
				digit = digit - 'a' + 10;
			else
				return -1;

			n += digit;
		}

		return n;
	}

	return -1;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c ) {
	if ( c >= 0x20 && c <= 0x7E ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_islower( int c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isupper( int c ) {
	if ( c >= 'A' && c <= 'Z' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isalpha( int c ) {
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ) {
		return ( 1 );
	}
	return ( 0 );
}

//----(SA)	added
int Q_isnumeric( int c ) {
	if ( c >= '0' && c <= '9' ) {
		return ( 1 );
	}
	return ( 0 );
}

int Q_isalphanumeric( int c ) {
	if ( Q_isalpha( c ) ||
		 Q_isnumeric( c ) ) {
		return( 1 );
	}
	return ( 0 );
}

int Q_isforfilename( int c ) {
	if ( ( Q_isalphanumeric( c ) || c == '_' ) && c != ' ' ) { // space not allowed in filename
		return( 1 );
	}
	return ( 0 );
}
//----(SA)	end

qboolean Q_isanumber( const char *s )
{
	char *p;
	double UNUSED_VAR d;

	if( *s == '\0' )
		return qfalse;

	d = strtod( s, &p );

	return *p == '\0';
}

qboolean Q_isintegral( float f )
{
	return (int)f == f;
}

#ifdef _WIN32
/*
=============
Q_vsnprintf

Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own vsnprintf() which is not broken. mingw-w64
however, uses Microsoft's broken _vsnprintf() function.
=============
*/

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int retval;
	
	retval = _vsnprintf(str, size, format, ap);

	if(retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.
		
		str[size - 1] = '\0';
		return size;
	}
	
	return retval;
}
#endif

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz( char *dest, const char *src, int destsize ) {
  if ( !dest ) {
    Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
  }
	if ( !src ) {
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}
	if ( destsize < 1 ) {
		Com_Error(ERR_FATAL,"Q_strncpyz: destsize < 1" ); 
	}

	strncpy( dest, src, destsize-1 );

	dest[destsize-1] = 0;
}
                 
int Q_stricmpn (const char *s1, const char *s2, int n) {
	int		c1, c2;

        if ( s1 == NULL ) {
           if ( s2 == NULL )
             return 0;
           else
             return -1;
        }
        else if ( s2==NULL )
          return 1;


	
	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}
		
		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z') {
				c1 -= ('a' - 'A');
			}
			if (c2 >= 'a' && c2 <= 'z') {
				c2 -= ('a' - 'A');
			}
			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);
	
	return 0;		// strings are equal
}

int Q_strncmp( const char *s1, const char *s2, int n ) {
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point
		}

		if ( c1 != c2 ) {
			return c1 < c2 ? -1 : 1;
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_stricmp( const char *s1, const char *s2 ) {
	return ( s1 && s2 ) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}


char *Q_strlwr( char *s1 ) {
	char    *s;

	s = s1;
	while ( *s ) {
		*s = tolower( *s );
		s++;
	}
	return s1;
}

char *Q_strupr( char *s1 ) {
	char    *s;

	s = s1;
	while ( *s ) {
		*s = toupper( *s );
		s++;
	}
	return s1;
}

int Q_strnicmp( const char *string1, const char *string2, int n ) {
	int c1, c2;

	if ( string1 == NULL ) {
		if ( string2 == NULL ) {
			return 0;
		} else {
			return -1;
		}
	} else if ( string2 == NULL )     {
		return 1;
	}

	do {
		c1 = *string1++;
		c2 = *string2++;

		if ( !n-- ) {
			return 0; // Strings are equal until end point

		}
		if ( c1 != c2 ) {
			if ( c1 >= 'a' && c1 <= 'z' ) {
				c1 -= ( 'a' - 'A' );
			}
			if ( c2 >= 'a' && c2 <= 'z' ) {
				c2 -= ( 'a' - 'A' );
			}

			if ( c1 != c2 ) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while ( c1 );

	return 0; // Strings are equal
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src ) {
	int l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}
	Q_strncpyz( dest + l1, src, size - l1 );
}

/*
* Find the first occurrence of find in s.
*/
const char *Q_stristr( const char *s, const char *find ) {
	char c, sc;
	size_t len;

	if ( ( c = *find++ ) != 0 ) {
		if ( c >= 'a' && c <= 'z' ) {
			c -= ( 'a' - 'A' );
		}
		len = strlen( find );
		do
		{
			do
			{
				if ( ( sc = *s++ ) == 0 ) {
					return NULL;
				}
				if ( sc >= 'a' && sc <= 'z' ) {
					sc -= ( 'a' - 'A' );
				}
			} while ( sc != c );
		} while ( Q_stricmpn( s, find, len ) != 0 );
		s--;
	}
	return s;
}

int Q_PrintStrlen( const char *string ) {
	int len;
	const char  *p;

	if ( !string ) {
		return 0;
	}

	len = 0;
	p = string;
	while ( *p ) {
		if ( Q_IsColorString( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char *Q_CleanStr( char *string ) {
	char*   d;
	char*   s;
	int c;

	s = string;
	d = string;
	while ( ( c = *s ) != 0 ) {
		if ( Q_IsColorString( s ) ) {
			s++;
		} else if ( c >= 0x20 && c <= 0x7E )   {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

int Q_CountChar(const char *string, char tocount)
{
	int count;
	
	for(count = 0; *string; string++)
	{
		if(*string == tocount)
			count++;
	}
	
	return count;
}

int QDECL Com_sprintf(char *dest, int size, const char *fmt, ...)
{
	int len;
	va_list argptr;

	va_start( argptr,fmt );
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end( argptr );

	if(len >= size)
		Com_Printf("Com_sprintf: Output length %d too short, requires %d bytes.\n", size, len + 1);
	
	return len;
}

// Ridah, ripped from l_bsp.c
int Q_strncasecmp( char *s1, char *s2, int n ) {
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;       // strings are equal until end point

		}
		if ( c1 != c2 ) {
			if ( Q_islower( c1 ) ) {
				c1 -= ( 'a' - 'A' );
			}
			if ( Q_islower( c2 ) ) {
				c2 -= ( 'a' - 'A' );
			}
			if ( c1 != c2 ) {
				return -1;      // strings not equal
			}
		}
	} while ( c1 );

	return 0;       // strings are equal
}

int Q_strcasecmp( char *s1, char *s2 ) {
	return Q_strncasecmp( s1, s2, 99999 );
}
// done.

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.

Ridah, modified this into a circular list, to further prevent stepping on
previous strings
============
*/
char    * QDECL va( char *format, ... ) {
	va_list argptr;
	#define MAX_VA_STRING   32000
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING];      // in case va is called by nested functions
	static int index = 0;
	char    *buf;
	int len;


	va_start( argptr, format );
	Q_vsnprintf (temp_buffer, sizeof(temp_buffer), format, argptr);
	va_end( argptr );

	if ( ( len = strlen( temp_buffer ) ) >= MAX_VA_STRING ) {
		Com_Error( ERR_DROP, "Attempted to overrun string in call to va()\n" );
	}

	if ( len + index >= MAX_VA_STRING - 1 ) {
		index = 0;
	}

	buf = &string[index];
	memcpy( buf, temp_buffer, len + 1 );

	index += len + 1;

	return buf;
}


/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
============

char	* QDECL va( char *format, ... ) {
	va_list		argptr;
	static char string[2][32000]; // in case va is called by nested functions
	static int	index = 0;
	char		*buf;

	buf = string[index & 1];
	index++;

	va_start (argptr, format);
	Q_vsnprintf (buf, sizeof(*string), format, argptr);
	va_end (argptr);

	return buf;
}
*/

/*
============
Com_TruncateLongString

Assumes buffer is atleast TRUNCATE_LENGTH big
============
*/
void Com_TruncateLongString( char *buffer, const char *s )
{
	int length = strlen( s );

	if( length <= TRUNCATE_LENGTH )
		Q_strncpyz( buffer, s, TRUNCATE_LENGTH );
	else
	{
		Q_strncpyz( buffer, s, ( TRUNCATE_LENGTH / 2 ) - 3 );
		Q_strcat( buffer, TRUNCATE_LENGTH, " ... " );
		Q_strcat( buffer, TRUNCATE_LENGTH, s + length - ( TRUNCATE_LENGTH / 2 ) + 3 );
	}
}

/*
=============
TempVector

(SA) this is straight out of g_utils.c around line 210

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float   *tv( float x, float y, float z ) {
	static int index;
	static vec3_t vecs[8];
	float   *v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = ( index + 1 ) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key ) {
	char pkey[BIG_INFO_KEY];
	static char value[2][BIG_INFO_VALUE];   // use two buffers so compares
											// work without stomping on each other
	static int valueindex = 0;
	char    *o;

	if ( !s || !key ) {
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring" );
	}

	valueindex ^= 1;
	if ( *s == '\\' ) {
		s++;
	}
	while ( 1 )
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s ) {
				return "";
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while ( *s != '\\' && *s )
		{
			*o++ = *s++;
		}
		*o = 0;

		if ( !Q_stricmp( key, pkey ) ) {
			return value[valueindex];
		}

		if ( !*s ) {
			break;
		}
		s++;
	}

	return "";
}


/*
===================
Info_NextPair

Used to itterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value ) {
	char    *o;
	const char  *s;

	s = *head;

	if ( *s == '\\' ) {
		s++;
	}
	key[0] = 0;
	value[0] = 0;

	o = key;
	while ( *s != '\\' ) {
		if ( !*s ) {
			*o = 0;
			*head = s;
			return;
		}
		*o++ = *s++;
	}
	*o = 0;
	s++;

	o = value;
	while ( *s != '\\' && *s ) {
		*o++ = *s++;
	}
	*o = 0;

	*head = s;
}


/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key ) {
	char    *start;
	char pkey[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	char    *o;

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if ( strchr( key, '\\' ) ) {
		return;
	}

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' ) {
			s++;
		}
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ( *s != '\\' && *s )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) ) {
			memmove(start, s, strlen(s) + 1); // remove this part
			return;
		}

		if ( !*s ) {
			return;
		}
	}

}

/*
===================
Info_RemoveKey_Big
===================
*/
void Info_RemoveKey_Big( char *s, const char *key ) {
	char    *start;
	char pkey[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];
	char    *o;

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_RemoveKey_Big: oversize infostring" );
	}

	if ( strchr( key, '\\' ) ) {
		return;
	}

	while ( 1 )
	{
		start = s;
		if ( *s == '\\' ) {
			s++;
		}
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while ( *s != '\\' && *s )
		{
			if ( !*s ) {
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		if ( !strcmp( key, pkey ) ) {
			memmove(start, s, strlen(s) + 1); // remove this part
			return;
		}

		if ( !*s ) {
			return;
		}
	}

}




/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s ) {
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value ) {
	char newi[MAX_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= MAX_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey( s, key );
	if ( !value || !strlen( value ) ) {
		return;
	}

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if ( strlen( newi ) + strlen( s ) >= MAX_INFO_STRING ) {
		Com_Printf( "Info string length exceeded\n" );
		return;
	}

	strcat( s, newi );
}

/*
==================
Info_SetValueForKey_Big

Changes or adds a key/value pair
Includes and retains zero-length values
==================
*/
void Info_SetValueForKey_Big( char *s, const char *key, const char *value ) {
	char newi[BIG_INFO_STRING];
	const char* blacklist = "\\;\"";

	if ( strlen( s ) >= BIG_INFO_STRING ) {
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring" );
	}

	for(; *blacklist; ++blacklist)
 	{
		if (strchr (key, *blacklist) || strchr (value, *blacklist))
		{
			Com_Printf (S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey_Big( s, key );
	if (!value)
		return;

	Com_sprintf( newi, sizeof( newi ), "\\%s\\%s", key, value );

	if ( strlen( newi ) + strlen( s ) >= BIG_INFO_STRING ) {
		Com_Printf( "BIG Info string length exceeded\n" );
		return;
	}

	strcat( s, newi );
}




//====================================================================

/*
==================
Com_CharIsOneOfCharset
==================
*/
static qboolean Com_CharIsOneOfCharset( char c, char *set )
{
	int i;

	for( i = 0; i < strlen( set ); i++ )
	{
		if( set[ i ] == c )
			return qtrue;
	}

	return qfalse;
}

/*
==================
Com_SkipCharset
==================
*/
char *Com_SkipCharset( char *s, char *sep )
{
	char	*p = s;

	while( p )
	{
		if( Com_CharIsOneOfCharset( *p, sep ) )
			p++;
		else
			break;
	}

	return p;
}

/*
==================
Com_SkipTokens
==================
*/
char *Com_SkipTokens( char *s, int numTokens, char *sep )
{
	int		sepCount = 0;
	char	*p = s;

	while( sepCount < numTokens )
	{
		if( Com_CharIsOneOfCharset( *p++, sep ) )
		{
			sepCount++;
			while( Com_CharIsOneOfCharset( *p, sep ) )
				p++;
		}
		else if( *p == '\0' )
			break;
	}

	if( sepCount == numTokens )
		return p;
	else
		return s;
}

