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

#include <Engine/Sound/SoundProfile.h>

// profile form for profiling sounds
CSoundProfile _spSoundProfile;
CProfileForm &_pfSoundProfile = _spSoundProfile;

/////////////////////////////////////////////////////////////////////
// CSoundProfile

CSoundProfile::CSoundProfile(void)
 : CProfileForm ("Sound", "updates",
    CSoundProfile::PCI_COUNT, CSoundProfile::PTI_COUNT)
{
  SETTIMERNAME( PTI_MIXSOUNDS,    "MixSounds()", "");
  SETTIMERNAME( PTI_DECODESOUND,  "  DecodeSound()", "");
  SETTIMERNAME( PTI_MIXSOUND,     "  MixSound()", "");
  SETTIMERNAME( PTI_RAWMIXER,     "    Raw Mixer Loop", "");
  SETTIMERNAME( PTI_UPDATESOUNDS, "UpdateSounds()", "");

  SETCOUNTERNAME( PCI_MIXINGS,       "number of mixings");
  SETCOUNTERNAME( PCI_SOUNDSMIXED,   "sounds mixed");
  SETCOUNTERNAME( PCI_SOUNDSSKIPPED, "sounds skipped for low volume");
  SETCOUNTERNAME( PCI_SOUNDSDELAYED, "sounds delayed for sound speed latency");
  SETCOUNTERNAME( PCI_SAMPLES,       "samples mixed");
}
