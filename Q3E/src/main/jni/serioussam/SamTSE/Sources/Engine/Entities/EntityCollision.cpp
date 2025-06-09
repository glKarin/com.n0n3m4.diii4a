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
#include <Engine/Base/ListIterator.inl>
#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Clipping.inl>
#include <Engine/Brushes/Brush.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Network/Network.h>
#include <Engine/Network/SessionState.h>

class CClipTest {
public:
  CEntity *ct_penEntity;        // the entity
  CEntity *ct_penObstacle;      // obstacle entity (if cannot change)

  INDEX ct_iNewCollisionBox;    // index of new collision box to set
  CCollisionInfo ct_ciNew;      // collision info with new box
  FLOATaabbox3D ct_boxTotal;    // union box for old and new in absolute coordinates

  CListHead ct_lhActiveSectors; // sectors that may be of interest

  BOOL PointTouchesSphere(
    const FLOAT3D &vPoint,
    const FLOAT3D &vSphereCenter,
    const FLOAT fSphereRadius);
  BOOL PointTouchesCylinder(
    const FLOAT3D &vPoint,
    const FLOAT3D &vCylinderBottomCenter,
    const FLOAT3D &vCylinderTopCenter,
    const FLOAT fCylinderRadius);
  // project spheres of a collision info to given placement
  void ProjectSpheresToPlacement(CCollisionInfo &ci, 
    FLOAT3D &vPosition, FLOATmatrix3D &mRotation);

  // test if a sphere touches brush polygon
  BOOL SphereTouchesBrushPolygon(const CMovingSphere &msMoving,
                                      CBrushPolygon *pbpoPolygon);
  // test if entity touches brush polygon
  BOOL EntityTouchesBrushPolygon(CBrushPolygon *pbpoPolygon);
  // test if an entity can change to a new collision box without intersecting anything
  BOOL CanChange(CEntity *pen, INDEX iNewCollisionBox);

  ~CClipTest(void);

};

// test if an entity can change to a new collision box without intersecting anything
BOOL CanEntityChangeCollisionBox(CEntity *pen, INDEX iNewCollisionBox, CEntity **ppenObstacle)
{
  // if the entity is not linked to any sectors
  if (pen->en_rdSectors.IsEmpty()) {
    // make sure that the classification is ok
    pen->FindSectorsAroundEntity();
  }

  CClipTest ct;
  BOOL bCan = ct.CanChange(pen, iNewCollisionBox);
  *ppenObstacle = ct.ct_penObstacle;
  return bCan;
}
  // project spheres of a collision info to given placement
void CClipTest::ProjectSpheresToPlacement(CCollisionInfo &ci, 
  FLOAT3D &vPosition, FLOATmatrix3D &mRotation)
{
  // for each sphere
  FOREACHINSTATICARRAY(ci.ci_absSpheres, CMovingSphere, itms) {
    // project it in start point
    itms->ms_vRelativeCenter0 = itms->ms_vCenter*mRotation+vPosition;
  }
}

// test point to a sphere
BOOL CClipTest::PointTouchesSphere(
  const FLOAT3D &vPoint,
  const FLOAT3D &vSphereCenter,
  const FLOAT fSphereRadius)
{
  FLOAT fD = (vSphereCenter-vPoint).Length();
  return fD<fSphereRadius;
}

// test sphere to the edge (point to the edge cylinder)
BOOL CClipTest::PointTouchesCylinder(
  const FLOAT3D &vPoint,
  const FLOAT3D &vCylinderBottomCenter,
  const FLOAT3D &vCylinderTopCenter,
  const FLOAT fCylinderRadius)
{
  const FLOAT3D vCylinderBottomToTop       = vCylinderTopCenter - vCylinderBottomCenter;
  const FLOAT   fCylinderBottomToTopLength = vCylinderBottomToTop.Length();
  const FLOAT3D vCylinderDirection         = vCylinderBottomToTop/fCylinderBottomToTopLength;
  
  FLOAT3D vBottomToPoint = vPoint-vCylinderBottomCenter;
  FLOAT fPointL = vBottomToPoint%vCylinderDirection;

  // if not between top and bottom
  if (fPointL<0 || fPointL>fCylinderBottomToTopLength) {
    // doesn't touch
    return FALSE;
  }

  // find distance from point to cylinder axis
  FLOAT fD = (vBottomToPoint-vCylinderDirection*fPointL).Length();

  return fD<fCylinderRadius;
}

// test if a sphere touches brush polygon
BOOL CClipTest::SphereTouchesBrushPolygon(const CMovingSphere &msMoving,
                                    CBrushPolygon *pbpoPolygon)
{
  const FLOATplane3D &plPolygon = pbpoPolygon->bpo_pbplPlane->bpl_plAbsolute;
  // calculate point distance from polygon plane
  FLOAT fDistance = plPolygon.PointDistance(msMoving.ms_vRelativeCenter0);

  // if is further away than sphere radius
  if (fDistance>msMoving.ms_fR || fDistance<-msMoving.ms_fR) {
    // no collision
    return FALSE;
  }

  // calculate coordinate projected to the polygon plane
  FLOAT3D vPosMid = msMoving.ms_vRelativeCenter0;
  FLOAT3D vHitPoint = plPolygon.ProjectPoint(vPosMid);
  // find major axes of the polygon plane
  INDEX iMajorAxis1, iMajorAxis2;
  GetMajorAxesForPlane(plPolygon, iMajorAxis1, iMajorAxis2);

  // create an intersector
  CIntersector isIntersector(vHitPoint(iMajorAxis1), vHitPoint(iMajorAxis2));
  // for all edges in the polygon
  FOREACHINSTATICARRAY(pbpoPolygon->bpo_abpePolygonEdges, CBrushPolygonEdge,
    itbpePolygonEdge) {
    // get edge vertices (edge direction is irrelevant here!)
    const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
    const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
    // pass the edge to the intersector
    isIntersector.AddEdge(
      vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
      vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
  }
  // if the polygon is intersected by the ray
  if (isIntersector.IsIntersecting()) {
    return TRUE;
  }

  // for each edge in polygon
  FOREACHINSTATICARRAY(pbpoPolygon->bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
    // get edge vertices (edge direction is important here!)
    FLOAT3D vVertex0, vVertex1;
    itbpe->GetVertexCoordinatesAbsolute(vVertex0, vVertex1);

    // test sphere to the edge (point to the edge cylinder)
    if (PointTouchesCylinder(
          msMoving.ms_vRelativeCenter0, // point,
          vVertex0,                     // cylinder bottom center,
          vVertex1,                     // cylinder top center,
          msMoving.ms_fR                // cylinder radius
        )) {
      return TRUE;
    }
    // test sphere to the first vertex
    // NOTE: using point to sphere collision
    if (PointTouchesSphere(
          msMoving.ms_vRelativeCenter0,  // pount
          vVertex0,                      // sphere center
          msMoving.ms_fR                 // sphere radius
          )) {
      return TRUE;
    }
  }

  return FALSE;
}

// test if entity touches brush polygon
BOOL CClipTest::EntityTouchesBrushPolygon(CBrushPolygon *pbpoPolygon)
{
  // for each sphere
  FOREACHINSTATICARRAY(ct_ciNew.ci_absSpheres, CMovingSphere, itms) {
    // if it touches
    if (SphereTouchesBrushPolygon(*itms, pbpoPolygon)) {
      return TRUE;
    }
  }
  return FALSE;
}

// test if an entity can change to a new collision box without intersecting anything
BOOL CClipTest::CanChange(CEntity *pen, INDEX iNewCollisionBox)
{
  // can be used only for models
  ASSERT(
    pen->en_RenderType==CEntity::RT_MODEL ||
    pen->en_RenderType==CEntity::RT_EDITORMODEL ||
    pen->en_RenderType==CEntity::RT_SKAMODEL ||
    pen->en_RenderType==CEntity::RT_SKAEDITORMODEL);

  // safety check
  if (pen->en_pciCollisionInfo==NULL) {
    return FALSE;
  }

  // remember parameters
  ct_penEntity = pen;
  ct_iNewCollisionBox = iNewCollisionBox;
  ct_penObstacle = NULL;

  // create new temporary collision info
  ct_ciNew.FromModel(pen, iNewCollisionBox);
  // project it to entity placement
  ProjectSpheresToPlacement(ct_ciNew, 
    pen->en_plPlacement.pl_PositionVector, pen->en_mRotation);

  // get total bounding box encompassing both old and new collision boxes
  FLOATaabbox3D boxOld, boxNew;
  ASSERT(ct_penEntity->en_pciCollisionInfo!=NULL);
  CCollisionInfo &ciOld = *ct_penEntity->en_pciCollisionInfo;
  ciOld.MakeBoxAtPlacement(ct_penEntity->en_plPlacement.pl_PositionVector,
    ct_penEntity->en_mRotation, boxOld);
  ct_ciNew.MakeBoxAtPlacement(ct_penEntity->en_plPlacement.pl_PositionVector,
    ct_penEntity->en_mRotation, boxNew);

  ct_boxTotal  = boxOld;
  ct_boxTotal |= boxNew;

  // for each zoning sector that this entity is in
  {FOREACHSRCOFDST(ct_penEntity->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    // add it to list of active sectors
    ct_lhActiveSectors.AddTail(pbsc->bsc_lnInActiveSectors);
  ENDFOR}

  // for each active sector
  FOREACHINLIST(CBrushSector, bsc_lnInActiveSectors, ct_lhActiveSectors, itbsc) {
    // for non-zoning brush entities in the sector
    {FOREACHDSTOFSRC(itbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
      if (pen->en_RenderType!=CEntity::RT_BRUSH&&
          (_pNetwork->ga_ulDemoMinorVersion<=4 || pen->en_RenderType!=CEntity::RT_FIELDBRUSH)) {
        break;  // brushes are sorted first in list
      }

      // get first mip
      CBrushMip *pbm = pen->en_pbrBrush->GetFirstMip();
      // if brush mip exists for that mip factor
      if (pbm!=NULL) {
        // for each sector in the mip
        {FOREACHINDYNAMICARRAY(pbm->bm_abscSectors, CBrushSector, itbscNonZoning) {
          CBrushSector &bscNonZoning = *itbscNonZoning;
          // add it to list of active sectors
          if(!bscNonZoning.bsc_lnInActiveSectors.IsLinked()) {
            ct_lhActiveSectors.AddTail(bscNonZoning.bsc_lnInActiveSectors);
          }
        }}
      }
    ENDFOR}

    // if the sector's brush doesn't have collision
    CEntity *penSectorBrush = itbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    if (penSectorBrush->en_ulCollisionFlags==0 || 
      (_pNetwork->ga_ulDemoMinorVersion>2 && penSectorBrush->en_RenderType!=CEntity::RT_BRUSH) ) {
      // skip it
      continue;
    }

    // for each polygon in the sector
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      CBrushPolygon *pbpo = itbpo;
      // if its bbox has no contact with bbox to test
      if (!pbpo->bpo_boxBoundingBox.HasContactWith(ct_boxTotal) ) {
        // skip it
        continue;
      }

      // if it is passable
      if (pbpo->bpo_ulFlags&BPOF_PASSABLE) {
        // for each sector related to the portal
        {FOREACHDSTOFSRC(pbpo->bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbscRelated)
          // if the sector is not active
          if (!pbscRelated->bsc_lnInActiveSectors.IsLinked()) {
            // add it to active list
            ct_lhActiveSectors.AddTail(pbscRelated->bsc_lnInActiveSectors);
          }
        ENDFOR}

      // if it is not passable
      } else {
        // if entity touches it
        if (EntityTouchesBrushPolygon(pbpo)) {
          // test fails
          ct_penObstacle = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
          return FALSE;
        }
      }
    }
  }
  return TRUE;
}

CClipTest::~CClipTest(void)
{
  // clear list of active sectors
  {FORDELETELIST(CBrushSector, bsc_lnInActiveSectors, ct_lhActiveSectors, itbsc) {
    itbsc->bsc_lnInActiveSectors.Remove();
  }}
}
