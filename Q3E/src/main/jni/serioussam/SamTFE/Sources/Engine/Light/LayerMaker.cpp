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

#include <Engine/Rendering/Render_internal.h>

#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Light/LightSource.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Graphics/Color.h>
#include <Engine/World/World.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Math/Clipping.inl>

#include <Engine/Light/Shadows_internal.h>
#include <Engine/World/WorldEditingProfile.h>

//#include <Engine/Graphics/ImageInfo.h>
//#include <Engine/Base/ErrorReporting.h>

#ifdef PLATFORM_UNIX
INDEX _ctShadowLayers=0;
INDEX _ctShadowClusters=0;
#else
extern INDEX _ctShadowLayers=0;
extern INDEX _ctShadowClusters=0;
#endif

// class used for making shadow layers (used only locally)
class CLayerMaker {
// implementation:
public:
  // information about current polygon
  CBrushPolygon *lm_pbpoPolygon;  // the polygon
  CBrushShadowMap *lm_pbsmShadowMap;  // the shadow map on the polygon
  FLOAT lm_fMipFactor;            // the mip factor of this polygon
  CWorld *lm_pwoWorld;            // world for casting rays in
  CBrushShadowLayer *lm_pbslLayer;// current layer

  // dimensions of currently processed shadow map
  MEX lm_mexSizeU;     // size in mex
  MEX lm_mexSizeV;
  MEX lm_mexOffsetU;   // offsets in mex
  MEX lm_mexOffsetV;
  INDEX lm_iMipLevel;  // mip level

  PIX lm_pixSizeU;     // size in pixels
  PIX lm_pixSizeV;
  struct MipmapTable lm_mmtPolygonMask; // mip map table of the polygon mask

  PIX lm_pixLayerMinU;  // layer rectangle inside shadow map
  PIX lm_pixLayerMinV;
  PIX lm_pixLayerSizeU;
  PIX lm_pixLayerSizeV;
  FLOAT lm_fpixHotU;
  FLOAT lm_fpixHotV;
  FLOAT lm_fLightPlaneDistance;
  struct MipmapTable lm_mmtLayer; // mip map table of the layer

  UBYTE *lm_pubPolygonMask;   // bit-packed mask of where the current polygon is
  UBYTE *lm_pubLayer;         // bit-packed mask of where the current light lights the polygon

  FLOAT3D lm_vLight;    // position of the light source

  // gradients for shadow map walking
  FLOAT3D lm_vO;     // upper left corner of shadow map in 3D
  FLOAT3D lm_vStepU; // step between pixels in same row
  FLOAT3D lm_vStepV; // step between rows

  ANGLE3D lm_aMappingOrientation;         // orientation of the texture map in 3D
  ANGLE3D lm_aInverseMappingOrientation;
  FLOATmatrix3D lm_mToInverseMapping;     // matrix for parallel lights

  CLightSource *lm_plsLight;    // current light

  // remember general data
  void CalculateData(void);

  // make bit-packed mask of where the polygon is in the shadow map
  void MakePolygonMask(void);

  // make shadow mask for the light
  ULONG MakeShadowMask(CBrushShadowLayer *pbsl);
  ULONG MakeOneShadowMaskMip(INDEX iMip);
  // flip shadow mask around V axis (for parallel lights)
  void FlipShadowMask(INDEX iMip);

  /* Spread the shadow towards pixels outside of polygon. */
  void SpreadShadowMaskOutwards(void);
  /* Spread the shadow towards pixels inside of polygon. */
  void SpreadShadowMaskInwards(void);

public:
// interface:
  /* Constructor. */
  CLayerMaker(void);
  /* Cast shadows for all layers of a given polygon. */
  BOOL CreateLayers(CBrushPolygon &bpo, CWorld &woWorld, BOOL bDoDirectionalLights);
};

/* Make mip-maps of the shadow mask. */
static void MakeMipmapsForMask(UBYTE *pubMask, PIX pixSizeU, PIX pixSizeV, SLONG slTotalSize)
{
  // remember pointer after first mip map
  UBYTE *pubSecond = pubMask+pixSizeU*pixSizeV;
  // start at the first mip map
  UBYTE *pubThis = pubMask;
  PIX pixThisSizeU = pixSizeU;
  PIX pixThisSizeV = pixSizeV;
  UBYTE *pubNext;
  PIX pixNextSizeU;
  PIX pixNextSizeV;
  // repeat
  for(;;) {
    // calculate size and position of next mip map
    pubNext = pubThis+pixThisSizeU*pixThisSizeV;
    pixNextSizeU = pixThisSizeU/2;
    pixNextSizeV = pixThisSizeV/2;

    // if the next mip map is too small
    if (pixNextSizeU<1 || pixNextSizeV<1) {
      // stop
      break;
    }
    // for each pixel in the next mip map
    UBYTE *pub = pubNext;
    for (PIX pixNextV=0; pixNextV<pixNextSizeV; pixNextV++) {
      for (PIX pixNextU=0; pixNextU<pixNextSizeU; pixNextU++) {
        // calculate the pixel from four pixels in this mip-map
        UBYTE *pubUL = pubThis+pixNextU*2+pixNextV*2*pixThisSizeU;
        UBYTE *pubUR = pubUL+1;
        UBYTE *pubDL = pubUL+pixThisSizeU;
        UBYTE *pubDR = pubDL+1;
        ASSERT(pubDR<pubNext);
        ULONG ulTotal = ULONG(*pubUL)+ULONG(*pubUR)+ULONG(*pubDL)+ULONG(*pubDR);
        *pub++ = ulTotal/4;
      }
    }
    // make next mip-map current
    pubThis      = pubNext;
    pixThisSizeU = pixNextSizeU;
    pixThisSizeV = pixNextSizeV;
  }

  // must have passed exactly all mip-maps
  ASSERT(pubNext==pubMask+slTotalSize);

  // for each pixel in all mip-maps after first
  UBYTE *pubEnd = pubNext;
  for (UBYTE *pub=pubSecond; pub<pubEnd; pub++) {
    // normalize the pixel to 0-255
    if (*pub<=128) {
      *pub = 0;
    } else {
      *pub = 255;
    }
  }
}

// convert byte packed array to bits (can perform inplace conversion)
// NOTE: needs +8 bytes safety wall at the end
static void ConvertBytesToBits(UBYTE *pubBytesSrc, UBYTE *pubBitsDst, INDEX ctBytes)
{

  // for each byte of bits
  for (INDEX i=0; i<(ctBytes+7)/8; i++) {
    // compose it from 8 bytes (note that bytes are either 0 or 255)
    *pubBitsDst++ =
      (pubBytesSrc[0]&0x01)|
      (pubBytesSrc[1]&0x02)|
      (pubBytesSrc[2]&0x04)|
      (pubBytesSrc[3]&0x08)|
      (pubBytesSrc[4]&0x10)|
      (pubBytesSrc[5]&0x20)|
      (pubBytesSrc[6]&0x40)|
      (pubBytesSrc[7]&0x80);
    pubBytesSrc+=8;
  }
}
// convert bit packed array to bytes
// NOTE: needs +8 bytes safety wall at the end
static void ConvertBitsToBytes(UBYTE *pubBitsSrc, UBYTE *pubBytesDst, INDEX ctBytes)
{
  // for each byte of bits
  for (INDEX i=0; i<(ctBytes+7)/8; i++) {
    // decompose it to 8 bytes
    pubBytesDst[0] = (*pubBitsSrc)&0x01;
    pubBytesDst[1] = (*pubBitsSrc)&0x02;
    pubBytesDst[2] = (*pubBitsSrc)&0x04;
    pubBytesDst[3] = (*pubBitsSrc)&0x08;
    pubBytesDst[4] = (*pubBitsSrc)&0x10;
    pubBytesDst[5] = (*pubBitsSrc)&0x20;
    pubBytesDst[6] = (*pubBitsSrc)&0x40;
    pubBytesDst[7] = (*pubBitsSrc)&0x80;
    pubBitsSrc++;
    pubBytesDst+=8;
  }
}

// remember general data
void CLayerMaker::CalculateData(void)
{
  lm_mexSizeU   = lm_pbsmShadowMap->sm_mexWidth;
  lm_mexSizeV   = lm_pbsmShadowMap->sm_mexHeight;
  lm_mexOffsetU = lm_pbsmShadowMap->sm_mexOffsetX;
  lm_mexOffsetV = lm_pbsmShadowMap->sm_mexOffsetY;
  lm_iMipLevel  = lm_pbsmShadowMap->sm_iFirstMipLevel;
  lm_pixSizeU   = lm_mexSizeU>>lm_iMipLevel;
  lm_pixSizeV   = lm_mexSizeV>>lm_iMipLevel;

  // find mip-mapping information for the polygon mask
  MakeMipmapTable( lm_pixSizeU, lm_pixSizeV, lm_mmtPolygonMask);

  CEntity *penWithPolygon = lm_pbpoPolygon->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  ASSERT(penWithPolygon!=NULL);
  const FLOATmatrix3D &mPolygonRotation = penWithPolygon->en_mRotation;
  const FLOAT3D &vPolygonTranslation = penWithPolygon->GetPlacement().pl_PositionVector;

  // get first pixel in texture in 3D
  Vector<MEX, 2> vmex0;
  vmex0(1) = -lm_mexOffsetU; //+(1<<(lm_iMipLevel-1));
  vmex0(2) = -lm_mexOffsetV; //+(1<<(lm_iMipLevel-1));
  lm_pbpoPolygon->bpo_mdShadow.GetSpaceCoordinates(
      lm_pbpoPolygon->bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative,
      vmex0, lm_vO);
  lm_vO = lm_vO*mPolygonRotation+vPolygonTranslation;
  // get steps for walking in texture in 3D
  Vector<MEX, 2> vmexU, vmexV;
  vmexU(1) = (1<<lm_iMipLevel)-lm_mexOffsetU; //+(1<<(lm_iMipLevel-1));
  vmexU(2) = (0<<lm_iMipLevel)-lm_mexOffsetV; //+(1<<(lm_iMipLevel-1));
  vmexV(1) = (0<<lm_iMipLevel)-lm_mexOffsetU; //+(1<<(lm_iMipLevel-1));
  vmexV(2) = (1<<lm_iMipLevel)-lm_mexOffsetV; //+(1<<(lm_iMipLevel-1));
  lm_pbpoPolygon->bpo_mdShadow.GetSpaceCoordinates(
      lm_pbpoPolygon->bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative,
      vmexU, lm_vStepU);
  lm_vStepU = lm_vStepU*mPolygonRotation+vPolygonTranslation;
  lm_pbpoPolygon->bpo_mdShadow.GetSpaceCoordinates(
      lm_pbpoPolygon->bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative,
      vmexV, lm_vStepV);
  lm_vStepV = lm_vStepV*mPolygonRotation+vPolygonTranslation;
  lm_vStepU-=lm_vO;
  lm_vStepV-=lm_vO;

  // make 3 orthogonal vectors that define mapping orientation
  FLOAT3D vX = lm_vStepU;
  FLOAT3D vY = -lm_vStepV;
  FLOAT3D vZ = vX*vY;
  // make a rotation matrix from those vectors
  vX.Normalize();
  vY.Normalize();
  vZ.Normalize();
  FLOATmatrix3D mOrientation;
  mOrientation(1,1) = vX(1); mOrientation(1,2) = vY(1); mOrientation(1,3) = vZ(1);
  mOrientation(2,1) = vX(2); mOrientation(2,2) = vY(2); mOrientation(2,3) = vZ(2);
  mOrientation(3,1) = vX(3); mOrientation(3,2) = vY(3); mOrientation(3,3) = vZ(3);
  FLOATmatrix3D mInvOrientation;
  mInvOrientation(1,1) = -vX(1); mInvOrientation(1,2) = vY(1); mInvOrientation(1,3) = -vZ(1);
  mInvOrientation(2,1) = -vX(2); mInvOrientation(2,2) = vY(2); mInvOrientation(2,3) = -vZ(2);
  mInvOrientation(3,1) = -vX(3); mInvOrientation(3,2) = vY(3); mInvOrientation(3,3) = -vZ(3);
  // make orientation angles from the matrix
  DecomposeRotationMatrixNoSnap(lm_aMappingOrientation, mOrientation);
  DecomposeRotationMatrixNoSnap(lm_aInverseMappingOrientation, mInvOrientation);

  // remember matrix for parallel lights
  lm_mToInverseMapping = !mInvOrientation;
}

// test if a point is inside a brush polygon
inline BOOL TestPointInPolygon(CBrushPolygon &bpo, const FLOAT3D &v)
{
  // find major axes of the polygon plane
  INDEX iMajorAxis1 = bpo.bpo_pbplPlane->bpl_iPlaneMajorAxis1;
  INDEX iMajorAxis2 = bpo.bpo_pbplPlane->bpl_iPlaneMajorAxis2;
  // if the point is not inside the bounding box of polygon (projected to the major plane)
  if (v(iMajorAxis1)<bpo.bpo_boxBoundingBox.Min()(iMajorAxis1)
    ||v(iMajorAxis1)>bpo.bpo_boxBoundingBox.Max()(iMajorAxis1)
    ||v(iMajorAxis2)<bpo.bpo_boxBoundingBox.Min()(iMajorAxis2)
    ||v(iMajorAxis2)>bpo.bpo_boxBoundingBox.Max()(iMajorAxis2)
    ) {
    // it is not inside the polygon
    return FALSE;
  }

  // create an intersector
  CIntersector isIntersector(v(iMajorAxis1), v(iMajorAxis2));
  // for all edges in the polygon
  FOREACHINSTATICARRAY(bpo.bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
    // get edge vertices (edge direction is irrelevant here!)
    const FLOAT3D &vVertex0 = itbpe->bpe_pbedEdge->bed_pbvxVertex0->bvx_vAbsolute;
    const FLOAT3D &vVertex1 = itbpe->bpe_pbedEdge->bed_pbvxVertex1->bvx_vAbsolute;
    // pass the edge to the intersector
    isIntersector.AddEdge(
      vVertex0(iMajorAxis1), vVertex0(iMajorAxis2),
      vVertex1(iMajorAxis1), vVertex1(iMajorAxis2));
  }

  // ask the intersector if the point is inside the polygon
  return isIntersector.IsIntersecting();
}

/////////////////////////////////////////////////////////////////////
// CLayerMaker

/*
 * Constructor.
 */
CLayerMaker::CLayerMaker(void)
{
}

/* Spread the shadow towards pixels outside of polygon. */
void CLayerMaker::SpreadShadowMaskOutwards(void)
{
  // for each mip map of the layer
  for(INDEX iMipmap=0; iMipmap<lm_mmtLayer.mmt_ctMipmaps; iMipmap++) {
    PIX pixLayerMinU = lm_pixLayerMinU>>iMipmap;
    PIX pixLayerMinV = lm_pixLayerMinV>>iMipmap;
    PIX pixLayerSizeU = lm_pixLayerSizeU>>iMipmap;
    PIX pixLayerSizeV = lm_pixLayerSizeV>>iMipmap;
    //PIX pixSizeU = lm_pixSizeU>>iMipmap;
    //PIX pixSizeV = lm_pixSizeV>>iMipmap;
    PIX pixSizeULog2 = FastLog2(lm_pixSizeU)-iMipmap;
    UBYTE *pubLayer = lm_pubLayer+lm_mmtLayer.mmt_aslOffsets[iMipmap];
    UBYTE *pubPolygonMask = lm_pubPolygonMask+lm_mmtPolygonMask.mmt_aslOffsets[iMipmap];

    SLONG slOffsetLayer = 0;
    // for each pixel in the layer shadow mask
    for (PIX pixLayerV=0; pixLayerV<pixLayerSizeV; pixLayerV++) {
      for (PIX pixLayerU=0; pixLayerU<pixLayerSizeU; pixLayerU++) {
        PIX pixMapU = pixLayerU+pixLayerMinU;
        PIX pixMapV = pixLayerV+pixLayerMinV;
        PIX slOffsetMap = pixMapU+(pixMapV<<pixSizeULog2);

        // if the pixel is not inside the polygon
        if (!pubPolygonMask[slOffsetMap]) {
          // find number of all of its neighbours that are in the polygon
          // and number of all of them that are not in the shadow
          INDEX ctInPolygon = 0;
          INDEX ctLighted = 0;

  #define ADDNEIGHBOUR(du, dv)                                              \
    if ((pixLayerU+(du)>=0)                                                 \
      &&(pixLayerU+(du)<pixLayerSizeU)                                      \
      &&(pixLayerV+(dv)>=0)                                                 \
      &&(pixLayerV+(dv)<pixLayerSizeV)                                      \
      &&(pubPolygonMask[slOffsetMap+(du)+((dv)<<pixSizeULog2)])) {          \
      ctInPolygon++;                                                        \
      ctLighted+=pubLayer[slOffsetLayer+(du)+(dv)*pixLayerSizeU]&0x01;   \
    }

          ADDNEIGHBOUR(-2, -2);
          ADDNEIGHBOUR(-1, -2);
          ADDNEIGHBOUR(+0, -2);
          ADDNEIGHBOUR(+1, -2);
          ADDNEIGHBOUR(+2, -2);

          ADDNEIGHBOUR(-2, -1);
          ADDNEIGHBOUR(-1, -1);
          ADDNEIGHBOUR(+0, -1);
          ADDNEIGHBOUR(+1, -1);
          ADDNEIGHBOUR(+2, -1);

          ADDNEIGHBOUR(-2, +0);
          ADDNEIGHBOUR(-1, +0);
  //      ADDNEIGHBOUR(+0, +0);
          ADDNEIGHBOUR(+1, +0);
          ADDNEIGHBOUR(+2, +0);

          ADDNEIGHBOUR(-2, +1);
          ADDNEIGHBOUR(-1, +1);
          ADDNEIGHBOUR(+0, +1);
          ADDNEIGHBOUR(+1, +1);
          ADDNEIGHBOUR(+2, +1);

          ADDNEIGHBOUR(-2, +2);
          ADDNEIGHBOUR(-1, +2);
          ADDNEIGHBOUR(+0, +2);
          ADDNEIGHBOUR(+1, +2);
          ADDNEIGHBOUR(+2, +2);

          // if there are some neighbours inside, and most of them are lighted
          if ((ctInPolygon>0) && (ctLighted>ctInPolygon/2)) {
            // make this one lighted
            pubLayer[slOffsetLayer] = 255;
          // otherwise
          } else {
            // make this one shadowed
            pubLayer[slOffsetLayer] = 0;
          }
        // if the pixel is inside the polygon
        } else {
          NOTHING;
        }

        slOffsetLayer++;
      }
    }
  }
}

/* Spread the shadow towards pixels inside of polygon. */
void CLayerMaker::SpreadShadowMaskInwards(void)
{
  // for each mip map of the layer
  for(INDEX iMipmap=0; iMipmap<lm_mmtLayer.mmt_ctMipmaps; iMipmap++) {
    PIX pixLayerMinU = lm_pixLayerMinU>>iMipmap;
    PIX pixLayerMinV = lm_pixLayerMinV>>iMipmap;
    PIX pixLayerSizeU = lm_pixLayerSizeU>>iMipmap;
    PIX pixLayerSizeV = lm_pixLayerSizeV>>iMipmap;
    //PIX pixSizeU = lm_pixSizeU>>iMipmap;
    //PIX pixSizeV = lm_pixSizeV>>iMipmap;
    PIX pixSizeULog2 = FastLog2(lm_pixSizeU)-iMipmap;
    UBYTE *pubLayer = lm_pubLayer+lm_mmtLayer.mmt_aslOffsets[iMipmap];
    UBYTE *pubPolygonMask = lm_pubPolygonMask+lm_mmtPolygonMask.mmt_aslOffsets[iMipmap];

    SLONG slOffsetLayer = 0;
    // for each pixel in the layer shadow mask
    for (PIX pixLayerV=0; pixLayerV<pixLayerSizeV; pixLayerV++) {
      for (PIX pixLayerU=0; pixLayerU<pixLayerSizeU; pixLayerU++) {
        PIX pixMapU = pixLayerU+pixLayerMinU;
        PIX pixMapV = pixLayerV+pixLayerMinV;
        PIX slOffsetMap = pixMapU+(pixMapV<<pixSizeULog2);

        // if the pixel is inside the polygon
        if (pubPolygonMask[slOffsetMap]) {
          // find number of all of its neighbours that are out of the polygon
          // and number of all of them that are not in the shadow
          INDEX ctOutPolygon = 0;
          INDEX ctLighted = 0;

#undef ADDNEIGHBOUR

  #define ADDNEIGHBOUR(du, dv)                                              \
    if ((pixLayerU+(du)>=0)                                                 \
      &&(pixLayerU+(du)<pixLayerSizeU)                                      \
      &&(pixLayerV+(dv)>=0)                                                 \
      &&(pixLayerV+(dv)<pixLayerSizeV))                                      \
          {\
      if(!pubPolygonMask[slOffsetMap+(du)+((dv)<<pixSizeULog2)]) {          \
      ctOutPolygon++;                                                        \
      ctLighted+=pubLayer[slOffsetLayer+(du)+(dv)*pixLayerSizeU]&0x01;   \
      }\
    }


 /*
          ADDNEIGHBOUR(-2, -2);
          ADDNEIGHBOUR(+2, -2);
          ADDNEIGHBOUR(-2, +2);
          ADDNEIGHBOUR(+2, +2);
          */

          ADDNEIGHBOUR(-1, -2);
          ADDNEIGHBOUR(+0, -2);
          ADDNEIGHBOUR(+1, -2);
          ADDNEIGHBOUR(-2, -1);
          ADDNEIGHBOUR(+2, -1);
          ADDNEIGHBOUR(-2, +0);
          ADDNEIGHBOUR(+2, +0);
          ADDNEIGHBOUR(-2, +1);
          ADDNEIGHBOUR(+2, +1);
          ADDNEIGHBOUR(-1, +2);
          ADDNEIGHBOUR(+0, +2);
          ADDNEIGHBOUR(+1, +2);
 
          ADDNEIGHBOUR(-1, -1);
          ADDNEIGHBOUR(+0, -1);
          ADDNEIGHBOUR(+1, -1);
          ADDNEIGHBOUR(-1, +0);
  //      ADDNEIGHBOUR(+0, +0);
          ADDNEIGHBOUR(+1, +0);
          ADDNEIGHBOUR(-1, +1);
          ADDNEIGHBOUR(+0, +1);
          ADDNEIGHBOUR(+1, +1);

          // if there are some neighbours outside
          if (ctOutPolygon>0) {
            // if some are not lighted
            if (ctLighted<ctOutPolygon) {
              // make this one shadowed
              pubLayer[slOffsetLayer] = 0;
            // otherwise
            } else {
              // make this one lighted
//              pubLayer[slOffsetLayer] = 255;
            }
          }
        // if the pixel is not inside the polygon
        } else {
          NOTHING;
        }

        slOffsetLayer++;
      }
    }
  }
}

// make bit-packed mask of where the polygon is in the shadow map
void CLayerMaker::MakePolygonMask(void)
{
  // allocate memory for the mask
  lm_pubPolygonMask = (UBYTE *)AllocMemory(lm_mmtPolygonMask.mmt_slTotalSize+8);

  // if there is packed polygon mask remembered in the shadow map
  if (lm_pbsmShadowMap->bsm_pubPolygonMask!=NULL) {
    // convert it from bit-packed into byte-packed mask
    ConvertBitsToBytes(
      lm_pbsmShadowMap->bsm_pubPolygonMask,
      lm_pubPolygonMask,
      lm_mmtPolygonMask.mmt_slTotalSize);

  } else {
    UBYTE *pub = lm_pubPolygonMask;
    // for each mip-map
    for (INDEX iMipmap=0; iMipmap<lm_mmtPolygonMask.mmt_ctMipmaps; iMipmap++) {
      //UBYTE *pubForSaving = pub;
      // start at the first pixel
      FLOAT3D vRow = lm_vO+(lm_vStepU+lm_vStepV)*(FLOAT(1<<iMipmap)/2.0f);
      // for each pixel in the shadow map
      for (PIX pixV=0; pixV<lm_pixSizeV>>iMipmap; pixV++) {
        FLOAT3D vPoint = vRow;
        for (PIX pixU=0; pixU<lm_pixSizeU>>iMipmap; pixU++) {
          // if the point is in the polygon
          if (TestPointInPolygon(*lm_pbpoPolygon, vPoint)) {
            // set the pixel
            *pub = 255;
          // if the point is not in the polygon
          } else {
            // clear the pixel
            *pub = 0;
          }
          // go to the next pixel
          pub++;
          vPoint+=lm_vStepU*FLOAT(1<<iMipmap);
        }
        vRow+=lm_vStepV*FLOAT(1<<iMipmap);
      }
    }
    // make mip maps of the polygon mask
//    MakeMipmapsForMask(lm_pubPolygonMask, lm_pixSizeU, lm_pixSizeV,
//      lm_mmtPolygonMask.mmt_slTotalSize);

    // convert it from byte-packed into bit-packed mask
    lm_pbsmShadowMap->bsm_pubPolygonMask = (UBYTE *)AllocMemory((lm_mmtPolygonMask.mmt_slTotalSize+7)/8);
    ConvertBytesToBits(
      lm_pubPolygonMask,
      lm_pbsmShadowMap->bsm_pubPolygonMask,
      lm_mmtPolygonMask.mmt_slTotalSize);
  }
}

// flip shadow mask around V axis (for parallel lights)
void CLayerMaker::FlipShadowMask(INDEX iMip)
{
  //PIX pixLayerMinU  = lm_pixLayerMinU>>iMip;
  //PIX pixLayerMinV  = lm_pixLayerMinV>>iMip;
  PIX pixLayerSizeU = lm_pixLayerSizeU>>iMip;
  PIX pixLayerSizeV = lm_pixLayerSizeV>>iMip;
  UBYTE *pubLayer = lm_pubLayer+lm_mmtLayer.mmt_aslOffsets[iMip];

  UBYTE *pubRow = pubLayer;
  // for each row
  for (PIX pixV=0; pixV<pixLayerSizeV; pixV++) {
    // flip the row
    UBYTE *pubLt = pubRow;
    UBYTE *pubRt = pubRow+pixLayerSizeU-1;
    for (PIX pixU=0; pixU<pixLayerSizeU/2; pixU++) {
      Swap(*pubLt, *pubRt);
      pubLt++;
      pubRt--;
    }
    pubRow+=pixLayerSizeU;
  }
}

// make shadow mask for the light
ULONG CLayerMaker::MakeShadowMask(CBrushShadowLayer *pbsl)
{
  // if the light doesn't cast shadows, or the polygon does not receive them
  if (!(pbsl->bsl_plsLightSource->ls_ulFlags&LSF_CASTSHADOWS) ||
      (lm_pbpoPolygon->bpo_ulFlags&BPOF_DOESNOTRECEIVESHADOW) ) {
    // do nothing
    return BSLF_ALLLIGHT;
  }

  // remember current layer and its light source
  lm_plsLight = pbsl->bsl_plsLightSource;
  lm_pbslLayer = pbsl;
  lm_vLight = lm_plsLight->ls_penEntity->GetPlacement().pl_PositionVector;

  // find the influenced rectangle
  CLightRectangle lr;
  lm_pbsmShadowMap->FindLightRectangle(*lm_plsLight, lr);
  ASSERT(lr.lr_pixSizeU == lm_pbslLayer->bsl_pixSizeU);
  ASSERT(lr.lr_pixSizeV == lm_pbslLayer->bsl_pixSizeV);
  lm_fpixHotU = lr.lr_fpixHotU;
  lm_fpixHotV = lr.lr_fpixHotV;
  lm_fLightPlaneDistance = lr.lr_fLightPlaneDistance;
  lm_pixLayerMinU = lr.lr_pixMinU;
  lm_pixLayerMinV = lr.lr_pixMinV;
  lm_pixLayerSizeU= lr.lr_pixSizeU;
  lm_pixLayerSizeV= lr.lr_pixSizeV;
  // find mip-mapping information for the rectangle
  MakeMipmapTable(lm_pixLayerSizeU, lm_pixLayerSizeV, lm_mmtLayer);

  ASSERT(lr.lr_pixSizeU == lm_pbslLayer->bsl_pixSizeU);
  ASSERT(lr.lr_pixSizeV == lm_pbslLayer->bsl_pixSizeV);
  ASSERT(lr.lr_pixSizeU <= lm_pixSizeU);
  ASSERT(lr.lr_pixSizeV <= lm_pixSizeV);

  // if there is no influence, do nothing
  if ((lr.lr_pixSizeU==0) || (lr.lr_pixSizeV==0)) return BSLF_ALLDARK;
                             
  lm_pbslLayer->bsl_pixMinU  = lr.lr_pixMinU;
  lm_pbslLayer->bsl_pixMinV  = lr.lr_pixMinV;
  lm_pbslLayer->bsl_pixSizeU = lr.lr_pixSizeU;
  lm_pbslLayer->bsl_pixSizeV = lr.lr_pixSizeV;
  lm_pbslLayer->bsl_slSizeInPixels = lm_mmtLayer.mmt_slTotalSize;

  // allocate shadow mask for the light (+8 is safety wall for fast conversions)
  lm_pubLayer = (UBYTE *)AllocMemory(lm_mmtLayer.mmt_slTotalSize+8);
  //const FLOAT fEpsilon = (1<<lm_iMipLevel)/1024.0f;

  ULONG ulLighted=BSLF_ALLLIGHT|BSLF_ALLDARK;
  // if this polygon requires exact shadows
  if (lm_pbpoPolygon->bpo_ulFlags & BPOF_ACCURATESHADOWS) {
    // make each mip-map of mask for itself
    for(INDEX iMip=0; iMip<lm_mmtLayer.mmt_ctMipmaps; iMip++) {
      ulLighted&=MakeOneShadowMaskMip(iMip);
    }
  } else {
    // make first mip-map of mask
    ulLighted&=MakeOneShadowMaskMip(0);
    // make other shadow mask mips from the first one
    MakeMipmapsForMask( lm_pubLayer, lm_mmtLayer.mmt_pixU, lm_mmtLayer.mmt_pixV,
                        lm_mmtLayer.mmt_slTotalSize);
  }

  // spread the shadow mask towards pixels outside of polygon
  if( !(lm_pbpoPolygon->bpo_ulFlags&BPOF_DARKCORNERS)) {
    SpreadShadowMaskOutwards();
  } else {
    SpreadShadowMaskInwards();
    SpreadShadowMaskOutwards();
  }

  // convert the shadow mask from byte-packed into bit-packed mask
  ConvertBytesToBits(lm_pubLayer, lm_pubLayer, lm_mmtLayer.mmt_slTotalSize);
  ShrinkMemory((void **)&lm_pubLayer, (lm_mmtLayer.mmt_slTotalSize+7)/8);
  pbsl->bsl_pubLayer = lm_pubLayer;

  // update statistics
  _ctShadowLayers++;
  _ctShadowClusters+=lm_mmtLayer.mmt_slTotalSize;

  return ulLighted;
}

ULONG CLayerMaker::MakeOneShadowMaskMip(INDEX iMip)
{
  ULONG ulLighted=BSLF_ALLLIGHT|BSLF_ALLDARK;

  PIX pixLayerMinU  = lm_pixLayerMinU>>iMip;
  PIX pixLayerMinV  = lm_pixLayerMinV>>iMip;
  PIX pixLayerSizeU = lm_pixLayerSizeU>>iMip;
  PIX pixLayerSizeV = lm_pixLayerSizeV>>iMip;
  INDEX iMipLevel = lm_iMipLevel+iMip;
  FLOAT3D vO = lm_vO+(lm_vStepU+lm_vStepV)*(FLOAT(1<<iMip)/2.0f);
  FLOAT3D vStepU = lm_vStepU*FLOAT(1<<iMip);
  FLOAT3D vStepV = lm_vStepV*FLOAT(1<<iMip);
  FLOAT fpixHotU = lm_fpixHotU/FLOAT(1<<iMip);
  FLOAT fpixHotV = lm_fpixHotV/FLOAT(1<<iMip);
  UBYTE *pubLayer = lm_pubLayer+lm_mmtLayer.mmt_aslOffsets[iMip];

  // if the light is directional
  if (lm_plsLight->ls_ulFlags&LSF_DIRECTIONAL) {
    // prepare parallel projection as if viewing from polygon and the shadow map is screen
    CParallelProjection3D prProjection;
    prProjection.ScreenBBoxL() = FLOATaabbox2D(
      FLOAT2D(pixLayerMinU,
              pixLayerMinV),
      FLOAT2D(pixLayerMinU+pixLayerSizeU,
              pixLayerMinV+pixLayerSizeV)
    );
    prProjection.AspectRatioL() = 1.0f;
    prProjection.NearClipDistanceL() = 0.00f;
    prProjection.pr_vZoomFactors(1) =
    prProjection.pr_vZoomFactors(2) = 1024.0f/(1<<iMipLevel);

    FLOAT3D vDirection;
    AnglesToDirectionVector(
      lm_plsLight->ls_penEntity->GetPlacement().pl_OrientationAngle,
      vDirection);
    // if polygon is turned away from the light
    if ((vDirection%(const FLOAT3D &)lm_pbpoPolygon->bpo_pbplPlane->bpl_plAbsolute)>-0.001) {
      // layer is all dark
      return BSLF_ALLDARK;
    }

    vDirection = vDirection*lm_mToInverseMapping;

    prProjection.pr_vStepFactors(1) = -vDirection(1)/vDirection(3);
    prProjection.pr_vStepFactors(2) = -vDirection(2)/vDirection(3);

    prProjection.pr_vStepFactors(1)*=prProjection.pr_vZoomFactors(1);
    prProjection.pr_vStepFactors(2)*=prProjection.pr_vZoomFactors(2);

    CPlacement3D plCenter;
    plCenter.pl_OrientationAngle = lm_aInverseMappingOrientation;
    plCenter.pl_PositionVector =
      vO
      +vStepU*(FLOAT(pixLayerSizeU)/2-0.5f) // !!!!
      +vStepV*(FLOAT(pixLayerSizeV)/2);//+5.0f);
    prProjection.ViewerPlacementL() = plCenter;

    // render the view to the shadow layer (but ignore the target polygon)
    CAnyProjection3D apr;
    apr = prProjection;
    ULONG ulFlagsBefore = lm_pbpoPolygon->bpo_ulFlags;
    lm_pbpoPolygon->bpo_ulFlags |= BPOF_INVISIBLE;
    ulLighted&=RenderShadows(*lm_pwoWorld, *(CEntity*)NULL, apr,
      lm_pbpoPolygon->bpo_boxBoundingBox, pubLayer, pixLayerSizeU, pixLayerSizeV,
      lm_plsLight->ls_ubPolygonalMask);
    lm_pbpoPolygon->bpo_ulFlags = ulFlagsBefore;

    // flip the shadow mask around v axis (left-right)
    FlipShadowMask(iMip);

  // if the light is point
  } else {

    // prepare perspective projection as if viewing from light and the shadow map is screen
    CPerspectiveProjection3D prProjection;
    if( !(lm_pbpoPolygon->bpo_ulFlags&BPOF_DARKCORNERS)) {
      prProjection.ScreenBBoxL() = FLOATaabbox2D(
        FLOAT2D(pixLayerMinU-fpixHotU+1,
                pixLayerMinV-fpixHotV+1),
        FLOAT2D(pixLayerMinU+pixLayerSizeU-fpixHotU+1,
                pixLayerMinV+pixLayerSizeV-fpixHotV+1)
      );
    } else {
      prProjection.ScreenBBoxL() = FLOATaabbox2D(
        FLOAT2D(pixLayerMinU-fpixHotU,
                pixLayerMinV-fpixHotV),
        FLOAT2D(pixLayerMinU+pixLayerSizeU-fpixHotU,
                pixLayerMinV+pixLayerSizeV-fpixHotV)
      );
    }
    prProjection.AspectRatioL() = 1.0f;

    prProjection.NearClipDistanceL() = lm_plsLight->ls_fNearClipDistance;
    prProjection.FarClipDistanceL()
      = lm_fLightPlaneDistance-lm_plsLight->ls_fFarClipDistance; //-fEpsilon/2; !!!! use minimal epsilon for polygon
    prProjection.ppr_fMetersPerPixel = (1<<iMipLevel)/1024.0f;
    prProjection.ppr_fViewerDistance = lm_fLightPlaneDistance;

    CPlacement3D plLight;
    plLight.pl_OrientationAngle = lm_aMappingOrientation;
    plLight.pl_PositionVector = lm_vLight;
    prProjection.ViewerPlacementL() = plLight;
    CAnyProjection3D apr;
    apr = prProjection;

    // ignore the target polygon during rendering
    ULONG ulFlagsBefore = lm_pbpoPolygon->bpo_ulFlags;
    lm_pbpoPolygon->bpo_ulFlags |= BPOF_INVISIBLE;
    // if light is not illumination light
    if (lm_plsLight->ls_ubPolygonalMask==0) {
      // just render starting at the light entity position
      ulLighted&=RenderShadows(*lm_pwoWorld, *lm_plsLight->ls_penEntity,
        apr, FLOATaabbox3D(),
        pubLayer, pixLayerSizeU, pixLayerSizeV,
        lm_plsLight->ls_ubPolygonalMask);
    // if light is illumination light
    } else {
      // add entire box around target polygon and light position to rendering
      FLOATaabbox3D box = lm_pbpoPolygon->bpo_boxBoundingBox;
      box|=lm_plsLight->ls_penEntity->GetPlacement().pl_PositionVector;
      ulLighted&=RenderShadows(*lm_pwoWorld, *(CEntity*)NULL, apr, box,
        pubLayer, pixLayerSizeU, pixLayerSizeV,
        lm_plsLight->ls_ubPolygonalMask);
    }
    lm_pbpoPolygon->bpo_ulFlags = ulFlagsBefore;
  }

  return ulLighted;
}

/*
 * Create a shadow map for a given polygon.
 */
BOOL CLayerMaker::CreateLayers(CBrushPolygon &bpo, CWorld &woWorld, BOOL bDoDirectionalLights)
{
//  __pfWorldEditingProfile.IncrementAveragingCounter();
//  __pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_MAKESHADOWS);

  BOOL bInitialized = FALSE;
  BOOL bCalculatedSome = FALSE;
  BOOL bSomeAreUncalculated = FALSE;

  // remember the world
  lm_pwoWorld = &woWorld;
  lm_pbsmShadowMap = &bpo.bpo_smShadowMap;
  lm_pbpoPolygon = &bpo;

  // for each layer that should be calculated, but isn't
  FORDELETELIST(CBrushShadowLayer, bsl_lnInShadowMap, lm_pbsmShadowMap->bsm_lhLayers, itbsl) {
    // if already calculated, or dynamic
    if (itbsl->bsl_ulFlags&BSLF_CALCULATED || 
        itbsl->bsl_plsLightSource->ls_ulFlags&LSF_DYNAMIC) {
      // skip it
      continue;
    }
    // if we are not doing directional lights and it is directional
    if (!bDoDirectionalLights
      && (itbsl->bsl_plsLightSource->ls_ulFlags&LSF_DIRECTIONAL)) {
      // skip it
      bSomeAreUncalculated = TRUE;
      continue;
    }

    // if not yet initialized
    if( !bInitialized) {
      // remember general data
      CalculateData();
      // make bit-packed mask of where the polygon is in the shadow map
      MakePolygonMask();
      bInitialized = TRUE;
    }

    CBrushShadowLayer &bsl = *itbsl;
    // mark the layer is calculated
    bsl.bsl_ulFlags |= BSLF_CALCULATED;
    // make shadow mask for the light
    ULONG ulLighted=MakeShadowMask(itbsl);
    ASSERT((ulLighted==0) || (ulLighted==BSLF_ALLLIGHT) || (ulLighted==BSLF_ALLDARK));
    bsl.bsl_ulFlags &= ~(BSLF_ALLLIGHT|BSLF_ALLDARK);
    bsl.bsl_ulFlags |= ulLighted;

    // if the layer is not needed
    if( ulLighted&(BSLF_ALLLIGHT|BSLF_ALLDARK)) {
      // free it
      if( bsl.bsl_pubLayer!=NULL) FreeMemory( bsl.bsl_pubLayer);
      bsl.bsl_pubLayer = NULL;
    }
    bCalculatedSome = TRUE;
  }

  // if was intialized
  if( bInitialized) {
    // free bit-packed polygon mask
    FreeMemory( lm_pubPolygonMask);
  }

  // if some new layers have been calculated
  if( bCalculatedSome) {
    // invalidate mixed layers
    bpo.bpo_smShadowMap.Invalidate();
  }

  return bSomeAreUncalculated;
}

/*
 * Create shadow map for the polygon.
 */
void CBrushPolygon::MakeShadowMap(CWorld *pwoWorld, BOOL bDoDirectionalLights)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_MAKESHADOWMAP);
  // create new shadow map
  CLayerMaker lmMaker;
  BOOL bSomeAreUncalculated = lmMaker.CreateLayers(*this, *pwoWorld, bDoDirectionalLights);
  // unqueue the shadow map
  if (!bSomeAreUncalculated && bpo_smShadowMap.bsm_lnInUncalculatedShadowMaps.IsLinked()) {
    bpo_smShadowMap.bsm_lnInUncalculatedShadowMaps.Remove();
  }
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_MAKESHADOWMAP);
}
