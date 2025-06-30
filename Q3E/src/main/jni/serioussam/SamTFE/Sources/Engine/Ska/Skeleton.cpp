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

#include <Engine/StdH.h>
#include "Skeleton.h"

#include <Engine/Graphics/DrawPort.h>
#include <Engine/Math/Projection.h>
#include <Engine/Ska/StringTable.h>
#include <Engine/Ska/Render.h>
#include <Engine/Base/Stream.h>
#include <Engine/Templates/DynamicContainer.cpp>

#define SKELETON_VERSION  6
#define SKELETON_ID       "SKEL"

static CStaticArray<struct SkeletonBone> _aSortArray;
INDEX ctSortBones;

CSkeleton::CSkeleton()
{
}

CSkeleton::~CSkeleton()
{
}
// Find bone in skeleton lod
INDEX CSkeleton::FindBoneInLOD(INDEX iBoneID,INDEX iSkeletonLod)
{
  ASSERT(iSkeletonLod>=0);
  INDEX ctslods = skl_aSkeletonLODs.Count();
  if(ctslods < 1) return -1;

  SkeletonLOD &slod = skl_aSkeletonLODs[iSkeletonLod];
  INDEX ctb = slod.slod_aBones.Count();
  // for each bone in skeleton
  for(INDEX isb=0;isb<ctb;isb++) {
    // if bone id match
    if(slod.slod_aBones[isb].sb_iID == iBoneID) {
      // return index in array of bones
      return isb;
    }
  }
  return -1;
}

// Sorts bones in skeleton so parent bones are allways before child bones in array
void CSkeleton::SortSkeleton()
{
  // sort each lod in skeleton
  INDEX ctslods = skl_aSkeletonLODs.Count();
  // for each lod in skeleton
  for(INDEX islod=0;islod<ctslods;islod++)
  {
    // create SkeletonBones array for sorting array
    _aSortArray.New(skl_aSkeletonLODs[islod].slod_aBones.Count());
    // start sort for parent bone
    SortSkeletonRecursive(-1,islod);
    // just copy sorted array
    skl_aSkeletonLODs[islod].slod_aBones.CopyArray(_aSortArray);
    // clear array
    _aSortArray.Clear();
    // calculate abs transforms for bones in this lod
    CalculateAbsoluteTransformations(islod);
  }
}

// Sort skeleton bones in lod so parent bones are before child bones in array
void CSkeleton::SortSkeletonRecursive(INDEX iParentID, INDEX iSkeletonLod)
{
  // reset global counter for sorted bones if this bone is parent bone
  if(iParentID == (-1)) ctSortBones = 0;
  SkeletonLOD &slod = skl_aSkeletonLODs[iSkeletonLod];
  INDEX ctsb = slod.slod_aBones.Count();
  // for each bone in lod
  for(INDEX isb=0;isb<ctsb;isb++)
  {
    SkeletonBone &sb = slod.slod_aBones[isb];
    if(sb.sb_iParentID == iParentID)
    {
      // just copy data to _aSortArray
      _aSortArray[ctSortBones].sb_iID = sb.sb_iID;
      _aSortArray[ctSortBones].sb_iParentID = sb.sb_iParentID;
      _aSortArray[ctSortBones].sb_fBoneLength = sb.sb_fBoneLength;
      memcpy(&_aSortArray[ctSortBones].sb_mAbsPlacement,&sb.sb_mAbsPlacement,sizeof(float)*12);
      memcpy(&_aSortArray[ctSortBones].sb_qvRelPlacement,&sb.sb_qvRelPlacement,sizeof(QVect));
      _aSortArray[ctSortBones].sb_fOffSetLen = sb.sb_fOffSetLen;
      // increase couter for sorted bones
      ctSortBones++;
    }
  }
  // sort children bones
  for(INDEX icsb=0;icsb<ctsb;icsb++) {
    SkeletonBone &sb = slod.slod_aBones[icsb];
    if(sb.sb_iParentID == iParentID) {
      SortSkeletonRecursive(sb.sb_iID,iSkeletonLod);
    }
  }
}

// Calculate absolute transformations for all bones in this lod
void CSkeleton::CalculateAbsoluteTransformations(INDEX iSkeletonLod)
{
  INDEX ctbones = skl_aSkeletonLODs[iSkeletonLod].slod_aBones.Count();
  SkeletonLOD &slod = skl_aSkeletonLODs[iSkeletonLod];
  // for each bone
  for(INDEX isb=0; isb<ctbones; isb++) {
    SkeletonBone &sb = slod.slod_aBones[isb];
    INDEX iParentID = sb.sb_iParentID;
    INDEX iParentIndex = FindBoneInLOD(iParentID,iSkeletonLod);
    if(iParentID > (-1)) {
      SkeletonBone &sbParent = slod.slod_aBones[iParentIndex];
      MatrixMultiplyCP(sb.sb_mAbsPlacement,sbParent.sb_mAbsPlacement,sb.sb_mAbsPlacement);
    }
  }
}

// Add skeleton lod to skeleton
void CSkeleton::AddSkletonLod(SkeletonLOD &slod)
{
  INDEX ctlods = skl_aSkeletonLODs.Count();
  skl_aSkeletonLODs.Expand(ctlods+1);
  skl_aSkeletonLODs[ctlods] = slod;
}

// Remove skleton lod form skeleton
void CSkeleton::RemoveSkeletonLod(SkeletonLOD *pslodRemove)
{
  INDEX ctslod = skl_aSkeletonLODs.Count();
  // create temp space for skeleton lods
  CStaticArray<struct SkeletonLOD> aTempSLODs;
  aTempSLODs.New(ctslod-1);
  INDEX iIndexSrc=0;

  // for each skeleton lod in skeleton
  for(INDEX islod=0;islod<ctslod;islod++) {
    SkeletonLOD *pslod = &skl_aSkeletonLODs[islod];
    // copy all skeleton lods except the selected one
    if(pslod != pslodRemove) {
      aTempSLODs[iIndexSrc] = *pslod;
      iIndexSrc++;
    }
  }
  // copy temp array of skeleton lods back in skeleton
  skl_aSkeletonLODs.CopyArray(aTempSLODs);
  // clear temp skleletons lods array
  aTempSLODs.Clear();
}

// write to stream
void CSkeleton::Write_t(CTStream *ostrFile)
{
  INDEX ctslods = skl_aSkeletonLODs.Count();
  // write id
  ostrFile->WriteID_t(CChunkID(SKELETON_ID));
  // write version
  (*ostrFile)<<(INDEX)SKELETON_VERSION;
  // write lods count
  (*ostrFile)<<ctslods;
  // for each lod in skeleton
  for(INDEX islod=0;islod<ctslods;islod++)
  {
    SkeletonLOD &slod = skl_aSkeletonLODs[islod];
    // write source file name
    (*ostrFile)<<slod.slod_fnSourceFile;
    // write MaxDistance
    (*ostrFile)<<slod.slod_fMaxDistance;

    // write bone count
    INDEX ctb = slod.slod_aBones.Count();
    (*ostrFile)<<ctb;
    // write skeleton bones
    for(INDEX ib=0;ib<ctb;ib++) {
      CTString strNameID = ska_GetStringFromTable(slod.slod_aBones[ib].sb_iID);
      CTString strParentID = ska_GetStringFromTable(slod.slod_aBones[ib].sb_iParentID);
      SkeletonBone &sb = slod.slod_aBones[ib];
      // write ID
      (*ostrFile)<<strNameID;
      // write Parent ID
      (*ostrFile)<<strParentID;
      //(*ostrFile)<<slod.slod_aBones[ib].sb_iParentIndex;
      // write AbsPlacement matrix
      ostrFile->Write_t(&sb.sb_mAbsPlacement,sizeof(FLOAT)*12);
      // write RelPlacement Qvect stuct
      ostrFile->Write_t(&sb.sb_qvRelPlacement,sizeof(QVect));
      // write offset len
      (*ostrFile)<<sb.sb_fOffSetLen;
      // write bone length
      (*ostrFile)<<sb.sb_fBoneLength;
    }
  }
}

// read from stream
void CSkeleton::Read_t(CTStream *istrFile)
{
  INDEX iFileVersion;
  INDEX ctslods;
  // read chunk id
  istrFile->ExpectID_t(CChunkID(SKELETON_ID));
  // check file version
  (*istrFile)>>iFileVersion;
  if(iFileVersion != SKELETON_VERSION) {
		ThrowF_t(TRANS("File '%s'.\nInvalid skeleton file version.\nExpected Ver \"%d\" but found \"%d\"\n"),
      (const char*)istrFile->GetDescription(),SKELETON_VERSION,iFileVersion);
  }
  // read skeleton lod count
  (*istrFile)>>ctslods;

  if(ctslods>0) {
    skl_aSkeletonLODs.Expand(ctslods);
  }
  // for each skeleton lod
  for(INDEX islod=0;islod<ctslods;islod++) {
    SkeletonLOD &slod = skl_aSkeletonLODs[islod];
    // read source file name
    (*istrFile)>>slod.slod_fnSourceFile;
    // read MaxDistance
    (*istrFile)>>slod.slod_fMaxDistance;
    // read bone count
    INDEX ctb;
    (*istrFile)>>ctb;
    // create bone array
    slod.slod_aBones.New(ctb);
    // read skeleton bones
    for(INDEX ib=0;ib<ctb;ib++) {
      CTString strNameID;
      CTString strParentID;
      SkeletonBone &sb = slod.slod_aBones[ib];
      // read ID
      (*istrFile)>>strNameID;
      // read Parent ID
      (*istrFile)>>strParentID;
      //(*istrFile)>>slod.slod_aBones[ib].sb_iParentIndex ;
      sb.sb_iID = ska_GetIDFromStringTable(strNameID);
      sb.sb_iParentID = ska_GetIDFromStringTable(strParentID);
      // read AbsPlacement matrix
      for (int i = 0; i < 12; i++)
        (*istrFile)>>sb.sb_mAbsPlacement[i];
      // read RelPlacement Qvect stuct
      (*istrFile)>>sb.sb_qvRelPlacement;
      // read offset len
      (*istrFile)>>sb.sb_fOffSetLen;
      // read bone length
      (*istrFile)>>sb.sb_fBoneLength;
    }
  }
}

// Clear skeleton
void CSkeleton::Clear(void)
{
  // for each LOD
  for (INDEX islod=0; islod<skl_aSkeletonLODs.Count(); islod++) {
    // clear bones array
    skl_aSkeletonLODs[islod].slod_aBones.Clear();
  }
  // clear all lods
  skl_aSkeletonLODs.Clear();
}

// Count used memory
SLONG CSkeleton::GetUsedMemory(void)
{
  SLONG slMemoryUsed = sizeof(*this);
  INDEX ctslods = skl_aSkeletonLODs.Count();
  // for each skeleton lods
  for(INDEX islod=0;islod<ctslods;islod++) {
    SkeletonLOD &slod = skl_aSkeletonLODs[islod];
    slMemoryUsed += sizeof(slod);
    slMemoryUsed += slod.slod_aBones.Count() * sizeof(SkeletonBone);
  }
  return slMemoryUsed;
}
