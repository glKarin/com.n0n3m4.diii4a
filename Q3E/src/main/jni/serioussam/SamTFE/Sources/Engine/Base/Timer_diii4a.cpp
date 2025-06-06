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

#include <Engine/Base/Registry.h>
#include <Engine/Base/Profiling.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Statistics_Internal.h>

#include <Engine/Base/ListIterator.inl>
#include <Engine/Base/Priority.inl>
#include "unistd.h"
#include "pthread.h"

int64_t getTimeNsec() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

// Read the Pentium TimeStampCounter
static inline __int64 ReadTSC(void)
{
//  __int64 mmRet;
//  __asm {
//    rdtsc
//    mov   dword ptr [mmRet+0],eax
//    mov   dword ptr [mmRet+4],edx
//  }
//  return mmRet;
  return getTimeNsec();
}

#ifdef _WIN32
// link with Win-MultiMedia
#pragma comment(lib, "winmm.lib")
#endif

// current game time always valid for the currently active task
CThreadLocal<TIME> _CurrentTickTimer;

// CTimer implementation

// pointer to global timer object
CTimer *_pTimer = NULL;

pthread_t g_timerMainThread;

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

    // increment the 'real time' timer
    _pTimer->tm_RealTimeTimer += _pTimer->TickQuantum;

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

void *CTimer_TimerMain(void *input) {
  while(true) {
    // sleep
    usleep(ULONG(_pTimer->TickQuantum * 1000.0f * 1000.0f)); // microsecond

    // TODO: unsynch
#if 0
    if (_pTimer->tm_bPaused) {
      usleep(50000);
      continue;
    }
#endif

    // access to the list of handlers must be locked
    CTSingleLock slHooks(&_pTimer->tm_csHooks, TRUE);
    // handle all timers
    CTimer_TimerFunc_internal();
  }
}


#pragma inline_depth()

#define MAX_MEASURE_TRIES 5
static INDEX _aiTries[MAX_MEASURE_TRIES];

/*
 * Constructor.
 */
CTimer::CTimer(BOOL bInterrupt /*=TRUE*/)
{
  tm_csHooks.cs_iIndex = 1000;
  // set global pointer
  ASSERT(_pTimer == NULL);
  _pTimer = this;
  tm_bInterrupt = bInterrupt;

  { // this part of code must be executed as precisely as possible
//    CSetPriority sp(REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL);
    tm_llCPUSpeedHZ = 1000000000;
    tm_llPerformanceCounterFrequency = tm_llCPUSpeedHZ;

    // measure profiling errors and set epsilon corrections
    CProfileForm::CalibrateProfilingTimers();
  }

  // clear counters
  *_CurrentTickTimer = TIME(0);
  tm_RealTimeTimer = TIME(0);

  tm_tmLastTickOnTime = TIME(0);
  tm_tvLastTimeOnTime = GetHighPrecisionTimer();
  // disable lerping by default
  tm_fLerpFactor = 1.0f;
  tm_fLerpFactor2 = 1.0f;

  if (tm_bInterrupt) {
    int ret = pthread_create(&g_timerMainThread, 0, &CTimer_TimerMain, nullptr);
    if (ret != 0) {
      const char *err;
      FatalError("Cannot create multimedia thread: %s (%i)", strerror(ret), ret);
    }
//    pthread_setname_np(g_timerMainThread, "SeriousSamTimer");

    // make sure that timer interrupt is ticking

    // In web threads are spawned in async
#ifndef EMSCRIPTEN
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
#endif

  }
}

/*
 * Destructor.
 */
CTimer::~CTimer(void)
{
  ASSERT(_pTimer == this);

  // destroy timer
//  if (tm_bInterrupt) {
//    ASSERT(tm_TimerID!=NULL);
//    ULONG rval = timeKillEvent(tm_TimerID);
//    ASSERT(rval == TIMERR_NOERROR);
//  }
  // check that all handlers have been removed
  ASSERT(tm_lhHooks.IsEmpty());

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
  *_CurrentTickTimer = tNewCurrentTick;
}

/*
 * Get current game time, always valid for the currently active task.
 */
const TIME CTimer::CurrentTick(void) const {
  ASSERT(this!=NULL);
  return *_CurrentTickTimer;
}
const TIME CTimer::GetLerpedCurrentTick(void) const {
  ASSERT(this!=NULL);
  return *_CurrentTickTimer+tm_fLerpFactor*TickQuantum;
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
  int iSec = floor(fTime);
  int iMin = iSec/60;
  iSec = iSec%60;
  int iHou = iMin/60;
  iMin = iMin%60;
  strTime.PrintF("%02d:%02d:%02d", iHou, iMin, iSec);
  return strTime;
}

/*
 * Get current timer value of high precision timer.
 */
CTimerValue CTimer::GetHighPrecisionTimer(void)
{
  return ReadTSC();
}
