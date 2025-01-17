/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2020 DarkPlaces contributors

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

#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <assert.h>
#include "qtypes.h"
#include "qdefs.h"

/// MSVC has a different name for several standard functions
#ifdef WIN32
# define strcasecmp _stricmp
# define strncasecmp _strnicmp
#else
#include "strings.h"
#endif

// Create our own define for Mac OS X
#if defined(__APPLE__) && defined(__MACH__)
# define MACOSX
#endif


//============================================================================

#define ContainerOf(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct sizebuf_s
{
	qbool	allowoverflow;	///< if false, do a Sys_Error
	qbool	overflowed;		///< set to true if the buffer size failed
	unsigned char		*data;
	int			maxsize;
	int			cursize;
	int			readcount;
	qbool	badread;		// set if a read goes beyond end of message
} sizebuf_t;

void SZ_Clear (sizebuf_t *buf);
unsigned char *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const unsigned char *data, int length);
void SZ_HexDumpToConsole(const sizebuf_t *buf);

void Com_HexDumpToConsole(const unsigned char *data, int size);

unsigned short CRC_Block(const unsigned char *data, size_t size);
unsigned short CRC_Block_CaseInsensitive(const unsigned char *data, size_t size); // for hash lookup functions that use strcasecmp for comparison

unsigned char COM_BlockSequenceCRCByteQW(unsigned char *base, int length, int sequence);

// these are actually md4sum (mdfour.c)
unsigned Com_BlockChecksum (void *buffer, int length);
void Com_BlockFullChecksum (void *buffer, int len, unsigned char *outbuf);

void COM_Init_Commands(void);


//============================================================================
//							Endianess handling
//============================================================================

// check mem_bigendian if you need to know the system byte order

/*! \name Byte order functions.
 * @{
 */

// unaligned memory access crashes on some platform, so always read bytes...
#define BigShort(l) BuffBigShort((unsigned char *)&(l))
#define LittleShort(l) BuffLittleShort((unsigned char *)&(l))
#define BigLong(l) BuffBigLong((unsigned char *)&(l))
#define LittleLong(l) BuffLittleLong((unsigned char *)&(l))
#define BigFloat(l) BuffBigFloat((unsigned char *)&(l))
#define LittleFloat(l) BuffLittleFloat((unsigned char *)&(l))

/// Extract a big endian 32bit float from the given \p buffer.
float BuffBigFloat (const unsigned char *buffer);

/// Extract a big endian 32bit int from the given \p buffer.
int BuffBigLong (const unsigned char *buffer);

/// Extract a big endian 16bit short from the given \p buffer.
short BuffBigShort (const unsigned char *buffer);

/// Extract a little endian 32bit float from the given \p buffer.
float BuffLittleFloat (const unsigned char *buffer);

/// Extract a little endian 32bit int from the given \p buffer.
int BuffLittleLong (const unsigned char *buffer);

/// Extract a little endian 16bit short from the given \p buffer.
short BuffLittleShort (const unsigned char *buffer);

/// Encode a big endian 32bit int to the given \p buffer
void StoreBigLong (unsigned char *buffer, unsigned int i);

/// Encode a big endian 16bit int to the given \p buffer
void StoreBigShort (unsigned char *buffer, unsigned short i);

/// Encode a little endian 32bit int to the given \p buffer
void StoreLittleLong (unsigned char *buffer, unsigned int i);

/// Encode a little endian 16bit int to the given \p buffer
void StoreLittleShort (unsigned char *buffer, unsigned short i);
//@}

//============================================================================

// these versions are purely for internal use, never sent in network protocol
// (use Protocol_EnumForNumber and Protocol_NumberToEnum to convert)
typedef enum protocolversion_e
{
	PROTOCOL_UNKNOWN,
	PROTOCOL_DARKPLACES8, ///< added parting messages. WIP
	PROTOCOL_DARKPLACES7, ///< added QuakeWorld-style movement protocol to allow more consistent prediction
	PROTOCOL_DARKPLACES6, ///< various changes
	PROTOCOL_DARKPLACES5, ///< uses EntityFrame5 entity snapshot encoder/decoder which is based on a Tribes networking article at http://www.garagegames.com/articles/networking1/
	PROTOCOL_DARKPLACES4, ///< various changes
	PROTOCOL_DARKPLACES3, ///< uses EntityFrame4 entity snapshot encoder/decoder which is broken, this attempted to do partial snapshot updates on a QuakeWorld-like protocol, but it is broken and impossible to fix
	PROTOCOL_DARKPLACES2, ///< various changes
	PROTOCOL_DARKPLACES1, ///< uses EntityFrame entity snapshot encoder/decoder which is a QuakeWorld-like entity snapshot delta compression method
	PROTOCOL_QUAKEDP, ///< darkplaces extended quake protocol (used by TomazQuake and others), backwards compatible as long as no extended features are used
	PROTOCOL_NEHAHRAMOVIE, ///< Nehahra movie protocol, a big nasty hack dating back to early days of the Quake Standards Group (but only ever used by neh_gl.exe), this is potentially backwards compatible with quake protocol as long as no extended features are used (but in actuality the neh_gl.exe which wrote this protocol ALWAYS wrote the extended information)
	PROTOCOL_QUAKE, ///< quake (aka netquake/normalquake/nq) protocol
	PROTOCOL_QUAKEWORLD, ///< quakeworld protocol
	PROTOCOL_NEHAHRABJP, ///< same as QUAKEDP but with 16bit modelindex
	PROTOCOL_NEHAHRABJP2, ///< same as NEHAHRABJP but with 16bit soundindex
	PROTOCOL_NEHAHRABJP3 ///< same as NEHAHRABJP2 but with some changes
}
protocolversion_t;

/*! \name Message IO functions.
 * Handles byte ordering and avoids alignment errors
 * @{
 */

void MSG_InitReadBuffer (sizebuf_t *buf, unsigned char *data, int size);
void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, vec_t f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteUnterminatedString (sizebuf_t *sb, const char *s);
void MSG_WriteAngle8i (sizebuf_t *sb, vec_t f);
void MSG_WriteAngle16i (sizebuf_t *sb, vec_t f);
void MSG_WriteAngle32f (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord13i (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord16i (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord32f (sizebuf_t *sb, vec_t f);
void MSG_WriteCoord (sizebuf_t *sb, vec_t f, protocolversion_t protocol);
void MSG_WriteVector (sizebuf_t *sb, const vec3_t v, protocolversion_t protocol);
void MSG_WriteAngle (sizebuf_t *sb, vec_t f, protocolversion_t protocol);

void MSG_BeginReading (sizebuf_t *sb);
int MSG_ReadLittleShort (sizebuf_t *sb);
int MSG_ReadBigShort (sizebuf_t *sb);
int MSG_ReadLittleLong (sizebuf_t *sb);
int MSG_ReadBigLong (sizebuf_t *sb);
float MSG_ReadLittleFloat (sizebuf_t *sb);
float MSG_ReadBigFloat (sizebuf_t *sb);
char *MSG_ReadString (sizebuf_t *sb, char *string, size_t maxstring);
/// Same as MSG_ReadString except it returns the number of bytes written to *string excluding the \0 terminator.
size_t MSG_ReadString_len (sizebuf_t *sb, char *string, size_t maxstring);
size_t MSG_ReadBytes (sizebuf_t *sb, size_t numbytes, unsigned char *out);

#define MSG_ReadChar(sb) ((sb)->readcount >= (sb)->cursize ? ((sb)->badread = true, -1) : (signed char)(sb)->data[(sb)->readcount++])
#define MSG_ReadByte(sb) ((sb)->readcount >= (sb)->cursize ? ((sb)->badread = true, -1) : (unsigned char)(sb)->data[(sb)->readcount++])
/// Same as MSG_ReadByte but with no need to copy twice (first to `int` to check for -1) so each byte can be copied directly to a string[]
#define MSG_ReadByte_opt(sb) ((sb)->readcount >= (sb)->cursize ? ((sb)->badread = true, '\0') : (unsigned char)(sb)->data[(sb)->readcount++])
#define MSG_ReadShort MSG_ReadLittleShort
#define MSG_ReadLong MSG_ReadLittleLong
#define MSG_ReadFloat MSG_ReadLittleFloat

float MSG_ReadAngle8i (sizebuf_t *sb);
float MSG_ReadAngle16i (sizebuf_t *sb);
float MSG_ReadAngle32f (sizebuf_t *sb);
float MSG_ReadCoord13i (sizebuf_t *sb);
float MSG_ReadCoord16i (sizebuf_t *sb);
float MSG_ReadCoord32f (sizebuf_t *sb);
float MSG_ReadCoord (sizebuf_t *sb, protocolversion_t protocol);
void MSG_ReadVector (sizebuf_t *sb, vec3_t v, protocolversion_t protocol);
float MSG_ReadAngle (sizebuf_t *sb, protocolversion_t protocol);
//@}
//============================================================================

typedef float (*COM_WordWidthFunc_t) (void *passthrough, const char *w, size_t *length, float maxWidth); // length is updated to the longest fitting string into maxWidth; if maxWidth < 0, all characters are used and length is used as is
typedef int (*COM_LineProcessorFunc) (void *passthrough, const char *line, size_t length, float width, qbool isContination);
int COM_Wordwrap(const char *string, size_t length, float continuationSize, float maxWidth, COM_WordWidthFunc_t wordWidth, void *passthroughCW, COM_LineProcessorFunc processLine, void *passthroughPL);

extern char com_token[MAX_INPUTLINE];
extern unsigned com_token_len;
qbool COM_ParseToken_Simple(const char **datapointer, qbool returnnewline, qbool parsebackslash, qbool parsecomments);
qbool COM_ParseToken_QuakeC(const char **datapointer, qbool returnnewline);
qbool COM_ParseToken_VM_Tokenize(const char **datapointer, qbool returnnewline);
qbool COM_ParseToken_Console(const char **datapointer);

void COM_Init (void);
void COM_Shutdown (void);

char *va(char *buf, size_t buflen, const char *format, ...) DP_FUNC_PRINTF(3);
// does a varargs printf into provided buffer, returns buffer (so it can be called in-line unlike dpsnprintf)

/* Some versions of GCC with -Wc++-compat will complain if static_assert
 * is used even though the macro is valid C11, so make it happy anyway
 * because having build logs without any purple text is pretty satisfying.
 * TODO: Disable the flag by default in makefile, with an optional variable
 * to reenable it.
 */
#ifndef __cplusplus
#define DP_STATIC_ASSERT(expr, str) _Static_assert(expr, str)
#else
#define DP_STATIC_ASSERT(expr, str) static_assert(expr, str)
#endif

// snprintf and vsnprintf are NOT portable. Use their DP counterparts instead
#undef snprintf
#define snprintf DP_STATIC_ASSERT(0, "snprintf is forbidden for portability reasons. Use dpsnprintf instead.")
#undef vsnprintf
#define vsnprintf DP_STATIC_ASSERT(0, "vsnprintf is forbidden for portability reasons. Use dpvsnprintf instead.")

// documentation duplicated deliberately for the benefit of IDEs that support https://www.doxygen.nl/manual/docblocks.html
/// Returns the number of printed characters, excluding the final '\0'
/// or returns -1 if the buffer isn't big enough to contain the entire string.
/// Buffer is ALWAYS null-terminated.
extern int dpsnprintf (char *buffer, size_t buffersize, const char *format, ...) DP_FUNC_PRINTF(3);
/// Returns the number of printed characters, excluding the final '\0'
/// or returns -1 if the buffer isn't big enough to contain the entire string.
/// Buffer is ALWAYS null-terminated.
extern int dpvsnprintf (char *buffer, size_t buffersize, const char *format, va_list args);

// A bunch of functions are forbidden for security reasons (and also to please MSVS 2005, for some of them)
// LadyHavoc: added #undef lines here to avoid warnings in Linux
#undef strcat
#define strcat DP_STATIC_ASSERT(0, "strcat is forbidden for security reasons. Use dp_strlcat or memcpy instead.")
#undef strncat
#define strncat DP_STATIC_ASSERT(0, "strncat is forbidden for security reasons. Use dp_strlcat or memcpy instead.")
#undef strcpy
#define strcpy DP_STATIC_ASSERT(0, "strcpy is forbidden for security reasons. Use dp_strlcpy or memcpy instead.")
#undef strncpy
#define strncpy DP_STATIC_ASSERT(0, "strncpy is forbidden for security reasons. Use dp_strlcpy or memcpy instead.")
#undef stpcpy
#define stpcpy DP_STATIC_ASSERT(0, "stpcpy is forbidden for security reasons. Use dp_stpecpy or memcpy instead.")
#undef ustpcpy
#define ustpcpy DP_STATIC_ASSERT(0, "ustpcpy is forbidden for security reasons. Use dp_ustr2stp or memcpy instead.")
#undef ustr2stp
#define ustr2stp DP_STATIC_ASSERT(0, "ustr2stp is forbidden for security reasons. Use dp_ustr2stp or memcpy instead.")
#undef sprintf
#define sprintf DP_STATIC_ASSERT(0, "sprintf is forbidden for security reasons. Use dpsnprintf instead.")

#undef strlcpy
#define strlcpy DP_STATIC_ASSERT(0, "strlcpy is forbidden for stability and correctness. See common.h and common.c comments.")
#undef strlcat
#define strlcat DP_STATIC_ASSERT(0, "strlcat is forbidden for stability and correctness. See common.h and common.c comments.")


//============================================================================

extern	struct cvar_s	registered;
extern	struct cvar_s	cmdline;

typedef enum userdirmode_e
{
	USERDIRMODE_NOHOME, // basedir only
	USERDIRMODE_HOME, // Windows basedir, general POSIX (~/.)
	USERDIRMODE_MYGAMES, // pre-Vista (My Documents/My Games/), general POSIX (~/.)
	USERDIRMODE_SAVEDGAMES, // Vista (%USERPROFILE%/Saved Games/), OSX (~/Library/Application Support/), Linux (~/.config)
	USERDIRMODE_COUNT
}
userdirmode_t;

/// Returns the number of bytes written to *out excluding the \0 terminator.
size_t COM_ToLowerString(const char *in, char *out, size_t size_out);
/// Returns the number of bytes written to *out excluding the \0 terminator.
size_t COM_ToUpperString(const char *in, char *out, size_t size_out);
int COM_StringBeginsWith(const char *s, const char *match);
int COM_ReadAndTokenizeLine(const char **text, char **argv, int maxargc, char *tokenbuf, int tokenbufsize, const char *commentprefix);
size_t COM_StringLengthNoColors(const char *s, size_t size_s, qbool *valid);
size_t COM_StringDecolorize(const char *in, size_t size_in, char *out, size_t size_out, qbool escape_carets);



#define dp_strlcpy(dst, src, dsize) dp__strlcpy(dst, src, dsize, __func__, __LINE__)
#define dp_strlcat(dst, src, dsize) dp__strlcat(dst, src, dsize, __func__, __LINE__)
size_t dp__strlcpy(char *dst, const char *src, size_t dsize, const char *func, unsigned line);
size_t dp__strlcat(char *dst, const char *src, size_t dsize, const char *func, unsigned line);
char *dp_stpecpy(char *dst, char *end, const char *src);
char *dp_ustr2stp(char *dst, size_t dsize, const char *src, size_t slen);


void FindFraction(double val, int *num, int *denom, int denomMax);

// decodes XPM file to XPM array (as if #include'd)
char **XPM_DecodeString(const char *in);

size_t base64_encode(unsigned char *buf, size_t buflen, size_t outbuflen);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

float Com_CalcRoll (const vec3_t angles, const vec3_t velocity, const vec_t angleval, const vec_t velocityval);

#endif

