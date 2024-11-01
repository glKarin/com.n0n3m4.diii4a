//  See Copyright Notice in gmMachine.h
#ifndef _gmtimer_h
#define _gmtimer_h

#ifdef WIN32
#include <windows.h>	
#undef min
#undef max
#else
#include <time.h>
#endif

class gmTimer
{
protected:
#ifdef WIN32
	LARGE_INTEGER iCounterFrequency;
	LARGE_INTEGER iLastCount;
#else
	clock_t iLastCount;
#endif

public:
	gmTimer()
	{
#ifdef WIN32
		QueryPerformanceFrequency(&iCounterFrequency);
		QueryPerformanceCounter(&iLastCount);
#else
		iLastCount = clock();
#endif
	}

	/*
	Function: Reset
	Description: Resets the timer to 0	
	*/
	inline void Reset()
	{
#ifdef WIN32
		QueryPerformanceCounter(&iLastCount);
#else
		iLastCount = clock();
#endif
	}
	/*
	Function: GetElapsedSeconds
	Description:		
	Returns: 
	Returns the elapsed seconds since the timer was last reset.
	*/
	double GetElapsedSeconds()
	{
#ifdef WIN32
		// Get the current count
		LARGE_INTEGER iCurrent;
		QueryPerformanceCounter(&iCurrent);

		return double((iCurrent.QuadPart - iLastCount.QuadPart) /
			double(iCounterFrequency.QuadPart));
#else
		return static_cast<double>(clock() - iLastCount) / CLOCKS_PER_SEC;
#endif
	}	
};

#endif //_timer_h
