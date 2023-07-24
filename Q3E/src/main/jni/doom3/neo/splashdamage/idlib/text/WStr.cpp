// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include <wctype.h>

#if defined( MACOS_X )
#pragma GCC visibility push(hidden)
#endif
wideStringDataAllocator_t*	idWStr::stringDataAllocator;
bool						idWStr::stringAllocatorIsShared;

struct ShutdownStringAllocator {
	~ShutdownStringAllocator() {
		idWStr::ShutdownMemory();
	}
};
#if defined( MACOS_X )
#pragma GCC visibility pop
#endif

struct strColor_t;

#if defined( MACOS_X )
#pragma GCC visibility push(hidden)
#endif
static ShutdownStringAllocator shutdownStringAllocator;
#if defined( MACOS_X )
#pragma GCC visibility pop
#endif
idWStr::hmsFormat_t	idWStr::defaultHMSFormat;

extern dword g_dword_color_table[COLOR_BITS+1];
/*
============
idWStr::ColorForIndex
============
*/
const idVec4& idWStr::ColorForIndex( int i ) {
	return idStr::ColorForIndex( i );
}

/*
============
idWStr::ColorForChar
============
*/
const idVec4& idWStr::ColorForChar( wchar_t c ) {
	return idStr::ColorForIndex( ColorIndex( c ) );
}

/*
============
idWStr::StrForColorIndex
============
*/
const char* idWStr::StrForColorIndex( int i ) {
	return idStr::StrForColorIndex( i );
}

/*
============
idWStr::DColorForIndex
============
*/
dword& idWStr::DColorForIndex( int i ) {
	return g_dword_color_table[ i & W_COLOR_BITS ];
}

/*
============
idWStr::ColorForChar
============
*/
dword& idWStr::DColorForChar( wchar_t c ) {
 	return g_dword_color_table[ ColorIndex( c ) ];
}

/*
============
idWStr::ReAllocate
============
*/
void idWStr::ReAllocate( int amount, bool keepold ) {
	wchar_t	*newbuffer;
	int		newsize;
	int		mod;

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
			wcsncpy( newbuffer, data, len );
			newbuffer[ len ] = L'\0';
		} else {
			newbuffer[0] = L'\0';
		}
	}

	if ( data && data != baseBuffer ) {
		stringDataAllocator->Free( data );
	}

	data = newbuffer;
}

/*
============
idWStr::FreeData
============
*/
void idWStr::FreeData( void ) {
	if ( data && data != baseBuffer ) {
		stringDataAllocator->Free( data );
		data = baseBuffer;
	}
}

/*
============
idWStr::operator=
============
*/
void idWStr::operator=( const wchar_t *text ) {
	int l;
	int diff;
	int i;

	if ( text == NULL ) {
		// safe behaviour if NULL
		EnsureAlloced( 1, false );
		data[ 0 ] = L'\0';
		len = 0;
		return;
	}

	if ( text == data ) {
		return; // copying same thing
	}

	// check if we're aliasing
	if ( text >= data && text <= data + len ) {
		diff = text - data;

		assert( idWStr::Length( text ) < len );

		for ( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		}

		data[ i ] = L'\0';

		len -= diff;

		return;
	}

	l = Length( text );
	EnsureAlloced( l + 1, false );
	wcscpy( data, text );
	len = l;
}

/*
============
idWStr::FindChar

returns INVALID_POSITION if not found otherwise the index of the char
============
*/
int idWStr::FindChar( const wchar_t *str, const wchar_t c, int start, int end ) {
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
idWStr::FindText

returns INVALID_POSITION if not found otherwise the index of the text
============
*/
int idWStr::FindText( const wchar_t *str, const wchar_t *text, bool casesensitive, int start, int end ) {
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
				if ( towupper( str[i+j] ) != towupper( text[j] ) ) {
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
idWStr::ReplaceChar
============
*/
void idWStr::ReplaceChar( wchar_t oldChar, wchar_t newChar ) {
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
idWStr::Replace
============
*/
void idWStr::Replace( const wchar_t *old, const wchar_t *nw ) {
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
		idWStr	oldString( data );
		int		j;

		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		for ( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if( !Cmpn( &oldString[i], old, oldLen ) ) {
				wmemcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		data[j] = L'\0';
		len = Length( data );
	}
}

/*
============
idWStr::ReplaceFirst
============
*/
void idWStr::ReplaceFirst( const wchar_t *old, const wchar_t *nw ) {
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
		idWStr	oldString( data );
		int		j;

		EnsureAlloced( len + ( newLen - oldLen ) + 2, false );

		// Replace the old data with the new data
		for ( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if ( !Cmpn( &oldString[i], old, oldLen ) ) {
				wmemcpy( data + j, nw, newLen );
				i += oldLen;
				j += newLen;
				break;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		::wmemcpy( data + j, &oldString[i], oldString.Length() - i );
		data[j + oldString.Length() - i] = L'\0';
		len = Length( data );
	}
}

/*
============
idWStr::Mid
============
*/
const wchar_t *idWStr::Mid( int start, int len, idWStr& result ) const {
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
	return result.c_str();
}

/*
============
idWStr::Mid
============
*/
idWStr idWStr::Mid( int start, int len ) const {
	int i;
	idWStr result;

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
=====================================================================

  char * methods to replace library functions

=====================================================================
*/

/*
================
idWStr::StripFilename
================
*/
wchar_t* idWStr::StripFilename( wchar_t* string ) {
	int pos;
	
	pos = idWStr::Length( string ) - 1;
	while( ( pos > 0 ) && ( string[ pos ] != L'/' ) && ( string[ pos ] != L'\\' ) ) {
		pos--;
	}

	if ( pos < 0 ) {
		pos = 0;
	}

	string[ pos ] = L'\0';

	return string;
}

/*
================
idWStr::Cmp
================
*/
int idWStr::Cmp( const wchar_t *s1, const wchar_t *s2 ) {
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
idWStr::Cmpn
================
*/
int idWStr::Cmpn( const wchar_t *s1, const wchar_t *s2, int n ) {
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
idWStr::Icmp
================
*/
int idWStr::Icmp( const wchar_t *s1, const wchar_t *s2 ) {
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
idWStr::Icmpn
================
*/
int idWStr::Icmpn( const wchar_t *s1, const wchar_t *s2, int n ) {
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
idWStr::Icmp
================
*/
int idWStr::IcmpNoColor( const wchar_t *s1, const wchar_t *s2 ) {
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
=============
idWStr::Copynz
 
Safe strncpy that ensures a trailing zero
=============
*/
void idWStr::Copynz( wchar_t *dest, const wchar_t *src, int destsize ) {
	if ( src == NULL ) {
		idLib::common->Warning( "idWStr::Copynz: NULL src" );
		return;
	}
	if ( destsize < 1 ) {
		idLib::common->Warning( "idWStr::Copynz: destsize < 1" ); 
		return;
	}

	::wcsncpy( dest, src, destsize-1 );
    dest[destsize-1] = L'\0';
}

/*
================
idWStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void idWStr::Append( wchar_t *dest, int size, const wchar_t *src ) {
	int		l1;

	l1 = Length( dest );
	if ( l1 >= size ) {
		idLib::common->Error( "idWStr::Append: already overflowed" );
	}
	Copynz( dest + l1, src, size - l1 );
}

/*
================
idWStr::LengthWithoutColors
================
*/
int idWStr::LengthWithoutColors( const wchar_t *s ) {
	int len;
	const wchar_t *p;

	if ( !s ) {
		return 0;
	}

	len = 0;
	p = s;
	while( *p != L'\0' ) {
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
idWStr::RemoveColors
================
*/
wchar_t *idWStr::RemoveColors( wchar_t *string ) {
	wchar_t *d;
	wchar_t *s;
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
	*d = L'\0';

	return string;
}

/*
================
idWStr::snPrintf
================
*/
int idWStr::snPrintf( wchar_t *dest, int size, const wchar_t *fmt, ...) {
	int ret;
	va_list argptr;

#ifdef _WIN32
#undef _vsnwprintf
	va_start( argptr, fmt );
	ret = _vsnwprintf( dest, size-1, fmt, argptr );
	va_end( argptr );
#define _vsnprintf	use_idStr_vsnPrintf
#else
	// there is a vswprintf( idWStr ), so no #undef
	va_start( argptr, fmt );
	ret = vswprintf( dest, size, fmt, argptr );
	va_end( argptr );
#endif
	dest[size-1] = L'\0';

#ifdef _DEBUG
	// never pass a %s formatting, always use %hs or %ls (%s works only on windows)
	if ( FindText( fmt, L"%s" ) != INVALID_POSITION ) {
		common->Error( "idWStr::snPrintf: attempted to pass a non-portable format string" );
	}
#endif
	if ( ret < 0 || ret >= size ) {
		return -1;
	}
	return ret;
}

/*
============
idWStr::vsnPrintf

see idStr::vsnPrintf
same _WIN32 vs rest of the world for _vsnwprintf/vswprintf

you can call vsnPrintf( buffer, size ) with wchar_t buffer[size]
always has a terminating null character after call
will return -1 on error or overflow
============
*/
int idWStr::vsnPrintf( wchar_t *dest, int size, const wchar_t *fmt, va_list argptr ) {
	int ret;

#ifdef _WIN32
#undef _vsnwprintf
	ret = _vsnwprintf( dest, size-1, fmt, argptr );
#define _vsnwprintf	use_idWStr_vsnPrintf
#else
	// there is a vswprintf( idWStr ), so no #undef
	ret = vswprintf( dest, size, fmt, argptr );
#endif
	dest[size-1] = L'\0';

#ifdef _DEBUG
	// never pass a %s formatting, always use %hs or %ls (%s works only on windows)
	if ( FindText( fmt, L"%s" ) != INVALID_POSITION ) {
		common->Error( "idWStr::vsnPrintf: attempted to pass a non-portable format string" );
	}
#endif
	if ( ret < 0 || ret >= size ) {
		return -1;
	}
	return ret;
}

/*
===============
idWStr::Test
FIXME: not working correctly on !_WIN32. investigate
===============
*/
void idWStr::Test( void ) {
	wchar_t	buffer[10];
	int ret;
	idWStr test;

	idLib::common->Printf( "idWStr::Test\n" );

	idWStr::Copynz( buffer, L"012345678", 10 );
	assert( buffer[9] == L'\0' );

	ret = test.snPrintf( buffer, 10, L"%ls", L"876543210" );
	assert( buffer[9] == L'\0' );
#ifdef _WIN32
	OutputDebugStringW( va( L"%d %ls\n", ret, buffer ) );
#else
	printf( "%d %ls\n", ret, buffer );
#endif

	ret = test.snPrintf( buffer, 10, L"%ls", L"0123456789" );
	assert( buffer[9] == '\0' );
#ifdef _WIN32
	OutputDebugStringW( va( L"%d %ls\n", ret, buffer ) );
#else
	printf( "%d %ls\n", ret, buffer );
#endif

	ret = test.snPrintf( buffer, 10, L"%hs", "towide" );
#ifdef _WIN32
	OutputDebugStringW( va( L"%d %ls\n", ret, buffer ) );
#else
	printf( "%d %ls\n", ret, buffer );
#endif
}

/*
============
swprintf

Sets the value of the string using a printf interface.
============
*/
int swprintf( idWStr &string, const wchar_t *fmt, ... ) {
	static const int BUFFER_SIZE = 32000;
	int l;
	va_list argptr;
	wchar_t buffer[BUFFER_SIZE];
	
	va_start( argptr, fmt );
	l = idWStr::vsnPrintf( buffer, BUFFER_SIZE, fmt, argptr );
	va_end( argptr );

	string = buffer;
	return l;
}

/*
============
vswprintf

Sets the value of the string using a vprintf interface.
============
*/
int vswprintf( idWStr &string, const wchar_t *fmt, va_list argptr ) {
	static const int BUFFER_SIZE = 32000;
	int l;
	wchar_t buffer[BUFFER_SIZE];
	
	l = idWStr::vsnPrintf( buffer, BUFFER_SIZE, fmt, argptr );
	
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
#define VA_BUF_LEN 16384
wchar_t *va( const wchar_t *fmt, ... ) {
	va_list argptr;
	static int index = 0;
	static wchar_t string[4][VA_BUF_LEN];	// in case called by nested functions
	wchar_t *buf;

	buf = string[index];
	index = (index + 1) & 3;

	va_start( argptr, fmt );
	idWStr::vsnPrintf( buf, VA_BUF_LEN, fmt, argptr );
	va_end( argptr );

	return buf;
}

/*
================
idWStr::InitMemory
================
*/
void idWStr::InitMemory( void ) {
	if( !stringDataAllocator ) {
		stringDataAllocator = new wideStringDataAllocator_t;
		stringDataAllocator->Init();
		stringAllocatorIsShared = false;
	}	
}

/*
================
idWStr::ShutdownMemory
================
*/
void idWStr::ShutdownMemory( void ) {
	if( stringDataAllocator && !stringAllocatorIsShared ) {
		stringDataAllocator->Shutdown();
		delete stringDataAllocator;
		stringDataAllocator = NULL;
	}
}

/*
================
idWStr::PurgeMemory
================
*/
void idWStr::PurgeMemory( void ) {
	stringDataAllocator->FreeEmptyBaseBlocks();
}

/*
================
idWStr::ShowMemoryUsage_f
================
*/
void idWStr::ShowMemoryUsage_f( const idCmdArgs &args ) {
	idLib::common->Printf( "%6d KB wide string memory (%d KB free in %d blocks, %d empty base blocks)\n",
		stringDataAllocator->GetBaseBlockMemory() >> 10, stringDataAllocator->GetFreeBlockMemory() >> 10,
		stringDataAllocator->GetNumFreeBlocks(), stringDataAllocator->GetNumEmptyBaseBlocks() );
}


/*
============
idWStr::SetStringAllocator
============
*/
void idWStr::SetStringAllocator( wideStringDataAllocator_t* allocator ) {
	if( !stringAllocatorIsShared ) {
		delete stringDataAllocator;
	}
	stringDataAllocator = allocator;
	stringAllocatorIsShared = true;
}

/*
============
idWStr::GetStringAllocator
============
*/
wideStringDataAllocator_t* idWStr::GetStringAllocator( void ) {
	return stringDataAllocator;
}

/*
============
idWStr::EraseRange
============
*/
void idWStr::EraseRange( int start, int len ) {
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


	if ( totalLength - start - len ) {
		::wmemmove( &data[ start ], &data[ start + len ], totalLength - start - len );
	}
	
	data[ totalLength - len ] = L'\0';
	this->len -= len;
}


/*
============
idWStr::EraseChar
============
*/
void idWStr::EraseChar( const wchar_t c, int start ) {
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
		data[ start ] = L'\0';
		start--;
	}
	len = start + 1;
}


/*
============
idWStr::Append
============
*/
void idWStr::Append( int count, const wchar_t c ) {
	EnsureAlloced( len + count + 1 );
	int start = len;
	int end = len + count;
	while( start < end ) {
		data[ start ] = c;
		start++;
	}
	data[ start ] = L'\0';
	len += count;
}

/*
============
idWStr::MS2HMS
============
*/
const wchar_t* idWStr::MS2HMS( double ms, const hmsFormat_t& formatSpec ) {
	if ( ms < 0.0 ) {
		ms = 0.0;
	}

	int sec = idMath::Ftoi( MS2SEC( ms ) );
	if( sec == 0 && formatSpec.showZeroSeconds == false ) {
		return L"";
	}

	int min = sec / 60;
	int hour = min / 60;

	sec -= min * 60;
	min -= hour * 60;

	// don't show minutes if they're zeroed
	if( min == 0 && hour == 0 && formatSpec.showZeroMinutes == false && formatSpec.showZeroHours == false ) {
		return va( L"%02i", sec );
	}

	// don't show hours if they're zeroed
	if( hour == 0 && formatSpec.showZeroHours == false ) {
		return va( L"%02i:%02i", min, sec );
	}
	return va( L"%02i:%02i:%02i", hour, min, sec );
}


/*
============
idWStr::CollapseColors
============
*/
idWStr& idWStr::CollapseColors( void ) {
	int colorBegin = -1;
	int lastColor = -1;
	for( int i = 0; i < len; i++ ) {
		while( idWStr::IsColor( &data[ i ] ) && i < len ) {
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

/*
============
idWStr::StripFileExtension
============
*/
idWStr &idWStr::StripFileExtension( void ) {
	int i;

	for ( i = len-1; i >= 0; i-- ) {
		if ( data[i] == L'/' || data[i] == L'\\' ) {
			break;
		}
		if ( data[i] == L'.' ) {
			data[i] = L'\0';
			len = i;
			break;
		}
	}
	return *this;
}
