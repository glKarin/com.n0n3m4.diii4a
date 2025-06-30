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

#include <Engine/Entities/LastPositions.h>
#include <Engine/Math/Functions.h>
#include <Engine/Base/Timer.h>
#include <Engine/Templates/StaticArray.cpp>

CLastPositions::CLastPositions(const CLastPositions &lpOrg)
{
  lp_avPositions = lpOrg.lp_avPositions ;
  lp_iLast       = lpOrg.lp_iLast       ;
  lp_ctUsed      = lpOrg.lp_ctUsed      ;
  lp_tmLastAdded = lpOrg.lp_tmLastAdded ;
}

// add a new position
void CLastPositions::AddPosition(const FLOAT3D &vPos)
{
  lp_iLast++;
  if (lp_iLast>=lp_avPositions.Count()) {
    lp_iLast=0;
  }
  lp_ctUsed = Min(INDEX(lp_ctUsed+1), lp_avPositions.Count());
  lp_avPositions[lp_iLast] = vPos;
  lp_tmLastAdded = _pTimer->CurrentTick();
}

// get a position
const FLOAT3D &CLastPositions::GetPosition(INDEX iPre)
{
  INDEX iPos = lp_iLast-iPre;
  if (iPos<0) {
    iPos+=lp_avPositions.Count();
  }
  return lp_avPositions[iPos];
}
