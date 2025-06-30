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
#include <Engine/Rendering/Render.h>
#include <Engine/Rendering/Render_internal.h>
#include <Engine/Base/Console.h>
#include <Engine/Templates/DynamicContainer.h>
#include <Engine/Templates/DynamicContainer.cpp>

#include <Engine/Light/LightSource.h>
#include <Engine/Light/Gradient.h>
#include <Engine/Base/ListIterator.inl>
#include <Engine/World/World.h>
#include <Engine/Entities/Entity.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Math/Clipping.inl>
#include <Engine/Entities/EntityClass.h>
#include <Engine/World/WorldSettings.h>
#include <Engine/Entities/EntityProperties.h>
#include <Engine/Entities/FieldSettings.h>
#include <Engine/Entities/ShadingInfo.h>
#include <Engine/Light/LensFlares.h>
#include <Engine/Models/ModelObject.h>
#include <Engine/Models/RenderModel.h>
#include <Engine/Ska/Render.h>
#include <Engine/Terrain/Terrain.h>
#include <Engine/Templates/BSP.h>
#include <Engine/World/WorldEditingProfile.h>
#include <Engine/Brushes/BrushArchive.h>
#include <Engine/Math/Float.h>
#include <Engine/Math/OBBox.h>
#include <Engine/Math/Geometry.inl>

#include <Engine/Graphics/DrawPort.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/Fog_internal.h>

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Rendering/RenderProfile.h>

#include <Engine/Templates/LinearAllocator.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>
#include <Engine/Templates/DynamicStackArray.cpp>

extern BOOL _bSomeDarkExists;
extern INDEX d3d_bAlternateDepthReads;

// general coordinate stack referenced by the scene polygons
extern CStaticStackArray<GFXVertex3> _avtxScene;


//#pragma optimize ("gt", on)
#pragma inline_depth(255)
#pragma inline_recursion(on)

#ifndef NDEBUG
//#define ASER_EXTREME_CHECKING 1
#endif

// the renderer structures used in rendering
#define MAX_RENDERERS 2
static CRenderer _areRenderers[MAX_RENDERERS];
static BOOL _bMirrorDrawn = FALSE;

extern INDEX wld_bAlwaysAddAll;
extern INDEX wld_bRenderEmptyBrushes;
extern INDEX wld_bRenderDetailPolygons;
extern INDEX gfx_bRenderParticles;
extern INDEX gfx_bRenderModels;
extern INDEX gfx_bRenderFog;
extern INDEX gfx_bRenderPredicted;
extern INDEX gfx_iLensFlareQuality;
extern BOOL _bMultiPlayer;

// variables for selection on rendering
#ifdef PLATFORM_UNIX
CBrushVertexSelection *_pselbvxtSelectOnRender = NULL;
CStaticStackArray<PIX2D> *_pavpixSelectLasso = NULL;
CEntitySelection *_pselenSelectOnRender = NULL;
PIX2D _vpixSelectNearPoint = PIX2D(0,0);
BOOL _bSelectAlternative   = FALSE;
PIX _pixDeltaAroundVertex  = 10;
#else
extern CBrushVertexSelection *_pselbvxtSelectOnRender = NULL;
extern CStaticStackArray<PIX2D> *_pavpixSelectLasso = NULL;
extern CEntitySelection *_pselenSelectOnRender = NULL;
extern PIX2D _vpixSelectNearPoint = PIX2D(0,0);
extern BOOL _bSelectAlternative   = FALSE;
extern PIX _pixDeltaAroundVertex  = 10;
#endif

// shading info for viewer of last rendered view
FLOAT3D _vViewerLightDirection;
COLOR _colViewerLight;
COLOR _colViewerAmbient;


// handy statistic helper routines

static enum CStatForm::StatTimerIndex _stiLastStatsMode = (enum CStatForm::StatTimerIndex)-1;

void StopStatsMode(void)
{
  ASSERT( (INDEX)_stiLastStatsMode != -1);
  if( _stiLastStatsMode>=0) _sfStats.StopTimer(_stiLastStatsMode);
  _stiLastStatsMode = (enum CStatForm::StatTimerIndex)-1;
}

void StartStatsMode( enum CStatForm::StatTimerIndex sti)
{
  ASSERT( (INDEX)sti != -1);
  ASSERT( (INDEX)_stiLastStatsMode == -1);
  if( sti>=0) _sfStats.StartTimer(sti);
  _stiLastStatsMode = sti;
}

void ChangeStatsMode( enum CStatForm::StatTimerIndex sti)
{
  StopStatsMode();
  StartStatsMode(sti);
}


// screen edges, polygons and trapezoids used in rasterizing
CDynamicStackArray<CAddEdge> CRenderer::re_aadeAddEdges;
CDynamicStackArray<CScreenEdge> CRenderer::re_asedScreenEdges;
// spans for current scan line
CDynamicStackArray<CSpan> CRenderer::re_aspSpans;

// vertices clipped to current clip plane
CStaticStackArray<INDEX> CRenderer::re_aiClipBuffer;
// buffers for edges of polygons
CStaticStackArray<INDEX> CRenderer::re_aiEdgeVxClipSrc;
CStaticStackArray<INDEX> CRenderer::re_aiEdgeVxClipDst;

// add and remove lists for each scan line
CStaticArray<CListHead> CRenderer::re_alhAddLists;
CStaticArray<INDEX> CRenderer::re_actAddCounts;   // count of edges in given add list
CStaticArray<CScreenEdge *> CRenderer::re_apsedRemoveFirst;
CStaticStackArray<CActiveEdge>  CRenderer::re_aaceActiveEdgesTmp;
CStaticStackArray<CActiveEdge>  CRenderer::re_aaceActiveEdges;

// container for sorting translucent polygons
CDynamicStackArray<CTranslucentPolygon> CRenderer::re_atcTranslucentPolygons;

// container for all light influencing current model
struct ModelLight {
  CLightSource *ml_plsLight;  // the light source
  FLOAT3D ml_vDirection;      // direction from light to the model position (normalized)
  FLOAT ml_fShadowIntensity;  // intensity at the model position (for shadow)
  FLOAT ml_fR, ml_fG, ml_fB;  // light components at light source (0..255)
  inline void Clear(void) {};
};
static CDynamicStackArray<struct ModelLight> _amlLights;

static INDEX _ctMaxAddEdges=0;
static INDEX _ctMaxActiveEdges=0;

void RendererInfo(void)
{
  CPrintF("Renderer information:\n");

  SLONG slMem = 0;

  slMem += CRenderer::re_aadeAddEdges.da_Count*sizeof(CAddEdge);
  slMem += CRenderer::re_asedScreenEdges.da_Count*sizeof(CScreenEdge);

  slMem += CRenderer::re_aspSpans.da_Count*sizeof(CSpan);

  slMem += CRenderer::re_aiClipBuffer.sa_Count*sizeof(INDEX);
  slMem += CRenderer::re_aiEdgeVxClipSrc.sa_Count*sizeof(INDEX);
  slMem += CRenderer::re_aiEdgeVxClipDst.sa_Count*sizeof(INDEX);

  slMem += CRenderer::re_alhAddLists.sa_Count*sizeof(CListHead);
  slMem += CRenderer::re_actAddCounts.sa_Count*sizeof(INDEX);
  slMem += CRenderer::re_apsedRemoveFirst.sa_Count*sizeof(CScreenEdge *);

  slMem += CRenderer::re_atcTranslucentPolygons.da_Count*sizeof(CTranslucentPolygon);
  slMem += CRenderer::re_aaceActiveEdges.sa_Count*sizeof(CActiveEdge);
  slMem += CRenderer::re_aaceActiveEdgesTmp.sa_Count*sizeof(CActiveEdge);

  for (INDEX ire = 0; ire<MAX_RENDERERS; ire++) {
    CRenderer &re = _areRenderers[ire];
    slMem += re.re_aspoScreenPolygons.da_Count*sizeof(CScreenPolygon);
    slMem += re.re_admDelayedModels.da_Count*sizeof(CDelayedModel);
    slMem += re.re_cenDrawn.sa_Count*sizeof(CEntity*);
    slMem += re.re_alfiLensFlares.sa_Count*sizeof(CLensFlareInfo);
    slMem += re.re_amiMirrors.da_Count*sizeof(CMirror);
    slMem += re.re_avvxViewVertices.sa_Count*sizeof(CViewVertex);
    slMem += re.re_aiEdgeVxMain.sa_Count*sizeof(INDEX);
  }

  CPrintF("Temporary memory used: %dk\n", slMem/1024);
}

void ClearRenderer(void)
{
  CRenderer::re_aadeAddEdges.Clear();
  CRenderer::re_asedScreenEdges.Clear();
  CRenderer::re_aspSpans.Clear();

  CRenderer::re_aiClipBuffer.Clear();
  CRenderer::re_aiEdgeVxClipSrc.Clear();
  CRenderer::re_aiEdgeVxClipDst.Clear();

  CRenderer::re_alhAddLists.Clear();
  CRenderer::re_actAddCounts.Clear();
  CRenderer::re_apsedRemoveFirst.Clear();
  CRenderer::re_atcTranslucentPolygons.Clear();
  CRenderer::re_aaceActiveEdges.Clear();
  CRenderer::re_aaceActiveEdgesTmp.Clear();

  for (INDEX ire = 0; ire<MAX_RENDERERS; ire++) {
    CRenderer &re = _areRenderers[ire];

    re.re_aspoScreenPolygons.Clear();
    re.re_admDelayedModels.Clear();
    re.re_cenDrawn.Clear();
    re.re_alfiLensFlares.Clear();
    re.re_amiMirrors.Clear();
    re.re_avvxViewVertices.Clear();
    re.re_aiEdgeVxMain.Clear();
  }

  CPrintF("Renderer buffers cleared.\n");
}

/*
 * How much to offset left, right, top and bottom clipping towards inside (in pixels).
 * This can be used to test clipping or to add an epsilon value for it.
 */
//#define CLIPMARGIN 10.0f    // used for debugging clipping
#define CLIPMARGIN 0.0f
#define CLIPEPSILON 0.5f
#define CLIPMARGADD (CLIPMARGIN-CLIPEPSILON)
#define CLIPMARGSUB (CLIPMARGIN+CLIPEPSILON)
#define SENTINELEDGE_EPSILON 0.4f

#include "RendMisc.cpp"
#include "RenCache.cpp"
#include "RendClip.cpp"
#include "RendASER.cpp"
#include "RenderModels.cpp"
#include "RenderBrushes.cpp"
#include "RenderAdding.cpp"

extern FLOAT wld_fEdgeOffsetI;
extern FLOAT wld_fEdgeAdjustK;

// initialize all rendering structures
void CRenderer::Initialize(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_INITIALIZATION);

  // used for fixing problems with extra trapezoids generated on t-junctions
  if( !re_bRenderingShadows) {
    re_fEdgeOffsetI = wld_fEdgeOffsetI;  //0.125f;
    re_fEdgeAdjustK = wld_fEdgeAdjustK;  //1.0001f;
  } else {
    re_fEdgeOffsetI = 0.0f;
    re_fEdgeAdjustK = 1.0f;
  }

  // prepare the raw projection (used for rendering target lines and getting object distances)
  re_prProjection->ObjectPlacementL() = CPlacement3D(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  re_prProjection->ObjectFaceForwardL() = FALSE;
  re_prProjection->ObjectStretchL() = FLOAT3D(1.0f, 1.0f, 1.0f);
  re_prProjection->DepthBufferNearL() = 0.0f;
  re_prProjection->DepthBufferFarL()  = 0.9f;
  re_prProjection->Prepare();

  re_asedScreenEdges.PopAll();
  re_aadeAddEdges.PopAll();
  re_aspSpans.PopAll();
  re_avvxViewVertices.PopAll();
  re_aiEdgeVxMain.PopAll();

  // if more scan lines are needed than last time
  if (re_alhAddLists.Count()<re_ctScanLines) {
    re_alhAddLists.Clear();
    re_alhAddLists.New(re_ctScanLines);
    re_actAddCounts.Clear();
    re_actAddCounts.New(re_ctScanLines);

    re_apsedRemoveFirst.Clear();
    re_apsedRemoveFirst.New(re_ctScanLines);
  }
  // clear all add/remove lists
  for(INDEX iScan=0; iScan<re_ctScanLines; iScan++) {
    re_actAddCounts[iScan] = 0;
    re_apsedRemoveFirst[iScan] = NULL;
  }

  // find selection color
  re_colSelection = C_RED;
  if (_wrpWorldRenderPrefs.GetSelectionType() == CWorldRenderPrefs::ST_POLYGONS) {
    re_colSelection = C_YELLOW;
  } else if (_wrpWorldRenderPrefs.GetSelectionType() == CWorldRenderPrefs::ST_SECTORS) {
    re_colSelection = C_GREEN;
  } else if (_wrpWorldRenderPrefs.GetSelectionType() == CWorldRenderPrefs::ST_ENTITIES) {
    re_colSelection = C_BLUE;
  }

  // set up renderer for first scan line
  re_iCurrentScan = 0;
  re_pixCurrentScanJ = re_iCurrentScan + re_pixTopScanLineJ;
  re_fCurrentScanJ = FLOAT(re_pixCurrentScanJ);

  // no fog or haze initially
  re_bCurrentSectorHasHaze = FALSE;
  re_bCurrentSectorHasFog  = FALSE;

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_INITIALIZATION);
}
// add initial sectors to active lists
void CRenderer::AddInitialSectors(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_ADDINITIAL);

  re_bViewerInHaze = FALSE;
  re_ulVisExclude = 0;
  re_ulVisInclude = 0;

  // if showing vis tweaks
  if (_wrpWorldRenderPrefs.wrp_bShowVisTweaksOn && _pselbscVisTweaks!=NULL) {
    // add flags for selected flags
    if (_pselbscVisTweaks->Count()>0) {
      re_ulVisExclude = VISM_INCLUDEEXCLUDE;
    }
    FOREACHINDYNAMICCONTAINER(*_pselbscVisTweaks, CBrushSector, itbsc) {
      if (itbsc->bsc_ulFlags2&BSCF2_VISIBILITYINCLUDE) {
        re_ulVisInclude = itbsc->bsc_ulVisFlags&VISM_INCLUDEEXCLUDE;
      } else {
        re_ulVisExclude &= itbsc->bsc_ulVisFlags&VISM_INCLUDEEXCLUDE;
      }
    }
  }

  // check if the background is needed
  re_bBackgroundEnabled = FALSE;
  if (!re_bRenderingShadows && _wrpWorldRenderPrefs.wrp_bBackgroundTextureOn) {
    CEntity *penBackgroundViewer = re_pwoWorld->GetBackgroundViewer();
    if (penBackgroundViewer!=NULL) {
      re_bBackgroundEnabled = TRUE;
      re_penBackgroundViewer = penBackgroundViewer;
      re_prBackgroundProjection = re_prProjection;
      CPlacement3D plViewer = re_prProjection->ViewerPlacementR();
      plViewer.pl_PositionVector = FLOAT3D(0,0,0);
      CPlacement3D plBcgViewer = penBackgroundViewer->GetLerpedPlacement();
      if (re_prProjection->pr_bMirror) {
        ReflectPositionVectorByPlane(re_prProjection->pr_plMirror, plBcgViewer.pl_PositionVector);
      }
      plViewer.RelativeToAbsoluteSmooth(plBcgViewer);
      re_prBackgroundProjection->ViewerPlacementL() = plViewer;
      re_prBackgroundProjection->ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
      re_prBackgroundProjection->FarClipDistanceL() = -1.0f;
      re_prBackgroundProjection->DepthBufferNearL() = 0.9f;
      re_prBackgroundProjection->DepthBufferFarL()  = 1.0f;
      re_prBackgroundProjection->TurnOffWarpPlane();  // background never needs warp-plane clipping
      re_prBackgroundProjection->Prepare();
    }
  }

  // if a viewer entity is given
  if (re_penViewer!=NULL) {
    // add all zoning sectors near the entity
    AddZoningSectorsAroundEntity(re_penViewer, re_prProjection->ViewerPlacementR().pl_PositionVector);
    // make sure the viewer is always added (if model)
    if(re_penViewer->en_RenderType==CEntity::RT_MODEL ||
       re_penViewer->en_RenderType==CEntity::RT_EDITORMODEL) {
      AddModelEntity(re_penViewer);
    }
  // if a viewer polygons are given
  } else if (re_pcspoViewPolygons!=NULL) {
    // for each polygon
    FOREACHINDYNAMICCONTAINER(*re_pcspoViewPolygons, CScreenPolygon, itspo) {
      CBrushPolygon *pbpo = itspo->spo_pbpoBrushPolygon;
      // get the sector, sector's brush mip, brush and entity
      CBrushSector *pbsc = pbpo->bpo_pbscSector;
      CBrushMip *pbmBrushMip = pbsc->bsc_pbmBrushMip;
      CBrush3D *pbrBrush = pbmBrushMip->bm_pbrBrush;
      ASSERT(pbrBrush!=NULL);
      CEntity *penBrush = pbrBrush->br_penEntity;
      // if the brush is zoning
      if (penBrush->en_ulFlags&ENF_ZONING) {
        // add the sector that the polygon is in
        AddGivenZoningSector(pbsc);
      // if the brush is non-zoning
      } else {
        // add sectors around it
        AddZoningSectorsAroundEntity(penBrush, penBrush->GetPlacement().pl_PositionVector);
      }
    }
  // if there is no viewer entity/polygon
  } else {
    // set up viewer bounding box as box of minimum redraw range around viewer position
    if (re_bRenderingShadows) {
      // NOTE: when rendering shadows, this is set in ::RenderShadows()
      //re_boxViewer = FLOATaabbox3D(re_prProjection->ViewerPlacementR().pl_PositionVector,
      //  1.0f);
    } else {
      re_boxViewer = FLOATaabbox3D(re_prProjection->ViewerPlacementR().pl_PositionVector,
        _wrpWorldRenderPrefs.wrp_fMinimumRenderRange);
    }
    // add all zoning sectors near viewer box
    AddZoningSectorsAroundBox(re_boxViewer);
    // NOTE: this is so entities outside of world can be edited in WEd
    // if editor models should be rendered
    if (_wrpWorldRenderPrefs.IsEditorModelsOn()) {
      // add all nonzoning entities near viewer box
      AddEntitiesInBox(re_boxViewer);
    }
  }

  if( wld_bAlwaysAddAll) {
    AddAllEntities(); // used for profiling
  } else {
    // NOTE: this is so that world can be viewed from the outside in game
    // if no brush sectors have been added so far
    if (!re_bRenderingShadows && re_lhActiveSectors.IsEmpty()) {
      // add all entities in the world
      AddAllEntities();
    }
  }

  // add the background if needed
  if (re_bBackgroundEnabled) {
    AddZoningSectorsAroundEntity(re_penBackgroundViewer, 
      re_penBackgroundViewer->GetPlacement().pl_PositionVector);
  }

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_ADDINITIAL);
}
// scan through portals for other sectors
void CRenderer::ScanForOtherSectors(void)
{
  ChangeStatsMode(CStatForm::STI_WORLDVISIBILITY);
  // if shadows or polygons should be drawn
  if (re_bRenderingShadows
    ||_wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
    // rasterize edges into spans
    ScanEdges();
  }
  // for each of models that were kept for delayed rendering
  for(INDEX iModel=0; iModel<re_admDelayedModels.Count(); iModel++) {
    // mark the entity as not active in rendering anymore
    re_admDelayedModels[iModel].dm_penModel->en_ulFlags &= ~ENF_INRENDERING;
  }
  ChangeStatsMode(CStatForm::STI_WORLDTRANSFORM);
}
// cleanup after scanning
void CRenderer::CleanupScanning(void)
{
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_CLEANUP);

  // for all active sectors
  {FORDELETELIST(CBrushSector, bsc_lnInActiveSectors, re_lhActiveSectors, itbsc) {
    // remove it from list
    itbsc->bsc_lnInActiveSectors.Remove();

    // for all polygons in sector
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itpo) {
      CBrushPolygon &bpo = *itpo;
      // clear screen polygon pointers
      bpo.bpo_pspoScreenPolygon = NULL;
    }
  }}
  ASSERT(re_lhActiveSectors.IsEmpty());

  // for all active brushes
  {FORDELETELIST(CBrush3D, br_lnInActiveBrushes, re_lhActiveBrushes, itbr) {
    // remove it from list
    itbr->br_lnInActiveBrushes.Remove();
  }}
  ASSERT(re_lhActiveBrushes.IsEmpty());

  // for all active terrains
  {FORDELETELIST(CTerrain, tr_lnInActiveTerrains, re_lhActiveTerrains, ittr) {
    // remove it from list
    ittr->tr_lnInActiveTerrains.Remove();
  }}
  ASSERT(re_lhActiveTerrains.IsEmpty());

  _pfRenderProfile.StopTimer(CRenderProfile::PTI_CLEANUP);
}

// Render active terrains
void CRenderer::RenderTerrains(void)
{
  CAnyProjection3D *papr;
  papr = &re_prProjection;

  // for all active terrains
  {FORDELETELIST(CTerrain, tr_lnInActiveTerrains, re_lhActiveTerrains, ittr) {
    // render terrain
    ittr->Render(*papr, re_pdpDrawPort);
  }}
}

// Render active terrains in wireframe mode
void CRenderer::RenderWireFrameTerrains(void)
{
  CAnyProjection3D *papr;
  papr = &re_prProjection;

  BOOL bShowEdges = _wrpWorldRenderPrefs.wrp_ftEdges != CWorldRenderPrefs::FT_NONE;
  //BOOL bShowVertices = _wrpWorldRenderPrefs.wrp_ftVertices != CWorldRenderPrefs::FT_NONE;
  // BOOL bForceRegenerate = _wrpWorldRenderPrefs.wrp_ftPolygons

  COLOR colEdges    = _wrpWorldRenderPrefs.wrp_colEdges;
  //COLOR colVertices = 0xFF0000FF;
  // for all active terrains
  {FORDELETELIST(CTerrain, tr_lnInActiveTerrains, re_lhActiveTerrains, ittr) {
    // render terrain
    if(bShowEdges) {
      ittr->RenderWireFrame(*papr, re_pdpDrawPort,colEdges);
    }
    /*if(bShowVertices) {
      //ittr->RenderVertices(*papr, re_pdpDrawPort,colVertices);
    }*/
  }}
}
// draw the prepared things to screen
void CRenderer::DrawToScreen(void)
{
  ChangeStatsMode(CStatForm::STI_WORLDRENDERING);
  
  //------------------------------------------------- first render background
  // if polygons should be drawn
  if (!re_bRenderingShadows &&
    _wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
    _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERSCENE);
    if( re_bBackgroundEnabled) {
      // render the polygons to screen
      CPerspectiveProjection3D *pprPerspective =
          (CPerspectiveProjection3D *)(CProjection3D *)(re_prBackgroundProjection);
      pprPerspective->Prepare();
      RenderScene( re_pdpDrawPort, re_pspoFirstBackground, re_prBackgroundProjection, re_colSelection, FALSE);
    } else {
      // this is just for far sentinel
      RenderSceneBackground( re_pdpDrawPort, re_spoFarSentinel.spo_spoScenePolygon.spo_cColor);
    }
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERSCENE);
  }
  
  if (re_bBackgroundEnabled) {
    // render models that were kept for delayed rendering.
    ChangeStatsMode(CStatForm::STI_MODELSETUP);
    RenderModels(TRUE);   // render background models
    ChangeStatsMode(CStatForm::STI_WORLDRENDERING);
  }
  
  // if polygons should be drawn
  if (!re_bRenderingShadows &&
    re_bBackgroundEnabled
    &&_wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
    // render translucent portals
    _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERSCENE);
    //CPerspectiveProjection3D *pprPerspective = (CPerspectiveProjection3D*)(CProjection3D*)(re_prBackgroundProjection);
    RenderScene( re_pdpDrawPort, SortTranslucentPolygons(re_pspoFirstBackgroundTranslucent),
                 re_prBackgroundProjection, re_colSelection, TRUE);
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERSCENE);
  }
  
  if( re_bBackgroundEnabled) {
    ChangeStatsMode(CStatForm::STI_PARTICLERENDERING);
    RenderParticles(TRUE); // render background particles
    ChangeStatsMode(CStatForm::STI_WORLDRENDERING);
  }
  
  //------------------------------------------------- second render non-background
  // if polygons should be drawn
  if( !re_bRenderingShadows
   && _wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
    // render the spans to screen
    re_prProjection->Prepare();
    _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERSCENE);
    //CPerspectiveProjection3D *pprPerspective = (CPerspectiveProjection3D*)(CProjection3D*)re_prProjection;
    RenderScene( re_pdpDrawPort, re_pspoFirst, re_prProjection, re_colSelection, FALSE);
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERSCENE);
  }

  // Render active terrains
  if( !re_bRenderingShadows
    && _wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
    RenderTerrains();
  }

  // if wireframe should be drawn
  if( !re_bRenderingShadows &&
    ( _wrpWorldRenderPrefs.wrp_ftEdges     != CWorldRenderPrefs::FT_NONE
   || _wrpWorldRenderPrefs.wrp_ftVertices  != CWorldRenderPrefs::FT_NONE
   || _wrpWorldRenderPrefs.wrp_stSelection == CWorldRenderPrefs::ST_VERTICES
   || _wrpWorldRenderPrefs.IsFieldBrushesOn())) {
    // render in wireframe all brushes that were added (in orthographic projection!)
    re_pdpDrawPort->SetOrtho();
    RenderWireFrameBrushes();
    RenderWireFrameTerrains();
  }

  // render models that were kept for delayed rendering
  ChangeStatsMode(CStatForm::STI_MODELSETUP);
  RenderModels(FALSE); // render non-background models
  ChangeStatsMode(CStatForm::STI_PARTICLERENDERING);
  RenderParticles(FALSE); // render non-background particles
  ChangeStatsMode(CStatForm::STI_WORLDRENDERING);
  
  // if polygons should be drawn
  if (!re_bRenderingShadows
    &&_wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
    // render translucent portals
    _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERSCENE);
    CPerspectiveProjection3D *pprPerspective = (CPerspectiveProjection3D*)(CProjection3D*)re_prProjection;
    pprPerspective->Prepare();
    RenderScene( re_pdpDrawPort, SortTranslucentPolygons(re_pspoFirstTranslucent),
                 re_prProjection, re_colSelection, TRUE);
    _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERSCENE);
  }

  // render lens flares
  if( !re_bRenderingShadows) {
    ChangeStatsMode(CStatForm::STI_FLARESRENDERING);
    RenderLensFlares(); // (this also sets orthographic projection!)
    ChangeStatsMode(CStatForm::STI_WORLDRENDERING);
  }

  // if entity targets should be drawn
  if( !re_bRenderingShadows && _wrpWorldRenderPrefs.wrp_bShowTargetsOn) {
    // render entity targets
    RenderEntityTargets();
  }

  // if entity targets should be drawn
  if( !re_bRenderingShadows && _wrpWorldRenderPrefs.wrp_bShowEntityNames) {
    RenderEntityNames();
  }

  // clean all buffers after rendering
  re_aspoScreenPolygons.PopAll();
  re_admDelayedModels.PopAll();
  re_cenDrawn.PopAll();
  re_avvxViewVertices.PopAll();
}


// draw mirror polygons to z-buffer to enable drawing of mirror
void CRenderer::FillMirrorDepth(CMirror &mi)
{
  // create a list of scene polygons for mirror
  ScenePolygon *pspoFirst = NULL;
  // for each polygon
  FOREACHINDYNAMICCONTAINER(mi.mi_cspoPolygons, CScreenPolygon, itspo) {
    CScreenPolygon &spo = *itspo;
    //CBrushPolygon &bpo = *spo.spo_pbpoBrushPolygon;
    // create a new screen polygon
    CScreenPolygon &spoNew = re_aspoScreenPolygons.Push();
    ScenePolygon &sppoNew = spoNew.spo_spoScenePolygon;
    // add it to mirror list
    sppoNew.spo_pspoSucc = pspoFirst;
    pspoFirst = &sppoNew;

    // use same triangles
    sppoNew.spo_iVtx0 = spo.spo_spoScenePolygon.spo_iVtx0;
    sppoNew.spo_ctVtx = spo.spo_spoScenePolygon.spo_ctVtx;
    sppoNew.spo_piElements = spo.spo_spoScenePolygon.spo_piElements;
    sppoNew.spo_ctElements = spo.spo_spoScenePolygon.spo_ctElements;
  }

  // render all those polygons just to clear z-buffer
  RenderSceneZOnly( re_pdpDrawPort, pspoFirst, re_prProjection);
}



// do the rendering
void CRenderer::Render(void)
{
  // if the world doesn't have all portal-sector links updated
  if( !re_pwoWorld->wo_bPortalLinksUpToDate) {
    // update the links
    CSetFPUPrecision FPUPrecision(FPT_53BIT);
    re_pwoWorld->wo_baBrushes.LinkPortalsAndSectors();
    re_pwoWorld->wo_bPortalLinksUpToDate = TRUE;
  }

  StartStatsMode(CStatForm::STI_WORLDTRANSFORM);
  _pfRenderProfile.IncrementAveragingCounter();
  _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERING);

  // set FPU to single precision while rendering
  CSetFPUPrecision FPUPrecision(FPT_24BIT);

  // initialize all rendering structures
  Initialize();
  // init select-on-render functionality if not rendering shadows
  extern void InitSelectOnRender( PIX pixSizeI, PIX pixSizeJ);
  if( re_pdpDrawPort!=NULL) InitSelectOnRender( re_pdpDrawPort->GetWidth(), re_pdpDrawPort->GetHeight());
  // add initial sectors to active lists
  AddInitialSectors();
  // scan through portals for other sectors
  ScanForOtherSectors();

  // force finishing of all OpenGL pending operations, if required
  ChangeStatsMode(CStatForm::STI_SWAPBUFFERS);
  extern INDEX ogl_iFinish;  ogl_iFinish = Clamp( ogl_iFinish, (INDEX)0, (INDEX)3);
  extern INDEX d3d_iFinish;  d3d_iFinish = Clamp( d3d_iFinish, (INDEX)0, (INDEX)3);
  if( (ogl_iFinish==1 && _pGfx->gl_eCurrentAPI==GAT_OGL) 
#ifdef SE1_D3D
   || (d3d_iFinish==1 && _pGfx->gl_eCurrentAPI==GAT_D3D)
#endif // SE1_D3D
   ) 
   gfxFinish();

  // check any eventual delayed depth points outside the mirror (if API and time allows)
  if( !re_bRenderingShadows && re_iIndex==0) {
    // OpenGL allows us to check z-buffer from previous frame - cool deal!
    // Direct3D is, of course, totally different story. :(
    if( _pGfx->gl_eCurrentAPI==GAT_OGL || d3d_bAlternateDepthReads) {
      ChangeStatsMode(CStatForm::STI_FLARESRENDERING);
      extern void CheckDelayedDepthPoints( const CDrawPort *pdp, INDEX iMirrorLevel=0);
      CheckDelayedDepthPoints(re_pdpDrawPort);
    }
    // in 1st pass - mirrors are not drawn
    _bMirrorDrawn = FALSE;
  }

  // if may render one more mirror recursion
  ChangeStatsMode(CStatForm::STI_WORLDTRANSFORM);
  if(  !re_bRenderingShadows
    &&  re_prProjection.IsPerspective()
    &&  re_iIndex<MAX_RENDERERS-1
    &&  re_amiMirrors.Count()>0
    && !re_pdpDrawPort->IsOverlappedRendering())
  {
    // cleanup after scanning
    CleanupScanning();

    // take next renderer
    CRenderer &re = _areRenderers[re_iIndex+1];
    // for each mirror
    for( INDEX i=0; i<re_amiMirrors.Count(); i++)
    {
      // skip invalid mirrors
      CMirror &mi = re_amiMirrors[i];
      if( mi.mi_iMirrorType<0) continue;

      // calculate all needed data for the mirror
      mi.FinishAdding();
      // skip mirror that has no significant area
      if( mi.mi_fpixMaxPolygonArea<5) continue;

      // expand mirror in each direction, but keep it inside drawport
      PIX pixDPSizeI = re_pdpDrawPort->GetWidth();
      PIX pixDPSizeJ = re_pdpDrawPort->GetHeight();
      mi.mi_boxOnScreen.Expand(1);
      mi.mi_boxOnScreen &= PIXaabbox2D( PIX2D(0,0), PIX2D(pixDPSizeI,pixDPSizeJ));

      // get drawport and mirror coordinates
      PIX pixMirrorMinI = mi.mi_boxOnScreen.Min()(1);
      PIX pixMirrorMinJ = mi.mi_boxOnScreen.Min()(2);
      PIX pixMirrorMaxI = mi.mi_boxOnScreen.Max()(1);
      PIX pixMirrorMaxJ = mi.mi_boxOnScreen.Max()(2);

      // calculate mirror size
      PIX pixMirrorSizeI = pixMirrorMaxI-pixMirrorMinI;
      PIX pixMirrorSizeJ = pixMirrorMaxJ-pixMirrorMinJ;
      // clone drawport (must specify doubles here, to keep the precision)
      re_pdpDrawPort->Unlock();
      CDrawPort dpMirror( re_pdpDrawPort, pixMirrorMinI /(DOUBLE)pixDPSizeI, pixMirrorMinJ /(DOUBLE)pixDPSizeJ,
                                          pixMirrorSizeI/(DOUBLE)pixDPSizeI, pixMirrorSizeJ/(DOUBLE)pixDPSizeJ);
      // skip if cannot be locked
      if( !dpMirror.Lock()) {
        // lock back the original drawport
        re_pdpDrawPort->Lock();
        continue;
      }

      // recalculate mirror size to compensate for possible lost precision
      pixMirrorMinI  = dpMirror.dp_MinI - re_pdpDrawPort->dp_MinI;
      pixMirrorMinJ  = dpMirror.dp_MinJ - re_pdpDrawPort->dp_MinJ;
      pixMirrorMaxI  = dpMirror.dp_MaxI - re_pdpDrawPort->dp_MinI +1;
      pixMirrorMaxJ  = dpMirror.dp_MaxJ - re_pdpDrawPort->dp_MinJ +1;
      pixMirrorSizeI = pixMirrorMaxI-pixMirrorMinI;
      pixMirrorSizeJ = pixMirrorMaxJ-pixMirrorMinJ;
      ASSERT( pixMirrorSizeI==dpMirror.dp_Width && pixMirrorSizeJ==dpMirror.dp_Height);

      // set it up for rendering
      re.re_pwoWorld     = re_pwoWorld;
      re.re_prProjection = re_prProjection;
      re.re_pdpDrawPort  = &dpMirror;
      // initialize clipping rectangle around the mirror size
      re.InitClippingRectangle( 0, 0, pixMirrorSizeI, pixMirrorSizeJ);
      // setup projection to use the mirror drawport and keep same perspective as before
      re.re_prProjection->ScreenBBoxL() = FLOATaabbox2D( FLOAT2D(0,0), FLOAT2D(pixDPSizeI, pixDPSizeJ));
      ((CPerspectiveProjection3D&)(*re.re_prProjection)).ppr_boxSubScreen =
        FLOATaabbox2D( FLOAT2D(pixMirrorMinI, pixMirrorMinJ), FLOAT2D(pixMirrorMaxI, pixMirrorMaxJ));

      // warp?
      if( mi.mi_mp.mp_ulFlags&MPF_WARP) {
        // warp clip plane is parallel to view plane and contains the closest point
        re.re_penViewer = mi.mi_mp.mp_penWarpViewer;
        re.re_pcspoViewPolygons = NULL;
        re.re_prProjection->WarpPlaneL() = FLOATplane3D(FLOAT3D(0,0,-1), mi.mi_vClosest);
        // create new viewer placement
        CPlacement3D pl = re.re_prProjection->ViewerPlacementR();
        FLOATmatrix3D m;
        MakeRotationMatrixFast(m, pl.pl_OrientationAngle);
        pl.AbsoluteToRelativeSmooth(mi.mi_mp.mp_plWarpIn);
        pl.RelativeToAbsoluteSmooth(mi.mi_mp.mp_plWarpOut);
        re.re_prProjection->ViewerPlacementL() = pl;
        if (re.re_prProjection.IsPerspective() && mi.mi_mp.mp_fWarpFOV>=1 && mi.mi_mp.mp_fWarpFOV<=170) {
          ((CPerspectiveProjection3D&)*re.re_prProjection).FOVL() = mi.mi_mp.mp_fWarpFOV;
        }
      // mirror!
      } else {
        re.re_penViewer = NULL;
        re.re_pcspoViewPolygons = &mi.mi_cspoPolygons;
        re.re_prProjection->MirrorPlaneL() = mi.mi_plPlane;
        re.re_prProjection->MirrorPlaneL().Offset(0.05f); // move projection towards mirror a bit, to avoid cracks
      }
      re.re_bRenderingShadows = FALSE;
      re.re_ubLightIllumination = 0;

      // just flat-fill if mirrors are disabled
      extern INDEX wld_bRenderMirrors;
      if( !wld_bRenderMirrors) dpMirror.Fill(C_GRAY|CT_OPAQUE);
      else {
        // render the view inside mirror
        StopStatsMode();
        _pfRenderProfile.StopTimer(CRenderProfile::PTI_RENDERING);
        re.Render();
        _pfRenderProfile.StartTimer(CRenderProfile::PTI_RENDERING);
        StartStatsMode(CStatForm::STI_WORLDTRANSFORM);
      } 
      // unlock mirror's and lock back the original drawport
      dpMirror.Unlock();
      re_pdpDrawPort->Lock();
      // clear entire buffer to back value
      re_pdpDrawPort->FillZBuffer(ZBUF_BACK);
      // fill depth buffer of the mirror, so that scene cannot be drawn through it
      FillMirrorDepth(mi);
     _bMirrorDrawn = TRUE;
    }

    // flush all mirrors
    re_amiMirrors.PopAll();

    // fill z-buffer only if no mirrors have been drawn, not rendering second layer in world editor and not in wireframe mode
    if(  !_bMirrorDrawn
      && !re_pdpDrawPort->IsOverlappedRendering()
      && _wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE) {
      re_pdpDrawPort->FillZBuffer(ZBUF_BACK);
    }
    // draw the prepared things to screen
    DrawToScreen();
  }

  // no mirrors
  else
  {
    // if rendering a mirror
    // or not rendering second layer in world editor
    // and not in wireframe mode
    if(  re_iIndex>0 
     || (!re_bRenderingShadows
         && !re_pdpDrawPort->IsOverlappedRendering()
         && _wrpWorldRenderPrefs.wrp_ftPolygons != CWorldRenderPrefs::FT_NONE)) {
      re_pdpDrawPort->FillZBuffer(ZBUF_BACK);
    }
    // draw the prepared things to screen and finish
    DrawToScreen();
    CleanupScanning();
  }

  // disable fog/haze
  StopFog();
  StopHaze();
  // reset vertex arrays if this is the last renderer  
  if( re_iIndex==0) _avtxScene.PopAll();

  // for D3D (or mirror) we have to check depth points now, because we need back (not depth!) buffer for it,
  // and D3D can't guarantee that it won't be discarded upon swapbuffers (especially if multisampling is on!) :(
#ifdef SE1_D3D
  if( !re_bRenderingShadows && ((_pGfx->gl_eCurrentAPI==GAT_D3D && !d3d_bAlternateDepthReads) || re_iIndex>0)) {
    extern void CheckDelayedDepthPoints( const CDrawPort *pdp, INDEX iMirrorLevel=0);
    CheckDelayedDepthPoints( re_pdpDrawPort, re_iIndex);
  }
#endif // SE1_D3D
  
  // end select-on-render functionality
  extern void EndSelectOnRender(void);
  EndSelectOnRender();

  // assure that FPU precision was low all the rendering time
  ASSERT( GetFPUPrecision()==FPT_24BIT);
  StopStatsMode();
}


/*
 * Constructor.
 */
CRenderer::CRenderer(void)
{
  // setup self index
  INDEX i = this-_areRenderers;
  ASSERT(i>=0 && i<MAX_RENDERERS);
  re_iIndex = i;
}
/*
 * Destructor.
 */
CRenderer::~CRenderer(void)
{
}

// initialize clipping rectangle
void CRenderer::InitClippingRectangle(PIX pixMinI, PIX pixMinJ, PIX pixSizeI, PIX pixSizeJ)
{
  re_pspoFirst = NULL;
  re_pspoFirstTranslucent = NULL;
  re_pspoFirstBackground = NULL;
  re_pspoFirstBackgroundTranslucent = NULL;

  re_fMinJ = (FLOAT) pixMinJ;
  re_fMaxJ = (FLOAT) pixSizeJ+pixMinJ;
  re_pixSizeI = pixSizeI;
  re_fbbClipBox =
    FLOATaabbox2D( FLOAT2D((FLOAT) pixMinI+CLIPMARGADD,
                           (FLOAT) pixMinJ+CLIPMARGADD),
                   FLOAT2D((FLOAT) pixMinI+pixSizeI-CLIPMARGSUB,
                           (FLOAT) pixMinJ+pixSizeJ-CLIPMARGSUB));
  re_pixTopScanLineJ = PIXCoord(pixMinJ+CLIPMARGADD);
  re_ctScanLines =
    PIXCoord(pixSizeJ-CLIPMARGSUB) - PIXCoord(CLIPMARGADD)/* +1*/;
  re_pixBottomScanLineJ = re_pixTopScanLineJ+re_ctScanLines;
}

// render a 3D view to a drawport
void RenderView(CWorld &woWorld, CEntity &enViewer,
  CAnyProjection3D &prProjection, CDrawPort &dpDrawport)
{
  // let the worldbase execute its render function
  if (woWorld.wo_pecWorldBaseClass!=NULL
    &&woWorld.wo_pecWorldBaseClass->ec_pdecDLLClass!=NULL
    &&woWorld.wo_pecWorldBaseClass->ec_pdecDLLClass->dec_OnWorldRender!=NULL) {
    woWorld.wo_pecWorldBaseClass->ec_pdecDLLClass->dec_OnWorldRender(&woWorld);
  }

  if(_wrpWorldRenderPrefs.GetShadowsType() == CWorldRenderPrefs::SHT_FULL)
  {
    // calculate all non directional shadows that are not up to date
    woWorld.CalculateNonDirectionalShadows();
  }

  // take first renderer object
  CRenderer &re = _areRenderers[0];
  // set it up for rendering
  re.re_penViewer = &enViewer;
  re.re_pcspoViewPolygons = NULL;
  re.re_pwoWorld = &woWorld;
  re.re_prProjection = prProjection;
  re.re_pdpDrawPort = &dpDrawport;
  // initialize clipping rectangle around the drawport
  re.InitClippingRectangle(0, 0, dpDrawport.GetWidth(), dpDrawport.GetHeight());
  prProjection->ScreenBBoxL() = FLOATaabbox2D(
    FLOAT2D(0.0f, 0.0f),
    FLOAT2D((float)dpDrawport.GetWidth(), (float)dpDrawport.GetHeight())
  );
  re.re_bRenderingShadows = FALSE;
  re.re_ubLightIllumination = 0;

  // render the view (with eventuall t-buffer effect)
  extern void SetTBufferEffect( BOOL bEnable);
  SetTBufferEffect(TRUE);
  re.Render();
  SetTBufferEffect(FALSE);
}


// Render a world with some viewer, projection and drawport. (viewer may be NULL)
// internal version used for rendering shadows
ULONG RenderShadows(CWorld &woWorld, CEntity &enViewer,
  CAnyProjection3D &prProjection, const FLOATaabbox3D &boxViewer,
  UBYTE *pubShadowMask, SLONG slShadowWidth, SLONG slShadowHeight,
  UBYTE ubIllumination)
{
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_RENDERSHADOWS);

  // take a renderer object
  CRenderer &re = _areRenderers[0];
  // set it up for rendering
  re.re_penViewer = &enViewer;
  re.re_pcspoViewPolygons = NULL;
  re.re_pwoWorld = &woWorld;
  re.re_prProjection = prProjection;
  re.re_pdpDrawPort = NULL;
  re.re_boxViewer = boxViewer;
  // initialize clipping rectangle around the drawport
  const FLOATaabbox2D &box = prProjection->ScreenBBoxR();
  //re.InitClippingRectangle(box.Min()(1), box.Min()(2), box.Size()(1), box.Size()(2));
  re.InitClippingRectangle(0, 0, (PIX) box.Size()(1), (PIX) box.Size()(2));

  re.re_bRenderingShadows = TRUE;
  re.re_bDirectionalShadows = prProjection.IsParallel();
  re.re_bSomeLightExists = FALSE;
  re.re_bSomeDarkExists = FALSE;
  _bSomeDarkExists = FALSE;
  re.re_pubShadow = pubShadowMask;
  re.re_slShadowWidth  = slShadowWidth;
  re.re_slShadowHeight = slShadowHeight;
  re.re_ubLightIllumination = ubIllumination;
  // render the view
  re.Render();

  ULONG ulFlags = 0;
  if (!re.re_bSomeLightExists) {
    ulFlags|=BSLF_ALLDARK;
  }
  if (!(re.re_bSomeDarkExists|_bSomeDarkExists)) {
    ulFlags|=BSLF_ALLLIGHT;
  }

  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_RENDERSHADOWS);

  return ulFlags;
}
