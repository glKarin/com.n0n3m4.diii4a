/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "Engine/StdH.h"

#include <Engine/Base/Timer.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Translation.h>
#include <Engine/Base/ThreadLocalStorage.h>  //rcg10242001

#include <Engine/Base/Registry.h>
#include <Engine/Base/Profiling.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Statistics_Internal.h>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/Priority.inl>

#if defined PLATFORM_UNIX
#include <sys/time.h>
#include <pthread.h>
#endif

#if PLATFORM_FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef _DIII4A //karin: no SDL
#include "Timer_diii4a.cpp"
#else

// Read the Pentium TimeStampCounter (or something like that).
static inline __int64 ReadTSC(void)
{
#if PLATFORM_NOT_X86 || NOT_USE_ASM
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return( (((__int64) tp.tv_sec) * 1000000000LL) + ((__int64) tp.tv_nsec));
#elif (defined __MSVC_INLINE__) && (defined  PLATFORM_32BIT)
  __int64 mmRet;
  __asm {
    rdtsc
    mov   dword ptr [mmRet+0],eax
    mov   dword ptr [mmRet+4],edx
  }
  return mmRet;

#elif (defined _MSC_VER) && (defined  PLATFORM_64BIT)
	unsigned __int64 i;
	i = __rdtsc();

	return i;

#elif (defined __GNU_INLINE_X86_32__ )	
  __int64 mmRet;
  __asm__ __volatile__ (
    "rdtsc                    \n\t"
    "movl   %%eax, 0(%%esi)   \n\t"
    "movl   %%edx, 4(%%esi)   \n\t"
        :
        : "S" (&mmRet)
        : "memory", "eax", "edx"
  );
  return(mmRet);
#elif (defined __GNU_INLINE_X86_64__ )
    uint64_t x;
    asm volatile (
        ".intel_syntax noprefix  \n\t" // switch to prettier syntax
        // 'If software requires RDTSC to be executed only after all previous
        // instructions have executed and all previous loads and stores are
        // globally visible, it can execute the sequence MFENCE;LFENCE
        // immediately before RDTSC.'
        // https://www.felixcloutier.com/x86/rdtsc
        "mfence                  \n\t"
        "lfence                  \n\t"
        // similar effect, execute CPUID before RDTSC
        // cf. https://www.intel.de/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
        //"cpuid                   \n\t" // writes to EAX, EBX, ECX, EDX
        "rdtsc                   \n\t" // counter into EDX:EAX
        "shl     rdx, 0x20       \n\t" // shift higher-half left
        "or      rax, rdx        \n\t" // combine them
        ".att_syntax prefix      \n\t" // switch back to the default syntax


        : "=a" (x)       // output operands,
                         // i.e. overwrites (=)  R'a'X which is mapped to x
        :                // input operands
        : "rdx");        // additional clobbers (with cpuid also: rbx, rcx)
    return x;
#else
  #error Please implement for your platform/compiler.
#endif
}

//
// We will use several functions from libcpuid to get TSC
// https://github.com/anrieff/libcpuid
//

/**
 * @brief Internal structure, used in cpu_tsc_mark, cpu_tsc_unmark and
 *        cpu_clock_by_mark
 */
struct cpu_mark_t {
	uint64_t tsc;		/*!< Time-stamp from RDTSC */
	uint64_t sys_clock;	/*!< In microsecond resolution */
};

#ifdef PLATFORM_WIN32
#include <stdint.h> // portable: uint64_t   MSVC: __int64 
// MSVC defines this in winsock2.h!?
//typedef struct timeval {
//	long tv_sec;
//	long tv_usec;
//} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}
#endif

// sys_precise_clock
void sys_precise_clock(uint64_t *result)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*result = (uint64_t) tv.tv_sec * (uint64_t) 1000000 +
	          (uint64_t) tv.tv_usec;
}

#if !PLATFORM_NOT_X86 && !NOT_USE_ASM
// cpu_rdtsc
void cpu_rdtsc(uint64_t* result)
{
	uint32_t low_part, hi_part;
#if defined( __GNU_INLINE_X86_32__ ) || defined( __GNU_INLINE_X86_64__ )
	__asm __volatile (
		"	rdtsc\n"
		"	mov	%%eax,	%0\n"
		"	mov	%%edx,	%1\n"
		:"=m"(low_part), "=m"(hi_part)::"memory", "eax", "edx"
	);
#elif (defined __MSVC_INLINE__) || defined( PLATFORM_32BIT )
    low_part = 0;
    hi_part = 0;

	__asm {
		rdtsc
		mov	low_part,	eax
		mov	hi_part,	edx
	};
#elif (defined _MSC_VER) && (defined  PLATFORM_64BIT)
	unsigned __int64 i;
	i = __rdtsc();
	*result = i;
	return;
#else
#    error "Unsupported compiler"
#endif
	*result = (uint64_t)low_part + (((uint64_t) hi_part) << 32);
}

// cpu_tsc_mark
void cpu_tsc_mark(struct cpu_mark_t* mark)
{
	cpu_rdtsc(&mark->tsc);
	sys_precise_clock(&mark->sys_clock);
}

// cpu_clock_by_mark
int cpu_clock_by_mark(struct cpu_mark_t* mark)
{
	uint64_t result;

	/* Check if some subtraction resulted in a negative number: */
	if ((mark->tsc >> 63) != 0 || (mark->sys_clock >> 63) != 0) return -1;

	/* Divide-by-zero check: */
	if (mark->sys_clock == 0) return -1;

	/* Check if the result fits in 32bits */
	result = mark->tsc / mark->sys_clock;
	if (result > (uint64_t) 0x7fffffff) return -1;
	return (int) result;
}


/* out = a - b */
static void mark_t_subtract(struct cpu_mark_t* a, struct cpu_mark_t* b, struct cpu_mark_t *out)
{
	out->tsc = a->tsc - b->tsc;
	out->sys_clock = a->sys_clock - b->sys_clock;
}

/* Emulate doing useful CPU intensive work */
static int busy_loop(int amount)
{
	int i, j, k, s = 0;
	static volatile int data[42] = {32, 12, -1, 5, 23, 0 };
	for (i = 0; i < amount; i++)
		for (j = 0; j < 65536; j++)
			for (k = 0; k < 42; k++)
				s += data[k];
	return s;
}

// cpu_clock_measure
int cpu_clock_measure(int millis, int quad_check)
{
	struct cpu_mark_t begin[4], end[4], temp, temp2;
	int results[4], cycles, n, k, i, j, bi, bj, mdiff, diff, _zero = 0;
	uint64_t tl;

	if (millis < 1) return -1;
	tl = millis * (uint64_t) 1000;
	if (quad_check)
		tl /= 4;
	n = quad_check ? 4 : 1;
	cycles = 1;
	for (k = 0; k < n; k++) {
		cpu_tsc_mark(&begin[k]);
		end[k] = begin[k];
		do {
			/* Run busy loop, and fool the compiler that we USE the garbishy
			   value it calculates */
			_zero |= (1 & busy_loop(cycles));
			cpu_tsc_mark(&temp);
			mark_t_subtract(&temp, &end[k], &temp2);
			/* If busy loop is too short, increase it */
			if (temp2.sys_clock < tl / 8)
				cycles *= 2;
			end[k] = temp;
		} while (end[k].sys_clock - begin[k].sys_clock < tl);
		mark_t_subtract(&end[k], &begin[k], &temp);
		results[k] = cpu_clock_by_mark(&temp);
	}
	if (n == 1) return results[0];
	mdiff = 0x7fffffff;
	bi = bj = -1;
	for (i = 0; i < 4; i++) {
		for (j = i + 1; j < 4; j++) {
			diff = results[i] - results[j];
			if (diff < 0) diff = -diff;
			if (diff < mdiff) {
				mdiff = diff;
				bi = i;
				bj = j;
			}
		}
	}
    ASSERT(1 == 1);
	if (results[bi] == -1) return -1;
	return (results[bi] + results[bj] + _zero) / 2;
}

//
// END libcpuid functions
//
#endif // not PLATFORM_NOT_X86

// link with Win-MultiMedia
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

// current game time always valid for the currently active task
#ifdef _MSC_VER
static _declspec(thread) TIME _CurrentTickTimer = 0.0f;
#else
THREADLOCAL(TIME, _CurrentTickTimer, 0.0f);
#endif

// CTimer implementation

// pointer to global timer object
CTimer *_pTimer = NULL;

#ifdef PLARFORM_UNIX
pthread_t g_timerMainThread;
#endif

const TIME CTimer::TickQuantum = TIME(1/20.0);    // 20 ticks per second

/*
 * Timer interrupt callback function.
 */
/*
  NOTE:
  This function is a bit more complicated than it could be, because
  it has to deal with a feature in the windows multimedia timer that
  is undesired here.
  That is the fact that, if the timer function is stalled for a while,
  because some other thread or itself took too much time, the timer function
  is called more times to catch up with the hardware clock.
  This can cause complete lockout if timer handlers constantly consume more
  time than is available between two calls of timer function.
  As a workaround, this function measures hardware time and refuses to call
  the handlers if it is not on time.

  In effect, if some timer handler starts spending too much time, the
  handlers are called at lower frequency until the application (hopefully)
  stabilizes.

  When such a catch-up situation occurs, 'real time' timer still keeps
  more or less up to date with the hardware time, but the timer handlers
  skip some ticks. E.g. if timer handlers start spending twice more time
  than is tick quantum, they get called approx. every two ticks.

  EXTRA NOTE:
  Had to disable that, because it didn't work well (caused jerking) on 
  Win95 osr2 with no patches installed!
*/
void CTimer_TimerFunc_internal(void)
{
// Access to stream operations might be invoked in timer handlers, but
// this is disabled for now. Should also synchronize access to list of
// streams and to group file before enabling that!
//  CTSTREAM_BEGIN {

#ifdef SINGLE_THREADED

    // rcg10272001 experimenting here...
    static CTimerValue highResQuantum((double) _pTimer->TickQuantum);

    CTimerValue upkeep = _pTimer->GetHighPrecisionTimer() - _pTimer->tm_InitialTimerUpkeep;
    TIME t = upkeep.GetSeconds();

    if (t < _pTimer->TickQuantum)  // not time to do an update, yet.
        return;

    while (t >= _pTimer->TickQuantum) {
      _pTimer->tm_InitialTimerUpkeep += highResQuantum;
      _pTimer->tm_RealTimeTimer += _pTimer->TickQuantum;
      t -= _pTimer->TickQuantum;
    }

#else

    // increment the 'real time' timer
    _pTimer->tm_RealTimeTimer += _pTimer->TickQuantum;

#endif

    // get the current time for real and in ticks
    CTimerValue tvTimeNow = _pTimer->GetHighPrecisionTimer();
    TIME        tmTickNow = _pTimer->tm_RealTimeTimer;
    // calculate how long has passed since we have last been on time
    TIME tmTimeDelay = (TIME)(tvTimeNow - _pTimer->tm_tvLastTimeOnTime).GetSeconds();
    TIME tmTickDelay =       (tmTickNow - _pTimer->tm_tmLastTickOnTime);

    _sfStats.StartTimer(CStatForm::STI_TIMER);
    // if we are keeping up to time (more or less)
//    if (tmTimeDelay>=_pTimer->TickQuantum*0.9f) {

      // for all hooked handlers
      FOREACHINLIST(CTimerHandler, th_Node, _pTimer->tm_lhHooks, itth) {
        // handle
        itth->HandleTimer();
      }
//    }
    _sfStats.StopTimer(CStatForm::STI_TIMER);

    // remember that we have been on time now
    _pTimer->tm_tvLastTimeOnTime = tvTimeNow;
    _pTimer->tm_tmLastTickOnTime = tmTickNow;

    _pTimer->tm_tvLowPrecisionTimer = tvTimeNow;

//  } CTSTREAM_END;
}

// !!! FIXME : rcg10192001 Abstract this!
#if (!defined SINGLE_THREADED)
#ifdef PLATFORM_WIN32
void __stdcall CTimer_TimerFunc(UINT uID, UINT uMsg, ULONG dwUser, ULONG dw1, ULONG dw2)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // handle all timers
  CTimer_TimerFunc_internal();
}
#elif (defined PLATFORM_UNIX)

#include "SDL.h"
Uint32 CTimer_TimerFunc_SDL(Uint32 interval, void* param)
{
  (void)param;
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // handle all timers
  CTimer_TimerFunc_internal();
  return(interval);
}
/*
void *CTimer_TimerMain(void *input) {
  while(true) {
    // sleep
    usleep(ULONG(_pTimer->TickQuantum * 1000.0f * 1000.0f)); // microsecond

    // TODO: unsynch
    if (_pTimer->tm_bPaused) {
      usleep(50000);
      continue;
    }

    // access to the list of handlers must be locked
    CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
    // handle all timers
    CTimer_TimerFunc_internal();
  }
}*/

#endif
#endif

#pragma inline_depth()


#ifdef PLATFORM_WIN32 // DG: not used on other platforms
#define MAX_MEASURE_TRIES 5
static INDEX _aiTries[MAX_MEASURE_TRIES];

// Get processor speed in Hertz
static __int64 GetCPUSpeedHz(void)
{
  // get the frequency of the 'high' precision timer
  __int64 llTimerFrequency;
  BOOL bPerformanceCounterPresent = QueryPerformanceFrequency((LARGE_INTEGER*)&llTimerFrequency);
  // fail if the performance counter is not available on this system
  if( !bPerformanceCounterPresent) {
    CPrintF( TRANS("PerformanceTimer is not available!\n"));
    return 1;
  }

  INDEX iSpeed, iTry;
  INDEX ctTotalFaults=0;
  __int64 llTimeLast, llTimeNow;
  __int64 llCPUBefore, llCPUAfter; 
  __int64 llTimeBefore, llTimeAfter;
  __int64 llSpeedMeasured;

  // try to measure 10 times
  INDEX iSet = 0;
  for (; iSet<10; iSet++)
  { // one time has several tries
    for( iTry=0; iTry<MAX_MEASURE_TRIES; iTry++)
    { // wait the state change on the timer
      QueryPerformanceCounter((LARGE_INTEGER*)&llTimeNow);
      do {
        llTimeLast = llTimeNow;
        QueryPerformanceCounter((LARGE_INTEGER*)&llTimeNow);
      } while( llTimeLast==llTimeNow);
      // wait for some time, and count the CPU clocks passed
      llCPUBefore  = ReadTSC();
      llTimeBefore = llTimeNow;
      llTimeAfter  = llTimeNow + llTimerFrequency/4;
      do {
        QueryPerformanceCounter((LARGE_INTEGER*)&llTimeNow);
      } while( llTimeNow<llTimeAfter );
      llCPUAfter = ReadTSC();
      // calculate the CPU clock frequency from gathered data
      llSpeedMeasured = (llCPUAfter-llCPUBefore)*llTimerFrequency / (llTimeNow-llTimeBefore);
      _aiTries[iTry]  = llSpeedMeasured/1000000;
    }
    // see if we had good measurement
    INDEX ctFaults = 0;
    iSpeed = _aiTries[0];
    const INDEX iTolerance = iSpeed *1/100; // %1 tolerance should be enough
    for( iTry=1; iTry<MAX_MEASURE_TRIES; iTry++) {
      if( abs(iSpeed-_aiTries[iTry]) > iTolerance) ctFaults++;
    }
    // done if no faults
    if( ctFaults==0) break;
    Sleep(1000);
  }

  // fail if couldn't readout CPU speed
  if( iSet==10) {
    CPrintF( TRANS("PerformanceTimer is not vaild!\n"));
    //return 1; 
    // NOTE: this function must never fail, or the engine will crash! 
    // if this failed, the speed will be read from registry (only happens on Win2k)
  }

  // keep readout speed and read speed from registry
  const SLONG slSpeedRead = _aiTries[0];
  SLONG slSpeedReg = 0;
  BOOL bFoundInReg = REG_GetLong("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\\~MHz", (ULONG&)slSpeedReg);

  // if not found in registry
  if( !bFoundInReg) {
    // use measured
    CPrintF(TRANSV("  CPU speed not found in registry, using calculated value\n\n"));
    return (__int64)slSpeedRead*1000000;
  // if found in registry
  } else {
    // if different than measured
    const INDEX iTolerance = slSpeedRead *1/100; // %1 tolerance should be enough
    if( abs(slSpeedRead-slSpeedReg) > iTolerance) {
      // report warning and use registry value
      CPrintF(TRANSV("  WARNING: calculated CPU speed different than stored in registry!\n\n"));
      return (__int64)slSpeedReg*1000000;
    }
    // use measured value
    return (__int64)slSpeedRead*1000000;
  }
}
#endif // PLATFORM_WIN32

/*
#if PLATFORM_MACOSX
extern "C" { signed int GetCPUSpeed(void); }  // carbon function, avoid header.
#endif
*/
/*
 * Constructor.
 */
CTimer::CTimer(BOOL bInterrupt /*=TRUE*/)
{
#if (defined SINGLE_THREADED)
  bInterrupt = FALSE;
#endif

  tm_csHooks.cs_iIndex = 1000;
  // set global pointer
  ASSERT(_pTimer == NULL);
  _pTimer = this;
  tm_bInterrupt = bInterrupt;

#if PLATFORM_NOT_X86 || NOT_USE_ASM
  // just use clock_gettime.
  tm_llCPUSpeedHZ = tm_llPerformanceCounterFrequency = 1000000000LL;
#elif defined(PLATFORM_WIN32)
  { // this part of code must be executed as precisely as possible
    CSetPriority sp(REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL);
    tm_llCPUSpeedHZ = GetCPUSpeedHz();
    tm_llPerformanceCounterFrequency = tm_llCPUSpeedHZ;

    // measure profiling errors and set epsilon corrections
    CProfileForm::CalibrateProfilingTimers();
  }
/*
#elif PLATFORM_MACOSX
  tm_llPerformanceCounterFrequency = tm_llCPUSpeedHZ = ((__int64) GetCPUSpeed()) * 1000000;

#elif PLATFORM_FREEBSD
  __int64 mhz = 0;
  size_t len = sizeof(mhz);

  sysctlbyname("hw.clockrate", &mhz, &len, NULL, 0);
  tm_llPerformanceCounterFrequency = tm_llCPUSpeedHZ = (__int64) (mhz * 1000000);
*/
#else
  int mhz;
  // Here we use the RDTSC-based routine
  mhz = cpu_clock_measure(400,1);
  tm_llPerformanceCounterFrequency = tm_llCPUSpeedHZ = mhz * 1000000LL;

#endif

  // clear counters
  _CurrentTickTimer = TIME(0);
  tm_RealTimeTimer = TIME(0);

  tm_tmLastTickOnTime = TIME(0);
  tm_tvLastTimeOnTime = GetHighPrecisionTimer();
  // disable lerping by default
  tm_fLerpFactor = 1.0f;
  tm_fLerpFactor2 = 1.0f;

  // start interrupt (eventually)
#if (defined SINGLE_THREADED)

  tm_InitialTimerUpkeep = GetHighPrecisionTimer();

#else

  if( tm_bInterrupt)
  {

   // !!! FIXME : rcg10192001 Abstract this!
   #ifdef PLATFORM_WIN32

    tm_TimerID = timeSetEvent(
      ULONG(TickQuantum*1000.0f),	  // period value [ms]
      0,	                          // resolution (0==max. possible)
      (LPTIMECALLBACK)&CTimer_TimerFunc,	          // callback
      0,                            // user
      TIME_PERIODIC);               // event type

    // check that interrupt was properly started
    if( tm_TimerID==NULL) FatalError(TRANS("Cannot initialize multimedia timer!"));

   #else

    if (SDL_Init(SDL_INIT_TIMER) == -1) FatalError(TRANS("Cannot initialize multimedia timer!"));
    tm_TimerID = SDL_AddTimer(ULONG(TickQuantum*1000.0f), CTimer_TimerFunc_SDL, NULL);
   #ifdef PLATFORM_FREEBSD
    if( tm_TimerID==0x00) FatalError(TRANS("Cannot initialize multimedia timer!"));
   #else
    if( tm_TimerID==0x00) FatalError(TRANS("Cannot initialize multimedia timer!"));
   #endif
   #endif

    // make sure that timer interrupt is ticking
    INDEX iTry;
    for(iTry=1; iTry<=3; iTry++) {
      const TIME tmTickBefore = GetRealTimeTick();
      Sleep(1000* iTry*3 *TickQuantum);
      const TIME tmTickAfter = GetRealTimeTick();
      ASSERT(tmTickBefore <= tmTickAfter);
      if( tmTickBefore!=tmTickAfter) break;
      Sleep(1000*iTry);
    }
    // report fatal
    if( iTry>3) FatalError(TRANS("Problem with initializing multimedia timer - please try again."));
  }
/*
    int ret = pthread_create(&g_timerMainThread, 0, &CTimer_TimerMain, nullptr);
    if (ret != 0) {
      const char *err;
      FatalError("Cannot create multimedia thread: %s (%i)", strerror(ret), ret);
    }
    pthread_setname_np(g_timerMainThread, "SeriousSamTimer");
   #endif

    // make sure that timer interrupt is ticking
    INDEX iTry = 1;
    for (; iTry <= 3; iTry++) {
      const TIME tmTickBefore = GetRealTimeTick();
      usleep(1000 * 1000 * iTry * 3 * TickQuantum);
      const TIME tmTickAfter = GetRealTimeTick();
      if (tmTickBefore != tmTickAfter) break;
      usleep(1000 * 1000 * iTry);
    }
    // report fatal
    if (iTry > 3) {
      FatalError(TRANS("Problem with initializing multimedia timer - please try again."));
    }
  }*/
#endif  // !defined SINGLE_THREADED
}

/*
 * Destructor.
 */
CTimer::~CTimer(void)
{
    // !!! FIXME : abstract this.
#if (!defined SINGLE_THREADED)
#ifdef PLATFORM_WIN32
  ASSERT(_pTimer == this);

  // destroy timer
  if (tm_bInterrupt) {
    ASSERT(tm_TimerID);
    ULONG rval = timeKillEvent(tm_TimerID);
    ASSERT(rval == TIMERR_NOERROR);
  }
  // check that all handlers have been removed
  ASSERT(tm_lhHooks.IsEmpty());

#else
    SDL_RemoveTimer(tm_TimerID);
#endif

#endif

  // clear global pointer
  _pTimer = NULL;

}

/*
 * Add a timer handler.
 */
void CTimer::AddHandler(CTimerHandler *pthNew)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&tm_csHooks, TRUE);

  ASSERT(this!=NULL);
  tm_lhHooks.AddTail(pthNew->th_Node);
}

/*
 * Remove a timer handler.
 */
void CTimer::RemHandler(CTimerHandler *pthOld)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&tm_csHooks, TRUE);

  ASSERT(this!=NULL);
  pthOld->th_Node.Remove();
}

/* Handle timer handlers manually. */
void CTimer::HandleTimerHandlers(void)
{
  // access to the list of handlers must be locked
  CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
  // handle all timers
  CTimer_TimerFunc_internal();
}

/*
 * Get current timer value of high precision timer.
 */
CTimerValue CTimer::GetHighPrecisionTimer(void)
{
  return ReadTSC();
}

/*
 * Set the real time tick value.
 */
void CTimer::SetRealTimeTick(TIME tNewRealTimeTick)
{
  ASSERT(this!=NULL);
  tm_RealTimeTimer = tNewRealTimeTick;
}

/*
 * Get the real time tick value.
 */
TIME CTimer::GetRealTimeTick(void) const
{
  ASSERT(this!=NULL);
  return tm_RealTimeTimer;
}

/*
 * Set the current game tick used for time dependent tasks (animations etc.).
 */
void CTimer::SetCurrentTick(TIME tNewCurrentTick) {
  ASSERT(this!=NULL);
  _CurrentTickTimer = tNewCurrentTick;
}

/*
 * Get current game time, always valid for the currently active task.
 */
const TIME CTimer::CurrentTick(void) const {
  ASSERT(this!=NULL);
  return _CurrentTickTimer;
}
const TIME CTimer::GetLerpedCurrentTick(void) const {
  ASSERT(this!=NULL);
  return _CurrentTickTimer+tm_fLerpFactor*TickQuantum;
}
// Set factor for lerping between ticks.
void CTimer::SetLerp(FLOAT fFactor) // sets both primary and secondary
{
  ASSERT(this!=NULL);
  tm_fLerpFactor = fFactor;
  tm_fLerpFactor2 = fFactor;
}
void CTimer::SetLerp2(FLOAT fFactor)  // sets only secondary
{
  ASSERT(this!=NULL);
  tm_fLerpFactor2 = fFactor;
}
// Disable lerping factor (set both factors to 1)
void CTimer::DisableLerp(void)
{
  ASSERT(this!=NULL);
  tm_fLerpFactor =1.0f;
  tm_fLerpFactor2=1.0f;
}

// convert a time value to a printable string (hh:mm:ss)
CTString TimeToString(FLOAT fTime)
{
  CTString strTime;
  int iSec = (int) floor(fTime);
  int iMin = iSec/60;
  iSec = iSec%60;
  int iHou = iMin/60;
  iMin = iMin%60;
  strTime.PrintF("%02d:%02d:%02d", iHou, iMin, iSec);
  return strTime;
}
#endif
