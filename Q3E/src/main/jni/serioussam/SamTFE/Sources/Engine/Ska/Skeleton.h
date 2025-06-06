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

#ifndef SE_INCL_SKELETON_H
#define SE_INCL_SKELETON_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Math/Quaternion.h>
#include <Engine/Base/CTString.h>
#include <Engine/Math/Vector.h>
#include <Engine/Math/Placement.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Templates/DynamicArray.h>
#include <Engine/Templates/StaticArray.h>
#include <Engine/Base/Serial.h>


struct QVect
{
  FLOAT3D vPos;
  FLOATquat3D qRot;
};

static inline CTStream &operator>>(CTStream &strm, QVect &qv)
{
    strm>>qv.vPos;
    strm>>qv.qRot;
    return(strm);
}

static inline CTStream &operator<<(CTStream &strm, const QVect &qv)
{
    strm<<qv.vPos;
    strm<<qv.qRot;
    return(strm);
}


struct ENGINE_API SkeletonLOD
{
  FLOAT slod_fMaxDistance;                        // distance in witch this lod is visible
  CStaticArray<struct SkeletonBone> slod_aBones;  // array of bones for this lod
  CTString slod_fnSourceFile;                     // source filename of ascii skleton lod
};

struct ENGINE_API SkeletonBone
{
  INDEX sb_iID;               // ID of bone
  INDEX sb_iParentID;         // ID of parent bone
  Matrix12 sb_mAbsPlacement;  // default bone placement
  QVect sb_qvRelPlacement;    // default bone placement (same as mAbsPlacement)
  FLOAT sb_fOffSetLen;
  FLOAT sb_fBoneLength;       // length of bone
};

class ENGINE_API CSkeleton : public CSerial
{
public:
  CSkeleton();
  ~CSkeleton();
  
  // Find bone in skeleton lod
  INDEX FindBoneInLOD(INDEX iBoneID,INDEX iSkeletonLod);
  // Sorts bones in skeleton so parent bones are allways before child bones in array
  void SortSkeleton();
  void SortSkeletonRecursive(INDEX iParentID, INDEX iSkeletonLod);
  // Calculate absolute transformations for all bones in this lod
  void CalculateAbsoluteTransformations(INDEX iSkeletonLod);
  // Add skeleton lod to skeleton
  void AddSkletonLod(SkeletonLOD &slod);
  // Remove skleton lod form skeleton
  void RemoveSkeletonLod(SkeletonLOD *pslodRemove);

  void Read_t( CTStream *istrFile); // throw char *
  void Write_t( CTStream *ostrFile); // throw char *
  void Clear(void);
  SLONG GetUsedMemory(void);


  CStaticArray<struct SkeletonLOD>  skl_aSkeletonLODs;
}; 




#endif  /* include-once check. */

