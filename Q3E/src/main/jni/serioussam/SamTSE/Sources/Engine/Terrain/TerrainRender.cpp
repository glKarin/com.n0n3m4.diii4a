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
#include <Engine/Terrain/TerrainRender.h>
#include <Engine/Terrain/TerrainEditing.h>
#include <Engine/Math/Projection.h>
#include <Engine/Math/OBBox.h>
#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/Fog_internal.h>
#include <Engine/Rendering/Render.h>
#include <Engine/Entities/Entity.h>

static CAnyProjection3D _aprProjection;   // Current projection
static CDrawPort       *_pdp = NULL;      // Current drawport
CTerrain               *_ptrTerrain;      // Current terrain
FLOAT3D                 _vViewerAbs;      // Position of viewer
static COLOR            _colTerrainEdges; // Color of terrain edges

static FLOATmatrix3D _mObjectToView;
static FLOAT3D       _vObjectToView;
static FLOAT3D       _vViewer;
static FLOAT3D       _vViewerObj;

extern INDEX _ctNodesVis;       // visible quad nodes
extern INDEX _ctTris;           // tris rendered
extern INDEX _ctDelayedNodes;   // DelayedNodes 
extern INDEX ter_bLerpVertices; // prepare smoth vertices before rendering

// Vertex array for calculating smoth vertices
CStaticStackArray<GFXVertex4>  _avLerpedVerices;
CStaticStackArray<GFXVertex4>  _avLerpedTileLayerVertices;

// Arrays for batch rendering of tiles is lowest mip
static CStaticStackArray<GFXVertex4>  _avDelayedVertices;
static CStaticStackArray<INDEX_T>     _aiDelayedIndices;
static CStaticStackArray<GFXTexCoord> _auvDelayedTexCoords;
static CStaticStackArray<GFXTexCoord> _auvDelayedShadowMapTC;

typedef FLOAT Matrix16[16];
typedef FLOAT Matrix12[12];

static void RenderFogLayer(INDEX itt);
static void RenderHazeLayer(INDEX itt);

FLOATaabbox3D _bboxDrawNextFrame; // TEMP

SLONG GetUsedMemoryForTileBatching(void)
{
  SLONG slUsedMemory = 0;
  slUsedMemory += _avDelayedVertices.sa_Count * sizeof(GFXVertex4);
  slUsedMemory += _aiDelayedIndices.sa_Count * sizeof(INDEX);
  slUsedMemory += _auvDelayedTexCoords.sa_Count * sizeof(GFXTexCoord);
  slUsedMemory += _auvDelayedShadowMapTC.sa_Count * sizeof(GFXTexCoord);
  return slUsedMemory;
}

CStaticStackArray<GFXColor> _acolVtxConstColors;
static void FillConstColorArray(INDEX ctVertices)
{
  INDEX ctColors=_acolVtxConstColors.Count();
  _acolVtxConstColors.PopAll();
  _acolVtxConstColors.Push(ctVertices);
  // if requested array is larger then existing one
  if(ctVertices>ctColors) {
    memset(&_acolVtxConstColors[ctColors],255,(ctVertices-ctColors)*sizeof(GFXColor));
  }
}

// Regenerate one tile
void ReGenerateTile(INDEX itt)
{
  ASSERT(_ptrTerrain!=NULL);
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[itt];
  tt.ReGenerate();
}

// Convert matrix12 to 
void CreateOpenGLMatrix(Matrix12 &m12,Matrix16 &mgl16)
{
  mgl16[ 0] = m12[ 0];  mgl16[ 1] = m12[ 4];  mgl16[ 2] = m12[ 8];  mgl16[ 3] = 0;
  mgl16[ 4] = m12[ 1];  mgl16[ 5] = m12[ 5];  mgl16[ 6] = m12[ 9];  mgl16[ 7] = 0;
  mgl16[ 8] = m12[ 2];  mgl16[ 9] = m12[ 6];  mgl16[10] = m12[10];  mgl16[11] = 0;
  mgl16[12] = m12[ 3];  mgl16[13] = m12[ 7];  mgl16[14] = m12[11];  mgl16[15] = 1;
}

// set given matrix as identity matrix
inline static void SetMatrixDiagonal(Matrix12 &mat,FLOAT fValue)
{
  memset(&mat,0,sizeof(mat));
  mat[0] = fValue;
  mat[5] = fValue;
  mat[10] = fValue;
}

// Set texture matrix
static inline void gfxSetTextureMatrix2(Matrix12 *pMatrix)
{
  /*pglMatrixMode( GL_TEXTURE);
  if(pMatrix==NULL) {
    pglLoadIdentity();
  } else {
    Matrix16 mrot16;
    Matrix16 mtra16;
    CreateOpenGLMatrix(*pMatrix,mrot16);

    Matrix12 mtr12;
    SetMatrixDiagonal(mtr12,1);
    CreateOpenGLMatrix(mtr12,mtra16);
    pglLoadMatrixf(mtra16);
    pglMultMatrixf(mrot16);
  }
  pglMatrixMode(GL_MODELVIEW);*/
}


/*
 *  Render
 */ 
// Prepare scene for terrain rendering
void PrepareScene(CAnyProjection3D &apr, CDrawPort *pdp, CTerrain *ptrTerrain)
{
  ASSERT(ptrTerrain!=NULL);
  ASSERT(ptrTerrain->tr_penEntity!=NULL);

  // Set current terrain
  _ptrTerrain = ptrTerrain;

  // Set drawport
  _pdp = pdp;

  // Prepare and set the projection
  apr->ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  apr->Prepare();
  _aprProjection = apr;
  _pdp->SetProjection( _aprProjection);

  CEntity *pen = ptrTerrain->tr_penEntity;

  // calculate projection of viewer in absolute space
  const FLOATmatrix3D &mViewer = _aprProjection->pr_ViewerRotationMatrix;
  _vViewer(1) = -mViewer(3,1);
  _vViewer(2) = -mViewer(3,2);
  _vViewer(3) = -mViewer(3,3);
  // calculate projection of viewer in object space
  _vViewerObj = _vViewer * !pen->en_mRotation;


  const CPlacement3D &plTerrain = pen->GetLerpedPlacement();

  _mObjectToView  = mViewer * pen->en_mRotation;
  _vObjectToView  = (plTerrain.pl_PositionVector - _aprProjection->pr_vViewerPosition) * mViewer;

  // make transform matrix 
  const FLOATmatrix3D &m = _mObjectToView;
  const FLOAT3D       &v = _vObjectToView;
  FLOAT glm[16];
  glm[0] = m(1,1);  glm[4] = m(1,2);  glm[ 8] = m(1,3);  glm[12] = v(1);
  glm[1] = m(2,1);  glm[5] = m(2,2);  glm[ 9] = m(2,3);  glm[13] = v(2);
  glm[2] = m(3,1);  glm[6] = m(3,2);  glm[10] = m(3,3);  glm[14] = v(3);
  glm[3] = 0;       glm[7] = 0;       glm[11] = 0;       glm[15] = 1;
  gfxSetViewMatrix(glm);

  // Get viewer in absolute space
  _vViewerAbs = (_aprProjection->ViewerPlacementR().pl_PositionVector - 
                 pen->en_plPlacement.pl_PositionVector) * !pen->en_mRotation;

  gfxDisableBlend();
  gfxDisableTexture();
  gfxDisableAlphaTest();
  gfxEnableDepthTest();
  gfxEnableDepthWrite();
  gfxCullFace(GFX_BACK);
}


__forceinline void Lerp(GFXVertex &vResult, const GFXVertex &vOriginal, const GFXVertex &v1, const GFXVertex &v2, const FLOAT &fFactor)
{
  FLOAT fHalfPosY = Lerp(v1.y,v2.y,0.5f);
  vResult.x = vOriginal.x;
  vResult.y = Lerp(vOriginal.y, fHalfPosY, fFactor);
  vResult.z = vOriginal.z;
}

void PrepareSmothVertices(INDEX itt)
{
  CTerrainTile &tt  = _ptrTerrain->tr_attTiles[itt];
  const INDEX ctVertices = tt.GetVertices().Count();
  const FLOAT &fLerpFactor = tt.tt_fLodLerpFactor;

  // Allocate memory for all vertices
  _avLerpedVerices.PopAll();
  _avLerpedVerices.Push(ctVertices);
  
  // Get pointers to src and dst vertex arrays
  GFXVertex *pavSrcFirst = &tt.GetVertices()[0];
  GFXVertex *pavDstFirst = &_avLerpedVerices[0];
  GFXVertex *pavSrc = &pavSrcFirst[0];
  GFXVertex *pavDst = &pavDstFirst[0];

  INDEX iFacing=0;
  // for each vertex column
  for(INDEX iy=0;iy<tt.tt_ctLodVtxY;iy++) {
    // for each vertex in row in even column step by 2
    for(INDEX ix=0;ix<tt.tt_ctLodVtxX-2;ix+=2) {
      // Copy first vertex
      pavDst[0] = pavSrc[0];
      // Second vertex is lerped between left and right vertices
      Lerp(pavDst[1],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
      // Increment vertex pointers
      pavDst+=2;
      pavSrc+=2;
    }
    // Copy last vertex in row
    pavDst[0] = pavSrc[0];
   
    // Increment vertex pointers and go to odd column 
    pavDst++;
    pavSrc++;
    iy++;

    // if this is not last row
    if(iy<tt.tt_ctLodVtxY) {
      // for each vertex in row in odd column step by 2
      for(INDEX ix=0;ix<tt.tt_ctLodVtxX-2;ix+=2) {
        // First vertex is lerped between top and bottom vertices
        Lerp(pavDst[0],pavSrc[0],pavSrc[-tt.tt_ctLodVtxX],pavSrc[tt.tt_ctLodVtxX],fLerpFactor);
        // is this odd vertex in row
        //#pragma message(">> Fix this")
        if(((ix+iy)/2)%2) {
        // if(iFacing&1)
          // Second vertex (diagonal one) is lerped between topright and bottom left vertices
          Lerp(pavDst[1],pavSrc[1],pavSrc[-tt.tt_ctLodVtxX+2],pavSrc[tt.tt_ctLodVtxX],fLerpFactor);
        } else {
          // Second vertex (diagonal one) is lerped between topleft and bottom right vertices
          Lerp(pavDst[1],pavSrc[1],pavSrc[-tt.tt_ctLodVtxX],pavSrc[tt.tt_ctLodVtxX+2],fLerpFactor);
        }
        iFacing++;
        // Increment vertex pointers
        pavDst+=2;
        pavSrc+=2;
      }
      // Last vertex in row is lerped between top and bottom vertices (same as first in row)
      Lerp(pavDst[0],pavSrc[0],pavSrc[-tt.tt_ctLodVtxX],pavSrc[tt.tt_ctLodVtxX],fLerpFactor);
    }
    // Increment vertex pointers
    pavDst++;
    pavSrc++;
  }

  pavDst--;
  pavSrc--;
/*
  // Copy border vertices
  GFXVertex *pvBorderDst = pavDst;
  GFXVertex *pvBorderSrc = pavSrc;

  for(INDEX ivx=tt.tt_ctNonBorderVertices;ivx<ctVertices;ivx++) {
    // *pavDst++ = *pavSrc++;
    pvBorderDst[0] = pvBorderSrc[0];
    pvBorderDst++;
    pvBorderSrc++;
  }
*/
  // Lerp top border vertices
  const INDEX &iTopNeigbour = tt.tt_aiNeighbours[NB_TOP];
  // if top neighbour exists
  if(iTopNeigbour>=0) {
    CTerrainTile &ttTop = _ptrTerrain->tr_attTiles[iTopNeigbour];
    const FLOAT &fLerpFactor = ttTop.tt_fLodLerpFactor;
    // Get source vertex pointer in top neighbour (vertex in bottom left corner of top neighbour)
    const INDEX iSrcVtx = ttTop.tt_ctLodVtxX * (ttTop.tt_ctLodVtxY-1);
    GFXVertex *pavSrc = &ttTop.GetVertices()[iSrcVtx];
    // Calculate num of vertices that needs to be lerped
    const INDEX ctLerps = (ttTop.tt_ctLodVtxX-1)/2;

    // is top tile in same lod as this tile and has smaller or equal lerp factor
    if(tt.tt_iLod==ttTop.tt_iLod && fLerpFactor<=tt.tt_fLodLerpFactor) {
      // Get destination vertex pointer in this tile (first vertex in top left corner of this tile - first vertex in array)
      const INDEX iDstVtx = 0;
      GFXVertex *pavDst = &pavDstFirst[iDstVtx];

      // for each vertex in bottom row of top tile that needs to be lerped
      for(INDEX ivx=0;ivx<ctLerps;ivx++) {
        // First vertex is same as in top tile
        pavDst[0] = pavSrc[0];
        // Second vertex is lerped between left and right vertices
        Lerp(pavDst[1],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
        pavDst+=2;
        pavSrc+=2;
      }
    // is top tile in higher lod
    } else if(tt.tt_iLod>ttTop.tt_iLod) {
      const INDEX iVtxDiff = (ttTop.tt_ctLodVtxX-1) / (tt.tt_ctLodVtxX-1);
      // Get destination vertex pointer to copy vertices from top neighbour (first vertex in top left corner of this tile - first vertex in array)
      // Get destination vertex pointer to lerp vertices from top neighbour (first vertex added as additional top border vertex)
      const INDEX iDstCopyVtx = 0;
      const INDEX iDstLerpVtx = tt.tt_iFirstBorderVertex[NB_TOP];
      GFXVertex *pavDstCopy = &pavDstFirst[iDstCopyVtx];
      GFXVertex *pavDstLerp = &pavDstFirst[iDstLerpVtx];

      // if diference is in one lod
      if(iVtxDiff==2) {
        // for each vertex in bottom row of top tile that needs to be lerped
        for(INDEX ivx=0;ivx<ctLerps;ivx++) {
          // Copy src vertex in normal dst vertex array
          pavDstCopy[0] = pavSrc[0];
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
          pavDstLerp++;
          pavDstCopy++;
          pavSrc+=2;
        }
      // diference is more than one lod
      } else {
        INDEX ctbv = tt.tt_ctBorderVertices[NB_TOP];
        INDEX ivxInQuad = 2; // This is 2 cos first and last non border vertex 
        // for each border vertex
        for(INDEX ivx=0;ivx<ctbv;ivx+=2) {
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
          // if this border vertex is not last in quad
          if(ivxInQuad!=iVtxDiff) {
            // Copy second border vertex
            pavDstLerp[1] = pavSrc[2];
            pavDstLerp+=2;
            ivxInQuad+=2;
          // this is last border vertex
          } else {
            // Copy second non border vertex
            pavDstCopy[1] = pavSrc[2];
            pavDstCopy++;
            // since this wasn't border vertex, fix border vertex loop counter
            ctbv++;
            pavDstLerp++;
            ivxInQuad=2;
          }
          pavSrc+=2;
        }
      }
    }
  }

  // Lerp bottom border vertices
  const INDEX &iBottomNeigbour = tt.tt_aiNeighbours[NB_BOTTOM];
  // if bottom neighbour exists
  if(iBottomNeigbour>=0) {
    CTerrainTile &ttBottom = _ptrTerrain->tr_attTiles[iBottomNeigbour];
    const FLOAT &fLerpFactor = ttBottom.tt_fLodLerpFactor;
    // Get source vertex pointer in bottom neighbour (vertex in top left corner of bottom neighbour - first vertex in array) 
    const INDEX iSrcVtx = 0;
    GFXVertex *pavSrc = &ttBottom.GetVertices()[iSrcVtx];
    // Calculate num of vertices that needs to be lerped
    const INDEX ctLerps = (ttBottom.tt_ctLodVtxX-1)/2;

    // is bottom tile in same lod as this tile and has smaller lerp factor
    if(tt.tt_iLod==ttBottom.tt_iLod && fLerpFactor<tt.tt_fLodLerpFactor) {
      // Get destination vertex pointer in this tile (first vertex in bottom left corner of this tile)
      const INDEX iDstVtx = tt.tt_ctLodVtxX * (tt.tt_ctLodVtxY-1);
      GFXVertex *pavDst = &pavDstFirst[iDstVtx];

      // for each vertex in top row of bottom tile that needs to be lerped
      for(INDEX ivx=0;ivx<ctLerps;ivx++) {
        // First vertex is same as in bottom tile
        pavDst[0] = pavSrc[0];
        // Second vertex is lerped between left and right vertices
        Lerp(pavDst[1],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
        pavDst+=2;
        pavSrc+=2;
      }
    // is bottom tile in higher lod
    } else if(tt.tt_iLod>ttBottom.tt_iLod) {
      const INDEX iVtxDiff = (ttBottom.tt_ctLodVtxX-1) / (tt.tt_ctLodVtxX-1);
      // Get destination vertex pointer to copy vertices from bottom neighbour (first vertex in bottom left corner of this tile)
      // Get destination vertex pointer to lerp vertices from bottom neighbour (first vertex added as additional bottom border vertex)
      const INDEX iDstCopyVtx = tt.tt_ctLodVtxX * (tt.tt_ctLodVtxY-1);
      const INDEX iDstLerpVtx = tt.tt_iFirstBorderVertex[NB_BOTTOM];
      GFXVertex *pavDstCopy = &pavDstFirst[iDstCopyVtx];
      GFXVertex *pavDstLerp = &pavDstFirst[iDstLerpVtx];

      // if diference is in one lod
      if(iVtxDiff==2) {
        // for each vertex in top row of bottom tile that needs to be lerped
        for(INDEX ivx=0;ivx<ctLerps;ivx++) {
          // Copy src vertex in normal dst vertex array
          pavDstCopy[0] = pavSrc[0];
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
          pavDstLerp++;
          pavDstCopy++;
          pavSrc+=2;
        }
      // diference is more than one lod
      } else {
        INDEX ctbv = tt.tt_ctBorderVertices[NB_BOTTOM];
        INDEX ivxInQuad = 2; // This is 2 cos first and last non border vertex 
        // for each border vertex
        for(INDEX ivx=0;ivx<ctbv;ivx+=2) {
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[1],pavSrc[0],pavSrc[2],fLerpFactor);
          // if this border vertex is not last in quad
          if(ivxInQuad!=iVtxDiff) {
            // Copy second border vertex
            pavDstLerp[1] = pavSrc[2];
            pavDstLerp+=2;
            ivxInQuad+=2;
          // this is last border vertex
          } else {
            // Copy second non border vertex
            pavDstCopy[1] = pavSrc[2];
            pavDstCopy++;
            // since this wasn't border vertex, fix border vertex loop counter
            ctbv++;
            pavDstLerp++;
            ivxInQuad=2;
          }
          pavSrc+=2;
        }
      }
    }
  }

  // Lerp left border vertices
  const INDEX &iLeftNeigbour = tt.tt_aiNeighbours[NB_LEFT];
  // if left neighbour exists
  if(iLeftNeigbour>=0) {
    CTerrainTile &ttLeft = _ptrTerrain->tr_attTiles[iLeftNeigbour];
    const FLOAT &fLerpFactor = ttLeft.tt_fLodLerpFactor;
    // Get source vertex pointer in left neighbour (vertex in top right corner of left neighbour) 
    const INDEX iSrcVtx = ttLeft.tt_ctLodVtxX-1;
    const INDEX iSrcStep = ttLeft.tt_ctLodVtxX;
    GFXVertex *pavSrc = &ttLeft.GetVertices()[iSrcVtx];
    // Calculate num of vertices that needs to be lerped
    const INDEX ctLerps = (ttLeft.tt_ctLodVtxX-1)/2;

    // is left tile in same lod as this tile and has smaller or equal lerp factor
    if(tt.tt_iLod==ttLeft.tt_iLod && fLerpFactor<=tt.tt_fLodLerpFactor) {
      // Get destination vertex pointer in this tile (first vertex in top left corner of this tile - first vertex in array)
      const INDEX iDstVtx = 0;
      const INDEX iDstStep = tt.tt_ctLodVtxX;
      GFXVertex *pavDst = &pavDstFirst[iDstVtx];

      // for each vertex in last column of left tile that needs to be lerped
      for(INDEX ivx=0;ivx<ctLerps;ivx++) {
        // First vertex is same as in left tile
        pavDst[0] = pavSrc[0];
        // Second vertex is lerped between top and bottom vertices
        Lerp(pavDst[iDstStep],pavSrc[iSrcStep],pavSrc[0],pavSrc[iSrcStep*2],fLerpFactor);
        pavDst+=iDstStep*2;
        pavSrc+=iSrcStep*2;
      }
    // is left tile in higher lod
    } else if(tt.tt_iLod>ttLeft.tt_iLod) {
      const INDEX iVtxDiff = (ttLeft.tt_ctLodVtxX-1) / (tt.tt_ctLodVtxX-1);
      // Get destination vertex pointer to copy vertices from left neighbour (first vertex in top left corner of this tile - first vertex in array)
      // Get destination vertex pointer to lerp vertices from left neighbour (first vertex added as additional left border vertex)
      const INDEX iDstCopyVtx = 0;
      const INDEX iDstLerpVtx = tt.tt_iFirstBorderVertex[NB_LEFT];
      const INDEX iDstStep = tt.tt_ctLodVtxX;
      GFXVertex *pavDstCopy = &pavDstFirst[iDstCopyVtx];
      GFXVertex *pavDstLerp = &pavDstFirst[iDstLerpVtx];

      // if diference is in one lod
      if(iVtxDiff==2) {
        // for each vertex in last column of left tile that needs to be lerped
        for(INDEX ivx=0;ivx<ctLerps;ivx++) {
          // Copy src vertex in normal dst vertex array
          pavDstCopy[0] = pavSrc[0];
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[iSrcStep],pavSrc[0],pavSrc[iSrcStep*2],fLerpFactor);
          pavDstLerp++;
          pavDstCopy+=iDstStep;
          pavSrc+=iSrcStep*2;
        }
      // diference is more than one lod
      } else {
        INDEX ctbv = tt.tt_ctBorderVertices[NB_LEFT];
        INDEX ivxInQuad = 2; // This is 2 cos first and last non border vertex 
        // for each border vertex
        for(INDEX ivx=0;ivx<ctbv;ivx+=2) {
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[iSrcStep],pavSrc[0],pavSrc[iSrcStep*2],fLerpFactor);
          // if this border vertex is not last in quad
          if(ivxInQuad!=iVtxDiff) {
            // Copy second border vertex
            pavDstLerp[1] = pavSrc[iSrcStep*2];
            pavDstLerp+=2;
            ivxInQuad+=2;
          // this is last border vertex
          } else {
            // Copy second non border vertex
            pavDstCopy[iDstStep] = pavSrc[iSrcStep*2];
            pavDstCopy+=iDstStep;
            // since this wasn't border vertex, fix border vertex loop counter
            ctbv++;
            pavDstLerp++;
            ivxInQuad=2;
          }
          pavSrc+=iSrcStep*2;
        }
      }
    }
  }

  // Lerp right border vertices
  const INDEX &iRightNeigbour = tt.tt_aiNeighbours[NB_RIGHT];
  // if right neighbour exists
  if(iRightNeigbour>=0) {
    CTerrainTile &ttRight = _ptrTerrain->tr_attTiles[iRightNeigbour];
    const FLOAT &fLerpFactor = ttRight.tt_fLodLerpFactor;
    // Get source vertex pointer in right neighbour (vertex in top left corner of left neighbour - first vertex in array) 
    const INDEX iSrcVtx = 0;
    const INDEX iSrcStep = ttRight.tt_ctLodVtxX;
    GFXVertex *pavSrc = &ttRight.GetVertices()[iSrcVtx];
    // Calculate num of vertices that needs to be lerped
    const INDEX ctLerps = (ttRight.tt_ctLodVtxX-1)/2;

    // is right tile in same lod as this tile and has smaller lerp factor
    if(tt.tt_iLod==ttRight.tt_iLod && fLerpFactor<tt.tt_fLodLerpFactor) {
      // Get destination vertex pointer in this tile (first vertex in top right corner of this tile)
      INDEX iDstVtx = tt.tt_ctLodVtxX-1;
      INDEX iDstStep = tt.tt_ctLodVtxX;
      GFXVertex *pavDst = &pavDstFirst[iDstVtx];

      // for each vertex in first column of right tile that needs to be lerped
      for(INDEX ivx=0;ivx<ctLerps;ivx++) {
        // First vertex is same as in right tile
        pavDst[0] = pavSrc[0];
        // Second vertex is lerped between top and bottom vertices
        Lerp(pavDst[iDstStep],pavSrc[iSrcStep],pavSrc[0],pavSrc[iSrcStep*2],fLerpFactor);
        pavDst+=iDstStep*2;
        pavSrc+=iSrcStep*2;
      }
    // is right tile in higher lod
    } else if(tt.tt_iLod>ttRight.tt_iLod) {
      const INDEX iVtxDiff = (ttRight.tt_ctLodVtxX-1) / (tt.tt_ctLodVtxX-1);
      // Get destination vertex pointer to copy vertices from right neighbour (first vertex in top right corner of this tile)
      // Get destination vertex pointer to lerp vertices from right neighbour (first vertex added as additional right border vertex)
      const INDEX iDstCopyVtx = tt.tt_ctLodVtxX-1;
      const INDEX iDstLerpVtx = tt.tt_iFirstBorderVertex[NB_RIGHT];
      const INDEX iDstStep = tt.tt_ctLodVtxX;
      GFXVertex *pavDstCopy = &pavDstFirst[iDstCopyVtx];
      GFXVertex *pavDstLerp = &pavDstFirst[iDstLerpVtx];

      // if diference is in one lod
      if(iVtxDiff==2) {
        // for each vertex in first column of right tile that needs to be lerped
        for(INDEX ivx=0;ivx<ctLerps;ivx++) {
          // Copy src vertex in normal dst vertex array
          pavDstCopy[0] = pavSrc[0];
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[iSrcStep],pavSrc[0],pavSrc[iSrcStep*2],fLerpFactor);
          pavDstLerp++;
          pavDstCopy+=iDstStep;
          pavSrc+=iSrcStep*2;
        }
      // diference is more than one lod
      } else {
        INDEX ctbv = tt.tt_ctBorderVertices[NB_RIGHT];
        INDEX ivxInQuad = 2; // This is 2 cos first and last non border vertex 
        // for each border vertex
        for(INDEX ivx=0;ivx<ctbv;ivx+=2) {
          // Lerp left and right src vertices in border dst vertex
          Lerp(pavDstLerp[0],pavSrc[iSrcStep],pavSrc[0],pavSrc[iSrcStep*2],fLerpFactor);
          // if this border vertex is not last in quad
          if(ivxInQuad!=iVtxDiff) {
            // Copy second border vertex
            pavDstLerp[1] = pavSrc[iSrcStep*2];
            pavDstLerp+=2;
            ivxInQuad+=2;
          // this is last border vertex
          } else {
            // Copy second non border vertex
            pavDstCopy[iDstStep] = pavSrc[iSrcStep*2];
            pavDstCopy+=iDstStep;
            // since this wasn't border vertex, fix border vertex loop counter
            ctbv++;
            pavDstLerp++;
            ivxInQuad=2;
          }
          pavSrc+=iSrcStep*2;
        }
      }
    }
  }
}

void PrepareSmothVerticesOnTileLayer(INDEX iTerrainTile, INDEX iTileLayer)
{
  CTerrainTile &tt  = _ptrTerrain->tr_attTiles[iTerrainTile];
  //CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[iTileLayer];
  TileLayer &ttl    = tt.GetTileLayers()[iTileLayer];

  ASSERT(tt.tt_iLod==0);

  const INDEX ctVertices = ttl.tl_avVertices.Count();
  const FLOAT &fLerpFactor = tt.tt_fLodLerpFactor;

  // Allocate memory for all vertices
  _avLerpedTileLayerVertices.PopAll();
  _avLerpedTileLayerVertices.Push(ctVertices);
  
  // Get pointers to src and dst vertex arrays
  GFXVertex *pavSrcFirst = &ttl.tl_avVertices[0];
  GFXVertex *pavDstFirst = &_avLerpedTileLayerVertices[0];
  GFXVertex *pavSrc = &pavSrcFirst[0];
  GFXVertex *pavDst = &pavDstFirst[0];

  INDEX ctQuadsPerRow   = _ptrTerrain->tr_ctQuadsInTileRow;
  INDEX ctVerticesInRow = _ptrTerrain->tr_ctQuadsInTileRow*2;
  INDEX iFacing = 1;

  // Minimize popping on vertices using 4 quads, 2 from current row and 2 from next row in same tile
  for(INDEX iz=0;iz<ctQuadsPerRow;iz+=2) {
    for(INDEX ix=0;ix<ctQuadsPerRow;ix+=2) {
      // Get pointer for quads in next row
      GFXVertex *pavNRSrc = &pavSrc[ctVerticesInRow*2];
      GFXVertex *pavNRDst = &pavDst[ctVerticesInRow*2];

      pavDst[0]   = pavSrc[0];
      Lerp(pavDst[1],pavSrc[1],pavSrc[0],pavSrc[5],fLerpFactor);
      Lerp(pavDst[2],pavSrc[2],pavSrc[0],pavNRSrc[2],fLerpFactor);

      if(iFacing&1) {
        Lerp(pavDst[3],pavSrc[3],pavSrc[0],pavNRSrc[7],fLerpFactor);
      } else {
        Lerp(pavDst[3],pavSrc[3],pavSrc[5],pavNRSrc[2],fLerpFactor);
      }

      pavDst[4]   = pavDst[1];
      pavDst[5]   = pavSrc[5];
      pavDst[6]   = pavDst[3];
      Lerp(pavDst[7],pavSrc[7],pavSrc[5],pavNRSrc[7],fLerpFactor);
      pavNRDst[0] = pavDst[2];
      pavNRDst[1] = pavDst[3];
      pavNRDst[2] = pavNRSrc[2];
      Lerp(pavNRDst[3],pavNRSrc[3],pavNRSrc[2],pavNRSrc[7],fLerpFactor);
      pavNRDst[4] = pavDst[3];
      pavNRDst[5] = pavDst[7];
      pavNRDst[6] = pavNRDst[3];
      pavNRDst[7] = pavNRSrc[7];

      // Increment vertex pointers
      pavSrc+=8;
      pavDst+=8;
      iFacing++;
    }
    iFacing++;
    pavSrc+=ctVerticesInRow*2;
    pavDst+=ctVerticesInRow*2;
  }

  // Lerp top border
  INDEX iTopNeighbour = tt.tt_aiNeighbours[NB_TOP];
  // if top border exists
  if(iTopNeighbour>=0) {
    CTerrainTile &ttTop = _ptrTerrain->tr_attTiles[iTopNeighbour];
    const FLOAT fTopLerpFactor = ttTop.tt_fLodLerpFactor;
    // is top tile in highest lod and has smaller or equal lerp factor
    if(ttTop.tt_iLod==0 && fTopLerpFactor<=fLerpFactor) {
      TileLayer &ttl = ttTop.GetTileLayers()[iTileLayer];
      INDEX iFirstVertex = ctVerticesInRow*(ctVerticesInRow-2);
      GFXVertex *pavSrc = &ttl.tl_avVertices[iFirstVertex];
      GFXVertex *pavDst = &_avLerpedTileLayerVertices[0];
      // for each quad
      for(INDEX ix=0;ix<ctQuadsPerRow;ix+=2) {
        Lerp(pavDst[1],pavSrc[6],pavSrc[2],pavSrc[7],fTopLerpFactor);
        pavDst[4] = pavDst[1];
        pavSrc+=8;
        pavDst+=8;
      }
    }
  }

  // Lerp bottom border
  INDEX iBottomNeighbour = tt.tt_aiNeighbours[NB_BOTTOM];
  // if bottom border exists
  if(iBottomNeighbour>=0) {
    CTerrainTile &ttBottom = _ptrTerrain->tr_attTiles[iBottomNeighbour];
    const FLOAT fBottomLerpFactor = ttBottom.tt_fLodLerpFactor;
    // is bottom tile in highest lod and has smaller lerp factor
    if(ttBottom.tt_iLod==0 && fBottomLerpFactor<fLerpFactor) {
      TileLayer &ttl = ttBottom.GetTileLayers()[iTileLayer];
      INDEX iFirstVertex = ctVerticesInRow*(ctVerticesInRow-2);
      GFXVertex *pavSrc = &ttl.tl_avVertices[0];
      GFXVertex *pavDst = &_avLerpedTileLayerVertices[iFirstVertex];
      // for each quad
      for(INDEX ix=0;ix<ctQuadsPerRow;ix+=2) {
        Lerp(pavDst[3],pavSrc[1],pavSrc[0],pavSrc[5],fBottomLerpFactor);
        pavDst[6] = pavDst[3];
        pavSrc+=8;
        pavDst+=8;
      }
    }
  }

  // Lerp left border
  INDEX iLeftNeighbour = tt.tt_aiNeighbours[NB_LEFT];
  // if left neightbour exits
  if(iLeftNeighbour>=0) {
    CTerrainTile &ttLeft = _ptrTerrain->tr_attTiles[iLeftNeighbour];
    const FLOAT fLeftLerpFactor = ttLeft.tt_fLodLerpFactor;
    // is left tile in highest lod and has smaller or equal left factor
    if(ttLeft.tt_iLod==0 && fLeftLerpFactor<=fLerpFactor) {
      TileLayer &ttl = ttLeft.GetTileLayers()[iTileLayer];
      INDEX iFirstVertex = ctVerticesInRow*2-8;
      GFXVertex *pavSrc = &ttl.tl_avVertices[iFirstVertex];
      GFXVertex *pavDst = &_avLerpedTileLayerVertices[0];
      // for each quad
      for(INDEX ix=0;ix<ctQuadsPerRow;ix+=2) {
        GFXVertex *pavNRSrc = &pavSrc[ctVerticesInRow*2];
        GFXVertex *pavNRDst = &pavDst[ctVerticesInRow*2];

        Lerp(pavDst[2],pavSrc[7],pavSrc[5],pavNRSrc[7],fLeftLerpFactor);
        pavNRDst[0] = pavDst[2];
        pavSrc+=ctVerticesInRow*4;
        pavDst+=ctVerticesInRow*4;
      }
    }
  }

  // Lerp right border
  INDEX iRightNeighbour = tt.tt_aiNeighbours[NB_RIGHT];
  // if right neightbour exits
  if(iRightNeighbour>=0) {
    CTerrainTile &ttRight = _ptrTerrain->tr_attTiles[iRightNeighbour];
    const FLOAT fRightLerpFactor = ttRight.tt_fLodLerpFactor;
    // is right tile in highest lod and has smaller left factor
    if(ttRight.tt_iLod==0 && fRightLerpFactor<fLerpFactor) {
      TileLayer &ttl = ttRight.GetTileLayers()[iTileLayer];
      INDEX iFirstVertex = ctVerticesInRow*2-8;
      GFXVertex *pavSrc = &ttl.tl_avVertices[0];
      GFXVertex *pavDst = &_avLerpedTileLayerVertices[iFirstVertex];
      // for each quad
      for(INDEX ix=0;ix<ctQuadsPerRow;ix+=2) {
        GFXVertex *pavNRSrc = &pavSrc[ctVerticesInRow*2];
        GFXVertex *pavNRDst = &pavDst[ctVerticesInRow*2];

        Lerp(pavDst[7],pavSrc[2],pavSrc[0],pavNRSrc[2],fRightLerpFactor);
        pavNRDst[5] = pavDst[7];
        pavSrc+=ctVerticesInRow*4;
        pavDst+=ctVerticesInRow*4;
      }
    }
  }
}


// Draw all tiles that are in lowest lod
static void RenderBatchedTiles(void)
{
  // Set texture wrapping
  gfxSetTextureWrapping(GFX_CLAMP,GFX_CLAMP);
  // Use terrains global top map as texture
  _ptrTerrain->tr_tdTopMap.SetAsCurrent();

  GFXVertex4  *pavVertices     = &_avDelayedVertices[0];
  GFXTexCoord *pauvTexCoords   = &_auvDelayedTexCoords[0];
  GFXTexCoord *pauvShadowMapTC = &_auvDelayedShadowMapTC[0];
  INDEX_T     *paiIndices      = &_aiDelayedIndices[0];
  INDEX        ctVertices      = _avDelayedVertices.Count();
  INDEX        ctIndices       = _aiDelayedIndices.Count();

  // Prepare white color array
  FillConstColorArray(ctVertices);
  GFXColor    *pacolColors     = &_acolVtxConstColors[0];

  gfxEnableAlphaTest();
  gfxDisableBlend();
  gfxSetVertexArray(pavVertices,ctVertices);
  gfxSetTexCoordArray(pauvTexCoords, FALSE);
  gfxSetColorArray(pacolColors);
  gfxLockArrays();
  gfxDrawElements(ctIndices,paiIndices);
  gfxDisableAlphaTest();
  _ctTris +=ctIndices/2;

  // if shadows are visible
  if(_wrpWorldRenderPrefs.wrp_shtShadows!=CWorldRenderPrefs::SHT_NONE) {
    gfxDepthFunc(GFX_EQUAL);

    gfxBlendFunc(GFX_DST_COLOR,GFX_SRC_COLOR);
    gfxEnableBlend();
    gfxSetTexCoordArray(pauvShadowMapTC, FALSE);
    _ptrTerrain->tr_tdShadowMap.SetAsCurrent();
    gfxDrawElements(ctIndices,paiIndices);
    gfxDepthFunc(GFX_LESS_EQUAL);
  }

  if(_ptrTerrain->GetFlags()&TR_HAS_FOG) {
    RenderFogLayer(-1);
  }
  if(_ptrTerrain->GetFlags()&TR_HAS_HAZE) {
    RenderHazeLayer(-1);
  }
  gfxUnlockArrays();

  // Popall delayed arrays 
  _avDelayedVertices.PopAll();
  _auvDelayedTexCoords.PopAll();
  _auvDelayedShadowMapTC.PopAll();
  _aiDelayedIndices.PopAll();
}

static void BatchTile(INDEX itt)
{
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[itt];
  ASSERT(tt.GetVertices().Count()==9);
  ASSERT(tt.GetIndices().Count()==24);

  INDEX ctDelayedVertices = _avDelayedVertices.Count();
  
  GFXVertex4  *pavVertices        = &tt.GetVertices()[0];
  GFXTexCoord *pauvTexCoords      = &tt.GetTexCoords()[0];
  GFXTexCoord *pauvShadowMapTC    = &tt.GetShadowMapTC()[0];
  INDEX_T     *paiIndices         = &tt.GetIndices()[0];

  GFXVertex4  *pavDelVertices     = _avDelayedVertices.Push(9);
  GFXTexCoord *pauvDelTexCoords   = _auvDelayedTexCoords.Push(9);
  GFXTexCoord *pauvDelShadowMapTC = _auvDelayedShadowMapTC.Push(9);
  INDEX_T     *paiDelIndices      = _aiDelayedIndices.Push(24);

  // for each vertex in tile
  for(INDEX ivx=0;ivx<9;ivx++) {
    // copy vertex, texcoord & shadow map texcoord to delayed array
    pavDelVertices[ivx]     = pavVertices[ivx];
    pauvDelTexCoords[ivx]   = pauvTexCoords[ivx];
    pauvDelShadowMapTC[ivx] = pauvShadowMapTC[ivx];
  }
  // for each index in tile
  for(INDEX iind=0;iind<24;iind++) {
    // reindex indice for new arrays
    paiDelIndices[iind] = paiIndices[iind] + ctDelayedVertices;
  }

  _ctDelayedNodes++;
}

// returns haze/fog value in vertex 
static FLOAT3D _vFViewerObj, _vHDirObj;
static FLOAT   _fFogAddZ, _fFogAddH;
static FLOAT   _fHazeAdd;

// check vertex against haze
//#pragma message(">> no asm in GetHazeMapInVertex and GetFogMapInVertex")
static void GetHazeMapInVertex( GFXVertex4 &vtx, GFXTexCoord &txHaze)
{
  const FLOAT fD = vtx.x*_vViewerObj(1) + vtx.y*_vViewerObj(2) + vtx.z*_vViewerObj(3);
  txHaze.uv.u = (fD + _fHazeAdd) * _haze_fMul;
  txHaze.uv.v = 0.0f;
}

static void GetFogMapInVertex( GFXVertex4 &vtx, GFXTexCoord &tex)
{
  const FLOAT fD = vtx.x*_vFViewerObj(1) + vtx.y*_vFViewerObj(2) + vtx.z*_vFViewerObj(3);
  const FLOAT fH = vtx.x*_vHDirObj(1)    + vtx.y*_vHDirObj(2)    + vtx.z*_vHDirObj(3);
  tex.uv.u = (fD + _fFogAddZ) * _fog_fMulZ;
  tex.uv.v = (fH + _fFogAddH) * _fog_fMulH;
}

static CStaticStackArray<GFXTexCoord> _atcHaze;
static CStaticStackArray<GFXColor>    _acolHaze;

static void RenderFogLayer(INDEX itt)
{
  FLOATmatrix3D &mViewer = _aprProjection->pr_ViewerRotationMatrix;
  FLOAT3D vObjPosition = _ptrTerrain->tr_penEntity->en_plPlacement.pl_PositionVector;

  // get viewer -z in object space
  _vFViewerObj = FLOAT3D(0,0,-1) * !_mObjectToView;
  // get fog direction in object space
  _vHDirObj = _fog_vHDirAbs * !(!mViewer*_mObjectToView);
  // get viewer offset
  _fFogAddZ  = _vViewer(1) * (vObjPosition(1) - _aprProjection->pr_vViewerPosition(1));
  _fFogAddZ += _vViewer(2) * (vObjPosition(2) - _aprProjection->pr_vViewerPosition(2));
  _fFogAddZ += _vViewer(3) * (vObjPosition(3) - _aprProjection->pr_vViewerPosition(3));
  // get fog offset
  _fFogAddH = (_fog_vHDirAbs % vObjPosition) + _fog_fp.fp_fH3;

  GFXVertex *pvVtx;
  INDEX_T   *piIndices;
  INDEX ctVertices;
  INDEX ctIndices;
  // if this is tile 
  if(itt>=0) {
    CTerrainTile &tt = _ptrTerrain->tr_attTiles[itt];
    pvVtx      = &tt.GetVertices()[0];
    piIndices  = &tt.GetIndices()[0];
    ctVertices = tt.GetVertices().Count();
    ctIndices  = tt.GetIndices().Count();
  // else this are batched tiles
  } else {
    pvVtx      = &_avDelayedVertices[0];
    piIndices  = &_aiDelayedIndices[0];
    ctVertices = _avDelayedVertices.Count();
    ctIndices  = _aiDelayedIndices.Count();
  }

  GFXTexCoord *pfFogTC  = _atcHaze.Push(ctVertices);
  GFXColor    *pcolFog  = _acolHaze.Push(ctVertices);

  const COLOR colF = AdjustColor( _fog_fp.fp_colColor, _slTexHueShift, _slTexSaturation);
  GFXColor colFog(colF);

  // for each vertex in tile
  for(INDEX ivx=0;ivx<ctVertices;ivx++) {
    GetFogMapInVertex(pvVtx[ivx],pfFogTC[ivx]);
    pcolFog[ivx] = colFog;
  }

  // render fog layer
  gfxDepthFunc(GFX_EQUAL);
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxSetTexture( _fog_ulTexture, _fog_tpLocal);
  gfxSetTexCoordArray(pfFogTC, FALSE);
  gfxSetColorArray(pcolFog);
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxEnableBlend();
  gfxDisableAlphaTest();
  gfxDrawElements(ctIndices,piIndices);
  gfxDepthFunc(GFX_LESS_EQUAL);

  _atcHaze.PopAll();
  _acolHaze.PopAll();
}

static void RenderHazeLayer(INDEX itt)
{
  FLOAT3D vObjPosition = _ptrTerrain->tr_penEntity->en_plPlacement.pl_PositionVector;

  _fHazeAdd  = -_haze_hp.hp_fNear;
  _fHazeAdd += _vViewer(1) * (vObjPosition(1) - _aprProjection->pr_vViewerPosition(1));
  _fHazeAdd += _vViewer(2) * (vObjPosition(2) - _aprProjection->pr_vViewerPosition(2));
  _fHazeAdd += _vViewer(3) * (vObjPosition(3) - _aprProjection->pr_vViewerPosition(3));

  GFXVertex *pvVtx;
  INDEX_T   *piIndices;
  INDEX ctVertices;
  INDEX ctIndices;
  // if this is tile 
  if(itt>=0) {
    CTerrainTile &tt = _ptrTerrain->tr_attTiles[itt];
    pvVtx      = &tt.GetVertices()[0];
    piIndices  = &tt.GetIndices()[0];
    ctVertices = tt.GetVertices().Count();
    ctIndices  = tt.GetIndices().Count();
  // else this are batched tiles
  } else {
    pvVtx      = &_avDelayedVertices[0];
    piIndices  = &_aiDelayedIndices[0];
    ctVertices = _avDelayedVertices.Count();
    ctIndices  = _aiDelayedIndices.Count();
  }

  GFXTexCoord *pfHazeTC  = _atcHaze.Push(ctVertices);
  GFXColor    *pcolHaze  = _acolHaze.Push(ctVertices);

  const COLOR colH = AdjustColor( _haze_hp.hp_colColor, _slTexHueShift, _slTexSaturation);
  GFXColor colHaze(colH);
  // for each vertex in tile
  for(INDEX ivx=0;ivx<ctVertices;ivx++) {
    GetHazeMapInVertex(pvVtx[ivx],pfHazeTC[ivx]);
    pcolHaze[ivx] = colHaze;
  }

  // render haze layer
  gfxDepthFunc(GFX_EQUAL);
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxSetTexture( _haze_ulTexture, _haze_tpLocal);
  gfxSetTexCoordArray(pfHazeTC, FALSE);
  gfxSetColorArray(pcolHaze);
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxEnableBlend();
  gfxDrawElements(ctIndices,piIndices);
  gfxDepthFunc(GFX_LESS_EQUAL);

  _atcHaze.PopAll();
  _acolHaze.PopAll();
}

// Render one tile
static void RenderTile(INDEX itt)
{
  ASSERT(_ptrTerrain!=NULL);
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[itt];
  INDEX ctVertices = tt.GetVertices().Count();

  extern INDEX ter_bOptimizeRendering;
  // if tile is in posible lowest lod and doesn't have any border vertices
  if(ter_bOptimizeRendering && tt.GetFlags()&TT_IN_LOWEST_LOD) {
    // delay tile rendering
    BatchTile(itt);
    return;
  }

  GFXVertex4 *pavVertices;
  // if vertex lerping is requested
  if(ter_bLerpVertices==1) {
    // Prepare smoth vertices
    PrepareSmothVertices(itt);
    pavVertices = &_avLerpedVerices[0];
  } else {
    // use non smoth vertices
    pavVertices = &tt.GetVertices()[0];
  }

  // if tile is in highest lod
  if(tt.tt_iLod==0) {
    gfxBlendFunc(GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxSetVertexArray(pavVertices,ctVertices);

    gfxLockArrays();
    // for each tile layer
    INDEX cttl= tt.GetTileLayers().Count();
    for(INDEX itl=0;itl<cttl;itl++) {
      CTerrainLayer &tl = _ptrTerrain->tr_atlLayers[itl];
      // if layer isn't visible
      if(!tl.tl_bVisible) {
        continue; // skip it
      }

      TileLayer &ttl = tt.GetTileLayers()[itl];

      // Set tile stretch
      Matrix12 m12;
      SetMatrixDiagonal(m12,tl.tl_fStretchX);
      gfxSetTextureMatrix2(&m12);

      // Set tile blend mode
      if(tl.tl_fSmoothness==0) {
        gfxDisableBlend();
        gfxEnableAlphaTest();
      } else {
        gfxEnableBlend();
        gfxDisableAlphaTest();
      }

      // if this tile has any polygons in this layer
      INDEX ctIndices = ttl.tl_auiIndices.Count();
      if(ctIndices>0) {
        gfxSetTextureWrapping(GFX_REPEAT,GFX_REPEAT);
        tl.tl_ptdTexture->SetAsCurrent();

        // if this is tile layer 
        if(tl.tl_ltType==LT_TILE) {
          gfxUnlockArrays();
          GFXVertex4 *pavLayerVertices;
          if(ter_bLerpVertices==1) {
            PrepareSmothVerticesOnTileLayer(itt,itl);
            pavLayerVertices = &_avLerpedTileLayerVertices[0];
          } else {
            pavLayerVertices = &ttl.tl_avVertices[0];
          }
          gfxSetVertexArray(pavLayerVertices,ttl.tl_avVertices.Count());
          gfxLockArrays();
          // gfxSetColorArray(&ttl.tl_acColors[0]);
          gfxSetTexCoordArray(&ttl.tl_atcTexCoords[0], FALSE);

          
          // set wireframe mode
          /*
          gfxEnableDepthBias();
          gfxPolygonMode(GFX_LINE);
          gfxDisableTexture();*/
          gfxSetConstantColor(0xFFFFFFFF);

          // Draw tiled layer
          gfxDrawElements(ttl.tl_auiIndices.Count(),&ttl.tl_auiIndices[0]);
          _ctTris +=ttl.tl_auiIndices.Count()/2;

          /*
          // set fill mode
          gfxDisableDepthBias();
          gfxPolygonMode(GFX_FILL);*/

          // Set old vertex array
          gfxUnlockArrays();
          gfxSetVertexArray(pavVertices,ctVertices);
          gfxLockArrays();
        // if this is normal layer
        } else {
          // render layer
          gfxSetColorArray(&ttl.tl_acColors[0]);
          gfxSetTexCoordArray(&ttl.tl_atcTexCoords[0], FALSE);
          gfxDrawElements(ctIndices,&ttl.tl_auiIndices[0]);
          _ctTris +=ctIndices/2;
        }
      }
    }
    gfxSetTextureMatrix2(NULL);
    INDEX ctIndices = tt.GetIndices().Count();
    if(ctIndices>0) {
      INDEX_T *paiIndices = &tt.GetIndices()[0];

      // if detail map exists
      if(_ptrTerrain->tr_ptdDetailMap!=NULL) {
        gfxSetTextureWrapping(GFX_REPEAT,GFX_REPEAT);
        gfxDisableAlphaTest();
        shaBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR);
        gfxEnableBlend();
        gfxSetTexCoordArray(&tt.GetDetailTC()[0], FALSE);
        _ptrTerrain->tr_ptdDetailMap->SetAsCurrent();
        gfxDrawElements(ctIndices,paiIndices);
      }

      // if shadows are visible
      if(_wrpWorldRenderPrefs.wrp_shtShadows!=CWorldRenderPrefs::SHT_NONE) {
        gfxDisableAlphaTest();
        shaBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR);
        gfxEnableBlend();
        gfxSetTextureWrapping(GFX_CLAMP,GFX_CLAMP);
        gfxSetTexCoordArray(&tt.GetShadowMapTC()[0], FALSE);
        _ptrTerrain->tr_tdShadowMap.SetAsCurrent();
        gfxDrawElements(ctIndices,paiIndices);
      }
    }
  // if tile is not in highest lod
  } else {
    gfxSetTextureWrapping(GFX_CLAMP,GFX_CLAMP);
    // if tile is in lowest lod
    if(tt.tt_iLod == _ptrTerrain->tr_iMaxTileLod) {
      // use terrains global top map
      _ptrTerrain->tr_tdTopMap.SetAsCurrent();
    // else tile is in some midle lod
    } else {
      // use its own topmap
      tt.GetTopMap()->SetAsCurrent();
    }

    // Render tile
    INDEX ctIndices = tt.GetIndices().Count();
    gfxEnableAlphaTest();
    gfxDisableBlend();
    gfxSetVertexArray(pavVertices,ctVertices);
    gfxSetTexCoordArray(&tt.GetTexCoords()[0], FALSE);
    FillConstColorArray(ctVertices);
    gfxSetColorArray(&_acolVtxConstColors[0]);
    gfxLockArrays();
    gfxDrawElements(ctIndices,&tt.GetIndices()[0]);
    _ctTris +=ctIndices/2;
    gfxDisableAlphaTest();

    // if shadows are visible
    if(_wrpWorldRenderPrefs.wrp_shtShadows!=CWorldRenderPrefs::SHT_NONE) {
      gfxDepthFunc(GFX_EQUAL);
      INDEX ctIndices = tt.GetIndices().Count();
      INDEX_T *paiIndices = &tt.GetIndices()[0];

      gfxSetTextureWrapping(GFX_CLAMP,GFX_CLAMP);
      gfxBlendFunc(GFX_DST_COLOR,GFX_SRC_COLOR);
      gfxEnableBlend();
      gfxSetTexCoordArray(&tt.GetShadowMapTC()[0], FALSE);
      _ptrTerrain->tr_tdShadowMap.SetAsCurrent();
      gfxDrawElements(ctIndices,paiIndices);
      gfxDepthFunc(GFX_LESS_EQUAL);
    }
  }

  if(_ptrTerrain->GetFlags()&TR_HAS_FOG) {
    RenderFogLayer(itt);
  }
  if(_ptrTerrain->GetFlags()&TR_HAS_HAZE) {
    RenderHazeLayer(itt);
  }

  gfxUnlockArrays();
}

// Draw one quad tree node ( draws terrain tile if leaf node )
static void DrawQuadTreeNode(INDEX iqtn)
{
  ASSERT(_ptrTerrain!=NULL);
  CEntity *pen = _ptrTerrain->tr_penEntity;
  QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[iqtn];
  
  FLOATmatrix3D &mAbsToView = _aprProjection->pr_ViewerRotationMatrix;
  FLOATobbox3D obbox = FLOATobbox3D( qtn.qtn_aabbox, 
    (pen->en_plPlacement.pl_PositionVector-_aprProjection->pr_vViewerPosition)*mAbsToView, mAbsToView*pen->en_mRotation);

  INDEX iFrustumTest = _aprProjection->TestBoxToFrustum(obbox);
  if(iFrustumTest!=(-1)) {
    // is this leaf node
    if(qtn.qtn_iTileIndex != -1) {
      _ctNodesVis++;
      // draw terrain tile for this node 
      RenderTile(qtn.qtn_iTileIndex);
    // this node has some children
    } else {
      for(INDEX iqc=0;iqc<4;iqc++) {
        INDEX iChildNode = qtn.qtn_iChild[iqc];
        // if child node exists
        if(iChildNode != -1) {
          // draw child node
          DrawQuadTreeNode(qtn.qtn_iChild[iqc]);
        }
      }
    }
  }
}

// Render one terrain
void RenderTerrain(void)
{
  ASSERT(_ptrTerrain!=NULL);
  ASSERT(_ptrTerrain->tr_penEntity!=NULL);
  
  _ctNodesVis = 0;
  _ctTris = 0;
  _ctDelayedNodes = 0;
  // draw node from last level 
  INDEX ctqtl = _ptrTerrain->tr_aqtlQuadTreeLevels.Count();
  QuadTreeLevel &qtl = _ptrTerrain->tr_aqtlQuadTreeLevels[ctqtl-1];
  DrawQuadTreeNode(qtl.qtl_iFirstNode);

  // if any delayed tiles
  if(_ctDelayedNodes>0) {
    // Draw delayed tiles
    RenderBatchedTiles();
  }

  //CEntity *pen = _ptrTerrain->tr_penEntity;

  extern void ShowRayPath(CDrawPort *pdp);
  ShowRayPath(_pdp);
/*

  extern CStaticStackArray<GFXVertex> _avExtVertices;
  extern CStaticStackArray<INDEX>     _aiExtIndices;

  extern FLOATaabbox3D _bboxDrawOne;
  extern FLOATaabbox3D _bboxDrawTwo;
  //#pragma message(">> Remove gfxDrawWireBox")

  FLOATaabbox3D bboxAllTerrain;
  extern FLOAT3D _vHitBegin;
  extern FLOAT3D _vHitEnd;
  extern FLOAT3D _vHitExact;
  _ptrTerrain->GetAllTerrainBBox(bboxAllTerrain);
  gfxDrawWireBox(bboxAllTerrain,0xFFFF00FF);
  
  gfxEnableDepthBias();
  gfxDisableDepthTest();
  _pdp->DrawPoint3D(_vHitBegin,0x00FF00FF,8);
  _pdp->DrawPoint3D(_vHitEnd,0xFF0000FF,8);
  _pdp->DrawPoint3D(_vHitExact,0x00FFFF,8);

  _pdp->DrawLine3D(_vHitBegin,FLOAT3D(_vHitEnd(1),_vHitBegin(2),_vHitEnd(3)),0x00FF00FF);
  _pdp->DrawLine3D(FLOAT3D(_vHitBegin(1),_vHitEnd(2),_vHitBegin(3)),_vHitEnd,0xFF0000FF);
  _pdp->DrawLine3D(_vHitBegin,_vHitEnd,0xFFFF00FF);
  gfxEnableDepthTest();
  gfxDisableDepthBias();
*/

  //gfxDrawWireBox(_bboxDrawOne,0xFF0000FF);
  //gfxDrawWireBox(_bboxDrawTwo,0x0000FFFF);
  //gfxDrawWireBox(_bboxDrawNextFrame,0xFFFFFFFF);
}

// Render one tile in wireframe mode
static void RenderWireTile(INDEX itt)
{
  ASSERT(_ptrTerrain!=NULL);
  CTerrainTile &tt = _ptrTerrain->tr_attTiles[itt];
  INDEX ctVertices = tt.GetVertices().Count();

  GFXVertex4 *pavVertices;
  if(ter_bLerpVertices) {
    PrepareSmothVertices(itt);
    pavVertices = &_avLerpedVerices[0];
  } else {
    pavVertices = &tt.GetVertices()[0];
  }

  INDEX ctIndices = tt.GetIndices().Count();
  if(ctIndices>0) {
    gfxDisableBlend();
    gfxDisableTexture();
    gfxSetConstantColor(_colTerrainEdges);
    gfxSetVertexArray(pavVertices,ctVertices);
    gfxLockArrays();
    gfxDrawElements(ctIndices,&tt.GetIndices()[0]);
    gfxUnlockArrays();
  }
}

// Draw one quad tree node ( draws terrain tile in wireframe mode if leaf node )
static void DrawWireQuadTreeNode(INDEX iqtn)
{
  ASSERT(_ptrTerrain!=NULL);
  CEntity *pen = _ptrTerrain->tr_penEntity;
  QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[iqtn];
  
  FLOATmatrix3D &mAbsToView = _aprProjection->pr_ViewerRotationMatrix;
  FLOATobbox3D obbox = FLOATobbox3D( qtn.qtn_aabbox, 
    (pen->en_plPlacement.pl_PositionVector-_aprProjection->pr_vViewerPosition)*mAbsToView, mAbsToView*pen->en_mRotation);

  INDEX iFrustumTest = _aprProjection->TestBoxToFrustum(obbox);
  if(iFrustumTest!=(-1)) {
    // is this leaf node
    if(qtn.qtn_iTileIndex != -1) {
      _ctNodesVis++;
      // draw terrain tile for this node 
      RenderWireTile(qtn.qtn_iTileIndex);
    // this node has some children
    } else {
      for(INDEX iqc=0;iqc<4;iqc++) {
        INDEX iChildNode = qtn.qtn_iChild[iqc];
        // if child node exists
        if(iChildNode != -1) {
          // draw child node
          DrawWireQuadTreeNode(qtn.qtn_iChild[iqc]);
        }
      }
    }
  }
}

// Render one terrain in wireframe mode
void RenderTerrainWire(COLOR &colEdges)
{
  // set wireframe mode
  gfxEnableDepthBias();
  gfxPolygonMode(GFX_LINE);
  
  // remember edges color
  _colTerrainEdges = colEdges;

  ASSERT(_ptrTerrain!=NULL);
  // draw last node 
  INDEX ctqtl = _ptrTerrain->tr_aqtlQuadTreeLevels.Count();
  QuadTreeLevel &qtl = _ptrTerrain->tr_aqtlQuadTreeLevels[ctqtl-1];
  DrawWireQuadTreeNode(qtl.qtl_iFirstNode);

  // set fill mode
  gfxDisableDepthBias();
  gfxPolygonMode(GFX_FILL);
}

// Draw terrain quad tree
void DrawQuadTree(void)
{
  ASSERT(_ptrTerrain!=NULL);
  QuadTreeLevel &qtl = _ptrTerrain->tr_aqtlQuadTreeLevels[0];
  gfxDisableTexture();
  // for each quad tree node 
  for(INDEX iqtn=qtl.qtl_iFirstNode;iqtn<qtl.qtl_iFirstNode+qtl.qtl_ctNodes;iqtn++) {
    // draw node
    QuadTreeNode &qtn = _ptrTerrain->tr_aqtnQuadTreeNodes[iqtn];
    gfxDrawWireBox(qtn.qtn_aabbox,0x00FF00FF);
  }
}

void DrawSelectedVertices(GFXVertex *pavVertices, GFXColor *pacolColors, INDEX ctVertices)
{
  gfxEnableDepthBias();
  // for each vertex
  for(INDEX ivx=0;ivx<ctVertices;ivx++) {
    GFXVertex &vtx = pavVertices[ivx];
    GFXColor  &col = pacolColors[ivx];
    // draw vertex
	_pdp->DrawPoint3D(FLOAT3D(vtx.x, vtx.y, vtx.z), ByteSwap(col.ul.abgr), 3);
  }
  gfxDisableDepthBias();
}

// TEMP - Draw one AABBox
void gfxDrawWireBox(FLOATaabbox3D &bbox, COLOR col)
{
  FLOAT3D vMinVtx = bbox.Min();
  FLOAT3D vMaxVtx = bbox.Max();
  // fill vertex array so it represents bounding box
  FLOAT3D vBoxVtxs[8];
  vBoxVtxs[0] = FLOAT3D( vMinVtx(1), vMinVtx(2), vMinVtx(3));
  vBoxVtxs[1] = FLOAT3D( vMaxVtx(1), vMinVtx(2), vMinVtx(3));
  vBoxVtxs[2] = FLOAT3D( vMaxVtx(1), vMinVtx(2), vMaxVtx(3));
  vBoxVtxs[3] = FLOAT3D( vMinVtx(1), vMinVtx(2), vMaxVtx(3));
  vBoxVtxs[4] = FLOAT3D( vMinVtx(1), vMaxVtx(2), vMinVtx(3));
  vBoxVtxs[5] = FLOAT3D( vMaxVtx(1), vMaxVtx(2), vMinVtx(3));
  vBoxVtxs[6] = FLOAT3D( vMaxVtx(1), vMaxVtx(2), vMaxVtx(3));
  vBoxVtxs[7] = FLOAT3D( vMinVtx(1), vMaxVtx(2), vMaxVtx(3));

  // connect vertices into lines of bounding box
  INDEX iBoxLines[12][2];
  iBoxLines[ 0][0] = 0;  iBoxLines[ 0][1] = 1;  iBoxLines[ 1][0] = 1;  iBoxLines[ 1][1] = 2;
  iBoxLines[ 2][0] = 2;  iBoxLines[ 2][1] = 3;  iBoxLines[ 3][0] = 3;  iBoxLines[ 3][1] = 0;
  iBoxLines[ 4][0] = 0;  iBoxLines[ 4][1] = 4;  iBoxLines[ 5][0] = 1;  iBoxLines[ 5][1] = 5;
  iBoxLines[ 6][0] = 2;  iBoxLines[ 6][1] = 6;  iBoxLines[ 7][0] = 3;  iBoxLines[ 7][1] = 7;
  iBoxLines[ 8][0] = 4;  iBoxLines[ 8][1] = 5;  iBoxLines[ 9][0] = 5;  iBoxLines[ 9][1] = 6;
  iBoxLines[10][0] = 6;  iBoxLines[10][1] = 7;  iBoxLines[11][0] = 7;  iBoxLines[11][1] = 4;
  // for all vertices in bounding box
  for( INDEX i=0; i<12; i++) {
    // get starting and ending vertices of one line
    FLOAT3D &v0 = vBoxVtxs[iBoxLines[i][0]];
    FLOAT3D &v1 = vBoxVtxs[iBoxLines[i][1]];
    _pdp->DrawLine3D(v0,v1,col);
  } 
}

