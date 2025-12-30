// Misc stuff to transition from ZDoom to Wolf

#ifndef __ZDOOM_SUPPORT__
#define __ZDOOM_SUPPORT__

#include "m_crc32.h"
#include "templates.h"

#include <cstring>

#if defined(LIBRETRO) && LIBRETRO
void libretro_log(const char *format, ...);
#define printf(...) libretro_log(__VA_ARGS__)
#elif defined(__ANDROID__) && !defined(_DIII4A) //karin: using std printf
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO,"ECWolf",__VA_ARGS__)
#endif

int ParseHex(const char* hex);

// Ensures that a double is a whole number or a half
static inline bool CheckTicsValid(double tics)
{
	double ipart;
	double fpart = modf(tics, &ipart);
	if(MIN(fabs(fpart), fabs(0.5 - fpart)) > 0.0001)
		return false;
	return true;
}

static inline char* copystring(const char* src)
{
	char *dest = new char[strlen(src)+1];
	strcpy(dest, src);
	dest[strlen(src)] = 0;
	return dest;
}

static inline void ReplaceString(char* &ptr, const char* str)
{
	if(ptr)
	{
		if(ptr == str)
			return;
		delete[] ptr;
	}
	ptr = copystring(str);
}

static inline unsigned int MakeKey(const char *s, size_t len)
{
	BYTE* hashString = new BYTE[len];
	memcpy(hashString, s, len);
	for(size_t i = 0;i < len;++i)
		hashString[i] = tolower(*s++);
	const DWORD ret = CalcCRC32(hashString, static_cast<unsigned int>(len));
	delete[] hashString;
	return ret;
}
static inline unsigned int MakeKey(const char *s) { return MakeKey(s, strlen(s)); }

// Technically T should always be FString, but we use a template to avoid a
// circular dependency issue.
template<class T> void FixPathSeperator (T &path) { path.ReplaceChars('\\', '/'); }

static inline void DPrintf(const char* fmt, ...) {}

#define countof(x) (sizeof(x)/sizeof(x[0]))
#ifndef __BIG_ENDIAN__
#define MAKE_ID(a,b,c,d)	((DWORD)((a)|((b)<<8)|((c)<<16)|((d)<<24)))
#else
#define MAKE_ID(a,b,c,d)	((DWORD)((d)|((c)<<8)|((b)<<16)|((a)<<24)))
#endif

#define MAXWIDTH 5120
#define Printf printf

#define MulScale16(x,y) (SDWORD((SQWORD(x)*SQWORD(y))>>16))

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define STACK_ARGS __cdecl
#else
#define STACK_ARGS
#endif

#endif /* __ZDOOM_SUPPORT__ */
