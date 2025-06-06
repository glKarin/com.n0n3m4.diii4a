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

#ifndef SE_INCL_GFXSTEREO_H
#define SE_INCL_GFXSTEREO_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

//
// stereo rendering support
//

// note: left and right are in succession, to be able to make 'for' loops
// if stereo is disabled, 'left' has same effect as 'both' to all functions
#define STEREO_LEFT    0
#define STEREO_RIGHT   1
#define STEREO_BOTH    2

// query whether user has turned stereo rendering on
ENGINE_API BOOL Stereo_IsEnabled(void);
// set buffer for stereo rendering left/right/both
ENGINE_API void Stereo_SetBuffer(INDEX iEye);
// adjust perspective projection for stereo rendering left/right/both
ENGINE_API void Stereo_AdjustProjection(CProjection3D &pr, INDEX iEye, FLOAT fFactor);

#endif  /* include-once check. */
