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

#ifndef SE_INCL_STATISTICS_INTERNAL_H
#define SE_INCL_STATISTICS_INTERNAL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#if !ENGINE_EXPORTS
  #error engine-internal file included out of engine!
#endif

#include <Engine/Base/CTString.h>
#include <Engine/Base/Timer.h>
#include <Engine/Templates/StaticArray.h>

class CStatEntry {
public:
  INDEX se_iOrder;   // order in output
  virtual CTString Report(void) = 0;
};

/*
 * Statistics counter.
 */
class CStatCounter : public CStatEntry {
public:
  CTString sc_strFormat;  // printing format (must contain one %f or %g or %e)
  FLOAT sc_fCount;        // the counter itself
  FLOAT sc_fFactor;       // printout factor
  inline void Clear(void) {};
  virtual CTString Report(void);
};

/*
 * Statistics timer.
 */
class CStatTimer : public CStatEntry {
public:
  CTString st_strFormat;  // printing format (must contain one %f or %g or %e)
  CTimerValue st_tvStarted; // time when the timer was started last time
  CTimerValue st_tvElapsed; // total elapsed time of the timer
  FLOAT st_fFactor;       // printout factor
  inline void Clear(void) {};
  virtual CTString Report(void);
};

/*
 * Statistics label.
 */
class CStatLabel : public CStatEntry {
public:
  CTString sl_strFormat;  // printing format
  inline void Clear(void) {};
  virtual CTString Report(void);
};

/*
 * Class for gathering and reporting statistics information.
 */
class CStatForm {
public:
// implementation:
  CStaticArray<class CStatCounter> sf_ascCounters;   // profiling counters
  CStaticArray<class CStatTimer>   sf_astTimers;     // profiling timers
  CStaticArray<class CStatLabel>   sf_aslLabels;     // profiling labels
  BOOL sf_enabled;

// interface:
  enum StatLabelIndex
  {
    SLI_COUNT
  };
  enum StatTimerIndex
  {
    STI_WORLDTRANSFORM,
    STI_WORLDVISIBILITY,
    STI_WORLDRENDERING,
    STI_MODELSETUP,
    STI_MODELRENDERING,
    STI_PARTICLERENDERING,
    STI_FLARESRENDERING,

    STI_SOUNDUPDATE,
    STI_SOUNDMIXING,
    STI_TIMER,
    STI_MAINLOOP,
    STI_RAYCAST,

    STI_SHADOWUPDATE,
    STI_EFFECTRENDER,
    STI_BINDTEXTURE, 

    STI_GFXAPI,
    STI_SWAPBUFFERS,

    STI_COUNT
  };
  enum StatCounterIndex
  {
    SCI_SCENE_TRIANGLES,
    SCI_SCENE_TRIANGLEPASSES,
    SCI_SECTORS,
    SCI_POLYGONS,
    SCI_DETAILPOLYGONS,
    SCI_POLYGONEDGES,
    SCI_EDGETRANSITIONS,

    SCI_SOUNDSMIXING,
    SCI_SOUNDSACTIVE,

    SCI_CACHEDSHADOWS,
    SCI_FLATSHADOWS,
    SCI_CACHEDSHADOWBYTES,
    SCI_DYNAMICSHADOWS,
    SCI_DYNAMICSHADOWBYTES,
    SCI_SHADOWBINDS,
    SCI_SHADOWBINDBYTES,

    SCI_TEXTUREBINDS,
    SCI_TEXTUREBINDBYTES,
    SCI_TEXTUREUPLOADS,
    SCI_TEXTUREUPLOADBYTES,

    SCI_PARTICLES,
    SCI_MODELS,
    SCI_MODELSHADOWS,
    SCI_TRIANGLES_USEDMIP,
    SCI_TRIANGLES_FIRSTMIP,
    SCI_SHADOWTRIANGLES_USEDMIP,
    SCI_SHADOWTRIANGLES_FIRSTMIP,

    SCI_COUNT
  };

  CStatForm(void);
  void Clear(void);

  /* Increment counter by given count. */
  inline void IncrementCounter(INDEX iCounter, FLOAT fAdd=1) {
    sf_ascCounters[iCounter].sc_fCount += fAdd;
  };

  /* Start a timer. */
  inline void StartTimer(INDEX iTimer) {
    CStatTimer &st = sf_astTimers[iTimer];
    ASSERT( sf_astTimers[iTimer].st_tvStarted.tv_llValue == -1);
    if (sf_enabled)
      st.st_tvStarted = _pTimer->GetHighPrecisionTimer();
  };
  /* Stop a timer. */
  inline void StopTimer(INDEX iTimer) {
    CStatTimer &st = sf_astTimers[iTimer];
    ASSERT( sf_astTimers[iTimer].st_tvStarted.tv_llValue != -1);
    if (sf_enabled)
      st.st_tvElapsed += _pTimer->GetHighPrecisionTimer()-st.st_tvStarted;
    st.st_tvStarted.tv_llValue = -1;
  };

  /* Check whether the timer is counting. */
  inline BOOL CheckTimer(INDEX iTimer) { 
    CStatTimer &st = sf_astTimers[iTimer];
    return (st.st_tvStarted.tv_llValue != -1);
  };

  // initialize component
  void InitCounter(INDEX iCounter, INDEX iOrder, const char *strFormat, FLOAT fFactor);
  void InitTimer(INDEX iTimer, INDEX iOrder, const char *strFormat, FLOAT fFactor);
  void InitLabel(INDEX iLabel, INDEX iOrder, const char *strFormat);

  // reset all values
  void Reset(void);
  // make a new report
  void Report(CTString &strReport);
};

// one globaly used stats report
ENGINE_API extern CStatForm _sfStats;


#endif  /* include-once check. */

