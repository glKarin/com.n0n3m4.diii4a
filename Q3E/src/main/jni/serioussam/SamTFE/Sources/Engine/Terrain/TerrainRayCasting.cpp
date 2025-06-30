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
#include <Engine/Math/Plane.h>
#include <Engine/Math/Clipping.inl>
#include <Engine/Math/Geometry.inl>
#include <Engine/Entities/Entity.h>

static CTerrain *_ptrTerrain = NULL;
static FLOAT3D   _vOrigin;           // Origin of ray
static FLOAT3D   _vTarget;           // Ray target
static FLOAT     _fMinHeight;        // Min height that ray will pass through in tested quad
static FLOAT     _fMaxHeight;        // Max height that ray will pass through in tested quad
static BOOL      _bHitInvisibleTris; // Does ray hits invisible triangles

static FLOAT3D _vHitExact;           // hit point
static FLOATplane3D _plHitPlane;     // hit plane

// TEMP
static CStaticStackArray<GFXVertex> _avRCVertices;
static CStaticStackArray<INDEX_T>     _aiRCIndices;
static FLOAT3D _vHitBegin;
static FLOAT3D _vHitEnd;
static FLOAT   _fDistance;

// Test ray agains one quad on terrain (if it's visible)
static FLOAT HitCheckQuad(const PIX ix, const PIX iz)
{
  FLOAT fDistance = UpperLimit(0.0f);

  // if quad is outside terrain
  if(ix<0 || iz<0 || ix>= (_ptrTerrain->tr_pixHeightMapWidth-1) || iz >= (_ptrTerrain->tr_pixHeightMapHeight-1)) {
    return fDistance;
  }

  ASSERT(ix>=0 && iz>=0);
  ASSERT(ix<(_ptrTerrain->tr_pixHeightMapWidth-1) && iz<(_ptrTerrain->tr_pixHeightMapHeight-1));

  const PIX pixMapWidth = _ptrTerrain->tr_pixHeightMapWidth;
  const INDEX ctVertices = _avRCVertices.Count(); // TEMP

  UWORD *puwHeight = &_ptrTerrain->tr_auwHeightMap[ix + iz*pixMapWidth];
  UBYTE *pubMask   = &_ptrTerrain->tr_aubEdgeMap[ix + iz*pixMapWidth];
  GFXVertex *pvx  = _avRCVertices.Push(4);
  
  GFXVertex *pavVertices = &_avRCVertices[0];
  // Add four vertices
  pvx[0].x = (ix+0) * _ptrTerrain->tr_vStretch(1);
  pvx[0].y = puwHeight[0] * _ptrTerrain->tr_vStretch(2);
  pvx[0].z = (iz+0) * _ptrTerrain->tr_vStretch(3);
  pvx[0].shade = pubMask[0];

  pvx[1].x = (ix+1) * _ptrTerrain->tr_vStretch(1);
  pvx[1].y = puwHeight[1] * _ptrTerrain->tr_vStretch(2);
  pvx[1].z = (iz+0) * _ptrTerrain->tr_vStretch(3);
  pvx[1].shade = pubMask[1];

  pvx[2].x = (ix+0) * _ptrTerrain->tr_vStretch(1);
  pvx[2].y = puwHeight[pixMapWidth] * _ptrTerrain->tr_vStretch(2);
  pvx[2].z = (iz+1) * _ptrTerrain->tr_vStretch(3);
  pvx[2].shade = pubMask[pixMapWidth];

  pvx[3].x = (ix+1) * _ptrTerrain->tr_vStretch(1);
  pvx[3].y = puwHeight[pixMapWidth+1] * _ptrTerrain->tr_vStretch(2);
  pvx[3].z = (iz+1) * _ptrTerrain->tr_vStretch(3);
  pvx[3].shade = pubMask[pixMapWidth+1];

  BOOL bFacing = (ix + iz*pixMapWidth)&1;

  INDEX ctIndices=0;
  // Add one quad
  if(bFacing) {
    // if at least one point of triangle is above min height and bellow max height of ray and traingle is visible
    if((pvx[0].y>=_fMinHeight || pvx[2].y>=_fMinHeight || pvx[1].y>=_fMinHeight) &&
       (pvx[0].y<=_fMaxHeight || pvx[2].y<=_fMinHeight || pvx[1].y<=_fMinHeight) &&
       ((pvx[0].shade + pvx[2].shade + pvx[1].shade == 255*3) | _bHitInvisibleTris)) {
      // Add this triangle
      INDEX_T *pind = _aiRCIndices.Push(3);
      pind[0] = ctVertices+0;
      pind[1] = ctVertices+2;
      pind[2] = ctVertices+1;
      ctIndices+=3;
    }
    // if at least one point of triangle is above min height and bellow max height of ray and traingle is visible
    if((pvx[1].y>=_fMinHeight || pvx[2].y>=_fMinHeight || pvx[3].y>=_fMinHeight) &&
       (pvx[1].y<=_fMaxHeight || pvx[2].y<=_fMaxHeight || pvx[3].y<=_fMaxHeight) &&
       ((pvx[1].shade + pvx[2].shade + pvx[3].shade == 255*3) | _bHitInvisibleTris)) {
      // Add this triangle
      INDEX_T *pind = _aiRCIndices.Push(3);
      pind[0] = ctVertices+1;
      pind[1] = ctVertices+2;
      pind[2] = ctVertices+3;
      ctIndices+=3;
    }
  } else {
    // if at least one point of triangle is above min height and bellow max height of ray and traingle is visible
    if((pvx[2].y>=_fMinHeight || pvx[3].y>=_fMinHeight || pvx[0].y>=_fMinHeight) &&
       (pvx[2].y<=_fMaxHeight || pvx[3].y<=_fMaxHeight || pvx[0].y<=_fMaxHeight) &&
       ((pvx[2].shade + pvx[3].shade + pvx[0].shade == 255*3) | _bHitInvisibleTris)) {
      // Add this triangle
      INDEX_T *pind = _aiRCIndices.Push(3);
      pind[0] = ctVertices+2;
      pind[1] = ctVertices+3;
      pind[2] = ctVertices+0;
      ctIndices+=3;
    }
    // if at least one point of triangle is above min height and bellow max height of ray and traingle is visible
    if((pvx[0].y>=_fMinHeight || pvx[3].y>=_fMinHeight || pvx[1].y>=_fMinHeight) &&
       (pvx[0].y<=_fMaxHeight || pvx[3].y<=_fMaxHeight || pvx[1].y<=_fMaxHeight) &&
       ((pvx[0].shade + pvx[3].shade + pvx[1].shade == 255*3) | _bHitInvisibleTris)) {
      // Add this triangle
      INDEX_T *pind = _aiRCIndices.Push(3);
      pind[0] = ctVertices+0;
      pind[1] = ctVertices+3;
      pind[2] = ctVertices+1;
      ctIndices+=3;
    }
  }

  if(ctIndices==0) {
    return fDistance;
  }

  INDEX_T *paiIndices = &_aiRCIndices[_aiRCIndices.Count() - ctIndices];
  // for each triangle
  for(INDEX iTri=0;iTri<ctIndices;iTri+=3) {
    INDEX_T *pind = &paiIndices[iTri];
    GFXVertex &v0 = pavVertices[pind[0]];
    GFXVertex &v1 = pavVertices[pind[1]];
    GFXVertex &v2 = pavVertices[pind[2]];

    FLOAT3D vx0(v0.x,v0.y,v0.z);
    FLOAT3D vx1(v1.x,v1.y,v1.z);
    FLOAT3D vx2(v2.x,v2.y,v2.z);

    FLOATplane3D plTriPlane(vx0,vx1,vx2);
    FLOAT fDistance0 = plTriPlane.PointDistance(_vOrigin);
	  FLOAT fDistance1 = plTriPlane.PointDistance(_vTarget);

    // if the ray hits the polygon plane
	  if (fDistance0>=0 && fDistance0>=fDistance1) {
		  // calculate fraction of line before intersection
		  FLOAT fFraction = fDistance0/(fDistance0-fDistance1);
		  // calculate intersection coordinate
		  FLOAT3D vHitPoint = _vOrigin+(_vTarget-_vOrigin)*fFraction;
		  // calculate intersection distance
		  FLOAT fHitDistance = (vHitPoint-_vOrigin).Length();
		  // if the hit point can not be new closest candidate
		  if (fHitDistance>fDistance) {
			  // skip this triangle
			  continue;
		  }

      // find major axes of the polygon plane
			INDEX iMajorAxis1, iMajorAxis2;
			GetMajorAxesForPlane(plTriPlane, iMajorAxis1, iMajorAxis2);

      // create an intersector
			CIntersector isIntersector(vHitPoint(iMajorAxis1), vHitPoint(iMajorAxis2));

			// check intersections for all three edges of the polygon
			isIntersector.AddEdge(
					vx0(iMajorAxis1), vx0(iMajorAxis2),
					vx1(iMajorAxis1), vx1(iMajorAxis2));
			isIntersector.AddEdge(
					vx1(iMajorAxis1), vx1(iMajorAxis2),
					vx2(iMajorAxis1), vx2(iMajorAxis2));
			isIntersector.AddEdge(
					vx2(iMajorAxis1), vx2(iMajorAxis2),
					vx0(iMajorAxis1), vx0(iMajorAxis2));

      // if the polygon is intersected by the ray, and it is the closest intersection so far
			if (isIntersector.IsIntersecting() && (fHitDistance < fDistance)) {
				// remember hit coordinates
        if(fHitDistance<fDistance) {
				  fDistance = fHitDistance;
          _vHitExact = vHitPoint;
          _plHitPlane = plTriPlane;
        }
  		}
    }
  }
  return fDistance;
}

//#pragma message(">> Remove defined NUMDIM, RIGHT, LEFT ...")
#define NUMDIM	3
#define RIGHT	  0
#define LEFT	  1
#define MIDDLE	2

// Check if ray hits aabbox and return coords where ray enter and exit the box
static BOOL HitAABBox(const FLOAT3D &vOrigin, const FLOAT3D &vTarget, FLOAT3D &vHitBegin,
                      FLOAT3D &vHitEnd, const FLOATaabbox3D &bbox) 
{
  const FLOAT3D vDir = (vTarget - vOrigin).Normalize();
  const FLOAT3D vMin = bbox.minvect;
  const FLOAT3D vMax = bbox.maxvect;
  FLOAT3D vBeginCandidatePlane;
  FLOAT3D vEndCandidatePlane;
  FLOAT3D vBeginTDistance;
  FLOAT3D vEndTDistance;
  INDEX   iOriginSide[3];
  BOOL    bOriginInside = TRUE;

	INDEX i;

	// Find candidate planes
  for(i=1;i<4;i++) {
    // Check begining of ray
		if(vOrigin(i) < vMin(i)) {
			vBeginCandidatePlane(i) = vMin(i);
      vEndCandidatePlane(i)   = vMax(i);
			bOriginInside = FALSE;
			iOriginSide[i-1] = LEFT;
    } else if(vOrigin(i) > vMax(i)) {
			vBeginCandidatePlane(i) = vMax(i);
      vEndCandidatePlane(i)   = vMin(i);
			bOriginInside = FALSE;
			iOriginSide[i-1] = RIGHT;
    } else {
			iOriginSide[i-1] = MIDDLE;
      if(vDir(i)>0.0f) {
        vEndCandidatePlane(i) = vMax(i);
      } else {
        vEndCandidatePlane(i) = vMin(i);
      }
		}
  }

	// Calculate T distances to candidate planes
  for(i=1;i<4;i++) {
    if(iOriginSide[i-1]!=MIDDLE && vDir(i)!=0.0f) {
			vBeginTDistance(i) = (vBeginCandidatePlane(i)-vOrigin(i)) / vDir(i);
    } else {
			vBeginTDistance(i) = -1.0f;
    }
    if(vDir(i)!=0.0f) {
			vEndTDistance(i) = (vEndCandidatePlane(i)-vOrigin(i)) / vDir(i);
    } else {
			vEndTDistance(i) = -1.0f;
    }
  }

	// Get largest of the T distances for final choice of intersection
	INDEX iBeginMaxT = 1;
  INDEX iEndMinT = 1;
  for(i=2;i<4;i++) {
    if(vBeginTDistance(i) > vBeginTDistance(iBeginMaxT)) {
			iBeginMaxT = i;
    }
    if(vEndTDistance(i)>=0.0f && (vEndTDistance(iEndMinT)<0.0f || vEndTDistance(i) < vEndTDistance(iEndMinT)) ) {
			iEndMinT = i;
    }
  }

  // if origin inside box
  if(bOriginInside) {
    // Begining of ray is origin point
    vHitBegin = vOrigin;
  // else 
  } else {
	  // Check final candidate actually inside box
    if(vBeginTDistance(iBeginMaxT)<0.0f) {
      return FALSE;
    }
    if(vEndTDistance(iEndMinT)<0.0f) {
      return FALSE;
    }

    // Calculate point where ray enter box
    for(i=1;i<4;i++) {
		  if(iBeginMaxT != i) {
			  vHitBegin(i) = vOrigin(i) + vBeginTDistance(iBeginMaxT) * vDir(i);
        if(vHitBegin(i) < vMin(i) || vHitBegin(i) > vMax(i)) {
          return FALSE;
        }
		  } else {
			  vHitBegin(i) = vBeginCandidatePlane(i);
		  }
    }
  }

  // Caclulate point where ray exit box
  for(i=1;i<4;i++) {
    if(iEndMinT != i) {
			vHitEnd(i) = vOrigin(i) + vEndTDistance(iEndMinT) * vDir(i);
      if(vHitEnd(i) < vMin(i) || vHitEnd(i) > vMax(i)) {
        // no ray exit point !?
        ASSERT(FALSE);
      }
		} else {
			vHitEnd(i) = vEndCandidatePlane(i);
		}
  }

  return TRUE;
}

// Test all quads in ray direction and return exact hit location
static FLOAT GetExactHitLocation(CTerrain *ptrTerrain, const FLOAT3D &vHitBegin, const FLOAT3D &vHitEnd,
                                 const FLOAT fOldDistance)
{
  // set global vars
  _ptrTerrain = ptrTerrain;
  _vOrigin    = vHitBegin;
  _vTarget    = vHitEnd;

  // TEMP
  _avRCVertices.PopAll();
  _aiRCIndices.PopAll();

  const FLOAT fX0 = vHitBegin(1) / ptrTerrain->tr_vStretch(1);
  const FLOAT fY0 = vHitBegin(3) / ptrTerrain->tr_vStretch(3);
  const FLOAT fH0 = vHitBegin(2);// / ptrTerrain->tr_vStretch(2);
  const FLOAT fX1 = vHitEnd(1)   / ptrTerrain->tr_vStretch(1);
  const FLOAT fY1 = vHitEnd(3)   / ptrTerrain->tr_vStretch(3);
  const FLOAT fH1 = vHitEnd(2);//   / ptrTerrain->tr_vStretch(2);

  FLOAT fDeltaX = Abs(fX1-fX0);
  FLOAT fDeltaY = Abs(fY1-fY0);
  FLOAT fIterator;

  if(fDeltaX>fDeltaY) {
    fIterator = fDeltaX;
  } else {
    fIterator = fDeltaY;
  }
  if(fIterator==0) {
    fIterator = 0.01f;
  }

  const FLOAT fStepX = (fX1-fX0) / fIterator;
  const FLOAT fStepY = (fY1-fY0) / fIterator;
  const FLOAT fStepH = (fH1-fH0) / fIterator;
  const FLOAT fEpsilonH = Abs(fStepH);

  FLOAT fX;
  FLOAT fY;
  FLOAT fH;
  // calculate prestep
  if(fDeltaX>fDeltaY) {
    if(fX0<fX1) {
      fX = ceil(fX0);
      fY = fY0 + (fX-fX0)*fStepY;
      fH = fH0 + (fX-fX0)*fStepH;
    } else {
      fX = floor(fX0);
      fY = fY0 + (fX0-fX)*fStepY;
      fH = fH0 + (fX0-fX)*fStepH;
    }
  } else {
    if(fY0<fY1) {
      fY = ceil(fY0);
      fX = fX0 + (fY-fY0)*fStepX;
      fH = fH0 + (fY-fY0)*fStepH;
    } else {
      fY = floor(fY0);
      fX = fX0 + (fY0-fY)*fStepX;
      fH = fH0 + (fY0-fY)*fStepH;
    }
  }


  // Chech quad where ray starts
  _fMinHeight = vHitBegin(2)-fEpsilonH;
  _fMaxHeight = vHitBegin(2)+fEpsilonH;
  FLOAT fDistanceStart = HitCheckQuad((SLONG) floor(fX0),(SLONG) floor(fY0));
  if(fDistanceStart<fOldDistance) {
    return fDistanceStart;
  }

  // for each iteration
  INDEX ctit = (INDEX) ceil(fIterator);
  for(INDEX iit=0;iit<ctit;iit++) {
    PIX pixX = (PIX) floor(fX);
    PIX pixY = (PIX) floor(fY);

    FLOAT fDistance0;
    FLOAT fDistance1;

    // Check first quad
    _fMinHeight = fH-fEpsilonH;
    _fMaxHeight = fH+fEpsilonH;
    fDistance0 = HitCheckQuad(pixX,pixY);
    // if iterating by x
    if(fDeltaX>fDeltaY) {
      // check left quad
      fDistance1 = HitCheckQuad(pixX-1,pixY);
    // else 
    } else {
      // check upper quad
      fDistance1 = HitCheckQuad(pixX,pixY-1);
    }

    // find closer of two quads
    if(fDistance1<fDistance0) {
      fDistance0 = fDistance1;
    }
    // if distance is closer than old distance
    if(fDistance0<fOldDistance) {
      // return distance
      return fDistance0;
    }

    fX+=fStepX;
    fY+=fStepY;
    fH+=fStepH;
  }

  // Chech quad where ray ends
  _fMinHeight = vHitEnd(2)-fEpsilonH;
  _fMaxHeight = vHitEnd(2)+fEpsilonH;
  FLOAT fDistanceEnd = HitCheckQuad((SLONG) floor(fX1), (SLONG) floor(fY1));
  if(fDistanceEnd<fOldDistance) {
    return fDistanceEnd;
  }


  // no hit
  return UpperLimit(0.0f);
}

// Test a ray agains given terrain
FLOAT TestRayCastHit(CTerrain *ptrTerrain, const FLOATmatrix3D &mRotation, const FLOAT3D &vPosition, 
                     const FLOAT3D &vOrigin, const FLOAT3D &vTarget,const FLOAT fOldDistance, const BOOL bHitInvisibleTris)
{
  _vHitBegin = FLOAT3D(0,0,0);
  _vHitEnd   = FLOAT3D(0,0,0);
  _vHitExact = FLOAT3D(0,0,0);
  _bHitInvisibleTris = bHitInvisibleTris;

  FLOATaabbox3D bboxAll;
  FLOATmatrix3D mInvertRot = !mRotation;

  FLOAT3D vStart = (vOrigin-vPosition) * mInvertRot;
  FLOAT3D vEnd   = (vTarget-vPosition) * mInvertRot;
  FLOAT3D vHitBegin;
  FLOAT3D vHitEnd;
  FLOAT fDistance = UpperLimit(0.0f);

  ptrTerrain->GetAllTerrainBBox(bboxAll);

  extern INDEX ter_bTempFreezeCast;
  static FLOAT3D _vFrozenStart;
  static FLOAT3D _vFrozenEnd;

  if(ter_bTempFreezeCast) {
    vStart = _vFrozenStart;
    vEnd   = _vFrozenEnd;
  } else  {
    _vFrozenStart = vStart;
    _vFrozenEnd   = vEnd;
  }

  // if ray hits terrain box
  if(HitAABBox(vStart,vEnd,vHitBegin,vHitEnd,bboxAll)) {
    // if begin and end are at same pos
    if(vHitBegin==vHitEnd) {
      // move end hit
      vHitBegin(2)+=0.1f;
      vHitEnd(2)-=0.1f;
    }
    _vHitBegin = vHitBegin;
    _vHitEnd   = vHitEnd;
    // find exact hit location on terrain
    fDistance = GetExactHitLocation(ptrTerrain,vHitBegin,vHitEnd,fOldDistance);
    fDistance += (vStart-vHitBegin).Length();
  }
  _fDistance = fDistance;
  return fDistance;
}

FLOAT TestRayCastHit(CTerrain *ptrTerrain, const FLOATmatrix3D &mRotation, const FLOAT3D &vPosition, 
                     const FLOAT3D &vOrigin, const FLOAT3D &vTarget,const FLOAT fOldDistance, 
                     const BOOL bHitInvisibleTris, FLOATplane3D &plHitPlane, FLOAT3D &vHitPoint)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);

  CEntity *pen = ptrTerrain->tr_penEntity;
  
  // casting ray
  FLOAT fDistance = TestRayCastHit(ptrTerrain, mRotation, vPosition, vOrigin, vTarget, fOldDistance, bHitInvisibleTris);
  // convert hit point to absulute point
  vHitPoint  = (_vHitExact * pen->en_mRotation) + pen->en_plPlacement.pl_PositionVector;

  plHitPlane = _plHitPlane;
  return fDistance;

}

#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/Font.h>
void ShowRayPath(CDrawPort *pdp)
{
  return;
  INDEX ctVertices = _avRCVertices.Count();
  INDEX ctIndices  = _aiRCIndices.Count();
  if(ctVertices>0 && ctIndices>0) {
    gfxDisableTexture();
    gfxDisableBlend();
    gfxEnableDepthBias();
    gfxPolygonMode(GFX_LINE);
    gfxSetVertexArray(&_avRCVertices[0],_avRCVertices.Count());
    gfxSetConstantColor(0xFFFFFFFF);
    gfxDrawElements(_aiRCIndices.Count(),&_aiRCIndices[0]);
    gfxDisableDepthBias();
    gfxPolygonMode(GFX_FILL);
  }

  gfxEnableDepthBias();
  gfxDisableDepthTest();
  pdp->DrawPoint3D(_vHitBegin,0x00FF00FF,8);
  pdp->DrawPoint3D(_vHitEnd,0xFF0000FF,8);
  pdp->DrawPoint3D(_vHitExact,0x00FFFF,8);
  pdp->DrawLine3D(_vHitBegin,_vHitEnd,0xFFFF00FF);
  pdp->DrawLine3D(FLOAT3D(_vHitBegin(1),_vHitEnd(2),_vHitBegin(3)),_vHitEnd,0xFF0000FF);
  gfxEnableDepthTest();
  gfxDisableDepthBias();

  /*
  extern void gfxDrawWireBox(FLOATaabbox3D &bbox, COLOR col);
  if(_ptrTerrain!=NULL) {
    FLOATaabbox3D bboxAll;
    _ptrTerrain->GetAllTerrainBBox(bboxAll);
    gfxDrawWireBox(bboxAll,0xFFFF00FF);
  }
  pdp->SetFont( _pfdConsoleFont);
  pdp->SetTextAspect( 1.0f);
  pdp->SetOrtho();
  pdp->PutText(CTString(0,"%g",_fDistance),0,0,0xFFFFFFFF);
  */
}
