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


// for generating unique IDs for lens flares
// (32 bit should be enough for generating 1 lens flare per second during approx. 136 years)
static INDEX _iNextLensFlareID = 1; // 0 is reserved for no-id

extern void SelectEntityOnRender( CProjection3D &prProjection, CEntity &en);


// creates oriented bounding box for a model
inline void CreateModelOBBox( CEntity *penModel, FLOAT3D &vHandle, FLOATmatrix3D &mAbsToView, FLOATobbox3D &obbox)
{
  FLOAT3D vAntiStretch = penModel->GetClassificationBoxStretch();
  vAntiStretch(1) = 1.0f/vAntiStretch(1);
  vAntiStretch(2) = 1.0f/vAntiStretch(2);
  vAntiStretch(3) = 1.0f/vAntiStretch(3);
  FLOATaabbox3D boxModel = penModel->en_boxSpatialClassification;
  boxModel.StretchByVector( vAntiStretch);
  obbox = FLOATobbox3D( boxModel, vHandle, mAbsToView*penModel->en_mRotation);
}


/*
 * Add a model entity to rendering.
 */
void CRenderer::AddModelEntity(CEntity *penModel)
{
  // if the entity is currently active or hidden, don't add it again
  if( penModel->en_ulFlags&(ENF_INRENDERING|ENF_HIDDEN)) return;
  // skip the entity if predicted, and predicted entities should not be rendered
  if( penModel->IsPredicted() && !gfx_bRenderPredicted) return;
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDMODELENTITY);

  // get its model object
  CModelObject *pmoModelObject;
  if( penModel->en_RenderType!=CEntity::RT_BRUSH &&
    penModel->en_RenderType != CEntity::RT_FIELDBRUSH) {
    pmoModelObject = penModel->GetModelForRendering();
  } else {
    // empty brushes are also rendered as models
    pmoModelObject = _wrpWorldRenderPrefs.wrp_pmoEmptyBrush;
  }

  // mark the entity as active in rendering
  penModel->en_ulFlags |= ENF_INRENDERING;

  // add it to container of all drawn entities
  re_cenDrawn.Add(penModel);
  // add it to a container for delayed rendering
  CDelayedModel &dm = re_admDelayedModels.Push();
  dm.dm_penModel = penModel;
  dm.dm_pmoModel = pmoModelObject;
  dm.dm_ulFlags  = NONE; // invisible until proved otherwise

  // get proper projection for the entity
  CProjection3D *pprProjection;
  if (re_bBackgroundEnabled && (penModel->en_ulFlags & ENF_BACKGROUND)) {
    pprProjection = re_prBackgroundProjection;
  } else {
    pprProjection = re_prProjection;
  }

  // transform its handle to view space
  FLOAT3D vHandle;
  pprProjection->PreClip(penModel->en_plPlacement.pl_PositionVector, vHandle);
  // setup mip factor
  FLOAT fDistance  = vHandle(3)+penModel->GetDepthSortOffset();

  FLOAT fMipFactor = pprProjection->MipFactor(fDistance);

  penModel->AdjustMipFactor(fMipFactor);
  dm.dm_fDistance  = fDistance;
  dm.dm_fMipFactor = fMipFactor;

  FLOAT fR = penModel->en_fSpatialClassificationRadius;
  if( penModel->en_RenderType==CEntity::RT_BRUSH
   || penModel->en_RenderType==CEntity::RT_FIELDBRUSH) {
    fR = 1.0f;
  }
  ASSERT(fR>0.0f);

  // test object sphere to frustum
  FLOATobbox3D boxEntity;
  BOOL  bModelHasBox = FALSE;
  INDEX iFrustumTest = pprProjection->TestSphereToFrustum( vHandle, fR);
  // if test is indeterminate
  if( iFrustumTest==0) {
    // create oriented box and test it to frustum
    CreateModelOBBox( penModel, vHandle, pprProjection->pr_ViewerRotationMatrix, boxEntity);
    bModelHasBox = TRUE; // mark that box has been created
    iFrustumTest = pprProjection->TestBoxToFrustum(boxEntity);
  }
  // if not inside 
  if( iFrustumTest<0) {
    // don't add it
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // do additional check for eventual mirror plane (if allowed)
  INDEX iMirrorPlaneTest = -1;
  extern INDEX gap_iOptimizeClipping;
  if( gap_iOptimizeClipping>0 && (pprProjection->pr_bMirror || pprProjection->pr_bWarp)) {
    // test sphere against plane
    const FLOAT fPlaneDistance = pprProjection->pr_plMirrorView.PointDistance(vHandle);
         if( fPlaneDistance < -fR) iMirrorPlaneTest = -1;
    else if( fPlaneDistance > +fR) iMirrorPlaneTest = +1;
    else { // if test is indeterminate
      // create box entity if needed
      if( !bModelHasBox) {
        CreateModelOBBox( penModel, vHandle, pprProjection->pr_ViewerRotationMatrix, boxEntity);
        bModelHasBox = TRUE;
      } // test it to mirror/warp plane
      iMirrorPlaneTest = (INDEX) (boxEntity.TestAgainstPlane(pprProjection->pr_plMirrorView));
    }
    // if not in mirror
    if( iMirrorPlaneTest<0) {
      // don't add it
      _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
      return;
    }
  } 

  // if it has a light source with a lens flare and we're not rendering shadows
  CLightSource *pls = penModel->GetLightSource();
  if( !re_bRenderingShadows && pls!=NULL && pls->ls_plftLensFlare!=NULL) {
    // add the lens flare to rendering
    AddLensFlare( penModel, pls, pprProjection, re_iIndex);
  }

  // adjust model flags
  if( pmoModelObject->HasAlpha()) dm.dm_ulFlags |= DMF_HASALPHA;
  if( re_bCurrentSectorHasFog)    dm.dm_ulFlags |= DMF_FOG;
  if( re_bCurrentSectorHasHaze)   dm.dm_ulFlags |= DMF_HAZE;
  if( iFrustumTest>0)             dm.dm_ulFlags |= DMF_INSIDE;    // mark the need for clipping
  if( iMirrorPlaneTest>0)         dm.dm_ulFlags |= DMF_INMIRROR;  // mark the need for clipping to mirror/warp plane

  // if this is an editor model and editor models are disabled
  if( penModel->en_RenderType==CEntity::RT_EDITORMODEL
   && !_wrpWorldRenderPrefs.IsEditorModelsOn()) {
    // don't render it
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // safety check
  if( pmoModelObject==NULL) {
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // if the object is not visible at its distance
  if( (!re_bRenderingShadows && !pmoModelObject->IsModelVisible(fMipFactor))) {
    // don't add it
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // if entity selecting by laso requested, test for selecting
  if( _pselenSelectOnRender != NULL) SelectEntityOnRender( *pprProjection, *penModel);
  
  // allow its rendering
  dm.dm_ulFlags |= DMF_VISIBLE;
  ASSERT(pmoModelObject!=NULL);
  // if rendering shadows use only first mip level
  if( re_bRenderingShadows) dm.dm_fMipFactor = 0;

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
}
/*
 * Add a ska model entity to rendering.
 */
void CRenderer::AddSkaModelEntity(CEntity *penModel)
{
  // if the entity is currently active or hidden, don't add it again
  if( penModel->en_ulFlags&(ENF_INRENDERING|ENF_HIDDEN)) return;
  // skip the entity if predicted, and predicted entities should not be rendered
  if( penModel->IsPredicted() && !gfx_bRenderPredicted) return;
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDMODELENTITY);

  // get its model object
  CModelInstance *pmiModelInstance = penModel->GetModelInstance();

  // mark the entity as active in rendering
  penModel->en_ulFlags |= ENF_INRENDERING;

  // add it to container of all drawn entities
  re_cenDrawn.Add(penModel);
  // add it to a container for delayed rendering
  CDelayedModel &dm = re_admDelayedModels.Push();
  dm.dm_penModel = penModel;
  dm.dm_ulFlags  = NONE; // invisible until proved otherwise
  // dm.dm_pmoModel = pmoModelObject;

  // get proper projection for the entity
  CProjection3D *pprProjection;
  if (re_bBackgroundEnabled && (penModel->en_ulFlags & ENF_BACKGROUND)) {
    pprProjection = re_prBackgroundProjection;
  } else {
    pprProjection = re_prProjection;
  }

  // transform its handle to view space
  FLOAT3D vHandle;
  pprProjection->PreClip(penModel->en_plPlacement.pl_PositionVector, vHandle);
  // setup mip factor
  FLOAT fDistance  = vHandle(3)+penModel->GetDepthSortOffset();

  FLOAT fMipFactor = pprProjection->MipFactor(fDistance);
  fMipFactor = -fDistance;

  penModel->AdjustMipFactor(fMipFactor);
  dm.dm_fDistance  = fDistance;
  dm.dm_fMipFactor = fMipFactor;

  FLOAT fR = penModel->en_fSpatialClassificationRadius;
  ASSERT(fR>0.0f);

  // test object sphere to frustum
  FLOATobbox3D boxEntity;
  BOOL  bModelHasBox = FALSE;
  INDEX iFrustumTest = pprProjection->TestSphereToFrustum( vHandle, fR);
  // if test is indeterminate
  if( iFrustumTest==0) {
    // create oriented box and test it to frustum
    CreateModelOBBox( penModel, vHandle, pprProjection->pr_ViewerRotationMatrix, boxEntity);
    bModelHasBox = TRUE; // mark that box has been created
    iFrustumTest = pprProjection->TestBoxToFrustum(boxEntity);
  }
  // if not inside 
  if( iFrustumTest<0) {
    // don't add it
    // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // do additional check for eventual mirror plane (if allowed)
  INDEX iMirrorPlaneTest = -1;
  extern INDEX gap_iOptimizeClipping;
  if( gap_iOptimizeClipping>0 && (pprProjection->pr_bMirror || pprProjection->pr_bWarp)) {
    // test sphere against plane
    const FLOAT fPlaneDistance = pprProjection->pr_plMirrorView.PointDistance(vHandle);
         if( fPlaneDistance < -fR) iMirrorPlaneTest = -1;
    else if( fPlaneDistance > +fR) iMirrorPlaneTest = +1;
    else { // if test is indeterminate
      // create box entity if needed
      if( !bModelHasBox) {
        CreateModelOBBox( penModel, vHandle, pprProjection->pr_ViewerRotationMatrix, boxEntity);
        bModelHasBox = TRUE;
      } // test it to mirror/warp plane
      iMirrorPlaneTest = (INDEX) boxEntity.TestAgainstPlane(pprProjection->pr_plMirrorView);
    }
    // if not in mirror
    if( iMirrorPlaneTest<0) {
      // don't add it
      // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
      return;
    }
  } 

  // if it has a light source with a lens flare and we're not rendering shadows
  CLightSource *pls = penModel->GetLightSource();
  if( !re_bRenderingShadows && pls!=NULL && pls->ls_plftLensFlare!=NULL) {
    // add the lens flare to rendering
    AddLensFlare( penModel, pls, pprProjection, re_iIndex);
  }

  // adjust model flags
  if( pmiModelInstance->HasAlpha()) dm.dm_ulFlags |= DMF_HASALPHA;
  if( re_bCurrentSectorHasFog)      dm.dm_ulFlags |= DMF_FOG;
  if( re_bCurrentSectorHasHaze)     dm.dm_ulFlags |= DMF_HAZE;
  if( iFrustumTest>0)               dm.dm_ulFlags |= DMF_INSIDE;    // mark the need for clipping
  if( iMirrorPlaneTest>0)           dm.dm_ulFlags |= DMF_INMIRROR;  // mark the need for clipping to mirror/warp plane

  // if this is an editor model and editor models are disabled
  if( penModel->en_RenderType==CEntity::RT_SKAEDITORMODEL
   && !_wrpWorldRenderPrefs.IsEditorModelsOn()) {
    // don't render it
    // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // safety check
  if( pmiModelInstance==NULL) {
    // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // if the object is not visible at its distance
  if( (!re_bRenderingShadows && !pmiModelInstance->IsModelVisible(fMipFactor))) {
    // don't add it
    // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
    return;
  }

  // if entity selecting by laso requested, test for selecting
  if( _pselenSelectOnRender != NULL) SelectEntityOnRender( *pprProjection, *penModel);
  
  // allow its rendering
  dm.dm_ulFlags |= DMF_VISIBLE;
  // if rendering shadows use only first mip level
  if( re_bRenderingShadows) dm.dm_fMipFactor = 0;

  // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDMODELENTITY);
}


/* Add a lens flare to rendering. */
void CRenderer::AddLensFlare( CEntity *penLight, CLightSource *pls, CProjection3D *pprProjection, INDEX iMirrorLevel/*=0*/)
{
  // if flares are off
  if (_wrpWorldRenderPrefs.wrp_lftLensFlares == CWorldRenderPrefs::LFT_NONE) {
    // don't add any new flares
    return;
  }

  // project the light source position to view space
  FLOAT3D vRotated;
  pprProjection->PreClip(penLight->GetLerpedPlacement().pl_PositionVector, vRotated);
  // if it is behind near clip plane
  if (-vRotated(3)<pprProjection->NearClipDistanceR()) {
    // skip it
    return;
  }
  // project it to screen
  FLOAT3D vScreen;
  pprProjection->PostClip(vRotated, vScreen);

  ASSERT( re_pdpDrawPort!=NULL);
  const ULONG ulDrawPortID = re_pdpDrawPort->GetID();

  // for each existing lens flare
  CLensFlareInfo *plfi = NULL;
  const INDEX ctFlares = re_alfiLensFlares.Count();
  {for(INDEX iFlare=0; iFlare<ctFlares; iFlare++) {
    CLensFlareInfo &lfiOld = re_alfiLensFlares[iFlare];
    // if it is this one
    if( lfiOld.lfi_plsLightSource==pls
    && (lfiOld.lfi_ulDrawPortID==ulDrawPortID || lfiOld.lfi_iMirrorLevel>0)) {
      // use it
      plfi = &lfiOld;
      break;
    }
  }}
  // if it is not found
  if (plfi==NULL) {
    // create a new one
    plfi = &re_alfiLensFlares.Push();
    plfi->lfi_iID = _iNextLensFlareID++;
    plfi->lfi_tmLastFrame  = _pTimer->GetRealTimeTick()-0.05f;
    plfi->lfi_iMirrorLevel = iMirrorLevel;
    plfi->lfi_fFadeFactor  = 0.0f;
    plfi->lfi_ulFlags = NONE;
  }

  CLensFlareInfo &lfi = *plfi;
  lfi.lfi_ulDrawPortID = ulDrawPortID;
  lfi.lfi_plsLightSource = pls;
  lfi.lfi_fI = vScreen(1);
  lfi.lfi_fJ = vScreen(2);
  lfi.lfi_fDistance = -vRotated(3);
  lfi.lfi_fOoK = (1-pprProjection->pr_fDepthBufferFactor/vRotated(3))
               * pprProjection->pr_fDepthBufferMul+pprProjection->pr_fDepthBufferAdd;
  lfi.lfi_vProjected = vRotated;
  lfi.lfi_ulFlags |=  LFF_ACTIVE;
  lfi.lfi_ulFlags &= ~LFF_FOG;
  lfi.lfi_ulFlags &= ~LFF_HAZE;
  if( re_bCurrentSectorHasFog)  lfi.lfi_ulFlags |= LFF_FOG;
  if( re_bCurrentSectorHasHaze) lfi.lfi_ulFlags |= LFF_HAZE;
}



/* Add a moving brush entity to rendering list (add all sectors immediately). */
void CRenderer::AddNonZoningBrush( CEntity *penBrush, CBrushSector *pbscThatAdds)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
  ASSERT( penBrush!=NULL);
  // get its brush
  CBrush3D &brBrush = *penBrush->en_pbrBrush;

  // if hidden
  if( penBrush->en_ulFlags&ENF_HIDDEN) {
    // skip it
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
    return;
  }

  // if the brush is already added
  if( brBrush.br_lnInActiveBrushes.IsLinked()) {
    // skip it
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
    return;
  }

  const BOOL bDisableVisTweaks = _wrpWorldRenderPrefs.wrp_bDisableVisTweaks;
  // if visibility tweaking is enabled
  if( !bDisableVisTweaks)
  {
    // if vistweaks exclude this brush from rendering in this position
    ULONG ulVisTweaks = penBrush->GetVisTweaks();
    if ((pbscThatAdds!=NULL && (VISM_DONTCLASSIFY&pbscThatAdds->bsc_ulVisFlags&ulVisTweaks))
      ||(ulVisTweaks&re_ulVisExclude&VISM_INCLUDEEXCLUDE)
      ||(re_ulVisInclude!=0 && !(ulVisTweaks&re_ulVisInclude&VISM_INCLUDEEXCLUDE) ) ) {
      // skip it
      _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
      return;
    }
  }


  // skip whole non-zoning brush if invisible for rendering and not in wireframe mode
  const BOOL bWireFrame = _wrpWorldRenderPrefs.wrp_ftEdges != CWorldRenderPrefs::FT_NONE
                       || _wrpWorldRenderPrefs.wrp_ftVertices != CWorldRenderPrefs::FT_NONE;
  if( !(penBrush->en_ulFlags&ENF_ZONING) && !bWireFrame)
  { // test every brush polygon for it's visibility flag
    // for every sector in brush
    FOREACHINDYNAMICARRAY( brBrush.GetFirstMip()->bm_abscSectors, CBrushSector, itbsc) {
      // for all polygons in sector
      FOREACHINSTATICARRAY( itbsc->bsc_abpoPolygons, CBrushPolygon, itpo) {
        // advance to next polygon if invisible
        CBrushPolygon &bpo = *itpo;
        if( !(bpo.bpo_ulFlags&BPOF_INVISIBLE)) goto addBrush;
      }
    }
    // skip this brush
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
    return;
  }

addBrush:
  // prepare the brush entity for rendering if not yet prepared
  PrepareBrush(brBrush.br_penEntity);

  // get relevant mip factor for that brush and current rendering prefs
  CBrushMip *pbm = brBrush.GetBrushMipByDistance(
    _wrpWorldRenderPrefs.GetCurrentMipBrushingFactor(brBrush.br_prProjection->MipFactor()));
  // if brush mip exists for that mip factor
  if (pbm!=NULL) {
    // if entity selecting by laso requested
    if( _pselenSelectOnRender != NULL)
    {
      // test for selecting
      SelectEntityOnRender( *re_prProjection, *penBrush);
    }
    // for each sector
    FOREACHINDYNAMICARRAY(pbm->bm_abscSectors, CBrushSector, itbsc) {
      // if the sector is not hidden
      if(!((itbsc->bsc_ulFlags&BSCF_HIDDEN) && !re_bRenderingShadows)) {
        // add that sector to active sectors
        AddActiveSector(itbsc.Current());
      }
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
}

/* Add a terrain entity to rendering list. */
void CRenderer::AddTerrainEntity(CEntity *penTerrain)
{
  // _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
  ASSERT( penTerrain!=NULL);
  // get its terrain
  CTerrain &trTerrain = *penTerrain->GetTerrain();

  // if hidden
  if(penTerrain->en_ulFlags&ENF_HIDDEN) {
    // skip it
    // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
    return;
  }

  if(re_bCurrentSectorHasFog) {
    trTerrain.AddFlag(TR_HAS_FOG);
  }
  if(re_bCurrentSectorHasHaze) {
    trTerrain.AddFlag(TR_HAS_HAZE);
  }
  // if the brush is already added
  if( trTerrain.tr_lnInActiveTerrains.IsLinked()) {
    // skip it
    // _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDNONZONINGBRUSH);
    return;
  }

  // add it to list of active terrains
  re_lhActiveTerrains.AddTail(trTerrain.tr_lnInActiveTerrains);
  // add it to container of all drawn entities
  re_cenDrawn.Add(penTerrain);
}

/* Add to rendering all entities in the world (used in special cases in world editor). */
void CRenderer::AddAllEntities(void)
{
  // for all entities in world
  FOREACHINDYNAMICCONTAINER(re_pwoWorld->wo_cenEntities, CEntity, iten) {
    // if it is brush
    if (iten->en_RenderType==CEntity::RT_BRUSH
     ||(iten->en_RenderType==CEntity::RT_FIELDBRUSH 
        && _wrpWorldRenderPrefs.IsFieldBrushesOn())) {
      // add all of its sectors
      AddNonZoningBrush(&iten.Current(), NULL);

    // if it is model, or editor model that should be drawn
    } else if (iten->en_RenderType==CEntity::RT_MODEL
            ||(iten->en_RenderType==CEntity::RT_EDITORMODEL 
               && _wrpWorldRenderPrefs.IsEditorModelsOn())) {
      // add it as a model
      AddModelEntity(&iten.Current());
    } else if (iten->en_RenderType==CEntity::RT_SKAMODEL
            ||(iten->en_RenderType==CEntity::RT_SKAEDITORMODEL 
               && _wrpWorldRenderPrefs.IsEditorModelsOn())) {
      AddSkaModelEntity(&iten.Current());
    // if this is terrain
    } else if(iten->en_RenderType==CEntity::RT_TERRAIN) {
      // add it as a terrain
      AddTerrainEntity(&iten.Current());
    }
  }
}

/* Add to rendering all entities that are inside an zoning brush sector. */
void CRenderer::AddEntitiesInSector(CBrushSector *pbscSectorInside)
{
  // if we don't have a relevant sector to test with 
  if (pbscSectorInside==NULL || pbscSectorInside->bsc_bspBSPTree.bt_pbnRoot==NULL) {
    // do nothing
    return;
  }

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDENTITIESINSECTOR);

  // for all entities in the sector
  {FOREACHDSTOFSRC(pbscSectorInside->bsc_rsEntities, CEntity, en_rdSectors, pen)
     // if it is brush
    if (pen->en_RenderType==CEntity::RT_BRUSH
     ||(pen->en_RenderType==CEntity::RT_FIELDBRUSH 
        && _wrpWorldRenderPrefs.IsFieldBrushesOn())) {
      // add all of its sectors
      AddNonZoningBrush(pen, pbscSectorInside);

    // if it is model, or editor model that should be drawn
    } else if (pen->en_RenderType==CEntity::RT_MODEL||pen->en_RenderType==CEntity::RT_EDITORMODEL) {
      // add it as a model
      AddModelEntity(pen);
    // if it is a ska model, or editor model that should be drawn
    } else if(pen->en_RenderType==CEntity::RT_SKAMODEL||pen->en_RenderType==CEntity::RT_SKAEDITORMODEL) {
      AddSkaModelEntity(pen);
    // if it is a terrain
    } else if(pen->en_RenderType==CEntity::RT_TERRAIN) {
      AddTerrainEntity(pen);
    }
  ENDFOR}
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDENTITIESINSECTOR);
};

/* Add to rendering all entities that are inside a given box. */
void CRenderer::AddEntitiesInBox(const FLOATaabbox3D &boxNear)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDENTITIESINBOX);

  // for all entities in world
  FOREACHINDYNAMICCONTAINER(re_pwoWorld->wo_cenEntities, CEntity, iten) {
    // if it is brush
    if (iten->en_RenderType==CEntity::RT_BRUSH
     ||(iten->en_RenderType==CEntity::RT_FIELDBRUSH 
        && _wrpWorldRenderPrefs.IsFieldBrushesOn())) {
      // if it is zoning
      if (iten->en_ulFlags&ENF_ZONING) {
        // skip it
        continue;
      }

      // if the first mip of the brush has contact with the box
      CEntity *penBrush = iten;
      CBrushMip *pbmFirst = penBrush->en_pbrBrush->GetFirstMip();
      if (pbmFirst->bm_boxBoundingBox.HasContactWith(boxNear)) {
        // add all of its sectors
        AddNonZoningBrush(&iten.Current(), NULL);
      }

    // if it is model, or editor model that should be drawn
    } else if (iten->en_RenderType==CEntity::RT_MODEL
            ||iten->en_RenderType==CEntity::RT_EDITORMODEL) {
      // get model's bounding box for current frame
      FLOATaabbox3D boxModel;
      iten->en_pmoModelObject->GetCurrentFrameBBox(boxModel);
      // get center and radius of the bounding sphere
      FLOAT fSphereRadius = Max( boxModel.Min().Length(), boxModel.Max().Length() );
      FLOAT3D vSphereCenter = iten->en_plPlacement.pl_PositionVector;
      // create maximum box for the model (in cases of any rotation) from the sphere
      FLOATaabbox3D boxMaxModel(vSphereCenter, fSphereRadius);
      // if the model box is near the given box
      if (boxMaxModel.HasContactWith(boxNear)) {
        // add it as a model
        AddModelEntity(&iten.Current());
      }
    } else if( iten->en_RenderType==CEntity::RT_SKAMODEL
      || iten->en_RenderType==CEntity::RT_SKAEDITORMODEL) {
      // get model's bounding box for current frame
      FLOATaabbox3D boxModel;
      iten->GetModelInstance()->GetCurrentColisionBox(boxModel);
      // get center and radius of the bounding sphere
      FLOAT fSphereRadius = Max( boxModel.Min().Length(), boxModel.Max().Length() );
      FLOAT3D vSphereCenter = iten->en_plPlacement.pl_PositionVector;
      // create maximum box for the model (in cases of any rotation) from the sphere
      FLOATaabbox3D boxMaxModel(vSphereCenter, fSphereRadius);
      // if the model box is near the given box
      AddSkaModelEntity(&iten.Current());
    // if this is terrain entity
    } else if( iten->en_RenderType==CEntity::RT_TERRAIN) {
      // get model's bounding box for current frame
      //#pragma message(">> Is terrain visible")
      FLOATaabbox3D boxTerrain;
      iten->GetTerrain()->GetAllTerrainBBox(boxTerrain);
      // get center and radius of the bounding sphere
      FLOAT fSphereRadius = Max( boxTerrain.Min().Length(), boxTerrain.Max().Length() );
      FLOAT3D vSphereCenter = iten->en_plPlacement.pl_PositionVector;
      // create maximum box for the model (in cases of any rotation) from the sphere
      FLOATaabbox3D boxMaxTerrain(vSphereCenter, fSphereRadius);
      // if the model box is near the given box
      AddTerrainEntity(&iten.Current());
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDENTITIESINBOX);
};

// update VisTweak flags with given zoning sector
inline void CRenderer::UpdateVisTweaks(CBrushSector *pbsc)
{
  // if not showing vis tweaks
  if (!(_wrpWorldRenderPrefs.wrp_bShowVisTweaksOn && _pselbscVisTweaks!=NULL)) {
    // add tweaks for this sector
    if (pbsc->bsc_ulFlags2&BSCF2_VISIBILITYINCLUDE) {
      re_ulVisInclude = pbsc->bsc_ulVisFlags&VISM_INCLUDEEXCLUDE;
    } else {
      re_ulVisExclude |= pbsc->bsc_ulVisFlags&VISM_INCLUDEEXCLUDE;
    }
  }
}

/* Add to rendering all zoning brush sectors that an entity is in. */
void CRenderer::AddZoningSectorsAroundEntity(CEntity *pen, const FLOAT3D &vEyesPos)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDZONINGSECTORS);

  // works only for non-zoning entities
  ASSERT(!(pen->en_ulFlags&ENF_ZONING));

  // make parameters for minimum sphere to add
  re_vdViewSphere = FLOATtoDOUBLE(vEyesPos);
  re_dViewSphereR = re_prProjection->NearClipDistanceR()*1.5f;

  CListHead lhToAdd;
  // for all sectors this entity is in
  {FOREACHSRCOFDST(pen->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
    // if the sector is not active
    if (!pbsc->bsc_lnInActiveSectors.IsLinked()) {
      // add to list of sectors to add
      lhToAdd.AddTail(pbsc->bsc_lnInActiveSectors);
    }
  ENDFOR}

  // for each active sector
  while (!lhToAdd.IsEmpty()) {
    CBrushSector *pbsc = LIST_HEAD(lhToAdd, CBrushSector, bsc_lnInActiveSectors);
    // remove it from list of sectors to add
    pbsc->bsc_lnInActiveSectors.Remove();
    // add it to final list
    AddGivenZoningSector(pbsc);
    // if isn't really added (wrong mip)
    if (!pbsc->bsc_lnInActiveSectors.IsLinked()) {
      // skip it
      continue;
    }

    // for each portal in the sector
    FOREACHINSTATICARRAY(pbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
      CBrushPolygon *pbpo = itbpo;                      
      if (!(pbpo->bpo_ulFlags&BPOF_PORTAL)) {
        continue;
      }
      // for each sector related to the portal
      {FOREACHDSTOFSRC(pbpo->bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbscRelated)
        // if the sector is not active
        if (!pbscRelated->bsc_lnInActiveSectors.IsLinked()) {
          // if the view sphere is in the sector
          if (pbscRelated->bsc_bspBSPTree.TestSphere(
              re_vdViewSphere, re_dViewSphereR)>=0) {
            // add it to list to add
            lhToAdd.AddTail(pbscRelated->bsc_lnInActiveSectors);
          }
        }
      ENDFOR}
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDZONINGSECTORS);
}

/* Add to rendering one particular zoning brush sector. */
void CRenderer::AddGivenZoningSector(CBrushSector *pbsc)
{
  // get the sector's brush mip, brush and entity
  CBrushMip *pbmBrushMip = pbsc->bsc_pbmBrushMip;
  CBrush3D *pbrBrush = pbmBrushMip->bm_pbrBrush;
  ASSERT(pbrBrush!=NULL);
  CEntity *penBrush = pbrBrush->br_penEntity;
  ASSERT(penBrush!=NULL);

  // if the brush is field brush
  if (penBrush->en_RenderType==CEntity::RT_FIELDBRUSH) {
    // skip it
    return;
  }

  // prepare the brush entity for rendering if not yet prepared
  PrepareBrush(penBrush); 
  penBrush->en_pbrBrush->br_ulFlags|=BRF_DRAWFIRSTMIP;

  // here, get only the first brush mip
  CBrushMip *pbmRelevant = pbrBrush->GetFirstMip();
  // if it is the one of that sector
  if (pbmRelevant==pbmBrushMip) {
    // add that sector to active sectors
    AddActiveSector(*pbsc);
    UpdateVisTweaks(pbsc);
  }
}

/* Add to rendering all zoning brush sectors near a given box in absolute space. */
void CRenderer::AddZoningSectorsAroundBox(const FLOATaabbox3D &boxNear)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDZONINGSECTORS);

  // get center and radius of the bounding sphere
  FLOAT fSphereRadius = boxNear.Size().Length()/2.0f;
  FLOAT3D vSphereCenter = boxNear.Center();

  re_dViewSphereR = re_prProjection->NearClipDistanceR()*1.5f;
  re_vdViewSphere = FLOATtoDOUBLE(vSphereCenter);

  // for all entities in world
  FOREACHINDYNAMICCONTAINER(re_pwoWorld->wo_cenEntities, CEntity, iten) {
    // if the brush is field brush, and field brushes are not rendered
    if (iten->en_RenderType==CEntity::RT_FIELDBRUSH 
        && !_wrpWorldRenderPrefs.IsFieldBrushesOn()) {
      // skip it
      continue;
    }
    // if it is not zoning brush
    if ((iten->en_RenderType!=CEntity::RT_BRUSH && iten->en_RenderType!=CEntity::RT_FIELDBRUSH)
      ||!(iten->en_ulFlags&ENF_ZONING)) {
      // skip it
      continue;
    }

    CEntity *penBrush = &*iten;
    // get its brush
    CBrush3D &brBrush = *penBrush->en_pbrBrush;

    /* !!!! this always prepares all brushes in world,
    should be moved inside loop, but the mip factor must be calculated
    in some other ways.
    */
    // prepare the brush entity for rendering if not yet prepared
    PrepareBrush(penBrush); 

    // get relevant mip factor for that brush and current rendering prefs
    CBrushMip *pbm = brBrush.GetBrushMipByDistance(
      _wrpWorldRenderPrefs.GetCurrentMipBrushingFactor(brBrush.br_prProjection->MipFactor()));
    // if brush mip exists for that mip factor
    if (pbm!=NULL) {
      // for each sector
      FOREACHINDYNAMICARRAY(pbm->bm_abscSectors, CBrushSector, itbsc) {
        // if the sector's bounding box has contact with given bounding box, 
        // and the sector is not hidden
        if(itbsc->bsc_boxBoundingBox.HasContactWith(boxNear) 
         &&!((itbsc->bsc_ulFlags&BSCF_HIDDEN) && !re_bRenderingShadows)) {
          // if the sphere is inside the sector
          if (itbsc->bsc_bspBSPTree.TestSphere(
			  FLOATtoDOUBLE(vSphereCenter), FLOATtoDOUBLE(fSphereRadius)) >= 0) {

            // add that sector to active sectors
            AddActiveSector(itbsc.Current());
            UpdateVisTweaks(itbsc);
          }
        }
      }
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDZONINGSECTORS);
}

void CMirror::Clear(void)
{
  mi_cspoPolygons.Clear();
}

// add given polygon to this mirror
void CMirror::AddPolygon(CRenderer &re, CScreenPolygon &spo)
{
  CBrushPolygon *pbpo = spo.spo_pbpoBrushPolygon;
  // if not already added
  if (!mi_cspoPolygons.IsMember(&spo)) {
    // add it
    mi_cspoPolygons.Add(&spo);
    CBrushSector &bsc = *pbpo->bpo_pbscSector;
    // for each edge of the polygon
    FOREACHINSTATICARRAY(pbpo->bpo_abpePolygonEdges, CBrushPolygonEdge, itpe) {
      // get transformed end vertices
      CBrushVertex &bvx0 = *itpe->bpe_pbedEdge->bed_pbvxVertex0;
      CBrushVertex &bvx1 = *itpe->bpe_pbedEdge->bed_pbvxVertex1;
      INDEX ivx0 = bsc.bsc_abvxVertices.Index(&bvx0);
      INDEX ivx1 = bsc.bsc_abvxVertices.Index(&bvx1);
      FLOAT3D &tv0 = re.re_avvxViewVertices[bsc.bsc_ivvx0+ivx0].vvx_vView;
      FLOAT3D &tv1 = re.re_avvxViewVertices[bsc.bsc_ivvx0+ivx1].vvx_vView;
      // check both vertices and update closest vertex
      if (tv0(3)>mi_vClosest(3)) {
        mi_vClosest = tv0;
      }
      if (tv1(3)>mi_vClosest(3)) {
        mi_vClosest = tv1;
      }
    }
  }
}

// calculate all needed data from screen polygons
void CMirror::FinishAdding(void)
{
  // clear all data
  mi_boxOnScreen = PIXaabbox2D();
  mi_fpixArea = 0;
  mi_fpixMaxPolygonArea = 0;

  // for each polygon
  FOREACHINDYNAMICCONTAINER(mi_cspoPolygons, CScreenPolygon, itspo) {
    CScreenPolygon &spo = *itspo;

    mi_boxOnScreen|=PIX2D(spo.spo_pixMinI, spo.spo_pixMinJ);
    mi_boxOnScreen|=PIX2D(spo.spo_pixMaxI, spo.spo_pixMaxJ);
    mi_fpixArea += spo.spo_pixTotalArea;
    mi_fpixMaxPolygonArea = Max(mi_fpixMaxPolygonArea, FLOAT(spo.spo_pixTotalArea));
  }
}

/* Add a mirror/portal. */
void CRenderer::AddMirror(CScreenPolygon &spo)
{
  // if far sentinel
  if (spo.spo_pbpoBrushPolygon==NULL) {
    // do nothing
    return;
  }

  // get its index
  CBrushPolygon &bpo = *spo.spo_pbpoBrushPolygon;
  INDEX iMirrorType = bpo.bpo_bppProperties.bpp_ubMirrorType;

  // if no mirror
  if (iMirrorType == 0) {
    // do nothing
    return;
  }

  // if this is last renderer (no more recursion)
  if (re_iIndex>=MAX_RENDERERS-1) {
    // do nothing
    return;
  }

  // if mirrors are disabled
  if (!_wrpWorldRenderPrefs.wrp_bMirrorsOn) {
    // do nothing
    return;
  }

  // for each mirror
  for(INDEX i=0; i<re_amiMirrors.Count(); i++) {
    CMirror &mi = re_amiMirrors[i];
    // if it has same index
    if (mi.mi_iMirrorType==iMirrorType) {
      // add the polygon to the mirror
      mi.AddPolygon(*this, spo);
      return;
    }
  }
  // if no index is found, create new one
  CMirror &mi = re_amiMirrors.Push();
  mi.Clear();
  mi.mi_iMirrorType = iMirrorType;
  CEntity &en = *bpo.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  CPlacement3D plLerped = en.GetLerpedPlacement();
  FLOATmatrix3D m;
  MakeRotationMatrixFast(m, plLerped.pl_OrientationAngle);
  mi.mi_vClosest = FLOAT3D(0,0,-100000);
  mi.mi_plPlane = 
    bpo.bpo_pbplPlane->bpl_plRelative*m + plLerped.pl_PositionVector;
  BOOL bSuccess = en.GetMirror(iMirrorType, mi.mi_mp);
  // if mirror is valid
  if (bSuccess) {
    // add the polygon to it
    mi.AddPolygon(*this, spo);
  // if mirror is not valid
  } else {
    // remove the mirror
    mi.mi_iMirrorType = -1;
  }

}
