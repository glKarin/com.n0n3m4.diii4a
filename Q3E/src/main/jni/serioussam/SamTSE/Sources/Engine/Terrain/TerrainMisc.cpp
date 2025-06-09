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
#include <Engine/Math/Vector.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/Functions.h>
#include <Engine/Math/Geometry.inl>
#include <Engine/Math/Clipping.inl>
#include <Engine/Terrain/Terrain.h>
#include <Engine/Terrain/TerrainMisc.h>
#include <Engine/Entities/Entity.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldRayCasting.h>
#include <Engine/Light/LightSource.h>
#include <Engine/Rendering/Render.h>
#include <Engine/Terrain/TerrainRayCasting.h>

/*
 * Terrain raycasting and colision 
 */

extern CTerrain *_ptrTerrain; // Current terrain
static FLOAT3D _vHitLocation = FLOAT3D(-100,-100,-100);

CStaticStackArray<GFXVertex4> _avExtVertices;
CStaticStackArray<INDEX_T>    _aiExtIndices;
CStaticStackArray<GFXColor>   _aiExtColors;
CStaticStackArray<INDEX>      _aiHitTiles;

static ULONG *_pulSharedTopMap = NULL; // Shared memory used for topmap regeneration
#ifdef PLATFORM_WIN32
extern SLONG  _slSharedTopMapSize = 0; // Size of shared memory allocated for topmap regeneration
#else
SLONG  _slSharedTopMapSize = 0; // Size of shared memory allocated for topmap regeneration
#endif
extern INDEX  _ctShadowMapUpdates;
//#pragma message(">> Create class with destructor to clear shared topmap memory")

FLOATaabbox3D _bboxDrawOne;
FLOATaabbox3D _bboxDrawTwo;

#define NUMDIM	3
#define RIGHT	  0
#define LEFT	  1
#define MIDDLE	2

#if 0 // DG: unused.
// Test AABBox agains ray
static BOOL HitBoundingBox(FLOAT3D &vOrigin, FLOAT3D &vDir, FLOAT3D &vHit, FLOATaabbox3D &bbox)
{
	BOOL bInside = TRUE;
	BOOL quadrant[NUMDIM];
	register int i;
	int whichPlane;

  double maxT[NUMDIM];
	double candidatePlane[NUMDIM];
  double minB[NUMDIM], maxB[NUMDIM];  /*box */
  double origin[NUMDIM], dir[NUMDIM]; /*ray */
  double coord[NUMDIM]; /* hit point */

  minB[0]   = bbox.minvect(1); minB[1] = bbox.minvect(2); minB[2] = bbox.minvect(3);
  maxB[0]   = bbox.maxvect(1); maxB[1] = bbox.maxvect(2); maxB[2] = bbox.maxvect(3);
  origin[0] = vOrigin(1); origin[1] = vOrigin(2); origin[2] = vOrigin(3);
  dir[0]    = vDir(1);    dir[1]    = vDir(2);    dir[2]    = vDir(3);

	/* Find candidate planes; this loop can be avoided if
   	rays cast all from the eye(assume perpsective view) */
  for (i=0; i<NUMDIM; i++) {
		if(origin[i] < minB[i]) {
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			bInside = FALSE;
    } else if (origin[i] > maxB[i]) {
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			bInside = FALSE;
    } else {
			quadrant[i] = MIDDLE;
		}
  }

	/* Ray origin inside bounding box */
	if(bInside)	{
    vHit = FLOAT3D(origin[0],origin[1],origin[2]);
		return (TRUE);
	}


	/* Calculate T distances to candidate planes */
	for (i = 0; i < NUMDIM; i++)
		if (quadrant[i] != MIDDLE && dir[i] !=0.)
			maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
		else
			maxT[i] = -1.;

	/* Get largest of the maxT's for final choice of intersection */
	whichPlane = 0;
	for (i = 1; i < NUMDIM; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	/* Check final candidate actually inside box */
	if (maxT[whichPlane] < 0.) return (FALSE);
  for (i = 0; i < NUMDIM; i++) {
		if (whichPlane != i) {
			coord[i] = origin[i] + maxT[whichPlane] *dir[i];
      if (coord[i] < minB[i] || coord[i] > maxB[i]) {
        return (FALSE);
      }
		} else {
			coord[i] = candidatePlane[i];
		}
  }
	return (TRUE);				/* ray hits box */
}

// Test AABBox agains ray
static BOOL RayHitsAABBox(FLOAT3D &vOrigin, FLOAT3D &vDir, FLOAT3D &vHit, FLOATaabbox3D &bbox)
{
  FLOAT minB[3];
  FLOAT maxB[3];
  FLOAT origin[3];
  FLOAT dir[3];
  FLOAT coord[3];
  

  minB[0]   = bbox.minvect(1); minB[1] = bbox.minvect(2); minB[2] = bbox.minvect(3);
  maxB[0]   = bbox.maxvect(1); maxB[1] = bbox.maxvect(2); maxB[2] = bbox.maxvect(3);
  origin[0] = vOrigin(1); origin[1] = vOrigin(2); origin[2] = vOrigin(3);
  dir[0]    = vDir(1);    dir[1]    = vDir(2);    dir[2]    = vDir(3);

	char inside = TRUE;
	char quadrant[3];
	register int i;
	int whichPlane;
	FLOAT maxT[3];
	FLOAT candidatePlane[3];

	/* Find candidate planes; this loop can be avoided if
   	rays cast all from the eye(assume perpsective view) */
	for (i=0; i<3; i++)
		if(origin[i] < minB[i]) {
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			inside = FALSE;
		}else if (origin[i] > maxB[i]) {
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			inside = FALSE;
		}else	{
			quadrant[i] = MIDDLE;
		}

	/* Ray origin inside bounding box */
	if(inside)	{
    vHit = FLOAT3D(origin[0],origin[1],origin[2]);
    return TRUE;
	}


	/* Calculate T distances to candidate planes */
  for (i = 0; i < 3; i++) {
    if (quadrant[i] != MIDDLE && dir[i] !=0.) {
			maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
    } else {
			maxT[i] = -1.;
    }
  }

	/* Get largest of the maxT's for final choice of intersection */
	whichPlane = 0;
	for (i = 1; i < 3; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	/* Check final candidate actually inside box */
    if (maxT[whichPlane] < 0.) {
      return FALSE;
    }
	for (i = 0; i < 3; i++)
		if (whichPlane != i) {
			coord[i] = origin[i] + maxT[whichPlane] *dir[i];
      if (coord[i] < minB[i] || coord[i] > maxB[i]) {
				return FALSE;
      }
		} else {
			coord[i] = candidatePlane[i];
		}

  // ray hits box
  vHit = FLOAT3D(coord[0],coord[1],coord[2]);
	return TRUE;
}
#endif // 0 (unused)

// Get exact hit location in tile
FLOAT GetExactHitLocation(INDEX iTileIndex, FLOAT3D &vOrigin, FLOAT3D &vTarget, FLOAT3D &vHitLocation)
{
  //CTerrainTile &tt  = _ptrTerrain->tr_attTiles[iTileIndex];
  QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[iTileIndex];

  GFXVertex *pavVertices;
  INDEX_T   *paiIndices;
  INDEX      ctVertices;
  INDEX      ctIndices;

  ExtractPolygonsInBox(_ptrTerrain,qtn.qtn_aabbox,&pavVertices,&paiIndices,ctVertices,ctIndices);


  FLOAT fDummyDist = 100000;//(vTarget - vOrigin).Length() * 2; 
  FLOAT fDistance = fDummyDist;

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
    FLOAT fDistance0 = plTriPlane.PointDistance(vOrigin);
	  FLOAT fDistance1 = plTriPlane.PointDistance(vTarget);

    // if the ray hits the polygon plane
	  if (fDistance0>=0 && fDistance0>=fDistance1) {
		  // calculate fraction of line before intersection
		  FLOAT fFraction = fDistance0/(fDistance0-fDistance1);
		  // calculate intersection coordinate
		  FLOAT3D vHitPoint = vOrigin+(vTarget-vOrigin)*fFraction;
		  // calculate intersection distance
		  FLOAT fHitDistance = (vHitPoint-vOrigin).Length();
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
				fDistance = fHitDistance;
        vHitLocation = vHitPoint;
  		}
    }
  }
  if(fDistance!=fDummyDist) {
    _vHitLocation = vHitLocation;
    return fDistance;
  } else {
    return -1;
  }
}

FLOAT3D _vHitBegin;// TEMP
FLOAT3D _vHitEnd;  // TEMP
FLOAT3D _vDirection; // TEMP
FLOAT3D _vHitExact; // TEMP

//#pragma message(">> Remove Rect from ExtractPolygonsInBox")
// Extract polygons in given box and returns clipped rectangle
Rect ExtractPolygonsInBox(CTerrain *ptrTerrain, const FLOATaabbox3D &bboxExtract, GFXVertex4 **pavVtx, 
                          INDEX_T **paiInd, INDEX &ctVtx,INDEX &ctInd,BOOL bFixSize/*=FALSE*/)
{
  ASSERT(ptrTerrain!=NULL);

  FLOATaabbox3D bbox = bboxExtract;
  
  bbox.minvect(1) /= ptrTerrain->tr_vStretch(1);
  bbox.minvect(3) /= ptrTerrain->tr_vStretch(3);
  bbox.maxvect(1) /= ptrTerrain->tr_vStretch(1);
  bbox.maxvect(3) /= ptrTerrain->tr_vStretch(3);
  
  _avExtVertices.PopAll();
  _aiExtIndices.PopAll();
  _aiExtColors.PopAll();

  Rect rc;
  if(!bFixSize) {
    // max vector of bbox in incremented for one, because first vertex is at 0,0,0 in world and in heightmap is at 1,1
#ifdef __arm__
#if defined(PLATFORM_PANDORA) || defined(PLATFORM_PYRA) || defined(ANDROID) //karin: no isinff
  #define Isinf(a) (((*(unsigned int*)&a)&0x7fffffff)==0x7f800000)
#else
  #define Isinf isinff
#endif
    rc.rc_iLeft   = (Isinf(bbox.minvect(1)))?(INDEX)0:Clamp((INDEX)(bbox.minvect(1)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iTop    = (Isinf(bbox.minvect(3)))?(INDEX)0:Clamp((INDEX)(bbox.minvect(3)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
    rc.rc_iRight  = (Isinf(bbox.maxvect(1)))?(INDEX)0:Clamp((INDEX)ceil(bbox.maxvect(1)+1),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iBottom = (Isinf(bbox.maxvect(3)))?(INDEX)0:Clamp((INDEX)ceil(bbox.maxvect(3)+1),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
#else
    rc.rc_iLeft   = Clamp((INDEX)(bbox.minvect(1)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iTop    = Clamp((INDEX)(bbox.minvect(3)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
    rc.rc_iRight  = Clamp((INDEX)ceil(bbox.maxvect(1)+1),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iBottom = Clamp((INDEX)ceil(bbox.maxvect(3)+1),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
#endif
  } else {
    // max vector of bbox in incremented for one, because first vertex is at 0,0,0 in world and in heightmap is at 1,1
#ifdef __arm__
    rc.rc_iLeft   = (Isinf(bbox.minvect(1)))?(INDEX)0:Clamp((INDEX)(bbox.minvect(1)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iTop    = (Isinf(bbox.minvect(3)))?(INDEX)0:Clamp((INDEX)(bbox.minvect(3)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
    rc.rc_iRight  = (Isinf(bbox.maxvect(1)))?(INDEX)0:Clamp((INDEX)(bbox.maxvect(1)+0),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iBottom = (Isinf(bbox.maxvect(3)))?(INDEX)0:Clamp((INDEX)(bbox.maxvect(3)+0),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
#else
    rc.rc_iLeft   = Clamp((INDEX)(bbox.minvect(1)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iTop    = Clamp((INDEX)(bbox.minvect(3)-0),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
    rc.rc_iRight  = Clamp((INDEX)(bbox.maxvect(1)+0),(INDEX)0,ptrTerrain->tr_pixHeightMapWidth);
    rc.rc_iBottom = Clamp((INDEX)(bbox.maxvect(3)+0),(INDEX)0,ptrTerrain->tr_pixHeightMapHeight);
#endif
  }

  INDEX iStartX = rc.rc_iLeft;
  INDEX iStartY = rc.rc_iTop;
  INDEX iWidth  = rc.Width();
  INDEX iHeight = rc.Height();

  INDEX iFirst = iStartX + iStartY * ptrTerrain->tr_pixHeightMapWidth;
  INDEX iPitchX = ptrTerrain->tr_pixHeightMapWidth  - iWidth;
  //INDEX iPitchY = ptrTerrain->tr_pixHeightMapHeight - iHeight;

  // get first pixel in height map
  UWORD *puwHeight = &ptrTerrain->tr_auwHeightMap[iFirst];
  UBYTE *pubMask   = &ptrTerrain->tr_aubEdgeMap[iFirst];

  INDEX ctVertices = iWidth*iHeight;
  INDEX ctIndices = (iWidth-1)*(iHeight-1)*6;
  
//  ASSERT(ctVertices>0 && ctIndices>0);
  if(ctVertices==0 || ctIndices==0) {
    ctVtx = 0;
    ctInd = 0;
    return Rect(0,0,0,0);
  }

  // Allocate space for vertices and indices
  _avExtVertices.Push(ctVertices);
  _aiExtIndices.Push(ctIndices);

  GFXVertex4 *pavVertices = &_avExtVertices[0];
  INDEX_T *pauiIndices = &_aiExtIndices[0];

  // for each row
  INDEX iy, ix;
  for(iy=0;iy<iHeight;iy++) {
    // for each column
    for(ix=0;ix<iWidth;ix++) {
      // Add one vertex
      GFXVertex4 &vx = *pavVertices;
      vx.x = (FLOAT)(ix+iStartX)*ptrTerrain->tr_vStretch(1);
      vx.z = (FLOAT)(iy+iStartY)*ptrTerrain->tr_vStretch(3);
      vx.y = *puwHeight * ptrTerrain->tr_vStretch(2);
      vx.shade = *pubMask;

      puwHeight++;
      pubMask++;
      pavVertices++;
    }
    puwHeight+=iPitchX;
    pubMask+=iPitchX;
  }

  INDEX ivx=0;
  //INDEX ind=0;
  INDEX iFacing=iFirst;

  GFXVertex *pavExtVtx = &_avExtVertices[0];
  INDEX ctVisTris = 0; // Visible tris

  // for each row
  for(iy=0;iy<iHeight-1;iy++) {
    // for each column
    for(ix=0;ix<iWidth-1;ix++) {
      // Add one quad ( if it is visible )
      if(iFacing&1) {
        // if all vertices in this triangle are visible
        if(pavExtVtx[ivx].shade + pavExtVtx[ivx+iWidth].shade + pavExtVtx[ivx+1].shade == 255*3) { 
          // Add one triangle
          pauiIndices[0] = ivx;
          pauiIndices[1] = ivx+iWidth;
          pauiIndices[2] = ivx+1;
          pauiIndices+=3;
          ctVisTris++;
        }
        // if all vertices in this triangle are visible
        if(pavExtVtx[ivx+1].shade + pavExtVtx[ivx+iWidth].shade + pavExtVtx[ivx+iWidth+1].shade == 255*3) {
          // Add one triangle
          pauiIndices[0] = ivx+1;
          pauiIndices[1] = ivx+iWidth;
          pauiIndices[2] = ivx+iWidth+1;
          pauiIndices+=3;
          ctVisTris++;
        }
      } else {
        // if all vertices in this triangle are visible
        if(pavExtVtx[ivx+iWidth].shade + pavExtVtx[ivx+iWidth+1].shade + pavExtVtx[ivx].shade == 255*3) { 
          // Add one triangle
          pauiIndices[0] = ivx+iWidth;
          pauiIndices[1] = ivx+iWidth+1;
          pauiIndices[2] = ivx;
          pauiIndices+=3;
          ctVisTris++;
        }
        // if all vertices in this triangle are visible
        if(pavExtVtx[ivx].shade + pavExtVtx[ivx+iWidth+1].shade + pavExtVtx[ivx+1].shade == 255*3) {
          // Add one triangle
          pauiIndices[0] = ivx;
          pauiIndices[1] = ivx+iWidth+1;
          pauiIndices[2] = ivx+1;
          pauiIndices+=3;
          ctVisTris++;
        }
      }
      iFacing++;
      ivx++;
    }
    if(iWidth&1) iFacing++;
    ivx++;
  }

  ctVtx = ctVertices;
  ctInd = ctVisTris*3;
  *pavVtx = &_avExtVertices[0];
  *paiInd = &_aiExtIndices[0];
  return rc;
}

void ExtractVerticesInRect(CTerrain *ptrTerrain, Rect &rc, GFXVertex4 **pavVtx, 
                          INDEX_T **paiInd, INDEX &ctVtx,INDEX &ctInd)
{
  _avExtVertices.PopAll();
  _aiExtIndices.PopAll();
  _aiExtColors.PopAll();

  INDEX iWidth  = rc.Width();
  INDEX iHeight = rc.Height();

  ctVtx = iWidth*iHeight;
  ctInd  = (iWidth-1)*(iHeight-1)*6;

  if(ctVtx==0 || ctInd==0) {
    return;
  }
  _avExtVertices.Push(ctVtx);
  _aiExtIndices.Push(ctInd);

  *pavVtx = &_avExtVertices[0];
  *paiInd = &_aiExtIndices[0];

  INDEX iStartX = rc.rc_iLeft;
  INDEX iStartY = rc.rc_iTop;

  INDEX iFirstHeight = rc.rc_iTop*ptrTerrain->tr_pixHeightMapWidth + rc.rc_iLeft;
  INDEX iStepY       = ptrTerrain->tr_pixHeightMapWidth - iWidth;
  UWORD *puwHeight   = &ptrTerrain->tr_auwHeightMap[iFirstHeight];

  GFXVertex *pavVertices = &_avExtVertices[0];
  INDEX iy, ix;
  for(iy=0;iy<iHeight;iy++) {
    for(ix=0;ix<iWidth;ix++) {
      pavVertices->x = (FLOAT)(ix+iStartX)*ptrTerrain->tr_vStretch(1);
      pavVertices->z = (FLOAT)(iy+iStartY)*ptrTerrain->tr_vStretch(3);
      pavVertices->y = *puwHeight * ptrTerrain->tr_vStretch(2);
      puwHeight++;
      pavVertices++;
    }
    puwHeight+=iStepY;
  }

  INDEX_T *pauiIndices = &_aiExtIndices[0];
  INDEX ivx=0;
  //INDEX ind=0;
  INDEX iFacing=iFirstHeight;
  // for each row
  for(iy=0;iy<iHeight-1;iy++) {
    // for each column
    for(INDEX ix=0;ix<iWidth-1;ix++) {
      if(iFacing&1) {
        pauiIndices[0] = ivx;   pauiIndices[1] = ivx+iWidth; pauiIndices[2] = ivx+1;
        pauiIndices[3] = ivx+1; pauiIndices[4] = ivx+iWidth; pauiIndices[5] = ivx+iWidth+1;
      } else {
        pauiIndices[0] = ivx+iWidth; pauiIndices[1] = ivx+iWidth+1; pauiIndices[2] = ivx;
        pauiIndices[3] = ivx;        pauiIndices[4] = ivx+iWidth+1; pauiIndices[5] = ivx+1;
      }
      // Add one quad
      pauiIndices+=6;
      iFacing++;
      ivx++;
    }
    if(iWidth&1) iFacing++;
    ivx++;
  }
}

// Extract all tiles that intersect with given box
void FindTilesInBox(CTerrain *ptrTerrain, FLOATaabbox3D &bbox)
{
  ASSERT(ptrTerrain!=NULL);
  _aiHitTiles.PopAll();
  // for each terrain tile
  for(INDEX itt=0;itt<_ptrTerrain->tr_ctTiles;itt++) {
    QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[itt];
    // if it is coliding with given box
    if(qtn.qtn_aabbox.HasContactWith(bbox)) {
      // add it to array of coliding tiles
      INDEX &iHitTile = _aiHitTiles.Push();
      iHitTile = itt;
    }
  }
}

// Add these flags to all tiles that have been extracted
void AddFlagsToExtractedTiles(ULONG ulFlags)
{
  ASSERT(_ptrTerrain!=NULL);
  // for each tile that has contact with extraction box
  INDEX ctht = _aiHitTiles.Count();
  for(INDEX iht=0;iht<ctht;iht++) {
    // Add tile to regen queue
    INDEX iTileIndex = _aiHitTiles[iht];
    CTerrainTile &tt = _ptrTerrain->tr_attTiles[iTileIndex];
    tt.AddFlag(ulFlags);
    _ptrTerrain->AddTileToRegenQueue(iTileIndex);
  }
}

// Get value from layer at given point
UBYTE GetValueFromMask(CTerrain *ptrTerrain, INDEX iLayer, FLOAT3D vHitPoint)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);
  
  CEntity *penEntity = ptrTerrain->tr_penEntity;
  // convert hit point to terrain space and remove terrain stretch from terrain
  FLOAT3D vHit = (vHitPoint - penEntity->en_plPlacement.pl_PositionVector) * !penEntity->en_mRotation;
  vHit(1)=ceil(vHit(1)/ptrTerrain->tr_vStretch(1));
  vHit(3)=ceil(vHit(3)/ptrTerrain->tr_vStretch(3));

  CTerrainLayer &tl = ptrTerrain->GetLayer(iLayer);
  INDEX iVtx = (INDEX) (vHit(1) + tl.tl_iMaskWidth*vHit(3));
  if(iVtx<0 || iVtx>=tl.tl_iMaskWidth*tl.tl_iMaskHeight) {
    ASSERTALWAYS("Invalid hit point");
    return 0;
  }
  UBYTE ubValue = tl.tl_aubColors[iVtx];
  return ubValue;
}

// Allocate memory of one top map
void CreateTexture(CTextureData &tdTopMap, PIX pixWidth, PIX pixHeight,ULONG ulFlags)
{
  // clear current top map
  if(tdTopMap.td_pulFrames!=NULL) {
    FreeMemory( tdTopMap.td_pulFrames);
    tdTopMap.td_pulFrames = NULL;
  }

  // Create new top map
  tdTopMap.td_mexWidth  = pixWidth;
  tdTopMap.td_mexHeight = pixHeight;
  tdTopMap.td_ulFlags   = ulFlags;
  // Allocate memory for top map
  INDEX ctMipMaps = GetNoOfMipmaps(pixWidth,pixHeight);
  SLONG slSize = GetMipmapOffset(ctMipMaps,pixWidth,pixHeight)*BYTES_PER_TEXEL;
  tdTopMap.td_pulFrames = (ULONG*)AllocMemory(slSize);
  tdTopMap.td_slFrameSize = slSize;
  tdTopMap.td_ctFrames = 1;
  tdTopMap.td_iFirstMipLevel = 0;
  tdTopMap.td_ctFineMipLevels = GetNoOfMipmaps(pixWidth,pixHeight);
  // Prepare dithering type
  tdTopMap.td_ulInternalFormat = TS.ts_tfRGBA8;
}

// Create one topmap
void CreateTopMap(CTextureData &tdTopMap, PIX pixWidth , PIX pixHeight)
{
  ASSERT(tdTopMap.td_pulFrames==NULL);
  // Prepare new top map
  INDEX ctMipMaps = GetNoOfMipmaps(pixWidth,pixHeight);
  SLONG slSize = GetMipmapOffset(ctMipMaps,pixWidth,pixHeight)*BYTES_PER_TEXEL;

  tdTopMap.td_mexWidth  = pixWidth;
  tdTopMap.td_mexHeight = pixHeight;
  tdTopMap.td_ulFlags   = TEX_ALPHACHANNEL|TEX_STATIC; // Pretend this texture is static
  tdTopMap.td_pulFrames = NULL; // This will be shared memory
  tdTopMap.td_slFrameSize = slSize;
  tdTopMap.td_ctFrames = 1;
  tdTopMap.td_iFirstMipLevel = 0;
  tdTopMap.td_ctFineMipLevels = GetNoOfMipmaps(pixWidth,pixHeight);
  tdTopMap.td_ulInternalFormat = TS.ts_tfRGB5A1;
}

// Set topmap frames pointer to shared memory
void PrepareSharedTopMapMemory(CTextureData *ptdTopMap, INDEX iTileIndex)
{
  SLONG slSize = ptdTopMap->td_slFrameSize;
  // if this is global top map
  if(iTileIndex==(-1)) {
    // if shared memory is larger then global top map
    if(slSize<=_slSharedTopMapSize && _pulSharedTopMap!=NULL) {
      // assign pointer of global top map to shared memory
      ptdTopMap->td_pulFrames = _pulSharedTopMap;
      return;
    // else
    } else {
      // Allocate new memory for global top map
      ptdTopMap->td_pulFrames = (ULONG*)AllocMemory(slSize);
    }
  // else this is normal top map
  } else {
    // if required memory is larger than currently allocated one
    if(slSize>_slSharedTopMapSize) {
      // if shared memory exists
      if(_pulSharedTopMap!=NULL) {
        // free current shared memory
        FreeMemory(_pulSharedTopMap);
        _pulSharedTopMap = NULL;
      }
      // allocate new shared memory for top maps
      _pulSharedTopMap = (ULONG*)AllocMemory(slSize);
      // remember new memory size
      _slSharedTopMapSize = slSize;
    }
    // assign pointer of top map to shared memory
    ptdTopMap->td_pulFrames = _pulSharedTopMap;
  }
}

void FreeSharedTopMapMemory(CTextureData *ptdTopMap, INDEX iTileIndex)
{
  // if this is global top map
  if(iTileIndex==(-1)) {
    // if global top map isn't using shared memory
    if(ptdTopMap->td_pulFrames!=_pulSharedTopMap) {
      // free memory global top map is using
      FreeMemory(ptdTopMap->td_pulFrames);
    }
  }
  // Just clear pointer to memory
  ptdTopMap->td_pulFrames = NULL;
}

static FLOAT3D CalculateNormalFromPoint(FLOAT fPosX, FLOAT fPosZ, FLOAT3D *pvStrPos=NULL)
{
  FLOAT3D vNormal;
  INDEX iPosX = (INDEX)fPosX;
  INDEX iPosZ = (INDEX)fPosZ;
  FLOAT fLerpX = fPosX - iPosX;
  FLOAT fLerpZ = fPosZ - iPosZ;

  FLOAT3D avVtx[4];
  INDEX iHMapWidth = _ptrTerrain->tr_pixHeightMapWidth;
  FLOAT3D vStretch = _ptrTerrain->tr_vStretch;

  avVtx[0](1) = (FLOAT)(iPosX  ) * vStretch(1);
  avVtx[1](1) = (FLOAT)(iPosX+1) * vStretch(1);
  avVtx[2](1) = (FLOAT)(iPosX  ) * vStretch(1);
  avVtx[3](1) = (FLOAT)(iPosX+1) * vStretch(1);

  avVtx[0](3) = (FLOAT)(iPosZ  ) * vStretch(3);
  avVtx[1](3) = (FLOAT)(iPosZ  ) * vStretch(3);
  avVtx[2](3) = (FLOAT)(iPosZ+1) * vStretch(3);
  avVtx[3](3) = (FLOAT)(iPosZ+1) * vStretch(3);

  avVtx[0](2) = (FLOAT)_ptrTerrain->tr_auwHeightMap[ (iPosX  ) + (iPosZ  )*iHMapWidth ] * vStretch(2);
  avVtx[1](2) = (FLOAT)_ptrTerrain->tr_auwHeightMap[ (iPosX+1) + (iPosZ  )*iHMapWidth ] * vStretch(2);
  avVtx[2](2) = (FLOAT)_ptrTerrain->tr_auwHeightMap[ (iPosX  ) + (iPosZ+1)*iHMapWidth ] * vStretch(2);
  avVtx[3](2) = (FLOAT)_ptrTerrain->tr_auwHeightMap[ (iPosX+1) + (iPosZ+1)*iHMapWidth ] * vStretch(2);

  FLOAT fHDeltaX = Lerp(avVtx[1](2)-avVtx[0](2), avVtx[3](2)-avVtx[2](2), fLerpZ);
  FLOAT fHDeltaZ = Lerp(avVtx[0](2)-avVtx[2](2), avVtx[1](2)-avVtx[3](2), fLerpX);
  FLOAT fDeltaX  = avVtx[1](1) - avVtx[0](1);
  FLOAT fDeltaZ  = avVtx[0](3) - avVtx[2](3);

  vNormal(2) = sqrt(1 / (((fHDeltaX*fHDeltaX)/(fDeltaX*fDeltaX)) + ((fHDeltaZ*fHDeltaZ)/(fDeltaZ*fDeltaZ)) + 1));
  vNormal(1) = sqrt(vNormal(2)*vNormal(2) * ((fHDeltaX*fHDeltaX) / (fDeltaX*fDeltaX)));
  vNormal(3) = sqrt(vNormal(2)*vNormal(2) * ((fHDeltaZ*fHDeltaZ) / (fDeltaZ*fDeltaZ)));
  if (fHDeltaX>0) {
    vNormal(1) = -vNormal(1);
  }
  if (fHDeltaZ<0) {
    vNormal(3) = -vNormal(3);
  }
  ASSERT(Abs(vNormal.Length()-1)<0.01);

  if(pvStrPos!=NULL) {
    FLOAT fResX1 = Lerp(avVtx[0](2),avVtx[1](2),fLerpX);
    FLOAT fResX2 = Lerp(avVtx[2](2),avVtx[3](2),fLerpX);
    FLOAT fPosY  = Lerp(fResX1,fResX2,fLerpZ);

    (*pvStrPos)(1) = fPosX * vStretch(1);
    (*pvStrPos)(2) = fPosY; // * vStretch(2);
    (*pvStrPos)(3) = fPosZ * vStretch(3);
  }

  return vNormal;
}

static void CalcPointLight(CPlacement3D &plLight, CLightSource *plsLight, Rect &rcUpdate)
{
  FLOAT fSHDiffX = (FLOAT)_ptrTerrain->tr_pixHeightMapWidth  / _ptrTerrain->GetShadowMapWidth();
  FLOAT fSHDiffZ = (FLOAT)_ptrTerrain->tr_pixHeightMapHeight / _ptrTerrain->GetShadowMapHeight();

  PIX pixLeft   = rcUpdate.rc_iLeft;
  PIX pixRight  = rcUpdate.rc_iRight;
  PIX pixTop    = rcUpdate.rc_iTop;
  PIX pixBottom = rcUpdate.rc_iBottom;
  PIX pixWidth  = pixRight - pixLeft;
  PIX pixStepX  = _ptrTerrain->GetShadowMapWidth()  - pixWidth;

  // Get color pointer in shadow map
  PIX       pixFirst   = pixLeft + pixTop*_ptrTerrain->GetShadowMapWidth();
  GFXColor *pacolData  = (GFXColor*)&_ptrTerrain->tr_tdShadowMap.td_pulFrames[pixFirst];

  // for each row in shadow map
  for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
    // for each in column
    for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
      FLOAT fPosX = (FLOAT)(pixX*fSHDiffX);
      FLOAT fPosZ = (FLOAT)(pixY*fSHDiffZ);

      FLOAT3D vPosStr;
      FLOAT3D vNormal = CalculateNormalFromPoint(fPosX,fPosZ,&vPosStr);
      
      // Calculate normal from light position
      FLOAT3D vDistance = vPosStr - plLight.pl_PositionVector;
      FLOAT   fDistance = vDistance.Length();
      FLOAT3D vLightNormal = -vDistance.Normalize();
      GFXColor colLight   = plsLight->GetLightColor();

      // Calculate light intensity
      FLOAT fIntensity = 1.0f;
      FLOAT fFallOff   = plsLight->ls_rFallOff;
      FLOAT fHotSpot   = plsLight->ls_rHotSpot;
      if(fDistance>fFallOff) {
        fIntensity = 0;
      } else if(fDistance>fHotSpot) {
        fIntensity = CalculateRatio(fDistance, fHotSpot, fFallOff, 0.0f, 1.0f);
      }
      ULONG ulIntensity = NormFloatToByte(fIntensity);
      ulIntensity = (ulIntensity<<CT_RSHIFT)|(ulIntensity<<CT_GSHIFT)|(ulIntensity<<CT_BSHIFT);
	  colLight = MulColors(ByteSwap(colLight.ul.abgr), ulIntensity);

      FLOAT fDot = vNormal%vLightNormal;
      fDot = Clamp(fDot,0.0f,1.0f);
      SLONG slDot = NormFloatToByte(fDot);
	  pacolData->ub.r = ClampUp(pacolData->ub.r + ((colLight.ub.r*slDot) >> 8), (SLONG)255);
	  pacolData->ub.g = ClampUp(pacolData->ub.g + ((colLight.ub.g*slDot) >> 8), (SLONG)255);
	  pacolData->ub.b = ClampUp(pacolData->ub.b + ((colLight.ub.b*slDot) >> 8), (SLONG)255);
	  pacolData->ub.a = 255;
      pacolData++;
    }
    pacolData+=pixStepX;
  }
}

static void CalcDirectionalLight(CPlacement3D &plLight, CLightSource *plsLight, Rect &rcUpdate)
{
  FLOAT fSHDiffX = (FLOAT)_ptrTerrain->tr_pixHeightMapWidth  / _ptrTerrain->GetShadowMapWidth();
  FLOAT fSHDiffZ = (FLOAT)_ptrTerrain->tr_pixHeightMapHeight / _ptrTerrain->GetShadowMapHeight();

  PIX pixLeft   = rcUpdate.rc_iLeft;
  PIX pixRight  = rcUpdate.rc_iRight;
  PIX pixTop    = rcUpdate.rc_iTop;
  PIX pixBottom = rcUpdate.rc_iBottom;
  PIX pixWidth  = pixRight - pixLeft;
  PIX pixStepX  = _ptrTerrain->GetShadowMapWidth()  - pixWidth;

  // Get color pointer in shadow map
  PIX       pixFirst   = pixLeft + pixTop*_ptrTerrain->GetShadowMapWidth();
  GFXColor *pacolData  = (GFXColor*)&_ptrTerrain->tr_tdShadowMap.td_pulFrames[pixFirst];

  FLOAT3D vLightNormal;
  GFXColor colLight   = plsLight->GetLightColor();
  GFXColor colAmbient = plsLight->GetLightAmbient();

  UBYTE ubColShift = 8;
  SLONG slar = colAmbient.ub.r;
  SLONG slag = colAmbient.ub.g;
  SLONG slab = colAmbient.ub.b;

  extern INDEX mdl_bAllowOverbright;
  BOOL bOverBrightning = mdl_bAllowOverbright && _pGfx->gl_ctTextureUnits>1;

  // is overbrightning enabled
  if(bOverBrightning) {
    slar = ClampUp(slar, (SLONG)127);
    slag = ClampUp(slag, (SLONG)127);
    slab = ClampUp(slab, (SLONG)127);
    ubColShift = 8;
  } else {
    slar*=2;
    slag*=2;
    slab*=2;
    ubColShift = 7;
  }

  // Calculate light normal
  AnglesToDirectionVector(plLight.pl_OrientationAngle,vLightNormal);
  vLightNormal *= !_ptrTerrain->tr_penEntity->en_mRotation;
  vLightNormal = -vLightNormal.Normalize();

  // for each row in shadow map
  for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
    // for each in column
    for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
      FLOAT fPosX = (FLOAT)(pixX*fSHDiffX);
      FLOAT fPosZ = (FLOAT)(pixY*fSHDiffZ);
      FLOAT3D vNormal = CalculateNormalFromPoint(fPosX,fPosZ);

      FLOAT fDot = vNormal%vLightNormal;
      fDot = Clamp(fDot,0.0f,1.0f);
      SLONG slDot = NormFloatToByte(fDot);
	  pacolData->ub.r = ClampUp(pacolData->ub.r + slar + ((colLight.ub.r*slDot) >> ubColShift), (SLONG)255);
	  pacolData->ub.g = ClampUp(pacolData->ub.g + slag + ((colLight.ub.g*slDot) >> ubColShift), (SLONG)255);
	  pacolData->ub.b = ClampUp(pacolData->ub.b + slab + ((colLight.ub.b*slDot) >> ubColShift), (SLONG)255);
	  pacolData->ub.a = 255;
      pacolData++;
    }
    pacolData+=pixStepX;
  }
}

static void ClearPartOfShadowMap(CTerrain *ptrTerrain, Rect &rcUpdate)
{
  PIX pixLeft   = rcUpdate.rc_iLeft;
  PIX pixRight  = rcUpdate.rc_iRight;
  PIX pixTop    = rcUpdate.rc_iTop;
  PIX pixBottom = rcUpdate.rc_iBottom;
  PIX pixWidth  = pixRight - pixLeft;
  PIX pixStepX  = _ptrTerrain->GetShadowMapWidth() - pixWidth;

  // Get color pointer in shadow map
  PIX pixFirst = rcUpdate.rc_iLeft + rcUpdate.rc_iTop * ptrTerrain->GetShadowMapWidth();
  GFXColor *pacolData  = (GFXColor*)&_ptrTerrain->tr_tdShadowMap.td_pulFrames[pixFirst];
  // for each row in shadow map
  for(PIX pixY=pixTop;pixY<pixBottom;pixY++) {
    // for each in column
    for(PIX pixX=pixLeft;pixX<pixRight;pixX++) {
      *pacolData = 0x00000000;
      pacolData++;
    }
    pacolData+=pixStepX;
  }
}

static Rect GetUpdateRectFromBox(CTerrain *ptrTerrain, FLOATaabbox3D &boxUpdate)
{
  Rect rcUpdate;
  // Prepare update rect
  FLOAT fSHDiffX = (FLOAT)ptrTerrain->tr_pixHeightMapWidth  / ptrTerrain->GetShadowMapWidth();
  FLOAT fSHDiffZ = (FLOAT)ptrTerrain->tr_pixHeightMapHeight / ptrTerrain->GetShadowMapHeight();
  rcUpdate.rc_iLeft   = (INDEX)floor((boxUpdate.minvect(1)/ptrTerrain->tr_vStretch(1)) / fSHDiffX);
  rcUpdate.rc_iRight  = (INDEX)ceil ((boxUpdate.maxvect(1)/ptrTerrain->tr_vStretch(1)) / fSHDiffX);
  rcUpdate.rc_iTop    = (INDEX)floor((boxUpdate.minvect(3)/ptrTerrain->tr_vStretch(3)) / fSHDiffZ);
  rcUpdate.rc_iBottom = (INDEX)ceil ((boxUpdate.maxvect(3)/ptrTerrain->tr_vStretch(3)) / fSHDiffZ);
  return rcUpdate;
}

static FLOATaabbox3D AbsoluteToRelative(const CTerrain *ptrTerrain, const FLOATaabbox3D &bbox)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);
  FLOATaabbox3D bboxRelative;
  CEntity *pen = ptrTerrain->tr_penEntity;
  
  #define TRANSPT(x) (x-pen->en_plPlacement.pl_PositionVector) * !pen->en_mRotation
  bboxRelative  = TRANSPT(FLOAT3D(bbox.minvect(1),bbox.minvect(2),bbox.minvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.minvect(1),bbox.minvect(2),bbox.maxvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.maxvect(1),bbox.minvect(2),bbox.minvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.maxvect(1),bbox.minvect(2),bbox.maxvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.minvect(1),bbox.maxvect(2),bbox.minvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.minvect(1),bbox.maxvect(2),bbox.maxvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.maxvect(1),bbox.maxvect(2),bbox.minvect(3)));
  bboxRelative |= TRANSPT(FLOAT3D(bbox.maxvect(1),bbox.maxvect(2),bbox.maxvect(3)));
  return bboxRelative;
}

//static ULONG ulTemp = 0xFFFFFFFF;

void UpdateTerrainShadowMap(CTerrain *ptrTerrain, FLOATaabbox3D *pboxUpdate/*=NULL*/, BOOL bAbsoluteSpace/*=FALSE*/)
{
  // if this is not world editor app 
  extern BOOL _bWorldEditorApp;
  if(!_bWorldEditorApp) {

    ASSERTALWAYS("Terrain shadow map can only be updated from world editor!");
    return;
  }

  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);
  ASSERT(ptrTerrain->tr_penEntity->en_pwoWorld!=NULL);
  
  FLOATaabbox3D boxUpdate;
  FLOATaabbox3D boxAllTerrain;
  CEntity *penEntity = ptrTerrain->tr_penEntity;

  ptrTerrain->GetAllTerrainBBox(boxAllTerrain);
  // if request to update whole terrain is given
  if(pboxUpdate==NULL) {
    // take all terrain bbox as update box
    boxUpdate = boxAllTerrain;
  } else {
    // use given bbox as update box
    boxUpdate = *pboxUpdate;
    if(bAbsoluteSpace) {
      boxUpdate = AbsoluteToRelative(ptrTerrain, boxUpdate);
    }
    
    // do not update terrain if update box isn't in terrain box
    if(!boxUpdate.HasContactWith(boxAllTerrain)) {
      return;
    }

    boxUpdate.minvect(1) = Clamp(boxUpdate.minvect(1),boxAllTerrain.minvect(1),boxAllTerrain.maxvect(1));
    boxUpdate.minvect(3) = Clamp(boxUpdate.minvect(3),boxAllTerrain.minvect(3),boxAllTerrain.maxvect(3));
    boxUpdate.maxvect(1) = Clamp(boxUpdate.maxvect(1),boxAllTerrain.minvect(1),boxAllTerrain.maxvect(1));
    boxUpdate.maxvect(3) = Clamp(boxUpdate.maxvect(3),boxAllTerrain.minvect(3),boxAllTerrain.maxvect(3));
    boxUpdate.minvect(2) = boxAllTerrain.minvect(2);
    boxUpdate.maxvect(2) = boxAllTerrain.maxvect(2);
  }

  _ptrTerrain = ptrTerrain;
  // Get pointer to world that holds this terrain
  CWorld *pwldWorld = penEntity->en_pwoWorld;
  
  //PIX pixWidth  = ptrTerrain->GetShadowMapWidth();
  //PIX pixHeight = ptrTerrain->GetShadowMapHeight();

  CTextureData &tdShadowMap = ptrTerrain->tr_tdShadowMap;
  ASSERT(tdShadowMap.td_pulFrames!=NULL);

  Rect rcUpdate = GetUpdateRectFromBox(ptrTerrain, boxUpdate);
  // Clear part of shadow map that will be updated
  ClearPartOfShadowMap(ptrTerrain,rcUpdate);

  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(pwldWorld->wo_cenEntities, CEntity, iten) {
    // if it is light entity and it influences the given range
    CLightSource *pls = iten->GetLightSource();
    CPlacement3D plLight = iten->en_plPlacement;
    
    // Translate light placement to terrain space
    plLight.pl_PositionVector = 
     (plLight.pl_PositionVector - penEntity->en_plPlacement.pl_PositionVector) * !penEntity->en_mRotation;

    if (pls!=NULL) {
      // Get light bounding box
      FLOATaabbox3D boxLight(plLight.pl_PositionVector, pls->ls_rFallOff);
      // if light is directional
      if(pls->ls_ulFlags &LSF_DIRECTIONAL) {
        // Calculate lightning
        CalcDirectionalLight(plLight,pls,rcUpdate);
      // if it is point light
      } else {
        _bboxDrawOne = boxLight;
        _bboxDrawTwo = boxUpdate;
        // if point light box have contact with update box
        if(boxLight.HasContactWith(boxUpdate)) {
          _ctShadowMapUpdates++;

          // if light box is inside update box
          if(boxLight.minvect(1)>=boxUpdate.minvect(1) && boxLight.minvect(3)>boxUpdate.minvect(3) && 
            boxLight.maxvect(1)<=boxUpdate.maxvect(1) && boxLight.maxvect(3)<=boxUpdate.maxvect(3)) {
            // Recalculate only light box
            Rect rcLightUpdate = GetUpdateRectFromBox(ptrTerrain,boxLight);
            CalcPointLight(plLight,pls,rcLightUpdate);
          // else 
          } else {
            // Recalculate update box
            CalcPointLight(plLight,pls,rcUpdate);
          }
        }
      }
    }
  }

  // Create shadow map mipmaps 
  INDEX ctMipMaps = GetNoOfMipmaps(tdShadowMap.td_mexWidth,tdShadowMap.td_mexHeight);
  MakeMipmaps(ctMipMaps, tdShadowMap.td_pulFrames, tdShadowMap.td_mexWidth, tdShadowMap.td_mexHeight);


  // Update shading map from one mip of shadow map
  INDEX iMipOffset = GetMipmapOffset(ptrTerrain->tr_iShadingMapSizeAspect,ptrTerrain->GetShadowMapWidth(),ptrTerrain->GetShadowMapHeight());
  UWORD *puwShade = &ptrTerrain->tr_auwShadingMap[0];
  ULONG *ppixShadowMip = &ptrTerrain->tr_tdShadowMap.td_pulFrames[iMipOffset];

  INDEX ctpixs = ptrTerrain->GetShadingMapWidth()*ptrTerrain->GetShadingMapHeight();
  for(PIX ipix=0;ipix<ctpixs;ipix++) {
    ULONG ulPixel = ByteSwap(*ppixShadowMip);
    // ULONG ulPixel = ulTemp;
    *puwShade = (((ulPixel>>27)&0x001F)<<10) | 
                (((ulPixel>>19)&0x001F)<< 5) | 
                (((ulPixel>>11)&0x001F)<< 0);
    puwShade++;
    ppixShadowMip++;
  }

  // discard cached model info
  ptrTerrain->DiscardShadingInfos();

  ptrTerrain->tr_tdShadowMap.SetAsCurrent(0,TRUE);
}


// Calculate 2d relative point in terrain from absolute 3d point in world
Point Calculate2dHitPoint(CTerrain *ptrTerrain, FLOAT3D &vHitPoint)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);

  // Get entity that holds this terrain
  CEntity *penEntity = ptrTerrain->tr_penEntity;
  // Get relative hit point
  FLOAT3D vRelHitPoint = (vHitPoint - penEntity->en_plPlacement.pl_PositionVector) * !penEntity->en_mRotation;
  
  // Unstretch hit point and convert it to 2d
  Point pt;
  pt.pt_iX = (INDEX) (ceil(vRelHitPoint(1) / ptrTerrain->tr_vStretch(1) - 0.5f));
  pt.pt_iY = (INDEX) (ceil(vRelHitPoint(3) / ptrTerrain->tr_vStretch(3) - 0.5f));
  
  return pt;
}

// Calculate tex coords on shading map from absolute 3d point in world
FLOAT2D CalculateShadingTexCoords(CTerrain *ptrTerrain, FLOAT3D &vPoint)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);
  // Get entity that holds this terrain
  CEntity *penEntity = ptrTerrain->tr_penEntity;
  // Get relative hit point
  FLOAT3D vRelPoint = (vPoint - penEntity->en_plPlacement.pl_PositionVector) * !penEntity->en_mRotation;

  // Unstretch hit point and convert it to 2d point in shading map
  FLOAT fX = vRelPoint(1) / ptrTerrain->tr_vStretch(1);
  FLOAT fY = vRelPoint(3) / ptrTerrain->tr_vStretch(3);
  FLOAT fU = fX / ((FLOAT)(ptrTerrain->tr_pixHeightMapWidth)  / ptrTerrain->GetShadingMapWidth());
  FLOAT fV = fY / ((FLOAT)(ptrTerrain->tr_pixHeightMapHeight) / ptrTerrain->GetShadingMapHeight());
  
  ASSERT(fU>0.0f && fU<ptrTerrain->GetShadingMapWidth());
  ASSERT(fV>0.0f && fV<ptrTerrain->GetShadingMapHeight());
  return FLOAT2D(fU,fV);
}
