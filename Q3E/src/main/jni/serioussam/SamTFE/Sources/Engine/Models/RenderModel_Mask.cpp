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

#include <Engine/Models/ModelObject.h>
#include <Engine/Base/Translation.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/ModelProfile.h>
#include <Engine/Models/RenderModel.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Base/Lists.inl>
#include <Engine/Base/Console.h>

#include <Engine/Models/RenderModel_internal.h>

// polygon visibility constants
#define VISIBLE_NOT     0
#define VISIBLE_FRONT (+1)
#define VISIBLE_BACK  (-1)

// some rendering variables and so ...
static PIX pixWidth;

// some projection parameters
static FLOAT fCenterI, fCenterJ;
static FLOAT fRatioI, fRatioJ;
static FLOAT fStepI, fStepJ;
static FLOAT fZoomI, fZoomJ;
static FLOAT fFrontClipDistance, f1oFrontClipDistance;
static FLOAT fBackClipDistance, f1oBackClipDistance;
static FLOAT fDepthBufferFactor;
//static BOOL  bBackFaced, bDoubleSided;
static BOOL  bPerspective;
static BOOL  b16BitCompression;
static ULONG ulColorMask;
static ULONG ulRenderFlags;
static PIX   pixMipWidth, pixMipHeight;
static INDEX iMipLevel;

// vertex array for clipped polygons
#define MAX_CLIPPEDVERTICES 32
// double buffer for clipping
static TransformedVertexData atvdClipped1[MAX_CLIPPEDVERTICES];
static TransformedVertexData atvdClipped2[MAX_CLIPPEDVERTICES];
static TransformedVertexData *ptvdSrc = atvdClipped1;
static TransformedVertexData *ptvdDst = atvdClipped2;
static INDEX ctvxSrc, ctvxDst;



// prepare list of all visible polygons that are to be rendered to current drawport
static void RenderOneSide( CRenderModel &rm, const INDEX iVisibility)
{
  // for each surface in finest mip model
  ModelMipInfo &mmiMip = rm.rm_pmdModelData->md_MipInfos[0];
  FOREACHINSTATICARRAY( mmiMip.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    MappingSurface &ms = *itms;
    ULONG ulFlags = ms.ms_ulRenderingFlags;
    // if surface is invisible or empty, skip it
    if( (ulFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx==0) continue;
    // if rendering back side and surface is not double sided, skip entire surface
    if( iVisibility==VISIBLE_BACK && !(ulFlags&SRF_DOUBLESIDED)) continue;

    // for each vertex in the surface
    BOOL bTransparency = ms.ms_sttTranslucencyType!=STT_OPAQUE;
    for( INDEX ivx=0; ivx<ms.ms_aiTextureVertices.Count(); ivx++) {
      ModelTextureVertex    &mtv = mmiMip.mmpi_TextureVertices[ms.ms_aiTextureVertices[ivx]];
      TransformedVertexData &tvd = rm.rm_pmdModelData->md_TransformedVertices[mtv.mtv_iTransformedVertex];
      // adjust texture coordinates for texture mapping and clipping
      tvd.tvd_fU = (mtv.mtv_UV(1)>>iMipLevel);
      tvd.tvd_fV = (mtv.mtv_UV(2)>>iMipLevel);
      tvd.tvd_pv2.pv2_fUoK = tvd.tvd_fU * tvd.tvd_pv2.pv2_f1oK;
      tvd.tvd_pv2.pv2_fVoK = tvd.tvd_fV * tvd.tvd_pv2.pv2_f1oK;
    }

    // for each polygon in the surface
    for( INDEX iipo=0; iipo<ms.ms_aiPolygons.Count(); iipo++)
    {
      ModelPolygon &mpPolygon = mmiMip.mmpi_Polygons[ms.ms_aiPolygons[iipo]];
      // if the polygon is not visible on this side, skip it
      if( mpPolygon.mp_slVisibility != iVisibility) continue;

      // if the polygon needs to be clipped to near clip plane
      if( mpPolygon.mp_bClipped) {
        // create array of vertices for polygon clipped to near clip plane
        ctvxDst=0;
        INDEX ivx0=mpPolygon.mp_PolygonVertices.Count()-1;
        INDEX ivx1=0;
        {for( INDEX ivx=0; ivx<mpPolygon.mp_PolygonVertices.Count(); ivx++)
        {
          TransformedVertexData &tvd0 = *mpPolygon.mp_PolygonVertices[ivx0].mpv_ptvTransformedVertex;
          TransformedVertexData &tvd1 = *mpPolygon.mp_PolygonVertices[ivx1].mpv_ptvTransformedVertex;
          FLOAT fd0 = fFrontClipDistance-tvd0.tvd_fZ;
          FLOAT fd1 = fFrontClipDistance-tvd1.tvd_fZ;
          // if first vertex is in
          if( fd0>=0) {
            // add it to clip array
            ptvdDst[ctvxDst] = tvd0;
            ctvxDst++;
            // if second vertex is out
            if( fd1<0) {
              // add clipped vertex at exit
              TransformedVertexData &tvdClipped = ptvdDst[ctvxDst];
              ctvxDst++;
              FLOAT fF = fd1/(fd1-fd0);
              tvdClipped.tvd_fX = tvd1.tvd_fX - (tvd1.tvd_fX - tvd0.tvd_fX) *fF;
              tvdClipped.tvd_fY = tvd1.tvd_fY - (tvd1.tvd_fY - tvd0.tvd_fY) *fF;
              tvdClipped.tvd_fZ = fFrontClipDistance;
              tvdClipped.tvd_pv2.pv2_f1oK = fDepthBufferFactor * f1oFrontClipDistance;
              FLOAT fU = tvd1.tvd_fU - (tvd1.tvd_fU - tvd0.tvd_fU) *fF;
              FLOAT fV = tvd1.tvd_fV - (tvd1.tvd_fV - tvd0.tvd_fV) *fF;
              tvdClipped.tvd_pv2.pv2_fUoK = fU * tvdClipped.tvd_pv2.pv2_f1oK;
              tvdClipped.tvd_pv2.pv2_fVoK = fV * tvdClipped.tvd_pv2.pv2_f1oK;
            }
          // if first vertex is out (don't add it into clip array)
          } else {
            // if second vertex is in
            if( fd1>=0) {
              // add clipped vertex at entry
              TransformedVertexData &tvdClipped = ptvdDst[ctvxDst];
              ctvxDst++;
              FLOAT fF = fd0/(fd0-fd1);
              tvdClipped.tvd_fX = tvd0.tvd_fX - (tvd0.tvd_fX - tvd1.tvd_fX) *fF;
              tvdClipped.tvd_fY = tvd0.tvd_fY - (tvd0.tvd_fY - tvd1.tvd_fY) *fF;
              tvdClipped.tvd_fZ = fFrontClipDistance;
              tvdClipped.tvd_pv2.pv2_f1oK = fDepthBufferFactor * f1oFrontClipDistance;
              FLOAT fU = tvd0.tvd_fU - (tvd0.tvd_fU - tvd1.tvd_fU) *fF;
              FLOAT fV = tvd0.tvd_fV - (tvd0.tvd_fV - tvd1.tvd_fV) *fF;
              tvdClipped.tvd_pv2.pv2_fUoK = fU * tvdClipped.tvd_pv2.pv2_f1oK;
              tvdClipped.tvd_pv2.pv2_fVoK = fV * tvdClipped.tvd_pv2.pv2_f1oK;
            }
          }
          // proceed to next vertex in list (i.e. new pair of vertices)
          ivx0=ivx1;
          ivx1++;
        }}
        // swap buffers
        Swap( ptvdSrc, ptvdDst);
        Swap( ctvxSrc, ctvxDst);

        // if clipping to far clip plane is on
        if( fBackClipDistance<0) {
          ctvxDst=0;
          INDEX ivx0=ctvxSrc-1;
          INDEX ivx1=0;
          {for( INDEX ivx=0; ivx<ctvxSrc; ivx++)
          {
            TransformedVertexData &tvd0 = ptvdSrc[ivx0];
            TransformedVertexData &tvd1 = ptvdSrc[ivx1];
            FLOAT fd0 = tvd0.tvd_fZ-fBackClipDistance;
            FLOAT fd1 = tvd1.tvd_fZ-fBackClipDistance;
            // if first vertex is in
            if( fd0>=0) {
              // add it to clip array
              ptvdDst[ctvxDst] = tvd0;
              ctvxDst++;
              // if second vertex is out
              if( fd1<0) {
                // add clipped vertex at exit
                TransformedVertexData &tvdClipped = ptvdDst[ctvxDst];
                ctvxDst++;
                FLOAT fF = fd1/(fd1-fd0);
                tvdClipped.tvd_fX = tvd1.tvd_fX - (tvd1.tvd_fX - tvd0.tvd_fX) *fF;
                tvdClipped.tvd_fY = tvd1.tvd_fY - (tvd1.tvd_fY - tvd0.tvd_fY) *fF;
                tvdClipped.tvd_fZ = fBackClipDistance;
                tvdClipped.tvd_pv2.pv2_f1oK = fDepthBufferFactor * f1oBackClipDistance;
                FLOAT fU = tvd1.tvd_fU - (tvd1.tvd_fU - tvd0.tvd_fU) *fF;
                FLOAT fV = tvd1.tvd_fV - (tvd1.tvd_fV - tvd0.tvd_fV) *fF;
                tvdClipped.tvd_pv2.pv2_fUoK = fU * tvdClipped.tvd_pv2.pv2_f1oK;
                tvdClipped.tvd_pv2.pv2_fVoK = fV * tvdClipped.tvd_pv2.pv2_f1oK;
              }
            // if first vertex is out (don't add it into clip array)
            } else {
              // if second vertex is in
              if( fd1>=0) {
                // add clipped vertex at entry
                TransformedVertexData &tvdClipped = ptvdDst[ctvxDst];
                ctvxDst++;
                FLOAT fF = fd0/(fd0-fd1);
                tvdClipped.tvd_fX = tvd0.tvd_fX - (tvd0.tvd_fX - tvd1.tvd_fX) *fF;
                tvdClipped.tvd_fY = tvd0.tvd_fY - (tvd0.tvd_fY - tvd1.tvd_fY) *fF;
                tvdClipped.tvd_fZ = fBackClipDistance;
                tvdClipped.tvd_pv2.pv2_f1oK = fDepthBufferFactor * f1oBackClipDistance;
                FLOAT fU = tvd0.tvd_fU - (tvd0.tvd_fU - tvd1.tvd_fU) *fF;
                FLOAT fV = tvd0.tvd_fV - (tvd0.tvd_fV - tvd1.tvd_fV) *fF;
                tvdClipped.tvd_pv2.pv2_fUoK = fU * tvdClipped.tvd_pv2.pv2_f1oK;
                tvdClipped.tvd_pv2.pv2_fVoK = fV * tvdClipped.tvd_pv2.pv2_f1oK;
              }
            }
            // proceed to next vertex in list (i.e. new pair of vertices)
            ivx0=ivx1;
            ivx1++;
          }}
          // swap buffers
          Swap( ptvdSrc, ptvdDst);
          Swap( ctvxSrc, ctvxDst);
        }

        // for each vertex
        {for( INDEX ivx=0; ivx<ctvxSrc; ivx++)
        {
          // calculate projection
          TransformedVertexData &tvd = ptvdSrc[ivx];
          if( bPerspective) {
            const FLOAT f1oZ = 1.0f/tvd.tvd_fZ;
            tvd.tvd_pv2.pv2_fI = fCenterI+tvd.tvd_fX*fRatioI*f1oZ;
            tvd.tvd_pv2.pv2_fJ = fCenterJ-tvd.tvd_fY*fRatioJ*f1oZ;
          } else {
            tvd.tvd_pv2.pv2_fI = fCenterI+tvd.tvd_fX*fZoomI+tvd.tvd_fZ*fStepI;
            tvd.tvd_pv2.pv2_fJ = fCenterJ-tvd.tvd_fY*fZoomJ-tvd.tvd_fZ*fStepJ;
          }
        }}

        // clip polygon against left edge
        ctvxDst=0;
        ivx0=ctvxSrc-1;
        ivx1=0;
        {for( INDEX ivx=0; ivx<ctvxSrc; ivx++)
        {
          PolyVertex2D &pv20 = ptvdSrc[ivx0].tvd_pv2;
          PolyVertex2D &pv21 = ptvdSrc[ivx1].tvd_pv2;
          FLOAT fd0 = pv20.pv2_fI-0;
          FLOAT fd1 = pv21.pv2_fI-0;
          // if first vertex is in
          if( fd0>=0) {
            // add it to clip array
            ptvdDst[ctvxDst].tvd_pv2 = pv20;
            ctvxDst++;
            // if second vertex is out
            if( fd1<0) {
              PolyVertex2D &pv2Clipped = ptvdDst[ctvxDst].tvd_pv2;
              ctvxDst++;
              FLOAT fF = fd1/(fd1-fd0);
              pv2Clipped.pv2_fI = 0;
              pv2Clipped.pv2_fJ = pv21.pv2_fJ - (pv21.pv2_fJ - pv20.pv2_fJ) *fF;
              pv2Clipped.pv2_f1oK = pv21.pv2_f1oK - (pv21.pv2_f1oK - pv20.pv2_f1oK) *fF;
              pv2Clipped.pv2_fUoK = pv21.pv2_fUoK - (pv21.pv2_fUoK - pv20.pv2_fUoK) *fF;
              pv2Clipped.pv2_fVoK = pv21.pv2_fVoK - (pv21.pv2_fVoK - pv20.pv2_fVoK) *fF;
            }
          // if first vertex is out (don't add it into clip array)
          } else {
            // if second vertex is in
            if( fd1>=0) {
              // add clipped vertex at entry
              PolyVertex2D &pv2Clipped = ptvdDst[ctvxDst].tvd_pv2;
              ctvxDst++;
              FLOAT fF = fd0/(fd0-fd1);
              pv2Clipped.pv2_fI = 0;
              pv2Clipped.pv2_fJ = pv20.pv2_fJ - (pv20.pv2_fJ - pv21.pv2_fJ)*fF;
              pv2Clipped.pv2_f1oK = pv20.pv2_f1oK - (pv20.pv2_f1oK - pv21.pv2_f1oK) *fF;
              pv2Clipped.pv2_fUoK = pv20.pv2_fUoK - (pv20.pv2_fUoK - pv21.pv2_fUoK) *fF;
              pv2Clipped.pv2_fVoK = pv20.pv2_fVoK - (pv20.pv2_fVoK - pv21.pv2_fVoK) *fF;
            }
          }
          // proceed to next vertex in list (i.e. new pair of vertices)
          ivx0=ivx1;
          ivx1++;
        }}
        // swap buffers
        Swap( ptvdSrc, ptvdDst);
        Swap( ctvxSrc, ctvxDst);

        // clip polygon against right edge
        ctvxDst=0;
        ivx0=ctvxSrc-1;
        ivx1=0;
        {for( INDEX ivx=0; ivx<ctvxSrc; ivx++)
        {
          PolyVertex2D &pv20 = ptvdSrc[ivx0].tvd_pv2;
          PolyVertex2D &pv21 = ptvdSrc[ivx1].tvd_pv2;
          FLOAT fd0 = pixWidth-pv20.pv2_fI;
          FLOAT fd1 = pixWidth-pv21.pv2_fI;
          // if first vertex is in
          if( fd0>=0) {
            // add it to clip array
            ptvdDst[ctvxDst].tvd_pv2 = pv20;
            ctvxDst++;
            // if second vertex is out
            if( fd1<0) {
              PolyVertex2D &pv2Clipped = ptvdDst[ctvxDst].tvd_pv2;
              ctvxDst++;
              FLOAT fF = fd1/(fd1-fd0);
              pv2Clipped.pv2_fI = pixWidth;
              pv2Clipped.pv2_fJ = pv21.pv2_fJ - (pv21.pv2_fJ - pv20.pv2_fJ)*fF;
              pv2Clipped.pv2_f1oK = pv21.pv2_f1oK - (pv21.pv2_f1oK - pv20.pv2_f1oK) *fF;
              pv2Clipped.pv2_fUoK = pv21.pv2_fUoK - (pv21.pv2_fUoK - pv20.pv2_fUoK) *fF;
              pv2Clipped.pv2_fVoK = pv21.pv2_fVoK - (pv21.pv2_fVoK - pv20.pv2_fVoK) *fF;
            }
          // if first vertex is out (don't add it into clip array)
          } else {
            // if second vertex is in
            if( fd1>=0) {
              // add clipped vertex at entry
              PolyVertex2D &pv2Clipped = ptvdDst[ctvxDst].tvd_pv2;
              ctvxDst++;
              FLOAT fF = fd0/(fd0-fd1);
              pv2Clipped.pv2_fI = pixWidth;
              pv2Clipped.pv2_fJ = pv20.pv2_fJ - (pv20.pv2_fJ - pv21.pv2_fJ)*fF;
              pv2Clipped.pv2_f1oK = pv20.pv2_f1oK - (pv20.pv2_f1oK - pv21.pv2_f1oK) *fF;
              pv2Clipped.pv2_fUoK = pv20.pv2_fUoK - (pv20.pv2_fUoK - pv21.pv2_fUoK) *fF;
              pv2Clipped.pv2_fVoK = pv20.pv2_fVoK - (pv20.pv2_fVoK - pv21.pv2_fVoK) *fF;
            }
          }
          // proceed to next vertex in list (i.e. new pair of vertices)
          ivx0=ivx1;
          ivx1++;
        }}
        // swap buffers
        Swap( ptvdSrc, ptvdDst);
        Swap( ctvxSrc, ctvxDst);

        // draw all triangles in clipped polygon as a triangle fan, with clipping
        PolyVertex2D &pvx0 = ptvdSrc[0].tvd_pv2;
        {for( INDEX ivx=1; ivx<ctvxSrc-1; ivx++) {
          PolyVertex2D &pvx1 = ptvdSrc[ivx+0].tvd_pv2;
          PolyVertex2D &pvx2 = ptvdSrc[ivx+1].tvd_pv2;
          DrawTriangle_Mask( _pubMask, _slMaskWidth, _slMaskHeight, &pvx0, &pvx1, &pvx2, bTransparency);
        }}
        _pfModelProfile.IncrementCounter(CModelProfile::PCI_MASK_TRIANGLES, mpPolygon.mp_PolygonVertices.Count()-2);
        _pfModelProfile.IncrementCounter(CModelProfile::PCI_MASK_POLYGONS);
      } // if the polygon is not clipped
      else
      {
        // draw all triangles as a triangle fan
        PolyVertex2D &pvx0 = mpPolygon.mp_PolygonVertices[0].mpv_ptvTransformedVertex->tvd_pv2;
        {for( INDEX ivx=1; ivx<mpPolygon.mp_PolygonVertices.Count()-1; ivx++) {
          PolyVertex2D &pvx1 = mpPolygon.mp_PolygonVertices[ivx+0].mpv_ptvTransformedVertex->tvd_pv2;
          PolyVertex2D &pvx2 = mpPolygon.mp_PolygonVertices[ivx+1].mpv_ptvTransformedVertex->tvd_pv2;
          DrawTriangle_Mask( _pubMask, _slMaskWidth, _slMaskHeight, &pvx0, &pvx1, &pvx2, bTransparency);
        }}
        _pfModelProfile.IncrementCounter(CModelProfile::PCI_MASK_TRIANGLES, mpPolygon.mp_PolygonVertices.Count()-2);
        _pfModelProfile.IncrementCounter(CModelProfile::PCI_MASK_POLYGONS);
      }
    }
  }
}




// prepare model for rendering (i.e. project model vertices)
void CModelObject::RenderModel_Mask( CRenderModel &rm)
{
  // skip shadow generation if effect texture has been set
  CTextureData *ptd = (CTextureData*)mo_toTexture.GetData();
  if( ptd!=NULL && ptd->td_ptegEffect!=NULL) {
    // report to console
    CPrintF( TRANS("WARNING: model '%s' cast cluster shadows but has an effect texture.\n"), (const char *) GetData()->GetName());
    return;
  }

  _pfModelProfile.StartTimer( CModelProfile::PTI_MASK_INITMODELRENDERING);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_MASK_INITMODELRENDERING);

  // cache drawport width (for horizontal screen clipping purposes)
  pixWidth = _slMaskWidth;

  // test if projection is parallel or perspective
  bPerspective = TRUE;
  if( !_aprProjection.IsPerspective()) bPerspective = FALSE;
  b16BitCompression = rm.rm_pmdModelData->md_Flags & MF_COMPRESSED_16BIT;
  ulColorMask = mo_ColorMask;

  // if texture is invalid, backup to white color mode
  if( ptd==NULL) rm.rm_rtRenderType = (rm.rm_rtRenderType&~RT_TEXTURE_MASK)|RT_WHITE_TEXTURE;

  // if texture is ok
  iMipLevel = 31;
  ULONG *pulCurrentMipmap = NULL;
  if( rm.rm_rtRenderType & RT_TEXTURE) {
    // reload texture
    ptd->Force( TEX_STATIC);
    // get texture parameters for current frame and needed mip factor
    pulCurrentMipmap = ptd->td_pulFrames + (mo_toTexture.GetFrame()*ptd->td_slFrameSize)/BYTES_PER_TEXEL;
    iMipLevel        = ptd->td_iFirstMipLevel;
    pixMipWidth      = ptd->GetPixWidth();
    pixMipHeight     = ptd->GetPixHeight();
  }
  // initialize texture for usage thru render triangle routine
  SetTriangleTexture( pulCurrentMipmap, pixMipWidth, pixMipHeight);

  CPerspectiveProjection3D &prPerspective = (CPerspectiveProjection3D &)*_aprProjection;
  CParallelProjection3D &prParallel = (CParallelProjection3D &)*_aprProjection;

  const FLOATmatrix3D &m = rm.rm_mObjectToView;
  const FLOAT3D       &v = rm.rm_vObjectToView;

  if( bPerspective) {
    fCenterI = prPerspective.pr_ScreenCenter(1);
    fCenterJ = prPerspective.pr_ScreenCenter(2);
    fRatioI  = prPerspective.ppr_PerspectiveRatios(1);
    fRatioJ  = prPerspective.ppr_PerspectiveRatios(2);
    fFrontClipDistance   = -prPerspective.pr_NearClipDistance;
    fBackClipDistance    = -prPerspective.pr_FarClipDistance;
    f1oFrontClipDistance = -1/prPerspective.pr_NearClipDistance;
    f1oBackClipDistance  = -1/prPerspective.pr_FarClipDistance;
    fDepthBufferFactor   = prPerspective.pr_fDepthBufferFactor;
  } else {
    fCenterI = prParallel.pr_ScreenCenter(1);
    fCenterJ = prParallel.pr_ScreenCenter(2);
    fStepI = prParallel.pr_vStepFactors(1);
    fStepJ = prParallel.pr_vStepFactors(2);
    fZoomI = prParallel.pr_vZoomFactors(1);
    fZoomJ = prParallel.pr_vZoomFactors(2);
    fFrontClipDistance   = -prPerspective.pr_NearClipDistance;
    fBackClipDistance    = -prPerspective.pr_FarClipDistance;
    f1oFrontClipDistance = 1;
    f1oBackClipDistance  = 1;
    fDepthBufferFactor   = 1;
  }

  // for each vertex
  for( INDEX ivx=0; ivx<rm.rm_pmdModelData->md_VerticesCt; ivx++)
  {
    TransformedVertexData &tvd = rm.rm_pmdModelData->md_TransformedVertices[ ivx];
    tvd.tvd_bClipped = FALSE;   // initially, vertex is not clipped
    float fxOld, fyOld, fzOld;
    if( b16BitCompression) {
      ModelFrameVertex16 &mfv = rm.rm_pFrame16_0[ivx];
      fxOld = (mfv.mfv_SWPoint(1)-rm.rm_vOffset(1)) *rm.rm_vStretch(1);
      fyOld = (mfv.mfv_SWPoint(2)-rm.rm_vOffset(2)) *rm.rm_vStretch(2);
      fzOld = (mfv.mfv_SWPoint(3)-rm.rm_vOffset(3)) *rm.rm_vStretch(3);
    } else {
      ModelFrameVertex8 &mfv = rm.rm_pFrame8_0[ivx];
      fxOld = (mfv.mfv_SBPoint(1)-rm.rm_vOffset(1)) *rm.rm_vStretch(1);
      fyOld = (mfv.mfv_SBPoint(2)-rm.rm_vOffset(2)) *rm.rm_vStretch(2);
      fzOld = (mfv.mfv_SBPoint(3)-rm.rm_vOffset(3)) *rm.rm_vStretch(3);
    }
    // rotate the vertex and remember transformed coordinates, for eventual clipping
    tvd.tvd_fX = fxOld*m(1,1) + fyOld*m(1,2) + fzOld*m(1,3) + v(1);
    tvd.tvd_fY = fxOld*m(2,1) + fyOld*m(2,2) + fzOld*m(2,3) + v(2);
    tvd.tvd_fZ = fxOld*m(3,1) + fyOld*m(3,2) + fzOld*m(3,3) + v(3);

    // prepare screen coordinates for software
    if( bPerspective) {
      const FLOAT f1oZ = 1.0f/tvd.tvd_fZ;
      tvd.tvd_pv2.pv2_fI   = fCenterI+tvd.tvd_fX*fRatioI*f1oZ;
      tvd.tvd_pv2.pv2_fJ   = fCenterJ-tvd.tvd_fY*fRatioJ*f1oZ;
      tvd.tvd_pv2.pv2_f1oK = fDepthBufferFactor*f1oZ;
    } else {
      tvd.tvd_pv2.pv2_fI   = fCenterI+tvd.tvd_fX*fZoomI+tvd.tvd_fZ*fStepI;
      tvd.tvd_pv2.pv2_fJ   = fCenterJ-tvd.tvd_fY*fZoomJ-tvd.tvd_fZ*fStepJ;
      tvd.tvd_pv2.pv2_f1oK = 1;
    }

    // check clipping against horizontal screen boundaries and near clip plane
    if( tvd.tvd_pv2.pv2_fI<0 || tvd.tvd_pv2.pv2_fI>=pixWidth  ||
        tvd.tvd_fZ>fFrontClipDistance || (fBackClipDistance<0 && tvd.tvd_fZ<fBackClipDistance)) {
      tvd.tvd_bClipped = TRUE;
    }
  }

  // for all polygons in current mip model
  ModelMipInfo &mmiMip = rm.rm_pmdModelData->md_MipInfos[0];
  FOREACHINSTATICARRAY( mmiMip.mmpi_Polygons, ModelPolygon, itmp)
  {
    ModelPolygon &mp = *itmp;
    ulRenderFlags = mmiMip.mmpi_MappingSurfaces[mp.mp_Surface].ms_ulRenderingFlags;
    // get first three of polygon's transformed vertices
    const TransformedVertexData &tvd0 = *mp.mp_PolygonVertices[0].mpv_ptvTransformedVertex;
    const TransformedVertexData &tvd1 = *mp.mp_PolygonVertices[1].mpv_ptvTransformedVertex;
    const TransformedVertexData &tvd2 = *mp.mp_PolygonVertices[2].mpv_ptvTransformedVertex;

    // calculate polygon normal with front plane clipping
    FLOAT fD1X = tvd2.tvd_fX - tvd1.tvd_fX;
    FLOAT fD1Y = tvd2.tvd_fY - tvd1.tvd_fY;
    FLOAT fD1Z = tvd2.tvd_fZ - tvd1.tvd_fZ;
    FLOAT fD2X = tvd0.tvd_fX - tvd1.tvd_fX;
    FLOAT fD2Y = tvd0.tvd_fY - tvd1.tvd_fY;
    FLOAT fD2Z = tvd0.tvd_fZ - tvd1.tvd_fZ;
    FLOAT fNX = fD2Y*fD1Z - fD2Z*fD1Y;
    FLOAT fNY = fD2Z*fD1X - fD2X*fD1Z;
    FLOAT fNZ = fD2X*fD1Y - fD2Y*fD1X;
    // calculate polygon normal visibility
    FLOAT fVisible;
    if( bPerspective) {
      fVisible = fNX*tvd0.tvd_fX + fNY*tvd0.tvd_fY + fNZ*tvd0.tvd_fZ;
    } else {
      fVisible = fNX*prParallel.pr_vViewDirection(1) +
                 fNY*prParallel.pr_vViewDirection(2) +
                 fNZ*prParallel.pr_vViewDirection(3);
    }

    // if the polygon is back-facing
    if( fVisible<0) {
      // if the polygon is double sided
      if( ulRenderFlags & SRF_DOUBLESIDED) {
        // mark it as back-facing double sided
        mp.mp_slVisibility = VISIBLE_BACK;
      // if the polygon is not double sided
      } else {
        // mark it as invisible
        mp.mp_slVisibility = VISIBLE_NOT;
      }
    // if the polygon is front-facing
    } else {
      // mark it as visible front-facing
      mp.mp_slVisibility = VISIBLE_FRONT;
    }

    // initally assume that polygon doesn't need clipping
    mp.mp_bClipped = FALSE;
    // if the polygon plane is not invisible
    if( mp.mp_slVisibility != VISIBLE_NOT) {
      // for all vertices
      {for( INDEX ivx=0; ivx<mp.mp_PolygonVertices.Count(); ivx++) {
        const TransformedVertexData &tvd = *mp.mp_PolygonVertices[ivx].mpv_ptvTransformedVertex;
        // if vertex is clipped to near plane
        if( tvd.tvd_bClipped) {
          // mark that the polygon needs clipping
          mp.mp_bClipped = TRUE;
          break;
        }
      }}
    }
  }
  _pfModelProfile.StopTimer( CModelProfile::PTI_MASK_INITMODELRENDERING);

  _pfModelProfile.StartTimer( CModelProfile::PTI_MASK_RENDERMODEL);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_MASK_RENDERMODEL);

  // render back side first and front side after that
  RenderOneSide( rm, VISIBLE_BACK);
  RenderOneSide( rm, VISIBLE_FRONT);

  _pfModelProfile.StopTimer( CModelProfile::PTI_MASK_RENDERMODEL);
}
