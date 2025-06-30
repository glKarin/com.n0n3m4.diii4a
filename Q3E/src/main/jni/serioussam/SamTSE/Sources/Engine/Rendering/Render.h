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

#include <Engine/Entities/Entity.h>
#include <Engine/Brushes/Brush.h>

/*
 * Preferences for rendering world (another class is used for rendering models).
 */
class ENGINE_API CWorldRenderPrefs {
public:
  enum FillType {
    FT_NONE,            // do not draw this element
    FT_INKCOLOR,        // draw all in same color
    FT_POLYGONCOLOR,    // draw in colors of corresponding polygons
    FT_SECTORCOLOR,     // draw in colors of corresponding sectors
    FT_TEXTURE,         // draw with texture
  };
  enum SelectionType {
    ST_NONE,            // no selection is drawn
    ST_ENTITIES,        // selected entities are marked
    ST_POLYGONS,        // selected polygons are marked
    ST_SECTORS,         // selected sectors are marked
    ST_VERTICES,        // selected vertices are marked
  };
  enum ShadowsType {
    SHT_NONE = 0,         // do not use any shadows
    SHT_NOAUTOCALCULATE,  // use shadows, but don't auto recalculate
    SHT_FULL = 255,       // use full shadows
  };
  enum LensFlaresType {
    LFT_NONE = 0,               // do not render lens flares at all
    LFT_SINGLE_FLARE,           // render only single flare without reflections
    LFT_REFLECTIONS,            // render flares and reflections
    LFT_REFLECTIONS_AND_GLARE,  // render flare, reflections and glare
  };
public:
  BOOL wrp_bHiddenLinesOn;  // set if hidden lines should be drawn
  BOOL wrp_bShowTargetsOn;  // set if lines to target entities should be drawn
  BOOL wrp_bShowEntityNames;// set if entity names should be typed
  BOOL wrp_bEditorModelsOn; // set if editor models should be drawn
  BOOL wrp_bFieldBrushesOn; // set if field brushes should be drawn
  BOOL wrp_bBackgroundTextureOn; // set if background texture should be drawn
  BOOL wrp_bShowVisTweaksOn;  // set if visibility tweaks for selected sectors should be shown
  BOOL wrp_bDisableVisTweaks; // set if entities utilitizing visibility tweaks should be shown allways
  BOOL wrp_bAutoMipBrushingOn;        // set if mip brushing should be automatic
  FLOAT wrp_fManualMipBrushingFactor; // mip factor for manual mip brushing
  FLOAT wrp_fFarClipPlane;                          // far clip plane
  BOOL wrp_bApplyFarClipPlaneInIsometricProjection; // if far clip plane should be applied in isometric projections

  enum FillType wrp_ftVertices; // fill type for vertices
  COLOR wrp_colVertices;        // color used for drawing vertices in ink-color mode

  enum FillType wrp_ftEdges;    // fill type for edges
  COLOR wrp_colEdges;           // color used for drawing edges in ink-color mode

  enum FillType wrp_ftPolygons; // fill type for polygons
  COLOR wrp_colPolygons;        // color used for drawing polygons in ink-color mode

  enum ShadowsType wrp_shtShadows;  // degree of using shadows
  enum LensFlaresType wrp_lftLensFlares;  // degree of using lens flares

  BOOL wrp_abTextureLayers[3];  // set for texture layers that are rendered

  BOOL wrp_bFogOn;          // set if fog should be rendered
  BOOL wrp_bHazeOn;         // set if haze should be rendered
  BOOL wrp_bMirrorsOn;      // set if mirrors should be rendered

  FLOAT wrp_fMinimumRenderRange; // range (in meters) around viewer that is always drawn

  CModelObject *wrp_pmoSelectedEntity; // model used for marking selected entities
  CModelObject *wrp_pmoSelectedPortal; // model used for marking selected portals
  CModelObject *wrp_pmoEmptyBrush;     // model used for marking brushes with no sectors

  enum SelectionType wrp_stSelection;   // what kind of selection is shown

public:
  /* Constructor -- sets default values. */
  CWorldRenderPrefs(void);

  /* Test if drawing of hidden edges is turned on. */
  inline BOOL IsHiddenLinesOn(void) { return wrp_bHiddenLinesOn; };
  /* Set drawing of hidden edges on or off. */
  inline void SetHiddenLinesOn(BOOL bOn) { wrp_bHiddenLinesOn = bOn; };

  /* Test if drawing of lines to target entities is turned on. */
  inline BOOL IsShowTargetsOn(void) { return wrp_bShowTargetsOn; };
  /* Set drawing of lines to target entities on or off. */
  inline void SetShowTargetsOn(BOOL bOn) { wrp_bShowTargetsOn = bOn; };

  /* Test if typing of entity names is turned on. */
  inline BOOL IsShowEntityNamesOn(void) { return wrp_bShowEntityNames; };
  /* Set typing of entity names on or off. */
  inline void SetShowEntityNamesOn(BOOL bOn) { wrp_bShowEntityNames = bOn; };  

  /* Test if drawing of editor models is turned on. */
  inline BOOL IsEditorModelsOn(void) { return wrp_bEditorModelsOn; };
  /* Set drawing of editor models on or off. */
  inline void SetEditorModelsOn(BOOL bOn) { wrp_bEditorModelsOn = bOn; };
  /* Test if drawing of editor models is turned on. */
  inline BOOL IsFieldBrushesOn(void) { return wrp_bFieldBrushesOn; };
  /* Set drawing of editor models on or off. */
  inline void SetFieldBrushesOn(BOOL bOn) { wrp_bFieldBrushesOn = bOn; };

  /* Test if drawing background texture is on. */
  inline BOOL IsBackgroundTextureOn(void) { return wrp_bBackgroundTextureOn; };
  /* Set drawing of background texture on or off. */
  inline void SetBackgroundTextureOn(BOOL bOn) { wrp_bBackgroundTextureOn = bOn; };

  /* Test if drawing visibility tweaks for selected sectors is on. */
  inline BOOL IsVisTweaksOn(void) { return wrp_bShowVisTweaksOn; };
  /* Set drawing of visibility tweaks for selected sectors on or off. */
  inline void SetVisTweaksOn(BOOL bOn) { wrp_bShowVisTweaksOn = bOn; };

  /* Test if visibility tweaks are off. */
  inline BOOL IsVisTweaksDisabled(void) { return wrp_bDisableVisTweaks; };
  /* Set disabling of visibility tweaks on or off. */
  inline void DisableVisTweaks(BOOL bOn) { wrp_bDisableVisTweaks = bOn; };
  
  /* Test if drawing fog is on. */
  inline BOOL IsFogOn(void) { return wrp_bFogOn; };
  /* Set drawing of fog on or off. */
  inline void SetFogOn(BOOL bOn) { wrp_bFogOn = bOn; };
  /* Test if drawing haze is on. */
  inline BOOL IsHazeOn(void) { return wrp_bHazeOn; };
  /* Set drawing of haze on or off. */
  inline void SetHazeOn(BOOL bOn) { wrp_bHazeOn = bOn; };
  /* Test if drawing mirrors is on. */
  inline BOOL IsMirrorsOn(void) { return wrp_bMirrorsOn; };
  /* Set drawing of mirrors on or off. */
  inline void SetMirrorsOn(BOOL bOn) { wrp_bMirrorsOn = bOn; };

  /* Test if mip brushing should be automatic. */
  inline BOOL IsAutoMipBrushingOn(void) { return wrp_bAutoMipBrushingOn; };
  /* Set mip brushing should to automatic or manual. */
  inline void SetAutoMipBrushingOn(BOOL bOn) { wrp_bAutoMipBrushingOn = bOn; };
  /* Set/get mip factor for manual mip brushing. */
  inline void SetManualMipBrushingFactor(FLOAT fFactor) { wrp_fManualMipBrushingFactor = fFactor; };
  inline FLOAT GetManualMipBrushingFactor(void) { return wrp_fManualMipBrushingFactor; };

  /* Test if texture layer is shown. */
  inline BOOL IsTextureLayerOn(INDEX iTexture) {
    ASSERT(iTexture>=0 && iTexture<3);
    return wrp_abTextureLayers[iTexture];
  };
  /* Set texture layer shown/hidden. */
  inline void SetTextureLayerOn(BOOL bOn, INDEX iTexture) {
    ASSERT(iTexture>=0 && iTexture<3);
    wrp_abTextureLayers[iTexture] = bOn;
  };

  // Get mip brushing factor relevant for given distance mip factor
  FLOAT GetCurrentMipBrushingFactor(FLOAT fDistanceMipFactor);

  /* Set/get the fill type for vertices. */
  inline void SetVerticesFillType(enum FillType ft) { wrp_ftVertices = ft; };
  inline enum FillType GetVerticesFillType(void) { return wrp_ftVertices; };
  /* Set/get the ink color for vertices. */
  inline void SetVerticesInkColor(COLOR color) { wrp_colVertices = color; };
  inline COLOR GetVerticesInkColor(void) { return wrp_colVertices; };

  /* Set/get the fill type for edges. */
  inline void SetEdgesFillType(enum FillType ft) { wrp_ftEdges = ft; };
  inline enum FillType GetEdgesFillType(void) { return wrp_ftEdges; };
  /* Set/get the ink color for edges. */
  inline void SetEdgesInkColor(COLOR color) { wrp_colEdges = color; };
  inline COLOR GetEdgesInkColor(void) { return wrp_colEdges; };

  /* Set/get the fill type for polygons. */
  inline void SetPolygonsFillType(enum FillType ft) { wrp_ftPolygons= ft; };
  inline enum FillType GetPolygonsFillType(void) { return wrp_ftPolygons; };
  /* Set/get the ink color for polygons. */
  inline void SetPolygonsInkColor(COLOR color) { wrp_colPolygons = color; };
  inline COLOR GetPolygonsInkColor(void) { return wrp_colPolygons; };

  /* Set/get the brush shadow quality. */
  inline void SetShadowsType( enum ShadowsType sht) { wrp_shtShadows = sht; };
  inline enum ShadowsType GetShadowsType(void) { return wrp_shtShadows; };

  /* Set/get the lens flare quality. */
  inline void SetLensFlaresType( enum LensFlaresType lft) { wrp_lftLensFlares = lft; };
  inline enum LensFlaresType GetLensFlares(void) { return wrp_lftLensFlares; };

  /* Set/get range (in meters) around viewer that is always drawn. */
  inline void SetMinimumRenderRange(FLOAT fRange) { wrp_fMinimumRenderRange = fRange; };
  inline FLOAT GetMinimumRenderRange(void) { return wrp_fMinimumRenderRange; };

  /* Set/get model object used for marking selected models. */
  inline void SetSelectedEntityModel(CModelObject *pmoSelection) { wrp_pmoEmptyBrush = wrp_pmoSelectedEntity = pmoSelection; };
  inline CModelObject *GetSelectedEntityModel(void) { return wrp_pmoSelectedEntity; };

  /* Set/get model object used for marking selected portals. */
  inline void SetSelectedPortalModel(CModelObject *pmoSelection) { wrp_pmoSelectedPortal = pmoSelection; };
  inline CModelObject *GetSelectedPortalModel(void) { return wrp_pmoSelectedPortal; };

  /* Set/get model object used for marking empty brushes. */
  inline void SetEmptyBrushModel(CModelObject *pmoBrush) { wrp_pmoEmptyBrush = pmoBrush; };
  inline CModelObject *GetEmptyBrushModel(void) { return wrp_pmoEmptyBrush; };

  /* Set/get the selection type. */
  inline void SetSelectionType(enum SelectionType st) { wrp_stSelection = st; };
  inline enum SelectionType GetSelectionType(void) { return wrp_stSelection; };
};

// global instance used in rendering
ENGINE_API extern CWorldRenderPrefs _wrpWorldRenderPrefs;

// variables for selection on rendering
ENGINE_API extern CBrushVertexSelection *_pselbvxtSelectOnRender;
ENGINE_API extern CEntitySelection *_pselenSelectOnRender;
ENGINE_API extern CStaticStackArray<PIX2D> *_pavpixSelectLasso;
ENGINE_API extern PIX2D _vpixSelectNearPoint;
ENGINE_API extern BOOL _bSelectAlternative;
ENGINE_API extern PIX _pixDeltaAroundVertex;
ENGINE_API extern CBrushSectorSelection *_pselbscVisTweaks;

/*
 * Rendering interface
 */
// Render a world with some viewer, projection and drawport. (viewer may be NULL)
ENGINE_API extern void RenderView(CWorld &woWorld, CEntity &enViewer,
  CAnyProjection3D &prProjection, CDrawPort &dpDrawport);

// shading info for viewer of last rendered view
ENGINE_API extern FLOAT3D _vViewerLightDirection;
ENGINE_API extern COLOR _colViewerLight;
ENGINE_API extern COLOR _colViewerAmbient;
