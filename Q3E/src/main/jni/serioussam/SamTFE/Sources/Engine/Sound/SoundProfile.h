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

#ifndef SE_INCL_SOUNDPROFILE_H
#define SE_INCL_SOUNDPROFILE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Profiling.h>

/* Class for holding profiling information for sound operations. */
class CSoundProfile : public CProfileForm {
public:
  // indices for profiling counters and timers
  enum ProfileTimerIndex {
    PTI_MIXSOUNDS,          // MixSounds()
    PTI_DECODESOUND,        // DecodeSound()
    PTI_MIXSOUND,           // MixSound()
    PTI_RAWMIXER,           // Raw Mixer Loop
    PTI_UPDATESOUNDS,       // UpdateSounds()

    PTI_COUNT
  };
  enum ProfileCounterIndex {
    PCI_MIXINGS,           // number of mixings

    PCI_SOUNDSMIXED,       // sounds mixed
    PCI_SOUNDSSKIPPED,     // sounds skipped for low volume
    PCI_SOUNDSDELAYED,     // sounds delayed for sound speed latency
    PCI_SAMPLES,      // samples mixed

    PCI_COUNT
  };
  // constructor
  CSoundProfile(void);
};


#endif  /* include-once check. */

