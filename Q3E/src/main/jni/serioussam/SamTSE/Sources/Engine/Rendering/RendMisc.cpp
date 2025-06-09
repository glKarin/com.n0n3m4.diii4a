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
// CWorldRenderPrefs

// global instance used in rendering
CWorldRenderPrefs _wrpWorldRenderPrefs;
CBrushSectorSelection *_pselbscVisTweaks = NULL;

/*
 * Constructor -- sets default values.
 */
CWorldRenderPrefs::CWorldRenderPrefs(void)
{
  wrp_bHiddenLinesOn = TRUE;
  wrp_bEditorModelsOn = FALSE;
  wrp_bFieldBrushesOn = FALSE;
  wrp_bShowTargetsOn = FALSE;
  wrp_bShowEntityNames = FALSE;
  wrp_bBackgroundTextureOn = TRUE;
  wrp_bShowVisTweaksOn = FALSE;

  wrp_ftVertices = FT_NONE;
  wrp_colVertices = C_RED;

  wrp_ftEdges = FT_NONE;
  wrp_colEdges = C_BLACK;
                              
  wrp_ftPolygons = FT_TEXTURE;
  wrp_colPolygons = C_WHITE;

  wrp_shtShadows = SHT_FULL;
  wrp_lftLensFlares = LFT_REFLECTIONS_AND_GLARE;

  wrp_abTextureLayers[0] = TRUE;
  wrp_abTextureLayers[1] = TRUE;
  wrp_abTextureLayers[2] = TRUE;

  wrp_fMinimumRenderRange = 5.0f;

  wrp_pmoSelectedEntity = NULL;
  wrp_pmoSelectedPortal = NULL;

  wrp_stSelection = ST_NONE;

  wrp_bAutoMipBrushingOn = TRUE;
  wrp_fManualMipBrushingFactor = 0.0f;
  wrp_bFogOn = TRUE;
  wrp_bHazeOn = TRUE;
  wrp_bMirrorsOn = TRUE;

  wrp_pmoSelectedEntity = NULL;
  wrp_pmoSelectedPortal = NULL;
  wrp_pmoEmptyBrush = NULL;

  wrp_fFarClipPlane=-1.0f;
  wrp_bApplyFarClipPlaneInIsometricProjection=FALSE;
}

// Get mip brushing factor relevant for given distance mip factor
FLOAT CWorldRenderPrefs::GetCurrentMipBrushingFactor(FLOAT fDistanceMipFactor)
{
  // if mip brushing is automatical
  if (wrp_bAutoMipBrushingOn) {
    // use the distance factor
    return fDistanceMipFactor;
  // if mip brushing is manual
  } else {
    // use the manual factor
    return wrp_fManualMipBrushingFactor;
  }
}

/////////////////////////////////////////////////////////////////////
// helper classes
/////////////////////////////////////////////////////////////////////

/* Destructor. */
CScreenPolygon::~CScreenPolygon(void) {
  ASSERT(spo_iInStack == 0);
};

/////////////////////////////////////////////////////////////////////
// CRenderer
/////////////////////////////////////////////////////////////////////
// helper functions

// Coordinate conversion functions
static FLOAT fDiff;
static SLONG slTmp;


static inline PIX PIXCoord(FLOAT f) // (f+0.9999f) or (ceil(f))
{
 #if (defined __MSVC_INLINE__)
  PIX pixRet;
  __asm {
    fld     dword ptr [f]
    fist    dword ptr [slTmp]
    fisubr  dword ptr [slTmp]
    fstp    dword ptr [fDiff]
    mov     eax,dword ptr [slTmp]
    mov     edx,dword ptr [fDiff]
    add     edx,0x7FFFFFFF
    adc     eax,0
    mov     dword ptr [pixRet],eax
  }
  return pixRet;

 #elif (defined __GNU_INLINE_X86_32__)
  PIX pixRet;
  SLONG clobber;
  __asm__ __volatile__ (
    "flds    (%%eax)              \n\t"
    "fistl   (%%edx)              \n\t"
    "fisubrl (%%edx)              \n\t"
    "fstps   (%%ecx)              \n\t"
    "movl    (%%edx), %%eax       \n\t"
    "movl    (%%ecx), %%edx       \n\t"
    "addl    $0x7FFFFFFF, %%edx   \n\t"
    "adcl    $0, %%eax            \n\t"
        : "=a" (pixRet), "=d" (clobber)
        : "a" (&f), "d" (&slTmp), "c" (&fDiff)
        : "cc", "memory"
  );
  return pixRet;

 #else
  return((PIX) (f+0.9999f));

 #endif
}


static inline PIX PIXCoord(FIX16_16 x) { return (PIX)Ceil(x); };


/*
 * Calculate edge line type depending on whether its polygon is visible or not.
 */
static inline ULONG EdgeLineType(BOOL bPolygonVisible)
{
  // if polygon is visible
  if (bPolygonVisible) {
    // draw all edges with full lines
    return _FULL_;
  // if polygon is not visible
  } else {
    // draw all edges with dashed lines
    return _DOT4_;
  }
}

/*
 * Draw a fat dot used for drawing vertices.
 */
static inline void PutFatPixel(CDrawPort &dp, PIX i, PIX j, COLOR color)
{
  dp.Fill(i-1,j-1, 3,3, color|CT_OPAQUE);
}

/*
 * Draw a more fat dot used for drawing vertices.
 */
static inline void PutMoreFatPixel(CDrawPort &dp, PIX i, PIX j, COLOR color)
{
  dp.Fill(i-1,j-1, 5, 5, color|CT_OPAQUE);
}

/*
 * Draw the most fat dot used for drawing vertices.
 */
static inline void PutTheMostFatPixel(CDrawPort &dp, PIX i, PIX j, COLOR color)
{
  dp.Fill(i-1,j-1, 8,8, color|CT_OPAQUE);
}

/*
 * Draw a line for arrow drawing.
 */
static inline void DrawArrowLine(CDrawPort &dp, const FLOAT2D &vPoint0,
                                 const FLOAT2D &vPoint1, COLOR color, ULONG ulLineType)
{
  PIX x0 = (PIX)vPoint0(1);
  PIX x1 = (PIX)vPoint1(1);
  PIX y0 = (PIX)vPoint0(2);
  PIX y1 = (PIX)vPoint1(2);

  dp.DrawLine(x0, y0, x1, y1, color, ulLineType);
}

/*
 * Draw an arrow for debugging edge directions.
 */
static inline void DrawArrow(CDrawPort &dp, PIX i0, PIX j0, PIX i1, PIX j1, COLOR color,
                             ULONG ulLineType)
{
  FLOAT2D vPoint0 = FLOAT2D((FLOAT)i0, (FLOAT)j0);
  FLOAT2D vPoint1 = FLOAT2D((FLOAT)i1, (FLOAT)j1);
  FLOAT2D vDelta      = vPoint1-vPoint0;
  FLOAT fDelta = vDelta.Length();
  FLOAT2D vArrowLen, vArrowWidth;
  if (fDelta>0.01) {
    vArrowLen   = vDelta/fDelta*FLOAT(10.0);
    vArrowWidth = vDelta/fDelta*FLOAT(2.0);
  } else {
    vArrowWidth = vArrowLen = FLOAT2D(0.0f, 0.0f);
  }
//  FLOAT3D vArrowLen   = vDelta/5.0f;
//  FLOAT3D vArrowWidth = vDelta/30.0f;
  Swap(vArrowWidth(1), vArrowWidth(2));
  //vArrowWidth(2) *= -1.0f;

  DrawArrowLine(dp, vPoint0, vPoint1, color, ulLineType);
  DrawArrowLine(dp, vPoint1-vArrowLen+vArrowWidth, vPoint1, color, ulLineType);
  DrawArrowLine(dp, vPoint1-vArrowLen-vArrowWidth, vPoint1, color, ulLineType);
  //DrawArrowLine(dp, vPoint0+vArrowWidth, vPoint0-vArrowWidth, color, ulLineType);
}


/*
 * Determine color to use for coloring vertices.
 */
inline COLOR CRenderer::ColorForVertices(COLOR colorPolygon, COLOR colorSector)
{
  // check vertices fill type
  switch(_wrpWorldRenderPrefs.wrp_ftVertices) {
  // if it is fixed ink
  case CWorldRenderPrefs::FT_INKCOLOR:
    // use the vertex ink
    return _wrpWorldRenderPrefs.wrp_colVertices;
    break;
  // if it is polygon
  case CWorldRenderPrefs::FT_POLYGONCOLOR:
    // use the polygon color
    return colorPolygon;
    break;
  // if it is sector
  case CWorldRenderPrefs::FT_SECTORCOLOR:
    // use the sector color
    return colorSector;
    break;
  // if it is none
  case CWorldRenderPrefs::FT_NONE:
    // return any color, doesn't matter
    return C_BLACK;
    break;
  // in any other way
  default:
    // error
    ASSERTALWAYS("Invalid fill type for vertices");
    return C_BLACK;
  };
}

/*
 * Determine color to use for coloring edges.
 */
inline COLOR CRenderer::ColorForEdges(COLOR colorPolygon, COLOR colorSector)
{
  // check edges fill type
  switch(_wrpWorldRenderPrefs.wrp_ftEdges) {
  // if it is fixed ink
  case CWorldRenderPrefs::FT_INKCOLOR:
    // use the edge ink
    return _wrpWorldRenderPrefs.wrp_colEdges;
    break;
  // if it is polygon
  case CWorldRenderPrefs::FT_POLYGONCOLOR:
    // use the polygon color
    return colorPolygon;
    break;
  // if it is sector
  case CWorldRenderPrefs::FT_SECTORCOLOR:
    // use the sector color
    return colorSector;
    break;
  // if it is none
  case CWorldRenderPrefs::FT_NONE:
    // return any color, doesn't matter
    return C_BLACK;
    break;
  // in any other way
  default:
    // error
    ASSERTALWAYS("Invalid fill type for edges");
    return C_BLACK;
  };
}

/*
 * Determine color to use for coloring polygons.
 */
inline COLOR CRenderer::ColorForPolygons(COLOR colorPolygon, COLOR colorSector)
{
  // check polygons fill type
  switch(_wrpWorldRenderPrefs.wrp_ftPolygons) {
  // if it is fixed ink
  case CWorldRenderPrefs::FT_INKCOLOR:
    // use the polygon ink
    return _wrpWorldRenderPrefs.wrp_colPolygons;
    break;
  // if it is sector
  case CWorldRenderPrefs::FT_SECTORCOLOR:
    // use the sector color
    return colorSector;
    break;
  // if it is polygon
  case CWorldRenderPrefs::FT_POLYGONCOLOR:
  // or in any other way
  default:
    // use the polygon color
    return colorPolygon;
    break;
  };
}

/*
 * Check if a polygon is selected.
 */
inline BOOL PolygonIsSelected(CBrushPolygon &bpo,
                              CBrush3D &br,
                              CBrushSector &bsc)
{
  // if any selection drawing is active
  if (_wrpWorldRenderPrefs.GetSelectionType() != CWorldRenderPrefs::ST_NONE) {
    // if polygon selection drawing is active and this polygon is selected,
    if ((_wrpWorldRenderPrefs.GetSelectionType() == CWorldRenderPrefs::ST_POLYGONS
         && bpo.IsSelected(BPOF_SELECTED))
    // or sector selection drawing is active and this polygon's sector is selected,
    ||(_wrpWorldRenderPrefs.GetSelectionType() == CWorldRenderPrefs::ST_SECTORS
         && bsc.IsSelected(BSCF_SELECTED))
    // or entity selection drawing is active and this polygon's brush is selected
    ||(_wrpWorldRenderPrefs.GetSelectionType() == CWorldRenderPrefs::ST_ENTITIES
         && (br.br_ulFlags&BRF_DRAWSELECTED))
    ) {
      return TRUE;
    }
  }
  return FALSE;
}

void CRenderer::ProjectClipAndDrawArrow(
  const FLOAT3D &v0, const FLOAT3D &v1, COLOR colColor)
{
  // get transformed end vertices
  FLOAT3D tv0, tv1;
  re_prProjection->PreClip(v0, tv0);
  re_prProjection->PreClip(v1, tv1);

  // clip the edge line
  FLOAT3D vClipped0 = tv0;
  FLOAT3D vClipped1 = tv1;
  ULONG ulClipFlags = re_prProjection->ClipLine(vClipped0, vClipped1);
  // if the edge remains after clipping to front plane
  if (ulClipFlags != LCF_EDGEREMOVED) {
    // project the vertices
    FLOAT3D v3d0, v3d1;
    re_prProjection->PostClip(vClipped0, v3d0);
    re_prProjection->PostClip(vClipped1, v3d1);
    // make 2d vertices
    FLOAT2D v2d0, v2d1;
    v2d0(1) = v3d0(1); v2d0(2) = v3d0(2);
    v2d1(1) = v3d1(1); v2d1(2) = v3d1(2);

    // draw arrow-headed line between vertices
    DrawArrow(*re_pdpDrawPort, (PIX)v2d0(1), (PIX)v2d0(2), 
                             (PIX)v2d1(1), (PIX)v2d1(2), colColor, _FULL_);
  }
}

/*
 * Render target lines for each drawn entity that has some targets.
 */
void CRenderer::RenderEntityTargets(void)
{
  // for each drawn entity
  FOREACHINDYNAMICCONTAINER(re_cenDrawn, CEntity, iten) {
    CEntity &enSource = *iten;

    // if the entity has parent
    if (enSource.en_penParent!=NULL) {
      // draw the arrow from entity to its parent
      ProjectClipAndDrawArrow(
        enSource.GetLerpedPlacement().pl_PositionVector,
        enSource.en_penParent->GetLerpedPlacement().pl_PositionVector,
        C_dBLUE|CT_OPAQUE);
    }

    // for all classes in hierarchy of this entity
    for(CDLLEntityClass *pdecDLLClass = enSource.en_pecClass->ec_pdecDLLClass;
        pdecDLLClass!=NULL; 
        pdecDLLClass = pdecDLLClass->dec_pdecBase) {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) {
        CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
        // if the property is not entity pointer
        if( (epProperty.ep_eptType!=CEntityProperty::EPT_ENTITYPTR)||
            (strlen(epProperty.ep_strName)==0) ){
          // skip it
          continue;
        }
        // get the target
        CEntity *penTarget = ENTITYPROPERTY(&enSource, epProperty.ep_slOffset, CEntityPointer);
        // if there is no target
        if (penTarget==NULL) {
          // skip it
          continue;
        }

        // draw the arrow from entity to its target
        ProjectClipAndDrawArrow(
          enSource.GetLerpedPlacement().pl_PositionVector,
          penTarget->GetLerpedPlacement().pl_PositionVector,
          epProperty.ep_colColor);
      }
    }
  }
}

extern CFontData *_pfdConsoleFont;
void CRenderer::RenderEntityNames(void)
{
  // for each of models that were kept for delayed rendering
  for( INDEX iModel=0; iModel<re_admDelayedModels.Count(); iModel++)
  {
    CDelayedModel &dm = re_admDelayedModels[iModel];
    CEntity &en = *dm.dm_penModel;
    CTString strName=en.GetName();
    if( strName=="") continue;
    if( (en.GetRenderType()==CEntity::RT_EDITORMODEL || en.GetRenderType()==CEntity::RT_SKAEDITORMODEL) &&
      !_wrpWorldRenderPrefs.IsEditorModelsOn()) continue;

    FLOATaabbox3D boxModel;
    CModelObject *pmoModelObject;
    CModelInstance *pmiModelInstance;
    // get bounding box of current frame
    if (en.GetRenderType()==CEntity::RT_MODEL || en.GetRenderType()==CEntity::RT_EDITORMODEL) {
      pmoModelObject = dm.dm_pmoModel;
      pmoModelObject->GetCurrentFrameBBox(boxModel);
      if(en.en_pciCollisionInfo!=NULL) {
        // get its collision box
        INDEX iCollision = en.GetCollisionBoxIndex();
        FLOAT3D vMin = pmoModelObject->GetCollisionBoxMin(iCollision);
        FLOAT3D vMax = pmoModelObject->GetCollisionBoxMax(iCollision);
        // extend the box by the collision box
        boxModel|=FLOATaabbox3D(vMin, vMax);
      }
      // set position of marker at top of the model and it size to be proportional to the model
      boxModel.StretchByVector(pmoModelObject->mo_Stretch);
    } else if (en.GetRenderType()==CEntity::RT_SKAMODEL || en.GetRenderType()==CEntity::RT_SKAEDITORMODEL) {
      pmiModelInstance = en.GetModelInstance();
      pmiModelInstance->GetAllFramesBBox(boxModel);
      if(en.en_pciCollisionInfo!=NULL) {  
      // get its collision box
        FLOATaabbox3D box;
        pmiModelInstance->GetCurrentColisionBox(box);
        // extend the box by the collision box
        boxModel|=box;
      }
      boxModel.StretchByVector(pmiModelInstance->mi_vStretch);
    } else {
      continue; 
    }
        
    FLOAT fSize = boxModel.Size().Length()*0.3f;
    _wrpWorldRenderPrefs.wrp_pmoSelectedEntity->mo_Stretch = FLOAT3D( fSize, fSize, fSize);
    CPlacement3D plSelection = en.GetLerpedPlacement();
    plSelection.Translate_OwnSystem( FLOAT3D(0.0f, boxModel.Max()(2), 0.0f));
    FLOAT3D vOrigin=plSelection.pl_PositionVector;
    FLOAT3D vProjected=FLOAT3D(0,0,0);
    re_prProjection->ProjectCoordinate( vOrigin, vProjected);
    if( vProjected(3)>0.0f) continue;
    FLOAT fSizeFactor=1.0f+fSize;
    FLOAT fRatio=Clamp( -vProjected(3)/fSizeFactor, 0.0f, 25.0f);
    FLOAT fPower=CalculateRatio(fRatio, 0, 25.0f, 0, 0.25f);
    if( fPower==0) continue;

    PIX pixH=re_pdpDrawPort->GetHeight();
    re_pdpDrawPort->SetFont( _pfdConsoleFont);
    UBYTE ubAlpha=UBYTE(fPower*255.0f);
    re_pdpDrawPort->PutTextC( strName, (PIX) (vProjected(1)), (PIX) (pixH-vProjected(2)), C_RED|ubAlpha);
  }
}

