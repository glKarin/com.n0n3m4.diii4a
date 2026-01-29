// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include "progsint.h"

/*#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <setjmp.h>
#include <io.h>

#ifdef NeXT
#include <libc.h>
#endif
*/

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)NULL)->identifier)


// set these before calling CheckParm
extern int myargc;
extern const char **myargv;

//char *strupr (char *in);
//char *strlower (char *in);
int QCC_filelength (int handle);
int QCC_tell (int handle);

#if 0//def __GNUC__
	#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
	#define WARN_UNUSED_RESULT
#endif

int QC_strcasecmp (const char *s1, const char *s2);
int QC_strncasecmp(const char *s1, const char *s2, int n);

pbool QC_strlcat(char *dest, const char *src, size_t destsize) WARN_UNUSED_RESULT;
pbool QC_strlcpy(char *dest, const char *src, size_t destsize) WARN_UNUSED_RESULT;
pbool QC_strnlcpy(char *dest, const char *src, size_t srclen, size_t destsize) WARN_UNUSED_RESULT;
char *QC_strcasestr(const char *haystack, const char *needle);

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
	#define FTE_DEPRECATED  __attribute__((__deprecated__))	//no idea about the actual gcc version
	#if defined(_WIN32)
		#include <stdio.h>
		#ifdef __MINGW_PRINTF_FORMAT
			#define LIKEPRINTF(x) __attribute__((format(__MINGW_PRINTF_FORMAT,x,x+1)))
		#else
			#define LIKEPRINTF(x) __attribute__((format(ms_printf,x,x+1)))
		#endif
	#else
		#define LIKEPRINTF(x) __attribute__((format(printf,x,x+1)))
	#endif
#endif
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
	#define NORETURN __attribute__((noreturn))
#endif
#ifndef NORETURN
	#define NORETURN
#endif
#ifndef LIKEPRINTF
	#define LIKEPRINTF(x)
#endif

#ifdef _MSC_VER
#define QC_vsnprintf _vsnprintf
static void VARGS QC_snprintfz (char *dest, size_t size, const char *fmt, ...) LIKEPRINTF(3)
{
	va_list args;
	va_start (args, fmt);
	_vsnprintf (dest, size-1, fmt, args);
	va_end (args);
	//make sure its terminated.
	dest[size-1] = 0;
}
#define snprintf QC_snprintfz
#else
	#define QC_vsnprintf vsnprintf
	#define QC_snprintfz snprintf
#endif

double I_FloatTime (void);

void	VARGS QCC_Error (int errortype, const char *error, ...) LIKEPRINTF(2);
int QCC_CheckParm (const char *check);
const char *QCC_ReadParm (const char *check);


int 	SafeOpenWrite (char *filename, int maxsize);
int 	SafeOpenRead (char *filename);
void 	SafeRead (int handle, void *buffer, long count);
void 	SafeWrite (int handle, const void *buffer, long count);
pbool	SafeClose(int hand);
int SafeSeek(int hand, int ofs, int mode);
void 	*SafeMalloc (long size);


long	QCC_LoadFile (char *filename, void **bufferptr);
void	QCC_SaveFile (char *filename, void *buffer, long count);

void 	DefaultExtension (char *path, char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase (char *path, char *dest);
void	ExtractFileExtension (char *path, char *dest);

long 	ParseNum (char *str);

unsigned short *QCC_makeutf16(char *mem, size_t len, int *outlen, pbool *errors);
char *QCC_SanitizeCharSet(char *mem, size_t *len, pbool *freeresult, int *origfmt);


char *QCC_COM_Parse (const char *data);
char *QCC_COM_Parse2 (char *data);

unsigned int utf8_check(const void *in, unsigned int *value);

extern	char	qcc_token[1024];


#define qcc_iswhite(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t' || (c) == '\v')
#define qcc_iswhitesameline(c) ((c) == ' ' || (c) == '\t')
#define qcc_islineending(c,n) ((c) == '\n' || ((c) == '\r' && (n) != '\n'))	//to try to handle mac line endings, especially if they're in the middle of a line


enum
{
	UTF8_RAW,
	UTF8_BOM,
	UTF_ANSI,
	UTF16LE,
	UTF16BE,
	UTF32LE,
	UTF32BE,
};

#endif
