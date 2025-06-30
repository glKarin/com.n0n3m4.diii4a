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

#ifndef SE_INCL_TERRAIN_ARCHIVE_H
#define SE_INCL_TERRAIN_ARCHIVE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Serial.h>
#include <Engine/Base/Lists.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Templates/DynamicArray.cpp>

/*
 * Terrain archive class -- a collection of terrains used by a level.
 */
class ENGINE_API CTerrainArchive : public CSerial {
public:
  CDynamicArray<CTerrain> ta_atrTerrains; // all the terrains in archive
  CWorld *ta_pwoWorld;  // the world

  // overrides from CSerial
  /* Read/write to/from stream. */
  void Read_t( CTStream *istrFile);  // throw char *
  void Write_t( CTStream *ostrFile); // throw char *
};


#endif  /* include-once check. */

