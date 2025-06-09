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

#define OFFSET_DN (0.0625f)

#define CStaticArray_sa_Count 0
#define CStaticArray_sa_Array 4

#define CDynamicArray_da_Pointers    12
#define CDynamicArray_da_Count       16
#define CDynamicStackArray_da_ctUsed 24

#define sizeof_CWorkingVertex 32
#define sizeof_CWorkingEdge   32

static FLOATaabbox2D *pfbbClipBox;
static PIX pixCurrentScanJ;
static PIX pixBottomScanJ;
static const FLOAT f65536=65536.0f;

/////////////////////////////////////////////////////////////////////
// Functions for support of transformed caches

static FLOAT fCenterI, fCenterJ, fRatioI, fRatioJ;
static FLOAT fNearClipDistance, fooNearClipDistance;
static FLOAT fFarClipDistance,  fooFarClipDistance;
static FLOAT fooNearRatioI, fooNearRatioJ;
static FLOAT fooFarRatioI,  fooFarRatioJ;


static CRenderer *_preThis;

// check if a sector is inside view frustum
__forceinline INDEX CRenderer::IsSectorVisible(CBrush3D &br, CBrushSector &bsc)
{
  // project the bounding sphere of sector bbox to view
  FLOAT3D vCenter;
  CProjection3D *ppr;
  if (re_bBackgroundEnabled && (br.br_penEntity->en_ulFlags & ENF_BACKGROUND)) {
    ppr = re_prBackgroundProjection;
  } else {
    ppr = re_prProjection;
  }
  ppr->PreClip(bsc.bsc_boxBoundingBox.Center(), vCenter);
  FLOAT fR = bsc.bsc_boxBoundingBox.Size().Length()/2.0f;

  // if the sector bounding sphere is inside view frustum
  INDEX iFrustumTest = ppr->TestSphereToFrustum( vCenter, fR);
  if( iFrustumTest==0) { // if test was indeterminate
    // create oriented box and test it to frustum
    FLOATobbox3D boxEntity( bsc.bsc_boxBoundingBox, ppr->pr_TranslationVector, ppr->pr_ViewerRotationMatrix);
    iFrustumTest = ppr->TestBoxToFrustum(boxEntity);
  }
  // done
  return iFrustumTest;
}


/* Transform vertices in one sector before clipping. */
void CRenderer::PreClipVertices(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_TRANSFORMVERTICES);

  const FLOATmatrix3D &m = re_pbrCurrent->br_prProjection->pr_RotationMatrix;
  const FLOAT3D       &v = re_pbrCurrent->br_prProjection->pr_TranslationVector;
  re_pbscCurrent->bsc_ivvx0 = re_iViewVx0 = re_avvxViewVertices.Count();
  INDEX ctvx = re_pbscCurrent->bsc_awvxVertices.Count(); 
  CViewVertex *avvx = re_avvxViewVertices.Push(ctvx);
  // for each vertex in sector 
  for( INDEX ivx=0; ivx<ctvx; ivx++)
  {
    const CWorkingVertex &wvx = re_pbscCurrent->bsc_awvxVertices[ivx];
    // transform it to view space
    const FLOAT fx = wvx.wvx_vRelative(1);
    const FLOAT fy = wvx.wvx_vRelative(2);
    const FLOAT fz = wvx.wvx_vRelative(3);
    avvx[ivx].vvx_vView(1) = fx*m(1, 1)+fy*m(1, 2)+fz*m(1, 3)+v(1);
    avvx[ivx].vvx_vView(2) = fx*m(2, 1)+fy*m(2, 2)+fz*m(2, 3)+v(2);
    avvx[ivx].vvx_vView(3) = fx*m(3, 1)+fy*m(3, 2)+fz*m(3, 3)+v(3);
    // clear the outcode initially
    avvx[ivx].vvx_ulOutcode = 0;
  }
  _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_TRANSFORMEDVERTICES, ctvx);
  _pfRenderProfile.IncrementTimerAveragingCounter(CRenderProfile::PTI_TRANSFORMVERTICES, ctvx);

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_TRANSFORMVERTICES);
}

/* Transform planes in one sector before clipping. */
void CRenderer::PreClipPlanes(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_TRANSFORMPLANES);

  INDEX ctpl =re_pbscCurrent->bsc_awplPlanes.Count(); 
  CProjection3D *ppr = &*re_pbrCurrent->br_prProjection;
  const FLOATmatrix3D &m = ppr->pr_RotationMatrix;
  const FLOAT3D       &v = ppr->pr_TranslationVector;

  // if the projection is perspective
  if (re_pbrCurrent->br_prProjection.IsPerspective()) {

    // for each plane in sector 
    for(INDEX ipl=0; ipl<ctpl; ipl++){
      // transform it to view space
      CWorkingPlane &wpl = re_pbscCurrent->bsc_awplPlanes[ipl];
      const FLOAT fx = wpl.wpl_plRelative(1);
      const FLOAT fy = wpl.wpl_plRelative(2);
      const FLOAT fz = wpl.wpl_plRelative(3);
      //const FLOAT fd = wpl.wpl_plRelative.Distance();
      wpl.wpl_plView(1) = fx*m(1, 1)+fy*m(1, 2)+fz*m(1, 3);
      wpl.wpl_plView(2) = fx*m(2, 1)+fy*m(2, 2)+fz*m(2, 3);
      wpl.wpl_plView(3) = fx*m(3, 1)+fy*m(3, 2)+fz*m(3, 3);
      wpl.wpl_plView.Distance() = 
        wpl.wpl_plView(1)*v(1)+
        wpl.wpl_plView(2)*v(2)+
        wpl.wpl_plView(3)*v(3)+
        wpl.wpl_plRelative.Distance();
      // test if the plane is visible
      wpl.wpl_bVisible = (wpl.wpl_plView.Distance() < -0.01f);
    }
  // if the projection is not perspective
  } else {
    // !!!! speed this up for other projections too ?
    // for each plane in sector 
    for(INDEX ipl=0; ipl<ctpl; ipl++){
      // transform it to view space
      CWorkingPlane &wpl = re_pbscCurrent->bsc_awplPlanes[ipl];
      ppr->Project(wpl.wpl_plRelative, wpl.wpl_plView);
      // test if the plane is visible
      wpl.wpl_bVisible = ppr->IsViewerPlaneVisible(wpl.wpl_plView);
    }
  }

  // for each plane
  {for(INDEX ipl=0; ipl<ctpl; ipl++){
    CWorkingPlane &wpl = re_pbscCurrent->bsc_awplPlanes[ipl];
    // make gradients without fog
    ppr->MakeOoKGradient(wpl.wpl_plView, wpl.wpl_pgOoK);
    // transform it to view space
    const FLOAT fxO = wpl.wpl_mvRelative.mv_vO(1);
    const FLOAT fyO = wpl.wpl_mvRelative.mv_vO(2);
    const FLOAT fzO = wpl.wpl_mvRelative.mv_vO(3);
    const FLOAT fxU = wpl.wpl_mvRelative.mv_vU(1);
    const FLOAT fyU = wpl.wpl_mvRelative.mv_vU(2);
    const FLOAT fzU = wpl.wpl_mvRelative.mv_vU(3);
    const FLOAT fxV = wpl.wpl_mvRelative.mv_vV(1);
    const FLOAT fyV = wpl.wpl_mvRelative.mv_vV(2);
    const FLOAT fzV = wpl.wpl_mvRelative.mv_vV(3);
    wpl.wpl_mvView.mv_vO(1) = fxO*m(1, 1)+fyO*m(1, 2)+fzO*m(1, 3)+v(1);
    wpl.wpl_mvView.mv_vO(2) = fxO*m(2, 1)+fyO*m(2, 2)+fzO*m(2, 3)+v(2);
    wpl.wpl_mvView.mv_vO(3) = fxO*m(3, 1)+fyO*m(3, 2)+fzO*m(3, 3)+v(3);
    wpl.wpl_mvView.mv_vU(1) = fxU*m(1, 1)+fyU*m(1, 2)+fzU*m(1, 3);
    wpl.wpl_mvView.mv_vU(2) = fxU*m(2, 1)+fyU*m(2, 2)+fzU*m(2, 3);
    wpl.wpl_mvView.mv_vU(3) = fxU*m(3, 1)+fyU*m(3, 2)+fzU*m(3, 3);
    wpl.wpl_mvView.mv_vV(1) = fxV*m(1, 1)+fyV*m(1, 2)+fzV*m(1, 3);
    wpl.wpl_mvView.mv_vV(2) = fxV*m(2, 1)+fyV*m(2, 2)+fzV*m(2, 3);
    wpl.wpl_mvView.mv_vV(3) = fxV*m(3, 1)+fyV*m(3, 2)+fzV*m(3, 3);
  }}

  _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_TRANSFORMEDPLANES, ctpl);
  _pfRenderProfile.IncrementTimerAveragingCounter(CRenderProfile::PTI_TRANSFORMPLANES, ctpl);

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_TRANSFORMPLANES);
}

// project vertices in current sector after clipping
void CRenderer::PostClipVertices(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_PROJECTVERTICES);
  //const FLOATmatrix3D &m = re_pbrCurrent->br_prProjection->pr_RotationMatrix;
  //const FLOAT3D       &v = re_pbrCurrent->br_prProjection->pr_TranslationVector;

  // if the projection is perspective
  if (re_pbrCurrent->br_prProjection.IsPerspective()) {
    CPerspectiveProjection3D &prPerspective = (CPerspectiveProjection3D &)*re_pbrCurrent->br_prProjection;

    fCenterI = prPerspective.pr_ScreenCenter(1);
    fCenterJ = prPerspective.pr_ScreenCenter(2);
    fRatioI = prPerspective.ppr_PerspectiveRatios(1);
    fRatioJ = prPerspective.ppr_PerspectiveRatios(2);

    // for each active view vertex
    INDEX iVxTop = re_avvxViewVertices.Count();
    for(INDEX ivx = re_iViewVx0; ivx<iVxTop; ivx++) {
      CViewVertex &vvx = re_avvxViewVertices[ivx];
      // transform it to view space
      FLOAT fooz = 1.0f/vvx.vvx_vView(3);
      vvx.vvx_fI = fCenterI+vvx.vvx_vView(1)*fRatioI*fooz;
      vvx.vvx_fJ = fCenterJ-vvx.vvx_vView(2)*fRatioJ*fooz;
    }

  // if the projection is not perspective
  } else {
    // !!!! speed this up for other projections too ?
    // for each active view vertex
    INDEX iVxTop = re_avvxViewVertices.Count();
    for(INDEX ivx = re_iViewVx0; ivx<iVxTop; ivx++) {
      CViewVertex &vvx = re_avvxViewVertices[ivx];
      // transform it to view space
      FLOAT3D v;
      re_pbrCurrent->br_prProjection->PostClip(vvx.vvx_vView, v);
      vvx.vvx_fI = v(1);
      vvx.vvx_fJ = v(2);
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_PROJECTVERTICES);
}


// setup fog/haze for a sector
void CRenderer::SetupFogAndHaze(void)
{
  CBrush3D &br = *re_pbrCurrent;
  CBrushSector &bsc = *re_pbscCurrent;
  if( _bMultiPlayer) gfx_bRenderFog = 1; // must render fog in multiplayer mode!

  // if the sector is not part of a zoning brush
  if (!(br.br_penEntity->en_ulFlags&ENF_ZONING)) {
    // do nothing
    return;
  }

  // if fog is enabled
  re_bCurrentSectorHasFog = FALSE;
  if( _wrpWorldRenderPrefs.wrp_bFogOn && gfx_bRenderFog)
  { // if the sector has fog
    CFogParameters fp;
    if( bsc.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->GetFog(bsc.GetFogType(), fp)) {
      // activate fog if not already active
      if( !_fog_bActive) {
        StartFog( fp, br.br_prProjection->pr_vViewerPosition,
                      br.br_prProjection->pr_ViewerRotationMatrix);
      }
      // mark that current sector has fog
      re_bCurrentSectorHasFog = TRUE;
    }
  }

  // if haze is enabled
  re_bCurrentSectorHasHaze = FALSE;
  if( _wrpWorldRenderPrefs.wrp_bHazeOn && gfx_bRenderFog)
  { // if the sector has haze
    CHazeParameters hp;
    FLOAT3D vViewDir;
    FLOATmatrix3D &mAbsToView = bsc.bsc_pbmBrushMip->bm_pbrBrush->br_prProjection->pr_ViewerRotationMatrix;
    vViewDir(1) = -mAbsToView(3, 1);
    vViewDir(2) = -mAbsToView(3, 2);
    vViewDir(3) = -mAbsToView(3, 3);
    if( bsc.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->GetHaze(bsc.GetHazeType(), hp, vViewDir)) {
      // if viewer is not in haze
      if( !re_bViewerInHaze) {
        // if viewer is in this sector
        if( bsc.bsc_bspBSPTree.TestSphere(re_vdViewSphere, 0.01f)>=0) {
          // mark that viewer is in haze
          re_bViewerInHaze = TRUE;
        }
        // if there is a viewer
        else if( re_penViewer!=NULL) { 
          // check rest of sectors the viewer is in
          {FOREACHSRCOFDST( re_penViewer->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
            CHazeParameters hpDummy;
            if( pbsc->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->GetHaze( pbsc->GetHazeType(), hpDummy, vViewDir)) {
              // if viewer is in this sector
              if( pbsc->bsc_bspBSPTree.TestSphere(re_vdViewSphere, 0.01)>=0) {
                // mark that viewer is in haze
                re_bViewerInHaze = TRUE;
                break;
              }
            }
          ENDFOR}
        }
      } 
      // if viewer is in haze, or haze can be viewed from outside
      if( re_bViewerInHaze || (hp.hp_ulFlags&HPF_VISIBLEFROMOUTSIDE)) {
        // activate haze if not already active
        if( !_haze_bActive) {
          StartHaze( hp, br.br_prProjection->pr_vViewerPosition,
                         br.br_prProjection->pr_ViewerRotationMatrix);
        }
        // mark that current sector has haze
        re_bCurrentSectorHasHaze = TRUE;
      }
    }
  }
}


/*
 * Add an edge to add list of its top scanline.
 */
void CRenderer::AddEdgeToAddAndRemoveLists(CScreenEdge &sed)
{
  _pfRenderProfile.IncrementTimerAveragingCounter(CRenderProfile::PTI_ADDEDGETOADDLIST, 1);
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDEDGETOADDLIST);

  // add it to the remove list at its bottom scan line
  ASSERT(sed.sed_pixBottomJ-1-re_pixTopScanLineJ < re_ctScanLines);
  INDEX iBottomLine = sed.sed_pixBottomJ-1-re_pixTopScanLineJ;
  sed.sed_psedNextRemove = re_apsedRemoveFirst[iBottomLine];
  re_apsedRemoveFirst[iBottomLine] = &sed;

  // search all edges in the add list of top scan line of this edge
  INDEX iTopLine = sed.sed_pixTopJ-re_pixTopScanLineJ;
  CListNode *plnInList = re_alhAddLists[iTopLine].lh_Head;
  re_actAddCounts[iTopLine]++;
  SLONG slIThis = sed.sed_xI.slHolder;

  ASSERT(sed.sed_xI > FIX16_16(re_fbbClipBox.Min()(1)-SENTINELEDGE_EPSILON));
  ASSERT(sed.sed_xI < FIX16_16(re_fbbClipBox.Max()(1)+SENTINELEDGE_EPSILON));

  SLONG slIInList;
  while(plnInList->ln_Succ!=NULL) {
    slIInList = 
      ((CAddEdge*)((UBYTE*)plnInList-_offsetof(CAddEdge, ade_lnInAdd))) -> ade_xI.slHolder;
    // if the edge in list is right of the one to add
    if (slIInList>slIThis) {
      // stop searching
      break;
    }
    plnInList = plnInList->ln_Succ;
  }
  // add it to add list
  CAddEdge &ade = re_aadeAddEdges.Push();
  ade = CAddEdge(&sed);
  CListNode *plnThis  = &ade.ade_lnInAdd;
  CListNode *plnAfter  = plnInList;
  CListNode *plnBefore = plnInList->ln_Pred;
  plnThis->ln_Succ = plnAfter;
  plnThis->ln_Pred = plnBefore;
  plnBefore->ln_Succ = plnThis;
  plnAfter->ln_Pred = plnThis;
  // mark that it is added
  sed.sed_bAdded = TRUE;
  ASSERT(re_actAddCounts[iTopLine]==re_alhAddLists[iTopLine].Count());

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDEDGETOADDLIST);
}

/*
 * Make a screen edge from two vertices doing 2D clipping.
 */
inline void CRenderer::MakeScreenEdge(
  CScreenEdge &sed, FLOAT fI0, FLOAT fJ0, FLOAT fI1, FLOAT fJ1)
{
  // mark both vertices as not clipped
  _pfRenderProfile.IncrementTimerAveragingCounter(CRenderProfile::PTI_MAKESCREENEDGE, 1);
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_MAKESCREENEDGE);
  enum LineDirectionType ldtDirection;

  // bias edges towards each others - fix against generating extra trapezoids
  if (fJ0<fJ1) {
    fI0-=re_fEdgeOffsetI;
    fI1-=re_fEdgeOffsetI;
  } else {
    fI0+=re_fEdgeOffsetI;
    fI1+=re_fEdgeOffsetI;
  }

  // clamp vertices left and right
  if (fI0<re_fbbClipBox.Min()(1)) {
    fI0 = re_fbbClipBox.Min()(1);
  }
  if (fI1<re_fbbClipBox.Min()(1)) {
    fI1 = re_fbbClipBox.Min()(1);
  }
  if (fI0>re_fbbClipBox.Max()(1)) {
    fI0 = re_fbbClipBox.Max()(1);
  }
  if (fI1>re_fbbClipBox.Max()(1)) {
    fI1 = re_fbbClipBox.Max()(1);
  }

  FLOAT fDJ=fJ1-fJ0;
  FLOAT fDI=fI1-fI0;
  
  FLOAT fDIoDJ = fDI/fDJ;

  // mark edge as descending
  ldtDirection = LDT_DESCENDING;

  // if vertex 0 is below vertex 1
  if ((SLONG&)fDJ<0 ) { // fJ0>fJ1
    // mark edge as ascending
    ldtDirection = LDT_ASCENDING;
    Swap(fI0, fI1);
    Swap(fJ0, fJ1);
    fDI = -fDI;
    fDJ = -fDJ;
  }

  ASSERT(fI0>=re_fbbClipBox.Min()(1));
  ASSERT(fI1>=re_fbbClipBox.Min()(1));
  ASSERT(fI0>=-0.5f);
  ASSERT(fI1>=-0.5f);
  ASSERT(fI0<=re_fbbClipBox.Max()(1));
  ASSERT(fI1<=re_fbbClipBox.Max()(1));

  // set line direction
  sed.sed_ldtDirection = ldtDirection;

  // edge has polygon initially
  sed.sed_pspo = NULL;
  // edge is not linked to add and remove lists initially
  sed.sed_bAdded = FALSE;
  sed.sed_psedNextRemove = NULL;

  // if bottom vertex is above screen top or top vertex is below screen bottom
  FLOAT fDJ1Up = fJ1-re_fbbClipBox.Min()(2);
  FLOAT fDJ0Dn = re_fbbClipBox.Max()(2)-fJ0;
  if ((SLONG&)(fDJ1Up)<0 || (SLONG&)(fDJ0Dn)<0) {
    // generate dummy horizontal screen edge
    sed.sed_pixTopJ    = (PIX) 0;
    sed.sed_pixBottomJ = (PIX) 0;
    sed.sed_ldtDirection = LDT_HORIZONTAL;

  // otherwise
  } else {
    // calculate edge slope and convert it to fixed integer
    sed.sed_xIStep = (FIX16_16)fDIoDJ;
    // convert J coordinates to integers
    sed.sed_pixTopJ    = PIXCoord(fJ0);
    sed.sed_pixBottomJ = PIXCoord(fJ1);

    // if the edge bottom is below screen bottom
    if (sed.sed_pixTopJ<re_pixCurrentScanJ) {
      // set it to bottom
      sed.sed_pixTopJ = re_pixCurrentScanJ;
    }

    // if the edge bottom is below screen bottom
    if (sed.sed_pixBottomJ>re_pixBottomScanLineJ) {
      // set it to bottom
      sed.sed_pixBottomJ = re_pixBottomScanLineJ;
    }

    // make fixed integer representation of top I coordinate with correction
    sed.sed_xI = (FIX16_16) (fI0 + ((FLOAT)sed.sed_pixTopJ-fJ0) * fDIoDJ );
  }

  ASSERT( (sed.sed_xI > FIX16_16(-1.0f)
       && sed.sed_xI < FIX16_16(re_fbbClipBox.Max()(1) + SENTINELEDGE_EPSILON))
       || (sed.sed_pixTopJ >= sed.sed_pixBottomJ));

  // return the screen edge
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_MAKESCREENEDGE);
}

// add screen edges for all polygons in current sector
void CRenderer::AddScreenEdges(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDSCREENEDGES);
  // for each polygon
  INDEX ispo0 = re_pbscCurrent->bsc_ispo0;
  INDEX ispoTop = re_pbscCurrent->bsc_ispo0+re_pbscCurrent->bsc_ctspo;
  for(INDEX ispo = ispo0; ispo<ispoTop; ispo++) {
    CScreenPolygon &spo = re_aspoScreenPolygons[ispo];

    // if polygon has no edges
    if (spo.spo_ctEdgeVx==0) {
      // skip it
      continue;
    }

    // create as much screen edges as needed
    spo.spo_ised0 = re_asedScreenEdges.Count();
    spo.spo_ctsed = spo.spo_ctEdgeVx/2;
    re_asedScreenEdges.Push(spo.spo_ctsed);
    _sfStats.IncrementCounter(CStatForm::SCI_POLYGONEDGES, spo.spo_ctsed);

    // for each edge
    INDEX ivxTop = spo.spo_iEdgeVx0+spo.spo_ctEdgeVx;
    INDEX ised = spo.spo_ised0;
    for(INDEX ivx=spo.spo_iEdgeVx0; ivx<ivxTop; ivx+=2) {
      INDEX ivx0 = re_aiEdgeVxMain[ivx+0];
      INDEX ivx1 = re_aiEdgeVxMain[ivx+1];
      if (spo.spo_ubDirectionFlags) {
        Swap(ivx0, ivx1);
      }
      CScreenEdge &sed = re_asedScreenEdges[ised];
      // take vertices
      CViewVertex &vvx0 = re_avvxViewVertices[ivx0];
      CViewVertex &vvx1 = re_avvxViewVertices[ivx1];
      // make screen edge
      MakeScreenEdge(sed, vvx0.vvx_fI, vvx0.vvx_fJ, vvx1.vvx_fI, vvx1.vvx_fJ);
      // if it crosses any lines and is not past
      if (sed.sed_pixTopJ<sed.sed_pixBottomJ && sed.sed_pixBottomJ>re_pixCurrentScanJ) {
        // set its polygon
        sed.sed_pspo = &spo;
        // add it
        AddEdgeToAddAndRemoveLists(sed);
      }

      // go to next edge
      ised++;
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDSCREENEDGES);
}

// set scene rendering parameters for one polygon texture
void CRenderer::SetOneTextureParameters(CBrushPolygon &bpo, ScenePolygon &spo, INDEX iLayer)
{
  spo.spo_aptoTextures[iLayer] = NULL;
  CTextureData *ptd = (CTextureData *)bpo.bpo_abptTextures[iLayer].bpt_toTexture.GetData();

  // if there is no texture or it should not be shown
  if (ptd==NULL || !_wrpWorldRenderPrefs.wrp_abTextureLayers[iLayer]) {
    // do nothing
    return;
  }

  CWorkingPlane &wpl  = *bpo.bpo_pbplPlane->bpl_pwplWorking;
  // set texture and its parameters
  spo.spo_aptoTextures[iLayer] = &bpo.bpo_abptTextures[iLayer].bpt_toTexture;

  // get texture blending type
  CTextureBlending &tb = re_pwoWorld->wo_atbTextureBlendings[bpo.bpo_abptTextures[iLayer].s.bpt_ubBlend];

  // set texture blending flags
  ASSERT( BPTF_CLAMPU==STXF_CLAMPU && BPTF_CLAMPV==STXF_CLAMPV && BPTF_AFTERSHADOW==STXF_AFTERSHADOW);
  spo.spo_aubTextureFlags[iLayer] = 
     (bpo.bpo_abptTextures[iLayer].s.bpt_ubFlags & (BPTF_CLAMPU|BPTF_CLAMPV|BPTF_AFTERSHADOW))
   | (tb.tb_ubBlendingType);
  if( bpo.bpo_abptTextures[iLayer].s.bpt_ubFlags & BPTF_REFLECTION) spo.spo_aubTextureFlags[iLayer] |= STXF_REFLECTION;

  // set texture blending color
  spo.spo_acolColors[iLayer] = MulColors( bpo.bpo_abptTextures[iLayer].s.bpt_colColor, tb.tb_colMultiply);

  // if texture should be not transformed
  INDEX iTransformation = bpo.bpo_abptTextures[iLayer].s.bpt_ubScroll;
  if( iTransformation==0)
  {
    // if texture is wrapped on both axes
    if( (bpo.bpo_abptTextures[iLayer].s.bpt_ubFlags&(BPTF_CLAMPU|BPTF_CLAMPV))==0)
    { // make a mapping adjusted for texture wrapping
      const MEX mexMaskU = ptd->GetWidth()  -1;
      const MEX mexMaskV = ptd->GetHeight() -1;
      CMappingDefinition mdTmp = bpo.bpo_abptTextures[iLayer].bpt_mdMapping;
      mdTmp.md_fUOffset = (FloatToInt(mdTmp.md_fUOffset*1024.0f) & mexMaskU) /1024.0f;
      mdTmp.md_fVOffset = (FloatToInt(mdTmp.md_fVOffset*1024.0f) & mexMaskV) /1024.0f;
      const FLOAT3D vOffset = wpl.wpl_plView.ReferencePoint() - wpl.wpl_mvView.mv_vO;
      const FLOAT fS = vOffset % wpl.wpl_mvView.mv_vU;
      const FLOAT fT = vOffset % wpl.wpl_mvView.mv_vV;
      const FLOAT fU = fS*mdTmp.md_fUoS + fT*mdTmp.md_fUoT + mdTmp.md_fUOffset;
      const FLOAT fV = fS*mdTmp.md_fVoS + fT*mdTmp.md_fVoT + mdTmp.md_fVOffset;
      mdTmp.md_fUOffset += (FloatToInt(fU*1024.0f) & ~mexMaskU) /1024.0f;
      mdTmp.md_fVOffset += (FloatToInt(fV*1024.0f) & ~mexMaskV) /1024.0f;
      // make texture mapping vectors from default vectors of the plane
      mdTmp.MakeMappingVectors( wpl.wpl_mvView, spo.spo_amvMapping[iLayer]);
    }
    // if texture is clamped
    else {
      // just make texture mapping vectors from default vectors of the plane
      bpo.bpo_abptTextures[iLayer].bpt_mdMapping.MakeMappingVectors( wpl.wpl_mvView, spo.spo_amvMapping[iLayer]);
    }
  }
  // if texture should be transformed
  else {
    // make mapping vectors as normal and then transform them
    CMappingDefinition &mdBase = bpo.bpo_abptTextures[iLayer].bpt_mdMapping;
    CMappingDefinition &mdScroll = re_pwoWorld->wo_attTextureTransformations[iTransformation].tt_mdTransformation;
    CMappingVectors mvTmp;
    mdBase.MakeMappingVectors( wpl.wpl_mvView, mvTmp);
    mdScroll.TransformMappingVectors( mvTmp, spo.spo_amvMapping[iLayer]);
  }
}

/*
 * Make a screen polygon for a brush polygon
 */
CScreenPolygon *CRenderer::MakeScreenPolygon(CBrushPolygon &bpo)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_MAKESCREENPOLYGON);
  _pfRenderProfile.IncrementTimerAveragingCounter(CRenderProfile::PTI_MAKESCREENPOLYGON, 1);
  // create a new screen polygon
  CScreenPolygon &spo = re_aspoScreenPolygons.Push();
  ScenePolygon  &sppo = spo.spo_spoScenePolygon;
  bpo.bpo_pspoScreenPolygon = &spo;
  CBrush3D &br = *re_pbrCurrent;
  CBrushSector &bsc = *re_pbscCurrent;
  sppo.spo_pvPolygon = &bpo;
  sppo.spo_iVtx0 = 0;
  sppo.spo_ctVtx = 0;
  sppo.spo_piElements = NULL;
  sppo.spo_ctElements = 0;
  spo.spo_bActive = TRUE;
  spo.spo_ubSpanAdded = 0;
  // link with brush polygon
  spo.spo_pbpoBrushPolygon = &bpo;
  spo.spo_ubIllumination = bpo.bpo_bppProperties.bpp_ubIlluminationType;

  // if this is a field brush
  if (br.br_pfsFieldSettings!=NULL) {
    // set the polygon up to render as a field brush
    bpo.bpo_abptTextures[0].bpt_toTexture.SetData(br.br_pfsFieldSettings->fs_toTexture.GetData());
    bpo.bpo_abptTextures[0].s.bpt_colColor = br.br_pfsFieldSettings->fs_colColor;
    bpo.bpo_abptTextures[0].s.bpt_ubScroll = 0;
    bpo.bpo_abptTextures[0].s.bpt_ubFlags = 0;
    bpo.bpo_abptTextures[0].s.bpt_ubBlend = BPT_BLEND_BLEND;
    bpo.bpo_abptTextures[1].bpt_toTexture.SetData(NULL);
    bpo.bpo_abptTextures[2].bpt_toTexture.SetData(NULL);
    bpo.bpo_ulFlags = BPOF_PORTAL|BPOF_RENDERASPORTAL|BPOF_FULLBRIGHT|BPOF_TRANSLUCENT|BPOF_DETAILPOLYGON;
  }

  // just copy depth gradient from plane
  CWorkingPlane &wpl  = *bpo.bpo_pbplPlane->bpl_pwplWorking;
  spo.spo_pgOoK = wpl.wpl_pgOoK;
  spo.spo_spoScenePolygon.spo_cColor = ColorForPolygons(bpo.bpo_colColor, bsc.bsc_colColor);

  // set polygon shadow
  sppo.spo_psmShadowMap = NULL;
  if(_wrpWorldRenderPrefs.wrp_shtShadows != CWorldRenderPrefs::SHT_NONE
    &&!(bpo.bpo_ulFlags&BPOF_FULLBRIGHT)) {
    sppo.spo_psmShadowMap = &bpo.bpo_smShadowMap;
  }

  // make texture mapping vectors from default vectors of the plane
  CMappingDefinition mdShadow;
  mdShadow.md_fUoS = 1;
  mdShadow.md_fUoT = 0;
  mdShadow.md_fVoS = 0;
  mdShadow.md_fVoT = 1;
  mdShadow.md_fUOffset = -bpo.bpo_smShadowMap.sm_mexOffsetX/1024.0f;
  mdShadow.md_fVOffset = -bpo.bpo_smShadowMap.sm_mexOffsetY/1024.0f;
  mdShadow.MakeMappingVectors(wpl.wpl_mvView, sppo.spo_amvMapping[3]);
  // adjust shadow blending type
  CTextureBlending &tbShadow = re_pwoWorld->wo_atbTextureBlendings[bpo.bpo_bppProperties.bpp_ubShadowBlend];
  sppo.spo_aubTextureFlags[3] = STXF_CLAMPU|STXF_CLAMPV|tbShadow.tb_ubBlendingType;
  // set shadow blending color
  sppo.spo_acolColors[3] = MulColors(bpo.bpo_colShadow, tbShadow.tb_colMultiply);

  // set textures for the polygon 
  SetOneTextureParameters( bpo, sppo, 0);
  SetOneTextureParameters( bpo, sppo, 1);
  SetOneTextureParameters( bpo, sppo, 2);

  // clear polygon flags
  sppo.spo_ulFlags = 0;
  if (_wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_TEXTURE) {
    sppo.spo_aptoTextures[0] = NULL;
    sppo.spo_aptoTextures[1] = NULL;
    sppo.spo_aptoTextures[2] = NULL;
  }

  // if the sector has fog
  if (re_bCurrentSectorHasFog) {
    // mark for rendering with fog
    sppo.spo_ulFlags |= SPOF_RENDERFOG;
  }
  // if the sector has haze
  if (re_bCurrentSectorHasHaze) {
    // mark polygon for haze rendering
    sppo.spo_ulFlags |= SPOF_RENDERHAZE;
  }

  // if the polygon is selected
  BOOL bSelected = PolygonIsSelected( bpo, br, bsc);
  if (bSelected) {
    // mark this polygon for drawing as selected
    sppo.spo_ulFlags |= SPOF_SELECTED;
  }

  // if the polygon is transparent
  if( bpo.bpo_ulFlags & BPOF_TRANSPARENT) {
    // mark that this polygon will need alpha keying
    sppo.spo_ulFlags |= SPOF_TRANSPARENT;
  }

  BOOL bBackgroundPolygon = re_bBackgroundEnabled && 
    (bpo.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity->en_ulFlags&ENF_BACKGROUND);
  // if this is a backfill polygon or illumination polygon for rendering lights
  if (bBackgroundPolygon
    ||(re_ubLightIllumination!=0 && re_ubLightIllumination==bpo.bpo_bppProperties.bpp_ubIlluminationType)) {
    // mark this polygon as backlight (for shadow rendering)
    sppo.spo_ulFlags |= SPOF_BACKLIGHT;
    // adjust gradients used for sorting to be just before the far sentinel
    spo.spo_pgOoK.Add(-2.0f);
  }

  // init as not in stack
  ASSERT(!spo.spo_lnInStack.IsLinked());
  spo.spo_iInStack = 0;
  spo.spo_psedSpanStart = NULL;

  // eventually adjust polygon opacity depending on brush entity variable
  BOOL bForceTraslucency = FALSE;
  const FLOAT fOpacity = br.br_penEntity->GetOpacity();
  if( fOpacity<1)
  { // better to hold opacity in integer
    const SLONG slOpacity = NormFloatToByte(fOpacity);
    // for all texture layers (not shadowmap!)
    for( INDEX i=0; i<3; i++) {
      // if texture is opaque 
      if( (sppo.spo_aubTextureFlags[i] & STXF_BLEND_MASK) == 0) {
        // set it to blend with opaque alpha
        sppo.spo_aubTextureFlags[i] |= STXF_BLEND_ALPHA;
        sppo.spo_acolColors[i] |= CT_AMASK;
      }
      // if texture is blended
      if( sppo.spo_aubTextureFlags[i] & STXF_BLEND_ALPHA) {
        // adjust it's alpha factor
        SLONG slAlpha = (sppo.spo_acolColors[i] & CT_AMASK) >>CT_ASHIFT;
        slAlpha = (slAlpha*slOpacity)>>8;
        sppo.spo_acolColors[i] &= ~CT_AMASK;
        sppo.spo_acolColors[i] |= slAlpha;
      }
    }
    // mark that we need translucency
    bForceTraslucency = TRUE;
  }

  // not translucent by default
  bpo.bpo_ulFlags &= ~BPOF_RENDERTRANSLUCENT;

  // if the polygon is a portal that is either translucent or selected
  if( bForceTraslucency || ((bpo.bpo_ulFlags & BPOF_RENDERASPORTAL)
                        && ((bpo.bpo_ulFlags & BPOF_TRANSLUCENT) || bSelected))) {
    // if not rendering shadows
    if (!re_bRenderingShadows) {
      // mark for rendering as translucent
      bpo.bpo_ulFlags |= BPOF_RENDERTRANSLUCENT;
      // add it to the list of translucent span polygons in this renderer
      if (bBackgroundPolygon) {
        spo.spo_spoScenePolygon.spo_pspoSucc = re_pspoFirstBackgroundTranslucent;
        re_pspoFirstBackgroundTranslucent = &spo.spo_spoScenePolygon;
      } else {
        spo.spo_spoScenePolygon.spo_pspoSucc = re_pspoFirstTranslucent;
        re_pspoFirstTranslucent = &spo.spo_spoScenePolygon;
      }
      // if it is not translucent (ie. it is just plain portal, but selected)
      if( !(bpo.bpo_ulFlags & BPOF_TRANSLUCENT) && !bForceTraslucency) {
        // set its texture for selection
        CModelObject *pmoSelectedPortal = _wrpWorldRenderPrefs.wrp_pmoSelectedPortal;
        if (pmoSelectedPortal!=NULL) {
          sppo.spo_aptoTextures[0] = &pmoSelectedPortal->mo_toTexture;
          sppo.spo_acolColors[0] = C_WHITE|CT_OPAQUE;
          sppo.spo_aubTextureFlags[0] = STXF_BLEND_ALPHA;
        }
        // get its mapping gradients from shadowmap and stretch
        //CWorkingPlane &wpl  = *bpo.bpo_pbplPlane->bpl_pwplWorking;
        sppo.spo_amvMapping[0] = sppo.spo_amvMapping[3];
        FLOAT fStretch = bpo.bpo_boxBoundingBox.Size().Length()/1000;
        sppo.spo_amvMapping[0].mv_vU *= fStretch;
        sppo.spo_amvMapping[0].mv_vV *= fStretch;
      }
    }
  }
  // if the polygon is ordinary wall
  else { 
    // add it to the list of span polygons in this renderer
    if (bBackgroundPolygon) {
      spo.spo_spoScenePolygon.spo_pspoSucc = re_pspoFirstBackground;
      re_pspoFirstBackground = &spo.spo_spoScenePolygon;
    } else {
      spo.spo_spoScenePolygon.spo_pspoSucc = re_pspoFirst;
      re_pspoFirst = &spo.spo_spoScenePolygon;
    }
  }

  // return the screen polygon
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_MAKESCREENPOLYGON);
  return &spo;
}


/* Add a polygon to scene rendering. */
void CRenderer::AddPolygonToScene( CScreenPolygon *pspo)
{
  // if the polygon is not falid or occluder and not selected 
  CBrushPolygon *pbpo = pspo->spo_pbpoBrushPolygon;
  if(pbpo==NULL || ((pbpo->bpo_ulFlags&BPOF_OCCLUDER) && (!(pbpo->bpo_ulFlags&BPOF_SELECTED) ||
      _wrpWorldRenderPrefs.GetSelectionType()!=CWorldRenderPrefs::ST_POLYGONS))) {
    // do not add it to rendering
    return;
  }
  CBrushSector &bsc  = *pbpo->bpo_pbscSector;
  ScenePolygon &sppo = pspo->spo_spoScenePolygon;
  const CViewVertex *pvvx0 = &re_avvxViewVertices[bsc.bsc_ivvx0];
  const INDEX ctVtx = pbpo->bpo_apbvxTriangleVertices.Count();
  sppo.spo_iVtx0    = _avtxScene.Count();
  GFXVertex3 *pvtx  = _avtxScene.Push(ctVtx);

  // find vertex with nearest Z distance while copying vertices
  FLOAT fNearestZ = 123456789.0f;
  for( INDEX i=0; i<ctVtx; i++) {
    CBrushVertex *pbvx = pbpo->bpo_apbvxTriangleVertices[i];
    const INDEX iVtx = bsc.bsc_abvxVertices.Index(pbvx);
    const FLOAT3D &v = pvvx0[iVtx].vvx_vView;
    if( -v(3)<fNearestZ) fNearestZ = -v(3);  // inverted because of negative sign
    pvtx[i].x = v(1);
    pvtx[i].y = v(2);
    pvtx[i].z = v(3);
  }
  // nearestZ is larger one of plane distance and nearest vertex distance
  const FLOAT fNearestD = -pspo->spo_pbpoBrushPolygon->bpo_pbplPlane->bpl_pwplWorking->wpl_plView.Distance();
  sppo.spo_fNearestZ = Max( fNearestZ, fNearestD);

  // all done
  sppo.spo_ctVtx = ctVtx;
  sppo.spo_ctElements =  pbpo->bpo_aiTriangleElements.Count();
  sppo.spo_piElements = sppo.spo_ctElements ? &pbpo->bpo_aiTriangleElements[0] : NULL;
  _sfStats.IncrementCounter(CStatForm::SCI_SCENE_TRIANGLES, sppo.spo_ctElements/3);
}


/*
 * Generate a span for a polygon on current scan line.
 */
void CRenderer::MakeSpan(CScreenPolygon &spo, CScreenEdge *psed0, CScreenEdge *psed1)
{
  // the polygon must not be portal and not illuminating for rendering lights
  ASSERT(!(spo.IsPortal()
       && (re_ubLightIllumination==0 || re_ubLightIllumination!=spo.spo_ubIllumination)));

  // if rendering shadows
  if( re_bRenderingShadows) {
    // create a new span for it
    CSpan *pspSpan = &re_aspSpans.Push();
    // set up span values
    pspSpan->sp_psedEdge0 = psed0;
    pspSpan->sp_psedEdge1 = psed1;
    pspSpan->sp_pspoPolygon = &spo;
  }
  // if rendering view
  else {
    // if no span added to this polygon yet
    if( !spo.spo_ubSpanAdded) {
      spo.spo_ubSpanAdded = 1;
      // add mirror if needed
      AddMirror(spo);
      // add polygon to scene polygons
      AddPolygonToScene(&spo);
      const PIX pixI0 = PIXCoord(psed0->sed_xI);
      const PIX pixI1 = PIXCoord(psed1->sed_xI);
      spo.spo_pixMinI = pixI0;
      spo.spo_pixMaxI = pixI1;
      spo.spo_pixMinJ = re_pixCurrentScanJ;
      spo.spo_pixMaxJ = re_pixCurrentScanJ;
      spo.spo_pixTotalArea = pixI1-pixI0;
    } else {
      const PIX pixI0 = PIXCoord(psed0->sed_xI);
      const PIX pixI1 = PIXCoord(psed1->sed_xI);
      spo.spo_pixMinI = Min(spo.spo_pixMinI, pixI0);
      spo.spo_pixMaxI = Max(spo.spo_pixMaxI, pixI1);
      spo.spo_pixMinJ = Min(spo.spo_pixMinJ, re_pixCurrentScanJ);
      spo.spo_pixMaxJ = Max(spo.spo_pixMaxJ, re_pixCurrentScanJ);
      spo.spo_pixTotalArea += pixI1-pixI0;
    }
  }
}

/*
 * Add spans in current line to scene.
 */
void CRenderer::AddSpansToScene(void)
{
  if( !re_bRenderingShadows) {
    return;
  }

  //FLOAT fpixLastScanJOffseted = re_pixCurrentScanJ-1 +OFFSET_DN;
  // first, little safety check - quit if zero spans in line!
  INDEX ctSpans = re_aspSpans.Count();
  if( ctSpans==0) {
    return;
  }

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDSPANSTOSCENE);
  UBYTE *pubShadow = re_pubShadow+re_slShadowWidth*re_iCurrentScan;
#ifndef NDEBUG
  INDEX ctPixels = 0;
#endif
  // for all spans in the current line
  for( INDEX iSpan=0; iSpan<ctSpans; iSpan++)
  { // get span start and stop I from edges
    const CSpan &spSpan = re_aspSpans[iSpan];
    PIX pixI0 = PIXCoord( spSpan.sp_psedEdge0->sed_xI);
    PIX pixI1 = PIXCoord( spSpan.sp_psedEdge1->sed_xI);
    // get its length
    PIX pixLen = pixI1-pixI0;
    // skip this span if zero pixels long
    if( pixLen<=0) continue;

    // if the span's polygon is background and of proper illumination
    if ((spSpan.sp_pspoPolygon->spo_spoScenePolygon.spo_ulFlags & SPOF_BACKLIGHT)
      &&(spSpan.sp_pspoPolygon->spo_ubIllumination==re_ubLightIllumination)) {
      // mark those pixels as lighted
      memset(pubShadow, 255, pixLen);
      pubShadow+=pixLen;
      // mark that at least one pixel is lighted
      re_bSomeLightExists = TRUE;
    // if the spans polygon is some other polygon
    } else {
      // mark those pixels as shadowed
      memset(pubShadow, 0, pixLen);
      pubShadow+=pixLen;
      // mark that at least one pixel is darkened
      re_bSomeDarkExists = TRUE;
    }
    // add to pixel counter
#ifndef NDEBUG
    ctPixels+=pixLen;
#endif
  }
  ASSERT(ctPixels<=re_pixSizeI);
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDSPANSTOSCENE);
}

