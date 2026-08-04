/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#if !defined( ID_REDIRECT_NEWDELETE ) && !defined( MACOS_X )
	//#define USE_STRING_DATA_ALLOCATOR // duzenko: idDynamicBlockAlloc is not thread-safe, crashes with r_smp 1
#endif

#ifdef USE_STRING_DATA_ALLOCATOR
static idDynamicBlockAlloc<char, 1<<18, 128>	stringDataAllocator;
#endif

idVec4	g_color_table[16] =
{
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(1.0f, 0.0f, 0.0f, 1.0f), // S_COLOR_RED
	idVec4(0.0f, 1.0f, 0.0f, 1.0f), // S_COLOR_GREEN
	idVec4(1.0f, 1.0f, 0.0f, 1.0f), // S_COLOR_YELLOW
	idVec4(0.0f, 0.0f, 1.0f, 1.0f), // S_COLOR_BLUE
	idVec4(0.0f, 1.0f, 1.0f, 1.0f), // S_COLOR_CYAN
	idVec4(1.0f, 0.0f, 1.0f, 1.0f), // S_COLOR_MAGENTA
	idVec4(1.0f, 1.0f, 1.0f, 1.0f), // S_COLOR_WHITE
	idVec4(0.5f, 0.5f, 0.5f, 1.0f), // S_COLOR_GRAY
	idVec4(0.0f, 0.0f, 0.0f, 1.0f), // S_COLOR_BLACK
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
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
idVec4 & idStr::ColorForIndex( int i ) {
	return g_color_table[ i & 15 ];
}

/*
============
idStr::ReAllocate
============
*/
void idStr::ReAllocate( int amount, bool keepold ) {
	//assert( data );
	assert( amount > 0 );

	alloced = unsigned( amount + (STR_ALLOC_GRAN - 1) ) / STR_ALLOC_GRAN * STR_ALLOC_GRAN;

	char	*newbuffer;
#ifdef USE_STRING_DATA_ALLOCATOR
	newbuffer = stringDataAllocator.Alloc( alloced );
#else
	newbuffer = new char[ alloced ];
#endif
	if ( keepold && data ) {
		data[ len ] = '\0';
		strcpy( newbuffer, data );
	}

	if ( data && data != baseBuffer ) {
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free( data );
#else
		delete [] data;
#endif
	}

	data = newbuffer;
}

/*
============
idStr::FreeData
============
*/
void idStr::FreeData( void ) {
	if ( data && data != baseBuffer ) {
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free( data );
#else
		delete[] data;
#endif
		data = baseBuffer;
	}
}

/*
============
idStr::operator=
============
*/
void idStr::operator=( const char *text ) {
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

		assert( strlen( text ) < (size_t)len );

		for ( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		}

		data[ i ] = '\0';

		len -= diff;

		return;
	}

    int l = static_cast<int>(strlen(text));
	EnsureAlloced( l + 1, false );
	strcpy( data, text );
	len = l;
}

idStr idStr::RandomPart( const float rand, const char c ) const {
	idStr part;

	// check for list and if found, use random part
	int seps = Count(c);
	if (seps > 0) {
		// if we have X commata, we have X+1 pieces, so select one at random
		seps ++;
		//gameLocal.Printf("Found random list with %i parts.\n", seps);
		
		int idx = (int) (rand * (float)seps);
		//gameLocal.Printf("random part #%i\n", idx);
		// split string into pieces, and select idx
		int i = 0; int d = 0;
		int start = 0; int end = len;
		while (d < len) {
			if (data[d] == c) {
					i++;
			}
			if (i == idx) {
				// found start, find end
				start = d;
				if (i > 0)
				{
					// on first part, start at 0, other parts skip "c"
					start ++;
				}
				end = start;
				while (end < len && data[end] != c)
				{
					end++;
				}
				break;
			}
			d++;
		}
		//gameLocal.Printf("Cutting %s between %i and %i.\n", data, start, end);
		part = Mid(start, end - start);
		// left-over separator 
		part.Strip(c);
		// and spaces
		part.Strip(' ');
		if (part == "''") {
			// default
			part = "";
		}
		//gameLocal.Printf("Result: '%s'.\n", part.c_str() );
	// end for random
	}
	else {
		// copy
		part.Append( data );
	}
	
	return part;
}

/*
============
idStr::CountChar

returns count of c between start and end
============
*/
int idStr::CountChar( const char *str, const char c, int start, int end ) {
	int i;
	int count = 0;

	if ( end == -1 ) {
		end = static_cast<int>(strlen( str )) - 1;
	}

	for ( i = start; i <= end; i++ ) {
		if ( str[i] == c ) {
			count++;
		}
	}
	return count;
}

/*
============
idStr::FindChar

returns -1 if not found otherwise the index of the char
============
*/
int idStr::FindChar( const char *str, const char c, int start, int end ) {
	int i;

	if ( end == -1 ) {
        end = static_cast<int>(strlen(str)) - 1;
	}
	for ( i = start; i <= end; i++ ) {
		if ( str[i] == c ) {
			return i;
		}
	}
	return -1;
}

/*
============
idStr::FindText

returns -1 if not found otherwise the index of the text
============
*/
int idStr::FindText( const char *str, const char *text, bool casesensitive, int start, int end ) {
	int l, i, j;

	if ( end == -1 ) {
        end = static_cast<int>(strlen(str));
	}
    l = end - static_cast<int>(strlen(text));
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
	return -1;
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
			buf.Clear();
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
				if ( index == -1 ) {
					return false;
				}
				name += index + strlen(buf);
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

	mediaName.Clear();

	for ( c = *name; c; c = *(++name) ) {
		// truncate at an extension
		if ( c == '.' ) {
			break;
		}
		// convert backslashes to forward slashes
		if ( c == '\\' ) {
			mediaName.Append( '/' );
		} else {
			mediaName.Append( idStr::ToLower( c ) );
		}
	}
}

/*
=============
idStr::IstartsWith
=============
*/
bool idStr::IstartsWith( const char *text, const char *prefix ) {
	return IcmpPrefix( text, prefix ) == 0;
}

/*
=============
idStr::IendsWith
=============
*/
bool idStr::IendsWith( const char *text, const char *suffix ) {
	int textLen = Length( text );
	int suffLen = Length( suffix );
	if ( suffLen > textLen )
		return false;
	int offset = textLen - suffLen;

	for ( int i = 0; i < suffLen; i++ ) {
		char c1 = text[offset + i];
		char c2 = suffix[i];
		if ( ToLower(c1) != ToLower(c2) )
			return false;
	}

	return true;
}

bool idStr::IstartsWith( const char *prefix ) const
{
	return IstartsWith( data, prefix );
}

bool idStr::IendsWith( const char *suffix ) const
{
	return IendsWith( data, suffix );
}

/*
=============
idStr::CheckExtension
=============
*/
bool idStr::CheckExtension( const char *name, const char *ext ) {
	assert( ext && ext[0] == '.' );
	const char *s1 = name + Length( name ) - 1;
	const char *s2 = ext + Length( ext ) - 1;
	int c1, c2, d;

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
	} while( s1 > name && s2 > ext );

	return ( s1 >= name );
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

	idStr::snPrintf( format, sizeof( format ), "%%.%df", precision );
	n = idStr::snPrintf( s, sizeof( str[0] ), format, array[0] );
	if ( precision > 0 ) {
		while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
		while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
	}
	idStr::snPrintf( format, sizeof( format ), " %%.%df", precision );
	for ( i = 1; i < length; i++ ) {
		n += idStr::snPrintf( s + n, sizeof( str[0] ) - n, format, array[i] );
		if ( precision > 0 ) {
			while( n > 0 && s[n-1] == '0' ) s[--n] = '\0';
			while( n > 0 && s[n-1] == '.' ) s[--n] = '\0';
		}
	}
	return s;
}

/*
============
idStr::Last

returns -1 if not found otherwise the index of the char
============
*/
int idStr::Last( const char c ) const {
	int i;
	
	for( i = Length(); i > 0; i-- ) {
		if ( data[ i - 1 ] == c ) {
			return i - 1;
		}
	}

	return -1;
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char c ) {
	// Tels: The string is zero-terminated, so exit if trying to remove zeros
	if (c == 0x00) {
		return;
	}
	// Tels: first count how many chars to remove, then move only once
	int remove = 0;
	while( data[ remove ] == c ) {
		remove ++;
	}
	len -= remove;
	// +1 to copy the 0x00 at the end
	memmove( &data[ 0 ], &data[ remove ], len + 1 );
}

/*
============
idStr::StripLeadingWhitespace
============
*/
void idStr::StripLeadingWhitespace( void ) {
	// Tels: first count how many chars to remove, then move the data only once
	int remove = 0;
	// cast to unsigned char to prevent stripping off high-ASCII characters
	while( data[remove] && (unsigned char)data[ remove ] <= ' ' ) {
		remove ++;
	}
	len -= remove;
	// +1 to copy the 0x00 at the end
	memmove( &data[ 0 ], &data[ remove ], len + 1 );
}
/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading( const char *string ) {
	
    int l = static_cast<int>(strlen(string));
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
	
    int l = static_cast<int>(strlen(string));
	if ( ( l > 0 ) && !Cmpn( string, l ) ) {
		memmove( data, data + l, len - l + 1 );
		len -= l;
		return true;
	}
	return false;
}

/*
============
idStr::StripLeadingOnce(char)

tels - more efficient than StripLeadingOnce(*char)
============
*/
bool idStr::StripLeadingOnce( const char c ) {
	// Tels: The string is zero-terminated, so ignore if trying to remove zeros
	if (c != 0x00 && len > 0 && data[0] == c)
	{
		// len => copy the 0x00 at the end
		memmove( &data[ 0 ], &data[ 1 ], len );
		len--;
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
	
    int l = static_cast<int>(strlen(string));
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
	
    int l = static_cast<int>(strlen(string));
	if ( ( l > 0 ) && ( len >= l ) && !Cmpn( string, data + len - l, l ) ) {
		len -= l;
		data[len] = '\0';
		return true;
	}
	return false;
}

/*
============
idStr::Replace
============
*/
void idStr::Replace( const char *old, const char *nw ) {
	int		i, j, count, oldStrLen;
	idStr	oldString( data );

	assert(old);

	// if passed NULL, treat it as Remove()
	if (!nw) { return Remove(old); }

    int oldLen = static_cast<int>(strlen(old));
    int newLen = static_cast<int>(strlen(nw));
	oldStrLen = oldString.Length();

	// Work out how big the new string will be
	count = 0;
	for( i = 0; i <= oldStrLen - oldLen; i++ ) {
		if( !idStr::Cmpn( &oldString[i], old, oldLen ) ) {
			count++;
			i += oldLen - 1;
		}
	}

	if( count ) {
		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		for( i = 0, j = 0; i < oldStrLen; i++ ) {
			if( (i <= oldStrLen - oldLen) && !idStr::Cmpn( &oldString[i], old, oldLen ) ) {
				memcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		// zero-terminate
		data[j] = 0;
		// set new length
		len = j;
	}
}

/*
============
idStr::Replace
============
*/
void idStr::Replace( const char old, const char nw ) {
	// cannot replace 0x00 or swap 0xXX to 0x00
	assert(old);
	assert(nw);

	for( int i = 0; i < len; i++ ) {
		if (data[i] == old) { data[i] = nw; }
	}
}

/*
============
idStr::Remove - Tels: works like Replace("string","");
============
*/
void idStr::Remove( const char *old ) {
	int		i, j;
	idStr	oldString( data );

	assert(old);

    int oldLen = static_cast<int>(strlen(old));

	// Remove the old data
	for( i = 0, j = 0; i < len; i++ ) {
		if( (i <= len - oldLen) && !idStr::Cmpn( &oldString[i], old, oldLen ) ) {
			// continue after the string to be removed
			i += oldLen - 1;
		} else {
			data[j] = oldString[i];
			j++;
		}
	}
	// zero-terminate
	data[j] = 0;
	// set new length
	len = j;
}

/*
============
idStr::Remove - Tels: removes all occurances of the given char
============
*/
void idStr::Remove( const char rem ) {
	int		i, j;

	idStr	oldString( data );

	// Remove the old data
	for( i = 0, j = 0; i < len; i++ ) {
		if (oldString[i] != rem) {
			data[j] = oldString[i]; j++;
		}
	}
	// zero-terminate
	data[j] = 0;
	// set new length
	len = j;
}

/*
============
idStr::Remap

Tels: table-driven remap (replace A w/ B, and B w/ C etc.) many chars simultanously. Used by I18N
to convert ISO 8859-1 etc. to our specific character set.
============
*/
void idStr::Remap( const unsigned int count, const char *table ) {
	assert(table);
	assert(count < 256);

	// ignore tables larger than that
	unsigned int num = count > 256 ? 512 : 2 * count;

	// this nested loop is still faster than calling Replace() "count" times when
	// count is large, esp. as this is correct, while repeated Replace() are not,
	// as you cannot then swap A and B (replacing in "AB" first all A with B and
	// then all B with A will end you up with "AA" instead of "BA").
	for (int c = 0; c < len; c++) {
		for (unsigned int i = 0; i < num; i += 2) {
			if (data[c] == table[i]) {
				data[c] = table[i+1];
				break; // abort the search once we find the replacement
			}
		}
	}
}

/*
============
idStr::Mid
============
*/
const char *idStr::Mid( const int start, const int len, idStr &result ) const {
	int i;

	result.Clear();

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return NULL;
	}

	int l = ( start + len >= i ) ? i - start : len;

	result.Append( &data[ start ], l );
	return result;
}

/*
============
idStr::Mid
============
*/
idStr idStr::Mid( const int start, const int len ) const {
	int i;
	idStr result;

	i = Length();
	if ( i == 0 || len <= 0 || start >= i ) {
		return result;
	}

	int l = ( start + len >= i ) ? i - start : len;

	result.Append( &data[ start ], l );
	return result;
}

/*
============
idStr::StripTrailingWhitespace
============
*/
void idStr::StripTrailingWhitespace( void ) {
	int i;
	
	// cast to unsigned char to prevent stripping off high-ASCII characters
	for( i = Length(); i > 0 && (unsigned char)(data[ i - 1 ]) <= ' '; i-- ) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripWhitespace

tels: Convenience is king.
============
*/
void idStr::StripWhitespace( void ) {
	StripLeadingWhitespace();
	StripTrailingWhitespace();
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
============
idStr::Split

stgatilov: Finds all characters matching "delimiters" list, returns array of substrings
between them (including substrings before the first delimiter and after the last one).
If "delimiters" is NULL, then whitespace characters are treated as delimiters.
If "skipEmpty" is true, then empty substrings are dropped from result.
============
*/
idList<idStr> idStr::Split( const char *delimiters, bool skipEmpty = false ) const
{
	if (!delimiters)
		delimiters = " \n\r\t\f\v";
	idList<idStr> tokens;

	int start = 0;
	for (int i = 0; i <= len; i++) {
		if (i == len || idStr::FindChar(delimiters, data[i]) >= 0) {
			int len = i - start;
			if (len > 0 || !skipEmpty)
				tokens.Append(idStr::Mid(start, len));
			start = i+1;
		}
	}
	assert(start == len+1);

	return tokens;
}

/*
============
idStr::Join

stgatilov: concatenates the given array of strings, putting "separator" between them.
============
*/
idStr idStr::Join( const idList<idStr> &tokens, const char *separator )
{
	if (tokens.Num() == 0)
		return "";

	idStr res = tokens[0];
	for (int i = 1; i < tokens.Num(); i++) {
		res += separator;
		res += tokens[i];
	}
	return res;
}

/*
============
idStr::Split

stgatilov: Finds all substrings matching "delimiters" list, returns array of substrings
between them (including substrings before the first delimiter and after the last one).
Note that delimiters matching is done greedily, and if several delimiters start as same position, the first one wins.
If "skipEmpty" is true, then empty substrings are dropped from result.
============
*/
idList<idStr> idStr::Split( const idList<idStr> &delimiters, bool skipEmpty ) const {
	idList<idStr> tokens;

	int start = 0;
	for (int i = 0; i <= len; ) {
		bool tokenEnds = false;
		int delimLen = 1;
		if (i == len)
			tokenEnds = true;
		else {
			for (int j = 0; j < delimiters.Num(); j++)
				if (Cmpn(data + i, delimiters[j].data, delimiters[j].len) == 0) {
					tokenEnds = true;
					delimLen = delimiters[j].len;
					break;
				}
		}
		if (tokenEnds) {
			int len = i - start;
			if (len > 0 || !skipEmpty)
				tokens.Append(idStr::Mid(start, len));
			i += delimLen;
			start = i;
		}
		else {
			i++;
		}
	}
	assert(start == len + 1);

	return tokens;
}

/*
============
idStr::SplitLines

stgatilov: Given contents of text file, returns list of its lines.
Works properly with Windows/Linux/MacOS style of EOL characters.
See also splitlines in Python.
============
*/
idList<idStr> idStr::SplitLines( void ) const {
	static idList<idStr> EOLS = {"\r\n", "\n", "\r"};
	idList<idStr> lines = Split( EOLS, false );

	//like in Python's splitlines, remove last line if it is empty
	//that's because text file must end with EOL, which does NOT start a new line
	if ( lines.Num() > 0 && lines[lines.Num() - 1].Length() == 0 )
		lines.Pop();

	return lines;
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
int idStr::FileNameHash( void ) const {
	int		i;
	int     hash;
	char	letter;

	hash = 0;
	i = 0;
	while( data[i] != '\0' ) {
		letter = idStr::ToLower( data[i] );
		if ( letter == '.' ) {
			break;				// don't include extension
		}
		if ( letter =='\\' ) {
			letter = '/';
		}
		hash += (int)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}


idStr &idStr::NormalizePath() {
	static const int MAX_STACK = 1024;
	int start[MAX_STACK], end[MAX_STACK];
	int cnt = 0;

	//always take leading slash 'as is'
	int outLen = 0;
	if (len > 0 && data[0] == '\\' || data[0] == '/')
		outLen++;

	int st = 0;
	for (int i = 0; i <= len; i++) {
		if (i == len || data[i] == '\\' || data[i] == '/') {
			if (i > st) {
				int clen = i - st;
				if (clen == 2 && data[st] == '.' && data[st + 1] == '.' && cnt > 0) {
					//remove last component, ignore this '..'
					cnt--;
					outLen -= (end[cnt] - start[cnt] + 1);
				}
				else {
					start[cnt] = st;
					end[cnt] = i;
					cnt++;
					int addLen = clen + (i < len);
					memmove(&data[outLen], &data[st], addLen);
					outLen += addLen;
				}
			}
			st = i + 1;
		}
	}
	data[outLen] = 0;

	return *this;
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

Strips the string of anything after the first dot, so this will
turn "test.exe.dat.1.03" into "test".
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
        EnsureAlloced(len + static_cast<int>(strlen(text)) + 2);

		if ( pos ) {
			if ( data[ pos-1 ] != '/' ) {
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
		dest.Clear();
	} else {
		Right( Length() - pos, dest );
	}
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
idStr::IcmpNoColor
================
*/
int idStr::IcmpNoColor( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		while ( idStr::IsColor( s1 ) ) {
			s1 += 2;
		}
		while ( idStr::IsColor( s2 ) ) {
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

	strncpy( dest, src, destsize-1 );
    dest[destsize-1] = 0;
}

/*
================
idStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void idStr::Append( char *dest, int size, const char *src ) {
	
    int l1 = static_cast<int>(strlen(dest));
	if ( l1 >= size ) {
		idLib::common->Error( "idStr::Append: already overflowed" );
	}
	idStr::Copynz( dest + l1, src, size - l1 );
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
		if ( idStr::IsColor( p ) ) {
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
		if ( idStr::IsColor( s ) ) {
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
idStr::snPrintf
================
*/
int idStr::snPrintf( char *dest, int size, const char *fmt, ...) {
	unsigned int len;
	va_list argptr;
	char buffer[32000];	// big, but small enough to fit in PPC stack

	assert(size > 0);

	va_start( argptr, fmt );
	len = vsprintf( buffer, fmt, argptr );
	va_end( argptr );
	if ( len >= sizeof( buffer ) ) {
		idLib::common->Error( "idStr::snPrintf: overflowed buffer" );
	}
	if ( len >= (unsigned int) size ) {
		idLib::common->Warning( "idStr::snPrintf: overflow of %i in %i", len, size );
		len = size;
	}
	idStr::Copynz( dest, buffer, size );
	return len;
}

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

	assert(size > 0);

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
	l = idStr::vsnPrintf( buffer, sizeof(buffer)-1, fmt, argptr );
	va_end( argptr );
	buffer[sizeof(buffer)-1] = '\0';

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
	
	l = idStr::vsnPrintf( buffer, sizeof(buffer)-1, fmt, argptr );
	buffer[sizeof(buffer)-1] = '\0';
	
	string = buffer;
	return l;
}

/*
============
va

does a varargs printf into a temp buffer
============
*/
char *va( const char *fmt, ... ) {
	va_list argptr;
	static thread_local int index = 0;
	static thread_local char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1) & 3;

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}



/*
============
idStr::BestUnit
============
*/
int idStr::BestUnit( const char *format, float value, Measure_t measure ) {
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
void idStr::SetUnit( const char *format, float value, int unit, Measure_t measure ) {
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
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Init();
#endif
}

/*
================
idStr::ShutdownMemory
================
*/
void idStr::ShutdownMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Shutdown();
#endif
}

/*
================
idStr::PurgeMemory
================
*/
void idStr::PurgeMemory( void ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.FreeEmptyBaseBlocks();
#endif
}

/*
================
idStr::ShowMemoryUsage_f
================
*/
void idStr::ShowMemoryUsage_f( const idCmdArgs &args ) {
#ifdef USE_STRING_DATA_ALLOCATOR
	idLib::common->Printf( "%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
		stringDataAllocator.GetBaseBlockMemory() >> 10, stringDataAllocator.GetFreeBlockMemory() >> 10,
			stringDataAllocator.GetNumFreeBlocks(), stringDataAllocator.GetNumEmptyBaseBlocks() );
#endif
}

/*
================
idStr::FormatNumber
================
*/
idStr idStr::FormatNumber( int64 number ) {
	// elements of list need to decend in size
	static const int64 formatList[] = {
		1000000000000LL, 1000000000, 1000000, 1000
	};
	static const int numFormatList = sizeof( formatList ) / sizeof( formatList[0] );

	// reset
	int counts[numFormatList] = {0};

	// main loop
	bool hit;

	do {
		hit = false;
		for ( int i = 0; i < numFormatList; i++ ) {
			if ( number >= formatList[i] ) {
				counts[i]++;
				number -= formatList[i];
				hit = true;
				break;
			}
		}
	} while ( hit );

	// print out
	bool found = false;
	idStr string;

	for ( int i = 0; i < numFormatList; i++ ) {
		if ( counts[i] ) {
			if ( !found ) {
				string += va( "%i,", counts[i] );
			} else {
				string += va( "%3.3i,", counts[i] );
			}
			found = true;
		}
		else if ( found ) {
			string += va( "%3.3i,", counts[i] );
		}
	}

	if ( found ) {
		string += va( "%3.3lli", number );
	}
	else {
		string += va( "%lli", number );
	}

	// pad to proper size
	int count = 14 - string.Length();

	for ( int i = 0; i < count; i++ ) {
		string.Insert( " ", 0 );
	}

	return string;
}

idStr idStr::Fmt( const char* fmt, ... ) {
	idStr res;
	va_list args;
	va_start( args, fmt );
	vsprintf( res, fmt, args );
	va_end( args );
	return res;
}