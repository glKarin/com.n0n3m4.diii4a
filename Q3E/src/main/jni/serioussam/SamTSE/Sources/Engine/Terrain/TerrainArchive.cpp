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

#include <Engine/Terrain/Terrain.h>
#include <Engine/Terrain/TerrainArchive.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/World/World.h>
#include <Engine/Math/Float.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Base/Stream.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Base/ListIterator.inl>

#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>

template class CDynamicArray<CBrush3D>;

/*
 * Read from stream.
 */
void CTerrainArchive::Read_t( CTStream *istrFile) // throw char *
{
  istrFile->ExpectID_t("TRAR");   // terrain archive

  INDEX ctTerrains;
  // read number of terrains
  (*istrFile)>>ctTerrains;

  // if there are some terrains
  if (ctTerrains!=0) {
    // create that much terrains
    /* CTerrain *atrBrushes = */ ta_atrTerrains.New(ctTerrains);
    // for each of the new terrains
    for (INDEX iTerrain=0; iTerrain<ctTerrains; iTerrain++) {
      // read it from stream
      CallProgressHook_t(FLOAT(iTerrain)/ctTerrains);
      ta_atrTerrains[iTerrain].Read_t(istrFile);
    }
  }

  istrFile->ExpectID_t("EOTA");   // end of terrain archive
}

/*
 * Write to stream.
 */
void CTerrainArchive::Write_t( CTStream *ostrFile) // throw char *
{
  ostrFile->WriteID_t("TRAR");   // terrain archive

  // write the number of terrains
  (*ostrFile)<<ta_atrTerrains.Count();
  // for each of the terrains
  FOREACHINDYNAMICARRAY(ta_atrTerrains, CTerrain, ittr) {
    // write it to stream
    ittr->Write_t(ostrFile);
  }

  ostrFile->WriteID_t("EOTA");   // end of terrain archive
}
