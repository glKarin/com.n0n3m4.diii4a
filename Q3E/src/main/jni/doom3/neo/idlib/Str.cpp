/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

// dhewm/dhewm3
// DG: idDynamicBlockAlloc isn't thread-safe and idStr is used both in the main thread
//     and the async thread! For some reason this seems to cause lots of problems on
//     newer Linux distros if dhewm3 is built with GCC9 or newer (see #391).
//     No idea why it apparently didn't cause that (noticeable) issues before..
#if 0 // !defined(_RAVEN) //k: quake4 not work
#if !defined( ID_REDIRECT_NEWDELETE ) && !defined( MACOS_X )
#define USE_STRING_DATA_ALLOCATOR
#endif
#endif

#ifdef USE_STRING_DATA_ALLOCATOR
static idDynamicBlockAlloc<char, 1<<18, 128>	stringDataAllocator;
#endif

idVec4	g_color_table[16] = {
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
#ifdef _RAVEN
// RAVEN BEGIN
// bdube: console color
	idVec4(0.94f, 0.62f, 0.05f, 1.0f),	// S_COLOR_CONSOLE
// RAVEN END
#else
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
#endif
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
	idVec4(0.0f, 0.0f, 0.0f, 1.0f),
};

const char *units[2][4] = {
	{ "B", "KB", "MB", "GB" },
	{ "B/s", "KB/s", "MB/s", "GB/s" }
};

/*
============
idStr::ColorForIndex
============
*/
idVec4 &idStr::ColorForIndex(int i)
{
	return g_color_table[ i & 15 ];
}

/*
============
idStr::ReAllocate
============
*/
void idStr::ReAllocate(int amount, bool keepold)
{
	char	*newbuffer;
	int		newsize;
	int		mod;

	//assert( data );
	assert(amount > 0);

	mod = amount % STR_ALLOC_GRAN;

	if (!mod) {
		newsize = amount;
	} else {
		newsize = amount + STR_ALLOC_GRAN - mod;
	}

	alloced = newsize;

#ifdef USE_STRING_DATA_ALLOCATOR
	newbuffer = stringDataAllocator.Alloc(alloced);
#else
	newbuffer = new char[ alloced ];
#endif

	if (keepold && data) {
		data[ len ] = '\0';
		strcpy(newbuffer, data);
	}

	if (data && data != baseBuffer) {
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free(data);
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
void idStr::FreeData(void)
{
	if (data && data != baseBuffer) {
#ifdef USE_STRING_DATA_ALLOCATOR
		stringDataAllocator.Free(data);
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
void idStr::operator=(const char *text)
{
	int l;
	int diff;
	int i;

	if (!text) {
		// safe behaviour if NULL
		EnsureAlloced(1, false);
		data[ 0 ] = '\0';
		len = 0;
		return;
	}

	if (text == data) {
		return; // copying same thing
	}

	// check if we're aliasing
	if (text >= data && text <= data + len) {
		diff = text - data;

		assert(strlen(text) < (unsigned)len);

		for (i = 0; text[ i ]; i++) {
			data[ i ] = text[ i ];
		}

		data[ i ] = '\0';

		len -= diff;

		return;
	}

	l = strlen(text);
	EnsureAlloced(l + 1, false);
	strcpy(data, text);
	len = l;
}

/*
============
idStr::FindChar

returns -1 if not found otherwise the index of the char
============
*/
int idStr::FindChar(const char *str, const char c, int start, int end)
{
	int i;

	if (end == -1) {
		end = strlen(str) - 1;
	}

	for (i = start; i <= end; i++) {
		if (str[i] == c) {
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
int idStr::FindText(const char *str, const char *text, bool casesensitive, int start, int end)
{
	int l, i, j;

	if (end == -1) {
		end = strlen(str);
	}

	l = end - strlen(text);

	for (i = start; i <= l; i++) {
		if (casesensitive) {
			for (j = 0; text[j]; j++) {
				if (str[i+j] != text[j]) {
					break;
				}
			}
		} else {
			for (j = 0; text[j]; j++) {
				if (::toupper(str[i+j]) != ::toupper(text[j])) {
					break;
				}
			}
		}

		if (!text[j]) {
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
bool idStr::Filter(const char *filter, const char *name, bool casesensitive)
{
	idStr buf;
	int i, found, index;

	while (*filter) {
		if (*filter == '*') {
			filter++;
			buf.Empty();

			for (i = 0; *filter; i++) {
				if (*filter == '*' || *filter == '?' || (*filter == '[' && *(filter+1) != '[')) {
					break;
				}

				buf += *filter;

				if (*filter == '[') {
					filter++;
				}

				filter++;
			}

			if (buf.Length()) {
				index = idStr(name).Find(buf.c_str(), casesensitive);

				if (index == -1) {
					return false;
				}

				name += index + strlen(buf);
			}
		} else if (*filter == '?') {
			filter++;
			name++;
		} else if (*filter == '[') {
			if (*(filter+1) == '[') {
				if (*name != '[') {
					return false;
				}

				filter += 2;
				name++;
			} else {
				filter++;
				found = false;

				while (*filter && !found) {
					if (*filter == ']' && *(filter+1) != ']') {
						break;
					}

					if (*(filter+1) == '-' && *(filter+2) && (*(filter+2) != ']' || *(filter+3) == ']')) {
						if (casesensitive) {
							if (*name >= *filter && *name <= *(filter+2)) {
								found = true;
							}
						} else {
							if (::toupper(*name) >= ::toupper(*filter) && ::toupper(*name) <= ::toupper(*(filter+2))) {
								found = true;
							}
						}

						filter += 3;
					} else {
						if (casesensitive) {
							if (*filter == *name) {
								found = true;
							}
						} else {
							if (::toupper(*filter) == ::toupper(*name)) {
								found = true;
							}
						}

						filter++;
					}
				}

				if (!found) {
					return false;
				}

				while (*filter) {
					if (*filter == ']' && *(filter+1) != ']') {
						break;
					}

					filter++;
				}

				filter++;
				name++;
			}
		} else {
			if (casesensitive) {
				if (*filter != *name) {
					return false;
				}
			} else {
				if (::toupper(*filter) != ::toupper(*name)) {
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
void idStr::StripMediaName(const char *name, idStr &mediaName)
{
	char c;

	mediaName.Empty();

	for (c = *name; c; c = *(++name)) {
		// truncate at an extension
		if (c == '.') {
			break;
		}

		// convert backslashes to forward slashes
		if (c == '\\') {
			mediaName.Append('/');
		} else {
			mediaName.Append(idStr::ToLower(c));
		}
	}
}

/*
=============
idStr::CheckExtension
=============
*/
bool idStr::CheckExtension(const char *name, const char *ext)
{
	const char *s1 = name + Length(name) - 1;
	const char *s2 = ext + Length(ext) - 1;
	int c1, c2, d;

	do {
		c1 = *s1--;
		c2 = *s2--;

		d = c1 - c2;

		while (d) {
			if (c1 <= 'Z' && c1 >= 'A') {
				d += ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c2 <= 'Z' && c2 >= 'A') {
				d -= ('a' - 'A');

				if (!d) {
					break;
				}
			}

			return false;
		}
	} while (s1 > name && s2 > ext);

	return (s1 >= name);
}

/*
=============
idStr::FloatArrayToString
=============
*/
const char *idStr::FloatArrayToString(const float *array, const int length, const int precision)
{
	static int index = 0;
	static char str[4][16384];	// in case called by nested functions
	int i, n;
	char format[16], *s;

	// use an array of string so that multiple calls won't collide
	s = str[ index ];
	index = (index + 1) & 3;

	idStr::snPrintf(format, sizeof(format), "%%.%df", precision);
	n = idStr::snPrintf(s, sizeof(str[0]), format, array[0]);

	if (precision > 0) {
		while (n > 0 && s[n-1] == '0') s[--n] = '\0';

		while (n > 0 && s[n-1] == '.') s[--n] = '\0';
	}

	idStr::snPrintf(format, sizeof(format), " %%.%df", precision);

	for (i = 1; i < length; i++) {
		n += idStr::snPrintf(s + n, sizeof(str[0]) - n, format, array[i]);

		if (precision > 0) {
			while (n > 0 && s[n-1] == '0') s[--n] = '\0';

			while (n > 0 && s[n-1] == '.') s[--n] = '\0';
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
int idStr::Last(const char c) const
{
	int i;

	for (i = Length(); i > 0; i--) {
		if (data[ i - 1 ] == c) {
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
void idStr::StripLeading(const char c)
{
	while (data[ 0 ] == c) {
		memmove(&data[ 0 ], &data[ 1 ], len);
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripLeading(const char *string)
{
	int l;

	l = strlen(string);

	if (l > 0) {
		while (!Cmpn(string, l)) {
			memmove(data, data + l, len - l + 1);
			len -= l;
		}
	}
}

/*
============
idStr::StripLeadingOnce
============
*/
bool idStr::StripLeadingOnce(const char *string)
{
	int l;

	l = strlen(string);

	if ((l > 0) && !Cmpn(string, l)) {
		memmove(data, data + l, len - l + 1);
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
void idStr::StripTrailing(const char c)
{
	int i;

	for (i = Length(); i > 0 && data[ i - 1 ] == c; i--) {
		data[ i - 1 ] = '\0';
		len--;
	}
}

/*
============
idStr::StripLeading
============
*/
void idStr::StripTrailing(const char *string)
{
	int l;

	l = strlen(string);

	if (l > 0) {
		while ((len >= l) && !Cmpn(string, data + len - l, l)) {
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
bool idStr::StripTrailingOnce(const char *string)
{
	int l;

	l = strlen(string);

	if ((l > 0) && (len >= l) && !Cmpn(string, data + len - l, l)) {
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
void idStr::Replace(const char *old, const char *nw)
{
	int		oldLen, newLen, i, j, count;
	idStr	oldString(data);

	oldLen = strlen(old);
	newLen = strlen(nw);

	// Work out how big the new string will be
	count = 0;

	for (i = 0; i < oldString.Length(); i++) {
		if (!idStr::Cmpn(&oldString[i], old, oldLen)) {
			count++;
			i += oldLen - 1;
		}
	}

	if (count) {
		EnsureAlloced(len + ((newLen - oldLen) * count) + 2, false);

		// Replace the old data with the new data
		for (i = 0, j = 0; i < oldString.Length(); i++) {
			if (!idStr::Cmpn(&oldString[i], old, oldLen)) {
				memcpy(data + j, nw, newLen);
				i += oldLen - 1;
				j += newLen;
			} else {
				data[j] = oldString[i];
				j++;
			}
		}

		data[j] = 0;
		len = strlen(data);
	}
}

/*
============
idStr::Mid
============
*/
const char *idStr::Mid(int start, int len, idStr &result) const
{
	int i;

	result.Empty();

	i = Length();

	if (i == 0 || len <= 0 || start >= i) {
		return NULL;
	}

	if (start + len >= i) {
		len = i - start;
	}

	result.Append(&data[ start ], len);
	return result;
}

/*
============
idStr::Mid
============
*/
idStr idStr::Mid(int start, int len) const
{
	int i;
	idStr result;

	i = Length();

	if (i == 0 || len <= 0 || start >= i) {
		return result;
	}

	if (start + len >= i) {
		len = i - start;
	}

	result.Append(&data[ start ], len);
	return result;
}

/*
============
idStr::StripTrailingWhitespace
============
*/
void idStr::StripTrailingWhitespace(void)
{
	int i;

	// cast to unsigned char to prevent stripping off high-ASCII characters
	for (i = Length(); i > 0 && (unsigned char)(data[ i - 1 ]) <= ' '; i--) {
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
idStr &idStr::StripQuotes(void)
{
	if (data[0] != '\"') {
		return *this;
	}

	// Remove the trailing quote first
	if (data[len-1] == '\"') {
		data[len-1] = '\0';
		len--;
	}

	// Strip the leading quote now
	len--;
	memmove(&data[ 0 ], &data[ 1 ], len);
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
int idStr::FileNameHash(void) const
{
	int		i;
	/* 64long */int	hash;
	char	letter;

	hash = 0;
	i = 0;

	while (data[i] != '\0') {
		letter = idStr::ToLower(data[i]);

		if (letter == '.') {
			break;				// don't include extension
		}

		if (letter =='\\') {
			letter = '/';
		}

		hash += (/* 64long */int)(letter)*(i+119);
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
idStr &idStr::BackSlashesToSlashes(void)
{
	int i;

	for (i = 0; i < len; i++) {
		if (data[ i ] == '\\') {
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
idStr &idStr::SetFileExtension(const char *extension)
{
	StripFileExtension();

	if (*extension != '.') {
		Append('.');
	}

	Append(extension);
	return *this;
}

/*
============
idStr::StripFileExtension
============
*/
idStr &idStr::StripFileExtension(void)
{
	int i;

	for (i = len-1; i >= 0; i--) {
		if (data[i] == '.') {
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
idStr &idStr::StripAbsoluteFileExtension(void)
{
	int i;

	for (i = 0; i < len; i++) {
		if (data[i] == '.') {
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
idStr &idStr::DefaultFileExtension(const char *extension)
{
	int i;

	// do nothing if the string already has an extension
	for (i = len-1; i >= 0; i--) {
		if (data[i] == '.') {
			return *this;
		}
	}

	if (*extension != '.') {
		Append('.');
	}

	Append(extension);
	return *this;
}

/*
==================
idStr::DefaultPath
==================
*/
idStr &idStr::DefaultPath(const char *basepath)
{
	if (((*this)[ 0 ] == '/') || ((*this)[ 0 ] == '\\')) {
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
void idStr::AppendPath(const char *text)
{
	int pos;
	int i = 0;

	if (text && text[i]) {
		pos = len;
		EnsureAlloced(len + strlen(text) + 2);

		if (pos) {
			if (data[ pos-1 ] != '/') {
				data[ pos++ ] = '/';
			}
		}

		if (text[i] == '/') {
			i++;
		}

		for (; text[ i ]; i++) {
			if (text[ i ] == '\\') {
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
idStr &idStr::StripFilename(void)
{
	int pos;

	pos = Length() - 1;

	while ((pos > 0) && ((*this)[ pos ] != '/') && ((*this)[ pos ] != '\\')) {
		pos--;
	}

	if (pos < 0) {
		pos = 0;
	}

	CapLength(pos);
	return *this;
}

/*
==================
idStr::StripPath
==================
*/
idStr &idStr::StripPath(void)
{
	int pos;

	pos = Length();

	while ((pos > 0) && ((*this)[ pos - 1 ] != '/') && ((*this)[ pos - 1 ] != '\\')) {
		pos--;
	}

	*this = Right(Length() - pos);
	return *this;
}

/*
====================
idStr::ExtractFilePath
====================
*/
void idStr::ExtractFilePath(idStr &dest) const
{
	int pos;

	//
	// back up until a \ or the start
	//
	pos = Length();

	while ((pos > 0) && ((*this)[ pos - 1 ] != '/') && ((*this)[ pos - 1 ] != '\\')) {
		pos--;
	}

	Left(pos, dest);
}

/*
====================
idStr::ExtractFileName
====================
*/
void idStr::ExtractFileName(idStr &dest) const
{
	int pos;

	//
	// back up until a \ or the start
	//
	pos = Length() - 1;

	while ((pos > 0) && ((*this)[ pos - 1 ] != '/') && ((*this)[ pos - 1 ] != '\\')) {
		pos--;
	}

	Right(Length() - pos, dest);
}

/*
====================
idStr::ExtractFileBase
====================
*/
void idStr::ExtractFileBase(idStr &dest) const
{
	int pos;
	int start;

	//
	// back up until a \ or the start
	//
	pos = Length() - 1;

	while ((pos > 0) && ((*this)[ pos - 1 ] != '/') && ((*this)[ pos - 1 ] != '\\')) {
		pos--;
	}

	start = pos;

	while ((pos < Length()) && ((*this)[ pos ] != '.')) {
		pos++;
	}

	Mid(start, pos - start, dest);
}

/*
====================
idStr::ExtractFileExtension
====================
*/
void idStr::ExtractFileExtension(idStr &dest) const
{
	int pos;

	//
	// back up until a . or the start
	//
	pos = Length() - 1;

	while ((pos > 0) && ((*this)[ pos - 1 ] != '.')) {
		pos--;
	}

	if (!pos) {
		// no extension
		dest.Empty();
	} else {
		Right(Length() - pos, dest);
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
bool idStr::IsNumeric(const char *s)
{
	int		i;
	bool	dot;

	if (*s == '-') {
		s++;
	}

	dot = false;

	for (i = 0; s[i]; i++) {
		if (!isdigit(s[i])) {
			if ((s[ i ] == '.') && !dot) {
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
bool idStr::HasLower(const char *s)
{
	if (!s) {
		return false;
	}

	while (*s) {
		if (CharIsLower(*s)) {
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
bool idStr::HasUpper(const char *s)
{
	if (!s) {
		return false;
	}

	while (*s) {
		if (CharIsUpper(*s)) {
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
int idStr::Cmp(const char *s1, const char *s2)
{
	int c1, c2, d;

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;

		if (d) {
			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
================
idStr::Cmpn
================
*/
int idStr::Cmpn(const char *s1, const char *s2, int n)
{
	int c1, c2, d;

	assert(n >= 0);

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;

		if (d) {
			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::Icmp(const char *s1, const char *s2)
{
	int c1, c2, d;

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;

		while (d) {
			if (c1 <= 'Z' && c1 >= 'A') {
				d += ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c2 <= 'Z' && c2 >= 'A') {
				d -= ('a' - 'A');

				if (!d) {
					break;
				}
			}

			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
================
idStr::Icmpn
================
*/
int idStr::Icmpn(const char *s1, const char *s2, int n)
{
	int c1, c2, d;

	assert(n >= 0);

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;

		while (d) {
			if (c1 <= 'Z' && c1 >= 'A') {
				d += ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c2 <= 'Z' && c2 >= 'A') {
				d -= ('a' - 'A');

				if (!d) {
					break;
				}
			}

			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
================
idStr::Icmp
================
*/
int idStr::IcmpNoColor(const char *s1, const char *s2)
{
	int c1, c2, d;

	do {
		while (idStr::IsColor(s1)) {
			s1 += 2;
		}

		while (idStr::IsColor(s2)) {
			s2 += 2;
		}

		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;

		while (d) {
			if (c1 <= 'Z' && c1 >= 'A') {
				d += ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c2 <= 'Z' && c2 >= 'A') {
				d -= ('a' - 'A');

				if (!d) {
					break;
				}
			}

			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
================
idStr::IcmpPath
================
*/
int idStr::IcmpPath(const char *s1, const char *s2)
{
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	idLib::common->Printf("WARNING: IcmpPath used on a case-sensitive filesystem?\n");
#endif

	do {
		c1 = *s1++;
		c2 = *s2++;

		d = c1 - c2;

		while (d) {
			if (c1 <= 'Z' && c1 >= 'A') {
				d += ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c1 == '\\') {
				d += ('/' - '\\');

				if (!d) {
					break;
				}
			}

			if (c2 <= 'Z' && c2 >= 'A') {
				d -= ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c2 == '\\') {
				d -= ('/' - '\\');

				if (!d) {
					break;
				}
			}

			// make sure folders come first
			while (c1) {
				if (c1 == '/' || c1 == '\\') {
					break;
				}

				c1 = *s1++;
			}

			while (c2) {
				if (c2 == '/' || c2 == '\\') {
					break;
				}

				c2 = *s2++;
			}

			if (c1 && !c2) {
				return -1;
			} else if (!c1 && c2) {
				return 1;
			}

			// same folder depth so use the regular compare
			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;
}

/*
================
idStr::IcmpnPath
================
*/
int idStr::IcmpnPath(const char *s1, const char *s2, int n)
{
	int c1, c2, d;

#if 0
//#if !defined( _WIN32 )
	idLib::common->Printf("WARNING: IcmpPath used on a case-sensitive filesystem?\n");
#endif

	assert(n >= 0);

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		d = c1 - c2;

		while (d) {
			if (c1 <= 'Z' && c1 >= 'A') {
				d += ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c1 == '\\') {
				d += ('/' - '\\');

				if (!d) {
					break;
				}
			}

			if (c2 <= 'Z' && c2 >= 'A') {
				d -= ('a' - 'A');

				if (!d) {
					break;
				}
			}

			if (c2 == '\\') {
				d -= ('/' - '\\');

				if (!d) {
					break;
				}
			}

			// make sure folders come first
			while (c1) {
				if (c1 == '/' || c1 == '\\') {
					break;
				}

				c1 = *s1++;
			}

			while (c2) {
				if (c2 == '/' || c2 == '\\') {
					break;
				}

				c2 = *s2++;
			}

			if (c1 && !c2) {
				return -1;
			} else if (!c1 && c2) {
				return 1;
			}

			// same folder depth so use the regular compare
			return (INTSIGNBITNOTSET(d) << 1) - 1;
		}
	} while (c1);

	return 0;
}

/*
=============
idStr::Copynz

Safe strncpy that ensures a trailing zero
=============
*/
void idStr::Copynz(char *dest, const char *src, int destsize)
{
	if (!src) {
		idLib::common->Warning("idStr::Copynz: NULL src");
		return;
	}

	if (destsize < 1) {
		idLib::common->Warning("idStr::Copynz: destsize < 1");
		return;
	}

	strncpy(dest, src, destsize-1);
	dest[destsize-1] = 0;
}

/*
================
idStr::Append

  never goes past bounds or leaves without a terminating 0
================
*/
void idStr::Append(char *dest, int size, const char *src)
{
	int		l1;

	l1 = strlen(dest);

	if (l1 >= size) {
		idLib::common->Error("idStr::Append: already overflowed");
	}

	idStr::Copynz(dest + l1, src, size - l1);
}

/*
================
idStr::LengthWithoutColors
================
*/
int idStr::LengthWithoutColors(const char *s)
{
	int len;
	const char *p;

	if (!s) {
		return 0;
	}

	len = 0;
	p = s;

	while (*p) {
		if (idStr::IsColor(p)) {
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
char *idStr::RemoveColors(char *string)
{
	char *d;
	char *s;
	int c;

	s = string;
	d = string;

	while ((c = *s) != 0) {
		if (idStr::IsColor(s)) {
			s++;
		} else {
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
int idStr::snPrintf(char *dest, int size, const char *fmt, ...)
{
	int len;
	va_list argptr;
	char buffer[32000];	// big, but small enough to fit in PPC stack

	va_start(argptr, fmt);
	len = vsprintf(buffer, fmt, argptr);
	va_end(argptr);

	if (len >= sizeof(buffer)) {
		idLib::common->Error("idStr::snPrintf: overflowed buffer");
	}

	if (len >= size) {
		idLib::common->Warning("idStr::snPrintf: overflow of %i in %i\n", len, size);
		len = size;
	}

	idStr::Copynz(dest, buffer, size);
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
int idStr::vsnPrintf(char *dest, int size, const char *fmt, va_list argptr)
{
	int ret;

#ifdef _WIN32
#undef _vsnprintf
	ret = _vsnprintf(dest, size-1, fmt, argptr);
#define _vsnprintf	use_idStr_vsnPrintf
#else
#undef vsnprintf
	ret = vsnprintf(dest, size, fmt, argptr);
#define vsnprintf	use_idStr_vsnPrintf
#endif
	dest[size-1] = '\0';

	if (ret < 0 || ret >= size) {
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
int sprintf(idStr &string, const char *fmt, ...)
{
	int l;
	va_list argptr;
	char buffer[32000];

	va_start(argptr, fmt);
	l = idStr::vsnPrintf(buffer, sizeof(buffer)-1, fmt, argptr);
	va_end(argptr);
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
int vsprintf(idStr &string, const char *fmt, va_list argptr)
{
	int l;
	char buffer[32000];

	l = idStr::vsnPrintf(buffer, sizeof(buffer)-1, fmt, argptr);
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
char *va(const char *fmt, ...)
{
	va_list argptr;
	static int index = 0;
	static char string[4][16384];	// in case called by nested functions
	char *buf;

	buf = string[index];
	index = (index + 1) & 3;

	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr);
	va_end(argptr);

	return buf;
}



/*
============
idStr::BestUnit
============
*/
int idStr::BestUnit(const char *format, float value, Measure_t measure)
{
	int unit = 1;

	while (unit <= 3 && (1 << (unit * 10) < value)) {
		unit++;
	}

	unit--;
	value /= 1 << (unit * 10);
	sprintf(*this, format, value);
	*this += " ";
	*this += units[ measure ][ unit ];
	return unit;
}

/*
============
idStr::SetUnit
============
*/
void idStr::SetUnit(const char *format, float value, int unit, Measure_t measure)
{
	value /= 1 << (unit * 10);
	sprintf(*this, format, value);
	*this += " ";
	*this += units[ measure ][ unit ];
}

/*
================
idStr::InitMemory
================
*/
void idStr::InitMemory(void)
{
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Init();
#endif
}

/*
================
idStr::ShutdownMemory
================
*/
void idStr::ShutdownMemory(void)
{
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.Shutdown();
#endif
}

/*
================
idStr::PurgeMemory
================
*/
void idStr::PurgeMemory(void)
{
#ifdef USE_STRING_DATA_ALLOCATOR
	stringDataAllocator.FreeEmptyBaseBlocks();
#endif
}

/*
================
idStr::ShowMemoryUsage_f
================
*/
void idStr::ShowMemoryUsage_f(const idCmdArgs &args)
{
#ifdef USE_STRING_DATA_ALLOCATOR
	idLib::common->Printf("%6d KB string memory (%d KB free in %d blocks, %d empty base blocks)\n",
	                      stringDataAllocator.GetBaseBlockMemory() >> 10, stringDataAllocator.GetFreeBlockMemory() >> 10,
	                      stringDataAllocator.GetNumFreeBlocks(), stringDataAllocator.GetNumEmptyBaseBlocks());
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

int numFormatList = sizeof(formatList) / sizeof(formatList[0]);


idStr idStr::FormatNumber(int number)
{
	idStr string;
	bool hit;

	// reset
	for (int i = 0; i < numFormatList; i++) {
		formatList_t *li = formatList + i;
		li->count = 0;
	}

	// main loop
	do {
		hit = false;

		for (int i = 0; i < numFormatList; i++) {
			formatList_t *li = formatList + i;

			if (number >= li->gran) {
				li->count++;
				number -= li->gran;
				hit = true;
				break;
			}
		}
	} while (hit);

	// print out
	bool found = false;

	for (int i = 0; i < numFormatList; i++) {
		formatList_t *li = formatList + i;

		if (li->count) {
			if (!found) {
				string += va("%i,", li->count);
			} else {
				string += va("%3.3i,", li->count);
			}

			found = true;
		} else if (found) {
			string += va("%3.3i,", li->count);
		}
	}

	if (found) {
		string += va("%3.3i", number);
	} else {
		string += va("%i", number);
	}

	// pad to proper size
	int count = 11 - string.Length();

	for (int i = 0; i < count; i++) {
		string.Insert(" ", 0);
	}

	return string;
}

#ifdef _WCHAR_LANG
/*
========================
idStr::IsValidUTF8
========================
*/
bool idStr::IsValidUTF8( const uint8_t* s, const int maxLen, utf8Encoding_t& encoding )
{
    struct local_t
    {
        static int GetNumEncodedUTF8Bytes( const uint8_t c )
        {
            if( c < 0x80 )
            {
                return 1;
            }
            else if( ( c >> 5 ) == 0x06 )
            {
                // 2 byte encoding - the next byte must begin with
                return 2;
            }
            else if( ( c >> 4 ) == 0x0E )
            {
                // 3 byte encoding
                return 3;
            }
            else if( ( c >> 5 ) == 0x1E )
            {
                // 4 byte encoding
                return 4;
            }
            // this isnt' a valid UTF-8 precursor character
            return 0;
        }
        static bool RemainingCharsAreUTF8FollowingBytes( const uint8_t* s, const int curChar, const int maxLen, const int num )
        {
            if( maxLen - curChar < num )
            {
                return false;
            }
            for( int i = curChar + 1; i <= curChar + num; i++ )
            {
                if( s[ i ] == '\0' )
                {
                    return false;
                }
                if( ( s[ i ] >> 6 ) != 0x02 )
                {
                    return false;
                }
            }
            return true;
        }
    };

    // check for byte-order-marker
    encoding = UTF8_PURE_ASCII;
    utf8Encoding_t utf8Type = UTF8_ENCODED_NO_BOM;
    if( maxLen > 3 && s[ 0 ] == 0xEF && s[ 1 ] == 0xBB && s[ 2 ] == 0xBF )
    {
        utf8Type = UTF8_ENCODED_BOM;
    }

    for( int i = 0; s[ i ] != '\0' && i < maxLen; i++ )
    {
        int numBytes = local_t::GetNumEncodedUTF8Bytes( s[ i ] );
        if( numBytes == 1 )
        {
            continue;	// just low ASCII
        }
        else if( numBytes == 2 )
        {
            // 2 byte encoding - the next byte must begin with bit pattern 10
            if( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 1 ) )
            {
                return false;
            }
            // skip over UTF-8 character
            i += 1;
            encoding = utf8Type;
        }
        else if( numBytes == 3 )
        {
            // 3 byte encoding - the next 2 bytes must begin with bit pattern 10
            if( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 2 ) )
            {
                return false;
            }
            // skip over UTF-8 character
            i += 2;
            encoding = utf8Type;
        }
        else if( numBytes == 4 )
        {
            // 4 byte encoding - the next 3 bytes must begin with bit pattern 10
            if( !local_t::RemainingCharsAreUTF8FollowingBytes( s, i, maxLen, 3 ) )
            {
                return false;
            }
            // skip over UTF-8 character
            i += 3;
            encoding = utf8Type;
        }
        else
        {
            // this isnt' a valid UTF-8 character
            if( utf8Type == UTF8_ENCODED_BOM )
            {
                encoding = UTF8_INVALID_BOM;
            }
            else
            {
                encoding = UTF8_INVALID;
            }
            return false;
        }
    }
    return true;
}

/*
========================
idStr::UTF8Length
========================
*/
int idStr::UTF8Length( const byte* s )
{
    int mbLen = 0;
    int charLen = 0;
    while( s[ mbLen ] != '\0' )
    {
        uint32_t cindex;
        cindex = s[ mbLen ];
        if( cindex < 0x80 )
        {
            mbLen++;
        }
        else
        {
            int trailing = 0;
            if( cindex >= 0xc0 )
            {
                static const byte trailingBytes[ 64 ] =
                        {
                                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
                        };
                trailing = trailingBytes[ cindex - 0xc0 ];
            }
            mbLen += trailing + 1;
        }
        charLen++;
    }
    return charLen;
}


/*
========================
idStr::AppendUTF8Char
========================
*/
void idStr::AppendUTF8Char( uint32_t c )
{
    if( c < 0x80 )
    {
        Append( ( char )c );
    }
    else if( c < 0x800 )      // 11 bits
    {
        Append( ( char )( 0xC0 | ( c >> 6 ) ) );
        Append( ( char )( 0x80 | ( c & 0x3F ) ) );
    }
    else if( c < 0x10000 )      // 16 bits
    {
        Append( ( char )( 0xE0 | ( c >> 12 ) ) );
        Append( ( char )( 0x80 | ( ( c >> 6 ) & 0x3F ) ) );
        Append( ( char )( 0x80 | ( c & 0x3F ) ) );
    }
    else if( c < 0x200000 )  	// 21 bits
    {
        Append( ( char )( 0xF0 | ( c >> 18 ) ) );
        Append( ( char )( 0x80 | ( ( c >> 12 ) & 0x3F ) ) );
        Append( ( char )( 0x80 | ( ( c >> 6 ) & 0x3F ) ) );
        Append( ( char )( 0x80 | ( c & 0x3F ) ) );
    }
    else
    {
        // UTF-8 can encode up to 6 bytes. Why don't we support that?
        // This is an invalid Unicode character
        Append( '?' );
    }
}

/*
========================
idStr::UTF8Char
========================
*/
uint32_t idStr::UTF8Char( const byte* s, int& idx )
{
    if( idx >= 0 )
    {
        while( s[ idx ] != '\0' )
        {
            uint32_t cindex = s[ idx ];
            if( cindex < 0x80 )
            {
                idx++;
                return cindex;
            }
            int trailing = 0;
            if( cindex >= 0xc0 )
            {
                static const byte trailingBytes[ 64 ] =
                        {
                                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
                        };
                trailing = trailingBytes[ cindex - 0xc0 ];
            }
            static const uint32_t trailingMask[ 6 ] = { 0x0000007f, 0x0000001f, 0x0000000f, 0x00000007, 0x00000003, 0x00000001 };
            cindex &= trailingMask[ trailing  ];
            while( trailing-- > 0 )
            {
                cindex <<= 6;
                cindex += s[ ++idx ] & 0x0000003f;
            }
            idx++;
            return cindex;
        }
    }
    idx++;
    return 0;	// return a null terminator if out of range
}
#endif

#ifdef _RAVEN
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

 // RAVEN BEGIN
 

/*
================
idStr::IsEscape
================
*/
 // bdube: escape codes
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
  
     return 0;       // strings are equal 
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
#endif

#ifdef _RAVEN
/*
===================
idStr::StripDoubleQuotes
===================
*/
void idStr::StripDoubleQuotes(void)
{
	idStr temp = *this;
	char* string = (char*)temp.c_str();

	if (*string == '\"')
	{
		strcpy(string, string + 1);
	}

	if (string[strlen(string) - 1] == '\"')
	{
		string[strlen(string) - 1] = '\0';
	}

	*this = string;
}
#endif

idList<idStr> idStr::SplitUnique(const char *macros, char ch)
{
	idStrList ret;

	if(!macros || !macros[0])
		return ret;

	int start = 0;
	int index;
	idStr str(macros);
	str.Strip(ch);
	while((index = str.Find(ch, start)) != -1)
	{
		if(index - start > 0)
		{
			idStr s = str.Mid(start, index - start);
			ret.AddUnique(s);
		}
		start = index + 1;
		if(index == str.Length() - 1)
			break;
	}
	if(start <= str.Length() - 1)
	{
		idStr s = str.Mid(start, str.Length() - start);
		ret.AddUnique(s);
	}

	return ret;
}

idList<idStr> idStr::Split(const char *macros, char ch)
{
	idStrList ret;

	if(!macros || !macros[0])
		return ret;

	int start = 0;
	int index;
	idStr str(macros);
	str.Strip(ch);
	while((index = str.Find(ch, start)) != -1)
	{
		if(index - start > 0)
		{
			idStr s = str.Mid(start, index - start);
			ret.Append(s);
		}
		start = index + 1;
		if(index == str.Length() - 1)
			break;
	}
	if(start <= str.Length() - 1)
	{
		idStr s = str.Mid(start, str.Length() - start);
		ret.Append(s);
	}

	return ret;
}

void idStr::StripWhitespace(idStr &str)
{
	str.StripTrailingWhitespace();
	str.StripLeading(' ');
}
