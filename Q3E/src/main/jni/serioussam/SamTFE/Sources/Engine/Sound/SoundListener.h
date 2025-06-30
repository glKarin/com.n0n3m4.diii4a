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

#ifndef SE_INCL_SOUNDLISTENER_H
#define SE_INCL_SOUNDLISTENER_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Matrix.h>

class CSoundListener {
public:
  CListNode sli_lnInActiveListeners;  // for linking for current frame of listening

  FLOAT3D sli_vPosition;          // listener position
  FLOATmatrix3D sli_mRotation;    // listener rotation matrix
  FLOAT3D sli_vSpeed;             // speed of the listener
  FLOAT sli_fVolume;              // listener volume (i.e. deaf factor)
  FLOAT sli_fFilter;              // global filter for all sounds on this listener
  CEntity *sli_penEntity;         // listener entity (for listener local sounds)
  INDEX sli_iEnvironmentType;     // EAX environment predefine
  FLOAT sli_fEnvironmentSize;     // EAX environment size
};


#endif  /* include-once check. */

