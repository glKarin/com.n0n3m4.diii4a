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

#ifndef SE_INCL_LASTPOSITIONS_H
#define SE_INCL_LASTPOSITIONS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/StaticArray.h>
#include <Engine/Math/Vector.h>

// last positions for particle systems
class ENGINE_API CLastPositions {
public:
  CStaticArray<FLOAT3D> lp_avPositions;
  INDEX lp_iLast;   // where entity was last placed
  INDEX lp_ctUsed;  // how many positions are actually used
  TIME lp_tmLastAdded;  // time when last updated
  
  CLastPositions() {};
  CLastPositions(const CLastPositions &lpOrg);

  // add a new position
  void AddPosition(const FLOAT3D &vPos);
  // get a position
  const FLOAT3D &GetPosition(INDEX iPre);
};


#endif  /* include-once check. */

