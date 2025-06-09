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

#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Base/Stream.h>
#include <Engine/Light/LightSource.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Graphics/Color.h>
#include <Engine/World/World.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/BSP.h>
#include <Engine/Terrain/Terrain.h>

#include <Engine/Light/Shadows_internal.h>

/////////////////////////////////////////////////////////////////////
// CLightSource
// constructor
CLightSource::CLightSource(void)
{
  // set invalid properties, must be initialized by its entity
  ls_ulFlags = (ULONG) -1;
  ls_rHotSpot = -1;
  ls_rFallOff = -1;
  ls_colColor = 0;
  ls_colAmbient = 0;
  ls_ubLightAnimationObject = (UBYTE) -1;
  ls_ubPolygonalMask = (UBYTE) -1;
  ls_penEntity = NULL;
  ls_plftLensFlare = NULL;
  ls_paoLightAnimation = NULL;
  ls_paoAmbientLightAnimation = NULL;
}
// destructor
CLightSource::~CLightSource(void)
{
  // discard all linked shadow layers
  DiscardShadowLayers();
  // if this isn't dynamic light
  if(!(ls_ulFlags&LSF_DYNAMIC) && ls_penEntity!=NULL) {
    UpdateTerrains();
  }

  // delete possible lens flare infos in renderer
  extern void DeleteLensFlare(CLightSource *pls);
  DeleteLensFlare(this);
}

// read/write from a stream
void CLightSource::Read_t( CTStream *pstrm)        // throw char *
{
  // if the light information is really saved here
  if (pstrm->PeekID_t()==CChunkID("LIGH")) { // light source
    pstrm->ExpectID_t("LIGH");

    // this light source must not be non-persistent
    ASSERT(!(ls_ulFlags&LSF_NONPERSISTENT));

    // read number of layers
    INDEX ctLayers;
    *pstrm>>ctLayers;
    // for each shadow layer
    for(INDEX iLayer=0; iLayer<ctLayers; iLayer++) {
      // read indices of brush, brush mip, sector, and polygon
      INDEX iBrush, iMip, iSector, iPolygon;
      *pstrm>>iBrush;
      *pstrm>>iMip;
      *pstrm>>iSector;
      *pstrm>>iPolygon;
      // find the shadow map
      CBrush3D *pbrBrush = &ls_penEntity->en_pwoWorld->wo_baBrushes.ba_abrBrushes[iBrush];
      CBrushMip *pbm = pbrBrush->GetBrushMipByIndex(iMip);
      ASSERT(pbm!=NULL);
      pbm->bm_abscSectors.Lock();
      CBrushSector *pbsc = &pbm->bm_abscSectors[iSector];
      pbm->bm_abscSectors.Unlock();
      CBrushPolygon *pbpo = &pbsc->bsc_abpoPolygons[iPolygon];
      CBrushShadowMap *pbsm = &pbpo->bpo_smShadowMap;

      // read the index of the layer in the shadow map
      INDEX iLayerInShadowMap;
      *pstrm>>iLayerInShadowMap;
      // for each layer in the shadow map
      INDEX iLayerInShadowMapCurrent = 0;
#ifndef NDEBUG
      BOOL bLayerFound = FALSE;
#endif // NDEBUG
      FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, pbsm->bsm_lhLayers, itbsl) {
        // if it is that layer
        if (iLayerInShadowMapCurrent==iLayerInShadowMap) {
          // attach the layer to the light source
          itbsl->bsl_plsLightSource = this;
          ls_lhLayers.AddTail(itbsl->bsl_lnInLightSource);
#ifndef NDEBUG
          bLayerFound = TRUE;
#endif // NDEBUG
          break;
        }
        iLayerInShadowMapCurrent++;
      }
      // some layer must be found
      ASSERT(bLayerFound);
    }
  }
}

void CLightSource::Write_t( CTStream *pstrm)       // throw char *
{
  // if this light source is non-persistent
  if (ls_ulFlags&LSF_NONPERSISTENT) {
    // don't save it
    return;
  }
  pstrm->WriteID_t("LIGH"); // light source
  // write number of layers
  *pstrm<<ls_lhLayers.Count();
  // for each shadow layer
  FOREACHINLIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    // get the layer polygon, sector, mip and brush
    CBrushPolygon *pbpo = itbsl->bsl_pbsmShadowMap->GetBrushPolygon();
    CBrushSector *pbsc = pbpo->bpo_pbscSector;
    CBrushMip *pbm = pbsc->bsc_pbmBrushMip;
    CBrush3D *pbr = pbm->bm_pbrBrush;
    // write their indices
    *pstrm<<ls_penEntity->en_pwoWorld->wo_baBrushes.ba_abrBrushes.Index(pbr);

    *pstrm<<pbm->GetMipIndex();

    pbm->bm_abscSectors.Lock();
    *pstrm<<pbm->bm_abscSectors.Index(pbsc);
    pbm->bm_abscSectors.Unlock();

    *pstrm<<pbsc->bsc_abpoPolygons.Index(pbpo);
    // find the index of the layer in its shadow map
    INDEX iLayerInShadowMap = 0;
    FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap,
      itbsl->bsl_pbsmShadowMap->bsm_lhLayers, itbsl2) {
      if (itbsl2->bsl_plsLightSource->ls_ulFlags&LSF_NONPERSISTENT) {
        continue;
      }
      if (&*itbsl == &*itbsl2) {
        break;
      }
      iLayerInShadowMap++;
    }
    // write that index
    *pstrm<<iLayerInShadowMap;
  }
}

// uncache all influenced shadow maps, but keep shadow layers
void CLightSource::UncacheShadowMaps(void)
{
  // for each shadow layer
  FOREACHINLIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    // just invalidate its shadow map
    itbsl->bsl_pbsmShadowMap->Invalidate(ls_ulFlags&LSF_DYNAMIC);
  }
}

// discard all linked shadow layers
void CLightSource::DiscardShadowLayers(void)
{
  // for each shadow layer
  FORDELETELIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    // invalidate its shadow map
    itbsl->bsl_pbsmShadowMap->Invalidate(ls_ulFlags&LSF_DYNAMIC);
    // delete the layer
    delete &*itbsl;
  }
}

// test if a polygon has a layer from this light source
BOOL CLightSource::PolygonHasLayer(CBrushPolygon &bpo)
{
  // for each shadow layer in the polygon
  FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, bpo.bpo_smShadowMap.bsm_lhLayers, itbsl) {
    // if it is from this light source
    if (itbsl->bsl_plsLightSource==this) {
      // it does have
      return TRUE;
    }
  }
  // otherwise, it doesn't have
  return FALSE;
}

// set layer parameters
void CLightSource::SetLayerParameters(CBrushShadowLayer &bsl, CBrushPolygon &bpo, class CLightRectangle &lr)
{
  // remember the rectangle of the layer
  bsl.bsl_pixMinU  = lr.lr_pixMinU;
  bsl.bsl_pixMinV  = lr.lr_pixMinV;
  bsl.bsl_pixSizeU = lr.lr_pixSizeU;
  bsl.bsl_pixSizeV = lr.lr_pixSizeV;
  bsl.bsl_ulFlags |= BSLF_RECTANGLE;

  // invalidate the shadow map
  bpo.bpo_smShadowMap.Invalidate(ls_ulFlags&LSF_DYNAMIC);

  // if the light casts shadows
  if (ls_ulFlags&LSF_CASTSHADOWS) {
    // queue the shadow map for calculating
    bpo.bpo_smShadowMap.QueueForCalculation();
  }
}

// add a layer to a polygon
void CLightSource::AddLayer(CBrushPolygon &bpo)
{
  // find the influenced rectangle
  CLightRectangle lr;
  bpo.bpo_smShadowMap.FindLightRectangle(*this, lr);
  // if there is no influence
  if ((lr.lr_pixSizeU==0) || (lr.lr_pixSizeV==0)) {
    // do nothing
    return;
  }
  // create a new layer
  CBrushShadowLayer &bsl = *new CBrushShadowLayer;
  bsl.bsl_colLastAnim = 0x12345678;
  // attach it to light source and shadow map
  bsl.bsl_plsLightSource = this;
  ls_lhLayers.AddTail(bsl.bsl_lnInLightSource);
  bsl.bsl_pbsmShadowMap = &bpo.bpo_smShadowMap;
  // if the light is dark light
  if (ls_ulFlags & LSF_DARKLIGHT) {
    // add to end of list
    bpo.bpo_smShadowMap.bsm_lhLayers.AddTail(bsl.bsl_lnInShadowMap);
  // if the light is normal light
  } else {
    // add to beginning of list
    bpo.bpo_smShadowMap.bsm_lhLayers.AddHead(bsl.bsl_lnInShadowMap);
  }

  // initially it has no shadow
  bsl.bsl_pubLayer = NULL;
  bsl.bsl_ulFlags = 0;

  SetLayerParameters(bsl, bpo, lr);
}

void CLightSource::UpdateLayer(CBrushShadowLayer &bsl)
{
  CBrushPolygon &bpo = *bsl.bsl_pbsmShadowMap->GetBrushPolygon();
  // find the influenced rectangle
  CLightRectangle lr;
  bsl.bsl_pbsmShadowMap->FindLightRectangle(*this, lr);

  // if there is no influence
  if ((lr.lr_pixSizeU==0) || (lr.lr_pixSizeV==0)) {
    // invalidate its shadow map
    bsl.bsl_pbsmShadowMap->Invalidate(ls_ulFlags&LSF_DYNAMIC);
    bpo.bpo_ulFlags &= ~BPOF_MARKEDLAYER;
    // delete the layer
    delete &bsl;
    return;
  }
  // discard shadows on the layer
  bsl.DiscardShadows();
  SetLayerParameters(bsl, bpo, lr);
}

static inline BOOL IsPolygonInfluencedByDirectionalLight(CBrushPolygon *pbpo)
{
  ULONG ulFlags = pbpo->bpo_ulFlags;
  // if polygon has no directional light
  if (!(ulFlags&(BPOF_HASDIRECTIONALLIGHT|BPOF_HASDIRECTIONALAMBIENT))) {
    // not influenced
    return FALSE;
  }

  // if has no shadows
  BOOL bIsTransparent = (ulFlags&BPOF_PORTAL) && !(ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT));
  BOOL bTakesShadow = !(ulFlags&BPOF_FULLBRIGHT);
  if (bIsTransparent || !bTakesShadow) {
    // not influenced
    return FALSE;
  }

  // influenced
  return TRUE;
}

// find all shadow maps that should have layers from this light source
void CLightSource::FindShadowLayersDirectional(BOOL bSelectedOnly)
{
  // for each layer of the light source
  {FORDELETELIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    CBrushPolygon *pbpo = itbsl->bsl_pbsmShadowMap->GetBrushPolygon();
    // if only selected polygons are checked, and this one is not selected
    if (bSelectedOnly && !(pbpo->bpo_ulFlags&BPOF_SELECTED)) {
      // skip it
      continue;
    }
    // if the polygon is influenced
    if (IsPolygonInfluencedByDirectionalLight(pbpo)) {
      // mark it
      pbpo->bpo_ulFlags |= BPOF_MARKEDLAYER;
      // update its parameters
      UpdateLayer(*itbsl);
    // if the polygon is not influenced
    } else {
      // invalidate its shadow map
      itbsl->bsl_pbsmShadowMap->Invalidate(ls_ulFlags&LSF_DYNAMIC);
      // delete the layer
      delete &*itbsl;
    }
  }}

  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(ls_penEntity->en_pwoWorld->wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for each polygon in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            CBrushPolygon *pbpo = itbpo;
            // if only selected polygons are checked, and this one is not selected
            if (bSelectedOnly && !(pbpo->bpo_ulFlags&BPOF_SELECTED)) {
              // skip it
              continue;
            }
            // if the polygon is not marked but it is influenced
            if (!(pbpo->bpo_ulFlags&BPOF_MARKEDLAYER)
              &&IsPolygonInfluencedByDirectionalLight(pbpo)) {
              // add a layer to the polygon
              AddLayer(*pbpo);
            }
          }
        }
      }
    }
  }}

  // for each layer of the light source
  {FOREACHINLIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    CBrushPolygon *pbpo = itbsl->bsl_pbsmShadowMap->GetBrushPolygon();
    // unmark the polygon
    pbpo->bpo_ulFlags &= ~BPOF_MARKEDLAYER;
  }}
}

static const FLOAT3D *_pvOrigin;
static FLOATaabbox3D _boxLight;
static BOOL  _bCastShadows;
static FLOAT _rRange;
static FLOAT _fEpsilon;
static INDEX _iDynamic; // 0=disallow, 1=maybe (depend on flag), 2=allow


static inline BOOL IsPolygonInfluencedByPointLight(CBrushPolygon *pbpo)
{
  ULONG ulFlags = pbpo->bpo_ulFlags;

  // if has no shadows
  BOOL bIsTransparent = (ulFlags&BPOF_PORTAL) && !(ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT));
  BOOL bTakesShadow  = !(ulFlags&BPOF_FULLBRIGHT);
  if( bIsTransparent || !bTakesShadow) {
    // not influenced
    return FALSE;
  }

  // if in range of light
  if( _boxLight.HasContactWith(pbpo->bpo_boxBoundingBox)) {
    // find distance of light from the plane
    const FLOAT fDistance = pbpo->bpo_pbplPlane->bpl_plAbsolute.PointDistance(*_pvOrigin);
    // if the polygon is in range, (and not behind for diffuse lights)
    if( fDistance<=_rRange && (!_bCastShadows || fDistance>_fEpsilon)) {
      // if this light is allowed on this polygon
      if( _iDynamic==2 || (!(ulFlags&BPOF_NODYNAMICLIGHTS) && _iDynamic==1)) {
        // influenced
        return TRUE;
      }
    }
  }
  // not influenced
  return FALSE;
}

#ifdef PLATFORM_WIN32
extern CEntity *_penLightUpdating = NULL;
#else
CEntity *_penLightUpdating = NULL;
#endif

void CLightSource::FindShadowLayersPoint(BOOL bSelectedOnly)
{
  // find bounding sphere and bounding box of the light influence
  _rRange = ls_rFallOff;
  _pvOrigin = &ls_penEntity->en_plPlacement.pl_PositionVector;
  _boxLight = FLOATaabbox3D(*_pvOrigin, _rRange);
  _bCastShadows = ls_ulFlags & LSF_CASTSHADOWS;
  _fEpsilon = (ls_fFarClipDistance+ls_fNearClipDistance)*1.1f;

  // determine whether this light influences polygon
  _iDynamic = 2;
  if( ls_ulFlags&LSF_NONPERSISTENT) {
    extern INDEX shd_iAllowDynamic;
    if( ((ULONG)shd_iAllowDynamic) > 2) shd_iAllowDynamic = 1L; // clamp fast
    _iDynamic = shd_iAllowDynamic;
  }

  // for each layer of the light source
  DOUBLE3D dvOrigin = FLOATtoDOUBLE(*_pvOrigin);
  {FORDELETELIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    CBrushPolygon *pbpo = itbsl->bsl_pbsmShadowMap->GetBrushPolygon();
    CEntity *penWithPolygon = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    // fixup for fast moving brush shadow recalculation
    if (_penLightUpdating!=NULL && _penLightUpdating!=penWithPolygon) {
      continue;
    }
    // if only selected polygons are checked, and this one is not selected
    if (bSelectedOnly && !(pbpo->bpo_ulFlags&BPOF_SELECTED)) {
      // skip it
      continue;
    }

    // if the polygon is influenced
    if (IsPolygonInfluencedByPointLight(pbpo)) {
      // mark it
      pbpo->bpo_ulFlags |= BPOF_MARKEDLAYER;
      // update its parameters
      UpdateLayer(*itbsl);
    // if the polygon is not influenced
    } else {
      // invalidate its shadow map
      itbsl->bsl_pbsmShadowMap->Invalidate(ls_ulFlags&LSF_DYNAMIC);
      // delete the layer
      delete &*itbsl;
    }
  }}

  // if it is a movable entity
  if (ls_penEntity->en_ulPhysicsFlags&EPF_MOVABLE) {
    CMovableEntity *pen = (CMovableEntity *)ls_penEntity;

    // for each polygon cached near the entity
    CStaticStackArray<CBrushPolygon*> &apbpo = pen->en_apbpoNearPolygons;
    for (INDEX iPolygon=0; iPolygon<apbpo.Count(); iPolygon++) {
      CBrushPolygon *pbpo = apbpo[iPolygon];
      CEntity *penWithPolygon = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
      // fixup for fast moving brush shadow recalculation
      if (_penLightUpdating!=NULL && _penLightUpdating!=penWithPolygon) {
        continue;
      }
      // if the polygon is not marked but it is influenced
      if (!(pbpo->bpo_ulFlags&BPOF_MARKEDLAYER)
        &&IsPolygonInfluencedByPointLight(pbpo)) {
        // add a layer to the polygon
        AddLayer(*pbpo);
      }
    }

  // if it is not a movable entity
  } else {
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(ls_penEntity->en_pwoWorld->wo_cenEntities, CEntity, iten) {
      // fixup for fast moving brush shadow recalculation
      if (_penLightUpdating!=NULL && _penLightUpdating!=&*iten) {
        continue;
      }
      // if it is brush entity
      if (iten->en_RenderType == CEntity::RT_BRUSH) {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
          // if the mip doesn't have contact with the light
          if (!itbm->bm_boxBoundingBox.HasContactWith(_boxLight)) {
            // skip it
            continue;
          }
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
            // if the sector doesn't have contact with the light
            if (!itbsc->bsc_boxBoundingBox.HasContactWith(_boxLight)
              ||(itbsc->bsc_bspBSPTree.bt_pbnRoot!=NULL
              &&!(itbsc->bsc_bspBSPTree.TestSphere(
                 dvOrigin, FLOATtoDOUBLE(_rRange))>=0) )) {
              // skip it
              continue;
            }
            // for each polygon in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
              // if only selected polygons are checked, and this one is not selected
              if (bSelectedOnly && !(itbpo->bpo_ulFlags&BPOF_SELECTED)) {
                // skip it
                continue;
              }
              // if the polygon is not marked but it is influenced
              if (!(itbpo->bpo_ulFlags&BPOF_MARKEDLAYER)
                &&IsPolygonInfluencedByPointLight(itbpo)) {
                // add a layer to the polygon
                AddLayer(*itbpo);
              }
            }
          }
        }
      }
    }}
  }

  // for each layer of the light source
  {FOREACHINLIST(CBrushShadowLayer, bsl_lnInLightSource, ls_lhLayers, itbsl) {
    CBrushPolygon *pbpo = itbsl->bsl_pbsmShadowMap->GetBrushPolygon();
    // unmark the polygon
    pbpo->bpo_ulFlags &= ~BPOF_MARKEDLAYER;
  }}
}

void CLightSource::FindShadowLayers(BOOL bSelectedOnly)
{
  // if the light is used for lens flares only
  if (ls_ulFlags&LSF_LENSFLAREONLY) {
    // do nothing
    return;
  }

  // use spatial classification!!!!

  // find the influenced polygons
  if (ls_ulFlags&LSF_DIRECTIONAL) {
    FindShadowLayersDirectional(bSelectedOnly);
  } else {
    FindShadowLayersPoint(bSelectedOnly);
  }
}


// Update shadow map on all terrains in world without moving
void CLightSource::UpdateTerrains(void)
{
  if(ls_penEntity==NULL) {
    return;
  }
  CPlacement3D &pl = ls_penEntity->en_plPlacement;
  UpdateTerrains(pl,pl);

}
// Update shadow map on all terrains in world
void CLightSource::UpdateTerrains(CPlacement3D plOld, CPlacement3D plNew)
{
  // if this is dynamic light
  if(ls_ulFlags&LSF_DYNAMIC) {
    // do not terrain update shadow map
    return;
  }

  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(ls_penEntity->en_pwoWorld->wo_cenEntities, CEntity, iten) {
    // if it is terrain entity
    if(iten->en_RenderType == CEntity::RT_TERRAIN) {
      CTerrain *ptrTerrain = iten->GetTerrain();
      ASSERT(ptrTerrain!=NULL);
      // Calculate bboxes of light at old position and new position
      FLOATaabbox3D bboxLightOld = FLOATaabbox3D(plOld.pl_PositionVector,ls_rFallOff);
      FLOATaabbox3D bboxLightNew = FLOATaabbox3D(plNew.pl_PositionVector,ls_rFallOff);
      FLOATaabbox3D bboxLightPos = FLOATaabbox3D(plNew.pl_PositionVector);

      if(ls_ulFlags&LSF_DIRECTIONAL) {
        ptrTerrain->UpdateShadowMap();
      } else {
        if(bboxLightPos.HasContactWith(bboxLightOld)) {
          FLOATaabbox3D bboxLightAll = bboxLightOld;
          bboxLightAll |= bboxLightNew;
          ptrTerrain->UpdateShadowMap(&bboxLightAll,TRUE);
        } else {
          // Update part of shadow map where light was before moving
          ptrTerrain->UpdateShadowMap(&bboxLightOld,TRUE);
          // Update part of shadow map where light is now
          ptrTerrain->UpdateShadowMap(&bboxLightNew,TRUE);
        }
      }
    }
  }}
}

// set properties of the light source without discarding shadows
void CLightSource::SetLightSourceWithNoDiscarding( const CLightSource &lsOriginal)
{
  // just copy all properties
  ls_ulFlags                  = lsOriginal.ls_ulFlags;
  ls_rHotSpot                 = lsOriginal.ls_rHotSpot;
  ls_rFallOff                 = lsOriginal.ls_rFallOff;
  ls_colColor                 = lsOriginal.ls_colColor   & ~0xFF;
  ls_colAmbient               = lsOriginal.ls_colAmbient & ~0xFF;
  ls_ubLightAnimationObject   = lsOriginal.ls_ubLightAnimationObject;
  ls_ubPolygonalMask          = lsOriginal.ls_ubPolygonalMask;
  ls_fNearClipDistance        = lsOriginal.ls_fNearClipDistance;
  ls_fFarClipDistance         = lsOriginal.ls_fFarClipDistance;
  ls_plftLensFlare            = lsOriginal.ls_plftLensFlare;
  ls_paoLightAnimation        = lsOriginal.ls_paoLightAnimation;
  ls_paoAmbientLightAnimation = lsOriginal.ls_paoAmbientLightAnimation;
}

// set properties of the light source and discard shadows if neccessary
void CLightSource::SetLightSource(const CLightSource &lsOriginal)
{
  // test if layers should be discarded
  BOOL bDiscardLayers =
    ls_rFallOff               !=  lsOriginal.ls_rFallOff               ||
    ls_ubPolygonalMask        !=  lsOriginal.ls_ubPolygonalMask        ||
    ls_ulFlags                !=  lsOriginal.ls_ulFlags                ||
    ls_fNearClipDistance      !=  lsOriginal.ls_fNearClipDistance      ||
    ls_fFarClipDistance       !=  lsOriginal.ls_fFarClipDistance       ;
  // test if shadows should be uncached
  BOOL bUncacheShadows = bDiscardLayers ||
    ls_rHotSpot               !=  lsOriginal.ls_rHotSpot               ||
    ls_colColor               !=  lsOriginal.ls_colColor               ||
    ls_colAmbient             !=  lsOriginal.ls_colAmbient             ||
    ls_ubLightAnimationObject !=  lsOriginal.ls_ubLightAnimationObject ;
  // discard shadows if needed
  if( bDiscardLayers) {
    DiscardShadowLayers();
  } else if( bUncacheShadows) {
    UncacheShadowMaps();
  }
  // set the light properties
  SetLightSourceWithNoDiscarding(lsOriginal);
  // find all shadow maps that should have layers from this light source if needed
  if( bDiscardLayers) {
    FindShadowLayers(FALSE);
  }

  UpdateTerrains();
}


// get color of light accounting for possible animation
COLOR CLightSource::GetLightColor(void) const 
{
  // no animation?
  if( ls_paoLightAnimation==NULL) return ls_colColor;
  // animation!
  UBYTE ubR, ubG, ubB;
  GetLightColor( ubR, ubG, ubB);
  return RGBToColor( ubR, ubG, ubB);
}

void CLightSource::GetLightColor( UBYTE &ubR, UBYTE &ubG, UBYTE &ubB) const 
{
  // no animation?
  ColorToRGB( ls_colColor, ubR, ubG, ubB);
  if( ls_paoLightAnimation==NULL) return;
  // animation!
  FLOAT fRatio;
  COLOR col0, col1;
  UBYTE ubMR, ubMG, ubMB;
  ls_paoLightAnimation->GetFrame( (SLONG&)col0, (SLONG&)col1, fRatio);
  LerpColor( col0, col1, fRatio, ubMR, ubMG, ubMB);
  ubR = ( ((((SLONG)ubR)<<8)|ubR) * ((((SLONG)ubMR)<<8)|ubMR) ) >>24;
  ubG = ( ((((SLONG)ubG)<<8)|ubG) * ((((SLONG)ubMG)<<8)|ubMG) ) >>24;
  ubB = ( ((((SLONG)ubB)<<8)|ubB) * ((((SLONG)ubMB)<<8)|ubMB) ) >>24;
}


// get ambient color of light accounting for possible animation
COLOR CLightSource::GetLightAmbient(void) const 
{
  if( ls_paoAmbientLightAnimation==NULL) return ls_colAmbient;
  UBYTE ubAR, ubAG, ubAB;
  GetLightAmbient( ubAR, ubAG, ubAB);
  return RGBToColor( ubAR, ubAG, ubAB);
}

void CLightSource::GetLightAmbient( UBYTE &ubAR, UBYTE &ubAG, UBYTE &ubAB) const 
{
  ColorToRGB( ls_colAmbient, ubAR, ubAG, ubAB);
  if( ls_paoAmbientLightAnimation==NULL) return;
  FLOAT fRatio;
  COLOR col0, col1;
  UBYTE ubMR, ubMG, ubMB;
  ls_paoAmbientLightAnimation->GetFrame( (SLONG&)col0, (SLONG&)col1, fRatio);
  LerpColor( col0, col1, fRatio, ubMR, ubMG, ubMB);
  ubAR = ( ((((SLONG)ubAR)<<8)|ubAR) * ((((SLONG)ubMR)<<8)|ubMR) ) >>24;
  ubAG = ( ((((SLONG)ubAG)<<8)|ubAG) * ((((SLONG)ubMG)<<8)|ubMG) ) >>24;
  ubAB = ( ((((SLONG)ubAB)<<8)|ubAB) * ((((SLONG)ubMB)<<8)|ubMB) ) >>24;
}
