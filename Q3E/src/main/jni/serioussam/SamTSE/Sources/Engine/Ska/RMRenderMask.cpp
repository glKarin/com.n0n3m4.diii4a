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

#include <Engine/Math/Projection.h>
#include <Engine/Graphics/Shader.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/RenderPoly.h>
#include <Engine/Models/Model.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Models/RenderModel_internal.h>

// vertex array for clipped polygons
#define MAX_CLIPPEDVERTICES 32
// double buffer for clipping
static TransformedVertexData atvdClipped1[MAX_CLIPPEDVERTICES];
static TransformedVertexData atvdClipped2[MAX_CLIPPEDVERTICES];
static TransformedVertexData *ptvdSrc = atvdClipped1;
static TransformedVertexData *ptvdDst = atvdClipped2;
static INDEX ctvxSrc, ctvxDst;



extern void InternalShader_Mask(void)
{
  // need arrays and texture
  INDEX ctIdx = shaGetIndexCount();
  INDEX ctVtx = shaGetVertexCount();
  if( ctIdx==0 || ctVtx==0) return;
  INDEX_T *pidx = shaGetIndexArray();
  GFXVertex4  *pvtx = shaGetVertexArray();
  GFXTexCoord *ptex = shaGetUVMap(0);
  CTextureObject *pto = shaGetTexture(0);
  ASSERT( (ctIdx%3) == 0); // must have triangles?

  // prepare texture
  ULONG *pulTexFrame = NULL;
  PIX pixMipWidth=0, pixMipHeight=0;

  if( pto!=NULL && ptex!=NULL) {
    CTextureData *ptd = (CTextureData*)pto->GetData();
    if( ptd!=NULL && ptd->td_ptegEffect==NULL) {
      // fetch some texture params
      pulTexFrame  = ptd->td_pulFrames + (pto->GetFrame()*ptd->td_slFrameSize)/BYTES_PER_TEXEL;
      pixMipWidth  = ptd->GetPixWidth();
      pixMipHeight = ptd->GetPixHeight();
      // reload texture and keep in memory
      ptd->Force(TEX_STATIC);
    }
  }
  // initialize texture for usage thru render triangle routine
  SetTriangleTexture( pulTexFrame, pixMipWidth, pixMipHeight);

  // prepare projection
  const BOOL bPerspective = _aprProjection.IsPerspective();
  CPerspectiveProjection3D &prPerspective = (CPerspectiveProjection3D &)*_aprProjection;
  CParallelProjection3D &prParallel = (CParallelProjection3D &)*_aprProjection;
  FLOAT fCenterI, fCenterJ, fRatioI, fRatioJ, fStepI, fStepJ, fZoomI, fZoomJ; 
  FLOAT fFrontClipDistance, fBackClipDistance, f1oFrontClipDistance, f1oBackClipDistance, fDepthBufferFactor;

  if( bPerspective) {
    fCenterI = prPerspective.pr_ScreenCenter(1);
    fCenterJ = prPerspective.pr_ScreenCenter(2);
    fRatioI  = prPerspective.ppr_PerspectiveRatios(1);
    fRatioJ  = prPerspective.ppr_PerspectiveRatios(2);
    fFrontClipDistance   = -prPerspective.pr_NearClipDistance;
    fBackClipDistance    = -prPerspective.pr_FarClipDistance;
    f1oFrontClipDistance = -1.0f / prPerspective.pr_NearClipDistance;
    f1oBackClipDistance  = -1.0f / prPerspective.pr_FarClipDistance;
    fDepthBufferFactor   = prPerspective.pr_fDepthBufferFactor;
  } else {
    fCenterI = prParallel.pr_ScreenCenter(1);
    fCenterJ = prParallel.pr_ScreenCenter(2);
    fStepI   = prParallel.pr_vStepFactors(1);
    fStepJ   = prParallel.pr_vStepFactors(2);
    fZoomI   = prParallel.pr_vZoomFactors(1);
    fZoomJ   = prParallel.pr_vZoomFactors(2);
    fFrontClipDistance   = -prPerspective.pr_NearClipDistance;
    fBackClipDistance    = -prPerspective.pr_FarClipDistance;
    f1oFrontClipDistance = 1.0f;
    f1oBackClipDistance  = 1.0f;
    fDepthBufferFactor   = 1.0f;
  }

  // copy view space vertices, project 'em to screen space and mark clipping
  CStaticStackArray<TransformedVertexData> atvd;
  INDEX iVtx;
  for( iVtx=0; iVtx<ctVtx; iVtx++)
  {
    // copy viewspace and texture coords
    TransformedVertexData &tvd = atvd.Push();
    tvd.tvd_fX = pvtx[iVtx].x;
    tvd.tvd_fY = pvtx[iVtx].y;
    tvd.tvd_fZ = pvtx[iVtx].z;
    tvd.tvd_bClipped = FALSE;   // initially, vertex is not clipped

    // prepare screen coordinates
    if( bPerspective) {
      const FLOAT f1oZ = 1.0f / tvd.tvd_fZ;
      tvd.tvd_pv2.pv2_fI   = fCenterI + tvd.tvd_fX*fRatioI *f1oZ;
      tvd.tvd_pv2.pv2_fJ   = fCenterJ - tvd.tvd_fY*fRatioJ *f1oZ;
      tvd.tvd_pv2.pv2_f1oK = fDepthBufferFactor*f1oZ;
    } else {
      tvd.tvd_pv2.pv2_fI   = fCenterI + tvd.tvd_fX*fZoomI + tvd.tvd_fZ*fStepI;
      tvd.tvd_pv2.pv2_fJ   = fCenterJ - tvd.tvd_fY*fZoomJ - tvd.tvd_fZ*fStepJ;
      tvd.tvd_pv2.pv2_f1oK = 1;
    }

    // adjust texture coords (if any!)
    if( ptex!=NULL) {
      tvd.tvd_fU = ptex[iVtx].st.s;
      tvd.tvd_fV = ptex[iVtx].st.t;
      tvd.tvd_pv2.pv2_fUoK = tvd.tvd_fU * tvd.tvd_pv2.pv2_f1oK *pixMipWidth;
      tvd.tvd_pv2.pv2_fVoK = tvd.tvd_fV * tvd.tvd_pv2.pv2_f1oK *pixMipHeight;
    } else tvd.tvd_fU = tvd.tvd_fV = 0;

    // check clipping against horizontal screen boundaries and near clip plane
    if( tvd.tvd_pv2.pv2_fI<0 || tvd.tvd_pv2.pv2_fI>=_slMaskWidth
     || tvd.tvd_fZ>fFrontClipDistance || (fBackClipDistance<0 && tvd.tvd_fZ<fBackClipDistance)) {
      tvd.tvd_bClipped = TRUE;
    }
  }

  // lets clip and render - triangle by triangle
  for( INDEX iIdx=0; iIdx<ctIdx; iIdx+=3)
  {
    // get transformed triangle
    TransformedVertexData tvd[3];
    iVtx = pidx[iIdx+0];  tvd[0] = atvd[iVtx];
    iVtx = pidx[iIdx+1];  tvd[1] = atvd[iVtx];
    iVtx = pidx[iIdx+2];  tvd[2] = atvd[iVtx];

    // clipped?
    if( tvd[0].tvd_bClipped || tvd[1].tvd_bClipped || tvd[2].tvd_bClipped)
    {
      // create array of vertices for polygon clipped to near clip plane
      ctvxDst=0;
      INDEX ivx0=2;
      INDEX ivx1=0;
      {for( INDEX ivx=0; ivx<3; ivx++)
      {
        TransformedVertexData &tvd0 = tvd[ivx0];
        TransformedVertexData &tvd1 = tvd[ivx1];
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
          const FLOAT f1oZ = 1.0f / tvd.tvd_fZ;
          tvd.tvd_pv2.pv2_fI = fCenterI + tvd.tvd_fX*fRatioI *f1oZ;
          tvd.tvd_pv2.pv2_fJ = fCenterJ - tvd.tvd_fY*fRatioJ *f1oZ;
        } else {
          tvd.tvd_pv2.pv2_fI = fCenterI + tvd.tvd_fX*fZoomI + tvd.tvd_fZ*fStepI;
          tvd.tvd_pv2.pv2_fJ = fCenterJ - tvd.tvd_fY*fZoomJ - tvd.tvd_fZ*fStepJ;
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
        FLOAT fd0 = _slMaskWidth - pv20.pv2_fI;
        FLOAT fd1 = _slMaskWidth - pv21.pv2_fI;
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
            pv2Clipped.pv2_fI = _slMaskWidth;
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
            pv2Clipped.pv2_fI = _slMaskWidth;
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
        DrawTriangle_Mask( _pubMask, _slMaskWidth, _slMaskHeight, &pvx0, &pvx1, &pvx2, TRUE);
      }}
    }

    // not clipped - just draw!
    else {
      DrawTriangle_Mask( _pubMask, _slMaskWidth, _slMaskHeight,
                         &tvd[0].tvd_pv2, &tvd[1].tvd_pv2, &tvd[2].tvd_pv2, TRUE);
    }
  }
}


extern void InternalShaderDesc_Mask(ShaderDesc &shDesc)
{
  shDesc.sd_astrTextureNames.New(1);
  shDesc.sd_astrTexCoordNames.New(1);
  shDesc.sd_astrTextureNames[0]  = "Mask texture";
  shDesc.sd_astrTexCoordNames[0] = "Mask uvmap";
  shDesc.sd_strShaderInfo = "Mask shader for shadowmaps";
}
