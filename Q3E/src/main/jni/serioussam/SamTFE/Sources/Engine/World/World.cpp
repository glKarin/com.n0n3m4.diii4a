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

#include <Engine/Base/Console.h>
#include <Engine/Math/Float.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Graphics/RenderScene.h>
#include <Engine/World/WorldSettings.h>
#include <Engine/Entities/EntityClass.h>
#include <Engine/Entities/Precaching.h>
#include <Engine/Entities/EntityProperties.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Graphics/Color.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Terrain/TerrainArchive.h>
#include <Engine/Entities/InternalClasses.h>
#include <Engine/Network/Network.h>
#include <Engine/Network/SessionState.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Brushes/Brush.h>
#include <Engine/Light/LightSource.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/Selection.cpp>
#include <Engine/Terrain/Terrain.h>

#include <Engine/Templates/Stock_CEntityClass.h>

template class CDynamicContainer<CEntity>;
template class CSelection<CBrushPolygon, BPOF_SELECTED>;
template class CSelection<CBrushSector, BSCF_SELECTED>;
template class CSelection<CEntity, ENF_SELECTED>;

extern BOOL _bPortalSectorLinksPreLoaded;
extern BOOL _bEntitySectorLinksPreLoaded;
extern INDEX _ctPredictorEntities;

#if 0 // DG: unused.
// calculate ray placement from origin and target positions (obsolete?)
static inline CPlacement3D CalculateRayPlacement(
  const FLOAT3D &vOrigin, const FLOAT3D &vTarget)
{
  CPlacement3D plRay;
  // ray position is at origin
  plRay.pl_PositionVector = vOrigin;
  // calculate ray direction vector
  FLOAT3D vDirection = vTarget-vOrigin;
  // calculate ray orientation from the direction vector
  vDirection.Normalize();
  DirectionVectorToAngles(vDirection, plRay.pl_OrientationAngle);
  return plRay;
}
#endif // 0

/* Constructor. */
CTextureTransformation::CTextureTransformation(void)
{
  tt_strName = "";
}

/* Constructor. */
CTextureBlending::CTextureBlending(void)
{
  tb_strName = "";
  tb_ubBlendingType = STXF_BLEND_OPAQUE;
  tb_colMultiply = C_WHITE|0xFF;
}

/*
 * Constructor.
 */
CWorld::CWorld(void)
  : wo_pecWorldBaseClass(NULL)      // worldbase class must be obtained before using the world
  , wo_baBrushes(*new CBrushArchive)
  , wo_taTerrains(*new CTerrainArchive)
  , wo_colBackground(C_lGRAY)       // clear background color
  , wo_ulSpawnFlags(0)
  , wo_bPortalLinksUpToDate(FALSE)  // portal-sector links must be updated
{
  wo_baBrushes.ba_pwoWorld = this;
  wo_taTerrains.ta_pwoWorld = this;

  // create empty texture movements
  wo_attTextureTransformations.New(256);
  wo_atbTextureBlendings.New(256);
  wo_astSurfaceTypes.New(256);
  wo_actContentTypes.New(256);
  wo_aetEnvironmentTypes.New(256);
  wo_aitIlluminationTypes.New(256);

  // initialize collision grid
  InitCollisionGrid();

  wo_slStateDictionaryOffset = 0;
  wo_strBackdropUp = "";
  wo_strBackdropFt = "";
  wo_strBackdropRt = "";
  wo_strBackdropObject = "";
  wo_fUpW = wo_fUpL = 1.0f; wo_fUpCX = wo_fUpCZ = 0.0f;
  wo_fFtW = wo_fFtH = 1.0f; wo_fFtCX = wo_fFtCY = 0.0f;
  wo_fRtL = wo_fRtH = 1.0f; wo_fRtCZ = wo_fRtCY = 0.0f;

  wo_ulNextEntityID = 1;

  // set default placement
  wo_plFocus = CPlacement3D( FLOAT3D(3.0f, 4.0f, 10.0f),
                             ANGLE3D(AngleDeg( 20.0f), AngleDeg( -20.0f), 0));
  wo_fTargetDistance = 10.0f;

  // set default thumbnail placement
  wo_plThumbnailFocus = CPlacement3D( FLOAT3D(3.0f, 4.0f, 10.0f),
                             ANGLE3D(AngleDeg( 20.0f), AngleDeg( -20.0f), 0));
  wo_fThumbnailTargetDistance = 10.0f;
}

/*
 * Destructor.
 */
CWorld::~CWorld()
{
  // clear all arrays
  Clear();
  // destroy collision grid
  DestroyCollisionGrid();

  delete &wo_baBrushes;
  delete &wo_taTerrains;
}

/*
 * Clear all arrays.
 */
void CWorld::Clear(void)
{
  // detach worldbase class
  if (wo_pecWorldBaseClass!=NULL) {
    if ( wo_pecWorldBaseClass->ec_pdecDLLClass!=NULL
       &&wo_pecWorldBaseClass->ec_pdecDLLClass->dec_OnWorldEnd!=NULL) {
      wo_pecWorldBaseClass->ec_pdecDLLClass->dec_OnWorldEnd(this);
    }
    wo_pecWorldBaseClass=NULL;
  }

  {
    // must be in 24bit mode when managing entities
    CSetFPUPrecision FPUPrecision(FPT_24BIT);

    // clear background viewer
    SetBackgroundViewer(NULL);
    // make a new container of entities
    CDynamicContainer<CEntity> cenToDestroy = wo_cenEntities;
    // for each of the entities
    {FOREACHINDYNAMICCONTAINER(cenToDestroy, CEntity, iten) {
      // destroy it
      iten->Destroy();
    }}
    // the original container must be empty
    ASSERT(wo_cenEntities.Count()==0);
    ASSERT(wo_cenAllEntities.Count()==0);
    wo_cenEntities.Clear();
    wo_cenAllEntities.Clear();
    cenToDestroy.Clear();
    wo_ulNextEntityID = 1;
  }

  // clear brushes
  wo_baBrushes.ba_abrBrushes.Clear();
  // clear terrains
  wo_taTerrains.ta_atrTerrains.Clear();

  extern void ClearMovableEntityCaches(void);
  ClearMovableEntityCaches();

  // clear collision grid
  ClearCollisionGrid();
}

/*
 * Create a new entity of given class.
 */
CEntity *CWorld::CreateEntity(const CPlacement3D &plPlacement, CEntityClass *pecClass)
{
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);
  
  // if the world base class is not yet remembered and this class is world base
  if (wo_pecWorldBaseClass==NULL
    && stricmp(pecClass->ec_pdecDLLClass->dec_strName, "WorldBase")==0) {
    // remember it
    wo_pecWorldBaseClass = pecClass;
    // execute the class attach function
    if (pecClass->ec_pdecDLLClass->dec_OnWorldInit!=NULL) {
      pecClass->ec_pdecDLLClass->dec_OnWorldInit(this);
    }
  }
  // gather CRCs of that class
  pecClass->AddToCRCTable();

  // ask the class to instance a new member
  CEntity *penEntity = pecClass->New();
  // add the reference made by the entity itself
  penEntity->AddReference();

  // set the entity's world pointer to this world
  penEntity->en_pwoWorld = this;
  // add the new member to this world's entity container
  wo_cenEntities.Add(penEntity);
  wo_cenAllEntities.Add(penEntity);
  // set a new identifier
  penEntity->en_ulID = wo_ulNextEntityID++;
  // set up the placement
  penEntity->en_plPlacement = plPlacement;
  // calculate rotation matrix
  MakeRotationMatrixFast(penEntity->en_mRotation, penEntity->en_plPlacement.pl_OrientationAngle);

  // if now predicting
  if (_pNetwork->IsPredicting()) {
    // mark entity as a temporary predictor
    penEntity->en_ulFlags |= ENF_PREDICTOR|ENF_TEMPPREDICTOR;
    wo_cenPredictor.Add(penEntity);
    _ctPredictorEntities++;
  }

  // return it
  return penEntity;
}

/*
 * Create a new entity of given class.
 */
CEntity *CWorld::CreateEntity_t(const CPlacement3D &plPlacement,
                                const CTFileName &fnmClass) // throw char *
{
  // obtain a new entity class from global stock
  CEntityClass *pecClass = _pEntityClassStock->Obtain_t(fnmClass);
  // create entity with that class (obtains it once more)
  CEntity *penNew = CreateEntity(plPlacement, pecClass);
  // release the class
  _pEntityClassStock->Release(pecClass);
  // return the entity
  return penNew;
}

/*
 * Destroy one entities.
 */
void CWorld::DestroyOneEntity( CEntity *penToDestroy)
{
  // if the entity is targetable
  if (penToDestroy->IsTargetable()) {
    // remove all eventual pointers to it
    UntargetEntity( penToDestroy);
  }
  // destroy it
  penToDestroy->Destroy();
}

/*
 * Destroy a selection of entities.
 */
void CWorld::DestroyEntities(CEntitySelection &senToDestroy)
{
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // for each entity in selection
  FOREACHINDYNAMICCONTAINER(senToDestroy, CEntity, iten) {
    // if the entity is targetable
    if (iten->IsTargetable()) {
      // remove all eventual pointers to it
      UntargetEntity(iten);
    }
    // destroy it
    iten->Destroy();
  }
  // clear the selection on the container level
  /* NOTE: we must not clear the selection directly, since the entity objects
     contained there are already freed and deselecting them would make an access
     violation.
   */
  senToDestroy.CDynamicContainer<CEntity>::Clear();
}

/*
 * Clear all entity pointers that point to this entity.
 */
void CWorld::UntargetEntity(CEntity *penToUntarget)
{
  // for all entities in this world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, itenInWorld){
    // get the DLL class of this entity
    CDLLEntityClass *pdecDLLClass = itenInWorld->en_pecClass->ec_pdecDLLClass;

    // for all classes in hierarchy of this entity
    for(;
        pdecDLLClass!=NULL;
        pdecDLLClass = pdecDLLClass->dec_pdecBase) {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) {
        CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];

        // if the property type is entity pointer
        if (epProperty.ep_eptType == CEntityProperty::EPT_ENTITYPTR) {
          // get the pointer
          CEntityPointer &penPointed = ENTITYPROPERTY(&*itenInWorld, epProperty.ep_slOffset, CEntityPointer);
          // if it points to the entity to be untargeted
          if (penPointed == penToUntarget) {
            itenInWorld->End();
            // clear the pointer
            penPointed = NULL;
            itenInWorld->Initialize();
          }
        }
      }
    }
  }
  // if the entity is background viewer
  if (wo_penBackgroundViewer==penToUntarget) {
    // reset background viewer
    SetBackgroundViewer(NULL);
  }
}

/*
 * Find an entity with given character.
 */
CPlayerEntity *CWorld::FindEntityWithCharacter(CPlayerCharacter &pcCharacter)
{
  ASSERT(pcCharacter.pc_strName != "");

  // for each entity
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    CEntity *pen = &*iten;
    // if it is player entity
    if (IsDerivedFromClass(pen, "PlayerEntity")) {
      CPlayerEntity *penPlayer = (CPlayerEntity *)pen;
      // if it has got that character
      if (penPlayer->en_pcCharacter == pcCharacter) {
        // return its pointer
        return penPlayer;
      }
    }
  }
  // otherwise, none exists
  return NULL;
}

/*
 * Add an entity to list of thinkers.
 */
void CWorld::AddTimer(CRationalEntity *penThinker)
{
  ASSERT(penThinker->en_timeTimer>=_pTimer->CurrentTick());
  ASSERT(GetFPUPrecision()==FPT_24BIT);

  // if the entity is already in the list
  if (penThinker->en_lnInTimers.IsLinked()) {
    // remove it
    penThinker->en_lnInTimers.Remove();
  }
  // for each entity in the thinker list
  FOREACHINLISTKEEP(CRationalEntity, en_lnInTimers, wo_lhTimers, iten) {
    // if the entity in list has greater or same think time than the one to add
    if (iten->en_timeTimer>=penThinker->en_timeTimer) {
      // stop searching
      break;
    }
  }
  // add the new entity before current one
  iten.InsertBeforeCurrent(penThinker->en_lnInTimers);
}

// set overdue timers to be due in current time
void CWorld::AdjustLateTimers(TIME tmCurrentTime)
{
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // for each entity in the thinker list
  FOREACHINLIST(CRationalEntity, en_lnInTimers, wo_lhTimers, iten) {
    CRationalEntity &en = *iten;
    // if the entity in list is overdue
    if (en.en_timeTimer<tmCurrentTime) {
      // set it to current time
      en.en_timeTimer = tmCurrentTime;
    }
  }
}


/*
 * Lock all arrays.
 */
void CWorld::LockAll(void)
{
  wo_cenEntities.Lock();
  wo_cenAllEntities.Lock();
  // lock the brush archive
  wo_baBrushes.ba_abrBrushes.Lock();
  // lock the terrain archive
  wo_taTerrains.ta_atrTerrains.Lock();
}

/*
 * Unlock all arrays.
 */
void CWorld::UnlockAll(void)
{
  wo_cenEntities.Unlock();
  wo_cenAllEntities.Unlock();
  // unlock the brush archive
  wo_baBrushes.ba_abrBrushes.Unlock();
  // unlock the brush archive
  wo_taTerrains.ta_atrTerrains.Unlock();
}

/* Get background color for this world. */
COLOR CWorld::GetBackgroundColor(void)
{
  return wo_colBackground;
}

/* Set background color for this world. */
void CWorld::SetBackgroundColor(COLOR colBackground)
{
  wo_colBackground = colBackground;
}

/* Set background viewer entity for this world. */
void CWorld::SetBackgroundViewer(CEntity *penEntity)
{
  wo_penBackgroundViewer = penEntity;
}

/* Get background viewer entity for this world. */
CEntity *CWorld::GetBackgroundViewer(void)
{
  // if the background viewer entity is deleted
  if (wo_penBackgroundViewer!=NULL && wo_penBackgroundViewer->en_ulFlags&ENF_DELETED) {
    // clear the pointer
    wo_penBackgroundViewer = NULL;
  }
  return wo_penBackgroundViewer;
}

/* Set description for this world. */
void CWorld::SetDescription(const CTString &strDescription)
{
  wo_strDescription = strDescription;
}
/* Get description for this world. */
const CTString &CWorld::GetDescription(void)
{
  return wo_strDescription;
}

// get/set name of the world
void CWorld::SetName(const CTString &strName)
{
  wo_strName = strName;
}
const CTString &CWorld::GetName(void)
{
  return wo_strName;
}

// get/set spawn flags for the world
void CWorld::SetSpawnFlags(ULONG ulFlags)
{
  wo_ulSpawnFlags = ulFlags;
}
ULONG CWorld::GetSpawnFlags(void)
{
  return wo_ulSpawnFlags;
}

/////////////////////////////////////////////////////////////////////
// Shadow manipulation functions

/*
 * Recalculate all shadow maps that are not valid or of smaller precision.
 */
void CWorld::CalculateDirectionalShadows(void)
{
  extern INDEX _ctShadowLayers;
  extern INDEX _ctShadowClusters;
  CTimerValue tvStart;

  // clear shadow rendering stats
  tvStart = _pTimer->GetHighPrecisionTimer();
  _ctShadowLayers=0;
  _ctShadowClusters=0;

  // for each shadow map that is queued for calculation
  FORDELETELIST(CBrushShadowMap, bsm_lnInUncalculatedShadowMaps,
    wo_baBrushes.ba_lhUncalculatedShadowMaps, itbsm) {
    // calculate shadows on it
    itbsm->GetBrushPolygon()->MakeShadowMap(this, TRUE);
  }

  // report shadow rendering stats
  CTimerValue tvStop = _pTimer->GetHighPrecisionTimer();
  CPrintF("Shadow calculation: total %d clusters in %d layers, %fs\n",
    _ctShadowClusters,
    _ctShadowLayers,
    (tvStop-tvStart).GetSeconds());
}

void CWorld::CalculateNonDirectionalShadows(void)
{
  // for each shadow map that is queued for calculation
  FORDELETELIST(CBrushShadowMap, bsm_lnInUncalculatedShadowMaps,
    wo_baBrushes.ba_lhUncalculatedShadowMaps, itbsm) {
    // calculate shadows on it
    itbsm->GetBrushPolygon()->MakeShadowMap(this, FALSE);
  }
}


/* Find all shadow layers near a certain position. */
void CWorld::FindShadowLayers(
  const FLOATaabbox3D &boxNear,
  BOOL bSelectedOnly /*=FALSE*/,
  BOOL bDirectional /*= TRUE*/)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_FINDSHADOWLAYERS);
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is light entity and it influences the given range
    CLightSource *pls = iten->GetLightSource();
    if (pls!=NULL) {
      FLOATaabbox3D boxLight(iten->en_plPlacement.pl_PositionVector, pls->ls_rFallOff);
      if ( (bDirectional && (pls->ls_ulFlags & LSF_DIRECTIONAL))
        ||boxLight.HasContactWith(boxNear)) {
        // find layers for that light source
        pls->FindShadowLayers(bSelectedOnly);
      }
    }
  }
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_FINDSHADOWLAYERS);
}
/* Discard shadows on all brush polygons in the world. */
void CWorld::DiscardAllShadows(void)
{
  FLOATaabbox3D box;
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        box|=itbm->bm_boxBoundingBox;
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for each polygon in the sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            // discard its shadow map
            itbpo->DiscardShadows();
          }
        }
      }
    }
  }
  // find all shadow layers in the world
  FindShadowLayers(box);
}
/////////////////////////////////////////////////////////////////////
// Hide/Show functions

/*
 * Hide entities contained in given selection.
 */
void CWorld::HideSelectedEntities(CEntitySelection &selenEntitiesToHide)
{
  // for all entities in the selection
  FOREACHINDYNAMICCONTAINER(selenEntitiesToHide, CEntity, iten) {
    if( iten->IsSelected(ENF_SELECTED) &&
        !((iten->en_RenderType==CEntity::RT_BRUSH) && (iten->en_ulFlags&ENF_ZONING)) )
    {
      // hide the entity
      iten->en_ulFlags |= ENF_HIDDEN;
    }
  }
}

/*
 * Hide all unselected entities.
 */
void CWorld::HideUnselectedEntities(void)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten)
  {
    if( !iten->IsSelected(ENF_SELECTED) &&
        !((iten->en_RenderType==CEntity::RT_BRUSH)&&(iten->en_ulFlags&ENF_ZONING)) )
    {
      // hide it
      iten->en_ulFlags |= ENF_HIDDEN;
    }
  }
}

/*
 * Show all entities.
 */
void CWorld::ShowAllEntities(void)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten)
  {
    iten->en_ulFlags &= ~ENF_HIDDEN;
  }
}

/*
 * Hide sectors contained in given selection.
 */
void CWorld::HideSelectedSectors(CBrushSectorSelection &selbscSectorsToHide)
{
  // for all sectors in the selection
  FOREACHINDYNAMICCONTAINER(selbscSectorsToHide, CBrushSector, itbsc) {
    // hide the sector
    itbsc->bsc_ulFlags |= BSCF_HIDDEN;
  }
}

/*
 * Hide all unselected sectors.
 */
void CWorld::HideUnselectedSectors(void)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // if the sector is not selected
          if (!itbsc->IsSelected(BSCF_SELECTED)) {
            // hide it
            itbsc->bsc_ulFlags |= BSCF_HIDDEN;
          }
        }
      }
    }
  }
}

/*
 * Show all sectors.
 */
void CWorld::ShowAllSectors(void)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // show the sector
          itbsc->bsc_ulFlags &= ~BSCF_HIDDEN;
        }
      }
    }
  }
}

/*
 * Select all polygons in selected sectors with same texture.
 */
void CWorld::SelectByTextureInSelectedSectors(
         CTFileName fnTexture, CBrushPolygonSelection &selbpoSimilar, INDEX iTexture)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // if sector is selected
          if (itbsc->IsSelected(BSCF_SELECTED)) {
            // for all polygons in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
            {
              // if it is not portal and is not selected and has same texture
              if ( (!(itbpo->bpo_ulFlags&BPOF_PORTAL) || (itbpo->bpo_ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT))) &&
                  !itbpo->IsSelected(BPOF_SELECTED) &&
                  (itbpo->bpo_abptTextures[iTexture].bpt_toTexture.GetData() != NULL) &&
                  (itbpo->bpo_abptTextures[iTexture].bpt_toTexture.GetData()->GetName()
                  == fnTexture) )
              // select this polygon
              selbpoSimilar.Select(*itbpo);
            }
          }
        }
      }
    }
  }
}

/*
 * Select all polygons in world with same texture.
 */
void CWorld::SelectByTextureInWorld(
         CTFileName fnTexture, CBrushPolygonSelection &selbpoSimilar, INDEX iTexture)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
          {
            // if it is not non translucent portal and is not selected and has same texture
            if ( (!(itbpo->bpo_ulFlags&BPOF_PORTAL) || (itbpo->bpo_ulFlags&(BPOF_TRANSLUCENT|BPOF_TRANSPARENT))) &&
                !itbpo->IsSelected(BPOF_SELECTED) &&
                (itbpo->bpo_abptTextures[iTexture].bpt_toTexture.GetData() != NULL) &&
                (itbpo->bpo_abptTextures[iTexture].bpt_toTexture.GetData()->GetName()
                == fnTexture) )
            // select this polygon
            selbpoSimilar.Select(*itbpo);
          }
        }
      }
    }
  }
}

/*
 * Reinitialize entities from their properties. (use only in WEd!)
 */
void CWorld::ReinitializeEntities(void)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_REINITIALIZEENTITIES);

  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  CTmpPrecachingNow tpn;

  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // reinitialize it
    iten->Reinitialize();
  }

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_REINITIALIZEENTITIES);
}
/* Precache data needed by entities. */
void CWorld::PrecacheEntities_t(void)
{
  // for each entity in the world
  INDEX ctEntities = wo_cenEntities.Count();
  INDEX iEntity = 0;
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // precache
    CallProgressHook_t(FLOAT(iEntity)/ctEntities);
    iten->Precache();
    iEntity++;
  }
}
// delete all entities that don't fit given spawn flags
void CWorld::FilterEntitiesBySpawnFlags(ULONG ulFlags)
{
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  BOOL bOldAllowRandom = _pNetwork->ga_sesSessionState.ses_bAllowRandom;
  _pNetwork->ga_sesSessionState.ses_bAllowRandom = TRUE;

  // create an empty selection of entities
  CEntitySelection senToDestroy;
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if brush
    if (iten->en_RenderType==CEntity::RT_BRUSH
      ||iten->en_RenderType==CEntity::RT_FIELDBRUSH) {
      // skip it (brushes must not be deleted on the fly)
      continue;
    }

    // if it shouldn't exist
    ULONG ulEntityFlags = iten->GetSpawnFlags();
    if (!(ulEntityFlags&ulFlags&SPF_MASK_DIFFICULTY)
      ||!(ulEntityFlags&ulFlags&SPF_MASK_GAMEMODE)) {
      // add it to the selection
      senToDestroy.Select(*iten);
    }
  }}
  // destroy all selected entities
  DestroyEntities(senToDestroy);
  _pNetwork->ga_sesSessionState.ses_bAllowRandom = bOldAllowRandom;
}

// create links between zoning-brush sectors and non-zoning entities in sectors
void CWorld::LinkEntitiesToSectors(void)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_LINKENTITIESTOSECTORS);
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    CEntity &en = *iten;
    // cache eventual collision info
    en.FindCollisionInfo();
    en.UpdateSpatialRange();
    // link it
    if (!_bEntitySectorLinksPreLoaded) {
      en.FindSectorsAroundEntity();
    }
  }
  // NOTE: this is here to force relinking for all moving zoning brushes after loading!
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    CEntity &en = *iten;
    if (en.en_RenderType==CEntity::RT_BRUSH && 
      (en.en_ulFlags&ENF_ZONING) && (en.en_ulPhysicsFlags&EPF_MOVABLE)){
      // recalculate all bounding boxes relative to new position
      extern BOOL _bDontDiscardLinks;
      _bDontDiscardLinks = TRUE;
      en.en_pbrBrush->CalculateBoundingBoxes();
      _bDontDiscardLinks = FALSE;
      // FPU must be in 53-bit mode
      CSetFPUPrecision FPUPrecision(FPT_53BIT);

      // for all brush mips
      FOREACHINLIST(CBrushMip, bm_lnInBrush, en.en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in the mip
        {FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // find entities in sector
          itbsc->FindEntitiesInSector();
        }}
      }
    }
  }}
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_LINKENTITIESTOSECTORS);
}

// rebuild all links in world
void CWorld::RebuildLinks(void)
{
  wo_baBrushes.LinkPortalsAndSectors();
  _bEntitySectorLinksPreLoaded = FALSE;
  LinkEntitiesToSectors();
}

/* Update sectors during brush vertex moving */
void CWorld::UpdateSectorsDuringVertexChange( CBrushVertexSelection &selVertex)
{
  // create container of sectors that will need to be updated
  CDynamicContainer<CBrushSector> cbscToUpdate;

  {FOREACHINDYNAMICCONTAINER( selVertex, CBrushVertex, itbvx)
  {
    // add the sector of that vertex to list for updating
    if (!cbscToUpdate.IsMember(itbvx->bvx_pbscSector)) {
      cbscToUpdate.Add(itbvx->bvx_pbscSector);
    }
  }}

  // for each sector to be updated
  {FOREACHINDYNAMICCONTAINER( cbscToUpdate, CBrushSector, itbsc){
    // recalculate planes for polygons from their vertices
    itbsc->MakePlanesFromVertices();
  }}
}

/* Update sectors after brush vertex moving */
void CWorld::UpdateSectorsAfterVertexChange( CBrushVertexSelection &selVertex)
{
  // create container of sectors that will need to be updated
  CDynamicContainer<CBrushSector> cbscToUpdate;

  {FOREACHINDYNAMICCONTAINER( selVertex, CBrushVertex, itbvx)
  {
    // add the sector of that vertex to list for updating
    if (!cbscToUpdate.IsMember(itbvx->bvx_pbscSector)) {
      cbscToUpdate.Add(itbvx->bvx_pbscSector);
    }
  }}

  // for each sector to be updated
  {FOREACHINDYNAMICCONTAINER( cbscToUpdate, CBrushSector, itbsc){
    // update it
    itbsc->UpdateVertexChanges();
  }}
}

/* Triangularize polygons that contain vertices from given selection */
void CWorld::TriangularizeForVertices( CBrushVertexSelection &selVertex)
{
  // create container of sectors that contain polygons that need to be triangularized
  CDynamicContainer<CBrushSector> cbscToTriangularize;

  {FOREACHINDYNAMICCONTAINER( selVertex, CBrushVertex, itbvx)
  {
    // add the sector of that vertex to list for triangularizing
    if (!cbscToTriangularize.IsMember(itbvx->bvx_pbscSector)) {
      cbscToTriangularize.Add(itbvx->bvx_pbscSector);
    }
  }}

  // for each sector to be updated
  {FOREACHINDYNAMICCONTAINER( cbscToTriangularize, CBrushSector, itbsc){
    // update it
    itbsc->TriangularizeForVertices(selVertex);
  }}
}

// add this entity to prediction
void CEntity::AddToPrediction(void)
{
  // this function may be called even for NULLs - TODO: fix those cases
  //   (The compiler is free to assume that "this" is never NULL and optimize
  //   based on that assumption. For example, an "if (this==NULL) {...}" could
  //   be optimized away completely.)
  if(this==NULL) {return;}

  ASSERT(this!=NULL);
  // if already added
  if (en_ulFlags&ENF_WILLBEPREDICTED) 
  {
    // do nothing
    return;
  }
  // mark as added
  en_ulFlags|=ENF_WILLBEPREDICTED;
  en_pwoWorld->wo_cenWillBePredicted.Add(this);
  // add your dependents
  AddDependentsToPrediction();
}

// mark all predictable entities that will be predicted using user-set criterions
void CWorld::MarkForPrediction(void)
{
  extern INDEX cli_bPredictIfServer;
  extern INDEX cli_bPredictLocalPlayers;
  extern INDEX cli_bPredictRemotePlayers;
  extern FLOAT cli_fPredictEntitiesRange;
  static CStaticStackArray<FLOAT3D> avLocalPlayers;
  avLocalPlayers.PopAll();

  // for each player
  for (INDEX iPlayer=0; iPlayer<CEntity::GetMaxPlayers(); iPlayer++) {
    CEntity *pen = CEntity::GetPlayerEntity(iPlayer);
    // if it exists
    if (pen!=NULL) {
      // find whether it is local
      BOOL bLocal = _pNetwork->IsPlayerLocal(pen);
      // if allowed for prediction
      if (  (bLocal && cli_bPredictLocalPlayers)
        || (!bLocal && cli_bPredictRemotePlayers)) {
        // add it
        pen->AddToPrediction();
      }
      // if local
      if (bLocal) {
        // remember coordinates of the original entity, and eventual predictor coords
        avLocalPlayers.Push() = pen->GetPlacement().pl_PositionVector;
        avLocalPlayers.Push() = _pNetwork->ga_sesSessionState.GetPlayerPredictorPosition(iPlayer);
      }
    }
  }

  TIME tmNow = _pNetwork->ga_sesSessionState.ses_tmPredictionHeadTick;

  // for each predictable entity
  {FOREACHINDYNAMICCONTAINER(wo_cenPredictable, CEntity, iten){
    CEntity &en = *iten;
    // it must not be void (so that its coordinates are relevant)
    ASSERT(en.GetRenderType()!=CEntity::RT_VOID);

    // get its upper time limit for prediction
    TIME tmLimit = en.GetPredictionTime();
    // if now inside time prediction interval
    if (tmNow<tmLimit) {
      // add it to prediction
      iten->AddToPrediction();
      continue;
    }

    // if predicting entities by range
    if (cli_fPredictEntitiesRange>0) {
      FLOAT fRange = en.GetPredictionRange();
      if (fRange<=0) {
        continue;
      }
      fRange = Min(fRange, cli_fPredictEntitiesRange);

      // get its coordinates and maximal prediction range
      const FLOAT3D &v = en.GetPlacement().pl_PositionVector;
      // check if it is within range of any local player
      BOOL bInRange = FALSE;
      for(INDEX i=0; i<avLocalPlayers.Count(); i++) {
        if ((avLocalPlayers[i]-v).Length()<fRange) {
          bInRange = TRUE;
          break;
        }
      }
      // if it is within the range
      if (bInRange) {
        // add it
        iten->AddToPrediction();
      }
    }

  }}
}

// unmark all predictable entities marked for prediction
void CWorld::UnmarkForPrediction(void)
{
  // for each entity marked
  {FOREACHINDYNAMICCONTAINER(wo_cenWillBePredicted, CEntity, iten){
    // unmark for prediction
    iten->en_ulFlags&=~ENF_WILLBEPREDICTED;
  }}
  wo_cenWillBePredicted.Clear();
}

// create predictors for predictable entities that are marked for prediction
void CWorld::CreatePredictors(void)
{
  CDynamicContainer<CEntity> cenForPrediction;
  // for each entity marked
  {FOREACHINDYNAMICCONTAINER(wo_cenWillBePredicted, CEntity, iten){
    // if not deleted
    if (!(iten->en_ulFlags&ENF_DELETED)) {
      // add to container
      cenForPrediction.Add(iten);
    }
    // unmark
    iten->en_ulFlags&=~ENF_WILLBEPREDICTED;
  }}
  wo_cenWillBePredicted.Clear();
//  CPrintF("for prediction: %d\n", cenForPrediction.Count());

  // create copies of those entities as predictors
  CopyEntitiesToPredictors(cenForPrediction);
}

// delete all predictor entities
void CWorld::DeletePredictors(void)
{
  // must be in 24bit mode when managing entities
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // first remember eventual predicted player positions
  _pNetwork->ga_sesSessionState.RememberPlayerPredictorPositions();

  // make a copy of predictor container (for safe iteration)
  CDynamicContainer<CEntity> cenPredictor = wo_cenPredictor;
  // for each predictor
  {FOREACHINDYNAMICCONTAINER( cenPredictor, CEntity, iten){
    CEntity &en = *iten;
    ASSERT(en.IsPredictor());
    // destroy it
    en.Destroy();
  }}

  // for each predicted
  {FOREACHINDYNAMICCONTAINER( wo_cenPredicted, CEntity, iten){
    CEntity &en = *iten;
    ASSERT(en.IsPredicted());
    // kill its pointer to predictor
    en.SetPredictionPair(NULL);
    // mark as not predicted
    en.en_ulFlags&=~ENF_PREDICTED;
  }}

  ASSERT(_ctPredictorEntities==0);

  wo_cenPredictor.Clear();
  wo_cenPredicted.Clear();

  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    CEntity &en = *iten;
    ASSERT(!en.IsPredictor());
  }
}

// get entity by its ID
CEntity *CWorld::EntityFromID(ULONG ulID)
{
  FOREACHINDYNAMICCONTAINER(wo_cenAllEntities, CEntity, iten) {
    if (iten->en_ulID==ulID) {
      return iten;
    }
  }
  ASSERT(FALSE);
  return NULL;
}

/* Triangularize selected polygons. */
void CWorld::TriangularizePolygons(CDynamicContainer<CBrushPolygon> &dcPolygons)
{
  ClearMarkedForUseFlag();
  CDynamicContainer<CBrushSector> cbscToProcess;
  // for each polyon in selection
  FOREACHINDYNAMICCONTAINER(dcPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bp=*itbpo;
    bp.bpo_ulFlags |= BPOF_MARKED_FOR_USE;
    CBrushSector *pbsc=bp.bpo_pbscSector;
    if( !cbscToProcess.IsMember( pbsc))
    {
      cbscToProcess.Add( pbsc);
    }
  }

  FOREACHINDYNAMICCONTAINER(cbscToProcess, CBrushSector, itbsc)
  {
    itbsc->TriangularizeMarkedPolygons();
    itbsc->UpdateVertexChanges();
  }
}

// Clear marked for use flag on all polygons in world
void CWorld::ClearMarkedForUseFlag(void)
{
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for each polygon in the sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            // discard marked for use flag
            itbpo->bpo_ulFlags &= ~BPOF_MARKED_FOR_USE;
          }
        }
      }
    }
  }
}
