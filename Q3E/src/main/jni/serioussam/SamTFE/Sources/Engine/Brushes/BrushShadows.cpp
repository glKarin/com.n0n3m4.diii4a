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
#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushTransformed.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Base/Stream.h>
#include <Engine/Light/LightSource.h>
#include <Engine/Light/Gradient.h>
#include <Engine/Entities/ShadingInfo.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Graphics/Color.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/World/World.h>
#include <Engine/Entities/Entity.h>

#include <Engine/Light/Shadows_internal.h>

#define VERSION_CURRENT 1

// max allowed size of shadowmap in pixels
#define MAX_SHADOWMAP_SIZE 65536


CBrushShadowLayer::CBrushShadowLayer()
{
  bsl_ulFlags = 0;
  bsl_pbsmShadowMap = NULL;
  bsl_plsLightSource = NULL;
  bsl_pixMinU = 0;
  bsl_pixMinV = 0;
  bsl_pixSizeU = 0;
  bsl_pixSizeV = 0;
  bsl_slSizeInPixels = 0;
  bsl_pubLayer = NULL;
  bsl_colLastAnim = C_BLACK;
}

// destructor
CBrushShadowLayer::~CBrushShadowLayer(void)
{
  DiscardShadows();
}


// discard shadows but keep the layer
void CBrushShadowLayer::DiscardShadows(void)
{
  // if the layer is calculated
  if (bsl_pubLayer!=NULL) {
    // free its memory
    FreeMemory(bsl_pubLayer);
    bsl_pubLayer = NULL;
    bsl_slSizeInPixels = 0;
  }
  bsl_ulFlags&=~(BSLF_CALCULATED|BSLF_ALLDARK|BSLF_ALLLIGHT);
}


/*
 * Discard shadow on the polygon.
 */
void CBrushPolygon::DiscardShadows(void)
{
  bpo_smShadowMap.DiscardAllLayers();
  InitializeShadowMap();
}


/* Initialize shadow map for the polygon. */
void CBrushPolygon::InitializeShadowMap(void)
{
  // reset shadow mapping to be default for its plane
  bpo_mdShadow = CMappingDefinition();
  // init the bounding box of the shadow map as empty
  MEXaabbox2D boxPolygonMap;
  // for each edge in polygon
  FOREACHINSTATICARRAY(bpo_abpePolygonEdges, CBrushPolygonEdge, itbpe) {
    // find coordinates for first vertex
    FLOAT3D v0, v1;
    itbpe->GetVertexCoordinatesRelative(v0, v1);
    // find mapping coordinates for first vertex
    MEX2D vTexture;
    bpo_mdShadow.GetTextureCoordinates(
      bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, v0, vTexture);
    // add the vertex to the box
    boxPolygonMap |= vTexture;
  }

  // extract mexel dimensions from the bounding box
  MEX2D vmexShadowMin = boxPolygonMap.Min();
  MEX mexMinU = vmexShadowMin(1);
  MEX mexMinV = vmexShadowMin(2);
  MEX2D vmexShadowSize = boxPolygonMap.Size();
  MEX mexSizeU = vmexShadowSize(1);
  MEX mexSizeV = vmexShadowSize(2);

  // mip level is initially minimum mip level that generates needed precision for the polygon
  // (size=(2^ub)*0.5m) (-1 is for *0.5m)
  INDEX iMipLevel = (MAX_MEX_LOG2+bpo_bppProperties.bpp_sbShadowClusterSize-1);

  // expand shadow map for the sake of dark corners
  if( bpo_ulFlags&BPOF_DARKCORNERS) {
    mexSizeU += 2<<iMipLevel;
    mexSizeV += 2<<iMipLevel;
  }
  // round the dimensions up to power of 2
  INDEX iSizeULog2 = (INDEX)ceil(Log2(mexSizeU));
  INDEX iSizeVLog2 = (INDEX)ceil(Log2(mexSizeV));
  mexSizeU = 1<<iSizeULog2;
  mexSizeV = 1<<iSizeVLog2;

  // calculate dimensions in pixels and eventually reduce shadowmap size
  PIX pixSizeU  = mexSizeU>>iMipLevel;
  PIX pixSizeV  = mexSizeV>>iMipLevel;
  INDEX iMipAdj = ClampTextureSize( (PIX)MAX_SHADOWMAP_SIZE, _pGfx->gl_pixMaxTextureDimension, pixSizeU, pixSizeV);
  pixSizeU   = ClampDn( pixSizeU>>iMipAdj, (INDEX)1);
  pixSizeV   = ClampDn( pixSizeV>>iMipAdj, (INDEX)1);
  iMipLevel += iMipAdj;

  // move shadow map offset for the sake of dark corners
  if( bpo_ulFlags&BPOF_DARKCORNERS) {
    mexMinU -= 1<<iMipLevel;
    mexMinV -= 1<<iMipLevel;
  }
  // recalculate dimensions and offsets in mex back from the dimensions in pixels
  mexSizeU = (pixSizeU<<iMipLevel);
  mexSizeV = (pixSizeV<<iMipLevel);
  MEX mexOffsetU = -mexMinU;
  MEX mexOffsetV = -mexMinV;
  // remember size of polygon (not necessarily 2^n) (min is always (0,0))
  bpo_smShadowMap.sm_pixPolygonSizeU = Min( (PIX)((vmexShadowSize(1)>>iMipLevel)+3), pixSizeU);
  bpo_smShadowMap.sm_pixPolygonSizeV = Min( (PIX)((vmexShadowSize(2)>>iMipLevel)+3), pixSizeV);
  // safety check
  ASSERT( bpo_smShadowMap.sm_pixPolygonSizeU <= _pGfx->gl_pixMaxTextureDimension &&
          bpo_smShadowMap.sm_pixPolygonSizeV <= _pGfx->gl_pixMaxTextureDimension &&
         (bpo_smShadowMap.sm_pixPolygonSizeU*bpo_smShadowMap.sm_pixPolygonSizeV) <= MAX_SHADOWMAP_SIZE);

  // initialize the shadow map
  bpo_smShadowMap.Initialize(iMipLevel, mexOffsetU, mexOffsetV, mexSizeU, mexSizeV);
  // discard polygon mask if calculated
  if (bpo_smShadowMap.bsm_pubPolygonMask != NULL) {
    FreeMemory(bpo_smShadowMap.bsm_pubPolygonMask);
    bpo_smShadowMap.bsm_pubPolygonMask = NULL;
  }

  // discard all cached shading infos for models
  DiscardShadingInfos();
}

// get shadow/light percentage at given coordinates in shadow layer
FLOAT CBrushShadowLayer::GetLightStrength(PIX pixU, PIX pixV, FLOAT fLRRatio, FLOAT fUDRatio)
{
  // if full dark layer
  if (bsl_ulFlags&BSLF_ALLDARK) {
    // full dark
    return 0.0f;
  }
  // if there is no layer mask, full light layer or the coordinates are out of the layer
  if (bsl_pubLayer==NULL || (bsl_ulFlags&BSLF_ALLLIGHT) ||
    (pixU<bsl_pixMinU) || (pixV<bsl_pixMinV) ||
    (pixU>=bsl_pixMinU+bsl_pixSizeU) || (pixV>=bsl_pixMinV+bsl_pixSizeV)) {
    // full light
    return 1.0f;
  }

  // get the coordinates of the four pixels
  PIX pixU0 = pixU-bsl_pixMinU; PIX pixU1 = Min(pixU0+1, bsl_pixSizeU-1);
  PIX pixV0 = pixV-bsl_pixMinV; PIX pixV1 = Min(pixV0+1, bsl_pixSizeV-1);
  ULONG ulOffsetUL = pixU0+pixV0*bsl_pixSizeU;
  ULONG ulOffsetUR = pixU1+pixV0*bsl_pixSizeU;
  ULONG ulOffsetDL = pixU0+pixV1*bsl_pixSizeU;
  ULONG ulOffsetDR = pixU1+pixV1*bsl_pixSizeU;
  // get light at the four pixels
  FLOAT fUL=0.0f, fUR=0.0f, fDL=0.0f, fDR=0.0f;
  if (bsl_pubLayer[ulOffsetUL/8]&(1<<(ulOffsetUL%8))) { fUL = 1.0f; };
  if (bsl_pubLayer[ulOffsetUR/8]&(1<<(ulOffsetUR%8))) { fUR = 1.0f; };
  if (bsl_pubLayer[ulOffsetDL/8]&(1<<(ulOffsetDL%8))) { fDL = 1.0f; };
  if (bsl_pubLayer[ulOffsetDR/8]&(1<<(ulOffsetDR%8))) { fDR = 1.0f; };

  // return interpolated value
  return Lerp( Lerp(fUL, fUR, fLRRatio), Lerp(fDL, fDR, fLRRatio), fUDRatio);
}

void CBrushShadowMap::ReadLayers_t( CTStream *pstrm)  // throw char *
{
  BOOL bUncalculated = FALSE;
  // if the old version of layers information is really saved here
  if (pstrm->PeekID_t()==CChunkID("SHLY")) {  // shadow layers
    pstrm->ExpectID_t("SHLY");

    // read number of layers
    INDEX ctLayers;
    *pstrm>>ctLayers;
    // for each shadow layer
    for(INDEX iLayer=0; iLayer<ctLayers; iLayer++) {
      // create a new layer
      CBrushShadowLayer *pbsl = new CBrushShadowLayer;
      pbsl->bsl_colLastAnim = 0x12345678;
      // attach it to the shadow map
      bsm_lhLayers.AddTail(pbsl->bsl_lnInShadowMap);
      // make the layer point to its shadow map
      pbsl->bsl_pbsmShadowMap = this;
      // make the light pointer dummy (it is set while loading lights)
      pbsl->bsl_plsLightSource = NULL;

      // read the layer data
      *pstrm>>pbsl->bsl_ulFlags;    // flags
      // if it is new version
      if (pbsl->bsl_ulFlags&BSLF_RECTANGLE) {
        SLONG slLayerSize;
        *pstrm>>slLayerSize;
        if (slLayerSize != 0) {
          pbsl->bsl_pubLayer = (UBYTE *)AllocMemory(slLayerSize);
          pstrm->Read_t(pbsl->bsl_pubLayer, slLayerSize); // the bit packed layer mask
        } else {
          bUncalculated = TRUE;
          pbsl->bsl_pubLayer = NULL;
        }
        // read layer rectangle
        *pstrm>>pbsl->bsl_pixMinU;
        *pstrm>>pbsl->bsl_pixMinV;
        *pstrm>>pbsl->bsl_pixSizeU;
        *pstrm>>pbsl->bsl_pixSizeV;
      // if it is old version
      } else {
        // skip it
        SLONG slLayerSize;
        *pstrm>>slLayerSize;
        if (slLayerSize != 0) {
          pstrm->Seek_t(slLayerSize, CTStream::SD_CUR);
          pbsl->bsl_pubLayer = NULL;
          bUncalculated = TRUE;
        } else {
          bUncalculated = TRUE;
          pbsl->bsl_pubLayer = NULL;
        }
        // destroy it
        pbsl->bsl_lnInShadowMap.Remove();
        delete pbsl;
      }
    }
  // if the new version of layers information is really saved here
  } else if (pstrm->PeekID_t()==CChunkID("SHLA")) {  // shadow layers
    pstrm->ExpectID_t("SHLA");
    // read polygon size
    *pstrm>>sm_pixPolygonSizeU;
    *pstrm>>sm_pixPolygonSizeV;

    // read number of layers
    INDEX ctLayers;
    *pstrm>>ctLayers;
    // for each shadow layer
    for(INDEX iLayer=0; iLayer<ctLayers; iLayer++) {
      // create a new layer
      CBrushShadowLayer *pbsl = new CBrushShadowLayer;
      pbsl->bsl_colLastAnim = 0x12345678;
      // attach it to the shadow map
      bsm_lhLayers.AddTail(pbsl->bsl_lnInShadowMap);
      // make the layer point to its shadow map
      pbsl->bsl_pbsmShadowMap = this;
      // make the light pointer dummy (it is set while loading lights)
      pbsl->bsl_plsLightSource = NULL;

      // read the layer data
      *pstrm>>pbsl->bsl_ulFlags;    // flags
      SLONG slLayerSize;
      *pstrm>>slLayerSize;
      if (slLayerSize != 0) {
        pstrm->Seek_t(slLayerSize, CTStream::SD_CUR);
      }
      bUncalculated = TRUE;
      pbsl->bsl_pubLayer = NULL;
      pbsl->bsl_ulFlags&=~BSLF_CALCULATED;
      // read layer rectangle
      *pstrm>>pbsl->bsl_pixMinU;
      *pstrm>>pbsl->bsl_pixMinV;
      *pstrm>>pbsl->bsl_pixSizeU;
      *pstrm>>pbsl->bsl_pixSizeV;

    }
  // if the new version of layers information is really saved here
  } else if (pstrm->PeekID_t()==CChunkID("SHAL")) {  // shadow layers
    pstrm->ExpectID_t("SHAL");
    // read version number
    INDEX iVersion;
    *pstrm>>iVersion;
    ASSERT(iVersion==VERSION_CURRENT);
    // read polygon size
    *pstrm>>sm_pixPolygonSizeU;
    *pstrm>>sm_pixPolygonSizeV;

    // read number of layers
    INDEX ctLayers;
    *pstrm>>ctLayers;
    // for each shadow layer
    for(INDEX iLayer=0; iLayer<ctLayers; iLayer++) {
      // create a new layer
      CBrushShadowLayer *pbsl = new CBrushShadowLayer;
      pbsl->bsl_colLastAnim = 0x12345678;
      // attach it to the shadow map
      bsm_lhLayers.AddTail(pbsl->bsl_lnInShadowMap);
      // make the layer point to its shadow map
      pbsl->bsl_pbsmShadowMap = this;
      // make the light pointer dummy (it is set while loading lights)
      pbsl->bsl_plsLightSource = NULL;

      // read the layer data
      *pstrm>>pbsl->bsl_ulFlags;    // flags
      *pstrm>>pbsl->bsl_slSizeInPixels;
      if (pbsl->bsl_slSizeInPixels != 0) {
        SLONG slLayerSize = (pbsl->bsl_slSizeInPixels+7)/8;
        pbsl->bsl_pubLayer = (UBYTE *)AllocMemory(slLayerSize);
        pstrm->Read_t(pbsl->bsl_pubLayer, slLayerSize); // the bit packed layer mask
      } else {
        bUncalculated = TRUE;
        pbsl->bsl_pubLayer = NULL;
      }
      // read layer rectangle
      *pstrm>>pbsl->bsl_pixMinU;
      *pstrm>>pbsl->bsl_pixMinV;
      *pstrm>>pbsl->bsl_pixSizeU;
      *pstrm>>pbsl->bsl_pixSizeV;

      // fixup for old levels before alllight and alldark flags
      if ((pbsl->bsl_ulFlags&BSLF_CALCULATED)
        && (pbsl->bsl_pubLayer==NULL)
        &&!(pbsl->bsl_ulFlags&BSLF_ALLLIGHT)
        &&!(pbsl->bsl_ulFlags&BSLF_ALLDARK)) {
        pbsl->bsl_ulFlags|=BSLF_ALLLIGHT;
      }
    }
  }

  // if some layers are uncalculated
  if (bUncalculated) {
    extern CWorld *_pwoCurrentLoading;  // world that is currently loading
    // add the shadow map for calculation
    _pwoCurrentLoading->wo_baBrushes.ba_lhUncalculatedShadowMaps
      .AddTail(bsm_lnInUncalculatedShadowMaps);
  }
}

void CBrushShadowMap::WriteLayers_t( CTStream *pstrm) // throw char *
{
  pstrm->WriteID_t("SHAL"); // shadow layers
  // write version number
  *pstrm<<INDEX(VERSION_CURRENT);

  // write polygon size
  *pstrm<<sm_pixPolygonSizeU;
  *pstrm<<sm_pixPolygonSizeV;

  // write number of layers
  INDEX ctLayers = 0;
  {FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl) {
    if (itbsl->bsl_plsLightSource->ls_ulFlags&LSF_NONPERSISTENT) {
      continue;
    }
    ctLayers++;
  }}
  *pstrm<<ctLayers;
  // for each shadow layer
  FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl) {
    CBrushShadowLayer &bsl = *itbsl;
    if (itbsl->bsl_plsLightSource->ls_ulFlags&LSF_NONPERSISTENT) {
      continue;
    }
    // write the layer data
    *pstrm<<bsl.bsl_ulFlags;    // flags
    if (bsl.bsl_pubLayer == NULL ) {
      *pstrm<<SLONG(0);
    } else {
      *pstrm<<bsl.bsl_slSizeInPixels;
      SLONG slLayerSize = (bsl.bsl_slSizeInPixels+7)/8;
      pstrm->Write_t(bsl.bsl_pubLayer, slLayerSize); // the bit packed layer mask
    }
    // write layer rectangle
    *pstrm<<bsl.bsl_pixMinU;
    *pstrm<<bsl.bsl_pixMinV;
    *pstrm<<bsl.bsl_pixSizeU;
    *pstrm<<bsl.bsl_pixSizeV;
  }
}

// constructor
CBrushShadowMap::CBrushShadowMap(void)
{
  bsm_pubPolygonMask = NULL;  // no polygon mask is calculated initially
  sm_pixPolygonSizeU = -1;    // polygon size must be calculated
  sm_pixPolygonSizeV = -1;
}

// discard all layers on this shadow map
void CBrushShadowMap::DiscardAllLayers(void)
{
  // for each shadow layer
  FORDELETELIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl) {
    // delete it
    delete &*itbsl;
  }
  // uncache the shadow map
  Uncache();
}


// discard shadows on all layers on this shadow map
void CBrushShadowMap::DiscardShadows(void)
{
  // for each shadow layer
  FORDELETELIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl) {
    // discard shadows on it
    itbsl->DiscardShadows();
  }
}


// remove shadow layers without valid light source
void CBrushShadowMap::RemoveDummyLayers(void)
{
  // for each shadow layer
  FORDELETELIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl) {
    // if dummy
    if (itbsl->bsl_plsLightSource==NULL) {
      // remove it
      delete &*itbsl;
    }
  }
}

// destructor
CBrushShadowMap::~CBrushShadowMap(void)
{
  // discard all layers before deleting the object itself
  DiscardAllLayers();
  // discard polygon mask if calculated
  if (bsm_pubPolygonMask != NULL) {
    FreeMemory(bsm_pubPolygonMask);
    bsm_pubPolygonMask = NULL;
  }
}

// queue the shadow map for calculation
void CBrushShadowMap::QueueForCalculation(void)
{
  // if not already queued
  if (!bsm_lnInUncalculatedShadowMaps.IsLinked()) {
    // find the world of the polygon
    CBrushPolygon *pbpo = GetBrushPolygon();
    CBrushSector *pbsc = pbpo->bpo_pbscSector;
    CBrushMip *pbm = pbsc->bsc_pbmBrushMip;
    CBrush3D *pbr = pbm->bm_pbrBrush;
    CWorld *pwo = pbr->br_penEntity->GetWorld();
    // queue the shadow map in the world
    pwo->wo_baBrushes.ba_lhUncalculatedShadowMaps.AddTail(bsm_lnInUncalculatedShadowMaps);
  }
}


// calculate the rectangle where a light influences the shadow map
void CBrushShadowMap::FindLightRectangle(CLightSource &ls, class CLightRectangle &lr)
{
  // get needed data
  CBrushPolygon &bpoPolygon = *GetBrushPolygon();
  const FLOATplane3D &plPlane = bpoPolygon.bpo_pbplPlane->bpl_plAbsolute;
  INDEX iMipLevel = sm_iFirstMipLevel;
  FLOAT3D vLight;
  PIX pixMinU, pixMinV, pixMaxU, pixMaxV;

  // if the light is directional
  if( ls.ls_ulFlags&LSF_DIRECTIONAL)
  {
    pixMinU = 0;
    pixMinV = 0;
    // rectangle is around entire shadowmap
    // pixMaxU = PIX(sm_mexWidth >>iMipLevel);
    // pixMaxV = PIX(sm_mexHeight>>iMipLevel);
    // rectangle is around entire polygon
    pixMaxU = Min( sm_pixPolygonSizeU+16, sm_mexWidth >>iMipLevel);
    pixMaxV = Min( sm_pixPolygonSizeV+16, sm_mexHeight>>iMipLevel);
  }
  // if the light is point
  else
  {
    // light position is at the light entity
    vLight = ls.ls_penEntity->GetPlacement().pl_PositionVector;
    // find the point where the light is closest to the polygon
    FLOAT3D vHotPoint = plPlane.ProjectPoint(vLight);
    lr.lr_fLightPlaneDistance = plPlane.PointDistance(vLight);

    CEntity *penWithPolygon = bpoPolygon.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    ASSERT(penWithPolygon!=NULL);
    const FLOATmatrix3D &mPolygonRotation = penWithPolygon->en_mRotation;
    const FLOAT3D &vPolygonTranslation = penWithPolygon->GetPlacement().pl_PositionVector;

    vHotPoint = (vHotPoint-vPolygonTranslation)*!mPolygonRotation;

    Vector<MEX, 2> vmexHotPoint;
    bpoPolygon.bpo_mdShadow.GetTextureCoordinates(
      bpoPolygon.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, vHotPoint, vmexHotPoint);

    if( !(bpoPolygon.bpo_ulFlags&BPOF_DARKCORNERS)) {
      lr.lr_fpixHotU = FLOAT(vmexHotPoint(1)+sm_mexOffsetX+(1<<iMipLevel))/(1L<<iMipLevel);
      lr.lr_fpixHotV = FLOAT(vmexHotPoint(2)+sm_mexOffsetY+(1<<iMipLevel))/(1L<<iMipLevel);
    } else {
      lr.lr_fpixHotU = FLOAT(vmexHotPoint(1)+sm_mexOffsetX)/(1L<<iMipLevel);
      lr.lr_fpixHotV = FLOAT(vmexHotPoint(2)+sm_mexOffsetY)/(1L<<iMipLevel);
    }
    // calculate maximum radius of light on the polygon
    MEX mexFallOff = MEX( sqrt(ls.ls_rFallOff*ls.ls_rFallOff -
                               lr.lr_fLightPlaneDistance*lr.lr_fLightPlaneDistance)*1024.0f);
    // find rectangle coordinates from that
    pixMinU = ((vmexHotPoint(1)+sm_mexOffsetX-mexFallOff)>>iMipLevel);
    pixMinV = ((vmexHotPoint(2)+sm_mexOffsetY-mexFallOff)>>iMipLevel);
    pixMaxU = ((vmexHotPoint(1)+sm_mexOffsetX+mexFallOff)>>iMipLevel)+1;
    pixMaxV = ((vmexHotPoint(2)+sm_mexOffsetY+mexFallOff)>>iMipLevel)+1;
    // clamp the rectangle to the size of shadow map
    // pixMinU = Min( Max(pixMinU, 0L), sm_mexWidth >>iMipLevel);
    // pixMinV = Min( Max(pixMinV, 0L), sm_mexHeight>>iMipLevel);
    // pixMaxU = Min( Max(pixMaxU, 0L), sm_mexWidth >>iMipLevel);
    // pixMaxV = Min( Max(pixMaxV, 0L), sm_mexHeight>>iMipLevel);
    // clamp the rectangle to the size of polygon
    pixMinU = Min( Max(pixMinU, (PIX)0), Min(sm_pixPolygonSizeU+16, sm_mexWidth >>iMipLevel));
    pixMinV = Min( Max(pixMinV, (PIX)0), Min(sm_pixPolygonSizeV+16, sm_mexHeight>>iMipLevel));
    pixMaxU = Min( Max(pixMaxU, (PIX)0), Min(sm_pixPolygonSizeU+16, sm_mexWidth >>iMipLevel));
    pixMaxV = Min( Max(pixMaxV, (PIX)0), Min(sm_pixPolygonSizeV+16, sm_mexHeight>>iMipLevel));
  }
  // all done
  lr.lr_pixMinU = pixMinU;
  lr.lr_pixMinV = pixMinV;
  lr.lr_pixSizeU = pixMaxU-pixMinU;
  lr.lr_pixSizeV = pixMaxV-pixMinV;
  ASSERT(lr.lr_pixSizeU>=0);
  ASSERT(lr.lr_pixSizeV>=0);
}


// check if all layers are up to date
void CBrushShadowMap::CheckLayersUpToDate(void)
{
  // do nothing if the shadow map is not cached at all or hasn't got any animating lights
  if( ((sm_pulDynamicShadowMap==NULL || (sm_ulFlags&SMF_DYNAMICBLACK))
   && !(sm_ulFlags&SMF_ANIMATINGLIGHTS))
   ||   sm_pulCachedShadowMap==NULL) return;

  // for each layer
  FOREACHINLIST( CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl)
  { // ignore if the layer is all dark
    CBrushShadowLayer &bsl = *itbsl;
    if( bsl.bsl_ulFlags&BSLF_ALLDARK) continue;
    // light source must be valid
    ASSERT( bsl.bsl_plsLightSource!=NULL);
    if( bsl.bsl_plsLightSource==NULL) continue;
    CLightSource &ls = *bsl.bsl_plsLightSource;
    // if the layer is not up to date
    if( bsl.bsl_colLastAnim != ls.GetLightColor()) {
      // invalidate entire shadow map
      Invalidate( ls.ls_ulFlags&LSF_DYNAMIC);
      if( !(ls.ls_ulFlags&LSF_DYNAMIC)) return;
    }
  }
}


// test if there is any dynamic layer
BOOL CBrushShadowMap::HasDynamicLayers(void)
{
  // for each layer
  FOREACHINLIST( CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl)
  { // light source must be valid
    ASSERT( itbsl->bsl_plsLightSource!=NULL);
    if( itbsl->bsl_plsLightSource==NULL) continue;
    CLightSource &ls = *itbsl->bsl_plsLightSource;
    // if the layer is dynamic, it has
    if( ls.ls_ulFlags&LSF_DYNAMIC) return TRUE;
  }
  // hasn't
  return FALSE;
}



// returns TRUE if shadowmap is all flat along with colFlat variable set to that color
BOOL CBrushShadowMap::IsShadowFlat( COLOR &colFlat)
{
  // fail if flat shadows are not allowed
  extern INDEX shd_bAllowFlats;
  extern INDEX shd_iForceFlats;
  shd_iForceFlats = Clamp( shd_iForceFlats, (INDEX)0, (INDEX)2);
  if( !shd_bAllowFlats && shd_iForceFlats<1) return FALSE;

  COLOR col;
  UBYTE ubR,ubG,ubB, ubR1,ubG1,ubB1;
  SLONG slR=0,slG=0,slB=0;
  //INDEX ctPointLights=0;
  CBrushPolygon *pbpo = GetBrushPolygon();

  // if the shadowmap is not using the shading mode
  if (pbpo->bpo_bppProperties.bpp_ubShadowBlend != BPT_BLEND_SHADE) {
    // it must not be flat
    return FALSE;
  }

  // initial color is sector color
  col = AdjustColor( pbpo->bpo_pbscSector->bsc_colAmbient, _slShdHueShift, _slShdSaturation);
  ColorToRGB( col, ubR,ubG,ubB);
  slR += ubR;  slG += ubG;  slB += ubB; 

  // if gradient layer is present
  const ULONG ulGradientType = pbpo->bpo_bppProperties.bpp_ubGradientType;
  if( ulGradientType>0)
  { 
    CGradientParameters gp;
    CEntity *pen = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    if( pen!=NULL && pen->GetGradient( ulGradientType, gp)) {
      // shadowmap cannot be flat
      if( shd_iForceFlats<1) return FALSE;
      // unless it has been forced
      ColorToRGB( gp.gp_col0, ubR, ubG, ubB);
      ColorToRGB( gp.gp_col1, ubR1,ubG1,ubB1);
      const SLONG slAvgR = ((ULONG)ubR + ubR1) /2;
      const SLONG slAvgG = ((ULONG)ubG + ubG1) /2;
      const SLONG slAvgB = ((ULONG)ubB + ubB1) /2;
      if( gp.gp_bDark) { slR -= slAvgR;  slG -= slAvgG;  slB -= slAvgB; }
      else             { slR += slAvgR;  slG += slAvgG;  slB += slAvgB; }
    }
  }

  // for each layer
  BOOL bDirLightApplied = FALSE;
  FOREACHINLIST( CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl)
  { 
    // skip dynamic layers
    CBrushShadowLayer &bsl = *itbsl;
    CLightSource &ls = *bsl.bsl_plsLightSource;
    if( ls.ls_ulFlags&LSF_DYNAMIC) continue;

    // if light is directional 
    if( ls.ls_ulFlags&LSF_DIRECTIONAL)
    {
      // fail if calculated and not all dark or all light
      if( (bsl.bsl_ulFlags&BSLF_CALCULATED) && !(bsl.bsl_ulFlags&(BSLF_ALLDARK|BSLF_ALLLIGHT))) {
        // but only if flats have not been forced!
        if( shd_iForceFlats<1) return FALSE;
      }

      // if polygon allows directional light ambient component
      if( pbpo->bpo_ulFlags&BPOF_HASDIRECTIONALAMBIENT) {
        // mix in ambient color
        col = AdjustColor( ls.GetLightAmbient(), _slShdHueShift, _slShdSaturation);
        ColorToRGB( col, ubR,ubG,ubB);
        slR += ubR;  slG += ubG;  slB += ubB; 
      }
      // done with this layer if it's all dark, or all light but without directional component
      if( (bsl.bsl_ulFlags&BSLF_ALLDARK) || !(pbpo->bpo_ulFlags&BPOF_HASDIRECTIONALLIGHT)) continue;

      // layer is all light, so calculate intensity
      col = ls.GetLightColor();
      if( !(pbpo->bpo_ulFlags&BPOF_NOPLANEDIFFUSION)) {
        FLOAT3D vLightDirection;
        AnglesToDirectionVector( ls.ls_penEntity->GetPlacement().pl_OrientationAngle, vLightDirection);
        const FLOAT fIntensity = -((pbpo->bpo_pbplPlane->bpl_plAbsolute)%vLightDirection);
        // done if polygon is turn away from light source (we already added ambient component)
        if( fIntensity<0.01f) continue;
        ULONG ulIntensity = NormFloatToByte(fIntensity);
        ulIntensity = (ulIntensity<<CT_RSHIFT) | (ulIntensity<<CT_GSHIFT) | (ulIntensity<<CT_BSHIFT);
        col = MulColors( col, ulIntensity);
      }
      // determine and add light color
      col = AdjustColor( col, _slShdHueShift, _slShdSaturation);
      ColorToRGB( col, ubR,ubG,ubB);
      slR += ubR;  slG += ubG;  slB += ubB; 
      bDirLightApplied = TRUE;
    }

    // if light is point
    else
    {
      // just fail if layer isn't all dark
      if( !(bsl.bsl_ulFlags&BSLF_ALLDARK)) {
        // and flat shadows aren't forced
        if( shd_iForceFlats<1) return FALSE;
      }
    }
  }

  // fake directional light if needed and allowed
  if( shd_iForceFlats>0 && !bDirLightApplied) {
    FLOAT3D vLightDir;
    vLightDir(1) = -3.0f;
    vLightDir(2) = -2.0f;
    vLightDir(3) = -1.0f;
    vLightDir.Normalize();
    const FLOAT fIntensity = -((pbpo->bpo_pbplPlane->bpl_plAbsolute)%vLightDir);
    if( fIntensity>0.01f) {
      const UBYTE ubGray = NormFloatToByte(fIntensity*0.49f);
      slR += ubGray;  slG += ubGray;  slB += ubGray; 
    }
  }
  // done - phew, layer is flat
  slR = Clamp( slR, (SLONG)0, (SLONG)255);
  slG = Clamp( slG, (SLONG)0, (SLONG)255);
  slB = Clamp( slB, (SLONG)0, (SLONG)255);
  colFlat = RGBToColor(slR,slG,slB);
  return TRUE;
}



// get amount of memory used by this object
SLONG CBrushShadowMap::GetUsedMemory(void)
{
  // basic size of class
  SLONG slUsedMemory = sizeof(CBrushShadowMap);

  // add polyhon mask (if any)
  if( bsm_pubPolygonMask!=NULL) {
    // loop and add mip-maps
    SLONG slPolyMaskSize = 0;
    PIX pixPolySizeU = sm_pixPolygonSizeU;
    PIX pixPolySizeV = sm_pixPolygonSizeV;
    while( pixPolySizeU>0 && pixPolySizeV>0) {
      slPolyMaskSize += pixPolySizeU * pixPolySizeV;
      pixPolySizeU  >>= 1;
      pixPolySizeV  >>= 1;
    }
    // sum it up
    slUsedMemory += (slPolyMaskSize+8) /8; // bit mask!
  }

  // loop thru layers and add 'em too
  FOREACHINLIST( CBrushShadowLayer, bsl_lnInShadowMap, bsm_lhLayers, itbsl) { // count shadow layers
    CBrushShadowLayer &bsl = *itbsl;
    slUsedMemory += sizeof(CBrushShadowLayer);
    if( bsl.bsl_pubLayer!=NULL) slUsedMemory += bsl.bsl_pixSizeU * bsl.bsl_pixSizeV /8; 
  }

  // done
  return slUsedMemory;
}
