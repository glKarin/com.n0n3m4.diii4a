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

#include <Engine/Entities/Entity.h>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Clipping.inl>
#include <Engine/Math/OBBox.h>
#include <Engine/Brushes/Brush.h>
#include <Engine/Templates/BSP.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>

static CEntity *penField;
static CBrushSector *_pbsc;
static CStaticStackArray<CEntity *> _apenActive;

static BOOL EntityIsInside(CEntity *pen)
{
  // get bounding sphere
  FLOAT fSphereRadius = pen->en_fSpatialClassificationRadius;
  const FLOAT3D &vSphereCenter = pen->en_plPlacement.pl_PositionVector;

  // if the entity touches sector's bounding box
  if (_pbsc->bsc_boxBoundingBox.TouchesSphere(vSphereCenter, fSphereRadius)) {
    // make oriented bounding box of the entity
    const FLOAT3D &v = pen->en_plPlacement.pl_PositionVector;
    const FLOATmatrix3D &m = pen->en_mRotation;
    FLOATobbox3D boxEntity = FLOATobbox3D(pen->en_boxSpatialClassification, v, m);
    DOUBLEobbox3D boxdEntity = FLOATtoDOUBLE(boxEntity);

    // if the box touches the sector's BSP
    if (boxEntity.HasContactWith(FLOATobbox3D(_pbsc->bsc_boxBoundingBox)) &&
      _pbsc->bsc_bspBSPTree.TestBox(boxdEntity)<=0) {

      // for each collision sphere
      CStaticArray<CMovingSphere> &absSpheres = pen->en_pciCollisionInfo->ci_absSpheres;
      for(INDEX iSphere=0; iSphere<absSpheres.Count(); iSphere++) {
        CMovingSphere &ms = absSpheres[iSphere];
        ms.ms_vRelativeCenter0 = ms.ms_vCenter*m+v;
        // if the sphere is in the sector
        if (_pbsc->bsc_bspBSPTree.TestSphere(
          FLOATtoDOUBLE(ms.ms_vRelativeCenter0), ms.ms_fR)<=0) {
          return TRUE;
        }
      }
    }
  }

  // otherwise it doesn't touch
  return FALSE;
}

// find first entity touching a field (this entity must be a field brush)
CEntity *CEntity::TouchingEntity(BOOL (*ConsiderEntity)(CEntity *), CEntity *penHintMaybeInside)
{
  // if not a field brush
  if (en_RenderType!=RT_FIELDBRUSH) {
    // error
    ASSERT(FALSE);
    return NULL;
  }

  // remember the entity and its first sector
  penField = this;
  CBrushMip *pbm = en_pbrBrush->GetBrushMipByDistance(0.0f);
  _pbsc = NULL;
  {FOREACHINDYNAMICARRAY(pbm->bm_abscSectors, CBrushSector, itbsc) {
    _pbsc = itbsc;
    break;
  }}
  // if illegal number of sectors
  if (_pbsc==NULL || pbm->bm_abscSectors.Count()>1) {
    // error
    CPrintF("Field doesn't have exactly one sector - ignoring!\n");
    return NULL;
  }

  // if a specific entity to check is given
  if (penHintMaybeInside!=NULL) {
    // if it is inside
    if (EntityIsInside(penHintMaybeInside)) {
      // return it
      return penHintMaybeInside;
    }
  }

  CEntity *penTouched = NULL;
  // no entities active initially
  _apenActive.PopAll();
  // for each zoning sector that this entity is in
  {FOREACHSRCOFDST(en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    // for all movable model entities that should be considered in the sector
    {FOREACHDSTOFSRC(pbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
      if (!(pen->en_ulPhysicsFlags&EPF_MOVABLE)
        || (pen->en_RenderType!=RT_MODEL&&pen->en_RenderType!=RT_EDITORMODEL)
        || (!ConsiderEntity(pen))) {
        continue;
      }
      // if already active
      if (pen->en_ulFlags&ENF_FOUNDINGRIDSEARCH) {
        // skip it
        continue;
      }
      // if it is inside
      if (EntityIsInside(pen)) {
        // stop the search
        penTouched = pen;
        break;
      }
      // add it to active
      _apenActive.Push() = pen;
      pen->en_ulFlags |= ENF_FOUNDINGRIDSEARCH;
    }}
  ENDFOR}

  // mark all as inactive
  {for (INDEX ien=0; ien<_apenActive.Count(); ien++) {
    CEntity *pen = _apenActive[ien];
    pen->en_ulFlags &= ~ENF_FOUNDINGRIDSEARCH;
  }}

  _apenActive.PopAll();

  return penTouched;
}
