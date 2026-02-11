// cmdlib.c

#include "qcc.h"
#include <ctype.h>
//#include <sys/time.h>

#undef progfuncs

#define PATHSEPERATOR   '/'

#ifndef QCC
extern jmp_buf qcccompileerror;
#endif

// set these before calling CheckParm
int myargc;
const char **myargv;

char	qcc_token[1024];
int		qcc_eof;

const unsigned int		type_size[] = {1,	//void
						sizeof(string_t)/4,	//string
						1,	//float
						3,	//vector
						1,	//entity
						1,	//field
						sizeof(func_t)/4,//function
						1,  //pointer (its an int index)
						1,	//integer
						1,	//uint
						2,	//long
						2,	//ulong
						2,	//double
						3,	//fixme: how big should a variant be?
						0,	//ev_struct. variable sized.
						0,	//ev_union. variable sized.
						0,	//ev_accessor...
						0,	//ev_enum...
						0,	//ev_typedef
						1,	//ev_bool...
						0,	//bitfld...
						};

char *basictypenames[] = {
	"void",
	"string",
	"float",
	"vector",
	"entity",
	"field",
	"function",
	"pointer",
	"integer",
	"uint",
	"long",
	"ulong",
	"double",
	"variant",
	"struct",
	"union",
	"accessor",
	"enum",
	"typedef",
	"bool",
	"bitfield",
};

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/
short   (*PRBigShort) (short l);
short   (*PRLittleShort) (short l);
int     (*PRBigLong) (int l);
int     (*PRLittleLong) (int l);
float   (*PRBigFloat) (float l);
float   (*PRLittleFloat) (float l);


static short   QCC_SwapShort (short l)
{
	pbyte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

static short   QCC_Short (short l)
{
	return l;
}


static int    QCC_SwapLong (int l)
{
	pbyte    b1,b2,b3,b4;

	b1 = (pbyte)l;
	b2 = (pbyte)(l>>8);
	b3 = (pbyte)(l>>16);
	b4 = (pbyte)(l>>24);

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

static int    QCC_Long (int l)
{
	return l;
}


static float	QCC_SwapFloat (float l)
{
	union {pbyte b[4]; float f;} in, out;

	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.f;
}

static float	QCC_Float (float l)
{
	return l;
}

void SetEndian(void)
{
	union {pbyte b[2]; unsigned short s;} ed;
	ed.s = 255;
	if (ed.b[0] == 255)
	{
		PRBigShort		= QCC_SwapShort;
		PRLittleShort	= QCC_Short;
		PRBigLong		= QCC_SwapLong;
		PRLittleLong	= QCC_Long;
		PRBigFloat		= QCC_SwapFloat;
		PRLittleFloat	= QCC_Float;
	}
	else
	{
		PRBigShort		= QCC_Short;
		PRLittleShort	= QCC_SwapShort;
		PRBigLong		= QCC_Long;
		PRLittleLong	= QCC_SwapLong;
		PRBigFloat		= QCC_Float;
		PRLittleFloat	= QCC_SwapFloat;
	}
}


pbool QC_strlcat(char *dest, const char *src, size_t destsize)
{
	size_t curlen = strlen(dest);
	if (!destsize)
		return false;	//err
	dest += curlen;
	while(*src && ++curlen < destsize)
		*dest++ = *src++;
	*dest = 0;
	return !*src;
}
pbool QC_strlcpy(char *dest, const char *src, size_t destsize)
{
	size_t curlen = 0;
	if (!destsize)
		return false;	//err
	while(*src && ++curlen < destsize)
		*dest++ = *src++;
	*dest = 0;
	return !*src;
}
pbool QC_strnlcpy(char *dest, const char *src, size_t srclen, size_t destsize)
{
	size_t curlen = 0;
	if (!destsize)
		return false;	//err
	for(; *src && srclen > 0 && ++curlen < destsize; srclen--)
		*dest++ = *src++;
	*dest = 0;
	return !srclen;
}

char *QC_strcasestr(const char *haystack, const char *needle)
{
	int i;
	int matchamt=0;
	for(i=0;haystack[i];i++)
	{
		if (tolower(haystack[i]) != tolower(needle[matchamt]))
			matchamt = 0;
		if (tolower(haystack[i]) == tolower(needle[matchamt]))
		{
			matchamt++;
			if (needle[matchamt]==0)
				return (char *)&haystack[i-(matchamt-1)];
		}
	}
	return 0;
}

#if !defined(MINIMAL) && !defined(OMIT_QCC)
/*
================
I_FloatTime
================
*/
/*
double I_FloatTime (void)
{
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

  */


#ifdef QCC
int QC_strncasecmp (const char *s1, const char *s2, int n)
{
	int             c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}

	return -1;
}

int QC_strcasecmp (const char *s1, const char *s2)
{
	return QC_strncasecmp(s1, s2, 0x7fffffff);
}

#else
int QC_strncasecmp(const char *s1, const char *s2, int n);
int QC_strcasecmp (const char *s1, const char *s2)
{
	return QC_strncasecmp(s1, s2, 0x7fffffff);
}

#endif



#endif	//minimal
/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *QCC_COM_Parse (const char *data)
{
	int		c;
	int		len;

	len = 0;
	qcc_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) && qcc_iswhite(c))
		data++;
	if (!c)
		return NULL;

// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

	// skip /* comments
	if (c=='/' && data[1] == '*')
	{
		while (data[1] && (data[0] != '*' || data[1] != '/'))
			data++;
		data+=2;
		goto skipwhite;
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		do
		{
			c = *data++;
			if (c=='\\' && *data == '\"')
				c = *data++;	//allow C-style string escapes
			else if (c=='\\' && *data == '\\')
				c = *data++;	// \ is now a special character so it needs to be marked up using itself
			else if (c=='\\' && *data == 'n')
			{					// and do new lines while we're at it.
				c = '\n';
				data++;
			}
			else if (c=='\\' && *data == 'r')
			{					// and do mac lines while we're at it.
				c = '\r';
				data++;
			}
			else if (c=='\\' && *data == 't')
			{					// and do tabs while we're at it.
				c = '\t';
				data++;
			}
			else if (c=='\"')
			{
				qcc_token[len] = 0;
				return (char*)data;
			}
			else if (c=='\0')
			{
//				printf("ERROR: Unterminated string\n");
				qcc_token[len] = 0;
				return (char*)data;
			}
			else if (c=='\n' || c=='\r')
			{	//new lines are awkward.
				//vanilla saved games do not add \ns on load
				//terminating the string on a new line thus has compatbility issues.
				//while "wad" "c:\foo\" does happen in the TF community (fucked tools)
				//so \r\n terminates the string if the last char was an escaped quote, but not otherwise.
				if (len > 0 && qcc_token[len-1] == '\"')
				{
//					printf("ERROR: new line in string\n");
					qcc_token[len] = 0;
					return (char*)data;
				}
			}
			if (len >= sizeof(qcc_token)-1)
				;
			else
				qcc_token[len] = c;
			len++;
		} while (1);
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c==',')
	{
		qcc_token[len] = c;
		len++;
		qcc_token[len] = 0;
		return (char*)data+1;
	}

// parse a regular word
	do
	{
		if (len >= sizeof(qcc_token)-1)
			;
		else
			qcc_token[len++] = c;
		data++;
		c = *data;
		if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c=='\"' || c==',')
			break;
	} while (c && !qcc_iswhite(c));

	qcc_token[len] = 0;
	return (char*)data;
}

//more C tokens...
char *QCC_COM_Parse2 (char *data)
{
	int		c;
	int		len;

	len = 0;
	qcc_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) && qcc_iswhite(c))
		data++;
	if (!c)
		return NULL;

// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		do
		{
			c = *data++;
			if (c=='\\' && *data == '\"')
				c = *data++;	//allow C-style string escapes
			else if (c=='\\' && *data == '\\')
				c = *data++;	// \ is now a special character so it needs to be marked up using itself
			else if (c=='\\' && *data == 'n')
			{					// and do new lines while we're at it.
				c = '\n';
				data++;
			}
			else if (c=='\"'||c=='\0')
			{
				if (len < sizeof(qcc_token)-1)
					qcc_token[len++] = 0;
				break;
			}
			if (len >= sizeof(qcc_token)-1)
				;
			else
				qcc_token[len++] = c;
		} while (1);
	}

// parse numbers
	if (c >= '0' && c <= '9')
	{
		if (c == '0' && data[1] == 'x')
		{	//parse hex
			qcc_token[0] = '0';
			c='x';
			len=1;
			data++;
			for(;;)
			{	//parse regular number
				if (len >= sizeof(qcc_token)-1)
					;
				else
					qcc_token[len++] = c;
				data++;
				c = *data;
				if ((c<'0'|| c>'9') && (c<'a'||c>'f') && (c<'A'||c>'F') && c != '.')
					break;
			}

		}
		else
		{
			for(;;)
			{	//parse regular number
				if (len >= sizeof(qcc_token)-1)
					;
				else
					qcc_token[len++] = c;
				data++;
				c = *data;
				if ((c<'0'|| c>'9') && c != '.')
					break;
			}
		}

		qcc_token[len] = 0;
		return data;
	}
// parse words
	else if ((c>= 'a' && c <= 'z') || (c>= 'A' && c <= 'Z') || c == '_')
	{
		do
		{
			if (len >= sizeof(qcc_token)-1)
				;
			else
				qcc_token[len++] = c;
			data++;
			c = *data;
		} while ((c>= 'a' && c <= 'z') || (c>= 'A' && c <= 'Z') || (c>= '0' && c <= '9') || c == '_');

		qcc_token[len] = 0;
		return data;
	}
	else
	{
		qcc_token[len] = c;
		len++;
		qcc_token[len] = 0;
		return data+1;
	}
}

char *VARGS qcva (char *text, ...)
{
	va_list argptr;
	static char msg[2048];

	va_start (argptr,text);
	QC_vsnprintf (msg,sizeof(msg)-1, text,argptr);
	va_end (argptr);

	return msg;
}


#if !defined(MINIMAL) && !defined(OMIT_QCC)
/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/

/*
=================
Error

For abnormal program terminations
=================
*/
void VARGS QCC_Error (int errortype, const char *error, ...)
{
	progfuncs_t *progfuncs = qccprogfuncs;
	extern int numsourcefiles;
	va_list argptr;
	char msg[2048];

	va_start (argptr,error);
	QC_vsnprintf (msg,sizeof(msg)-1, error,argptr);
	va_end (argptr);

	externs->Printf ("\n************ ERROR ************\n%s\n", msg);


	editbadfile(s_filen, pr_source_line);

	numsourcefiles = 0;

#ifndef QCC
	longjmp(qcccompileerror, 1);
#else
	print ("Press any key\n");
	getch();
#endif
	exit (1);
}


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int QCC_CheckParm (const char *check)
{
	int i;

	for (i = 1;i<myargc;i++)
	{
		if ( !QC_strcasecmp(check, myargv[i]) )
			return i;
	}

	return 0;
}

const char *QCC_ReadParm (const char *check)
{
	int i;

	for (i = 1;i+1<myargc;i++)
	{
		if ( !QC_strcasecmp(check, myargv[i]) )
			return myargv[i+1];
	}

	return NULL;
}

/*


#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef QCC
int SafeOpenWrite (char *filename)
{
	int     handle;

	umask (0);

	handle = open(filename,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY
	, 0666);

	if (handle == -1)
		QCC_Error ("Error opening %s: %s",filename,strerror(errno));

	return handle;
}
#endif

int SafeOpenRead (char *filename)
{
	int     handle;

	handle = open(filename,O_RDONLY | O_BINARY);

	if (handle == -1)
		QCC_Error ("Error opening %s: %s",filename,strerror(errno));

	return handle;
}


void SafeRead (int handle, void *buffer, long count)
{
	if (read (handle,buffer,count) != count)
		QCC_Error ("File read failure");
}

#ifdef QCC
void SafeWrite (int handle, void *buffer, long count)
{
	if (write (handle,buffer,count) != count)
		QCC_Error ("File write failure");
}
#endif


void *SafeMalloc (long size)
{
	void *ptr;

	ptr = (void *)Hunk_Alloc (size);

	if (!ptr)
		QCC_Error ("Malloc failure for %lu bytes",size);

	return ptr;
}

*/



void DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}


void    StripFilename (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

/*
====================
Extract file parts
====================
*/
void ExtractFilePath (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileBase (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void ExtractFileExtension (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.')
		src--;
	if (src == path)
	{
		*dest = 0;	// no extension
		return;
	}

	strcpy (dest,src);
}


/*
==============
ParseNum / ParseHex
==============
*/
static long ParseHex (char *hex)
{
	char    *str;
	long    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			QCC_Error (ERR_BADHEX, "Bad hex number: %s",hex);
		str++;
	}

	return num;
}


long ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}








//buffer size and max size are different. buffer is bigger.

#define MAXQCCFILES 3
struct {
	char *name;
	FILE *stdio;
	char *buff;
	int buffsize;
	int ofs;
	int maxofs;
} qccfile[MAXQCCFILES];
int SafeOpenWrite (char *filename, int maxsize)
{
	int i;
	for (i = 0; i < MAXQCCFILES; i++)
	{
		if (!qccfile[i].stdio && !qccfile[i].buff)
		{
			qccfile[i].name = strdup(filename);
			qccfile[i].buffsize = maxsize;
			qccfile[i].maxofs = 0;
			qccfile[i].ofs = 0;
			qccfile[i].stdio = NULL;
			qccfile[i].buff = NULL;
			if (maxsize < 0)
				qccfile[i].stdio = fopen(filename, "wb");
			else
				qccfile[i].buff = malloc(qccfile[i].buffsize);
			if (!qccfile[i].stdio && !qccfile[i].buff)
			{
				QCC_Error(ERR_TOOMANYOPENFILES, "Unable to open %s", filename);
				return -1;
			}
			return i;
		}
	}
	QCC_Error(ERR_TOOMANYOPENFILES, "Too many open files on file %s", filename);
	return -1;
}

static void ResizeBuf(int hand, int newsize)
{
	char *nb;

	if (qccfile[hand].buffsize >= newsize)
		return;	//already big enough
	nb = malloc(newsize);

	memcpy(nb, qccfile[hand].buff, qccfile[hand].maxofs);
	free(qccfile[hand].buff);
	qccfile[hand].buff = nb;
	qccfile[hand].buffsize = newsize;
}
void SafeWrite(int hand, const void *buf, long count)
{
	if (qccfile[hand].stdio)
	{
		fwrite(buf, 1, count, qccfile[hand].stdio);
	}
	else
	{
		if (qccfile[hand].ofs +count >= qccfile[hand].buffsize)
			ResizeBuf(hand, qccfile[hand].ofs + count+(64*1024));

		memcpy(&qccfile[hand].buff[qccfile[hand].ofs], buf, count);
	}
	qccfile[hand].ofs+=count;
	if (qccfile[hand].ofs > qccfile[hand].maxofs)
		qccfile[hand].maxofs = qccfile[hand].ofs;
}
int SafeSeek(int hand, int ofs, int mode)
{
	if (mode == SEEK_CUR)
		return qccfile[hand].ofs;
	else
	{
		if (qccfile[hand].stdio)
			fseek(qccfile[hand].stdio, ofs, SEEK_SET);
		else
			ResizeBuf(hand, ofs+1024);
		qccfile[hand].ofs = ofs;
		if (qccfile[hand].ofs > qccfile[hand].maxofs)
			qccfile[hand].maxofs = qccfile[hand].ofs;
		return 0;
	}
}
pbool SafeClose(int hand)
{
	progfuncs_t *progfuncs = qccprogfuncs;
	pbool ret;
	if (qccfile[hand].stdio)
		ret = 0==fclose(qccfile[hand].stdio);
	else
	{
		ret = externs->WriteFile(qccfile[hand].name, qccfile[hand].buff, qccfile[hand].maxofs);
		free(qccfile[hand].buff);
	}
	free(qccfile[hand].name);
	qccfile[hand].name = NULL;
	qccfile[hand].buff = NULL;
	qccfile[hand].stdio = NULL;
	return ret;
}

qcc_cachedsourcefile_t *qcc_sourcefile;

//return 0 if the input is not valid utf-8.
unsigned int utf8_check(const void *in, unsigned int *value)
{
	//uc is the output unicode char
	unsigned int uc = 0xfffdu;	//replacement character
	const unsigned char *str = in;

	if (!(*str & 0x80))
	{
		*value = *str;
		return 1;
	}
	else if ((*str & 0xe0) == 0xc0)
	{
		if ((str[1] & 0xc0) == 0x80)
		{
			*value = uc = ((str[0] & 0x1f)<<6) | (str[1] & 0x3f);
			if (!uc || uc >= (1u<<7))	//allow modified utf-8 (only for nulls)
				return 2;
		}
	}
	else if ((*str & 0xf0) == 0xe0)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80)
		{
			*value = uc = ((str[0] & 0x0f)<<12) | ((str[1] & 0x3f)<<6) | ((str[2] & 0x3f)<<0);
			if (uc >= (1u<<11))
				return 3;
		}
	}
	else if ((*str & 0xf8) == 0xf0)
	{
		if ((str[1] & 0xc0) == 0x80 && (str[2] & 0xc0) == 0x80 && (str[3] & 0xc0) == 0x80)
		{
			*value = uc = ((str[0] & 0x07)<<18) | ((str[1] & 0x3f)<<12) | ((str[2] & 0x3f)<<6) | ((str[3] & 0x3f)<<0);
			if (uc >= (1u<<16))	//overlong
				if (uc <= 0x10ffff)	//aand we're not allowed to exceed utf-16 surrogates.
					return 4;
		}
	}
	*value = 0xFFFD;
	return 0;
}

//read utf-16 chars and output the 'native' utf-8.
//we don't expect essays written in code, so we don't need much actual support for utf-8.
static char *decodeUTF(int type, unsigned char *inputf, size_t inbytes, size_t *outlen, pbool usemalloc)
{
	char *utf8, *start;
	unsigned int inc;
	unsigned int chars, i;
	int w, maxperchar;
	switch(type)
	{
	case UTF16LE:
		w = 2;
		maxperchar = 4;
		break;
	case UTF16BE:
		w = 2;
		maxperchar = 4;
		break;
	case UTF32LE:
		w = 4;
		maxperchar = 4;	//we adhere to RFC3629 and clamp to U+10FFFF, which is only 4 bytes.
		break;
	case UTF32BE:
		w = 4;
		maxperchar = 4;
		break;
	default:
		//error
		*outlen = 0;
		return NULL;
	}
	chars = inbytes / w;
	if (usemalloc)
		utf8 = start = malloc(chars * maxperchar + 2+1);
	else
		utf8 = start = qccHunkAlloc(chars * maxperchar + 2+1);
	for (i = 0; i < chars; i++)
	{
		switch(type)
		{
		default:
			inc = 0;
			break;
		case UTF16LE:
		case UTF16BE:
			inc = inputf[type==UTF16BE];
			inc|= inputf[type==UTF16LE]<<8;
			inputf += 2;
			//handle surrogates
			if (inc >= 0xd800u && inc < 0xdc00u && i+1 < chars)
			{
				unsigned int l;

				l = inputf[type==UTF16BE];
				l|= inputf[type==UTF16LE]<<8;
				if (l >= 0xdc00u && l < 0xe000u)
				{
					inputf+=2;
					inc = (((inc & 0x3ffu)<<10) | (l & 0x3ffu)) + 0x10000;
					i++;
				}
			}
			break;
		case UTF32LE:
			inc = *inputf++;
			inc|= (*inputf++)<<8;
			inc|= (*inputf++)<<16;
			inc|= (*inputf++)<<24;
			break;
		case UTF32BE:
			inc = (*inputf++)<<24;
			inc|= (*inputf++)<<16;
			inc|= (*inputf++)<<8;
			inc|= *inputf++;
			break;
		}
		if (inc > 0x10FFFF)
			inc = 0xFFFD;

		if (inc <= 127)
			*utf8++ = inc;
		else if (inc <= 0x7ff)
		{
			*utf8++ = ((inc>>6) & 0x1f) | 0xc0;
			*utf8++ = ((inc>>0) & 0x3f) | 0x80;
		}
		else if (inc <= 0xffff)
		{
			*utf8++ = ((inc>>12) & 0xf) | 0xe0;
			*utf8++ = ((inc>>6) & 0x3f) | 0x80;
			*utf8++ = ((inc>>0) & 0x3f) | 0x80;
		}
		else if (inc <= 0x1fffff)
		{
			*utf8++ = ((inc>>18) & 0x07) | 0xf0;
			*utf8++ = ((inc>>12) & 0x3f) | 0x80;
			*utf8++ = ((inc>> 6) & 0x3f) | 0x80;
			*utf8++ = ((inc>> 0) & 0x3f) | 0x80;
		}
		else
		{
			inc = 0xFFFD;
			*utf8++ = ((inc>>12) & 0xf) | 0xe0;
			*utf8++ = ((inc>>6) & 0x3f) | 0x80;
			*utf8++ = ((inc>>0) & 0x3f) | 0x80;
		}
	}
	*outlen = utf8 - start;
	*utf8 = 0;
	return start;
}

//the gui is a windows program.
//this means that its fucked.
//on the plus side, its okay with a bom...
unsigned short *QCC_makeutf16(char *mem, size_t len, int *outlen, pbool *errors)
{
	unsigned int code;
	int l;
	unsigned short *out, *outstart;
	pbool nonascii = false;
	//sanitise the input.
	if (len >= 4 && mem[0] == '\xff' && mem[1] == '\xfe' && mem[2] == '\x00' && mem[3] == '\x00')
		mem = decodeUTF(UTF32LE, (unsigned char*)mem+4, len-4, &len, true);
	else if (len >= 4 && mem[0] == '\x00' && mem[1] == '\x00' && mem[2] == '\xfe' && mem[3] == '\xff')
		mem = decodeUTF(UTF32BE, (unsigned char*)mem+4, len-4, &len, true);
	else if (len >= 2 && mem[0] == '\xff' && mem[1] == '\xfe')
	{
		//already utf8, just return it as-is
		out = malloc(len+3);
		memcpy(out, mem, len);
		out[len/2] = 0;
		return out;
		//mem = decodeUTF(UTF16LE, (unsigned char*)mem+2, len-2, &len, false);
	}
	else if (len >= 2 && mem[0] == '\xfe' && mem[1] == '\xff')
		mem = decodeUTF(UTF16BE, (unsigned char*)mem+2, len-2, &len, false);
	//utf-8 BOM, for compat with broken text editors (like windows notepad).
	else if (len >= 3 && mem[0] == '\xef' && mem[1] == '\xbb' && mem[2] == '\xbf')
	{
		mem += 3;
		len -= 3;
	}

	outstart = malloc(len*2+3);
	out = outstart;
	while(len)
	{
		l = utf8_check(mem, &code);
		if (!l)
		{l = 1; code = 0xe000|(unsigned char)*mem; nonascii = true;}//fucked up. convert to 0xe000 private-use range.
		len -= l;
		mem += l;

		if (code > 0xffff)
		{
			code -= 0x10000;
			*out++ = 0xd800u | ((code>>10) & 0x3ff);
//			*out++ = 0xdc00u | ((code>>00) & 0x3ff);
		}
		else
			*out++ = code;
	}
	if (outlen)
		*outlen = out - outstart;
	*out++ = 0;

	if (errors)
		*errors = nonascii;
	return outstart;
}

//input is a raw file (will not be changed
//output is utf-8 data
char *QCC_SanitizeCharSet(char *mem, size_t *len, pbool *freeresult, int *origfmt)
{
	if (freeresult)
		*freeresult = true;
	if (*len >= 4 && mem[0] == '\xff' && mem[1] == '\xfe' && mem[2] == '\x00' && mem[3] == '\x00')
		mem = decodeUTF(*origfmt=UTF32LE, (unsigned char*)mem+4, *len-4, len, !!freeresult);
	else if (*len >= 4 && mem[0] == '\x00' && mem[1] == '\x00' && mem[2] == '\xfe' && mem[3] == '\xff')
		mem = decodeUTF(*origfmt=UTF32BE, (unsigned char*)mem+4, *len-4, len, !!freeresult);
	else if (*len >= 2 && mem[0] == '\xff' && mem[1] == '\xfe')
		mem = decodeUTF(*origfmt=UTF16LE, (unsigned char*)mem+2, *len-2, len, !!freeresult);
	else if (*len >= 2 && mem[0] == '\xfe' && mem[1] == '\xff')
		mem = decodeUTF(*origfmt=UTF16BE, (unsigned char*)mem+2, *len-2, len, !!freeresult);
	//utf-8 BOM, for compat with broken text editors (like windows notepad).
	else if (*len >= 3 && mem[0] == '\xef' && mem[1] == '\xbb' && mem[2] == '\xbf')
	{
		*origfmt=UTF8_BOM;
		mem += 3;
		*len -= 3;
		if (freeresult)
			*freeresult = false;
	}
	else
	{
		unsigned int ch, cl;
		char *p, *e=mem+*len;

		*origfmt=UTF8_RAW;
		for (p = mem; p < e; p += cl)	//validate it, so we're sure what format it actually is
		{
			cl = utf8_check(p, &ch);
			if (!cl)
			{
				*origfmt = UTF_ANSI;
				break;
			}
		}
		if (freeresult)
			*freeresult = false;
/*
#ifdef _WIN32
		//even if we wrote the bom, resaving with wordpad will translate the file to the system's active code page, which will fuck up any comments. thanks for that, wordpad.
		//(weirdly, notepad does the right thing)
		int wchars = MultiByteToWideChar(CP_ACP, 0, mem, *len, NULL, 0);

		if (wchars)
		{
			BOOL failed = false;
			wchar_t *wc = malloc(wchars * sizeof(wchar_t));
			int mchars;
			MultiByteToWideChar(CP_ACP, 0, mem, *len, wc, wchars);
			mchars = WideCharToMultiByte(CP_UTF8, 0, wc, wchars, NULL, 0, NULL, NULL);
			if (mchars && !failed)
			{
				mem = (freeresult?malloc(mchars+2):qccHunkAlloc(mchars+2));
				mem[mchars] = 0;
				*len = mchars;
				if (freeresult)
					*freeresult = true;
				WideCharToMultiByte(CP_UTF8, 0, wc, wchars, mem, mchars, NULL, NULL);
			}
			free(wc);
		}
#endif
*/
	}


	return mem;
}

static unsigned char *PDECL QCC_LoadFileHunk(void *ctx, size_t size)
{	//2 ensures we can always put a \n in there.
	return (unsigned char*)qccHunkAlloc(sizeof(qcc_cachedsourcefile_t)+strlen(ctx)+size+2) + sizeof(qcc_cachedsourcefile_t) + strlen(ctx);
}

long	QCC_LoadFile (char *filename, void **bufferptr)
{
	qcc_cachedsourcefile_t *sfile;
	progfuncs_t *progfuncs = qccprogfuncs;
	char *mem;
	int check;
	size_t len;
	int line;
	int orig;
	pbool warned = false;

	mem = externs->ReadFile(filename, QCC_LoadFileHunk, filename, &len, true);
	if (!mem)
	{
		QCC_Error(ERR_COULDNTOPENFILE, "Couldn't open file %s", filename);
		return -1;
	}
	sfile = (qcc_cachedsourcefile_t*)(mem-sizeof(qcc_cachedsourcefile_t)-strlen(filename));
	mem[len] = 0;

	mem = QCC_SanitizeCharSet(mem, &len, NULL, &orig);

	//actual utf-8 handling is somewhat up to the engine. the qcc can only ensure that utf8 works in symbol names etc.
	//its only in strings where it actually makes a difference, and the interpretation of those is basically entirely up to the engine.
	//that said, we could insert a utf-8 BOM into ones with utf-8 chars, but that would mess up a lot of builtins+mods, so we won't.

	for (check = 0, line = 1; check < len; check++)
	{
		if (mem[check] == '\n')
			line++;
		else if (!mem[check])
		{
			if (!warned)
				QCC_PR_Warning(WARN_UNEXPECTEDPUNCT, filename, line, "file contains null bytes %u/%"pPRIuSIZE, check, len);
			warned = true;
			//fixme: insert modified-utf-8 nulls instead.
			mem[check] = ' ';
		}
	}

	mem[len] = '\n';
	mem[len+1] = '\0';

	strcpy(sfile->filename, filename);
	sfile->size = len;
	sfile->file = mem;
	sfile->type = FT_CODE;
	sfile->next = qcc_sourcefile;
	qcc_sourcefile = sfile;

	*bufferptr=mem;

	return len;
}
void	QCC_AddFile (char *filename)
{
	qcc_cachedsourcefile_t *sfile;
	progfuncs_t *progfuncs = qccprogfuncs;
	char *mem;
	size_t len;

	mem = externs->ReadFile(filename, QCC_LoadFileHunk, filename, &len, false);
	if (!mem)
		externs->Abort("failed to find file %s", filename);
	sfile = (qcc_cachedsourcefile_t*)(mem-sizeof(qcc_cachedsourcefile_t)-strlen(filename));
	mem[len] = '\0';

	sfile->size = len;
	strcpy(sfile->filename, filename);
	sfile->file = mem;
	sfile->type = FT_DATA;
	sfile->next = qcc_sourcefile;
	qcc_sourcefile = sfile;

}
static unsigned char *PDECL FS_ReadToMem_Alloc(void *ctx, size_t size)
{
	unsigned char *mem;
	progfuncs_t *progfuncs = qccprogfuncs;
	mem = externs->memalloc(size+1);
	mem[size] = 0;
	return mem;
}
void *FS_ReadToMem(char *filename, size_t *len)
{
	progfuncs_t *progfuncs = qccprogfuncs;
	return externs->ReadFile(filename, FS_ReadToMem_Alloc, NULL, len, false);
}

void FS_CloseFromMem(void *mem)
{
	progfuncs_t *progfuncs = qccprogfuncs;
	externs->memfree(mem);
}


#endif

void    StripExtension (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}
