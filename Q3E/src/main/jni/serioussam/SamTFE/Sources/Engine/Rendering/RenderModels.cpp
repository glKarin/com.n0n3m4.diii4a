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


extern INDEX mdl_iShadowQuality;
// model shadow precision
// 0 = no shadows
// 1 = one simple shadow
// 2 = one complex shadow
// 3 = all shadows


/*
 * Compare two models for sorting.
 */
static inline int CompareDelayedModels( const CDelayedModel &dm0, const CDelayedModel &dm1)
{
  BOOL bHasAlpha0 = dm0.dm_ulFlags&DMF_HASALPHA;
  BOOL bHasAlpha1 = dm1.dm_ulFlags&DMF_HASALPHA;
  if(! bHasAlpha0 &&  bHasAlpha1) {
     return -1;
  }  else if(  bHasAlpha0 && !bHasAlpha1) {
     return +1;
  }

  if(dm0.dm_fDistance<dm1.dm_fDistance) {
     return -1;
  } else if(dm0.dm_fDistance>dm1.dm_fDistance) {
     return +1;
  } else {
     return  0;
  }
}

static int qsort_CompareDelayedModels( const void *ppdm0, const void *ppdm1)
{
  CDelayedModel &dm0 = **(CDelayedModel **)ppdm0;
  CDelayedModel &dm1 = **(CDelayedModel **)ppdm1;
  return +CompareDelayedModels(dm0, dm1);
}


static inline FLOAT IntensityAtDistance( FLOAT fFallOff, FLOAT fHotSpot, FLOAT fDistance)
{
  // intensity is zero if further than fall-off range
  if( fDistance>fFallOff) return 0.0f;
  // intensity is maximum if closer than hot-spot range
  if( fDistance<fHotSpot) return 1.0f;
  // interpolate if between fall-off and hot-spot range
  return (fFallOff-fDistance)/(fFallOff-fHotSpot);
}


/* Find lights for one model. */
BOOL CRenderer::FindModelLights( CEntity &en, const CPlacement3D &plModel,
                                 COLOR &colLight, COLOR &colAmbient, FLOAT &fTotalShadowIntensity,
                                 FLOAT3D &vTotalLightDirection, FLOATplane3D &plFloorPlane)
{
  // find shading info if not already cached
  if (en.en_psiShadingInfo!=NULL && !(en.en_ulFlags&ENF_VALIDSHADINGINFO)) {
    _pfRenderProfile.StartTimer(CRenderProfile::PTI_FINDSHADINGINFO);
    _pfRenderProfile.IncrementTimerAveragingCounter(CRenderProfile::PTI_FINDSHADINGINFO, 1);
    if (en.en_ulFlags&ENF_NOSHADINGINFO) {
      en.en_psiShadingInfo=NULL;
    } else {
      en.FindShadingInfo();
    }
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDSHADINGINFO);
  }

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_FINDLIGHTS);

  // clear list of active lights
  _amlLights.PopAll();

  // if there is no valid shading info
  if( en.en_psiShadingInfo==NULL/* || en.en_psiShadingInfo->si_pbpoPolygon==NULL*/)
  { // no shadow
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDLIGHTS);
    return FALSE;
  }
  // if there is valid shading info
  else
  {
    // if model is above terrain
    if(en.en_psiShadingInfo->si_ptrTerrain!=NULL) {
      CTerrain *ptrTerrain = en.en_psiShadingInfo->si_ptrTerrain;
      // if full bright rendering
      if (_wrpWorldRenderPrefs.wrp_shtShadows==CWorldRenderPrefs::SHT_NONE) {
        // no model shading
        colLight = C_BLACK;
        colAmbient = C_GRAY;
        vTotalLightDirection = FLOAT3D(1.0f, -1.0f, 1.0f);
        _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDLIGHTS);
        return FALSE;
      }

      // create floor plane for shadow
      ASSERT(en.en_psiShadingInfo->si_lnInPolygon.IsLinked());

      COLOR colShade = ptrTerrain->GetShadeColor(en.en_psiShadingInfo);
      colLight = MulColors(colShade,0xD8D8D8FF);
      colAmbient = MulColors(colShade,0xB2B2B2FF);
      vTotalLightDirection = FLOAT3D(1.0f, -1.0f, 1.0f);
      fTotalShadowIntensity = 0.5f;
      plFloorPlane = ptrTerrain->GetPlaneFromPoint(en.en_psiShadingInfo->si_vNearPoint);
    
    // else if model is above polygon
    } else if(en.en_psiShadingInfo->si_pbpoPolygon!=NULL) {
      // if full bright rendering
      if (_wrpWorldRenderPrefs.wrp_shtShadows==CWorldRenderPrefs::SHT_NONE) {
        // no model shading
        colLight = C_BLACK;
        colAmbient = C_GRAY;
        vTotalLightDirection = FLOAT3D(1.0f, -1.0f, 1.0f);
        _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDLIGHTS);
        return FALSE;
      }

      // create floor plane for shadow
      ASSERT(en.en_psiShadingInfo->si_lnInPolygon.IsLinked());
      plFloorPlane = en.en_psiShadingInfo->si_pbpoPolygon->bpo_pbplPlane->bpl_plAbsolute;

      // if full bright polygon
      if( en.en_psiShadingInfo->si_pbpoPolygon->bpo_ulFlags&BPOF_FULLBRIGHT) {
        // take light from polygon shadow
        COLOR col  = en.en_psiShadingInfo->si_pbpoPolygon->bpo_colShadow;
        colLight   = LerpColor( C_BLACK, col, 0.25f);
        colAmbient = LerpColor( C_BLACK, col, 0.33f);
        fTotalShadowIntensity = NormByteToFloat((en.en_psiShadingInfo->si_pbpoPolygon->bpo_colShadow&CT_AMASK)>>CT_ASHIFT);
        vTotalLightDirection  = FLOAT3D(1.0f, -1.0f, 1.0f);
        _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDLIGHTS);
        return TRUE;
      }

      // get the shadow map of the underlying polygon
      CBrushShadowMap &bsm = en.en_psiShadingInfo->si_pbpoPolygon->bpo_smShadowMap;
      // get ambient light
      UBYTE ubAR, ubAG, ubAB;
      ColorToRGB( bsm.GetBrushPolygon()->bpo_pbscSector->bsc_colAmbient, ubAR, ubAG, ubAB);
      SLONG slSAR=ubAR, slSAG=ubAG, slSAB=ubAB;

      fTotalShadowIntensity = 0.0f;
      // for each shadow layer
      {FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm.bsm_lhLayers, itbsl)
      {
        // get the light source
        CLightSource *plsLight = itbsl->bsl_plsLightSource;

        // remember the light parameters
        UBYTE ubR, ubG, ubB;
        UBYTE ubDAR, ubDAG, ubDAB;
        plsLight->GetLightColorAndAmbient( ubR, ubG, ubB, ubDAR, ubDAG, ubDAB);

        // add directional ambient if needed
        if( en.en_psiShadingInfo->si_pbpoPolygon->bpo_ulFlags&BPOF_HASDIRECTIONALAMBIENT) {
          slSAR += ubDAR;
          slSAG += ubDAG;
          slSAB += ubDAB;
        }

        // get the layer intensity at the point
        FLOAT fShadowFactor;
        if (en.en_ulFlags&ENF_CLUSTERSHADOWS) {
          fShadowFactor = 1.0f;
        } else {
          fShadowFactor = itbsl->GetLightStrength( en.en_psiShadingInfo->si_pixShadowU, en.en_psiShadingInfo->si_pixShadowV,
                                                   en.en_psiShadingInfo->si_fUDRatio,   en.en_psiShadingInfo->si_fLRRatio);
        }
        // skip this light if no intensity
        if( fShadowFactor<0.01f) continue;

        const CPlacement3D &plLight = plsLight->ls_penEntity->GetPlacement();
        const FLOAT3D &vLight = plLight.pl_PositionVector;
        // get its parameters at the model position
        FLOAT3D vDirection;
        FLOAT fDistance;
        FLOAT fFallOffFactor;

        if (plsLight->ls_ulFlags&LSF_DIRECTIONAL) {
          fFallOffFactor = 1.0f;
          AnglesToDirectionVector(plLight.pl_OrientationAngle, vDirection);
          plModel.pl_PositionVector-vLight;
          if (!(en.en_psiShadingInfo->si_pbpoPolygon->bpo_ulFlags&BPOF_HASDIRECTIONALLIGHT)) {
            ubR = ubG = ubB = 0;
          }
          fDistance = 1.0f;
        } else {
          vDirection = plModel.pl_PositionVector-vLight;
          fDistance = vDirection.Length();

          if (fDistance>plsLight->ls_rFallOff) {
            continue;
          } else if (fDistance<plsLight->ls_rHotSpot) {
            fFallOffFactor = 1.0f;
          } else {
            fFallOffFactor = (plsLight->ls_rFallOff-fDistance)/
              (plsLight->ls_rFallOff-plsLight->ls_rHotSpot);
          }
        }
        // add the light to active lights
        struct ModelLight &ml = _amlLights.Push();
        ml.ml_plsLight = plsLight;
        // normalize direction vector
        if (fDistance>0.001f) {
          ml.ml_vDirection = vDirection/fDistance;
        } else {
          ml.ml_vDirection = FLOAT3D(0.0f,0.0f,0.0f);
        }
        // special case for substract sector ambient light
        if (plsLight->ls_ulFlags&LSF_SUBSTRACTSECTORAMBIENT) {
          ubR = (UBYTE)Clamp( (SLONG)ubR-slSAR, (SLONG)0, (SLONG)255);
          ubG = (UBYTE)Clamp( (SLONG)ubG-slSAG, (SLONG)0, (SLONG)255);
          ubB = (UBYTE)Clamp( (SLONG)ubB-slSAB, (SLONG)0, (SLONG)255);
        }
        // calculate light intensity
        FLOAT fShade = (ubR+ubG+ubB)*(2.0f/(3.0f*255.0f));
        ml.ml_fShadowIntensity = fShade*fShadowFactor;
        fTotalShadowIntensity += ml.ml_fShadowIntensity;
        // special case for dark light
        if (plsLight->ls_ulFlags&LSF_DARKLIGHT) {
          ml.ml_fR = -ubR*fFallOffFactor; 
          ml.ml_fG = -ubG*fFallOffFactor; 
          ml.ml_fB = -ubB*fFallOffFactor;
        } else {
          ml.ml_fR = +ubR*fFallOffFactor; 
          ml.ml_fG = +ubG*fFallOffFactor; 
          ml.ml_fB = +ubB*fFallOffFactor;
        }
      }}

      FLOAT fTR=0.0f; FLOAT fTG=0.0f; FLOAT fTB=0.0f;
      FLOAT3D vDirection(0.0f,0.0f,0.0f);
      // for each active light
      {for(INDEX iLight=0; iLight<_amlLights.Count(); iLight++) {
        struct ModelLight &ml = _amlLights[iLight];
        // add it to total intensity
        fTR += ml.ml_fR;
        fTG += ml.ml_fG;
        fTB += ml.ml_fB;
        // add it to direction vector
        FLOAT fWeight = Abs(ml.ml_fR+ml.ml_fG+ml.ml_fB) * (1.0f/(3.0f*255.0f));
        vDirection+=ml.ml_vDirection*fWeight;
      }}
      // normalize average direction vector
      FLOAT fDirection = vDirection.Length();
      if (fDirection>0.001f) {
        vDirection /= fDirection;
      } else {
        vDirection = FLOAT3D(0.0f,0.0f,0.0f);
      }

      // for each active light
      FLOAT fDR=0.0f; FLOAT fDG=0.0f; FLOAT fDB=0.0f;
      {for(INDEX iLight=0; iLight<_amlLights.Count(); iLight++) {
        struct ModelLight &ml = _amlLights[iLight];
        // find its contribution to direction vector
        const FLOAT fFactor = ClampDn( vDirection%ml.ml_vDirection, 0.0f);
        // add it to directional intensity
        fDR += ml.ml_fR*fFactor;
        fDG += ml.ml_fG*fFactor;
        fDB += ml.ml_fB*fFactor;
      }}

      // adjust ambient light with gradient if needed
      ULONG ulGradientType = en.en_psiShadingInfo->si_pbpoPolygon->bpo_bppProperties.bpp_ubGradientType;
      if( ulGradientType>0) {
        CGradientParameters gp;
        COLOR colGradientPoint;
        CEntity *pen = en.en_psiShadingInfo->si_pbpoPolygon->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
        if( pen!=NULL && pen->GetGradient( ulGradientType, gp)) {
          FLOAT fGrPt = (en.en_psiShadingInfo->si_vNearPoint % gp.gp_vGradientDir - gp.gp_fH0) / (gp.gp_fH1-gp.gp_fH0);
          fGrPt = Clamp( fGrPt, 0.0f, 1.0f);
          colGradientPoint = LerpColor( gp.gp_col0, gp.gp_col1, fGrPt);
          UBYTE ubGR,ubGG,ubGB;
          ColorToRGB( colGradientPoint, ubGR,ubGG,ubGB);
          // add or substract gradient component to total ambient
          if( gp.gp_bDark) { slSAR-=ubGR;  slSAG-=ubGG;  slSAB-=ubGB; }
          else             { slSAR+=ubGR;  slSAG+=ubGG;  slSAB+=ubGB; }
        }
      }
      // clamp ambient component
      slSAR = Clamp( slSAR, (SLONG)0, (SLONG)255);
      slSAG = Clamp( slSAG, (SLONG)0, (SLONG)255);
      slSAB = Clamp( slSAB, (SLONG)0, (SLONG)255);

      // calculate average light properties
      SLONG slAR = Clamp( (SLONG)FloatToInt(fTR-fDR) +slSAR, (SLONG)0, (SLONG)255);
      SLONG slAG = Clamp( (SLONG)FloatToInt(fTG-fDG) +slSAG, (SLONG)0, (SLONG)255);
      SLONG slAB = Clamp( (SLONG)FloatToInt(fTB-fDB) +slSAB, (SLONG)0, (SLONG)255);
      SLONG slLR = Clamp( (SLONG)FloatToInt(fDR), (SLONG)0, (SLONG)255);
      SLONG slLG = Clamp( (SLONG)FloatToInt(fDG), (SLONG)0, (SLONG)255);
      SLONG slLB = Clamp( (SLONG)FloatToInt(fDB), (SLONG)0, (SLONG)255);
      colLight   = RGBToColor( slLR,slLG,slLB);
      colAmbient = RGBToColor( slAR,slAG,slAB);

      // adjust for changed polygon shadow color
      COLOR colShadowMap = en.en_psiShadingInfo->si_pbpoPolygon->bpo_colShadow;
      CTextureBlending &tbShadow = re_pwoWorld->wo_atbTextureBlendings[
        en.en_psiShadingInfo->si_pbpoPolygon->bpo_bppProperties.bpp_ubShadowBlend];
      COLOR colShadowMapAdjusted = MulColors(colShadowMap, tbShadow.tb_colMultiply);
      colLight   = MulColors( colLight,   colShadowMapAdjusted);
      colAmbient = MulColors( colAmbient, colShadowMapAdjusted);
      vTotalLightDirection = vDirection;
      // else no valid shading info
    } else {
      // no shadow
      _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDLIGHTS);
      return FALSE;
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_FINDLIGHTS);
  return TRUE;
}


/*
 * Render one model with shadow (eventually)
 */
void CRenderer::RenderOneModel( CEntity &en, CModelObject &moModel, const CPlacement3D &plModel,
                                const FLOAT fDistanceFactor, BOOL bRenderShadow, ULONG ulDMFlags)
{
  // skip invisible models
  if( moModel.mo_Stretch == FLOAT3D(0,0,0)) return;

  // do nothing, if rendering shadows and this model doesn't cast cluster shadows
  if( re_bRenderingShadows && !(en.en_ulFlags&ENF_CLUSTERSHADOWS)) return;
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERONEMODEL);

  CPlacement3D plLight;
  plLight.pl_PositionVector = plModel.pl_PositionVector + FLOAT3D(3.0f,3.0f,3.0f);
  plLight.pl_OrientationAngle = ANGLE3D(AngleDeg(30.0f), AngleDeg(-45.0f),0);

  // create a default light
  COLOR colLight   = C_GRAY;
  COLOR colAmbient = C_dGRAY;
  FLOAT3D vTotalLightDirection( 1.0f, -1.0f, 1.0f);
  FLOATplane3D plFloorPlane(FLOAT3D( 0.0f, 1.0f, 0.0f), 0.0f);

  BOOL bRenderModelShadow = FALSE;
  FLOAT fTotalShadowIntensity = 0.0f;
  // if not rendering cluster shadows
  if( !re_bRenderingShadows) {
    // find model lights
    bRenderModelShadow = FindModelLights( en, plModel, colLight, colAmbient,
                                          fTotalShadowIntensity, vTotalLightDirection, plFloorPlane);
  }

  // let the entity adjust shading parameters if it wants to
  mdl_iShadowQuality = Clamp( mdl_iShadowQuality, (INDEX)0, (INDEX)3);
  const BOOL bAllowShadows = en.AdjustShadingParameters( vTotalLightDirection, colLight, colAmbient);
  bRenderModelShadow = (bRenderModelShadow && bAllowShadows && bRenderShadow && mdl_iShadowQuality>0);
  
  // prepare render model structure
  CRenderModel rm;
  rm.rm_vLightDirection = vTotalLightDirection;
  rm.rm_fDistanceFactor = fDistanceFactor;
  rm.rm_colLight   = colLight;
  rm.rm_colAmbient = colAmbient;
  rm.SetObjectPlacement(plModel);
  if( ulDMFlags & DMF_FOG)      rm.rm_ulFlags |= RMF_FOG;
  if( ulDMFlags & DMF_HAZE)     rm.rm_ulFlags |= RMF_HAZE;
  if( ulDMFlags & DMF_INSIDE)   rm.rm_ulFlags |= RMF_INSIDE;
  if( ulDMFlags & DMF_INMIRROR) rm.rm_ulFlags |= RMF_INMIRROR;

  // mark that we don't actualy need entire model
  if( re_penViewer==&en) {
    rm.rm_ulFlags |= RMF_SPECTATOR;
    bRenderModelShadow = FALSE;
  }

  // TEMP: disable Truform usage on weapon models
  if( IsOfClass( &en, "Player Weapons")) rm.rm_ulFlags |= RMF_WEAPON; 

  // set tesselation level of models
  rm.rm_iTesselationLevel = (INDEX) (en.GetMaxTessellationLevel());

  // prepare CRenderModel structure for rendering of one model
  moModel.SetupModelRendering(rm);

  // determine shadow intensity
  
  fTotalShadowIntensity *= NormByteToFloat( (moModel.mo_colBlendColor&CT_AMASK)>>CT_ASHIFT);
  fTotalShadowIntensity  = Clamp( fTotalShadowIntensity, 0.0f, 1.0f);

  // if should render shadow for this model
  if( bRenderModelShadow && !(en.en_ulFlags&ENF_CLUSTERSHADOWS) && moModel.HasShadow(rm.rm_iMipLevel)) {
    // if only simple shadow
    if( mdl_iShadowQuality==1) {
      // render simple shadow
      fTotalShadowIntensity = 0.1f + fTotalShadowIntensity*0.9f;
      moModel.AddSimpleShadow( rm, fTotalShadowIntensity, plFloorPlane);
    }
    // if only one shadow
    else if( mdl_iShadowQuality==2) {
      // render one shadow of model from shading light direction
      const FLOAT fHotSpot = 1E10f;
      const FLOAT fFallOff = 1E11f;
      CPlacement3D plLight;
      plLight.pl_PositionVector = plModel.pl_PositionVector - rm.rm_vLightDirection*1000.0f;
      moModel.RenderShadow( rm, plLight, fFallOff, fHotSpot, fTotalShadowIntensity, plFloorPlane);
    }
    // if full shadows
    else if( mdl_iShadowQuality==3) {
      // for each active light
      for( INDEX iLight=0; iLight<_amlLights.Count(); iLight++) {
        struct ModelLight &ml = _amlLights[iLight];
        // skip light if doesn't cast shadows
        if( !(ml.ml_plsLight->ls_ulFlags&LSF_CASTSHADOWS)) continue;
        // get light parameters
        CPlacement3D plLight = ml.ml_plsLight->ls_penEntity->en_plPlacement;
        FLOAT fHotSpot = ml.ml_plsLight->ls_rHotSpot;
        FLOAT fFallOff = ml.ml_plsLight->ls_rFallOff;
        if (ml.ml_plsLight->ls_ulFlags & LSF_DIRECTIONAL) {
          fHotSpot = 1E10f;
          fFallOff = 1E11f;
          FLOAT3D vDirection;
          AnglesToDirectionVector( plLight.pl_OrientationAngle, vDirection);
          plLight.pl_PositionVector = plModel.pl_PositionVector-(vDirection*1000.0f);
        } 
        // render one shadow of model
        const FLOAT fShadowIntensity  = Clamp( ml.ml_fShadowIntensity, 0.0f, 1.0f);
        moModel.RenderShadow( rm, plLight, fFallOff, fHotSpot, fShadowIntensity, plFloorPlane);
      }
    }
  }

  // if the entity is not the viewer, or this is not primary renderer
  if( re_penViewer!=&en) {
    // render model
    moModel.RenderModel(rm);
  // if the entity is viewer
  } else {
    // just remember the shading info (needed for first-person-weapon rendering)
    _vViewerLightDirection = rm.rm_vLightDirection;
    _colViewerLight        = rm.rm_colLight;
    _colViewerAmbient      = rm.rm_colAmbient;
  }

  // all done
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERONEMODEL);
}

/*
 * Render one ska model with shadow (eventually)
 */
void CRenderer::RenderOneSkaModel( CEntity &en, const CPlacement3D &plModel,
                                  const FLOAT fDistanceFactor, BOOL bRenderShadow, ULONG ulDMFlags)
{
  // skip invisible models
  if( en.GetModelInstance()->mi_vStretch == FLOAT3D(0,0,0)) return;

  // do nothing, if rendering shadows and this model doesn't cast cluster shadows
  if( re_bRenderingShadows && !(en.en_ulFlags&ENF_CLUSTERSHADOWS)) return;
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERONEMODEL);

  CPlacement3D plLight;
  plLight.pl_PositionVector = plModel.pl_PositionVector + FLOAT3D(3.0f,3.0f,3.0f);
  plLight.pl_OrientationAngle = ANGLE3D(AngleDeg(30.0f), AngleDeg(-45.0f),0);

  // create a default light
  COLOR colLight   = C_GRAY;
  COLOR colAmbient = C_dGRAY;
  FLOAT3D vTotalLightDirection( 1.0f, -1.0f, 1.0f);
  FLOATplane3D plFloorPlane(FLOAT3D( 0.0f, 1.0f, 0.0f), 0.0f);

  BOOL bRenderModelShadow = FALSE;
  FLOAT fTotalShadowIntensity = 0.0f;
  // if not rendering cluster shadows
  if( !re_bRenderingShadows) {
    // find model lights
    bRenderModelShadow = FindModelLights( en, plModel, colLight, colAmbient,
                                          fTotalShadowIntensity, vTotalLightDirection, plFloorPlane);
  }

  // let the entity adjust shading parameters if it wants to
  mdl_iShadowQuality = Clamp( mdl_iShadowQuality, (INDEX)0, (INDEX)3);
  const BOOL bAllowShadows = en.AdjustShadingParameters( vTotalLightDirection, colLight, colAmbient);
  bRenderModelShadow = (bRenderModelShadow && bAllowShadows && bRenderShadow && mdl_iShadowQuality>0);

  ULONG &ulRenFlags = RM_GetRenderFlags();
  ulRenFlags = 0;
  if( ulDMFlags & DMF_FOG)      ulRenFlags |= RMF_FOG;
  if( ulDMFlags & DMF_HAZE)     ulRenFlags |= RMF_HAZE;
  if( ulDMFlags & DMF_INSIDE)   ulRenFlags |= RMF_INSIDE;
  if( ulDMFlags & DMF_INMIRROR) ulRenFlags |= RMF_INMIRROR;
  
  // mark that we don't actualy need entire model
  if( re_penViewer==&en) {
    ulRenFlags |= RMF_SPECTATOR;
    bRenderModelShadow = FALSE;
  }

  RM_SetObjectPlacement(en.GetLerpedPlacement());
  RM_SetLightColor(colAmbient,colLight);
  RM_SetLightDirection(vTotalLightDirection);

  // determine shadow intensity
  fTotalShadowIntensity *= NormByteToFloat( (en.GetModelInstance()->GetModelColor()&CT_AMASK)>>CT_ASHIFT);
  fTotalShadowIntensity  = Clamp( fTotalShadowIntensity, 0.0f, 1.0f);

  // if should render shadow for this model
  if( bRenderModelShadow && !(en.en_ulFlags&ENF_CLUSTERSHADOWS) && en.GetModelInstance()->HasShadow(1/*rm.rm_iMipLevel*/)) {
    // if only simple shadow
    if( mdl_iShadowQuality==1) {
      // render simple shadow
      fTotalShadowIntensity = 0.1f + fTotalShadowIntensity*0.9f;
        en.GetModelInstance()->AddSimpleShadow(fTotalShadowIntensity, plFloorPlane);
    }
    // if only one shadow
    else if( mdl_iShadowQuality==2) {
      /*
      // render one shadow of model from shading light direction
      const FLOAT fHotSpot = 1E10f;
      const FLOAT fFallOff = 1E11f;
      CPlacement3D plLight;
      plLight.pl_PositionVector = plModel.pl_PositionVector - vTotalLightDirection*1000.0f;
      // moModel.RenderShadow( rm, plLight, fFallOff, fHotSpot, fTotalShadowIntensity, plFloorPlane);
      */
      fTotalShadowIntensity = 0.1f + fTotalShadowIntensity*0.9f;
      en.GetModelInstance()->AddSimpleShadow(fTotalShadowIntensity, plFloorPlane);
    }
    // if full shadows
    else if( mdl_iShadowQuality==3) {
      /*
      // for each active light
      for( INDEX iLight=0; iLight<_amlLights.Count(); iLight++) {
        struct ModelLight &ml = _amlLights[iLight];
        // skip light if doesn't cast shadows
        if( !(ml.ml_plsLight->ls_ulFlags&LSF_CASTSHADOWS)) continue;
        // get light parameters
        CPlacement3D plLight = ml.ml_plsLight->ls_penEntity->en_plPlacement;
        FLOAT fHotSpot = ml.ml_plsLight->ls_rHotSpot;
        FLOAT fFallOff = ml.ml_plsLight->ls_rFallOff;
        if (ml.ml_plsLight->ls_ulFlags & LSF_DIRECTIONAL) {
          fHotSpot = 1E10f;
          fFallOff = 1E11f;
          FLOAT3D vDirection;
          AnglesToDirectionVector( plLight.pl_OrientationAngle, vDirection);
          plLight.pl_PositionVector = plModel.pl_PositionVector-(vDirection*1000.0f);
        } 
        // render one shadow of model
        const FLOAT fShadowIntensity  = Clamp( ml.ml_fShadowIntensity, 0.0f, 1.0f);
        // moModel.RenderShadow( rm, plLight, fFallOff, fHotSpot, fShadowIntensity, plFloorPlane);
      }
      */
      fTotalShadowIntensity = 0.1f + fTotalShadowIntensity*0.9f;
      en.GetModelInstance()->AddSimpleShadow(fTotalShadowIntensity, plFloorPlane);
    }
  }

  // if the entity is not the viewer, or this is not primary renderer
  if( re_penViewer!=&en) {
    // render model
    RM_SetBoneAdjustCallback(&EntityAdjustBonesCallback,&en);
    RM_SetShaderParamsAdjustCallback(&EntityAdjustShaderParamsCallback,&en);
    RM_RenderSKA(*en.GetModelInstanceForRendering());
  // if the entity is viewer
  } else {
    // just remember the shading info (needed for first-person-weapon rendering)
    _vViewerLightDirection = vTotalLightDirection;
    _colViewerLight        = colLight;
    _colViewerAmbient      = colAmbient;
  }

  // all done
  // _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERONEMODEL);

}

/* 
 * Render models that were kept for delayed rendering.
 */
void CRenderer::RenderModels( BOOL bBackground)
{
  if( _bMultiPlayer) gfx_bRenderModels = 1; // must render in multiplayer mode!
  if( !gfx_bRenderModels && !re_bRenderingShadows) return;

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERMODELS);

  // sort all the delayed models by distance
  qsort(re_admDelayedModels.GetArrayOfPointers(), re_admDelayedModels.Count(),
      sizeof(CDelayedModel *), qsort_CompareDelayedModels);

  CAnyProjection3D *papr;
  if( bBackground) {
    papr = &re_prBackgroundProjection;
  } else {
    papr = &re_prProjection;
  }

  // begin model rendering
  if( !re_bRenderingShadows) {
    BeginModelRenderingView( *papr, re_pdpDrawPort);
    RM_BeginRenderingView(   *papr, re_pdpDrawPort);
  } else {
    BeginModelRenderingMask(    *papr, re_pubShadow, re_slShadowWidth, re_slShadowHeight);
    RM_BeginModelRenderingMask( *papr, re_pubShadow, re_slShadowWidth, re_slShadowHeight);
  }


  // for each of models that were kept for delayed rendering
  for( INDEX iModel=0; iModel<re_admDelayedModels.Count(); iModel++) {
    CDelayedModel &dm = re_admDelayedModels[iModel];
    CEntity &en = *dm.dm_penModel;
    BOOL bIsBackground = re_bBackgroundEnabled && (en.en_ulFlags&ENF_BACKGROUND);

    // skip if not rendered in this pass or not visible
    if(  (bBackground && !bIsBackground)
     || (!bBackground &&  bIsBackground)
     || !(dm.dm_ulFlags&DMF_VISIBLE)) continue;

    if(en.en_RenderType == CEntity::RT_SKAMODEL || en.en_RenderType == CEntity::RT_SKAEDITORMODEL)
    {
      RenderOneSkaModel(en, en.GetLerpedPlacement(), dm.dm_fMipFactor, TRUE, dm.dm_ulFlags);

      // if selected entities should be drawn and this one is selected
      if( !re_bRenderingShadows && _wrpWorldRenderPrefs.wrp_stSelection==CWorldRenderPrefs::ST_ENTITIES
       && _wrpWorldRenderPrefs.wrp_pmoSelectedEntity!=NULL && en.IsSelected(ENF_SELECTED))
      { // get bounding box of current frame
        FLOATaabbox3D boxModel;
        en.GetModelInstance()->GetCurrentColisionBox(boxModel);
        // if model has collision
        if( en.en_pciCollisionInfo!=NULL ) {
          // get its collision box
          INDEX iCollision = en.GetCollisionBoxIndex();
          FLOAT3D vMin = en.GetModelInstance()->GetCollisionBoxMin(iCollision);
          FLOAT3D vMax = en.GetModelInstance()->GetCollisionBoxMax(iCollision);
          // extend the box by the collision box
          boxModel|=FLOATaabbox3D(vMin, vMax);
        }
        // set position of marker at top of the model and it size to be proportional to the model
        boxModel.StretchByVector(en.GetModelInstance()->mi_vStretch);
        FLOAT fSize = boxModel.Size().Length()*0.3f;
        _wrpWorldRenderPrefs.wrp_pmoSelectedEntity->mo_Stretch = FLOAT3D( fSize, fSize, fSize);
        CPlacement3D plSelection = en.GetLerpedPlacement();
        plSelection.Translate_OwnSystem( FLOAT3D(0.0f, boxModel.Max()(2), 0.0f));

        // render the selection model without shadow
        RenderOneModel( en, *_wrpWorldRenderPrefs.wrp_pmoSelectedEntity, plSelection, dm.dm_fMipFactor, FALSE, 0);
      }
    }
    else
    {
      // render the model with its shadow
      CModelObject &moModelObject = *dm.dm_pmoModel;
      RenderOneModel( en, moModelObject, en.GetLerpedPlacement(), dm.dm_fMipFactor, TRUE, dm.dm_ulFlags);

      // if selected entities should be drawn and this one is selected
      if( !re_bRenderingShadows && _wrpWorldRenderPrefs.wrp_stSelection==CWorldRenderPrefs::ST_ENTITIES
       && _wrpWorldRenderPrefs.wrp_pmoSelectedEntity!=NULL && en.IsSelected(ENF_SELECTED))
      { // get bounding box of current frame
        FLOATaabbox3D boxModel;
        moModelObject.GetCurrentFrameBBox(boxModel);
        // if model has collision
        if( en.en_pciCollisionInfo!=NULL && 
          (en.GetRenderType()==CEntity::RT_MODEL || en.GetRenderType()==CEntity::RT_EDITORMODEL)) {
          // get its collision box
          INDEX iCollision = en.GetCollisionBoxIndex();
          FLOAT3D vMin = moModelObject.GetCollisionBoxMin(iCollision);
          FLOAT3D vMax = moModelObject.GetCollisionBoxMax(iCollision);
          // extend the box by the collision box
          boxModel|=FLOATaabbox3D(vMin, vMax);
        }
        // set position of marker at top of the model and it size to be proportional to the model
        boxModel.StretchByVector(moModelObject.mo_Stretch);
        FLOAT fSize = boxModel.Size().Length()*0.3f;
        _wrpWorldRenderPrefs.wrp_pmoSelectedEntity->mo_Stretch = FLOAT3D( fSize, fSize, fSize);
        CPlacement3D plSelection = en.GetLerpedPlacement();
        plSelection.Translate_OwnSystem( FLOAT3D(0.0f, boxModel.Max()(2), 0.0f));
        // render the selection model without shadow
        RenderOneModel( en, *_wrpWorldRenderPrefs.wrp_pmoSelectedEntity, plSelection, dm.dm_fMipFactor, FALSE, 0);
      }
    }

  }
  // end model rendering
  if( !re_bRenderingShadows) {
    EndModelRenderingView(FALSE); // don't restore ortho projection for now
    RM_EndRenderingView(FALSE);
  } else {
    EndModelRenderingMask();
    RM_EndModelRenderingMask();
  }



  // done
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERMODELS);
}


extern CEntity *_Particle_penCurrentViewer;
extern FLOAT _Particle_fCurrentMip;
extern BOOL  _Particle_bHasFog;
extern BOOL  _Particle_bHasHaze;

void Particle_PrepareEntity( FLOAT fMipFactor, BOOL bHasFog, BOOL bHasHaze, CEntity *penViewer)
{
  _Particle_fCurrentMip = fMipFactor;
  _Particle_bHasFog     = bHasFog;
  _Particle_bHasHaze    = bHasHaze;
  _Particle_penCurrentViewer = penViewer;
}

/* Render particles for models that were kept for delayed rendering. */
void CRenderer::RenderParticles(BOOL bBackground)
{
  if( _bMultiPlayer) gfx_bRenderParticles = 1; // must render in multiplayer mode!
  if( re_bRenderingShadows || !gfx_bRenderParticles) return;

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERPARTICLES);

  // prepare gfx library for particles
  if (bBackground) {
    Particle_PrepareSystem(re_pdpDrawPort, re_prBackgroundProjection);
  } else {
    Particle_PrepareSystem(re_pdpDrawPort, re_prProjection);
  }

  // for each of models that were kept for delayed rendering
  for(INDEX iModel=0; iModel<re_admDelayedModels.Count(); iModel++) {
    CDelayedModel &dm = re_admDelayedModels[iModel];
    CEntity &en = *dm.dm_penModel;
    BOOL bIsBackground = re_bBackgroundEnabled && (en.en_ulFlags&ENF_BACKGROUND);

    // if not rendered in this pass
    if( (bBackground && !bIsBackground) || (!bBackground &&  bIsBackground)) continue;
    Particle_PrepareEntity( dm.dm_fMipFactor, dm.dm_ulFlags&DMF_FOG, dm.dm_ulFlags&DMF_HAZE, re_penViewer);
    // render particles made by this entity
    en.RenderParticles();
    _Particle_penCurrentViewer = NULL;
  }

  // end gfx library for particles
  Particle_EndSystem(FALSE);
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERPARTICLES);
}


void DeleteLensFlare(CLightSource *pls)
{
  // for each renderer
  for (INDEX ire = 0; ire<MAX_RENDERERS; ire++) {
    CRenderer &re = _areRenderers[ire];

    // for each lens flare of this light source
    for(INDEX iFlare=0; iFlare<re.re_alfiLensFlares.Count(); iFlare++) {
      CLensFlareInfo &lfi = re.re_alfiLensFlares[iFlare];
      if (lfi.lfi_plsLightSource == pls) {
        // invalidate lens flare info to be deleted in next rendering
        lfi.lfi_plsLightSource = NULL;
      }
    }
  }
}

/* Render lens flares. */
void CRenderer::RenderLensFlares(void)
{
  // make sure we have orthographic projection
  re_pdpDrawPort->SetOrtho(); 

  // if there are no flares or flares are off, do nothing
  gfx_iLensFlareQuality = Clamp( gfx_iLensFlareQuality, (INDEX)0, (INDEX)3);
  if( gfx_iLensFlareQuality==0 || re_alfiLensFlares.Count()==0) return;

  // get drawport ID
  ASSERT( re_pdpDrawPort!=NULL);
  const ULONG ulDrawPortID = re_pdpDrawPort->GetID();
  
  // for each lens flare of this drawport
  {for(INDEX iFlare=0; iFlare<re_alfiLensFlares.Count(); iFlare++) {
    CLensFlareInfo &lfi = re_alfiLensFlares[iFlare];
    // skip if not in this drawport
    if( lfi.lfi_ulDrawPortID!=ulDrawPortID && lfi.lfi_iMirrorLevel==0) continue;
    // test if it is still visible
    lfi.lfi_ulFlags &= ~LFF_VISIBLE;
    if( re_pdpDrawPort->IsPointVisible( (PIX) lfi.lfi_fI, (PIX) lfi.lfi_fJ, lfi.lfi_fOoK, lfi.lfi_iID, lfi.lfi_iMirrorLevel)) {
      lfi.lfi_ulFlags |= LFF_VISIBLE;
    }
  }}

  // get count of currently existing flares and time
  INDEX ctFlares   = re_alfiLensFlares.Count();
  const TIME tmNow = _pTimer->GetRealTimeTick();

  // for each lens flare
  INDEX iFlare=0;
  while(iFlare<ctFlares) {
    CLensFlareInfo &lfi = re_alfiLensFlares[iFlare];
    // if the flare is not active any more, or its drawport was not refreshed long
    if( lfi.lfi_plsLightSource==NULL || // marked when entity is deleted
       (lfi.lfi_ulDrawPortID==ulDrawPortID && 
     (!(lfi.lfi_ulFlags&LFF_ACTIVE) || lfi.lfi_plsLightSource->ls_plftLensFlare==NULL)) ||
       (lfi.lfi_ulDrawPortID!=ulDrawPortID && lfi.lfi_tmLastFrame<tmNow-5.0f)) {
      // delete it by moving the last one on its place
      lfi = re_alfiLensFlares[ctFlares-1];
      re_alfiLensFlares[ctFlares-1].Clear();
      ctFlares--;
    // if the flare is still active
    } else {
      // go to next flare
      iFlare++;
    }
  }

  // remove unused flares at the end
  if( ctFlares==0) re_alfiLensFlares.PopAll();
  else re_alfiLensFlares.PopUntil(ctFlares-1);

  // for each lens flare of this drawport
  {for(INDEX iFlare=0; iFlare<re_alfiLensFlares.Count(); iFlare++) {
    CLensFlareInfo &lfi = re_alfiLensFlares[iFlare];
    if (lfi.lfi_ulDrawPortID!=ulDrawPortID || lfi.lfi_plsLightSource->ls_plftLensFlare==NULL) {
      continue;
    }
    // clear active flag for next frame
    lfi.lfi_ulFlags &= ~LFF_ACTIVE;

    // fade the flare in/out
    #define FLAREINSPEED  (0.2f)
    #define FLAREOUTSPEED (0.1f)
    if( lfi.lfi_ulFlags&LFF_VISIBLE) {
      lfi.lfi_fFadeFactor += (tmNow-lfi.lfi_tmLastFrame) / FLAREINSPEED;
    } else {
      lfi.lfi_fFadeFactor -= (tmNow-lfi.lfi_tmLastFrame) / FLAREOUTSPEED;
    }
    lfi.lfi_fFadeFactor = Max( Min(lfi.lfi_fFadeFactor, 1.0f), 0.0f);

    // reset timer of flare
    lfi.lfi_tmLastFrame = tmNow;
    // skip if the flare is invisible
    if( lfi.lfi_fFadeFactor<0.01f) continue;

    // calculate general flare factors
    FLOAT fScreenSizeI   = re_pdpDrawPort->GetWidth();
    FLOAT fScreenSizeJ   = re_pdpDrawPort->GetHeight();
    FLOAT fScreenCenterI = fScreenSizeI*0.5f;
    FLOAT fScreenCenterJ = fScreenSizeJ*0.5f;
    FLOAT fLightI = lfi.lfi_fI;
    FLOAT fLightJ = lfi.lfi_fJ;
    FLOAT fIPositionFactor = (fLightI-fScreenCenterI)/fScreenSizeI;
    FLOAT fReflectionDirI = fScreenCenterI-fLightI;
    FLOAT fReflectionDirJ = fScreenCenterJ-fLightJ;
    UBYTE ubR, ubG, ubB;
    lfi.lfi_plsLightSource->GetLightColor(ubR, ubG, ubB);
    UBYTE ubI = (ULONG(ubR)+ULONG(ubG)+ULONG(ubB))/3;
    FLOAT fReflectionDistance = sqrt(fReflectionDirI*fReflectionDirI+fReflectionDirJ*fReflectionDirJ);
    FLOAT fOfCenterFadeFactor = 1.0f-2.0f*fReflectionDistance/fScreenSizeI;
    fOfCenterFadeFactor = Max(fOfCenterFadeFactor, 0.0f);

    FLOAT fFogHazeFade = 1.0f;
    // if in haze
    if( lfi.lfi_ulFlags&LFF_HAZE) {
      // get haze strength at light position
      FLOAT fS = (-lfi.lfi_vProjected(3)+_haze_fAdd)*_haze_fMul;
      FLOAT fHazeStrength = NormByteToFloat(GetHazeAlpha(fS));
      // fade flare with haze
      fFogHazeFade *= 1-fHazeStrength;
    }
    // if in fog
    if( lfi.lfi_ulFlags&LFF_FOG) {
      // get fog strength at light position
      GFXTexCoord tex;
      tex.st.s= -lfi.lfi_vProjected(3)*_fog_fMulZ;
      tex.st.t = (lfi.lfi_vProjected%_fog_vHDirView+_fog_fAddH)*_fog_fMulH;
      FLOAT fFogStrength = NormByteToFloat(GetFogAlpha(tex));
      // fade flare with fog
      fFogHazeFade *= 1-fFogStrength;
    }

    // get number of flares to render for this light
    CStaticArray<COneLensFlare> &aolf = lfi.lfi_plsLightSource->ls_plftLensFlare->lft_aolfFlares;
    INDEX ctReflections = aolf.Count();
    // clamp number reflections if required
    if( gfx_iLensFlareQuality<2 || _wrpWorldRenderPrefs.wrp_lftLensFlares==CWorldRenderPrefs::LFT_SINGLE_FLARE) {
      ctReflections = 1;
    }

    // for each flare in the lens flare effect
    {for(INDEX iReflection=0; iReflection<ctReflections; iReflection++) {
      COneLensFlare &olf = aolf[iReflection];
      // calculate its fading factors
      FLOAT fFadeFactor = lfi.lfi_fFadeFactor * IntensityAtDistance(
        lfi.lfi_plsLightSource->ls_rFallOff*olf.oft_fFallOffFactor,
        lfi.lfi_plsLightSource->ls_rHotSpot*olf.oft_fFallOffFactor,
        lfi.lfi_fDistance);
      fFadeFactor*=fFogHazeFade;
      if (olf.olf_ulFlags&OLF_FADEOFCENTER) {
        fFadeFactor*=fOfCenterFadeFactor;
      }

      FLOAT fSizeIFactor = fScreenSizeI;
      if (olf.olf_ulFlags&OLF_FADESIZE) {
        fSizeIFactor *= fFadeFactor; 
      }
      FLOAT fIntensityFactor = olf.olf_fLightAmplification;
      if (olf.olf_ulFlags&OLF_FADEINTENSITY) {
        fIntensityFactor *= fFadeFactor; 
      }

      // calculate flare size
      FLOAT fSizeI = olf.olf_fSizeIOverScreenSizeI*fSizeIFactor;
      FLOAT fSizeJ = olf.olf_fSizeJOverScreenSizeI*fSizeIFactor;

      // skip if this flare is invisible
      if( fIntensityFactor<0.01f || fSizeI<2.0f || fSizeJ<2.0f) continue;
      
      // determine color
      FLOAT fThisR  = (ubR + (FLOAT(ubI)-ubR)*olf.olf_fLightDesaturation)*fIntensityFactor;
      FLOAT fThisG  = (ubG + (FLOAT(ubI)-ubG)*olf.olf_fLightDesaturation)*fIntensityFactor;
      FLOAT fThisB  = (ubB + (FLOAT(ubI)-ubB)*olf.olf_fLightDesaturation)*fIntensityFactor;
      UBYTE ubThisR = (UBYTE) (Min( fThisR, 255.0f));
      UBYTE ubThisG = (UBYTE) (Min( fThisG, 255.0f));
      UBYTE ubThisB = (UBYTE) (Min( fThisB, 255.0f));
      COLOR colBlending = RGBToColor( ubThisR,ubThisG,ubThisB);

      // render the flare
      re_pdpDrawPort->RenderLensFlare(
        &olf.olf_toTexture, 
        fLightI+olf.olf_fReflectionPosition*fReflectionDirI,
        fLightJ+olf.olf_fReflectionPosition*fReflectionDirJ,
        fSizeI, fSizeJ,
        olf.olf_aRotationFactor*fIPositionFactor,
        colBlending);
    }} // for each flare in the lens flare effect

    // if screen glare is on
    const CLensFlareType &lft = *lfi.lfi_plsLightSource->ls_plftLensFlare;
    const FLOAT fGlearCompression = lft.lft_fGlareCompression;
    if( gfx_iLensFlareQuality>2
     && _wrpWorldRenderPrefs.wrp_lftLensFlares >= CWorldRenderPrefs::LFT_REFLECTIONS_AND_GLARE
     && lft.lft_fGlareIntensity>0.01f) {
      // calculate glare factor for current position
      const FLOAT fIntensity = IntensityAtDistance( lfi.lfi_plsLightSource->ls_rFallOff*lft.lft_fGlareFallOffFactor,
                                              lfi.lfi_plsLightSource->ls_rHotSpot*lft.lft_fGlareFallOffFactor,
                                              lfi.lfi_fDistance);
      const FLOAT fCenterFactor = (1-fOfCenterFadeFactor);
      const FLOAT fGlare = lft.lft_fGlareIntensity*fIntensity
                   * (exp(1/(1+fGlearCompression*fCenterFactor*fCenterFactor)) -1) / (exp(1.0f)-1.0f);
      const ULONG ulGlareA = ClampUp( (ULONG) NormFloatToByte(fGlare), (ULONG) 255);
      // if there is any relevant glare
      if( ulGlareA>1) {
        // calculate glare color
        FLOAT fGlareR = (ubR + (FLOAT(ubI)-ubR) *lft.lft_fGlareDesaturation);
        FLOAT fGlareG = (ubG + (FLOAT(ubI)-ubG) *lft.lft_fGlareDesaturation);
        FLOAT fGlareB = (ubB + (FLOAT(ubI)-ubB) *lft.lft_fGlareDesaturation);
        FLOAT fMax = Max( fGlareR, Max(fGlareG, fGlareB));
        FLOAT fBrightFactor = 255.0f/fMax;
        fGlareR *= fBrightFactor;
        fGlareG *= fBrightFactor;
        fGlareB *= fBrightFactor;
        ULONG ulGlareR = ClampUp( FloatToInt(fGlareR), (SLONG)255);
        ULONG ulGlareG = ClampUp( FloatToInt(fGlareG), (SLONG)255);
        ULONG ulGlareB = ClampUp( FloatToInt(fGlareB), (SLONG)255);
        // add the glare to screen blending
        re_pdpDrawPort->dp_ulBlendingRA += ulGlareR*ulGlareA;
        re_pdpDrawPort->dp_ulBlendingGA += ulGlareG*ulGlareA;
        re_pdpDrawPort->dp_ulBlendingBA += ulGlareB*ulGlareA;
        re_pdpDrawPort->dp_ulBlendingA  += ulGlareA;
      }
    }
  }}
}
