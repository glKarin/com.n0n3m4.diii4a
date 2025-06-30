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

#ifndef SE_INCL_FOG_H
#define SE_INCL_FOG_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Vector.h>

enum AttenuationType {
  AT_LINEAR,          // t*density
  AT_EXP,             // 1-exp(-t*density)
  AT_EXP2,            // 1-exp(-(t*density)^2)
};
enum FogGraduationType {
  FGT_CONSTANT,        // constant density
  FGT_LINEAR,          // h*graduation
  FGT_EXP,             // 1-exp(-h*graduation)
};

class CFogParameters {
public:
  FLOAT3D fp_vFogDir;   // fog direction in absolute space
  FLOAT fp_fH0; // lowest point in LUT
  FLOAT fp_fH1; // bottom of fog in LUT
  FLOAT fp_fH2; // top of fog in LUT
  FLOAT fp_fH3; // highest point in LUT
  FLOAT fp_fFar; // farthest point in LUT
  INDEX fp_iSizeL;      // LUT size in distance (must be pow2)
  INDEX fp_iSizeH;      // LUT size in depth    (must be pow2)

  COLOR fp_colColor;      // fog color and alpha
  enum AttenuationType fp_atType; // type of attenuation
  FLOAT fp_fDensity;    // attenuation parameter
  enum FogGraduationType fp_fgtType;  // type of graduation
  FLOAT fp_fGraduation;    // graduation parameter
};

#define HPF_VISIBLEFROMOUTSIDE  (1UL<<0)  // can be viewed from sectors that are not hazed
class CHazeParameters {
public:
  ULONG hp_ulFlags;
  FLOAT hp_fNear; // nearest point in LUT
  FLOAT hp_fFar;  // farthest point in LUT
  INDEX hp_iSize; // LUT size in distance (must be pow2)

  COLOR hp_colColor;      // fog color and alpha
  enum AttenuationType hp_atType; // type of attenuation
  FLOAT hp_fDensity;      // attenuation parameter
};


#endif  /* include-once check. */

