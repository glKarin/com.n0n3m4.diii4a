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

/////////////////////////////////////////////////////////////////////
// Clipping functions

// check if a polygon is to be visible
__forceinline ULONG CRenderer::GetPolygonVisibility(const CBrushPolygon &bpo)
{
  // get transformed polygon's plane
  CWorkingPlane *pwplPolygonPlane = bpo.bpo_pbplPlane->bpl_pwplWorking;
  CWorkingPlane wplReverse;
  BOOL bInvertPolygon = FALSE;

  // if the polygon should be inverted or double sided
  if((re_bRenderingShadows
   &&!re_bDirectionalShadows
   &&re_ubLightIllumination!=0 
   &&bpo.bpo_bppProperties.bpp_ubIlluminationType==re_ubLightIllumination)
   || (re_pbrCurrent->br_pfsFieldSettings!=NULL && !pwplPolygonPlane->wpl_bVisible)
   ) {
    bInvertPolygon = TRUE;
  }

  if (bInvertPolygon) {
    // make temporary inverted polygon plane
    pwplPolygonPlane = &wplReverse;
    pwplPolygonPlane->wpl_plView = -bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_plView;
    pwplPolygonPlane->wpl_bVisible = 
      re_pbrCurrent->br_prProjection->IsViewerPlaneVisible(pwplPolygonPlane->wpl_plView);
  }

  // if the poly is double-sided and detail
  if( !re_bRenderingShadows && (bpo.bpo_ulFlags&BPOF_DOUBLESIDED) && (bpo.bpo_ulFlags&BPOF_DETAILPOLYGON)) {
    // it's definately visible
    return PDF_POLYGONVISIBLE;
  }

  // if the plane is invisible
  if (!pwplPolygonPlane->wpl_bVisible) {
    // polygon is invisible
    return 0;
  }

  // if the polygon is invisible
  if ((bpo.bpo_ulFlags&BPOF_INVISIBLE)
    ||(re_bRenderingShadows && (bpo.bpo_ulFlags&BPOF_DOESNOTCASTSHADOW))) {
    // skip it
    return 0;
  }

  ULONG ulDirection = PDF_POLYGONVISIBLE;
  BOOL bProjectionInverted = re_prProjection->pr_bInverted;
  if (bProjectionInverted && !bInvertPolygon) {
    ulDirection |= PDF_FLIPEDGESPRE;
  } else if (!bProjectionInverted && bInvertPolygon){
    ulDirection |= PDF_FLIPEDGESPOST;
  }

  // else, polygon is visible
  return ulDirection;
}


// check if polygon is outside viewfrustum
__forceinline BOOL CRenderer::IsPolygonCulled(const CBrushPolygon &bpo)
{
  CBrushSector  &bsc = *bpo.bpo_pbscSector;
  // setup initial mask
  ULONG ulMask = 0xFFFFFFFF;

  // for each vertex
  INDEX ctVtx = bpo.bpo_apbvxTriangleVertices.Count();
  {for(INDEX i=0; i<ctVtx; i++) {
    CBrushVertex *pbvx = bpo.bpo_apbvxTriangleVertices[i];
    INDEX ivx = bsc.bsc_abvxVertices.Index(pbvx);
    // get the outcodes for that vertex
    ULONG ulCode = re_avvxViewVertices[bsc.bsc_ivvx0+ivx].vvx_ulOutcode;
    // and them to the mask
    ulMask &= ulCode;
  }}

  // if any bit in the mask is still set, it means that all points are out
  // wtr to that plane
  return ulMask;
}

// find which portals should be rendered as portals or as pretenders
void CRenderer::FindPretenders(void)
{
  re_pbscCurrent->bsc_ispo0 = re_aspoScreenPolygons.Count();
  // for all polygons in sector
  FOREACHINSTATICARRAY(re_pbscCurrent->bsc_abpoPolygons, CBrushPolygon, itpo) {
    CBrushPolygon &bpo = *itpo;
    // initially not rendered as portal
    bpo.bpo_ulFlags&=~BPOF_RENDERASPORTAL;
    // if it is portal, 
    if (bpo.bpo_ulFlags&BPOF_PORTAL) {
      // initially rendered as portal
      bpo.bpo_ulFlags|=BPOF_RENDERASPORTAL;

      // if could be a pretender
      if (bpo.bpo_bppProperties.bpp_uwPretenderDistance!=0) {
        // get distance at which it is a pretender
        FLOAT fPretenderDistance = bpo.bpo_bppProperties.bpp_uwPretenderDistance;

        // for each vertex in the polygon
        INDEX ctVtx = bpo.bpo_apbvxTriangleVertices.Count();
        CBrushSector &bsc = *bpo.bpo_pbscSector;
        {for(INDEX i=0; i<ctVtx; i++) {
          CBrushVertex *pbvx = bpo.bpo_apbvxTriangleVertices[i];
          INDEX ivx = bsc.bsc_abvxVertices.Index(pbvx);
          // get distance of the vertex from the view plane
          FLOAT fx = re_avvxViewVertices[bsc.bsc_ivvx0+ivx].vvx_vView(1);
          FLOAT fy = re_avvxViewVertices[bsc.bsc_ivvx0+ivx].vvx_vView(2);
          FLOAT fz = re_avvxViewVertices[bsc.bsc_ivvx0+ivx].vvx_vView(3);
          FLOAT fD = fx*fx+fy*fy+fz*fz;
          // if nearer than allowed pretender distance
          if (fD<fPretenderDistance*fPretenderDistance) {
            // this polygon is not a pretender
            goto nextpolygon;
          }
        }}
        // if all vertices passed the check, mark as pretender
        bpo.bpo_ulFlags&=~BPOF_RENDERASPORTAL;
      }
    }
nextpolygon:;
  }
}

// make screen polygons for nondetail polygons in current sector
void CRenderer::MakeNonDetailScreenPolygons(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_MAKENONDETAILSCREENPOLYGONS);

  re_pbscCurrent->bsc_ispo0 = re_aspoScreenPolygons.Count();
  // detail polygons are not skipped if rendering shadows
  const ULONG ulDetailMask = re_bRenderingShadows ? 0 : BPOF_DETAILPOLYGON;

  // for all polygons in sector
  FOREACHINSTATICARRAY(re_pbscCurrent->bsc_abpoPolygons, CBrushPolygon, itpo) {
    CBrushPolygon &bpo = *itpo;

    // if polygon does not contribute to the visibility determination
    if ( (bpo.bpo_ulFlags&ulDetailMask)
      &&!(bpo.bpo_ulFlags&BPOF_RENDERASPORTAL)) {
      // skip it
      continue;
    }

    // no screen polygon by default
    bpo.bpo_pspoScreenPolygon = NULL;

    // skip if the polygon is not visible
    ASSERT( !IsPolygonCulled(bpo));  // cannot be culled yet!
    const ULONG ulVisible = GetPolygonVisibility(bpo);
    if( ulVisible==0) continue;

    _sfStats.IncrementCounter(CStatForm::SCI_POLYGONS);
    _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_NONDETAILPOLYGONS);

    // make screen polygon for the polygon
    CScreenPolygon &spo = *MakeScreenPolygon(bpo);

    // add its edges
    MakeInitialPolygonEdges(bpo, spo, ulVisible);
  }

  // remember number of polygons in sector
  re_pbscCurrent->bsc_ctspo = re_aspoScreenPolygons.Count()-re_pbscCurrent->bsc_ispo0;

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_MAKENONDETAILSCREENPOLYGONS);
}


// make screen polygons for detail polygons in current sector
void CRenderer::MakeDetailScreenPolygons(void)
{
  // if rendering shadows or not rendering detail polygons
  if (re_bRenderingShadows
    ||!wld_bRenderDetailPolygons) {
    // do nothing
    return;
  }

  _pfRenderProfile.StartTimer(CRenderProfile::PTI_MAKEDETAILSCREENPOLYGONS);
  // for all polygons in sector
  FOREACHINSTATICARRAY(re_pbscCurrent->bsc_abpoPolygons, CBrushPolygon, itpo) {
    CBrushPolygon &bpo = *itpo;

    // if polygon is not detail
    if (!(bpo.bpo_ulFlags&BPOF_DETAILPOLYGON)
      || (bpo.bpo_ulFlags&BPOF_RENDERASPORTAL)) {
      // skip it
      continue;
    }

    // no screen polygon by default
    bpo.bpo_pspoScreenPolygon = NULL;

    // skip if the polygon is not visible
    if( GetPolygonVisibility(bpo)==0) continue;
    // skip if outside the frustum
    if( (re_pbscCurrent->bsc_ulFlags&BSCF_NEEDSCLIPPING) && IsPolygonCulled(bpo)) continue;

    _sfStats.IncrementCounter(CStatForm::SCI_DETAILPOLYGONS);
    _pfRenderProfile.IncrementCounter(CRenderProfile::PCI_DETAILPOLYGONS);

    // make screen polygon for the polygon
    CScreenPolygon &spo = *MakeScreenPolygon(bpo);

    // if it is portal
    if (spo.IsPortal()) {
      // pass it immediately
      PassPortal(spo);
    } else {
      // add polygon to scene polygons for rendering
      AddPolygonToScene(&spo);
    }
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_MAKEDETAILSCREENPOLYGONS);
}

// make initial edges for a polygon
void CRenderer::MakeInitialPolygonEdges(CBrushPolygon &bpo, CScreenPolygon &spo, BOOL ulDirection)
{
  // get number of edges
  INDEX ctEdges = bpo.bpo_abpePolygonEdges.Count();
  spo.spo_ubDirectionFlags = ulDirection&PDF_FLIPEDGESPOST;
  BOOL bInvert = (ulDirection&PDF_FLIPEDGESPRE)!=0;
  
  // remember edge vertex start and count
  spo.spo_ctEdgeVx = ctEdges*2;
  spo.spo_iEdgeVx0 = re_aiEdgeVxClipSrc.Count();

  // create edge vertices
  INDEX *ai = re_aiEdgeVxClipSrc.Push(ctEdges*2);

  // for each edge
  for (INDEX iEdge=0; iEdge<ctEdges; iEdge++) {
    // set the two vertices
    CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[iEdge];
    CWorkingEdge &wed = *bpe.bpe_pbedEdge->bed_pwedWorking;
    if (bpe.bpe_bReverse^bInvert) {
      ai[iEdge*2+0] = wed.wed_iwvx1+re_iViewVx0;
      ai[iEdge*2+1] = wed.wed_iwvx0+re_iViewVx0;
    } else {
      ai[iEdge*2+0] = wed.wed_iwvx0+re_iViewVx0;
      ai[iEdge*2+1] = wed.wed_iwvx1+re_iViewVx0;
    }
  }
}

// make final edges for all polygons in current sector
void CRenderer::MakeFinalPolygonEdges(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_MAKEFINALPOLYGONEDGES);
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

    INDEX iEdgeVx0New = re_aiEdgeVxMain.Count();
    INDEX *piVertices = re_aiEdgeVxMain.Push(spo.spo_ctEdgeVx);

    // for each vertex
    INDEX ivxTop = spo.spo_iEdgeVx0+spo.spo_ctEdgeVx;
    for(INDEX ivx=spo.spo_iEdgeVx0; ivx<ivxTop; ivx++) {
      // copy to final array
      *piVertices++ = re_aiEdgeVxClipSrc[ivx];
    }

    // remember new edge vertex positions
    spo.spo_iEdgeVx0 = iEdgeVx0New;
  }

  // clear temporary arrays
  re_aiEdgeVxClipSrc.PopAll();
  re_aiEdgeVxClipDst.PopAll();
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_MAKEFINALPOLYGONEDGES);
}

// clip all polygons to one clip plane
void CRenderer::ClipToOnePlane(const FLOATplane3D &plView)
{
  // remember clip plane
  re_plClip = plView;
  // no need for clipping if no vertices are outside
  ASSERT( re_pbscCurrent->bsc_ulFlags&BSCF_NEEDSCLIPPING);
  if( !MakeOutcodes()) return;

  // for each polygon
  INDEX ispo0 = re_pbscCurrent->bsc_ispo0;
  INDEX ispoTop = re_pbscCurrent->bsc_ispo0+re_pbscCurrent->bsc_ctspo;
  for(INDEX ispo = ispo0; ispo<ispoTop; ispo++) {
    // clip to the plane
    ClipOnePolygon(re_aspoScreenPolygons[ispo]);
  }

  // swap edge buffers
  Swap(re_aiEdgeVxClipSrc.sa_Count    , re_aiEdgeVxClipDst.sa_Count    );
  Swap(re_aiEdgeVxClipSrc.sa_Array    , re_aiEdgeVxClipDst.sa_Array    );
  Swap(re_aiEdgeVxClipSrc.sa_UsedCount, re_aiEdgeVxClipDst.sa_UsedCount);
  re_aiEdgeVxClipDst.PopAll();
}

// clip all polygons to all clip planes of a projection
void CRenderer::ClipToAllPlanes(CAnyProjection3D &pr)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_CLIPTOALLPLANES);
  // clip to up/down/left/right clip planes
  FLOATplane3D pl;
  pl = pr->pr_plClipU;  pl.Offset(-0.001f);  ClipToOnePlane(pl);
  pl = pr->pr_plClipD;  pl.Offset(-0.001f);  ClipToOnePlane(pl);
  pl = pr->pr_plClipL;  pl.Offset(-0.001f);  ClipToOnePlane(pl);
  pl = pr->pr_plClipR;  pl.Offset(-0.001f);  ClipToOnePlane(pl);
  // clip to near clip plane
  ClipToOnePlane(FLOATplane3D(FLOAT3D(0,0,-1), pr->pr_NearClipDistance));
  // clip to far clip plane if existing
  if (pr->pr_FarClipDistance>0) {
    ClipToOnePlane(FLOATplane3D(FLOAT3D(0,0,1), -pr->pr_FarClipDistance));
  }

  // if projection is mirrored or warped
  if (pr->pr_bMirror||pr->pr_bWarp) {
    // clip to mirror plane
    ClipToOnePlane(pr->pr_plMirrorView);
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_CLIPTOALLPLANES);
}

// make outcodes for current clip plane for all active vertices
__forceinline BOOL CRenderer::MakeOutcodes(void)
{
  SLONG slMask = 0;
  // for each active view vertex
  INDEX iVxTop = re_avvxViewVertices.Count();
  for(INDEX ivx = re_iViewVx0; ivx<iVxTop; ivx++) {
    CViewVertex &vvx = re_avvxViewVertices[ivx];
    // calculate the distance
    vvx.vvx_fD = re_plClip.PointDistance(vvx.vvx_vView);
    // calculate the outcode
    const ULONG ulOutCode = (*(SLONG*)&vvx.vvx_fD) & 0x80000000;
    // add to the outcode of the vertex
    vvx.vvx_ulOutcode = (vvx.vvx_ulOutcode>>1)|ulOutCode;
    // add to mask
    slMask|=ulOutCode;
  }
  // if any was negative, return true -- needs clipping
  return slMask;
}

// clip one polygon to current clip plane
void CRenderer::ClipOnePolygon(CScreenPolygon &spo)
{
//
// NOTE: There is one ugly problem with this loop.
// I don't know any better way to fix it, so I have commented it.
// If the vertex references (vvx0 and vvx1) are taken _before_ pushing the vvxNew,
// it can cause access violation, because pushing can move array in memory.
// Therefore, it is important to take the references _after_ the Push() call.
  
  INDEX iEdgeVx0New = re_aiEdgeVxClipDst.Count();

  // for each edge
  INDEX ivxTop = spo.spo_iEdgeVx0+spo.spo_ctEdgeVx;
  for(INDEX ivx=spo.spo_iEdgeVx0; ivx<ivxTop; ivx+=2) {
    INDEX ivx0 = re_aiEdgeVxClipSrc[ivx+0];
    INDEX ivx1 = re_aiEdgeVxClipSrc[ivx+1];

    // get vertices
    FLOAT fD0 = re_avvxViewVertices[ivx0].vvx_fD;
    FLOAT fD1 = re_avvxViewVertices[ivx1].vvx_fD;
    if (fD0<=0) {
      // if both are back
      if (fD1<=0) {
        // no screen edge remains
        continue;
      // if first is back, second front
      } else {
        // make new vertex
        INDEX ivxNew = re_avvxViewVertices.Count();
        CViewVertex &vvxNew = re_avvxViewVertices.Push();
        CViewVertex &vvx0 = re_avvxViewVertices[ivx0];  // must do this after push, see note above!
        CViewVertex &vvx1 = re_avvxViewVertices[ivx1];  // must do this after push, see note above!
        // clip first
        FLOAT fDivisor = 1.0f/(fD0-fD1);
        FLOAT fFactor = fD0*fDivisor;
        vvxNew.vvx_vView(1) = vvx0.vvx_vView(1)-(vvx0.vvx_vView(1)-vvx1.vvx_vView(1))*fFactor;
        vvxNew.vvx_vView(2) = vvx0.vvx_vView(2)-(vvx0.vvx_vView(2)-vvx1.vvx_vView(2))*fFactor;
        vvxNew.vvx_vView(3) = vvx0.vvx_vView(3)-(vvx0.vvx_vView(3)-vvx1.vvx_vView(3))*fFactor;
        // remember new edge
        re_aiEdgeVxClipDst.Push() = ivxNew;
        re_aiEdgeVxClipDst.Push() = ivx1;
        // add new vertex to clip buffer
        re_aiClipBuffer.Push() = ivxNew;
      }
    } else {
      // if first is front, second back
      if ((SLONG&)fD1<=0) {
        // make new vertex
        INDEX ivxNew = re_avvxViewVertices.Count();
        CViewVertex &vvxNew = re_avvxViewVertices.Push();
        CViewVertex &vvx0 = re_avvxViewVertices[ivx0];  // must do this after push, see note above!
        CViewVertex &vvx1 = re_avvxViewVertices[ivx1];  // must do this after push, see note above!
        // clip second
        FLOAT fDivisor = 1.0f/(fD0-fD1);
        FLOAT fFactor = fD1*fDivisor;
        vvxNew.vvx_vView(1) = vvx1.vvx_vView(1)-(vvx0.vvx_vView(1)-vvx1.vvx_vView(1))*fFactor;
        vvxNew.vvx_vView(2) = vvx1.vvx_vView(2)-(vvx0.vvx_vView(2)-vvx1.vvx_vView(2))*fFactor;
        vvxNew.vvx_vView(3) = vvx1.vvx_vView(3)-(vvx0.vvx_vView(3)-vvx1.vvx_vView(3))*fFactor;
        // remember new edge
        re_aiEdgeVxClipDst.Push() = ivx0;
        re_aiEdgeVxClipDst.Push() = ivxNew;
        // add new vertex to clip buffer
        re_aiClipBuffer.Push() = ivxNew;
      // if both are front
      } else {
        // just copy the edge
        re_aiEdgeVxClipDst.Push() = ivx0;
        re_aiEdgeVxClipDst.Push() = ivx1;
      }
    }
  }

  // if there is anything in clip buffer
  if (re_aiClipBuffer.Count()>0) {
    // generate clip edges
    GenerateClipEdges(spo);
  }

  // remember new edge vertex positions
  spo.spo_ctEdgeVx = re_aiEdgeVxClipDst.Count()-iEdgeVx0New;
  spo.spo_iEdgeVx0 = iEdgeVx0New;
}

/*
 * Compare two vertices for quick-sort.
 */
static UBYTE *_aVertices=NULL;
static int qsort_CompareVertices_plus( const void *ppvVertex0, const void *ppvVertex1)
{
  INDEX ivx0 = *(const INDEX*)ppvVertex0;
  INDEX ivx1 = *(const INDEX*)ppvVertex1;
  FLOAT f0 = *(FLOAT*)(_aVertices+ivx0*sizeof(CViewVertex));
  FLOAT f1 = *(FLOAT*)(_aVertices+ivx1*sizeof(CViewVertex));
       if (f0<f1) return -1;
  else if (f0>f1) return +1;
  else            return  0;
}
static int qsort_CompareVertices_minus( const void *ppvVertex0, const void *ppvVertex1)
{
  INDEX ivx0 = *(const INDEX*)ppvVertex0;
  INDEX ivx1 = *(const INDEX*)ppvVertex1;
  FLOAT f0 = *(FLOAT*)(_aVertices+ivx0*sizeof(CViewVertex));
  FLOAT f1 = *(FLOAT*)(_aVertices+ivx1*sizeof(CViewVertex));
       if (f0<f1) return +1;
  else if (f0>f1) return -1;
  else            return  0;
}

// generate clip edges for one polygon
void CRenderer::GenerateClipEdges(CScreenPolygon &spo)
{
  ASSERT(re_aiClipBuffer.Count()>0);

  FLOATplane3D &plPolygonPlane = spo.spo_pbpoBrushPolygon->bpo_pbplPlane->bpl_pwplWorking->wpl_plView;
  // calculate the clip buffer direction in 3d as:
  //   clip_normal_vector x polygon_normal_vector
  FLOAT3D vClipDir = ((FLOAT3D &)re_plClip)*((FLOAT3D &)plPolygonPlane);

  // get max axis
  INDEX iMaxAxis = 1;
  FLOAT fMaxAbs = Abs(vClipDir(1));
  if (Abs(vClipDir(2))>fMaxAbs) {
    iMaxAxis = 2;
    fMaxAbs = Abs(vClipDir(2));
  }
  if (Abs(vClipDir(3))>fMaxAbs) {
    iMaxAxis = 3;
    fMaxAbs = Abs(vClipDir(3));
  }

  _aVertices = (UBYTE*) &re_avvxViewVertices[0].vvx_vView(iMaxAxis);

  INDEX *aIndices = &re_aiClipBuffer[0];
  INDEX ctIndices = re_aiClipBuffer.Count();

  // there must be even number of vertices in the buffer
  ASSERT(ctIndices%2 == 0);

  // if the sign of axis is negative
  if (vClipDir(iMaxAxis)<0) {
    // sort them inversely
    qsort(aIndices, ctIndices, sizeof(INDEX), qsort_CompareVertices_minus);
  // if it is negative
  } else {
    // sort them normally
    qsort(aIndices, ctIndices, sizeof(INDEX), qsort_CompareVertices_plus);
  }

  // for each two vertices
  for(INDEX iClippedVertex=0; iClippedVertex<ctIndices; iClippedVertex+=2) {
    // add the edge
    re_aiEdgeVxClipDst.Push() = aIndices[iClippedVertex+0];
    re_aiEdgeVxClipDst.Push() = aIndices[iClippedVertex+1];
  }

  // clear the clip buffer
  re_aiClipBuffer.PopAll();
}
