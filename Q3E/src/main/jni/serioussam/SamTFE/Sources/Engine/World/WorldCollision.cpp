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

#include <Engine/World/World.h>
#include <Engine/World/WorldCollision.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/World/PhysicsProfile.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Brushes/Brush.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Math/Clipping.inl>
#include <Engine/Entities/EntityCollision.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Terrain/TerrainMisc.h>


// these are used for making projections for converting from X space to Y space this way:
//  MatrixMulT(mY, mX, mXToY);
//  VectMulT(mY, vX-vY, vXToY);

// C=AtxB
static inline void MatrixMulT(const FLOATmatrix3D &mA, const FLOATmatrix3D &mB, FLOATmatrix3D &mC)
{
  mC(1,1) = mA(1,1)*mB(1,1)+mA(2,1)*mB(2,1)+mA(3,1)*mB(3,1);
  mC(1,2) = mA(1,1)*mB(1,2)+mA(2,1)*mB(2,2)+mA(3,1)*mB(3,2);
  mC(1,3) = mA(1,1)*mB(1,3)+mA(2,1)*mB(2,3)+mA(3,1)*mB(3,3);

  mC(2,1) = mA(1,2)*mB(1,1)+mA(2,2)*mB(2,1)+mA(3,2)*mB(3,1);
  mC(2,2) = mA(1,2)*mB(1,2)+mA(2,2)*mB(2,2)+mA(3,2)*mB(3,2);
  mC(2,3) = mA(1,2)*mB(1,3)+mA(2,2)*mB(2,3)+mA(3,2)*mB(3,3);

  mC(3,1) = mA(1,3)*mB(1,1)+mA(2,3)*mB(2,1)+mA(3,3)*mB(3,1);
  mC(3,2) = mA(1,3)*mB(1,2)+mA(2,3)*mB(2,2)+mA(3,3)*mB(3,2);
  mC(3,3) = mA(1,3)*mB(1,3)+mA(2,3)*mB(2,3)+mA(3,3)*mB(3,3);
}
// v2 = Mt*v1
static inline void VectMulT(const FLOATmatrix3D &mM, const FLOAT3D &vV1, FLOAT3D &vV2)
{
  vV2(1) = vV1(1)*mM(1,1)+vV1(2)*mM(2,1)+vV1(3)*mM(3,1);
  vV2(2) = vV1(1)*mM(1,2)+vV1(2)*mM(2,2)+vV1(3)*mM(3,2);
  vV2(3) = vV1(1)*mM(1,3)+vV1(2)*mM(2,3)+vV1(3)*mM(3,3);
}

/////////////////////////////////////////////////////////////////////
// CClipMove

// get start and end positions of an entity in this tick
inline void CClipMove::GetPositionsOfEntity(
  CEntity *pen, FLOAT3D &v0, FLOATmatrix3D &m0, FLOAT3D &v1, FLOATmatrix3D &m1)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_GETPOSITIONSOFENTITY);

  // start is where entity is now
  v0 = pen->en_plPlacement.pl_PositionVector;
  m0 = pen->en_mRotation;
  // if entity is movable
  if (pen->en_ulPhysicsFlags&EPF_MOVABLE) {
    // get end position from movable entity
    CMovableEntity *penMovable = (CMovableEntity*)pen;
    v1 = penMovable->en_vNextPosition;
    m1 = penMovable->en_mNextRotation;

    // NOTE: this prevents movable entities from hanging in the air when a brush moves
    // beneath their feet

    // if moving entity is reference of this entity
    if (penMovable->en_penReference == cm_penMoving)  {
      // add this entity to list of movers
      penMovable->AddToMoversDuringMoving();
    }

  // if entity is not movable
  } else {
    // end position is same as start
    v1 = v0;
    m1 = m0;
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_GETPOSITIONSOFENTITY);
}

/*
 * Constructor.
 */
CClipMove::CClipMove(CMovableEntity *penEntity)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_PREPARECLIPMOVE);

  // clear last-hit statistics
  cm_penHit = NULL;
  cm_pbpoHit = NULL;
  cm_fMovementFraction = 2.0f;

  cm_penMoving = penEntity;
  // if the entity is deleted, or couldn't possible collide with anything
  if ((cm_penMoving->en_ulFlags&ENF_DELETED)
    ||!(cm_penMoving->en_ulCollisionFlags&ECF_TESTMASK)
    ||cm_penMoving->en_pciCollisionInfo==NULL) {
    // do nothing
    _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PREPARECLIPMOVE);
    return;
  }

  // if entity is model
  if (penEntity->en_RenderType==CEntity::RT_MODEL ||
      penEntity->en_RenderType==CEntity::RT_EDITORMODEL || 
      penEntity->en_RenderType==CEntity::RT_SKAMODEL ||
      penEntity->en_RenderType==CEntity::RT_SKAEDITORMODEL ) {
    cm_bMovingBrush = FALSE;

    // remember entity and placements
    cm_penA = penEntity;
    GetPositionsOfEntity(cm_penA, cm_vA0, cm_mA0, cm_vA1, cm_mA1);

    // create spheres for the entity
    ASSERT(penEntity->en_pciCollisionInfo!=NULL);
    cm_pamsA = &penEntity->en_pciCollisionInfo->ci_absSpheres;

    // create aabbox for entire movement path
    FLOATaabbox3D box0, box1;
    penEntity->en_pciCollisionInfo->MakeBoxAtPlacement(cm_vA0, cm_mA0, box0);
    penEntity->en_pciCollisionInfo->MakeBoxAtPlacement(cm_vA1, cm_mA1, box1);
    cm_boxMovementPath  = box0;
    cm_boxMovementPath |= box1;

  // if entity is brush
  } else if (penEntity->en_RenderType==CEntity::RT_BRUSH) {
    cm_bMovingBrush = TRUE;

    // remember entity and placements
    cm_penB = penEntity;
    GetPositionsOfEntity(cm_penB, cm_vB0, cm_mB0, cm_vB1, cm_mB1);

    // create spheres for the entity
    ASSERT(penEntity->en_pciCollisionInfo!=NULL);
    // create aabbox for entire movement path
    FLOATaabbox3D box0, box1;
    penEntity->en_pciCollisionInfo->MakeBoxAtPlacement(cm_vB0, cm_mB0, box0);
    penEntity->en_pciCollisionInfo->MakeBoxAtPlacement(cm_vB1, cm_mB1, box1);
    cm_boxMovementPath  = box0;
    cm_boxMovementPath |= box1;

  } else {
    ASSERT(FALSE);
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PREPARECLIPMOVE);
}

// send pass if needed
inline BOOL CClipMove::SendPassEvent(CEntity *penTested)
{
  BOOL bSent = FALSE;
  if (cm_ulPassMaskA & penTested->en_ulCollisionFlags) {

    EPass ePassA;
    ePassA.penOther = penTested;
    ePassA.bThisMoved = TRUE;
    cm_penMoving->SendEvent(ePassA);

    bSent = TRUE;
  }
  if (cm_ulPassMaskB & penTested->en_ulCollisionFlags) {

    EPass ePassB;
    ePassB.penOther = cm_penMoving;
    ePassB.bThisMoved = FALSE;
    penTested->SendEvent(ePassB);

    bSent = TRUE;
  }

  return bSent;
}

/*
 * Clip a moving point to a sphere, update collision data.
 */
inline void CClipMove::ClipMovingPointToSphere(
  const FLOAT3D &vStart,
  const FLOAT3D &vEnd,
  const FLOAT3D &vSphereCenter,
  const FLOAT fSphereRadius)
{
  const FLOAT3D vSphereCenterToStart = vStart - vSphereCenter;
  const FLOAT3D vStartToEnd          = vEnd - vStart;
  // calculate discriminant for intersection parameters
  const FLOAT fP = ((vStartToEnd%vSphereCenterToStart)/(vStartToEnd%vStartToEnd));
  const FLOAT fQ = (((vSphereCenterToStart%vSphereCenterToStart)
    - (fSphereRadius*fSphereRadius))/(vStartToEnd%vStartToEnd));
  const FLOAT fD = fP*fP-fQ;
  // if it is less than zero
  if (fD<0) {
    // no collision will occur
    return;
  }
  // calculate intersection parameters
  const FLOAT fSqrtD = sqrt(fD);
  const FLOAT fLambda1 = -fP+fSqrtD;
  const FLOAT fLambda2 = -fP-fSqrtD;
  // use lower one
  const FLOAT fMinLambda = Min(fLambda1, fLambda2);
  // if it is betwen zero and last collision found
  if (0.0f<=fMinLambda && fMinLambda<cm_fMovementFraction) {
    _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_SPHERETOSPHEREHITS);

    // if cannot pass
    if (!SendPassEvent(cm_penTested)) {
      // mark this as the new closest found collision point
      cm_fMovementFraction = fMinLambda;
      cm_vClippedLine = (vStartToEnd*(1.0f-fMinLambda))*cm_mBToAbsolute;
      ASSERT(cm_vClippedLine.Length()<100.0f);
      FLOAT3D vCollisionPoint = vStartToEnd*fMinLambda + vStart;
      FLOAT3D vCollisionNormal = vCollisionPoint - vSphereCenter;
      FLOATplane3D plClippedPlane(vCollisionNormal, vCollisionPoint);
      // project the collision plane from space B to absolute space
      cm_plClippedPlane = plClippedPlane*cm_mBToAbsolute+cm_vBToAbsolute;
      // remember hit entity
      cm_penHit = cm_penTested;
      cm_pbpoHit = cm_pbpoTested;
    }
  }
}

/*
 * Clip a moving point to a cylinder, update collision data.
 */
inline void CClipMove::ClipMovingPointToCylinder(
  const FLOAT3D &vStart,
  const FLOAT3D &vEnd,
  const FLOAT3D &vCylinderBottomCenter,
  const FLOAT3D &vCylinderTopCenter,
  const FLOAT fCylinderRadius)
{
  const FLOAT3D vStartToEnd                = vEnd - vStart;
  const FLOAT3D vCylinderBottomToStart     = vStart - vCylinderBottomCenter;

  const FLOAT3D vCylinderBottomToTop       = vCylinderTopCenter - vCylinderBottomCenter;
  const FLOAT   fCylinderBottomToTopLength = vCylinderBottomToTop.Length();
  const FLOAT3D vCylinderDirection         = vCylinderBottomToTop/fCylinderBottomToTopLength;

  const FLOAT3D vB = vStartToEnd - vCylinderDirection*(vCylinderDirection%vStartToEnd);
  const FLOAT3D vC = vCylinderBottomToStart - vCylinderDirection*
    (vCylinderDirection%vCylinderBottomToStart);

  const FLOAT fP = (vB%vC)/(vB%vB);
  const FLOAT fQ = (vC%vC-fCylinderRadius*fCylinderRadius)/(vB%vB);

  const FLOAT fD = fP*fP-fQ;
  // if it is less than zero
  if (fD<0) {
    // no collision will occur
    return;
  }
  // calculate intersection parameters
  const FLOAT fSqrtD = sqrt(fD);
  const FLOAT fLambda1 = -fP+fSqrtD;
  const FLOAT fLambda2 = -fP-fSqrtD;
  // use lower one
  const FLOAT fMinLambda = Min(fLambda1, fLambda2);
  // if it is betwen zero and last collision found
  if (0.0f<=fMinLambda && fMinLambda<cm_fMovementFraction) {
    // calculate the collision point
    FLOAT3D vCollisionPoint = vStartToEnd*fMinLambda + vStart;
    // create plane at cylinder bottom
    FLOATplane3D plCylinderBottom(vCylinderBottomToTop, vCylinderBottomCenter);
    // find distance of the collision point from the bottom plane
    FLOAT fCollisionToBottomPlaneDistance = plCylinderBottom.PointDistance(vCollisionPoint);
    // if the point is between bottom and top of cylinder
    if (0<=fCollisionToBottomPlaneDistance
      &&fCollisionToBottomPlaneDistance<fCylinderBottomToTopLength) {

      // if cannot pass
      if (!SendPassEvent(cm_penTested)) {
        // mark this as the new closest found collision point
        cm_fMovementFraction = fMinLambda;
        cm_vClippedLine = (vStartToEnd*(1.0f-fMinLambda))*cm_mBToAbsolute;
        ASSERT(cm_vClippedLine.Length()<100.0f);
        FLOAT3D vCollisionNormal = plCylinderBottom.ProjectPoint(vCollisionPoint)
          - vCylinderBottomCenter;
        FLOATplane3D plClippedPlane(vCollisionNormal, vCollisionPoint);
        // project the collision plane from space B to absolute space
        cm_plClippedPlane = plClippedPlane*cm_mBToAbsolute+cm_vBToAbsolute;
        // remember hit entity
        cm_penHit = cm_penTested;
        cm_pbpoHit = cm_pbpoTested;
      }
    }
  }
}

/*
 * Clip a moving sphere to a standing sphere, update collision data.
 */
void CClipMove::ClipMovingSphereToSphere(const CMovingSphere &msMoving,
  const CMovingSphere &msStanding)
{
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_SPHERETOSPHERETESTS);
  // use moving point to sphere collision with sum of sphere radii
  ClipMovingPointToSphere(
      msMoving.ms_vRelativeCenter0,       // start
      msMoving.ms_vRelativeCenter1,       // end
      msStanding.ms_vCenter,              // sphere center
      msMoving.ms_fR + msStanding.ms_fR   // sphere radius
    );
}
/*
 * Clip a moving sphere to a brush polygon, update collision data.
 */
void CClipMove::ClipMovingSphereToBrushPolygon(const CMovingSphere &msMoving,
                                               CBrushPolygon *pbpoPolygon)
{
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_SPHERETOPOLYGONTESTS);
  cm_pbpoTested = pbpoPolygon;
  const FLOATplane3D &plPolygon = pbpoPolygon->bpo_pbplPlane->bpl_plRelative;
  // calculate point distances from polygon plane
  FLOAT fDistance0 = plPolygon.PointDistance(msMoving.ms_vRelativeCenter0)-msMoving.ms_fR;
  FLOAT fDistance1 = plPolygon.PointDistance(msMoving.ms_vRelativeCenter1)-msMoving.ms_fR;

  // if first point is in front and second point is behind
  if (fDistance0>=0 && fDistance1<0) {
    // calculate fraction of line before intersection
    FLOAT fFraction = fDistance0/(fDistance0-fDistance1);
    ASSERT(fFraction>=0.0f && fFraction<=1.0f);

    // if fraction is less than minimum found fraction
    if (fFraction<cm_fMovementFraction) {
      // calculate intersection coordinate, projected to the polygon plane
      FLOAT3D vPosMid = msMoving.ms_vRelativeCenter0+(msMoving.ms_vRelativeCenter1-msMoving.ms_vRelativeCenter0)*fFraction;
      FLOAT3D vHitPoint = plPolygon.ProjectPoint(vPosMid);
      // find major axes of the polygon plane
      INDEX iMajorAxis1, iMajorAxis2;
      GetMajorAxesForPlane(plPolygon, iMajorAxis1, iMajorAxis2);

      // create an intersector
      CIntersector isIntersector(vHitPoint(iMajorAxis1), vHitPoint(iMajorAxis2));
      // for all edges in the polygon
      /*FOREACHINSTATICARRAY(pbpoPolygon->bpo_abpePolygonEdges, CBrushPolygonEdge,
        itbpePolygonEdge) {*/
      CBrushPolygonEdge *itbpePolygonEdge = pbpoPolygon->bpo_abpePolygonEdges.sa_Array;
      int i;
      const int count = pbpoPolygon->bpo_abpePolygonEdges.sa_Count;
      for (i = 0; i < count; i++, itbpePolygonEdge++) {
        // get edge vertices (edge direction is irrelevant here!)
        const FLOAT3D &vVertex0 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex0->bvx_vRelative;
        const FLOAT3D &vVertex1 = itbpePolygonEdge->bpe_pbedEdge->bed_pbvxVertex1->bvx_vRelative;
        // pass the edge to the intersector
        isIntersector.AddEdge(
          vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
          vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
      }
      // if the polygon is intersected by the ray
      if (isIntersector.IsIntersecting()) {
        // if cannot pass
        if (!SendPassEvent(cm_penTested)) {
          // mark this as the new closest found collision point
          cm_fMovementFraction = fFraction;
          cm_vClippedLine = msMoving.ms_vRelativeCenter1 - vPosMid;
          ASSERT(cm_vClippedLine.Length()<100.0f);
          // project the collision plane from space B to absolute space
          // only the normal of the plane is correct, not the distance!!!!
          cm_plClippedPlane = plPolygon*cm_mBToAbsolute+cm_vBToAbsolute;
          // remember hit entity
          cm_penHit = cm_penTested;
          cm_pbpoHit = cm_pbpoTested;
        }
      }
    }
  }

  // for each edge in polygon
  //FOREACHINSTATICARRAY(pbpoPolygon->bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
  CBrushPolygonEdge *itbpe = pbpoPolygon->bpo_abpePolygonEdges.sa_Array;
  int i;
  const int count = pbpoPolygon->bpo_abpePolygonEdges.sa_Count;
  for (i = 0; i < count; i++, itbpe++) {
    // get edge vertices (edge direction is important here!)
    FLOAT3D vVertex0, vVertex1;
    itbpe->GetVertexCoordinatesRelative(vVertex0, vVertex1);

    // clip moving sphere to the edge (moving point to the edge cylinder)
    ClipMovingPointToCylinder(
      msMoving.ms_vRelativeCenter0, // start,
      msMoving.ms_vRelativeCenter1, // end,
      vVertex0,                     // cylinder bottom center,
      vVertex1,                     // cylinder top center,
      msMoving.ms_fR                // cylinder radius
    );
    // clip moving sphere to the first vertex
    // NOTE: use moving point to sphere collision
    ClipMovingPointToSphere(
        msMoving.ms_vRelativeCenter0,  // start
        msMoving.ms_vRelativeCenter1,  // end
        vVertex0,                      // sphere center
        msMoving.ms_fR                 // sphere radius
      );
  }
}

/* Clip a moving sphere to a terrain polygon, update collision data. */
void CClipMove::ClipMovingSphereToTerrainPolygon(
  const CMovingSphere &msMoving, const FLOAT3D &v0, const FLOAT3D &v1, const FLOAT3D &v2)
{
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_SPHERETOPOLYGONTESTS);
  cm_pbpoTested = NULL;

  const FLOATplane3D plPolygon = FLOATplane3D(v0,v1,v2);

  // calculate point distances from polygon plane
  FLOAT fDistance0 = plPolygon.PointDistance(msMoving.ms_vRelativeCenter0)-msMoving.ms_fR;
  FLOAT fDistance1 = plPolygon.PointDistance(msMoving.ms_vRelativeCenter1)-msMoving.ms_fR;

  // if first point is in front and second point is behind
  if (fDistance0>=0 && fDistance1<0) {
    // calculate fraction of line before intersection
    FLOAT fFraction = fDistance0/(fDistance0-fDistance1);
    ASSERT(fFraction>=0.0f && fFraction<=1.0f);

    // if fraction is less than minimum found fraction
    if (fFraction<cm_fMovementFraction) {
      // calculate intersection coordinate, projected to the polygon plane
      FLOAT3D vPosMid = msMoving.ms_vRelativeCenter0+(msMoving.ms_vRelativeCenter1-msMoving.ms_vRelativeCenter0)*fFraction;
      FLOAT3D vHitPoint = plPolygon.ProjectPoint(vPosMid);
      // find major axes of the polygon plane
      INDEX iMajorAxis1, iMajorAxis2;
      GetMajorAxesForPlane(plPolygon, iMajorAxis1, iMajorAxis2);

      // create an intersector
      CIntersector isIntersector(vHitPoint(iMajorAxis1), vHitPoint(iMajorAxis2));

      // for all edges in the polygon, pass the edge to the intersector
      isIntersector.AddEdge(v0(iMajorAxis1), v0(iMajorAxis2), v1(iMajorAxis1), v1(iMajorAxis2));
      isIntersector.AddEdge(v1(iMajorAxis1), v1(iMajorAxis2), v2(iMajorAxis1), v2(iMajorAxis2));
      isIntersector.AddEdge(v2(iMajorAxis1), v2(iMajorAxis2), v0(iMajorAxis1), v0(iMajorAxis2));

      // if the polygon is intersected by the ray
      if (isIntersector.IsIntersecting()) {
        // if cannot pass
        if (!SendPassEvent(cm_penTested)) {
          // mark this as the new closest found collision point
          cm_fMovementFraction = fFraction;
          cm_vClippedLine = msMoving.ms_vRelativeCenter1 - vPosMid;
          ASSERT(cm_vClippedLine.Length()<100.0f);
          // project the collision plane from space B to absolute space
          // only the normal of the plane is correct, not the distance!!!!
          cm_plClippedPlane = plPolygon*cm_mBToAbsolute+cm_vBToAbsolute;
          // remember hit entity
          cm_penHit = cm_penTested;
          cm_pbpoHit = cm_pbpoTested;
        }
      }
    }
  }

  // for all edges in the polygon, clip moving sphere to the edge (moving point to the edge cylinder)
  ClipMovingPointToCylinder(
    msMoving.ms_vRelativeCenter0, // start,
    msMoving.ms_vRelativeCenter1, // end,
    v0,                     // cylinder bottom center,
    v1,                     // cylinder top center,
    msMoving.ms_fR                // cylinder radius
  );
  ClipMovingPointToCylinder(
    msMoving.ms_vRelativeCenter0, // start,
    msMoving.ms_vRelativeCenter1, // end,
    v1,                     // cylinder bottom center,
    v2,                     // cylinder top center,
    msMoving.ms_fR                // cylinder radius
  );
  ClipMovingPointToCylinder(
    msMoving.ms_vRelativeCenter0, // start,
    msMoving.ms_vRelativeCenter1, // end,
    v2,                     // cylinder bottom center,
    v0,                     // cylinder top center,
    msMoving.ms_fR                // cylinder radius
  );

  // for each edge in polygon, clip moving sphere to the first vertex
  // NOTE: use moving point to sphere collision
  ClipMovingPointToSphere(
      msMoving.ms_vRelativeCenter0,  // start
      msMoving.ms_vRelativeCenter1,  // end
      v0,                      // sphere center
      msMoving.ms_fR                 // sphere radius
    );
  ClipMovingPointToSphere(
      msMoving.ms_vRelativeCenter0,  // start
      msMoving.ms_vRelativeCenter1,  // end
      v1,                     // sphere center
      msMoving.ms_fR                 // sphere radius
    );
  ClipMovingPointToSphere(
      msMoving.ms_vRelativeCenter0,  // start
      msMoving.ms_vRelativeCenter1,  // end
      v2,                      // sphere center
      msMoving.ms_fR                 // sphere radius
    );
}

/* Clip movement to a terrain polygon. */
void CClipMove::ClipMoveToTerrainPolygon(const FLOAT3D &v0, const FLOAT3D &v1, const FLOAT3D &v2)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHPOLYGON);
  // for each sphere of entity A
  //FOREACHINSTATICARRAY(*cm_pamsA, CMovingSphere, itmsMoving) {
    CMovingSphere *itmsMoving = cm_pamsA->sa_Array;
  int i;
  const int count = cm_pamsA->sa_Count;
  for (i = 0; i < count; i++, itmsMoving++) {
  // clip moving sphere to the polygon
    ClipMovingSphereToTerrainPolygon(*itmsMoving, v0, v1, v2);
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHPOLYGON);
}

/*
 * Clip movement to a brush polygon.
 */
void CClipMove::ClipMoveToBrushPolygon(CBrushPolygon *pbpoPolygon)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHPOLYGON);
  // for each sphere of entity A
  //FOREACHINSTATICARRAY(*cm_pamsA, CMovingSphere, itmsMoving) {
  CMovingSphere *itmsMoving = cm_pamsA->sa_Array;
  int i;
  const int count = cm_pamsA->sa_Count;
  for (i = 0; i < count; i++, itmsMoving++) {
    // clip moving sphere to the polygon
    ClipMovingSphereToBrushPolygon(*itmsMoving, pbpoPolygon);
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHPOLYGON);
}

/*
 * Project spheres of moving entity to standing entity space.
 */
void CClipMove::ProjectASpheresToB(void)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_PROJECTASPHERESTOB);
  // for each sphere
  FOREACHINSTATICARRAY(*cm_pamsA, CMovingSphere, itmsA) {
    // project it in start point
    itmsA->ms_vRelativeCenter0 = itmsA->ms_vCenter*cm_mAToB0+cm_vAToB0;
    // project it in end point
    itmsA->ms_vRelativeCenter1 = itmsA->ms_vCenter*cm_mAToB1+cm_vAToB1;
    // make bounding box
    itmsA->ms_boxMovement = FLOATaabbox3D(itmsA->ms_vRelativeCenter0, itmsA->ms_vRelativeCenter1);
    itmsA->ms_boxMovement.Expand(itmsA->ms_fR);
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PROJECTASPHERESTOB);
}

/* Find movement box in absolute space for A entity. */
void CClipMove::FindAbsoluteMovementBoxForA(void)
{
/*
  // position at beginning of movement is absolute position
  FLOAT3D &vPosition0 = cm_vA0;
  FLOATmatrix3D &mRotation0 = cm_mA0;

  FLOATmatrix3D mB0Abs, mB1Abs;
  FLOAT3D vB0Abs, vB1Abs;

  // make absolute positions of B0 and B1
  mB0Abs.Diagonal(1.0f);
  vB0Abs = FLOAT3D(0,0,0);

  vAToB0
  mAToB0

  MatrixMulT(cm_mB1, cm_mB0, mB1Abs);
  VectMulT(cm_mB1, cm_vB0-cm_vB1, vB1Abs);

// these are used for making projections for converting from X space to Y space this way:
//  MatrixMulT(mY, mX, mXToY);
//  VectMulT(mY, vX-vY, vXToY);

  FLOAT3D vPosition1;
  FLOATmatrix3D mRotation1;

  MatrixMulT(mB1Abs, cm_mA0, mRotation1);
  VectMulT(mB1Abs, cm_vA0-vB1Abs, vPosition1);

  FLOATaabbox3D box0, box1;
  cm_penA->en_pciCollisionInfo->MakeBoxAtPlacement(vPosition0, mRotation0, box0);
  cm_penA->en_pciCollisionInfo->MakeBoxAtPlacement(vPosition1, mRotation1, box1);
  cm_boxMovementPathAbsoluteA  = box0;
  cm_boxMovementPathAbsoluteA |= box1;
*/

  cm_boxMovementPathAbsoluteA = FLOATaabbox3D();
  // for each sphere
  FOREACHINSTATICARRAY(*cm_pamsA, CMovingSphere, itmsA) {
    // project it in start point
    FLOAT3D v0 = (itmsA->ms_vCenter*cm_mAToB0+cm_vAToB0)*cm_mB0+cm_vB0;
    // project it in end point
    FLOAT3D v1 = (itmsA->ms_vCenter*cm_mAToB1+cm_vAToB1)*cm_mB0+cm_vB0;
    // make bounding box
    FLOATaabbox3D box = FLOATaabbox3D(v0, v1);
    box.Expand(itmsA->ms_fR);
    cm_boxMovementPathAbsoluteA|=box;
  }
}

/*
 * Clip movement if B is a model.
 */
void CClipMove::ClipModelMoveToModel(void)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMODELMOVETOMODEL);
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_MODELMODELTESTS);

  // assumes that all spheres in one entity have same radius
  FLOAT fRB = (*cm_pamsB)[0].ms_fR;

  // for each sphere in entity A
  FOREACHINSTATICARRAY(*cm_pamsA, CMovingSphere, itmsA) {
    CMovingSphere &msA = *itmsA;
    FLOATaabbox3D &boxMovingSphere = msA.ms_boxMovement;

    // for each sphere in entity B
    FOREACHINSTATICARRAY(*cm_pamsB, CMovingSphere, itmsB) {
      CMovingSphere &msB = *itmsB;
      // if the sphere is too far
      if (
        (boxMovingSphere.Min()(1)>msB.ms_vCenter(1)+fRB) ||
        (boxMovingSphere.Max()(1)<msB.ms_vCenter(1)-fRB) ||
        (boxMovingSphere.Min()(2)>msB.ms_vCenter(2)+fRB) ||
        (boxMovingSphere.Max()(2)<msB.ms_vCenter(2)-fRB) ||
        (boxMovingSphere.Min()(3)>msB.ms_vCenter(3)+fRB) ||
        (boxMovingSphere.Max()(3)<msB.ms_vCenter(3)-fRB)) {
        // skip it
        continue;
      }
      // clip sphere A to sphere B
      ClipMovingSphereToSphere(msA, msB);
    }
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMODELMOVETOMODEL);
}

/*
 * Clip movement if B is a brush.
 */
void CClipMove::ClipBrushMoveToModel(void)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPBRUSHMOVETOMODEL);
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_MODELBRUSHTESTS);
  // get first mip of the brush
  CBrushMip *pbmMip = cm_penB->en_pbrBrush->GetFirstMip();
  // for each sector in the brush mip
  FOREACHINDYNAMICARRAY(pbmMip->bm_abscSectors, CBrushSector, itbsc) {
    // if the sector's bbox has no contact with bbox of movement path
    if ( !itbsc->bsc_boxBoundingBox.HasContactWith(cm_boxMovementPathAbsoluteA, 0.01f) ) {
      // skip it
      continue;
    }
    // for each polygon in the sector
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      // if it is passable or its bbox has no contact with bbox of movement path
      if ((itbpo->bpo_ulFlags&BPOF_PASSABLE)
          ||!itbpo->bpo_boxBoundingBox.HasContactWith(cm_boxMovementPathAbsoluteA, 0.01f) ) {
        // skip it
        continue;
      }
      // clip movement to the polygon
      ClipMoveToBrushPolygon(itbpo);
    }
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPBRUSHMOVETOMODEL);
}

/*
 * Prepare projections and spheres for movement clipping.
 */
void CClipMove::PrepareProjectionsAndSpheres(void)
{
 // Formula: C=AxB --> Cij=Sum(s=1..k)(Ais*Bsj)

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_PREPAREPROJECTIONSANDSPHERES);
  // make projections for converting from A space to B space
  MatrixMulT(cm_mB0, cm_mA0, cm_mAToB0);
  VectMulT(cm_mB0, cm_vA0-cm_vB0, cm_vAToB0);
  MatrixMulT(cm_mB1, cm_mA1, cm_mAToB1);
  VectMulT(cm_mB1, cm_vA1-cm_vB1, cm_vAToB1);

  // projection for converting from B space to absolute space
  cm_mBToAbsolute = cm_mB0;
  cm_vBToAbsolute = cm_vB0;

  // project spheres of entity A to space B
  ProjectASpheresToB();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_PREPAREPROJECTIONSANDSPHERES);
}

/*
 * Clip movement to a model entity.
 */
void CClipMove::ClipMoveToModel(CEntity *penModel)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOMODEL);
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_MODELXTESTS);

  // if not possibly colliding
  ASSERT(penModel->en_pciCollisionInfo!=NULL);
  const FLOATaabbox3D &boxModel = penModel->en_pciCollisionInfo->ci_boxCurrent;
  if (
    (cm_boxMovementPath.Min()(1)>boxModel.Max()(1)) ||
    (cm_boxMovementPath.Max()(1)<boxModel.Min()(1)) ||
    (cm_boxMovementPath.Min()(2)>boxModel.Max()(2)) ||
    (cm_boxMovementPath.Max()(2)<boxModel.Min()(2)) ||
    (cm_boxMovementPath.Min()(3)>boxModel.Max()(3)) ||
    (cm_boxMovementPath.Max()(3)<boxModel.Min()(3))) {
    // do nothing
    _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOMODEL);
    return;
  }

  // remember tested entity
  cm_penTested = penModel;
  cm_pbpoTested = NULL;

  // if clipping a moving model
  if (!cm_bMovingBrush) {
    // moving model is A and other model is B
    cm_penB = penModel;
    GetPositionsOfEntity(cm_penB, cm_vB0, cm_mB0, cm_vB1, cm_mB1);
    // create bounding spheres for the model
    ASSERT(penModel->en_pciCollisionInfo!=NULL);
    cm_pamsB = &penModel->en_pciCollisionInfo->ci_absSpheres;

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOMODELNONTRIVIAL);
    // prepare new projections and spheres
    PrepareProjectionsAndSpheres();
    // clip model to model
    ClipModelMoveToModel();
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOMODELNONTRIVIAL);

  // if clipping a moving brush
  } else {
    // moving brush is B and still model is A
    cm_penA = penModel;
    GetPositionsOfEntity(cm_penA, cm_vA0, cm_mA0, cm_vA1, cm_mA1);
    // create bounding spheres for the model
    ASSERT(penModel->en_pciCollisionInfo!=NULL);
    cm_pamsA = &penModel->en_pciCollisionInfo->ci_absSpheres;

    // prepare new projections and spheres
    PrepareProjectionsAndSpheres();
    FindAbsoluteMovementBoxForA();
    // clip brush to model
    ClipBrushMoveToModel();
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOMODEL);
}

/* Cache near polygons of movable entity. */
void CClipMove::CacheNearPolygons(void)
{
  // if movement box is still inside cached box
  if (cm_boxMovementPath<=cm_penMoving->en_boxNearCached) {
    // do nothing
    return;
  }
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS);
  _pfPhysicsProfile.IncrementTimerAveragingCounter(
    CPhysicsProfile::PTI_CACHENEARPOLYGONS, 1);


  FLOATaabbox3D &box = cm_penMoving->en_boxNearCached;
  CStaticStackArray<CBrushPolygon *> &apbpo = cm_penMoving->en_apbpoNearPolygons;

  // flush old cached polygons
  apbpo.PopAll();
  // set new box to union of movement box and future estimate
  box  = cm_boxMovementPath;
  box |= cm_penMoving->en_boxMovingEstimate;

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_ADDINITIAL);
  // for each zoning sector that this entity is in
  {FOREACHSRCOFDST(cm_penMoving->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    // add it to list of active sectors
    cm_lhActiveSectors.AddTail(pbsc->bsc_lnInActiveSectors);
  ENDFOR}
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_ADDINITIAL);

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_MAINLOOP);
  // for each active sector
  FOREACHINLIST(CBrushSector, bsc_lnInActiveSectors, cm_lhActiveSectors, itbsc) {
  _pfPhysicsProfile.IncrementTimerAveragingCounter(
    CPhysicsProfile::PTI_CACHENEARPOLYGONS_MAINLOOP, 1);
    // for each polygon in the sector
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      CBrushPolygon *pbpo = itbpo;
      // if its bbox has no contact with bbox to cache
      if (!pbpo->bpo_boxBoundingBox.HasContactWith(box) ) {
        // skip it
        continue;
      }
      _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_MAINLOOPFOUND);
      _pfPhysicsProfile.IncrementTimerAveragingCounter(
        CPhysicsProfile::PTI_CACHENEARPOLYGONS_MAINLOOPFOUND, 1);
      // add it to cache
      apbpo.Push() = pbpo;
      // if it is passable
      if (pbpo->bpo_ulFlags&BPOF_PASSABLE) {
        // for each sector related to the portal
        {FOREACHDSTOFSRC(pbpo->bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbscRelated)
          // if the sector is not active
          if (!pbscRelated->bsc_lnInActiveSectors.IsLinked()) {
            // add it to active list
            cm_lhActiveSectors.AddTail(pbscRelated->bsc_lnInActiveSectors);
          }
        ENDFOR}
      }
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_MAINLOOPFOUND);
    }

    // for non-zoning non-movable brush entities in the sector
    {FOREACHDSTOFSRC(itbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
      if (pen->en_RenderType==CEntity::RT_TERRAIN) {
        continue;
      }
      if (pen->en_RenderType!=CEntity::RT_BRUSH&&
          pen->en_RenderType!=CEntity::RT_FIELDBRUSH) {
        break;  // brushes are sorted first in list
      }
      if(pen->en_ulPhysicsFlags&EPF_MOVABLE) {
        continue;
      }
      if(!MustTest(pen)) {
        continue;
      }

      _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDNONZONING);
      // get first mip
      CBrushMip *pbm = pen->en_pbrBrush->GetFirstMip();
      // if brush mip exists for that mip factor
      if (pbm!=NULL) {
        // for each sector in the mip
        {FOREACHINDYNAMICARRAY(pbm->bm_abscSectors, CBrushSector, itbscNonZoning) {
          CBrushSector &bscNonZoning = *itbscNonZoning;
          // add it to list of active sectors
          if(!bscNonZoning.bsc_lnInActiveSectors.IsLinked()) {
            cm_lhActiveSectors.AddTail(bscNonZoning.bsc_lnInActiveSectors);
          }
        }}
      }
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDNONZONING);
    ENDFOR}

    _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_FINDNONZONING);
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_MAINLOOP);

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_CLEANUP);
  // clear list of active sectors
  {FORDELETELIST(CBrushSector, bsc_lnInActiveSectors, cm_lhActiveSectors, itbsc) {
    itbsc->bsc_lnInActiveSectors.Remove();
  }}
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS_CLEANUP);

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CACHENEARPOLYGONS);
}

void CClipMove::ClipToNonZoningSector(CBrushSector *pbsc)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPTONONZONINGSECTOR);
  _pfPhysicsProfile.IncrementTimerAveragingCounter(
    CPhysicsProfile::PTI_CLIPTONONZONINGSECTOR, pbsc->bsc_abpoPolygons.Count());

#ifdef PLATFORM_WIN32
  // for each polygon in the sector
  FOREACHINSTATICARRAY(pbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
	  // if its bbox has no contact with bbox of movement path, or it is passable
	  if (!itbpo->bpo_boxBoundingBox.HasContactWith(cm_boxMovementPath)
		  || (itbpo->bpo_ulFlags&BPOF_PASSABLE)) {
		  // skip it
		  continue;
	  }
	  // clip movement to the polygon
	  ClipMoveToBrushPolygon(itbpo);
  }
#else
  // for each polygon in the sector
  //FOREACHINSTATICARRAY(pbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
  CBrushPolygon *itbpo = pbsc->bsc_abpoPolygons.sa_Array;
  int i;
  const int count = pbsc->bsc_abpoPolygons.sa_Count;
  for (i = 0; i < count; i++, itbpo++) {
    // if its bbox has no contact with bbox of movement path, or it is passable
    __builtin_prefetch(&itbpo[1].bpo_ulFlags);
    if ((itbpo->bpo_ulFlags&BPOF_PASSABLE)
      ||!itbpo->bpo_boxBoundingBox.HasContactWith(cm_boxMovementPath)) {
      // skip it
      continue;
    }
    // clip movement to the polygon
    ClipMoveToBrushPolygon(itbpo);
  }
#endif

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPTONONZONINGSECTOR);
}


void CClipMove::ClipToTerrain(CEntity *pen)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPTONONZONINGSECTOR);
//  _pfPhysicsProfile.IncrementTimerAveragingCounter(
//    CPhysicsProfile::PTI_CLIPTONONZONINGSECTOR, pbsc->bsc_abpoPolygons.Count());

  
  CTerrain &tr = *pen->en_ptrTerrain;
  GFXVertex4 *pavVertices;
  INDEX_T      *paiIndices;
  INDEX ctVertices,ctIndices;

  FLOAT3D vMin = cm_boxMovementPath.Min();
  FLOAT3D vMax = cm_boxMovementPath.Max();

  FLOATaabbox3D boxMovementPath;
  #define TRANSPT(x) (x-pen->en_plPlacement.pl_PositionVector) * !pen->en_mRotation
  boxMovementPath  = TRANSPT(FLOAT3D(vMin(1),vMin(2),vMin(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMin(1),vMin(2),vMax(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMax(1),vMin(2),vMin(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMax(1),vMin(2),vMax(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMin(1),vMax(2),vMin(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMin(1),vMax(2),vMax(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMax(1),vMax(2),vMin(3)));
  boxMovementPath |= TRANSPT(FLOAT3D(vMax(1),vMax(2),vMax(3)));

/*
  boxMovementPath.minvect(1) /= tr.tr_vStretch(1);
  boxMovementPath.minvect(3) /= tr.tr_vStretch(3);
  boxMovementPath.maxvect(1) /= tr.tr_vStretch(1);
  boxMovementPath.maxvect(3) /= tr.tr_vStretch(3);
*/
  ExtractPolygonsInBox(&tr,boxMovementPath,&pavVertices,&paiIndices,ctVertices,ctIndices);
  
  // for each triangle
  for(INDEX iTri=0;iTri<ctIndices;iTri+=3) {
    INDEX_T &iind1 = paiIndices[iTri+0];
    INDEX_T &iind2 = paiIndices[iTri+1];
    INDEX_T &iind3 = paiIndices[iTri+2];
    FLOAT3D v0(pavVertices[iind1].x,pavVertices[iind1].y,pavVertices[iind1].z);
    FLOAT3D v1(pavVertices[iind2].x,pavVertices[iind2].y,pavVertices[iind2].z);
    FLOAT3D v2(pavVertices[iind3].x,pavVertices[iind3].y,pavVertices[iind3].z);
    ClipMoveToTerrainPolygon(v0,v1,v2);
  }

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPTONONZONINGSECTOR);
}

void CClipMove::ClipToZoningSector(CBrushSector *pbsc)
{

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPTOZONINGSECTOR);
  CStaticStackArray<CBrushPolygon *> &apbpo = cm_penMoving->en_apbpoNearPolygons;

  _pfPhysicsProfile.IncrementTimerAveragingCounter(
    CPhysicsProfile::PTI_CLIPTOZONINGSECTOR, apbpo.Count());

  // for each cached polygon
  for(INDEX iPolygon=0; iPolygon<apbpo.Count(); iPolygon++) {
    CBrushPolygon *pbpo = apbpo[iPolygon];
    // if it doesn't belong to the sector or its bbox has no contact with bbox of movement path
    if (pbpo->bpo_pbscSector != pbsc ||
      !pbpo->bpo_boxBoundingBox.HasContactWith(cm_boxMovementPath)) {
      // skip it
      continue;
    }
    // if it is not passable
    if (!(pbpo->bpo_ulFlags&BPOF_PASSABLE)) {
      // clip movement to the polygon
      ClipMoveToBrushPolygon(pbpo);
    // if it is passable
    } else {
      // for each sector related to the portal
      {FOREACHDSTOFSRC(pbpo->bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbscRelated)
        // if the sector is not active
        if (pbscRelated->bsc_pbmBrushMip->IsFirstMip() &&
           !pbscRelated->bsc_lnInActiveSectors.IsLinked()) {
          // add it to active list
          cm_lhActiveSectors.AddTail(pbscRelated->bsc_lnInActiveSectors);
        }
      ENDFOR}
    }
  }

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPTOZONINGSECTOR);
}

/* Clip movement to brush sectors near the entity. */
void CClipMove::ClipMoveToBrushes(void)
{
  // we never clip moving brush to a brush
  if (cm_bMovingBrush) {
    return;
  }
  if (cm_penMoving->en_ulCollisionFlags&ECF_IGNOREBRUSHES) {
    return;
  }

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES);
  _pfPhysicsProfile.IncrementTimerAveragingCounter(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES, 1);

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDINITIAL);
  // for each zoning sector that this entity is in
  {FOREACHSRCOFDST(cm_penMoving->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    _pfPhysicsProfile.IncrementTimerAveragingCounter(
      CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDINITIAL, 1);
    // if it collides with this one
    if (pbsc->bsc_pbmBrushMip->IsFirstMip() &&
      pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_pfsFieldSettings==NULL &&
      MustTest(pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity)) {
      // add it to list of active sectors
      cm_lhActiveSectors.AddTail(pbsc->bsc_lnInActiveSectors);
    }
  ENDFOR}
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDINITIAL);

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_MAINLOOP);
  // for each active sector
  FOREACHINLIST(CBrushSector, bsc_lnInActiveSectors, cm_lhActiveSectors, itbsc) {
    _pfPhysicsProfile.IncrementTimerAveragingCounter(
      CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_MAINLOOP, 1);

    _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_FINDNONZONING);
    // for non-zoning brush entities in the sector
    {FOREACHDSTOFSRC(itbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
      if (pen->en_RenderType!=CEntity::RT_BRUSH&&
          pen->en_RenderType!=CEntity::RT_FIELDBRUSH&&
          pen->en_RenderType!=CEntity::RT_TERRAIN) {
        break;  // brushes are sorted first in list
      }
      if(!MustTest(pen)) {
        continue;
      }

      if (pen->en_RenderType==CEntity::RT_TERRAIN) {
        // remember currently tested entity
        cm_penTested = pen;
        // moving model is A and still terrain is B
        cm_penB = pen;
        GetPositionsOfEntity(cm_penB, cm_vB0, cm_mB0, cm_vB1, cm_mB1);

        // prepare new projections and spheres
        PrepareProjectionsAndSpheres();

        // clip movement to the terrain
        ClipToTerrain(pen);

        // don't process as brush
        continue;
      }

      _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDNONZONING);
      // get first mip
      CBrushMip *pbm = pen->en_pbrBrush->GetFirstMip();
      // if brush mip exists for that mip factor
      if (pbm!=NULL) {
        // for each sector in the mip
        {FOREACHINDYNAMICARRAY(pbm->bm_abscSectors, CBrushSector, itbscNonZoning) {
          CBrushSector &bscNonZoning = *itbscNonZoning;
          // add it to list of active sectors
          if(!bscNonZoning.bsc_lnInActiveSectors.IsLinked()) {
            cm_lhActiveSectors.AddTail(bscNonZoning.bsc_lnInActiveSectors);
          }
        }}
      }
      _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_ADDNONZONING);
    ENDFOR}
    _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_FINDNONZONING);

    // get the sector's brush mip, brush and entity
    CBrushMip *pbmBrushMip = itbsc->bsc_pbmBrushMip;
    CBrush3D *pbrBrush = pbmBrushMip->bm_pbrBrush;
    ASSERT(pbrBrush!=NULL);
    CEntity *penBrush = pbrBrush->br_penEntity;
    ASSERT(penBrush!=NULL);

    // remember currently tested entity
    cm_penTested = penBrush;
    // moving model is A and still brush is B
    cm_penB = penBrush;
    GetPositionsOfEntity(cm_penB, cm_vB0, cm_mB0, cm_vB1, cm_mB1);

    // prepare new projections and spheres
    PrepareProjectionsAndSpheres();

    // clip movement to the sector
    if (penBrush->en_ulFlags&ENF_ZONING) {
      ClipToZoningSector(itbsc);
    } else {
      ClipToNonZoningSector(itbsc);
    }
  }
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_MAINLOOP);

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_CLEANUP);
  // clear list of active sectors
  {FORDELETELIST(CBrushSector, bsc_lnInActiveSectors, cm_lhActiveSectors, itbsc) {
    itbsc->bsc_lnInActiveSectors.Remove();
  }}
  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES_CLEANUP);

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOBRUSHES);
}

/* Clip movement to models near the entity. */
void CClipMove::ClipMoveToModels(void)
{
  if (cm_penMoving->en_ulCollisionFlags&ECF_IGNOREMODELS) {
    return;
  }

  // create mask for skipping deleted entities
  ULONG ulSkipMask = ENF_DELETED;
  // if the moving entity is predictor
  if (cm_penMoving->IsPredictor()) {
    // add predicted entities to the mask
    ulSkipMask |= ENF_PREDICTED;
  }

  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOMODELS);

  // find colliding entities near the box of movement path
  static CStaticStackArray<CEntity*> apenNearEntities;
  cm_pwoWorld->FindEntitiesNearBox(cm_boxMovementPath, apenNearEntities);

  // for each of the found entities
  {for(INDEX ienFound=0; ienFound<apenNearEntities.Count(); ienFound++) {
    CEntity &enToCollide = *apenNearEntities[ienFound];
    _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_XXTESTS);
    // if it is the one that is moving, or if it is skiped by the mask
    if (&enToCollide == cm_penMoving || (enToCollide.en_ulFlags&ulSkipMask)) {
      // skip it
      continue;
    }
    // if it can collide with this entity
    if (MustTest(&enToCollide)) {
      // if it is model entity
      if (enToCollide.en_RenderType == CEntity::RT_MODEL ||
          enToCollide.en_RenderType == CEntity::RT_EDITORMODEL ||
          enToCollide.en_RenderType == CEntity::RT_SKAMODEL ||
          enToCollide.en_RenderType == CEntity::RT_SKAEDITORMODEL) {
        // clip movement to the model
        ClipMoveToModel(&enToCollide);
      }
    }
  }}
  apenNearEntities.PopAll();

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOMODELS);
}


/*
 * Clip movement to the world.
 */
void CClipMove::ClipMoveToWorld(class CWorld *pwoWorld)
{
  _pfPhysicsProfile.StartTimer(CPhysicsProfile::PTI_CLIPMOVETOWORLD);
  _pfPhysicsProfile.IncrementTimerAveragingCounter(CPhysicsProfile::PTI_CLIPMOVETOWORLD);
  _pfPhysicsProfile.IncrementCounter(CPhysicsProfile::PCI_CLIPMOVES);

  // if there is no move or if the entity is deleted, or doesn't collide with anything
  // test if there is no movement !!!!
  if (/*!cm_bMovingBrush&&(cm_vA0 == cm_vA1 && cm_mA0 == cm_mA1)
    || cm_bMovingBrush&&(cm_vB0 == cm_vB1 && cm_mB0 == cm_mB1)
    ||*/(cm_penMoving->en_ulFlags&ENF_DELETED)
    ||!(cm_penMoving->en_ulCollisionFlags&ECF_TESTMASK)) {
    // skip clipping
    _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOWORLD);
    return;
  }

  cm_pwoWorld = pwoWorld;

  // prepare flags masks for testing which entities collide with this
  cm_ulTestMask1 = ((cm_penMoving->en_ulCollisionFlags&ECF_TESTMASK)>>ECB_TEST)<<ECB_IS;
  cm_ulTestMask2 = ((cm_penMoving->en_ulCollisionFlags&ECF_ISMASK  )>>ECB_IS  )<<ECB_TEST;

  cm_ulPassMaskA = ((cm_penMoving->en_ulCollisionFlags&ECF_PASSMASK)>>ECB_PASS)<<ECB_IS;
  cm_ulPassMaskB = ((cm_penMoving->en_ulCollisionFlags&ECF_ISMASK  )>>ECB_IS  )<<ECB_PASS;

  // cache near polygons of zoning brushes
  CacheNearPolygons();

  // clip to brush sectors near the entity
  ClipMoveToBrushes();
  // clip to models near the entity
  ClipMoveToModels();

  _pfPhysicsProfile.StopTimer(CPhysicsProfile::PTI_CLIPMOVETOWORLD);
}

/*
 * Test if a movement is clipped by something and where.
 */
void CWorld::ClipMove(CClipMove &cmMove)
{
  cmMove.ClipMoveToWorld(this);
}
