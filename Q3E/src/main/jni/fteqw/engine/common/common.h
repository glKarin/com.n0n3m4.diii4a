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
// comndef.h  -- general definitions

#include <stdio.h>

//make shared
#ifndef QDECL
	#ifdef _MSC_VER
		#define QDECL _cdecl
	#else
		#define QDECL
	#endif
#endif

#define VK_NO_STDINT_H //we're handling this. please don't cause conflicts. grr.
#if __STDC_VERSION__ >= 199901L || defined(__GNUC__) || _MSC_VER >= 1600
	//C99 has a stdint header which hopefully contains an intptr_t
	//its optional... but if its not in there then its unlikely you'll actually be able to get the engine to a stage where it *can* load anything
	#include <stdint.h>
	typedef intptr_t qintptr_t;
	typedef uintptr_t quintptr_t;
	#define qint16_t int16_t
	#define quint16_t uint16_t
	#define qint32_t int32_t
	#define quint32_t uint32_t
	#define qint64_t int64_t
	#define quint64_t uint64_t
	#define qintmax_t intmax_t
	#define quintmax_t uintmax_t
#else
	#define qint8_t signed char	//be explicit with this one.
	#define quint8_t unsigned char
	#define qint16_t short
	#define quint16_t unsigned short
	#define qint32_t int
	#define quint32_t unsigned qint32_t
	#if defined(_WIN64)
		#define qintptr_t __int64
		#define FTE_WORDSIZE 64
		#define quintptr_t unsigned qintptr_t
		#define qint64_t __int64
		#define quint64_t unsigned __int64
	#elif defined(_WIN32)
		#if !defined(_MSC_VER) || _MSC_VER < 1300
			#define __w64
		#endif
		typedef __int32 __w64 qintptr_t;	//add __w64 if you need msvc to shut up about unsafe type conversions
		typedef unsigned __int32 __w64 quintptr_t;
//		#define qintptr_t __int32
//		#define quintptr_t unsigned qintptr_t
		#define qint64_t __int64
		#define quint64_t unsigned __int64
		#define FTE_WORDSIZE 32
	#else
		#ifdef __LP64__
			#define qintptr_t long int
			#define qint64_t long int
			#define FTE_WORDSIZE 64
		#elif __WORDSIZE == 64
			#define qintptr_t long long
			#define qint64_t long long
			#define FTE_WORDSIZE 64
		#else
			#define qintptr_t long
			#define qint64_t long long
			#define FTE_WORDSIZE 32
		#endif
		#define quintptr_t unsigned qintptr_t
		#define quint64_t unsigned qint64_t
	#endif

	#define qintmax_t qint64_t
	#define quintmax_t quint64_t

	#ifndef uint32_t
		#define int8_t		qint8_t
		#define uint8_t		quint8_t
		#define int16_t		qint16_t
		#define uint16_t	quint16_t
		#define int32_t		qint32_t
		#define uint32_t	quint32_t
		#define int64_t		qint64_t
		#define uint64_t	quint64_t
		#define intptr_t	qintptr_t
		#define uintptr_t	quintptr_t
		#define intmax_t	qintmax_t
		#define uintmax_t	quintmax_t
	#endif
#endif

#ifndef FTE_WORDSIZE
	#ifdef __WORDSIZE
		#define FTE_WORDSIZE __WORDSIZE
	#elif defined(_WIN64)
		#define FTE_WORDSIZE 64
	#else
		#define FTE_WORDSIZE 32
	#endif
#endif

#ifdef _MSC_VER
	#if _MSC_VER >= 1900
		// MSVC 14 supports these
	#elif _MSC_VER >= 1310
		#define strtoull _strtoui64
		#define strtoll _strtoi64
	#else
		#define strtoull strtoul	//hopefully this won't cause too many issues
		#define strtoll strtol	//hopefully this won't cause too many issues
		#define DWORD_PTR DWORD		//32bit only
		#define ULONG_PTR ULONG
	#endif
#endif


typedef unsigned char 		qbyte;

// KJB Undefined true and false defined in SciTech's DEBUG.H header
#undef true
#undef false

#ifdef __cplusplus
typedef enum {qfalse, qtrue} qboolean;//false and true are forcivly defined.
//#define true qtrue
//#define false qfalse
#else
typedef enum {qfalse, qtrue}	qboolean;
#define true qtrue
#define false qfalse
#endif

#define STRINGIFY2(s) #s
#define STRINGIFY(s) STRINGIFY2(s)

#define	BASIC_INFO_STRING			196	//regular quakeworld. Sickening isn't it.
#define	EXTENDED_INFO_STRING	1024
#define	MAX_SERVERINFO_STRING	1024	//standard quake has 512 here.
#define	MAX_LOCALINFO_STRING	32768


#ifdef HAVE_LEGACY
#define legacyval(_legval,_newval) _legval
#else
#define legacyval(_legval,_newval) _newval
#endif

#ifdef HAVE_CLIENT
#define cls_state cls.state
#else
#define cls_state 0
#endif

#ifdef HAVE_SERVER
#define sv_state sv.state
#else
#define sv_state 0
#endif

struct netprim_s
{
	qbyte coordtype;	//low 4 bits are the size, upper 4 are disambiguation...
		#define COORDTYPE_UNDEFINED		0			//invalid
		#define COORDTYPE_FIXED_13_3	2			//vanilla/etc
		#define COORDTYPE_FIXED_16_8	3			//rmq
		#define COORDTYPE_FIXED_28_4	4			//rmq, pointless
		#define COORDTYPE_FLOAT_32		(4|0x80)	//fte/dp/rmq
		#define COORDTYPE_SIZE_MASK		0xf			//coordtype&mask == number of bytes.
	qbyte anglesize;
	qbyte flags;
		#define NPQ2_ANG16				(1u<<0)
		#define NPQ2_SOLID32			(1u<<1)
		#define NPQ2_R1Q2_UCMD			(1u<<2)

	qbyte pad;
};
//============================================================================

typedef enum {
	SZ_BAD,
	SZ_RAWBYTES,
	SZ_RAWBITS,
	SZ_HUFFMAN	//q3 style packets are horrible.
} sbpacking_t;
typedef struct sizebuf_s
{
	qbyte		*data;
	int			maxsize;	//storage size of data
	int			cursize;	//assigned size of data
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	sbpacking_t	packing;	//required for q3
	int			currentbit; //ignored for rawbytes

	struct netprim_s prim;	//for unsized write/read coord/angles
} sizebuf_t;

void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, const void *data, int length);
void SZ_Print (sizebuf_t *buf, const char *data);	// strcats onto the sizebuf

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

#ifdef USEAREAGRID
typedef struct
{
	link_t l;
	void *ed;
} areagridlink_t;
#endif


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((qbyte *)l - (qbyte*)&(((t *)0)->m)))

#define FOR_EACH_LINK(l,node) for (l = node.next ; l != &node ; l = l->next)
//============================================================================

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
//#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
//#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

#if defined(FTE_LITTLE_ENDIAN)
	#define bigendian		false

	#define LittleShort(x)	((short)(x))
	#define LittleLong(x)	((int)(x))
	#define LittleI64(x)	((qint64_t)(x))
	#define LittleFloat(x)	((float)(x))

	#define BigShort(x)		(ShortSwap(x))
	#define BigLong(x)		(LongSwap(x))
	#define BigI64(x)		(I64Swap(x))
	#define BigFloat(x)		(FloatSwap(x))
#elif defined(FTE_BIG_ENDIAN)
	#define bigendian		true

	#define BigShort(x)		((short)(x))
	#define BigLong(x)		((int)(x))
	#define BigI64(x)		((qint64_t)(x))
	#define BigFloat(x)		((float)(x))

	#define LittleShort(x)	(ShortSwap(x))
	#define LittleLong(x)	(LongSwap(x))
	#define LittleI64(x)	(I64Swap(x))
	#define LittleFloat(x)	(FloatSwap(x))
#else
	extern	qboolean		bigendian;

	extern	short	(*BigShort) (short l);
	extern	short	(*LittleShort) (short l);
	extern	int	(*BigLong) (int l);
	extern	int	(*LittleLong) (int l);
	extern	qint64_t	(*BigI64) (qint64_t l);
	extern	qint64_t	(*LittleI64) (qint64_t l);
	extern	float	(*BigFloat) (float l);
	extern	float	(*LittleFloat) (float l);
#endif

short		ShortSwap	(short l);
int			LongSwap	(int l);
qint64_t    I64Swap		(qint64_t l);
float		FloatSwap	(float f);

void COM_CharBias (signed char *c, int size);
void COM_SwapLittleShortBlock (short *s, int size);

//============================================================================

struct usercmd_s;

extern const struct usercmd_s nullcmd;

typedef union {	//note: reading from packets can be misaligned
	char b[4];
	short b2;
	int b4;
	float f;
} coorddata;
float MSG_FromCoord(coorddata c, int bytes);
coorddata MSG_ToCoord(float f, int bytes);
coorddata MSG_ToAngle(float f, int bytes);

void MSG_BeginWriting (sizebuf_t *msg, struct netprim_s prim, void *bufferstorage, size_t buffersize);
void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteBits (sizebuf_t *msg, int value, int bits);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteInt64 (sizebuf_t *sb, qint64_t c);
void MSG_WriteUInt64 (sizebuf_t *sb, quint64_t c);
void MSG_WriteULEB128  (sizebuf_t *sb, quint64_t c);
void MSG_WriteSignedQEX (sizebuf_t *sb, qint64_t c);
void MSG_WriteEntity (sizebuf_t *sb, unsigned int e);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteDouble (sizebuf_t *sb, double f);
void MSG_WriteString (sizebuf_t *sb, const char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteBigCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle8 (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSGFTE_WriteDeltaUsercmd (sizebuf_t *buf, const short baseanges[3], const struct usercmd_s *from, const struct usercmd_s *cmd);
void MSGQW_WriteDeltaUsercmd (sizebuf_t *sb, const struct usercmd_s *from, const struct usercmd_s *cmd);
void MSGCL_WriteDeltaUsercmd (sizebuf_t *sb, const struct usercmd_s *from, const struct usercmd_s *cmd);
void MSG_WriteDir (sizebuf_t *sb, float dir[3]);

extern	qboolean	msg_badread;		// set if a read goes beyond end of message
extern struct netprim_s msg_nullnetprim;

int MSG_PeekByte(void);

void MSG_BeginReading (sizebuf_t *sb, struct netprim_s prim);
void MSG_ChangePrimitives(struct netprim_s prim);
int MSG_GetReadCount(void);
int MSG_ReadChar (void);
int MSG_ReadBits(int bits);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadUInt16 (void);
int MSG_ReadLong (void);
qint64_t MSG_ReadInt64 (void);
quint64_t MSG_ReadUInt64 (void);
qint64_t MSG_ReadSLEB128 (void);
quint64_t MSG_ReadULEB128 (void);
qint64_t MSG_ReadSignedQEX (void);
struct client_s;
unsigned int MSGSV_ReadEntity (struct client_s *fromclient);
unsigned int MSGCL_ReadEntity (void);
unsigned int MSG_ReadBigEntity(void);
float MSG_ReadFloat (void);
double MSG_ReadDouble (void);
char *MSG_ReadStringBuffer (char *out, size_t outsize);
char *MSG_ReadString (void);
char *MSG_ReadStringLine (void);

float MSG_ReadCoord (void);
float MSG_ReadCoordFloat (void);
void MSG_ReadPos (float *pos);	//uses md2 normals to approximate a unit vector into a single byte.
void MSG_ReadDir (float *dir);	//simply 3 coords
float MSG_ReadAngle (void);
float MSG_ReadAngle16 (void);
void MSGQW_ReadDeltaUsercmd (const struct usercmd_s *from, struct usercmd_s *cmd, int qwprotocolver);
void MSGFTE_ReadDeltaUsercmd (const struct usercmd_s *from, struct usercmd_s *move);
void MSGQ2_ReadDeltaUsercmd (struct client_s *cl, const struct usercmd_s *from, struct usercmd_s *move);
void MSG_ReadData (void *data, int len);
void MSG_ReadSkip (int len);

int MSG_ReadSize16 (sizebuf_t *sb);
void MSG_WriteSize16 (sizebuf_t *sb, unsigned int sz);
void COM_DecodeSize(int solid, float *mins, float *maxs);
int COM_EncodeSize(const float *mins, const float *maxs);

//============================================================================

char *Q_strcpyline(char *out, const char *in, int maxlen);	//stops at '\n' (and '\r')

void Q_ftoa(char *str, float in);
char *Q_strlwr(char *str);
int wildcmp(const char *wild, const char *string);	//1 if match

#define Q_memset(d, f, c) memset((d), (f), (c))
#define Q_memcpy(d, s, c) memcpy((d), (s), (c))
#define Q_memmove(d, s, c) memmove((d), (s), (c))
#define Q_memcmp(m1, m2, c) memcmp((m1), (m2), (c))
#define Q_strcpy(d, s) strcpy((d), (s))
#define Q_strncpy(d, s, n) strncpy((d), (s), (n))
#define Q_strlen(s) ((int)strlen(s))
#define Q_strrchr(s, c) strrchr((s), (c))
#define Q_strcat(d, s) strcat((d), (s))
#define Q_strcmp(s1, s2) strcmp((s1), (s2))
#define Q_strncmp(s1, s2, n) strncmp((s1), (s2), (n))

qboolean VARGS Q_snprintfz (char *dest, size_t size, const char *fmt, ...) LIKEPRINTF(3);	//true means truncated (will also warn in debug builds).
qboolean VARGS Q_vsnprintfz (char *dest, size_t size, const char *fmt, va_list args);		//true means truncated (will also warn in debug builds).
void VARGS Com_sprintf(char *buffer, int size, const char *format, ...) LIKEPRINTF(3);

#define Q_strncpyS(d, s, n) do{const char *____in=(s);char *____out=(d);int ____i; for (____i=0;*(____in); ____i++){if (____i == (n))break;*____out++ = *____in++;}if (____i < (n))*____out='\0';}while(0)	//only use this when it should be used. If undiciided, use N
#define Q_strncpyN(d, s, n) do{if (n < 0)Sys_Error("Bad length in strncpyz");Q_strncpyS((d), (s), (n));((char *)(d))[n] = '\0';}while(0)	//this'll stop me doing buffer overflows. (guarenteed to overflow if you tried the wrong size.)
//#define Q_strncpyNCHECKSIZE(d, s, n) do{if (n < 1)Sys_Error("Bad length in strncpyz");Q_strncpyS((d), (s), (n));((char *)(d))[n-1] = '\0';((char *)(d))[n] = '255';}while(0)	//This forces nothing else to be within the buffer. Should be used for testing and nothing else.
#if 0
#define Q_strncpyz(d, s, n) Q_strncpyN(d, s, (n)-1)
#else
void QDECL Q_strncpyz(char*d, const char*s, int n);
#define Q_strncatz(dest, src, sizeofdest)	\
	do {	\
		strncat(dest, src, sizeofdest - strlen(dest) - 1);	\
		(dest)[sizeofdest - 1] = 0;	\
	} while (0)
#define Q_strncatz2(dest, src)	Q_strncatz(dest, src, sizeof(dest))
#endif
//#define Q_strncpy Please remove all strncpys
/*#ifndef strncpy
#define strncpy Q_strncpy
#endif*/

/*replacement functions which do not care for locale in text formatting ('C' locale), or are non-standard*/
char *Q_strcasestr(const char *haystack, const char *needle);
int Q_strncasecmp (const char *s1, const char *s2, int n);
int Q_strcasecmp (const char *s1, const char *s2);
int Q_strstopcasecmp(const char *s1start, const char *s1end, const char *s2);
int	Q_atoi (const char *str);
float Q_atof (const char *str);
void deleetstring(char *result, const char *leet);


//============================================================================

extern	char		com_token[65536];

typedef enum com_tokentype_e {TTP_UNKNOWN, TTP_STRING, TTP_LINEENDING, TTP_RAWTOKEN, TTP_EOF, TTP_PUNCTUATION} com_tokentype_t;
extern com_tokentype_t com_tokentype;

//these cast away the const for the return value.
//char *COM_Parse (const char *data);
#define COM_Parse(d) COM_ParseOut(d,com_token, sizeof(com_token))
#define COM_ParseOut(d,o,l) COM_ParseType(d,o,l,NULL)
char *COM_ParseType (const char *data, char *out, size_t outlen, com_tokentype_t *toktype);
char *COM_ParseStringSet (const char *data, char *out, size_t outlen);	//whitespace or semi-colon separators
char *COM_ParseStringSetSep (const char *data, char sep, char *out, size_t outsize);	//single-char-separator, no whitespace
char *COM_ParseCString (const char *data, char *out, size_t maxoutlen, size_t *writtenlen);
char *COM_StringParse (const char *data, char *token, unsigned int tokenlen, qboolean expandmacros, qboolean qctokenize);	//fancy version used for console etc parsing
#define COM_ParseToken(data,punct) COM_ParseTokenOut(data, punct, com_token, sizeof(com_token), &com_tokentype)
char *COM_ParseTokenOut (const char *data, const char *punctuation, char *token, size_t tokenlen, com_tokentype_t *tokentype);	//note that line endings are a special type of token.
qboolean COM_TrimString(char *str, char *buffer, int buffersize);	//trims leading+trailing whitespace writing to the specified buffer. returns false on truncation.
const char *COM_QuotedString(const char *string, char *buf, int buflen, qboolean omitquotes);	//inverse of COM_StringParse


extern	int		com_argc;
extern	const char	**com_argv;

int COM_CheckParm (const char *parm);	//WARNING: Legacy arguments should be listed in CL_ArgumentOverrides!
int COM_CheckNextParm (const char *parm, int last);
void COM_AddParm (const char *parm);

void COM_Shutdown (void);
void COM_Init (void);
void COM_InitArgv (int argc, const char **argv);
void COM_ParsePlusSets (qboolean docbuf);

typedef unsigned int conchar_t;
char *COM_DeFunString(conchar_t *str, conchar_t *stop, char *out, int outsize, qboolean ignoreflags, qboolean forceutf8);
#define PFS_KEEPMARKUP		1	//leave markup in the final string (but do parse it)
#define PFS_FORCEUTF8		2	//force utf-8 decoding
#define PFS_NOMARKUP		4	//strip markup completely
#ifdef HAVE_LEGACY
#define PFS_EZQUAKEMARKUP	8	//aim for compat with ezquake instead of q3 compat
#endif
#define PFS_CENTERED		16	//flag used by console prints (text should remain centered)
#define PFS_NONOTIFY		32	//flag used by console prints (text won't be visible other than by looking at the console)
conchar_t *COM_ParseFunString(conchar_t defaultflags, const char *str, conchar_t *out, int outsize_bytes, int keepmarkup);	//ext is usually CON_WHITEMASK, returns its null terminator
unsigned int utf8_decode(int *error, const void *in, char const**out);
unsigned int utf8_encode(void *out, unsigned int unicode, int maxlen);
unsigned int iso88591_encode(char *out, unsigned int unicode, int maxlen, qboolean markup);
unsigned int qchar_encode(char *out, unsigned int unicode, int maxlen, qboolean markup);
unsigned int COM_DeQuake(unsigned int unichar);

void COM_BiDi_Shutdown(void);

//small macro to tell COM_ParseFunString (and related functions like con_printf) that the input is a utf-8 string.
#define U8(s) "^`u8:" s "`="

//handles whatever charset is active, including ^U stuff.
unsigned int unicode_byteofsfromcharofs(const char *str, unsigned int charofs, qboolean markup);
unsigned int unicode_charofsfrombyteofs(const char *str, unsigned int byteofs, qboolean markup);
unsigned int unicode_encode(char *out, unsigned int unicode, int maxlen, qboolean markup);
unsigned int unicode_decode(int *error, const void *in, char const**out, qboolean markup);
size_t unicode_strtolower(const char *in, char *out, size_t outsize, qboolean markup);
size_t unicode_strtoupper(const char *in, char *out, size_t outsize, qboolean markup);
unsigned int unicode_charcount(const char *in, size_t buffersize, qboolean markup);
void unicode_strpad(char *out, size_t outsize, const char *in, qboolean leftalign, size_t minwidth, size_t maxwidth, qboolean markup);

char *COM_SkipPath (const char *pathname);
void QDECL COM_StripExtension (const char *in, char *out, int outlen);
void COM_StripAllExtensions (const char *in, char *out, int outlen);
void COM_FileBase (const char *in, char *out, int outlen);
int QDECL COM_FileSize(const char *path);
void COM_DefaultExtension (char *path, const char *extension, int maxlen);
qboolean COM_RequireExtension(char *path, const char *extension, int maxlen);
char *COM_FileExtension (const char *in, char *result, size_t sizeofresult);	//copies the extension, without the dot
const char *COM_GetFileExtension (const char *in, const char *term);	//returns the extension WITH the dot, allowing for scanning backwards.
void COM_CleanUpPath(char *str);

char	*VARGS va(const char *format, ...) LIKEPRINTF(1);
// does a varargs printf into a temp buffer

//============================================================================

struct cache_user_s;

extern char	com_gamepath[MAX_OSPATH];
extern char	com_homepath[MAX_OSPATH];
//extern char	com_configdir[MAX_OSPATH];	//dir to put cfg_save configs in
//extern	char	*com_basedir;

//qofs_Make is used to 'construct' a variable of qofs_t type. this is so the code can merge two 32bit ints on old systems and use a long long type internally without generating warnings about bit shifts when qofs_t is only 32bit instead.
//#if defined(__amd64__) || defined(_AMD64_) || __WORDSIZE == 64
#if !defined(FTE_TARGET_WEB)
	#if !defined(_MSC_VER) || _MSC_VER > 1200
		#define FS_64BIT
	#endif
#endif
#ifdef FS_64BIT
	//we should probably use off_t here, but then we have fun as to whether its actually 64bit or not, which results in warnings and problems with printf etc.
	typedef quint64_t qofs_t;	//type to use for a file offset
	#define qofs_Make(low,high) (low | (((qofs_t)(high))<<32))
	#define qofs_Low(ofs) ((ofs)&0xffffffffu)
	#define qofs_High(ofs) ((ofs)>>32)
	#define qofs_Error(ofs) ((ofs) == ~(quint64_t)0u)

	#define PRIxQOFS PRIx64
	#define PRIuQOFS PRIu64
#else
	typedef quint32_t qofs_t;	//type to use for a file offset
	#define qofs_Make(low,high) (low)
	#define qofs_Low(ofs) (ofs)
	#define qofs_High(ofs) (0)
	#define qofs_Error(ofs) ((ofs) == ~0ul)

	#define PRIxQOFS "x"
	#define PRIuQOFS "u"
#endif
#define qofs_ErrorValue() (~(qofs_t)0u)

typedef struct searchpathfuncs_s searchpathfuncs_t;
typedef struct searchpath_s
{
	searchpathfuncs_t *handle;

	unsigned int flags; //SPF_*

	char logicalpath[MAX_OSPATH];	//printable hunam-readable location of the package. generally includes a system path, including nested packages.
	char purepath[256];	//server tracks the path used to load them so it can tell the client
	char prefix[MAX_QPATH];	//prefix to add to each file within the archive. may also be ".." to mean ignore the top-level path.
	int crc_seed;	//can skip some hashes if this is cached.
	int crc_check;	//client sorts packs according to this checksum
	int crc_reply;	//client sends a different crc back to the server, for the paks it's actually loaded.
	int orderkey;	//used to check to see if the paths were actually changed or not.

	struct searchpath_s *next;
	struct searchpath_s *nextpure;
} searchpath_t;
typedef struct flocation_s{
	struct searchpath_s	*search;			//used to say which filesystem driver to open the file from
	void			*fhandle;				//used by the filesystem driver as a simple reference to the file
	char			rawname[MAX_OSPATH];	//blank means not readable directly
	qofs_t			offset;					//only usable if rawname is set.
	qofs_t			len;					//uncompressed length
} flocation_t;
struct vfsfile_s;

#define FSLF_IFFOUND			0		//
#define FSLF_DEEPONFAILURE		(1u<<0)	//upon failure, report that the file is so far into the filesystem as to be irrelevant
#define FSLF_DEPTH_INEXPLICIT	(1u<<1)	//depth is incremented for EVERY package, not just system/explicit paths.
#define FSLF_IGNOREBASEDEPTH	(1u<<3)	//depth is incremented for explicit mod paths, but not id1/qw/fte/paks/pk3s
#define FSLF_SECUREONLY			(1u<<4)	//ignore files from downloaded packages (ie: configs)
#define FSLF_DONTREFERENCE		(1u<<5) //don't add any reference flags to packages
#define FSLF_IGNOREPURE			(1u<<6) //use only the client's package list, ignore any lists obtained from the server (including any reordering)
#define FSLF_IGNORELINKS		(1u<<7) //ignore any pak/pk3 symlinks. system ones may still be followed.
#define FSLF_QUIET				(1u<<8)	//don't spam warnings about any dodgy paths.

//if loc is valid, loc->search is always filled in, the others are filled on success.
//standard return value is 0 on failure, or depth on success.
int FS_FLocateFile(const char *filename, unsigned int flags, flocation_t *loc);
struct vfsfile_s *FS_OpenReadLocation(const char *fname, flocation_t *location);	//fname used for extension-based filters
#define WP_REFERENCE	1
#define WP_FULLPATH		2
#define WP_FORCE		4
const char *FS_WhichPackForLocation(flocation_t *loc, unsigned int flags);
qboolean FS_GetLocationForPackageHandle(flocation_t *loc, searchpathfuncs_t *spath, const char *fname);
qboolean FS_GetLocMTime(flocation_t *location, time_t *modtime);
const char *FS_GetPackageDownloadFilename(flocation_t *loc);	//returns only packages (or null)
const char *FS_GetRootPackagePath(flocation_t *loc);			//favours packages, but falls back on gamedirs.

searchpath_t *FS_GetPackage(const char *package); //for fancy stuff that should probably have its own helper...
qboolean FS_GetPackageDownloadable(const char *package);
char *FS_GetPackHashes(char *buffer, int buffersize, qboolean referencedonly);
char *FS_GetPackNames(char *buffer, int buffersize, int referencedonly, qboolean ext);
qboolean FS_GenCachedPakName(const char *pname, const char *crc, char *local, int llen);	//returns false if the name is invalid.
void FS_ReferenceControl(unsigned int refflag, unsigned int resetflags);

#define FDEPTH_MISSING 0x7fffffff
#define COM_FDepthFile(filename,ignorepacks) FS_FLocateFile(filename,FSLF_DONTREFERENCE|FSLF_DEEPONFAILURE|(ignorepacks?0:FSLF_DEPTH_INEXPLICIT), NULL)
#define COM_FCheckExists(filename) FS_FLocateFile(filename,FSLF_IFFOUND, NULL)

typedef struct vfsfile_s
{
	int (QDECL *ReadBytes) (struct vfsfile_s *file, void *buffer, int bytestoread);
	int (QDECL *WriteBytes) (struct vfsfile_s *file, const void *buffer, int bytestowrite);
	qboolean (QDECL *Seek) (struct vfsfile_s *file, qofs_t pos);	//returns false for error
	qofs_t (QDECL *Tell) (struct vfsfile_s *file);
	qofs_t (QDECL *GetLen) (struct vfsfile_s *file);	//could give some lag
	qboolean (QDECL *Close) (struct vfsfile_s *file);	//returns false if there was some error.
	void (QDECL *Flush) (struct vfsfile_s *file);
	enum
	{
		SS_SEEKABLE,
		SS_SLOW,	//probably readonly, its fine for an occasional seek, its just really. really. slow.
		SS_PIPE,	//read can be seeked, write appends only.
		SS_UNSEEKABLE
	} seekstyle;

#ifdef _DEBUG
	char dbgname[MAX_QPATH];
#endif
} vfsfile_t;

#define VFS_ERROR_TRYLATER		0	//nothing to write/read yet.
#define VFS_ERROR_UNSPECIFIED	-1	//no reason given
#define VFS_ERROR_NORESPONSE	-2	//no reason given
#define VFS_ERROR_REFUSED		-3	//no reason given
#define VFS_ERROR_EOF			-4	//no reason given
#define VFS_ERROR_DNSFAILURE	-5	//weird one, but oh well
#define VFS_ERROR_WRONGCERT		-6	//server gave a certificate with the wrong name
#define VFS_ERROR_UNTRUSTED		-7	//server gave a certificate with the right name, but we don't have a full chain of trust

#define VFS_CLOSE(vf) ((vf)->Close(vf))
#define VFS_TELL(vf) ((vf)->Tell(vf))
#define VFS_GETLEN(vf) ((vf)->GetLen(vf))
#define VFS_SEEK(vf,pos) ((vf)->Seek(vf,pos))
#define VFS_READ(vf,buffer,buflen) ((vf)->ReadBytes(vf,buffer,buflen))
#define VFS_WRITE(vf,buffer,buflen) ((vf)->WriteBytes(vf,buffer,buflen))
#define VFS_FLUSH(vf) do{if((vf)->Flush)(vf)->Flush(vf);}while(0)
#define VFS_PUTS(vf,s) do{const char *t=s;(vf)->WriteBytes(vf,t,strlen(t));}while(0)
char *VFS_GETS(vfsfile_t *vf, char *buffer, size_t buflen);
void VARGS VFS_PRINTF(vfsfile_t *vf, const char *fmt, ...) LIKEPRINTF(2);

enum fs_relative{
	//note that many of theses paths can map to multiple system locations. FS_SystemPath/FS_DisplayPath can vary somewhat in terms of what it returns, generally favouring writable locations rather then the path that actually contains a file.
	FS_BINARYPATH,	//where the 'exe' is located. we'll check here for dlls too.
	FS_LIBRARYPATH,	//for system dlls and stuff
	FS_ROOT,		//./ (effectively -homedir if enabled, otherwise effectively -basedir arg)
	FS_SYSTEM,		//a system path. absolute paths are explicitly allowed and expected, but not required.
#define FS_RELATIVE_ISSPECIAL(r) ((r)<FS_GAME)
	//after this point, all types must be relative to a gamedir
	FS_GAME,		//standard search (not generally valid for writing/save/rename/delete/etc)
	FS_GAMEONLY,	//$gamedir/
	FS_BASEGAMEONLY,	//fte/
	FS_PUBGAMEONLY,		//$gamedir/ or qw/ but not fte/
	FS_PUBBASEGAMEONLY	//qw/ (fixme: should be the last non-private basedir)
};

qboolean COM_WriteFile (const char *filename, enum fs_relative fsroot, const void *data, int len);

char *FS_AbbreviateSize(char *buf, size_t bufsize, qofs_t fsize);	//just formats a filesize into the buffer and returns it.

void FS_FlushFSHashWritten(const char *fname);
void FS_FlushFSHashRemoved(const char *fname);
void FS_FlushFSHashFull(void);	//too much/unknown changed...
void FS_CreatePath(const char *pname, enum fs_relative relativeto);
qboolean FS_Rename(const char *oldf, const char *newf, enum fs_relative relativeto);	//0 on success, non-0 on error
qboolean FS_Rename2(const char *oldf, const char *newf, enum fs_relative oldrelativeto, enum fs_relative newrelativeto);
qboolean FS_Remove(const char *fname, enum fs_relative relativeto);	//0 on success, non-0 on error
qboolean FS_RemoveTree(searchpathfuncs_t *pathhandle, const char *fname);
qboolean FS_Copy(const char *source, const char *dest, enum fs_relative relativesource, enum fs_relative relativedest);
//qboolean FS_NativePath(const char *fname, enum fs_relative relativeto, char *out, int outlen);	//if you really need to fopen yourself
qboolean FS_SystemPath(const char *fname, enum fs_relative relativeto, char *out, int outlen);	//if you really need to fopen yourself
qboolean FS_DisplayPath(const char *fname, enum fs_relative relativeto, char *out, int outlen);	//retrieves a string for user display. prefixes may be masked for privacy.
qboolean FS_WriteFile (const char *filename, const void *data, int len, enum fs_relative relativeto);
void *FS_MallocFile(const char *filename, enum fs_relative relativeto, qofs_t *filesize);
vfsfile_t *QDECL FS_OpenVFS(const char *filename, const char *mode, enum fs_relative relativeto);
vfsfile_t *FS_OpenTemp(void);
vfsfile_t *FS_OpenTCP(const char *name, int defaultport, qboolean assumetls);

vfsfile_t *FS_OpenWithFriends(const char *fname, char *sysname, size_t sysnamesize, int numfriends, ...);

#define countof(array) (sizeof(array)/sizeof((array)[0]))
#ifdef _WIN32
//windows doesn't support utf-8. Which is a shame really, because that's the charset we expect from everything.
char *narrowen(char *out, size_t outlen, wchar_t *wide);
wchar_t *widen(wchar_t *out, size_t outbytes, const char *utf8);
#define __L(x) L ## x
#define _L(x) __L(x)
int MyRegGetIntValue(void *base, const char *keyname, const char *valuename, int defaultval);
qboolean MyRegGetStringValue(void *base, const char *keyname, const char *valuename, void *data, size_t datalen);	//data is utf8
qboolean MyRegGetStringValueMultiSz(void *base, const char *keyname, const char *valuename, void *data, int datalen);
qboolean MyRegSetValue(void *base, const char *keyname, const char *valuename, int type, const void *data, int datalen);	//string values are utf8
void MyRegDeleteKeyValue(void *base, const char *keyname, const char *valuename);
#endif

void FS_UnloadPackFiles(void);
void FS_ReloadPackFiles(void);
char *FSQ3_GenerateClientPacksList(char *buffer, int maxlen, int basechecksum);
void FS_PureMode(const char *gamedir, int mode, char *purenamelist, char *purecrclist, char *refnamelist, char *refcrclist, int seed);	//implies an fs_restart. ref package names are optional, for q3 where pure names don't contain usable paths
int FS_PureOkay(void);

//recursively tries to open files until it can get a zip.
vfsfile_t *CL_OpenFileInPackage(searchpathfuncs_t *search, char *name);
qboolean CL_ListFilesInPackage(searchpathfuncs_t *search, char *name, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, void *recursioninfo);

qbyte *QDECL COM_LoadStackFile (const char *path, void *buffer, int bufsize, size_t *fsize);
qbyte *COM_LoadTempFile (const char *path, unsigned int locateflags, size_t *fsize);
qbyte *COM_LoadTempMoreFile (const char *path, size_t *fsize);	//allocates a little bit more without freeing old temp
//qbyte *COM_LoadHunkFile (const char *path);

searchpathfuncs_t *COM_IteratePaths (void **iterator, char *pathbuffer, int pathbuffersize, char *dirname, int dirnamesize);
void COM_FlushFSCache(qboolean purge, qboolean domutex);	//a file was written using fopen
qboolean FS_Restarted(unsigned int *since);

enum manifestdeptype_e
{
	mdt_invalid,
	mdt_singlepackage,	//regular package, versioned.
	mdt_installation	//allowed to install to the root, only downloaded as part of an initial install.
};
typedef struct
{
	char *filename;		//filename the manifest was read from. not necessarily writable... NULL when the manifest is synthesised or from http.
	enum
	{
		MANIFEST_SECURITY_NOT,		//don't trust it, don't even allow downloadsurl.
		MANIFEST_SECURITY_DEFAULT,	//the default.fmf file may suggest packages+force sources
		MANIFEST_SECURITY_INSTALLER	//built-in fmf files can 'force' packages+sources
	} security;		//manifest was embedded in the engine. don't assume its already installed, but ask to install it (also, enable some extra permissions for writing dlls)

	enum
	{
		MANIFEST_UNSPECIFIED=0,
		MANIFEST_CURRENTVER
	} parsever;
	int minver;	//if the engine svn revision is lower than this, the manifest will not be used as an 'upgrade'.
	int maxver;	//if not 0, the manifest will not be used
	enum
	{
		MANIFEST_HOMEDIRWHENREADONLY=0,
		MANIFEST_NOHOMEDIR,
		MANIFEST_FORCEHOMEDIR,
	} homedirtype;
	char *mainconfig;	//eg "fte.cfg", reducing conflicts with other engines, but can be other values...
	char *updateurl;	//url to download an updated manifest file from.
	qboolean blockupdate;	//set to block the updateurl from being used this session. this avoids recursive updates when manifests contain the same update url.
	char *installation;	//optional hardcoded commercial name, used for scanning the registry to find existing installs.
	char *formalname;	//the commercial name of the game. you'll get FULLENGINENAME otherwise.
#ifdef PACKAGEMANAGER
	char *downloadsurl;	//optional installable files (menu)
	char *installupd;	//which download/updated package to install.
	qboolean installable;	//(expected) available packages give a playable experience, even if just a basic/demo version.
#endif
	char *protocolname;	//the name used for purposes of dpmaster
	char *defaultexec;	//execed after cvars are reset, to give game-specific engine-defaults.
	char *defaultoverrides;	//execed after default.cfg, to give usable defaults even when the mod the user is running is shit.
	char *eula;			//when running as an installer, the user will be presented with this as a prompt
	char *basedir;		//this is where we expect to find the data.
	char *iconname;		//path we can find the icon (relative to the fmf's location)

	char *schemes;		//protocol scheme used to connect to a server running this game, use com_parse.
	struct
	{
		enum
		{
			GAMEDIR_DEFAULTFLAGS=0,		//forgotten on gamedir switches (and a higher priority)
			GAMEDIR_BASEGAME=1u<<0,		//not forgotten on gamedir switches (and a lower priority)
			GAMEDIR_PRIVATE=1u<<1,		//don't report as the gamedir on networks.
			GAMEDIR_READONLY=1u<<2,		//don't write here...
			GAMEDIR_USEBASEDIR=1u<<3,	//packages will be read from the basedir (and homedir), but not other files. path is an empty string.
			GAMEDIR_STEAMGAME=1u<<4,	//finds the game via steam. must also be private+readonly.
			GAMEDIR_QSHACK=1u<<8,

			GAMEDIR_SPECIAL=GAMEDIR_USEBASEDIR|GAMEDIR_STEAMGAME,	//if one of these flags, then the gamedir cannot be simply concatenated to the basedir/homedir.
		} flags;
		char *path;
	} gamepath[8];
	struct manpack_s	//FIXME: this struct should be replaced with packagemanager info instead.
	{
		enum manifestdeptype_e type;
		char *packagename;	//"package:arch=ver"
		char *path;			//the 'pure' name
		char *prefix;
		qboolean crcknown;	//if the crc was specified
		unsigned int crc;	//the public crc
		char *mirrors[8];	//a randomized (prioritized-on-load) list of mirrors to use. (may be 'prompt:game,package', 'unzip:file,url', 'xz:url', 'gz:url'
		char *condition;	//only downloaded if this cvar is set | delimited allows multiple cvars.
		char *sha512;		//package must match this hash, if specified
		char *signature;	//signs the hash
		qofs_t filesize;
		int mirrornum;		//the index we last tried to download from, so we still work even if mirrors are down.
	} package[64];
} ftemanifest_t;
extern ftemanifest_t	*fs_manifest;	//currently active manifest.
void FS_Manifest_Free(ftemanifest_t *man);
ftemanifest_t *FS_Manifest_ReadMem(const char *fname, const char *basedir, const char *data);
ftemanifest_t *FS_Manifest_ReadSystem(const char *fname, const char *basedir);
void PM_Shutdown(qboolean soft);
void PM_Command_f(void);
qboolean PM_CanInstall(const char *packagename);

void COM_InitFilesystem (void);	//does not set up any gamedirs.
qboolean FS_DownloadingPackage(void);
void FS_CreateBasedir(const char *path);
qboolean FS_DirHasAPackage(char *basedir, ftemanifest_t *man);
qboolean FS_ChangeGame(ftemanifest_t *newgame, qboolean allowreloadconfigs, qboolean allowbasedirchange);
qboolean FS_GameIsInitialised(void);
void FS_Shutdown(void);
struct gamepacks
{
	char *package;
	char *path;
	char *url;
	char *subpath;	//within the package (for zips)
};
void COM_Gamedir (const char *dir, const struct gamepacks *packagespaths);
qboolean FS_PathURLCache(const char *url, char *path, size_t pathsize);	//converts a url to something that can be shoved into a filesystem
qboolean FS_GamedirIsOkay(const char *path);
char *FS_GetGamedir(qboolean publicpathonly);
char *FS_GetManifestArgs(void);
int FS_GetManifestArgv(char **argv, int maxargs);

struct zonegroup_s;
void *FS_LoadMallocGroupFile(struct zonegroup_s *ctx, char *path, size_t *fsize, qboolean filters);
void *FS_LoadMallocFile (const char *path, size_t *fsize);
qbyte *FS_LoadMallocFileFlags (const char *path, unsigned int locateflags, size_t *fsize);
qofs_t FS_LoadFile(const char *name, void **file);
void FS_FreeFile(void *file);

qbyte *COM_LoadFile (const char *path, unsigned int locateflags, int usehunk, size_t *filesize);

qboolean FS_LoadMapPackFile (const char *filename, searchpathfuncs_t *archive);
void FS_CloseMapPackFile (searchpathfuncs_t *archive);
void COM_FlushTempoaryPacks(void);

void COM_EnumerateFiles (const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm);
void COM_EnumerateFilesReverse (const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm);
searchpathfuncs_t *FS_OpenPackByExtension(vfsfile_t *f, searchpathfuncs_t *parent, const char *filename, const char *pakname, const char *pakpathprefix);

extern qboolean com_installer;	//says that the engine is running in an 'installer' mode, and that the correct basedir is not yet known.
extern	struct cvar_s	registered;
extern qboolean standard_quake;	//fixme: remove

#ifdef NQPROT
void COM_Effectinfo_Enumerate(int (*cb)(const char *pname));
#endif

struct model_s;
unsigned int COM_RemapMapChecksum(struct model_s *model, unsigned int checksum);

#define	MAX_INFO_KEY	256
char *Info_ValueForKey (const char *s, const char *key);
void Info_SetValueForKey (char *s, const char *key, const char *value, int maxsize);
void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize);
void Info_RemovePrefixedKeys (char *start, char prefix);
void Info_RemoveKey (char *s, const char *key);
char *Info_KeyForNumber (const char *s, int num);
void Info_Print (const char *s, const char *lineprefix);
void Info_Enumerate (const char *s, void *ctx, void(*cb)(void *ctx, const char *key, const char *value));
/*
void Info_RemoveNonStarKeys (char *start);
void Info_WriteToFile(vfsfile_t *f, char *info, char *commandname, int cvarflags);
*/

/*
  Info Buffers
  Keynames are still length limited, and may not contain nulls, but neither restriction applies to values.
  Using base64 encoding, we're able to encode problematic chars like quotes and newlines (and nulls).
  This allows mods to store image files inside userinfo.
*/
typedef struct
{
	struct infokey_s
	{
		qbyte			partial:1;		//partial values read as "".
		qbyte			large:1;		//requires partial/encoded transmission
		char			*name;
		size_t			size;
		size_t			buffersize;		//to avoid excessive reallocs
		char			*value;
	} *keys;
	size_t numkeys;
	size_t totalsize;	//so we can limit userinfo abuse.

	void (*ChangeCB)(void *context, const char *key);	//usually calls InfoSync_Add on all the interested parties.
	void *ChangeCTX;
} infobuf_t;
typedef struct
{
	struct
	{
		void			*context;
		char			*name;
		size_t			syncpos;		//reset to 0 when dirty.
	} *keys;
	size_t numkeys;
} infosync_t;
void InfoSync_Remove(infosync_t *sync, size_t k);
void InfoSync_Add(infosync_t *sync, void *context, const char *name);
void InfoSync_Clear(infosync_t *sync);	//wipes all memory etc.
void InfoSync_Strip(infosync_t *sync, void *context);	//Clears away all infos from that context.
extern const char *basicuserinfos[];	//note: has a leading *
extern const char *privateuserinfos[];	//key names that are not broadcast from the server
qboolean InfoBuf_FindKey (infobuf_t *info, const char *key, size_t *idx);
const char *InfoBuf_KeyForNumber (infobuf_t *info, int num);
const char *InfoBuf_BlobForKey (infobuf_t *info, const char *key, size_t *blobsize, qboolean *large);
char *InfoBuf_ReadKey (infobuf_t *info, const char *key, char *outbuf, size_t outsize);
char *InfoBuf_ValueForKey (infobuf_t *info, const char *key);
qboolean InfoBuf_RemoveKey (infobuf_t *info, const char *key);
qboolean InfoBuf_SetKey (infobuf_t *info, const char *key, const char *val);	//refuses to set *keys.
qboolean InfoBuf_SetStarKey (infobuf_t *info, const char *key, const char *val);
qboolean InfoBuf_SetStarBlobKey (infobuf_t *info, const char *key, const char *val, size_t valsize);
#define InfoBuf_SetValueForKey InfoBuf_SetKey
#define InfoBuf_SetValueForStarKey InfoBuf_SetStarKey
void InfoBuf_Clear(infobuf_t *info, qboolean all);
void InfoBuf_Clone(infobuf_t *dest, infobuf_t *src);
void InfoBuf_FromString(infobuf_t *info, const char *infostring, qboolean append);
char *InfoBuf_DecodeString(const char *instart, const char *inend, size_t *sz);
qboolean InfoBuf_EncodeString(const char *n, size_t s, char *out, size_t outsize);
size_t InfoBuf_ToString(infobuf_t *info, char *infostring, size_t maxsize, const char **priority, const char **ignore, const char **exclusive, infosync_t *sync, void *synccontext);	//_ and * can be used to indicate ALL such keys.
qboolean InfoBuf_SyncReceive (infobuf_t *info, const char *key, size_t keysize, const char *val, size_t valsize, size_t offset, qboolean final);
void InfoBuf_Print(infobuf_t *info, const char *prefix);
void InfoBuf_WriteToFile(vfsfile_t *f, infobuf_t *info, const char *commandname, int cvarflags);
void InfoBuf_Enumerate (infobuf_t *info, void *ctx, void(*cb)(void *ctx, const char *key, const char *value));


qbyte	COM_BlockSequenceCheckByte (qbyte *base, int length, int sequence, unsigned mapchecksum);
qbyte	COM_BlockSequenceCRCByte (qbyte *base, int length, int sequence);
qbyte	Q2COM_BlockSequenceCRCByte (qbyte *base, int length, int sequence);

size_t Base64_EncodeBlock(const qbyte *in, size_t length, char *out, size_t outsize);	//tries to null terminate, but returns length without termination.
size_t Base64_EncodeBlockURI(const qbyte *in, size_t length, char *out, size_t outsize); //slightly different chars for uri safety. also trims.
size_t Base64_DecodeBlock(const char *in, const char *in_end, qbyte *out, size_t outsize); // +/ and =
size_t Base16_EncodeBlock(const char *in, size_t length, qbyte *out, size_t outsize);
size_t Base16_DecodeBlock(const char *in, qbyte *out, size_t outsize);

#define DIGEST_MAXSIZE	(512/8)	//largest valid digest size, in bytes
typedef struct
{
	unsigned int digestsize;
	unsigned int contextsize;	//you need to alloca(te) this much memory...
	void (*init) (void *context);
	void (*process) (void *context, const void *data, size_t datasize);
	void (*terminate) (unsigned char *digest, void *context);
} hashfunc_t;
extern hashfunc_t hash_md4;			//required for vanilla qw mapchecks
extern hashfunc_t hash_md5;			//required for turn/etc
extern hashfunc_t hash_sha1;		//required for websockets, and ezquake's crypted rcon
extern hashfunc_t hash_sha2_224;
extern hashfunc_t hash_sha2_256;	//required for webrtc
extern hashfunc_t hash_sha2_384;
extern hashfunc_t hash_sha2_512;
extern hashfunc_t hash_crc16;		//aka ccitt, required for qw's clc_move and various bits of dp compat
extern hashfunc_t hash_crc16_lower;
#define hash_certfp hash_sha2_256	//This is the hash function we're using to compute *fp serverinfo. we can detect 1/2-256/2-512 by sizes, but we need consistency to avoid confusion in clientside things too.
unsigned int hashfunc_terminate_uint(const hashfunc_t *hash, void *context); //terminate, except returning the digest as a uint instead of a blob. folds the digest if longer than 4 bytes.
unsigned int CalcHashInt(const hashfunc_t *hash, const void *data, size_t datasize);
size_t CalcHash(const hashfunc_t *hash, unsigned char *digest, size_t maxdigestsize, const unsigned char *data, size_t datasize);
size_t CalcHMAC(const hashfunc_t *hashfunc, unsigned char *digest, size_t maxdigestsize, const unsigned char *data, size_t datalen, const unsigned char *key, size_t keylen);

int parse_revision_number(const char *revstr, qboolean strict);	//returns our 'svn' revision numbers
int revision_number(qboolean strict);	//returns our 'svn' revision numbers
int version_number(void);
char *version_string(void);


void TL_InitLanguages(const char *langpath);	//langpath is where the .po files can be found
void TL_Shutdown(void);
void T_FreeStrings(void);
char *T_GetString(int num);
void T_FreeInfoStrings(void);
char *T_GetInfoString(int num);

struct po_s;
struct po_s *PO_Create(void);
void PO_Merge(struct po_s *po, vfsfile_t *file);
const char *PO_GetText(struct po_s *po, const char *msg);
void PO_Close(struct po_s *po);
const char *TL_Translate(int language, const char *src);	//$foo translations.
void TL_Reformat(int language, char *out, size_t outsize, size_t numargs, const char **arg);	//"{0} died\n" formatting (with $foo translation, on each arg)

//
// log.c
//
typedef enum {
	LOG_CONSOLE,
	LOG_PLAYER,
	LOG_RCON,
	LOG_TYPES
} logtype_t;
void Log_String (logtype_t lognum, const char *s);
void Con_Log (const char *s);
void Log_Init(void);
void Log_ShutDown(void);
#ifdef IPLOG
void IPLog_Add(const char *ip, const char *name);	//for associating player ip addresses with names.
qboolean IPLog_Merge_File(const char *fname);
#endif
enum certlog_problem_e
{
	CERTLOG_WRONGHOST	=1<<0,
	CERTLOG_EXPIRED		=1<<1,
	CERTLOG_MISSINGCA	=1<<2,

	CERTLOG_UNKNOWN		=1<<3,
};
qboolean CertLog_ConnectOkay(const char *hostname, void *cert, size_t certsize, unsigned int certlogproblems);

#if defined(HAVE_SERVER) && defined(HAVE_CLIENT)
qboolean Log_CheckMapCompletion(const char *packagename, const char *mapname, float *besttime, float *fulltime, float *bestkills, float *bestsecrets);
void Log_MapNowCompleted(void);
#endif


/*used by and for botlib and q3 gamecode*/
#define MAX_TOKENLENGTH		1024
typedef struct pc_token_s
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
} pc_token_t;
#define fileHandle_t int
#define fsMode_t int


typedef struct
{
	int sec;
	int min;
	int hour;
	int day;
	int mon;
	int year;
	char str[128];
} date_t;
void COM_TimeOfDay(date_t *date);

//json.c
typedef struct json_s
{
	enum
	{
		json_type_string,
		json_type_number,
		json_type_object,
		json_type_array,
		json_type_true,
		json_type_false,
		json_type_null
	} type;
	const char *bodystart;
	const char *bodyend;

	struct json_s *parent;
	struct json_s *child;
	struct json_s *sibling;
	union
	{
		struct json_s **childlink;
		struct json_s **array;
	};
	size_t arraymax;	//note that child+siblings are kinda updated with arrays too, just not orphaned cleanly...
	qboolean used;	//set to say when something actually read/walked it, so we can flag unsupported things gracefully
	char name[1];
} json_t;
//main functions
json_t *JSON_Parse(const char *json);	//simple parsing. returns NULL if there's any kind of parsing error.
void JSON_Destroy(json_t *t);			//call this on the root once you're done
json_t *JSON_FindChild(json_t *t, const char *child);	//find a named child in an object (or an array, if you're lazy)
json_t *JSON_GetIndexed(json_t *t, unsigned int idx);	//find an indexed child in an array (or object, though slower)
double JSON_ReadFloat(json_t *t, double fallback);		//read a numeric value.
size_t JSON_ReadBody(json_t *t, char *out, size_t outsize);	//read a string value.
size_t JSON_GetCount(json_t *t);
//exotic fancy functions
struct jsonparsectx_s
{
	char const *const data;
	const size_t size;
	size_t pos;
};
json_t *JSON_ParseNode(json_t *t, const char *namestart, const char *nameend, struct jsonparsectx_s *ctx); //fancy parsing.
//helpers
json_t *JSON_FindIndexedChild(json_t *t, const char *child, unsigned int idx);		//just a helper.
qboolean JSON_Equals(json_t *t, const char *child, const char *expected);			//compares a bit faster.
quintptr_t JSON_GetUInteger(json_t *t, const char *child, unsigned int fallback);	//grabs a child node's uint value
qintptr_t JSON_GetInteger(json_t *t, const char *child, int fallback);				//grabs a child node's int value
qintptr_t JSON_GetIndexedInteger(json_t *t, unsigned int idx, int fallback);		//grabs an int from an array
double JSON_GetFloat(json_t *t, const char *child, double fallback);				//grabs a child node's numeric value
double JSON_GetIndexedFloat(json_t *t, unsigned int idx, double fallback);			//grabs a numeric value from an array
const char *JSON_GetString(json_t *t, const char *child, char *buffer, size_t buffersize, const char *fallback); //grabs a child node's string value. do your own damn indexing for an array.
//there's no write logic. Its probably easier to just snprintf it or something anyway.
