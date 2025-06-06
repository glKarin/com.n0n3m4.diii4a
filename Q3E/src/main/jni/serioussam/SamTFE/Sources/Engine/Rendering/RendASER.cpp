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

/*
 * Add all edges in add list to active list.
 */
void CRenderer::AddAddListToActiveList(INDEX iScanLine)
{
  INDEX ctAddEdges = re_actAddCounts[iScanLine];
  // if the add list is empty
  if (ctAddEdges==0) {
    // do nothing
    return;
  }

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDADDLIST);
  CListHead &lhAdd = re_alhAddLists[iScanLine];
  ASSERT(ctAddEdges==lhAdd.Count());
  // mark that scan-line coherence is lost
  re_bCoherentScanLine = 0;

  // allocate space in destination for sum of source and add
  INDEX ctActiveEdges = re_aaceActiveEdges.Count();
  re_aaceActiveEdgesTmp.Push(ctAddEdges+ctActiveEdges);
  // check that the add list is sorted right
  #if ASER_EXTREME_CHECKING
  {
    LISTITER(CAddEdge, ade_lnInAdd) itadeAdd(lhAdd);
    FIX16_16 xLastI;
    xLastI.slHolder = MIN_SLONG;
    while(!itadeAdd.IsPastEnd()) {
      CAddEdge &adeAdd = *itadeAdd;
      ASSERT(adeAdd.ade_xI==adeAdd.ade_psedEdge->sed_xI);
      ASSERT(xLastI.slHolder <= adeAdd.ade_xI.slHolder);
      xLastI = adeAdd.ade_xI;
      itadeAdd.MoveToNext();
    }
  }
  #endif

  // check that the active list is sorted right
  #if ASER_EXTREME_CHECKING
  {
    CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
    CActiveEdge *paceSrc = &re_aaceActiveEdges[0];
    while (paceSrc<paceEnd) {
      ASSERT(paceSrc[0].ace_xI.slHolder <= paceSrc[1].ace_xI.slHolder);
      paceSrc++;
    };
  }
  #endif

  // start at begining of add list, source active list and destination active list
  LISTITER(CAddEdge, ade_lnInAdd) itadeAdd(lhAdd);
  CActiveEdge *paceSrc = &re_aaceActiveEdges[0];
  CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
  CActiveEdge *paceDst = &re_aaceActiveEdgesTmp[0];

  IFDEBUG(INDEX ctNewActive=0);
  IFDEBUG(INDEX ctOldActive1=0);
  IFDEBUG(INDEX ctOldActive2=0);
  // for each edge in add list
  while(!itadeAdd.IsPastEnd()) {
    CAddEdge &ade = *itadeAdd;

    // while the edge in active list is left of the edge in add list
    while ((paceSrc->ace_xI.slHolder < ade.ade_xI.slHolder) && (paceSrc!=paceEnd)) {
      // copy the active edge
      ASSERT(paceSrc<=&re_aaceActiveEdges[ctActiveEdges-1]);
      *paceDst++=*paceSrc++;
      IFDEBUG(ctOldActive1++);
    }

    // copy the add edge
    ASSERT(paceDst > &re_aaceActiveEdgesTmp[0]);
    ASSERT(ade.ade_xI.slHolder == ade.ade_psedEdge->sed_xI.slHolder);
    ASSERT(paceDst[-1].ace_xI.slHolder <= ade.ade_xI.slHolder);

    *paceDst++=CActiveEdge(itadeAdd->ade_psedEdge);
    IFDEBUG(ctNewActive++);
    // advance iterator in add list
    itadeAdd.MoveToNext();
  }
  // clear the add list
  lhAdd.Clear();
  re_actAddCounts[iScanLine] = 0;
  // copy all edges left in the active list
  while (paceSrc<=&re_aaceActiveEdges[ctActiveEdges-1]) {
    *paceDst++=*paceSrc++;
    IFDEBUG(ctOldActive2++);
  }

  // swap the lists
  Swap(re_aaceActiveEdges.sa_Count    , re_aaceActiveEdgesTmp.sa_Count    );
  Swap(re_aaceActiveEdges.sa_Array    , re_aaceActiveEdgesTmp.sa_Array    );
  Swap(re_aaceActiveEdges.sa_UsedCount, re_aaceActiveEdgesTmp.sa_UsedCount);
  re_aaceActiveEdgesTmp.PopAll();

  if (ctAddEdges>_ctMaxAddEdges) {
    _ctMaxAddEdges=ctAddEdges;
  }
  if (re_aaceActiveEdges.Count()>_ctMaxActiveEdges) {
    _ctMaxActiveEdges=re_aaceActiveEdges.Count();
  }

  // check that the active list is sorted right
  #if ASER_EXTREME_CHECKING
  {
    CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
    CActiveEdge *paceSrc = &re_aaceActiveEdges[0];
    while (paceSrc<paceEnd) {
      ASSERT(paceSrc[0].ace_xI.slHolder <= paceSrc[1].ace_xI.slHolder);
      paceSrc++;
    };
  }
  #endif

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDADDLIST);
}

/*
 * Remove all edges in remove list from active list and from other lists.
 */
void CRenderer::RemRemoveListFromActiveList(CScreenEdge *psedFirst)
{
  // if the remove list is empty
  if (psedFirst==NULL) {
    // do nothing
    return;
  }
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_REMREMLIST);
  // mark that scan-line coherence is lost
  re_bCoherentScanLine = 0;

  // for each edge to be removed on this line
  CScreenEdge *psed = psedFirst;
  do {
    // mark it as removed
    psed->sed_xI.slHolder = ACE_REMOVED;
    psed = psed->sed_psedNextRemove;
  } while (psed!=NULL);

  // for each active edge
  CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
  CActiveEdge *paceSrc = &re_aaceActiveEdges[1];
  CActiveEdge *paceDst = paceSrc;
  do {
    // if it is not removed
    if (paceSrc->ace_psedEdge->sed_xI.slHolder!=ACE_REMOVED) {
      // copy it
      *paceDst = *paceSrc;
      paceDst++;
    }
    paceSrc++;
  } while (paceSrc<=paceEnd);
  // trim the end of active list
  re_aaceActiveEdges.PopUntil(paceDst-&re_aaceActiveEdges[0]-1);

  // check that the active list is sorted right
  #if ASER_EXTREME_CHECKING
  {
    CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
    CActiveEdge *paceSrc = &re_aaceActiveEdges[0];
    while (paceSrc<paceEnd) {
      ASSERT(paceSrc[0].ace_xI.slHolder <= paceSrc[1].ace_xI.slHolder);
      paceSrc++;
    };
  }
  #endif
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_REMREMLIST);
}

/*
 * Step all edges in active list by one scan line and resort them.
 */
void CRenderer::StepAndResortActiveList(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_STEPANDRESORT);

  // start after the left sentinel
  CActiveEdge *pace = &re_aaceActiveEdges[1];

  // for all edges before right sentinel
  CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
  do {

    // step the edge by one scan line
    pace->ace_xI.slHolder += pace->ace_xIStep.slHolder;

    // if the previous is right of the current
    if (pace[-1].ace_xI.slHolder > pace->ace_xI.slHolder) {
      // mark that scan-line coherence is lost
      re_bCoherentScanLine = 0;

      // find last one that is not right
      CActiveEdge *pacePred = pace;
      do {
        pacePred--;
      } while(pacePred->ace_xI.slHolder > pace->ace_xI.slHolder);

      // remember the current one
      CActiveEdge aceCurrent = *pace;
      // move all of the edges between one place forward
      CActiveEdge *paceMove=pace-1;
      do {
        paceMove[1]=paceMove[0];
        paceMove--;
      } while (paceMove>pacePred);
      // insert the current to its new place
      paceMove[1] = aceCurrent;
    }

    pace++;
  } while (pace < paceEnd);

  // check that the active list is sorted right
  #if ASER_EXTREME_CHECKING
  {
    CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
    CActiveEdge *paceSrc = &re_aaceActiveEdges[0];
    while (paceSrc<paceEnd) {
      ASSERT(paceSrc[0].ace_xI.slHolder <= paceSrc[1].ace_xI.slHolder);
      paceSrc++;
    };
  }
  #endif
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_STEPANDRESORT);
}

/* Copy I coordinates from active list to edge data. */
void CRenderer::CopyActiveCoordinates(void)
{
  // for all active edges
  CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
  CActiveEdge *pace = &re_aaceActiveEdges[1];
  do {
    // copy active coordinates
    pace->ace_psedEdge->sed_xI.slHolder = pace->ace_xI.slHolder;
    pace++;
  } while (pace<=paceEnd);
}

/*
 * Remove an active portal from rendering
 */
void CRenderer::RemovePortal(CScreenPolygon &spo)
{
  ASSERT(spo.IsPortal());
  ASSERT(spo.spo_bActive);
  spo.spo_bActive = FALSE;

  // if it is a translucent portal
  if (spo.spo_pbpoBrushPolygon->bpo_ulFlags & (BPOF_RENDERTRANSLUCENT|BPOF_TRANSPARENT)) {
    _pfRenderProfile.StartTimer(CRenderProfile::PTI_PROCESSTRANSPORTAL);
    // add polygon to scene polygons for rendering
    AddPolygonToScene(&spo);
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_PROCESSTRANSPORTAL);
  }

  // if it is in the surface stack
  if (spo.spo_lnInStack.IsLinked()) {
    // remove it from surface stack
    RemPolygonFromSurfaceStack(spo);
    spo.spo_iInStack = 0;
  }
}

/*
 * Add sector(s) adjoined to a portal to rendering and remove the portal.
 */
void CRenderer::PassPortal(CScreenPolygon &spo)
{
  ChangeStatsMode(CStatForm::STI_WORLDTRANSFORM);
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_PASSPORTAL);
  // remove the portal from rendering
  RemovePortal(spo);

  // for all sectors related to the portal
  {FOREACHDSTOFSRC(spo.spo_pbpoBrushPolygon->bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbsc)
    // if the sector is hidden when not rendering shadows
    if ((pbsc->bsc_ulFlags&BSCF_HIDDEN) && !re_bRenderingShadows) {
      // skip it
      continue;
    }
    // get brush of the sector
    CBrushMip *pbmSectorMip = pbsc->bsc_pbmBrushMip;
    CBrush3D &brBrush = *pbmSectorMip->bm_pbrBrush;
    // prepare the brush entity for rendering if not yet prepared
    PrepareBrush(brBrush.br_penEntity);
    // get relevant mip factor for that brush and current rendering prefs
    CBrushMip *pbmRelevantMip;
    if (brBrush.br_ulFlags&BRF_DRAWFIRSTMIP) {
      pbmRelevantMip = brBrush.GetBrushMipByDistance(
        _wrpWorldRenderPrefs.GetCurrentMipBrushingFactor(0.0f));
    } else {
      pbmRelevantMip = brBrush.GetBrushMipByDistance(
        _wrpWorldRenderPrefs.GetCurrentMipBrushingFactor(brBrush.br_prProjection->MipFactor()));
    }
    // if relevant brush mip is same as the sector's brush mip
    if (pbmSectorMip==pbmRelevantMip) {
      // add that sector to active sectors
      AddActiveSector(*pbsc);
    }
  ENDFOR}
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_PASSPORTAL);
  ChangeStatsMode(CStatForm::STI_WORLDVISIBILITY);
}

/*
 * Add a sector of a brush to rendering queues.
 */
void CRenderer::AddActiveSector(CBrushSector &bscSector)
{
  // if already active
  if (bscSector.bsc_lnInActiveSectors.IsLinked()) {
    // do nothing;
    return;
  }
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDSECTOR);

  // add it to active sectors list
  re_lhActiveSectors.AddTail(bscSector.bsc_lnInActiveSectors);

// !!! FIXME : rcg10132001 I'm lazy.
#ifdef PLATFORM_WIN32
  ASSERT((_controlfp(0, 0)&_MCW_RC)==_RC_NEAR);
#endif

  CBrush3D &br = *bscSector.bsc_pbmBrushMip->bm_pbrBrush;
  // if should render field brush sector
  if (br.br_penEntity->en_RenderType==CEntity::RT_FIELDBRUSH 
      && !_wrpWorldRenderPrefs.IsFieldBrushesOn()) {
    // skip it
    bscSector.bsc_ulFlags|=BSCF_INVISIBLE;
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDSECTOR);
    return;
  }

  // test sector visibility
  const INDEX iFrustrumTest = IsSectorVisible( br, bscSector);
  if( iFrustrumTest==-1) {
    // outside of frustrum - skip it
    bscSector.bsc_ulFlags |= BSCF_INVISIBLE;
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDSECTOR);
    return;
  } else if( iFrustrumTest==0) {
    // partially in frustrum - needs clipping
    bscSector.bsc_ulFlags |= BSCF_NEEDSCLIPPING;
  } else {
    // completely in frustrum - doesn't need clipping
    bscSector.bsc_ulFlags &= ~BSCF_NEEDSCLIPPING;
  }
  // mark that sector is visible
  bscSector.bsc_ulFlags &= ~BSCF_INVISIBLE;

  // remember current sector
  re_pbscCurrent = &bscSector;
  re_pbrCurrent = &br;
  _sfStats.IncrementCounter(CStatForm::SCI_SECTORS);

  // if projection is perspective
  if( br.br_prProjection.IsPerspective()) {
    // prepare fog/haze
    SetupFogAndHaze();
  }

  // transform all vertices and planes in this sector
  PreClipVertices();
  PreClipPlanes();

  // if polygons should be drawn
  if (_wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE
    ||re_bRenderingShadows) {
    // find which portals should be rendered as portals or as pretenders
    FindPretenders();
    // make screen polygons for nondetail polygons in current sector
    MakeNonDetailScreenPolygons();
    // clip all polygons to all clip planes
    if( bscSector.bsc_ulFlags&BSCF_NEEDSCLIPPING) ClipToAllPlanes( br.br_prProjection);
    // project vertices to 2d
    PostClipVertices();
    // make final edges for all polygons in current sector
    MakeFinalPolygonEdges();
    // add screen edges for all polygons in current sector
    AddScreenEdges();
    // make screen polygons for detail polygons in current sector
    MakeDetailScreenPolygons();
  }

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDSECTOR);

  // get the entity the sector is in
  CEntity *penSectorEntity = bscSector.bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  // if it has the entity (it is not the background brush)
  if (penSectorEntity != NULL) {
    // add all other entities near the sector
    AddEntitiesInSector(&bscSector);
  }
}

/*
 * Initialize list of active edges.
 */
void CRenderer::InitScanEdges(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_INITSCANEDGES);

  // empty active lists
  re_aaceActiveEdges.PopAll();    re_aaceActiveEdges.SetAllocationStep(256);
  re_aaceActiveEdgesTmp.PopAll(); re_aaceActiveEdgesTmp.SetAllocationStep(256);

  // set up left sentinel as left edge of screen and add it to head of active list
  re_sedLeftSentinel.sed_xI = FIX16_16(re_fbbClipBox.Min()(1)-SENTINELEDGE_EPSILON);
  re_sedLeftSentinel.sed_xIStep = FIX16_16(0, 0);
  re_sedLeftSentinel.sed_pspo = &re_spoFarSentinel;
  re_aaceActiveEdges.Push() = CActiveEdge(&re_sedLeftSentinel);

  // set up right sentinel as right edge of screen and add it to tail of active list
  re_sedRightSentinel.sed_xI = FIX16_16(re_fbbClipBox.Max()(1)+SENTINELEDGE_EPSILON);
  re_sedRightSentinel.sed_xIStep = FIX16_16(0,0);
  re_sedRightSentinel.sed_pspo  = &re_spoFarSentinel;
  re_aaceActiveEdges.Push() = CActiveEdge(&re_sedRightSentinel);

  // set up far sentinel as infinitely far polygon
  re_spoFarSentinel.spo_pgOoK.Constant(-999999.0f); // further than infinity
  re_spoFarSentinel.spo_iInStack = 1;
  re_spoFarSentinel.spo_psedSpanStart = &re_sedLeftSentinel;
  re_spoFarSentinel.spo_pbpoBrushPolygon = NULL;
  re_spoFarSentinel.spo_ubIllumination = 0;

  // initialize list of spans for far sentinel
  re_spoFarSentinel.spo_spoScenePolygon.spo_cColor = re_pwoWorld->wo_colBackground;
  re_spoFarSentinel.spo_spoScenePolygon.spo_aptoTextures[0] = NULL;
  re_spoFarSentinel.spo_spoScenePolygon.spo_aptoTextures[1] = NULL;
  re_spoFarSentinel.spo_spoScenePolygon.spo_aptoTextures[2] = NULL;
  re_spoFarSentinel.spo_spoScenePolygon.spo_psmShadowMap = NULL;
  re_spoFarSentinel.spo_spoScenePolygon.spo_ulFlags = SPOF_BACKLIGHT;

  // add it to the list of background span polygons in this renderer
  re_spoFarSentinel.spo_spoScenePolygon.spo_pspoSucc = re_pspoFirstBackground;
  re_pspoFirstBackground = &re_spoFarSentinel.spo_spoScenePolygon;

  // add far sentinel as bottom of surface stack
  ASSERT(re_lhSurfaceStack.IsEmpty());
  re_lhSurfaceStack.AddTail(re_spoFarSentinel.spo_lnInStack);

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_INITSCANEDGES);
}

/*
 * Clean up list of active edges.
 */
void CRenderer::EndScanEdges(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ENDSCANEDGES);

  // remove far sentinel from surface stack
  ASSERT(re_spoFarSentinel.spo_iInStack == 1);
  re_spoFarSentinel.spo_lnInStack.Remove();
  re_spoFarSentinel.spo_iInStack = 0;

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ENDSCANEDGES);
}

/*
 * Add a polygon to surface stack.
 */
BOOL CRenderer::AddPolygonToSurfaceStack(CScreenPolygon &spo)
{
  // increment in-stack counter
  spo.spo_iInStack++;

  // if it doesn't have to be added
  if (spo.spo_iInStack!=1) {
    // return that this is not new top
    return FALSE;
  }

#define BIAS (1)
//#define BIAS (0)

  FLOAT fScanI = FLOAT(re_xCurrentScanI)+BIAS;
  FLOAT fScanJ = re_fCurrentScanJ;//+re_fMinJ;

  // calculate 1/k for new polygon
  CPlanarGradients &pg = spo.spo_pgOoK; 
  FLOAT fOoK = pg.pg_f00 + pg.pg_fDOverDI*fScanI + pg.pg_fDOverDJ*fScanJ;

  // bias for right edges - fix against generating extra trapezoids
  fOoK*=re_fEdgeAdjustK;

  // must not be infinitely far, except if background polygon
  //ASSERT(fOneOverK>0.0f || fOneOverK==-9999.0f); 
  // cannot assert on this, because of +1 bias

  // start at top surface in stack
  LISTITER(CScreenPolygon, spo_lnInStack) itspo(re_lhSurfaceStack);

  // if the projection is not perspective
  if (!re_prProjection.IsPerspective()) {
    // while new polygon is further than polygon in stack
    while(
     ((fOoK - 
      itspo->spo_pgOoK.pg_f00 -
      itspo->spo_pgOoK.pg_fDOverDI*fScanI -
      itspo->spo_pgOoK.pg_fDOverDJ*fScanJ)<0)
      && (&*itspo != &re_spoFarSentinel)) {
      // move to next polygon in stack
      itspo.MoveToNext();
    }
  } else {
    // while new polygon is further than polygon in stack
    FLOAT fDelta = fOoK -
      itspo->spo_pgOoK.pg_f00 -
      itspo->spo_pgOoK.pg_fDOverDI*fScanI -
      itspo->spo_pgOoK.pg_fDOverDJ*fScanJ;

    if (((SLONG &)fDelta) < 0) {
      do {
        // the polygon in stack must not be far sentinel
        ASSERT(&*itspo != &re_spoFarSentinel);
        // move to next polygon in stack
        itspo.MoveToNext();
        fDelta = fOoK -
          itspo->spo_pgOoK.pg_f00 -
          itspo->spo_pgOoK.pg_fDOverDI*fScanI -
          itspo->spo_pgOoK.pg_fDOverDJ*fScanJ;
      } while (((SLONG &)fDelta) < 0);
    }
  }

  // add the new polygon before the one in stack
  itspo.InsertBeforeCurrent(spo.spo_lnInStack);

  // return if this is new top of stack
  return spo.spo_lnInStack.IsHead();
}

/*
 * Remove a polygon from surface stack.
 */
BOOL CRenderer::RemPolygonFromSurfaceStack(CScreenPolygon &spo)
{
  // decrement in-stack counter
  spo.spo_iInStack--;

  // if it doesn't have to be removed
  if (spo.spo_iInStack!=0) {
    // return that this was not top
    return FALSE;
  }

  // if the polygon is top of stack
  if (spo.spo_lnInStack.IsHead()) {
    // remove the polygon from stack
    spo.spo_lnInStack.Remove();
    // return that this was top
    return TRUE;

  // if the polygon is not top of stack
  } else {
    // remove the polygon from stack
    spo.spo_lnInStack.Remove();
    // return that this was not top
    return FALSE;
  }
}

/*
 * Swap two polygons in surface stack.
 */
BOOL CRenderer::SwapPolygonsInSurfaceStack(CScreenPolygon &spoOld, CScreenPolygon &spoNew)
{
  // !!!!! fix the problems with surfaces beeing multiple times added
  // to the stack before reenabling this feature!

  ASSERT(FALSE);
  // decrement/increment in-stack counters
  spoOld.spo_iInStack--;
  spoNew.spo_iInStack++;

  ASSERT(spoOld.spo_iInStack==0);
  ASSERT(spoNew.spo_iInStack==1);

  // if the left polygon is top of stack
  if (spoOld.spo_lnInStack.IsHead()) {
    // swap them
    CListNode &lnBefore = spoOld.spo_lnInStack.IterationPred();
    spoOld.spo_lnInStack.Remove();
    lnBefore.IterationInsertAfter(spoNew.spo_lnInStack);
    return TRUE;

  // if the polygon is not top of stack
  } else {
    // swap them
    CListNode &lnBefore = spoOld.spo_lnInStack.IterationPred();
    spoOld.spo_lnInStack.Remove();
    lnBefore.IterationInsertAfter(spoNew.spo_lnInStack);
    // return that this was not top
    return FALSE;
  }
}

/*
 * Remove all polygons from surface stack.
 */
void CRenderer::FlushSurfaceStack(void)
{
  // while there is some polygon above far sentinel in surface stack
  CScreenPolygon *pspoTop;
  while ((pspoTop = LIST_HEAD(re_lhSurfaceStack, CScreenPolygon, spo_lnInStack))
    != &re_spoFarSentinel) {

    // it must be linked in stack
#if 0
    if (pspoTop->spo_iInStack<=0) {
      _pSCape->DebugSave();
      FatalError("Surface stack bug encountered!\nDebug game saved!");
    }
    ASSERT(pspoTop->spo_iInStack>0);
#endif

    // remove it from stack
    pspoTop->spo_lnInStack.Remove();
    pspoTop->spo_iInStack = 0;
  }
}

/*
 * Scan list of active edges into spans.
 */
CScreenPolygon *CRenderer::ScanOneLine(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_SCANONELINE);
  IFDEBUG(FIX16_16 xCurrentI = FIX16_16(-10));

  // reinit far sentinel
  re_spoFarSentinel.spo_iInStack = 1;
  re_spoFarSentinel.spo_psedSpanStart = &re_sedLeftSentinel;

  // clear list of spans for current line
  re_aspSpans.PopAll();

  // if left and right sentinels are sorted wrong
  if (re_aaceActiveEdges[0].ace_psedEdge!=&re_sedLeftSentinel
    ||re_aaceActiveEdges[re_aaceActiveEdges.Count()-1].ace_psedEdge!=&re_sedRightSentinel) {
    // skip entire line (this patches some extremely rare crash situations)
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_SCANONELINE);
    return NULL;
  }

  // for all edges in the line
  CActiveEdge *pace = &re_aaceActiveEdges[1];
  CActiveEdge *paceEnd = &re_aaceActiveEdges[re_aaceActiveEdges.Count()-1];
  while (pace<paceEnd) {
    CScreenEdge &sed = *pace->ace_psedEdge;
    ASSERT(&sed!=&re_sedLeftSentinel);
    ASSERT(&sed!=&re_sedRightSentinel);
    // set up current I coordinate on the scan line
    re_xCurrentScanI = sed.sed_xI = pace->ace_xI;

    // check that edges are sorted ok

    ASSERT(xCurrentI <= sed.sed_xI);

    // count edge transitions
    _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_EDGETRANSITIONS);
    _sfStats.IncrementCounter(CStatForm::SCI_EDGETRANSITIONS);

    // if this edge has active polygon
    if (sed.sed_pspo!= NULL && sed.sed_pspo->spo_bActive) {
      CScreenPolygon &spo = *sed.sed_pspo;
      // if it is right edge of the polygon
      if (sed.sed_ldtDirection==LDT_ASCENDING) {

        // remove the left polygon from stack
        BOOL bWasTop = RemPolygonFromSurfaceStack(spo);

        // if that was top polygon in surface stack
        if (bWasTop) {
          // if it is portal
          if (spo.IsPortal() && 
             (re_ubLightIllumination==0||re_ubLightIllumination!=spo.spo_ubIllumination)) {
            // fail scanning and add that portal
            _pfRenderProfile.StopTimer(CRenderProfile::PTI_SCANONELINE);
            return &spo;
          }
          // generate a span for it
          MakeSpan(spo, spo.spo_psedSpanStart, &sed);

          // mark that span of new top starts here
          CScreenPolygon *pspoNewTop =
            LIST_HEAD(re_lhSurfaceStack, CScreenPolygon, spo_lnInStack);
          pspoNewTop->spo_psedSpanStart = &sed;
        }

      // if it is left edge of the polygon
      } else {
        ASSERT(sed.sed_ldtDirection==LDT_DESCENDING);

        // add the right polygon to stack
        BOOL bIsTop = AddPolygonToSurfaceStack(spo);

        // if it is the new top of surface stack
        if (bIsTop) {
          // get the old top
          CScreenPolygon &spoOldTop = *LIST_SUCC(spo, CScreenPolygon, spo_lnInStack);
          // if it is portal
          if (spoOldTop.IsPortal() &&
             (re_ubLightIllumination==0||re_ubLightIllumination!=spoOldTop.spo_ubIllumination)) {
            // if its span has at least one pixel in length
            if ( PIXCoord(re_xCurrentScanI)-PIXCoord(spoOldTop.spo_psedSpanStart->sed_xI)>0) {
              // fail scanning and add that portal
              _pfRenderProfile.StopTimer(CRenderProfile::PTI_SCANONELINE);
              return &spoOldTop;
            }
          // if it is not portal
          } else {
            // generate span for old top
            MakeSpan(spoOldTop, spoOldTop.spo_psedSpanStart, &sed);
          }

          // mark that span of new polygon starts here
          spo.spo_psedSpanStart = &sed;
        }
      }

      IFDEBUG(xCurrentI = pace->ace_xI);
    // if this edge has no active polygon
    } else {
      // mark it for removal
      sed.sed_xI.slHolder = ACE_REMOVED;
    }

    pace++;
  }

  // NOTE: In some rare and extreme situations (usually when casting shadows)
  // the stack might not be empty after scanning - this code fixes that.

  // if surface stack contains something else except background
  CScreenPolygon *pspoTop = LIST_HEAD(re_lhSurfaceStack, CScreenPolygon, spo_lnInStack);
  if (&re_spoFarSentinel != pspoTop) {
    // ASSERTALWAYS("Bug in ASER: Surface stack not empty!");
    CScreenPolygon &spo = *pspoTop;
    // generate span of the top polygon to the right border
    if (!(spo.IsPortal()
       && (re_ubLightIllumination==0 || re_ubLightIllumination!=spo.spo_ubIllumination))) {
      MakeSpan(spo, spo.spo_psedSpanStart, &re_sedRightSentinel);
    }
    // remove all left-over polygons from stack
    do {
      /* BOOL bWasTop = */ RemPolygonFromSurfaceStack(*pspoTop);
      pspoTop = LIST_HEAD(re_lhSurfaceStack, CScreenPolygon, spo_lnInStack);
    } while (&re_spoFarSentinel != pspoTop);
    // mark start of background span at right border
    re_spoFarSentinel.spo_psedSpanStart = &re_sedRightSentinel;
  }

  // generate span for far sentinel
  MakeSpan(re_spoFarSentinel, re_spoFarSentinel.spo_psedSpanStart, &re_sedRightSentinel);

  // return that no portal was encountered
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_SCANONELINE);
  return NULL;
}

/*
 * Rasterize edges into spans.
 */
void CRenderer::ScanEdges(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_SCANEDGES);
  // set up the list of active edges, surface stack and the sentinels
  InitScanEdges();

  // mark that first line is never coherent with previous one
  re_bCoherentScanLine = 0;

  // for each scan line, top to bottom
  for (re_iCurrentScan = 0; re_iCurrentScan<re_ctScanLines; re_iCurrentScan++) {
    re_pixCurrentScanJ = re_iCurrentScan + re_pixTopScanLineJ;
    re_fCurrentScanJ = FLOAT(re_pixCurrentScanJ);

    _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_OVERALLSCANLINES);

    CScreenPolygon *pspoPortal;     // pointer to portal encountered while scanning

    // add all edges that start on this scan line to active list
    AddAddListToActiveList(re_iCurrentScan);

    // if scan-line is coherent with the last one
    /*!!!! if (re_bCoherentScanLine>0) {
      // increment counter of coherent scan lines
      _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_COHERENTSCANLINES);
      // just copy I coordinates from active list to edge data
      CopyActiveCoordinates();

    // if scan-line is not coherent with the last one
    } else*/ {

      // scan list of active edges into spans
      pspoPortal = ScanOneLine();

      // while portal is encountered during scanning
      while (pspoPortal != NULL) {
        // increment counter of portal retries
        _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_SCANLINEPORTALRETRIES);

        // remove all polygons from surface stack
        FlushSurfaceStack();

        // add sectors near the encountered portal to rendering
        PassPortal(*pspoPortal);
        // add all newly added edges that start on this scan line to active list
        AddAddListToActiveList(re_iCurrentScan);

        // rescan list of active edges into spans again
        pspoPortal = ScanOneLine();
      }
    }

    // set scan-line coherence marker
    re_bCoherentScanLine++;

    // surface stack must contain only background
    ASSERT(&re_spoFarSentinel == LIST_HEAD(re_lhSurfaceStack, CScreenPolygon, spo_lnInStack)
        && &re_spoFarSentinel == LIST_TAIL(re_lhSurfaceStack, CScreenPolygon, spo_lnInStack));

    // add spans in this line to the scene
    AddSpansToScene();

    // uncomment this for extreme checking of surface stack management -- very slow
    #if 0
      // all surfaces must have in-stack counter of zero
      FOREACHINDYNAMICARRAY(re_aspoScreenPolygons, CScreenPolygon, itspo) {
        CScreenPolygon &spo = itspo.Current();
        ASSERT(spo.spo_iInStack == 0);
      }
    #endif

    // remove all edges that stop on this scan from active list and from other lists.
    RemRemoveListFromActiveList(re_apsedRemoveFirst[re_iCurrentScan]);

    // step all remaining edges by one scan line and resort the active list
    StepAndResortActiveList();

  }

  // clean up the list of active edges, surface stack and the sentinels

  EndScanEdges();
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_SCANEDGES);
}
