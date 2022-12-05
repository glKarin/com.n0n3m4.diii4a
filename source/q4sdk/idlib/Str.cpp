
#include "precompiled.h"
#pragma hdrstop

// TTimo - don't do anything funky if you're on a real OS ;-)
#if defined(_WINDOWS) || defined(_XENON)

// RAVEN BEGIN
// jsinger: attempt at removing the DLL cross boundary issue
// mwhitlock: Dynamic memory consolidation - may want to consier making a totally separate string allocator
#if ( !defined(ID_REDIRECT_NEWDELETE) && !defined(RV_UNIFIED_ALLOCATOR) ) || defined(_RV_MEM_SYS_SUPPORT)
// RAVEN END
	#define USE_STRING_DATA_ALLOCATOR
#endif

#endif

#ifdef USE_STRING_DATA_ALLOCATOR
static idDynamicBlockAlloc<char, 1<<18, 128, MA_STRING>	stringDataAllocator;
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
// RAVEN BEGIN
// bdube: console color
	idVec4(0.94f, 0.62f, 0.05f, 1.0f),	// S_COLOR_CONSOLE
// RAVEN END	
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

// P means the character is Polish
// C means the character is Czech

const bool idStr::printableCharacter[256] =
{
//
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
//	       !      "      #      $      &      %      '      (      )      *      +      ,      -      .      /
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  0      1      2      3      4      5      6      7      8      9      :      ;      <      =      >      ?
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  @      A      B      C      D      E      F      G      H      I      J      K      L      M      N      O
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  P      Q      R      S      T      U      V      W      X      Y      Z      [      \      ]      ^      _
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  `      a      b      c      d      e      f      g      h      i      j      k      l      m      n      o
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  p      q      r      s      t      u      v      w      x      y      z      {      |      }      ~
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  false,
//  Ä             X                                                                                                                            S  C                      å P         T C         Z C          Z P
	true,  true,  false, false, false, false, false, false, false, false, true,  false, true,  true,  true,  true,
//                                                                 ô      s C           ú P    t C    z C    z P 
	false, false, false, false, false, false, false, false, false, true,  true,  false, true,  true,  true,  true,
//         °             £ P    §      • P           ß             ©             ´                    Æ        P
	false, true,  false, true,  true,  true,  false, true,  false, true,  false, true,  false, false, true,  true,
//  ∞                    £ P    ¥      µ             ∑               P           ª                           ø P
	true,  false, false, true,  true,  true,  false, true,  false, true,  false, true,  false, false, false, true,
//  ¿      ¡ C    ¬      √      ƒ      ≈      ∆ P    «      » C    … C      P    À      Ã C    Õ C    Œ      œ C
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  –      — P    “ C    ” PC   ‘      ’      ÷      ◊      ÿ C    Ÿ C    ⁄ C    €      ‹      › C    ﬁ      ﬂ
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//  ‡      · C    ‚      „      ‰      Â      Ê P    Á      Ë C    È C    Í P    Î      Ï C    Ì C    Ó      Ô C
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
//        Ò P    Ú C    Û PC   Ù      ı      ˆ      ˜      ¯ C    ˘ C    ˙ C    ˚      ¸      ˝ C    ˛      ˇ
	true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
};

const char idStr::upperCaseCharacter[256] =
{
//
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//	     !    "    #    $    &    %    '    (    )    *    +    ,    -    .    /
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//  0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//  @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
	0,   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//  P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0,   0,   0,   0,   0,
//  `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
	0,   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
//  p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0,   0,   0,   0,   0,
//  Ä         X                                                                                        S                   å      T        Z
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   'ä', 0,   'å', 'ç', 'é', 'é',
//                                               TM   s         ú    t    z 
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   'ä', 0,   'å', 'ç', 'é', 'è',
//       °         £    §    •         ß         ©         ´              Æ
	0,   0,   0,   '£', 0,   '•', 0,   0,   0,   0,   0,   0,   0,   0,   0,   'ø',
//  ∞                   ¥    µ         ∑                   ª                   ø
	0,   0,   0,   '£', 0,   0,   0,   0,   0,   '•', 0,   0,   0,   0,   0,   'ø',
//  ¿    ¡    ¬    √    ƒ    ≈    ∆    «    »    …         À    Ã    Õ    Œ    œ
	'¿', '¡', '¬', '√', 'ƒ', '≈', '∆', '«', '»', '…', ' ', 'À', 'Ã', 'Õ', 'Œ', 'œ',
//  –    —    “    ”    ‘    ’    ÷    ◊    ÿ    Ÿ    ⁄    €    ‹    ›    ﬁ    ﬂ
	'–', '—', '“', '”', '‘', '’', '÷', 0,   'ÿ', 'Ÿ', '⁄', '€', '‹', '›', 'ﬁ', 'ﬂ',
//  ‡    ·    ‚    „    ‰    Â    Ê    Á    Ë    È    Í    Î    Ï    Ì    Ó    Ô
	'¿', '¡', '¬', '√', 'ƒ', '≈', '∆', '«', '»', '…', ' ', 'À', 'Ã', 'Õ', 'Œ', 'œ',
//      Ò    Ú    Û    Ù    ı    ˆ    ˜    ¯    ˘    ˙    ˚    ¸    ˝    ˛    ˇ
	'–', '—', '“', '”', '‘', '’', '÷', 0,   'ÿ', 'Ÿ', '⁄', '€', '‹', '›', 'ﬁ', 'ﬂ',
};

const char idStr::lowerCaseCharacter[256] =
{
//
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//	     !    "    #    $    &    %    '    (    )    *    +    ,    -    .    /
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//  0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
//  @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
	0,   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
//  P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0,   0,   0,   0,   0,
//  `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
	0,   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
//  p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0,   0,   0,   0,   0,
//  Ä         X                                                                                        S                   å      T        Z
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   'ö', 0,   'ú', 'ù', 'û', 'ü',
//                                               TM   s         ú    t    z 
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   'ö', 0,   'ú', 'ù', 'û', 'ü',
//       °         £    §    •         ß         ©         ´              Æ
	0,   0,   0,   '≥', 0,   'π', 0,   0,   0,   0,   0,   0,   0,   0,   0,   'Ø',
//  ∞                   ¥    µ         ∑                   ª                   ø
	0,   0,   0,   '≥', 0,   0,   0,   0,   0,   'π', 0,   0,   0,   0,   0,   'Ø',
//  ¿    ¡    ¬    √    ƒ    ≈    ∆    «    »    …         À    Ã    Õ    Œ    œ
	'‡', '·', '‚', '„', '‰', 'Â', 'Ê', 'Á', 'Ë', 'È', 'Í', 'Î', 'Ï', 'Ì', 'Ó', 'Ô',
//  –    —    “    ”    ‘    ’    ÷    ◊    ÿ    Ÿ    ⁄    €    ‹    ›    ﬁ    ﬂ
	'', 'Ò', 'Ú', 'Û', 'Ù', 'ı', 'ˆ', 0,   '¯', '˘', '˙', '˚', '¸', '˝', '˛', 'ﬂ',
//  ‡    ·    ‚    „    ‰    Â    Ê    Á    Ë    È    Í    Î    Ï    Ì    Ó    Ô
	'‡', '·', '‚', '„', '‰', 'Â', 'Ê', 'Á', 'Ë', 'È', 'Í', 'Î', 'Ï', 'Ì', 'Ó', 'Ô',
//      Ò    Ú    Û    Ù    ı    ˆ    ˜    ¯    ˘    ˙    ˚    ¸    ˝    ˛    ˇ
	'', 'Ò', 'Ú', 'Û', 'Ù', 'ı', 'ˆ', 0,   '¯', '˘', '˙', '˚', '¸', '˝', '˛', 'ﬂ',
};
// RAVEN END

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
	char	*newbuffer;
	int		newsize;
	int		mod;

	//assert( data );
	assert( amount > 0 );

	mod = amount % STR_ALLOC_GRAN;
	if ( !mod ) {
		newsize = amount;
	}
	else {
		newsize = amount + STR_ALLOC_GRAN - mod;
	}
	alloced = newsize;

#ifdef USE_STRING_DATA_ALLOCATOR

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_PERMANENT);
#endif
// RAVEN END

	newbuffer = stringDataAllocator.Alloc( alloced );

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	RV_POP_HEAP();
#endif
// RAVEN END

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
// RAVEN BEGIN
// jnewquist: Ignore free request if allocator is empty, probably shutdown
		if ( stringDataAllocator.GetNumUsedBlocks() > 0 ) {
			stringDataAllocator.Free( data );
		}
// RAVEN END
#else
		delete[] data;
#endif

// RAVEN BEGIN
// jsinger: was exhibiting a buffer overrun when an idStr was contained in an idList
// due to having the wrong alloced value.  This corrects that
		Init();
// RAVEN END
	}
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

		assert( strlen( text ) < (unsigned)len );

		for ( i = 0; text[ i ]; i++ ) {
			data[ i ] = text[ i ];
		}

		data[ i ] = '\0';

		len -= diff;

		return;
	}

	l = strlen( text );
	EnsureAlloced( l + 1, false );
	strcpy( data, text );
	len = l;
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
		end = strlen( str ) - 1;
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
		end = strlen( str );
	}
	l = end - strlen( text );
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
			mediaName.Append( idStr::ToLower( c ) );
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

	l = strlen( string );
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

	l = strlen( string );
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

	l = strlen( string );
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

	l = strlen( string );
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
// RAVEN BEGIN
// scork: this needs to return an int of the replacement count, like MS CString etc, otherwise you can't do things like this:
//
// idStr str;
//  while (str.Replace("\n\n","\n")) {};
//
// ... to remove blank lines from an output string if you have 3 blank lines in a row. Without the while(), you'd just be guessing about
//	converting (eg) 3 blank lines to 2, then 2 to 1, depending on how many times you straight-line the Replace() call.
//
int idStr::Replace( const char *old, const char *nw ) {
	int		iReplaced = 0;
	int		oldLen, newLen, i, j, count;
	idStr	oldString( data );

	oldLen = strlen( old );
	newLen = strlen( nw );

	// Work out how big the new string will be
	count = 0;
	for( i = 0; i < oldString.Length(); i++ ) {
		if( !idStr::Cmpn( &oldString[i], old, oldLen ) ) {
			count++;
			i += oldLen - 1;
		}
	}

	if( count ) {
		EnsureAlloced( len + ( ( newLen - oldLen ) * count ) + 2, false );

		// Replace the old data with the new data
		for( i = 0, j = 0; i < oldString.Length(); i++ ) {
			if( !idStr::Cmpn( &oldString[i], old, oldLen ) ) {
				memcpy( data + j, nw, newLen );
				i += oldLen - 1;
				j += newLen;
				iReplaced++;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}
		data[j] = 0;
		len = strlen( data );
	}
	return iReplaced;
}
// RAVEN END

/*
============
idStr::Mid
============
*/
const char *idStr::Mid( int start, int len, idStr &result ) const {
	int i;

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

// RAVEN BEGIN
/*
============
idStr::StripUntil
============
*/
void idStr::StripUntil( const char c )
{
	while( data[ 0 ] != c && len ) {
		memmove( &data[ 0 ], &data[ 1 ], len );
		len--;
	}
}
// RAVEN END

/*
=====================================================================

  info strings

=====================================================================
*/

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
int idStr::FileNameHash( void ) const {
	int		i;
	long	hash;
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
		hash += (long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
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

// RAVEN BEGIN
// jscott: for file systems that require backslashes
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

// nmckenzie: char replacing routine

/*
============
idStr::SlashesToBackSlashes
============
*/

bool idStr::HasChar( const char check ){
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[ i ] == check ) {
			return true;
		}
	}
	return false;
}

/*
============
idStr::HasChars
============
*/

bool idStr::HasChars( const char *check ){
	int i;

	while( *check ){
		for ( i = 0; i < len; i++ ) {
			if ( data[ i ] == *check ) {
				return true;
			}
		}
		check++;
	}
	return false;
}

/*
============
idStr::ReplaceChar
============
*/

idStr &idStr::ReplaceChar( const char from, const char to ) {
	int i;

	for ( i = 0; i < len; i++ ) {
		if ( data[ i ] == from ) {
			data[ i ] = to;
		}
	}
	return *this;
}

/*
============
idStr::ReplaceChars
============
*/

idStr &idStr::ReplaceChars( const char *from, const char to ) {
	int i;

	while( *from ){
		for ( i = 0; i < len; i++ ) {
			if ( data[ i ] == *from ) {
				data[ i ] = to;
			}
		}
		from++;
	}
	return *this;
}

// RAVEN END

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
		EnsureAlloced( len + strlen( text ) + 2 );

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
// RAVEN BEGIN
// jscott: for getting the file base out of addnormals() style filenames
	while( ( pos < Length() ) && ( ( *this )[ pos ] != '.' ) && ( ( *this )[ pos ] != ',' ) ) {
// RAVEN END
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

// RAVEN BEGIN
// twhitaker: Turns a bad file name into a good one or your money back
void idStr::ScrubFileName( void )
{
	int i;

	RemoveEscapes();
	StripFileExtension();

	for ( i = 0; i < len; i++ ) {
		if( !idStr::upperCaseCharacter[data[i]] && !isdigit(data[i]) ) {
			data[i] = '_';
		}
	}
}

// jscott: like the declManager version, but globally accessable
void idStr::MakeNameCanonical( void )
{
	ToLower();
	BackSlashesToSlashes();
	StripFileExtension();
}

void idStr::EnsurePrintable( void ) {

	int		i;

	for( i = 0; i < len; i++ ) {

		if( !CharIsPrintable( data[i] ) ) {

			memmove( &data[i], &data[i + 1], len - i );
			len--;
		}
	}
}
// RAVEN END

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
		if ( !isdigit( ( byte )s[i] ) ) {
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
// RAVEN BEGIN
// bdube: escape codes
int idStr::IcmpNoEscape ( const char *s1, const char *s2 ) {
	int c1, c2, d;

	do {
		for ( d = idStr::IsEscape( s1 ); d; d = idStr::IsEscape( s1 ) ) {
			s1 += d;
		}
		for ( d = idStr::IsEscape( s2 ); d; d = idStr::IsEscape( s2 ) ) {
			s2 += d;
		}
// RAVEN END
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
	int		l1;

	l1 = strlen( dest );
	if ( l1 >= size ) {
		idLib::common->Error( "idStr::Append: already overflowed" );
	}
	idStr::Copynz( dest + l1, src, size - l1 );
}

// bdube: escape codes
/*
================
idStr::LengthWithoutEscapes
================
*/
int idStr::LengthWithoutEscapes( const char *s ) {
	int len;
	const char *p;

	if ( !s ) {
		return 0;
	}

	len = 0;
	p = s;
	while( *p ) {
		int esc;
		esc = idStr::IsEscape ( p );
		if ( esc ) {
			p += esc;
			continue;
		}
		p++;
		len++;
	}

	return len;
}

/*
================
idStr::RemoveEscapes
================
*/
char *idStr::RemoveEscapes( char *string, int escapes ) {
	char *d;
	char *s;
	int c;

	s = string;
	d = string;
	while( (c = *s) != 0 ) {
		int esc;
		int type;
		esc = idStr::IsEscape( s, &type );
		if ( esc && (type & escapes) ) {
			s += esc;
			continue;
		}		
		else {
			*d++ = c;
			if ( c == C_COLOR_ESCAPE && *(s+1) ) {
				s++;
			}
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
================
idStr::IsEscape
================
*/
int idStr::IsEscape( const char *s, int* type )  {
	if ( !s || *s != C_COLOR_ESCAPE || *(s+1) == C_COLOR_ESCAPE ) {
		return 0;
	}
	if ( type ) {
		*type = S_ESCAPE_UNKNOWN;
	}
	switch ( *(s+1) ) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case ':':
			if ( type ) {
				*type = S_ESCAPE_COLORINDEX;
			}
			return 2;

		case '-': case '+':
			if ( type ) {
				*type = S_ESCAPE_COLOR;
			}
			return 2;
			
		case 'r': case 'R': 
			if ( type ) {			
				*type = S_ESCAPE_COMMAND;
			}
			return 2;
			
		case 'c': case 'C':
			if ( *(s+2) ) {
				if ( *(s+3) ) {
					if ( *(s+4) ) {
						if ( type ) {
							*type = S_ESCAPE_COLOR;
						}
						return 5;
					}
				}
			}
			return 0;
			
		case 'n': case 'N':
			if ( type ) {
				*type = S_ESCAPE_COMMAND;
			}
			if ( *(s+2) ) {
				return 3;
			}
			return 0;
			
		case 'i': case 'I':
			if ( *(s+2) && *(s+3) && *(s+4) ) {
				if ( type ) {
					*type = S_ESCAPE_ICON;
				}
				return 5;
			}
			return 0;		
	}			
	return 0;
}

// RAVEN END

/*
================
idStr::snPrintf
================
*/
int idStr::snPrintf( char *dest, int size, const char *fmt, ...) {
	int len;
	va_list argptr;
	char buffer[32000];	// big, but small enough to fit in PPC stack

	va_start( argptr, fmt );
	len = vsprintf( buffer, fmt, argptr );
	va_end( argptr );
	if ( len >= sizeof( buffer ) ) {
		idLib::common->Error( "idStr::snPrintf: overflowed buffer" );
	}
	if ( len >= size ) {
		idLib::common->Warning( "idStr::snPrintf: overflow of %i in %i\n", len, size );
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
NOTE: not thread safe
============
*/
char *va( const char *fmt, ... ) {
	va_list argptr;
	static int index = 0;
// RAVEN BEGIN
// scork: tweaked from 4 to 8, since one of my funcs uses it twice. Better safe than sorry
	static char string[VA_NUM_BUFS][VA_BUF_LEN];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1) & 7;
// RAVEN END

	va_start( argptr, fmt );
	vsprintf( buf, fmt, argptr );
	va_end( argptr );

	return buf;
}

#ifdef _WIN32
/*
============
fe

Used for formatting windows errors
NOTE: not thread safe
============
*/
// RAVEN BEGIN
// abahr
char *fe( int errorId ) {
	static int index = 0;
	static char string[4][256];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1) & 3;
#ifndef _XBOX
	FormatMessage( 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorId,
		0, // Default language
		(LPTSTR) buf,
		256,
		NULL 
	);
#endif
	return buf;
}
#endif
// RAVEN END

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
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_STRING);
// RAVEN END
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
struct formatList_t {
	int			gran;
	int			count;
};

// elements of list need to decend in size
formatList_t formatList[] = {
	{ 1000000000, 0 },
	{ 1000000, 0 },
	{ 1000, 0 }
};

int numFormatList = sizeof(formatList) / sizeof( formatList[0] );


idStr idStr::FormatNumber( int number ) {
	idStr string;
	bool hit;

	// reset
	for ( int i = 0; i < numFormatList; i++ ) {
		formatList_t *li = formatList + i;
		li->count = 0;
	}

	// main loop
	do {
		hit = false;

		for ( int i = 0; i < numFormatList; i++ ) {
			formatList_t *li = formatList + i;

			if ( number >= li->gran ) {
				li->count++;
				number -= li->gran;
				hit = true;
				break;
			}
		}
	} while ( hit );

	// print out
	bool found = false;

	for ( int i = 0; i < numFormatList; i++ ) {
		formatList_t *li = formatList + i;

		if ( li->count ) {
			if ( !found ) {
				string += va( "%i,", li->count );
			} else {
				string += va( "%3.3i,", li->count );
			}
			found = true;
		}
		else if ( found ) {
			string += va( "%3.3i,", li->count );
		}
	}

	if ( found ) {
		string += va( "%3.3i", number );
	}
	else {
		string += va( "%i", number );
	}

	// pad to proper size
	int count = 11 - string.Length();

	for ( int i = 0; i < count; i++ ) {
		string.Insert( " ", 0 );
	}

	return string;
}

// RAVEN BEGIN
// abahr
/*
================
idStr::Split
================
*/
void idStr::Split( const char* source, idList<idStr>& list, const char delimiter, const char groupDelimiter  ) {
	const idStr localSource( source );
	int sourceLength = localSource.Length();
	idStr element;
	int startIndex = 0;
	int endIndex = -1;
	char currentChar = '\0';

	list.Clear();
	while( startIndex < sourceLength ) {
		currentChar = localSource[ startIndex ];
		if( currentChar == groupDelimiter ) {
			endIndex = localSource.Find( groupDelimiter, ++startIndex );
			if( endIndex == -1 ) {
				common->Error( "Couldn't find expected char %c in idStr::Split\n", groupDelimiter );
			}
			element = localSource.Mid( startIndex, endIndex );
			element.Strip( groupDelimiter );
			list.Append( element );
			element.Clear();
			startIndex = endIndex + 1;
			continue;
		} else if( currentChar == delimiter ) {
			element += '\0';
			list.Append( element );
			element.Clear();
			endIndex = ++startIndex;
			continue;
		}

		startIndex++;
		element += currentChar;
	}

	if( element.Length() ) {
		element += '\0';
		list.Append( element );
	}
}

/*
================
idStr::Split
================
*/
void idStr::Split( idList<idStr>& list, const char delimiter, const char groupDelimiter ) {
	Split( c_str(), list, delimiter, groupDelimiter );
}
// RAVEN END

idStr idStr::GetLastColorCode( void ) const {
	for ( int i = Length(); i > 0; i-- ) {
		int escapeType = 0;
		int escapeLength = idStr::IsEscape( &data[i-1], &escapeType );

		if ( escapeLength && ( escapeType == S_ESCAPE_COLORINDEX || S_ESCAPE_COLOR ) ) {
			idStr result = "";
			result.Append( &data[i-1], escapeLength );
			return result;
		}
	}

	return "";
} 
