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

#ifndef SE_INCL_NORMALS_H
#define SE_INCL_NORMALS_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#define MAX_GOURAUDNORMALS 256
extern FLOAT3D  avGouraudNormals[MAX_GOURAUDNORMALS];

/* Find nearest Gouraud normal for a vector. */
INDEX GouraudNormal(    const FLOAT3D &vNormal);
void CompressNormal_HQ( const FLOAT3D &vNormal, UBYTE &ubH, UBYTE &ubP);
void DecompressNormal_HQ(     FLOAT3D &vNormal, UBYTE  ubH, UBYTE  ubP);


#endif  /* include-once check. */

