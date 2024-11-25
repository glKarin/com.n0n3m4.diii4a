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
#endif

static LARGE_INTEGER		perfCounter;
static LARGE_INTEGER		perfFrequency;
static bool					timerInitialized = false;

void gmPlatformTimerStart()
{
	if(!timerInitialized)
	{
		timerInitialized = true;
		QueryPerformanceFrequency(&perfFrequency);
	}

	QueryPerformanceCounter(&perfCounter);
}

double gmPlatformTimerGetElapsedSecs()
{
	LARGE_INTEGER perfCurrent;
	QueryPerformanceCounter(&perfCurrent);

	return double((perfCurrent.QuadPart - perfCounter.QuadPart) /
		double(perfFrequency.QuadPart));
}
