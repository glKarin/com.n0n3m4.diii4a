/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// common.c -- misc functions used in client and server

#include "quakedef.h"

#include <wctype.h>
#include <ctype.h>
#include <errno.h>

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_major__) //ffs
#include <emscripten/version.h>
#endif

qboolean sys_nounload;
double		host_frametime;
double		realtime;				// without any filtering or bounding
qboolean	host_initialized;		// true if into command execution (compatability)
quakeparms_t host_parms;


//by adding 'extern' to one definition of a function in a translation unit, then the definition in that TU is NOT considered an inline definition. meaning non-inlined references in other TUs can link to it instead of their own if needed.
fte_inlinebody conchar_t *Font_Decode(conchar_t *start, unsigned int *codeflags, unsigned int *codepoint);
fte_inlinebody float M_SRGBToLinear(float x, float mag);
fte_inlinebody float M_LinearToSRGB(float x, float mag);


// These 4 libraries required for the version command

#ifdef AVAIL_ZLIB
	#include <zlib.h>
#endif
#if defined(FTE_SDL3)
	#include <SDL3/SDL.h>
#elif defined(FTE_SDL)
	#include <SDL.h>
#endif

const usercmd_t nullcmd; // guarenteed to be zero

entity_state_t nullentitystate;	//this is the default state

static char	*safeargvs[] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-nohome", "-window"};

static const char	*largv[MAX_NUM_ARGVS + countof(safeargvs) + 1];
static char	*argvdummy = " ";

#ifdef CRAZYDEBUGGING
cvar_t	developer = CVAR("developer","1");
#else
cvar_t	developer = CVARD("developer","0", "Enables the spewing of additional developer/debugging messages. 2 will give even more spam, much of it unwanted.");
#endif

cvar_t	registered				= CVARD("registered","0","Set if quake's pak1.pak is available");
cvar_t	gameversion				= CVARFD("gameversion","", CVAR_SERVERINFO, "gamecode version for server browsers");
cvar_t	gameversion_min			= CVARD("gameversion_min","", "gamecode version for server browsers");
cvar_t	gameversion_max			= CVARD("gameversion_max","", "gamecode version for server browsers");
#ifndef SVNREVISION
static cvar_t	pr_engine		= CVARFD("pr_engine",DISTRIBUTION" -", CVAR_NOSAVE, "This cvar exists so that the menuqc is able to determine which engine-specific settings/values to list/suggest. It must not be used to detect formal QC extensions/builtins. Use checkextension/checkbuiltin/checkcommand for that.");
#else
static cvar_t	pr_engine		= CVARFD("pr_engine",DISTRIBUTION" "STRINGIFY(SVNREVISION), CVAR_NOSAVE, "This cvar exists so that the menuqc is able to determine which engine-specific settings/values to list/suggest. It must not be used to detect formal QC extensions/builtins. Use checkextension/checkbuiltin/checkcommand for that.");
#endif
cvar_t	fs_gamename				= CVARAD("com_fullgamename", NULL, "fs_gamename", "The filesystem is trying to run this game");
cvar_t	com_protocolname		= CVARAD("com_protocolname", NULL, "com_gamename", "The protocol game name used for dpmaster queries. For compatibility with DP, you can set this to 'DarkPlaces-Quake' in order to be listed in DP's master server, and to list DP servers.");
cvar_t	com_protocolversion		= CVARAD("com_protocolversion", "3", NULL, "The protocol version used for dpmaster queries.");	//3 as strong default for compat with DP which uses its netchan rather than protocol version here, even if our QW protocol uses different versions entirely. really it only matters for master servers.
cvar_t	com_parseutf8			= CVARD("com_parseutf8", "1", "Interpret console messages/playernames/etc as UTF-8. Requires special fonts. -1=iso 8859-1. 0=quakeascii(chat uses high chars). 1=utf8, revert to ascii on decode errors. 2=utf8 ignoring errors");	//1 parse. 2 parse, but stop parsing that string if a char was malformed.
cvar_t	com_highlightcolor		= CVARD("com_highlightcolor", STRINGIFY(COLOR_RED), "ANSI colour to be used for highlighted text, used when com_parseutf8 is active.");
cvar_t	com_gamedirnativecode	= CVARFD("com_gamedirnativecode", "0", CVAR_NOTFROMSERVER, FULLENGINENAME" blocks all downloads of files with a .dll or .so extension, however other engines (eg: ezquake and fodquake) do not - this omission can be used to trigger delayed eremote exploits in any engine (including "DISTRIBUTION") which is later run from the same gamedir.\nQuake2, Quake3(when debugging), and KTX typically run native gamecode from within gamedirs, so if you wish to run any of these games you will need to ensure this cvar is changed to 1, as well as ensure that you don't run unsafe clients.");
cvar_t	sys_platform			= CVAR("sys_platform", PLATFORM);
cvar_t	host_mapname			= CVARAFD("mapname", "", "host_mapname", 0, "Cvar that holds the short name of the current map, for scripting type stuff");
#ifdef HAVE_LEGACY
cvar_t	ezcompat_markup			= CVARD("ezcompat_markup", "1", "Attempt compatibility with ezquake's text markup.0: disabled.\n1: Handle markup ampersand markup.\n2: Handle chevron markup (only in echo commands, for config compat, because its just too unreliable otherwise).");
cvar_t	pm_noround				= CVARD("pm_noround", "0", "Disables player prediction snapping, in a way that cannot be reliably predicted but may be needed to avoid map bugs.");
cvar_t	scr_usekfont			= CVARD("scr_usekfont"/*kex*/, "0", "Exists for compat with the quake rerelease, changing the behaviour of QC's sprint/bprint/centerprint builtins.");
#endif

qboolean	com_modified;	// set true if using non-id files

qboolean		static_registered = true;	// only for startup check, then set

qboolean		msg_suppress_1 = false;
int				isPlugin;	//if 2, we qcdebug to external program
qboolean		wantquit;


// if a packfile directory differs from this, it is assumed to be hacked
#define	PAK0_COUNT		339
#define	PAK0_CRC		52883

#ifdef NQPROT
qboolean		standard_quake = true;	//unfortunately, the vanilla NQ protocol(and 666) subtly changes when -rogue or -hipnotic are used (and by extension -quoth). QW/FTE protocols don't not need to care, but compat...
#endif

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.

*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

void QDECL Q_strncpyz(char *d, const char *s, int n)
{
	int i;
	n--;
	if (n < 0)
		return;	//this could be an error

	for (i=0; *s; i++)
	{
		if (i == n)
			break;
		*d++ = *s++;
	}
	*d='\0';
}

//returns true on truncation
qboolean VARGS Q_vsnprintfz (char *dest, size_t size, const char *fmt, va_list argptr)
{
	size_t ret;
#ifdef _WIN32
	//doesn't null terminate.
	//returns -1 on truncation
	ret = _vsnprintf (dest, size, fmt, argptr);
	dest[size-1] = 0;	//shitty paranoia
#else
	//always null terminates.
	//returns length regardless of truncation.
	ret = vsnprintf (dest, size, fmt, argptr);
#endif
#ifdef _DEBUG
	if (ret>=size)
		Sys_Error("Q_vsnprintfz: Truncation\n");
#endif
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return ret>=size;
}

//windows/linux have inconsistant snprintf
//this is an attempt to get them consistant and safe
//size is the total size of the buffer
//returns true on overflow (will be truncated).
qboolean VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
	va_list		argptr;
	size_t ret;

	va_start (argptr, fmt);
#ifdef _WIN32
	//doesn't null terminate.
	//returns -1 on truncation
	ret = _vsnprintf (dest, size, fmt, argptr);
	dest[size-1] = 0;	//shitty paranoia
#else
	//always null terminates.
	//returns length regardless of truncation.
	ret = vsnprintf (dest, size, fmt, argptr);
#endif
	va_end (argptr);
#ifdef _DEBUG
	if (ret>=size)
		Sys_Error("Q_vsnprintfz: Truncation\n");
#endif
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return ret>=size;
}


#if 0
void Q_memset (void *dest, int fill, int count)
{
	int		i;

	if ( (((long)dest | count) & 3) == 0)
	{
		count >>= 2;
		fill = fill | (fill<<8) | (fill<<16) | (fill<<24);
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = fill;
	}
	else
		for (i=0 ; i<count ; i++)
			((qbyte *)dest)[i] = fill;
}

void Q_memcpy (void *dest, void *src, int count)
{
	int		i;

	if (( ( (long)dest | (long)src | count) & 3) == 0 )
	{
		count>>=2;
		for (i=0 ; i<count ; i++)
			((int *)dest)[i] = ((int *)src)[i];
	}
	else
		for (i=0 ; i<count ; i++)
			((qbyte *)dest)[i] = ((qbyte *)src)[i];
}

int Q_memcmp (void *m1, void *m2, int count)
{
	while(count)
	{
		count--;
		if (((qbyte *)m1)[count] != ((qbyte *)m2)[count])
			return -1;
	}
	return 0;
}

void Q_strcpy (char *dest, char *src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}

void Q_strncpy (char *dest, char *src, int count)
{
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	if (count)
		*dest++ = 0;
}

int Q_strlen (char *str)
{
	int		count;

	count = 0;
	while (str[count])
		count++;

	return count;
}

char *Q_strrchr(char *s, char c)
{
	int len = Q_strlen(s);
	s += len;
	while (len--)
		if (*--s == c) return s;
	return 0;
}

void Q_strcat (char *dest, char *src)
{
	dest += Q_strlen(dest);
	Q_strcpy (dest, src);
}

int Q_strcmp (char *s1, char *s2)
{
	while (1)
	{
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

int Q_strncmp (char *s1, char *s2, int count)
{
	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;		// strings not equal
		if (!*s1)
			return 0;		// strings are equal
		s1++;
		s2++;
	}

	return -1;
}

#endif

//case comparisons are specific to ascii only, so this should be 'safe' for utf-8 strings too.
int Q_strncasecmp (const char *s1, const char *s2, int n)
{
	int		c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
			{	// strings not equal
				if (c1 > c2)
					return 1;		// strings not equal
				return -1;
			}
		}
		if (!c1)
			return 0;		// strings are equal
//		s1++;
//		s2++;
	}

	return -1;
}

int Q_strcasecmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 0x7fffffff);
}
int QDECL Q_stricmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 0x7fffffff);
}
int Q_strstopcasecmp(const char *s1start, const char *s1end, const char *s2)
{	//safer version of strncasecmp, where s1 is the one with the length, and must exactly match s2 (which is null terminated and probably an immediate.
	//return value isn't suitable for sorting.
	if (s1end - s1start != strlen(s2))
		return -1;
	return Q_strncasecmp (s1start, s2, s1end - s1start);
}

char *Q_strcasestr(const char *haystack, const char *needle)
{
	int c1, c2, c2f;
	int i;
	c2f = *needle;
	if (c2f >= 'a' && c2f <= 'z')
		c2f -= ('a' - 'A');
	if (!c2f)
		return (char*)haystack;
	while (1)
	{
		c1 = *haystack;
		if (!c1)
			return NULL;
		if (c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
		if (c1 == c2f)
		{
			for (i = 1; ; i++)
			{
				c1 = haystack[i];
				c2 = needle[i];
				if (c1 >= 'a' && c1 <= 'z')
					c1 -= ('a' - 'A');
				if (c2 >= 'a' && c2 <= 'z')
					c2 -= ('a' - 'A');
				if (!c2)
					return (char*)haystack;	//end of needle means we found a complete match
				if (!c1)	//end of haystack means we can't possibly find needle in it any more
					return NULL;
				if (c1 != c2)	//mismatch means no match starting at haystack[0]
					break;
			}
		}
		haystack++;
	}
	return NULL;	//didn't find it
}

void VARGS Com_sprintf(char *buffer, int size, const char *format, ...)
{
	va_list		argptr;

	va_start (argptr, format);
	Q_vsnprintfz (buffer, size, format, argptr);
	va_end (argptr);
}
void	QDECL Com_Error( int level, const char *error, ... )
{
	Sys_Error("%s", error);
}

char *Q_strlwr(char *s)
{
	char *ret=s;
	while(*s)
	{
		if (*s >= 'A' && *s <= 'Z')
			*s=*s-'A'+'a';
		s++;
	}

	return ret;
}

fte_inlinestatic char Q_tolower(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c-'A'+'a';
	return c;
}
int wildcmp(const char *wild, const char *string)
{
/*
	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
		{
			return 0;
		}
		wild++;
		string++;
	}
*/
	while (*string)
	{
		if (*wild == '*')
		{
			if (*string == '/' || *string == '\\')
			{
				//* terminates if we get a match on the char following it, or if its a \ or / char
				wild++;
				continue;
			}
			if (wildcmp(wild+1, string))
				return true;
			string++;
		}
		else if ((Q_tolower(*wild) == Q_tolower(*string)) || (*wild == '?'))
		{
			//this char matches
			wild++;
			string++;
		}
		else
		{
			//failure
			return false;
		}
	}

	while (*wild == '*')
	{
		wild++;
	}
	return !*wild;
}

// Q_ftoa: convert IEEE 754 float to a base-10 string with "infinite" decimal places
void Q_ftoa(char *str, float in)
{
	unsigned int i = *((int *)&in);

	int signbit = (i & 0x80000000) >> 31;
	int exp = (signed int)((i & 0x7F800000) >> 23) - 127;
	int mantissa = (i & 0x007FFFFF);

	if (exp == 128) // 255(NaN/Infinity bits) - 127(bias)
	{
		if (signbit)
		{
			*str = '-';
			str++;
		}
		if (mantissa == 0) // infinity
			strcpy(str, "1.#INF");
		else // NaN or indeterminate
			strcpy(str, "1.#NAN");
		return;
	}

	exp = -exp;
	exp = (int)(exp * 0.30102999957f); // convert base 2 to base 10
	exp += 8;

	if (exp <= 0)
		sprintf(str, "%.0f", in);
	else
	{
		char tstr[32];
		char *lsig = str - 1;
		sprintf(tstr, "%%.%if", exp);
		sprintf(str, tstr, in);
		// find last significant digit and trim
		while (*str)
		{
			if (*str >= '1' && *str <= '9')
				lsig = str;
			else if (*str == '.')
				lsig = str - 1;
			str++;
		}
		lsig[1] = '\0';
	}
}

static int dehex(int i)
{
	if      (i >= '0' && i <= '9')
		return (i-'0');
	else if (i >= 'A' && i <= 'F')
		return (i-'A'+10);
	else
		return (i-'a'+10);
}

int Q_atoi (const char *str)
{
	int		val;
	int		sign;
	int		c;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}

	return 0;
}


float Q_atof (const char *str)
{
	double	val;
	int		sign;
	int		c;
	int		decimal, total;

	while(*str == ' ')
		str++;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else
		sign = 1;

	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

//
// assume decimal
//
	decimal = -1;
	total = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
			break;
		val = val*10 + c - '0';
		total++;
	}

	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}

/*
attempts to remove leet strange chars from a name
the resulting string is not intended to be visible to humans, but this functions results can be matched against each other.
*/
void deleetstring(char *result, const char *leet)
{
	char *s = result;
	const unsigned char *s2 = (const unsigned char*)leet;
	while(*s2)
	{
		if (*s2 == 0xff)
		{
			s2++;
			continue;
		}
		if (*s2 >= 0xa0)
			*s = *s2 & ~128;
		else
			*s = *s2;
		s2++;
		if (*s == '3')
			*s = 'e';
		else if (*s == '4')
			*s = 'a';
		else if (*s == '0')
			*s = 'o';
		else if (*s == '1' || *s == '7')
			*s = 'l';
		else if (*s >= 18 && *s < 27)
			*s = *s - 18 + '0';
		else if (*s >= 'A' && *s <= 'Z')
			*s = *s - 'A' + 'a';
		else if (*s == '_' || *s == ' ' || *s == '~')
			continue;
		s++;
	}
	*s = '\0';
}


/*
============================================================================

					qbyte ORDER FUNCTIONS

============================================================================
*/

#if !defined(FTE_BIG_ENDIAN) && !defined(FTE_LITTLE_ENDIAN)
qboolean	bigendian;

short		(*BigShort)		(short l);
short		(*LittleShort)	(short l);
int			(*BigLong)		(int l);
int			(*LittleLong)	(int l);
qint64_t	(*BigI64)		(qint64_t l);
qint64_t	(*LittleI64)	(qint64_t l);
float		(*BigFloat)		(float l);
float		(*LittleFloat)	(float l);

static short	ShortNoSwap (short l)	{	return l;	}
static int		LongNoSwap (int l)		{	return l;	}
static qint64_t	I64NoSwap (qint64_t l)	{	return l;	}
static float	FloatNoSwap (float f)	{	return f;	}
#endif

short		ShortSwap	(short l)
{
	return	((l>> 8)&0x00ff)|
			((l<< 8)&0xff00);
}
int			LongSwap	(int l)
{
	return	((l>>24)&0x000000ff)|
			((l>> 8)&0x0000ff00)|
			((l<< 8)&0x00ff0000)|
			((l<<24)&0xff000000);
}
qint64_t    I64Swap		(qint64_t l)
{
	return	((l>>56)&        0x000000ff)|
			((l>>40)&        0x0000ff00)|
			((l>>24)&        0x00ff0000)|
			((l>> 8)&        0xff000000)|
			((l<< 8)&0x000000ff00000000)|
			((l<<24)&0x0000ff0000000000)|
			((l<<40)&0x00ff000000000000)|
			((l<<56)&0xff00000000000000);
}
float FloatSwap (float f)
{
	union
	{
		float	f;
		qbyte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

void COM_SwapLittleShortBlock (short *s, int size)
{
	if (size <= 0)
		return;

	if (!bigendian)
		return;

	while (size)
	{
		*s = ShortSwap(*s);
		s++;
		size--;
	}
}

void COM_CharBias (signed char *c, int size)
{
	if (size <= 0)
		return;

	while (size)
	{
		*c = (*(unsigned char *)c) - 128;
		c++;
		size--;
	}
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles qbyte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_BeginWriting (sizebuf_t *msg, struct netprim_s prim, void *bufferstorage, size_t buffersize)
{
	if (bufferstorage || buffersize)
	{	//otherwise just clear it.
		msg->data = bufferstorage;
		msg->maxsize = buffersize;
	}
	msg->overflowed = false;
	msg->cursize = 0;
	msg->currentbit = 0;
	msg->packing = SZ_RAWBYTES;
	msg->prim = prim;
	msg->allowoverflow = false;
}

static void MSG_WriteRawBytes(sizebuf_t *msg, int value, int bits)
{
	qbyte	*buf;

	if (bits <= 8)
	{
		buf = SZ_GetSpace(msg, 1);
		buf[0] = value;
	}
	else if (bits <= 16)
	{
		buf = SZ_GetSpace(msg, 2);
		buf[0] = value & 0xFF;
		buf[1] = value >> 8;
	}
	else //if (bits <= 32)
	{
		buf = SZ_GetSpace(msg, 4);
		buf[0] = value & 0xFF;
		buf[1] = (value >> 8) & 0xFF;
		buf[2] = (value >> 16) & 0xFF;
		buf[3] = value >> 24;
	}
}

static void MSG_WriteRawBits(sizebuf_t *msg, int value, int bits)
{
	int i;
	for (i=0; i<bits; )
	{
		if (!(msg->currentbit&7))
		{	//we need another byte now...
			msg->cursize++;
			if (bits >= 8)
			{	//splurge an entire byte
				msg->data[msg->currentbit>>3] = (value>>i)&0xff;
				i += 8;
				msg->currentbit += 8;
				continue;
			}
			//clear it for the following 8 bits to splurge
			msg->data[msg->currentbit>>3] = 0;
		}
		msg->data[msg->currentbit>>3] |= ((value>>i)&1) << (msg->currentbit & 7);
		msg->currentbit++;
		i++;
	}
}

#ifdef HUFFNETWORK
static void MSG_WriteHuffBits(sizebuf_t *msg, int value, int bits)
{
	int		remaining;
	int		i;

	value &= 0xFFFFFFFFu >> (32 - bits);
	remaining = bits & 7;

	for( i=0; i<remaining ; i++ )
	{
		if( !(msg->currentbit & 7) )
		{
			msg->data[msg->currentbit >> 3] = 0;
		}
		msg->data[msg->currentbit >> 3] |= (value & 1) << (msg->currentbit & 7);
		msg->currentbit++;
		value >>= 1;
	}
	bits -= remaining;

	if( bits > 0 )
	{
		for( i=0 ; i<(bits+7)>>3 ; i++ )
		{
			Huff_EmitByte( value & 255, msg->data, &msg->currentbit );
			value >>= 8;
		}
	}

	msg->cursize = (msg->currentbit >> 3) + 1;
}
#endif

/*
============
MSG_WriteBits
============
*/
void MSG_WriteBits(sizebuf_t *msg, int value, int bits)
{
	if( !bits || bits < -31 || bits > 32 )
		Sys_Error("MSG_WriteBits: bad bits %i", bits);

	if (bits < 0)
	{	//negative means sign extension on reading
		if (value & (1u<<(bits-1)))
			value |= ~1u<<bits;	//sign extend, just in case it matters (rawbytes with bits < n*8)...
		bits = -bits;
	}

	switch( msg->packing )
	{
	default:
	case SZ_BAD:
		Sys_Error("MSG_WriteBits: bad msg->packing %i", msg->packing );
		break;
	case SZ_RAWBYTES:
		MSG_WriteRawBytes( msg, value, bits );
		break;
	case SZ_RAWBITS:
		MSG_WriteRawBits( msg, value, bits );
		break;
#ifdef HUFFNETWORK
	case SZ_HUFFMAN:
		if( msg->maxsize - msg->cursize < 4 )
		{
			if (!msg->allowoverflow)
			msg->overflowed = true;
			return;
		}
		MSG_WriteHuffBits( msg, value, bits );
		break;
#endif
	}
}

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	qbyte	*buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		Sys_Error ("MSG_WriteChar: range error");
#endif

	buf = (qbyte*)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	qbyte	*buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		Sys_Error ("MSG_WriteByte: range error");
#endif

	buf = (qbyte*)SZ_GetSpace (sb, 1);
	buf[0] = c&0xff;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	qbyte	*buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c > (short)0x7fff)
		Sys_Error ("MSG_WriteShort: range error");
#endif

	buf = (qbyte*)SZ_GetSpace (sb, 2);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	qbyte	*buf;

	buf = (qbyte*)SZ_GetSpace (sb, 4);
	buf[0] = c&0xff;
	buf[1] = (c>>8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = (c>>24)&0xff;
}
void MSG_WriteULEB128 (sizebuf_t *sb, quint64_t c)
{
	qbyte b;
	for(;;)
	{
		b = c&0x7f;
		c>>=7;
		if (!c)
			break;
		MSG_WriteByte(sb, b|0x80);
	}
	MSG_WriteByte(sb, b);
}
/*void MSG_WriteSLEB128 (sizebuf_t *sb, qint64_t c)
{
	qbyte b;
	for(;;)
	{
		b = c&0x7f;
		c>>=7;
		if ((c==0 && (b&64)==0) || (c==-1 && (b&64)!=0))
			break;
		MSG_WriteByte(sb, b|0x80);
	}
	MSG_WriteByte(sb, b);
}*/
void MSG_WriteSignedQEX (sizebuf_t *sb, qint64_t c)
{
	if (c < 0)
		MSG_WriteULEB128(sb, ((quint64_t)(-1-c)<<1)|1);
	else
		MSG_WriteULEB128(sb, c<<1);
}
void MSG_WriteUInt64 (sizebuf_t *sb, quint64_t c)
{	//0* 10*,*, 110*,*,* etc, up to 0xff followed by 8 continuation bytes
	qbyte *buf;
	int b = 0;
	quint64_t l = 128;
	while (c > l-1u && b < 8)
	{	//count the extra bytes we need
		b++;
		l <<= 7;	//each byte we add gains 8 bits, but we spend one on length.
	}
	buf = (qbyte*)SZ_GetSpace (sb, 1+b);
	*buf++ = 0xffu<<(8-b) | (c >> (b*8));
	while(b --> 0)
		*buf++ = (c >> (b*8))&0xff;
}
void MSG_WriteInt64 (sizebuf_t *sb, qint64_t c)
{	//move the sign bit into the low bit and avoid sign extension for more efficient length coding.
	if (c < 0)
		MSG_WriteUInt64(sb, ((quint64_t)(-1-c)<<1)|1);
	else
		MSG_WriteUInt64(sb, c<<1);
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;


	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}
void MSG_WriteDouble (sizebuf_t *sb, double f)
{
	union
	{
		double	f;
		quint64_t l;
	} dat = {f};
	quint64_t c = dat.l;
	qbyte	*buf;

	buf = (qbyte*)SZ_GetSpace (sb, 8);
	buf[0] = (c>> 0)&0xff;
	buf[1] = (c>> 8)&0xff;
	buf[2] = (c>>16)&0xff;
	buf[3] = (c>>24)&0xff;
	buf[4] = (c>>32)&0xff;
	buf[5] = (c>>40)&0xff;
	buf[6] = (c>>48)&0xff;
	buf[7] = (c>>56)&0xff;
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, Q_strlen(s)+1);
}

vec_t MSG_FromCoord(coorddata c, int type)
{
	switch(type)
	{
	case COORDTYPE_FIXED_13_3:	//encode 1/8th precision, giving -4096 to 4096 map sizes
		return LittleShort(c.b2)/8.0f;
	case COORDTYPE_FIXED_16_8:
		return LittleShort(c.b2) + (((unsigned char*)c.b)[2] * (1/255.0)); /*FIXME: RMQe uses 255, should be 256*/
	case COORDTYPE_FIXED_28_4:
		return LittleLong(c.b4)/16.0f;
	case COORDTYPE_FLOAT_32:
		return LittleFloat(c.f);
	default:
		Sys_Error("MSG_ToCoord: not a sane coordsize");
		return 0;
	}
}
coorddata MSG_ToCoord(float f, int type)	//return value should be treated as (char*)&ret;
{
	coorddata r;
	switch(type)
	{
	case COORDTYPE_FIXED_13_3:
		r.b4 = 0;
		if (f >= 0)
			r.b2 = LittleShort((short)(f*8+0.5f));
		else
			r.b2 = LittleShort((short)(f*8-0.5f));
		break;
	case COORDTYPE_FIXED_16_8:
		r.b2 = LittleShort((short)f);
		r.b[2] = (int)(f*255)%255;
		r.b[3] = 0;
		break;
	case COORDTYPE_FIXED_28_4:
		if (f >= 0)
			r.b4 = LittleLong((short)(f*16+0.5f));
		else
			r.b4 = LittleLong((short)(f*16-0.5f));
		break;
	case COORDTYPE_FLOAT_32:
		r.f = LittleFloat(f);
		break;
	default:
		Sys_Error("MSG_ToCoord: not a sane coordsize");
		r.b4 = 0;
	}

	return r;
}

coorddata MSG_ToAngle(float f, int bytes)	//return value is NOT byteswapped.
{
	coorddata r;
	switch(bytes)
	{
	case 1:
		r.b4 = 0;
		if (f >= 0)
			r.b[0] = (int)(f*(256.0f/360.0f) + 0.5f) & 255;
		else
			r.b[0] = (int)(f*(256.0f/360.0f) - 0.5f) & 255;
		break;
	case 2:
		r.b4 = 0;
		if (f >= 0)
			r.b2 = LittleShort((int)(f*(65536.0f/360.0f) + 0.5f) & 65535);
		else
			r.b2 = LittleShort((int)(f*(65536.0f/360.0f) - 0.5f) & 65535);
		break;
	case 4:
		r.f = LittleFloat(f);
		break;
	default:
		Sys_Error("MSG_ToCoord: not a sane coordsize");
		r.b4 = 0;
	}

	return r;
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	coorddata i = MSG_ToCoord(f, sb->prim.coordtype);
	SZ_Write (sb, (void*)&i, sb->prim.coordtype&COORDTYPE_SIZE_MASK);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	if (f >= 0)
		MSG_WriteShort (sb, (int)(f*(65536.0f/360.0f) + 0.5f) & 65535);
	else
		MSG_WriteShort (sb, (int)(f*(65536.0f/360.0f) - 0.5f) & 65535);
}
void MSG_WriteAngle8 (sizebuf_t *sb, float f)
{
	if (f >= 0)
		MSG_WriteByte (sb, (int)(f*(256.0f/360.0f) + 0.5f) & 255);
	else
		MSG_WriteByte (sb, (int)(f*(256.0f/360.0f) - 0.5f) & 255);
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	if (sb->prim.anglesize==2)
		MSG_WriteAngle16(sb, f);
	else if (sb->prim.anglesize==4)
		MSG_WriteFloat(sb, f);
	else if (sb->prim.anglesize==1)
		MSG_WriteAngle8 (sb, f);
	else
		Sys_Error("MSG_WriteAngle: undefined network primitive size");
}

#if defined(HAVE_CLIENT) || defined(HAVE_SERVER)
int MSG_ReadSize16 (sizebuf_t *sb)
{
	unsigned short ssolid = MSG_ReadShort();
	if (ssolid == ES_SOLID_BSP)
		return ssolid;
	else
	{
		int solid = (((ssolid>>7) & 0x1F8) - 32+32768)<<16;	/*up can be negative*/
		solid|= ((ssolid & 0x1F)<<3);
		solid|= ((ssolid & 0x3E0)<<6);
		return solid;
	}
}
void MSG_WriteSize16 (sizebuf_t *sb, unsigned int sz)
{
	if (sz == ES_SOLID_BSP)
		MSG_WriteShort(sb, ES_SOLID_BSP);
	else if (sz)
	{
		//decode the 32bit version and recode it.
		int x = sz & 255;
		int zd = (sz >> 8) & 255;
		int zu = ((sz >> 16) & 65535) - 32768;
		MSG_WriteShort(sb, 
			((x>>3)<<0) |
			((zd>>3)<<5) |
			(((zu+32)>>3)<<10));
	}
	else
		MSG_WriteShort(sb, 0);
}
void COM_DecodeSize(int solid, float *mins, float *maxs)
{
#if 0
	//q2e 32bit-encoding
	int x,y,d,u;
	x = (solid>>0)&0xff;
	y = (solid>>8)&0xff;
	d = (solid>>16)&0xff;
	u = (solid>>24)&0xff;

	mins[0] = -x; maxs[0] = x;
	mins[1] = -y; maxs[1] = y;
	mins[2] = -d; maxs[2] = u - 32;
#elif 1
	//r1q2/q2pro 32bit-encoding
	maxs[0] = maxs[1] = solid & 255;
	mins[0] = mins[1] = -maxs[0];
	mins[2] = -((solid>>8) & 255);
	maxs[2] = ((solid>>16) & 65535) - 32768;
#else
	//classic q2 16-bit encoding.
	maxs[0] = maxs[1] = 8*(solid & 31);
	mins[0] = mins[1] = -maxs[0];
	mins[2] = -8*((solid>>5) & 31);
	maxs[2] = 8*((solid>>10) & 63) - 32;
#endif
}
int COM_EncodeSize(const float *mins, const float *maxs)
{
	int solid;
#if 1
	solid = bound(0, (int)-mins[0], 255);
	solid |= bound(0, (int)-mins[2], 255)<<8;
	solid |= bound(0, (int)((maxs[2]+32768)), 65535)<<16;	/*up can be negative*/;
	if (solid == 0x80000000)
		solid = 0;	//point sized stuff should just be non-solid. you'll thank me for splitscreens.
#else
	//vanilla q2
	solid = bound(0, (int)-mins[0]/8, 31);
	solid |= bound(0, (int)-mins[2]/8, 31)<<5;
	solid |= bound(0, (int)((maxs[2]+32)/8), 63)<<10;	/*up can be negative*/;
	if (solid == 4096)
		solid = 0;	//point sized stuff should just be non-solid. you'll thank me for splitscreens.
#endif
	return solid;
}

void MSG_WriteEntity(sizebuf_t *sb, unsigned int entnum)
{
	if (entnum > MAX_EDICTS)
		Host_EndGame("index %#x is not a valid entity\n", entnum);

	if (entnum >= 0x8000)
	{
		MSG_WriteShort(sb, (entnum>>8) | 0x8000);
		MSG_WriteByte(sb, entnum & 0xff);
	}
	else
		MSG_WriteShort(sb, entnum);
}
unsigned int MSG_ReadBigEntity(void)
{
	unsigned int num;
	num = MSG_ReadShort();
	if (num & 0x8000)
	{
		num = (num & 0x7fff) << 8;
		num |= MSG_ReadByte();
	}
	return num;
}
#endif

//we use the high bit of the entity number to state that this is a large entity.
#ifdef HAVE_SERVER
unsigned int MSGSV_ReadEntity(client_t *fromclient)
{
	unsigned int num;
	if (fromclient->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
		num = MSG_ReadBigEntity();
	else
		num = (unsigned short)(short)MSG_ReadShort();
	if (num >= sv.world.max_edicts)
	{
		Con_Printf("client %s sent invalid entity\n", fromclient->name);
		fromclient->drop = true;
		return 0;
	}
	return num;
}
#endif
#ifdef HAVE_CLIENT
unsigned int MSGCL_ReadEntity(void)
{
	unsigned int num;
	if (cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
		num = MSG_ReadBigEntity();
	else
		num = (unsigned short)(short)MSG_ReadShort();
	return num;
}
#endif

#if defined(Q2CLIENT) && defined(HAVE_CLIENT)
void MSGQ2EX_WriteDeltaUsercmd (sizebuf_t *buf, const usercmd_t *from, const usercmd_t *cmd)
{
	unsigned int  bits = 0;
//	unsigned char buttons = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= Q2CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= Q2CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= Q2CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= Q2CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= Q2CM_SIDE;
//	if (cmd->upmove != from->upmove)
//		bits |= Q2CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= Q2CM_BUTTONS;
//	if (cmd->impulse != from->impulse)
//		bits |= Q2CM_IMPULSE;

	MSG_WriteByte (buf, bits);

	if (bits & Q2CM_ANGLE1)
		MSG_WriteFloat (buf, SHORT2ANGLE(cmd->angles[0]));
	if (bits & Q2CM_ANGLE2)
		MSG_WriteFloat (buf, SHORT2ANGLE(cmd->angles[1]));
	if (bits & Q2CM_ANGLE3)
		MSG_WriteFloat (buf, SHORT2ANGLE(cmd->angles[2]));

	if (bits & Q2CM_FORWARD)
		MSG_WriteFloat (buf, cmd->forwardmove);
	if (bits & Q2CM_SIDE)
		MSG_WriteFloat (buf, cmd->sidemove);
//	if (bits & Q2CM_UP)
//		MSG_WriteFloat (buf, cmd->upmove);

	if (bits & Q2CM_BUTTONS)
		MSG_WriteByte (buf, cmd->buttons);
	//if (bits & Q2CM_IMPULSE)
//		MSG_WriteByte (buf, cmd->impulse);
	MSG_WriteByte (buf, bound(0, cmd->msec, 250));	//clamp msecs to 250, because r1q2 likes kicking us if we stall for any reason

//	MSG_WriteByte (buf, cmd->lightlevel);
}
void MSGQ2_WriteDeltaUsercmd (sizebuf_t *buf, const usercmd_t *from, const usercmd_t *cmd)
{
	unsigned int  bits = 0;
	unsigned char buttons = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= Q2CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= Q2CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= Q2CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= Q2CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= Q2CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= Q2CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= Q2CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= Q2CM_IMPULSE;


	if (buf->prim.flags & NPQ2_R1Q2_UCMD)
	{
		if (bits & Q2CM_ANGLE1)
			buttons = cmd->buttons & (1|2|128);	//attack, jump, any.
		if ((bits & Q2CM_FORWARD) && !(cmd->forwardmove % 5) && abs(cmd->forwardmove/5) < 128)
			buttons |= R1Q2_BUTTON_BYTE_FORWARD;
		if ((bits & Q2CM_SIDE) && !(cmd->sidemove % 5) && abs(cmd->sidemove/5) < 128)
			buttons |= R1Q2_BUTTON_BYTE_SIDE;
		if ((bits & Q2CM_UP) && !(cmd->upmove % 5) && abs(cmd->upmove/5) < 128)
			buttons |= R1Q2_BUTTON_BYTE_UP;
		if ((bits & Q2CM_ANGLE1) && !(cmd->angles[0] % 64) && abs(cmd->angles[0] / 64) < 128)
			buttons |= R1Q2_BUTTON_BYTE_ANGLE1;
		if ((bits & Q2CM_ANGLE2) && !(cmd->angles[1] % 256))
			buttons |= R1Q2_BUTTON_BYTE_ANGLE2;
		if (buttons & (R1Q2_BUTTON_BYTE_FORWARD|R1Q2_BUTTON_BYTE_SIDE|R1Q2_BUTTON_BYTE_UP|R1Q2_BUTTON_BYTE_ANGLE1|R1Q2_BUTTON_BYTE_ANGLE2))
			bits |= Q2CM_BUTTONS;
	}

	MSG_WriteByte (buf, bits);
	if (buf->prim.flags & NPQ2_R1Q2_UCMD)
	{
		if (bits & Q2CM_BUTTONS)
			MSG_WriteByte (buf, buttons);
	}

	if (bits & Q2CM_ANGLE1)
	{
		if (buttons & R1Q2_BUTTON_BYTE_ANGLE1)
			MSG_WriteChar (buf, cmd->angles[0] / 64);
		else
			MSG_WriteShort (buf, cmd->angles[0]);
	}
	if (bits & Q2CM_ANGLE2)
	{
		if (buttons & R1Q2_BUTTON_BYTE_ANGLE2)
			MSG_WriteChar (buf, cmd->angles[1] / 256);
		else
			MSG_WriteShort (buf, cmd->angles[1]);
	}
	if (bits & Q2CM_ANGLE3)
		MSG_WriteShort (buf, cmd->angles[2]);

	if (bits & Q2CM_FORWARD)
	{
		if (buttons & R1Q2_BUTTON_BYTE_FORWARD)
			MSG_WriteChar (buf, cmd->forwardmove/5);
		else
			MSG_WriteShort (buf, cmd->forwardmove);
	}
	if (bits & Q2CM_SIDE)
	{
		if (buttons & R1Q2_BUTTON_BYTE_SIDE)
			MSG_WriteChar (buf, cmd->sidemove/5);
		else
			MSG_WriteShort (buf, cmd->sidemove);
	}
	if (bits & Q2CM_UP)
	{
		if (buttons & R1Q2_BUTTON_BYTE_UP)
			MSG_WriteChar (buf, cmd->upmove/5);
		else
			MSG_WriteShort (buf, cmd->upmove);
	}

	if (!(buf->prim.flags & NPQ2_R1Q2_UCMD))
	{
		if (bits & Q2CM_BUTTONS)
			MSG_WriteByte (buf, cmd->buttons);
	}
	if (bits & Q2CM_IMPULSE)
		MSG_WriteByte (buf, cmd->impulse);
	MSG_WriteByte (buf, bound(0, cmd->msec, 250));	//clamp msecs to 250, because r1q2 likes kicking us if we stall for any reason

	MSG_WriteByte (buf, cmd->lightlevel);
}
#endif

#define	UC_ANGLE1		(1<<0)
#define	UC_ANGLE2		(1<<1)
#define	UC_ANGLE3		(1<<2)
#define	UC_FORWARD		(1<<3)
#define	UC_RIGHT		(1<<4)
#define	UC_BUTTONS		(1<<5)
#define	UC_IMPULSE		(1<<6)

#define	UC_UP			(1<<7)	//split from forward/right because its rare, and this avoids sending an extra byte.
#define UC_ABSANG		(1<<8)	//angle values are shorts
#define UC_BIGMOVES		(1<<9)	//fwd/left/up are shorts, rather than a fith.
#define	UC_WEAPON		(1<<10)
#define	UC_CURSORFLDS	(1<<11)	//lots of data in one.
#define	UC_LIGHTLEV		(1<<12)
#define	UC_VR_HEAD		(1<<13)

#define	UC_VR_RIGHT		(1<<14)
#define	UC_VR_LEFT		(1<<15)
//#define	UC_UNUSED		(1<<16)
//#define	UC_UNUSED		(1<<17)
#define	UC_MSEC_DEBUG		(1<<18)	//FIXME: temporary
//#define	UC_UNUSED		(1<<19)
//#define	UC_UNUSED		(1<<20)

//#define	UC_UNUSED		(1<<21)
//#define	UC_UNUSED		(1<<22)
//#define	UC_UNUSED		(1<<23)
//#define	UC_UNUSED		(1<<24)
//#define	UC_UNUSED		(1<<25)
//#define	UC_UNUSED		(1<<26)
//#define	UC_UNUSED		(1<<27)

//#define	UC_UNUSED		(1<<28)
//#define	UC_UNUSED		(1<<29)
//#define	UC_UNUSED		(1<<30)
//#define	UC_UNUSED		(1<<31)
#define UC_UNSUPPORTED (~(UC_ANGLE1 | UC_ANGLE2 | UC_ANGLE3 | UC_FORWARD | UC_RIGHT | UC_BUTTONS | UC_IMPULSE | UC_UP | UC_ABSANG | UC_BIGMOVES | UC_WEAPON | UC_CURSORFLDS | UC_LIGHTLEV | UC_VR_HEAD | UC_VR_RIGHT | UC_VR_LEFT | UC_MSEC_DEBUG))

#define UC_VR_STATUS		(1<<0)
#define UC_VR_ANG			(1<<1)
#define UC_VR_AVEL			(1<<2)
#define UC_VR_ORG			(1<<3)
#define UC_VR_VEL			(1<<4)
#define UC_VR_WEAPON		(1<<5)

#ifdef HAVE_CLIENT
fte_inlinestatic qboolean MSG_CompareVR(int i, const usercmd_t *from, const usercmd_t *cmd)
{
	if (cmd->vr[i].status != from->vr[i].status)
		return true;
	return
		(cmd->vr[i].angles[0] != from->vr[i].angles[0]||cmd->vr[i].angles[1] != from->vr[i].angles[1]||cmd->vr[i].angles[2] != from->vr[i].angles[2])|
		(cmd->vr[i].avelocity[0] != from->vr[i].avelocity[0]||cmd->vr[i].avelocity[1] != from->vr[i].avelocity[1]||cmd->vr[i].avelocity[2] != from->vr[i].avelocity[2])|
		(cmd->vr[i].origin[0] != from->vr[i].angles[0]||cmd->vr[i].origin[1] != from->vr[i].origin[1]||cmd->vr[i].origin[2] != from->vr[i].origin[2])|
		(cmd->vr[i].velocity[0] != from->vr[i].velocity[0]||cmd->vr[i].velocity[1] != from->vr[i].velocity[1]||cmd->vr[i].velocity[2] != from->vr[i].velocity[2]);
}
static  void MSG_WriteVR(int i, sizebuf_t *buf, const usercmd_t *from, const usercmd_t *cmd)
{
	unsigned int bits = 0;
	if (cmd->vr[i].status != from->vr[i].status)
		bits |= UC_VR_STATUS;
	if (cmd->vr[i].angles[0] != from->vr[i].angles[0] || cmd->vr[i].angles[1] != from->vr[i].angles[1] || cmd->vr[i].angles[2] != from->vr[i].angles[2])
		bits |= UC_VR_ANG;
	if (cmd->vr[i].avelocity[0] != from->vr[i].avelocity[0] || cmd->vr[i].avelocity[1] != from->vr[i].avelocity[1] || cmd->vr[i].avelocity[2] != from->vr[i].avelocity[2])
		bits |= UC_VR_AVEL;
	if (cmd->vr[i].origin[0] != from->vr[i].origin[0] || cmd->vr[i].origin[1] != from->vr[i].origin[1] || cmd->vr[i].origin[2] != from->vr[i].origin[2])
		bits |= UC_VR_ORG;
	if (cmd->vr[i].velocity[0] != from->vr[i].velocity[0] || cmd->vr[i].velocity[1] != from->vr[i].velocity[1] || cmd->vr[i].velocity[2] != from->vr[i].velocity[2])
		bits |= UC_VR_VEL;
	if (cmd->vr[i].weapon != from->vr[i].weapon)
		bits |= UC_VR_WEAPON;

	MSG_WriteUInt64(buf, bits);
	if (bits & UC_VR_STATUS)
		MSG_WriteUInt64(buf, cmd->vr[i].status);
	if (bits & UC_VR_ANG)
	{
		MSG_WriteShort(buf, cmd->vr[i].angles[0]);
		MSG_WriteShort(buf, cmd->vr[i].angles[1]);
		MSG_WriteShort(buf, cmd->vr[i].angles[2]);
	}
	if (bits & UC_VR_AVEL)
	{
		MSG_WriteShort(buf, cmd->vr[i].avelocity[0]);
		MSG_WriteShort(buf, cmd->vr[i].avelocity[1]);
		MSG_WriteShort(buf, cmd->vr[i].avelocity[2]);
	}
	if (bits & UC_VR_ORG)
	{
		MSG_WriteFloat(buf, cmd->vr[i].origin[0]);
		MSG_WriteFloat(buf, cmd->vr[i].origin[1]);
		MSG_WriteFloat(buf, cmd->vr[i].origin[2]);
	}
	if (bits & UC_VR_VEL)
	{
		MSG_WriteFloat(buf, cmd->vr[i].velocity[0]);
		MSG_WriteFloat(buf, cmd->vr[i].velocity[1]);
		MSG_WriteFloat(buf, cmd->vr[i].velocity[2]);
	}
	if (bits & UC_VR_WEAPON)
		MSG_WriteUInt64(buf, cmd->vr[i].weapon);
}
void MSGFTE_WriteDeltaUsercmd (sizebuf_t *buf, const short baseangles[3], const usercmd_t *from, const usercmd_t *cmd)
{
	unsigned int		bits = 0;
	int i;
	short d;

//
// send the movement message
//
	for (i = 0; i < 3; i++)
	{
		d = cmd->angles[i]-from->angles[i];
		if (d)
		{
			bits |= UC_ANGLE1<<i;
			if (d < -128 || d > 127)
				bits |= UC_ABSANG;	//can't delta it.
		}
	}
	if (cmd->forwardmove != from->forwardmove)
	{
		bits |= UC_FORWARD;
		if ((cmd->forwardmove%5) || cmd->forwardmove > 127*5 || cmd->forwardmove < -128*5)
			bits |= UC_BIGMOVES;	//can't compact it.
	}
	if (cmd->sidemove != from->sidemove)
	{
		bits |= UC_RIGHT;
		if ((cmd->sidemove%5) || cmd->sidemove > 127*5 || cmd->sidemove < -128*5)
			bits |= UC_BIGMOVES;	//can't compact it.
	}
	if (cmd->upmove != from->upmove)
	{
		bits |= UC_UP;
		if ((cmd->upmove%5) || cmd->upmove > 127*5 || cmd->upmove < -128*5)
			bits |= UC_BIGMOVES;	//can't compact it.
	}

	if (cmd->buttons != from->buttons)
		bits |= UC_BUTTONS;
	if (cmd->buttons != from->buttons)
		bits |= UC_WEAPON;
	if (cmd->impulse != from->impulse)
		bits |= UC_IMPULSE;
	if (cmd->lightlevel != from->lightlevel)
		bits |= UC_LIGHTLEV;

	if (cmd->cursor_screen[0] != from->cursor_screen[0] || cmd->cursor_screen[1] != from->cursor_screen[1] ||
		cmd->cursor_start[0] != from->cursor_start[0] || cmd->cursor_start[1] != from->cursor_start[1] || cmd->cursor_start[2] != from->cursor_start[2] ||
		cmd->cursor_impact[0] != from->cursor_impact[0] || cmd->cursor_impact[1] != from->cursor_impact[1] || cmd->cursor_impact[2] != from->cursor_impact[2] ||
		cmd->cursor_entitynumber != from->cursor_entitynumber)
		bits |= UC_CURSORFLDS;

	if (MSG_CompareVR(VRDEV_HEAD, from, cmd))
		bits |= UC_VR_HEAD;
	if (MSG_CompareVR(VRDEV_RIGHT, from, cmd))
		bits |= UC_VR_RIGHT;
	if (MSG_CompareVR(VRDEV_LEFT, from, cmd))
		bits |= UC_VR_LEFT;

#ifdef _DEBUG
	if (developer.ival)
		bits |= UC_MSEC_DEBUG;
#endif

	//NOTE: WriteUInt64 actually uses some length coding, so its not quite as bloated as it looks.
	MSG_WriteUInt64(buf, bits);

	MSG_WriteUInt64(buf, cmd->servertime-from->servertime);
	if (bits & UC_MSEC_DEBUG)
		MSG_WriteUInt64(buf, cmd->msec);
	for (i = 0; i < 3; i++)
	{
		if (bits & (UC_ANGLE1<<i))
		{
			if (bits & UC_ABSANG)
				MSG_WriteShort(buf, cmd->angles[i]-baseangles[i]);
			else
				MSG_WriteChar(buf, cmd->angles[i]-from->angles[i]);
		}
	}
	if (bits & UC_FORWARD)
	{
		if (bits & UC_BIGMOVES)
			MSG_WriteInt64(buf, cmd->forwardmove);
		else
			MSG_WriteChar(buf, cmd->forwardmove/5);
	}
	if (bits & UC_RIGHT)
	{
		if (bits & UC_BIGMOVES)
			MSG_WriteInt64(buf, cmd->sidemove);
		else
			MSG_WriteChar(buf, cmd->sidemove/5);
	}
	if (bits & UC_UP)
	{
		if (bits & UC_BIGMOVES)
			MSG_WriteInt64(buf, cmd->upmove);
		else
			MSG_WriteChar(buf, cmd->upmove/5);
	}


	if (bits & UC_BUTTONS)
		MSG_WriteUInt64 (buf, cmd->buttons);
	if (bits & UC_IMPULSE)
		MSG_WriteUInt64 (buf, cmd->impulse);
	if (bits & UC_WEAPON)
		MSG_WriteUInt64 (buf, cmd->weapon);
	if (bits & UC_CURSORFLDS)
	{
		//prydon cursor crap. kinda bloated.
		MSG_WriteShort(buf, cmd->cursor_screen[0] * 32767);
		MSG_WriteShort(buf, cmd->cursor_screen[1] * 32767);
		MSG_WriteFloat(buf, cmd->cursor_start[0]);	//avoiding WriteAngle/WriteCoord means we can avoid netprim size difference issues.
		MSG_WriteFloat(buf, cmd->cursor_start[1]);
		MSG_WriteFloat(buf, cmd->cursor_start[2]);
		MSG_WriteFloat(buf, cmd->cursor_impact[0]);
		MSG_WriteFloat(buf, cmd->cursor_impact[1]);
		MSG_WriteFloat(buf, cmd->cursor_impact[2]);
		MSG_WriteEntity(buf, cmd->cursor_entitynumber);
	}
	if (bits & UC_LIGHTLEV)
		MSG_WriteUInt64 (buf, cmd->lightlevel);	//yay hdr?

	if (bits & UC_VR_HEAD)
		MSG_WriteVR(VRDEV_HEAD, buf, from, cmd);
	if (bits & UC_VR_RIGHT)
		MSG_WriteVR(VRDEV_RIGHT, buf, from, cmd);
	if (bits & UC_VR_LEFT)
		MSG_WriteVR(VRDEV_LEFT, buf, from, cmd);
}
#endif
#ifdef HAVE_SERVER
static void MSG_ReadVR(int i, usercmd_t *cmd)
{
	quint64_t bits = MSG_ReadUInt64();
	if (bits & UC_VR_STATUS)
		cmd->vr[i].status = MSG_ReadUInt64();
	if (bits & UC_VR_ANG)
	{
		cmd->vr[i].angles[0] = MSG_ReadShort();
		cmd->vr[i].angles[1] = MSG_ReadShort();
		cmd->vr[i].angles[2] = MSG_ReadShort();
	}
	if (bits & UC_VR_AVEL)
	{
		cmd->vr[i].avelocity[0] = MSG_ReadShort();
		cmd->vr[i].avelocity[1] = MSG_ReadShort();
		cmd->vr[i].avelocity[2] = MSG_ReadShort();
	}
	if (bits & UC_VR_ORG)
	{
		cmd->vr[i].origin[0] = MSG_ReadFloat();
		cmd->vr[i].origin[1] = MSG_ReadFloat();
		cmd->vr[i].origin[2] = MSG_ReadFloat();
	}
	if (bits & UC_VR_VEL)
	{
		cmd->vr[i].velocity[0] = MSG_ReadFloat();
		cmd->vr[i].velocity[1] = MSG_ReadFloat();
		cmd->vr[i].velocity[2] = MSG_ReadFloat();
	}
	if (bits & UC_VR_WEAPON)
		cmd->vr[i].weapon = MSG_ReadUInt64();
}
void MSGFTE_ReadDeltaUsercmd (const usercmd_t *from, usercmd_t *cmd)
{
	int i;
	unsigned int bits;

	bits = MSG_ReadUInt64();

	if (bits & UC_UNSUPPORTED)
	{
		if (!msg_badread)
			Con_Printf("MSG_ReadDeltaUsercmdNew: Unsupported bits (%#x)\n", bits&UC_UNSUPPORTED);
		msg_badread = true;
		return;
	}
	*cmd = *from;
	cmd->servertime = from->servertime+MSG_ReadUInt64();
	cmd->fservertime = cmd->servertime/1000.0;
	if (bits & UC_MSEC_DEBUG)
		cmd->msec = MSG_ReadUInt64();	//for debugging only. only sent when developer 1, for now.
	else
		cmd->msec = 0;	//no info...
	for (i = 0; i < 3; i++)
	{
		if (bits & (UC_ANGLE1<<i))
		{
			if (bits & UC_ABSANG)
				cmd->angles[i] = MSG_ReadShort();
			else
				cmd->angles[i] = from->angles[i]+MSG_ReadChar();
		}
	}
	if (bits & UC_FORWARD)
	{
		if (bits & UC_BIGMOVES)
			cmd->forwardmove = MSG_ReadInt64();
		else
			cmd->forwardmove = MSG_ReadChar()*5;
	}
	if (bits & UC_RIGHT)
	{
		if (bits & UC_BIGMOVES)
			cmd->sidemove = MSG_ReadInt64();
		else
			cmd->sidemove = MSG_ReadChar()*5;
	}
	if (bits & UC_UP)
	{
		if (bits & UC_BIGMOVES)
			cmd->upmove = MSG_ReadInt64();
		else
			cmd->upmove = MSG_ReadChar()*5;
	}

	if (bits & UC_BUTTONS)
		cmd->buttons = MSG_ReadUInt64();
	if (bits & UC_IMPULSE)
		cmd->impulse = MSG_ReadUInt64();
	if (bits & UC_WEAPON)
		cmd->weapon = MSG_ReadUInt64();
	if (bits & UC_CURSORFLDS)
	{	//prydon cursor crap. kinda bloated.
		cmd->cursor_screen[0] = MSG_ReadShort() / 32767.0;
		cmd->cursor_screen[1] = MSG_ReadShort() / 32767.0;
		cmd->cursor_start[0] = MSG_ReadFloat();	//avoiding WriteAngle/WriteCoord means we can avoid netprim size difference issues.
		cmd->cursor_start[1] = MSG_ReadFloat();
		cmd->cursor_start[2] = MSG_ReadFloat();
		cmd->cursor_impact[0] = MSG_ReadFloat();
		cmd->cursor_impact[1] = MSG_ReadFloat();
		cmd->cursor_impact[2] = MSG_ReadFloat();
		cmd->cursor_entitynumber = MSG_ReadBigEntity();
	}
	if (bits & UC_LIGHTLEV)
		cmd->lightlevel = MSG_ReadUInt64();
	if (bits & UC_VR_HEAD)
		MSG_ReadVR(VRDEV_HEAD, cmd);
	if (bits & UC_VR_RIGHT)
		MSG_ReadVR(VRDEV_RIGHT, cmd);
	if (bits & UC_VR_LEFT)
		MSG_ReadVR(VRDEV_LEFT, cmd);
}
#endif
void MSGQW_WriteDeltaUsercmd (sizebuf_t *buf, const usercmd_t *from, const usercmd_t *cmd)
{
	int		bits;

//
// send the movement message
//
	bits = 0;
#if defined(Q2CLIENT) && defined(HAVE_CLIENT)
	if (cls_state && cls.protocol == CP_QUAKE2)
		MSGQ2_WriteDeltaUsercmd(buf, from, cmd);
	else
#endif
	{
		if (cmd->angles[0] != from->angles[0])
			bits |= CM_ANGLE1;
		if (cmd->angles[1] != from->angles[1])
			bits |= CM_ANGLE2;
		if (cmd->angles[2] != from->angles[2])
			bits |= CM_ANGLE3;
		if (cmd->forwardmove != from->forwardmove)
			bits |= CM_FORWARD;
		if (cmd->sidemove != from->sidemove)
			bits |= CM_SIDE;
		if (cmd->upmove != from->upmove)
			bits |= CM_UP;
		if (cmd->buttons != from->buttons)
			bits |= CM_BUTTONS;
		if (cmd->impulse != from->impulse)
			bits |= CM_IMPULSE;

		MSG_WriteByte (buf, bits);

		if (bits & CM_ANGLE1)
			MSG_WriteShort (buf, cmd->angles[0]);
		if (bits & CM_ANGLE2)
			MSG_WriteShort (buf, cmd->angles[1]);
		if (bits & CM_ANGLE3)
			MSG_WriteShort (buf, cmd->angles[2]);

		if (bits & CM_FORWARD)
			MSG_WriteShort (buf, cmd->forwardmove);
		if (bits & CM_SIDE)
			MSG_WriteShort (buf, cmd->sidemove);
		if (bits & CM_UP)
			MSG_WriteShort (buf, cmd->upmove);

		if (bits & CM_BUTTONS)
			MSG_WriteByte (buf, cmd->buttons);
		if (bits & CM_IMPULSE)
			MSG_WriteByte (buf, cmd->impulse);
		MSG_WriteByte (buf, bound(0, cmd->msec, 255));
	}
}

#ifdef HAVE_CLIENT
void MSGCL_WriteDeltaUsercmd (sizebuf_t *buf, const usercmd_t *from, const usercmd_t *cmd)
{
#if defined(Q2CLIENT)
	if (cls_state && cls.protocol == CP_QUAKE2)
	{
		if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
			MSGQ2EX_WriteDeltaUsercmd(buf, from, cmd);
		else
			MSGQ2_WriteDeltaUsercmd(buf, from, cmd);
	}
	else
#endif
		MSGQW_WriteDeltaUsercmd(buf, from, cmd);
}
#endif

//
// reading functions
//
qboolean	msg_badread;
struct netprim_s msg_nullnetprim;
sizebuf_t	*msg_readmsg;

void MSG_BeginReading (sizebuf_t *sb, struct netprim_s prim)
{
	msg_readmsg = sb;
	msg_badread = false;
	sb->currentbit = 0;
	sb->packing = SZ_RAWBYTES;
	sb->prim = prim;
}

void MSG_ChangePrimitives(struct netprim_s prim)
{
	net_message.prim = prim;
}

int MSG_GetReadCount(void)
{
	return msg_readmsg->currentbit>>3;
}


/*
============
MSG_ReadRawBytes
============
*/
static int MSG_ReadRawBytes(sizebuf_t *msg, int bits)
{
	int bitmask = 0;
	unsigned int readcount = msg->currentbit>>3;

	if (readcount + (bits>>3) >= msg->cursize)
	{
		msg_badread = true;
		msg->currentbit += bits;
		return -1;
	}

	if (bits <= 8)
	{
		bitmask = (unsigned char)msg->data[readcount];
		msg->currentbit += 8;
	}
	else if (bits <= 16)
	{
		bitmask = (unsigned short)(msg->data[readcount]
			+ (msg->data[readcount+1] << 8));
		msg->currentbit += 16;
	}
	else if (bits <= 32)
	{
		bitmask = msg->data[readcount]
			+ (msg->data[readcount+1] << 8)
			+ (msg->data[readcount+2] << 16)
			+ (msg->data[readcount+3] << 24);
		msg->currentbit += 32;
	}

	return bitmask;
}

/*
============
MSG_ReadRawBits
============
*/
static int MSG_ReadRawBits(sizebuf_t *msg, int bits)
 {
	int i;
	int val;
	int bitmask = 0;

	if (msg->currentbit + bits > (msg->cursize<<3))
	{
		msg_badread = true;
		msg->currentbit = msg->cursize<<3;
		return -1;
	}

	for(i=0 ; i<bits ; i++)
	{
		val = msg->data[msg->currentbit >> 3] >> (msg->currentbit & 7);
		msg->currentbit++;
		bitmask |= (val & 1) << i;
	}

	return bitmask;
}

#ifdef HUFFNETWORK
/*
============
MSG_ReadHuffBits
============
*/
static int MSG_ReadHuffBits(sizebuf_t *msg, int bits)
{
	int i;
	int val;
	int bitmask;
	int remaining = bits & 7;

	bitmask = MSG_ReadRawBits(msg, remaining);

	for (i=0 ; i<bits-remaining ; i+=8)
	{
		val = Huff_GetByte(msg->data, &msg->currentbit);
		bitmask |= val << (i + remaining);
	}

	if (msg->currentbit > (msg->cursize<<3))
	{
		msg_badread = true;
		msg->currentbit = msg->cursize<<3;
		return -1;
	}

	return bitmask;
}
#endif

int MSG_ReadBits(int bits)
{
	int bitmask = 0;
	qboolean extend = false;

#ifdef PARANOID
	if (!bits || bits < -31 || bits > 32)
		Host_EndGame("MSG_ReadBits: bad bits %i", bits );
#endif

	if (bits < 0)
	{
		bits = -bits;
		extend = true;
	}

	switch(msg_readmsg->packing)
	{
	default:
	case SZ_BAD:
		Sys_Error("MSG_ReadBits: bad msg_readmsg->packing");
		break;
	case SZ_RAWBYTES:
		bitmask = MSG_ReadRawBytes(msg_readmsg, bits);
		break;
	case SZ_RAWBITS:
		bitmask = MSG_ReadRawBits(msg_readmsg, bits);
		break;
#ifdef HUFFNETWORK
	case SZ_HUFFMAN:
		bitmask = MSG_ReadHuffBits(msg_readmsg, bits);
		break;
#endif
	}

	if (extend)
	{
		if(bitmask & (1 << (bits - 1)))
		{
			bitmask |= ~((1 << bits) - 1);
		}
	}

	return bitmask;
}

void MSG_ReadSkip(int bytes)
{
	if (msg_readmsg->packing!=SZ_RAWBYTES)
	{
		while (bytes > 4)
		{
			MSG_ReadBits(32);
			bytes-=4;
		}
		while (bytes > 0)
		{
			MSG_ReadBits(8);
			bytes--;
		}
	}
	msg_readmsg->currentbit += bytes<<3;
	if (msg_readmsg->currentbit < 0)
	{
		msg_readmsg->currentbit = 0;
		msg_badread = true;
		return;
	}
	if (msg_readmsg->currentbit > msg_readmsg->cursize<<3)
	{
		msg_readmsg->currentbit = msg_readmsg->cursize<<3;
		msg_badread = true;
		return;
	}
}


// returns -1 and sets msg_badread if no more characters are available
int MSG_ReadChar (void)
{
	int	c;
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
		return MSG_ReadBits(-8);

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+1 > msg_readmsg->cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (signed char)msg_readmsg->data[msg_readcount];
	msg_readcount++;
	msg_readmsg->currentbit = msg_readcount<<3;

	return c;
}

int MSG_PeekByte(void)
{
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
		return -1;

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+1 > msg_readmsg->cursize)
		return -1;

	return (unsigned char)msg_readmsg->data[msg_readcount];
}

int MSG_ReadByte (void)
{
	unsigned char	c;
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
		return MSG_ReadBits(8);

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+1 > msg_readmsg->cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (unsigned char)msg_readmsg->data[msg_readcount];
	msg_readcount++;
	msg_readmsg->currentbit = msg_readcount<<3;

	return c;
}

int MSG_ReadShort (void)
{
	int	c;
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
		return (short)MSG_ReadBits(16);

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+2 > msg_readmsg->cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (short)(msg_readmsg->data[msg_readcount]
	+ (msg_readmsg->data[msg_readcount+1]<<8));

	msg_readcount += 2;
	msg_readmsg->currentbit = msg_readcount<<3;

	return c;
}

int MSG_ReadUInt16 (void)
{
	int	c;
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
		return (short)MSG_ReadBits(16);

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+2 > msg_readmsg->cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = (unsigned short)(msg_readmsg->data[msg_readcount]
	+ (msg_readmsg->data[msg_readcount+1]<<8));

	msg_readcount += 2;
	msg_readmsg->currentbit = msg_readcount<<3;

	return c;
}

int MSG_ReadLong (void)
{
	int	c;
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
		return (int)MSG_ReadBits(32);

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+4 > msg_readmsg->cursize)
	{
		msg_badread = true;
		return -1;
	}

	c = msg_readmsg->data[msg_readcount]
	+ (msg_readmsg->data[msg_readcount+1]<<8)
	+ (msg_readmsg->data[msg_readcount+2]<<16)
	+ (msg_readmsg->data[msg_readcount+3]<<24);

	msg_readcount += 4;
	msg_readmsg->currentbit = msg_readcount<<3;

	return c;
}
quint64_t MSG_ReadULEB128 (void)
{
	quint64_t r = 0;
	qbyte b, o=0;
	while (!msg_badread)
	{
		b = MSG_ReadByte();
		r |= (b&0x7f)<<o;
		o+=7;
		if (!(b & 0x80))
			break;
	}
	return r;
}
qint64_t MSG_ReadSignedQEX (void)
{	//this is not signed leb128 (which would normally just sign-extend)
	quint64_t c = MSG_ReadULEB128();
	if (c&1)
		return -1-(qint64_t)(c>>1);
	else
		return (qint64_t)(c>>1);
}
quint64_t MSG_ReadUInt64 (void)
{	//0* 10*,*, 110*,*,* etc, up to 0xff followed by 8 continuation bytes
	qbyte l=0x80, v, b = 0;
	quint64_t r;
	v = MSG_ReadByte();
	for (; v&l; l>>=1)
	{
		v-=l;
		b++;
	}
	r = (quint64_t)v<<(b*8);
	while(b --> 0)
		r |= (quint64_t)MSG_ReadByte()<<(b*8);
	return r;
}
qint64_t MSG_ReadInt64 (void)
{	//we do some fancy bit recoding for more efficient length coding.
	quint64_t c = MSG_ReadUInt64();
	if (c&1)
		return -1-(qint64_t)(c>>1);
	else
		return (qint64_t)(c>>1);
}

float MSG_ReadFloat (void)
{
	union
	{
		qbyte	b[4];
		float	f;
		int	l;
	} dat;
	unsigned int msg_readcount;

	if (msg_readmsg->packing!=SZ_RAWBYTES)
	{
		dat.l = MSG_ReadBits(32);
		return dat.f;
	}

	msg_readcount = msg_readmsg->currentbit>>3;
	if (msg_readcount+4 > msg_readmsg->cursize)
	{
		msg_badread = true;
		return -1;
	}

	dat.b[0] =	msg_readmsg->data[msg_readcount];
	dat.b[1] =	msg_readmsg->data[msg_readcount+1];
	dat.b[2] =	msg_readmsg->data[msg_readcount+2];
	dat.b[3] =	msg_readmsg->data[msg_readcount+3];
	msg_readcount += 4;
	msg_readmsg->currentbit = msg_readcount<<3;

	if (bigendian)
		dat.l = LittleLong (dat.l);

	return dat.f;
}
double MSG_ReadDouble (void)
{
	union
	{
		quint64_t l;
		double	f;
	} dat;
	unsigned int msg_readcount = msg_readmsg->currentbit>>3;

	if (msg_readcount+8 > net_message.cursize)
	{
		msg_badread = true;
		return -1;
	}

	dat.l = (           net_message.data[msg_readcount+0]<< 0)|
			(           net_message.data[msg_readcount+1]<< 8)|
			(           net_message.data[msg_readcount+2]<<16)|
			(           net_message.data[msg_readcount+3]<<24)|
			((quint64_t)net_message.data[msg_readcount+4]<<32)|
			((quint64_t)net_message.data[msg_readcount+5]<<40)|
			((quint64_t)net_message.data[msg_readcount+6]<<48)|
			((quint64_t)net_message.data[msg_readcount+7]<<56);
	msg_readcount += 8;

	return dat.f;
}

char *MSG_ReadStringBuffer (char *out, size_t outsize)
{
	int		l,c;

	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (msg_badread || c == 0)
			break;
		out[l] = c;
		l++;
	} while (l < outsize-1);

	out[l] = 0;

	return out;
}
char *MSG_ReadString (void)
{
	static char	string[65536];
	int		l,c;

	l = 0;
	for(;;)
	{
		c = MSG_ReadChar ();
		if (msg_badread || c == 0)
			break;
		if (l < sizeof(string)-1)
			string[l++] = c;
		else
			msg_badread = true;
	}

	string[l] = 0;

	return string;
}

char *MSG_ReadStringLine (void)
{
	static char	string[2048];
	int		l,c;

	l = 0;
	do
	{
		c = MSG_ReadChar ();
		if (msg_badread || c == 0 || c == '\n')
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);

	string[l] = 0;

	return string;
}

float MSG_ReadCoord (void)
{
	coorddata c = {{0}};
	unsigned char coordtype = msg_readmsg->prim.coordtype;
	if (coordtype == COORDTYPE_UNDEFINED)
	{
		static float throttle;
		Con_ThrottlePrintf(&throttle, 0, CON_WARNING"MSG_ReadCoord: primitives not yet configured. assuming 13.3\n");
		coordtype = COORDTYPE_FIXED_13_3;
	}
	if ((coordtype&COORDTYPE_SIZE_MASK)>sizeof(c))
		return 0;
	MSG_ReadData(c.b, coordtype&COORDTYPE_SIZE_MASK);
	return MSG_FromCoord(c, coordtype);
}
float MSG_ReadCoordFloat (void)
{
	coorddata c = {{0}};
	MSG_ReadData(c.b, COORDTYPE_FLOAT_32&COORDTYPE_SIZE_MASK);
	return MSG_FromCoord(c, COORDTYPE_FLOAT_32);
}

void MSG_ReadPos (float *pos)
{
	pos[0] = MSG_ReadCoord();
	pos[1] = MSG_ReadCoord();
	pos[2] = MSG_ReadCoord();
}

#if 1//defined(Q2SERVER) || !defined(SERVERONLY)
#define Q2NUMVERTEXNORMALS	162
vec3_t	bytedirs[Q2NUMVERTEXNORMALS] =
{
#include "../client/q2anorms.h"
};
#endif
#ifdef HAVE_CLIENT
void MSG_ReadDir (vec3_t dir)
{
	int		b;

	b = MSG_ReadByte ();
	if (b >= Q2NUMVERTEXNORMALS)
	{
		CL_DumpPacket();
		Host_EndGame ("MSG_ReadDir: out of range");
	}
	VectorCopy (bytedirs[b], dir);
}
#endif
#if 1//def Q2SERVER
void MSG_WriteDir (sizebuf_t *sb, float dir[3])
{
	int		i, best;
	float	d, bestd;

	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<Q2NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	MSG_WriteByte (sb, best);
}
#endif

float MSG_ReadAngle16 (void)
{
	return MSG_ReadShort() * (360.0/65536);
}
float MSG_ReadAngle (void)
{
	int sz = msg_readmsg->prim.anglesize;
	if (!sz)
	{
		static float throttle;
		Con_ThrottlePrintf(&throttle, 0, CON_WARNING"MSG_ReadAngle: primitives not yet configured. assuming 8 bit\n");
		sz = 1;
	}
	switch(sz)
	{
	case 2:
		return MSG_ReadAngle16();
	case 4:
		return MSG_ReadFloat();
	case 1:
		return MSG_ReadChar() * (360.0/256);
	default:
		Sys_Error("Bad angle size\n");
		return 0;
	}
}

void MSGQW_ReadDeltaUsercmd (const usercmd_t *from, usercmd_t *move, int protover)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte ();

	if (protover <= 26)
	{
		if (bits & CM_ANGLE1)
			move->angles[0] = MSG_ReadShort();
		if (1)
			move->angles[1] = MSG_ReadShort();
		if (bits & CM_ANGLE3)
			move->angles[2] = MSG_ReadShort();

		if (bits & CM_FORWARD)
			move->forwardmove = MSG_ReadByte()<<3;
		if (bits & CM_SIDE)
			move->sidemove = MSG_ReadByte()<<3;
		if (bits & CM_UP)
			move->upmove = MSG_ReadByte()<<3;

		// read buttons
		if (bits & CM_BUTTONS)
			move->buttons = MSG_ReadByte();

		if (bits & CM_IMPULSE)
			move->impulse = MSG_ReadByte();

// read time to run command
		if (bits & CM_ANGLE2)
			move->msec = MSG_ReadByte();
	}
	else
	{
// read current angles
		if (bits & CM_ANGLE1)
			move->angles[0] = MSG_ReadShort();
		if (bits & CM_ANGLE2)
			move->angles[1] = MSG_ReadShort();
		if (bits & CM_ANGLE3)
			move->angles[2] = MSG_ReadShort();

// read movement
		if (bits & CM_FORWARD)
			move->forwardmove = MSG_ReadShort();
		if (bits & CM_SIDE)
			move->sidemove = MSG_ReadShort();
		if (bits & CM_UP)
			move->upmove = MSG_ReadShort();

// read buttons
		if (bits & CM_BUTTONS)
			move->buttons = MSG_ReadByte();

		if (bits & CM_IMPULSE)
			move->impulse = MSG_ReadByte();

// read time to run command
		move->msec = MSG_ReadByte();
	}
}

#ifdef HAVE_SERVER
void MSGQ2_ReadDeltaUsercmd (client_t *cl, const usercmd_t *from, usercmd_t *move)
{
	int bits;
	unsigned int buttons = 0;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte ();

	if (msg_readmsg->prim.flags & NPQ2_R1Q2_UCMD)
		buttons = MSG_ReadByte();

	if (cl->protocol == SCP_QUAKE2EX)
	{
		if (bits & Q2CM_ANGLE1)
			move->angles[0] = ANGLE2SHORT(MSG_ReadFloat());
		if (bits & Q2CM_ANGLE2)
			move->angles[1] = ANGLE2SHORT(MSG_ReadFloat());
		if (bits & Q2CM_ANGLE3)
			move->angles[2] = ANGLE2SHORT(MSG_ReadFloat());

		// read movement
		if (bits & Q2CM_FORWARD)
			move->forwardmove = MSG_ReadFloat ();
		if (bits & Q2CM_SIDE)
			move->sidemove = MSG_ReadFloat();
		if (bits & Q2CM_UP)
			Con_DPrintf("Q2CM_UP unexpected\n");

		if (bits & Q2CM_BUTTONS)
			move->buttons = MSG_ReadByte ();

		if (bits & Q2CM_IMPULSE/*repurposed*/)
			move->sequence = MSG_ReadLong();

		if (move->buttons & (1<<3))
			move->upmove = 200;
		else if (move->buttons & (1<<4))
			move->upmove = -200;
	}
	else
	{
		// read current angles
		if (bits & Q2CM_ANGLE1)
		{
			if (buttons & R1Q2_BUTTON_BYTE_ANGLE1)
				move->angles[0] = MSG_ReadChar ()*64;
			else
				move->angles[0] = MSG_ReadShort ();
		}
		if (bits & Q2CM_ANGLE2)
		{
			if (buttons & R1Q2_BUTTON_BYTE_ANGLE2)
				move->angles[1] = MSG_ReadChar ()*256;
			else
				move->angles[1] = MSG_ReadShort ();
		}
		if (bits & Q2CM_ANGLE3)
			move->angles[2] = MSG_ReadShort ();

		// read movement
		if (bits & Q2CM_FORWARD)
		{
			if (buttons & R1Q2_BUTTON_BYTE_FORWARD)
				move->forwardmove = MSG_ReadChar ()*5;
			else
				move->forwardmove = MSG_ReadShort ();
		}
		if (bits & Q2CM_SIDE)
		{
			if (buttons & R1Q2_BUTTON_BYTE_SIDE)
				move->sidemove = MSG_ReadChar ()*5;
			else
				move->sidemove = MSG_ReadShort ();
		}
		if (bits & Q2CM_UP)
		{
			if (buttons & R1Q2_BUTTON_BYTE_UP)
				move->upmove = MSG_ReadChar ()*5;
			else
				move->upmove = MSG_ReadShort ();
		}

		// read buttons
		if (bits & Q2CM_BUTTONS)
		{
			if (msg_readmsg->prim.flags & NPQ2_R1Q2_UCMD)
				move->buttons = buttons & (1|2|128);	//only use the bits that are actually buttons, so gamecode can't get excited despite being crippled by this.
			else
				move->buttons = MSG_ReadByte ();
		}

		if (bits & Q2CM_IMPULSE)
			move->impulse = MSG_ReadByte ();
	}

	// read time to run command
	move->msec = MSG_ReadByte ();

	if (cl->protocol == SCP_QUAKE2EX)
		move->lightlevel = 255; //light level removed.
	else
		move->lightlevel = MSG_ReadByte ();
}
#endif

void MSG_ReadData (void *data, int len)
{
	int		i;

	for (i=0 ; i<len ; i++)
		((qbyte *)data)[i] = MSG_ReadByte ();
}


//===========================================================================

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void	*data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Sys_Error ("SZ_GetSpace: overflow without allowoverflow set (%d)", buf->maxsize);

		Sys_Printf ("SZ_GetSpace: overflow (%i+%i bytes of %i)\n", buf->cursize, length, buf->maxsize);	// because Con_Printf may be redirected
		SZ_Clear (buf);
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	Q_memcpy (SZ_GetSpace(buf,length),data,length);
}

void SZ_Print (sizebuf_t *buf, const char *data)
{
	int		len;

	len = Q_strlen(data)+1;

	if (!buf->cursize || buf->data[buf->cursize-1])
		Q_memcpy ((qbyte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
	else
	{
		qbyte *msg;
		msg = (qbyte*)SZ_GetSpace(buf, len-1);
		if (msg == buf->data)	//whoops. SZ_GetSpace can return buf->data if it overflowed.
			msg++;
		Q_memcpy (msg-1,data,len); // write over trailing 0
	}
}


//============================================================================

qboolean COM_TrimString(char *str, char *buffer, int buffersize)
{
	int i;
	if (buffersize <= 0)
	{
		Sys_Error("COM_TrimString: no buffer\n");
		return false;
	}

	while (*str <= ' ' && *str>'\0')
		str++;

	for (i = 0; ; i++)
	{
		if (i == buffersize-1)
		{
			buffer[i] = '\0';
			return false;
		}
		if (*str <= ' ')
			break;
		buffer[i] = *str++;
	}
	buffer[i] = '\0';
	return true;
}

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/' || *pathname == '\\')
			last = pathname+1;
		pathname++;
	}
	return (char *)last;
}

/*
============
COM_StripExtension
============
*/
void QDECL COM_StripExtension (const char *in, char *out, int outlen)
{
	char *s;

	if (out != in)	//optimisation, most calls use the same buffer
		Q_strncpyz(out, in, outlen);

	s = out+strlen(out);

	while(*s != '/' && s != out)
	{
		if (*s == '.')
		{
			*s = 0;

			//some extensions don't really count, strip the next one too...
			if (!strcmp(s+1,"gz") || !strcmp(s+1,"xz"))
				;
			else
				break;
		}

		s--;
	}
}

void COM_StripAllExtensions (const char *in, char *out, int outlen)
{
	char *s;

	if (out != in)
		Q_strncpyz(out, in, outlen);

	s = out+strlen(out);

	while(*s != '/' && s != out)
	{
		if (*s == '.')
		{
			*s = 0;
		}

		s--;
	}
}

/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (const char *in, char *result, size_t sizeofresult)
{
	int		i;
	const char *dot;

	for (dot = in + strlen(in); dot >= in && *dot != '.' && *dot != '/' && *dot != '\\'; dot--)
		;
	if (dot < in || *dot != '.')
	{
		*result = 0;
		return result;
	}
	in = dot;

	in++;
	for (i=0 ; i<sizeofresult-1 && *in ; i++,in++)
		result[i] = *in;
	result[i] = 0;
	return result;
}

//returns a pointer to the extension text, including the dot
//term is the end of the string (or null, to make things easy). if its a previous (non-empty) return value, then you can scan backwards to skip .gz or whatever extra postfixes.
const char *COM_GetFileExtension (const char *in, const char *term)
{
	const char *dot;

	if (!term)
		term = in + strlen(in);

	for (dot = term-1; dot >= in && *dot != '/' && *dot != '\\'; dot--)
	{
		if (*dot == '.')
			return dot;
	}
	return "";
}

//Quake 2's tank model has a borked skin (or two).
void COM_CleanUpPath(char *str)
{
	char *dots;
	char *slash;
	int criticize = 0;
	for (dots = str; *dots; dots++)
	{
		/*if (*dots >= 'A' && *dots <= 'Z')
		{
			*dots = *dots - 'A' + 'a';
			criticize = 1;
		}
		else */if (*dots == '\\')
		{
			*dots = '/';
			criticize = 2;
		}
	}
	while ((dots = strstr(str, "..")))
	{
		criticize = 0;
		for (slash = dots-1; slash >= str; slash--)
		{
			if (*slash == '/')
			{
				memmove(slash, dots+2, strlen(dots+2)+1);
				criticize = 3;
				break;
			}
		}
		if (criticize != 3)
		{
			memmove(dots, dots+2, strlen(dots+2)+1);
			criticize = 3;
		}
	}
	while(*str == '/')
	{
		memmove(str, str+1, strlen(str+1)+1);
		criticize = 4;
	}
/*	if(criticize)
	{
		if (criticize == 1)	//not a biggy, so not red.
			Con_Printf("Please fix file case on your files\n");
		else if (criticize == 2)	//you're evil.
			Con_Printf("^1NEVER use backslash in a quake filename (we like portability)\n");
		else if (criticize == 3)	//compleatly stupid. The main reason why this function exists. Quake2 does it!
			Con_Printf("You realise that relative paths are a waste of space?\n");
		else if (criticize == 4)	//AAAAHHHHH! (consider sys_error instead)
			Con_Printf("^1AAAAAAAHHHH! An absolute path!\n");
	}
*/
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (const char *in, char *out, int outlen)
{
	const char *s, *s2;

	s = in + strlen(in) - 1;

	while (s > in)
	{
		if ((*s == '.'&&strcmp(s+1,"gz")&&strcmp(s+1,"xz")) || *s == '/')
			break;
		s--;
	}

	for (s2 = s ; s2 > in && *s2 && *s2 != '/' ; s2--)
		;

	if (s-s2 < 2)
	{
		if (s == s2)
			Q_strncpyz(out, in, outlen);
		else
			Q_strncpyz(out,"?model?", outlen);
	}
	else
	{
		s--;
		outlen--;
		if (outlen > s-s2)
			outlen = s-s2;
		Q_strncpyS (out,s2+1, outlen);
		out[outlen] = 0;
	}
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, const char *extension, int maxlen)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (src > path && *src != '/')
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	if (*extension != '.')
		Q_strncatz (path, ".", maxlen);
	Q_strncatz (path, extension, maxlen);
}

//adds .ext only if it isn't already present (either case).
//extension *must* contain a leading . as this is really a requiresuffix rather than an actual extension
//returns false if truncated. will otherwise still succeed.
qboolean COM_RequireExtension(char *path, const char *extension, int maxlen)
{
	qboolean okay = true;
	int plen = strlen(path);
	int elen = strlen(extension);

	//check if its aready suffixed
	if (plen >= elen)
	{
		if (!Q_strcasecmp(path+plen-elen, extension))
			return okay;
	}

	//truncate if required
	if (plen+1+elen > maxlen)
	{
		if (elen+1 > maxlen)
			Sys_Error("extension longer than path buffer");
		okay = false;
		plen = maxlen - 1+elen;
	}

	//do the copy
	while(*extension)
		path[plen++] = *extension++;
	path[plen] = 0;
	return okay;
}

//errors:
//1 sequence error
//2 over-long
//3 invalid unicode char
//4 invalid utf-16 lead/high surrogate
//5 invalid utf-16 tail/low surrogate
unsigned int utf8_decode(int *error, const void *in, char const**out)
{
	//uc is the output unicode char
	unsigned int uc = 0xfffdu;	//replacement character
	//l is the length
	unsigned int l = 1;
	const unsigned char *str = in;

	if ((*str & 0xe0) == 0xc0)
	{
		if ((str[1] & 0xc0) == 0x80)
		{
			l = 2;
			uc = ((str[0] & 0x1f)<<6) | (str[1] & 0x3f);
			if (!uc || uc >= (1u<<7))	//allow modified utf-8
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xf0) == 0xe0)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80)
		{
			l = 3;
			uc = ((str[0] & 0x0f)<<12) | ((str[1] & 0x3f)<<6) | ((str[2] & 0x3f)<<0);
			if (uc >= (1u<<11))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xf8) == 0xf0)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80)
		{
			l = 4;
			uc = ((str[0] & 0x07)<<18) | ((str[1] & 0x3f)<<12) | ((str[2] & 0x3f)<<6) | ((str[3] & 0x3f)<<0);
			if (uc >= (1u<<16))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xfc) == 0xf8)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 5;
			uc = ((str[0] & 0x03)<<24) | ((str[1] & 0x3f)<<18) | ((str[2] & 0x3f)<<12) | ((str[3] & 0x3f)<<6) | ((str[4] & 0x3f)<<0);
			if (uc >= (1u<<21))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xfe) == 0xfc)
	{
		//six bytes
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 6;
			uc = ((str[0] & 0x01)<<30) | ((str[1] & 0x3f)<<24) | ((str[2] & 0x3f)<<18) | ((str[3] & 0x3f)<<12) | ((str[4] & 0x3f)<<6) | ((str[5] & 0x3f)<<0);
			if (uc >= (1u<<26))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	//0xfe and 0xff, while plausable leading bytes, are not permitted.
#if 0
	else if ((*str & 0xff) == 0xfe)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 7;
			uc = 0 | ((str[1] & 0x3f)<<30) | ((str[2] & 0x3f)<<24) | ((str[3] & 0x3f)<<18) | ((str[4] & 0x3f)<<12) | ((str[5] & 0x3f)<<6) | ((str[6] & 0x3f)<<0);
			if (uc >= (1u<<31))
				*error = 0;
			else
				*error = 2;
		}
		else *error = 1;
	}
	else if ((*str & 0xff) == 0xff)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80 && (str[4] & 0xc0) == 0x80)
		{
			l = 8;
			uc = 0 | ((str[1] & 0x3f)<<36) | ((str[2] & 0x3f)<<30) | ((str[3] & 0x3f)<<24) | ((str[4] & 0x3f)<<18) | ((str[5] & 0x3f)<<12) | ((str[6] & 0x3f)<<6) | ((str[7] & 0x3f)<<0);
			if (uc >= (1llu<<36))
				*error = false;
			else
				*error = 2;
		}
		else *error = 1;
	}
#endif
	else if (*str & 0x80)
	{
		//sequence error
		*error = 1;
		uc = 0xe000u + *str;
	}
	else 
	{
		//ascii char
		*error = 0;
		uc = *str;
	}

	*out = (const void*)(str + l);

	if (!*error)
	{
		//try to deal with surrogates by decoding the low if we see a high.
		if (uc >= 0xd800u && uc < 0xdc00u)
		{
#if 1
			//cesu-8
			const char *lowend;
			unsigned int lowsur = utf8_decode(error, str + l, &lowend);
			if (*error == 4)
			{
				*out = lowend;
				uc = (((uc&0x3ffu) << 10) | (lowsur&0x3ffu)) + 0x10000;
				*error = false;
			}
			else
#endif
			{
				*error = 3;	//bad - lead surrogate without tail.
			}
		}
		if (uc >= 0xdc00u && uc < 0xe000u)
			*error = 4;	//bad - tail surrogate

		//these are meant to be illegal too
		if (uc == 0xfffeu || uc == 0xffffu || uc > 0x10ffffu)
			*error = 2;	//illegal code
	}

	return uc;
}

unsigned int unicode_decode(int *error, const void *in, char const**out, qboolean markup)
{
	unsigned int charcode;
	if (markup && ((char*)in)[0] == '^' && ((char*)in)[1] == 'U' && ishexcode(((char*)in)[2]) && ishexcode(((char*)in)[3]) && ishexcode(((char*)in)[4]) && ishexcode(((char*)in)[5]))
	{
		*error = 0;
		*out = (char*)in + 6;
		charcode = (dehex(((char*)in)[2]) << 12) | (dehex(((char*)in)[3]) << 8) | (dehex(((char*)in)[4]) << 4) | (dehex(((char*)in)[5]) << 0);
	}
	else if (markup && ((char*)in)[0] == '^' && ((char*)in)[1] == '{')
	{
		*error = 0;
		*out = (char*)in + 2;
		charcode = 0;
		while (ishexcode(**out))
		{
			charcode <<= 4;
			charcode |= dehex(**out);
			*out+=1;
		}
		if (**out == '}')
			*out+=1;
	}
	else if (com_parseutf8.ival > 0)
		charcode = utf8_decode(error, in, out);
	else if (com_parseutf8.ival)
	{
		*error = 0;
		charcode = *(unsigned char*)in;	//iso8859-1
		*out = (char*)in + 1;
	}
	else
	{	//quake
		*error = 0;
		charcode = *(unsigned char*)in;
		if (charcode && charcode != '\n' && charcode != '\t' && charcode != '\r' && (charcode < ' ' || charcode > 127))
			charcode |= 0xe000;
		*out = (char*)in + 1;
	}

	return charcode;
}

unsigned int utf8_encode(void *out, unsigned int unicode, int maxlen)
{
	unsigned int bcount = 1;
	unsigned int lim = 0x80;
	unsigned int shift;
	if (!unicode)
	{	//modified utf-8 encodes encapsulated nulls as over-long.
		bcount = 2;
	}
	else
	{
		while (unicode >= lim)
		{
			if (bcount == 1)
				lim <<= 4;
			else if (bcount < 7)
				lim <<= 5;
			else
				lim <<= 6;
			bcount++;
		}
	}

	//error if needed
	if (maxlen < bcount)
		return 0;

	//output it.
	if (bcount == 1)
	{
		*((unsigned char *)out) = (unsigned char)(unicode&0x7f);
		out = (char*)out + 1;
	}
	else
	{
		shift = bcount*6;
		shift = shift-6;
		*((unsigned char *)out) = (unsigned char)((unicode>>shift)&(0x0000007f>>bcount)) | ((0xffffff00 >> bcount) & 0xff);
		out = (char*)out + 1;
		do
		{
			shift = shift-6;
			*((unsigned char *)out) = (unsigned char)((unicode>>shift)&0x3f) | 0x80;
			out = (char*)out + 1;
		}
		while(shift);
	}
	return bcount;
}

unsigned int qchar_encode(char *out, unsigned int unicode, int maxlen, qboolean markup)
{
	static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	//FIXME: is it a bug that we can't distinguish between true ascii and 0xe0XX ?
	//ntrv are considered special by parsefunstring and are not remapped back to the quake glyphs, so try to keep them as quake glyphs where possible
	if (((unicode >= 32 || unicode == '\n' || unicode == '\t' || unicode == '\r') && unicode < 128) || (unicode >= 0xe000 && unicode <= 0xe0ff && unicode != (0xe000|'\n') && unicode != (0xe000|'\t') && unicode != (0xe000|'\r') && unicode != (0xe000|'\v')))
	{	//quake compatible chars
		if (maxlen < 1)
			return 0;
		*out++ = unicode & 0xff;
		return 1;
	}
	else if (!markup)
	{
		if (maxlen < 1)
			return 0;
		*out++ = '?';
		return 1;
	}
	else if (unicode > 0xffff)
	{	//chars longer than 16 bits
		char *o = out;
		if (maxlen < 11)
			return 0;
		*out++ = '^';
		*out++ = '{';
		if (unicode > 0xfffffff)
			*out++ = hex[(unicode>>28)&15];
		if (unicode > 0xffffff)
			*out++ = hex[(unicode>>24)&15];
		if (unicode > 0xfffff)
			*out++ = hex[(unicode>>20)&15];
		if (unicode > 0xffff)
			*out++ = hex[(unicode>>16)&15];
		if (unicode > 0xfff)
			*out++ = hex[(unicode>>12)&15];
		if (unicode > 0xff)
			*out++ = hex[(unicode>>8)&15];
		if (unicode > 0xf)
			*out++ = hex[(unicode>>4)&15];
		if (unicode > 0x0)
			*out++ = hex[(unicode>>0)&15];
		*out++ = '}';
		return out - o;
	}
	else
	{	//16bit chars
		if (maxlen < 6)
			return 0;
		*out++ = '^';
		*out++ = 'U';
		*out++ = hex[(unicode>>12)&15];
		*out++ = hex[(unicode>>8)&15];
		*out++ = hex[(unicode>>4)&15];
		*out++ = hex[(unicode>>0)&15];
		return 6;
	}
}

unsigned int iso88591_encode(char *out, unsigned int unicode, int maxlen, qboolean markup)
{
	static const char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	if (unicode < 256 || (unicode >= 0xe020 && unicode < 0xe080))
	{	//iso8859-1 compatible chars
		if (maxlen < 1)
			return 0;
		*out++ = unicode;
		return 1;
	}
	else if (!markup)
	{
		if (maxlen < 1)
			return 0;
		*out++ = '?';
		return 1;
	}
	else if (unicode > 0xffff)
	{	//chars longer than 16 bits
		char *o = out;
		if (maxlen < 11)
			return 0;
		*out++ = '^';
		*out++ = '{';
		if (unicode > 0xfffffff)
			*out++ = hex[(unicode>>28)&15];
		if (unicode > 0xffffff)
			*out++ = hex[(unicode>>24)&15];
		if (unicode > 0xfffff)
			*out++ = hex[(unicode>>20)&15];
		if (unicode > 0xffff)
			*out++ = hex[(unicode>>16)&15];
		if (unicode > 0xfff)
			*out++ = hex[(unicode>>12)&15];
		if (unicode > 0xff)
			*out++ = hex[(unicode>>8)&15];
		if (unicode > 0xf)
			*out++ = hex[(unicode>>4)&15];
		if (unicode > 0x0)
			*out++ = hex[(unicode>>0)&15];
		*out++ = '}';
		return out - o;
	}
	else
	{	//16bit chars
		if (maxlen < 6)
			return 0;
		*out++ = '^';
		*out++ = 'U';
		*out++ = hex[(unicode>>12)&15];
		*out++ = hex[(unicode>>8)&15];
		*out++ = hex[(unicode>>4)&15];
		*out++ = hex[(unicode>>0)&15];
		return 6;
	}
}

unsigned int unicode_encode(char *out, unsigned int unicode, int maxlen, qboolean markup)
{
	if (com_parseutf8.ival > 0)
		return utf8_encode(out, unicode, maxlen);
	else if (com_parseutf8.ival)
		return iso88591_encode(out, unicode, maxlen, markup);
	else
		return qchar_encode(out, unicode, maxlen, markup);
}

//char-based strlen.
unsigned int unicode_charcount(const char *in, size_t buffersize, qboolean markup)
{
	int error;
	const char *end = in + buffersize;
	int chars = 0;
	for(chars = 0; in < end && *in; chars+=1)
	{
		unicode_decode(&error, in, &in, markup);

		if (in > end)
			break;	//exceeded buffer size uncleanly
	}
	return chars;
}

//handy hacky function.
unsigned int unicode_byteofsfromcharofs(const char *str, unsigned int charofs, qboolean markup)
{
	const char *in = str;
	int error;
	int chars;
	for(chars = 0; *in; chars+=1)
	{
		if (chars >= charofs)
			return in - str;

		unicode_decode(&error, in, &in, markup);
	}
	return in - str;
}
//handy hacky function.
unsigned int unicode_charofsfrombyteofs(const char *str, unsigned int byteofs, qboolean markup)
{
	int error;
	const char *end = str + byteofs;
	int chars = 0;
	for(chars = 0; str < end && *str; chars+=1)
	{
		unicode_decode(&error, str, &str, markup);

		if (str > end)
			break;	//exceeded buffer size uncleanly
	}
	return chars;
}
void unicode_strpad(char *out, size_t outsize, const char *in, qboolean leftalign, size_t minwidth, size_t maxwidth, qboolean markup)
{
	if(com_parseutf8.ival <= 0 && !markup)
	{
		Q_snprintfz(out, outsize, "%*.*s", leftalign ? -(int) minwidth : (int) minwidth, (int) maxwidth, in);
	}
	else
	{
		size_t l = unicode_byteofsfromcharofs(in, maxwidth, markup);
		size_t actual_width = unicode_charcount(in, l, markup);
		int pad = (int)((actual_width >= minwidth) ? 0 : (minwidth - actual_width));
		int prec = (int)l;
		int lpad = leftalign ? 0 : pad;
		int rpad = leftalign ? pad : 0;
		Q_snprintfz(out, outsize, "%*s%.*s%*s", lpad, "", prec, in, rpad, "");
	}
}


#if defined(FTE_TARGET_WEB) || defined(__DJGPP__)
//targets that don't support towupper/towlower...
#define towupper Q_towupper
#define towlower Q_towlower
int towupper(int c)
{
	if (c < 128)
		return toupper(c);
	return c;
}
int towlower(int c)
{
	if (c < 128)
		return tolower(c);
	return c;
}
#endif

size_t unicode_strtoupper(const char *in, char *out, size_t outsize, qboolean markup)
{
	//warning: towupper is locale-specific (eg: turkish has both I and dotted-I and thus i should transform to dotted-I rather than to I).
	//also it can't easily cope with accent prefixes.
	int error;
	unsigned int c;
	size_t l = 0;
	outsize -= 1;

	while(*in)
	{
		c = unicode_decode(&error, in, &in, markup);
		if (c >= 0xe020 && c <= 0xe07f)	//quake-char-aware.
			c = towupper(c & 0x7f) + (c & 0xff80);
		else
			c = towupper(c);
		l = unicode_encode(out, c, outsize - l, markup);
		out += l;
	}
	*out = 0;

	return l;
}

size_t unicode_strtolower(const char *in, char *out, size_t outsize, qboolean markup)
{
	//warning: towlower is locale-specific (eg: turkish has both i and dotless-i and thus I should transform to dotless-i rather than to i).
	//also it can't easily cope with accent prefixes.
	int error;
	unsigned int c;
	size_t l = 0;
	outsize -= 1;

	while(*in)
	{
		c = unicode_decode(&error, in, &in, markup);
		if (c >= 0xe020 && c <= 0xe07f)	//quake-char-aware.
			c = towlower(c & 0x7f) + (c & 0xff80);
		else
			c = towlower(c);
		l = unicode_encode(out, c, outsize - l, markup);
		out += l;
	}
	*out = 0;

	return l;
}

///=====================================

// This is the standard RGBI palette used in CGA text mode
consolecolours_t consolecolours[MAXCONCOLOURS] = {
	{0,    0,    0   }, // black
	{0,    0,    0.67}, // blue
	{0,    0.67, 0   }, // green
	{0,    0.67, 0.67}, // cyan
	{0.67, 0,    0   }, // red
	{0.67, 0,    0.67}, // magenta
	{0.67, 0.33, 0   }, // brown
	{0.67, 0.67, 0.67}, // light gray
	{0.33, 0.33, 0.33}, // dark gray
	{0.33, 0.33, 1   }, // light blue
	{0.33, 1,    0.33}, // light green
	{0.33, 1,    1   }, // light cyan
	{1,    0.33, 0.33}, // light red
	{1,    0.33, 1   }, // light magenta
	{1,    1,    0.33}, // yellow
	{1,    1,    1   }  // white
};

// This is for remapping the Q3 color codes to character masks, including ^9
// if using this table, make sure the truecolour flag is disabled first.
conchar_t q3codemasks[MAXQ3COLOURS] = {
	COLOR_BLACK		<< CON_FGSHIFT,	// 0, black
	COLOR_RED		<< CON_FGSHIFT,	// 1, red
	COLOR_GREEN		<< CON_FGSHIFT,	// 2, green
	COLOR_YELLOW	<< CON_FGSHIFT,	// 3, yellow
	COLOR_BLUE		<< CON_FGSHIFT,	// 4, blue
	COLOR_CYAN		<< CON_FGSHIFT,	// 5, cyan
	COLOR_MAGENTA	<< CON_FGSHIFT,	// 6, magenta
	COLOR_WHITE		<< CON_FGSHIFT,	// 7, white
	(COLOR_WHITE	<< CON_FGSHIFT)|CON_HALFALPHA,	// 8, half-alpha white (BX_COLOREDTEXT)
	COLOR_GREY		<< CON_FGSHIFT	// 9, "half-intensity" (BX_COLOREDTEXT)
};

//Converts a conchar_t string into a char string. returns the null terminator. pass NULL for stop to calc it
char *COM_DeFunString(conchar_t *str, conchar_t *stop, char *out, int outsize, qboolean ignoreflags, qboolean forceutf8)
{
	static char tohex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	unsigned int codeflags, codepoint;
	if (!stop)
	{
		for (stop = str; *stop; stop++)
			;
	}
#ifdef _DEBUG
	if (!outsize)
		Sys_Error("COM_DeFunString given outsize=0");
#endif
	outsize--;

	/*if (ignoreflags)
	{
		while(str < stop)
		{
			if (!--outsize)
				break;
			*out++ = (unsigned char)(*str++&255);
		}
		*out = 0;
	}
	else*/
	{
		unsigned int fl, d;
		unsigned int c;
		int prelinkflags = CON_WHITEMASK;	//if used, its already an error.
		//FIXME: TEST!

		fl = CON_WHITEMASK;
		while(str <= stop)
		{
			if (str == stop)
			{
				codeflags = CON_WHITEMASK;
				codepoint = 0;
				str++;
			}
			else
				str = Font_Decode(str, &codeflags, &codepoint);
			if ((codeflags & CON_HIDDEN) && ignoreflags)
			{
				continue;
			}
			if (codeflags == (CON_LINKSPECIAL | CON_HIDDEN) && codepoint == '[')
			{
				if (!ignoreflags)
				{
					if (outsize<=2)
						break;
					outsize -= 2;
					*out++ = '^';
					*out++ = '[';
				}
				prelinkflags = fl;
				fl = COLOR_RED << CON_FGSHIFT;
				continue;
			}
			else if (codeflags == (CON_LINKSPECIAL | CON_HIDDEN) && codepoint == ']')
			{
				if (!ignoreflags)
				{
					if (outsize<=2)
						break;
					outsize -= 2;
					*out++ = '^';
					*out++ = ']';
				}
				fl = prelinkflags;
				continue;
			}
			else if (codeflags != fl && !ignoreflags)
			{
				d = fl^codeflags;
				if (d & CON_BLINKTEXT)
				{
					if (outsize<=2)
						break;
					outsize -= 2;
					*out++ = '^';
					*out++ = 'b';
				}
				if (d & CON_2NDCHARSETTEXT)
				{	//FIXME: convert to quake glyphs...
					if (!com_parseutf8.ival && !forceutf8 && codepoint >= 32 && codepoint <= 127 && (codeflags&CON_2NDCHARSETTEXT))
					{	//strip the flag and encode it in private use (so it gets encoded as quake-compatible)
						codeflags &= ~CON_2NDCHARSETTEXT;
						codepoint |= 0xe080;
					}
					else
					{
						if (outsize<=2)
							break;
						outsize -= 2;
						*out++ = '^';
						*out++ = 'a';
					}
				}

				if (codeflags & CON_RICHFORECOLOUR)
				{
					if (d & (CON_RICHFORECOLOUR|CON_RICHFOREMASK))
					{
						if (outsize<=5)
							break;
						outsize -= 5;
						*out++ = '^';
						*out++ = 'x';
						*out++ = tohex[(codeflags>>CON_RICHRSHIFT)&15];
						*out++ = tohex[(codeflags>>CON_RICHGSHIFT)&15];
						*out++ = tohex[(codeflags>>CON_RICHBSHIFT)&15];
					}
				}
				else
				{
					if (d & (CON_RICHFORECOLOUR | CON_FGMASK | CON_BGMASK | CON_NONCLEARBG))
					{
						static char q3[16] = {	'0', 0,   0,   0,
												0,   0,   0,   0,
												0,	 '4', '2', '5',
												'1', '6', '3', '7'};
						if (d & CON_RICHFORECOLOUR)
							d = (d&~CON_RICHFOREMASK) | (CON_WHITEMASK&CON_RICHFOREMASK);
						if (!(d & (CON_BGMASK | CON_NONCLEARBG)) && q3[(codeflags & CON_FGMASK) >> CON_FGSHIFT])
						{
							if (outsize<=2)
								break;
							outsize -= 2;

							d = codeflags;
							*out++ = '^';
							*out++ = q3[(codeflags & CON_FGMASK) >> CON_FGSHIFT];
						}
						else
						{
							if (outsize<=4)
								break;
							outsize -= 4;

							*out++ = '^';
							*out++ = '&';
							if ((codeflags & CON_FGMASK) == CON_WHITEMASK)
								*out = '-';
							else
								*out = tohex[(codeflags>>24)&0xf];
							out++;
							if (codeflags & CON_NONCLEARBG)
								*out = tohex[(codeflags>>28)&0xf];
							else
								*out = '-';
							out++;
						}
					}
					if (d & CON_HALFALPHA)
					{
						if (outsize<=2)
							break;
						outsize -= 2;
						*out++ = '^';
						*out++ = 'h';
					}
				}
				fl = codeflags;
			}

			//don't magically show hidden text
			if (ignoreflags && (codeflags & CON_HIDDEN))
				continue;

			if (str > stop)
				break;

			if (forceutf8)
				c = utf8_encode(out, codepoint, outsize-1);
			else
				c = unicode_encode(out, codepoint, outsize-1, !ignoreflags);
			if (!c)
				break;
			outsize -= c;
			out += c;
		}
		*out = 0;
	}
	return out;
}

#ifdef HAVE_LEGACY
static unsigned int koi2wc (unsigned char uc)
{
	static const char koi2wc_table[64] =
	{
			0x4e,0x30,0x31,0x46,0x34,0x35,0x44,0x33,0x45,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,
			0x3f,0x4f,0x40,0x41,0x42,0x43,0x36,0x32,0x4c,0x4b,0x37,0x48,0x4d,0x49,0x47,0x4a,
			0x2e,0x10,0x11,0x26,0x14,0x15,0x24,0x13,0x25,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,
			0x1f,0x2f,0x20,0x21,0x22,0x23,0x16,0x12,0x2c,0x2b,0x17,0x28,0x2d,0x29,0x27,0x2a
	};
	if (uc >= 192 /* && (unsigned char)c <= 255 */)
		return koi2wc_table[uc - 192] + 0x400;
	else if (uc == '#' + 128)
		return 0x0451;	// russian small yo
	else if (uc == '3' + 128)
		return 0x0401;	// russian capital yo
	else if (uc == '4' + 128)
		return 0x0404;	// ukrainian capital round E
	else if (uc == '$' + 128)
		return 0x0454;	// ukrainian small round E
	else if (uc == '6' + 128)
		return 0x0406;	// ukrainian capital I
	else if (uc == '&' + 128)
		return 0x0456;	// ukrainian small i
	else if (uc == '7' + 128)
		return 0x0407;	// ukrainian capital I with two dots
	else if (uc == '\'' + 128)
		return 0x0457;	// ukrainian small i with two dots
	else if (uc == '>' + 128)
		return 0x040e;	// belarusian Y
	else if (uc == '.' + 128)
		return 0x045e;	// belarusian y
	else if (uc == '/' + 128)
		return 0x042a;	// russian capital hard sign
	else
		return uc;
}
#endif

enum
{
	BIDI_NEUTRAL,
	BIDI_LTR,
	BIDI_RTL,
};
static char *bidi_chartype;
static unsigned int bidi_charcount;


//semi-colon delimited tokens, without whitespace awareness
char *COM_ParseStringSetSep (const char *data, char sep, char *out, size_t outsize)
{
	int	c;
	size_t	len;

	if (out == com_token)
		COM_AssertMainThread("COM_ParseStringSetSep");

	len = 0;
	out[0] = 0;

	if (data)
	for (;*data;)
	{
		if (len >= outsize-1)
		{
			out[len] = 0;
			return (char*)data;
		}
		c = *data++;
		if (c == sep)
			break;
		out[len++] = c;
	}

	out[len] = 0;
	return (char*)data;
}
void COM_BiDi_Shutdown(void)
{
	bidi_charcount = 0;
	BZ_Free(bidi_chartype);
	bidi_chartype = NULL;
}
static void COM_BiDi_Setup(void)
{
	char *file;
	char *line;
	char *end;
	char *tok;
	unsigned int c;
	qofs_t size;

	COM_AssertMainThread("COM_ParseToken");

	file = FS_MallocFile("bidi.dat", FS_ROOT, &size);
	if (file)
	{
		bidi_chartype = file;
		bidi_charcount = size;
		return;
	}

	file = FS_MallocFile("UnicodeData.txt", FS_ROOT, NULL);
	if (!file)
		return;

	bidi_charcount = 0xffff;
	bidi_chartype = BZ_Malloc(bidi_charcount);
	if (!bidi_chartype)
		bidi_charcount = 0;
	else
	{
		for (c = 0; c < bidi_charcount; c++)
			bidi_chartype[c] = BIDI_NEUTRAL;
		for(line = file; line; line = end)
		{
			end = strchr(line, '\n');
			if (end)
				*end++ = 0;

			tok = COM_ParseStringSetSep(line,';', com_token, sizeof(com_token));	//number
			c = strtoul(com_token, NULL, 16);
			tok = COM_ParseStringSetSep(tok,';', com_token, sizeof(com_token));	//name
			tok = COM_ParseStringSetSep(tok,';', com_token, sizeof(com_token));	//class?
			tok = COM_ParseStringSetSep(tok,';', com_token, sizeof(com_token));	//?
			tok = COM_ParseStringSetSep(tok,';', com_token, sizeof(com_token));	//bidi
			if (c < bidi_charcount)
			{
				if (!Q_strcasecmp(com_token, "R") || !Q_strcasecmp(com_token, "AL"))
					bidi_chartype[c] = BIDI_RTL;
				else if (!Q_strcasecmp(com_token, "L"))
					bidi_chartype[c] = BIDI_LTR;
				else
					bidi_chartype[c] = BIDI_NEUTRAL;
			}
		}

		//trim
		while(bidi_charcount>0 && bidi_chartype[bidi_charcount-1] == BIDI_NEUTRAL)
			bidi_charcount--;
		FS_WriteFile("bidi.dat", bidi_chartype, bidi_charcount, FS_ROOT);
	}
	BZ_Free(file);
}
//bi-direction text is fun.
//the text is specified in input order. the first in the string is the first entered on the keyboard.
//this makes switching direction mid-line quite awkward. so lets hope you don't do that too often, mmkay?
static void COM_BiDi_Parse(conchar_t *fte_restrict start, size_t length)
{
	char fl[2048], next, run, prev, para = BIDI_LTR;
	size_t i, runstart, j, k;
	unsigned int c;
	conchar_t swap;
	if (!bidi_charcount || !length || length > sizeof(fl))
		return;

	for (i = 0; i < length; i++)
	{
		c = start[i] & CON_CHARMASK;
		if (c >= bidi_charcount)
			fl[i] = BIDI_NEUTRAL;
		else
			fl[i] = bidi_chartype[c];
	}
	
	//de-neutralise it
	prev = fl[0];
	for (i = 0; i < length; )
	{
		if (fl[i] == BIDI_NEUTRAL)
		{
			next = prev;	//trailing weak chars can just use the first side
			for (runstart = i; i < length; i++)
			{
				next = fl[i];
				if (next != BIDI_NEUTRAL)
				{
					i--;
					break;
				}
			}
			//this can happen if the only text is neutral
			if (prev == BIDI_NEUTRAL)
				run = next;
			//if the strong cars are the same direction on both side, we can just use that direction
			else if (prev == next)
				run = prev;
			//if the strong chars differ, we revert to the paragraph's direction.
			else
				run = para;

			while(runstart <= i)
				fl[runstart++] = run;
			i++;
		}
		else
		{
			prev = fl[i];
			i++;
		}
	}

	for (run = para, runstart = 0, i = 0; i <= length; i++) 
	{
		if (i >= length)
			next = para;
		else
			next = fl[i];
		if (next != run)
		{
			if (run == BIDI_NEUTRAL)
				break;
			if (run == BIDI_RTL)
			{	//now swap the rtl text
				k = (i-runstart)/2;
				for (j = 0; j < k; j++)
				{
					//FIXME: ( -> ) and vice versa.
					swap = start[runstart+j];
					start[runstart+j] = start[i-j-1];
					start[i-j-1] = swap;
				}
			}
			run = next;
			runstart = i;
		}
	}
}

//Takes a q3-style fun string, and returns an expanded string-with-flags (actual return value is the null terminator)
//outsize parameter is in _BYTES_ (so sizeof is safe).
conchar_t *COM_ParseFunString(conchar_t defaultflags, const char *str, conchar_t *out, int outsize, int flags)
{
	conchar_t extstack[4];
	int extstackdepth = 0;
	unsigned int uc;
	int utf8 = com_parseutf8.ival;
	conchar_t linkinitflags = CON_WHITEMASK;/*doesn't need the init, but msvc is stupid*/
	qboolean keepmarkup = !!(flags & PFS_KEEPMARKUP);
	qboolean linkkeep = keepmarkup;
	qboolean ezquakemess = false;
	conchar_t *linkstart = NULL;

	conchar_t ext;
	conchar_t *oldout = out;
#ifdef HAVE_LEGACY
	extern cvar_t dpcompat_console;
	extern cvar_t ezcompat_markup;

	if (flags & PFS_EZQUAKEMARKUP)
	{
		ezquakemess = true;
		utf8 = 0;
	}
#endif
	if (flags & PFS_FORCEUTF8)
		utf8 = 2;

	outsize /= sizeof(conchar_t);
	if (!outsize)
		return out;
	//then outsize is decremented then checked before each write, so the trailing null has space

#if 0
	while(*str)
	{
		*out++ = CON_WHITEMASK|(unsigned char)*str++;
	}
	*out = 0;
	return out;
#endif

	if (*str == 1 || *str == 2
#ifdef HAVE_LEGACY
		|| (*str == 3 && dpcompat_console.ival)
#endif
		)
	{
		defaultflags ^= CON_2NDCHARSETTEXT;
		str++;
	}

	ext = defaultflags;

	while(*str)
	{
		if ((*str & 0x80) && utf8 > 0)
		{	//check for utf-8
			int decodeerror;
			const char *end;
			uc = utf8_decode(&decodeerror, str, &end);
			if (decodeerror && !(utf8 & 2))
			{
				utf8 &= ~1;
				//malformed encoding we just drop through and stop trying to decode.
				//if its just a malformed or overlong string, we end up with a chunk of 'red' chars.
			}
			else
			{
				if (uc > 0x10ffff)
					uc = 0xfffd;
				if (!--outsize)
					break;

				if (uc > 0xffff)
				{
					if (!--outsize)
						break;
					*out++ = uc>>16 | CON_LONGCHAR | (ext & CON_HIDDEN);
					uc &= 0xffff;
				}
				*out++ = uc | ext;
				str = end;
				continue;
			}
		}
		if (ezquakemess && *str == '^')
		{
			str++;
			uc = (unsigned char)(*str++);
			*out++ = (uc | ext) ^ CON_2NDCHARSETTEXT;
			continue;
		}
		else if (*str == '^' && !(flags & PFS_NOMARKUP))
		{
			if (str[1] >= '0' && str[1] <= '9')
			{	//q3 colour codes
				if (ext & CON_RICHFORECOLOUR)
					ext = (COLOR_WHITE << CON_FGSHIFT) | (ext&~(CON_RICHFOREMASK|CON_RICHFORECOLOUR));
				ext = q3codemasks[str[1]-'0'] | (ext&~(CON_WHITEMASK|CON_HALFALPHA)); //change colour only.
			}
			else if (str[1] == '&') // extended code
			{
				if (isextendedcode(str[2]) && isextendedcode(str[3]))
				{
					if (ext & CON_RICHFORECOLOUR)
						ext = (COLOR_WHITE << CON_FGSHIFT) | (ext&~(CON_RICHFOREMASK|CON_RICHFORECOLOUR));

					// foreground char
					if (str[2] == '-') // default for FG
						ext = (COLOR_WHITE << CON_FGSHIFT) | (ext&~CON_FGMASK);
					else if (str[2] >= 'A')
						ext = ((str[2] - ('A' - 10)) << CON_FGSHIFT) | (ext&~CON_FGMASK);
					else
						ext = ((str[2] - '0') << CON_FGSHIFT) | (ext&~CON_FGMASK);
					// background char
					if (str[3] == '-') // default (clear) for BG
						ext &= ~CON_BGMASK & ~CON_NONCLEARBG;
					else if (str[3] >= 'A')
						ext = ((str[3] - ('A' - 10)) << CON_BGSHIFT) | (ext&~CON_BGMASK) | CON_NONCLEARBG;
					else
						ext = ((str[3] - '0') << CON_BGSHIFT) | (ext&~CON_BGMASK) | CON_NONCLEARBG;

					if (!keepmarkup)
					{
						str += 4;
						continue;
					}
				}
				// else invalid code
				goto messedup;
			}
			else if (str[1] == '[' && !linkstart)
			{
				if (keepmarkup)
				{
					if (!--outsize)
						break;
					*out++ = '^' | CON_HIDDEN;
				}
				if (!--outsize)
					break;

				//preserved flags and reset to white. links must contain their own colours.
				linkinitflags = ext;
				ext = COLOR_RED << CON_FGSHIFT;
				if (!(linkinitflags & CON_RICHFORECOLOUR))
					ext |= linkinitflags & (CON_NONCLEARBG|CON_HALFALPHA|CON_BGMASK);
				linkstart = out;
				*out++ = '[';

				//never keep the markup
				linkkeep = keepmarkup;
				keepmarkup = false;
				str+=2;
				continue;
			}
			else if (str[1] == ']')
			{
				if (keepmarkup)
				{
					if (!--outsize)
						break;
					*out++ = '^' | CON_HIDDEN;
				}

				if (!--outsize)
					break;
				if (linkstart)
				{
					*out++ = ']'|CON_HIDDEN|CON_LINKSPECIAL;
					
					//its a valid link, so we can hide it all now
					*linkstart++ |= CON_HIDDEN|CON_LINKSPECIAL;	//leading [ is hidden
					while(linkstart < out-1 && (*linkstart&CON_CHARMASK) != '\\')	//link text is NOT hidden
						linkstart++;
					while(linkstart < out)	//but the infostring behind it is, as well as the terminator
						*linkstart++ |= CON_HIDDEN;

					//reset colours to how they used to be
					ext = linkinitflags;
					linkstart = NULL;
					keepmarkup = linkkeep;
				}
				else
					*out++ = ']'|CON_LINKSPECIAL;

				//never keep the markup
				str+=2;
				continue;
			}
			else if (str[1] == '`' && str[2] == 'u' && str[3] == '8' && str[4] == ':' && !keepmarkup)
			{
				int l;
				char temp[1024];
				str += 5;
				while(*str)
				{
					l = 0;
					while (*str && l < sizeof(temp)-32 && !(str[0] == '`' && str[1] == '='))
						temp[l++] = *str++;
					//recurse
					temp[l] = 0;
					l = COM_ParseFunString(ext, temp, out, outsize, PFS_FORCEUTF8) - out;
					outsize -= l;
					out += l;
					if (str[0] == '`' && str[1] == '=')
					{
						str+=2;
						break;
					}
				}
				continue;
			}
			else if (str[1] == 'b')
			{
				ext ^= CON_BLINKTEXT;
			}
			else if (str[1] == 'd')
			{
				if (linkstart)
					ext = COLOR_RED << CON_FGSHIFT;
				else
					ext = defaultflags;
			}
			else if (str[1] == 'm'||str[1] == 'a')
				ext ^= CON_2NDCHARSETTEXT;
			else if (str[1] == 'h')
				ext ^= CON_HALFALPHA;
			else if (str[1] == 's')	//store on stack (it's great for names)
			{
				if (extstackdepth < sizeof(extstack)/sizeof(extstack[0]))
				{
					extstack[extstackdepth] = ext;
					extstackdepth++;
				}
			}
			else if (str[1] == 'r')	//restore from stack (it's great for names)
			{
				if (extstackdepth)
				{
					extstackdepth--;
					ext = extstack[extstackdepth];
				}
			}
			else if (str[1] == 'U')	//unicode (16bit) char ^Uxxxx
			{
				if (!keepmarkup)
				{
					uc = 0;
					uc |= dehex(str[2])<<12;
					uc |= dehex(str[3])<<8;
					uc |= dehex(str[4])<<4;
					uc |= dehex(str[5])<<0;

					if (!--outsize)
						break;
					*out++ = uc | ext;
					str += 6;

					continue;
				}
			}
			else if (str[1] == '{')	//unicode (Xbit) char ^{xxxx}
			{
				if (!keepmarkup)
				{
					int len;
					uc = 0;
					for (len = 2; ishexcode(str[len]); len++)
					{
						uc <<= 4;
						uc |= dehex(str[len]);
					}

					//and eat the close too. oh god I hope its there.
					if (str[len] == '}')
						len++;

					if (uc > 0x10ffff)	//utf-16 imposes a limit on standard unicode codepoints (any encoding)
						uc = 0xfffd;

					if (!--outsize)
						break;
					if (uc > 0xffff)	//utf-16 imposes a limit on standard unicode codepoints (any encoding)
					{
						if (!--outsize)
							break;
						*out++ = uc>>16 | CON_LONGCHAR | (ext & CON_HIDDEN);
						uc &= 0xffff;
					}
					*out++ = uc | ext;
					str += len;

					continue;
				}
			}
			else if (str[1] == 'x')	//RGB colours
			{
				if (ishexcode(str[2]) && ishexcode(str[3]) && ishexcode(str[4]))
				{
					int r, g, b;
					r = dehex(str[2]);
					g = dehex(str[3]);
					b = dehex(str[4]);

					ext = (ext & ~CON_RICHFOREMASK) | CON_RICHFORECOLOUR;
					ext |= r<<CON_RICHRSHIFT;
					ext |= g<<CON_RICHGSHIFT;
					ext |= b<<CON_RICHBSHIFT;

					if (!keepmarkup)
					{
						str += 5;
						continue;
					}
				}
			}
			else if (str[1] == '^')
			{
				if (keepmarkup)
				{
					if (!--outsize)
						break;
					*out++ = (unsigned char)(*str) | ext;
				}
				str++;

				if (*str)
					goto messedup;
				continue;
			}
			else
			{
				goto messedup;
			}

			if (!keepmarkup)
			{
				str+=2;
				continue;
			}
		}
#ifdef HAVE_LEGACY
		else if (*str == '&' && str[1] == 'c' && !(flags & PFS_NOMARKUP) && ezcompat_markup.ival)
		{
			// ezQuake color codes

			if (ishexcode(str[2]) && ishexcode(str[3]) && ishexcode(str[4]))
			{
				int r, g, b;
				r = dehex(str[2]);
				g = dehex(str[3]);
				b = dehex(str[4]);

				ext = (ext & ~CON_RICHFOREMASK) | CON_RICHFORECOLOUR;
				ext |= r<<CON_RICHRSHIFT;
				ext |= g<<CON_RICHGSHIFT;
				ext |= b<<CON_RICHBSHIFT;

				if (!keepmarkup)
				{
					str += 5;
					continue;
				}
			}
		}
		else if (*str == '&' && str[1] == 'r' && !(flags & PFS_NOMARKUP) && ezcompat_markup.ival)
		{
			//ezquake revert
			ext = (COLOR_WHITE << CON_FGSHIFT) | (ext&~(CON_RICHFOREMASK|CON_RICHFORECOLOUR));
			if (!keepmarkup)
			{
				str+=2;
				continue;
			}
		}
		else if (str[0] == '=' && str[1] == '`' && str[2] == 'k' && str[3] == '8' && str[4] == ':' && !keepmarkup && ezcompat_markup.ival)
		{
			//ezquake compat: koi8 compat for crazy russian people.
			//we parse for compat but don't generate (they'll see utf-8 from us).
			//this code can just recurse. saves affecting the rest of the code with weird encodings.
			int l;
			char temp[1024];
			str += 5;
			while(*str)
			{
				l = 0;
				while (*str && l < sizeof(temp)-32 && !(str[0] == '`' && str[1] == '='))
					l += utf8_encode(temp+l, koi2wc(*str++), sizeof(temp)-1);
				//recurse
				temp[l] = 0;
				l = COM_ParseFunString(ext, temp, out, outsize, PFS_FORCEUTF8) - out;
				outsize -= l;
				out += l;
				if (str[0] == '`' && str[1] == '=')
				{
					str+=2;
					break;
				}
			}
			continue;
		}
#endif

/*
		else if ((str[0] == 'h' && str[1] == 't' && str[2] == 't' && str[3] == 'p' && str[4] == ':' && !linkstart && !(flags & (PFS_NOMARKUP|PFS_KEEPMARKUP))) ||
				(str[0] == 'h' && str[1] == 't' && str[2] == 't' && str[3] == 'p' && str[4] == 's' && str[5] == ':' && !linkstart && !(flags & (PFS_NOMARKUP|PFS_KEEPMARKUP))))
		{
			//this code can just recurse. saves affecting the rest of the code with weird encodings.
			int l;
			char temp[1024];
			conchar_t *ls, *le;
			l = 0;
			while (*str && l < sizeof(temp)-32 && (
					(*str >= 'a' && *str <= 'z') ||
					(*str >= 'A' && *str <= 'Z') ||
					(*str >= '0' && *str <= '9') ||
					*str == '.' || *str == '/' || *str == '&' || *str == '=' || *str == '_' || *str == '%' || *str == '?' || *str == ':'))
				l += utf8_encode(temp+l, *str++, sizeof(temp)-1);
			//recurse
			temp[l] = 0;

			if (!--outsize)
				break;
			*out++ = CON_LINKSTART;
			ls = out;
			l = COM_ParseFunString(COLOR_BLUE << CON_FGSHIFT, temp, out, outsize, PFS_FORCEUTF8|PFS_NOMARKUP) - out;
			outsize -= l;
			out += l;
			le = out;

			*out++ = '\\' | CON_HIDDEN;
			*out++ = 'u' | CON_HIDDEN;
			*out++ = 'r' | CON_HIDDEN;
			*out++ = 'l' | CON_HIDDEN;
			*out++ = '\\' | CON_HIDDEN;
			while (ls < le)
				*out++ = (*ls++ & CON_CHARMASK) | CON_HIDDEN;
			*out++ = CON_LINKEND;

			if (!--outsize)
				break;
			*out++ = CON_LINKEND;
			continue;
		}
*/
messedup:
		if (!--outsize)
			break;
		uc = (unsigned char)(*str++);
		if (utf8)
		{
			//utf8/iso8859-1 has it easy.
			*out++ = uc | ext;
		}
		else
		{
			if (uc == '\n' || uc == '\r' || uc == '\t' || uc == '\v' || uc == ' ')
				*out++ = uc | ext;
			else if (uc >= 32 && uc < 127)
				*out++ = uc | ext;
			else if (uc >= 0x80+32 && uc <= 0xff)	//anything using high chars is ascii, with the second charset
				*out++ = ((uc&127) | ext) | CON_2NDCHARSETTEXT;
			else	//(other) control chars are regular printables in quake, and are not ascii. These ALWAYS use the bitmap/fallback font.
				*out++ = uc | ext | 0xe000;
		}
	}
	*out = 0;

	COM_BiDi_Parse(oldout, out - oldout);
	return out;
}

//remaps conchar_t character values to something valid in unicode, such that it is likely to be printable with standard char sets.
//unicode-to-ascii is not provided. you're expected to utf-8 the result or something.
//does not handle colour codes or hidden chars. add your own escape sequences if you need that.
//does not guarentee removal of control codes if eg the code was specified as an explicit unicode char.
unsigned int COM_DeQuake(unsigned int chr)
{
	/*only this range are quake chars*/
	if (chr >= 0xe000 && chr < 0xe100)
	{
		chr &= 0xff;
		if (chr >= 146 && chr < 156)
			chr = chr - 146 + '0';
		if (chr >= 0x12 && chr <= 0x1b)
			chr = chr - 0x12 + '0';
		if (chr == 143)
			chr = '.';
		if (chr == 128 || chr == 129 || chr == 130 || chr == 157 || chr == 158 || chr == 159)
			chr = '-';
		if (chr >= 128)
			chr -= 128;
		if (chr == 16)
			chr = '[';
		if (chr == 17)
			chr = ']';
		if (chr == 0x1c)
			chr = 249;
	}
	/*this range contains pictograms*/
	if (chr >= 0xe100 && chr < 0xe200)
	{
		chr = '?';
	}
	return chr;
}

//============================================================================

#define TOKENSIZE sizeof(com_token)
char		com_token[TOKENSIZE];
int		com_argc;
const char	**com_argv;

com_tokentype_t com_tokentype;


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
#ifndef COM_Parse
char *COM_Parse (const char *data)
{
	int		c;
	int		len;

	if (out == com_token)
		COM_AssertMainThread("COM_ParseOut: com_token");

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= TOKENSIZE-1)
				return (char*)data;

			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return (char*)data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse a regular word
	do
	{
		if (len >= TOKENSIZE-1)
			return (char*)data;

		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	com_token[len] = 0;
	return (char*)data;
}
#endif

//semi-colon delimited tokens
char *COM_ParseStringSet (const char *data, char *out, size_t outsize)
{
	int	c;
	int	len;

	if (out == com_token)
		COM_AssertMainThread("COM_ParseOut: com_token");

	len = 0;
	out[0] = 0;

	if (!data)
		return NULL;

// skip whitespace and semicolons
	while ( (c = *data) <= ' ' || c == ';' )
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

	if (*data == '\"')
	{
		return COM_ParseCString(data, out, outsize, NULL);
	}

// parse a regular word
	do
	{
		if (len >= outsize-1)
		{
			out[len] = 0;
			return (char*)data;
		}

		out[len] = c;
		data++;
		len++;
		c = *(unsigned char*)data;
	} while (c>32 && c != ';');

	out[len] = 0;
	return (char*)data;
}


char *COM_ParseType (const char *data, char *out, size_t outlen, com_tokentype_t *toktype)
{
	int		c;
	int		len;

	if (out == com_token)
		COM_AssertMainThread("COM_ParseOut: com_token");

	len = 0;
	out[0] = 0;
	if (toktype)
		*toktype = TTP_EOF;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*')
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		if (toktype)
			*toktype = TTP_STRING;

		data++;
		while (1)
		{
			if (len >= outlen-1)
			{
				out[len] = 0;
				return (char*)data;
			}

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return (char*)data;
			}
			out[len] = c;
			len++;
		}
	}

// parse a regular word
	if (toktype)
		*toktype = TTP_RAWTOKEN;
	do
	{
		if (len >= outlen-1)
		{
			out[len] = 0;
			return (char*)data;
		}

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	out[len] = 0;
	return (char*)data;
}

//same as COM_Parse, but parses two quotes next to each other as a single quote as part of the string
char *COM_StringParse (const char *data, char *token, unsigned int tokenlen, qboolean expandmacros, qboolean qctokenize)
{
#ifdef HAVE_LEGACY
	extern cvar_t dpcompat_console;
#endif
	int		c;
	int		len;
	char *s;

	if (token == com_token)
		COM_AssertMainThread("COM_StringParse: com_token");

	len = 0;
	token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data), (unsigned)c <= ' ' && c != '\n')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}
	if (c == '\n')
	{
		token[len++] = c;
		token[len] = 0;
		return (char*)data+1;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}

//skip / * comments
	if (c == '/' && data[1] == '*' && !qctokenize)
	{
		data+=2;
		while(*data)
		{
			if (*data == '*' && data[1] == '/')
			{
				data+=2;
				goto skipwhite;
			}
			data++;
		}
		goto skipwhite;
	}

	if (c == '\\' && data[1] == '\"')
	{
		return COM_ParseCString(data+1, token, tokenlen, NULL);
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
#ifdef HAVE_LEGACY
		if (dpcompat_console.ival)
		{
			while (1)
			{
				if (len >= tokenlen-1)
				{
					token[len] = '\0';
					return (char*)data;
				}

				c = *data++;
				if (c=='\\' && (*data == '\"' || *data == '\\'))
					c = *data++;	//eat limited escaping inside strings.
				else if (c=='\"')
				{
					token[len] = 0;
					return (char*)data;
				}
				else if (!c)
				{
					token[len] = 0;
					return (char*)data-1;
				}
				token[len] = c;
				len++;
			}
		}
		else
#endif
		{
			while (1)
			{
				if (len >= tokenlen-1)
				{
					token[len] = '\0';
					return (char*)data;
				}

				c = *data++;
				if (c=='\"')
				{
					c = *(data);
					if (c!='\"')
					{
						token[len] = 0;
						return (char*)data;
					}
					data++;
				}
				if (!c)
				{
					token[len] = 0;
					return (char*)data-1;
				}
				token[len] = c;
				len++;
			}
		}
	}

	// handle quoted strings specially
	if (c == '\'' && qctokenize)
	{
		data++;
		while (1)
		{
			if (len >= tokenlen-1)
			{
				token[len] = '\0';
				return (char*)data;
			}


			c = *data++;
			if (c=='\'')
			{
				c = *(data);
				if (c!='\'')
				{
					token[len] = 0;
					return (char*)data;
				}
				while (c=='\'')
				{
					token[len] = c;
					len++;
					data++;
					c = *(data+1);
				}
			}
			if (!c)
			{
				token[len] = 0;
				return (char*)data;
			}
			token[len] = c;
			len++;
		}
	}

	if (qctokenize && (c == '\n' || c == '{' || c == '}' || c == ')' || c == '(' || c == ']' || c == '[' || c == '\'' || c == ':' || c == ',' || c == ';'))
	{
		// single character
		token[len++] = c;
		token[len] = 0;
		return (char*)data+1;
	}

// parse a regular word
	do
	{
		if (len >= tokenlen-1)
		{
			token[len] = '\0';
			return (char*)data;
		}

		token[len] = c;
		data++;
		len++;
		c = *data;
	} while ((unsigned)c>32 && !(qctokenize && (c == '\n' || c == '{' || c == '}' || c == ')' || c == '(' || c == ']' || c == '[' || c == '\'' || c == ':' || c == ',' || c == ';')));

	token[len] = 0;

	if (!expandmacros)
		return (char*)data;

	//now we check for macros.
	for (s = token, c= 0; c < len; c++, s++)	//this isn't a quoted token by the way.
	{
		if (*s == '$')
		{
			cvar_t *macro;
			char name[64];
			int i;

			for (i = 1; i < sizeof(name); i++)
			{
				if (((unsigned char*)s)[i] <= ' ' || s[i] == '$')
					break;
			}

			Q_strncpyz(name, s+1, i);
			i-=1;

			macro = Cvar_FindVar(name);
			if (macro)	//got one...
			{
				if (len+strlen(macro->string)-(i+1) >= tokenlen-1)	//give up.
				{
					token[len] = '\0';
					return (char*)data;
				}
				memmove(s+strlen(macro->string), s+i+1, len-c-i);
				memcpy(s, macro->string, strlen(macro->string));
				s+=strlen(macro->string);
				len+=strlen(macro->string)-(i+1);
			}
		}
	}

	return (char*)data;
}

#define DEFAULT_PUNCTUATION "(,{})(\':;=!><&|+"
char *COM_ParseTokenOut (const char *data, const char *punctuation, char *token, size_t tokenlen, com_tokentype_t *tokentype)
{
	int		c;
	size_t	len;

	if (!punctuation)
		punctuation = DEFAULT_PUNCTUATION;

	if (token == com_token || tokentype == &com_tokentype)
		COM_AssertMainThread("COM_ParseTokenOut: com_token");

	len = 0;
	token[0] = 0;

	if (!data)
	{
		if (tokentype)
			*tokentype = TTP_EOF;
		return NULL;
	}

// skip whitespace
//line endings count as whitespace only if we can report the token type.
skipwhite:
	while ( (c = *(unsigned char*)data) <= ' ' && ((c != '\r' && c != '\n') || !tokentype))
	{
		if (c == 0)
		{
			if (tokentype)
				*tokentype = TTP_EOF;
			return NULL;			// end of file;
		}
		data++;
	}

	//if windows, ignore the \r.
	if (c == '\r' && data[1] == '\n')
		c = *(unsigned char*)data++;

	if (c == '\r' || c == '\n')
	{
		if (tokentype)
			*tokentype = TTP_LINEENDING;
		token[0] = '\n';
		token[1] = '\0';
		data++;
		return (char*)data;
	}

// skip comments
	if (c=='/')
	{
		if (data[1] == '/')
		{	// style comments
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
		else if (data[1] == '*')
		{	/* style comments */
			data+=2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			if (*data)
				data++;
			if (*data)
				data++;
			goto skipwhite;
		}
	}

// handle quoted strings specially
	if (c == '\"')
	{
		if (tokentype)
			*tokentype = TTP_STRING;
		data++;
		while (1)
		{
			if (len >= tokenlen-1)
			{
				token[len] = '\0';
				return (char*)data;
			}
			c = *data++;
			if (c=='\"' || !c)
			{
				token[len] = 0;
				return (char*)data;
			}
			token[len] = c;
			len++;
		}
	}
	if (c == '\\' && data[1] == '\"')
	{
		if (tokentype)
			*tokentype = TTP_STRING;
		return COM_ParseCString(data+1, token, tokenlen, NULL);
	}

// parse single characters
	if (strchr(punctuation, c))
	{
		token[len] = c;
		len++;
		token[len] = 0;
		if (tokentype)
			*tokentype = TTP_PUNCTUATION;
		return (char*)(data+1);
	}

// parse a regular word
	do
	{
		if (len >= tokenlen-1)
			break;
		token[len] = c;
		data++;
		len++;
		c = *data;
		if (strchr(punctuation, c))
			break;
	} while (c>32);

	token[len] = 0;
	if (tokentype)
		*tokentype = TTP_RAWTOKEN;
	return (char*)data;
}

//escape a string so that COM_Parse will give the same string.
//maximum expansion is strlen(string)*2+4 (includes null terminator)
const char *COM_QuotedString(const char *string, char *buf, int buflen, qboolean omitquotes)
{
#ifdef HAVE_LEGACY
	extern cvar_t dpcompat_console;
#else
	static const cvar_t dpcompat_console = {0};
#endif
	const char *result = buf;
	if (strchr(string, '\r') || strchr(string, '\n') || (!dpcompat_console.ival && strchr(string, '\"')))
	{	//strings of the form \"foo" can contain c-style escapes, including for newlines etc.
		//it might be fancy to ALWAYS escape non-ascii chars too, but mneh
		if (!omitquotes)
		{
			*buf++ = '\\';	//prefix so the reader knows its a quoted string.
			*buf++ = '\"';	//opening quote
			buflen -= 4;
		}
		else
			buflen -= 1;
		while(*string && buflen >= 2)
		{
			switch(*string)
			{
			case '\n':
				*buf++ = '\\';
				*buf++ = 'n';
				break;
			case '\r':
				*buf++ = '\\';
				*buf++ = 'r';
				break;
			case '\t':
				*buf++ = '\\';
				*buf++ = 't';
				break;
			case '\'':
				*buf++ = '\\';
				*buf++ = '\'';
				break;
			case '\"':
				*buf++ = '\\';
				*buf++ = '\"';
				break;
			case '\\':
				*buf++ = '\\';
				*buf++ = '\\';
				break;
			case '$':
				*buf++ = '\\';
				*buf++ = '$';
				break;
			default:
				*buf++ = *string++;
				buflen--;
				continue;
			}
			buflen -= 2;
			string++;
		}
		if (!omitquotes)
			*buf++ = '\"';	//closing quote
		*buf++ = 0;
		return result;
	}
	else
	{
		if (!omitquotes)
		{
			*buf++ = '\"';	//opening quote
			buflen -= 3;
		}
		else
			buflen -= 1;
		if (dpcompat_console.ival)
		{	//dp escapes \\ and \", but nothing else.
			//so no new-lines etc
			while(*string && buflen >= 2)
			{
				if (*string == '\\' || *string == '\"')
				{
					*buf++ = '\\';
					buflen--;
				}
				*buf++ = *string++;
				buflen--;
			}
		}
		else
		{	//vanilla quake's console doesn't support any escapes.
			while(*string && buflen >= 1)
			{
				*buf++ = *string++;
				buflen--;
			}
		}
		if (!omitquotes)
			*buf++ = '\"';	//closing quote
		*buf++ = 0;
		return result;
	}
}

char *COM_ParseCString (const char *data, char *token, size_t sizeoftoken, size_t *lengthwritten)
{
	int		c;
	size_t		len;

	len = 0;
	token[0] = 0;

	if (token == com_token)
		COM_AssertMainThread("COM_ParseCString: com_token");

	if (lengthwritten)
		*lengthwritten = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= sizeoftoken-2)
			{
				token[len] = '\0';
				if (lengthwritten)
					*lengthwritten = len;
				return (char*)data;
			}

			c = *data++;
			if (!c)
			{
				token[len] = 0;
				if (lengthwritten)
					*lengthwritten = len;
				return (char*)data-1;
			}
			if (c == '\\')
			{
				c = *data++;
				switch(c)
				{
				case '\r':
					if (*data == '\n')
						data++;
				case '\n':
					continue;
				case 'n':
					c = '\n';
					break;
				case 't':
					c = '\t';
					break;
				case 'r':
					c = '\r';
					break;
				case 'x':
					c = 0;
					if (ishexcode(*data))
						c |= dehex(*data++);
					if (ishexcode(*data))
					{
						c <<= 4;
						c |= dehex(*data++);
					}
					break;
				case '$':
				case '\\':
				case '\'':
					break;
				case '"':
					c = '"';
					token[len] = c;
					len++;
					continue;
				default:
					c = '?';
					break;
				}
			}
			if (c=='\"' || !c)
			{
				token[len] = 0;
				if (lengthwritten)
					*lengthwritten = len;
				return (char*)data;
			}
			token[len] = c;
			len++;
		}
	}

// parse a regular word
	do
	{
		if (len >= sizeoftoken-1)
			break;
		token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);

	token[len] = 0;
	if (lengthwritten)
		*lengthwritten = len;
	return (char*)data;
}


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/

int COM_CheckNextParm (const char *parm, int last)
{
	int i = last+1;

	for ( ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP sometimes clears appkit vars.
		if (!Q_strcmp (parm,com_argv[i]))
			return i;
	}

	return 0;
}

int COM_CheckParm (const char *parm)
{
	return COM_CheckNextParm(parm, 0);
}

/*
===============
COM_ParsePlusSets

Looks for +set blah blah on the commandline, and creates cvars so that engine
functions may use the cvar before anything's loaded.
This isn't really needed, but might make some thing nicer.
===============
*/
void COM_ParsePlusSets (qboolean docbuf)
{
	int i;
	int c;
	for (i=1 ; i<com_argc-2 ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP sometimes clears appkit vars.
		for (c = 1; i+c < com_argc && com_argv[i+c] && *com_argv[i+c] != '-' && *com_argv[i+c] != '+'; c++)
			;

		if (docbuf)
		{
			if (c == 3 && (!strcmp(com_argv[i], "+set") || !strcmp(com_argv[i], "+seta") ||
				!strcmp(com_argv[i], "-set") || !strcmp(com_argv[i], "-seta")))
			{
				char buf[8192];
				Cbuf_AddText(com_argv[i]+1, RESTRICT_LOCAL);
				Cbuf_AddText(" ", RESTRICT_LOCAL);
				Cbuf_AddText(COM_QuotedString(com_argv[i+1], buf, sizeof(buf), false), RESTRICT_LOCAL);
				Cbuf_AddText(" ", RESTRICT_LOCAL);
				Cbuf_AddText(COM_QuotedString(com_argv[i+2], buf, sizeof(buf), false), RESTRICT_LOCAL);
				Cbuf_AddText("\n", RESTRICT_LOCAL);
			}
			else if (c == 2 && !strcmp(com_argv[i], "-exec"))
			{
				char buf[8192];
				Cbuf_AddText(com_argv[i]+1, RESTRICT_LOCAL);
				Cbuf_AddText(" ", RESTRICT_LOCAL);
				Cbuf_AddText(COM_QuotedString(com_argv[i+1], buf, sizeof(buf), false), RESTRICT_LOCAL);
				Cbuf_AddText("\n", RESTRICT_LOCAL);
			}
		}
		else
		{
			if (c == 3 && (!strcmp(com_argv[i], "+set") || !strcmp(com_argv[i], "+seta")))
			{
#if defined(Q2CLIENT) || defined(Q2SERVER)
				if (!strcmp("basedir", com_argv[i+1]))
					host_parms.basedir = com_argv[i+2];
				else
#endif
					Cvar_Get(com_argv[i+1], com_argv[i+2], (!strcmp(com_argv[i], "+seta"))?CVAR_ARCHIVE:0, "Cvars set on commandline");
			}
		}
		i += c-1;
	}
}

void Cvar_DefaultFree(char *str);
/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered (void)
{
	char *newdef;
	vfsfile_t	*h;

	h = FS_OpenVFS("gfx/pop.lmp", "rb", FS_GAME);

	if (h)
	{
		static_registered = true;
		VFS_CLOSE(h);
	}
	else
		static_registered = false;


	newdef = static_registered?"1":"0";

	if (strcmp(registered.enginevalue, newdef))
	{
		if (registered.defaultstr != registered.enginevalue)
		{
			Cvar_DefaultFree(registered.defaultstr);
			registered.defaultstr = NULL;
		}
		registered.enginevalue = newdef;
		registered.defaultstr = newdef;
		Cvar_ForceSet(&registered, newdef);
		if (static_registered)
			Con_TPrintf ("Playing registered version.\n");
	}
}



/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, const char **argv)	//not allowed to tprint
{
	qboolean	safe;
	int			i;

#if !defined(FTE_TARGET_WEB)
	FILE *f;

	if (argv && argv[0])
		f = fopen(va("%s_p.txt", argv[0]), "rb");
	else
		f = NULL;
	if (f)
	{
		size_t result;
		char *buffer;
		int len;
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);

		buffer = (char*)malloc(len+1);
		result = fread(buffer, 1, len, f); // do something with result

		if (result != len)
			Con_Printf("COM_InitArgv() fread: Filename: %s, expected %i, result was %u (%s)\n",va("%s_p.txt", argv[0]),len,(unsigned int)result,strerror(errno));

		buffer[len] = '\0';

		while (*buffer && (argc < MAX_NUM_ARGVS))
		{
			while (*buffer && ((*buffer <= 32) || (*buffer > 126)))
				buffer++;

			if (*buffer)
			{
				argv[argc] = buffer;
				argc++;

				while (*buffer && ((*buffer > 32) && (*buffer <= 126)))
					buffer++;

				if (*buffer)
				{
					*buffer = 0;
					buffer++;
				}

			}
		}


		fclose(f);
	}
#endif

	safe = false;

	for (com_argc=0 ; (com_argc<MAX_NUM_ARGVS) && (com_argc < argc) ;
		 com_argc++)
	{
		largv[com_argc] = argv[com_argc];
		if (!Q_strcmp ("-safe", argv[com_argc]))
			safe = true;
	}

	if (safe)
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (i=0 ; i<countof(safeargvs) ; i++)
		{
			largv[com_argc] = safeargvs[i];
			com_argc++;
		}
	}

	largv[com_argc] = argvdummy;
	com_argv = largv;
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/
void COM_AddParm (const char *parm)
{
	largv[com_argc++] = parm;
}

/*
=======================
COM_Version_f
======================
*/
static void COM_Version_f (void)
{
	Con_Printf("\n");
	Con_Printf("^&F0%s\n", FULLENGINENAME);
	Con_Printf("^4"ENGINEWEBSITE"\n");
	Con_Printf("%s\n", version_string());

#ifdef FTE_BRANCH
	Con_Printf("^3Branch:^7 "STRINGIFY(FTE_BRANCH)"\n");
	Con_Printf("^3Revision:^7 %s - %s\n",STRINGIFY(SVNREVISION), STRINGIFY(SVNDATE));
#elif defined(SVNREVISION) && defined(SVNDATE)
	if (!strncmp(STRINGIFY(SVNREVISION), "git-", 4))
		Con_Printf("^3GIT Revision:^7 %s - %s\n",STRINGIFY(SVNREVISION), STRINGIFY(SVNDATE));
	else
		Con_Printf("^3SVN Revision:^7 %s - %s\n",STRINGIFY(SVNREVISION), STRINGIFY(SVNDATE));
#else
	Con_TPrintf ("^3Exe:^7 %s %s\n", __DATE__, __TIME__);
#ifdef SVNREVISION
	if (!strncmp(STRINGIFY(SVNREVISION), "git-", 4))
		Con_Printf("^3GIT Revision:^7 %s\n",STRINGIFY(SVNREVISION));
	else if (strcmp(STRINGIFY(SVNREVISION), "-"))
		Con_Printf("^3SVN Revision:^7 %s\n",STRINGIFY(SVNREVISION));
#endif
#endif
#ifdef CONFIG_FILE_NAME
	Con_Printf("^3Build config:^7 %s\n\n", COM_SkipPath(STRINGIFY(CONFIG_FILE_NAME)));
#endif

	Con_Printf("^3Build type:^7");
#ifdef MINIMAL
	Con_Printf("minimal\n");
#endif
#ifdef CLIENTONLY
	Con_Printf(" client-only\n");
#endif
#ifdef SERVERONLY
	Con_Printf(" dedicated\n");
#endif
#ifdef _DEBUG
	Con_Printf(" debug");
#else
	Con_Printf(" release");
#endif
	Con_Printf("\n");

#if defined(FTE_SDL3)
	{
		int ver = SDL_GetVersion();
		Con_Printf("^3SDL version:^9 %d.%d.%d -> ^7%d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION, SDL_VERSIONNUM_MAJOR(ver), SDL_VERSIONNUM_MINOR(ver), SDL_VERSIONNUM_MICRO(ver));
	}
#elif defined(FTE_SDL)
	Con_Printf("^3SDL version:^7 %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
#endif

// Don't print both as a 64bit MinGW built client
#if defined(__MINGW32__)
	Con_Printf("Compiled with MinGW32/64 version: %i.%i\n",__MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION);
#endif

#ifdef __CYGWIN__
	Con_Printf("Compiled with Cygwin\n");
#endif

#ifdef FTE_TARGET_WEB
	Con_Printf("Compiled with emscripten %i.%i.%i\n", __EMSCRIPTEN_major__, __EMSCRIPTEN_minor__, __EMSCRIPTEN_tiny__);
#endif

#ifdef __clang__
	Con_Printf("^3Compiler:^7 clang %i.%i.%i (%s)\n",__clang_major__, __clang_minor__, __clang_patchlevel__, __VERSION__);
#elif defined(__GNUC__)
	Con_Printf("^3Compiler:^7 GCC %i.%i.%i (%s)\n",__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__, __VERSION__);

	#ifdef __OPTIMIZE__
		#ifdef __OPTIMIZE_SIZE__
			Con_Printf("Optimized for size\n");
		#else
			Con_Printf("Optimized for speed\n");
		#endif
	#endif

	#ifdef __NO_INLINE__
		Con_Printf("^3GCC Optimization:^7 Functions currently not inlined into their callers\n");
	#else
		Con_Printf("^3GCC Optimization:^7 Functions currently inlined into their callers\n");
	#endif
#elif defined(_MSC_VER)
	if (_MSC_VER == 600) {		Con_Printf(	"^3Compiler:^7 C Compiler version 6.0\n"); }
	else if (_MSC_VER == 700) { Con_Printf(	"^3Compiler:^7 C/C++ compiler version 7.0\n"); }
	else if (_MSC_VER == 800) { Con_Printf(	"^3Compiler:^7 Visual C++, Windows, version 1.0 or Visual C++, 32-bit, version 1.0\n"); }
	else if (_MSC_VER == 900) { Con_Printf(	"^3Compiler:^7 Visual C++, Windows, version 2.0 or Visual C++, 32-bit, version 2.x\n"); }
	else if (_MSC_VER == 1000) { Con_Printf("^3Compiler:^7 Visual C++, 32-bit, version 4.0\n"); }
	else if (_MSC_VER == 1020) { Con_Printf("^3Compiler:^7 Visual C++, 32-bit, version 4.2\n"); }
	else if (_MSC_VER == 1100) { Con_Printf("^3Compiler:^7 Visual C++, 32-bit, version 5.0\n"); }
	else if (_MSC_VER == 1200) { Con_Printf("^3Compiler:^7 Visual C++, 32-bit, version 6.0\n"); }
	else if (_MSC_VER == 1300) { Con_Printf("^3Compiler:^7 Visual C++, version 7.0\n"); }
	else if (_MSC_VER == 1310) { Con_Printf("^3Compiler:^7 Visual C++ 2003, version 7.1\n"); }
	else if (_MSC_VER == 1400) { Con_Printf("^3Compiler:^7 Visual C++ 2005, version 8.0\n"); }
	else if (_MSC_VER == 1500) { Con_Printf("^3Compiler:^7 Visual C++ 2008, version 9.0\n"); }
	else if (_MSC_VER == 1600) { Con_Printf("^3Compiler:^7 Visual C++ 2010, version 10.0\n"); }
	else if (_MSC_VER == 1700) { Con_Printf("^3Compiler:^7 Visual C++ 2012, version 11.0\n"); }
	else if (_MSC_VER == 1800) { Con_Printf("^3Compiler:^7 Visual C++ 2013, version 12.0\n"); }
	else if (_MSC_VER == 1900) { Con_Printf("^3Compiler:^7 Visual C++ 2015, version 14.0\n"); }
	else if (_MSC_VER >= 1910 && _MSC_VER < 1920) { Con_Printf("^3Compiler:^7 Visual C++ 2017, version 14.1x\n"); }
	else if (_MSC_VER >= 1920 && _MSC_VER < 1930) { Con_Printf("^3Compiler:^7 Visual C++ 2019, version 14.2x\n"); }
	else
	{
#ifdef _MSC_BUILD
		Con_Printf("^3Compiler:^7 Unknown Microsoft C++ compiler: %i %i %i\n",_MSC_VER, _MSC_FULL_VER, _MSC_BUILD);
#else
		Con_Printf("^3Compiler:^7 Unknown Microsoft C++ compiler: %i %i\n",_MSC_VER, _MSC_FULL_VER);
#endif
	}
#endif

	Con_Printf("^3CPU Arch:^7 " PLATFORM " " ARCH_CPU_POSTFIX
#ifdef ARCH_ALTCPU_POSTFIX
		"/"ARCH_ALTCPU_POSTFIX
#endif
	);
#ifdef __AVX512F__
	Con_Printf(" AVX512");
#elif defined(__AVX2__)
	Con_Printf(" AVX2");
#elif defined (__AVX__)
	Con_Printf(" AVX");
#elif defined (__SSE4_2__)
	Con_Printf(" SSE4.2");
#elif defined (__SSE4_1__)
	Con_Printf(" SSE4.1");
#elif defined (__SSE3__)
	Con_Printf(" SSE3");
#elif defined(_M_IX86_FP) && _M_IX86_FP == 2 	//32bit only - always enabled for amd64
	Con_Printf(" SSE2");
#elif defined(_M_IX86_FP) && _M_IX86_FP == 1 	//32bit only - always enabled for amd64
	Con_Printf(" SSE");
#elif defined(_M_IX86_FP) && _M_IX86_FP == 0 	//32bit only - always enabled for amd64
	Con_Printf(" x87");
#endif
	Con_Printf("\n");

#ifdef _M_IX86
	Con_Printf("^3x86 optimized for:^7 ");

	if (_M_IX86 == 600) { Con_Printf("Blend or Pentium Pro, Pentium II and Pentium III"); }
	else if (_M_IX86 == 500) { Con_Printf("Pentium"); }
	else if (_M_IX86 == 400) { Con_Printf("486"); }
	else if (_M_IX86 == 300) { Con_Printf("386"); }
	else
	{
		Con_Printf("Unknown (%i)\n",_M_IX86);
	}

	Con_Printf("\n");
#endif

#ifdef HAVE_CLIENT
	Con_Printf("^3Renderers:^7");
#ifdef GLQUAKE
#ifdef GLESONLY
	#ifdef FTE_TARGET_WEB	//shuld we be just asking the video code for a list?...
		Con_Printf(" WebGL");
	#else
		Con_Printf(" OpenGLES");
	#endif
#else
	Con_Printf(" OpenGL");
#endif
#ifdef GLSLONLY
	Con_Printf("(GLSL)");
#endif
#endif
#ifdef VKQUAKE
	Con_Printf(" Vulkan");
#endif
#ifdef D3D9QUAKE
	Con_Printf(" Direct3D9");
#endif
#ifdef D3D11QUAKE
	Con_Printf(" Direct3D11");
#endif
#ifdef SWQUAKE
	Con_Printf(" Software");
#endif
	Con_Printf("\n");
#endif

#ifdef MULTITHREAD
#ifdef LOADERTHREAD
	Con_Printf("^3multithreading:^7 enabled (loader enabled)\n");
#else
	Con_Printf("^3multithreading:^7 enabled (no loader)\n");
#endif
#else
	Con_Printf("^3multithreading^7: disabled\n");
#endif

	//print out which libraries are disabled
	Con_Printf("^3Compression:^7");
#ifdef AVAIL_ZLIB
	Con_Printf(" zlib^h("
#ifdef ZLIB_STATIC
			"static, "
#endif
			"%s)^h", ZLIB_VERSION);
#endif
#ifdef AVAIL_BZLIB
	Con_Printf(" bzlib"
		#ifdef BZLIB_STATIC
			"^h(static)^h"
		#endif
		);
#endif
	Con_Printf("\n");

#ifdef HAVE_CLIENT
	Image_PrintInputFormatVersions();

	Con_Printf("^3VoiceChat:^7");
	#if !defined(VOICECHAT)
		Con_Printf(" disabled");
	#else
		#ifdef HAVE_SPEEX
			#ifdef SPEEX_STATIC
				Con_Printf(" speex");
				Con_DPrintf("^h(static)");
			#else
				Con_Printf(" speex^h(dynamic)");
			#endif
		#endif
		#ifdef HAVE_OPUS
			#ifdef OPUS_STATIC
				Con_Printf(" opus");
				Con_DPrintf("^h(static)");
			#else
				Con_Printf(" opus^h(dynamic)");
			#endif
		#endif
	#endif
	Con_Printf("\n");

	Con_Printf("^3Audio Decoders:^7");
	#ifdef FTE_TARGET_WEB
		Con_Printf(" Browser");
	#endif
	#ifndef AVAIL_OGGVORBIS
		Con_DPrintf(" ^h(disabled: Ogg Vorbis)^7");
	#elif defined(LIBVORBISFILE_STATIC)
		Con_Printf(" Ogg-Vorbis");
		Con_DPrintf("^h(static)");
	#else
		Con_Printf(" Ogg-Vorbis^h(dynamic)");
	#endif
	#if defined(AVAIL_MP3_ACM)
		Con_Printf(" mp3(system)");
	#endif
	Con_Printf("\n");
#endif

#ifdef SQL
	Con_Printf("^3Databases:^7");
	#ifdef USE_MYSQL
		Con_Printf(" mySQL^h(dynamic)");
	#else
		Con_DPrintf(" ^h(disabled: mySQL)^7");
	#endif
	#ifdef USE_SQLITE
		Con_Printf(" sqlite^h(dynamic)");
	#else
		Con_DPrintf(" ^h(disabled: sqlite)^7");
	#endif
	Con_Printf("\n");
#endif

	Con_Printf("^3Misc:^7");
#ifdef SUBSERVERS
	Con_Printf(" mapcluster");
#else
	Con_DPrintf(" ^h(disabled: mapcluster)^7");
#endif
#ifdef HAVE_SERVER
#ifdef AVAIL_FREETYPE
	#ifdef FREETYPE_STATIC
		Con_Printf(" freetype2");
		Con_DPrintf("^h(static)");
	#else
		Con_Printf(" freetype2^h(dynamic)");
	#endif
#else
	Con_DPrintf(" ^h(disabled: freetype2)^7");
#endif
#ifdef AVAIL_OPENAL
	#ifdef FTE_TARGET_WEB
		Con_Printf(" WebAudio");
		Con_DPrintf("^h(static)");
	#else
		Con_Printf(" OpenAL^h(dynamic)");
	#endif
#else
	Con_DPrintf(" ^h(disabled: openal)^7");
#endif
#endif
#ifdef USE_INTERNAL_BULLET
	Con_Printf(" bullet");
#endif
#ifdef ENGINE_ROUTING
	Con_Printf(" routing");
#endif
#ifdef QCJIT
	Con_Printf(" qcjit");
#endif
	Con_Printf("\n");

#ifdef _WIN32
	#ifndef AVAIL_DINPUT
		Con_DPrintf("DirectInput disabled\n");
	#endif
	#ifndef AVAIL_DSOUND
		Con_DPrintf("DirectSound disabled\n");
	#endif
#endif

#if defined(HAVE_SERVER) || defined(HAVE_CLIENT)
	Con_Printf("^3Games:^7");
#if defined(Q3SERVER) && defined(Q3CLIENT)
	#ifdef BOTLIB_STATIC
		Con_Printf(" Quake3");
	#else
		Con_Printf(" Quake3^h(dynamic)^h");
	#endif
#elif defined(Q3SERVER)
	#ifdef BOTLIB_STATIC
		Con_Printf(" Quake3(server)");
	#else
		Con_Printf(" Quake3(server,dynamic)");
	#endif
#elif defined(Q3CLIENT)
	Con_Printf(" Quake3(client)");
#elif defined(Q3BSPS)
	Con_DPrintf(" ^hQuake3(bsp only)^7");
#else
	Con_DPrintf(" ^h(disabled: Quake3)^7");
#endif
#if defined(Q2SERVER) && defined(Q2CLIENT)
	Con_Printf(" Quake2");
#elif defined(Q2SERVER)
	Con_Printf(" Quake2(server)");
#elif defined(Q2CLIENT)
	Con_Printf(" Quake2(client)");
#elif defined(Q2BSPS)
	Con_DPrintf(" ^hQuake2(bsp only)^7");
#else
	Con_DPrintf(" ^h(disabled: Quake2)^7");
#endif
#if defined(HEXEN2)
	Con_Printf(" Hexen2");
#else
	Con_DPrintf(" ^h(disabled: Hexen2)^7");
#endif
#if defined(NQPROT)
	Con_Printf(" NetQuake");
#else
	Con_DPrintf(" ^h(disabled: NetQuake)");
#endif
#if defined(VM_Q1)
	Con_Printf(" ssq1qvm");
#endif
#if defined(VM_LUA)
	Con_Printf(" ssq1lua^h(dynamic)");
#endif
#if defined(MENU_DAT)
	Con_Printf(" menuqc");
#endif
#if defined(MENU_NATIVECODE)
	Con_Printf(" nmenu");
#endif
#if defined(CSQC_DAT)
	Con_Printf(" csqc");
#endif
#ifdef HAVE_SERVER
	Con_Printf(" ssqc");
#endif
	Con_Printf("\n");
#endif

	Con_Printf("^3Networking:^7");
#ifdef WEBCLIENT
	Con_Printf(" HTTPClient");
#endif
#ifdef HAVE_HTTPSV
	Con_Printf(" HTTPServer");
#endif
#ifdef FTPSERVER
	Con_Printf(" FTPServer");
#endif
#if (defined(SUPPORT_ICE)&&defined(HAVE_DTLS)) || defined(FTE_TARGET_WEB)
	Con_Printf(" WebRTC");
#elif defined(SUPPORT_ICE)
	Con_Printf(" ICE");
#endif
#ifdef FTE_TARGET_WEB
	Con_Printf(" WebSocket/WSS");
#else
	#if defined(HAVE_TCP)
		#ifdef TCPCONNECT
			Con_Printf(" TCPConnect");
		#endif
	#else
		Con_Printf(" ^h(disabled: TCP)");
	#endif
#endif
#ifdef HAVE_GNUTLS             //on linux
	Con_Printf(" GnuTLS");
#endif
#ifdef HAVE_WINSSPI            //on windows
	Con_Printf(" WINSSPI");
#endif
	Con_Printf("\n");
}

#ifdef _DEBUG
static void COM_LoopMe_f(void)
{
	while(1)
		;
}
static void COM_CrashMe_f(void)
{
	int *crashaddr = (int*)0x05;

	*crashaddr = 0;
}

static void COM_ErrorMe_f(void)
{
	Sys_Error("\"errorme\" command used");
}
#endif



#ifdef LOADERTHREAD
static void QDECL COM_WorkerCount_Change(cvar_t *var, char *oldvalue);
cvar_t worker_flush = CVARD("worker_flush", "1", "If set, process the entire load queue, loading stuff faster but at the risk of stalling the main thread.");
static cvar_t worker_count = CVARFCD("worker_count", "", CVAR_NOTFROMSERVER, COM_WorkerCount_Change, "Specifies the number of worker threads to utilise.");
static cvar_t worker_sleeptime = CVARFD("worker_sleeptime", "0", CVAR_NOTFROMSERVER, "Causes workers to sleep for a period of time after each job.");

#define WORKERTHREADS 16	//max
/*multithreading worker thread stuff*/
void *com_resourcemutex;
static int com_liveworkers[WG_COUNT];
static void *com_workercondition[WG_COUNT];
int com_hadwork[WG_COUNT];
static volatile int com_workeracksequence;
static struct com_worker_s
{
	void *thread;
	volatile enum {
		WR_NONE,
		WR_DIE,
		WR_ACK	//updates ackseq to com_workeracksequence and sends a signal to WG_MAIN
	} request;
	volatile int ackseq;
} com_worker[WORKERTHREADS];
qboolean com_workererror;
static struct com_work_s
{
	struct com_work_s *next;
	void(*func)(void *ctx, void *data, size_t a, size_t b);
	void *ctx;
	void *data;
	size_t a;
	size_t b;
} *com_work_head[WG_COUNT], *com_work_tail[WG_COUNT];
unsigned int COM_HasWorkers(wgroup_t tg)
{	//simply returns if adding work will block or not (and a hint for how many jobs should be queued at once).
	return com_liveworkers[tg];
}
//return if there's *any* loading that needs to be done anywhere.
qboolean COM_HasWork(void)
{
	unsigned int i;
	for (i = 0; i < WG_COUNT; i++)
	{
		if (com_work_head[i])
			return true;
	}
	return false;
}
void COM_InsertWork(wgroup_t tg, void(*func)(void *ctx, void *data, size_t a, size_t b), void *ctx, void *data, size_t a, size_t b)
{
	struct com_work_s *work;

	if (tg >= WG_COUNT)
		return;

	//no worker there, just do it immediately on this thread instead of pushing it to the worker.
	if (!com_liveworkers[tg] || (tg!=WG_MAIN && com_workererror))
	{
		func(ctx, data, a, b);
		return;
	}

	//build the work
	work = Z_Malloc(sizeof(*work));
	work->func = func;
	work->ctx = ctx;
	work->data = data;
	work->a = a;
	work->b = b;

	//queue it (fifo)
	Sys_LockConditional(com_workercondition[tg]);
	work->next = com_work_head[tg];
	if (!com_work_tail[tg])
		com_work_tail[tg] = work;
	com_work_head[tg] = work;

//	Sys_Printf("%x: Queued work %p (%s)\n", thread, work->ctx, work->ctx?(char*)work->ctx:"?");

	Sys_ConditionSignal(com_workercondition[tg]);
	Sys_UnlockConditional(com_workercondition[tg]);
}
void COM_AddWork(wgroup_t tg, void(*func)(void *ctx, void *data, size_t a, size_t b), void *ctx, void *data, size_t a, size_t b)
{
	struct com_work_s *work;

	if (tg >= WG_COUNT)
		return;

	//no worker there, just do it immediately on this thread instead of pushing it to the worker.
	if (!com_liveworkers[tg] || (tg!=WG_MAIN && com_workererror))
	{
		func(ctx, data, a, b);
		return;
	}

	//build the work
	work = Z_Malloc(sizeof(*work));
	work->func = func;
	work->ctx = ctx;
	work->data = data;
	work->a = a;
	work->b = b;

	//queue it (fifo)
	Sys_LockConditional(com_workercondition[tg]);
	if (com_work_tail[tg])
	{
		com_work_tail[tg]->next = work;
		com_work_tail[tg] = work;
	}
	else
		com_work_head[tg] = com_work_tail[tg] = work;

//	Sys_Printf("%x: Queued work %p (%s)\n", thread, work->ctx, work->ctx?(char*)work->ctx:"?");

	Sys_ConditionSignal(com_workercondition[tg]);
	Sys_UnlockConditional(com_workercondition[tg]);
}

/*static void COM_PrintWork(void)
{
	struct com_work_s *work;
	int tg;
	Sys_Printf("--------- BEGIN WORKER LIST ---------\n");
	for (tg = 0; tg < WG_COUNT; tg++)
	{
		Sys_LockConditional(com_workercondition[tg]);
		work = com_work_head[tg];
		while (work)
		{
			Sys_Printf("group%i: %s\n", tg, (char*)work->ctx);
			work = work->next;
		}
		Sys_UnlockConditional(com_workercondition[tg]);
	}
}*/

//leavelocked = false == poll mode.
//leavelocked = true == safe sleeping
qboolean COM_DoWork(int tg, qboolean leavelocked)
{
	struct com_work_s *work;
	if (tg >= WG_COUNT)
		return false;
	if (!leavelocked)
	{
		//skip the locks if it looks like we can be lazy.
		if (!com_work_head[tg])
			return false;
		Sys_LockConditional(com_workercondition[tg]);
	}
	work = com_work_head[tg];
	if (work)
		com_work_head[tg] = work->next;
	if (!com_work_head[tg])
		com_work_head[tg] = com_work_tail[tg] = NULL;

	if (work)
	{
		com_hadwork[tg]++;
//		Sys_Printf("%x: Doing work %p (%s)\n", thread, work->ctx, work->ctx?(char*)work->ctx:"?");
		Sys_UnlockConditional(com_workercondition[tg]);

		work->func(work->ctx, work->data, work->a, work->b);
		Z_Free(work);

		if (leavelocked)
			Sys_LockConditional(com_workercondition[tg]);

		return true;	//did something, check again
	}

	if (!leavelocked)
		Sys_UnlockConditional(com_workercondition[tg]);

	//nothing going on, if leavelocked then noone can add anything until we sleep.
	return false;
}
/*static void COM_WorkerSync_ThreadAck(void *ctx, void *data, size_t a, size_t b)
{
	int us;
	int *ackbuf = ctx;

	Sys_LockConditional(com_workercondition[WG_MAIN]);
	//find out which worker we are, and flag ourselves as having acked the main thread to clean us up
	for (us = 0; us < WORKERTHREADS; us++)
	{
		if (com_worker[us].thread && Sys_IsThread(com_worker[us].thread))
		{
			ackbuf[us] = true;
			break;
		}
	}
	*(int*)data += 1;
	//and tell the main thread it can stop being idle now
	Sys_ConditionSignal(com_workercondition[WG_MAIN]);
	Sys_UnlockConditional(com_workercondition[WG_MAIN]);
}
*/
/*static void COM_WorkerSync_SignalMain(void *ctx, void *data, size_t a, size_t b)
{
	Sys_LockConditional(com_workercondition[a]);
	com_workerdone[a] = true;
	Sys_ConditionSignal(com_workercondition[a]);
	Sys_UnlockConditional(com_workercondition[a]);
}*/
static void COM_WorkerSync_WorkerStopped(void *ctx, void *data, size_t a, size_t b)
{
	struct com_worker_s *thread = ctx;
	if (thread->thread)
	{
		//the worker signaled us then stopped looping
		Sys_WaitOnThread(thread->thread);
		thread->thread = NULL;

		Sys_LockConditional(com_workercondition[b]);
		com_liveworkers[b] -= 1;
		Sys_UnlockConditional(com_workercondition[b]);
	}
	else
		Con_Printf("worker thread died twice?\n");

	//if that was the last thread, make sure any work pending for that group is completed.
	if (!com_liveworkers[b])
	{
		while(COM_DoWork(b, false))
			;
	}
}
static int COM_WorkerThread(void *arg)
{
	struct com_worker_s *thread = arg;
	int group = WG_LOADER;
	Sys_LockConditional(com_workercondition[group]);
	com_liveworkers[group]++;
	for(;;)
	{
		while(COM_DoWork(group, true))
		{
			if (thread->request == WR_DIE)
				break;
			if (worker_sleeptime.value)
			{
				Sys_UnlockConditional(com_workercondition[group]);
				Sys_Sleep(worker_sleeptime.value);
				Sys_LockConditional(com_workercondition[group]);
			}
		}
		if (thread->request)	//flagged from some work
		{
			if (thread->request == WR_DIE)
				break;
			if (thread->request == WR_ACK)
			{
				thread->request = WR_NONE;
				thread->ackseq = com_workeracksequence;
				Sys_UnlockConditional(com_workercondition[group]);
				Sys_LockConditional(com_workercondition[WG_MAIN]);
				Sys_ConditionBroadcast(com_workercondition[WG_MAIN]); //try to wake up whoever wanted us to ack them
				Sys_UnlockConditional(com_workercondition[WG_MAIN]);
				Sys_LockConditional(com_workercondition[group]);
				continue;
			}
		}
		else if (!Sys_ConditionWait(com_workercondition[group]))
			break;
	}
	Sys_UnlockConditional(com_workercondition[group]);

	//and wake up main thread to clean up our handle
	COM_AddWork(WG_MAIN, COM_WorkerSync_WorkerStopped, thread, NULL, 0, group);
	return 0;
}
static void Sys_ErrorThread(void *ctx, void *data, size_t a, size_t b)
{
	if (ctx)
		COM_WorkerSync_WorkerStopped(ctx, NULL, a, b);

	//posted to main thread from a worker.
	Sys_Error("%s", (const char*)data);
}
void COM_WorkerAbort(char *message)
{
	int group = -1;
	int us;
	if (Sys_IsMainThread())
		return;
	com_workererror = true;

	if (!com_workercondition[WG_MAIN])
		return;	//Sys_IsMainThread was probably called too early...

	//find out which worker we are, and tell the main thread to clean us up
	for (us = 0; us < WORKERTHREADS; us++)
		if (com_worker[us].thread && Sys_IsThread(com_worker[us].thread))
		{
			group = WG_LOADER;
			COM_InsertWork(WG_MAIN, Sys_ErrorThread, &com_worker[us], Z_StrDup(message), 0, group);
			break;
		}

	if (us == WORKERTHREADS)	//don't know who it was.
		COM_AddWork(WG_MAIN, Sys_ErrorThread, NULL, Z_StrDup(message), 0, 0);

	Sys_ThreadAbort();
}

#ifndef COM_AssertMainThread
void COM_AssertMainThread(const char *msg)
{
	if (com_resourcemutex && !Sys_IsMainThread())
	{
		Sys_Error("Not on main thread: %s", msg);
	}
}
#endif
void COM_DestroyWorkerThread(void)
{
	int i;
	if (!com_resourcemutex)
		return;
//	com_workererror = false;
	Sys_LockConditional(com_workercondition[WG_LOADER]);
	for (i = 0; i < WORKERTHREADS; i++)
		com_worker[i].request = WR_DIE;	//flag them all to die
	Sys_ConditionBroadcast(com_workercondition[WG_LOADER]);	//and make sure they ALL wake up
	Sys_UnlockConditional(com_workercondition[WG_LOADER]);

	while(COM_DoWork(WG_LOADER, false))	//finish any work that got posted to it that it neglected to finish.
		;
	COM_WorkerFullSync();
	while(COM_DoWork(WG_MAIN, false))
		;

	for (i = 0; i < WG_COUNT; i++)
	{
		if (com_workercondition[i])
			Sys_DestroyConditional(com_workercondition[i]);
		com_workercondition[i] = NULL;
	}

	Sys_DestroyMutex(com_resourcemutex);
	com_resourcemutex = NULL;
}

//Dangerous: stops workers WITHOUT flushing their queue. Be SURE to 'unlock' to start them up again.
void COM_WorkerLock(void)
{
#define NOFLUSH 0x40000000
	int i;
	if (!com_liveworkers[WG_LOADER])
		return;	//nothing to do.

	//don't let liveworkers become 0 (so the main thread doesn't flush any pending work) and ask workers to die
	Sys_LockConditional(com_workercondition[WG_LOADER]);
	com_liveworkers[WG_LOADER] |= NOFLUSH;
	for (i = 0; i < WORKERTHREADS; i++)
		com_worker[i].request = WR_DIE;	//flag them all to die
	Sys_ConditionBroadcast(com_workercondition[WG_LOADER]);	//and make sure they ALL wake up to check their new death values.
	Sys_UnlockConditional(com_workercondition[WG_LOADER]);

	//wait for the workers to stop (leaving their work, because of our fake worker)
	while((com_liveworkers[WG_LOADER]&~NOFLUSH)>0)
	{
		if (!COM_DoWork(WG_MAIN, false))	//need to check this to know they're done.
			COM_DoWork(WG_LOADER, false);	//might as well, while we're waiting.
	}

	//remove our flush-blocker now...
	Sys_LockConditional(com_workercondition[WG_LOADER]);
	com_liveworkers[WG_LOADER] &= ~NOFLUSH;
	Sys_UnlockConditional(com_workercondition[WG_LOADER]);
}
//called after COM_WorkerLock
void COM_WorkerUnlock(void)
{
	qboolean restarted = false;
	int i;
	for (i = 0; i < WORKERTHREADS; i++)
	{
		if (i >= worker_count.ival)
			continue;	//worker stays dead

		//lower thread indexes need to be (re)created
		if (!com_worker[i].thread)
		{
			com_worker[i].request = WR_NONE;
			com_worker[i].thread = Sys_CreateThread(va("loadworker_%i", i), COM_WorkerThread, &com_worker[i], 0, 256*1024);
			if (com_worker[i].thread)
				restarted = true;
		}
	}

	if (!restarted)
		while (COM_DoWork(WG_LOADER, false))
			;
}

//fully flushes ALL pending work.
void COM_WorkerFullSync(void)
{
	qboolean repeat;
	int i;

	while(COM_DoWork(WG_MAIN, false))
		;

	if (!com_liveworkers[WG_LOADER])
		return;

	com_workeracksequence++;

	Sys_LockConditional(com_workercondition[WG_MAIN]);
	do
	{
		if (!COM_HasWork())
		{
			Sys_UnlockConditional(com_workercondition[WG_MAIN]);
			Sys_LockConditional(com_workercondition[WG_LOADER]);
			repeat = false;
			for (i = 0; i < WORKERTHREADS; i++)
			{
				if (com_worker[i].ackseq != com_workeracksequence && com_worker[i].request == WR_NONE)
				{
					com_worker[i].request = WR_ACK;
					repeat = true;
				}
			}
			if (repeat)	//we're unable to signal a specific thread due to only having one condition. oh well. WAKE UP GUYS!
				Sys_ConditionBroadcast(com_workercondition[WG_LOADER]);
			Sys_UnlockConditional(com_workercondition[WG_LOADER]);
			Sys_LockConditional(com_workercondition[WG_MAIN]);
		}

		repeat = COM_DoWork(WG_MAIN, true);

		if (repeat)
		{	//if we just did something, we may have posted something new to a worker... bum.
			com_workeracksequence++;
		}
		else
		{
			for (i = 0; i < WORKERTHREADS; i++)
			{
				if (com_worker[i].thread && com_worker[i].ackseq != com_workeracksequence)
					repeat = true;
			}
			if (repeat)
				Sys_ConditionWait(com_workercondition[WG_MAIN]);
		}
		if (com_workererror)
			break;
	} while(repeat);
	Sys_UnlockConditional(com_workercondition[WG_MAIN]);
}

//main thread wants a specific object to be prioritised.
//an ancestor of the work must be pending on either the main thread or the worker thread.
//typically the worker gives us a signal to handle the final activation of the object.
//the address should be the load status. the value is the current value.
//the work that we're waiting for will be considered complete when the address is no longer set to value.
void COM_WorkerPartialSync(void *priorityctx, int *address, int value)
{
	struct com_work_s **link, *work, *prev;
//	double time1 = Sys_DoubleTime();

//	Con_Printf("waiting for %p %s\n", priorityctx, priorityctx);

	COM_DoWork(WG_MAIN, false);

	//boost the priority of the object that we're waiting for on the other thread, if we can find it.
	//this avoids waiting for everything.
	//if we can't find it, then its probably currently being processed anyway.
	//main thread is meant to do all loadstate value changes anyway, ensuring that we're woken up properly in this case.
	if (priorityctx)
	{
		unsigned int grp;
		qboolean found = false;
		for (grp = WG_LOADER; grp < WG_MAIN && !found; grp++)
		{
			Sys_LockConditional(com_workercondition[grp]);
			for (link = &com_work_head[grp], work = NULL; *link; link = &(*link)->next)
			{
				prev = work;
				work = *link;
				if (work->ctx == priorityctx)
				{	//unlink it

					*link = work->next;
					if (!work->next)
						com_work_tail[grp] = prev;
					//link it in at the head, so its the next thing seen.
					work->next = com_work_head[grp];
					com_work_head[grp] = work;
					if (!work->next)
						com_work_tail[grp] = work;
					found = true;

					break;	//found it, nothing else to do.
				}
			}
			//we've not actually added any work, so no need to signal
			Sys_UnlockConditional(com_workercondition[grp]);
		}
		if (!found)
		{
			while(COM_DoWork(WG_MAIN, false))
			{
				//give up as soon as we're done
				if (*address != value)
					return;
			}
//			Con_Printf("Might be in for a long wait for %s\n", (char*)priorityctx);
		}
	}

	Sys_LockConditional(com_workercondition[WG_MAIN]);
	do
	{
		if (com_workererror)
			break;
		while(COM_DoWork(WG_MAIN, true))
		{
			//give up as soon as we're done
			if (*address != value)
				break;
		}
		//if our object's state has changed, we're done
		if (*address != value)
			break;
	} while (Sys_ConditionWait(com_workercondition[WG_MAIN]));
	Sys_UnlockConditional(com_workercondition[WG_MAIN]);

//	Con_Printf("Waited %f for %s\n", Sys_DoubleTime() - time1, priorityctx);
}

static void COM_WorkerPong(void *ctx, void *data, size_t a, size_t b)
{
	double *timestamp = data;
	Con_Printf("Ping: %g\n", Sys_DoubleTime() - *timestamp);
	Z_Free(timestamp);
}
static void COM_WorkerPing(void *ctx, void *data, size_t a, size_t b)
{
	COM_AddWork(WG_MAIN, COM_WorkerPong, ctx, data, 0, 0);
}
static void COM_WorkerTest_f(void)
{
	double *timestamp = Z_Malloc(sizeof(*timestamp));
	*timestamp = Sys_DoubleTime();
	COM_AddWork(WG_LOADER, COM_WorkerPing, NULL, timestamp, 0, 0);
}
static void COM_WorkerStatus_f(void)
{
	struct com_work_s *work;
	int i, count;
	for (i = 0, count = 0; i < WORKERTHREADS; i++)
	{
		if (com_worker[i].thread)
			count++;
	}
	Con_Printf("%i workers live\n", count);

	Sys_LockConditional(com_workercondition[WG_LOADER]);
	for (count = 0, work = com_work_head[WG_LOADER]; work; work = work->next)
		count++;
	Sys_UnlockConditional(com_workercondition[WG_LOADER]);
	Con_Printf("%i pending tasks\n", count);
}

static void QDECL COM_WorkerCount_Change(cvar_t *var, char *oldvalue)
{
	int i, count = var->ival;

	if (!*var->string)
	{
		count = var->ival = 4;
	}

	//try to respond to any kill requests now, so we don't get surprised by the cvar changing too often.
	while(COM_DoWork(WG_MAIN, false))
		;

	for (i = 0; i < WORKERTHREADS; i++)
	{
		if (i >= count)
		{
			//higher thread indexes need to die.
			com_worker[i].request = WR_DIE;	//flag them all to die
		}
		else
		{
			//lower thread indexes need to be created
			if (!com_worker[i].thread)
			{
				com_worker[i].request = WR_NONE;
				com_worker[i].thread = Sys_CreateThread(va("loadworker_%i", i), COM_WorkerThread, &com_worker[i], 0, 256*1024);
			}
		}
	}
	Sys_ConditionBroadcast(com_workercondition[WG_LOADER]);	//and make sure they ALL wake up to check their new death values.
}
static void COM_InitWorkerThread(void)
{
	int i;

	//in theory, we could run multiple workers, signalling a different one in turn for each bit of work.
	com_resourcemutex = Sys_CreateMutex();
	for (i = 0; i < WG_COUNT; i++)
	{
		com_workercondition[i] = Sys_CreateConditional();
	}
	com_liveworkers[WG_MAIN] = 1;

	//technically its ready now...

	if (COM_CheckParm("-noworker") || COM_CheckParm("-noworkers"))
	{
		worker_count.enginevalue = "0";
		worker_count.flags |= CVAR_NOSET;
	}
	Cvar_Register(&worker_count, NULL);

	Cmd_AddCommand ("worker_test", COM_WorkerTest_f);
	Cmd_AddCommand ("worker_status", COM_WorkerStatus_f);
	Cvar_Register(&worker_flush, NULL);
	Cvar_Register(&worker_sleeptime, NULL);
	Cvar_ForceCallback(&worker_count);
}

qboolean FTE_AtomicPtr_ConditionalReplace(qint32_t *ptr, qint32_t old, qint32_t new)
{
	Sys_LockMutex(com_resourcemutex);
	if (*ptr == old)
	{
		*ptr = new;
		Sys_UnlockMutex(com_resourcemutex);
		return true;
	}
	Sys_UnlockMutex(com_resourcemutex);
	return false;
}

qint32_t FTE_Atomic32Mutex_Add(qint32_t *ptr, qint32_t change)
{
	qint32_t r;
	Sys_LockMutex(com_resourcemutex);
	r = (*ptr += change);
	Sys_UnlockMutex(com_resourcemutex);
	return r;
}
#else
qint32_t FTE_Atomic32Mutex_Add(qint32_t *ptr, qint32_t change)
{
	qint32_t r;
	r = (*ptr += change);
	return r;
}
qboolean FTE_AtomicPtr_ConditionalReplace(qint32_t *ptr, qint32_t old, qint32_t new)
{	//hope it ain't threaded
	if (*ptr == old)
	{
		*ptr = new;
		return true;
	}
	return false;
}
#endif

/*
================
COM_Init
================
*/
void COM_Init (void)
{
#if !defined(FTE_BIG_ENDIAN) && !defined(FTE_LITTLE_ENDIAN)
// set the qbyte swapping variables in a portable manner
	qbyte	swaptest[2] = {1,0};
	if ( *(short *)swaptest == 1)
	{
		bigendian = false;
		BigShort = ShortSwap;
		LittleShort = ShortNoSwap;
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		BigI64 = I64Swap;
		LittleI64 = I64NoSwap;
		BigFloat = FloatSwap;
		LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendian = true;
		BigShort = ShortNoSwap;
		LittleShort = ShortSwap;
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		BigI64 = I64NoSwap;
		LittleI64 = I64Swap;
		BigFloat = FloatNoSwap;
		LittleFloat = FloatSwap;
	}
#endif

	wantquit = false;

	//random should be random from the start...
	srand(time(0));

#ifdef LOADERTHREAD
	COM_InitWorkerThread();
#endif

#ifdef PACKAGEMANAGER
	Cmd_AddCommandD("pkg", PM_Command_f,		"Provides a way to install / list / disable / purge packages via the console.");
#endif
	Cmd_AddCommandD("version", COM_Version_f,	"Reports engine revision and optional compile-time settings.");	//prints the pak or whatever where this file can be found.

#ifdef _DEBUG
	Cmd_AddCommand ("loopme", COM_LoopMe_f);
	Cmd_AddCommand ("crashme", COM_CrashMe_f);
	Cmd_AddCommand ("errorme", COM_ErrorMe_f);
#endif
	COM_InitFilesystem ();

	Cvar_Register (&host_mapname, "Scripting");
	Cvar_Register (&developer, "Debugging");
	Cvar_Register (&sys_platform, "Gamecode");
	Cvar_Register (&pr_engine, "Gamecode");
	Cvar_Register (&registered, "Copy protection");
	Cvar_Register (&gameversion, "Gamecode");
	Cvar_Register (&gameversion_min, "Gamecode");
	Cvar_Register (&gameversion_max, "Gamecode");
	Cvar_Register (&com_gamedirnativecode, "Gamecode");
	Cvar_Register (&com_parseutf8, "Internationalisation");
#ifdef HAVE_LEGACY
	Cvar_Register (&scr_usekfont, NULL);
	Cvar_Register (&ezcompat_markup, NULL);
	Cvar_Register (&pm_noround, NULL);
#endif
	Cvar_Register (&com_highlightcolor, "Internationalisation");
	com_parseutf8.ival = 1;

	TranslateInit();

	COM_BiDi_Setup();


	nullentitystate.hexen2flags = SCALE_ORIGIN_ORIGIN;
	nullentitystate.colormod[0] = 32;
	nullentitystate.colormod[1] = 32;
	nullentitystate.colormod[2] = 32;
	nullentitystate.glowmod[0] = 32;
	nullentitystate.glowmod[1] = 32;
	nullentitystate.glowmod[2] = 32;
	nullentitystate.trans = 255;
	nullentitystate.scale = 16;
	nullentitystate.solidsize = 0;//ES_SOLID_BSP;
}

void COM_Shutdown (void)
{
#ifdef LOADERTHREAD
	COM_DestroyWorkerThread();
#endif
	COM_BiDi_Shutdown();
	FS_Shutdown();
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char	*VARGS va(const char *format, ...)
{
#define VA_BUFFERS 2 //power of two
#define VA_BUFFER_SIZE 8192
	va_list		argptr;
	static char		string[VA_BUFFERS][VA_BUFFER_SIZE];
	static int bufnum;

	COM_AssertMainThread("va");

	bufnum++;
	bufnum &= (VA_BUFFERS-1);

	va_start (argptr, format);
	vsnprintf (string[bufnum],sizeof(string[bufnum])-1, format,argptr);
	va_end (argptr);

	return string[bufnum];
}

#if defined(HAVE_CLIENT) || defined(HAVE_SERVER)
#ifdef NQPROT
//for compat with dpp7 protocols, or dp gamecode that neglects to properly precache particles.
void COM_Effectinfo_Enumerate(int (*cb)(const char *pname))
{
	int i;
	char *f, *buf;
	static const char *dpnames[] =
	{
		"TE_GUNSHOT",
		"TE_GUNSHOTQUAD",
		"TE_SPIKE",
		"TE_SPIKEQUAD",
		"TE_SUPERSPIKE",
		"TE_SUPERSPIKEQUAD",
		"TE_WIZSPIKE",
		"TE_KNIGHTSPIKE",
		"TE_EXPLOSION",
		"TE_EXPLOSIONQUAD",
		"TE_TAREXPLOSION",
		"TE_TELEPORT",
		"TE_LAVASPLASH",
		"TE_SMALLFLASH",
		"TE_FLAMEJET",
		"EF_FLAME",
		"TE_BLOOD",
		"TE_SPARK",
		"TE_PLASMABURN",
		"TE_TEI_G3",
		"TE_TEI_SMOKE",
		"TE_TEI_BIGEXPLOSION",
		"TE_TEI_PLASMAHIT",
		"EF_STARDUST",
		"TR_ROCKET",
		"TR_GRENADE",
		"TR_BLOOD",
		"TR_WIZSPIKE",
		"TR_SLIGHTBLOOD",
		"TR_KNIGHTSPIKE",
		"TR_VORESPIKE",
		"TR_NEHAHRASMOKE",
		"TR_NEXUIZPLASMA",
		"TR_GLOWTRAIL",
		"SVC_PARTICLE",
		NULL
	};

	FS_LoadFile("effectinfo.txt", (void **)&f);
	if (!f)
		return;

	for (i = 0; dpnames[i]; i++)
		cb(dpnames[i]);

	buf = f;
	while (f && *f)
	{
		f = COM_ParseToken(f, NULL);
		if (strcmp(com_token, "\n"))
		{
			if (!strcmp(com_token, "effect"))
			{
				f = COM_ParseToken(f, NULL);
				cb(com_token);
			}

			do
			{
				f = COM_ParseToken(f, NULL);
			} while(f && *f && strcmp(com_token, "\n"));
		}
	}
	FS_FreeFile(buf);
}
#endif

/*************************************************************************/

/*remaps map checksums from known non-cheat GPL maps to authentic id1 maps.*/
unsigned int COM_RemapMapChecksum(model_t *model, unsigned int checksum)
{
#ifdef HAVE_LEGACY
	static const struct {
		const char *name;
		unsigned int gpl2;
//		unsigned int id11;
		unsigned int id12;
	} sums[] =
	{
		{"maps/start.bsp",	0xDC03BAF3,	/*0x2A9A3763,*/	0x1D69847B},

		{"maps/e1m1.bsp",	0xB7B19924,	/*0x1F392B02,*/	0xAD07D882},
		{"maps/e1m2.bsp",	0x80CD279B,	/*0x5D140D24,*/	0x67100127},
		{"maps/e1m3.bsp",	0x1F632D93,	/*0x3C20FA2E,*/	0x3546324A},
		{"maps/e1m4.bsp",	0xB75BC1B8,	/*0xE5A522CE,*/	0xEDDA0675},
		{"maps/e1m5.bsp",	0x65DEA50B,	/*0x6EA3A1CB,*/	0xA82C1C8A},
		{"maps/e1m6.bsp",	0x3C76263E,	/*0x4DC4FFC4,*/	0x2C0028E3},
		{"maps/e1m7.bsp",	0x51FAD6A8,	/*0xACBF5564,*/	0x97D6FB1A},
		{"maps/e1m8.bsp",	0x57A436A8,	/*0xF63C8EE5,*/	0x04B6E741},

		{"maps/e2m1.bsp",	0x992B120D,	/*0xD0732BA6,*/	0xDCF57032},
		{"maps/e2m2.bsp",	0xA23126C5,	/*0xEACA9423,*/	0xAF961D4D},
		{"maps/e2m3.bsp",	0x0956602E,	/*0x47B46758,*/	0xFC992551},
		{"maps/e2m4.bsp",	0xA4CDDCC6,	/*0x9EDD4CE8,*/	0xC3169BC9},
		{"maps/e3m5.bsp",	0xDC98420F,	/*0xAC371E07,*/	0x917A0631},
		{"maps/e2m6.bsp",	0x3E1AA34D,	/*0x22CD3B7B,*/	0x91A33B81},
		{"maps/e2m7.bsp",	0xA1A37724,	/*0x6C1F85F2,*/	0x7A3FE018},

		{"maps/e3m1.bsp",	0xBD5A7A83,	/*0xE4BE9A0B,*/	0x90B20D21},
		{"maps/e3m2.bsp",	0xE4043D8E,	/*0x2B1EC056,*/	0x9C6C7538},
		{"maps/e3m3.bsp",	0xEE12BAC9,	/*0xDFCFCB78,*/	0xC3D05D18},
		{"maps/e3m4.bsp",	0xF33D954A,	/*0x42003651,*/	0xB1790CB8},
		{"maps/e3m5.bsp",	0xDC98420F,	/*0xAC371E07,*/	0x917A0631},
		{"maps/e3m6.bsp",	0x9CC8F9BC,	/*0x6139434A,*/	0x2DC17DF8},
		{"maps/e3m7.bsp",	0x2E8DE70A,	/*0xA5CF7110,*/	0x1039C1B1},

		{"maps/e4m1.bsp",	0x5C4CDD45,	/*0x4AC23D4C,*/	0xBBF06350},
		{"maps/e4m2.bsp",	0xAC84C40A,	/*0x057FACCC,*/	0xFFF8CB18},
		{"maps/e4m3.bsp",	0xB6A519E2,	/*0x74E93DDD,*/	0x59BEF08C},
		{"maps/e4m4.bsp",	0x3233C45C,	/*0xE9A7693C,*/	0x2D3B183F},
		{"maps/e4m5.bsp",	0xE5D3E4DD,	/*0x17315A00,*/	0x699CE7F4},
		{"maps/e4m6.bsp",	0x5A7B37C0,	/*0x6636A6B8,*/	0x0620FF98},
		{"maps/e4m7.bsp",	0xE9497085,	/*0xDD1C14E2,*/	0x9DEC01AC},
		{"maps/e4m8.bsp",	0x325A2B54,	/*0x3F6274D5,*/	0x3CB46C57},

		{"maps/dm1.bsp",	0x7D37618E,	/*0xA3B80B3A,*/	0xC5C7DAB3},	//you should be able to use aquashark's untextured maps.
		{"maps/dm2.bsp",	0x7B337440,	/*0x1763B3DA,*/	0x65F63634},
		{"maps/dm3.bsp",	0x912781AE,	/*0x7AC99CDE,*/	0x15E20DF8},
		{"maps/dm4.bsp",	0xC374DF89,	/*0x13799D1F,*/	0x9C6FE4BF},
		{"maps/dm5.bsp",	0x77CA7CE5,	/*0x2DB66BBC,*/	0xB02D48FD},
		{"maps/dm6.bsp",	0x200C8B5D,	/*0x0EBB386D,*/	0x5208DA2B},

		{"maps/end.bsp",	0xF89B12AE,	/*0xA66198D8,*/	0xBBD4B4A5},	//unmodified gpl version (with the extra room)
		{"maps/end.bsp",	0x924F4D33,	/*0xA66198D8,*/	0xBBD4B4A5}, 	//aquashark's gpl version (with the extra room removed)

		//re-release maps. they are not 100% identical,
		//but they're generally close enough and its confusing to get kicked for having the official maps.
		//expect minor prediction issues in a few places.
		{"maps/start.bsp",	0x49A92170,	/*0x2A9A3763,*/	0x1D69847B},
		{"maps/e1m1.bsp",	0xA1937AD5,	/*0x1F392B02,*/	0xAD07D882},
		{"maps/e1m2.bsp",	0x65BC436B,	/*0x5D140D24,*/	0x67100127},
		{"maps/e1m3.bsp",	0x7A4FE4F2,	/*0x3C20FA2E,*/	0x3546324A},
		{"maps/e1m4.bsp",	0xEC07DCB0,	/*0xE5A522CE,*/	0xEDDA0675},
//buggy	{"maps/e1m5.bsp",	0xAD138551,	/*0x6EA3A1CB,*/	0xA82C1C8A},
		{"maps/e1m6.bsp",	0xA732C2E4,	/*0x4DC4FFC4,*/	0x2C0028E3},
		{"maps/e1m7.bsp",	0x9318DDF3,	/*0xACBF5564,*/	0x97D6FB1A},
		{"maps/e1m8.bsp",	0x0E858BF7,	/*0xF63C8EE5,*/	0x04B6E741},
		{"maps/e2m1.bsp",	0xCB350590,	/*0xD0732BA6,*/	0xDCF57032},
		{"maps/e2m2.bsp",	0x045DC982,	/*0xEACA9423,*/	0xAF961D4D},
		{"maps/e2m3.bsp",	0x4E14A67D,	/*0x47B46758,*/	0xFC992551},
		{"maps/e2m4.bsp",	0x5366D18C,	/*0x9EDD4CE8,*/	0xC3169BC9},
		{"maps/e3m5.bsp",	0x94086C83,	/*0xAC371E07,*/	0x917A0631},
//start	{"maps/e2m6.bsp",	0x460E3FE2,	/*0x22CD3B7B,*/	0x91A33B81},
		{"maps/e2m7.bsp",	0xB7477F61,	/*0x6C1F85F2,*/	0x7A3FE018},
		{"maps/e3m1.bsp",	0xBC433495,	/*0xE4BE9A0B,*/	0x90B20D21},
		{"maps/e3m2.bsp",	0x63E72C4D,	/*0x2B1EC056,*/	0x9C6C7538},
		{"maps/e3m3.bsp",	0x8DD3DF69,	/*0xDFCFCB78,*/	0xC3D05D18},
		{"maps/e3m4.bsp",	0xD41DD779,	/*0x42003651,*/	0xB1790CB8},
		{"maps/e3m5.bsp",	0x1EAA53D8,	/*0xAC371E07,*/	0x917A0631},
		{"maps/e3m6.bsp",	0xEFB7B728,	/*0x6139434A,*/	0x2DC17DF8},
		{"maps/e3m7.bsp",	0x7A46C0EA,	/*0xA5CF7110,*/	0x1039C1B1},
		{"maps/e4m1.bsp",	0x9AF0885B,	/*0x4AC23D4C,*/	0xBBF06350},
		{"maps/e4m2.bsp",	0x8E947D06,	/*0x057FACCC,*/	0xFFF8CB18},
		{"maps/e4m3.bsp",	0x134BCDEE,	/*0x74E93DDD,*/	0x59BEF08C},
		{"maps/e4m4.bsp",	0xBDB41FF0,	/*0xE9A7693C,*/	0x2D3B183F},
		{"maps/e4m5.bsp",	0xC1F0D4C6,	/*0x17315A00,*/	0x699CE7F4},
		{"maps/e4m6.bsp",	0x286A9410,	/*0x6636A6B8,*/	0x0620FF98},
		{"maps/e4m7.bsp",	0xB769356B,	/*0xDD1C14E2,*/	0x9DEC01AC},
//		{"maps/e4m8.bsp",	0xA62A7AEB,	/*0x3F6274D5,*/	0x3CB46C57},
//		{"maps/dm1.bsp",	0x6E4C13E6,	/*0xA3B80B3A,*/	0xC5C7DAB3},
		{"maps/dm2.bsp",	0x725B277D,	/*0x1763B3DA,*/	0x65F63634},
		{"maps/dm3.bsp",	0xB1DD97B1,	/*0x7AC99CDE,*/	0x15E20DF8},
		{"maps/dm4.bsp",	0x76A592A0,	/*0x13799D1F,*/	0x9C6FE4BF},
		{"maps/dm5.bsp",	0xD651996F,	/*0x2DB66BBC,*/	0xB02D48FD},
		{"maps/dm6.bsp",	0x33F7D9C9,	/*0x0EBB386D,*/	0x5208DA2B},
//		{"maps/end.bsp",	0x3C87824B,	/*0xA66198D8,*/	0xBBD4B4A5},

		//Quake2 Rerelease.
		//These are listed to remap the rerelease's crc32 checksums from the vanilla maps to the rerelease, so q2e can connect to regular servers.
		//the reverse is not a concern as there's too many translation errors that way anyway.
		//yes this is kinda backwards vs the q1 stuff above.

		//MAP NAME				VAN+fmd4	VAN+crc32	REMST+fmd4 REMST+crc32
//base12/pak0
//		{"maps/base1.bsp",		/*0x0,*/ 0xc49cac93, /*0x0,*/ 0x8939212b},	//expanded area and some wallhacky bits and areaportal screwups
		//{"maps/base2.bsp",		/*0x0,*/ 0x49ed539e, /*0x0,*/ 0xda61be7b},
		//{"maps/base3.bsp",		/*0x0,*/ 0xf29055e4, /*0x0,*/ 0x13ac20a1},
		//{"maps/biggun.bsp",		/*0x0,*/ 0x0b95a4ae, /*0x0,*/ 0x7cae003f},
		//{"maps/boss1.bsp",		/*0x0,*/ 0x7a52514a, /*0x0,*/ 0xeeff1917},
		//{"maps/boss2.bsp",		/*0x0,*/ 0x81ac4371, /*0x0,*/ 0x7b6392d6},
		//{"maps/bunk1.bsp",		/*0x0,*/ 0xa37f1a6d, /*0x0,*/ 0xe529ac52},
		//{"maps/city1.bsp",		/*0x0,*/ 0x4fd6239b, /*0x0,*/ 0x8876eb6a},
		//{"maps/city2.bsp",		/*0x0,*/ 0x6f7768d8, /*0x0,*/ 0xd0797f11},
		//{"maps/city3.bsp",		/*0x0,*/ 0x3809bd4e, /*0x0,*/ 0xb7135bdd},
		//{"maps/command.bsp",	/*0x0,*/ 0x80d7b67c, /*0x0,*/ 0x66e404f5},
		//{"maps/cool1.bsp",		/*0x0,*/ 0xeabfd863, /*0x0,*/ 0xbbe51fc9},
		//{"maps/fact1.bsp",		/*0x0,*/ 0xfba460e5, /*0x0,*/ 0xf9bf8d9f},
		//{"maps/fact2.bsp",		/*0x0,*/ 0x6655c1a5, /*0x0,*/ 0xb2cc8d37},
		//{"maps/fact3.bsp",		/*0x0,*/ 0xabe9c595, /*0x0,*/ 0xb64f87d4},
		//{"maps/hangar1.bsp",	/*0x0,*/ 0x5893e66d, /*0x0,*/ 0x1334d867},
		//{"maps/hangar2.bsp",	/*0x0,*/ 0x933c0a95, /*0x0,*/ 0x077e9ba3},
		//{"maps/jail1.bsp",		/*0x0,*/ 0xee5e64ec, /*0x0,*/ 0x158a1107},
		//{"maps/jail2.bsp",		/*0x0,*/ 0x154ae1bf, /*0x0,*/ 0x2921f388},
		//{"maps/jail3.bsp",		/*0x0,*/ 0x8f3de333, /*0x0,*/ 0x32554a02},
		//{"maps/jail4.bsp",		/*0x0,*/ 0xe66ec160, /*0x0,*/ 0xb1013e3b},
		//{"maps/jail5.bsp",		/*0x0,*/ 0x3af55df6, /*0x0,*/ 0xbd077c5b},
		//{"maps/lab.bsp",		/*0x0,*/ 0xf2bc9c9a, /*0x0,*/ 0x4eea0f97},
		//{"maps/mine1.bsp",		/*0x0,*/ 0x31db729e, /*0x0,*/ 0x5fc5f3cf},
		//{"maps/mine2.bsp",		/*0x0,*/ 0xf8c3c330, /*0x0,*/ 0x35eacacb},
		//{"maps/mine3.bsp",		/*0x0,*/ 0x1ba1ba64, /*0x0,*/ 0x6c8e3f66},
		//{"maps/mine4.bsp",		/*0x0,*/ 0x6d4d9c98, /*0x0,*/ 0x48a30d5d},
		//{"maps/mintro.bsp",		/*0x0,*/ 0x15bcf39c, /*0x0,*/ 0xfb72d23f},
		//{"maps/power1.bsp",		/*0x0,*/ 0x6e9407f7, /*0x0,*/ 0x8d15b695},
		//{"maps/power2.bsp",		/*0x0,*/ 0xd64d63b5, /*0x0,*/ 0xcf9fd9f5},
		//{"maps/security.bsp",	/*0x0,*/ 0xc75d06ff, /*0x0,*/ 0xcb7a0229},
		//{"maps/space.bsp",		/*0x0,*/ 0xe9b1ca59, /*0x0,*/ 0x34678329},
		//{"maps/strike.bsp",		/*0x0,*/ 0x373acf3b, /*0x0,*/ 0x1e6aa8da},
		//{"maps/train.bsp",		/*0x0,*/ 0x339a6bf3, /*0x0,*/ 0x1c6b7d5c},
		//{"maps/ware1.bsp",		/*0x0,*/ 0x039d00bc, /*0x0,*/ 0x34713cb1},
		//{"maps/ware2.bsp",		/*0x0,*/ 0xe25b761e, /*0x0,*/ 0x00c06d11},
		//{"maps/waste1.bsp",		/*0x0,*/ 0x14f41c2e, /*0x0,*/ 0x33dd40ff},
		//{"maps/waste2.bsp",		/*0x0,*/ 0x91267752, /*0x0,*/ 0x5b53de97},
		//{"maps/waste3.bsp",		/*0x0,*/ 0x7658206d, /*0x0,*/ 0x80101af0},

		//baseq2/pak1.pak
		{"maps/q2dm1.bsp",		/*0x0,*/ 0x6cc8eda7, /*0x0,*/ 0x23393278},
		{"maps/q2dm2.bsp",		/*0x0,*/ 0x60a8b392, /*0x0,*/ 0x2f03a689},
//		{"maps/q2dm3.bsp",		/*0x0,*/ 0xdc3aac9b, /*0x0,*/ 0x378903ee},	//bmodel snafus
		{"maps/q2dm4.bsp",		/*0x0,*/ 0x87db6388, /*0x0,*/ 0xa6b504da},
		{"maps/q2dm5.bsp",		/*0x0,*/ 0x7da73bb5, /*0x0,*/ 0xc639cbb7},
		{"maps/q2dm6.bsp",		/*0x0,*/ 0xc63f6546, /*0x0,*/ 0xbe784b8a},
		{"maps/q2dm7.bsp",		/*0x0,*/ 0x39f11899, /*0x0,*/ 0xea6ac852},
//		{"maps/q2dm8.bsp",		/*0x0,*/ 0x8659ee44, /*0x0,*/ 0x09a423b2},	//crates are missing

//		//one-offs
//		{"maps/base64.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x7db2883f},
//		{"maps/city64.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x635eb291},
//		{"maps/sewer64.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x46da9e37},
		//mg
//		{"maps/mgdm1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x826e737a},
//		{"maps/mgu1m1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xc27e2cef},
//		{"maps/mgu1m2.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x1ec516fa},
//		{"maps/mgu1m3.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xb9f13047},
//		{"maps/mgu1m4.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x8211337e},
//		{"maps/mgu1m5.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x1549c505},
//		{"maps/mgu1trial.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x0f75a608},
//		{"maps/mgu2m1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x4a769bb0},
//		{"maps/mgu2m2.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xd307cad8},
//		{"maps/mgu2m3.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xd9352a68},
//		{"maps/mgu3m1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x5ab2ea83},
//		{"maps/mgu3m2.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x4952301e},
//		{"maps/mgu3m3.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x7ed253fa},
//		{"maps/mgu3m4.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xc91edb2e},
//		{"maps/mgu3secret.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0xdb50aa0d},
//		{"maps/mgu4m1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x5558556a},
//		{"maps/mgu4m2.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x32090016},
//		{"maps/mgu4m3.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xa60d1e9a},
//		{"maps/mgu4trial.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x5cb612d1},
//		{"maps/mgu5m1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x60d40817},
//		{"maps/mgu5m2.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x88083b2f},
//		{"maps/mgu5m3.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x4dc5140a},
//		{"maps/mgu5trial.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x4808e583},
//		{"maps/mgu6m1.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xbcc16455},
//		{"maps/mgu6m2.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x6313d469},
//		{"maps/mgu6m3.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x2651b5f7},
//		{"maps/mgu6trial.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0xfabf1b9f},
//		{"maps/mguboss.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x12733e70},
//		{"maps/mguhub.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0x51fbdd73},
//
//		//new
//		{"maps/ndctf0.bsp",		/*0x0,*/ 0x0, /*0x0,*/ 0xa8c81b2b},
//		{"maps/q2kctf1.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x8d176d56},
//		{"maps/q2kctf2.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x01a32428},
//		{"maps/tutorial.bsp",	/*0x0,*/ 0x0, /*0x0,*/ 0x8869d954},

		//q2ctf maps
		//{"maps/q2ctf1.bsp",		/*0x0,*/ 0xe067a203, /*0x0,*/ 0x6143b5e7},
		//{"maps/q2ctf2.bsp",		/*0x0,*/ 0x28030fe8, /*0x0,*/ 0x5c71d212},
		//{"maps/q2ctf3.bsp",		/*0x0,*/ 0xca29c6a3, /*0x0,*/ 0x59bb4823},
		//{"maps/q2ctf4.bsp",		/*0x0,*/ 0x2fa28832, /*0x0,*/ 0x63fef5de},
		//{"maps/q2ctf5.bsp",		/*0x0,*/ 0x033ccd79, /*0x0,*/ 0x98116fdd},
//		{"maps/q2ctf6.bsp",		/*0x0,*/ 0xe0f199b9, /*0x0,*/ 0x0},	//no idea why these are missing in the rerelease..
//		{"maps/q2ctf7.bsp",		/*0x0,*/ 0x390abd05, /*0x0,*/ 0x0},
//		{"maps/q2ctf8.bsp",		/*0x0,*/ 0x3ba7c491, /*0x0,*/ 0x0},

		//xatrix maps
		//{"maps/badlands.bsp",	/*0x0,*/ 0xafa0e22e, /*0x0,*/ 0x203ffacb},
		//{"maps/industry.bsp",	/*0x0,*/ 0x876bd244, /*0x0,*/ 0x3cf0c2bd},
		//{"maps/outbase.bsp",	/*0x0,*/ 0xe7531d00, /*0x0,*/ 0x1c36a9e0},
		//{"maps/refinery.bsp",	/*0x0,*/ 0xe4e81e7b, /*0x0,*/ 0xaf61d172},
		//{"maps/w_treat.bsp",	/*0x0,*/ 0xde9eb38c, /*0x0,*/ 0x4a3685fe},
		//{"maps/xcompnd1.bsp",	/*0x0,*/ 0xb3540a91, /*0x0,*/ 0x24bc9f6f},
		//{"maps/xcompnd2.bsp",	/*0x0,*/ 0xe4745192, /*0x0,*/ 0x55df1931},
		//{"maps/xdm1.bsp",		/*0x0,*/ 0x3d48b316, /*0x0,*/ 0x26a71790},
		//{"maps/xdm2.bsp",		/*0x0,*/ 0x1152e7c4, /*0x0,*/ 0x11db3085},
		//{"maps/xdm3.bsp",		/*0x0,*/ 0xdb97aeac, /*0x0,*/ 0xb60a8631},
		//{"maps/xdm4.bsp",		/*0x0,*/ 0xeef68945, /*0x0,*/ 0x6e070a92},
		//{"maps/xdm5.bsp",		/*0x0,*/ 0x47c1f3a3, /*0x0,*/ 0x558d4dd6},
		//{"maps/xdm6.bsp",		/*0x0,*/ 0x852def91, /*0x0,*/ 0x3cb70b50},
		//{"maps/xdm7.bsp",		/*0x0,*/ 0xc4d93896, /*0x0,*/ 0xc0de3235},
		//{"maps/xhangar1.bsp",	/*0x0,*/ 0x9e18dceb, /*0x0,*/ 0x10de03f6},
		//{"maps/xhangar2.bsp",	/*0x0,*/ 0x781c8f17, /*0x0,*/ 0xd9f92aa3},
		//{"maps/xintell.bsp",	/*0x0,*/ 0x5cf61c9b, /*0x0,*/ 0x2c979acf},
		//{"maps/xmoon1.bsp",		/*0x0,*/ 0xc2ec98f6, /*0x0,*/ 0x57f1373c},
		//{"maps/xmoon2.bsp",		/*0x0,*/ 0x79cdedf8, /*0x0,*/ 0xc2236a38},
		//{"maps/xreactor.bsp",	/*0x0,*/ 0xa32f926d, /*0x0,*/ 0x5dc0db58},
		//{"maps/xsewer1.bsp",	/*0x0,*/ 0x53f393b3, /*0x0,*/ 0x68c01403},
		//{"maps/xsewer2.bsp",	/*0x0,*/ 0xdae8d63f, /*0x0,*/ 0xc78c8261},
		//{"maps/xship.bsp",		/*0x0,*/ 0xcaae64c3, /*0x0,*/ 0x16434a51},
		//{"maps/xswamp.bsp",		/*0x0,*/ 0x7a01ffe3, /*0x0,*/ 0x92852525},
		//{"maps/xware.bsp",		/*0x0,*/ 0x769d834c, /*0x0,*/ 0xa21dad68},

		//(q2) rogue maps
		//{"maps/rammo1.bsp",		/*0x0,*/ 0x157c38b5, /*0x0,*/ 0x70778442},
		//{"maps/rammo2.bsp",		/*0x0,*/ 0xe59eb4f3, /*0x0,*/ 0xb9a6bb64},
		//{"maps/rbase1.bsp",		/*0x0,*/ 0xfe907818, /*0x0,*/ 0xcc316310},
		//{"maps/rbase2.bsp",		/*0x0,*/ 0x6a9f5a26, /*0x0,*/ 0xd148fd82},
		//{"maps/rboss.bsp",		/*0x0,*/ 0x5c3167e7, /*0x0,*/ 0x0ba861c9},
		//{"maps/rdm1.bsp",		/*0x0,*/ 0x9d581b00, /*0x0,*/ 0xcc0dc613},
		//{"maps/rdm2.bsp",		/*0x0,*/ 0xcaef085e, /*0x0,*/ 0xcb93d84b},
		//{"maps/rdm3.bsp",		/*0x0,*/ 0xecd65aea, /*0x0,*/ 0x2c334d25},
		//{"maps/rdm4.bsp",		/*0x0,*/ 0xb0e01e67, /*0x0,*/ 0xc9d989ea},
		//{"maps/rdm5.bsp",		/*0x0,*/ 0x998f2929, /*0x0,*/ 0xf4e5d735},
		//{"maps/rdm6.bsp",		/*0x0,*/ 0x06041298, /*0x0,*/ 0x7dc53173},
		//{"maps/rdm7.bsp",		/*0x0,*/ 0x5d1a6d8e, /*0x0,*/ 0x8c618a24},
		//{"maps/rdm8.bsp",		/*0x0,*/ 0x9563932d, /*0x0,*/ 0x07b32d04},
		//{"maps/rdm9.bsp",		/*0x0,*/ 0x6abbd719, /*0x0,*/ 0x726d4859},
		//{"maps/rdm10.bsp",		/*0x0,*/ 0xe5097216, /*0x0,*/ 0x54b0b786},
		//{"maps/rdm11.bsp",		/*0x0,*/ 0x55602288, /*0x0,*/ 0x890dc692},
		//{"maps/rdm12.bsp",		/*0x0,*/ 0xeadab431, /*0x0,*/ 0x762197d8},
		//{"maps/rdm13.bsp",		/*0x0,*/ 0x8b51e2d9, /*0x0,*/ 0x20678587},
		//{"maps/rdm14.bsp",		/*0x0,*/ 0x9f12a7af, /*0x0,*/ 0xe22f5e58},
		//{"maps/rhangar1.bsp",	/*0x0,*/ 0x29f4a50f, /*0x0,*/ 0xf5b1eeee},
		//{"maps/rhangar2.bsp",	/*0x0,*/ 0xfa92ab46, /*0x0,*/ 0x6167fa5e},
		//{"maps/rlava1.bsp",		/*0x0,*/ 0x54a8f40b, /*0x0,*/ 0xa16d41ec},
		//{"maps/rlava2.bsp",		/*0x0,*/ 0x994cec5d, /*0x0,*/ 0x92e9c884},
		//{"maps/rmine1.bsp",		/*0x0,*/ 0xf5e042cf, /*0x0,*/ 0x7a9eb9a8},
		//{"maps/rmine2.bsp",		/*0x0,*/ 0xeb8ddc19, /*0x0,*/ 0xa4e07d51},
		//{"maps/rsewer1.bsp",	/*0x0,*/ 0x934df93c, /*0x0,*/ 0x605443ab},
		//{"maps/rsewer2.bsp",	/*0x0,*/ 0x07e18c79, /*0x0,*/ 0x62513621},
//		{"maps/runit2.bsp",		/*0x0,*/ 0x074e46de, /*0x0,*/ 0x},	//pointless content-skip rooms for coop, I guess
//		{"maps/runit3.bsp",		/*0x0,*/ 0x36c066a7, /*0x0,*/ 0x},
//		{"maps/runit4.bsp",		/*0x0,*/ 0xd31d0e32, /*0x0,*/ 0x},
		//{"maps/rware1.bsp",		/*0x0,*/ 0xbba033a9, /*0x0,*/ 0x031eda0a},
		//{"maps/rware2.bsp",		/*0x0,*/ 0xc2993fed, /*0x0,*/ 0xcba25fcf},
	};
	unsigned int i;
	for (i = 0; i < sizeof(sums)/sizeof(sums[0]); i++)
	{
		if (checksum == sums[i].gpl2)
			if (!Q_strcasecmp(model->name, sums[i].name))
				return sums[i].id12;
	}
#endif
	return checksum;
}
#endif

static char Base64_Encode(int byt)
{
	if (byt >= 0 && byt < 26)
		return 'A' + byt - 0;
	if (byt >= 26 && byt < 52)
		return 'a' + byt - 26;
	if (byt >= 52 && byt < 62)
		return '0' + byt - 52;
	if (byt == 62)
		return '+';
	if (byt == 63)
		return '/';
	return '!';
}
static int Base64_Decode(char inp)
{
	if (inp >= 'A' && inp <= 'Z')
		return (inp-'A') + 0;
	if (inp >= 'a' && inp <= 'z')
		return (inp-'a') + 26;
	if (inp >= '0' && inp <= '9')
		return (inp-'0') + 52;
	if (inp == '+' || inp == '-')
		return 62;
	if (inp == '/' || inp == '_')
		return 63;
	//if (inp == '=') //padding char
	return 0;	//invalid
}

size_t Base64_EncodeBlock(const qbyte *in, size_t length, char *out, size_t outsize)
{
	char *start = out;
	char *end = out+outsize-1;
	unsigned int v;
	while(length > 0)
	{
		v = 0;
		if (length > 0)
			v |= in[0]<<16;
		if (length > 1)
			v |= in[1]<<8;
		if (length > 2)
			v |= in[2]<<0;

		if (out < end) *out++ = (length>=1)?Base64_Encode((v>>18)&63):'=';
		if (out < end) *out++ = (length>=1)?Base64_Encode((v>>12)&63):'=';
		if (out < end) *out++ = (length>=2)?Base64_Encode((v>>6)&63):'=';
		if (out < end) *out++ = (length>=3)?Base64_Encode((v>>0)&63):'=';

		in+=3;
		if (length <= 3)
			break;
		length -= 3;
	}
	end++;
	if (out < end)
		*out = 0;
	return out-start;
}
size_t Base64_EncodeBlockURI(const qbyte *in, size_t length, char *out, size_t outsize)
{	//special uri-safe version (also trims)
	outsize = Base64_EncodeBlock(in, length, out, outsize);
	for (length = 0; length < outsize; length++)
	{
		if (out[length] == '+')
			out[length] = '-';
		else if (out[length] == '/')
			out[length] = '_';
		else if (out[length] == '=')
		{	//truncate it here.
			out[length] = 0;
			return length;
		}
	}
	return outsize;
}
size_t Base64_DecodeBlock(const char *in, const char *in_end, qbyte *out, size_t outsize)
{
	qbyte *start = out;
	unsigned int v;
	if (!in_end)
		in_end = in + strlen(in);
	if (!out)
		return ((in_end-in+3)/4)*3 + 1;	//upper estimate, with null terminator for convienience.

	for (; outsize > 1;)
	{
		while(*in > 0 && *in < ' ')
			in++;
		if (in >= in_end || !*in || outsize < 1)
			break;	//end of message when EOF, otherwise error
		v  = Base64_Decode(*in++)<<18;
		while(*in > 0 && *in < ' ')
			in++;
		if (in >= in_end || !*in || outsize < 1)
			break;	//some kind of error
		v |= Base64_Decode(*in++)<<12;
		*out++ = (v>>16)&0xff;
		if (in >= in_end || *in == '=' || !*in || outsize < 2)
			break;	//end of message when '=', otherwise error
		v |= Base64_Decode(*in++)<<6;
		*out++ = (v>>8)&0xff;
		if (in >= in_end || *in == '=' || !*in || outsize < 3)
			break;	//end of message when '=', otherwise error
		v |= Base64_Decode(*in++)<<0;
		*out++ = (v>>0)&0xff;
		outsize -= 3;
	}
	return out-start;	//total written (no null, output is considered binary)
}
size_t Base16_DecodeBlock(const char *in, qbyte *out, size_t outsize)
{
	qbyte *start = out;
	if (!out)
		return ((strlen(in)+1)/2) + 1;

	for (; ishexcode(in[0]) && ishexcode(in[1]) && outsize > 0; outsize--, in+=2)
		*out++ = (dehex(in[0])<<4) | dehex(in[1]);
	return out-start;
}
size_t Base16_EncodeBlock(const char *in, size_t length, qbyte *out, size_t outsize)
{
	const char tab[16] = "0123456789abcdef";
	qbyte *start = out;
	if (!out)
		return (length*2) + 1;

	if (outsize > length*2)
		*out = 0;
	while (length --> 0)
	{
		*out++ = tab[(*in>>4)&0xf];
		*out++ = tab[(*in>>0)&0xf];
		in++;
	}
	return out-start;
}

/*
  Info Buffers
*/
const char *basicuserinfos[] =	//these are used by the client itself, and ignored when the user isn't using csqc.
{
	"*",	//special: all '*' prefixed keys
	"name",
	"team",
	"skin",
	"topcolor",
	"bottomcolor",
	"chat",	//ezquake's afk indicators
	NULL
};
const char *privateuserinfos[] =	//these can be sent to the server, but must NOT be reported to other clients.
{
	"_",		//special prefix: ignore comments
	"password",	//many users will forget to clear it after.
	"prx",		//if someone has this set, don't bother broadcasting it.
	"*ip",		//this is the ip the client used to connect to the server. this isn't useful as any proxy that would affect it can trivially strip/rewrite it anyway.
	NULL
};

void InfoSync_Remove(infosync_t *sync, size_t k)
{
	sync->numkeys--;
	Z_Free(sync->keys[k].name);
	memmove(sync->keys + k, sync->keys + k + 1, sizeof(*sync->keys)*(sync->numkeys-k));
}
void InfoSync_Clear(infosync_t *sync)
{
	size_t k;
	for (k = 0; k < sync->numkeys; k++)
		Z_Free(sync->keys[k].name);
	Z_Free(sync->keys);
	sync->keys = NULL;
	sync->numkeys = 0;
}
void InfoSync_Strip(infosync_t *sync, void *context)
{
	size_t k;
	if (!sync->numkeys)
		return;

	for (k = 0; k < sync->numkeys; )
	{
		if (sync->keys[k].context == context)
		{
			sync->numkeys--;
			Z_Free(sync->keys[k].name);
			memmove(sync->keys + k, sync->keys + k + 1, sizeof(*sync->keys)*(sync->numkeys-k));
		}
		else
			k++;
	}
}
void InfoSync_Add(infosync_t *sync, void *context, const char *name)
{
	size_t k;

	for (k = 0; k < sync->numkeys; k++)
	{
		if (sync->keys[k].context == context && !strcmp(sync->keys[k].name, name))
		{	//urr, it changed while we were sending it. reset!
			sync->keys[k].syncpos = 0;
			return;
		}
	}

	if (!ZF_ReallocElements((void**)&sync->keys, &sync->numkeys, sync->numkeys+1, sizeof(*sync->keys)))
		return; //out of memory!
	sync->keys[k].context = context;
	sync->keys[k].name = Z_StrDup(name);
	sync->keys[k].syncpos = 0;
}

static qboolean InfoBuf_NeedsEncoding(const char *str, size_t size)
{
	const char *c, *e = str+size;
	for (c = str; c < e; c++)
	{
		switch((unsigned char)*c)
		{
		case 255:	//invalid for vanilla qw, and also used for special encoding
		case '\\':	//abiguity with end-of-token
		case '\"':	//parsing often sends these enclosed in quotes
		case '\n':	//REALLY screws up parsing
		case '\r':	//generally bad form
		case 0:		//are we really doing this?
		case '$':	//a number of engines like expanding things inside quotes. make sure that cannot ever happen.
		case ';':	//in case someone manages to break out of quotes
			return true;
		}
	}
	return false;
}
qboolean InfoBuf_FindKey (infobuf_t *info, const char *key, size_t *idx)
{
	size_t k;
	for (k = 0; k < info->numkeys; k++)
	{
		if (!strcmp(info->keys[k].name, key))
		{
			*idx = k;
			return true;
		}
	}
	return false;
}
const char *InfoBuf_KeyForNumber(infobuf_t *info, int idx)
{	//allows itteration, removal can change the names of this/higher keys, but not lower keys.
	if (idx >= 0 && idx < info->numkeys)
		return info->keys[idx].name;
	return NULL;
}
char *InfoBuf_ReadKey (infobuf_t *info, const char *key, char *outbuf, size_t outsize)	//not to be used with blobs. writes to a user-supplied buffer
{
	size_t k;
	if (InfoBuf_FindKey(info, key, &k) && !info->keys[k].partial)
	{
		Q_strncpyz(outbuf, info->keys[k].value, outsize);
		return outbuf;
	}
	*outbuf = 0;
	return outbuf;
}
char *InfoBuf_ValueForKey (infobuf_t *info, const char *key)	//not to be used with blobs. cycles buffer and imposes a length limit.
{
	static	char value[4][1024];	// use multiple buffers so compares work without stomping on each other
	static	int	valueindex;
	COM_AssertMainThread("InfoBuf_ValueForKey");
	valueindex = (valueindex+1)&3;
	return InfoBuf_ReadKey(info, key, value[valueindex], sizeof(value[valueindex]));
}
const char *InfoBuf_BlobForKey (infobuf_t *info, const char *key, size_t *blobsize, qboolean *large)	//obtains a direct pointer to temp memory
{
	size_t k;
	if (InfoBuf_FindKey(info, key, &k) && !info->keys[k].partial)
	{
		if (large)
			*large = info->keys[k].large;
		*blobsize = info->keys[k].size;
		return info->keys[k].value;
	}
	if (large)
		*large = InfoBuf_NeedsEncoding(key, strlen(key));
	*blobsize = 0;
	return NULL;
}
qboolean InfoBuf_RemoveKey (infobuf_t *info, const char *key)
{
	size_t k;
	if (InfoBuf_FindKey(info, key, &k))
	{
		char *kn = info->keys[k].name;	//paranoid
		Z_Free(info->keys[k].value);
		info->numkeys--;
		info->totalsize -= strlen(info->keys[k].name)+2;
		info->totalsize -= info->keys[k].size;
		memmove(info->keys+k+0, info->keys+k+1, sizeof(*info->keys) * (info->numkeys-k));

		if (info->ChangeCB)
			info->ChangeCB(info->ChangeCTX, kn);
		Z_Free(kn);
		return true;	//only one entry per key, so we can give up here
	}
	return false;
}
char *InfoBuf_DecodeString(const char *instart, const char *inend, size_t *sz)
{
	char *ret = Z_Malloc(inend-instart + 1);	//guarenteed to end up equal or smaller
	int i;
	unsigned int v;
	if (*instart == '\xff')
	{	//base64-coded
		instart++;
		for (i = 0; instart+1 < inend;)
		{
			v  = Base64_Decode(*instart++)<<18;
			v |= Base64_Decode(*instart++)<<12;
			ret[i++] = (v>>16)&0xff;
			if (instart >= inend || *instart == '=')
				break;
			v |= Base64_Decode(*instart++)<<6;
			ret[i++] = (v>>8)&0xff;
			if (instart >= inend || *instart == '=')
				break;
			v |= Base64_Decode(*instart++)<<0;
			ret[i++] = (v>>0)&0xff;
		}
		ret[i] = 0;
		*sz = i;
	}
	else
	{	//as-is
		memcpy(ret, instart, inend-instart);
		ret[inend-instart] = 0;
		*sz = inend-instart;
	}
	return ret;
}

static qboolean InfoBuf_IsLarge(struct infokey_s *key)
{
	size_t namesize;
	if (key->partial)
		return true;

	if (key->size >= 64)
		return true;	//value length limits is a thing in vanilla qw.
						//note that qw reads values up to 512, but only sets them up to 64 bytes...
						//probably just so that people don't spot buffer overflows so easily.
	namesize = strlen(key->name);
	if (namesize >= 64)
		return true;	//key length limits is a thing in vanilla qw.

	if (InfoBuf_NeedsEncoding(key->name, namesize))
		return true;
	if (InfoBuf_NeedsEncoding(key->value, key->size))
		return true;
	return false;
}
//like InfoBuf_SetStarBlobKey, but understands partials.
qboolean InfoBuf_SyncReceive (infobuf_t *info, const char *key, size_t keysize, const char *val, size_t valsize, size_t offset, qboolean final)
{
	size_t k;
	size_t newsize;

	if (!InfoBuf_FindKey(info, key, &k))
	{	//its new
		if (!valsize)
			return false;	//and not set to anything new either

		if (offset)
			return false;	//was missing the initial message...

		k = info->numkeys;
		if (!ZF_ReallocElements((void**)&info->keys, &info->numkeys, info->numkeys+1, sizeof(*info->keys)))
			return false; //out of memory!
		info->keys[k].name = Z_StrDup(key);
		info->keys[k].size = 0;
		info->keys[k].value = NULL;
		info->totalsize += strlen(info->keys[k].name)+2;
	}
	else
	{
		if (!valsize)	//probably an error.
			return InfoBuf_RemoveKey(info, key);

		if (offset)
		{
			if (offset != info->keys[k].size)	//probably an error... should be progressive.
				return InfoBuf_RemoveKey(info, key);
		}
//		else silently truncate.
		info->totalsize -= info->keys[k].size;
	}

	newsize = offset + valsize;
	if (final)
	{	//release any excess memory (which could potentially be in the MB)
		if (!ZF_ReallocElements((void**)&info->keys[k].value, &info->keys[k].buffersize, newsize+1, 1))
			return false;
		info->keys[k].buffersize = newsize+1;
	}
	else
	{
		if (info->keys[k].buffersize < newsize+1)
		{
			if (!ZF_ReallocElements((void**)&info->keys[k].value, &info->keys[k].buffersize, newsize*2+1, 1))
				return false;
		}
	}
	memcpy(info->keys[k].value+offset, val, valsize);
	info->keys[k].value[newsize] = 0;
	info->keys[k].size = newsize;
	info->keys[k].partial = !final;
	info->keys[k].large = InfoBuf_IsLarge(&info->keys[k]);
	info->totalsize += info->keys[k].size;

	if (final)
		if (info->ChangeCB)
			info->ChangeCB(info->ChangeCTX, key);
	return true;
}
qboolean InfoBuf_SetStarBlobKey (infobuf_t *info, const char *key, const char *val, size_t valsize)
{
	size_t k;
	if (!val)
	{
		val = "";
		valsize = 0;
	}

	if (!InfoBuf_FindKey(info, key, &k))
	{	//its new
		if (!valsize)
			return false;	//and not set to anything new either

		k = info->numkeys;
		if (!ZF_ReallocElements((void**)&info->keys, &info->numkeys, info->numkeys+1, sizeof(*info->keys)))
			return false; //out of memory!
		info->keys[k].name = Z_StrDup(key);

		info->totalsize += strlen(info->keys[k].name)+2;
	}
	else
	{
		if (!valsize)
			return InfoBuf_RemoveKey(info, key);

		if (info->keys[k].size == valsize && !memcmp(info->keys[k].value, val, valsize))
			return false;	//nothing new
		Z_Free(info->keys[k].value);
		info->totalsize -= info->keys[k].size;
	}

	info->keys[k].buffersize = valsize+1;
	info->keys[k].size = valsize;
	info->keys[k].value = Z_Malloc(info->keys[k].buffersize);
	memcpy(info->keys[k].value, val, valsize);
	info->keys[k].value[valsize] = 0;
	info->keys[k].partial = false;
	info->keys[k].large = InfoBuf_IsLarge(&info->keys[k]);
	info->totalsize += info->keys[k].size;

	if (info->ChangeCB)
		info->ChangeCB(info->ChangeCTX, key);
	return true;
}
qboolean InfoBuf_SetKey (infobuf_t *info, const char *key, const char *val)
{
	// *keys are meant to be secure (or rather unsettable by the user, preventing spoofing of stuff like *ip)
	//		but note that this is pointless as a hacked client can send whatever initial *keys it wants (they are blocked mid-connection at least)
	// * userinfos are always sent even to clients that can't support large infokey blobs
	if (*key == '*')
		return false;
	return InfoBuf_SetStarBlobKey (info, key, val, strlen(val));
}
qboolean InfoBuf_SetStarKey (infobuf_t *info, const char *key, const char *val)
{
	return InfoBuf_SetStarBlobKey (info, key, val, strlen(val));
}

void InfoBuf_Clear(infobuf_t *info, qboolean all)
{//if all is false, leaves *keys
	size_t k;
	for (k = info->numkeys; k --> 0; )
	{
		if (all || *info->keys[k].name != '*')
		{
			Z_Free(info->keys[k].name);
			Z_Free(info->keys[k].value);
			info->numkeys--;
			memmove(info->keys+k+0, info->keys+k+1, sizeof(*info->keys) * (info->numkeys-k));
		}
	}
	if (!info->numkeys)
	{
		Z_Free(info->keys);
		info->keys = NULL;
	}
	info->totalsize = 0;
}
//the callback reports how much data it splurged.
/*qboolean InfoBuf_SyncSend(infobuf_t *info, size_t(*cb)(void *ctx, const char *key, const char *data, size_t offset, size_t size), void *ctx)
{
	size_t k;
	for (k = 0; k < info->numkeys; k++)
	{
		if (!info->keys[k].size)
		{	//null keys are actually just present to flag removals.
			//the sync is meant to be reliable, so these can be stripped once the update is sent.
			cb(ctx, info->keys[k].name, NULL, 0, 0);
			Z_Free(info->keys[k].name);
			Z_Free(info->keys[k].value);
			info->numkeys--;
			memmove(info->keys+k, info->keys+k+1, (info->numkeys-k)*sizeof(*info->keys));
			return true;
		}
		if (info->keys[k].syncedsize < info->keys[k].size)
		{	//regular update, possibly partial.
			info->keys[k].syncedsize += cb(ctx, info->keys[k].name, info->keys[k].value, info->keys[k].syncedsize, info->keys[k].size-info->keys[k].syncedsize);
			return true;
		}
	}

	return false;	//nothing to change.
}*/
void InfoBuf_Clone(infobuf_t *dest, infobuf_t *src)
{
	size_t k;

	InfoBuf_Clear(dest, true);

	dest->numkeys = src->numkeys;
	dest->keys = BZ_Malloc(sizeof(*dest->keys) * dest->numkeys);
	for (k = 0; k < dest->numkeys; k++)
	{
		dest->keys[k].partial = src->keys[k].partial;	//this is a problem. should we just not replicate partials?
		dest->keys[k].large = src->keys[k].large;
		dest->keys[k].name = Z_StrDup(src->keys[k].name);
		dest->keys[k].size = src->keys[k].size;
		dest->keys[k].value = Z_Malloc(src->keys[k].size+1);
		memcpy(dest->keys[k].value, src->keys[k].value, src->keys[k].size);
		dest->keys[k].value[src->keys[k].size] = 0;

		dest->totalsize += strlen(dest->keys[k].name)+2+dest->keys[k].size;
	}
}
void InfoBuf_FromString(infobuf_t *info, const char *infostring, qboolean append)
{
	if (!append)
		InfoBuf_Clear(info, true);
	if (*infostring && *infostring != '\\')
		Con_Printf("InfoBuf_FromString: invalid infostring \"%s\"\n", infostring);

	//all keys must start with a backslash
	while (*infostring++ == '\\')
	{
		const char *keystart = infostring;
		const char *keyend;
		const char *valstart;
		const char *valend;
		char *key;
		char *val;
		size_t keysize, valsize;
		while (*infostring)
		{
			if (*infostring == '\\')
				break;
			else infostring += 1;
		}
		keyend = infostring;
		if (*infostring++ != '\\')
			break;	//missing value...
		valstart = infostring;
		while (*infostring)
		{
			if (*infostring == '\\')
				break;
			else infostring += 1;
		}
		valend = infostring;

		key = InfoBuf_DecodeString(keystart, keyend, &keysize);
		val = InfoBuf_DecodeString(valstart, valend, &valsize);
		InfoBuf_SetStarBlobKey(info, key, val, valsize);
		Z_Free(key);
		Z_Free(val);
	}
}
//internal logic
static qboolean InfoBuf_EncodeString_Internal(const char *n, size_t s, char *out, char *end)
{
	size_t r = 0;
	const char *c;
	if (InfoBuf_NeedsEncoding(n, s))
	{
		unsigned int base64_cur = 0;
		unsigned int base64_bits = 0;
		r += 1;
		if (out < end) *out++ = (char)255;

		for (c = n; c < n+s; c++)
		{
			base64_cur |= *(const unsigned char*)c<<(16-	base64_bits);//first byte fills highest bits
			base64_bits += 8;

			if (base64_bits == 24)
			{
				r += 4;
				if (out < end) *out++ = Base64_Encode((base64_cur>>18)&63);
				if (out < end) *out++ = Base64_Encode((base64_cur>>12)&63);
				if (out < end) *out++ = Base64_Encode((base64_cur>>6)&63);
				if (out < end) *out++ = Base64_Encode((base64_cur>>0)&63);
				base64_bits = 0;
				base64_cur = 0;
			}
		}
		if (base64_bits != 0)
		{
			r += 4;
			if (out < end) *out++ = Base64_Encode((base64_cur>>18)&63);
			if (out < end) *out++ = Base64_Encode((base64_cur>>12)&63);
			if (base64_bits == 8)
			{
				if (out < end) *out++ = '=';
				if (out < end) *out++ = '=';
			}
			else
			{
				if (out < end) *out++ = Base64_Encode((base64_cur>>6)&63);
				if (base64_bits == 16)
				{
					if (out < end) *out++ = '=';
				}
				else
				{
					if (out < end) *out++ = Base64_Encode((base64_cur>>0)&63);
				}
			}
		}
	}
	else
	{
		for (c = n; c < n+s; c++)
		{
			r++;
			if (out < end) *out++ = *c;
		}
	}
	return r;
}
//public interface to make things easy
qboolean InfoBuf_EncodeString(const char *n, size_t s, char *out, size_t outsize)
{
	size_t l = InfoBuf_EncodeString_Internal(n, s, out, out+outsize);
	if (l < outsize)
	{
		out[l] = 0;
		return true;
	}
	*out = 0;
	return false;
}
static void *InfoBuf_EncodeString_Malloc(const char *n, size_t s)
{
	size_t l = InfoBuf_EncodeString_Internal(n, s, NULL, NULL);
	char *ret = BZ_Malloc(l+1);
	if (!ret || l != InfoBuf_EncodeString_Internal(n, s, ret, ret+l))
		Sys_Error("InfoBuf_EncodeString_Malloc: error\n");
	ret[l] = 0;
	return ret;
}
static size_t InfoBuf_EncodeStringSlash(const char *n, size_t s, char *out, char *end)
{
	size_t l = 1+InfoBuf_EncodeString_Internal(n, s, out+1, end);
	if (out < end)
		*out = '\\';
	return l;
}
size_t InfoBuf_ToString(infobuf_t *info, char *infostring, size_t maxsize, const char **priority, const char **ignore, const char **exclusive, infosync_t *sync, void *synccontext)
{
	size_t k, r = 1, l;
	char *o = infostring;
	char *e = infostring?infostring + maxsize-1:infostring;
	int pri, p;

	if (sync)	//if we have a sync object then we just wiped whatever infostrings that were set
		InfoSync_Strip(sync, synccontext);

	for (pri = 0; pri < 2; pri++)
	{
		for (k = 0; k < info->numkeys; k++)
		{
			if (exclusive)
			{
				for (l = 0; exclusive[l]; l++)
				{
					if (!strcmp(exclusive[l], info->keys[k].name))
						break;
					else if (exclusive[l][0] == '*' && !exclusive[l][1] && *info->keys[k].name == '*')
						break;	//read-only
					else if (exclusive[l][0] == '_' && !exclusive[l][1] && *info->keys[k].name == '_')
						break;	//comment
				}
				if (!exclusive[l])
					continue;	//ignore when not in the list
			}
			if (ignore)
			{
				for (l = 0; ignore[l]; l++)
				{
					if (!strcmp(ignore[l], info->keys[k].name))
						break;
					else if (ignore[l][0] == '*' && !ignore[l][1] && *info->keys[k].name == '*')
						break;	//read-only
					else if (ignore[l][0] == '_' && !ignore[l][1] && *info->keys[k].name == '_')
						break;	//comment
				}
				if (ignore[l])
					continue;	//ignore when in the list
			}
			if (priority)
			{
				for (l = 0; priority[l]; l++)
				{
					if (!strcmp(priority[l], info->keys[k].name))
						break;
					else if (priority[l][0] == '*' && !priority[l][1] && *info->keys[k].name == '*')
						break;	//read-only
					else if (priority[l][0] == '_' && !priority[l][1] && *info->keys[k].name == '_')
						break;	//comment
				}
				if (priority[l])
					p = 0;	//high priority
				else
					p = 1;	//low priority
			}
			else
			{
				if (*info->keys[k].name == '*')
					p = 0;	//keys that cannot be changed always have the highest priority (fixme: useless stuff like version doesn't need to be in here
				else
					p = 1;
			}
			if (pri != p)
				continue;

			if (!info->keys[k].large)	//lower priorities don't bother with extended blocks. be sure to prioritise them explicitly. they'd just bug stuff out.
			{
				l = InfoBuf_EncodeStringSlash(info->keys[k].name, strlen(info->keys[k].name), o, e);
				l += InfoBuf_EncodeStringSlash(info->keys[k].value, info->keys[k].size, o+l, e);
				r += l;
				if (o && o + l < e)
					o += l;
				else if (sync)
					InfoSync_Add(sync, synccontext, info->keys[k].name);	//not enough space. send this one later
			}
			else if (sync)
				InfoSync_Add(sync, synccontext, info->keys[k].name);	//don't include large/weird keys in the initial string
		}
	}
	*o = 0;
	return r;
}

void InfoBuf_Print(infobuf_t *info, const char *lineprefix)
{
	const char *key;
	const char *val;
	size_t k;

	for (k = 0; k < info->numkeys; k++)
	{
		char *partial = info->keys[k].partial?"<PARTIAL>":"";
		key = info->keys[k].name;
		val = info->keys[k].value;

		if (info->keys[k].size != strlen(info->keys[k].value))
			Con_Printf ("%s"S_COLOR_GREEN"%-20s"S_COLOR_RED"%s<BINARY %u BYTES>\n", lineprefix, key, partial, (unsigned int)info->keys[k].size);
		else if (info->keys[k].size > 64 || strchr(val, '\n') || strchr(val, '\r') || strchr(val, '\t'))
			Con_Printf ("%s"S_COLOR_GREEN"%-20s"S_COLOR_RED"%s<%u BYTES>\n", lineprefix, key, partial, (unsigned int)info->keys[k].size);
		else
			Con_Printf ("%s"S_COLOR_GREEN"%-20s"S_COLOR_WHITE"%s%s\n", lineprefix, key, partial, val);
	}
}
void InfoBuf_Enumerate (infobuf_t *info, void *ctx, void(*cb)(void *ctx, const char *key, const char *value))
{
	const char *key;
	const char *val;
	size_t k;

	for (k = 0; k < info->numkeys; k++)
	{
		key = info->keys[k].name;
		val = info->keys[k].value;
		cb(ctx, key, val);
	}
}

void InfoBuf_WriteToFile(vfsfile_t *f, infobuf_t *info, const char *commandname, int cvarflags)
{
	char *key;
	char *val;
	cvar_t *var;
	size_t k;

	for (k = 0; k < info->numkeys; k++)
	{
		key = info->keys[k].name;
		val = info->keys[k].value;
		if (*key == '*')	//unsettable, so don't write it for later setting.
			continue;

		if (cvarflags)
		{
			var = Cvar_FindVar(key);
			if (var && (var->flags & cvarflags))
				continue;	//this is saved via a cvar.
		}

		//blobs over a certain size cannot safely be parsed (due to Cmd_ExecuteString and com_token having limits)
		//so just don't write them.
		//if someone forces a write then the blob will get truncated.
		//note that blobs are limited im size serverside anyway, so this is probably higher than it needs to be.
		if (info->keys[k].size > 48000)
			continue;

		key = InfoBuf_EncodeString_Malloc(key, strlen(key));
		val = InfoBuf_EncodeString_Malloc(val, info->keys[k].size);
		if (!commandname)
		{	//with no command name, just writes a (big) infostring that we can parse later
			VFS_WRITE(f, "\\", 1);
			VFS_WRITE(f, key, strlen(key));
			VFS_WRITE(f, "\\", 1);
			VFS_WRITE(f, val, strlen(val));
		}
		else
		{
			VFS_WRITE(f, commandname, strlen(commandname));
			VFS_WRITE(f, " \"", 2);
			VFS_WRITE(f, key, strlen(key));
			VFS_WRITE(f, "\" \"", 3);
			VFS_WRITE(f, val, strlen(val));
			VFS_WRITE(f, "\"\n", 2);
		}
		BZ_Free(key);
		BZ_Free(val);
	}
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
===============
*/
char *Info_ValueForKey (const char *s, const char *key)
{
	char	pkey[1024];
	static	char value[4][1024];	// use two buffers so compares
								// work without stomping on each other
	static	int	valueindex;
	char	*o;

	COM_AssertMainThread("Info_ValueForKey");

	valueindex = (valueindex + 1) % 4;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
			*o++ = *s++;

			if (o+2 >= value[valueindex]+sizeof(value[valueindex]))	//hrm. hackers at work..
			{
				*value[valueindex]='\0';
				return value[valueindex];
			}
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return value[valueindex];

		if (!*s)
		{
			*value[valueindex]='\0';
			return value[valueindex];
		}
		s++;
	}
}

char *Info_KeyForNumber (const char *s, int num)
{
	static char	pkey[1024];
	char	*o;

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*pkey='\0';
				return pkey;
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*pkey='\0';
				return pkey;
			}
		}
		*o = 0;
		s++;

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*pkey='\0';
				return pkey;
			}
			s++;
		}

		if (!num--)
			return pkey;	//found the right one

		if (!*s)
		{
			*pkey='\0';
			return pkey;
		}
		s++;
	}
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	if (strstr (key, "\\"))
	{
		Con_Printf ("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			//strip out the value by copying the next string over the top of this one
			//(we were using strcpy, but valgrind moaned)
			while(*s)
				*start++ = *s++;
			*start = 0;
			return;
		}

		if (!*s)
			return;
	}

}

void Info_RemovePrefixedKeys (char *start, char prefix)
{
	char	*s;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	s = start;

	while (1)
	{
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (pkey[0] == prefix)
		{
			Info_RemoveKey (start, pkey);
			s = start;
		}

		if (!*s)
			return;
	}
}

/*static void Info_RemoveNonStarKeys (char *start)
{
	char	*s;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	s = start;

	while (1)
	{
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (pkey[0] != '*')
		{
			Info_RemoveKey (start, pkey);
			s = start;
		}

		if (!*s)
			return;
	}
}*/

void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize)
{
	char	newv[1024], *v;
	int		c;
#ifdef SERVERONLY
	extern cvar_t sv_highchars;
#endif

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
		Con_Printf ("Can't use a key with a \\\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
		Con_Printf ("Can't use a key with a \"\n");
		return;
	}

	if (strlen(key) >= MAX_INFO_KEY)// || strlen(value) >= MAX_INFO_KEY)
	{
		Con_Printf ("Keys and values must be < %i characters.\n", MAX_INFO_KEY);
		return;
	}

	// this next line is kinda trippy
	if (*(v = Info_ValueForKey(s, key)))
	{
		// key exists, make sure we have enough room for new value, if we don't,
		// don't change it!
		if (strlen(value) - strlen(v) + strlen(s) + 1 > maxsize)
		{
			if (*Info_ValueForKey(s, "*ver"))	//quick hack to kill off unneeded info on overflow. We can't simply increase the quantity of this stuff.
			{
				Info_RemoveKey(s, "*ver");
				Info_SetValueForStarKey (s, key, value, maxsize);
				return;
			}
			Con_Printf ("Info string length exceeded on addition of %s\n", key);
			return;
		}
	}
	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	snprintf (newv, sizeof(newv), "\\%s\\%s", key, value);

	if ((int)(strlen(newv) + strlen(s) + 1) > maxsize)
	{
		Con_Printf ("Info string length exceeded on addition of %s\n", key);
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newv;
	while (*v)
	{
		c = (unsigned char)*v++;
#ifndef SERVERONLY
		// client only allows highbits on name
//		if (stricmp(key, "name") != 0) {
//			c &= 127;
//			if (c < 32 || c > 127)
//				continue;
//			// auto lowercase team
//			if (stricmp(key, "team") == 0)
//				c = tolower(c);
//		}
#else
		if (!sv_highchars.value) {
			c &= 127;
			if (c < 32 || c > 127)
				continue;
		}
#endif
//		c &= 127;		// strip high bits
		if (c > 13) // && c < 127)
			*s++ = c;
	}
	*s = 0;
}

void Info_SetValueForKey (char *s, const char *key, const char *value, int maxsize)
{
	if (key[0] == '*')
	{
		Con_Printf ("Can't set * keys\n");
		return;
	}

	Info_SetValueForStarKey (s, key, value, maxsize);
}

void Info_Enumerate (const char *s, void *ctx, void(*cb)(void *ctx, const char *key, const char *value))
{
	char	key[1024];
	char	value[1024];
	char	*o;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\' && o < key+countof(key)-1)
			*o++ = *s++;
		*o = 0;

		if (!*s++)
		{
			//should never happen.
			cb(ctx, key, "");
			return;
		}

		o = value;
		while (*s && *s != '\\' && o < value+countof(value)-1)
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		cb(ctx, key, value);
	}
}

static void Info_PrintCB (void *ctx, const char *key, const char *value)
{
	char *lineprefix = ctx;
	Con_Printf ("%s%-20s%s\n", lineprefix, key, value);
}
void Info_Print (const char *s, const char *lineprefix)
{
	Info_Enumerate(s, (void*)lineprefix, Info_PrintCB);
}

/*static void Info_WriteToFile(vfsfile_t *f, char *info, char *commandname, int cvarflags)
{
	const char *quotedvalue;
	char buffer[1024];
	char *command;
	char *value, t;
	cvar_t *var;

	while(*info == '\\')
	{
		command = info+1;
		value = strchr(command, '\\');
		info = strchr(value+1, '\\');
		if (!info)	//eot..
			info = value+strlen(value);

		if (*command == '*')	//unsettable, so don't write it for later setting.
			continue;

		if (cvarflags)
		{
			var = Cvar_FindVar(command);
			if (var && var->flags & cvarflags)
				continue;	//this is saved via a cvar.
		}

		VFS_WRITE(f, commandname, strlen(commandname));
		VFS_WRITE(f, " ", 1);
		VFS_WRITE(f, command, value-command);
		VFS_WRITE(f, " ", 1);
		t = *info;
		*info = 0;
		quotedvalue = COM_QuotedString(value+1, buffer, sizeof(buffer), false);
		VFS_WRITE(f, quotedvalue, strlen(quotedvalue));
		*info = t;
		VFS_WRITE(f, "\n", 1);
	}
}*/

#if defined(HAVE_CLIENT) || defined(HAVE_SERVER)
static qbyte chktbl[1024 + 4] = {
0x78,0xd2,0x94,0xe3,0x41,0xec,0xd6,0xd5,0xcb,0xfc,0xdb,0x8a,0x4b,0xcc,0x85,0x01,
0x23,0xd2,0xe5,0xf2,0x29,0xa7,0x45,0x94,0x4a,0x62,0xe3,0xa5,0x6f,0x3f,0xe1,0x7a,
0x64,0xed,0x5c,0x99,0x29,0x87,0xa8,0x78,0x59,0x0d,0xaa,0x0f,0x25,0x0a,0x5c,0x58,
0xfb,0x00,0xa7,0xa8,0x8a,0x1d,0x86,0x80,0xc5,0x1f,0xd2,0x28,0x69,0x71,0x58,0xc3,
0x51,0x90,0xe1,0xf8,0x6a,0xf3,0x8f,0xb0,0x68,0xdf,0x95,0x40,0x5c,0xe4,0x24,0x6b,
0x29,0x19,0x71,0x3f,0x42,0x63,0x6c,0x48,0xe7,0xad,0xa8,0x4b,0x91,0x8f,0x42,0x36,
0x34,0xe7,0x32,0x55,0x59,0x2d,0x36,0x38,0x38,0x59,0x9b,0x08,0x16,0x4d,0x8d,0xf8,
0x0a,0xa4,0x52,0x01,0xbb,0x52,0xa9,0xfd,0x40,0x18,0x97,0x37,0xff,0xc9,0x82,0x27,
0xb2,0x64,0x60,0xce,0x00,0xd9,0x04,0xf0,0x9e,0x99,0xbd,0xce,0x8f,0x90,0x4a,0xdd,
0xe1,0xec,0x19,0x14,0xb1,0xfb,0xca,0x1e,0x98,0x0f,0xd4,0xcb,0x80,0xd6,0x05,0x63,
0xfd,0xa0,0x74,0xa6,0x86,0xf6,0x19,0x98,0x76,0x27,0x68,0xf7,0xe9,0x09,0x9a,0xf2,
0x2e,0x42,0xe1,0xbe,0x64,0x48,0x2a,0x74,0x30,0xbb,0x07,0xcc,0x1f,0xd4,0x91,0x9d,
0xac,0x55,0x53,0x25,0xb9,0x64,0xf7,0x58,0x4c,0x34,0x16,0xbc,0xf6,0x12,0x2b,0x65,
0x68,0x25,0x2e,0x29,0x1f,0xbb,0xb9,0xee,0x6d,0x0c,0x8e,0xbb,0xd2,0x5f,0x1d,0x8f,
0xc1,0x39,0xf9,0x8d,0xc0,0x39,0x75,0xcf,0x25,0x17,0xbe,0x96,0xaf,0x98,0x9f,0x5f,
0x65,0x15,0xc4,0x62,0xf8,0x55,0xfc,0xab,0x54,0xcf,0xdc,0x14,0x06,0xc8,0xfc,0x42,
0xd3,0xf0,0xad,0x10,0x08,0xcd,0xd4,0x11,0xbb,0xca,0x67,0xc6,0x48,0x5f,0x9d,0x59,
0xe3,0xe8,0x53,0x67,0x27,0x2d,0x34,0x9e,0x9e,0x24,0x29,0xdb,0x69,0x99,0x86,0xf9,
0x20,0xb5,0xbb,0x5b,0xb0,0xf9,0xc3,0x67,0xad,0x1c,0x9c,0xf7,0xcc,0xef,0xce,0x69,
0xe0,0x26,0x8f,0x79,0xbd,0xca,0x10,0x17,0xda,0xa9,0x88,0x57,0x9b,0x15,0x24,0xba,
0x84,0xd0,0xeb,0x4d,0x14,0xf5,0xfc,0xe6,0x51,0x6c,0x6f,0x64,0x6b,0x73,0xec,0x85,
0xf1,0x6f,0xe1,0x67,0x25,0x10,0x77,0x32,0x9e,0x85,0x6e,0x69,0xb1,0x83,0x00,0xe4,
0x13,0xa4,0x45,0x34,0x3b,0x40,0xff,0x41,0x82,0x89,0x79,0x57,0xfd,0xd2,0x8e,0xe8,
0xfc,0x1d,0x19,0x21,0x12,0x00,0xd7,0x66,0xe5,0xc7,0x10,0x1d,0xcb,0x75,0xe8,0xfa,
0xb6,0xee,0x7b,0x2f,0x1a,0x25,0x24,0xb9,0x9f,0x1d,0x78,0xfb,0x84,0xd0,0x17,0x05,
0x71,0xb3,0xc8,0x18,0xff,0x62,0xee,0xed,0x53,0xab,0x78,0xd3,0x65,0x2d,0xbb,0xc7,
0xc1,0xe7,0x70,0xa2,0x43,0x2c,0x7c,0xc7,0x16,0x04,0xd2,0x45,0xd5,0x6b,0x6c,0x7a,
0x5e,0xa1,0x50,0x2e,0x31,0x5b,0xcc,0xe8,0x65,0x8b,0x16,0x85,0xbf,0x82,0x83,0xfb,
0xde,0x9f,0x36,0x48,0x32,0x79,0xd6,0x9b,0xfb,0x52,0x45,0xbf,0x43,0xf7,0x0b,0x0b,
0x19,0x19,0x31,0xc3,0x85,0xec,0x1d,0x8c,0x20,0xf0,0x3a,0xfa,0x80,0x4d,0x2c,0x7d,
0xac,0x60,0x09,0xc0,0x40,0xee,0xb9,0xeb,0x13,0x5b,0xe8,0x2b,0xb1,0x20,0xf0,0xce,
0x4c,0xbd,0xc6,0x04,0x86,0x70,0xc6,0x33,0xc3,0x15,0x0f,0x65,0x19,0xfd,0xc2,0xd3,

// map checksum goes here
0x00,0x00,0x00,0x00
};

#if 0

static qbyte chkbuf[16 + 60 + 4];

static unsigned last_mapchecksum = 0;


/*
====================
COM_BlockSequenceCheckByte

For proxy protecting
====================
*/
qbyte	COM_BlockSequenceCheckByte (qbyte *base, int length, int sequence, unsigned mapchecksum)
{
	int		checksum;
	qbyte	*p;

	if (last_mapchecksum != mapchecksum) {
		last_mapchecksum = mapchecksum;
		chktbl[1024] = (mapchecksum & 0xff000000) >> 24;
		chktbl[1025] = (mapchecksum & 0x00ff0000) >> 16;
		chktbl[1026] = (mapchecksum & 0x0000ff00) >> 8;
		chktbl[1027] = (mapchecksum & 0x000000ff);

		Com_BlockFullChecksum (chktbl, sizeof(chktbl), chkbuf);
	}

	p = chktbl + (sequence % (sizeof(chktbl) - 8));

	if (length > 60)
		length = 60;
	memcpy (chkbuf + 16, base, length);

	length += 16;

	chkbuf[length] = (sequence & 0xff) ^ p[0];
	chkbuf[length+1] = p[1];
	chkbuf[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	chkbuf[length+3] = p[3];

	length += 4;

	checksum = LittleLong(Com_BlockChecksum (chkbuf, length));

	checksum &= 0xff;

	return checksum;
}
#endif

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
qbyte	COM_BlockSequenceCRCByte (qbyte *base, int length, int sequence)
{
	unsigned short crc;
	qbyte	*p;
	qbyte chkb[60 + 4];

	p = chktbl + (sequence % (sizeof(chktbl) - 8));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = (sequence & 0xff) ^ p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CalcHashInt(&hash_crc16, chkb, length);

	crc &= 0xff;

	return crc;
}


#if defined(Q2CLIENT) || defined(Q2SERVER)
static qbyte q2chktbl[1024] = {
0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
qbyte	Q2COM_BlockSequenceCRCByte (qbyte *base, int length, int sequence)
{
	int		n;
	qbyte	*p;
	int		x;
	qbyte chkb[60 + 4];
	unsigned short crc;


	if (sequence < 0)
		Sys_Error("sequence < 0, this shouldn't happen\n");

	p = q2chktbl + (sequence % (sizeof(q2chktbl) - 4));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CalcHashInt(&hash_crc16, chkb, length);

	for (x=0, n=0; n<length; n++)
		x += chkb[n];

	crc = (crc ^ x) & 0xff;

	return crc;
}

#endif
#endif

#ifdef _WIN32
// don't use these functions in MSVC8
#if (_MSC_VER < 1400)
int VARGS linuxlike_snprintf(char *buffer, int size, const char *format, ...)
{
#undef _vsnprintf
	int ret;
	va_list		argptr;

	if (size <= 0)
		return !buffer?-1:0;
	size--;

	va_start (argptr, format);
	ret = _vsnprintf (buffer,size, format,argptr);
	va_end (argptr);

	buffer[size] = '\0';

	return ret;
}

int VARGS linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr)
{	//_vsnprintf truncates WITHOUT NULL, and returns -1
#undef _vsnprintf
	int ret;

	if (size <= 0)
		return !buffer?-1:0;
	size--;

	ret = _vsnprintf (buffer,size, format,argptr);

	buffer[size] = '\0';

	return ret;
}
#else
int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...)
{	//vsnprintf_s safely truncates with null, but returns -1 on truncation rather than untruncated full length
	int ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = vsnprintf_s (buffer,size, _TRUNCATE, format,argptr);
	va_end (argptr);

	return ret;
}
#endif

#endif

// libSDL.a and libSDLmain.a mingw32 libs use this function for some reason, just here to shut gcc up
/*#ifdef _MINGW_VFPRINTF
int __mingw_vfprintf (FILE *__stream, const char *__format, __VALIST __local_argv)
{
  return vfprintf( __stream, __format, __local_argv );
}
#endif*/

int version_number(void)
{
	int base = FTE_VER_MAJOR * 10000 + FTE_VER_MINOR * 100;

#ifdef OFFICIAL_RELEASE
	base -= 1;
#endif

	return base;
}

char *version_string(void)
{
	static char s[128];
	static qboolean done;

	if (!done)
	{
#ifdef OFFICIAL_RELEASE
		Q_snprintfz(s, sizeof(s), "%s v%i.%02i", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR);
#elif defined(SVNREVISION) && defined(SVNDATE)
	#ifdef FTE_BRANCH
		//something like 'FTE master 6410M-HASH'
		Q_snprintfz(s, sizeof(s), "%s %s %s", DISTRIBUTION, STRINGIFY(FTE_BRANCH), STRINGIFY(SVNREVISION));
	#else
		if (!strncmp(STRINGIFY(SVNREVISION), "git-", 4))
			Q_snprintfz(s, sizeof(s), "%s %s", DISTRIBUTION, STRINGIFY(SVNREVISION));	//if both are defined then its a known unmodified svn revision.
		else
			Q_snprintfz(s, sizeof(s), "%s SVN %s", DISTRIBUTION, STRINGIFY(SVNREVISION));	//if both are defined then its a known unmodified svn revision.
	#endif
#else
	#if defined(SVNREVISION)
		if (!strncmp(STRINGIFY(SVNREVISION), "git-", 4))
			Q_snprintfz(s, sizeof(s), "%s %s %s", DISTRIBUTION, STRINGIFY(SVNREVISION), __DATE__);
		else if (strcmp(STRINGIFY(SVNREVISION), "-"))
			Q_snprintfz(s, sizeof(s), "%s SVN %s %s", DISTRIBUTION, STRINGIFY(SVNREVISION), __DATE__);
		else
	#endif
		Q_snprintfz(s, sizeof(s), "%s build %s", DISTRIBUTION, __DATE__);
#endif
		done = true;
	}

	return s;
}

//returns <=0 on error.
//this function is useful for auto updates.
int parse_revision_number(const char *s, qboolean strict)
{
	int rev;
	char *e;

	//no revision info in this build, meaning its custom built and thus cannot check against the available updated versions.
	if (!s || !strcmp(s, "-") || !*s)
		return false; //no version info at all.

	if (!strncmp(s, "git-", 4))
	{	//git gets messy and takes the form of one of the following...
		//bad: git-XXXXXXXX[-dirty]
		//git-tag-extracommits-hash[-dirty]
		//if 'tag' is [R]VVVV then someone's tagging revisions to match svn revisions.
		//if a fork wants to block updates, then they can either just disable engine updates or they can fiddle with this tagging stuff.
		s+=4;
		if (*s == 'r' || *s == 'R')
			s++;	//R prefix is optional.

		if (strict && strstr(s, "-dirty"))
			return false;	//boo hiss.

		rev = strtoul(s, &e, 10);
		if (*e == '-')
		{	//we used --long so this should be a count of extra commits
			if (strtoul(e+1, &e, 10) && strict)
				return false;	//doesn't exactly match the tag, and we're strict
			if (*e != '-')
				return false;	//no hash info? something odd is happening...
			//hash is uninteresting.
		}
		else	//looks like there's no tag info there, just a commit hash. don't consider it a valid revision number.
			return false;	//--long didn't
	}
	else
	{
		//svn: [lower-]upper[M]
		//git: revision-git-hash[-dirty]
		//git: branch-revision-git-hash[-dirty]
		rev = strtoul(s, &e, 10);
		if (!strncmp(e, "-git", 4))
		{	//if there's a -dirty in there then its bad.
			//we can't validate that the commit id matches the same branch as this build. we'll just have to live with it.
			if (strict && strstr(s, "-dirty"))
				return false;
		}
		else if (*e && strict)
			return false;	//something odd.
	}
	return rev;
}
int revision_number(qboolean strict)
{
#ifdef SVNREVISION
	return parse_revision_number(STRINGIFY(SVNREVISION), strict);
#else
	return 0;
#endif
}

//C90
void COM_TimeOfDay(date_t *date)
{
	struct tm *newtime;
	time_t long_time;

	time(&long_time);
	newtime = localtime(&long_time);

	date->day = newtime->tm_mday;
	date->mon = newtime->tm_mon;
	date->year = newtime->tm_year + 1900;
	date->hour = newtime->tm_hour;
	date->min = newtime->tm_min;
	date->sec = newtime->tm_sec;
	strftime( date->str, 128,
		"%a %b %d, %H:%M:%S %Y", newtime);
}





/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
#define	MAXPRINTMSG	4096
// FIXME: make a buffer size safe vsprintf?
void SV_FlushRedirect (void);
#ifndef HAVE_CLIENT

vfsfile_t *con_pipe;
#ifdef HAVE_SERVER
vfsfile_t *Con_POpen(const char *conname)
{
	if (!conname || !*conname)
	{
		if (con_pipe)
			VFS_CLOSE(con_pipe);
		con_pipe = VFSPIPE_Open(2, false);
		return con_pipe;
	}
	return NULL;
}
#endif

static void Con_PrintFromThread (void *ctx, void *data, size_t a, size_t b)
{
	Con_Printf("%s", (char*)data);
	BZ_Free(data);
}
void VARGS Con_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Con_PrintFromThread, NULL, Z_StrDup(msg), 0, 0);
		return;
	}

#ifdef HAVE_SERVER
	// add to redirected message
	if (sv_redirected)
	{
		if (strlen (msg) + strlen(sv_redirected_buf) > sizeof(sv_redirected_buf) - 1)
			SV_FlushRedirect ();
		strcat (sv_redirected_buf, msg);
		if (sv_redirected != -1)
			return;
	}
#endif

	Sys_Printf ("%s", msg);	// also echo to debugging console
	Con_Log(msg); // log to console

	if (con_pipe)
		VFS_PUTS(con_pipe, msg);
}
void Con_TPrintf (translation_t stringnum, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	const char *fmt;

	if (!Sys_IsMainThread())
	{	//shouldn't be redirected anyway...
		fmt = localtext(stringnum);
		va_start (argptr,stringnum);
		vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
		va_end (argptr);
		COM_AddWork(WG_MAIN, Con_PrintFromThread, NULL, Z_StrDup(msg), 0, 0);
		return;
	}

#ifdef HAVE_SERVER
	// add to redirected message
	if (sv_redirected)
	{
		fmt = langtext(stringnum,sv_redirectedlang);
		va_start (argptr,stringnum);
		vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
		va_end (argptr);

		if (strlen (msg) + strlen(sv_redirected_buf) > sizeof(sv_redirected_buf) - 1)
			SV_FlushRedirect ();
		strcat (sv_redirected_buf, msg);
		return;
	}
#endif

	fmt = localtext(stringnum);

	va_start (argptr,stringnum);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	Sys_Printf ("%s", msg);	// also echo to debugging console
	Con_Log(msg); // log to console

	if (con_pipe)
		VFS_PUTS(con_pipe, msg);
}
/*
================
Con_DPrintf

A Con_Printf that only shows up if the "developer" cvar is set
================
*/
static void Con_DPrintFromThread (void *ctx, void *data, size_t a, size_t b)
{
	Con_DLPrintf(a, "%s", (char*)data);
	BZ_Free(data);
}
void Con_DPrintf (const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	extern cvar_t log_developer;

	if (!developer.value && !log_developer.value)
		return;

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Con_DPrintFromThread, NULL, Z_StrDup(msg), 0, 0);
		return;
	}

#ifdef HAVE_SERVER
	// add to redirected message
	if (sv_redirected)
	{
		if (strlen (msg) + strlen(sv_redirected_buf) > sizeof(sv_redirected_buf) - 1)
			SV_FlushRedirect ();
		strcat (sv_redirected_buf, msg);
		if (sv_redirected != -1)
			return;
	}
#endif

	if (developer.value)
		Sys_Printf ("%s", msg);	// also echo to debugging console

	if (log_developer.value)
		Con_Log(msg); // log to console
}
void Con_DLPrintf (int level, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	extern cvar_t log_developer;

	if (developer.ival < level && !log_developer.value)
		return;

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (!Sys_IsMainThread())
	{
		COM_AddWork(WG_MAIN, Con_DPrintFromThread, NULL, Z_StrDup(msg), level, 0);
		return;
	}

#ifdef HAVE_SERVER
	// add to redirected message
	if (sv_redirected)
	{
		if (strlen (msg) + strlen(sv_redirected_buf) > sizeof(sv_redirected_buf) - 1)
			SV_FlushRedirect ();
		strcat (sv_redirected_buf, msg);
		if (sv_redirected != -1)
			return;
	}
#endif

	if (developer.ival >= level)
		Sys_Printf ("%s", msg);	// also echo to debugging console

	if (log_developer.value)
		Con_Log(msg); // log to console
}

//for spammed warnings, so they don't spam prints with every single frame/call. the timer arg should be a static local.
void VARGS Con_ThrottlePrintf (float *timer, int developerlevel, const char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
	va_end (argptr);

	if (developerlevel)
		Con_DLPrintf (developerlevel, "%s", msg);
	else
		Con_Printf("%s", msg);
}
#endif
