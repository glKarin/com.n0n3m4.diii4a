// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( MACOS_X )
#pragma GCC visibility push(hidden)
#endif
stringDataAllocator_t*	idStr::stringDataAllocator;
bool					idStr::stringAllocatorIsShared;

struct ShutdownStringAllocator {
	~ShutdownStringAllocator() {
		idStr::ShutdownMemory();
	}
};
#if defined( MACOS_X )
#pragma GCC visibility pop
#endif

struct strColor_t {
	idVec4		color;
	const char* str;
};

#if defined( MACOS_X )
#pragma GCC visibility push(hidden)
#endif
static ShutdownStringAllocator shutdownStringAllocator;
#if defined( MACOS_X )
#pragma GCC visibility pop
#endif

idStr::hmsFormat_t	idStr::defaultHMSFormat;


strColor_t g_color_table[COLOR_BITS+1] = {
	{	idVec4( 0.0f,  0.0f,  0.0f,  1.0f ), "^0" },			// 0 - S_COLOR_DEFAULT			0
	{	idVec4( 1.0f,  0.0f,  0.0f,  1.0f ), "^1" }, 			// 1 - S_COLOR_RED				1
	{	idVec4( 0.0f,  1.0f,  0.0f,  1.0f ), "^2" }, 			// 2 - S_COLOR_GREEN			2
	{	idVec4( 1.0f,  1.0f,  0.0f,  1.0f ), "^3" }, 			// 3 - S_COLOR_YELLOW			3
	{	idVec4( 0.0f,  0.0f,  1.0f,  1.0f ), "^4" }, 			// 4 - S_COLOR_BLUE				4
	{	idVec4( 0.0f,  1.0f,  1.0f,  1.0f ), "^5" }, 			// 5 - S_COLOR_CYAN				5
	{	idVec4( 1.0f,  0.0f,  1.0f,  1.0f ), "^6" }, 			// 6 - S_COLOR_MAGENTA			6
	{	idVec4( 1.0f,  1.0f,  1.0f,  1.0f ), "^7" }, 			// 7 - S_COLOR_WHITE			7
	{	idVec4( 0.5f,  0.5f,  0.5f,  1.0f ), "^8" }, 			// 8 - S_COLOR_GRAY				8
	{	idVec4( 0.15f, 0.15f, 0.15f, 1.0f ), "^9" }, 			// 9 - S_COLOR_BLACK			9
	{	idVec4( 0.75f, 0.75f, 0.75f, 1.0f ), "^:" }, 			// : - lt.grey					10
	{	idVec4( 0.25f, 0.25f, 0.25f, 1.0f ), "^;" }, 			// ; - dk.grey					11
	{	idVec4( 0.0f,  0.5f,  0.0f,  1.0f ), "^<" }, 			// < - md.green					12
	{	idVec4( 0.5f,  0.5f,  0.0f,  1.0f ), "^=" }, 			// = - md.yellow				13
	{	idVec4( 0.0f,  0.0f,  0.5f,  1.0f ), "^>" }, 			// > - md.blue					14
	{	idVec4( 0.5f,  0.0f,  0.0f,  1.0f ), "^?" }, 			// ? - md.red					15
	{	idVec4( 0.5f,  0.25f, 0.0f,  1.0f ), "^@" }, 			// @ - md.orange				16
	{	idVec4( 1.0f,  0.6f,  0.1f,  1.0f ), "^A" }, 			// A - lt.orange				17
	{	idVec4( 0.0f,  0.5f,  0.5f,  1.0f ), "^B" }, 			// B - md.cyan					18
	{	idVec4( 0.5f,  0.0f,  0.5f,  1.0f ), "^C" }, 			// C - md.purple				19
	{	idVec4( 1.0f,  0.5f,  0.0f,  1.0f ), "^D" }, 			// D - orange					20
	{	idVec4( 0.5f,  0.0f,  1.0f,  1.0f ), "^E" }, 			// E							21
	{	idVec4( 0.2f,  0.6f,  0.8f,  1.0f ), "^F" }, 			// F							22
	{	idVec4( 0.8f,  1.0f,  0.8f,  1.0f ), "^G" }, 			// G							23
	{	idVec4( 0.0f,  0.4f,  0.2f,  1.0f ), "^H" }, 			// H							24
	{	idVec4( 1.0f,  0.0f,  0.2f,  1.0f ), "^I" }, 			// I							25
	{	idVec4( 0.7f,  0.1f,  0.1f,  1.0f ), "^J" }, 			// J							26
	{	idVec4( 0.6f,  0.2f,  0.0f,  1.0f ), "^K" }, 			// K							27
	{	idVec4( 0.8f,  0.6f,  0.2f,  1.0f ), "^L" }, 			// L							28
	{	idVec4( 0.6f,  0.6f,  0.2f,  1.0f ), "^M" }, 			// M							29
	{	idVec4( 1.0f,  1.0f,  0.75f, 1.0f ), "^N" }, 			// N							30
	{	idVec4( 1.0f,  1.0f,  0.5f,  1.0f ), "^O" }, 			// O							31
};

dword g_dword_color_table[COLOR_BITS+1] = {
#if defined( _XENON ) || ( defined( MACOS_X ) && defined( __ppc__ ) )
	0x000000FF, // S_COLOR_DEFAULT
	0xFF0000FF, // S_COLOR_RED
	0x00FF00FF, // S_COLOR_GREEN
	0xFFFF00FF, // S_COLOR_YELLOW
	0x0000FFFF, // S_COLOR_BLUE
	0x00FFFFFF, // S_COLOR_CYAN
	0xFF00FFFF, // S_COLOR_MAGENT
	0xFFFFFFFF, // S_COLOR_WHITE
	0x7F7F7FFF, // S_COLOR_GRAY
	0x121212FF, // S_COLOR_BLACK
	0xBFBFBFFF,
	0x404040FF,
	0x007F00FF,
	0x7F7F00FF,
	0x00007FFF,
	0x7F0000FF,
	0x7F3F00FF,
	0xFF9919FF,
	0x007F7FFF,
	0x7F007FFF,
	0xFF7F00FF,
	0x7F00FFFF,
	0x3399CCFF,
	0xCCFFCCFF,
	0x006633FF,
	0xFF0033FF,
	0xB21919FF,
	0x993300FF,
	0xCC9933FF,
	0x999933FF,
	0xFFFFBFFF,
	0xFFFF7FFF
#elif defined( _WIN32 ) || defined( __linux__ ) || ( defined( MACOS_X ) && !defined( __ppc__ ) )
	0xFF000000, // S_COLOR_DEFAULT
	0xFF0000FF, // S_COLOR_RED
	0xFF00FF00, // S_COLOR_GREEN
	0xFF00FFFF, // S_COLOR_YELLOW
	0xFFFF0000, // S_COLOR_BLUE
	0xFFFFFF00, // S_COLOR_CYAN
	0xFFFF00FF, // S_COLOR_MAGENT
	0xFFFFFFFF, // S_COLOR_WHITE
	0xFF7F7F7F, // S_COLOR_GRAY
	0xFF212121, // S_COLOR_BLACK
	0xFFBFBFBF,
	0xFF040404,
	0xFF007F00,
	0xFF007F7F,
	0xFF7F0000,
	0xFF00007F,
	0xFF003F7F,
	0xFF1999FF,
	0xFF7F7F00,
	0xFF7F007F,
	0xFF007FFF,
	0xFFFF007F,
	0xFFCC9933,
	0xFFCCFFCC,
	0xFF336600,
	0xFF3300FF,
	0xFF1919B2,
	0xFF003399,
	0xFF3399CC,
	0xFF339999,
	0xFFBFFFFF,
	0xFF7FFFFF
#else
#error OS define is required!
#endif
};

const char *units[2][4] =
{
	{ "B", "KB", "MB", "GB" },
	{ "B/s", "KB/s", "MB/s", "GB/s" }
};

/*
============
idStr::ColorForIndex
============
*/
const idVec4& idStr::ColorForIndex( int i ) {
	return g_color_table[ i & COLOR_BITS ].color;
}

/*
============
idStr::ColorForChar
============
*/
const idVec4& idStr::ColorForChar( int c ) {
	return g_color_table[ ColorIndex( c ) ].color;
}

/*
============
idStr::StrForColorIndex
============
*/
const char* idStr::StrForColorIndex( int i ) {
	return g_color_table[ i & COLOR_BITS ].str;
}

/*
============
idStr::ReAllocate
============
*/
void idStr::ReAllocate( int amount, bool keepold ) {
	char	*newbuffer;
	int		newsize;
	int		mod;

 	bool	staticBuffer = alloced < 0;

	//assert( data );
	assert( amount > 0 );

	mod = amount % STR_ALLOC_GRAN;
	if ( !mod ) {
		newsize = amount;
	} else {
		newsize = amount + STR_ALLOC_GRAN - mod;
	}
	alloced = newsize;

	newbuffer = stringDataAllocator->Alloc( alloced );

	if ( keepold && data ) {
		if ( len ) {
			strncpy( newbuffer, data, len );
			newbuffer[ len ] = '\0';
		} else {
			newbuffer[0] = '\0';
		}
	}

	if ( data && !staticBuffer /*data != baseBuffer*/ ) {
		stringDataAllocator->Free( data );
	}

	data = newbuffer;
}

/*
============
idStr::FreeData
============
*/
void idStr::FreeData( void ) {
	bool	staticBuffer = alloced < 0;
	if ( data && !staticBuffer ) {
		stringDataAllocator->Free( data );
		data = baseBuffer;
		alloced = -STR_ALLOC_BASE;
		len = 0;
	}
}

void idStr::SetStaticBuffer( char *buffer, int length ) {
	bool	staticBuffer = alloced < 0;
	if ( data && !staticBuffer ) {
		stringDataAllocator->Free( data );
	}
	data = buffer;
	alloced = -length;
	len = 0;
}


/*
============
idStr::operator=
============
*/
void idStr::operator=( const char *text ) {
	int l;
	int diff;
	int i;

	if ( !text ) {
		// safe behaviour if NULL
		EnsureAlloced( 1, false );
		data[ 0 ] = '\0';
		len = 0;
		return;
	}

	if ( text == data ) {
		return; // copying same thing
	}

	// check if we're aliasing
	if ( text >= data && text <= data + len ) {
		diff = text - data;

		assert( idStr::Length( text ) < (int)len );

		for ( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		}

		data[ i ] = '\0';

		len -= diff;

		return;
	}

	l = Length( text );
	EnsureAlloced( l + 1, false );
	strcpy( data, text );
	len = l;
}

/*
============
idStr::FindChar

returns INVALID_POSITION if not found otherwise the index of the char
============
*/
int idStr::FindChar( const char *str, const char c, int start, int end ) {
	int i;

	if ( end == INVALID_POSITION ) {
		end = Length( str ) - 1;
	}
	for ( i = start; i <= end; i++ ) {
		if ( str[i] == c ) {
			return i;
		}
	}
	return INVALID_POSITION;
}

/*
============
idStr::FindText

returns INVALID_POSITION if not found otherwise the index of the text
============
*/
int idStr::FindText( const char *str, const char *text, bool casesensitive, int start, int end ) {
	int l, i, j;

	if ( end == INVALID_POSITION ) {
		end = Length( str );
	}
	l = end - Length( text );
	for ( i = start; i <= l; i++ ) {
		if ( casesensitive ) {
			for ( j = 0; text[j]; j++ ) {
				if ( str[i+j] != text[j] ) {
					break;
				}
			}
		} else {
			for ( j = 0; text[j]; j++ ) {
				if ( ::toupper( str[i+j] ) != ::toupper( text[j] ) ) {
					break;
				}
			}
		}
		if ( !text[j] ) {
			return i;
		}
	}
	return INVALID_POSITION;
}

/*
============
idStr::CountChar
============
*/
int idStr::CountChar( const char *str, const char c ) {
	int i, count = 0;
	for ( i = 0; str[i] != '\0'; i++ ) {
		if ( str[i] == c ) {
			count++;
		}
	}
	return count;
}

/*
============
idStr::Filter

Returns true if the string conforms the given filter.
Several metacharacter may be used in the filter.

*          match any string of zero or more characters
?          match any single character
[abc...]   match any of the enclosed characters; a hyphen can
           be used to specify a range (e.g. a-z, A-Z, 0-9)

============
*/
bool idStr::Filter( const char *filter, const char *name, bool casesensitive ) {
	idStr buf;
	int i, found, index;

	while(*filter) {
		if (*filter == '*') {
			filter++;
			buf.Empty();
			for (i = 0; *filter; i++) {
				if ( *filter == '*' || *filter == '?' || (*filter == '[' && *(filter+1) != '[') ) {
					break;
				}
				buf += *filter;
				if ( *filter == '[' ) {
					filter++;
				}
				filter++;
			}
			if ( buf.Length() ) {
				index = idStr(name).Find( buf.c_str(), casesensitive );
				if ( index == INVALID_POSITION ) {
					return false;
				}
				name += index + idStr::Length( buf );
			}
		}
		else if (*filter == '?') {
			filter++;
			name++;
		}
		else if (*filter == '[') {
			if ( *(filter+1) == '[' ) {
				if ( *name != '[' ) {
					return false;
				}
				filter += 2;
				name++;
			}
			else {
				filter++;
				found = false;
				while(*filter && !found) {
					if (*filter == ']' && *(filter+1) != ']') {
						break;
					}
					if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
						if (casesensitive) {
							if (*name >= *filter && *name <= *(filter+2)) {
								found = true;
							}
						}
						else {
							if ( ::toupper(*name) >= ::toupper(*filter) && ::toupper(*name) <= ::toupper(*(filter+2)) ) {
								found = true;
							}
						}
						filter += 3;
					}
					else {
						if (casesensitive) {
							if (*filter == *name) {
								found = true;
							}
						}
						else {
							if ( ::toupper(*filter) == ::toupper(*name) ) {
								found = true;
							}
						}
						filter++;
					}
				}
				if (!found) {
					return false;
				}
				while(*filter) {
					if ( *filter == ']' && *(filter+1) != ']' ) {
						break;
					}
					filter++;
				}
				filter++;
				name++;
			}
		}
		else {
			if (casesensitive) {
				if (*filter != *name) {
					return false;
				}
			}
			else {
				if ( ::toupper(*filter) != ::toupper(*name) ) {
					return false;
				}
			}
			filter++;
			name++;
		}
	}
	return true;
}

/*
=============
idStr::StripMediaName

  makes the string lower case, replaces backslashes with forward slashes, and removes extension
=============
*/
void idStr::StripMediaName( const char *name, idStr &mediaName ) {
	char c;

	mediaName.Empty();

	for ( c = *name; c; c = *(++name) ) {
		// truncate at an extension
		if ( c == '.' ) {
			break;
		}
		// convert backslashes to forward slashes
		if ( c == '\\' ) {
			mediaName.Append( '/' );
		} else {
			mediaName.Append( ToLower( c ) );
		}
	}
}

/*
=============
idStr::CheckExtension
=============
*/
bool idStr::CheckExtension( const char *name, const char *ext ) {
	const char *s1 = name + Length( name ) - 1;
	const char *s2 = ext + Length( ext ) - 1;
	char c1, c2, d;

	do {
		c1 = *s1--;
		c2 = *s2--;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return false;
		}
	} while( s1 >= name && s2 >= ext );

	return ( s1 >= name ) && ( *s1 == '.' );
}

/*
=============
idStr::FloatArrayToString
=============
*/
const char *idStr::FloatArrayToString( const float *array, const int length, const int precision ) {
	static int index = 0;
	static char str[4][16384];	// in case called by nested functions
	int i, n;
	char format[16], *s;

	// use an array of string so that multiple calls won't collide
	s = str[ index ];
	index = (index + 1) & 3;

	snPrintf( format, sizeof( format ), "%%.%df", precision );
	n = snPrintf( s, sizeof( str[0] ), format, array[0] );
	if ( precision > 0 ) {
		while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
		while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
	}
	snPrintf( format, sizeof( format ), " %%.%df", precision );
	for ( i = 1; i < length; i++ ) {
		n += snPrintf( s + n, sizeof( str[0] ) - n, format, array[i] );
		if ( precision > 0 ) {
			while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
			while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
		}
	}
	return s;
}

/*
===============
idStr::NumLoneyLF
===============
*/
int idStr::NumLonelyLF( const char *src ) {
	int n = 0;
	for ( ; *src != 0x00; src++ ) {
		if ( *src == '\n' && *( src -1 ) != '\r' ) {
			n++;
		}
	}
	return n;
}

/*
===============
idStr::ToCRLF
===============
*/
bool idStr::ToCRLF( const char *src, char *dest, int maxLength ) {
	// copy, turning lonely linefeeds into CR\LF
	int j = 0;
	for ( int i = 0; src[ i ] != 0x00 && j < maxLength - 1; i++, j++ ) {
		int ch = src[ i ];
		if ( ch == '\n' && src[ i - 1 ] != '\r' ) {
			dest[ j ] = '\r';
			dest[ ++j ] = '\n';
		} else {
			dest[ j ] = ch;
		}
	}

	// 0 terminate
	if ( j < maxLength ) {
		dest[ j ] = 0x00;
		return true;
	}

	dest[ maxLength - 1 ] = 0x00;
	return false;
}

/*
===============
idStr::CStyleQuote
===============
*/
const char *idStr::CStyleQuote( const char *str ) {
	static int index = 0;
	static char buffers[4][16384];	// in case called by nested functions
	unsigned int i;
	char *buf;

	buf = buffers[index];
	index = ( index + 1 ) & 3;

	buf[0] = '\"';
	for ( i = 1; i < sizeof( buffers[0] ) - 2; i++ ) {
		int c = *str++;
		switch( c ) {
			case '\0': buf[i++] = '\"'; buf[i] = '\0'; return buf;
			case '\\': buf[i++] = '\\'; buf[i] = '\\'; break;
			case '\n': buf[i++] = '\\'; buf[i] = 'n'; break;
			case '\r': buf[i++] = '\\'; buf[i] = 'r'; break;
			case '\t': buf[i++] = '\\'; buf[i] = 't'; break;
			case '\v': buf[i++] = '\\'; buf[i] = 'v'; break;
			case '\b': buf[i++] = '\\'; buf[i] = 'b'; break;
			case '\f': buf[i++] = '\\'; buf[i] = 'f'; break;
			case '\a': buf[i++] = '\\'; buf[i] = 'a'; break;
			case '\'': buf[i++] = '\\'; buf[i] = '\''; break;
			case '\"': buf[i++] = '\\'; buf[i] = '\"'; break;
			case '\?': buf[i++] = '\\'; buf[i] = '\?'; break;
			default: buf[i] = c; break;
		}
	}
	buf[i++] = '\"';
	buf[i] = '\0';
	return buf;
}

/*
===============
idStr::CStyleUnQuote
===============
*/
const char *idStr::CStyleUnQuote( const char *str ) {
	static int index = 0;
	static char buffers[4][16384];	// in case called by nested functions
	unsigned int i;
	char *buf;

	buf = buffers[index];
	index = ( index + 1 ) & 3;

	assert( str[0] == '\"' );
	str++;
	for ( i = 0; i < sizeof( buffers[0] ) - 1; i++ ) {
		int c = *str++;
		if ( c == '\0' ) {
			break;
		} else if ( c == '\\' ) {
			c = *str++;
			switch( c ) {
				case '\\': buf[i] = '\\'; break;
				case 'n': buf[i] = '\n'; break;
				case 'r': buf[i] = '\r'; break;
				case 't': buf[i] = '\t'; break;
				case 'v': buf[i] = '\v'; break;
				case 'b': buf[i] = '\b'; break;
				case 'f': buf[i] = '\f'; break;
				case 'a': buf[i] = '\a'; break;
				case '\'': buf[i] = '\''; break;
				case '\"': buf[i] = '\"'; break;
				case '\?': buf[i] = '\?'; break;
			}
		} else {
			buf[i] = c;
		}
	}
	assert( buf[i-1] == '\"' );
	buf[i-1] = '\0';
	return buf;
}

/*
============
idStr::Last

returns INVALID_POSITION if not found otherwise the index of the char
============
*/
int idStr::Last( const char c, int index ) const {
	if( index == INVALID_POSITION ) {
		index = Length();
	}

	for( ; index >= 0; index-- ) {
		if ( data[ index ] == c ) {
			return index;
		}
	}
	return INVALID_POSITION;
}

/*
============
idStr::Last

returns INVALID_POSITION if not found otherwise the index of the string
============
*/
int idStr::Last( const char* str, bool casesensitive, int index ) const {
	if( index == INVALID_POSITION ) {
		index = Length();
	}
	int searchLength = Length( str ); 
	if( len - index > searchLength ) {
		index -= searchLength;
	}

	for( ; index >= 0; index-- ) {
		if( ( casesensitive && Cmpn( &data[ index ], str, searchLength ) == 0 ) ||
			( !casesensitive && Icmpn( &data[ index ], str, searchLength ) == 0 )) {
			return index;
		}
	}
	return INVALID_POSITION;
}


/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char c ) {
	while( data[ 0 ] == c ) {
		memmove( &data[ 0 ], &data[ 1 ], len );
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char *string ) {
	int l;

	l = Length( string );
	if ( l > 0 ) {
		while ( !Cmpn( string, l ) ) {
			memmove( data, data + l, len - l + 1 );
			len -= l;
		}
	}
}

/*
============
idStr::StripLeadingOnce
============
*/
bool idStr::StripLeadingOnce( const char *string ) {
	int l;

	l = Length( string );
	if ( ( l > 0 ) && !Cmpn( string, l ) ) {
		memmove( data, data + l, len - l + 1 );
		len -= l;
		return true;
	}
	return false;
}

/*
============
idStr::StripTrailing
============
*/
void idStr::StripTrailing( const char c ) {
	int i;
	
	for( i = Length(); i > 0 && data[ i - 1 ] == c; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripTrailing( const char *string ) {
	int l;

	l = Length( string );
	if ( l > 0 ) {
		while ( ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
			len -= l;
			data[len] = '\0';
		}
	}
}

/*
============
idStr::StripTrailingOnce
============
*/
bool idStr::StripTrailingOnce( const char *string ) {
	int l;

	l = Length( string );
	if ( ( l > 0 ) && ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
		len -= l;
		data[len] = '\0';
		return true;
	}
	return false;
}

/*
============
idStr::ReplaceChar
============
*/
void idStr::ReplaceChar( char oldChar, char newChar ) {
	int i;
	for ( i = 0; i < len; i++ ) {
		if ( data[ i ] != oldChar ) {
			continue;
		}

		data[ i ] = newChar;
	}
}

/*
============
idStr::Replace
============
*/
void idStr::Replace( const char *old, const char *nw ) {
	if ( Length( old ) == 0 ) {
		return;
	}

	int		oldLen, newLen, i, count;

	oldLen = Length( old );
	newLen = Length( nw );

	// Work out how big the new string will be
	count = 0;
	for ( i = 0; i < len; i++ ) {
		if( !Cmpn( &data[i], old, oldLen ) ) {
			count++;
			i += oldLen - 1;
		}
	}

	if ( count ) {
		idStr	oldString( data );
		int		j;

		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		for ( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if ( !Cmpn( &oldString[i], old, oldLen ) ) {
				memcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		data[j] = '\0';
		len = Length( data );
	}
}

/*
============
idStr::ReplaceFirst
============
*/
void idStr::ReplaceFirst( const char *old, const char *nw ) {
	if( Length( old ) == 0 ) {
		return;
	}

	int		oldLen, newLen, i;
	bool	present;

	oldLen = Length( old );
	newLen = Length( nw );

	// Work out how big the new string will be
	present = false;
	for ( i = 0; i < len; i++ ) {
		if ( !Cmpn( &data[i], old, oldLen ) ) {
			present = true;
			i += oldLen - 1;
			break;
		}
	}

	if ( present ) {
		idStr	oldString( data );
		int		j;

		EnsureAlloced( len + ( newLen - oldLen ) + 2, false );

		// Replace the old data with the new data
		for ( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if ( !Cmpn( &oldString[i], old, oldLen ) ) {
				memcpy( data + j, nw, newLen );
				i += oldLen;
				j += newLen;
				break;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		memcpy( data + j, &oldString[i], oldString.Length() - i );
		data[j + oldString.Length() - i] = '\0';
		len = Length( data );
	}
}

/*
============
idStr::Mid
============
*/
const char *idStr::Mid( int start, int len, idStr &result ) const {
	int i;

	assert( &result != this );

	result.Empty();

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return NULL;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
idStr::Mid
============
*/
idStr idStr::Mid( int start, int len ) const {
	int i;
	idStr result;

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return result;
	}

	if ( start + len >= i ) {
		len = i - start;
	}

	result.Append( &data[ start ], len );
	return result;
}

/*
============
idStr::StripLeadingWhiteSpace
============
*/
void idStr::StripLeadingWhiteSpace( void ) {
	int i;
	
	// cast to unsigned char to prevent stripping off high-ASCII characters
	for ( i = 0; i < Length() && (unsigned char)(data[ i ]) <= ' '; i++ );

	if ( i > 0 && i != Length() ) {
		memmove( data, data + i, len - i + 1 );
		len -= i;
	}
}

/*
============
idStr::StripTrailingWhiteSpace
============
*/
void idStr::StripTrailingWhiteSpace( void ) {
	int i;
	
	// cast to unsigned char to prevent stripping off high-ASCII characters
	for ( i = Length(); i > 0 && (unsigned char)(data[ i - 1 ]) <= ' '; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripQuotes

Removes the quotes from the beginning and end of the string
============
*/
idStr& idStr::StripQuotes ( void )
{
	if ( data[0] != '\"' )
	{
		return *this;
	}
	
	// Remove the trailing quote first
	if ( data[len-1] == '\"' )
	{
		data[len-1] = '\0';
		len--;
	}

	// Strip the leading quote now
	len--;	
	memmove( &data[ 0 ], &data[ 1 ], len );
	data[len] = '\0';
	
	return *this;
}

/*
=====================================================================

  filename methods

=====================================================================
*/

/*
============
idStr::FileNameHash
============
*/
int idStr::FileNameHash( const char *string, const int hashSize ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while( string[i] != '\0' ) {
		letter = idStr::ToLower( string[i] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter =='\\' ) {
			letter = '/';
		}
		hash += (long)letter * ( i + 119 );
		i++;
	}
	hash &= ( hashSize - 1 );
	return hash;
}

/*
============
idStr::BackSlashesToSlashes
============
*/
idStr &idStr::BackSlashesToSlashes( void ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[ i ] == '\\' ) {
			data[ i ] = '/';
		}
	}
	return *this;
}

/*
============
idStr::SlashesToBackSlashes
============
*/
idStr &idStr::SlashesToBackSlashes( void ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[ i ] == '/' ) {
			data[ i ] = '\\';
		}
	}
	return *this;
}

/*
============
idStr::CollapsePath

Removes '..' from path and changes backslashes to slashes.

Example:
W:/ETQW/base/../code/game/../idlib/../game/Game_local.h

Becomes:
W:/ETQW/code/game/Game_local.h
============
*/
idStr &idStr::CollapsePath( void ) {
	int i, length = 0;

	for ( i = 0; i < len; i++ ) {
		if ( data[i] == '.' ) {
			if ( data[i+1] == '.' && ( data[i+2] == '/' || data[i+2] == '\\' ) ) {		//   ../
				if ( length >= 2 && ( data[length-1] == '/' || data[length-1] == '\\' ) ) {
					if ( length == 2 || data[length-2] != '.' || data[length-3] != '.' ) {
						length--;
						while( length > 0 && data[length-1] != '/' && data[length-1] != '\\' ) {
							length--;
						}
						i += 2;
						continue;
					}
				}
				data[length++] = data[i++];
				data[length++] = data[i++];
				data[length++] = data[i];
			} else if ( data[i+1] == '/' || data[i+1] == '\\' ) {							//	./
				i++;
			} else {
				data[length++] = data[i];
			}
		} else {
			data[length++] = data[i];
		}
	}
	data[length] = '\0';
	len = length;
	return *this;
}

/*
============
idStr::SetFileExtension
============
*/
idStr &idStr::SetFileExtension( const char *extension ) {
	StripFileExtension();
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
============
idStr::StripFileExtension
============
*/
idStr &idStr::StripFileExtension( void ) {
	int i;

	for ( i = len-1; i >= 0; i-- ) {
		if ( data[i] == '/' || data[i] == '\\' ) {
			break;
		}
		if ( data[i] == '.' ) {
			data[i] = '\0';
			len = i;
			break;
		}
	}
	return *this;
}

/*
============
idStr::StripAbsoluteFileExtension
============
*/
idStr &idStr::StripAbsoluteFileExtension( void ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[i] == '.' ) {
			data[i] = '\0';
			len = i;
			break;
		}
	}

	return *this;
}

/*
==================
idStr::DefaultFileExtension
==================
*/
idStr &idStr::DefaultFileExtension( const char *extension ) {
	int i;

	// do nothing if the string already has an extension
	for ( i = len-1; i >= 0; i-- ) {
		if ( data[i] == '.' ) {
			return *this;
		}
	}
	if ( *extension != '.' ) {
		Append( '.' );
	}
	Append( extension );
	return *this;
}

/*
==================
idStr::DefaultPath
==================
*/
idStr &idStr::DefaultPath( const char *basepath ) {
	if ( ( ( *this )[ 0 ] == '/' ) || ( ( *this )[ 0 ] == '\\' ) ) {
		// absolute path location
		return *this;
	}

	*this = basepath + *this;
	return *this;
}

/*
====================
idStr::AppendPath
====================
*/
void idStr::AppendPath( const char *text ) {
	int pos;
	int i = 0;

	if ( text && text[i] ) {
		pos = len;
		EnsureAlloced( len + Length( text ) + 2 );

		if ( pos ) {
			if ( data[ pos-1 ] != '/' && data[ pos-1 ] != '\\' ) {
				data[ pos++ ] = '/';
			}
		}
		if ( text[i] == '/' ) {
			i++;
		}

		for ( ; text[ i ]; i++ ) {
			if ( text[ i ] == '\\' ) {
				data[ pos++ ] = '/';
			} else {
				data[ pos++ ] = text[ i ];
			}
		}
		len = pos;
		data[ pos ] = '\0';
	}
}

/*
==================
idStr::StripFilename
==================
*/
idStr &idStr::StripFilename( void ) {
	int pos;

	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos ] != '/' ) && ( ( *this )[ pos ] != '\\' ) ) {
		pos--;
	}

	if ( pos < 0 ) {
		pos = 0;
	}

	CapLength( pos );
	return *this;
}

/*
==================
idStr::StripPath
==================
*/
idStr &idStr::StripPath( void ) {
	int pos;

	pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	*this = Right( Length() - pos );
	return *this;
}

/*
====================
idStr::ExtractFilePath
====================
*/
void idStr::ExtractFilePath( idStr &dest ) const {
	int pos;

	//
	// back up until a \ or the start
	//
	pos = Length();
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Left( pos, dest );
}

/*
====================
idStr::ExtractFileName
====================
*/
void idStr::ExtractFileName( idStr &dest ) const {
	int pos;

	//
	// back up until a \ or the start
	//
	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	Right( Length() - pos, dest );
}

/*
====================
idStr::ExtractFileBase
====================
*/
void idStr::ExtractFileBase( idStr &dest ) const {
	int pos;
	int start;

	//
	// back up until a \ or the start
	//
	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '/' ) && ( ( *this )[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	start = pos;
	while( ( pos < Length() ) && ( ( *this )[ pos ] != '.' ) ) {
		pos++;
	}

	Mid( start, pos - start, dest );
}

/*
====================
idStr::ExtractFileExtension
====================
*/
void idStr::ExtractFileExtension( idStr &dest ) const {
	int pos;

	//
	// back up until a . or the start
	//
	pos = Length() - 1;
	while( ( pos > 0 ) && ( ( *this )[ pos - 1 ] != '.' ) ) {
		pos--;
	}

	if ( !pos ) {
		// no extension
		dest.Empty();
	} else {
		Right( Length() - pos, dest );
	}
}

/*
============
idStr::MakeNameCanonical
============
*/
void idStr::MakeNameCanonical( void ) {
	ToLower();
	BackSlashesToSlashes();
	StripFileExtension();
}

/*
=====================================================================

  char * methods to replace library functions

=====================================================================
*/

/*
============
idStr::IsNumeric

Checks a string to see if it contains only numerical values.
============
*/
bool idStr::IsNumeric( const char *s ) {
	int		i;
	bool	dot;

	if ( *s == '-' ) {
		s++;
	}

	dot = false;
	for ( i = 0; s[i]; i++ ) {
		if ( !isdigit( s[i] ) ) {
			if ( ( s[ i ] == '.' ) && !dot ) {
				dot = true;
				continue;
			}
			return false;
		}
	}

	return true;
}

/*
============
idStr::HasLower

Checks if a string has any lowercase chars
============
*/
bool idStr::HasLower( const char *s ) {
	if ( !s ) {
		return false;
	}
	
	while ( *s ) {
		if ( CharIsLower( *s ) ) {
			return true;
		}
		s++;
	}
	
	return false;
}

/*
============
idStr::HasUpper
	
Checks if a string has any uppercase chars
============
*/
bool idStr::HasUpper( const char *s ) {
	if ( !s ) {
		return false;
	}
	
	while ( *s ) {
		if ( CharIsUpper( *s ) ) {
			return true;
		}
		s++;
	}
	
	return false;
}

/*
================
idStr::Cmp
================
*/
int idStr::Cmp( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		if ( d ) {
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Cmpn
================
*/
int idStr::Cmpn( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		if ( d ) {
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::Icmp( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Icmpn
================
*/
int idStr::Icmpn( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::IcmpNoColor( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		while ( IsColor( s1 ) ) {
			s1 += 2;
		}
		while ( IsColor( s2 ) ) {
			s2 += 2;
		}
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;		// strings are equal
}

/*
================
idStr::IcmpPath
================
*/
int idStr::IcmpPath( const char *s1, const char *s2 ) {
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	idLib::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c1 == '\\' ) {
				d += ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 == '\\' ) {
				d -= ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			// make sure folders come first
			while( c1 ) {
				if ( c1 == '/' || c1 == '\\' ) {
					break;
				}
				c1 = *s1++;
			}
			while( c2 ) {
				if ( c2 == '/' || c2 == '\\' ) {
					break;
				}
				c2 = *s2++;
			}
			if ( c1 && !c2 ) {
				return -1;
			} else if ( !c1 && c2 ) {
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;
}

/*
================
idStr::IcmpnPath
================
*/
int idStr::IcmpnPath( const char *s1, const char *s2, int n ) {
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	idLib::common->Printf( "WARNING: IcmpPath used on a case-sensitive filesystem?\n" );
#endif

	assert( n >= 0 );

	do {
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- ) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;
		while( d ) {
			if ( c1 <= 'Z' && c1 >= 'A' ) {
				d += ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c1 == '\\' ) {
				d += ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			if ( c2 <= 'Z' && c2 >= 'A' ) {
				d -= ('a' - 'A');
				if ( !d ) {
					break;
				}
			}
			if ( c2 == '\\' ) {
				d -= ('/' - '\\');
				if ( !d ) {
					break;
				}
			}
			// make sure folders come first
			while( c1 ) {
				if ( c1 == '/' || c1 == '\\' ) {
					break;
				}
				c1 = *s1++;
			}
			while( c2 ) {
				if ( c2 == '/' || c2 == '\\' ) {
					break;
				}
				c2 = *s2++;
			}
			if ( c1 && !c2 ) {
				return -1;
			} else if ( !c1 && c2 ) {
				return 1;
			}
			// same folder depth so use the regular compare
			return ( INTSIGNBITNOTSET( d ) << 1 ) - 1;
		}
	} while( c1 );

	return 0;
}

/*
=============
idStr::Copynz
 
Safe strncpy that ensures a trailing zero
NOTE: the specs indicate strncpy pads with zeros up to destination size, which be a bit wasteful
=============
*/
void idStr::Copynz( char *dest, const char *src, int destsize ) {
	if ( !src ) {
		idLib::common->Warning( "idStr::Copynz: NULL src" );
		return;
	}
	if ( destsize < 1 ) {
		idLib::common->Warning( "idStr::Copynz: destsize < 1" ); 
		return;
	}

	strncpy( dest, src, destsize - 1 );
    dest[ destsize - 1 ] = '\0';
}

/*
================
idStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void idStr::Append( char *dest, int size, const char *src ) {
	int		l1;

	l1 = Length( dest );
	if ( l1 >= size ) {
		idLib::common->Error( "idStr::Append: already overflowed" );
	}
	Copynz( dest + l1, src, size - l1 );
}

/*
================
idStr::LengthWithoutColors
================
*/
int idStr::LengthWithoutColors( const char *s ) {
	int len;
	const char *p;

	if ( !s ) {
		return 0;
	}

	len = 0;
	p = s;
	while( *p ) {
		if ( IsColor( p ) ) {
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

/*
================
idStr::RemoveColors
================
*/
char *idStr::RemoveColors( char *string ) {
	char *d;
	char *s;
	int c;

	s = string;
	d = string;
	while( (c = *s) != 0 ) {
		if ( IsColor( s ) ) {
			s++;
		}		
		else {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
================
idStr::IsBadFilenameChar
================
*/
bool idStr::IsBadFilenameChar( char c ) {
	static char badFilenameChars[] = { ':', ';', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\t', '\'', '"', '\0' };

	for ( int i = 0; badFilenameChars[i] != '\0'; i++ ) {
		if ( c == badFilenameChars[i] ) {
			return true;
		}
	}

	return false;
}

/*
================
idStr::CleanFilename
================
*/
char* idStr::CleanFilename( char* string ) {
	char* d;
	char* s;

	s = string;
	d = string;

	// clear leading .'s
	while ( *s == '.' ) {
		s++;
	}

	while ( *s != '\0' ) {
		if ( !IsBadFilenameChar( *s ) ) {
			*d++ = *s;
		}
		s++;
	}
    *d = '\0';

	return string;
}

/*
================
idStr::StripFilename
================
*/
char* idStr::StripFilename( char* string ) {
	int pos;
	
	pos = idStr::Length( string ) - 1;
	while( ( pos > 0 ) && ( string[ pos ] != '/' ) && ( string[ pos ] != '\\' ) ) {
		pos--;
	}

	if ( pos < 0 ) {
		pos = 0;
	}

	string[ pos ] = '\0';

	return string;
}

/*
==================
idStr::StripPath
==================
*/
char* idStr::StripPath( char* string ) {
	int pos, length;

	length = pos = idStr::Length( string );
	while( ( pos > 0 ) && ( string[ pos - 1 ] != '/' ) && ( string[ pos - 1 ] != '\\' ) ) {
		pos--;
	}

	return &string[ pos ];
}

/*
================
idStr::snPrintf

see idStr::vsnPrintf
you can pass snPrintf( buffer, sizeof( buffer ) .. )
will return -1 on error or overflow (and warn)

upon overflow the string is written and truncated

returns the number of characters written, not including the terminal null
(terminating null character is always written, which means ret < size in all cases)
================
*/
int idStr::snPrintf( char *dest, int size, const char *fmt, ...) {
	int ret;
	va_list argptr;

#ifdef _WIN32
#undef _vsnprintf
	va_start( argptr, fmt );
	ret = _vsnprintf( dest, size-1, fmt, argptr );
	va_end( argptr );
#define _vsnprintf	use_idStr_vsnPrintf
#else
#undef vsnprintf
	va_start( argptr, fmt );
	ret = vsnprintf( dest, size, fmt, argptr );
	va_end( argptr );
#define vsnprintf	use_idStr_vsnPrintf
#endif
	dest[size-1] = '\0';
	if ( ret < 0 || ret >= size ) {
		return -1;
	}
	return ret;
}

// pedestrian version: verbose and explicit implementation - just stick to the easier one
#if 0
int idStr::snPrintf( char *dest, int size, const char *fmt, ...) {
	int len;
	va_list argptr;

	va_start( argptr, fmt );
	// VC7 only has _vsnprintf
	// VC8 adds vsnprintf, which does exactly the same
	// which is bad because their implementation still isn't C99 compliant
#ifdef _WIN32
	len = _vsnprintf( dest, size-1, fmt, argptr );
#else
	len = vsnprintf( dest, size-1, fmt, argptr );
#endif
	va_end( argptr );
	if ( len < 0 ) {
#ifdef _WIN32
		// unless _set_invalid_parameter_handler has been set to something other than default
		// then this is very likely an overflow
		idLib::common->Warning( "idStr::snPrintf: error or overflow %i", len );
#else
		idLib::common->Warning( "idStr::snPrintf: error %i", len );
#endif
		// put a terminating null character
		dest[size-1] = '\0';
		return -1;
	}
	if ( len >= size - 1 ) {
		// on Linux systems this means the output was truncated at size-1
		// (that is conformant to the C99 standard)
		// windows systems will just return -1 on overflow (handled above)
		// still, the retarded windows implementation may write exactly size - 1 characters and *not* put a terminating null character
#ifdef _WIN32
		assert( len == size - 1 );
		dest[size-1] = '\0';
		return len;
#else
		if ( len == size - 1 ) {
			// fixup to match win32 behaviour
			dest[size-1] = '\0';
			return len;
		}
		idLib::common->Warning( "idStr::snPrintf: overflow of %i in %i", len, size );
		dest[size-1] = '\0';
		return -1;
#endif
	}
	return len;
}
#endif

/*
============
idStr::vsnPrintf

vsnprintf portability:

C99 standard: vsnprintf returns the number of characters (excluding the trailing
'\0') which would have been written to the final string if enough space had been available
snprintf and vsnprintf do not write more than size bytes (including the trailing '\0')

win32: _vsnprintf returns the number of characters written, not including the terminating null character,
or a negative value if an output error occurs. If the number of characters to write exceeds count, then count 
characters are written and -1 is returned and no trailing '\0' is added.

idStr::vsnPrintf: always appends a trailing '\0', returns number of characters written (not including terminal \0)
or returns -1 on failure or if the buffer would be overflowed.
============
*/
int idStr::vsnPrintf( char *dest, int size, const char *fmt, va_list argptr ) {
	int ret;

#ifdef _WIN32
#undef _vsnprintf
	ret = _vsnprintf( dest, size-1, fmt, argptr );
#define _vsnprintf	use_idStr_vsnPrintf
#else
#undef vsnprintf
	ret = vsnprintf( dest, size, fmt, argptr );
#define vsnprintf	use_idStr_vsnPrintf
#endif
	dest[size-1] = '\0';
	if ( ret < 0 || ret >= size ) {
		return -1;
	}
	return ret;
}

/*
===============
idStr::Test
test those snPrintf/vsnPrintf functions
mostly to check the behaviour between win32 and other platforms is the same
===============
*/
void idStr::Test( void ) {
	char buffer[10];
	int ret;
	idStr test;

	idLib::common->Printf( "idStr::Test\n" );

	idStr::Copynz( buffer, "012345678", sizeof( buffer ) );
	assert( buffer[9] == '\0' );

	ret = test.snPrintf( buffer, 10, "%s", "876543210" );
	assert( buffer[9] == '\0' );
	idLib::common->Printf( "%d %s\n", ret, buffer );

	ret = test.snPrintf( buffer, 10, "%s", "0123456789" );
	assert( buffer[9] == '\0' );
	idLib::common->Printf( "%d %s\n", ret, buffer );
}

/*
============
sprintf

Sets the value of the string using a printf interface.
============
*/
int sprintf( idStr &string, const char *fmt, ... ) {
	int l;
	va_list argptr;
	char buffer[32000];
	
	va_start( argptr, fmt );
	l = idStr::vsnPrintf( buffer, sizeof(buffer), fmt, argptr );
	va_end( argptr );

	string = buffer;
	return l;
}

/*
============
vsprintf

Sets the value of the string using a vprintf interface.
============
*/
int vsprintf( idStr &string, const char *fmt, va_list argptr ) {
	int l;
	char buffer[32000];
	
	l = idStr::vsnPrintf( buffer, sizeof(buffer), fmt, argptr );
	
	string = buffer;
	return l;
}

/*
============
va

does a varargs printf into a temp buffer
NOTE: not thread safe
============
*/
char *va( const char *fmt, ... ) {
	va_list argptr;
	static int index = 0;
	static char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1) & 3;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}

char *vva( char *buf, const char *fmt, ... ) {
	va_list argptr;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}

/*
=================
va_floatstring
=================
*/
char* va_floatstring( const char *fmt, ... ) {
	va_list argPtr;
	static int bufferIndex = 0;
	static char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[bufferIndex];
	bufferIndex = (bufferIndex + 1) & 3;

	long i;
	unsigned long u;
	double f;
	char *str;
	int index;
	idStr tmp, format;

	index = 0;

	va_start( argPtr, fmt );
	while( *fmt ) {
		switch( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
					*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
						format += *fmt++;
					}
					format += *fmt;
					switch( *fmt ) {
						case 'f':
						case 'e':
						case 'E':
						case 'g':
						case 'G':
							f = va_arg( argPtr, double );
							if ( format.Length() <= 2 ) {
								// high precision floating point number without trailing zeros
								sprintf( tmp, "%1.10f", f );
								tmp.StripTrailing( '0' );
								tmp.StripTrailing( '.' );
								index += sprintf( buf+index, "%s", tmp.c_str() );
							}
							else {
								index += sprintf( buf+index, format.c_str(), f );
							}
							break;
						case 'd':
						case 'i':
							i = va_arg( argPtr, long );
							index += sprintf( buf+index, format.c_str(), i );
							break;
						case 'u':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'o':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'x':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'X':
							u = va_arg( argPtr, unsigned long );
							index += sprintf( buf+index, format.c_str(), u );
							break;
						case 'c':
							i = va_arg( argPtr, long );
							index += sprintf( buf+index, format.c_str(), (char) i );
							break;
						case 's':
							str = va_arg( argPtr, char * );
							index += sprintf( buf+index, format.c_str(), str );
							break;
						case '%':
							index += sprintf( buf+index, format.c_str() );
							break;
						default:
							common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
							break;
					}
					fmt++;
					break;
			case '\\':
				fmt++;
				switch( *fmt ) {
			case 't':
				index += sprintf( buf+index, "\t" );
				break;
			case 'v':
				index += sprintf( buf+index, "\v" );
				break;
			case 'n':
				index += sprintf( buf+index, "\n" );
				break;
			case '\\':
				index += sprintf( buf+index, "\\" );
				break;
			default:
				common->Error( "FS_WriteFloatString: unknown escape character \'%c\'", *fmt );
				break;
				}
				fmt++;
				break;
			default:
				index += sprintf( buf+index, "%c", *fmt );
				fmt++;
				break;
		}
	}
	va_end( argPtr );

	return buf;
}



/*
============
idStr::BestUnit
============
*/
int idStr::BestUnit( const char *format, float value, measure_t measure ) {
	int unit = 1;
	while ( unit <= 3 && ( 1 << ( unit * 10 ) < value ) ) {
		unit++;
	}
	unit--;
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];
	return unit;
}

/*
============
idStr::SetUnit
============
*/
void idStr::SetUnit( const char *format, float value, int unit, measure_t measure ) {
	value /= 1 << ( unit * 10 );
	sprintf( *this, format, value );
	*this += " ";
	*this += units[ measure ][ unit ];	
}

/*
================
idStr::InitMemory
================
*/
void idStr::InitMemory( void ) {
	if( !stringDataAllocator ) {
		stringDataAllocator = new stringDataAllocator_t;
		stringDataAllocator->Init();
		stringAllocatorIsShared = false;
	}	
}

/*
================
idStr::ShutdownMemory
================
*/
void idStr::ShutdownMemory( void ) {
	if( stringDataAllocator && !stringAllocatorIsShared ) {
		stringDataAllocator->Shutdown();
		delete stringDataAllocator;
		stringDataAllocator = NULL;
	}
}

/*
================
idStr::PurgeMemory
================
*/
void idStr::PurgeMemory( void ) {
	stringDataAllocator->FreeEmptyBaseBlocks();
}

/*
================
idStr::ShowMemoryUsage_f
================
*/
void idStr::ShowMemoryUsage_f( const idCmdArgs &args ) {
	idLib::common->Printf( "%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
		stringDataAllocator->GetBaseBlockMemory() >> 10, stringDataAllocator->GetFreeBlockMemory() >> 10,
		stringDataAllocator->GetNumFreeBlocks(), stringDataAllocator->GetNumEmptyBaseBlocks() );
	idWStr::ShowMemoryUsage_f( args );
}

/*
============
idStr::SetStringAllocator
============
*/
void idStr::SetStringAllocator( stringDataAllocator_t* allocator ) {
	if( !stringAllocatorIsShared ) {
		delete stringDataAllocator;
	}
	stringDataAllocator = allocator;
	stringAllocatorIsShared = true;
}

/*
============
idStr::GetStringAllocator
============
*/
stringDataAllocator_t* idStr::GetStringAllocator( void ) {
	return stringDataAllocator;
}

/*
===============
idStr::IndentAndPad

adds a formated, indented line to a string.  The line is indented "indent" 
characters and the formatted string is written. If the string size is less than 
the pad size, the remaining characters in the string are filled with spaces up
to the "pad" position.
===============
*/
void idStr::IndentAndPad( int indent, int pad, idStr &str, const char *fmt, ... ) {
	assert( pad >= 0 );
	if ( pad < 0 ) {
		pad = 0;
	}
	int max = 1024;
	char *buff = (char *)_alloca( 1024 + 1 );
	memset( buff, 0x20, indent > 128 ? 128 : indent );

	va_list	argptr;
	va_start( argptr, fmt );
	vsnPrintf( buff + indent, max - indent, fmt, argptr );
	va_end( argptr );

	int len = Length( buff );
	if ( pad && len <= pad ) {
		memset( buff + len, 0x20, pad - len );
		buff[ pad ] = '\0';
	} else {
		// ensure there's at least 1 space of padding if the formatted string
		// exceeded the pad size
		buff[ len ] = ' ';
		buff[ len + 1 ] = '\0';
	}

	str += buff;
}

/*
===============
idStr::FormatInt

formats integers with commas for readability
===============
*/
const char* idStr::FormatInt( const int num ) {
	static idStr val;
	val = va( "%d", num );
	int len = val.Length();
	for ( int i = 0 ; i < ( ( len - 1 ) / 3 ); i++ ) {
		int pos = val.Length() - ( ( i + 1 ) * 3 + i );
		val.Insert( ',', pos );
	}
	return ( val.c_str() );
}

/*
============
idStr::EraseRange
============
*/
void idStr::EraseRange( int start, int len ) {
	if( IsEmpty() || len == 0 ) {
		return;
	}

	if( start < 0 ) {
		start = 0;
	}

	if( start >= this->len ) {
		return;
	}

	int totalLength = Length();
	if( len == INVALID_POSITION ) {
		len = totalLength - start;
	}
	
	if( len == totalLength ) {
		// erase the whole thing
		Empty();
		return;
	}


	if( totalLength - start - len ) {
		memmove( &data[ start ], &data[ start + len ], totalLength - start - len );
	}
	
	data[ totalLength - len ] = '\0';
	this->len -= len;
}


/*
============
idStr::EraseChar
============
*/
void idStr::EraseChar( const char c, int start ) {
	if( start < 0 ) {
		start = 0;
	}

	int totalLength = Length();
	while( start < totalLength - 1 ) {
		int offset = start + 1;
		while( data[ start ] == c && offset < totalLength ) {
			idSwap( data[ start ], data[ offset ] );
			offset++;
		}
		start++;
	}

	start = totalLength - 1;
	while( start > 0 && data[ start ] == c ) {
		data[ start ] = '\0';
		start--;
	}
	len = start + 1;
}


/*
============
idStr::Append
============
*/
void idStr::Append( int count, const char c ) {
	EnsureAlloced( len + count + 1 );
	int start = len;
	int end = len + count;
	while( start < end ) {
		data[ start ] = c;
		start++;
	}
	data[ start ] = '\0';
	len += count;
}

/*
============
idStr::StripComments
============
*/
idStr& idStr::StripComments() {

	// handle C++-style comments
	int startIndex = Find( "//" );
	int endIndex = Find( "\n", true, startIndex + 2 );

	while( startIndex != -1 && endIndex != -1 ) {
		int oldLength = len;
		EraseRange( startIndex, endIndex - startIndex );
		if( len == oldLength ) {
			idLib::common->Warning( "StripCommentsFromString: Couldn't strip comments" );
			break;	//avoid infinite loops
		}
		startIndex = Find( "//" );
		endIndex = Find( "\n", true, startIndex + 2 );
	}

	// handle C-style comments
	startIndex = Find( "/*" );
	endIndex = Find( "*/", true, startIndex + 2 );

	if( ( startIndex != -1 && endIndex == -1 ) || ( startIndex == -1 && endIndex != -1 )) {
		idLib::common->Warning( "StripCommentsFromString: mismatched /* */ comment" );
		return *this;
	}	

	while( startIndex != -1 && endIndex != -1 ) {
		int oldLength = len;
		EraseRange( startIndex, endIndex - startIndex + 2 );
		if( len == oldLength ) {
			idLib::common->Warning( "StripCommentsFromString: Couldn't strip comments" );
			break;	//avoid infinite loops
		}
		startIndex = Find( "/*" );
		endIndex = Find( "*/", true, startIndex + 2 );
	}

	return *this;
}

/*
============
idStr::Indent
braces within C or C++ style comments are ignored
============
*/
idStr& idStr::Indent() {

	Replace( "\r\n", "\n" );	// kill windows line endings
	EraseChar( '\r' );			// kill broken windows line endings

	// strip out tabs at the beginning of lines
	int i;
	for( i = 0; i < len; ++i ) {
		if( data[ i ] == '\n' ) {
			++i;
			while( i < len && data[ i ] == '\t' ) {
				EraseRange( i, 1 );
			}
			--i;
		}
	}

	idStr output;
	output.EnsureAlloced( len, false );
	int indent = 0;
	for( i = 0; i < len; i++ ) {
		// skip braces within comments
		if( i + 1 < len) {
			if( data[ i ] == '/' && data[ i + 1 ] == '/' ) {
				while( i < len && data[ i ] != '\n' ) {
					output += data[ i ];
					i++;
				}
			}  else if( data[ i ] == '/' && data[ i + 1 ] == '*' ) {
				while( i < len && !( data[ i ] == '*' && data[ i + 1 ] == '/' )) {
					output += data[ i ];
					i++;
				}
			}
		}

		if( data[ i ] == '{' ) {
			indent++;
		} else if( data[ i ] == '}' ) {
			indent--;
			// unindent closing braces
			output.StripTrailingOnce( "\t" );
		}

		output += data[ i ];

		if( data[ i ] == '\n' && indent > 0 ) {
			output.Append( indent, '\t' );
		}		
	}

	*this = output;
	return *this;
}

/*
============
idStr::Unindent
unindent all lines; tabs are preserved if they are in the middle of a line
============
*/
idStr& idStr::Unindent() {

	idStr output;
	output.EnsureAlloced( len, false );
	int i;
	for( i = 0; i < len; ++i ) {	
		if( data[ i ] == '\t' && i > 0 && data[ i - 1 ] == '\n' ) {
			// strip leading tabs
			while( i < len && data[ i ] == '\t' ) {
				++i;
			}
		} else if( data[ i ] == '\t' && i > 0 && data[ i - 1 ] != '\n' ) {
			// strip trailing tabs 
			int temp = i;
			while( temp < len ) {
				if( data[ temp ] == '\r' || data[ temp ] == '\n' ) {
					i = temp;
					break;
				} else if( data[ temp ] != '\t' ){
					break;
				}
				++temp;
			}
		}
		output += data[ i ];				
	}

	*this = output;
	return *this;
}

static const char* const hexDigits = "0123456789ABCDEF";

/*
============
idStr::StringToBinaryString
============
*/
void idStr::StringToBinaryString( idStr& out, void *pv, int size ) {
	sdAutoPtr< unsigned char, sdArrayCleanupPolicy< unsigned char > > in( new unsigned char[ size ] );
	memset( in.Get(), 0, size );
	memcpy( in.Get(), pv, size );

	for( int i = 0; i < size; i++ ) {
		unsigned char c = in[ i ];
		out += hexDigits[ c >> 4 ];
		out += hexDigits[ c & 0x0f ];
	}
}

/*
============
idStr::BinaryStringToString
============
*/
bool idStr::BinaryStringToString( const char* str,  void* pv, int size ) {
	bool ret = false;

	int length = idStr::Length( str );

	if ( length / 2 == size ) {
		sdAutoPtr< unsigned char, sdArrayCleanupPolicy< unsigned char > > out( new unsigned char[ size ] );
		int j = 0;
		for ( int i = 0; i < length; i += 2 ) {
			char c;
			if( str[ i ] > '9' ) {
				c = str[ i ] - 'A' + 0x0a;
			} else {
				c = str[ i ] - 0x30;
			}
			c <<= 4;
			if( str[ i + 1 ] > '9' ) {
				c |= str[ i + 1 ] - 'A' + 0x0a;
			} else {
				c |= str[ i + 1 ] - 0x30;
			}
			assert( j < ( size ) );
			out[ j++ ] = c;			
		}

		memcpy( pv, out.Get(), size );
		ret = true;
	} else {
		assert( !"Invalid size for binary string" );
	}
	return ret;
}

/*
============
idStr::IsValidEmailAddress
============
*/
bool idStr::IsValidEmailAddress( const char* address ) {
	int count = 0;
	const char* c;
	const char* domain;
	static const char* rfc822 = "()<>@,;:\\\"[]";

	// validate name
	for ( c = address; *c != '\0'; c++ ) {
		if ( *c == '\"' && ( c == address || *(c - 1) == '.' || *(c - 1) == '\"' ) ) {
			while ( *++c ) {
				if ( *c == '\"' ) {
					break;
				}
				if ( *c == '\\' && ( *++c == ' ' ) ) {
					continue;
				}
				if ( *c <= ' ' || *c >= 127 ) {
					return 0;
				}
			}
			if ( *c++ == '\0' ) {
				return false;
			}
			if ( *c == '@' ) {
				break;
			}
			if ( *c == '.' ) {
				return false;
			}
			continue;
		}
		if ( *c == '@' ) {
			break;
		}
		if ( *c <= ' ' || *c >= 127 ) {
			return false;
		}
		if ( FindChar( rfc822, *c ) != INVALID_POSITION ) {
			return false;
		}
	}

	if ( c == address || *(c - 1 ) == '.' ) {
		return false;
	}

	// validate domain
	if ( *( domain = ++c ) == '\0' ) {
		return false;
	}

	do {
		if ( *c == '.' ) {
			if ( c == domain || *(c - 1) == '.' ) {
				return false;
			}
			count++;
		}
		if ( *c <= ' ' || *c >= 127 ) {
			return false;
		}
		if ( FindChar( rfc822, *c ) != INVALID_POSITION ) {
			return false;
		}
	} while ( *++c );

	return ( count >= 1 );
}

/*
============
idStr::MS2HMS
============
*/
const char*	idStr::MS2HMS( double ms, const hmsFormat_t& formatSpec ) {
	if ( ms < 0.0 ) {
		ms = 0.0;
	}

	int sec = idMath::Ftoi( MS2SEC( ms ) );

	if( sec == 0 && formatSpec.showZeroSeconds == false ) {
		return "";
	}

	int min = sec / 60;
	int hour = min / 60;
		
	sec -= min * 60;
	min -= hour * 60;

	// don't show minutes if they're zeroed
	if( min == 0 && hour == 0 && formatSpec.showZeroMinutes == false && formatSpec.showZeroHours == false ) {
		return va( "%02i", sec );
	}

	// don't show hours if they're zeroed
	if( hour == 0 && formatSpec.showZeroHours == false ) {
		return va( "%02i:%02i", min, sec );
	}
	return va( "%02i:%02i:%02i", hour, min, sec );
}


/*
============
idStr::CollapseColors
============
*/
idStr& idStr::CollapseColors( void ) {
	int colorBegin = -1;
	int lastColor = -1;
	for( int i = 0; i < len; i++ ) {
		while( idStr::IsColor( &data[ i ] ) && i < len ) {
			if( colorBegin == -1 ) {
				colorBegin = i;
			}
			lastColor = i;
			i += 2;
		}
		if( colorBegin != -1 && lastColor != colorBegin ) {
			EraseRange( colorBegin, lastColor - colorBegin );
			i -= lastColor - colorBegin;
		}
		colorBegin = -1;
		lastColor = -1;
	}
	return *this;
}
