#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOWINRES
	#define NOWINRES
#endif
#ifndef NOSERVICE
	#define NOSERVICE
#endif
#ifndef NOMCX
	#define NOMCX
#endif
#ifndef NOIME
	#define NOIME
#endif
#include <windows.h>
#include <assert.h>

double Prof_get_time(void)
{
   LARGE_INTEGER freq;
   LARGE_INTEGER time;

   BOOL ok = QueryPerformanceFrequency(&freq);
   assert(ok == TRUE);

   freq.QuadPart = freq.QuadPart;

   ok = QueryPerformanceCounter(&time);
   assert(ok == TRUE);

   return time.QuadPart / (double) freq.QuadPart;
}

#endif
