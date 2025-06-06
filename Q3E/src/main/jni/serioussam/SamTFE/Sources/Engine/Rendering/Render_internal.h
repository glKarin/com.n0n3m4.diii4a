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

#ifndef SE_INCL_RENDER_INTERNAL_H
#define SE_INCL_RENDER_INTERNAL_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Base/Lists.h>
#include <Engine/Math/FixInt.h>
#include <Engine/Graphics/RenderScene.h>
#include <Engine/Templates/DynamicStackArray.h>
#include <Engine/Templates/DynamicContainer.h>
#include <Engine/World/World.h>
#include <Engine/World/WorldSettings.h>
#include <Engine/Templates/StaticStackArray.h>
#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushTransformed.h>

#undef ALIGNED_NEW_AND_DELETE
#ifdef NDEBUG
#define ALIGNED_NEW_AND_DELETE(align) \
  void *operator new[] (size_t size) { return AllocMemoryAligned(size, align); }; \
  void operator delete[] (void* ptr) { FreeMemoryAligned(ptr); };
#else 
#define ALIGNED_NEW_AND_DELETE(align)
#endif

// type of edge orientation on screen
enum LineDirectionType {
  LDT_ASCENDING,          // edge is going bottom to top
  LDT_DESCENDING,         // edge is going top to bottom
  LDT_HORIZONTAL,         // edge is horizontal
};

// polygon direction flags
#define PDF_FLIPEDGESPRE    (1UL<<0)    // flip edges before clipping
#define PDF_FLIPEDGESPOST   (1UL<<1)    // flip edges after clipping
#define PDF_POLYGONVISIBLE  (1UL<<2)    // polygon is visible

/*
 * A polygon projected to screen, clipped and used in rendering.
 */
class CScreenPolygon {
public:
  // planar gradients of depth for this polygon used in sorting
  CPlanarGradients spo_pgOoK;
  ScenePolygon spo_spoScenePolygon;       // holder for scene data of this polygon

  CBrushPolygon *spo_pbpoBrushPolygon;
  BOOL spo_bActive;     // set if active in rendering

  CListNode spo_lnInStack;              // node in surface stack
  INDEX spo_iInStack;                   // counter of additions to surface stack
  class CScreenEdge *spo_psedSpanStart;   // edge where polygon's span started
  UBYTE spo_ubIllumination;             // illumination of the polygon (when rendering shadows)
  UBYTE spo_ubSpanAdded;    // set if polygon has created any span yet
  UBYTE spo_ubDirectionFlags; // set if should invert polygon edges
  UBYTE spo_ubDummy;
  INDEX spo_iEdgeVx0;   // first vertex of edge vertices
  INDEX spo_ctEdgeVx;   // number of vertices of edge vertices
  INDEX spo_ised0;   // first screen edge
  INDEX spo_ctsed;   // number of screen edges
  PIX spo_pixMinI;  // bounding box on screen
  PIX spo_pixMinJ;
  PIX spo_pixMaxI;
  PIX spo_pixMaxJ;
  PIX spo_pixTotalArea; // sum of all visible spans

  /* Default constructor. */
  CScreenPolygon(void) {
  #ifndef NDEBUG
    spo_iInStack = 0;
  #endif
  };
  /* Destructor. */
  inline ~CScreenPolygon(void);
  
  /* Test if this polygon is a portal. */
  inline BOOL IsPortal(void) { 
    return spo_pbpoBrushPolygon!=NULL && spo_pbpoBrushPolygon->bpo_ulFlags&BPOF_RENDERASPORTAL;
  };
  /* Clear the object. */
  inline void Clear(void) {};
};

// transformed vertex used in rendering
class CViewVertex {
public:
  FLOAT3D vvx_vView;      // coordinates in view space
  FLOAT vvx_fD; // distance from clip plane
  ULONG vvx_ulOutcode;    // outcode of the vertex -> bits set if polygon is behind the plane
  FLOAT vvx_fI; // screen-space coordinates
  FLOAT vvx_fJ;
};

/*
 * An edge projected to screen, clipped and used in rendering.
 */
class CScreenEdge {
public:
// first block is data needed for ASER
  FIX16_16 sed_xI;            // top I coordinate
  FIX16_16 sed_xIStep;        // I coordinate step per scan line

  CScreenPolygon *sed_pspo;   // polygon

  BOOL sed_bAdded;    // set if added to add/active list
  CScreenEdge *sed_psedNextRemove;     // node in remove list

// second block is data needed for clipping etc.
  enum LineDirectionType sed_ldtDirection;   // edge orientation on screen

  PIX sed_pixTopJ;            // top and bottom J coordinates of the edge
  PIX sed_pixBottomJ;

  ALIGNED_NEW_AND_DELETE(32);
  /* Clear the object. */
  inline void Clear(void) {};
};

#define ACE_REMOVED 0x7FFFFFFF  // I coordinate for removed edges
/*
 * Structure used for sorting edges in active list
 */
class CActiveEdge {
public:
  FIX16_16 ace_xI;            // top I coordinate
  FIX16_16 ace_xIStep;        // I coordinate step per scan line
  CScreenEdge *ace_psedEdge;  // the edge
  ULONG ace_ulDummy;  // alignment to 16 bytes

  ALIGNED_NEW_AND_DELETE(32);

  inline CActiveEdge(void) {};
  inline CActiveEdge(CScreenEdge *psed)
    : ace_xI(psed->sed_xI)
    , ace_xIStep(psed->sed_xIStep)
    , ace_psedEdge(psed)
  {};
  inline void Clear(void) {};
};

/*
 * Structure used for sorting edges in add list
 */
class CAddEdge { // size is 16 bytes
public:
  FIX16_16 ade_xI;            // top I coordinate
  CListNode ade_lnInAdd;        // node in add list
  CScreenEdge *ade_psedEdge;  // the edge

  ALIGNED_NEW_AND_DELETE(32);

  inline CAddEdge(void) {};
  inline CAddEdge(CScreenEdge *psed)
    : ade_xI(psed->sed_xI)
    , ade_psedEdge(psed)
  {};
  inline void Clear(void) {
    ade_lnInAdd.ln_Succ = NULL;
    ade_lnInAdd.ln_Pred = NULL;
  };
};

/*
 * A span of a polygon on current scan line.
 */
class CSpan {
public:
  CScreenEdge *sp_psedEdge0;      // edge left of this span
  CScreenEdge *sp_psedEdge1;      // edge right of this span
  CScreenPolygon *sp_pspoPolygon; // polygon of this span

  /* Clear the object. */
  inline void Clear(void) {};
};

/* We must declare dummy clear functions for external classes that
 * get stored in dynamic stack arrays.
 */
inline void Clear(Vector<float,2> &dummy) {};

/*
 * Model that is to be rendered in this frame.
 */
#define DMF_HASALPHA (1UL<<0)  // if the model uses alpha blending (sorted last)
#define DMF_VISIBLE  (1UL<<1)  // really visible (particles are rendered even for invisibles)
#define DMF_FOG      (1UL<<2)  // in fog
#define DMF_HAZE     (1UL<<3)  // in haze
#define DMF_INSIDE   (1UL<<4)  // completely inside frustum (not clipped)
#define DMF_INMIRROR (1UL<<5)  // completely inside mirror (not clipped)

class CDelayedModel {
public:
  FLOAT dm_fDistance;         // Z distance from viewer (for sorting)
  FLOAT dm_fMipFactor;        // mip factor of the model
  ULONG dm_ulFlags;           // various flags
  CEntity *dm_penModel;       // the model entity
  CModelObject *dm_pmoModel;  // model of the entity
  __forceinline void Clear(void) {};
};

/*
 * Lens flare that could rendered in this frame.
 */
#define LFF_ACTIVE  (1UL<<0) // set if was active in this frame
#define LFF_VISIBLE (1UL<<1) // set if the light source is visible in this frame
#define LFF_FOG     (1UL<<2) // in fog
#define LFF_HAZE    (1UL<<3) // in haze
class CLensFlareInfo {
public:
  INDEX lfi_iID;                      // unique ID of a lens flare info
  ULONG lfi_ulDrawPortID;             // unique ID of the lens flare's drawport
  CLightSource *lfi_plsLightSource;   // the light source
  FLOAT3D lfi_vProjected;             // coordinates in view space (for fog and haze)
  FLOAT lfi_fI, lfi_fJ;    // position of light source on screen in this frame
  FLOAT lfi_fDistance;     // distance of light source from viewer in this frame
  FLOAT lfi_fOoK;          // depth of light source in this frame
  TIME  lfi_tmLastFrame;   // last time it was animated
  INDEX lfi_iMirrorLevel;  // mirror recursion level in which the flare is
  FLOAT lfi_fFadeFactor;   // current fade ratio (0..1)
  ULONG lfi_ulFlags;       // various flags

  __forceinline void Clear(void) { };
};

class CTranslucentPolygon {
public:
  FLOAT tp_fViewerDistance;
  ScenePolygon *tp_pspoPolygon;
  __forceinline void Clear(void) {};
};

class CMirror {
public:
  INDEX mi_iMirrorType;       // mirror index
  FLOATplane3D mi_plPlane;    // plane in absolute space
  FLOAT3D mi_vClosest;        // point closest to viewer, in view space
  PIXaabbox2D mi_boxOnScreen; // bounding box of the mirror on screen
  FLOAT mi_fpixArea;     // total area of mirror
  FLOAT mi_fpixMaxPolygonArea;     // max area of single polygon of mirror

  // parameters
  CMirrorParameters mi_mp;

  // for mirrors
  CDynamicContainer<CScreenPolygon> mi_cspoPolygons; // polygons of the mirror

  void Clear(void);

  // add given polygon to this mirror
  void AddPolygon(CRenderer &re, CScreenPolygon &spo);
  // calculate all needed data from screen polygons
  void FinishAdding(void);
};

/*
 * Object that performs rendering of a scene as seen by an entity.
 */
class CRenderer {
public:
// implementation:
  INDEX re_iIndex;                // index of this renderer in static array
  CWorld             *re_pwoWorld;              // world to render
  CDrawPort          *re_pdpDrawPort;           // drawport that is drawn on
  // where the spans are emitted
  struct ScenePolygon *re_pspoFirst;  
  struct ScenePolygon *re_pspoFirstTranslucent;
  struct ScenePolygon *re_pspoFirstBackground;
  struct ScenePolygon *re_pspoFirstBackgroundTranslucent;

  FLOATaabbox3D re_boxViewer;         // bounding box of viewer
  CEntity           *re_penViewer;    // entity that is viewed from
  CDynamicContainer<CScreenPolygon> *re_pcspoViewPolygons;  // polygons that is viewed from (for mirrors)
  CAnyProjection3D   re_prProjection; // projection to viewer space
   DOUBLE3D re_vdViewSphere;
   DOUBLE   re_dViewSphereR;

  // used for fixing problems with extra trapezoids generated on t-junctions
  FLOAT re_fEdgeOffsetI;
  FLOAT re_fEdgeAdjustK;

  BOOL re_bBackgroundEnabled;         // set if should render background objects in background
  CEntity *re_penBackgroundViewer;    // background viewer entity
  CAnyProjection3D re_prBackgroundProjection;  // projection for background

  PIX re_pixSizeI;
  FLOAT re_fMinJ;   // top row
  FLOAT re_fMaxJ;   // bottom row+1
  FLOATaabbox2D re_fbbClipBox;    // clip rectangle on screen

  BOOL re_bRenderingShadows;    // set if rendering shadows instead of normal view
  BOOL re_bDirectionalShadows;  // set if rendering directional shadows
  UBYTE *re_pubShadow;          // byte-packed shadow mask
  SLONG re_slShadowWidth;
  SLONG re_slShadowHeight; 
  BOOL re_bSomeLightExists;     // set if rendering light, and at least one pixel is lighted
  BOOL re_bSomeDarkExists;      // set if rendering light, and at least one pixel is dark
  UBYTE re_ubLightIllumination;  // the illumination type used by light rendering shadows
  COLOR re_colSelection;        // selection color
  BOOL re_bCurrentSectorHasFog;   // set if currently added sector has fog
  BOOL re_bCurrentSectorHasHaze;  // set if currently added sector has haze
  BOOL re_bViewerInHaze;          // set if viewer is viewing from a hazed sector
  ULONG re_ulVisExclude;    // for visibility tweaking
  ULONG re_ulVisInclude;

  INDEX re_iViewVx0; // first view vertex for current sector
  FLOATplane3D re_plClip;         // current clip plane
  CBrush3D *re_pbrCurrent;        // current brush
  CBrushSector *re_pbscCurrent;   // current sector

  // screen edges
  static CDynamicStackArray<CAddEdge> re_aadeAddEdges;
  static CDynamicStackArray<CScreenEdge> re_asedScreenEdges;
  // spans for current scan line
  static CDynamicStackArray<CSpan> re_aspSpans;
 
  // structures needed to render the scanned scene (separated for each renderer)
  CDynamicStackArray<CScreenPolygon> re_aspoScreenPolygons;
  CDynamicStackArray<CDelayedModel> re_admDelayedModels; // model entities to be renderer later
  CDynamicContainer<CEntity> re_cenDrawn; // all drawn entities (for drawing target lines etc.)
  CStaticStackArray<CLensFlareInfo> re_alfiLensFlares;  // active lens flares
  CDynamicStackArray<CMirror> re_amiMirrors;    // mirrors/portals for recursion
  CStaticStackArray<CViewVertex> re_avvxViewVertices;   // transformed vertices
  CStaticStackArray<INDEX> re_aiEdgeVxMain;

  // vertices clipped to current clip plane
  static CStaticStackArray<INDEX> re_aiClipBuffer;
  // buffers for edges of polygons
  static CStaticStackArray<INDEX> re_aiEdgeVxClipSrc;
  static CStaticStackArray<INDEX> re_aiEdgeVxClipDst;

  // add and remove lists for each scan line
  static CStaticArray<CListHead> re_alhAddLists;
  static CStaticArray<INDEX> re_actAddCounts;   // count of edges in given add list
  static CStaticArray<CScreenEdge *> re_apsedRemoveFirst;

  // container for sorting translucent polygons
  static CDynamicStackArray<CTranslucentPolygon> re_atcTranslucentPolygons;

public:
  INDEX re_ctScanLines;       // number of scanlines
  PIX re_pixTopScanLineJ;     // J coordinate of the top scanline
  PIX re_pixBottomScanLineJ;  // J coordinate of the bottom scanline

  CListHead re_lhActiveBrushes;     // list of active brushes
  CListHead re_lhActiveSectors;     // list of active sectors
  CListHead re_lhActiveTerrains;    // list of active terrains

  static CStaticStackArray<CActiveEdge> re_aaceActiveEdges; // active edges for current scan line
  static CStaticStackArray<CActiveEdge> re_aaceActiveEdgesTmp;

  INDEX re_iCurrentScan;            // index of current scan line in tables
  PIX re_pixCurrentScanJ;           // J coordinate of current scan line
  FLOAT re_fCurrentScanJ;
  FIX16_16 re_xCurrentScanI;        // I coordinate on current scan line
  BOOL re_bCoherentScanLine;        // set if this line is coherent with previous one

  CScreenEdge re_sedLeftSentinel;   // sentinel edges for list of active edges
  CScreenEdge re_sedRightSentinel;
  CScreenPolygon re_spoFarSentinel; // sentinel polygon for surface stack
  CListHead re_lhSurfaceStack;      // list of polygons in stack - closest first

  /* Determine color to use for coloring vertices. */
  static inline COLOR ColorForVertices(COLOR colPolygon, COLOR colSector);
  /* Determine color to use for coloring edges. */
  static inline COLOR ColorForEdges(COLOR colPolygon, COLOR colSector);
  /* Determine color to use for coloring polygons. */
  static inline COLOR ColorForPolygons(COLOR colPolygon, COLOR colSector);
  /* Draw vertices and/or edges of a brush sector. */
  void DrawBrushPolygonVerticesAndEdges(CBrushPolygon &bpoPolygon);
  void DrawBrushSectorVerticesAndEdges(CBrushSector &bscSector);
  /* Draw edges of a field brush sector. */
  void DrawFieldBrushSectorEdges(CBrushSector &bscSector);

  /* Make a screen edge from two vertices. */
  inline void MakeScreenEdge(CScreenEdge &sed, FLOAT fI0, FLOAT fJ0, FLOAT fI1, FLOAT fJ1);
  // set scene rendering parameters for one polygon texture
  inline void SetOneTextureParameters(CBrushPolygon &bpo, ScenePolygon &spo, INDEX iTexture);
  /* Make a screen polygon for a brush polygon */
  CScreenPolygon *MakeScreenPolygon(CBrushPolygon &bpo);
  /* Add a polygon to scene rendering. */
  void AddPolygonToScene(CScreenPolygon *pspo);

  // check if a sector is inside view frustum
  __forceinline INDEX IsSectorVisible(CBrush3D &br, CBrushSector &bsc);
  // check if a polygon is to be visible
  __forceinline ULONG GetPolygonVisibility(const CBrushPolygon &bpo);
  // check if polygon is outside viewfrustum
  __forceinline BOOL IsPolygonCulled(const CBrushPolygon &bpo);
  // setup fog/haze for current sector
  void SetupFogAndHaze(void);
  // transform vertices in current sector before clipping
  void PreClipVertices(void);
  // transform planes in current sector before clipping
  void PreClipPlanes(void);
  // make initial edges for a polygon
  void MakeInitialPolygonEdges(CBrushPolygon &bpo, CScreenPolygon &spo, BOOL bInverted);
  // find which portals should be rendered as portals or as pretenders
  void FindPretenders(void);
  // make screen polygons for nondetail polygons in current sector
  void MakeNonDetailScreenPolygons(void);
  // make screen polygons for detail polygons in current sector
  void MakeDetailScreenPolygons(void);
  // make final edges for all polygons in current sector
  void MakeFinalPolygonEdges(void);
  // add screen edges for all polygons in current sector
  void AddScreenEdges(void);
  // clip all polygons to one clip plane
  void ClipToOnePlane(const FLOATplane3D &plView);
  // clip all polygons to all clip planes of a projection
  void ClipToAllPlanes(CAnyProjection3D &pr);
  // make outcodes for current clip plane for all active vertices
  __forceinline BOOL MakeOutcodes(void);
  // clip one polygon to current clip plane
  void ClipOnePolygon(CScreenPolygon &spo);
  // generate clip edges for one polygon
  void GenerateClipEdges(CScreenPolygon &spo);
  // project vertices in current sector after clipping
  void PostClipVertices(void);

  // switch a screen edge to using containers
  inline void SwitchEdgeToContainers(CScreenEdge &sed);
  /* Add an edge to add list of its top scanline and remove list at its bottom line. */
  inline void AddEdgeToAddAndRemoveLists(CScreenEdge &sed);
  /* Add all edges in add list to active list. */
  inline void AddAddListToActiveList(INDEX iScanLine);
  /* Remove all edges in remove list from active list and from other lists. */
  inline void RemRemoveListFromActiveList(CScreenEdge *psedFirst);
  /* Step all edges in active list by one scan line and resort them. */
  inline void StepAndResortActiveList(void);
  /* Copy I coordinates from active list to edge data. */
  inline void CopyActiveCoordinates(void);
  /* Remove an active portal from rendering */
  inline void RemovePortal(CScreenPolygon &spo);

  /* Add a sector of a brush to rendering queues. */
  void AddActiveSector(CBrushSector &bscSector);
  /* Add sector(s) adjoined to a portal to rendering and remove the portal. */
  void PassPortal(CScreenPolygon &spo);

  /* Generate a span for a polygon on current scan line. */
  inline void MakeSpan(CScreenPolygon &spo, CScreenEdge *psed0, CScreenEdge *psed1);

  /* Add spans in current line to scene. */
  void AddSpansToScene(void);

  /* Add a mirror/portal. */
  void AddMirror(CScreenPolygon &spo);
  
  /* Add a polygon to surface stack. */
  inline BOOL AddPolygonToSurfaceStack(CScreenPolygon &spo);
  /* Remove a polygon from surface stack. */
  inline BOOL RemPolygonFromSurfaceStack(CScreenPolygon &spo);
  /* Swap two polygons in surface stack. */
  inline BOOL SwapPolygonsInSurfaceStack(CScreenPolygon &spoOld, CScreenPolygon &spoNew);
  /* Remove all polygons from surface stack. */
  inline void FlushSurfaceStack(void);

  // update VisTweak flags with given zoning sector
  inline void UpdateVisTweaks(CBrushSector *pbsc);

  /* Initialize list of active edges and surface stack. */
  void InitScanEdges(void);
  /* Clean up list of active edges and surface stack. */
  void EndScanEdges(void);
  /* Scan list of active edges into spans. */
  inline CScreenPolygon *ScanOneLine(void);
  /* Rasterize edges into spans. */
  void ScanEdges(void);

  /* Render wireframe brushes. */
  void RenderWireFrameBrushes(void);
  /* Find lights for one model. */
  BOOL FindModelLights( CEntity &en, const CPlacement3D &plModel, COLOR &colLight, COLOR &colAmbient,
                        FLOAT &fTotalShadowIntensity, FLOAT3D &vTotalLightDirection, FLOATplane3D &plFloorPlane);
  /* Render a model. */
  void RenderOneModel( CEntity &en, CModelObject &moModel, const CPlacement3D &plModel,
                       const FLOAT fDistanceFactor, BOOL bRenderShadow, ULONG ulDMFlags);
  /* Render a ska model. */
  void RenderOneSkaModel( CEntity &en, const CPlacement3D &plModel,
                                  const FLOAT fDistanceFactor, BOOL bRenderShadow, ULONG ulDMFlags);
  /* Render models that were kept for delayed rendering. */
  void RenderModels(BOOL bBackground);
  /* Render active terrains */
  void RenderTerrains(void);
  /* Render active terrains in wireframe mode */
  void RenderWireFrameTerrains(void);
  /* Render particles for models that were kept for delayed rendering. */
  void RenderParticles(BOOL bBackground);
  // render one arrow given its 3d coordinates in world
  void ProjectClipAndDrawArrow(
    const FLOAT3D &v0, const FLOAT3D &v1, COLOR colColor);
  /* Render target lines for each drawn entity that has some targets. */
  void RenderEntityTargets(void);
  /* Render entity names. */
  void RenderEntityNames(void);
  /* Render lens flares. */
  void RenderLensFlares(void);
  /* Sort a list of translucent polygons. */
  ScenePolygon *SortTranslucentPolygons(ScenePolygon *pspoFirst);

  /* Prepare a brush entity for rendering if it is not yet prepared. */
  void PrepareBrush(CEntity *penBrush);

  /* Add a non-zoning brush entity to rendering list (add all sectors immediately). */
  void AddNonZoningBrush(CEntity *penBrush, CBrushSector *pbscThatAdds);
  /* Add a model entity to rendering. */
  void AddModelEntity(CEntity *penModel);
  /* Add a ska model entity to rendering. */
  void AddSkaModelEntity(CEntity *penModel);
  /* Add a terrain entity to rendering list. */
  void AddTerrainEntity(CEntity *penTerrain);
  /* Add a lens flare to rendering. */
  void AddLensFlare(CEntity *penLight, CLightSource *pls, CProjection3D *pprProjection, INDEX iMirrorLevel=0);

  /* Add to rendering all entities in the world (used in special cases in world editor). */
  void AddAllEntities(void);
  /* Add to rendering all entities that are inside an zoning brush sector. */
  void AddEntitiesInSector(CBrushSector *pbscSectorInside);
  /* Add to rendering all zoning brush sectors that an entity is in. */
  void AddZoningSectorsAroundEntity(CEntity *pen, const FLOAT3D &vEyesPos);
  /* Add to rendering one particular zoning brush sector. */
  void AddGivenZoningSector(CBrushSector *pbscSector);
  /* Add to rendering all zoning brush sectors near a given box in absolute space. */
  void AddZoningSectorsAroundBox(const FLOATaabbox3D &boxNear);
  /* Add to rendering all entities that are inside a given box. */
  void AddEntitiesInBox(const FLOATaabbox3D &boxNear);

  /* Constructor. */
  CRenderer(void);
  /* Destructor. */
  ~CRenderer(void);

  // initialize clipping rectangle
  void InitClippingRectangle(PIX pixMinI, PIX pixMinJ, PIX pixSizeI, PIX pixSizeJ);
  // do the rendering
  void Render(void);

  // initialize all rendering structures
  void Initialize(void);
  // add initial sectors to active lists
  void AddInitialSectors(void);
  // scan through portals for other sectors
  void ScanForOtherSectors(void);
  // cleanup after scanning
  void CleanupScanning(void);
  // draw the prepared things to screen
  void DrawToScreen(void);
  // draw mirror polygons to z-buffer to enable drawing of mirror
  void FillMirrorDepth(CMirror &mi);
};

// Render a world with some viewer, projection and drawport. (viewer may be NULL)
// internal version used for rendering shadows
extern ULONG RenderShadows(CWorld &woWorld, CEntity &enViewer,
  CAnyProjection3D &prProjection, const FLOATaabbox3D &boxViewer, 
  UBYTE *pubShadowMask, SLONG slShadowWidth, SLONG slShadowHeight,
  UBYTE ubIllumination);


#endif  /* include-once check. */

