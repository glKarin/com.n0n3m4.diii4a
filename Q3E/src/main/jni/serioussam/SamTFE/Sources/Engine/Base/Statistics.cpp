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

#include <Engine/Base/Statistics.h>
#include <Engine/Base/Statistics_Internal.h>

#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/StaticArray.cpp>

template class CStaticArray<CStatCounter>;
template class CStaticArray<CStatTimer>;
template class CStaticArray<CStatLabel>;

// one globaly used stats report
CStatForm _sfStats;

CStatForm::CStatForm()
{
  sf_ascCounters.New(SCI_COUNT);
  sf_astTimers.New(STI_COUNT);
  sf_aslLabels.New(SLI_COUNT);

  InitCounter( SCI_SCENE_TRIANGLES,      101, "^c00DF00tris=%.0f", 1);
  InitCounter( SCI_SCENE_TRIANGLEPASSES, 101, "\ntpas=%.0f", 1);
  InitCounter( SCI_SECTORS,              101, "\nsecs=%.0f", 1);
  InitCounter( SCI_POLYGONS,             101, "\nplys=%.0f+", 1);
  InitCounter( SCI_DETAILPOLYGONS,       101, "%.0f", 1);
  InitCounter( SCI_POLYGONEDGES,         101, "\npled=%.0f", 1);
  InitCounter( SCI_EDGETRANSITIONS,      101, "\nedtr=%.0f", 1);
  InitCounter( SCI_SOUNDSMIXING,         101, "^cDFDFAF\nsnds=%.0f", 1);
  InitCounter( SCI_SOUNDSACTIVE,         101, "/%.0f", 1);
               
  InitCounter( SCI_CACHEDSHADOWS,      101, "^cDFDF00\n\ncsh=%3.0f+", 1);
  InitCounter( SCI_FLATSHADOWS,        101, "%2.0f", 1);
  InitCounter( SCI_CACHEDSHADOWBYTES,  101, "/%.0fK", 1/1024.0f);
  InitCounter( SCI_DYNAMICSHADOWS,     101, "\ndyn=%3.0f", 1);
  InitCounter( SCI_DYNAMICSHADOWBYTES, 101, "/%.0fK", 1/1024.0f);

  InitCounter( SCI_SHADOWBINDS,        101, "^cEFEF00\nshd=%3.0f", 1);
  InitCounter( SCI_SHADOWBINDBYTES,    101, "/%.0fK", 1/1024.0f);
  InitCounter( SCI_TEXTUREBINDS,       101, "\ntex=%3.0f", 1);
  InitCounter( SCI_TEXTUREBINDBYTES,   101, "/%.0fK", 1/1024.0f);
  InitCounter( SCI_TEXTUREUPLOADS,     101, "\nupl=%3.0f", 1);
  InitCounter( SCI_TEXTUREUPLOADBYTES, 101, "/%.0fK", 1/1024.0f);
               
  InitCounter( SCI_PARTICLES,                101, "^c00EFEF\n\npart=%.0f", 1);
  InitCounter( SCI_MODELS,                   101, "^c00DFDF\nmdls=%.0f", 1);
  InitCounter( SCI_MODELSHADOWS,             101, "\nshds=%.0f", 1);
  InitCounter( SCI_TRIANGLES_USEDMIP,        101, "\ntris=%.0f", 1);
  InitCounter( SCI_TRIANGLES_FIRSTMIP,       101, "/%.0f", 1);
  InitCounter( SCI_SHADOWTRIANGLES_USEDMIP,  101, "\nstri=%.0f", 1);
  InitCounter( SCI_SHADOWTRIANGLES_FIRSTMIP, 101, "/%.0f", 1);
               
  InitTimer( STI_WORLDTRANSFORM,     101, "^C\n\nwldtra=%2.0f ms", 1000.0f);
  InitTimer( STI_WORLDVISIBILITY,    101, "\nwldvis=%2.0f ms", 1000.0f);
  InitTimer( STI_WORLDRENDERING,     101, "\nwldren=%2.0f ms", 1000.0f);
  InitTimer( STI_MODELSETUP,         101, "^c00FFFF\nmdlset=%2.0f ms", 1000.0f);
  InitTimer( STI_MODELRENDERING,     101, "\nmdlren=%2.0f ms", 1000.0f);
  InitTimer( STI_PARTICLERENDERING,  101, "\npartic=%2.0f ms", 1000.0f);
  InitTimer( STI_FLARESRENDERING,    101, "\nflares=%2.0f ms", 1000.0f);

  InitTimer( STI_SOUNDUPDATE, 101, "^cFFFFCF\nsndupd=%2.0f ms", 1000.0f);
  InitTimer( STI_SOUNDMIXING, 101, "\nsndmix=%2.0f ms", 1000.0f);
  InitTimer( STI_TIMER,       101, "\ntimer =%2.0f ms", 1000.0f);
  InitTimer( STI_MAINLOOP,    101, "\nmainlp=%2.0f ms", 1000.0f);
  InitTimer( STI_RAYCAST,     101, "\nraycst=%2.0f ms", 1000.0f);
             
  InitTimer( STI_SHADOWUPDATE, 101, "^cFFFF00\nshdupd=%2.0f ms", 1000.0f);
  InitTimer( STI_EFFECTRENDER, 101, "\nefftex=%2.0f ms", 1000.0f);
  InitTimer( STI_BINDTEXTURE,  101, "\nbindtx=%2.0f ms", 1000.0f);      

  InitTimer( STI_GFXAPI,      101, "^cFFFFFF\n\ngfxapi=%2.0f ms", 1000.0f);
  InitTimer( STI_SWAPBUFFERS, 101, "\nswpbuf=%2.0f ms^C", 1000.0f);
}


void CStatForm::Clear(void)
{
  sf_ascCounters.Clear();
  sf_astTimers.Clear();
  sf_aslLabels.Clear();
}


// make a new report
void CStatForm::Report(CTString &strReport)
{
  // clear the report initially
  strReport = "";

  // add all entries to print to a container
  CDynamicContainer<CStatEntry> cse;
  for (INDEX iCounter = 0; iCounter<sf_ascCounters.Count(); iCounter++) {
    cse.Add(&sf_ascCounters[iCounter]);
  }
  for (INDEX iTimer = 0; iTimer<sf_astTimers.Count(); iTimer++) {
    cse.Add(&sf_astTimers[iTimer]);
  }
  for (INDEX iLabel = 0; iLabel<sf_aslLabels.Count(); iLabel++) {
    cse.Add(&sf_aslLabels[iLabel]);
  }

  // sort the container here !!!!

  // for each entry
  {FOREACHINDYNAMICCONTAINER(cse, CStatEntry, itse) {
    strReport += itse->Report();
  }}
}

// initialize component
void CStatForm::InitCounter(INDEX iCounter, INDEX iOrder, const char *strFormat, FLOAT fFactor)
{
  CStatCounter &sc = sf_ascCounters[iCounter];
  sc.se_iOrder = iOrder;
  sc.sc_fCount = 0;
  sc.sc_fFactor = fFactor;
  sc.sc_strFormat = strFormat;
}

void CStatForm::InitTimer(INDEX iTimer, INDEX iOrder, const char *strFormat, FLOAT fFactor)
{
  CStatTimer &st = sf_astTimers[iTimer];
  st.se_iOrder = iOrder;
  st.st_tvElapsed.Clear();
  st.st_tvStarted.tv_llValue = -1;
  st.st_fFactor = fFactor;
  st.st_strFormat = strFormat;
}

void CStatForm::InitLabel(INDEX iLabel, INDEX iOrder, const char *strFormat)
{
  CStatLabel &sl = sf_aslLabels[iLabel];
  sl.se_iOrder = iOrder;
  sl.sl_strFormat = strFormat;
}

// Reset all profiling values
void CStatForm::Reset(void)
{
  // for each counter
  for (INDEX iCounter = 0; iCounter<sf_ascCounters.Count(); iCounter++) {
    // reset it
    sf_ascCounters[iCounter].sc_fCount = 0;
  }
  // for each timer
  for (INDEX iTimer = 0; iTimer<sf_astTimers.Count(); iTimer++) {
    // double-check that timer has been stopped (only for timers in main thread!)
    if( iTimer!=STI_TIMER && iTimer!=STI_SOUNDMIXING) {
      ASSERT( sf_astTimers[iTimer].st_tvStarted.tv_llValue == -1);
    } // reset it
    sf_astTimers[iTimer].st_tvElapsed.Clear();
  }
}

CTString CStatCounter::Report(void)
{
  CTString str( 0, sc_strFormat, sc_fCount*sc_fFactor);
  return str;
}

CTString CStatTimer::Report(void)
{
  CTString str( 0, st_strFormat, st_tvElapsed.GetSeconds()*st_fFactor);
  return str;
}

CTString CStatLabel::Report(void)
{
  return sl_strFormat;
}

// reset all values
void STAT_Reset(void)
{
  _sfStats.Reset();
}

// make a new report
void STAT_Report(CTString &strReport)
{
  _sfStats.Report(strReport);
}

void STAT_Enable(BOOL enable)
{
  _sfStats.sf_enabled = enable;
}
