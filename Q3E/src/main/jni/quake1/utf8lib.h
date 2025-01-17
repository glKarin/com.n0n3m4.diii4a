/*
Copyright (C) 2009-2020 DarkPlaces contributors

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

/*
 * UTF-8 utility functions for DarkPlaces
 */
#ifndef UTF8LIB_H__
#define UTF8LIB_H__

#include <stddef.h>
#include "qtypes.h"

// types for unicode strings
// let them be 32 bit for now
// normally, whcar_t is 16 or 32 bit, 16 on linux I think, 32 on haiku and maybe windows

// Uchar, a wide character
typedef int32_t Uchar;

// Initialize UTF8, this registers cvars which allows for UTF8 to be disabled
// completely.
// When UTF8 is disabled, every u8_ function will work exactly as you'd expect
// a non-utf8 version to work: u8_strlen() will wrap to strlen()
// u8_byteofs() and u8_charidx() will simply return whatever is passed as index parameter
// u8_getchar() will will just return the next byte, u8_fromchar will write one byte, ...
extern struct cvar_s utf8_enable;
void   u8_Init(void);

size_t u8_strlen(const char*);
size_t u8_strnlen(const char*, size_t);
int    u8_byteofs(const char*, size_t, size_t*);
int    u8_charidx(const char*, size_t, size_t*);
size_t u8_bytelen(const char*, size_t);
size_t u8_prevbyte(const char*, size_t);
Uchar  u8_getchar_utf8_enabled(const char*, const char**);
Uchar  u8_getnchar_utf8_enabled(const char*, const char**, size_t);
int    u8_fromchar(Uchar, char*, size_t);
size_t u8_mbstowcs(Uchar *, const char *, size_t);
size_t u8_wcstombs(char*, const Uchar*, size_t);
size_t u8_COM_StringLengthNoColors(const char *s, size_t size_s, qbool *valid);

// returns a static buffer, use this for inlining
char  *u8_encodech(Uchar ch, size_t*, char*buf16);

size_t u8_strpad(char *out, size_t outsize, const char *in, qbool leftalign, size_t minwidth, size_t maxwidth);
size_t u8_strpad_colorcodes(char *out, size_t outsize, const char *in, qbool leftalign, size_t minwidth, size_t maxwidth);

/* Careful: if we disable utf8 but not freetype, we wish to see freetype chars
 * for normal letters. So use E000+x for special chars, but leave the freetype stuff for the
 * rest:
 */
extern Uchar u8_quake2utf8map[256];
// these defines get a bit tricky, as c and e may be aliased to the same variable
#define u8_getchar(c,e) (utf8_enable.integer ? u8_getchar_utf8_enabled(c,e) : (u8_quake2utf8map[(unsigned char)(*(e) = (c) + 1)[-1]]))
#define u8_getchar_noendptr(c) (utf8_enable.integer ? u8_getchar_utf8_enabled(c,NULL) : (u8_quake2utf8map[(unsigned char)*(c)]))
#define u8_getchar_check(c,e) ((e) ? u8_getchar((c),(e)) : u8_getchar_noendptr((c)))
#define u8_getnchar(c,e,n) (utf8_enable.integer ? u8_getnchar_utf8_enabled(c,e,n) : ((n) <= 0 ? ((*(e) = c), 0) : (u8_quake2utf8map[(unsigned char)(*(e) = (c) + 1)[-1]])))
#define u8_getnchar_noendptr(c,n) (utf8_enable.integer ? u8_getnchar_utf8_enabled(c,NULL,n) : ((n) <= 0 ? 0  : (u8_quake2utf8map[(unsigned char)*(c)])))
#define u8_getnchar_check(c,e,n) ((e) ? u8_getchar((c),(e),(n)) : u8_getchar_noendptr((c),(n)))

Uchar u8_toupper(Uchar ch);
Uchar u8_tolower(Uchar ch);

#ifdef WIN32

// WTF-8 encoding to circumvent Windows encodings, be it UTF-16 or random codepages
// https://simonsapin.github.io/wtf-8/

typedef wchar_t wchar;

// whether to regard wchar as utf-32
// sizeof(wchar_t) is 2 for win32, we don't have sizeof in macros
#define WTF8U32 0
// check for extra sanity in conversion steps
#define WTF8CHECKS 1

int towtf8(const wchar* wstr, int wlen, char* str, int maxlen);
int fromwtf8(const char* str, int len, wchar* wstr, int maxwlen);
int wstrlen(const wchar* wstr);

// helpers for wchar code
/* convert given wtf-8 encoded char *str to wchar *wstr, only on win32 */
#define WIDE(str, wstr) fromwtf8(str, strlen(str), wstr, strlen(str))
/* convert given wchar *wstr to wtf-8 encoded char *str, only on win32 */
#define NARROW(wstr, str) towtf8(wstr, wstrlen(wstr), str, wstrlen(wstr) * (WTF8U32 ? 4 : 3))

#else

#define WIDE(str, wstr) ;
#define NARROW(wstr, str) ;

#endif // WIN32

#endif // UTF8LIB_H__
