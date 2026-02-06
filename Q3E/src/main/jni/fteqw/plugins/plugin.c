//contains generic plugin code for dll/qvm
//it's this one or the engine...
#include "plugin.h"
#include <stdarg.h>
#include <stdio.h>


plugcorefuncs_t *plugfuncs;
plugcmdfuncs_t *cmdfuncs;
plugcvarfuncs_t *cvarfuncs;
//plugclientfuncs_t *clientfuncs;




/* An implementation of some 'standard' functions */
void Q_strlncpy(char *d, const char *s, int sizeofd, int lenofs)
{
	int i;
	sizeofd--;
	if (sizeofd < 0)
		return;	//this could be an error

	for (i=0; lenofs-- > 0; i++)
	{
		if (i == sizeofd)
			break;
		*d++ = *s++;
	}
	*d='\0';
}
void Q_strlcpy(char *d, const char *s, int n)
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
void Q_strlcat(char *d, const char *s, int n)
{
	if (n)
	{
		int dlen = strlen(d);
		int slen = strlen(s)+1;
		if (slen > (n-1)-dlen)
			slen = (n-1)-dlen;
		memcpy(d+dlen, s, slen);
		d[n - 1] = 0;
	}
}

char *Plug_Info_ValueForKey (const char *s, const char *key, char *out, size_t outsize)
{
	int isvalue = 0;
	const char *start;
	char *oout = out;
	*out = 0;
	if (*s != '\\')
		return out;	//gah, get lost with your corrupt infostrings.

	start = ++s;
	while(1)
	{
		while(s[0] == '\\' && s[1] == '\\')
			s+=2;
		if (s[0] != '\\' && *s)
		{
			s++;
			continue;
		}

		//okay, it terminates here
		isvalue = !isvalue;
		if (isvalue)
		{
			if (strlen(key) == (size_t)(s - start) && !strncmp(start, key, s - start))
			{
				s++;
				while (outsize --> 1)
				{
					if (s[0] == '\\' && s[1] == '\\')
						s++;
					else if (s[0] == '\\' || !s[0])
						break;
					*out++ = *s++;
				}
				*out++ = 0;
				return oout;
			}
		}
		if (*s)
			start = ++s;
		else
			break;
	}
	return oout;
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
		plugfuncs->Error("Q_vsnprintfz: Truncation\n");
#endif
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return (ret>=size) ? qtrue : qfalse;
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
		plugfuncs->Error("Q_vsnprintfz: Truncation\n");
#endif
	//if ret is -1 (windows oversize, or general error) then it'll be treated as unsigned so really long. this makes the following check quite simple.
	return (ret>=size) ? qtrue : qfalse;
}

char	*va(const char *format, ...)	//Identical in function to the one in Quake, though I can assure you that I wrote it...
{					//It's not exactly hard, just easy to use, so gets duplicated lots.
	va_list		argptr;
	static char		string[1024];
		
	va_start (argptr, format);
	Q_vsnprintfz (string, sizeof(string), format,argptr);
	va_end (argptr);

	return string;	
}

#ifdef _WIN32
// don't use these functions in MSVC8
#if (_MSC_VER < 1400)
int QDECL linuxlike_snprintf(char *buffer, int size, const char *format, ...)
{
#undef _vsnprintf
	int ret;
	va_list		argptr;

	if (size <= 0)
		return 0;
	size--;

	va_start (argptr, format);
	ret = _vsnprintf (buffer,size, format,argptr);
	va_end (argptr);

	buffer[size] = '\0';

	return ret;
}
int QDECL linuxlike_vsnprintf(char *buffer, int size, const char *format, va_list argptr)
{
#undef _vsnprintf
	int ret;

	if (size <= 0)
		return 0;
	size--;

	ret = _vsnprintf (buffer,size, format,argptr);

	buffer[size] = '\0';

	return ret;
}
#elif (_MSC_VER < 1900)
int VARGS linuxlike_snprintf_vc8(char *buffer, int size, const char *format, ...)
{
	int ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = vsnprintf_s (buffer,size, _TRUNCATE, format,argptr);
	va_end (argptr);

	return ret;
}
#endif
#endif

void Con_Printf(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
		
	va_start (argptr, format);
	Q_vsnprintfz (string, sizeof(string), format,argptr);
	va_end (argptr);

	plugfuncs->Print(string);
}
void Con_DPrintf(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];

	if (!cvarfuncs->GetFloat("developer"))
		return;
		
	va_start (argptr, format);
	Q_vsnprintfz (string, sizeof(string), format,argptr);
	va_end (argptr);

	plugfuncs->Print(string);
}
void Sys_Errorf(const char *format, ...)
{
	va_list		argptr;
	static char		string[1024];
		
	va_start (argptr, format);
	Q_vsnprintfz (string, sizeof(string), format,argptr);
	va_end (argptr);

	plugfuncs->Error(string);
}


qboolean ZF_ReallocElements(void **ptr, size_t *elements, size_t newelements, size_t elementsize)
{
	void *n;
	size_t oldsize;
	size_t newsize;

	//protect against malicious overflows
	if (newelements > SIZE_MAX / elementsize)
#ifdef __cplusplus
		return qfalse;
#else
		return false;
#endif

	oldsize = *elements * elementsize;
	newsize = newelements * elementsize;

	n = plugfuncs->Realloc(*ptr, newsize);
	if (!n)
#ifdef __cplusplus
		return qfalse;
#else
		return false;
#endif
	if (newsize > oldsize)
		memset((char*)n+oldsize, 0, newsize - oldsize);
	*elements = newelements;
	*ptr = n;
#ifdef __cplusplus
		return qtrue;
#else
		return true;
#endif
}

// begin common.c
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
// end common.c

#ifdef __cplusplus
extern "C"
#endif
qboolean NATIVEEXPORT FTEPlug_Init(plugcorefuncs_t *corefuncs)
{
	plugfuncs = corefuncs;
	cmdfuncs = (plugcmdfuncs_t*)plugfuncs->GetEngineInterface(plugcmdfuncs_name, sizeof(*cmdfuncs));
	cvarfuncs = (plugcvarfuncs_t*)plugfuncs->GetEngineInterface(plugcvarfuncs_name, sizeof(*cvarfuncs));
	if (!plugfuncs || !cmdfuncs || !cvarfuncs)
		return qfalse; //erk

	return Plug_Init();
}
