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

void CRenderer::DrawBrushPolygonVerticesAndEdges(CBrushPolygon &bpo)
{
  // get transformed polygon's plane
  CWorkingPlane &wplPolygonPlane = *bpo.bpo_pbplPlane->bpl_pwplWorking;
  CBrushSector &bsc = *bpo.bpo_pbscSector;
  CBrushMip *pbm = bsc.bsc_pbmBrushMip;
  CBrush3D &br = *pbm->bm_pbrBrush;
  //INDEX iMinVx = bsc.bsc_ivvx0;
  //INDEX iMaxVx = bsc.bsc_ivvx0+bsc.bsc_awvxVertices.Count();

  // set line type and color for edges and vertices
  ULONG ulEdgesLineType = EdgeLineType(wplPolygonPlane.wpl_bVisible);
  COLOR colorEdges = ColorForEdges(bpo.bpo_colColor, bpo.bpo_pbscSector->bsc_colColor);
  COLOR colorVertices = ColorForVertices(bpo.bpo_colColor, bpo.bpo_pbscSector->bsc_colColor);
  
  if (bpo.IsSelected(BPOF_SELECTED)) {
    // for all edges in the polygon
    INDEX cttri = bpo.bpo_aiTriangleElements.Count()/3;
    for(INDEX itri=0; itri<cttri; itri++) {
      for(INDEX ied=0; ied<3; ied++) {
    
      // get transformed end vertices
      INDEX ielem0 = bpo.bpo_aiTriangleElements[itri*3+ied];
      INDEX ielem1 = bpo.bpo_aiTriangleElements[itri*3+(ied+1)%3];
      CBrushVertex &bvx0 = *bpo.bpo_apbvxTriangleVertices[ielem0];
      CBrushVertex &bvx1 = *bpo.bpo_apbvxTriangleVertices[ielem1];
      INDEX ivx0 = bsc.bsc_abvxVertices.Index(&bvx0);
      INDEX ivx1 = bsc.bsc_abvxVertices.Index(&bvx1);
      FLOAT3D &tv0 = re_avvxViewVertices[bsc.bsc_ivvx0+ivx0].vvx_vView;
      FLOAT3D &tv1 = re_avvxViewVertices[bsc.bsc_ivvx0+ivx1].vvx_vView;
    
      // clip the edge line
      FLOAT3D vClipped0 = tv0;
      FLOAT3D vClipped1 = tv1;
      ULONG ulClipFlags = br.br_prProjection->ClipLine(vClipped0, vClipped1);
      // if the edge remains after clipping to front plane
      if (ulClipFlags != LCF_EDGEREMOVED) {
        // project the vertices
        FLOAT3D v3d0, v3d1;
        br.br_prProjection->PostClip(vClipped0, v3d0);
        br.br_prProjection->PostClip(vClipped1, v3d1);
        // make 2d vertices
        FLOAT fI0 = v3d0(1); FLOAT fJ0 = v3d0(2);
        FLOAT fI1 = v3d1(1); FLOAT fJ1 = v3d1(2);
      
        // if the edge is too short
        if( Abs(fI1-fI0)<2 && fabs(fJ1-fJ0)<2) {
          // skip it
          continue;
        }

        // draw line between vertices
        re_pdpDrawPort->DrawLine((PIX)fI0, (PIX)fJ0, (PIX)fI1, (PIX)fJ1,
          colorEdges|CT_OPAQUE, ulEdgesLineType);
      }
    }}
  }

  FOREACHINSTATICARRAY(bpo.bpo_abpePolygonEdges, CBrushPolygonEdge, itpe) {
    // get transformed end vertices
    CBrushVertex &bvx0 = *itpe->bpe_pbedEdge->bed_pbvxVertex0;
    CBrushVertex &bvx1 = *itpe->bpe_pbedEdge->bed_pbvxVertex1;
    INDEX ivx0 = bsc.bsc_abvxVertices.Index(&bvx0);
    INDEX ivx1 = bsc.bsc_abvxVertices.Index(&bvx1);
    FLOAT3D &tv0 = re_avvxViewVertices[bsc.bsc_ivvx0+ivx0].vvx_vView;
    FLOAT3D &tv1 = re_avvxViewVertices[bsc.bsc_ivvx0+ivx1].vvx_vView;
    
    // clip the edge line
    FLOAT3D vClipped0 = tv0;
    FLOAT3D vClipped1 = tv1;
    ULONG ulClipFlags = br.br_prProjection->ClipLine(vClipped0, vClipped1);
    // if the edge remains after clipping to front plane
    if (ulClipFlags != LCF_EDGEREMOVED) {
      // project the vertices
      FLOAT3D v3d0, v3d1;
      br.br_prProjection->PostClip(vClipped0, v3d0);
      br.br_prProjection->PostClip(vClipped1, v3d1);
      // make 2d vertices
      FLOAT fI0 = v3d0(1); FLOAT fJ0 = v3d0(2);
      FLOAT fI1 = v3d1(1); FLOAT fJ1 = v3d1(2);
      
      // if the edge is too short
      if( Abs(fI1-fI0)<2 && fabs(fJ1-fJ0)<2) {
        // skip it
        continue;
      }

      BOOL bDrawVertex0 = ulClipFlags&LCFVERTEX0(LCF_UNCLIPPED) && !(bvx0.bvx_ulFlags&BVXF_DRAWNINWIREFRAME);
      BOOL bDrawVertex1 = ulClipFlags&LCFVERTEX1(LCF_UNCLIPPED) && !(bvx1.bvx_ulFlags&BVXF_DRAWNINWIREFRAME);
      // if edges should be drawn
      if (_wrpWorldRenderPrefs.wrp_ftEdges != CWorldRenderPrefs::FT_NONE) {
        // draw line between vertices
        re_pdpDrawPort->DrawLine((PIX)fI0, (PIX)fJ0, (PIX)fI1, (PIX)fJ1,
          colorEdges|CT_OPAQUE, ulEdgesLineType);

        #if 0 // used for debugging edge directions
          DrawArrow(*re_pdpDrawPort, (PIX)fI0, (PIX)fJ0, (PIX)fI1, (PIX)fJ1,
            colorEdges|CT_OPAQUE, ulEdgesLineType);
        #endif
      }

      extern void SelectVertexOnRender(CBrushVertex &bvx, const PIX2D &vpix);

      if (_wrpWorldRenderPrefs.wrp_stSelection== CWorldRenderPrefs::ST_VERTICES) {
        // draw them
        if (bDrawVertex0) {
          SelectVertexOnRender(bvx0, PIX2D((PIX)fI0, (PIX)fJ0));
          if (bvx0.bvx_ulFlags&BVXF_SELECTED) {
            PutMoreFatPixel(*re_pdpDrawPort, (PIX)fI0, (PIX)fJ0, C_BLACK|CT_OPAQUE);
          }
          bvx0.bvx_ulFlags|=BVXF_DRAWNINWIREFRAME;
        }
        if (bDrawVertex1) {
          SelectVertexOnRender(bvx1, PIX2D((PIX)fI1, (PIX)fJ1));
          if (bvx1.bvx_ulFlags&BVXF_SELECTED) {
            PutMoreFatPixel(*re_pdpDrawPort, (PIX)fI1, (PIX)fJ1, C_BLACK|CT_OPAQUE);
          }
          bvx1.bvx_ulFlags|=BVXF_DRAWNINWIREFRAME;
        }
      }
      // if vertices should be drawn
      if (_wrpWorldRenderPrefs.wrp_ftVertices != CWorldRenderPrefs::FT_NONE) {
        // draw them
        if (bDrawVertex0) {
          PutFatPixel(*re_pdpDrawPort, (PIX)fI0, (PIX)fJ0, colorVertices);
          bvx0.bvx_ulFlags|=BVXF_DRAWNINWIREFRAME;
        }
        if (bDrawVertex1) {
          PutFatPixel(*re_pdpDrawPort, (PIX)fI1, (PIX)fJ1, colorVertices);
          bvx1.bvx_ulFlags|=BVXF_DRAWNINWIREFRAME;
        }
      }
    }
  }
}

/*
 * Draw vertices and/or edges of a brush
 */
void CRenderer::DrawBrushSectorVerticesAndEdges(CBrushSector &bscSector)
{
  //CBrushMip *pbm = bscSector.bsc_pbmBrushMip;
  //CBrush3D &br = *pbm->bm_pbrBrush;

  // clear all vertex drawn flags
  FOREACHINSTATICARRAY(bscSector.bsc_abvxVertices, CBrushVertex, itbvx) {
    itbvx->bvx_ulFlags&=~BVXF_DRAWNINWIREFRAME;
  }

  // first render visible polygons
  FOREACHINSTATICARRAY(bscSector.bsc_abpoPolygons, CBrushPolygon, itpo) {
    CBrushPolygon &bpo = *itpo;
    CWorkingPlane &wplPolygonPlane = *bpo.bpo_pbplPlane->bpl_pwplWorking;
    if (wplPolygonPlane.wpl_bVisible) {
      DrawBrushPolygonVerticesAndEdges(bpo);
    }
  }
  // if hidden edges should be drawn
  if (_wrpWorldRenderPrefs.wrp_bHiddenLinesOn) {
    // render invisible polygons
    FOREACHINSTATICARRAY(bscSector.bsc_abpoPolygons, CBrushPolygon, itpo) {
      CBrushPolygon &bpo = *itpo;
      CWorkingPlane &wplPolygonPlane = *bpo.bpo_pbplPlane->bpl_pwplWorking;
      if (!wplPolygonPlane.wpl_bVisible) {
        DrawBrushPolygonVerticesAndEdges(bpo);
      }
    }
  }
}
/* Draw edges of a field brush sector. */
void CRenderer::DrawFieldBrushSectorEdges(CBrushSector &bscSector)
{
  CBrushMip *pbm = bscSector.bsc_pbmBrushMip;
  CBrush3D &br = *pbm->bm_pbrBrush;
  // for all polygons in sector
  FOREACHINSTATICARRAY(bscSector.bsc_abpoPolygons, CBrushPolygon, itpo) {
    CBrushPolygon &bpo = *itpo;
    
    // get transformed polygon's plane
    CWorkingPlane &wplPolygonPlane = *bpo.bpo_pbplPlane->bpl_pwplWorking;
    
    // set line type and color for edges and vertices
    ULONG ulEdgesLineType = EdgeLineType(wplPolygonPlane.wpl_bVisible);
    COLOR colorEdges = _wrpWorldRenderPrefs.wrp_colEdges;
    
    // for all edges in the polygon
    FOREACHINSTATICARRAY(itpo->bpo_abpePolygonEdges, CBrushPolygonEdge, itpe) {
      
      // get transformed end vertices
      INDEX ivx0 = bscSector.bsc_abvxVertices.Index(itpe->bpe_pbedEdge->bed_pbvxVertex0);
      INDEX ivx1 = bscSector.bsc_abvxVertices.Index(itpe->bpe_pbedEdge->bed_pbvxVertex1);
      FLOAT3D &tv0 = re_avvxViewVertices[bscSector.bsc_ivvx0+ivx0].vvx_vView;
      FLOAT3D &tv1 = re_avvxViewVertices[bscSector.bsc_ivvx0+ivx1].vvx_vView;
      
      // clip the edge line
      FLOAT3D vClipped0 = tv0;
      FLOAT3D vClipped1 = tv1;
      ULONG ulClipFlags = br.br_prProjection->ClipLine(vClipped0, vClipped1);
      // if the edge remains after clipping to front plane
      if (ulClipFlags != LCF_EDGEREMOVED) {
        // project the vertices
        FLOAT3D v3d0, v3d1;
        br.br_prProjection->PostClip(vClipped0, v3d0);
        br.br_prProjection->PostClip(vClipped1, v3d1);
        // make 2d vertices
        FLOAT2D v2d0, v2d1;
        v2d0(1) = v3d0(1); v2d0(2) = v3d0(2);
        v2d1(1) = v3d1(1); v2d1(2) = v3d1(2);
        
        re_pdpDrawPort->DrawLine((PIX)v2d0(1), (PIX)v2d0(2), 
          (PIX)v2d1(1), (PIX)v2d1(2), colorEdges|CT_OPAQUE, ulEdgesLineType);
      }
    }
  }
}

/* Prepare a brush entity for rendering if it is not yet prepared. */
void CRenderer::PrepareBrush(CEntity *penBrush)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_PREPAREBRUSH);
  ASSERT(penBrush!=NULL);
  // get its brush
  CBrush3D &brBrush = *penBrush->en_pbrBrush;
  // if the brush is already active in rendering
  if (brBrush.br_lnInActiveBrushes.IsLinked()) {
    // skip it
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_PREPAREBRUSH);
    return;
  }
  
  brBrush.br_ulFlags&=~BRF_DRAWFIRSTMIP;

  // if it has zero sectors and rendering of editor models is enabled
  if (brBrush.GetFirstMip()->bm_abscSectors.Count()==0
   && _wrpWorldRenderPrefs.IsEditorModelsOn() && wld_bRenderEmptyBrushes) {
    // add it for delayed rendering as a model (will use empty brush model)
    AddModelEntity(penBrush);
  }

  // add it to list of active brushes
  re_lhActiveBrushes.AddTail(brBrush.br_lnInActiveBrushes);
  // add it to container of all drawn entities
  re_cenDrawn.Add(penBrush);

  // set up a projection for the brush
  if (re_bBackgroundEnabled && (penBrush->en_ulFlags & ENF_BACKGROUND)) {
    brBrush.br_prProjection = re_prBackgroundProjection;
  } else {
    brBrush.br_prProjection = re_prProjection;
  }

  // prepare the brush projection
  if (penBrush->en_ulPhysicsFlags&EPF_MOVABLE) {
    // for moving brushes    
    brBrush.br_prProjection->ObjectPlacementL() = penBrush->GetLerpedPlacement();
    brBrush.br_prProjection->ObjectStretchL() = FLOAT3D(1.0f, 1.0f, 1.0f);
    brBrush.br_prProjection->ObjectFaceForwardL() = FALSE;
    brBrush.br_prProjection->Prepare();
  } else {
    // for static brushes
    CProjection3D &pr = *brBrush.br_prProjection;
    const FLOATmatrix3D &mRot = penBrush->en_mRotation;
    //const FLOAT3D       &vRot = penBrush->en_plPlacement.pl_PositionVector;
    // fixup projection to use placement of this brush
    pr.pr_mDirectionRotation = pr.pr_ViewerRotationMatrix*mRot;
    pr.pr_RotationMatrix     = pr.pr_mDirectionRotation;
    brBrush.br_prProjection->ObjectPlacementL() = penBrush->en_plPlacement;
    pr.pr_TranslationVector = pr.pr_ObjectPlacement.pl_PositionVector - pr.pr_vViewerPosition;
    pr.pr_TranslationVector = pr.pr_TranslationVector*pr.pr_ViewerRotationMatrix;
    pr.pr_Prepared = TRUE;
  }

  // mark brush as selected if the entity is selected
  if (penBrush->IsSelected(ENF_SELECTED)) {
    brBrush.br_ulFlags |= BRF_DRAWSELECTED;
  } else {
    brBrush.br_ulFlags &= ~BRF_DRAWSELECTED;
  }
  _pfRenderProfile.StopTimer(CRenderProfile::PTI_PREPAREBRUSH);
}


/*
 * Render wireframe brushes.
 */
void CRenderer::RenderWireFrameBrushes(void)
{
  BOOL bRenderNonField =
     _wrpWorldRenderPrefs.   wrp_ftEdges != CWorldRenderPrefs::FT_NONE
   ||_wrpWorldRenderPrefs.wrp_ftVertices != CWorldRenderPrefs::FT_NONE
   ||_wrpWorldRenderPrefs.wrp_stSelection== CWorldRenderPrefs::ST_VERTICES;

  // for all active sectors
  {FORDELETELIST(CBrushSector, bsc_lnInActiveSectors, re_lhActiveSectors, itbsc) {
    CBrushSector &bsc = *itbsc;
    // if invisible
    if (bsc.bsc_ulFlags&BSCF_INVISIBLE) {
      // skip it
      continue;
    }

    // if it is field brush
    if (bsc.bsc_pbmBrushMip->bm_pbrBrush->br_pfsFieldSettings!=NULL) {
      // if fields should be drawn
      if (_wrpWorldRenderPrefs.IsFieldBrushesOn()) {
        // draw it (all brush sectors in the list are already prepared and transformed)
        DrawFieldBrushSectorEdges(bsc);
      }
    } else {
      if (bRenderNonField) {
        // draw it (all brush sectors in the list are already prepared and transformed)
        DrawBrushSectorVerticesAndEdges(bsc);
      }
    }
  }}
}

/*
 * Compare two polygons for sorting.
 */
static inline int CompareTranslucentPolygons(
  const CTranslucentPolygon &tp0, const CTranslucentPolygon &tp1)
{
       if (tp0.tp_fViewerDistance<tp1.tp_fViewerDistance) return -1;
  else if (tp0.tp_fViewerDistance>tp1.tp_fViewerDistance) return +1;
  else                                                    return  0;
}
static int qsort_CompareTranslucentPolygons( const void *pptp0, const void *pptp1)
{
  CTranslucentPolygon &tp0 = **(CTranslucentPolygon **)pptp0;
  CTranslucentPolygon &tp1 = **(CTranslucentPolygon **)pptp1;
  return +CompareTranslucentPolygons(tp0, tp1);
}

/*
 * Sort a list of translucent polygons.
 */
ScenePolygon *CRenderer::SortTranslucentPolygons(ScenePolygon *pspoFirst)
{
  // if there are no polygons in list
  if (pspoFirst==NULL) {
    // do nothing
    return NULL;
  }

  // for each polygon in list
  for (ScenePolygon *pspo = pspoFirst; pspo!=NULL; pspo = pspo->spo_pspoSucc) {
    // add it to container for sorting
    CTranslucentPolygon &tp = re_atcTranslucentPolygons.Push();
    tp.tp_pspoPolygon = pspo;
    tp.tp_fViewerDistance = ((CBrushPolygon*)pspo->spo_pvPolygon)->bpo_pbplPlane
      ->bpl_pwplWorking->wpl_plView.Distance();
  }

  // sort the container
  qsort(re_atcTranslucentPolygons.GetArrayOfPointers(), re_atcTranslucentPolygons.Count(),
      sizeof(CTranslucentPolygon *), qsort_CompareTranslucentPolygons);

  // make empty new list of polygons
  ScenePolygon *pspoNewFirst = NULL;
  // for each polygon in container
  for(INDEX iPolygon=0; iPolygon<re_atcTranslucentPolygons.Count(); iPolygon++) {
    ScenePolygon *pspo = re_atcTranslucentPolygons[iPolygon].tp_pspoPolygon;
    // add it to new list
    pspo->spo_pspoSucc = pspoNewFirst;
    pspoNewFirst = pspo;
  }

  // clear containr for future use
  re_atcTranslucentPolygons.Clear();

  // return new list
  return pspoNewFirst;
}
