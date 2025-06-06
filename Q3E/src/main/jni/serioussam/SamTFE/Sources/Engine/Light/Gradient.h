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

#ifndef SE_INCL_GRADIENT_H
#define SE_INCL_GRADIENT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

class CGradientParameters {
public:
  FLOAT3D gp_vGradientDir;   // gradient direction in absolute space
  FLOAT gp_fH0; // position of color 0
  FLOAT gp_fH1; // position of color 1
  BOOL gp_bDark; // if dark light

  COLOR gp_col0;      // color 0
  COLOR gp_col1;      // color 1
};


#endif  /* include-once check. */

