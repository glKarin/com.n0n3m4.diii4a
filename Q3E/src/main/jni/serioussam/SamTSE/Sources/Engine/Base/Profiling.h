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

#ifndef SE_INCL_PROFILING_H
#define SE_INCL_PROFILING_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#if ENGINE_INTERNAL

#include <Engine/Base/CTString.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/Timer.inl>

#include <Engine/Templates/StaticArray.h>

/*
 * Profiling counter.
 */
class CProfileCounter {
friend class CProfileForm;
private:
  CTString pc_strName;    // name of this counter
  INDEX pc_ctCount;       // the counter itself

  /* Print one counter in report. */
  void Report(char *&strBuffer, INDEX ctAveragingCount);
};

/*
 * Profiling timer.
 */
class CProfileTimer {
friend class CProfileForm;
private:
  CTString pt_strName;      // name of this timer
  CTimerValue pt_tvStarted; // time when the timer was started last time
  CTimerValue pt_tvElapsed; // total elapsed time of the timer
  CTString pt_strAveragingName; // name of averaging counter
  INDEX pt_ctAveraging;         // averaging counter for this timer

  /* Print one timer in report. */
  void Report(char *&strBuffer, INDEX ctAveragingCount,
    CTimerValue tvAppElapsed, CTimerValue tvModElapsed);
};

// this file just defines TIMER_PROFILING as 1 or 0
#include <Engine/Base/ProfilingEnabled.h>

#endif //ENGINE_INTERNAL

/*
 * Class for gathering and reporting profiling information.
 */
class CProfileForm {
public:

#if ENGINE_INTERNAL
// implementation:
  CTString pf_strTitle;             // main title of the profiling form
  CTString pf_strAveragingUnits;    // name for averaging units

  CStaticArray<CProfileCounter> pf_apcCounters;   // profiling counters
  CStaticArray<CProfileTimer>   pf_aptTimers;     // profiling timers

  CTimerValue pf_tvOverAllStarted;  // timer when overall timer was started last time
  CTimerValue pf_tvOverAllElapsed;  // total elapsed time of the overall timer
  INDEX pf_ctRunningTimers; // counter of currently running timers

  INDEX pf_ctAveragingCounter;  // counter for calculating average results
  // override to provide external averaging
  virtual INDEX GetAveragingCounter(void);

  CTimerValue pf_tvLastReset;  // time when profile form was last reset

  /* Start a timer. */
  void StartTimer_internal(INDEX iTimer);
  /* Stop a timer. */
  void StopTimer_internal(INDEX iTimer);

// interface:
  /* Constructor for profile form with given number of counters and timers.
   * NOTE: Reset() must be called on a profile form before using it!
   */
  CProfileForm(const CTString &strTitle, const CTString &strAveragingUnits,
    INDEX ctCounters, INDEX ctTimers);
  void Clear(void);

  // set/test profiling activation flag
  static void SetProfilingActive(BOOL bActive);
  static BOOL GetProfilingActive(void);

  // Measure profiling errors and set epsilon corrections.
  static void CalibrateProfilingTimers(void);

  /* Increment averaging counter by given count. */
  inline void IncrementAveragingCounter(INDEX ctAdd=1) {
    pf_ctAveragingCounter += ctAdd;
  };

  /* Increment counter by given count. */
  inline void IncrementCounter(INDEX iCounter, INDEX ctAdd=1) {
    pf_apcCounters[iCounter].pc_ctCount += ctAdd;
  };
  /* Get current value of a counter. */
  INDEX GetCounterCount(INDEX iCounter);

  inline void CountersClear() {pf_apcCounters.Clear();};
  inline void TimersClear() {pf_aptTimers.Clear();};

#if TIMER_PROFILING
  /* Start a timer. */
  inline void StartTimer(INDEX iTimer) {
    StartTimer_internal(iTimer);
  };
  /* Stop a timer. */
  inline void StopTimer(INDEX iTimer) {
    StopTimer_internal(iTimer);
  };
  /* Increment averaging counter for a timer by given count. */
  inline void IncrementTimerAveragingCounter(INDEX iTimer, INDEX ctAdd=1) {
    pf_aptTimers[iTimer].pt_ctAveraging += ctAdd;
  };
  /* Set name of a counter. */
  void SetCounterName_internal(INDEX iCounter, const CTString &strName)
  {
    pf_apcCounters[iCounter].pc_strName = strName;
  }
  /* Set name of a timer. */
  void SetTimerName_internal(INDEX iTimer, const CTString &strName, const CTString &strAveragingName)
  {
    pf_aptTimers[iTimer].pt_strName = strName;
    pf_aptTimers[iTimer].pt_strAveragingName = strAveragingName;
  }
  #define SETCOUNTERNAME(a,b) SetCounterName_internal(a,b)
  #define SETTIMERNAME(a,b,c) SetTimerName_internal(a,b,c)

#else //TIMER_PROFILING
  inline void StartTimer(INDEX iTimer) {};
  inline void StopTimer(INDEX iTimer) {};
  inline void IncrementTimerAveragingCounter(INDEX iTimer, INDEX ctAdd=1) {};
  inline void SetCounterName_internal(INDEX iCounter, const CTString &strName) {};
  inline void SetTimerName_internal(INDEX iTimer, const CTString &strName, const CTString &strAveragingName) {};
  #define SETCOUNTERNAME(a,b) SetCounterName_internal(a,"")
  #define SETTIMERNAME(a,b,c) SetTimerName_internal(a,"","")
#endif

  /* Get current value of a timer in seconds or in percentage of module time. */
  double GetTimerPercentageOfModule(INDEX iTimer);
  double GetTimerAverageTime(INDEX iTimer);
  /* Get name of a counter. */
  const CTString &GetCounterName(INDEX iCounter);
  /* Get name of a timer. */
  const CTString &GetTimerName(INDEX iTimer);

  /* Get percentage of module time in application time. */
  double GetModulePercentage(void);

#else

   // !!! FIXME : rcg10102001 I needed to add these to compile
   // !!! FIXME :  Engine/Classes/MovableEntity.es. What am I doing wrong?
  inline void IncrementCounter(INDEX iCounter, INDEX ctAdd=1) {}
  inline void StartTimer(INDEX iTimer) {};
  inline void StopTimer(INDEX iTimer) {};
  inline void IncrementAveragingCounter(INDEX ctAdd=1) {};
  inline void IncrementTimerAveragingCounter(INDEX iTimer, INDEX ctAdd=1) {};
  inline void SetCounterName_internal(INDEX iCounter, const CTString &strName) {};
  inline void SetTimerName_internal(INDEX iTimer, const CTString &strName, const CTString &strAveragingName) {};
  #define SETCOUNTERNAME(a,b) SetCounterName_internal(a,"")
  #define SETTIMERNAME(a,b,c) SetTimerName_internal(a,"","")
  inline void CountersClear() {};
  inline void TimersClear() {};

#endif // ENGINE_INTERNAL

  /* Reset all profiling values. */
  ENGINE_API void  Reset(void);

  /* Report profiling results. */
  ENGINE_API void Report(CTString &strReport);
};

// profile form for profiling gfx
ENGINE_API extern CProfileForm &_pfGfxProfile;
// profile form for profiling model rendering
ENGINE_API extern CProfileForm &_pfModelProfile;
// profile form for profiling sound
ENGINE_API extern CProfileForm &_pfSoundProfile;
// profile form for profiling network
ENGINE_API extern CProfileForm &_pfNetworkProfile;
// profile form for profiling rendering
ENGINE_API extern CProfileForm &_pfRenderProfile;
// profile form for profiling world editing
ENGINE_API extern CProfileForm &_pfWorldEditingProfile;
// profile form for profiling phisics
ENGINE_API extern CProfileForm &_pfPhysicsProfile;


#endif  /* include-once check. */

