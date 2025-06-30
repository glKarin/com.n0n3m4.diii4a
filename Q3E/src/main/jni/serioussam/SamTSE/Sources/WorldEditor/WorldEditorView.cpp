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

// WorldEditorView.cpp : implementation of the CWorldEditorView class
//

#include "stdafx.h"
#include "WorldEditor.h"
#include <Engine/Base/Profiling.h>
#include <Engine/Base/Statistics.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Terrain/TerrainEditing.h>
#include <Engine/Terrain/TerrainMisc.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTimerValue _tvLastTerrainBrushingApplied;
static INDEX aiForCreationOfSizingVertices[14][6] =
{
  {0,0,2,2,5,5},
  {1,1,2,2,5,5},
  {1,1,2,2,4,4},
  {0,0,2,2,4,4},
  {0,0,3,3,5,5},
  {1,1,3,3,5,5},
  {1,1,3,3,4,4},
  {0,0,3,3,4,4},
  {0,1,2,3,5,5},
  {1,1,2,3,4,5},
  {0,1,2,3,4,4},
  {0,0,2,3,4,5},
  {0,1,2,2,4,5},
  {0,1,3,3,4,5},
};

static INDEX aiForAllowedSizing[14][6] =
{
  {1,0,1,0,0,1},
  {0,1,1,0,0,1},
  {0,1,1,0,1,0},
  {1,0,1,0,1,0},
  {1,0,0,1,0,1},
  {0,1,0,1,0,1},
  {0,1,0,1,1,0},
  {1,0,0,1,1,0},
  {0,0,0,0,0,1},
  {0,1,0,0,0,0},
  {0,0,0,0,1,0},
  {1,0,0,0,0,0},
  {0,0,1,0,0,0},
  {0,0,0,1,0,0},
};

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView

IMPLEMENT_DYNCREATE(CWorldEditorView, CView)

BEGIN_MESSAGE_MAP(CWorldEditorView, CView)
	//{{AFX_MSG_MAP(CWorldEditorView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_DROPFILES()
	ON_COMMAND(ID_ISOMETRIC_FRONT, OnIsometricFront)
	ON_COMMAND(ID_ISOMETRIC_BACK, OnIsometricBack)
	ON_COMMAND(ID_ISOMETRIC_BOTTOM, OnIsometricBottom)
	ON_COMMAND(ID_ISOMETRIC_LEFT, OnIsometricLeft)
	ON_COMMAND(ID_ISOMETRIC_RIGHT, OnIsometricRight)
	ON_COMMAND(ID_ISOMETRIC_TOP, OnIsometricTop)
	ON_COMMAND(ID_PERSPECTIVE, OnPerspective)
	ON_COMMAND(ID_ZOOM_LESS, OnZoomLess)
	ON_COMMAND(ID_ZOOM_MORE, OnZoomMore)
	ON_COMMAND(ID_MOVE_DOWN, OnMoveDown)
	ON_COMMAND(ID_MOVE_UP, OnMoveUp)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_MEASUREMENT_TAPE, OnMeasurementTape)
	ON_UPDATE_COMMAND_UI(ID_MEASUREMENT_TAPE, OnUpdateMeasurementTape)
	ON_COMMAND(ID_CIRCLE_MODES, OnCircleModes)
	ON_UPDATE_COMMAND_UI(ID_CIRCLE_MODES, OnUpdateCircleModes)
	ON_COMMAND(ID_DESELECT_ALL, OnDeselectAll)
	ON_COMMAND(ID_DELETE_ENTITIES, OnDeleteEntities)
	ON_UPDATE_COMMAND_UI(ID_DELETE_ENTITIES, OnUpdateDeleteEntities)
	ON_WM_SETCURSOR()
	ON_COMMAND(ID_TAKE_SS, OnTakeSs)
	ON_UPDATE_COMMAND_UI(ID_ENTITY_MODE, OnUpdateEntityMode)
	ON_COMMAND(ID_SECTOR_MODE, OnSectorMode)
	ON_UPDATE_COMMAND_UI(ID_SECTOR_MODE, OnUpdateSectorMode)
	ON_COMMAND(ID_POLYGON_MODE, OnPolygonMode)
	ON_UPDATE_COMMAND_UI(ID_POLYGON_MODE, OnUpdatePolygonMode)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_CLONE_CSG, OnCloneCSG)
	ON_UPDATE_COMMAND_UI(ID_CLONE_CSG, OnUpdateCloneCsg)
	ON_COMMAND(ID_MEASURE_ON, OnMeasureOn)
	ON_UPDATE_COMMAND_UI(ID_MEASURE_ON, OnUpdateMeasureOn)
	ON_COMMAND(ID_RESET_VIEWER, OnResetViewer)
	ON_COMMAND(ID_COPY_TEXTURE, OnCopyTexture)
	ON_COMMAND(ID_PASTE_TEXTURE, OnPasteTexture)
	ON_COMMAND(ID_CENTER_ENTITY, OnCenterEntity)
	ON_COMMAND(ID_FUNCTION, OnFunction)
	ON_UPDATE_COMMAND_UI(ID_CENTER_ENTITY, OnUpdateCenterEntity)
	ON_COMMAND(ID_DROP_MARKER, OnDropMarker)
	ON_UPDATE_COMMAND_UI(ID_DROP_MARKER, OnUpdateDropMarker)
	ON_COMMAND(ID_TEST_CONNECTIONS, OnTestConnections)
	ON_UPDATE_COMMAND_UI(ID_TEST_CONNECTIONS, OnUpdateTestConnections)
	ON_COMMAND(ID_ALIGN_VOLUME, OnAlignVolume)
	ON_UPDATE_COMMAND_UI(ID_ALIGN_VOLUME, OnUpdateAlignVolume)
	ON_COMMAND(ID_CURRENT_VIEW_PROPERTIES, OnCurrentViewProperties)
  ON_COMMAND(ID_DELETE_MIP, OnDeleteMip)
	ON_UPDATE_COMMAND_UI(ID_DELETE_MIP, OnUpdateDeleteMip)
	ON_COMMAND(ID_PREVIOUS_MIP_BRUSH, OnPreviousMipBrush)
	ON_UPDATE_COMMAND_UI(ID_PREVIOUS_MIP_BRUSH, OnUpdatePreviousMipBrush)
	ON_COMMAND(ID_NEXT_MIP_BRUSH, OnNextMipBrush)
	ON_UPDATE_COMMAND_UI(ID_NEXT_MIP_BRUSH, OnUpdateNextMipBrush)
	ON_COMMAND(ID_CROSSROAD_FOR_C, OnCrossroadForC)
	ON_COMMAND(ID_CHOOSE_COLOR, OnChooseColor)
	ON_UPDATE_COMMAND_UI(ID_CHOOSE_COLOR, OnUpdateChooseColor)
	ON_COMMAND(ID_MENU_COPY_MAPPING, OnMenuCopyMapping)
	ON_COMMAND(ID_MENU_PASTE_MAPPING, OnMenuPasteMapping)
	ON_COMMAND(ID_SET_AS_CSG_TARGET, OnSetAsCsgTarget)
	ON_COMMAND(ID_KEY_PASTE, OnKeyPaste)
	ON_WM_RBUTTONDBLCLK()
	ON_COMMAND(ID_SELECT_BY_TEXTURE_ADJACENT, OnSelectByTextureAdjacent)
	ON_COMMAND(ID_SELECT_BY_TEXTURE_IN_SECTOR, OnSelectByTextureInSector)
	ON_COMMAND(ID_SELECT_BY_COLOR_IN_SECTOR, OnSelectByColorInSector)
	ON_COMMAND(ID_CONUS_PRIMITIVE, OnConusPrimitive)
	ON_COMMAND(ID_TORUS_PRIMITIVE, OnTorusPrimitive)
	ON_COMMAND(ID_TERRAIN_PRIMITIVE, OnTerrainPrimitive)
	ON_COMMAND(ID_SPHERE_PRIMITIVE, OnSpherePrimitive)
	ON_COMMAND(ID_STAIRCASE_PRIMITIVE, OnStaircasePrimitive)
	ON_UPDATE_COMMAND_UI(ID_CONUS_PRIMITIVE, OnUpdateConusPrimitive)
	ON_UPDATE_COMMAND_UI(ID_SPHERE_PRIMITIVE, OnUpdateSpherePrimitive)
	ON_UPDATE_COMMAND_UI(ID_TERRAIN_PRIMITIVE, OnUpdateTerrainPrimitive)
	ON_UPDATE_COMMAND_UI(ID_TORUS_PRIMITIVE, OnUpdateTorusPrimitive)
	ON_UPDATE_COMMAND_UI(ID_STAIRCASE_PRIMITIVE, OnUpdateStaircasePrimitive)
	ON_COMMAND(ID_POPUP_CONUS, OnPopupConus)
	ON_COMMAND(ID_POPUP_SPHERE, OnPopupSphere)
	ON_COMMAND(ID_POPUP_STAIRS, OnPopupStairs)
	ON_COMMAND(ID_POPUP_TERRAIN, OnPopupTerrain)
	ON_COMMAND(ID_POPUP_TORUS, OnPopupTorus)
	ON_UPDATE_COMMAND_UI(ID_MOVE_DOWN, OnUpdateMoveDown)
	ON_UPDATE_COMMAND_UI(ID_MOVE_UP, OnUpdateMoveUp)
	ON_COMMAND(ID_SELECT_LIGHTS, OnSelectLights)
	ON_COMMAND(ID_DISCARD_SHADOWS, OnDiscardShadows)
	ON_COMMAND(ID_COPY_SECTOR_AMBIENT, OnCopySectorAmbient)
	ON_COMMAND(ID_PASTE_SECTOR_AMBIENT, OnPasteSectorAmbient)
	ON_COMMAND(ID_SELECT_ALL_POLYGONS, OnSelectAllPolygons)
	ON_COMMAND(ID_COPY_SECTORS, OnCopySectors)
	ON_COMMAND(ID_DELETE_SECTORS, OnDeleteSectors)
	ON_COMMAND(ID_PASTE_SECTORS, OnPasteSectors)
	ON_COMMAND(ID_CENTER_BCG_VIEWER, OnCenterBcgViewer)
	ON_COMMAND(ID_MENU_PASTE_AS_PROJECTED_MAPPING, OnMenuPasteAsProjectedMapping)
	ON_COMMAND(ID_KEY_PASTE_AS_PROJECTED, OnKeyPasteAsProjected)
	ON_COMMAND(ID_SELECT_ALL_ENTITIES, OnSelectAllEntitiesInSectors)
	ON_COMMAND(ID_SELECT_ALL_SECTORS, OnSelectAllSectors)
	ON_COMMAND(ID_LAST_PRIMITIVE, OnLastPrimitive)
	ON_UPDATE_COMMAND_UI(ID_LAST_PRIMITIVE, OnUpdateLastPrimitive)
  ON_COMMAND(ID_CLONE_TO_MORE_PRECISE_MIP, OnCloneToMorePreciseMip)
	ON_COMMAND(ID_CLONE_TO_ROUGHER_MIP_LEVEL, OnCloneToRougherMipLevel)
	ON_COMMAND(ID_CREATE_EMPTY_MORE_PRECISE_MIP, OnCreateEmptyMorePreciseMip)
	ON_COMMAND(ID_CREATE_EMPTY_ROUGHER_MIP, OnCreateEmptyRougherMip)
	ON_UPDATE_COMMAND_UI(ID_CLONE_TO_MORE_PRECISE_MIP, OnUpdateCloneToMorePreciseMip)
	ON_UPDATE_COMMAND_UI(ID_CLONE_TO_ROUGHER_MIP_LEVEL, OnUpdateCloneToRougherMipLevel)
	ON_UPDATE_COMMAND_UI(ID_CREATE_EMPTY_MORE_PRECISE_MIP, OnUpdateCreateEmptyMorePreciseMip)
	ON_UPDATE_COMMAND_UI(ID_CREATE_EMPTY_ROUGHER_MIP, OnUpdateCreateEmptyRougherMip)
  ON_COMMAND(ID_EDIT_PASTE_ALTERNATIVE, OnEditPasteAlternative)
	ON_COMMAND(ID_SELECT_ALL_ENTITIES_IN_WORLD, OnSelectAllEntitiesInWorld)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_ALTERNATIVE, OnUpdateEditPasteAlternative)
	ON_COMMAND(ID_ENTITY_MODE, OnEntityMode)
	ON_COMMAND(ID_FIND_TEXTURE, OnFindTexture)
	ON_COMMAND(ID_SELECT_SECTORS_WITH_SAME_NAME, OnSelectSectorsWithSameName)
	ON_COMMAND(ID_SELECT_SECTORS_ARROUND_ENTITY, OnSelectSectorsArroundEntity)
	ON_COMMAND(ID_SELECT_SECTORS_ARROUND_ENTITY_ON_CONTEXT, OnSelectSectorsArroundEntityOnContext)
	ON_COMMAND(ID_INSERT_VERTEX, OnInsertVertex)
	ON_COMMAND(ID_DELETE_VERTEX, OnDeleteVertex)
	ON_COMMAND(ID_SAVE_PICTURES_FOR_ENVIRONMENT, OnSavePicturesForEnvironment)
	ON_UPDATE_COMMAND_UI(ID_SAVE_PICTURES_FOR_ENVIRONMENT, OnUpdateSavePicturesForEnvironment)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_CSG_SELECT_SECTOR, OnCsgSelectSector)
	ON_COMMAND(ID_MENU_ALIGN_MAPPING_U, OnMenuAlignMappingU)
	ON_COMMAND(ID_MENU_ALIGN_MAPPING_V, OnMenuAlignMappingV)
	ON_WM_DESTROY()
	ON_COMMAND(ID_FALL_DOWN, OnFallDown)
	ON_COMMAND(ID_PREVIOUS, OnPrevious)
	ON_COMMAND(ID_NEXT, OnNext)
	ON_UPDATE_COMMAND_UI(ID_PREVIOUS, OnUpdatePrevious)
	ON_UPDATE_COMMAND_UI(ID_NEXT, OnUpdateNext)
	ON_COMMAND(ID_REMOVE_UNUSED_TEXTURES, OnRemoveUnusedTextures)
	ON_COMMAND(ID_ROTATE, OnRotate)
	ON_COMMAND(ID_ROTATE_BACK, OnRotateBack)
	ON_COMMAND(ID_SELECT_VISIBLE_SECTORS, OnSelectVisibleSectors)
	ON_COMMAND(ID_EDIT_COPY_ALTERNATIVE, OnEditCopyAlternative)
	ON_COMMAND(ID_ROTATE_LEFT, OnRotateLeft)
	ON_COMMAND(ID_ROTATE_RIGHT, OnRotateRight)
	ON_COMMAND(ID_ROTATE_UP, OnRotateUp)
	ON_COMMAND(ID_ROTATE_DOWN, OnRotateDown)
	ON_COMMAND(ID_SELECT_WHO_TARGETS, OnSelectWhoTargets)
	ON_COMMAND(ID_SELECT_INVALIDTRIS, OnSelectInvalidTris)
	ON_COMMAND(ID_TEST_CONNECTIONS_BACK, OnTestConnectionsBack)
	ON_UPDATE_COMMAND_UI(ID_TEST_CONNECTIONS_BACK, OnUpdateTestConnectionsBack)
	ON_WM_MOUSEWHEEL()
	ON_COMMAND(ID_SELECT_SECTORS_OTHER_SIDE, OnSelectSectorsOtherSide)
	ON_COMMAND(ID_SELECT_LINKS_TO_SECTOR, OnSelectLinksToSector)
	ON_COMMAND(ID_REMAIN_SELECTEDBY_ORIENTATION, OnRemainSelectedByOrientation)
	ON_COMMAND(ID_DESELECT_BY_ORIENTATION, OnDeselectByOrientation)
	ON_COMMAND(ID_VERTEX_MODE, OnVertexMode)
	ON_COMMAND(ID_REOPTIMIZE_BRUSHES, OnReoptimizeBrushes)
	ON_COMMAND(ID_MERGE_VERTICES, OnMergeVertices)
	ON_COMMAND(ID_EXPORT_DISPLACE_MAP, OnExportDisplaceMap)
	ON_COMMAND(ID_CUT_MODE, OnCutMode)
	ON_UPDATE_COMMAND_UI(ID_CUT_MODE, OnUpdateCutMode)
	ON_COMMAND(ID_SELECT_ALL_TARGETS, OnSelectAllTargets)
	ON_COMMAND(ID_SELECT_ALL_TARGETS_ON_CONTEXT, OnSelectAllTargetsOnContext)
	ON_COMMAND(ID_SELECT_CLONES, OnSelectClones)
	ON_COMMAND(ID_SELECT_CLONES_ON_CONTEXT, OnSelectClonesOnContext)
	ON_UPDATE_COMMAND_UI(ID_SELECT_CLONES, OnUpdateSelectClones)
	ON_COMMAND(ID_SELECT_ALL_VERTICES, OnSelectAllVertices)
	ON_COMMAND(ID_SELECT_OF_SAME_CLASS, OnSelectOfSameClass)
	ON_COMMAND(ID_SELECT_OF_SAME_CLASS_ON_CONTEXT, OnSelectOfSameClassOnContext)
	ON_COMMAND(ID_ALTERNATIVE_MOVING_MODE, OnAlternativeMovingMode)
	ON_COMMAND(ID_RE_TRIPLE, OnReTriple)
	ON_COMMAND(ID_SELECT_WHO_TARGETS_ON_CONTEXT, OnSelectWhoTargetsOnContext)
	ON_COMMAND(ID_CLEAR_ALL_TARGETS, OnClearAllTargets)
	ON_COMMAND(ID_SELECT_CSG_TARGET, OnSelectCsgTarget)
	ON_UPDATE_COMMAND_UI(ID_SELECT_CSG_TARGET, OnUpdateSelectCsgTarget)
	ON_COMMAND(ID_REMAIN_SELECTEDBY_ORIENTATION_SINGLE, OnRemainSelectedbyOrientationSingle)
	ON_UPDATE_COMMAND_UI(ID_RE_TRIPLE, OnUpdateReTriple)
	ON_COMMAND(ID_TRIANGULARIZE_POLYGON, OnTriangularizePolygon)
	ON_COMMAND(ID_ENTITY_CONTEXT_HELP, OnEntityContextHelp)
	ON_COMMAND(ID_POPUP_AUTO_FIT_MAPPING, OnPopupAutoFitMapping)
	ON_COMMAND(ID_TRIANGULARIZE_SELECTION, OnTriangularizeSelection)
	ON_UPDATE_COMMAND_UI(ID_TRIANGULARIZE_SELECTION, OnUpdateTriangularizeSelection)
	ON_COMMAND(ID_POPUP_AUTO_FIT_MAPPING_SMALL, OnPopupAutoFitMappingSmall)
	ON_COMMAND(ID_POPUP_AUTO_FIT_MAPPING_BOTH, OnPopupAutoFitMappingBoth)
	ON_COMMAND(ID_RESET_MAPPING_OFFSET, OnResetMappingOffset)
	ON_COMMAND(ID_RESET_MAPPING_ROTATION, OnResetMappingRotation)
	ON_COMMAND(ID_RESET_MAPPING_STRETCH, OnResetMappingStretch)
	ON_COMMAND(ID_CROSSROAD_FOR_L, OnCrossroadForL)
	ON_COMMAND(ID_SELECT_USING_TARGET_TREE, OnSelectUsingTargetTree)
	ON_COMMAND(ID_TARGET_TREE, OnTargetTree)
	ON_UPDATE_COMMAND_UI(ID_TARGET_TREE, OnUpdateTargetTree)
	ON_COMMAND(ID_SWAP_LAYERS_12, OnSwapLayers12)
	ON_COMMAND(ID_SWAP_LAYERS_23, OnSwapLayers23)
	ON_COMMAND(ID_SELECT_DESCENDANTS, OnSelectDescendants)
	ON_COMMAND(ID_CROSSROAD_FOR_CTRL_F, OnCrossroadForCtrlF)
	ON_COMMAND(ID_ROTATE_TO_TARGET_CENTER, OnRotateToTargetCenter)
	ON_COMMAND(ID_ROTATE_TO_TARGET_ORIGIN, OnRotateToTargetOrigin)
	ON_COMMAND(ID_COPY_ORIENTATION, OnCopyOrientation)
	ON_COMMAND(ID_COPY_PLACEMENT, OnCopyPlacement)
	ON_COMMAND(ID_COPY_POSITION, OnCopyPosition)
	ON_COMMAND(ID_PASTE_ORIENTATION, OnPasteOrientation)
	ON_COMMAND(ID_PASTE_PLACEMENT, OnPastePlacement)
	ON_COMMAND(ID_PASTE_POSITION, OnPastePosition)
	ON_COMMAND(ID_ALIGN_B, OnAlignB)
	ON_COMMAND(ID_ALIGN_H, OnAlignH)
	ON_COMMAND(ID_ALIGN_P, OnAlignP)
	ON_COMMAND(ID_ALIGN_X, OnAlignX)
	ON_COMMAND(ID_ALIGN_Y, OnAlignY)
	ON_COMMAND(ID_ALIGN_Z, OnAlignZ)
	ON_COMMAND(ID_AUTOTEXTURIZE_MIPS, OnAutotexturizeMips)
	ON_UPDATE_COMMAND_UI(ID_AUTOTEXTURIZE_MIPS, OnUpdateAutotexturizeMips)
	ON_COMMAND(ID_RANDOM_OFFSET_U, OnRandomOffsetU)
	ON_COMMAND(ID_RANDOM_OFFSET_V, OnRandomOffsetV)
	ON_COMMAND(ID_STRETCH_RELATIVE_OFFSET, OnStretchRelativeOffset)
	ON_COMMAND(ID_DESELECT_HIDDEN, OnDeselectHidden)
	ON_COMMAND(ID_SELECT_HIDDEN, OnSelectHidden)
	ON_COMMAND(ID_SECTORS_TO_BRUSH, OnSectorsToBrush)
	ON_COMMAND(ID_POLYGONS_TO_BRUSH, OnPolygonsToBrush)
	ON_COMMAND(ID_CLONE_POLYGONS, OnClonePolygons)
	ON_COMMAND(ID_DELETE_POLYGONS, OnDeletePolygons)
	ON_COMMAND(ID_KEY_U, OnKeyU)
	ON_COMMAND(ID_KEY_D, OnKeyD)
	ON_COMMAND(ID_FLIP_POLYGON, OnFlipPolygon)
	ON_COMMAND(ID_TERRAIN_MODE, OnTerrainMode)
	ON_UPDATE_COMMAND_UI(ID_TERRAIN_MODE, OnUpdateTerrainMode)
	ON_COMMAND(ID_KEY_M, OnKeyM)
	ON_COMMAND(ID_KEY_BACKSLASH, OnKeyBackslash)
	ON_COMMAND(ID_SELECT_BRUSH, OnSelectBrush)
	ON_COMMAND(ID_SELECT_TERRAIN, OnSelectTerrain)
	ON_COMMAND(ID_ALTITUDE_EDIT_MODE, OnAltitudeEditMode)
	ON_COMMAND(ID_LAYER_TEXTURE_EDIT_MODE, OnLayerTextureEditMode)
	ON_COMMAND(ID_TBRUSH_ALTITUDE, OnTbrushAltitude)
	ON_COMMAND(ID_TBRUSH_EQUILAZE, OnTbrushEquilaze)
	ON_COMMAND(ID_TBRUSH_ERASE, OnTbrushErase)
	ON_COMMAND(ID_TBRUSH_NOISE, OnTbrushNoise)
	ON_COMMAND(ID_TBRUSH_SMOOTH, OnTbrushSmooth)
	ON_COMMAND(ID_OPTIMIZE_TERRAIN, OnOptimizeTerrain)
	ON_COMMAND(ID_RECALCULATE_TERRAIN_SHADOWS, OnRecalculateTerrainShadows)
	ON_COMMAND(ID_VIEW_HEIGHTMAP, OnViewHeightmap)
	ON_COMMAND(ID_IMPORT_HEIGHTMAP, OnImportHeightmap)
	ON_COMMAND(ID_EXPORT_HEIGHTMAP, OnExportHeightmap)
	ON_COMMAND(ID_IMPORT_HEIGHTMAP16, OnImportHeightmap16)
	ON_COMMAND(ID_EXPORT_HEIGHTMAP16, OnExportHeightmap16)
	ON_COMMAND(ID_SELECT_LAYER, OnSelectLayer)
	ON_COMMAND(ID_PICK_LAYER, OnPickLayer)
	ON_COMMAND(ID_KEY_O, OnKeyO)
	ON_UPDATE_COMMAND_UI(ID_KEY_O, OnUpdateKeyO)
	ON_COMMAND(ID_POSTERIZE, OnPosterize)
	ON_COMMAND(ID_EQUILIZE, OnFlatten)
	ON_COMMAND(ID_APPLY_FILTER, OnApplyFilter)
	ON_COMMAND(ID_TE_SMOOTH, OnTeSmooth)
	ON_COMMAND(ID_EDIT_TERRAIN_PREFS, OnEditTerrainPrefs)
	ON_UPDATE_COMMAND_UI(ID_EDIT_TERRAIN_PREFS, OnUpdateEditTerrainPrefs)
	ON_COMMAND(ID_KEY_CTRL_SHIFT_E, OnKeyCtrlShiftE)
	ON_COMMAND(ID_KEY_CTRL_SHIFT_G, OnKeyCtrlShiftG)
	ON_UPDATE_COMMAND_UI(ID_KEY_CTRL_SHIFT_G, OnUpdateKeyCtrlShiftG)
	ON_COMMAND(ID_TERRAIN_LAYER_OPTIONS, OnTerrainLayerOptions)
	ON_UPDATE_COMMAND_UI(ID_TERRAIN_LAYER_OPTIONS, OnUpdateTerrainLayerOptions)
	ON_COMMAND(ID_KEY_CTRL_SHIFT_K, OnKeyCtrlShiftK)
	ON_COMMAND(ID_APPLY_CONTINOUS_NOISE, OnApplyContinousNoise)
	ON_COMMAND(ID_APPLY_MINIMUM, OnApplyMinimum)
	ON_COMMAND(ID_APPLY_MAXIMUM, OnApplyMaximum)
	ON_COMMAND(ID_APPLY_FLATTEN, OnApplyFlatten)
	ON_COMMAND(ID_APPLY_POSTERIZE, OnApplyPosterize)
	ON_COMMAND(ID_OPTIMIZE_LAYERS, OnOptimizeLayers)
	ON_COMMAND(ID_TBRUSH_CONTINOUS_NOISE, OnTbrushContinousNoise)
	ON_COMMAND(ID_TBRUSH_FILTER, OnTbrushFilter)
	ON_COMMAND(ID_TBRUSH_FLATTEN, OnTbrushFlatten)
	ON_COMMAND(ID_TBRUSH_MAXIMUM, OnTbrushMaximum)
	ON_COMMAND(ID_TBRUSH_MINIMUM, OnTbrushMinimum)
	ON_COMMAND(ID_TBRUSH_POSTERIZE, OnTbrushPosterize)
	ON_COMMAND(ID_TERRAIN_PROPERTIES, OnTerrainProperties)
	//}}AFX_MSG_MAP
  ON_COMMAND_RANGE(ID_BUFFER01, ID_BUFFER10, OnKeyBuffer)
  ON_COMMAND_RANGE(ID_EDIT_BUFFER01, ID_EDIT_BUFFER10, OnKeyEditBuffer)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView construction/destruction

CWorldEditorView::CWorldEditorView()
{
  m_iaInputAction = IA_NONE;
  m_iaLastInputAction=IA_NONE;
  m_pbpoTranslationPlane = NULL;

  m_vpViewPrefs = theApp.m_vpViewPrefs[ 0];
  m_ptProjectionType = CSlaveViewer::PT_PERSPECTIVE;

  m_pdpDrawPort = NULL;
  m_pvpViewPort = NULL;

  m_fGridInMeters = 10.0f;

  m_pbpoRightClickedPolygon = NULL;
  m_penEntityHitOnContext = NULL;

  m_iDragVertice = -1;
  m_iDragEdge = -1;

  _pselbvxtSelectOnRender = NULL;
  _pselenSelectOnRender = NULL;
  m_bRequestVtxClickSelect = FALSE;
  m_bRequestVtxLassoSelect = FALSE;
  m_bRequestEntityLassoSelect = FALSE;
  m_strTest="";
}

CWorldEditorView::~CWorldEditorView()
{
}

BOOL CWorldEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

static void GetToolTipText(void *pView, char *pToolTipText)
{
  CWorldEditorView *pWorldEditorView = (CWorldEditorView *) pView;
  pWorldEditorView->GetToolTipText( pToolTipText);
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView drawing

void CWorldEditorView::RenderBackdropTexture(CDrawPort *pDP,
                                             FLOAT3D v0, FLOAT3D v1, FLOAT3D v2, FLOAT3D v3,
                                             CTextureObject &to)
{
	if( to.GetData() == NULL) return;
  CWorldEditorDoc* pDoc = GetDocument();
  FLOAT3D v0p, v1p, v2p, v3p;
  // create a slave viewer
  CSlaveViewer svViewer(GetChildFrame()->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid, pDP);
  // create a projection for slave viewer
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();

  // project current base vertice and convert y coordinate from mathemathical representation
  // into screen representation
  prProjection->ProjectCoordinate( v0, v0p); v0p(2) = pDP->GetHeight() - v0p(2);
  prProjection->ProjectCoordinate( v1, v1p); v1p(2) = pDP->GetHeight() - v1p(2);
  prProjection->ProjectCoordinate( v2, v2p); v2p(2) = pDP->GetHeight() - v2p(2);
  prProjection->ProjectCoordinate( v3, v3p); v3p(2) = pDP->GetHeight() - v3p(2);

  FLOATaabbox3D box;
  box |= v0p;
  box |= v1p;
  box |= v2p;
  box |= v3p;
  if( (box.Size()(1) > 2) && (box.Size()(2) > 2) )
  {
    // create rectangle for backdrop picture
    PIXaabbox2D rectPict;
    rectPict = PIXaabbox2D( PIX2D((SLONG)box.Min()(1), (SLONG)box.Min()(2)),
                            PIX2D((SLONG)box.Max()(1), (SLONG)box.Max()(2)) );
    pDP->PutTexture( &to, rectPict);
  }
}

BOOL _bCursorMoved=FALSE;
void CWorldEditorView::RenderView( CDrawPort *pDP)
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CChildFrame *pChild = GetChildFrame();
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;

  // create a slave viewer
  CSlaveViewer svViewer(GetChildFrame()->m_mvViewer, m_ptProjectionType,
    pDoc->m_plGrid, pDP);

  // copy view's world rendering preferences to global world rendering preferences
  _wrpWorldRenderPrefs = m_vpViewPrefs.m_wrpWorldRenderPrefs;
  _wrpWorldRenderPrefs.SetTextureLayerOn( theApp.m_bTexture1, 0);
  _wrpWorldRenderPrefs.SetTextureLayerOn( theApp.m_bTexture2, 1);
  _wrpWorldRenderPrefs.SetTextureLayerOn( theApp.m_bTexture3, 2);
  // set shadow type from child frame
  _wrpWorldRenderPrefs.SetShadowsType( GetChildFrame()->m_stShadowType);
  // copy view's model rendering preferences to global model rendering preferences
  _mrpModelRenderPrefs = m_vpViewPrefs.m_mrpModelRenderPrefs;
  // copy auto mip modeling flag from child frame
  _wrpWorldRenderPrefs.SetAutoMipBrushingOn( GetChildFrame()->m_bAutoMipBrushingOn);
  // and manual mip factor
  _wrpWorldRenderPrefs.SetManualMipBrushingFactor( GetChildFrame()->m_fManualMipBrushingFactor);
  // view visibility tweaking
  _wrpWorldRenderPrefs.SetVisTweaksOn(GetChildFrame()->m_bShowVisibilityTweaks);
  _wrpWorldRenderPrefs.DisableVisTweaks(GetChildFrame()->m_bDisableVisibilityTweaks);
  _pselbscVisTweaks=&pDoc->m_selSectorSelection;

  _wrpWorldRenderPrefs.SetSelectedEntityModel( theApp.m_pEntityMarkerModelObject);
  _wrpWorldRenderPrefs.SetSelectedPortalModel( theApp.m_pPortalMarkerModelObject);
  _wrpWorldRenderPrefs.SetEmptyBrushModel( theApp.m_pEmptyBrushModelObject);

  // if we are using automatic rendering range
  if( m_vpViewPrefs.m_bAutoRenderingRange)
  {
    // set rendering range of 1.5 x viewer's target distance
    FLOAT fTargetDistance = svViewer.GetTargetDistance();
    // we have to take second layer distance in considiration also
    FLOAT fMinimumRenderRange = fTargetDistance;

    // if second layer's world exists
    if( pDoc->m_pwoSecondLayer != NULL)
    {
      CPlacement3D plViewer = GetChildFrame()->m_mvViewer.GetViewerPlacement();
      // calculate viewer-layer distance
      FLOAT fViewerLayerDistance =
        (plViewer.pl_PositionVector - pDoc->m_plSecondLayer.pl_PositionVector).Length();
      // if current minimum rendering range is smaller than one that viewer-layer
      // distance would request
      if( fMinimumRenderRange < fViewerLayerDistance )
      {
        fMinimumRenderRange = fViewerLayerDistance;
      }
    }

    // don't allow minimum rendering range to drop below 1.5 m
    if( fMinimumRenderRange < 1.5f)
    {
      fMinimumRenderRange = 1.5f;
    }
    _wrpWorldRenderPrefs.SetMinimumRenderRange( fMinimumRenderRange * 1.5f);
  }
  // we are using fixed rendering range
  else
  {
    // set global rendering range from fixed one remembered in prefs
    _wrpWorldRenderPrefs.SetMinimumRenderRange( m_vpViewPrefs.m_fRenderingRange);
  }

  // set visibility of link lines
  _wrpWorldRenderPrefs.SetShowTargetsOn(pChild->m_bShowTargets);
  // set visibility of entity names
  _wrpWorldRenderPrefs.SetShowEntityNamesOn(pChild->m_bShowEntityNames);

  // find out if z buffer clearing should occure
  BOOL bClearZBuffer = FALSE;
  BOOL bClearScreen = TRUE;

  // get world and model polygon fill flags
  BOOL bModelPolygonFilling =
    (_mrpModelRenderPrefs.GetRenderType() & RT_TEXTURE_MASK) != RT_NO_POLYGON_FILL;
  // find out if any of isometric projection modes is currently on
  BOOL bPerspectiveOn = m_ptProjectionType == CSlaveViewer::PT_PERSPECTIVE;
  BOOL bPolygonFillOn = _wrpWorldRenderPrefs.GetPolygonsFillType() != CWorldRenderPrefs::FT_NONE;

  // if something requires clearing of z-buffer
  if( bPolygonFillOn ||
      pDoc->m_bOrientationIcons ||
      ((!bCSGOn && bPerspectiveOn) && GetChildFrame()->m_bSceneRenderingTime) ||
      m_vpViewPrefs.m_bMeasurementTape ||
      theApp.m_bMeasureModeOn || theApp.m_bCutModeOn)
  {
    bClearZBuffer = TRUE;
  }

  // if we will render models using polygon fill, we must clear z-buffer
  if( bModelPolygonFilling)
  {
    bClearZBuffer = TRUE;
  }
  // if rendering will fill whole screen along with z-buffer
  if( bPolygonFillOn && bPerspectiveOn)
  {
    bClearZBuffer = FALSE;
    bClearScreen = FALSE;
  }

  COLOR colBcgFill=m_vpViewPrefs.m_PaperColor;
  if(_wrpWorldRenderPrefs.IsBackgroundTextureOn())
  {
    bClearScreen = TRUE;
    colBcgFill=pDoc->m_woWorld.wo_colBackground;
  }

  // set selected entity model
  _wrpWorldRenderPrefs.SetSelectedEntityModel( theApp.m_pEntityMarkerModelObject);
  // set selected portal model
  _wrpWorldRenderPrefs.SetSelectedPortalModel( theApp.m_pPortalMarkerModelObject);
  // set empty brush model
  _wrpWorldRenderPrefs.SetEmptyBrushModel( theApp.m_pEmptyBrushModelObject);

  switch( pDoc->GetEditingMode())
  {
  case POLYGON_MODE:
    {
      _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_POLYGONS);
      break;
    }
  case SECTOR_MODE:
    {
      _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_SECTORS);
      break;
    }
  case ENTITY_MODE:
    {
      _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_ENTITIES);
      break;
    }
  case VERTEX_MODE:
    {
      _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_VERTICES);
      break;
    }
  case TERRAIN_MODE:
    {
      _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_NONE);
      break;
    }
  case CSG_MODE:
    {
      _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_NONE);
      break;
    }
  default: { FatalError("Unknown editing mode."); break;};
  }

  // if selection shouldn't be rendered and we are not in vertex mode (selection should always be rendered)
  if( !GetChildFrame()->m_bSelectionVisible && (pDoc->GetEditingMode()!= VERTEX_MODE) )
  {
    _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_NONE);
  }

  // create a projection for slave viewer
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  // set object placement
  prProjection->ObjectPlacementL() = pDoc->m_plGrid;
  if( !bPerspectiveOn && !_wrpWorldRenderPrefs.wrp_bApplyFarClipPlaneInIsometricProjection)
  {
    prProjection->FarClipDistanceL() = -1;
  }
  else
  {
    prProjection->FarClipDistanceL() = _wrpWorldRenderPrefs.wrp_fFarClipPlane;
  }

  // prepare the projection
  prProjection->Prepare();

  // if screen clearance is required
  if( bClearScreen)
  {
    try
    {
      m_toBcgPicture.SetData_t(CTString(m_vpViewPrefs.m_achrBcgPicture));
    }
    // if failed
    catch (const char *strError)
    {
      (void) strError;
    }

    if( m_toBcgPicture.GetData() != NULL)
    {
      // fill background with bcg picture
      PIXaabbox2D screenBox;
      screenBox = PIXaabbox2D( PIX2D(0,0), PIX2D(pDP->GetWidth(), pDP->GetHeight()) );
      pDP->PutTexture( &m_toBcgPicture, screenBox);
    }
    else
    {
      // clear background using current color form rendering preferences
      pDP->Fill(colBcgFill | CT_OPAQUE);
    }

    if( pDoc->m_woWorld.wo_strBackdropUp != "" && GetChildFrame()->m_bRenderViewPictures)
    {
      FLOAT3D v0 = FLOAT3D(pDoc->m_woWorld.wo_fUpCX-pDoc->m_woWorld.wo_fUpW/2.0f, 0.0f,
                           pDoc->m_woWorld.wo_fUpCZ-pDoc->m_woWorld.wo_fUpL/2.0f);
      FLOAT3D v1 = FLOAT3D(pDoc->m_woWorld.wo_fUpCX+pDoc->m_woWorld.wo_fUpW/2.0f, 0.0f,
                           pDoc->m_woWorld.wo_fUpCZ-pDoc->m_woWorld.wo_fUpL/2.0f);
      FLOAT3D v2 = FLOAT3D(pDoc->m_woWorld.wo_fUpCX+pDoc->m_woWorld.wo_fUpW/2.0f, 0.0f,
                           pDoc->m_woWorld.wo_fUpCZ+pDoc->m_woWorld.wo_fUpL/2.0f);
      FLOAT3D v3 = FLOAT3D(pDoc->m_woWorld.wo_fUpCX-pDoc->m_woWorld.wo_fUpW/2.0f, 0.0f,
                           pDoc->m_woWorld.wo_fUpCZ+pDoc->m_woWorld.wo_fUpL/2.0f);
      RenderBackdropTexture( pDP, v0, v1, v2, v3, pDoc->m_toBackdropUp);
    }
    if( pDoc->m_woWorld.wo_strBackdropFt != "" && GetChildFrame()->m_bRenderViewPictures)
    {
      FLOAT3D v0 = FLOAT3D(pDoc->m_woWorld.wo_fFtCX-pDoc->m_woWorld.wo_fFtW/2.0f,
                           pDoc->m_woWorld.wo_fFtCY-pDoc->m_woWorld.wo_fFtH/2.0f, 0.0f);
      FLOAT3D v1 = FLOAT3D(pDoc->m_woWorld.wo_fFtCX+pDoc->m_woWorld.wo_fFtW/2.0f,
                           pDoc->m_woWorld.wo_fFtCY-pDoc->m_woWorld.wo_fFtH/2.0f, 0.0f);
      FLOAT3D v2 = FLOAT3D(pDoc->m_woWorld.wo_fFtCX+pDoc->m_woWorld.wo_fFtW/2.0f,
                           pDoc->m_woWorld.wo_fFtCY+pDoc->m_woWorld.wo_fFtH/2.0f, 0.0f);
      FLOAT3D v3 = FLOAT3D(pDoc->m_woWorld.wo_fFtCX-pDoc->m_woWorld.wo_fFtW/2.0f,
                           pDoc->m_woWorld.wo_fFtCY+pDoc->m_woWorld.wo_fFtH/2.0f, 0.0f);
      RenderBackdropTexture( pDP, v0, v1, v2, v3, pDoc->m_toBackdropFt);
    }
    if( pDoc->m_woWorld.wo_strBackdropRt != "" && GetChildFrame()->m_bRenderViewPictures)
    {
      FLOAT3D v0 = FLOAT3D(0.0f, pDoc->m_woWorld.wo_fRtCY-pDoc->m_woWorld.wo_fRtH/2.0f,
                           pDoc->m_woWorld.wo_fRtCZ-pDoc->m_woWorld.wo_fRtL/2.0f);
      FLOAT3D v1 = FLOAT3D(0.0f, pDoc->m_woWorld.wo_fRtCY+pDoc->m_woWorld.wo_fRtH/2.0f,
                           pDoc->m_woWorld.wo_fRtCZ-pDoc->m_woWorld.wo_fRtL/2.0f);
      FLOAT3D v2 = FLOAT3D(0.0f, pDoc->m_woWorld.wo_fRtCY+pDoc->m_woWorld.wo_fRtH/2.0f,
                           pDoc->m_woWorld.wo_fRtCZ+pDoc->m_woWorld.wo_fRtL/2.0f);
      FLOAT3D v3 = FLOAT3D(0.0f, pDoc->m_woWorld.wo_fRtCY-pDoc->m_woWorld.wo_fRtH/2.0f,
                           pDoc->m_woWorld.wo_fRtCZ+pDoc->m_woWorld.wo_fRtL/2.0f);
      RenderBackdropTexture( pDP, v0, v1, v2, v3, pDoc->m_toBackdropRt);
    }
    // if object should me rendered as backdrop
    if( GetChildFrame()->m_bRenderViewPictures)
    {
      CObject3D &ob = pDoc->m_o3dBackdropObject;
      FOREACHINDYNAMICARRAY(ob.ob_aoscSectors, CObjectSector, itosc)
      {
        FOREACHINDYNAMICARRAY(itosc->osc_aoedEdges, CObjectEdge, itoe)
        {
          FLOAT3D vtx0 = DOUBLEtoFLOAT(*itoe->oed_Vertex0);
          FLOAT3D vtx1 = DOUBLEtoFLOAT(*itoe->oed_Vertex1);
          prProjection->PreClip( vtx0, vtx0);
          prProjection->PreClip( vtx1, vtx1);
          prProjection->ClipLine(vtx0, vtx1);
          // apply perspective
          FLOAT3D vtx0p, vtx1p;
          prProjection->PostClip( vtx0, vtx0p);
          prProjection->PostClip( vtx1, vtx1p);
          // draw one line of bounding box
          pDP->DrawLine( (PIX)vtx0p(1), (PIX)vtx0p(2), (PIX)vtx1p(1), (PIX)vtx1p(2),
                          C_dGRAY|CT_OPAQUE);
        }
      }
    }
  }

  // if z-buffer clearance is required
  if( bClearZBuffer)
  {
    // erase z-buffer
    pDP->FillZBuffer(ZBUF_BACK);
  }

  FLOAT3D fGridOrigin;
  // project grid origin
  prProjection->ProjectCoordinate( FLOAT3D( 0.0f, 0.0f, 0.0f), fGridOrigin);
  // convert y coordinate form mathematical form info screen space
  fGridOrigin(2) = pDP->GetHeight() - fGridOrigin(2);

  // calculate grid in pixels but there must be bigger than given minimal size
#define SMALLEST_ALLOWED_GRID_IN_PIXELS 30
  // get zoom factor
  FLOAT fZoom = svViewer.GetZoomFactor();
  // declare decadic grid in meters
  FLOAT afDecadicGridInMeters[] = {0.25f,0.50f,1.0f,2.5f,5.0f,10.0f,25.0f,50.0f,100.0f,
                                  250.0f,500.0f,1000.0f,10000.0f,100000.0f};
  FLOAT afBinaryGridInMeters[]  = {0.25f,0.50f,1.0f,2.0f,4.0f,8.0f,16.0f,32.0f,64.0f,128.0f,256.0f,
                                  512.0f,1024.0f,2048.0f,4096.0f,8192.0f,16384.0f,32768.0f,65536.0f};
  // select max grid value
  if( theApp.m_bDecadicGrid)
  {
    m_fGridInMeters = afDecadicGridInMeters[ sizeof( afDecadicGridInMeters)/sizeof(FLOAT)-1];
  }
  else
  {
    m_fGridInMeters = afBinaryGridInMeters[ sizeof( afBinaryGridInMeters)/sizeof(FLOAT)-1];
  }

  // ptr to curently active grid table
  FLOAT *pafGridInMeters;
  INDEX ctTableMembers;
  // select active table
  if( theApp.m_bDecadicGrid)
  {
    pafGridInMeters = afDecadicGridInMeters;
    ctTableMembers = sizeof( afDecadicGridInMeters)/sizeof(FLOAT);
  }
  else
  {
    pafGridInMeters = afBinaryGridInMeters;
    ctTableMembers = sizeof( afBinaryGridInMeters)/sizeof(FLOAT);
  }

  // for all members in precalculated grid in meters array
  for( INDEX i=0; i<ctTableMembers; i++)
  {
    // calculate grid steep
    m_fpixGridSteep = fZoom * pafGridInMeters[ i];
    // is it bigger than smallest allowed grid size in pixels
    if( m_fpixGridSteep >= SMALLEST_ALLOWED_GRID_IN_PIXELS)
    {
      m_fGridInMeters = pafGridInMeters[ i];
      break;
    }
  }
  // we have calculated wanted grid in pixels, value is stored in pixGridSteep
  // if grid on, draw it
  if( GetChildFrame()->m_bGridOn && !bPerspectiveOn && !bPolygonFillOn)
  {
    // get grid's origin projected point
    FLOAT fGridOriginX = fGridOrigin(1);
    FLOAT fGridOriginY = fGridOrigin(2);
    // calculate starting coordinates of left-up grid line
    FLOAT fGridX = fGridOriginX - (m_fpixGridSteep * (int)(fGridOriginX / m_fpixGridSteep));
    FLOAT fGridY = fGridOriginY - (m_fpixGridSteep * (int)(fGridOriginY / m_fpixGridSteep));
    // remember these start grid values so they can be used for measurement
    m_fGridX = fGridX;
    m_fGridY = fGridY;
    // style of grid line
    ULONG ulLineStyle;
    // loop all vertical grid lines
#define LINE_EPSILON 0.05f
    FOREVER
    {
      // does this grid lines touch origin point
      if( Abs(fGridX - fGridOriginX) < LINE_EPSILON )
      {
        // if so, draw full line
        ulLineStyle = _FULL_;
      }
      else
      {
        // does not, draw point line
        ulLineStyle = _POINT_;
      }
      // draw one vertical grid line
      pDP->DrawLine( (int) fGridX, 0, (int) fGridX, pDP->GetHeight()-1,
                     m_vpViewPrefs.m_GridColor|CT_OPAQUE, ulLineStyle);
      // move to next grid line
      fGridX += m_fpixGridSteep;
      // is coordinate passed draw port's width
      if( fGridX > pDP->GetWidth())
      {
        // if so, vertical grid is drawn, stop endless loop
        break;
      }
    }
    // loop all horizontal grid lines
    FOREVER
    {
      // does this grid lines touch origin point
      if( Abs(fGridY - fGridOriginY) < LINE_EPSILON )
      {
        // if so, draw full line
        ulLineStyle = _FULL_;
      }
      else
      {
        // does not, draw point line
        ulLineStyle = _POINT_;
      }
      // draw one vertical grid line
      pDP->DrawLine( 0, (int) fGridY, pDP->GetWidth()-1, (int) fGridY,
                     m_vpViewPrefs.m_GridColor|CT_OPAQUE, ulLineStyle);
      // move to next grid line
      fGridY += m_fpixGridSteep;
      // is coordinate passed draw port's height
      if( fGridY > pDP->GetHeight())
      {
        // if so, vertical grid is drawn, stop endless loop
        break;
      }
    }
  }

  // if we should draw measurement tape
  if( m_vpViewPrefs.m_bMeasurementTape && !bPerspectiveOn)
  {
    // define tape in meters for decadic grid type
    FLOAT afDecadicTapeInMeters[] =
    { 0.01f, 0.1f, 1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
    CTString astrDecadicTapeInMeters[] =
    { "1 mm", "1 cm", "1 m", "10 m", "100 m", "1 km", "10 km", "100 km"};
    // define tape in meters for binary grid type
    FLOAT afBinaryTapeInMeters[] =
    { 1.0f/64, 1.0f/8, 1.0f, 8.0f, 64.0f, 256.0f, 1024.0f, 4096.0f, 16484.0f, 65536.0f};
    CTString astrBinaryTapeInMeters[] =
    { "1/64 m", "1/8 m", "1 m", "8 m", "64 m", "256 m", "1024 m", "4096 m", "16384 m", "65536 m"};

    CTString *pastrTapeInMetersTable;
    FLOAT *pafTapeInMetersTable;
    INDEX ctTableMembers;

    // select active table
    if( theApp.m_bDecadicGrid)
    {
      pafTapeInMetersTable = afDecadicTapeInMeters;
      pastrTapeInMetersTable = astrDecadicTapeInMeters;
      ctTableMembers = sizeof( afDecadicTapeInMeters)/sizeof(FLOAT);
    }
    else
    {
      pafTapeInMetersTable = afBinaryTapeInMeters;
      pastrTapeInMetersTable = astrBinaryTapeInMeters;
      ctTableMembers = sizeof( afBinaryTapeInMeters)/sizeof(FLOAT);
    }
    // here we will calculate our tape lenght in meters
    FLOAT fTapeLenMeters;
    // here we will calculate our tape lenght in pixels
    PIX pixTapeLen;
    // here we will place appropriate text saying tape's lenght in meters
    CTString strTapeLen;
    // for all possible dimensions of measurement tape
    for( INDEX i=ctTableMembers-1; i>=0; i--)
    {
      // get tape len in meters
      fTapeLenMeters = pafTapeInMetersTable[ i];
      // get appropriate descriptive string
      strTapeLen = pastrTapeInMetersTable[ i];
      // calculate tape in pixels
      pixTapeLen = (PIX)(fZoom * fTapeLenMeters);
      // is it smaller than 75% of draw port's width
      if( pixTapeLen <= pDP->GetWidth()*3/4)
      {
        // it is, we found our tape dimension
        break;
      }
    }
    // calculate tape position on screen
    PIX x = (PIX) ( pDP->GetWidth() * 0.05f); // x starts at  5% of draw port's width
    PIX y = (PIX) ( pDP->GetHeight()* 0.95f); // y starts at 95% of draw port's height
    // draw tape's left end, little vertical line
    pDP->DrawLine( x, y-2, x, y+2, C_BLACK|CT_OPAQUE);
    // draw tape's main, horizontal little
    pDP->DrawLine( x, y, x+pixTapeLen, y, C_BLACK|CT_OPAQUE);
    // draw tape's right end, little vertical line
    pDP->DrawLine( x+pixTapeLen, y-2, x+pixTapeLen, y+2, C_BLACK|CT_OPAQUE);
    // type appropriate text, lenght in meters
    pDP->SetFont( theApp.m_pfntSystem);
    pDP->SetTextAspect( 1.0f);
    pDP->SetTextScaling( 1.0f);  
    pDP->PutText( strTapeLen, x, y-16);
  }

  // we are not rendering scene over already rendered scene (used for CSG layer)
  pDP->SetOverlappedRendering(FALSE); 

  // print grid size (or no grid)
  char strPaneText[ 128];
  sprintf( strPaneText, "Grid: %g m", m_fGridInMeters);
  pMainFrame->m_wndStatusBar.SetPaneText( GRID_PANE, CString(strPaneText), TRUE);

  // create renderer and render world
  CEntity *penOnlySelected = NULL;
  if( pDoc->m_selEntitySelection.Count() == 1)
  {
    pDoc->m_selEntitySelection.Lock();
    penOnlySelected = &pDoc->m_selEntitySelection[0];
    pDoc->m_selEntitySelection.Unlock();
  }

  // request vertex selecting from renderer
  if( m_bRequestVtxClickSelect)
  {
    _bSelectAlternative = m_bOnSelectVertexShiftDown;
    _pselbvxtSelectOnRender = &pDoc->m_selVertexSelection;
  }  
  if( m_bRequestVtxLassoSelect)
  {
    _bSelectAlternative = m_bOnSelectVertexAltDown;
    _pselbvxtSelectOnRender = &pDoc->m_selVertexSelection;
    _pavpixSelectLasso = &m_avpixLaso;
  }
  if( m_bRequestEntityLassoSelect)
  {
    _bSelectAlternative = m_bOnSelectEntityAltDown;
    _pselenSelectOnRender = &pDoc->m_selEntitySelection;
    _pavpixSelectLasso = &m_avpixLaso;
  }

  if(pDoc->GetEditingMode()==TERRAIN_MODE &&
     theApp.m_penLastTerrainHit==GetTerrainEntity() &&
     !_pInput->IsInputEnabled())
  {
    RenderAndApplyTerrainEditBrush(theApp.m_vLastTerrainHit);
  }
  
  if( GetChildFrame()->m_bViewFromEntity &&
      (penOnlySelected != NULL) &&
      !(penOnlySelected->GetFlags()&ENF_ANCHORED) &&
      bPerspectiveOn )
  {
    // create a projection for slave viewer
    CAnyProjection3D prProjection;
    svViewer.MakeProjection(prProjection);
    prProjection->ViewerPlacementL() = penOnlySelected->GetPlacement();
    prProjection->Prepare();

    ::RenderView(pDoc->m_woWorld, *penOnlySelected, prProjection, *pDP);
  }
  else
  {
    // create renderer and render world
    ::RenderView(pDoc->m_woWorld, *(CEntity*) NULL, prProjection, *pDP);
  }
  
  // don't allow further laso select tests
  if( m_bRequestVtxLassoSelect || m_bRequestEntityLassoSelect)
  {
    m_avpixLaso.Clear();
  }

  _pselbvxtSelectOnRender = NULL;
  _pselenSelectOnRender = NULL;
  m_bRequestVtxClickSelect = FALSE;
  m_bRequestVtxLassoSelect = FALSE;
  m_bRequestEntityLassoSelect = FALSE;
  _pavpixSelectLasso = NULL;

  INDEX ctLasoPts = m_avpixLaso.Count();
  if(((m_iaInputAction == IA_SELECT_LASSO_BRUSH_VERTEX) ||
      (m_iaInputAction == IA_SELECT_LASSO_ENTITY)) && (ctLasoPts>1) )
  {
    for( INDEX iKnot=0; iKnot<ctLasoPts-1; iKnot++)
    {
      PIX x0 = m_avpixLaso[iKnot](1);
      PIX x1 = m_avpixLaso[iKnot+1](1);
      PIX y0 = m_avpixLaso[iKnot](2);
      PIX y1 = m_avpixLaso[iKnot+1](2);
      pDP->DrawLine(x0, y0, x1, y1, m_vpViewPrefs.m_GridColor|CT_OPAQUE);
    }    
  }


  prProjection->DepthBufferNearL() = 0.0f;
  prProjection->DepthBufferFarL()  = 0.9f;
  prProjection->Prepare();

  BeginModelRenderingView(prProjection, pDP);

  // if we have entity mode active, at least one entity selected and edititng property of
  // edit range type, and perspective is not on, render entities using rendering range model
  CPropertyID *ppidProperty = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
  if( (pDoc->GetEditingMode() == ENTITY_MODE) &&
      (pDoc->m_selEntitySelection.Count() != 0) &&
      (ppidProperty != NULL) &&
      ( (ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE3D) ||
        (ppidProperty->pid_eptType == CEntityProperty::EPT_RANGE) ||
        (ppidProperty->pid_eptType == CEntityProperty::EPT_FLOATAABBOX3D && !bPerspectiveOn)) )
  {
    // for all selected entities
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      // obtain property ptr
      CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
      // if this entity is just selected, it may not have range property but that was not
      // noticed (it will be during first on idle), so skip it
      if( penpProperty==NULL)
      {
        continue;
      }
      // if we are in perspective and entity property is marked not to be visible in perspective view
      if( (penpProperty->ep_ulFlags & EPROPF_HIDEINPERSPECTIVE) && bPerspectiveOn)
      {
        continue;
      }
      // this model we will render
      CModelObject *pModelObject;
      // this render model structure will be rendered
      CRenderModel rmRenderModel;
      // if we are editing range property
      if( ppidProperty->pid_eptType == CEntityProperty::EPT_RANGE)
      {
        // get editing range
        FLOAT fRange = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT);
        // set collision box's stretch factor
        theApp.m_pRangeSphereModelObject->mo_Stretch = FLOAT3D( fRange, fRange, fRange);
        // set texture rendering mode and phong shading
        //_mrpModelRenderPrefs.SetRenderType( RT_TEXTURE|RT_SHADING_PHONG);
        // copy range model's placement from entity
        rmRenderModel.SetObjectPlacement(iten->GetPlacement());
        // set range sphere model for rendering
        pModelObject = theApp.m_pRangeSphereModelObject;
      }
      // if we are editing angle3D property
      else if( ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE3D)
      {
        // get editting bounding box
        ANGLE3D aAngle = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ANGLE3D);
        rmRenderModel.SetObjectPlacement(
          CPlacement3D(iten->GetPlacement().pl_PositionVector, aAngle));
        // set angle3d vector's stretch factor
        FLOATaabbox3D box;
        iten->GetSize(box);
        FLOAT fStretch = box.Size().Length();
        theApp.m_pAngle3DModelObject->mo_Stretch = FLOAT3D( fStretch, fStretch, fStretch);
        // set angle3d model for rendering
        pModelObject = theApp.m_pAngle3DModelObject;
      }
      else
      {
        // get editting bounding box
        FLOATaabbox3D bbox = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOATaabbox3D);
        // reset orientation (axes alligned bounding box !!!)
        rmRenderModel.SetObjectPlacement(
          CPlacement3D(iten->GetPlacement().pl_PositionVector + bbox.Min(),
          ANGLE3D( 0, 0, 0)));
        // set collision box's stretch factor
        theApp.m_pBoundingBoxModelObject->mo_Stretch = bbox.Size();
        // render black wire frame allways
        _mrpModelRenderPrefs.SetWire( TRUE);
        _mrpModelRenderPrefs.SetInkColor( C_BLACK);
        // set bounding box model for rendering
        pModelObject = theApp.m_pBoundingBoxModelObject;
      }
      // set light placement
      rmRenderModel.rm_vLightDirection = FLOAT3D(1.0f, -1.0f, 1.0f);
      // set color of light
      rmRenderModel.rm_colLight= C_WHITE;
      rmRenderModel.rm_colAmbient = C_GRAY;
      // render range sphere model
      pModelObject->SetupModelRendering(rmRenderModel);
      pModelObject->RenderModel(rmRenderModel);
    }
  }
  // if we should render volume box model
  if( pDoc->m_bBrowseEntitiesMode)
  {
    // this model we will render
    CModelObject *pModelObject;
    // this render model structure will be rendered
    CRenderModel rmRenderModel;
    // create bbox from requested volume
    FLOATaabbox3D bboxVolume( pDoc->m_vCreateBoxVertice0, pDoc->m_vCreateBoxVertice1);
    // position it inside box
    rmRenderModel.SetObjectPlacement(CPlacement3D(bboxVolume.Min(), ANGLE3D( 0, 0, 0)));
    // set collision box's stretch factor
    theApp.m_pBoundingBoxModelObject->mo_Stretch = bboxVolume.Size();
    // render black wire frame allways
    _mrpModelRenderPrefs.SetWire( TRUE);
    _mrpModelRenderPrefs.SetHiddenLines( TRUE);
    _mrpModelRenderPrefs.SetInkColor( C_BLACK);
    // set bounding box model for rendering
    pModelObject = theApp.m_pBoundingBoxModelObject;
    // set light placement
    rmRenderModel.rm_vLightDirection = FLOAT3D(1.0f, -1.0f, 1.0f);
    // set color of light
    rmRenderModel.rm_colLight= C_WHITE;
    rmRenderModel.rm_colAmbient = C_GRAY;
    // render bounding volume model
    pModelObject->SetupModelRendering(rmRenderModel);
    pModelObject->RenderModel(rmRenderModel);
  }
  EndModelRenderingView();

  // if we should draw orientation icon, do it now because latter we may
  // have problems due to automatic InitRenderer (in that case we would try
  // to render icons on second layer's DrawPort, last locked, but it is allready
  // destructed)
  if( pDoc->m_bOrientationIcons)
  {
#define XS 10
#define YS 16
#define DX 32
#define DY 32

    PIXaabbox2D boxScreen(PIX2D(XS, YS), PIX2D(XS+DX, YS+DY));

    // choose view's orientation icon index
    INDEX iIcon = 0;
    CTString strView="None";
    CTString strU="?";
    CTString strV="?";
    switch( m_ptProjectionType)
    {
      case CSlaveViewer::PT_PERSPECTIVE: {      iIcon = 0; strView="persp";   strU="";    strV="";    break;}
      case CSlaveViewer::PT_ISOMETRIC_FRONT: {  iIcon = 5; strView="front";   strU="x";   strV="y";   break;}
      case CSlaveViewer::PT_ISOMETRIC_RIGHT: {  iIcon = 3; strView="right";   strU="-z";  strV="y";   break;}
      case CSlaveViewer::PT_ISOMETRIC_TOP: {    iIcon = 1; strView="top";     strU="x";   strV="-z";  break;}
      case CSlaveViewer::PT_ISOMETRIC_BACK: {   iIcon = 6; strView="back";    strU="-x";  strV="y";   break;}
      case CSlaveViewer::PT_ISOMETRIC_LEFT: {   iIcon = 4; strView="left";    strU="z";   strV="y";   break;}
      case CSlaveViewer::PT_ISOMETRIC_BOTTOM: { iIcon = 2; strView="bottom";  strU="x";   strV="z";   break;}
    }

    MEXaabbox2D boxTexture(MEX2D(iIcon*DX, 0), MEX2D((iIcon+1)*DX, DY));

    // render icon
    CTextureObject toIcon;
    toIcon.SetData( theApp.m_pViewIconsTD);
    pDP->PutTexture(&toIcon, boxScreen, boxTexture);

    PIX2D pixCenter=PIX2D( pDP->GetWidth()*0.03f, pDP->GetHeight()*0.97f);
    DrawAxis( pixCenter, 40, C_BLUE|CT_OPAQUE, C_RED|CT_OPAQUE, strU, strV);

    pDP->SetFont( _pfdConsoleFont);
    pDP->SetTextAspect( 1.0f);
    pDP->SetTextScaling( 1.0f);  
    pDP->PutTextCXY( strView, XS+DX/2, YS/2, C_RED|CT_OPAQUE);
  }

  pDP->BlendScreen();

  // if second layer's world exists
  if( pDoc->m_pwoSecondLayer != NULL)
  {
    // prepare second layer's projection ...
    // get viewer placement from first layer
    CPlacement3D plVirtualViewer = prProjection->ViewerPlacementR();
    // transform it to the system of the second layer
    plVirtualViewer.AbsoluteToRelative(pDoc->m_plSecondLayer);
    // use it in projection for second layer
    CAnyProjection3D prProjectionSecondLayer;
    prProjectionSecondLayer = prProjection;
    prProjectionSecondLayer->ViewerPlacementL() = plVirtualViewer;
    prProjectionSecondLayer->Prepare();

    // if we will not use any kind of polygon filling
    if( !bPolygonFillOn)
    {
      // render scene directly into primary draw port
      ::RenderView( *pDoc->m_pwoSecondLayer, *(CEntity*) NULL,
                    prProjectionSecondLayer, *pDP);
    }
    // if filling, create drawport for second layer and create picture using CopyViaZBuffer
    else
    {
      // disable shadow rendering on world in second layer
      _wrpWorldRenderPrefs.SetShadowsType( CWorldRenderPrefs::SHT_NONE);

      // set that we will rendering scene over allready rendered scene
      pDP->SetOverlappedRendering(TRUE);
      // render second layer's scene over already rendered scene using filled z-buffer
      ::RenderView( *pDoc->m_pwoSecondLayer, *(CEntity*) NULL, prProjectionSecondLayer, *pDP);
    }

    // if we are in triangularisation primitive mode
    if( (pDoc->m_bPrimitiveMode) &&
        (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) )

    {
      theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Lock();
      CDynamicArray<CObjectVertex> &aVtx = theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors[0].osc_aovxVertices;
      theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Unlock();

      aVtx.Lock();
      // render all selected vertices as boxes
      for( INDEX iVtx=0; iVtx<aVtx.Count(); iVtx++)
      {
        if( aVtx[ iVtx].ovx_ulFlags & OVXF_SELECTED)
        {
          FLOAT3D vProjected;
          // project vertice
          prProjectionSecondLayer->ProjectCoordinate( DOUBLEtoFLOAT(aVtx[ iVtx]), vProjected);
          // convert y coordinate from mathemathical representation into screen one
          vProjected(2) = pDP->GetHeight() - vProjected(2);
          pDP->Fill( (SLONG)vProjected(1)-3, (SLONG)vProjected(2)-3, 6, 6, C_BLACK|CT_OPAQUE);
        }
      }
      aVtx.Unlock();
    }
  }
  // if cut mode on, this isn't perspective view, and cut line modification has been performed on this view
  if( theApp.m_bCutModeOn && !bPerspectiveOn && ( pDoc->m_pCutLineView == this))
  {
    // render cut line
    FLOAT3D v0 = pDoc->m_vCutLineStart;
    FLOAT3D v1 = pDoc->m_vCutLineEnd;
    FLOAT3D vp0;
    FLOAT3D vp1;
    // create a projection for current viewer
    CAnyProjection3D prProjection;
    svViewer.MakeProjection(prProjection);
    prProjection->Prepare();

    prProjection->ProjectCoordinate( v0, vp0);
    prProjection->ProjectCoordinate( v1, vp1);

    // convert y coordinates into view space
    vp0(2) = pDP->GetHeight()-vp0(2);
    vp1(2) = pDP->GetHeight()-vp1(2);

    PIX x1 = (PIX)vp0(1);
    PIX y1 = (PIX)vp0(2);
    PIX x2 = (PIX)vp1(1);
    PIX y2 = (PIX)vp1(2);

    // spread cut line (using parametric line representation) to edges of the draw port
    FLOAT dx = x2-x1;
    FLOAT dy = y2-y1;

    FLOAT fT[4];
    fT[0] = (0-x1)/dx;
    fT[1] = (pDP->GetWidth()-x1)/dx;
    fT[2] = (0-y1)/dy;
    fT[3] = (pDP->GetHeight()-y1)/dy;

    // find smallest T greater than 0 and biggest T less than 0
    FLOAT fMaxNegT = -9999999.0f;
    FLOAT fMinPosT =  9999999.0f;
    for( INDEX iT=0; iT<4; iT++)
    {
      if(fT[iT]>1)
      {
        if(fT[iT]<fMinPosT)
        {
          fMinPosT = fT[iT];
        }
      }
      else if(fT[iT]<0)
      {
        if(fT[iT]>fMaxNegT)
        {
          fMaxNegT = fT[iT];
        }
      }
    }

    // calculate border coordinates for found two T values
    PIX xb1 = (PIX) x1+dx*fMaxNegT;
    PIX xb2 = (PIX) x1+dx*fMinPosT;
    PIX yb1 = (PIX) y1+dy*fMaxNegT;
    PIX yb2 = (PIX) y1+dy*fMinPosT;

    // draw cut line
    COLOR colCutBorderLine = C_RED|CT_OPAQUE;
    pDP->DrawLine( xb1, yb1, xb2, yb2, colCutBorderLine, _TY124_);
    
    // draw control point lines
    COLOR colControl = C_BLACK|CT_OPAQUE;
    pDP->DrawLine( x1-3, y1-3, x1+3, y1+3, colControl);
    pDP->DrawLine( x1+3, y1-3, x1-3, y1+3, colControl);
    pDP->DrawLine( x2-3, y2-3, x2+3, y2+3, colControl);
    pDP->DrawLine( x2+3, y2-3, x2-3, y2+3, colControl);
  }
  // if measure mode on
  if( m_iaInputAction == IA_MEASURING)
  {
    // calculate placement for mouse down
    CPlacement3D plMouseDown;
    // obtain information about where mouse pointed into the world in the moment of mouse down
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouseDown);
    plMouseDown.pl_PositionVector = crRayHit.cr_vHit;
    pDoc->SnapToGrid( plMouseDown, m_fGridInMeters/GRID_DISCRETE_VALUES);

    // calculate placement for current mouse position
    CPlacement3D plCurrentMousePos;
    // obtain information about where mouse points into the world right now
    crRayHit = GetMouseHitInformation( m_ptMouse);
    plCurrentMousePos.pl_PositionVector = crRayHit.cr_vHit;
    // snap to grid curent mouse placement
    pDoc->SnapToGrid( plCurrentMousePos, m_fGridInMeters/GRID_DISCRETE_VALUES);

    // construct bbox containing both points
    FLOATaabbox3D bbox( plMouseDown.pl_PositionVector);
    bbox |= FLOATaabbox3D( plCurrentMousePos.pl_PositionVector);

    FLOAT3D vCenter = plMouseDown.pl_PositionVector;
    FLOAT3D vUp = plMouseDown.pl_PositionVector;
    FLOAT3D vRight = plMouseDown.pl_PositionVector;
    FLOAT3D vForward = plMouseDown.pl_PositionVector;

    vUp(2) = plCurrentMousePos.pl_PositionVector(2);
    vRight(1) = plCurrentMousePos.pl_PositionVector(1);
    vForward(3) = plCurrentMousePos.pl_PositionVector(3);

    // prepare font
    pDP->SetFont( theApp.m_pfntSystem);
    pDP->SetTextAspect( 1.0f);
    pDP->SetTextScaling( 1.0f);  

    CTString strText;
    strText.PrintF( "%g", bbox.Size()(1));
    DrawArrowAndTypeText( *prProjection, vCenter, vRight, C_BLUE|CT_OPAQUE, strText);
    strText.PrintF( "%g", bbox.Size()(2));
    DrawArrowAndTypeText( *prProjection, vCenter, vUp, C_BLUE|CT_OPAQUE, strText);
    strText.PrintF( "%g", bbox.Size()(3));
    DrawArrowAndTypeText( *prProjection, vCenter, vForward, C_BLUE|CT_OPAQUE, strText);
    
    // in orto projections, draw diagonal line
    if( m_ptProjectionType != CSlaveViewer::PT_PERSPECTIVE)
    {
      //strText.PrintF( "%g", bbox.Size().Length());
      DrawArrowAndTypeText( *prProjection, 
        plMouseDown.pl_PositionVector, 
        plCurrentMousePos.pl_PositionVector, C_lGRAY|CT_OPAQUE, "");
    }

    m_strMeasuring.PrintF( "Distance: %g", bbox.Size().Length());
  }

  // test string
  if( m_strTest!="")
  {
    pDP->SetFont( theApp.m_pfntSystem);
    pDP->SetTextScaling( 1.0f);  
    pDP->SetTextAspect( 1.0f);
    pDP->PutText( m_strTest, 32, 32);      
  }
}


static TIME tmSwapBuffers = 0;
extern BOOL _bInOnDraw = FALSE;
void CWorldEditorView::OnDraw(CDC* pDC)
{
  // skip if already drawing
	if( _bInOnDraw) return;
	_bInOnDraw = TRUE;

  // get some view variables
  CWorldEditorDoc* pDoc = GetDocument();
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  BOOL bPerspectiveOn = m_ptProjectionType == CSlaveViewer::PT_PERSPECTIVE;
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  CViewPort *pvpValidViewPort = GetViewPort();

  // if there is a valid drawport, and the drawport can be locked
  if( pdpValidDrawPort!=NULL && pdpValidDrawPort->Lock())
  {
    // variable to recive start time
    CTimerValue tvStart = _pTimer->GetHighPrecisionTimer();
    // render view
    RenderView( pdpValidDrawPort);
    // obtain stop time
    CTimerValue tvStop = _pTimer->GetHighPrecisionTimer();
    // number telling how many miliseconds passed
    TIME tmDelta = (tvStop-tvStart).GetSeconds() +tmSwapBuffers;

    // mark that view has been changed
    m_chViewChanged.MarkChanged();

    // if we can and should print any kind of frame rate descriptive message
    if( !bCSGOn && bPerspectiveOn && GetChildFrame()->m_bSceneRenderingTime)
    { 
      // prepare string about things that impact to currently rendered picture
      CTString strFPS, strReport;
      STAT_Report(strReport);
      STAT_Reset();
      // adjust and set font
      pdpValidDrawPort->SetFont( _pfdConsoleFont);
      pdpValidDrawPort->SetTextScaling( 1.0f);
      // put filter
      PIX pixDPHeight = pdpValidDrawPort->GetHeight();
      pdpValidDrawPort->Fill( 0,0, 150,pixDPHeight, C_BLACK|128, C_BLACK|0, C_BLACK|192, C_BLACK|0);
      // printout statistics
      strFPS.PrintF( " %3.0f FPS (%2.0f ms)\n----------------\n", 1.0f/tmDelta, tmDelta*1000.0f);
      pdpValidDrawPort->PutText( strFPS,    0,  5, C_lCYAN|CT_OPAQUE);
      pdpValidDrawPort->PutText( strReport, 4, 30, C_GREEN|CT_OPAQUE);
    }

    // unlock the drawport
    pdpValidDrawPort->Unlock();

    // swap if there is a valid viewport
    if( pvpValidViewPort!=NULL) {
      tvStart = _pTimer->GetHighPrecisionTimer();
      pvpValidViewPort->SwapBuffers();
      tvStop = _pTimer->GetHighPrecisionTimer();
      tmSwapBuffers = (tvStop-tvStart).GetSeconds();
    }
  }
  // all done
	_bInOnDraw = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView diagnostics

#ifdef _DEBUG
void CWorldEditorView::AssertValid() const
{
	CView::AssertValid();
}

void CWorldEditorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWorldEditorDoc* CWorldEditorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWorldEditorDoc)));
	return (CWorldEditorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorView message handlers

void CWorldEditorView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

  // allow file drop
  DragAcceptFiles();

  // at this time, m_hWnd is valid, so we do canvas initialization here
 	_pGfx->CreateWindowCanvas(m_hWnd, &m_pvpViewPort, &m_pdpDrawPort);

  // get active view
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();

  if( (pWorldEditorView != NULL) && (pWorldEditorView != this)
      && (theApp.m_Preferences.ap_CopyExistingWindowPrefs) )
  {
    m_IsWinBcgTexture = pWorldEditorView->m_IsWinBcgTexture;
    m_fnWinBcgTexture = pWorldEditorView->m_fnWinBcgTexture;
    m_SelectionColor = pWorldEditorView->m_SelectionColor;
    m_PaperColor = pWorldEditorView->m_PaperColor;
    m_InkColor = pWorldEditorView->m_InkColor;
    CChildFrame *pcfThis = GetChildFrame();
    CChildFrame *pcfOld = pWorldEditorView->GetChildFrame();
    if( pcfThis != pcfOld)
    {
      pcfThis->m_mvViewer = pcfOld->m_mvViewer;
    }
  }
  else
  {
    m_IsWinBcgTexture = FALSE;
    m_SelectionColor = theApp.m_Preferences.ap_DefaultSelectionColor;
    m_PaperColor = theApp.m_Preferences.ap_DefaultPaperColor;
    m_InkColor = theApp.m_Preferences.ap_DefaultInkColor;
  }

  if( theApp.m_Preferences.ap_SetDefaultColors)
  {
    m_SelectionColor = CLRF_CLR( theApp.m_Preferences.ap_DefaultSelectionColor);
    m_PaperColor = CLRF_CLR( theApp.m_Preferences.ap_DefaultPaperColor);
    m_InkColor = CLRF_CLR( theApp.m_Preferences.ap_DefaultInkColor);
  }
}

void CWorldEditorView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

  // if we are not in game mode and changing of display mode is not on
  if( !theApp.m_bChangeDisplayModeInProgress)
  { // and window canvas is valid
    if( m_pvpViewPort!=NULL)
    { // resize it
      m_pvpViewPort->Resize();
    }
  }
}

int CWorldEditorView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

  // register drop target
	m_DropTarget.Register(this);

	return 0;
}

/*
 * Get the pointer to the main frame object of this application
 */
CMainFrame *CWorldEditorView::GetMainFrame()
{
	// get the MDIChildFrame of this window
	CChildFrame *pfrChild = (CChildFrame *)this->GetParentFrame();
  ASSERT(pfrChild!=NULL);
  // get the MDIFrameWnd
  CMainFrame *pfrMain = (CMainFrame *)pfrChild->GetParentFrame();
  ASSERT(pfrMain!=NULL);

	return pfrMain;
}


void CWorldEditorView::OnKillFocus(CWnd* pNewWnd)
{
  m_iaInputAction = IA_NONE;
  // discard laso selection
  m_avpixLaso.Clear();
  CView::OnKillFocus(pNewWnd);
}


void CWorldEditorView::OnActivateView(BOOL bActivate,
                                      CView* pActivateView, CView* pDeactiveView)
{
  // if new view's document is not same as last activated document and if document is
  // beiing activated
  if( (pActivateView != NULL) &&
      (pActivateView->GetDocument() != theApp.GetLastActivatedDocument()) &&
      (bActivate) &&
      (pActivateView->GetDocument() != NULL) )
  {
    // mark that we are in crossing fase of switching documents if anybody searches for
    // document, he will get NULL
    theApp.m_bDocumentChangeOn = TRUE;
    // force OnIdle so info and combos will adjust to situation when doc = NULL
    theApp.OnIdle( 0);
    // finish crossing fase, continue "normal" behaviour
    theApp.m_bDocumentChangeOn = FALSE;
  }
  // mark that new document will be activated
  theApp.ActivateDocument(GetDocument());

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

BOOL MyChooseColor( COLORREF &clrNewColor, CWnd &wndOwner)
{
	COLORREF MyCustColors[ 16];
  CHOOSECOLOR ccInit;

  ASSERT( &wndOwner != NULL);
  MyCustColors[ 0] = CLRF_CLR(C_BLACK);
  MyCustColors[ 1] = CLRF_CLR(C_WHITE);
  MyCustColors[ 2] = CLRF_CLR(C_dGRAY);
  MyCustColors[ 3] = CLRF_CLR(C_GRAY);
  MyCustColors[ 4] = CLRF_CLR(C_lGRAY);
  MyCustColors[ 5] = CLRF_CLR(C_dRED);
  MyCustColors[ 6] = CLRF_CLR(C_dGREEN);
  MyCustColors[ 7] = CLRF_CLR(C_dBLUE);
  MyCustColors[ 8] = CLRF_CLR(C_dCYAN);
  MyCustColors[ 9] = CLRF_CLR(C_dMAGENTA);
  MyCustColors[10] = CLRF_CLR(C_dYELLOW);
  MyCustColors[11] = CLRF_CLR(C_dORANGE);
  MyCustColors[12] = CLRF_CLR(C_dBROWN);
  MyCustColors[13] = CLRF_CLR(C_dPINK);
  MyCustColors[14] = CLRF_CLR(C_lORANGE);
  MyCustColors[15] = CLRF_CLR(C_lBROWN);

  memset(&ccInit, 0, sizeof(CHOOSECOLOR));

  ccInit.lStructSize = sizeof(CHOOSECOLOR);
  ccInit.Flags = CC_RGBINIT | CC_FULLOPEN;
  ccInit.rgbResult = clrNewColor;
  ccInit.lpCustColors = &MyCustColors[ 0];
  ccInit.hwndOwner = wndOwner.m_hWnd;

  if( !ChooseColor( &ccInit))
    return FALSE;

  clrNewColor = ccInit.rgbResult;
  return TRUE;
}

// Returns eather perspective view's or manual mip factor
FLOAT CWorldEditorView::GetCurrentlyActiveMipFactor(void)
{
  // get document
	CWorldEditorDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  CEntity *penBrush = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  if((GetChildFrame()->m_bAutoMipBrushingOn) &&
     (penBrush != NULL) &&
     (pdpValidDrawPort != NULL) )
  {
    // get entity's brush
    CBrush3D *pbrBrush = penBrush->GetBrush();
    // create a slave viewer
    CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, CSlaveViewer::PT_PERSPECTIVE,
                           pDoc->m_plGrid, pdpValidDrawPort);
    // create a projection for slave viewer
    CAnyProjection3D prProjection;
    svViewer.MakeProjection(prProjection);
    prProjection->ObjectPlacementL() = penBrush->GetPlacement();
    // prepare the projection
    prProjection->Prepare();
    // return mip factor for entity in perspective projection
    return( -prProjection->pr_TranslationVector(3));
  }
  else
  {
    return (GetChildFrame()->m_fManualMipBrushingFactor);
  }
}

void CWorldEditorView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
  // find window under mouse pointer
	CWorldEditorDoc* pDoc = GetDocument();
  pDoc->SetStatusLineModeInfoMessage();

  POINT point;
  GetCursorPos(&point);
  HWND hwndUnderMouse = ::WindowFromPoint( point);

  // if not this one (use parent because gfx-API child is over the window)
  if (m_hWnd != ::GetParent(hwndUnderMouse)) {
    // ignore the key
    return;
  }

	CView::OnKeyDown( nChar, nRepCnt, nFlags);
  BOOL bShift = (GetKeyState(VK_SHIFT)  &0x8000) != 0;
  BOOL bCtrl  = (GetKeyState(VK_CONTROL)&0x8000) != 0;
  BOOL bAlt   = (GetKeyState(VK_MENU)   &0x8000) != 0;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;

  if(bLMB && !bAlt && (pDoc->GetEditingMode()==TERRAIN_MODE) && !bSpace)
  {
    if( bCtrl) m_iaInputAction=IA_TERRAIN_EDITING_CTRL_LMB;
    else       m_iaInputAction=IA_TERRAIN_EDITING_LMB;
  }

  // tool tips are on when you press I
  if( (GetKeyState('I')&0x80) && !(bShift|bCtrl|bAlt)) {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    // if info doesn't yet exist
    if( pMainFrame->m_pwndToolTip == NULL)
    { // force it
      theApp.m_cttToolTips.ManualOn( m_ptMouse.x, m_ptMouse.y, &::GetToolTipText, this);
    }
  }

  // call parent's key pressed function
  GetChildFrame()->KeyPressed( nChar, nRepCnt, nFlags);
}

void CWorldEditorView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CWorldEditorDoc* pDoc = GetDocument();
  pDoc->SetStatusLineModeInfoMessage();

  // if we are in mip mode for setting, set new brush switch factor
  if( m_iaInputAction == IA_MIP_SETTING)
  {
    SetMipBrushFactor();
  }
  BOOL bCtrl  = (GetKeyState(VK_CONTROL)&0x8000) != 0;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;

  if(bLMB && !bAlt && (pDoc->GetEditingMode()==TERRAIN_MODE) && !bSpace)
  {
    if( bCtrl) m_iaInputAction=IA_TERRAIN_EDITING_CTRL_LMB;
    else       m_iaInputAction=IA_TERRAIN_EDITING_LMB;
    _tvLastTerrainBrushingApplied=_pTimer->GetHighPrecisionTimer();
  }

  // shut down tool tips
  theApp.m_cttToolTips.ManualOff();

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CWorldEditorView::SetMipBrushFactor(void)
{
  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();

  CChildFrame *pChild = GetChildFrame();
  // set auto mip brushing mode on
  pChild->m_bAutoMipBrushingOn = TRUE;
  // get auto mip factor
  FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();
  pChild->m_bAutoMipBrushingOn = pChild->m_bLastAutoMipBrushingOn;
  // set auto mip brushing mode as it was before mip setting started
  ASSERT( m_pbmToSetMipSwitch != NULL);
  // set new mip switch factor
  m_pbmToSetMipSwitch->SetMipDistance( fCurrentMipFactor-0.01f);
  // switch back to auto mip brushing mode
  GetChildFrame()->m_bAutoMipBrushingOn = TRUE;

  // get document
	CWorldEditorDoc* pDoc = GetDocument();
  // document has changed
  pDoc->SetModifiedFlag();
  // update all views
  pDoc->UpdateAllViews( NULL);
  m_iaInputAction = IA_NONE;
}

void CWorldEditorView::StartMouseInput( CPoint point)
{
  CWorldEditorDoc* pDoc = GetDocument();
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;

  pDoc->UpdateAllViews(NULL);
  // reset offseted placement
  m_plMouseOffset.pl_PositionVector = FLOAT3D(0.0f, 0.0f, 0.0f);
  m_plMouseOffset.pl_OrientationAngle = ANGLE3D(0, 0, 0);
  // if there is CSG operation active
  if( bCSGOn)
  {
    // copy original layer's placement because we need to perform "continous moving"
    m_plMouseMove = pDoc->m_plSecondLayer;
  }
  // otherwise if we are in entity mode and there is at least 1 entity selected
  else if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    if(pDoc->m_selEntitySelection.Count()==0)
    {
      pDoc->m_aSelectedEntityPlacements.Clear();
    }
    else
    {
      FLOATaabbox3D box;
      CPlacement3D plEntityCenter;
      plEntityCenter = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        // accumulate positions
        box |= iten->GetPlacement().pl_PositionVector;
        //plEntityCenter.pl_OrientationAngle += iten->GetPlacement().pl_OrientationAngle;
      }}
      plEntityCenter.pl_PositionVector = box.Center();
      plEntityCenter.pl_PositionVector(2) = box.Min()(2);
      // calculate average rotation
      //plEntityCenter.pl_OrientationAngle /= (ANGLE)pDoc->m_selEntitySelection.Count();
      // remember
      pDoc->m_aSelectedEntityPlacements.Clear();
      pDoc->m_aSelectedEntityPlacements.New(pDoc->m_selEntitySelection.Count());
      INDEX ienCurrent = 0;
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        CPlacement3D plRelative = iten->GetPlacement();
        plRelative.AbsoluteToRelativeSmooth(plEntityCenter);
        pDoc->m_aSelectedEntityPlacements[ ienCurrent] = plRelative;
        ienCurrent++;
      }}

      // copy it's placement for continous moving
      m_plMouseMove = plEntityCenter;
    }
  }
  else if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    pDoc->m_avStartDragVertices.Clear();
    INDEX ctDragVertices = pDoc->m_selVertexSelection.Count();
    if( ctDragVertices != 0)
    {
      // remember coordinates of vertices for dragging
      pDoc->m_avStartDragVertices.New(ctDragVertices);
      INDEX iVtx = 0;
      {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
      {
        pDoc->m_avStartDragVertices[ iVtx] = FLOATtoDOUBLE( itbvx->bvx_vAbsolute);
        iVtx++;
      }}
    }
  }

  // copy mouse point to both point for calculating relative (from last mouse position) and
  // absolute (from mouse down position) distances
  m_ptMouse = point;
}

void CWorldEditorView::StopMouseInput(void)
{
  // update all views when current view stopped modifying
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->UpdateAllViews( NULL);
}

// obtain information about what was hitted with mouse
CCastRay CWorldEditorView::GetMouseHitInformation( CPoint point,
                                                   BOOL bHitPortals /* = FALSE */,
                                                   BOOL bHitModels /* = TRUE */,
                                                   BOOL bHitFields /* = FALSE */,
                                                   CEntity *penSourceEntity /* = NULL */,
                                                   BOOL bHitBrushes /* = TRUE */)
{
  CWorldEditorDoc* pDoc = GetDocument();

  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    // return dummy result
    return CCastRay( NULL, CPlacement3D(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0)) );
  }

  // obtain viewer placement
  CPlacement3D plViewer = GetChildFrame()->m_mvViewer.GetViewerPlacement();
  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);

  // now we will try to obtain where mouse poonts to in the base world
  // create a projection for current viewer
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();
  // make the ray from viewer point through the mouse point, in current projection
  CPlacement3D plRay;
  prProjection->RayThroughPoint(FLOAT3D((float)point.x,
    pdpValidDrawPort->GetHeight()-(float)point.y, 0.0f), plRay);
  // construct cast ray for base world
  CCastRay crBaseWorldCastResult( penSourceEntity, plRay);
  // set portal "transparency" or "hitability" for hit beam
  crBaseWorldCastResult.cr_bHitPortals = bHitPortals;
  // set hitability for models
  if( (m_vpViewPrefs.m_mrpModelRenderPrefs.GetRenderType() & RT_TEXTURE) && bHitModels)
  {
    crBaseWorldCastResult.cr_ttHitModels = CCastRay::TT_FULL;
  }
  else
  {
    crBaseWorldCastResult.cr_ttHitModels = CCastRay::TT_NONE;
  }
  crBaseWorldCastResult.cr_bHitFields = bHitFields;
  crBaseWorldCastResult.cr_bHitBrushes = bHitBrushes;
  // cast ray, go for hit data
  pDoc->m_woWorld.CastRay( crBaseWorldCastResult);

  // if second layer's world exist
  if( pDoc->m_pwoSecondLayer != NULL)
  {
    // prepare second layer's projection ...
    // get viewer placement from first layer
    CPlacement3D plVirtualViewer = prProjection->ViewerPlacementR();
    // transform it to the system of the second layer
    plVirtualViewer.AbsoluteToRelative(pDoc->m_plSecondLayer);
    // use it in projection for second layer
    CAnyProjection3D prProjectionSecondLayer;
    prProjectionSecondLayer = prProjection;
    prProjectionSecondLayer->ViewerPlacementL() = plVirtualViewer;
    prProjectionSecondLayer->Prepare();
    // calculate valid ray for second layer
    CPlacement3D plLayerRay;
    prProjectionSecondLayer->RayThroughPoint(FLOAT3D((float)point.x,
      pdpValidDrawPort->GetHeight()-(float)point.y, 0.0f), plLayerRay);
    // construct cast ray for second layer's world
    CCastRay crLayerWorldCastResult( penSourceEntity, plLayerRay);
    // set portal "transparency" or "hitability" for hit beam
    crLayerWorldCastResult.cr_bHitPortals = bHitPortals;
    // set hitability for models
    if( m_vpViewPrefs.m_mrpModelRenderPrefs.GetRenderType() & RT_TEXTURE)
    {
      crLayerWorldCastResult.cr_ttHitModels = CCastRay::TT_SIMPLE;
    }
    else
    {
      crLayerWorldCastResult.cr_ttHitModels = CCastRay::TT_NONE;
    }
    // cast ray, go for hit data
    pDoc->m_pwoSecondLayer->CastRay( crLayerWorldCastResult);
    // if any kind of entity on second layer was hitted
    if( crLayerWorldCastResult.cr_penHit != NULL)
    {
      // calculate distance beetween viewer and hited point in layer's world
      FLOAT fSecondLayerHitDistance =
        (crLayerWorldCastResult.cr_vHit - plVirtualViewer.pl_PositionVector).Length();
      // transform hited coordinate back to absolute space
      CPlacement3D plHitPlacement = plLayerRay;
      plHitPlacement.pl_PositionVector = crLayerWorldCastResult.cr_vHit;
      plHitPlacement.RelativeToAbsolute( pDoc->m_plSecondLayer);
      // and set it as hited point
      crLayerWorldCastResult.cr_vHit = plHitPlacement.pl_PositionVector;

      // if ray didn't hit anything on base world
      if( crBaseWorldCastResult.cr_penHit == NULL)
      {
        // valid result is one received from second layer ray hit
        return crLayerWorldCastResult;
      }
      // calculate distance beetween viewer and base world's hit point
      FLOAT fBaseWorldHitDistance =
        (crBaseWorldCastResult.cr_vHit - plViewer.pl_PositionVector).Length();

      // if base world's hit point is closer than second layer's hit point
      if( Abs(fBaseWorldHitDistance) < Abs(fSecondLayerHitDistance))
      {
        // base world's hit result is valid result
        return crBaseWorldCastResult;
      }
      // otherwise
      else
      {
        // second layer's world hit result is valid result
        return crLayerWorldCastResult;
      }
    }
  }

  if( crBaseWorldCastResult.cr_penHit == NULL)
  {
    // placement equals ray placement
    CPlacement3D plPlacementInFront = plRay;
    // calculate position somewhere in front of viewer
    plPlacementInFront.Translate_OwnSystem( FLOAT3D( 0.0f, 0.0f, -10.0f));
    // copy result coordinate into cast object
    crBaseWorldCastResult.cr_vHit = plPlacementInFront.pl_PositionVector;
  }
  return crBaseWorldCastResult;
}

FLOAT3D CWorldEditorView::GetMouseHitOnPlane( CPoint point, const FLOATplane3D &plPlane)
{
  CWorldEditorDoc* pDoc = GetDocument();

  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    // return dummy result
    return FLOAT3D(0.0f,0.0f,0.0f);
  }

  // obtain viewer placement
  CPlacement3D plViewer = GetChildFrame()->m_mvViewer.GetViewerPlacement();
  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);

  // now we will try to obtain where mouse poonts to in the base world
  // create a projection for current viewer
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();
  // make the ray from viewer point through the mouse point, in current projection
  CPlacement3D plRay;
  prProjection->RayThroughPoint(FLOAT3D((float)point.x,
    pdpValidDrawPort->GetHeight()-(float)point.y, 0.0f), plRay);

  // get two points on the line
  FLOAT3D v0 = plRay.pl_PositionVector;
  FLOAT3D v1;
  AnglesToDirectionVector(plRay.pl_OrientationAngle, v1);
  v1*=100.0f;  // to increase precision on large scales
  v1+=v0;
     
  // find intersection of line and plane
  FLOAT f0 = plPlane.PointDistance(v0);
  FLOAT f1 = plPlane.PointDistance(v1);
  FLOAT fFraction = f0/(f0-f1);
  return v0+(v1-v0)*fFraction;
}

void CWorldEditorView::MarkClosestVtxAndEdgeOnPrimitiveBase(CPoint point)
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL) return;
  CWorldEditorDoc* pDoc = GetDocument();
  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);
  // create a projection for current viewer
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  // prepare the projection
  prProjection->Prepare();
  // make the ray from viewer point through the mouse point, in current projection
  CPlacement3D plRay;
  prProjection->RayThroughPoint(FLOAT3D((float)point.x,
    pdpValidDrawPort->GetHeight()-(float)point.y, 0.0f), plRay);
  // transform ray to the system of the second layer
  plRay.AbsoluteToRelative(pDoc->m_plSecondLayer);
  m_vMouseDownSecondLayer = plRay.pl_PositionVector;
  // prepare second layer's projection ...
  // get viewer placement from first layer
  CPlacement3D plVirtualViewer = prProjection->ViewerPlacementR();
  // transform it to the system of the second layer
  plVirtualViewer.AbsoluteToRelative(pDoc->m_plSecondLayer);
  // use it in projection for second layer
  CAnyProjection3D prProjectionSecondLayer;
  prProjectionSecondLayer = prProjection;
  prProjectionSecondLayer->ViewerPlacementL() = plVirtualViewer;
  prProjectionSecondLayer->Prepare();
  // get number of vertices for base polygon
  INDEX vtxCt = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
  // set distance of closest vertice to a hudge distance
  FLOAT fMinVertexDistance = 999999.9f;
  m_iDragVertice = -1;
  FLOAT fMinEdgeDistance = 999999.9f;
  m_iDragEdge = -1;
  // now we will project base vertices onto screen and try
  // to find closest vertice to mouse for moving vertices on the base
  for( INDEX iVtx=0; iVtx<vtxCt; iVtx++)
  {
    INDEX iNext = (iVtx+1)%vtxCt;
    FLOAT3D vProjectedVtx1, vProjectedVtx2;
    // project vertices
    prProjectionSecondLayer->ProjectCoordinate(
      DOUBLEtoFLOAT(theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iVtx]), vProjectedVtx1);
    prProjectionSecondLayer->ProjectCoordinate(
      DOUBLEtoFLOAT(theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iNext]), vProjectedVtx2);
    // convert y coordinate from mathemathical representation into screen one
    vProjectedVtx1(2) = pdpValidDrawPort->GetHeight() - vProjectedVtx1(2);
    vProjectedVtx2(2) = pdpValidDrawPort->GetHeight() - vProjectedVtx2(2);
    vProjectedVtx1(3) = 0.0f;
    vProjectedVtx2(3) = 0.0f;
    // calculate distance to mouse
    FLOAT fDVtx1 = ( FLOAT3D((FLOAT)point.x, (FLOAT)point.y, 0.0f) - vProjectedVtx1).Length();
    FLOAT fDVtx2 = ( FLOAT3D((FLOAT)point.x, (FLOAT)point.y, 0.0f) - vProjectedVtx2).Length();
    FLOAT fEdgeLen = ( vProjectedVtx1 - vProjectedVtx2).Length();
    FLOAT s = (fDVtx1+fDVtx2+fEdgeLen)/2.0f;
    FLOAT fArea = FLOAT(sqrt( FLOATtoDOUBLE( s*(s-fDVtx1)*(s-fDVtx2)*(s-fEdgeLen))));
    FLOAT fDEdge = 2.0f*fArea/fEdgeLen;
    // if distance of edge is smaller than last remembered and both edges are smaller than
    // edges of triangle
    if( (fDEdge < fMinEdgeDistance) && (fEdgeLen > fDVtx1) && (fEdgeLen > fDVtx2) )
    {
      fMinEdgeDistance = fDEdge;
      m_iDragEdge = iVtx;
    }
    // if distance of first vertex is smaller than last remembered
    if( fDVtx1 < fMinVertexDistance)
    {
      fMinVertexDistance = fDVtx1;
      m_iDragVertice = iVtx;
    }
  }
  // when we found closest vertice, remember its current position
  m_vStartDragVertice = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ m_iDragVertice];
  if( m_iDragEdge != -1)
  {
    INDEX iEdgeEnd = (m_iDragEdge+1)%vtxCt;
    m_vStartDragEdge =
      (theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ m_iDragEdge]+
       theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iEdgeEnd])/2.0f;
  }
}

void CWorldEditorView::MarkClosestVtxOnPrimitive( BOOL bToggleSelection)
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL) return;
  CWorldEditorDoc* pDoc = GetDocument();
  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);
  // create a projection for current viewer
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  // prepare the projection
  prProjection->Prepare();

  // prepare second layer's projection ...
  // get viewer placement from first layer
  CPlacement3D plVirtualViewer = prProjection->ViewerPlacementR();
  // transform it to the system of the second layer
  plVirtualViewer.AbsoluteToRelative(pDoc->m_plSecondLayer);
  // use it in projection for second layer
  CAnyProjection3D prProjectionSecondLayer;
  prProjectionSecondLayer = prProjection;
  prProjectionSecondLayer->ViewerPlacementL() = plVirtualViewer;
  prProjectionSecondLayer->Prepare();

  theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Lock();
  CDynamicArray<CObjectVertex> &aVtx = theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors[0].osc_aovxVertices;
  theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Unlock();

  aVtx.Lock();
  // set distance of closest vertice to a hudge distance
  FLOAT fMinVertexDistance = 999999.9f;
  FLOAT fMinVertexDistanceZ = 999999.9f;
  INDEX iClosest = -1;
  BOOL bHasSelection = FALSE;
  // now we will project base vertices onto screen and try
  // to find closest vertice to mouse for moving vertices on the base
  for( INDEX iVtx=0; iVtx<aVtx.Count(); iVtx++)
  {
    if( (aVtx[ iVtx].ovx_ulFlags & OVXF_SELECTED) && !bHasSelection)
    {
      bHasSelection = TRUE;
      m_vStartDragO3DVertice = aVtx[ iVtx];
    }
    aVtx[ iVtx].ovx_ulFlags &= ~OVXF_CLOSEST;
    FLOAT3D vProjected;
    // project current base vertice
    prProjectionSecondLayer->ProjectCoordinate( DOUBLEtoFLOAT(aVtx[ iVtx]), vProjected);
    // convert y coordinate from mathemathical representation into screen one
    vProjected(2) = pdpValidDrawPort->GetHeight() - vProjected(2);
    // calculate distance to mouse point
    FLOAT fDX = (FLOAT)m_ptMouse.x-vProjected(1);
    FLOAT fDY = (FLOAT)m_ptMouse.y-vProjected(2);
    FLOAT fDistance = (FLOAT)sqrt(fDX*fDX+fDY*fDY);
    FLOAT fDistanceZ = -vProjected(3);
    // if this distance is smaller than last remembered
    if( (fDistance < fMinVertexDistance-0.5) ||
        ((fDistance<fMinVertexDistance+0.5) && fDistanceZ<fMinVertexDistanceZ) )
    {
      // set this one as smallest
      fMinVertexDistance  = fDistance;
      fMinVertexDistanceZ = fDistanceZ;
      // remember its index
      iClosest = iVtx;
    }
  }
  aVtx[ iClosest].ovx_ulFlags |= OVXF_CLOSEST;
  if( bToggleSelection)
  {
    aVtx[ iClosest].ovx_ulFlags ^= OVXF_SELECTED;
  }

  if( !bHasSelection)
  {
    m_vStartDragO3DVertice = aVtx[ iClosest];
  }
  aVtx.Unlock();
}

POINT CWorldEditorView::Get2DCoordinateFrom3D( FLOAT3D vPoint)
{
  CDrawPort *pDP = m_pdpDrawPort;

  CWorldEditorDoc* pDoc = GetDocument();
  CAnyProjection3D prProjection;
  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid, pDP);
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();

  FLOAT3D vResult;
  prProjection->ProjectCoordinate( vPoint, vResult);
  POINT ptResult;
  ptResult.x = (LONG) vResult(1);
  ptResult.y = pDP->GetHeight()-(LONG) vResult(2);
  return ptResult;
}

FLOAT3D CWorldEditorView::Get3DCoordinateFrom2D( POINT &pt)
{
  CDrawPort *pDP = m_pdpDrawPort;

  // convert coordinate
  PIX pixX = pt.x;
  PIX pixY = pt.y;

  CWorldEditorDoc* pDoc = GetDocument();
  CAnyProjection3D prProjection;
  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid, pDP);
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();
  // make the ray from viewer point through the mouse point, in current projection
  CPlacement3D plRay;
  prProjection->RayThroughPoint(FLOAT3D((float)pixX, pDP->GetHeight()-(float)pixY, 0.0f), plRay);

  // snap to grid entity's placement
  pDoc->SnapToGrid( plRay, m_fGridInMeters/GRID_DISCRETE_VALUES);

  // get viewer's direction vector
  FLOAT3D vDirection;
  AnglesToDirectionVector( plRay.pl_OrientationAngle, vDirection);
  FLOAT3D vResult;
  vResult = plRay.pl_PositionVector + vDirection;

  // snap result
  SnapVector( vResult);

  return vResult;
}

void CWorldEditorView::SnapVector(FLOAT3D &vToSnap)
{
  CWorldEditorDoc* pDoc = GetDocument();
  
  if( pDoc->m_bAutoSnap)
  {
    Snap(vToSnap(1), m_fGridInMeters/GRID_DISCRETE_VALUES);
    Snap(vToSnap(2), m_fGridInMeters/GRID_DISCRETE_VALUES);
    Snap(vToSnap(3), m_fGridInMeters/GRID_DISCRETE_VALUES);
  }
  else
  {
    Snap(vToSnap(1), SNAP_DOUBLE_CM);
    Snap(vToSnap(2), SNAP_DOUBLE_CM);
    Snap(vToSnap(3), SNAP_DOUBLE_CM);
  }
}

void CWorldEditorView::SnapVector(DOUBLE3D &vToSnap)
{
  CWorldEditorDoc* pDoc = GetDocument();
  
  FLOAT3D vTemp = DOUBLEtoFLOAT(vToSnap);

  if( pDoc->m_bAutoSnap)
  {
    Snap(vTemp(1), m_fGridInMeters/GRID_DISCRETE_VALUES);
    Snap(vTemp(2), m_fGridInMeters/GRID_DISCRETE_VALUES);
    Snap(vTemp(3), m_fGridInMeters/GRID_DISCRETE_VALUES);
  }
  else
  {
    Snap(vTemp(1), SNAP_DOUBLE_CM);
    Snap(vTemp(2), SNAP_DOUBLE_CM);
    Snap(vTemp(3), SNAP_DOUBLE_CM);
  }
  
  vToSnap = FLOATtoDOUBLE(vTemp);
}

FLOAT GetDistance( POINT &pt1, POINT &pt2)
{
  FLOAT2D vpt1, vpt2;
  
  vpt1(1) = pt1.x;
  vpt1(2) = pt1.y;

  vpt2(1) = pt2.x;
  vpt2(2) = pt2.y;

  FLOAT2D vDistance = vpt1-vpt2;
  return vDistance.Length();
}

void CWorldEditorView::RenderAndApplyTerrainEditBrush(FLOAT3D vHit)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain!=NULL)
  {
    CTString strBrushFile;
    INDEX iBrush=INDEX(theApp.m_fCurrentTerrainBrush);
    strBrushFile.PrintF("Textures\\Editor\\TerrainBrush%02d.tex", iBrush);
    try
    {
      CTextureData *ptdBrush=_pTextureStock->Obtain_t(strBrushFile);
      ptdBrush->Force(TEX_STATIC);

      FLOAT fStrengthRatio=theApp.m_fTerrainBrushPressure/1024.0f;
      FLOAT fUncompensatedStrength=(pow(1.0f+theApp.m_fTerrainBrushPressure/1024.0f*16.0f,2)-1.0f)/64.0f;
      COLOR colSelection=C_WHITE|CT_OPAQUE;
      ETerrainEdit teTool=TE_NONE;
      SelectionFill sf;
      // heightmap mode
      if( theApp.m_iTerrainEditMode==TEM_HEIGHTMAP)
      {
        colSelection=C_ORANGE|CT_OPAQUE;
        switch(INDEX(theApp.m_iTerrainBrushMode))
        {
          case TBM_PAINT          : teTool=TE_BRUSH_ALTITUDE_PAINT    ;       break;
          case TBM_SMOOTH         : teTool=TE_BRUSH_ALTITUDE_SMOOTH   ;       break;
          case TBM_FILTER         : teTool=TE_BRUSH_ALTITUDE_FILTER   ;       break;
          case TBM_MINIMUM        : teTool=TE_BRUSH_ALTITUDE_MINIMUM  ;       break;
          case TBM_MAXIMUM        : teTool=TE_BRUSH_ALTITUDE_MAXIMUM  ;       break;
          case TBM_FLATTEN        : teTool=TE_BRUSH_ALTITUDE_FLATTEN  ;       break;
          case TBM_POSTERIZE      : teTool=TE_BRUSH_ALTITUDE_POSTERIZE;       break;
          case TBM_RND_NOISE      : teTool=TE_BRUSH_ALTITUDE_RND_NOISE;       break;
          case TBM_CONTINOUS_NOISE: teTool=TE_BRUSH_ALTITUDE_CONTINOUS_NOISE; break;
          case TBM_ERASE          : teTool=TE_BRUSH_EDGE_ERASE        ;       break;
        }
      }
      // texture layer mode
      else
      {
        colSelection=C_GREEN|CT_OPAQUE;
        teTool=TE_BRUSH_LAYER_PAINT;

        CTerrainLayer *ptlLayer=GetLayer();
        if(ptlLayer==NULL) return;

        if( ptlLayer->tl_ltType==LT_TILE)
        {
          teTool=TE_TILE_PAINT;
        }
        else
        {
          switch(INDEX(theApp.m_iTerrainBrushMode))
          {
            case TBM_PAINT          : teTool=TE_BRUSH_LAYER_PAINT    ;       break;
            case TBM_SMOOTH         : teTool=TE_BRUSH_LAYER_SMOOTH   ;       break;
            case TBM_FILTER         : teTool=TE_BRUSH_LAYER_FILTER   ;       break;
            case TBM_RND_NOISE      : teTool=TE_BRUSH_LAYER_RND_NOISE;       break;
            case TBM_CONTINOUS_NOISE: teTool=TE_BRUSH_LAYER_CONTINOUS_NOISE; break;
            case TBM_ERASE          : teTool=TE_BRUSH_EDGE_ERASE     ;       break;
          }
        }
      }
      // no cursor if selection is hidden 
      if(!GetChildFrame()->m_bSelectionVisible) colSelection&=0xFFFFFF00;

      if( m_iaInputAction==IA_TERRAIN_EDITING_LMB || m_iaInputAction==IA_TERRAIN_EDITING_CTRL_LMB)
      {
        // if we pressed mouse down (terrain edit start)
        if(m_iaInputAction!=m_iaLastInputAction)
        {
          TerrainEditBegin();
        }

        CTimerValue tvNow=_pTimer->GetHighPrecisionTimer();
        // get difference to time when last mip brushing option has been used
        FLOAT fSecondsPassed=(tvNow-_tvLastTerrainBrushingApplied).GetSeconds();
        _tvLastTerrainBrushingApplied=_pTimer->GetHighPrecisionTimer();
        // apply time passed factor, so terrain editing wouldn't depend upon frame rate
        fSecondsPassed=Clamp(fSecondsPassed,0.0f,1.0f);
        FLOAT fCompensatedStrength=fUncompensatedStrength*fSecondsPassed/(1.0f/50.0f);

        if( m_iaInputAction==IA_TERRAIN_EDITING_CTRL_LMB) fCompensatedStrength=-fCompensatedStrength;
        // apply terrain editing
        EditTerrain(ptdBrush, vHit, fCompensatedStrength, teTool);
      }
      // if we stopped editing
      else if( m_iaLastInputAction==IA_TERRAIN_EDITING_LMB || m_iaLastInputAction==IA_TERRAIN_EDITING_CTRL_LMB)
      {
        TerrainEditEnd();
      }      
      m_iaLastInputAction=m_iaInputAction;

      // render terrain editing cursor
      Rect rect;
      Point pt=Calculate2dHitPoint(ptrTerrain, vHit);
      rect.rc_iLeft=pt.pt_iX-ptdBrush->GetPixWidth()/2;
      rect.rc_iRight=pt.pt_iX+(ptdBrush->GetPixWidth()-ptdBrush->GetPixWidth()/2);
      rect.rc_iTop=pt.pt_iY-ptdBrush->GetPixHeight()/2;
      rect.rc_iBottom=pt.pt_iY+(ptdBrush->GetPixHeight()-ptdBrush->GetPixHeight()/2);

      if(GetChildFrame()->m_bSelectionVisible)
      {
        sf=(SelectionFill) theApp.m_Preferences.ap_iTerrainSelectionVisible;
      }
      else
      {
        sf=(SelectionFill) theApp.m_Preferences.ap_iTerrainSelectionHidden;
      }
      if(sf!=3)
      {
        ShowSelection(ptrTerrain, rect, ptdBrush, colSelection, 0.25f+fStrengthRatio*0.75f, sf);
      }
    }
    catch (const char *strError)
    {
      (void) strError;
    }
  }
}

void CWorldEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
  m_iaInputAction = IA_NONE;

  CDrawPort *pdpValidDrawPort = GetDrawPort();
  if( pdpValidDrawPort == NULL) return;

  m_pbpoTranslationPlane = NULL;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

  // key statuses
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bRMB = nFlags & MK_RBUTTON;
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;

  BOOL bHitModels =  (pDoc->GetEditingMode() == ENTITY_MODE) || bSpace;
  BOOL bHitFields =  (pDoc->GetEditingMode() == ENTITY_MODE) || bSpace;
  BOOL bHitBrushes = (pDoc->GetEditingMode() != TERRAIN_MODE) || bSpace;

  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( point, FALSE, bHitModels, bHitFields, NULL, bHitBrushes);
  m_vHitOnMouseDown = crRayHit.cr_vHit;
  theApp.m_vLastTerrainHit=crRayHit.cr_vHit;
  m_ptMouseDown = point;
  m_VFPMouseDown = theApp.m_vfpCurrent;

  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);
  CAnyProjection3D prProjection;
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();

  // space+ctrl+lmb zoomes in 2x
  if( bSpace && bCtrl)
  {
    if( bRMB) return;
    // set new target
    GetChildFrame()->m_mvViewer.SetTargetPlacement( crRayHit.cr_vHit);
    // move mouse to center of screen
    CPoint ptCenter = CPoint( m_pdpDrawPort->GetWidth()/2, m_pdpDrawPort->GetHeight()/2);
    ClientToScreen( &ptCenter);
    SetCursorPos(ptCenter.x, ptCenter.y);
    OnZoomMore();
    pDoc->UpdateAllViews( NULL);
    m_ptMouseDown = ptCenter;
    return;
  }
  // space+lmb is used for viewer and CSG layer movement in floor plane,+rmb rotates
  else if( bSpace)
  {
    if( bRMB) m_iaInputAction = IA_ROTATING_VIEWER;
    else      m_iaInputAction = IA_MOVING_VIEWER_IN_FLOOR_PLANE;
    StartMouseInput( point);
  }
  // in measure mode, only viewer moving is enabled
  else if( theApp.m_bMeasureModeOn)
  {
    m_iaInputAction = IA_MEASURING;
  }
  // in cut mode, only viewer moving is enabled
  else if( theApp.m_bCutModeOn)
  {
    // project 3D control points
    POINT ptStart = Get2DCoordinateFrom3D( pDoc->m_vCutLineStart);
    POINT ptEnd = Get2DCoordinateFrom3D( pDoc->m_vCutLineEnd);
    
    // calculate distances
    FLOAT fDStart = GetDistance( ptStart, point);
    FLOAT fDEnd = GetDistance( ptEnd, point);
    FLOAT fAllowedDistance = 32.0f;
    
    // if we clicked near coontrol point
    if( fDStart < fAllowedDistance)
    {
      m_iaInputAction = IA_MOVING_CUT_LINE_START;
      pDoc->m_vControlLineDragStart = pDoc->m_vCutLineStart;
    }
    else if( fDEnd < fAllowedDistance)
    {
      m_iaInputAction = IA_MOVING_CUT_LINE_END;
      pDoc->m_vControlLineDragStart = pDoc->m_vCutLineEnd;
    }
    else
    {
      m_iaInputAction = IA_CUT_MODE;
      pDoc->m_pCutLineView = this;
      pDoc->m_vCutLineStart = Get3DCoordinateFrom2D( point);
      pDoc->m_vCutLineEnd = pDoc->m_vCutLineStart;
    }
    pDoc->UpdateAllViews( this);
  }
  // test for mouse operations used while primitive CSG is on
  else if( bCSGOn)
  {
    if( bCtrl && bAlt)
    {
      SetAsCsgTarget(crRayHit.cr_penHit);
    }
    if( bCtrl && !bShift)
    {
      if( bRMB) m_iaInputAction = IA_ROTATING_SECOND_LAYER;
      else      m_iaInputAction = IA_MOVING_SECOND_LAYER_IN_FLOOR_PLANE;
    }
    else if( pDoc->m_bPrimitiveMode)
    {
      // in triangularisation primitive mode
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE)
      {
        // select vertices on primitive
        MarkClosestVtxOnPrimitive( bShift);
        // and start dragging them
        if( !bShift)  m_iaInputAction = IA_DRAG_VERTEX_ON_PRIMITIVE;
      }
      // shift+lmb inserts vertex on primitive base
      else if( !bCtrl && bShift)
      {
        // find and remember vertex on primitive base for draging
        MarkClosestVtxAndEdgeOnPrimitiveBase( point);
        if(m_iDragEdge != -1)
        {
          pDoc->InsertPrimitiveVertex( m_iDragEdge, m_vMouseDownSecondLayer);
        }
      }
      // lmb is used for dragging vertices on base of primitive
      else
      {
        // ctrl+shift+lmb stretches primitive
        if( bCtrl && bShift)  m_iaInputAction = IA_STRETCHING_PRIMITIVE;
        else                  m_iaInputAction = IA_DRAG_VERTEX_ON_PRIMITIVE_BASE;
        // ------ Find closest vertice on base of primitive and remember its position
        // prepare second layer's projection ...
        // get viewer placement from first layer
        CPlacement3D plVirtualViewer = prProjection->ViewerPlacementR();
        // transform it to the system of the second layer
        plVirtualViewer.AbsoluteToRelative(pDoc->m_plSecondLayer);
        // use it in projection for second layer
        CAnyProjection3D prProjectionSecondLayer;
        prProjectionSecondLayer = prProjection;
        prProjectionSecondLayer->ViewerPlacementL() = plVirtualViewer;
        prProjectionSecondLayer->Prepare();

        // find and remember vertex on primitive base for draging
        MarkClosestVtxAndEdgeOnPrimitiveBase( point);

        m_iSizeControlVertice = -1;
        FLOAT fMinDistance = 9999999.9f;
        // prepare array with vertices used as control points for sizing box arround primitive
        FLOAT3D avtxSizeControlVertices[ 14];

        // table of min x, max x, miny, ...
        FLOAT afCoordinates[ 6];
        afCoordinates[0] = theApp.m_vfpCurrent.vfp_fXMin;
        afCoordinates[1] = theApp.m_vfpCurrent.vfp_fXMax;
        afCoordinates[2] = theApp.m_vfpCurrent.vfp_fYMin;
        afCoordinates[3] = theApp.m_vfpCurrent.vfp_fYMax;
        afCoordinates[4] = theApp.m_vfpCurrent.vfp_fZMin;
        afCoordinates[5] = theApp.m_vfpCurrent.vfp_fZMax;
        // create control points using table with indices into afCoordinates table
        for( INDEX iVtx=0;iVtx<14;iVtx++)
        {
          avtxSizeControlVertices[iVtx](1) =
            (afCoordinates[aiForCreationOfSizingVertices[iVtx][0]]+
             afCoordinates[aiForCreationOfSizingVertices[iVtx][1]])/2;
          avtxSizeControlVertices[iVtx](2) =
            (afCoordinates[aiForCreationOfSizingVertices[iVtx][2]]+
             afCoordinates[aiForCreationOfSizingVertices[iVtx][3]])/2;
          avtxSizeControlVertices[iVtx](3) =
            (afCoordinates[aiForCreationOfSizingVertices[iVtx][4]]+
             afCoordinates[aiForCreationOfSizingVertices[iVtx][5]])/2;
        }

        // now we will project size control vertices and try to find closest one to mouse
        for( INDEX iControlVtx=0; iControlVtx<14; iControlVtx++)
        {
          FLOAT3D vProjectedCoordinate;
          // project current control vertex
          prProjectionSecondLayer->ProjectCoordinate(
            avtxSizeControlVertices[iControlVtx], vProjectedCoordinate);
          // convert y coordinate from mathemathical representation into screen one
          vProjectedCoordinate(2) = pdpValidDrawPort->GetHeight() - vProjectedCoordinate(2);
          FLOAT fDX = vProjectedCoordinate(1)-point.x;
          FLOAT fDY = vProjectedCoordinate(2)-point.y;
          // calculate distance to mouse point
          FLOAT fDistance = fDX*fDX+fDY*fDY;
          // if this distance is smaller than last remembered
          if( fDistance < fMinDistance)
          {
            // set this one as smallest
            fMinDistance = fDistance;
            // and remember its indice as souch
            m_iSizeControlVertice = iControlVtx;
          }
        }
      }
    }
    // reset relative placement (m_plMouseOffset)
    StartMouseInput( point);
  }
  // if only Alt key is pressed, we are not in vertex nor entity mode, we want to save thumbnail
  else if( bAlt && !bCtrl && !bShift && !bSpace &&
    (pDoc->GetEditingMode() != VERTEX_MODE) &&
    (pDoc->GetEditingMode() != ENTITY_MODE))
  {
    // if document is modified
    if( pDoc->IsModified() )
    {
      // report error
      AfxMessageBox(L"You must save your document before you can perform drag.");
    }
    else
    {
      CTFileName fnDocument = CTString(CStringA(pDoc->GetPathName()));
      // save the thumbnail
      pDoc->SaveThumbnail();
      // try to
      try
      {
        // remove application path
        fnDocument.RemoveApplicationPath_t();
        // create drag and drop object
        HGLOBAL hglobal = CreateHDrop( fnDocument);
        m_DataSource.CacheGlobalData( CF_HDROP, hglobal);
        m_DataSource.DoDragDrop( DROPEFFECT_COPY);
      }
      // if failed
      catch (const char *strError)
      {
        // report error
        AfxMessageBox(CString(strError));
      }
    }
  }
  // else if we are in entity mode
  else if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    // if ctrl pressed, we want to move or rotate entity selection
    if( bCtrl && !bShift && !bAlt)
    {
      if( bRMB) m_iaInputAction = IA_ROTATING_ENTITY_SELECTION;
      else      m_iaInputAction = IA_MOVING_ENTITY_SELECTION_IN_FLOOR_PLANE;
      StartMouseInput( point);
    }
    // if we are in browsing entities mode (or select by volume)
    else if( (pDoc->m_bBrowseEntitiesMode) && 
             (m_ptProjectionType != CSlaveViewer::PT_PERSPECTIVE) )
    {
      m_iaInputAction = IA_SIZING_SELECT_BY_VOLUME_BOX;
      // ------ Find closest vertice of volume box
      // create a slave viewer
      CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                             pDoc->m_plGrid, pdpValidDrawPort);
      // create a projection for current viewer
      CAnyProjection3D prProjection;
      svViewer.MakeProjection(prProjection);
      // prepare the projection
      prProjection->Prepare();
      // set distance of closest vertice to a hudge distance
      FLOAT fMinVertexDistance = 999999.9f;
      pDoc->m_iVolumeBoxDragVertice = 0;
      // now we will project volume box's vertices onto screen and find closest one to mouse
      for( INDEX iVtx=0; iVtx<8; iVtx++)
      {
        FLOAT3D vProjectedCoordinate;
        // project current base vertice
        prProjection->ProjectCoordinate( pDoc->m_avVolumeBoxVertice[iVtx], vProjectedCoordinate);
        // convert y coordinate from mathemathical representation into screen one
        vProjectedCoordinate(2) = pdpValidDrawPort->GetHeight() - vProjectedCoordinate(2);
        // reset on screen "z" coordinate
        vProjectedCoordinate(3) = 0.0f;
        // calculate distance to mouse point
        FLOAT fDistance =
          ( FLOAT3D((FLOAT)point.x, (FLOAT)point.y, 0.0f) - vProjectedCoordinate ).Length();
        // if this distance is smaller than last remembered
        if( fDistance < fMinVertexDistance)
        {
          // set this one as smallest
          fMinVertexDistance = fDistance;
          // and remember its indice as souch
          pDoc->m_iVolumeBoxDragVertice = iVtx;
        }
      }
      // when we found closest vertice, remember its current position
      pDoc->m_vVolumeBoxStartDragVertice = pDoc->m_avVolumeBoxVertice[ pDoc->m_iVolumeBoxDragVertice];
      // reset relative placement (m_plMouseOffset)
      StartMouseInput( point);
    }
    // ctrl+shift+lmb is used to edit range and angle3d properties
    else if( bCtrl && bShift)
    {
      if( pDoc->m_selEntitySelection.Count() != 0)
      {
        // get selected property
        CPropertyID *ppid = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
        // if property is of range or angle3d type
        if( (ppid != NULL) && (ppid->pid_eptType == CEntityProperty::EPT_RANGE) )
        {
          m_iaInputAction = IA_CHANGING_RANGE_PROPERTY;
        }
        if( (ppid != NULL) && (ppid->pid_eptType == CEntityProperty::EPT_ANGLE3D) )
        {
          m_iaInputAction = IA_CHANGING_ANGLE3D_PROPERTY;
        }
      }
    }
    // Alt+Ctrl+LMB requests that clicked entity becomes target to selected entity
    else if( bAlt && bCtrl && (crRayHit.cr_penHit != NULL))
    {
      if( pDoc->m_selEntitySelection.Count() == 0)
      {
        SetAsCsgTarget(crRayHit.cr_penHit);
      }
      else
      {
        // get selected property
        CPropertyID *ppid = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
        if( (ppid == NULL) ||
           !((ppid->pid_eptType == CEntityProperty::EPT_ENTITYPTR) ||
             (ppid->pid_eptType == CEntityProperty::EPT_PARENT)) ) return;

        BOOL bParentProperty = ppid->pid_eptType == CEntityProperty::EPT_PARENT;
        // for each of the selected entities
        FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
        {
          if( bParentProperty)
          {
            iten->SetParent( crRayHit.cr_penHit);
          }
          else if (crRayHit.cr_penHit->IsTargetable())
          {
            // obtain entity class ptr
            CDLLEntityClass *pdecDLLClass = iten->GetClass()->ec_pdecDLLClass;
            // for all classes in hierarchy of this entity
            for(;
                pdecDLLClass!=NULL;
                pdecDLLClass = pdecDLLClass->dec_pdecBase) {
              // for all properties
              for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) {
                CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
                if( (ppid->pid_strName == epProperty.ep_strName) &&
                    (ppid->pid_eptType == epProperty.ep_eptType) )
                {
                  // discard old entity settings
                  iten->End();
                  // set clicked entity as one that selected entity points to
                  ENTITYPROPERTY( &*iten, epProperty.ep_slOffset, CEntityPointer) = crRayHit.cr_penHit;
                  // apply new entity settings
                  iten->Initialize();
                }
              }
            }
          }
        }
        // update edit range control (by updating parent dialog)
        pMainFrame->m_PropertyComboBar.UpdateData( FALSE);
        // mark that selections have been changed
        pDoc->m_chSelections.MarkChanged();
      }
    }
    // we want to select entities
    else
    {
      m_iaInputAction = IA_SELECTING_ENTITIES;
      m_bOnSelectEntityShiftDown = bShift;
      m_bOnSelectEntityAltDown = bAlt;
    }
  }
  // else if we are in sector mode
  else if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    m_iaInputAction = IA_SELECTING_SECTORS;
    ToggleHittedSector( crRayHit);
  }
  // if we are in polygon mode
  else if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // Alt+Ctrl+LMB requests that clicked polygon sizes its texture
    if( bAlt && bCtrl && !bShift)
    {
      MultiplyMappingOnPolygon( 0.5f);
    }
    else if( bAlt && bCtrl && bShift)
    {
      OnRemainSelectedByOrientation(FALSE);
    }
    if( bAlt && bShift)
    {
      OnRemainSelectedByOrientation(TRUE);
    }
    // if we hit polygon
    else if( (crRayHit.cr_penHit != NULL) &&
             (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      // if we want to change polygon mapping
      if( bCtrl && !bShift)
      {
        if( bRMB) m_iaInputAction = IA_ROTATING_POLYGON_MAPPING;
        else      m_iaInputAction = IA_MOVING_POLYGON_MAPPING;
        StartMouseInput( point);
        // remember hitted point
        m_f3dRotationOrigin = crRayHit.cr_vHit;
        m_plMouseMove.pl_PositionVector = crRayHit.cr_vHit;
        m_plMouseMove.pl_OrientationAngle = ANGLE3D( 0, 0, 0);
        m_pbpoTranslationPlane = crRayHit.cr_pbpoBrushPolygon;
        m_plTranslationPlane = crRayHit.cr_pbpoBrushPolygon->bpo_pbplPlane->bpl_plAbsolute;
      }
      // select or deselect hitted polygon
      else if( !bCtrl)
      {
        m_iaInputAction = IA_SELECTING_POLYGONS;
        ToggleHittedPolygon( crRayHit);
      }
    }
  }
  // if we are in vertex mode
  else if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    // Ctrl+Alt is used to add vertex
    if( bCtrl && bAlt)
    {
      CBrushPolygon *pbpo = crRayHit.cr_pbpoBrushPolygon;
      if( (crRayHit.cr_penHit != NULL) && (pbpo != NULL) )
      {
        pDoc->m_selPolygonSelection.Clear();
        pDoc->m_selVertexSelection.Clear();
        pbpo->bpo_pbscSector->TriangularizePolygon( pbpo);
        pDoc->m_chSelections.MarkChanged();
        // cast ray again to hit newly created triangle
        CCastRay crRayHit = GetMouseHitInformation( point, FALSE, bHitModels, bHitFields);
        CBrushPolygon *pbpo = crRayHit.cr_pbpoBrushPolygon;
        if( (crRayHit.cr_penHit != NULL) && (pbpo != NULL) )
        {
          pDoc->m_selPolygonSelection.Select( *pbpo);
          pbpo->bpo_pbscSector->InsertVertexIntoTriangle( pDoc->m_selPolygonSelection, crRayHit.cr_vHit);
        }
      }
    }
    // Ctrl is used for rotating or moving vertices
    else if( bCtrl)
    {
      if( bShift)
      {
        m_iaInputAction = IA_STRETCH_BRUSH_VERTEX;
      }
      else if( bRMB)
      {
        m_iaInputAction = IA_ROTATE_BRUSH_VERTEX;
      }
      else
      {
        m_iaInputAction = IA_DRAG_BRUSH_VERTEX_IN_FLOOR_PLANE;
      }
      // clear polygon selection because some of the old polygons could disappear
      pDoc->m_selPolygonSelection.Clear();
      pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);
      StartMouseInput( point);
    }
    else
    {
      m_avpixLaso.Clear();
      m_iaInputAction = IA_SELECT_SINGLE_BRUSH_VERTEX;
      _vpixSelectNearPoint(1) = point.x;
      _vpixSelectNearPoint(2) = point.y;
      m_bOnSelectVertexShiftDown = bShift;
      m_bOnSelectVertexAltDown = bAlt;
    }
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    if(crRayHit.cr_penHit!=NULL && crRayHit.cr_penHit==GetTerrainEntity())
    {
      _bCursorMoved=FALSE;
      if( bAlt && bCtrl)
      {
        OnPickLayer();
      }
      else if( !bAlt && bCtrl)
      {
        m_iaInputAction = IA_TERRAIN_EDITING_CTRL_LMB;
        _tvLastTerrainBrushingApplied=_pTimer->GetHighPrecisionTimer();
      }
      else if( !bAlt)
      {
        m_iaInputAction = IA_TERRAIN_EDITING_LMB;
        _tvLastTerrainBrushingApplied=_pTimer->GetHighPrecisionTimer();
      }
    }
    else
    {
      // if terrain entity hit
      if(crRayHit.cr_penHit!=NULL && crRayHit.cr_penHit->GetRenderType()==CEntity::RT_TERRAIN)
      {
        pDoc->m_ptrSelectedTerrain=crRayHit.cr_penHit->GetTerrain();
        theApp.m_ctTerrainPage.MarkChanged();
      }
    }
  }

  CView::OnLButtonDown(nFlags, point);
}

void SelectLayerCommand(INDEX iSelectedLayer)
{
  SelectLayer(iSelectedLayer);
  theApp.m_ctTerrainPageCanvas.MarkChanged();

  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  if(pDoc!=NULL)
  {
    pDoc->m_chSelections.MarkChanged();
  }
}

void CWorldEditorView::InvokeSelectLayerCombo(void)
{
  CPoint pt=m_ptMouse;
  CCastRay crRayHit=GetMouseHitInformation( pt);
  if( (crRayHit.cr_penHit==NULL) || (crRayHit.cr_penHit->GetRenderType()!=CEntity::RT_TERRAIN) ) return;

  CCustomComboWnd *pCombo=new CCustomComboWnd;
  CTerrain *ptrTerrain=crRayHit.cr_penHit->GetTerrain();
  if(ptrTerrain==NULL) return;
  INDEX ctLayers=ptrTerrain->tr_atlLayers.Count();
  for(INDEX iLayer=ctLayers-1; iLayer>=0; iLayer--)
  {
    CTString strLayerName;
    UBYTE ubPower=GetValueFromMask(ptrTerrain, iLayer, crRayHit.cr_vHit);
    strLayerName.PrintF("Layer %d (%d%%)", iLayer+1, INDEX(ubPower/255.0f*100.0f));
    if(ubPower>0)
    {
      INDEX iAddedAs=pCombo->InsertItem( strLayerName);
      pCombo->SetItemValue( iAddedAs, iLayer);
    }
    if(ubPower==255) break;
  }
  GetCursorPos( &pt);
  pCombo->Initialize(NULL, SelectLayerCommand, pt.x-8, pt.y-8, TRUE);
}

void CWorldEditorView::OnRButtonDown(UINT nFlags, CPoint point)
{
  if( m_iaInputAction != IA_MIP_SETTING)  m_iaInputAction = IA_NONE;

	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bLMB = nFlags & MK_LBUTTON;
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  BOOL bCaps = (GetKeyState( VK_CAPITAL)&0x0001) != 0;

  BOOL bHitModels = (pDoc->GetEditingMode() == ENTITY_MODE) || bSpace;
  BOOL bHitFields = (pDoc->GetEditingMode() == ENTITY_MODE) || bSpace;

  StartMouseInput( point);
  // remember point in the view
  m_ptMouseDown = point;
  // remember values for primitive
  m_VFPMouseDown = theApp.m_vfpCurrent;

  // if nothing of below is activated, call popup menu
  if( !bShift && !bAlt && !bCtrl && !bSpace && !bLMB)
  {
    CallPopupMenu( point);
  }
  // space+ctrl+lmb zoomes out 2x
  else if( bSpace && bCtrl)
  {
    if( bLMB) return;
    OnZoomLess();
    return;
  }
  else if( bSpace && bLMB)
  {
    m_iaInputAction = IA_ROTATING_VIEWER;
  }
  // we have viewer movement in view plane
  else if( bSpace && !bShift)
  {
    if( bSpace) m_iaInputAction = IA_MOVING_VIEWER_IN_VIEW_PLANE;
    StartMouseInput( point);
  }
  // second layer movements
  else if( bCSGOn)
  {
    // if ctrl+rmb pressed, we want to move second layer in view plane
    if( bCtrl && !bShift)
    {
      if( bLMB) m_iaInputAction = IA_ROTATING_SECOND_LAYER;
      else      m_iaInputAction = IA_MOVING_SECOND_LAYER_IN_VIEW_PLANE;
      StartMouseInput( point);
    }
    // test for mouse operations used while primitive CSG is on
    else if( pDoc->m_bPrimitiveMode)
    {
      // ctrl+shift+rmb sheares primitive
      if( bCtrl && bShift)
      {
        if( m_ptProjectionType != CSlaveViewer::PT_PERSPECTIVE)
          m_iaInputAction = IA_SHEARING_PRIMITIVE;
      }
      // shift+rmb in deletes vertex on base of primitive
      else if( bShift && !bCtrl && !bLMB && !bSpace)
      {
        MarkClosestVtxAndEdgeOnPrimitiveBase( point);
        if( m_iDragVertice != -1) pDoc->DeletePrimitiveVertex( m_iDragVertice);
      }
    }
  }
  else if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // measure and cut modes disable all functions
    if( theApp.m_bMeasureModeOn || theApp.m_bCutModeOn)
    {
    }
    if( bAlt && bShift)
    {
      OnDeselectByOrientation();
    }
    // alt+ctrl+rmb sizes polygon's texture
    else if( bAlt && bCtrl)
    {
      MultiplyMappingOnPolygon( 2.0f);
    }
    else if( bCtrl && bLMB)
    {
      m_iaInputAction = IA_ROTATING_POLYGON_MAPPING;
    }
    // shift turns on portal testing
    else if( bShift && !bCtrl)
    {
      m_iaInputAction = IA_SELECTING_POLYGONS;
      // cast ray but ask if test for portal hits
      CCastRay crRayHit = GetMouseHitInformation( point, TRUE, FALSE);
      ToggleHittedPolygon( crRayHit);
    }
  }
  else if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    // shift turns on portal testing
    if( bShift)
    {
      m_iaInputAction = IA_SELECTING_SECTORS;
      // cast ray but ask if test for portal hits
      CCastRay crRayHit = GetMouseHitInformation( point, TRUE, FALSE);
      ToggleHittedSector( crRayHit);
    }
  }
  else if(pDoc->GetEditingMode() == ENTITY_MODE)
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CEntity *penBrush = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();
    // if we should start mip setting mode
    if( bShift && bCtrl && !bAlt && (penBrush != NULL) )
    {
      if( m_iaInputAction == IA_MIP_SETTING) return;
      // get entity's brush
      CBrush3D *pbrBrush = penBrush->GetBrush();
      if( pbrBrush != NULL)
      {
        CChildFrame *pChild = GetChildFrame();
        // set auto mip brushing mode as it was before mip setting started
        pChild->m_bLastAutoMipBrushingOn = pChild->m_bAutoMipBrushingOn;
        // get curently active mip factor
        FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();
        // get the mip brush for current factor
        m_pbmToSetMipSwitch = pbrBrush->GetBrushMipByDistance(fCurrentMipFactor);
        // if we are in automatic mip brushing mode
        if( (GetChildFrame()->m_bAutoMipBrushingOn) && (m_pbmToSetMipSwitch != NULL) )
        {
          // set manual mip factor so current mip brush would be visible
          GetChildFrame()->m_fManualMipBrushingFactor = fCurrentMipFactor;
          // set manual mip brushing mode
          GetChildFrame()->m_bAutoMipBrushingOn = FALSE;
          m_iaInputAction = IA_MIP_SETTING;
        }
        else
        {
          m_iaInputAction = IA_NONE;
        }
      }
      else
      {
        m_iaInputAction = IA_NONE;
      }
    }
    // Alt+Ctrl+RMB sets clicked entity as target on first available free target ptr slot
    else if( bAlt && bCtrl && !bShift && (pDoc->m_selEntitySelection.Count() != 0))
    {
      // obtain information about where mouse points into the world
      CCastRay crRayHit = GetMouseHitInformation( point, FALSE, bHitModels, bHitFields);
      if(crRayHit.cr_penHit != NULL) 
      {
        // get first empty selected property
        pMainFrame->m_PropertyComboBar.SetFirstValidEmptyTargetProperty( crRayHit.cr_penHit);
      }
    }
    // if ctrl+rmb pressed, we want to move entity selection in view plane
    else if( bCtrl && !bShift && !bAlt)
    {
      if( bLMB) m_iaInputAction = IA_ROTATING_ENTITY_SELECTION;
      else      m_iaInputAction = IA_MOVING_ENTITY_SELECTION_IN_VIEW_PLANE;
      StartMouseInput( point);
    }
    else if( bShift && bSpace && !GetChildFrame()->m_bAutoMipBrushingOn)
    {
      m_iaInputAction = IA_MANUAL_MIP_SWITCH_FACTOR_CHANGING;
    }
    else if( bShift && bAlt)
    {
      // obtain information about where mouse points into the world
      CCastRay crRayHit = GetMouseHitInformation( point, FALSE, bHitModels, bHitFields);
      ShowLinkTree(crRayHit.cr_penHit, TRUE, bCaps);
    }
    else if( bShift)
    {
      // obtain information about where mouse points into the world
      CCastRay crRayHit = GetMouseHitInformation( point, FALSE, bHitModels, bHitFields);
      ShowLinkTree(crRayHit.cr_penHit, FALSE, bCaps);
    }
  }
  // if we are in vertex mode
  else if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    if( bCtrl)
    {
      if( bLMB)
      {
        m_iaInputAction = IA_ROTATE_BRUSH_VERTEX;
      }
      else
      {
        m_iaInputAction = IA_DRAG_BRUSH_VERTEX_IN_VIEW_PLANE;
      }
      // clear polygon selection because some of the old polygons could disappear
      pDoc->m_selPolygonSelection.Clear();
      pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);
      StartMouseInput( point);
    }
    if( bShift)
    {
      if(pDoc->m_selVertexSelection.Count()==3)
      {
        pDoc->m_woWorld.CreatePolygon(pDoc->m_selVertexSelection);
        pDoc->ClearSelections();
      }
    }
  }
  // if we are in terrain mode
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    InvokeSelectLayerCombo();
  }

	CView::OnRButtonDown(nFlags, point);
}

void CWorldEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWorldEditorDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  StopMouseInput();
  // if measure mode on
  if( theApp.m_bMeasureModeOn)
  {
    Invalidate( FALSE);
  }

  // if we are in browse by volume mode and lmb released, refresh first selected volume
  if( pDoc->m_bBrowseEntitiesMode && !bSpace && !bCtrl)
  {
    pDoc->SelectEntitiesByVolumeBox();
  }

  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    if( m_iaInputAction == IA_SELECTING_ENTITIES)
    {
      // obtain information about where mouse points into the world
      CCastRay crRayHit = GetMouseHitInformation( point, FALSE, TRUE, TRUE);
      // if we hit some entity
      if( crRayHit.cr_penHit != NULL)
      {
        // if shift is not pressed
        if( !bShift)
        {
          BOOL bWasSelected = crRayHit.cr_penHit->IsSelected( ENF_SELECTED);
          // deselect all selected entities
          INDEX ctSelectedBefore = pDoc->m_selEntitySelection.Count();
          pDoc->m_selEntitySelection.Clear();
          // if entity was not selected before
          if( !((ctSelectedBefore == 1) && bWasSelected))
          {
            // select it
            pDoc->m_selEntitySelection.Select( *crRayHit.cr_penHit);
          }
        }
        else
        {
          // if entity is not yet selected
          if( !crRayHit.cr_penHit->IsSelected( ENF_SELECTED))
          {
            // select it
            pDoc->m_selEntitySelection.Select( *crRayHit.cr_penHit);
          }
          // otherwise deselect it
          else
          {
            pDoc->m_selEntitySelection.Deselect( *crRayHit.cr_penHit);
          }
        }
      }
      // mark that selections have been changed
      pDoc->m_chSelections.MarkChanged();

      // and refresh property combo manualy calling on idle
      pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
      // update all views
      pDoc->UpdateAllViews( this);
    }
    else if( m_iaInputAction == IA_SELECT_LASSO_ENTITY)
    {
      m_bRequestEntityLassoSelect = TRUE;
      pDoc->m_chSelections.MarkChanged();
      Invalidate( FALSE);
    }
  }
  if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    if( m_iaInputAction == IA_SELECT_SINGLE_BRUSH_VERTEX)
    {
      m_bRequestVtxClickSelect = TRUE;
      pDoc->m_chSelections.MarkChanged();
      Invalidate( FALSE);
    }
    else if( m_iaInputAction == IA_SELECT_LASSO_BRUSH_VERTEX)
    {
      m_bRequestVtxLassoSelect = TRUE;
      pDoc->m_chSelections.MarkChanged();
      Invalidate( FALSE);
    }
    else if(
    (m_iaInputAction == IA_DRAG_BRUSH_VERTEX_IN_FLOOR_PLANE) ||
    (m_iaInputAction == IA_DRAG_BRUSH_VERTEX_IN_VIEW_PLANE) ||
    (m_iaInputAction == IA_STRETCH_BRUSH_VERTEX) ||
    (m_iaInputAction == IA_ROTATE_BRUSH_VERTEX) )
    {
      pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
    }
  }

  m_iaInputAction = IA_NONE;

  CView::OnLButtonUp(nFlags, point);
}

void CWorldEditorView::OnRButtonUp(UINT nFlags, CPoint point)
{
	CWorldEditorDoc* pDoc = GetDocument();
  if( m_iaInputAction == IA_MIP_SETTING)
  {
    SetMipBrushFactor();
  }
  else if(
  (m_iaInputAction == IA_DRAG_BRUSH_VERTEX_IN_FLOOR_PLANE) ||
  (m_iaInputAction == IA_DRAG_BRUSH_VERTEX_IN_VIEW_PLANE) ||
  (m_iaInputAction == IA_STRETCH_BRUSH_VERTEX) ||
  (m_iaInputAction == IA_ROTATE_BRUSH_VERTEX) )
  {
    pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
  }

  m_iaInputAction = IA_NONE;
  StopMouseInput();
  CView::OnRButtonUp(nFlags, point);
}

// select all descendents of selected entity
void SelectDescendents( CEntitySelection &selEntity, CEntity &enParent)
{
  FOREACHINLIST( CEntity, en_lnInParent, enParent.en_lhChildren, itenChild)
  {
    SelectDescendents( selEntity, *itenChild);
  }
  if( !enParent.IsSelected( ENF_SELECTED))
  {
    selEntity.Select( enParent);
  }
}

void CWorldEditorView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  m_iaInputAction = IA_NONE;

  CWorldEditorDoc* pDoc = GetDocument();
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bShift = nFlags & MK_SHIFT;
  BOOL bRMB = (GetKeyState( VK_RBUTTON)&0x8000) != 0;

  // space+ctrl+lmb zoomes in 2x
  if( (bSpace) && (bCtrl))
  {
    if( bRMB) return;
    // set new target
    GetChildFrame()->m_mvViewer.SetTargetPlacement( m_vHitOnMouseDown);
    // move mouse to center of screen
    CPoint ptCenter = CPoint( m_pdpDrawPort->GetWidth()/2, m_pdpDrawPort->GetHeight()/2);
    ClientToScreen( &ptCenter);
    SetCursorPos(ptCenter.x, ptCenter.y);
    OnZoomMore();
    pDoc->UpdateAllViews( NULL);
    return;
  }

  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( point);

  // space + doubble click center point that mouse points to
  if( bSpace)
  {
    // set new target
    GetChildFrame()->m_mvViewer.SetTargetPlacement( crRayHit.cr_vHit);
    // update all views
    pDoc->UpdateAllViews( NULL);
  }
  // if we are in measure or cut mode, no operation
  else if( theApp.m_bMeasureModeOn || theApp.m_bCutModeOn)
  {
  }
  // if we are in sector mode, no operation
  else if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
  }
  // if we are in polygon mode, select similar polygons
  else if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // Alt+Ctrl+RMB requests that clicked polygon sizes its texture
    if( bAlt && bCtrl)
    {
      MultiplyMappingOnPolygon( 0.5f);
    }
    // if we hit some brush entity
    else if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL))
    {
      // if shift is not pressed, delelect all polygons
      if( !bShift && !bCtrl)
      {
        pDoc->m_selPolygonSelection.Clear();
      }

      // LMBx2 centers mapping on polygon
      if( bCtrl && !bShift &&(crRayHit.cr_pbpoBrushPolygon != NULL) )
      {
        CBrushPolygon &bpo = *crRayHit.cr_pbpoBrushPolygon;
        CEntity *pen = bpo.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
        CMappingDefinition mdOriginal = bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
        CMappingDefinition mdTranslated = mdOriginal;
        CSimpleProjection3D pr;
        pr.ObjectPlacementL() = _plOrigin;
        pr.ViewerPlacementL() = pen->GetPlacement();
        pr.Prepare();
        FLOAT3D vRelative;
        pr.ProjectCoordinate(crRayHit.cr_vHit, vRelative);
        CMappingDefinition mdRelative = theApp.m_mdMapping;
        mdTranslated.Center(bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, vRelative);
        FLOAT fUOffset = mdTranslated.md_fUOffset-mdOriginal.md_fUOffset;
        FLOAT fVOffset = mdTranslated.md_fVOffset-mdOriginal.md_fVOffset;
        if( !crRayHit.cr_pbpoBrushPolygon->IsSelected(BPOF_SELECTED))
        {
            // add the offsets to its mapping
            crRayHit.cr_pbpoBrushPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fUOffset+=fUOffset;
            crRayHit.cr_pbpoBrushPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fVOffset+=fVOffset;
        }
        else
        {
          // for each selected polygon
          FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
          {
            // add the offsets to its mapping
            itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fUOffset+=fUOffset;
            itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fVOffset+=fVOffset;
          }
        }
        pDoc->m_chSelections.MarkChanged();
        pDoc->SetModifiedFlag( TRUE);
        pDoc->UpdateAllViews( NULL);
      }
      // Ctrl+Shift+LMBx2 slects similar by texture in sector
      else if( bCtrl && bShift)
      {
        OnSelectByTextureInSector();
      }
      else
      {
        // select similar to hitted polygon
        crRayHit.cr_pbpoBrushPolygon->SelectSimilarByColor( pDoc->m_selPolygonSelection);
        pDoc->m_chSelections.MarkChanged();
        pDoc->UpdateAllViews( NULL);
      }
    }
  }
  else if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    // if we hit some polygon
    if( (crRayHit.cr_penHit != NULL) && (crRayHit.cr_pbpoBrushPolygon != NULL))
    {
      CBrushPolygon &bpo=*crRayHit.cr_pbpoBrushPolygon;
      // select vertices
      FOREACHINSTATICARRAY(bpo.bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx)
      {
        if( !(*itpbvx)->IsSelected(BVXF_SELECTED)) pDoc->m_selVertexSelection.Select( **itpbvx);
      }

      pDoc->m_chSelections.MarkChanged();
      pDoc->UpdateAllViews( NULL);
    }
  }
  // if we are in entity mode
  else if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    // if nothing pressed
    if( !bCtrl && !bAlt && !bSpace)
    {
      if( crRayHit.cr_penHit != NULL)
      {
        SelectDescendents( pDoc->m_selEntitySelection, *crRayHit.cr_penHit);
        pDoc->m_chSelections.MarkChanged();
        pDoc->UpdateAllViews( NULL);
      }
    }

    // CTRL + LMB teleports entities
    if( (pDoc->m_selEntitySelection.Count() != 0) && (bCtrl) && (!bAlt) )
    {
      // lock selection's dynamic container
      pDoc->m_selEntitySelection.Lock();
      FLOATaabbox3D box;
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        box |= iten->GetPlacement().pl_PositionVector;
      }}

      FLOAT3D f3dCenter = box.Center();
      f3dCenter(2) = box.Min()(2);
      FLOAT3D f3dOffset = crRayHit.cr_vHit - f3dCenter;
      CEntity *penBrush = NULL;
      // for each of the selected entities
      FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        if (iten->en_RenderType == CEntity::RT_BRUSH) {
          penBrush = &*iten;
        }
        // if movement of anchored entities is allowed or entity is not anchored
        if( (((iten->GetFlags() & ENF_ANCHORED) == 0) ||
             (GetChildFrame()->m_bAncoredMovingAllowed)) &&
             ((iten->GetParent()==NULL) || !(iten->GetParent()->IsSelected( ENF_SELECTED))) )
        {
          // get placement of current entity
          CPlacement3D plEntityPlacement = iten->GetPlacement();
          plEntityPlacement.pl_PositionVector += f3dOffset;
          // snap to grid entity's placement
          pDoc->SnapToGrid( plEntityPlacement, m_fGridInMeters/GRID_DISCRETE_VALUES);
          // set placement back to entity
          iten->SetPlacement( plEntityPlacement);
          // mark that document is changed
          pDoc->SetModifiedFlag();
        }
        if( penBrush != NULL) 
        {
          DiscardShadows( penBrush);
        }
      }
    }
    pDoc->UpdateAllViews( NULL);
    // refresh position page
    pDoc->RefreshCurrentInfoPage();
  }
  // if we are in csg mode, CTRL + LMB teleports second layer's world (primitive)
  else if( (pDoc->GetEditingMode() == CSG_MODE) &&  bCtrl)
  {
    // set new placement of second layer
    pDoc->m_plSecondLayer.pl_PositionVector = crRayHit.cr_vHit;
    // snap to grid whole second layer's placement
    pDoc->SnapToGrid( pDoc->m_plSecondLayer, m_fGridInMeters/GRID_DISCRETE_VALUES);
    theApp.m_vfpCurrent.vfp_plPrimitive = pDoc->m_plSecondLayer;
    // refresh all views
    pDoc->UpdateAllViews( NULL);
    // refresh position page
    pDoc->RefreshCurrentInfoPage();
  }

	CView::OnLButtonDblClk(nFlags, point);
}

void CWorldEditorView::CallPopupMenu(CPoint point)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	CWorldEditorDoc* pDoc = GetDocument();

	ASSERT_VALID(pDoc);
  BOOL bHitFields = (pDoc->GetEditingMode() == ENTITY_MODE);
  // obtain information about where mouse pointed into the world in the moment of mouse down
  CCastRay crRayHitOnContext = GetMouseHitInformation( point, FALSE, TRUE, bHitFields);

  // create popup menu
  CMenu menu;
  INDEX iMenuID;
  // choose popup menu
  switch( pDoc->GetEditingMode())
  {
  case POLYGON_MODE:{ iMenuID = IDR_POLYGON_POPUP; break; }
  case SECTOR_MODE:{  iMenuID = IDR_SECTOR_POPUP;break; }
  case VERTEX_MODE:{  iMenuID = IDR_VERTEX_POPUP;break; }
  case ENTITY_MODE:{  iMenuID = IDR_ENTITY_POPUP;break; }
  case TERRAIN_MODE:{ iMenuID = IDR_TERRAIN_POPUP;break; }
  case CSG_MODE:
    {
      if( pDoc->m_bPrimitiveMode) iMenuID = IDR_CSG_PRIMITIVE_POPUP;
      else                        iMenuID = IDR_CSG_TEMPLATE_POPUP;
      break;
    }
  default: { FatalError("Unknown editing mode."); break;};
  }

  // load popup menu
  if( menu.LoadMenu(iMenuID))
  {
    BOOL bEntityClicked = FALSE;
    BOOL bBrushClicked = FALSE;
    BOOL bSecondLayerHited = FALSE;
    BOOL bPortalClicked = FALSE;
    m_penEntityHitOnContext = NULL;

    if( crRayHitOnContext.cr_penHit != NULL)
    {
      m_penEntityHitOnContext = crRayHitOnContext.cr_penHit;
      bEntityClicked = TRUE;
      // test if we hited second layer
      if( crRayHitOnContext.cr_penHit->GetWorld() != &pDoc->m_woWorld)
      {
        bSecondLayerHited = TRUE;
      }
      m_plEntityHitOnContext = crRayHitOnContext.cr_penHit->GetPlacement();
      m_plEntityHitOnContext.pl_PositionVector = crRayHitOnContext.cr_vHit;

      CEntity::RenderType rt = crRayHitOnContext.cr_penHit->GetRenderType();
      if( rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        bBrushClicked = TRUE;
      }
      m_pbpoRightClickedPolygon = crRayHitOnContext.cr_pbpoBrushPolygon;
      if( m_pbpoRightClickedPolygon != NULL)
      {
        bPortalClicked = m_pbpoRightClickedPolygon->bpo_ulFlags & BPOF_PORTAL;
      }
    }

    // remember if we hited any entity with cntext menu
    m_bEntityHitedOnContext = bEntityClicked;
    CMenu* pPopup = menu.GetSubMenu(0);

    pPopup->EnableMenuItem( ID_ENTITY_CONTEXT_HELP, MF_DISABLED|MF_GRAYED);
    pPopup->EnableMenuItem(ID_SET_AS_CSG_TARGET, MF_DISABLED|MF_GRAYED);
    if(bBrushClicked && !bSecondLayerHited) pPopup->EnableMenuItem(ID_SET_AS_CSG_TARGET, MF_ENABLED);

    // disable commands that can't be applied
    switch( pDoc->GetEditingMode())
    {
    case POLYGON_MODE:
      {
        pPopup->EnableMenuItem(ID_POLYGON_MODE, MF_DISABLED|MF_GRAYED);
        if( !bPortalClicked)
        {
          pPopup->EnableMenuItem(ID_SELECT_SECTORS_OTHER_SIDE, MF_DISABLED|MF_GRAYED);
        }

        if( !bBrushClicked)
        {
          pPopup->EnableMenuItem(ID_MENU_COPY_MAPPING, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_COPY_TEXTURE, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_SELECT_BY_TEXTURE_ADJACENT, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_SELECT_BY_TEXTURE_IN_SECTOR, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_SELECT_BY_COLOR_IN_SECTOR, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_FIND_TEXTURE, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_DESELECT_BY_ORIENTATION, MF_DISABLED|MF_GRAYED);
        }
        if( pDoc->m_selPolygonSelection.Count() == 0)
        {
          pPopup->EnableMenuItem(ID_FILTER_SELECTION, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_CHOOSE_COLOR, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_EXPORT_DISPLACE_MAP, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_RANDOM_OFFSET_U, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_RANDOM_OFFSET_V, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_POLYGONS_TO_BRUSH, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_CLONE_POLYGONS, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_DELETE_POLYGONS, MF_DISABLED|MF_GRAYED);
        }
        if( !pDoc->m_woWorld.CanJoinPolygons( pDoc->m_selPolygonSelection))
        {
          pPopup->EnableMenuItem( ID_CSG_JOIN_POLYGONS, MF_DISABLED|MF_GRAYED);
        }
        if( !pDoc->m_woWorld.CanJoinAllPossiblePolygons( pDoc->m_selPolygonSelection))
        {
          pPopup->EnableMenuItem( ID_CSG_JOIN_ALL_POLYGONS, MF_DISABLED|MF_GRAYED);
        }
        BOOL bEnableRetripling = FALSE;
        if( pDoc->m_selPolygonSelection.Count() != 0)
        {
          CBrushPolygon *pbpo = pDoc->m_selPolygonSelection.GetFirstInSelection();
          bEnableRetripling = pbpo->bpo_pbscSector->IsReTripleAvailable( pDoc->m_selPolygonSelection);
        }
        if( !bEnableRetripling)
        {
          pPopup->EnableMenuItem( ID_RE_TRIPLE, MF_DISABLED|MF_GRAYED);
        }

        if( !pDoc->m_woWorld.CanJoinPolygons( pDoc->m_selPolygonSelection))
        {
          pPopup->EnableMenuItem( ID_CSG_JOIN_POLYGONS, MF_DISABLED|MF_GRAYED);
        }
        break;
      }
    case SECTOR_MODE:
      {
        if( !bBrushClicked)
        {
          pPopup->EnableMenuItem(ID_SELECT_LINKS_TO_SECTOR, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_SELECT_SECTORS_WITH_SAME_NAME, MF_DISABLED|MF_GRAYED);
        }
        if( pDoc->m_selSectorSelection.Count() == 0)
        {
          pPopup->EnableMenuItem(ID_HIDE_SELECTED_SECTORS, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_HIDE_UNSELECTED_SECTORS, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_DELETE_SECTORS, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_CHOOSE_COLOR, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_SECTORS_TO_BRUSH, MF_DISABLED|MF_GRAYED);
        }
        
        if( !pDoc->m_woWorld.CanCopySectors(pDoc->m_selSectorSelection))
        {
          pPopup->EnableMenuItem( ID_COPY_SECTORS, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_EDIT_COPY_ALTERNATIVE, MF_DISABLED|MF_GRAYED);
        }

        if( pDoc->m_woWorld.CanJoinSectors(pDoc->m_selSectorSelection))
        {
          pPopup->EnableMenuItem( ID_CSG_JOIN_SECTORS, MF_DISABLED|MF_GRAYED);
        }
        break;
      }
    case TERRAIN_MODE:
      {
        CTerrain *ptrTerrain=GetTerrain();
        if(ptrTerrain==NULL)
        {
          pPopup->EnableMenuItem( ID_PICK_LAYER, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_SELECT_LAYER, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_CHANGE_TERRAIN_SIZE, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_CHANGE_HEIGHTMAP_SIZE, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_RECALCULATE_TERRAIN_SHADOWS, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_OPTIMIZE_TERRAIN, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_IMPORT_HEIGHTMAP, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_IMPORT_HEIGHTMAP16, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_EXPORT_HEIGHTMAP, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_EXPORT_HEIGHTMAP16, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem( ID_VIEW_HEIGHTMAP, MF_DISABLED|MF_GRAYED);

          CTerrainLayer *ptlLayer=GetLayer();
          if(ptlLayer==NULL)
          {
            pPopup->EnableMenuItem( ID_LAYER_OPTIONS, MF_DISABLED|MF_GRAYED);
          }
        }
        break;
      }
    case ENTITY_MODE:
      {
        if( bEntityClicked)
        {
          pPopup->EnableMenuItem(ID_ENTITY_CONTEXT_HELP, MF_ENABLED);
        }

        BOOL bDisableSelectTarget = FALSE;
        if( pDoc->m_selEntitySelection.Count() == 1)
        {
          CPropertyID *ppidProperty = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
          if( (ppidProperty == NULL) ||
              !((ppidProperty->pid_eptType == CEntityProperty::EPT_PARENT) ||
                (ppidProperty->pid_eptType == CEntityProperty::EPT_ENTITYPTR)))
          {
            bDisableSelectTarget = TRUE;
          }
        }
        else
        {
          bDisableSelectTarget = TRUE;
        }
        if( bDisableSelectTarget)
        {
          pPopup->EnableMenuItem(ID_SELECT_TARGET, MF_DISABLED|MF_GRAYED);
        }

        pPopup->EnableMenuItem(ID_ENTITY_MODE, MF_DISABLED|MF_GRAYED);
        // see if should disable delete
        if( !IsDeleteEntityEnabled())
        {
          pPopup->EnableMenuItem(ID_DELETE_ENTITIES, MF_DISABLED|MF_GRAYED);
        }
        // if clear all target is disabled
        if( !m_bEntityHitedOnContext)
        {
          pPopup->EnableMenuItem(ID_ROTATE_TO_TARGET_ORIGIN, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_ROTATE_TO_TARGET_CENTER, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_COPY_POSITION, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_COPY_ORIENTATION, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_COPY_PLACEMENT, MF_DISABLED|MF_GRAYED);
          
          pPopup->EnableMenuItem(ID_PASTE_POSITION, MF_DISABLED|MF_GRAYED);          

          pPopup->EnableMenuItem(ID_CLEAR_ALL_TARGETS, MF_DISABLED|MF_GRAYED);
        }
        // if clone updating is disabled
        if( !pDoc->IsCloneUpdatingAllowed())
        {
          pPopup->EnableMenuItem(ID_UPDATE_CLONES, MF_DISABLED|MF_GRAYED);
        }
        // if brush updating is disabled
        if( !pDoc->IsBrushUpdatingAllowed())
        {
          pPopup->EnableMenuItem(ID_UPDATE_BRUSHES, MF_DISABLED|MF_GRAYED);
        }
        // if copy is not allowed
        if( pDoc->m_selEntitySelection.Count() == 0)
        {
          pPopup->EnableMenuItem(ID_EDIT_COPY, MF_DISABLED|MF_GRAYED);
          pPopup->EnableMenuItem(ID_SELECT_DESCENDANTS, MF_DISABLED|MF_GRAYED);
        }
        // if clipboard world does not exist
        if( GetFileAttributesA( _fnmApplicationPath + "Temp\\ClipboardWorld.wld") == -1)
        {
          // disable pasting
          pPopup->EnableMenuItem(ID_EDIT_PASTE, MF_DISABLED|MF_GRAYED);
        }
        if( !IsSelectClonesOnContextEnabled())
        {
          // disable select clones
          pPopup->EnableMenuItem(ID_SELECT_CLONES_ON_CONTEXT, MF_DISABLED|MF_GRAYED);
        }
        if( !IsSelectOfSameClassOnContextEnabled())
        {
          // disable select clones
          pPopup->EnableMenuItem(ID_SELECT_OF_SAME_CLASS_ON_CONTEXT, MF_DISABLED|MF_GRAYED);
        }
        break;
      }
    }

    // convert mouse coordinates into screen coordinates
    ClientToScreen( &point);

    // call popup menu
    pPopup->TrackPopupMenu( TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								            point.x, point.y, this);
  }
}

void CWorldEditorView::SetEditingDataPaneInfo( BOOL bImidiateRepainting)
{
  // obtain current time
  FLOAT fTimeNow = _pTimer->GetRealTimeTick();
  // get difference to time when last mip brushing option has been used
  FLOAT fSecondsPassed = fTimeNow-_fLastMipBrushingOptionUsed;
  // if we used any mip brushing option inside last 30 seconds
  BOOL bUsingMipBrushing;
  if( fSecondsPassed < 120)     bUsingMipBrushing = TRUE;
  else                          bUsingMipBrushing = FALSE;

  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    return;
  }

  if( !theApp.m_bShowStatusInfo)
  {
    return;
  }
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CWorldEditorDoc* pDoc = GetDocument();

  char strDataPaneText[ 128];
  // obtain button statuses and interesting flags
  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;
  BOOL bRMB = (GetKeyState( VK_RBUTTON)&0x8000) != 0;
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  BOOL bEntityMode = pDoc->GetEditingMode() == ENTITY_MODE;
  BOOL bPolygonMode = pDoc->GetEditingMode() == POLYGON_MODE;
  BOOL bTerrainMode = pDoc->GetEditingMode() == TERRAIN_MODE;
  BOOL bVertexMode = pDoc->GetEditingMode() == VERTEX_MODE;
  CEntity *penBrush = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();

  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);

  //---------------- Now we will prepare status line' text describing editing data

  if( theApp.m_bMeasureModeOn)
  {
    strcpy( strDataPaneText, m_strMeasuring);
  }
  else if( theApp.m_bCutModeOn)
  {
    sprintf( strDataPaneText, "Cut/Mirror mode");
  }
  else if( (penBrush != NULL) && bUsingMipBrushing)
  {
    CBrush3D *pbrBrush = penBrush->GetBrush();
    ASSERT( pbrBrush != NULL);
    // get currently active mip factor
    FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();
    // get mip brush
    if( pbrBrush != NULL)
    {
      CBrushMip *pbmCurrentMip = pbrBrush->GetBrushMipByDistance( fCurrentMipFactor);
      if( pbmCurrentMip != NULL)
      {
        // get mip switch factor for currently visible brush
        FLOAT fCurrentMipSwitchFactor = pbmCurrentMip->GetMipDistance();
        // so prepare string telling selected brush's mip switch factor
        sprintf( strDataPaneText, "factor=%.2f, mip=%d/%d, switch=%.2f",
                 fCurrentMipFactor, pbmCurrentMip->GetMipIndex(),
                 pbrBrush->br_lhBrushMips.Count(), fCurrentMipSwitchFactor);
      }
      else
      {
        // so prepare string telling selected brush's mip switch factor
        sprintf( strDataPaneText, "factor=%.2f, after last mip", fCurrentMipFactor);
      }
    }
    else
    {
      // brush wasn't valid, report error
      sprintf( strDataPaneText, "entity has invalid brush");
    }
  }
  // if space pressed, we are interested in viewer placement
  else if( bSpace)
  {
    // get viewer position and orientation data
    CPlacement3D plViewer = GetChildFrame()->m_mvViewer.GetViewerPlacement();
    // if both mouses are pressed, we want to rotate viewer
    if( bLMB && bRMB)
    {
      // so prepare string telling viewer orientation
      sprintf( strDataPaneText, "H=%g P=%g B=%g",
               DegAngle( plViewer.pl_OrientationAngle(1)),
               DegAngle( plViewer.pl_OrientationAngle(2)),
               DegAngle( plViewer.pl_OrientationAngle(3)) );
    }
    // we are interested in viewer's current position
    else
    {
      // so prepare string telling viewer position
      sprintf( strDataPaneText, "X=%g Y=%g Z=%g",
               plViewer.pl_PositionVector(1),
               plViewer.pl_PositionVector(2),
               plViewer.pl_PositionVector(3) );
    }
  }
  else if( (bVertexMode) && ( pDoc->m_selVertexSelection.Count() != 0) )
  {
    // so prepare string telling entity position
    if (pDoc->m_selVertexSelection.Count()==1) {
      const FLOAT3D &v = pDoc->m_selVertexSelection.GetFirst().bvx_vAbsolute;
      sprintf( strDataPaneText, "%f,%f,%f", v(1), v(2), v(3));
    } else {
      sprintf( strDataPaneText, "Dragging %d vertices", pDoc->m_selVertexSelection.Count());
    }
  }
  // if we are in entity mode and there is selected entity
  else if( (bEntityMode) && ( pDoc->m_selEntitySelection.Count() != 0) )
  {
    // lock selection's dynamic container
    pDoc->m_selEntitySelection.Lock();
    // get first entity
    CEntity *penEntityOne = pDoc->m_selEntitySelection.Pointer(0);
    // unlock selection's dynamic container
    pDoc->m_selEntitySelection.Unlock();
    // get placement of first entity
    CPlacement3D plEntityOnePlacement = penEntityOne->GetPlacement();
    // if both mouses are pressed, we want to rotate entity so prepare text telling angles
    if( bLMB && bRMB)
    {
      // so prepare string telling entity orientation
      sprintf( strDataPaneText, "H=%g P=%g B=%g",
               DegAngle( plEntityOnePlacement.pl_OrientationAngle(1)),
               DegAngle( plEntityOnePlacement.pl_OrientationAngle(2)),
               DegAngle( plEntityOnePlacement.pl_OrientationAngle(3)) );
    }
    // we are interested in entity one's current position
    else
    {
      // so prepare string telling entity position
      sprintf( strDataPaneText, "X=%g Y=%g Z=%g",
               plEntityOnePlacement.pl_PositionVector(1),
               plEntityOnePlacement.pl_PositionVector(2),
               plEntityOnePlacement.pl_PositionVector(3) );
    }
  }
  // if we are in polygon mode and only one polygon is selected
  else if( (bPolygonMode) && (pDoc->m_selPolygonSelection.Count() == 1) && bCtrl)
  {
    pDoc->m_selPolygonSelection.Lock();
    CBrushPolygon *pbpoBrushPolygon = pDoc->m_selPolygonSelection.Pointer(0);
    pDoc->m_selPolygonSelection.Unlock();

    CMappingDefinitionUI mdui;
    pbpoBrushPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.ToUI( mdui);
    // prepare text telling about mapping info
    sprintf( strDataPaneText, "Tex %d: U=%.1f V=%.1f RU=%.1f RV=%.1f",
      pDoc->m_iTexture+1, mdui.mdui_fUOffset, mdui.mdui_fVOffset,
      mdui.mdui_aURotation, mdui.mdui_aVRotation);
  }
  else if( bTerrainMode)
  {
    // pane data text has bee obtained on render's ray cast
    sprintf(strDataPaneText, "%s", m_strTerrainDataPaneText);
  }
  else if( bCSGOn)
  {
    // if primitive mode on and ctrl+shift+RMB pressed
    if( pDoc->m_bPrimitiveMode && bCtrl && bShift && bRMB)
    {
      // prepare pane text telling primitive shear info
      sprintf( strDataPaneText, "Shear X=%g Shear Z=%g",
               theApp.m_vfpCurrent.vfp_fShearX, theApp.m_vfpCurrent.vfp_fShearZ);
    }
    // if primitive mode on and ctrl+shift+LMB pressed
    else if( pDoc->m_bPrimitiveMode && bCtrl && bShift)
    {
      // prepare pane text telling primitive size info
      sprintf( strDataPaneText, "W=%g H=%g L=%g",
               theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin,
               theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin,
               theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin);
    }
    // else if ctrl and both mouses are pressed
    else if( bCtrl && bLMB && bRMB)
    {
      // prepare text telling CSG layer orientation
      sprintf( strDataPaneText, "H=%g P=%g B=%g",
               DegAngle( pDoc->m_plSecondLayer.pl_OrientationAngle(1)),
               DegAngle( pDoc->m_plSecondLayer.pl_OrientationAngle(2)),
               DegAngle( pDoc->m_plSecondLayer.pl_OrientationAngle(3)) );
    }
    // else
    else
    {
      // prepare text telling CSG layer position
      sprintf( strDataPaneText, "X=%g Y=%g Z=%g",
               pDoc->m_plSecondLayer.pl_PositionVector(1),
               pDoc->m_plSecondLayer.pl_PositionVector(2),
               pDoc->m_plSecondLayer.pl_PositionVector(3) );
    }
  }
  else
  {
    // sprintf( strDataPaneText, "Idle time");
    // obtain information about where mouse points into the world
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
    // if any kind of entity was hitted
    if( crRayHit.cr_penHit != NULL)
    {
      // get hitted coordinate
      FLOAT3D f3Coordinate = crRayHit.cr_vHit;
      // snap it
      SnapVector( f3Coordinate);

      // prepare text describing hitted coordinate
      sprintf( strDataPaneText, "X=%g Y=%g Z=%g", f3Coordinate(1), f3Coordinate(2), f3Coordinate(3));
    }
    else
    {
      // if the ray hits the empty space
      sprintf( strDataPaneText, "Over nothing");
    }
  }

  // put editing data info into status line
  pMainFrame->m_wndStatusBar.SetPaneText( POSITION_PANE, CString(strDataPaneText), bImidiateRepainting);
}

#define SCROLL_CHANGE 0.2f
#define ZOOM_CHANGE 0.05f
#define ANGLE_CHANGE 1.0f

void CWorldEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;
  BOOL bRMB = (GetKeyState( VK_RBUTTON)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  
  if(((m_iaInputAction == IA_SELECT_LASSO_ENTITY) ||
    (m_iaInputAction == IA_SELECT_LASSO_BRUSH_VERTEX)) && !bLMB)
  {
    m_iaInputAction = IA_NONE;
    // discard laso selection
    m_avpixLaso.Clear();
  }

  // if neather mouse key is pressed, simulate that action is none
  if( !bLMB && !bRMB) m_iaInputAction = IA_NONE;
   
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CWorldEditorDoc* pDoc = GetDocument();
  
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
  if( pWorldEditorView == NULL) return;

  HWND hwndUnderMouse = ::WindowFromPoint( point);
  HWND hwndInfo = NULL;
  if( pMainFrame->m_pInfoFrame != NULL)
  {
    hwndInfo = pMainFrame->m_pInfoFrame->m_hWnd;
  }
  
  HWND hwndFocused = ::GetFocus();
  //HWND hwndForeground = ::GetForegroundWindow();
  //if( hwndForeground==pMainFrame->m_hWnd)
  {
    // if this is the first mouse move over new view and it is not over info window,
    // set focus and loose all previous actions
    if( (m_hWnd != hwndFocused) && ( hwndInfo != hwndUnderMouse) )
    {
      SetActiveWindow();
      SetFocus();
      GetChildFrame()->SetActiveView( this);
      pMainFrame->MDIActivate(GetParentFrame());

      // cancel any possible previous actions
      m_iaInputAction = IA_NONE;
    }
  }

  CDrawPort *pdpValidDrawPort = GetDrawPort();
  if( pdpValidDrawPort == NULL) return;

  CView::OnMouseMove(nFlags, point);
	CPoint ptScreen = point;
  LONG lOffsetX = point.x - m_ptMouse.x;
  LONG lOffsetY = point.y - m_ptMouse.y;
  float fOriginOffsetX = (FLOAT) (point.x - m_ptMouseDown.x);
  float fOriginOffsetY = (FLOAT) (point.y - m_ptMouseDown.y);

  //_RPT2(_CRT_WARN, "%f %f\n", fOriginOffsetX, fOriginOffsetY);

  // create a slave viewer
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);
  BOOL bHitModels = (pDoc->GetEditingMode() != POLYGON_MODE) &&
                    (pDoc->GetEditingMode() != TERRAIN_MODE);
  BOOL bHitFields = pDoc->GetEditingMode() == ENTITY_MODE;
  BOOL bHitBrushes = pDoc->GetEditingMode() != TERRAIN_MODE;

  // set dummy ray hit result
  CCastRay crRayHit( NULL, CPlacement3D(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0)) );
  BOOL bRayHitNeeded;
  if( (m_iaInputAction == IA_MOVING_VIEWER_IN_FLOOR_PLANE) ||
      (m_iaInputAction == IA_MOVING_VIEWER_IN_VIEW_PLANE) ||
      (m_iaInputAction == IA_ROTATING_VIEWER) ||
      (m_iaInputAction == IA_MOVING_SECOND_LAYER_IN_FLOOR_PLANE) ||
      (m_iaInputAction == IA_MOVING_SECOND_LAYER_IN_VIEW_PLANE) ||
      (m_iaInputAction == IA_ROTATING_SECOND_LAYER) ||
      (m_iaInputAction == IA_MOVING_ENTITY_SELECTION_IN_FLOOR_PLANE) ||
      (m_iaInputAction == IA_MOVING_ENTITY_SELECTION_IN_VIEW_PLANE) ||
      (m_iaInputAction == IA_ROTATING_ENTITY_SELECTION) ||
      (m_iaInputAction == IA_DRAG_BRUSH_VERTEX_IN_FLOOR_PLANE) ||
      (m_iaInputAction == IA_DRAG_BRUSH_VERTEX_IN_VIEW_PLANE) ||
      (m_iaInputAction == IA_SELECT_LASSO_ENTITY) ||
      (m_iaInputAction == IA_SELECT_LASSO_BRUSH_VERTEX) ||
      (m_iaInputAction == IA_STRETCH_BRUSH_VERTEX) ||
      (m_iaInputAction == IA_ROTATE_BRUSH_VERTEX) ) {
    bRayHitNeeded = FALSE;
  } else {
    bRayHitNeeded = TRUE;
  }
      
  BOOL bSelectPortals = FALSE;
  // for right mouse button select portals
  if( bRMB)
  {
    bSelectPortals = TRUE;
  }

  if( bRayHitNeeded)
  {
    crRayHit = GetMouseHitInformation( m_ptMouse, bSelectPortals, bHitModels, bHitFields, NULL, bHitBrushes);
    if(abs(lOffsetX)>=1 || abs(lOffsetY)>=1)
    {
      theApp.m_vLastTerrainHit=crRayHit.cr_vHit;
      theApp.m_penLastTerrainHit=crRayHit.cr_penHit;
    }

    m_strTerrainDataPaneText="Terrain not hit";
    if( pDoc->GetEditingMode() == TERRAIN_MODE && !_pInput->IsInputEnabled())
    {
      if( (crRayHit.cr_penHit!=NULL) && (crRayHit.cr_penHit->GetRenderType() == CEntity::RT_TERRAIN))
      {
        CTerrain *ptrTerrain=crRayHit.cr_penHit->GetTerrain();
        INDEX iLayerBeneath=-1;
        UBYTE ubLayerPower=0;
        INDEX ctLayers=ptrTerrain->tr_atlLayers.Count();
        for(INDEX iLayer=0; iLayer<ctLayers; iLayer++)
        {
          UBYTE ubPower=GetValueFromMask(ptrTerrain, iLayer, crRayHit.cr_vHit);
          if(ubPower>0)
          {
            iLayerBeneath=iLayer;
            ubLayerPower=ubPower;
          }
        }
        if(iLayerBeneath!=-1)
        {
          if(theApp.m_iTerrainEditMode==TEM_HEIGHTMAP)
          {
            if( bCtrl&&bAlt)
            {
              Point pt=Calculate2dHitPoint(ptrTerrain, crRayHit.cr_vHit);
              INDEX iWidth=ptrTerrain->tr_pixHeightMapWidth;
              UWORD uwAltitude=*(ptrTerrain->tr_auwHeightMap+iWidth*pt.pt_iY+pt.pt_iX);
  	          FLOAT fAltitudeBeneath=FLOAT(uwAltitude)/65535*ptrTerrain->tr_vTerrainSize(2);
              m_strTerrainDataPaneText.PrintF("Altitude: %g", fAltitudeBeneath);
            }
            else
            {
  	          FLOAT fReferenceAltitude=FLOAT(theApp.m_uwEditAltitude)/65535*ptrTerrain->tr_vTerrainSize(2);
              m_strTerrainDataPaneText.PrintF("Reference altitude: %g", fReferenceAltitude);
            }
          }
          else
          {
            m_strTerrainDataPaneText.PrintF("Layer: %d (%d%%)", iLayerBeneath+1, INDEX(ubLayerPower/255.0f*100.0f));
          }
        }
        else
        {
          m_strTerrainDataPaneText.PrintF("No layers");
        }
      }
      else
      {
        m_strTerrainDataPaneText.PrintF("No terrain hit");
      }
    }
  }

  BOOL bRefreshView = FALSE;// view refresh is not needed for now 
  BOOL bObjectMoved = FALSE;// nothing has moved for now
  BOOL bRepaintImmediately = FALSE;   // force immediate repainting
  BOOL bRecreatePrimitive = FALSE;// there is no need for recreating primitive

  // act acording to action started in OnLButtonDown() and OnRButtonDown()
  switch( m_iaInputAction)
  {
    case IA_MIP_SETTING:
    {
      // remember current time as time when last mip brushing option has been used
      _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();
      if( lOffsetY > 0) lOffsetY = 0;
      // translate viewer in floor plane
      svViewer.Translate_OwnSystem( 0, 0, -lOffsetY);
      // set new viewer position
      GetChildFrame()->m_mvViewer = svViewer;
      bRefreshView = TRUE;
      break;
    }
    case IA_MANUAL_MIP_SWITCH_FACTOR_CHANGING:
    {
      if( GetChildFrame()->m_fManualMipBrushingFactor - lOffsetY/50.0f > 0.0f)
      {
        // remember current time as time when last mip brushing option has been used
        _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();
        GetChildFrame()->m_fManualMipBrushingFactor -= lOffsetY/50.0f;
      }
      bRefreshView = TRUE;
      break;
    }
    case IA_ROTATING_VIEWER:
    {
      svViewer.Rotate_HPB( lOffsetX, lOffsetY, 0);
      // set new viewer position
      GetChildFrame()->m_mvViewer = svViewer;
      bRefreshView = TRUE;
      break;
    }
    case IA_MOVING_VIEWER_IN_FLOOR_PLANE:
    {
      // translate viewer in floor plane
      svViewer.Translate_OwnSystem( lOffsetX, lOffsetY, 0);
      // set new viewer position
      GetChildFrame()->m_mvViewer = svViewer;
      bRefreshView = TRUE;
      break;
    }
    case IA_MOVING_VIEWER_IN_VIEW_PLANE:
    {
      // translate viewer in view plane
      svViewer.Translate_OwnSystem( lOffsetX, 0, -lOffsetY);
      // set new viewer position
      GetChildFrame()->m_mvViewer = svViewer;
      bRefreshView = TRUE;
      break;
    }
    case IA_SHEARING_PRIMITIVE:
    {
      // get translation vector in view space
      FLOAT fZoom = svViewer.GetZoomFactor();
      CPlacement3D plVector0(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
      CPlacement3D plVector1(
        FLOAT3D( fOriginOffsetX/fZoom, -fOriginOffsetY/fZoom, 0.0f),
        ANGLE3D(0,0,0));
      // project translation vector from view space to absolute space
      CPlacement3D plViewer = svViewer.GetViewerPlacement();
      plVector0.RelativeToAbsolute(plViewer);
      plVector1.RelativeToAbsolute(plViewer);
      // project translation vector from absolute space to second layer space
      plVector0.AbsoluteToRelative(pDoc->m_plSecondLayer);
      plVector1.AbsoluteToRelative(pDoc->m_plSecondLayer);
      // extract translation vector from placements
      FLOAT3D vSizeDelta = plVector1.pl_PositionVector-plVector0.pl_PositionVector;

      // apply mouse movement
      theApp.m_vfpCurrent.vfp_fShearX = m_VFPMouseDown.vfp_fShearX+vSizeDelta(1);
      theApp.m_vfpCurrent.vfp_fShearZ = m_VFPMouseDown.vfp_fShearZ+vSizeDelta(3);
      pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fShearX, m_fGridInMeters/GRID_DISCRETE_VALUES);
      pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fShearZ, m_fGridInMeters/GRID_DISCRETE_VALUES);
      bRefreshView = TRUE;
      bRecreatePrimitive = TRUE;
      break;
    }
    case IA_STRETCHING_PRIMITIVE:
    {
      // get translation vector in view space
      FLOAT fZoom = svViewer.GetZoomFactor();
      CPlacement3D plVector0(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
      CPlacement3D plVector1(
        FLOAT3D( fOriginOffsetX/fZoom, -fOriginOffsetY/fZoom,0.0f),
        ANGLE3D(0,0,0));
      // project translation vector from view space to absolute space
      CPlacement3D plViewer = svViewer.GetViewerPlacement();
      plVector0.RelativeToAbsolute(plViewer);
      plVector1.RelativeToAbsolute(plViewer);
      // project translation vector from absolute space to second layer space
      plVector0.AbsoluteToRelative(pDoc->m_plSecondLayer);
      plVector1.AbsoluteToRelative(pDoc->m_plSecondLayer);
      // extract translation vector from placements
      FLOAT3D vSizeDelta = plVector1.pl_PositionVector-plVector0.pl_PositionVector;

      // apply mouse movement
      CValuesForPrimitive vfpTemp = theApp.m_vfpCurrent;
      vfpTemp.vfp_fXMin = m_VFPMouseDown.vfp_fXMin+vSizeDelta(1)*aiForAllowedSizing[m_iSizeControlVertice][0];
      vfpTemp.vfp_fXMax = m_VFPMouseDown.vfp_fXMax+vSizeDelta(1)*aiForAllowedSizing[m_iSizeControlVertice][1];
      vfpTemp.vfp_fYMin = m_VFPMouseDown.vfp_fYMin+vSizeDelta(2)*aiForAllowedSizing[m_iSizeControlVertice][2];
      vfpTemp.vfp_fYMax = m_VFPMouseDown.vfp_fYMax+vSizeDelta(2)*aiForAllowedSizing[m_iSizeControlVertice][3];
      vfpTemp.vfp_fZMin = m_VFPMouseDown.vfp_fZMin+vSizeDelta(3)*aiForAllowedSizing[m_iSizeControlVertice][4];
      vfpTemp.vfp_fZMax = m_VFPMouseDown.vfp_fZMax+vSizeDelta(3)*aiForAllowedSizing[m_iSizeControlVertice][5];

      pDoc->SnapFloat( vfpTemp.vfp_fXMin, m_fGridInMeters/GRID_DISCRETE_VALUES);
      pDoc->SnapFloat( vfpTemp.vfp_fXMax, m_fGridInMeters/GRID_DISCRETE_VALUES);
      pDoc->SnapFloat( vfpTemp.vfp_fYMin, m_fGridInMeters/GRID_DISCRETE_VALUES);
      pDoc->SnapFloat( vfpTemp.vfp_fYMax, m_fGridInMeters/GRID_DISCRETE_VALUES);
      pDoc->SnapFloat( vfpTemp.vfp_fZMin, m_fGridInMeters/GRID_DISCRETE_VALUES);
      pDoc->SnapFloat( vfpTemp.vfp_fZMax, m_fGridInMeters/GRID_DISCRETE_VALUES);

      // if movement produced valid primitive
      if( (vfpTemp.vfp_fXMax>vfpTemp.vfp_fXMin) &&
          (vfpTemp.vfp_fYMax>vfpTemp.vfp_fYMin) &&
          (vfpTemp.vfp_fZMax>vfpTemp.vfp_fZMin) )
      {
        theApp.m_vfpCurrent = vfpTemp;
        bRefreshView = TRUE;
        bRecreatePrimitive = TRUE;
      }
      break;
    }
    case IA_CHANGING_RANGE_PROPERTY:
    case IA_CHANGING_ANGLE3D_PROPERTY:
    {
      CPropertyID *ppid = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
      if( ppid == NULL) return;
      //for all selected entities
      FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        // obtain property ptr
        CEntityProperty *penpProperty = iten->PropertyForName( ppid->pid_strName);
        if( penpProperty == NULL) return;
        // discard old entity settings
        iten->End();
        if( ppid->pid_eptType == CEntityProperty::EPT_RANGE)
        {
          // get editing range for current entity
          FLOAT fRange = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT);
          // add range offset received during L/R mouse move
          fRange += svViewer.PixelsToMeters(lOffsetX);
          // set new range
          ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT) = fRange;
          // change dialog's range variable
          pMainFrame->m_PropertyComboBar.m_fEditingFloat = fRange;
        }
        else
        {
          // get angle3d for current entity
          ANGLE3D aAngle = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ANGLE3D);
          // create dummy placement
          CPlacement3D plDummyPlacement;
          plDummyPlacement.pl_PositionVector = FLOAT3D( 0.0f, 0.0f, 0.0f);
          plDummyPlacement.pl_OrientationAngle = aAngle;
          // set new angles using trackball method
          svViewer.RotatePlacement_TrackBall(plDummyPlacement, lOffsetX, lOffsetY, 0);
          aAngle = plDummyPlacement.pl_OrientationAngle;
          // set new angle
          ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ANGLE3D) = aAngle;
          // change dialog's range variable
          pMainFrame->m_PropertyComboBar.m_fEditingHeading = DegAngle(aAngle(1));
          pMainFrame->m_PropertyComboBar.m_fEditingPitch = DegAngle(aAngle(2));
          pMainFrame->m_PropertyComboBar.m_fEditingBanking = DegAngle(aAngle(3));
        }

        // apply new entity settings
        iten->Initialize();
        // update edit range control (by updating parent dialog)
        pMainFrame->m_PropertyComboBar.UpdateData( FALSE);
        bRefreshView = TRUE;
      }
      // mark that document is changed
      pDoc->SetModifiedFlag( TRUE);
      break;
    }
    case IA_ROTATING_ENTITY_SELECTION:
    case IA_ROTATING_SECOND_LAYER:
    {
      // use trackball method for rotation
      svViewer.RotatePlacement_TrackBall(m_plMouseMove, -lOffsetX, -lOffsetY, 0);
      pDoc->m_chSelections.MarkChanged();
      bObjectMoved = TRUE;
      break;
    }
    case IA_MOVING_ENTITY_SELECTION_IN_FLOOR_PLANE:
    case IA_MOVING_SECOND_LAYER_IN_FLOOR_PLANE:
    {
      svViewer.TranslatePlacement_OwnSystem(m_plMouseMove, lOffsetX, lOffsetY, 0);
      pDoc->m_chSelections.MarkChanged();
      bObjectMoved = TRUE;
      break;
    }
    case IA_MOVING_ENTITY_SELECTION_IN_VIEW_PLANE:
    case IA_MOVING_SECOND_LAYER_IN_VIEW_PLANE:
    {
      svViewer.TranslatePlacement_OwnSystem(m_plMouseMove, lOffsetX, 0, lOffsetY);
      pDoc->m_chSelections.MarkChanged();
      bObjectMoved = TRUE;
      break;
    }
    case IA_SIZING_SELECT_BY_VOLUME_BOX:
    {
      // get translation vector in view space
      FLOAT fZoom = svViewer.GetZoomFactor();
      CPlacement3D plVector0(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
      CPlacement3D plVector1(FLOAT3D(fOriginOffsetX/fZoom,-fOriginOffsetY/fZoom,0.0f),
        ANGLE3D(0,0,0));
      // project translation vector from view space to absolute space
      CPlacement3D plViewer = svViewer.GetViewerPlacement();
      plVector0.RelativeToAbsolute(plViewer);
      plVector1.RelativeToAbsolute(plViewer);
      // extract translation vector from placements
      FLOAT3D vDelta = plVector1.pl_PositionVector-plVector0.pl_PositionVector;
      // apply movement
      FLOAT3D vNewVolumeBoxCorner = pDoc->m_vVolumeBoxStartDragVertice+vDelta;
      pDoc->CorrectBox( pDoc->m_iVolumeBoxDragVertice, vNewVolumeBoxCorner);
      bRefreshView = TRUE;
      break;
    }
    case IA_DRAG_VERTEX_ON_PRIMITIVE_BASE:
    {
      if( m_iDragVertice == -1) break;
      // ------ Move closest vertice on base of primitive
      CPlacement3D plVector0(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
      FLOAT fZoom = svViewer.GetZoomFactor();
      CPlacement3D plVector1(FLOAT3D(fOriginOffsetX/fZoom,-fOriginOffsetY/fZoom,0.0f),
        ANGLE3D(0,0,0));
      // project translation vector from view space to absolute space
      CPlacement3D plViewer = svViewer.GetViewerPlacement();
      plVector0.RelativeToAbsolute(plViewer);
      plVector1.RelativeToAbsolute(plViewer);
      // project translation vector from absolute space to second layer space
      plVector0.AbsoluteToRelative(pDoc->m_plSecondLayer);
      plVector1.AbsoluteToRelative(pDoc->m_plSecondLayer);
      // extract translation vector from placements
      DOUBLE3D vDelta = FLOATtoDOUBLE(plVector1.pl_PositionVector)-
                        FLOATtoDOUBLE(plVector0.pl_PositionVector);
      
      DOUBLE3D &vDrag = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[m_iDragVertice];
      // apply movement
      vDrag = m_vStartDragVertice + vDelta;
      // snap resulting vertex coordinate
      SnapVector( vDrag);

      // refresh primitive
      pDoc->CreatePrimitive();
      bRefreshView = TRUE;
      break;
    }
    case IA_DRAG_VERTEX_ON_PRIMITIVE:
    {
      FLOAT fZoom = svViewer.GetZoomFactor();
      DragVerticesOnPrimitive(fOriginOffsetX/fZoom,-fOriginOffsetY/fZoom, 0.0f, FALSE);
      bRefreshView = TRUE;
      break;
    }
    case IA_SELECT_LASSO_BRUSH_VERTEX:
    case IA_SELECT_LASSO_ENTITY:
    {
      INDEX ctLasoPts = m_avpixLaso.Count();
      m_avpixLaso.Push(1);
      m_avpixLaso[ctLasoPts](1) = point.x;
      m_avpixLaso[ctLasoPts](2) = point.y;
      Invalidate( FALSE);
      break;
    }
    case IA_SELECT_SINGLE_BRUSH_VERTEX:
    {
      // if mouse moved more than few pixels arround
      if( (abs( lOffsetX) > 3) || (abs( lOffsetY) > 3) )
      {
        // if shift nor alt were pressed on mouse down
        if( !m_bOnSelectVertexShiftDown && !m_bOnSelectVertexAltDown)
        {
          // clear vertex selection
          pDoc->m_selVertexSelection.Clear();
        }
        // switch to laso select
        m_iaInputAction = IA_SELECT_LASSO_BRUSH_VERTEX;
        // add mouse down and current mouse positions to lasso buffer
        m_avpixLaso.Push(2);
        m_avpixLaso[0](1) = m_ptMouseDown.x;
        m_avpixLaso[0](2) = m_ptMouseDown.y;
        m_avpixLaso[1](1) = point.x;
        m_avpixLaso[1](2) = point.y;
        pDoc->m_chSelections.MarkChanged();
        Invalidate( FALSE);
      }
      break;
    }
    case IA_DRAG_BRUSH_VERTEX_IN_FLOOR_PLANE:
    {
      bRepaintImmediately = TRUE;
      FLOAT fZoom = svViewer.GetZoomFactor();
      DragBrushVertex(fOriginOffsetX/fZoom,-fOriginOffsetY/fZoom, 0.0f);
      pDoc->SetModifiedFlag( TRUE);
      bRefreshView = TRUE;
      break;
    }
    case IA_DRAG_BRUSH_VERTEX_IN_VIEW_PLANE:
    {
      bRepaintImmediately = TRUE;
      FLOAT fZoom = svViewer.GetZoomFactor();
      DragBrushVertex(fOriginOffsetX/fZoom, 0.0f, fOriginOffsetY/fZoom);
      pDoc->SetModifiedFlag( TRUE);
      bRefreshView = TRUE;
      break;
    }
    case IA_ROTATE_BRUSH_VERTEX:
    {
      bRepaintImmediately = TRUE;
      // apply brush vertex rotation
      RotateOrStretchBrushVertex(-fOriginOffsetX, -fOriginOffsetY, TRUE);
      pDoc->SetModifiedFlag( TRUE);
      // calculate BBox of all vertices before rotating started
      bRefreshView = TRUE;
      break;
    }
    case IA_STRETCH_BRUSH_VERTEX:
    {
      bRepaintImmediately = TRUE;
      // apply brush stretch
      FLOAT fZoom = svViewer.GetZoomFactor();
      RotateOrStretchBrushVertex(fOriginOffsetX, fZoom, FALSE);
      pDoc->SetModifiedFlag( TRUE);
      // calculate BBox of all vertices before rotating started
      bRefreshView = TRUE;
      break;
    }
    case IA_SELECTING_POLYGONS:
    {
      // if mouse is not over some polygon, break
      if( (crRayHit.cr_penHit == NULL) ||
          (crRayHit.cr_pbpoBrushPolygon == NULL) ) break;
      // if polygon under mouse is selected and first polygon that we hitted
      // was deselected
      if( crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED) && m_bWeDeselectedFirstPolygon)
      {
        // deselect polygon under mouse
        pDoc->m_selPolygonSelection.Deselect( *crRayHit.cr_pbpoBrushPolygon);
      }
      // if polygon under mouse is not selected and first polygon that we hitted
      // at the begining of selecting was selected
      if( !crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED) && !m_bWeDeselectedFirstPolygon)
      {
        // select polygon under mouse
        pDoc->m_selPolygonSelection.Select( *crRayHit.cr_pbpoBrushPolygon);
      }
      // mark that selections have been changed
      pDoc->m_chSelections.MarkChanged();
      // and refresh property combo manualy calling on idle
      pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
      bRefreshView = TRUE;
      break;
    }
    case IA_SELECTING_SECTORS:
    {
      // if mouse is not over some polygon, break
      if( (crRayHit.cr_penHit == NULL) ||
          (crRayHit.cr_pbpoBrushPolygon == NULL) ) break;
      // if sector under mouse is selected and first sector that we hitted
      // at the begining of selecting was deselected
      if( crRayHit.cr_pbscBrushSector->IsSelected( BSCF_SELECTED) && m_bWeDeselectedFirstSector)
      {
        // deselect sector under mouse
        pDoc->m_selSectorSelection.Deselect( *crRayHit.cr_pbscBrushSector);
      }
      // if sector under mouse is not selected and first sector that we hitted
      // at the begining of selecting was selected
      if( !crRayHit.cr_pbscBrushSector->IsSelected( BSCF_SELECTED) && !m_bWeDeselectedFirstSector)
      {
        // select sector under mouse
        pDoc->m_selSectorSelection.Select( *crRayHit.cr_pbscBrushSector);
      }
      // mark that selections have been changed
      pDoc->m_chSelections.MarkChanged();
      // and refresh property combo manualy calling on idle
      pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
      bRefreshView = TRUE;
      break;
    }
    case IA_SELECTING_ENTITIES:
    {
      // if mouse moved more than few pixels arround
      if( (abs( lOffsetX) > 3) || (abs( lOffsetY) > 3) )
      {
        // if shift nor alt were pressed on mouse down
        if( !m_bOnSelectEntityShiftDown && !m_bOnSelectEntityAltDown)
        {
          // clear entity selection
          pDoc->m_selEntitySelection.Clear();
        }
        // switch to laso select
        m_iaInputAction = IA_SELECT_LASSO_ENTITY;
        // add mouse down and current mouse positions to lasso buffer
        m_avpixLaso.Push(2);
        m_avpixLaso[0](1) = m_ptMouseDown.x;
        m_avpixLaso[0](2) = m_ptMouseDown.y;
        m_avpixLaso[1](1) = point.x;
        m_avpixLaso[1](2) = point.y;
        Invalidate( FALSE);
      }
      break;
    }
    case IA_MEASURING:
    {
      // just refresh view
      bRefreshView = TRUE;
      break;
    }
    case IA_MOVING_CUT_LINE_START:
    {      
      FLOAT fZoom = svViewer.GetZoomFactor();
      FLOAT3D vDelta = ProjectVectorToWorldSpace( fOriginOffsetX/fZoom, -fOriginOffsetY/fZoom, 0.0f);
      // calculate and snap new position
      pDoc->m_vCutLineStart = pDoc->m_vControlLineDragStart+vDelta;
      SnapVector( pDoc->m_vCutLineStart);
      bRefreshView = TRUE;
      break;
    }
    case IA_MOVING_CUT_LINE_END:
    {      
      // get 3D delta vector in world, from mouse down point to current mouse position
      FLOAT fZoom = svViewer.GetZoomFactor();
      FLOAT3D vDelta = ProjectVectorToWorldSpace( fOriginOffsetX/fZoom, -fOriginOffsetY/fZoom, 0.0f);
      // calculate and snap new position
      pDoc->m_vCutLineEnd = pDoc->m_vControlLineDragStart+vDelta;
      SnapVector( pDoc->m_vCutLineEnd);
      bRefreshView = TRUE;
      break;
    }
    case IA_CUT_MODE:
    {
      // calculate current cut line end
      pDoc->m_vCutLineEnd = Get3DCoordinateFrom2D( point);
      bRefreshView = TRUE;
      break;
    }
    case IA_ROTATING_POLYGON_MAPPING:
    {
      if(crRayHit.cr_pbpoBrushPolygon==NULL) return;
      svViewer.RotatePlacement_TrackBall(m_plMouseMove, lOffsetX, lOffsetY, 0);
      // get rotation angle
      ANGLE3D angMappingRotation = m_plMouseMove.pl_OrientationAngle;

      CTString strDataPaneText;
      strDataPaneText.PrintF("H=%g,P=%g,B=%g",
        DegAngle( angMappingRotation(1)),
        DegAngle( angMappingRotation(2)),
        DegAngle( angMappingRotation(3)));
      pMainFrame->m_wndStatusBar.SetPaneText( STATUS_LINE_PANE, CString(strDataPaneText), TRUE);

      // for each selected polygon
      FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        CBrushPolygon *pbpo = itbpo;
        CEntity *pen = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
        CSimpleProjection3D pr;
        pr.ObjectPlacementL() = _plOrigin;
        pr.ViewerPlacementL() = pen->GetPlacement();
        pr.Prepare();
        FLOATplane3D vRelative;
        pr.ProjectCoordinate(m_vHitOnMouseDown, vRelative);

        // rotate it
        itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.Rotate(
          pbpo->bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative,
          vRelative, angMappingRotation(1));
      }
      // mark that document is changed
      pDoc->SetModifiedFlag();
      bRefreshView = TRUE;
      pDoc->UpdateAllViews( NULL);
      // reset mouse placement
      m_plMouseMove.pl_PositionVector = crRayHit.cr_vHit;
      m_plMouseMove.pl_OrientationAngle = ANGLE3D( 0, 0, 0);
      // refresh position page
      pDoc->RefreshCurrentInfoPage();
      break;
    }
    case IA_MOVING_POLYGON_MAPPING:
    {
      // find translation vector in 3d
      crRayHit.cr_vHit = GetMouseHitOnPlane(point, m_plTranslationPlane);
      FLOAT3D f3dMappingTranslation = crRayHit.cr_vHit-m_plMouseMove.pl_PositionVector;
      // find how much that offsets hit polygon
      if( m_pbpoTranslationPlane == NULL) return;
      CBrushPolygon &bpo = *m_pbpoTranslationPlane;
      CEntity *pen = bpo.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
      CSimpleProjection3D pr;
      pr.ObjectPlacementL() = _plOrigin;
      pr.ViewerPlacementL() = pen->GetPlacement();
      pr.Prepare();
      FLOATplane3D vRelative;
      pr.ProjectDirection(f3dMappingTranslation, vRelative);
      CMappingDefinition mdOriginal = bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
      CMappingDefinition mdTranslated = mdOriginal;

      mdTranslated.Translate(
          bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, vRelative);
      FLOAT fUOffset = mdTranslated.md_fUOffset-mdOriginal.md_fUOffset;
      FLOAT fVOffset = mdTranslated.md_fVOffset-mdOriginal.md_fVOffset;
      // for each selected polygon
      FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        // add the offsets to its mapping
        itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fUOffset+=fUOffset;
        itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fVOffset+=fVOffset;
      }
      // mark that document is changed
      pDoc->SetModifiedFlag();
      bRefreshView = TRUE;
      pDoc->UpdateAllViews( NULL);
      // reset mouse placement
      m_plMouseMove.pl_PositionVector = crRayHit.cr_vHit;
      m_plMouseMove.pl_OrientationAngle = ANGLE3D( 0, 0, 0);
      // refresh position page
      pDoc->RefreshCurrentInfoPage();
      break;
    }
  }

  // if recreating of primitive requested
  if( bRecreatePrimitive)
  {
    pDoc->RefreshPrimitivePage();
    pDoc->CreatePrimitive();
  }

  // if something has moved
  if( bObjectMoved)
  {
    if( (m_iaInputAction == IA_MOVING_SECOND_LAYER_IN_FLOOR_PLANE) ||
        (m_iaInputAction == IA_MOVING_SECOND_LAYER_IN_VIEW_PLANE) ||
        (m_iaInputAction == IA_ROTATING_SECOND_LAYER) )
    {
      // copy new placement to second layer
      pDoc->m_plSecondLayer = m_plMouseMove;
      pDoc->SnapToGrid( pDoc->m_plSecondLayer, m_fGridInMeters/GRID_DISCRETE_VALUES);
      theApp.m_vfpCurrent.vfp_plPrimitive = pDoc->m_plSecondLayer;
    }
    else if( (m_iaInputAction == IA_MOVING_ENTITY_SELECTION_IN_FLOOR_PLANE) ||
             (m_iaInputAction == IA_MOVING_ENTITY_SELECTION_IN_VIEW_PLANE) ||
             (m_iaInputAction == IA_ROTATING_ENTITY_SELECTION) )
    {
      ASSERT( pDoc->m_aSelectedEntityPlacements.Count()==pDoc->m_selEntitySelection.Count());
      if( pDoc->m_aSelectedEntityPlacements.Count()!=pDoc->m_selEntitySelection.Count()) return;
      // if there is no anchored entity in selection or if moving of
      // anchored entities is allowed
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        if( ((iten->GetFlags() & ENF_ANCHORED) != 0) &&
            (!GetChildFrame()->m_bAncoredMovingAllowed) ) return;
      }}

      INDEX ienCurrent = 0;
      CEntity *penBrush = NULL;
      // for each of the selected entities
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        if (iten->en_RenderType == CEntity::RT_BRUSH) {
          penBrush = &*iten;
        }
        if( (iten->GetParent()==NULL) || !(iten->GetParent()->IsSelected( ENF_SELECTED)) )
        {
          // set new entity placement
          CPlacement3D plEntityPlacement = pDoc->m_aSelectedEntityPlacements[ienCurrent];
          plEntityPlacement.RelativeToAbsoluteSmooth(m_plMouseMove);
          pDoc->SnapToGrid( plEntityPlacement, m_fGridInMeters/GRID_DISCRETE_VALUES);
          iten->SetPlacement(plEntityPlacement);
          pDoc->SetModifiedFlag();
        }
        ienCurrent++;
      }}
      if( penBrush != NULL) 
      {
        DiscardShadows( penBrush);
      }
      if(m_iaInputAction == IA_ROTATING_ENTITY_SELECTION)
      {
        // check for terrain updating
        {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
        {
          CLightSource *pls = iten->GetLightSource();
          if (pls!=NULL)
          {
            // if light is directional
            if(pls->ls_ulFlags &LSF_DIRECTIONAL)
            {
              CTerrain *ptrTerrain=GetTerrain();
              if(ptrTerrain!=NULL) ptrTerrain->UpdateShadowMap();
            }
          }
        }}
      }
    }
  }

  // if rotating/moving viewer (bRefreshView) or moving CSG layer, entities or polygon's
  // mapping (bObjectMoved) or editing primitive (bRecreatePrimitive)
  if( bRefreshView || bObjectMoved || bRecreatePrimitive)
  {
    // see in preferences if all views should be updated
    if( theApp.m_Preferences.ap_UpdateAllways || bRepaintImmediately ||
      (bCtrl && bLMB && pDoc->GetEditingMode()==POLYGON_MODE) )
    {
      // force immediatly repainting
      RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
      // update other views
      pDoc->UpdateAllViews( this);
    }
    else
    {
      // just invalidate
      Invalidate( FALSE);
    }
  }
  // set text describing data that is edited
  SetEditingDataPaneInfo( TRUE);

  // remember current mouse position as last pressed
  m_ptMouse = point;
}


/*
 * Called by document at the end of CSG
 */
void CWorldEditorView::AtStopCSG(void)
{
}

/*
 * Called by MFC from CWorldEditorDoc::UpdateAllViews().
 */
void CWorldEditorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
  // just invalidate the whole window area
	Invalidate(FALSE);
}

void CWorldEditorView::OnDropFiles(HDROP hDropInfo)
{
	CWorldEditorDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  // You can't drag anything while CSG is on
  if( bCSGOn)
  {
    return;
  }

  // get number of dropped files
  INDEX iNoOfFiles = DragQueryFile( hDropInfo, 0xFFFFFFFF, NULL, 0);

  // there can be only one file dropped
  if( iNoOfFiles != 1)
  {
    AfxMessageBox( L"You can drop only one file at a time.");
    return;
  }

	// buffer for dropped file name
  char chrFile[ 256];
  // place dropped file name into buffer
  DragQueryFileA( hDropInfo, 0, chrFile, 256);
  // create file name from buffer
  CTFileName fnDropped = CTString(chrFile);

  // don't allow document self-drop
  /*if( CTFileName(pDoc->GetPathName()) == fnDropped)
  {
    return;
  }*/

  // object to hold coordinates
  CPoint point;
  // get dropped coordinates
  DragQueryPoint( hDropInfo, &point);

  // try to
  try
  {
    // remove application path
    fnDropped.RemoveApplicationPath_t();
  }
  catch (const char* err_str)
  {
    AfxMessageBox( CString(err_str));
    return;
  }

  CPlacement3D plDrop;
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( point);
  // place the dropped object at hitted point
  plDrop.pl_PositionVector = crRayHit.cr_vHit;
  // reset angles
  plDrop.pl_OrientationAngle = ANGLE3D(0,0,0);

  // if the hit entity was a brush
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) ) {
    // get the hit normal in absolute space
    CSimpleProjection3D prBrushToAbsolute;
    prBrushToAbsolute.ObjectPlacementL() = crRayHit.cr_penHit->GetPlacement();
    prBrushToAbsolute.ViewerPlacementL() = CPlacement3D(
      FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
    prBrushToAbsolute.Prepare();

    FLOAT3D vHitNormal;
    prBrushToAbsolute.ProjectDirection(crRayHit.cr_pbpoBrushPolygon->bpo_pbplPlane->bpl_plAbsolute,
      vHitNormal);
    // if the normal is more horizontal
    if (Abs(vHitNormal(2))<0.5f) {
      // calculate angles to align -z axis towards the plane
      DirectionVectorToAngles(-vHitNormal, plDrop.pl_OrientationAngle);
    }
  }
  pDoc->SnapToGrid( plDrop, m_fGridInMeters/GRID_DISCRETE_VALUES);

  // if the dragged object is an entity class
  if (fnDropped.FileExt()==".ecl")
  {
    // create dragged entity
    CPlacement3D plEntity;
    CEntity *pen = NULL;
    try
    {
      extern BOOL _bInOnDraw; 
      _bInOnDraw = TRUE;
      pen = pDoc->m_woWorld.CreateEntity_t(plDrop, fnDropped);
      // prepare the entity
      pen->Initialize();
      _bInOnDraw = FALSE;
      // the drop was successful
      pDoc->SetModifiedFlag();
      // mark that document changeable was changed
      pDoc->m_chDocument.MarkChanged();
    }
    catch (const char *err_str)
    {
      _bInOnDraw = FALSE;
      AfxMessageBox(CString(err_str));
      if (pen!=NULL) {
        pen->Destroy();
      }
      // the drop was no successful
      return;
    }
    // deselect all entities
    pDoc->m_selEntitySelection.Clear();
    // if succeseful, switch to entity mode and only-select dropped entity
    pDoc->m_selEntitySelection.Select( *pen);
    // mark that selections have been changed
    pDoc->m_chSelections.MarkChanged();
    // switch to entity mode
    OnEntityMode();
    // and refresh property combo manualy calling on idle
    pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
    if( (pen->GetRenderType() == CEntity::RT_BRUSH) ||
        (pen->GetRenderType() == CEntity::RT_FIELDBRUSH) )
    {
      pMainFrame->m_CSGDesitnationCombo.OnIdle(0);
      pMainFrame->m_CSGDesitnationCombo.SelectBrushEntity(pen);
    }
    pDoc->SetModifiedFlag( TRUE);
    pDoc->m_chDocument.MarkChanged();
    // update all views
    pDoc->UpdateAllViews( this);
  }
  // if the dragged object is a world
  else if (fnDropped.FileExt()==".wld")
  {
    pDoc->StartTemplateCSG( plDrop, fnDropped, FALSE /*don't reset angles*/);
    // the drop was successful
    return;
  }
  else if (fnDropped.FileExt()==".tex")
  {
    // if we hit some entity and hitted entity is brush
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      // try to apply dropped texture to current selection
      try
      {
        // if polygon that is hit with mouse is selected
        if( crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED))
        {
          // paste dropped texture over polygon selection
          pDoc->PasteTextureOverSelection_t( fnDropped);
        }
        // otherwise
        else
        {
          // paste it only for drop-hitted polygon
          crRayHit.cr_pbpoBrushPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.SetData_t( fnDropped);
          // mark that document has been modified
          pDoc->SetModifiedFlag( TRUE);
          // mark that selections have been changed to update entity intersection properties
          pDoc->m_chSelections.MarkChanged();
          // update all views
          pDoc->UpdateAllViews( NULL);
        }
      }
      // if failed
      catch (const char *err_str)
      {
        // report error
        AfxMessageBox( CString(err_str));
        return;
      }
    }
    else
    {
      // inform user that drop can be achived only over polygons
      AfxMessageBox( L"You can drop textures only by hitting some polygon, but You hit some entity.");
      return;
    }
  }
  else
  {
    // the drop was not successful, report error
    AfxMessageBox( L"You can drop only textures, classes and worlds.");
    return;
  }
}

/*
 * Get pointer to the child frame of this view.
 */
CChildFrame *CWorldEditorView::GetChildFrame(void)
{
  return (CChildFrame *)GetParentFrame();
}

void CWorldEditorView::OnIsometricFront()
{
	m_ptProjectionType = CSlaveViewer::PT_ISOMETRIC_FRONT;
  Invalidate( FALSE);
}

void CWorldEditorView::OnIsometricBack()
{
	m_ptProjectionType = CSlaveViewer::PT_ISOMETRIC_BACK;
  Invalidate( FALSE);
}

void CWorldEditorView::OnIsometricBottom()
{
	m_ptProjectionType = CSlaveViewer::PT_ISOMETRIC_BOTTOM;
  Invalidate( FALSE);
}

void CWorldEditorView::OnIsometricLeft()
{
	m_ptProjectionType = CSlaveViewer::PT_ISOMETRIC_LEFT;
  Invalidate( FALSE);
}

void CWorldEditorView::OnIsometricRight()
{
	m_ptProjectionType = CSlaveViewer::PT_ISOMETRIC_RIGHT;
  Invalidate( FALSE);
}

void CWorldEditorView::OnIsometricTop()
{
	m_ptProjectionType = CSlaveViewer::PT_ISOMETRIC_TOP;
  Invalidate( FALSE);
}

void CWorldEditorView::OnPerspective()
{
	m_ptProjectionType = CSlaveViewer::PT_PERSPECTIVE;
  Invalidate( FALSE);
}


void CWorldEditorView::OnZoomLess()
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    return;
  }
  CWorldEditorDoc* pDoc = GetDocument();
  // create a slave viewer
  CSlaveViewer svViewer(GetChildFrame()->m_mvViewer, m_ptProjectionType,
    pDoc->m_plGrid, pdpValidDrawPort);

  // scale target distance by 2 - zoom out x2
  svViewer.ScaleTargetDistance( 2.0f);

  // copy slave viewer back to master
  GetChildFrame()->m_mvViewer = svViewer;
  // redraw all viewes
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnZoomMore()
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    return;
  }
  CWorldEditorDoc* pDoc = GetDocument();
  // create a slave viewer
  CSlaveViewer svViewer(GetChildFrame()->m_mvViewer, m_ptProjectionType,
    pDoc->m_plGrid, pdpValidDrawPort);

  // scale target distance by -1/2 - zoom in x2
  svViewer.ScaleTargetDistance( -0.5f);

  // copy slave viewer back to master
  GetChildFrame()->m_mvViewer = svViewer;
  // redraw all viewes
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::RotateOrStretchBrushVertex(FLOAT fDX, FLOAT fDY, BOOL bRotate)
{
  CWorldEditorDoc* pDoc = GetDocument();
  // project translation vector from view space to absolute space
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  if( pdpValidDrawPort == NULL) return;
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);
  // both selection and coordinate arrays must be sinchronized
  if( pDoc->m_avStartDragVertices.Count()!=pDoc->m_selVertexSelection.Count()) {
    ASSERT(FALSE);
    return;
  }

  // -------- Calculate BBox of all selected brush vertices
  FLOATaabbox3D boxVtx;
  // for each vertex
  {for( INDEX iVtx=0; iVtx<pDoc->m_avStartDragVertices.Count(); iVtx++)
  {
    // get new position
    boxVtx |= DOUBLEtoFLOAT(pDoc->m_avStartDragVertices[ iVtx]);
  }}
  
  // create dummy placement
  CPlacement3D plDummy;
  plDummy.pl_PositionVector = boxVtx.Center();
  DOUBLE3D vCenter = FLOATtoDOUBLE(plDummy.pl_PositionVector);
  plDummy.pl_OrientationAngle = ANGLE3D(0.0f, 0.0f, 0.0f);
  // use trackball method for rotation
  if( bRotate)
  {
    svViewer.RotatePlacement_TrackBall( plDummy, -fDX, -fDY, 0);
  }

  // get rotation matrix
  FLOATmatrix3D t3dRotation;
  MakeRotationMatrixFast( t3dRotation, plDummy.pl_OrientationAngle);

  DOUBLEmatrix3D t3ddRotation;
  t3ddRotation = FLOATtoDOUBLE(t3dRotation);

  FLOAT fFactor=1.0f;
  if( !bRotate)
  {
    FLOAT fDiag=boxVtx.Size().Length();
    // fDY is actually current view's zoom factor
    fFactor=(fDiag+fDX/fDY)/fDiag;
  }

  // for each vertex
  INDEX iVtx = 0;
  {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
  {
    DOUBLE3D vRelative = pDoc->m_avStartDragVertices[ iVtx]-vCenter;

    // rotate vertex around bbox center to get new position
    DOUBLE3D vNew = vRelative*fFactor*t3ddRotation;

    // snap resulting vertex coordinate
    SnapVector(vNew);

    // set its new position
    itbvx->SetAbsolutePosition(vNew+vCenter);
    iVtx++;
  }}

  // update sectors
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
}

FLOAT3D CWorldEditorView::ProjectVectorToWorldSpace(FLOAT fDX,FLOAT fDY,FLOAT fDZ)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CPlacement3D plVector0(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  CPlacement3D plVector1(FLOAT3D(fDX,fDY,fDZ), ANGLE3D(0,0,0));

  CDrawPort *pdp = GetDrawPort();
  if( pdp == NULL)
  {
    ASSERTALWAYS("Invalid draw port found!");
    return FLOAT3D( 0.0f, 0.0f, 0.0f);
  }

  // project translation vector from view space to absolute space
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid, pdp);
  CPlacement3D plViewer = svViewer.GetViewerPlacement();
  plVector0.RelativeToAbsolute(plViewer);
  plVector1.RelativeToAbsolute(plViewer);

  // obtain translation vector from placements
  FLOAT3D vDelta = (plVector1.pl_PositionVector-plVector0.pl_PositionVector);
  return vDelta;
}

void CWorldEditorView::DragBrushVertex(FLOAT fDX,FLOAT fDY,FLOAT fDZ)
{
  CWorldEditorDoc* pDoc = GetDocument();
  
  // obtain 3d translation vector
  DOUBLE3D vDelta = FLOATtoDOUBLE( ProjectVectorToWorldSpace( fDX, fDY, fDZ));

  // both selection and coordinate arrays must be sinchronized
  if( pDoc->m_avStartDragVertices.Count()!=pDoc->m_selVertexSelection.Count()) {
    ASSERT(FALSE);
    return;
  }

  // ---- apply movement

  // for each vertex
  INDEX iVtx = 0;
  {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
  {
    // get new position
    DOUBLE3D vNew = pDoc->m_avStartDragVertices[ iVtx] + vDelta;
    SnapVector( vNew);
    // set its new position
    itbvx->SetAbsolutePosition(vNew);
    iVtx++;
  }}

  // update sectors
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
}

void CWorldEditorView::DragVerticesOnPrimitive(FLOAT fDX,FLOAT fDY,FLOAT fDZ, BOOL bAbsolute,
                                               BOOL bSnap/*=TRUE*/)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CPlacement3D plVector0(FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  CPlacement3D plVector1(FLOAT3D(fDX,fDY,fDZ), ANGLE3D(0,0,0));
  // project translation vector from view space to absolute space
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  if( pdpValidDrawPort == NULL) return;
  CSlaveViewer svViewer( GetChildFrame()->m_mvViewer, m_ptProjectionType,
                         pDoc->m_plGrid, pdpValidDrawPort);
  CPlacement3D plViewer = svViewer.GetViewerPlacement();
  plVector0.RelativeToAbsolute(plViewer);
  plVector1.RelativeToAbsolute(plViewer);
  // project translation vector from absolute space to second layer space
  plVector0.AbsoluteToRelative(pDoc->m_plSecondLayer);
  plVector1.AbsoluteToRelative(pDoc->m_plSecondLayer);
  // extract translation vector from placements
  DOUBLE3D vDelta = FLOATtoDOUBLE(plVector1.pl_PositionVector)-
                    FLOATtoDOUBLE(plVector0.pl_PositionVector);

  theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Lock();
  CDynamicArray<CObjectVertex> &aVtx = theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors[0].osc_aovxVertices;
  theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Unlock();

  aVtx.Lock();
  DOUBLE3D vClosest;
  DOUBLE3D vFirstInSelection;
  BOOL bHasSelection = FALSE;
  // see if any of the vertices is selected
  INDEX iVtx=0;
  for( ; iVtx<aVtx.Count(); iVtx++)
  {
    if( aVtx[ iVtx].ovx_ulFlags & OVXF_CLOSEST) vClosest = aVtx[ iVtx];
    if( (aVtx[ iVtx].ovx_ulFlags & OVXF_SELECTED) && !bHasSelection)
    {
      vFirstInSelection = aVtx[ iVtx];
      bHasSelection = TRUE;
    }
  }
  DOUBLE3D vVectorForAdding;
  // if given delta vector should be applied absolutly
  if( bAbsolute)
  {
    vVectorForAdding = vDelta;
  }
  else
  {
    if( bHasSelection)
    {
      vVectorForAdding = m_vStartDragO3DVertice+vDelta-vFirstInSelection;
    }
    else
    {
      vVectorForAdding = m_vStartDragO3DVertice+vDelta-vClosest;
    }
  }

  // apply movement
  for( iVtx=0; iVtx<aVtx.Count(); iVtx++)
  {
    if( ((aVtx[ iVtx].ovx_ulFlags & OVXF_CLOSEST) && !bHasSelection) ||
        ((aVtx[ iVtx].ovx_ulFlags & OVXF_SELECTED) && bHasSelection) )
    {
      aVtx[ iVtx] += vVectorForAdding;
      
      if( pDoc->m_bAutoSnap && bSnap)
      {
        // snap resulting vertex coordinate
        Snap(aVtx[iVtx](1), m_fGridInMeters/GRID_DISCRETE_VALUES);
        Snap(aVtx[iVtx](2), m_fGridInMeters/GRID_DISCRETE_VALUES);
        Snap(aVtx[iVtx](3), m_fGridInMeters/GRID_DISCRETE_VALUES);
      }
      else
      {
        Snap(aVtx[iVtx](1), SNAP_DOUBLE_CM);
        Snap(aVtx[iVtx](2), SNAP_DOUBLE_CM);
        Snap(aVtx[iVtx](3), SNAP_DOUBLE_CM);
      }
    }
  }
  aVtx.Unlock();
  pDoc->ConvertObject3DToBrush(theApp.m_vfpCurrent.vfp_o3dPrimitive, TRUE);
  // redraw all viewes
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnMoveUp()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( (pDoc->m_pwoSecondLayer != NULL) &&
      (pDoc->m_bPrimitiveMode) &&
      (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) &&
      ((!theApp.m_vfpCurrent.vfp_bAutoCreateMipBrushes)||
      (theApp.m_vfpCurrent.vfp_ptPrimitiveType != PT_TERRAIN)) )
  {
    MarkClosestVtxOnPrimitive( FALSE);
    DragVerticesOnPrimitive( 0.0, 0.0, m_fGridInMeters/GRID_DISCRETE_VALUES, TRUE, FALSE);
  }
  // redraw all viewes
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnMoveDown()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( (pDoc->m_pwoSecondLayer != NULL) &&
      (pDoc->m_bPrimitiveMode) &&
      (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) &&
      ((!theApp.m_vfpCurrent.vfp_bAutoCreateMipBrushes)||
      (theApp.m_vfpCurrent.vfp_ptPrimitiveType != PT_TERRAIN)) )
  {
    MarkClosestVtxOnPrimitive( FALSE);
    DragVerticesOnPrimitive( 0.0, 0.0, -m_fGridInMeters/GRID_DISCRETE_VALUES, TRUE, FALSE);
  }
  // redraw all viewes
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnMeasurementTape()
{
  m_vpViewPrefs.m_bMeasurementTape = !m_vpViewPrefs.m_bMeasurementTape;
  Invalidate( FALSE);
}

void CWorldEditorView::OnUpdateMeasurementTape(CCmdUI* pCmdUI)
{
  if( m_ptProjectionType == CSlaveViewer::PT_PERSPECTIVE)
  {
    pCmdUI->Enable( FALSE);
    pCmdUI->SetCheck( FALSE);
  }
  else
  {
    pCmdUI->Enable( TRUE);
    pCmdUI->SetCheck( m_vpViewPrefs.m_bMeasurementTape);
  }
}

void CWorldEditorView::OnKeyBuffer(UINT nID)
{
  // get difference of key "1", which means buffer index
  INDEX iPreferencesBuffer = nID-ID_BUFFER01;

  // copy selected preferences to view's rendering preferences
  m_vpViewPrefs = theApp.m_vpViewPrefs[ iPreferencesBuffer];
  // see the change
  Invalidate( FALSE);
}

void CWorldEditorView::OnKeyEditBuffer(UINT nID)
{
  // get difference of key "1", which means buffer index
  INDEX iPreferencesBuffer = nID-ID_EDIT_BUFFER01;

  CDlgRenderingPreferences dlg( iPreferencesBuffer);
  // if dialog ended with cancel or esc, dont switch to changed preferences
  if( dlg.DoModal() != IDOK)
  {
    // don't set new preferences
    return;
  }
  // copy selected preferences to view's rendering preferences
  m_vpViewPrefs = theApp.m_vpViewPrefs[ iPreferencesBuffer];
  // see the change
  Invalidate( FALSE);
}

void CWorldEditorView::OnCircleModes()
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

  switch( pDoc->GetEditingMode())
  {
  case ENTITY_MODE:  { pDoc->SetEditingMode( SECTOR_MODE); break;};
  case SECTOR_MODE:  { pDoc->SetEditingMode( POLYGON_MODE); break;};
  case POLYGON_MODE: { pDoc->SetEditingMode( VERTEX_MODE); break;};
  case VERTEX_MODE:  { pDoc->SetEditingMode( TERRAIN_MODE); break;};
  case TERRAIN_MODE: { pDoc->SetEditingMode( ENTITY_MODE); break;};
  default: { FatalError("Unknown editing mode."); break;};
  }
}

void CWorldEditorView::OnUpdateCircleModes(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pCmdUI->Enable( pDoc->GetEditingMode() != CSG_MODE);
}

void CWorldEditorView::OnDeselectAll()
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->DeselectAll();

  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  // if we are in triangularisation primitive mode
  if( (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) &&
      (bCSGOn && pDoc->m_bPrimitiveMode) )
  {
    theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Lock();
    CDynamicArray<CObjectVertex> &aVtx = theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors[0].osc_aovxVertices;
    theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.Unlock();
    aVtx.Lock();
    // reset selected flags for all vertices
    for( INDEX iVtx=0; iVtx<aVtx.Count(); iVtx++)
    {
      aVtx[ iVtx].ovx_ulFlags &= ~OVXF_SELECTED;
    }
    aVtx.Unlock();
    // update all views
    pDoc->UpdateAllViews(NULL);
  }
}
void CWorldEditorView::RemoveFromLinkedChain(CEntity *pen)
{
  CTFileName fnDropClass;
  CTString strTargetProperty;
  CEntityProperty *penpProperty;

  CEntity &enOnly = *pen;
  // obtain drop class and target property name
  if( !enOnly.DropsMarker( fnDropClass, strTargetProperty)) return;
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  CEntity *penHisTarget = ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer);
  // or ptr is NULL
  if( penHisTarget == NULL) return;

  CEntity *penCurrent = pen;
  INDEX iInfiniteLoopProtector = 0;
  // loop forever
  FOREVER
  {
    if( !penCurrent->DropsMarker( fnDropClass, strTargetProperty)) return;
    penpProperty = penCurrent->PropertyForName( strTargetProperty);
    CEntity *penNext = ENTITYPROPERTY( penCurrent, penpProperty->ep_slOffset, CEntityPointer);
    if( penNext == NULL) return;
    if( penNext == pen) break;
    // jump to next in chain
    penCurrent = penNext;
    iInfiniteLoopProtector ++;
    if( iInfiniteLoopProtector >= 256) return;
  }
  if( !penCurrent->DropsMarker( fnDropClass, strTargetProperty)) return;
  penpProperty = penCurrent->PropertyForName( strTargetProperty);
  ENTITYPROPERTY( penCurrent, penpProperty->ep_slOffset, CEntityPointer) = penHisTarget;
}

void CWorldEditorView::OnDeleteEntities()
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	CWorldEditorDoc* pDoc = GetDocument();

  if( pDoc->GetEditingMode() == CSG_MODE)  return;

  if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    if( pDoc->m_selSectorSelection.Count() != 0)
    {
      if( theApp.m_Preferences.ap_bSaveUndoForDelete)
      {
        pDoc->RememberUndo();
      }
      pDoc->ClearSelections( ST_SECTOR);
      pDoc->m_woWorld.DeleteSectors( pDoc->m_selSectorSelection, TRUE);
    }
  }
  else
  {
    // if any of selected entities is anchored, anchored operation flag must be allowed or deleting is disabled
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        if( (iten->GetFlags() & ENF_ANCHORED) && !GetChildFrame()->m_bAncoredMovingAllowed) return;
      }}
    }

    if( pDoc->m_selEntitySelection.Count() != 0)
    {
      if( theApp.m_Preferences.ap_bSaveUndoForDelete)
      {
        pDoc->RememberUndo();
      }
    }
    // check for deleting terrain
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      CEntity &en=*iten;
      // if it is terrain
      if(en.GetRenderType()==CEntity::RT_TERRAIN)
      {
        // if it is selected terrain
        if(pDoc->m_ptrSelectedTerrain==en.GetTerrain())
        {
          pDoc->m_ptrSelectedTerrain=NULL;
          theApp.m_ctTerrainPage.MarkChanged();
          break;
        }
      }
    }}

    // for each of the selected entities
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      if( pDoc->m_cenEntitiesSelectedByVolume.IsMember(iten))
        pDoc->m_cenEntitiesSelectedByVolume.Remove(iten);
    }}
    // for each of the selected entities
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      RemoveFromLinkedChain(iten);
    }}
    // clear selections
    pDoc->m_selVertexSelection.Clear();
    pDoc->m_selSectorSelection.Clear();
    pDoc->m_selPolygonSelection.Clear();
    // delete all selected entities
    pDoc->m_woWorld.DestroyEntities( pDoc->m_selEntitySelection);
  }
  // mark that document was changed (for saving)
  pDoc->SetModifiedFlag();
  // mark that document changeable was changed to update CSG target combo box
  pDoc->m_chDocument.MarkChanged();
  // mark that selections have been changed to update entity intersection properties
  pDoc->m_chSelections.MarkChanged();
  // update all views
  pDoc->UpdateAllViews(NULL);
}

void CWorldEditorView::OnUpdateDeleteEntities(CCmdUI* pCmdUI)
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    pCmdUI->Enable( IsDeleteEntityEnabled());
  }
  else if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    pCmdUI->Enable( pDoc->m_woWorld.CanCopySectors( pDoc->m_selSectorSelection));
  }
  else
  {
    pCmdUI->Enable( FALSE);
  }
}

BOOL CWorldEditorView::IsDeleteEntityEnabled(void)
{
  CWorldEditorDoc* pDoc = GetDocument();
  // enable button if we are in entity mode and there is at least 1 brush entity unselected
  BOOL bEnable = FALSE;
  if( (pDoc->GetEditingMode() == ENTITY_MODE) &&
      (pDoc->m_selEntitySelection.Count() != 0) )
  {
    // for all of the world's entities
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      // if the entity is brush
      if (iten->GetRenderType() == CEntity::RT_BRUSH)
      {
        // if this is brush entity and is not selected
        if( !iten->IsSelected( ENF_SELECTED))
        {
          // enable delete entity button
          bEnable = TRUE;
          break;
        }
      }
    }
  }
  return bEnable;
}

void CWorldEditorView::UpdateCursor(void)
{
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
	CWorldEditorDoc* pDoc = GetDocument();

  CTerrainLayer *ptlLayer=GetLayer();

  if( theApp.m_bMeasureModeOn)
  {
    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_CROSS));
  }
  else if( theApp.m_bCutModeOn)
  {
    if( pDoc->GetEditingMode() == ENTITY_MODE)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_MIRROR));
    }
    else
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_CUT_LINE));
    }
  }
  else if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL && ptlLayer!=NULL)
  {
    if(bCtrl&&bAlt)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_PICK));
    }
    else if(theApp.m_iTerrainEditMode==TEM_LAYER && ptlLayer->tl_ltType==LT_TILE)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_TILE_PAINTING));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_ERASE)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_ERASE));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_SMOOTH)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_SMOOTH));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_FILTER)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_FILTER));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_MINIMUM)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_MIN));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_MAXIMUM)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_MAX));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_FLATTEN)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_FLATTEN));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_POSTERIZE)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_POSTERIZE));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_RND_NOISE)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_RND_NOISE));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_CONTINOUS_NOISE)
    {
      SetCursor( AfxGetApp()->LoadCursor(IDC_TE_CONTINOUS_NOISE));
    }
    else if(theApp.m_iTerrainBrushMode==TBM_PAINT)
    {
      if(theApp.m_iTerrainEditMode==TEM_HEIGHTMAP)
      {
        SetCursor( AfxGetApp()->LoadCursor(IDC_TE_HEIGHTMAP));
      }
      else if(theApp.m_iTerrainEditMode==TEM_LAYER)
      {
        SetCursor( AfxGetApp()->LoadCursor(IDC_TE_LAYER_PAINT));
      }
    }
  }
  else
  {
    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
  }
}

BOOL CWorldEditorView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
  return TRUE;
}

void CWorldEditorView::OnTakeSs()
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    return;
  }
  CImageInfo iiImageInfo;
  /* First remember current screen-shot in memory. */
  if( pdpValidDrawPort->Lock())
  {
    RenderView( pdpValidDrawPort);
    pdpValidDrawPort->Unlock();
  }

  // grab screen creating image info
  pdpValidDrawPort->GrabScreen( iiImageInfo, 1);

  // ask for save screen shot name
  CTFileName fnDocName = CTString( CStringA(GetDocument()->GetPathName()));
  CTFileName fnScreenShoot =  _EngineGUI.FileRequester(
    "Choose file name for screen shoot", FILTER_TGA FILTER_ALL FILTER_END, KEY_NAME_SCREEN_SHOT_DIR,
    "ScreenShots\\", fnDocName.FileName()+"xx"+".tga", NULL, FALSE);
  if( fnScreenShoot == "") return;

  // try to
  try {
    // save screen shot as TGA
    iiImageInfo.SaveTGA_t( fnScreenShoot);
  } // if failed
  catch (const char *strError) {
    // report error
    AfxMessageBox(CString(strError));
  }
}

void CWorldEditorView::OnEntityMode()
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->SetEditingMode( ENTITY_MODE);
}

void CWorldEditorView::OnUpdateEntityMode(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pCmdUI->Enable( pDoc->GetEditingMode() != CSG_MODE);
}

void CWorldEditorView::OnTerrainMode()
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->SetEditingMode( TERRAIN_MODE);
}

void CWorldEditorView::OnUpdateTerrainMode(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pCmdUI->Enable( pDoc->GetEditingMode() != CSG_MODE);
}

void CWorldEditorView::OnSectorMode()
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->SetEditingMode( SECTOR_MODE);
}

void CWorldEditorView::OnUpdateSectorMode(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pCmdUI->Enable( pDoc->GetEditingMode() != CSG_MODE);
}

void CWorldEditorView::OnPolygonMode()
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->SetEditingMode( POLYGON_MODE);
}

void CWorldEditorView::OnUpdatePolygonMode(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pCmdUI->Enable( pDoc->GetEditingMode() != CSG_MODE);
}

void CWorldEditorView::OnEditCopy()
{
  EditCopy( FALSE);
}

void CWorldEditorView::OnVertexMode() 
{
	CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
  pDoc->OnCsgCancel();
}

void CWorldEditorView::EditCopy( BOOL bAlternative)
{
	CWorldEditorDoc* pDoc = GetDocument();
  BOOL bCopyWorld = FALSE;
  // if we are in entity mode
  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    // if we have some entities selected
    if( pDoc->m_selEntitySelection.Count() != 0)
    {
      if( bAlternative)
      {
        CDynamicContainer<CEntity> selClones;
        CPlacement3D plPaste = GetMouseInWorldPlacement();
        // for all selected entities
        {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
        {
          CEntity *pClone = pDoc->m_woWorld.CopyEntityInWorld( *iten, plPaste);
          selClones.Add( pClone);
        }}

        // clear old selection and select clones
        pDoc->m_selEntitySelection.Clear();
        {FOREACHINDYNAMICCONTAINER( selClones, CEntity, itenClone)
        {
          pDoc->m_selEntitySelection.Select( *itenClone);
        }}

        pDoc->m_chSelections.MarkChanged();
        pDoc->SetModifiedFlag();
        pDoc->UpdateAllViews( NULL);
      }
      else
      {
        // calculate bounding boxes of all selected entities
        FLOATaabbox3D boxEntities;
        // for all selected entities
        {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
        {
          FLOATaabbox3D boxCurrentEntity;
          iten->GetSize( boxCurrentEntity);
          boxCurrentEntity += iten->GetPlacement().pl_PositionVector;
          boxEntities |= boxCurrentEntity;
        }}

        // average for all vectors of entities
        FLOAT3D vCenter = boxEntities.Center();
        vCenter(2) = boxEntities.Min()(2);
        CPlacement3D plCenter = CPlacement3D( -vCenter, ANGLE3D(0,0,0) );

        FLOAT3D vSize = boxEntities.Size();
        FLOAT fMaxEdge = Max( Max(vSize(1), vSize(2)), vSize(3));
        FLOAT fSnapValue;
        if( theApp.m_bDecadicGrid)
        {
          INDEX iLog = (INDEX) (Log2( fMaxEdge/GRID_DISCRETE_VALUES)/Log2(10.0f));
          fSnapValue = FLOAT(pow(10.0f, iLog));
        }
        else
        {
          INDEX iLog = (INDEX) Log2( fMaxEdge/GRID_DISCRETE_VALUES);
          fSnapValue = FLOAT(pow(2.0f, iLog));
        }
        pDoc->SnapToGrid( plCenter, fSnapValue);

        // create world to receive selected entities
        CWorld woEntityClipboard;
        woEntityClipboard.SetBackgroundColor( C_WHITE);
        woEntityClipboard.SetDescription( "No description");
        theApp.m_ctLastCopyType = CT_ENTITY;

        // copy entities in selection
        CEntitySelection senDummy;
        woEntityClipboard.CopyEntities(pDoc->m_woWorld, pDoc->m_selEntitySelection,
          senDummy, plCenter);
        senDummy.Clear();

        // try to
        try
        {
          // save entity clipboard world
          woEntityClipboard.Save_t(CTString("Temp\\ClipboardEntityWorld.wld"));
        }
        catch (const char *strError)
        {
          AfxMessageBox( CString(strError));
          return;
        }
      }
    }
    else
    {
      // we will copy whole world
      bCopyWorld = TRUE;
    }
  }
  // if we are in polygon mode
  else if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // obtain point in the world where mouse pointed last time it was moved
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse, FALSE, FALSE, FALSE);
    // if mouse is over polygon
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      // remember selected polygon's parameters to application
      theApp.m_pbpoClipboardPolygon->CopyProperties( *crRayHit.cr_pbpoBrushPolygon);
      if( bAlternative)
      {
        theApp.m_ctLastCopyType = CT_POLYGON_PROPERTIES_ALTERNATIVE;
        CopyMapping(crRayHit.cr_pbpoBrushPolygon);
      }
      else
      {
        theApp.m_ctLastCopyType = CT_POLYGON_PROPERTIES;
      }
    }
    else
    {
      // we will copy whole world
      bCopyWorld = TRUE;
    }
  }
  // if we are in sector mode
  else if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    // if there are no sectors selected
    if( pDoc->m_selSectorSelection.Count() == 0)
    {
      // we will copy whole world
      bCopyWorld = TRUE;
    }
    // copy selected sectors
    else
    {
      if( !pDoc->m_woWorld.CanCopySectors( pDoc->m_selSectorSelection))
      {
        WarningMessage( "Error: Can't copy selected sectors. "
                        "All sectors must be in same brush and same mip factor.");
        return;
      }
      // create world to receive selected sectors
      CWorld woClipboard;
      woClipboard.SetBackgroundColor( C_WHITE);
      woClipboard.SetDescription( "No description");
      // now create world base entity
      CEntity *penWorldBase;
      CPlacement3D plWorld = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
      // try to
      try
      {
        penWorldBase = woClipboard.CreateEntity_t(plWorld, CTFILENAME("Classes\\WorldBase.ecl"));
      }
      // catch and
      catch (const char *err_str)
      {
        // report errors
        WarningMessage( err_str);
        return;
      }
      // prepare the entity
      penWorldBase->Initialize();
      EFirstWorldBase eFirstWorldBase;
      penWorldBase->SendEvent( eFirstWorldBase);
      CEntity::HandleSentEvents();
      pDoc->m_woWorld.CopySectors( pDoc->m_selSectorSelection, penWorldBase, !bAlternative);
      try
      {
        // save world to clipboard
        woClipboard.Save_t(CTString("Temp\\ClipboardSectorWorld.wld"));
      }
      catch (const char *strError)
      {
        AfxMessageBox( CString(strError));
        return;
      }
      theApp.m_ctLastCopyType = CT_SECTOR;
    }
  }

  // if we should copy the whole world
  if( bCopyWorld)
  {
    try
    {
      // save world into clipboard file
      pDoc->m_woWorld.Save_t( (CTString)"Temp\\ClipboardWorld.wld");
    }
    catch (const char *strError)
    {
      AfxMessageBox( CString(strError));
    }
    theApp.m_ctLastCopyType = CT_WORLD;
  }
}

// obtain point in the world where mouse pointed last time it was moved
CPlacement3D CWorldEditorView::GetMouseInWorldPlacement(void)
{
	CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);

  // get hitted point
  CPlacement3D plMouse;
  // if any kind of entity was hitted with mouse
  if( crRayHit.cr_penHit != NULL)
  {
    // hitted entity exist
    plMouse.pl_PositionVector = crRayHit.cr_vHit;
  }
  // if none of entities (brushes) was hitted with mouse
  else
  {
    // set position in front of viewer, at the placement of viewing point (target)
    plMouse = GetChildFrame()->m_mvViewer.GetTargetPlacement();
  }
  // reset angles
  plMouse.pl_OrientationAngle = ANGLE3D(0,0,0);
  // snap placement
  pDoc->SnapToGrid( plMouse, m_fGridInMeters/GRID_DISCRETE_VALUES);
  return plMouse;
}

void CWorldEditorView::OnEditPaste()
{
	CWorldEditorDoc* pDoc = GetDocument();

  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  // You can't paste anything while CSG is on
  if( bCSGOn)
  {
    return;
  }

  BOOL bHitModels = FALSE;
  if( theApp.m_ctLastCopyType == CT_ENTITY) bHitModels = TRUE;

  CPlacement3D plPaste = GetMouseInWorldPlacement();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse, FALSE, bHitModels, FALSE);
  // if last copy operation was with entities
  if( theApp.m_ctLastCopyType == CT_ENTITY)
  {
    // switch to entity mode
    pDoc->SetEditingMode( ENTITY_MODE);
    // create world to receive pasted entities
    CWorld woEntityClipboard;
    // try to
    try
    {
      // load clipboard entity World
      woEntityClipboard.Load_t( (CTString)"Temp\\ClipboardEntityWorld.wld");
    }
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
      return;
    }
    // deselect all entities
    pDoc->m_selEntitySelection.Clear();

    // make container of entities to copy
    CDynamicContainer<CEntity> cenToCopy;
    cenToCopy = woEntityClipboard.wo_cenEntities;
    // remove empty brushes from it
    {FOREACHINDYNAMICCONTAINER(woEntityClipboard.wo_cenEntities, CEntity, iten)
    {
      if( iten->IsEmptyBrush() && (iten->GetFlags()&ENF_ZONING))
      {
        cenToCopy.Remove(iten);
      }
    }}
    // copy entities in clipboard
    pDoc->m_woWorld.CopyEntities(woEntityClipboard, cenToCopy,
      pDoc->m_selEntitySelection, plPaste);

    pDoc->m_chSelections.MarkChanged();
    pDoc->SetModifiedFlag();
    pDoc->UpdateAllViews( NULL);
  }
  // if last copy operation was with polygon properties
  else if( theApp.m_ctLastCopyType == CT_POLYGON_PROPERTIES)
  {
    // if mouse is over polygon
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      if( crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED))
      {
        FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
        {
          // apply remembered polygon's parameters to selection
          itbpo->CopyTextures( *theApp.m_pbpoClipboardPolygon);
        }
      }
      else
      {
        // apply remembered polygon's parameters to polygon under mouse
        crRayHit.cr_pbpoBrushPolygon->CopyTextures( *theApp.m_pbpoClipboardPolygon);
      }
    }
    pDoc->m_chSelections.MarkChanged();
    pDoc->SetModifiedFlag();
    pDoc->UpdateAllViews( NULL);
  }
  // if last copy operation was with whole world
  else if( theApp.m_ctLastCopyType == CT_WORLD)
  {
    // load world from clipboard file and start template CSG
    pDoc->StartTemplateCSG( plPaste, (CTString)"Temp\\ClipboardWorld.wld");
  }
  // if last copy operation was with selected sectors
  else if( theApp.m_ctLastCopyType == CT_SECTOR)
  {
    // load world from sectors clipboard file and start template CSG
    pDoc->StartTemplateCSG( plPaste, (CTString)"Temp\\ClipboardSectorWorld.wld");
  }
}

void ProcesWEDConsoleShortcuts( MSG *pMsg)
{
  if( pMsg->message==WM_KEYDOWN)
  {
    BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
    if( !bShift && pMsg->wParam == VK_F2) _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\F2.ini\"");
    if( !bShift && pMsg->wParam == VK_F3) _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\F3.ini\"");
    if( !bShift && pMsg->wParam == VK_F4) _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\F4.ini\"");
    if( bShift && (pMsg->wParam == VK_F2)) _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\Shift_F2.ini\"");
    if( bShift && (pMsg->wParam == VK_F3)) _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\Shift_F3.ini\"");
    if( bShift && (pMsg->wParam == VK_F4)) _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\Shift_F4.ini\"");
  }
  if( pMsg->message==WM_MBUTTONDOWN || pMsg->message==WM_MBUTTONDBLCLK )
  {
    _pShell->Execute( "include \"Scripts\\WorldEditorKeys\\MiddleMouse.ini\"");
  }
}

BOOL CWorldEditorView::PreTranslateMessage(MSG* pMsg)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get csg and primitive flags
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  BOOL bPrimitive = pDoc->m_bPrimitiveMode;
  // get editting mode flags
  BOOL bEntityMode = pDoc->GetEditingMode() == ENTITY_MODE;
  BOOL bSectorMode = pDoc->GetEditingMode() == SECTOR_MODE;
  BOOL bPolygonMode = pDoc->GetEditingMode() == POLYGON_MODE;
  BOOL bTerrainMode = pDoc->GetEditingMode() == TERRAIN_MODE;
  // get key statuses
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  // get mouse button statuses
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;
  BOOL bRMB = (GetKeyState( VK_RBUTTON)&0x8000) != 0;

  // process scripts that are invoked on shortcuts from within WED
  ProcesWEDConsoleShortcuts( pMsg);

  // Ctrl +'A':
  // - entity -> select all in volume
  // - sector -> select all entities
  // - polygon -> select all polygons in sector(s)

  // Alt+'A'
  // - sector -> show all sectors
  // - entity -> show all entities
  // - polygon-> auto fit mapping
  // - vertex->  auto fit mapping

  // 'A':
  // - select by volume -> allign volume
  // - entity -> allign entity selection or world
  // - primitive -> allign primitive to screen
  // - sector -> allign visible sectors to screen

  if( ((pMsg->message==WM_KEYDOWN) || (pMsg->message==WM_SYSKEYDOWN)) &&
      ((int)pMsg->wParam=='A') )
  {
    // setup mouse - ray hit information
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
    m_penEntityHitOnContext = crRayHit.cr_penHit;
    if( (crRayHit.cr_penHit != NULL) && (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      m_pbpoRightClickedPolygon = crRayHit.cr_pbpoBrushPolygon;
    }

    if( bCSGOn && !bShift)
    {
      if( bCtrl && bPrimitive)
      {
        OnAlignPrimitive();
      }
      else
      {
        FLOATaabbox3D boxSecondLayer;
        FOREACHINDYNAMICCONTAINER(pDoc->m_pwoSecondLayer->wo_cenEntities, CEntity, iten)
        {
          FLOATaabbox3D box;
          iten->GetSize( box);
          
          FLOAT fMinX = box.Min()(1);
          FLOAT fMinY = box.Min()(2);
          FLOAT fMinZ = box.Min()(3);
          FLOAT fMaxX = box.Max()(1);
          FLOAT fMaxY = box.Max()(2);
          FLOAT fMaxZ = box.Max()(3);
          FLOAT3D avCorners[8];
          avCorners[0] = FLOAT3D(fMinX,fMinY,fMinZ);
          avCorners[1] = FLOAT3D(fMaxX,fMinY,fMinZ);
          avCorners[2] = FLOAT3D(fMaxX,fMinY,fMaxZ);
          avCorners[3] = FLOAT3D(fMinX,fMinY,fMaxZ);
          avCorners[4] = FLOAT3D(fMinX,fMaxY,fMinZ);
          avCorners[5] = FLOAT3D(fMaxX,fMaxY,fMinZ);
          avCorners[6] = FLOAT3D(fMaxX,fMaxY,fMaxZ);
          avCorners[7] = FLOAT3D(fMinX,fMaxY,fMaxZ);
          
          for( INDEX iVtx=0; iVtx<8; iVtx++)
          {
            CPlacement3D plToProject = CPlacement3D( avCorners[iVtx], ANGLE3D(0,0,0));
            plToProject.RelativeToAbsolute( pDoc->m_plSecondLayer);
            boxSecondLayer |= FLOATaabbox3D(plToProject.pl_PositionVector);
          }
        }
        AllignBox( boxSecondLayer);
      }
    }
    else if( pDoc->GetEditingMode() == ENTITY_MODE && !bShift)
    {
      if( bCtrl && bAlt)
      {
        OnSelectAllSectors();
      }
      else if( bCtrl)
      {
        pDoc->OnSelectAllInVolume();
      }
      else if( bAlt)
      {
        pDoc->OnShowAllEntities();
      }
      else if( pDoc->m_bBrowseEntitiesMode)
      {
        OnAlignVolume();
      }
      else
      {
        CenterSelected();
      }
    }
    else if(pDoc->GetEditingMode() == SECTOR_MODE && !bShift)
    {
      if( bCtrl && bAlt)
      {
        OnSelectAllPolygons();
      }
      else if( bCtrl)
      {
        OnSelectAllEntitiesInSectors();
      }
      else if( bAlt)
      {
        pDoc->OnShowAllSectors();
      }
      else
      {
        CenterSelected();
      }
    }
    else if( pDoc->GetEditingMode() == POLYGON_MODE)
    {
      if( bCtrl && bAlt && !bShift)
      {
        OnSelectAllVertices();
      }
      else if( bCtrl && !bShift && !bAlt)
      {
        if( (crRayHit.cr_penHit != NULL) && (crRayHit.cr_pbpoBrushPolygon != NULL) )
        {
          // select all polygons in hitted sector
          CBrushSector *pbscSector = crRayHit.cr_pbpoBrushPolygon->bpo_pbscSector;
          FOREACHINSTATICARRAY(pbscSector->bsc_abpoPolygons, CBrushPolygon, itpo)
          {
            if( !itpo->IsSelected(BPOF_SELECTED)) pDoc->m_selPolygonSelection.Select( *itpo);
          }
          pDoc->m_chSelections.MarkChanged();
        }
      }
      else if( bAlt && bShift && bCtrl)
      {
        if( crRayHit.cr_penHit != NULL)
        {
          AutoFitMapping( crRayHit.cr_pbpoBrushPolygon, TRUE, TRUE);
        }
      }
      else if( bAlt && bShift)
      {
        if( crRayHit.cr_penHit != NULL)
        {
          AutoFitMapping( crRayHit.cr_pbpoBrushPolygon, TRUE);
        }
      }
      else if( bAlt)
      {
        if( crRayHit.cr_penHit != NULL)
        {
          AutoFitMapping( crRayHit.cr_pbpoBrushPolygon);
        }
      }
      else if(!bCtrl&&!bShift&&!bAlt)
      {
        CenterSelected();
      }
    }
    else if( pDoc->GetEditingMode() == TERRAIN_MODE)
    {
      if(!bCtrl&&!bShift&&!bAlt)
      {
        CenterSelected();
      }
    }
    else if(pDoc->GetEditingMode() == VERTEX_MODE && !bShift)
    {
      if( bCtrl && bAlt)
      {
        if( (crRayHit.cr_penHit != NULL) && (crRayHit.cr_pbpoBrushPolygon != NULL) )
        {
          // select all vertices on hitted polygon
          FOREACHINSTATICARRAY(crRayHit.cr_pbpoBrushPolygon->bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx)
          {
            if( !(*itpbvx)->IsSelected(BVXF_SELECTED)) pDoc->m_selVertexSelection.Select( **itpbvx);
          }
        }
        pDoc->m_chSelections.MarkChanged();
      }
      else if( bCtrl||bAlt)
      {
        CDlgAllignVertices dlg;
        dlg.DoModal();
        return TRUE;
      }
      else
      {
        CenterSelected();
      }
    }
  }
  // if return is pressed in primitive CSG mode
  if( (pMsg->message==WM_KEYDOWN) && ((int)pMsg->wParam==VK_RETURN) && bCSGOn && bPrimitive)
  {
    pMainFrame->ShowInfoWindow();
    CInfoSheet *pSheet = pMainFrame->m_pInfoFrame->m_pInfoSheet;
    CWnd *pwndWidthEditCtrl = pSheet->m_PgPrimitive.GetDlgItem( IDC_WIDTH);
    if( pwndWidthEditCtrl != NULL)
    {
      pSheet->SetActivePage( &pSheet->m_PgPrimitive);
      pwndWidthEditCtrl->SetFocus();
    }
  }

  FLOAT tmNow = _pTimer->GetHighPrecisionTimer().GetSeconds();
  // if we caught key or button down message and not status line info pause requested
  if( (theApp.m_tmStartStatusLineInfo<tmNow) &&
      ( (pMsg->message==WM_SYSKEYDOWN) ||
        (pMsg->message==WM_KEYDOWN) ||
        (pMsg->message==WM_KEYUP) ||
        (pMsg->message==WM_LBUTTONDOWN) ||
        (pMsg->message==WM_RBUTTONDOWN) ||
        (pMsg->message==WM_LBUTTONUP) ||
        (pMsg->message==WM_RBUTTONUP) ||
        (pMsg->message==WM_MOUSEMOVE) ||
        (pMsg->message==WM_CHANGE_EDITING_MODE) ||
        (bShift || bAlt || bCtrl || bSpace || bLMB || bRMB)) )
  {
    // space without mouse buttons but ctrl pressed
    if( bSpace && !bLMB && !bRMB && bCtrl)
    {
      STATUS_LINE_MESASGE( L"LMB zoomes in 2x, RMB zoomes out 2x");
    }
    // space without mouse buttons
    else if( bSpace && !bLMB && !bRMB && !bShift)
    {
      STATUS_LINE_MESASGE( L"Rotate or move viewer. Try: LMB,RMB,LMB+RMB,Space+Ctrl");
    }
    // space with left mouse button
    else if( bSpace && bLMB && !bRMB)
    {
      STATUS_LINE_MESASGE( L"Move viewer in view plane");
    }
    // space with right mouse button
    else if( bSpace && !bLMB && bRMB)
    {
      STATUS_LINE_MESASGE( L"Move viewer in floor plane");
    }
    // space with both mouse buttons
    else if( bSpace && bLMB && bRMB)
    {
      STATUS_LINE_MESASGE( L"Rotate viewer");
    }
    // Alt pressed
    else if( bAlt && !bCtrl && !bShift)
    {
      STATUS_LINE_MESASGE( L"You can drag any view and drop it into browser");
    }
    // for measure mode
    else if( theApp.m_bMeasureModeOn)
    {
      STATUS_LINE_MESASGE( L"Use LMB to measure distances");
    }
    // for cut mode
    else if( theApp.m_bCutModeOn)
    {
      STATUS_LINE_MESASGE( L"Use LMB to move cut/mirror line, enter to apply");
    }
    // for CSG mode
    else if( bCSGOn)
    {
      // for primitive mode
      if( bPrimitive)
      {
        // primitive mode, only <Ctrl> pressed
        if( bCtrl && !bLMB && !bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Use LMBx2 to teleport primitive. Try: LMB,RMB,LMB+RMB,Shift");
        }
        //-------- <Ctrl+Shift> commands for primitive
        // primitive mode, <Ctrl+Shift> pressed
        else if( bCtrl && !bLMB && !bRMB && bShift)
        {
          STATUS_LINE_MESASGE( L"Size and shear primitive. Try: LMB,RMB");
        }
        // primitive mode, <Ctrl+Shift+LMB> pressed
        else if( bCtrl && bLMB && !bRMB && bShift)
        {
          STATUS_LINE_MESASGE( L"Resize primitive");
        }
        // primitive mode, <Ctrl+Shift+RMB> pressed
        else if( bCtrl && !bLMB && bRMB && bShift)
        {
          STATUS_LINE_MESASGE( L"Shear primitive");
        }
        // primitive mode, <Ctrl+LMB> pressed
        else if( bCtrl && bLMB && !bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Move primitive in view plane");
        }
        // primitive mode, <Ctrl+RMB> pressed
        else if( bCtrl && !bLMB && bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Move primitive in floor plane");
        }
        // primitive mode, <Ctrl+LMB+RMB> pressed
        else if( bCtrl && bLMB && bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Rotate primitive in view plane");
        }
        // <Shift> pressed
        else if( !bCtrl && !bLMB && !bRMB && bShift)
        {
          STATUS_LINE_MESASGE( L"LMB inserts vertex, RMB deletes vertex. Try: Ctrl");
        }
        // primitive mode, nothing pressed
        else
        {
          STATUS_LINE_MESASGE( L"Primitive mode. Try: Alt,Ctrl,Shift");
        }
      }
      // for template mode
      else
      {
        //-------- <Ctrl> commands for template (moving and rotating)
        // template mode, only <Ctrl> pressed
        if( bCtrl && !bLMB && !bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Use LMB dbl. click to teleport template. Try: LMB,RMB,LMB+RMB");
        }
        // template mode, <Ctrl+LMB> pressed
        else if( bCtrl && bLMB && !bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Move template in view plane");
        }
        // template mode, <Ctrl+RMB> pressed
        else if( bCtrl && !bLMB && bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Move template in floor plane");
        }
        // template mode, <Ctrl+LMB+RMB> pressed
        else if( bCtrl && bLMB && bRMB && !bShift)
        {
          STATUS_LINE_MESASGE( L"Rotate template");
        }
        // template mode, nothing pressed
        else
        {
          STATUS_LINE_MESASGE( L"Template primitive mode. Try: Ctrl");
        }
      }
    }
    // for entity mode
    else if( bEntityMode)
    {
      // get property ID
      CPropertyID *ppidProperty = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
      // if we have at least one entity selected and edititng property of edit range type
      // and Ctrl+Shift pressed
      BOOL bEditingProperties = ( (pDoc->m_selEntitySelection.Count() != 0) &&
                             ( (ppidProperty != NULL) &&
                               ( (ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE3D) ||
                                 (ppidProperty->pid_eptType == CEntityProperty::EPT_RANGE))) );
      // entity mode, <Ctrl+Shift> pressed
      if( bEditingProperties && bCtrl && bShift )
      {
        STATUS_LINE_MESASGE( L"Use LMB to change range or direction");
      }
      // <Alt + Ctrl> pressed
      else if( bAlt && bCtrl)
      {
        STATUS_LINE_MESASGE( L"Use LMB to set entity ptr. (Entity ptr property must be selected)");
      }
      else if( bCtrl && bShift)
      {
        STATUS_LINE_MESASGE( L"Use RMB to set mip switch distance (in auto mode)");
      }
      else if( bSpace && bShift)
      {
        STATUS_LINE_MESASGE( L"Use RMB to change virtual mip distance");
      }
      // entity mode, <Shift> pressed
      else if( !bCtrl && bShift)
      {
        STATUS_LINE_MESASGE( L"Use LMB to toggle selection of one entity . Try: Ctrl,Space");
      }
      // entity mode, <Ctrl> pressed
      else if( bCtrl && !bLMB && !bRMB && !bShift)
      {
        if( bEditingProperties)
        {
          STATUS_LINE_MESASGE( L"Use LMBx2 click to teleport entity. Try: LMB,RMB,LMB+RMB,Shift");
        }
        else
        {
          STATUS_LINE_MESASGE( L"Use LMBx2 click to teleport entity. Try: Alt,LMB,RMB,LMB+RMB");
        }
      }
      // entity mode, <Ctrl+LMB> pressed
      else if( bCtrl && bLMB && !bRMB && !bShift)
      {
        STATUS_LINE_MESASGE( L"Move entities in view plane");
      }
      // entity mode, <Ctrl+RMB> pressed
      else if( bCtrl && !bLMB && bRMB && !bShift)
      {
        STATUS_LINE_MESASGE( L"Move entities in floor plane");
      }
      // entity mode, <Ctrl+LMB+RMB> pressed
      else if( bCtrl && bLMB && bRMB && !bShift)
      {
        STATUS_LINE_MESASGE( L"Rotate entities");
      }
      // entity mode, nothing pressed
      else
      {
        if( bEditingProperties)
        {
          STATUS_LINE_MESASGE( L"Use LMB to select entities. Try: Ctrl,Shift");
        }
        else
        {
          STATUS_LINE_MESASGE( L"Use LMB to select entities. Try: Ctrl,Shift,Ctrl+Alt");
        }
      }
    }
    // for polygon mode
    else if( bPolygonMode)
    {
      // polygon mode, <Ctrl> pressed
      if( bCtrl && !bLMB && !bRMB && !bShift && !bAlt)
      {
        STATUS_LINE_MESASGE( L"LMBx2 centers texture. RMBx2 selects similar by texture. Try: LMB,LMB+RMB");
      }
      // polygon mode, <Shift> pressed
      else if( !bCtrl && !bLMB && !bRMB && bShift && !bAlt)
      {
        STATUS_LINE_MESASGE( L"Toggle polygon using LMB or RMB. Add simillar with LMB dbl. click");
      }
      // polygon mode, <Ctrl+LMB> pressed
      else if( bCtrl && bLMB && !bRMB && !bShift && !bAlt)
      {
        STATUS_LINE_MESASGE( L"Scroll polygon's mapping texture");
      }
      // polygon mode, <Ctrl+LMB+RMB> pressed
      else if( bCtrl && bLMB && bRMB && !bShift && !bAlt)
      {
        STATUS_LINE_MESASGE( L"Rotate polygon's mapping texture");
      }
      else if( bCtrl && bAlt)
      {
        STATUS_LINE_MESASGE( L"Zoom in texture 2x using LMB, zoom out texture 2x using RMB");
      }
      // polygon mode, <Ctrl+Shift> pressed
      else if( bCtrl && bShift)
      {
        STATUS_LINE_MESASGE( L"Select polygons similar by texture in sector with LMBx2, similar by color with RMBx2");
      }
      // polygon mode, nothing pressed
      else if( !bCtrl && !bLMB && !bRMB && !bShift && !bAlt)
      {
        STATUS_LINE_MESASGE( L"Select polygons using LMB or RMB, by color using LMBx2. Try: Alt,Ctrl,Shift,Ctrl+Alt");
      }
    }
    // for sector mode
    else
    {
      // sector mode, <Shift> pressed
      if( !bCtrl && !bLMB && !bRMB && bShift)
      {
        STATUS_LINE_MESASGE( L"Add to selection or deselect sector using LMB or LMB dbl. click");
      }
      // sector mode, nothing pressed
      else
      {
        STATUS_LINE_MESASGE( L"Select or deselect sector using LMB or LMB dbl. click. Try: Alt,Shift");
      }
    }

    // for these events
    if( (pMsg->message==WM_KEYDOWN) ||
        (pMsg->message==WM_KEYUP) ||
        (pMsg->message==WM_LBUTTONDOWN) ||
        (pMsg->message==WM_RBUTTONDOWN) ||
        (pMsg->message==WM_LBUTTONUP) ||
        (pMsg->message==WM_RBUTTONUP) )
    {
      // set text describing data that is edited
      SetEditingDataPaneInfo( TRUE);
    }
  }

	return CView::PreTranslateMessage(pMsg);
}

void CWorldEditorView::OnKeyO()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_SMOOTH;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
  }
  else
  {
    OnCloneCSG();
  }
}

void CWorldEditorView::OnUpdateKeyO(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE)
  {
    pCmdUI->Enable( TRUE);
  }
  else
  {
    OnUpdateCloneCsg(pCmdUI);
  }
}

void CWorldEditorView::OnCloneCSG()
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CWorldEditorDoc* pDoc = GetDocument();
  CDlgProgress dlgProgressDialog( pMainFrame);

  // disable CSG report
  theApp.m_bCSGReportEnabled = FALSE;
  // and clear its values
  _pfWorldEditingProfile.Reset();
  // remember automatic info flag
  BOOL bAutomaticInfo = theApp.m_Preferences.ap_AutomaticInfo;
  // get shift pressed flag
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  // we will have 1 clone (by default)
  INDEX ctClones = 1;
  // if shift is pressed, we want "array of clones" (numeric option)
  if( bAlt)
  {
    CDlgAutoDeltaCSG dlgAutoDeltaCSG;
    // if dialog wasn't succeseful
    if( dlgAutoDeltaCSG.DoModal() != IDOK)
    {
      return;
    }
    // temporary disable automatic info
    theApp.m_Preferences.ap_AutomaticInfo = FALSE;
    // get wanted count of clones
    ctClones = dlgAutoDeltaCSG.m_ctNumberOfClones;
    // write it to .ini file for the next time
    theApp.WriteProfileInt(L"World editor", L"Number of CSG clones", ctClones);
  }
  // set wait cursor
  CWaitCursor StartWaitCursor;

  // if there is more than 1 clone in progress
  if( ctClones > 1)
  {
    // create progress dialog
    dlgProgressDialog.Create( IDD_PROGRESS_DIALOG);
    dlgProgressDialog.SetWindowText(L"Repeat CSG");
    // show progress window
    dlgProgressDialog.ShowWindow( SW_SHOW);
    // center window
    dlgProgressDialog.CenterWindow();
    // set progress range
    dlgProgressDialog.m_ctrlProgres.SetRange( 0, (short) ctClones);
  }

  // if primitive CSG was last used CSG operation
  if( pDoc->m_bLastUsedPrimitiveMode)
  {
    // for all clones
    for( INDEX iClone=0; iClone<ctClones; iClone++)
    {
      // if there is more than 1 clone in progress
      if( ctClones > 1)
      {
        char achrProgressMessage[ 256];
        sprintf( achrProgressMessage, "Cloning CSG operation ... (processing %d/%d)",
                 iClone+1, ctClones);
        // set message and progress position
        dlgProgressDialog.SetProgressMessageAndPosition( achrProgressMessage, iClone+1);
      }
      // create new placement from delta placement
	    CPlacement3D plNewPlacement = pDoc->m_plDeltaPlacement;
      // convert it into absolute space of last used placement (delta applyed)
      plNewPlacement.RelativeToAbsolute( pDoc->m_plLastPlacement);
      // copy last used values for primitive as initial values for this new primitive
      theApp.m_vfpPreLast = theApp.m_vfpLast;
      theApp.m_vfpCurrent = theApp.m_vfpLast;
      // apply width, sheer, stretch, ... delta
      theApp.m_vfpCurrent += theApp.m_vfpDelta;
      // after substraction we colud get some invalid values (like negative width!!!)
      // so we have to correct them
      theApp.m_vfpCurrent.CorrectInvalidValues();
      // we will snap primitive placement
      pDoc->SnapToGrid( plNewPlacement, SNAP_FLOAT_25);

      // create primitive but don't reset orientation angles
      pDoc->StartPrimitiveCSG( plNewPlacement, FALSE);
      // for last clone enable undo remembering
      if( iClone > 0)
      {
        // disable undo remembering
        theApp.m_bRememberUndo = FALSE;
      }
      // if shift was helded while starting clone CSG
      if( bAlt)
      {
        // apply same CSG operation as was used last time
        pDoc->ApplyCSG( pDoc->m_csgtLastUsedCSGOperation);
      }
    }
    // if there is more than 1 clone in progress
    if( ctClones > 1)
    {
      // destroy progress dialog
      dlgProgressDialog.DestroyWindow();
    }
  }
  // for templates
  else
  {
    // for all clones
    for( INDEX iClone=0; iClone<ctClones; iClone++)
    {
      // if there is more than 1 clone in progress
      if( ctClones > 1)
      {
        char achrProgressMessage[ 256];
        sprintf( achrProgressMessage, "Repeating last operation... (processing %d/%d)",
                 iClone+1, ctClones);
        // set message and progress position
        dlgProgressDialog.SetProgressMessageAndPosition( achrProgressMessage, iClone+1);
      }
      // create new placement from delta placement
	    CPlacement3D plNewPlacement = pDoc->m_plDeltaPlacement;
      // convert it into absolute space of last used placement (delta applyed)
      plNewPlacement.RelativeToAbsolute( pDoc->m_plLastPlacement);

      // we will snap template placement
      pDoc->SnapToGrid( plNewPlacement, SNAP_FLOAT_25);

      // for last clone enable undo remembering
      if( iClone > 0)
      {
        // disable undo remembering
        theApp.m_bRememberUndo = FALSE;
      }

      // start template CSG but don't reset orientation angles
      pDoc->StartTemplateCSG( plNewPlacement, pDoc->m_fnLastDroppedTemplate, FALSE);

      // if shift was helded while starting clone CSG, we want to apply CSG
      // if template was automaticly joined after droping, we are not in CSG_MODE any more
      if( bAlt && (pDoc->GetEditingMode() == CSG_MODE) )
      {
        // apply same CSG operation as was used last time
        pDoc->ApplyCSG( pDoc->m_csgtLastUsedCSGOperation);
      }
    }
    // if there is more than 1 clone in progress
    if( ctClones > 1)
    {
      // destroy progress dialog
      dlgProgressDialog.DestroyWindow();
    }
  }
  // enable undo remembering
  theApp.m_bRememberUndo = TRUE;
  // restore automatic info flag
  theApp.m_Preferences.ap_AutomaticInfo = bAutomaticInfo;
  // enable CSG report
  theApp.m_bCSGReportEnabled = TRUE;
  // and report calculated CSG report values
  _pfWorldEditingProfile.Report( theApp.m_strCSGAndShadowStatistics);
  theApp.m_strCSGAndShadowStatistics.SaveVar(CTString("Temp\\Profile_CSGWizard.txt"));
}

void CWorldEditorView::OnUpdateCloneCsg(CCmdUI* pCmdUI)
{
  CWorldEditorDoc* pDoc = GetDocument();
  // there can't be any kind of CSG going on if we want to clone last CSG operation
  BOOL bCSGOn = pDoc->m_pwoSecondLayer != NULL;
  // there must be at least one CSG operation done
  BOOL bCanBeCloned = ( (pDoc->m_csgtLastUsedCSGOperation == CSG_ADD) ||
                        (pDoc->m_csgtLastUsedCSGOperation == CSG_ADD_REVERSE) ||
                        (pDoc->m_csgtLastUsedCSGOperation == CSG_REMOVE) ||
                        (pDoc->m_csgtLastUsedCSGOperation == CSG_REMOVE_REVERSE) ||
                        (pDoc->m_csgtLastUsedCSGOperation == CSG_SPLIT_SECTORS) ||
                        (pDoc->m_csgtLastUsedCSGOperation == CSG_SPLIT_POLYGONS) ||
                        (pDoc->m_csgtLastUsedCSGOperation == CSG_JOIN_LAYERS) );
  // last and current primitive CSG operation must have same primitive types
  pCmdUI->Enable( bCanBeCloned && !bCSGOn &&
    (pDoc->m_csgtPreLastUsedCSGOperation==pDoc->m_csgtLastUsedCSGOperation) &&
     (pDoc->m_bPreLastUsedPrimitiveMode==pDoc->m_bLastUsedPrimitiveMode) &&
     (!pDoc->m_bLastUsedPrimitiveMode ||
      (pDoc->m_bLastUsedPrimitiveMode &&
      (theApp.m_vfpPreLast.vfp_ptPrimitiveType == theApp.m_vfpLast.vfp_ptPrimitiveType))) );
}


void CWorldEditorView::OnPopupConus()
{
  // start conus primitive
  theApp.m_vfpCurrent = theApp.m_vfpConus;
  CreatePrimitiveCalledFromPopup();
}

void CWorldEditorView::OnPopupTorus()
{
  // start conus primitive
  theApp.m_vfpCurrent = theApp.m_vfpTorus;
  CreatePrimitiveCalledFromPopup();
}

void CWorldEditorView::OnPopupSphere()
{
  // start conus primitive
  theApp.m_vfpCurrent = theApp.m_vfpSphere;
  CreatePrimitiveCalledFromPopup();
}

void CWorldEditorView::OnPopupStairs()
{
  // start conus primitive
  theApp.m_vfpCurrent = theApp.m_vfpStaircases;
  CreatePrimitiveCalledFromPopup();
}

void CWorldEditorView::OnPopupTerrain()
{
  // start conus primitive
  theApp.m_vfpCurrent = theApp.m_vfpTerrain;
  CreatePrimitiveCalledFromPopup();
}

void CWorldEditorView::CreatePrimitiveCalledFromPopup()
{
  if( !m_bEntityHitedOnContext)
  {
    OnConusPrimitive();
  }
  CWorldEditorDoc* pDoc = GetDocument();
  // create placement for primitive's position
  CPlacement3D plDrop = m_plEntityHitOnContext;

  // snap placement
  pDoc->SnapToGrid( plDrop, m_fGridInMeters/GRID_DISCRETE_VALUES);
  // create primitive for the first time, don't reset angles
  pDoc->StartPrimitiveCSG( plDrop, FALSE);
}

void CWorldEditorView::OnMeasureOn()
{
	theApp.m_bMeasureModeOn = !theApp.m_bMeasureModeOn;
	theApp.m_bCutModeOn = FALSE;
}

void CWorldEditorView::OnUpdateMeasureOn(CCmdUI* pCmdUI)
{
  pCmdUI->SetCheck( theApp.m_bMeasureModeOn);
}

void CWorldEditorView::OnResetViewer()
{
  CWorldEditorDoc* pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	// set new target
  GetChildFrame()->m_mvViewer.SetTargetPlacement( FLOAT3D(0.0f, 0.0f, 0.0f));
  _fFlyModeSpeedMultiplier=theApp.m_Preferences.ap_fDefaultFlyModeSpeed;
  pMainFrame->ResetInfoWindowPos();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnCenterBcgViewer()
{
  CWorldEditorDoc* pDoc = GetDocument();
  // for all of the world's entities
  CEntity *penBackgroundViewer = pDoc->m_woWorld.GetBackgroundViewer();
  if( penBackgroundViewer != NULL)
  {
	  // set new target
    GetChildFrame()->m_mvViewer.SetTargetPlacement(
      penBackgroundViewer->GetPlacement().pl_PositionVector);
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnCopyTexture()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    OnPickLayer();
    return;
  }
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    if( pDoc->GetEditingMode() == SECTOR_MODE)
    {
      CBrushSector *pbscSector = crRayHit.cr_pbpoBrushPolygon->bpo_pbscSector;
      CopySectorAmbient( pbscSector);
    }
    else
    {
      // get polygon's texture
      CTextureData *pTD = (CTextureData *)crRayHit.cr_pbpoBrushPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData();
      if( pTD != NULL)
      {
        // get name from serial object
        CTFileName fnTextureName = pTD->GetName();
        // set it as new active texture
        theApp.SetNewActiveTexture( _fnmApplicationPath + fnTextureName);
      }
    }
  }
}

void CWorldEditorView::OnPasteTexture()
{
	CWorldEditorDoc* pDoc = GetDocument();
  
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_RND_NOISE;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
    return;
  }

  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL))
  {
    if( pDoc->GetEditingMode() == SECTOR_MODE)
    {
      CBrushSector *pbscSector = crRayHit.cr_pbpoBrushPolygon->bpo_pbscSector;
      PasteSectorAmbient( pbscSector);
    }
    else if( pDoc->GetEditingMode() == POLYGON_MODE)
    {
      PasteTexture( crRayHit.cr_pbpoBrushPolygon);
    }
  }
}

void CWorldEditorView::OnSelectByTextureAdjacent()
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    // select similar by texture to hitted polygon
    crRayHit.cr_pbpoBrushPolygon->SelectSimilarByTexture( pDoc->m_selPolygonSelection, pDoc->m_iTexture);
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnSelectByTextureInSector()
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    // select similar by texture to hitted polygon
    crRayHit.cr_pbpoBrushPolygon->SelectByTextureInSector( pDoc->m_selPolygonSelection, pDoc->m_iTexture);
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnSelectByColorInSector()
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    // select similar by color to hitted polygon
    crRayHit.cr_pbpoBrushPolygon->SelectByColorInSector( pDoc->m_selPolygonSelection);
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnFunction() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_iMode == ENTITY_MODE)
  {
    pDoc->m_cenEntitiesSelectedByVolume.Clear();
    // for each of the entities selected by volume
    FOREACHINDYNAMICCONTAINER( pDoc->m_selEntitySelection, CEntity, iten)
    {
      // add entity into volume selection
      if( !pDoc->m_cenEntitiesSelectedByVolume.IsMember(iten))
      {
        pDoc->m_cenEntitiesSelectedByVolume.Add( iten);
      }
    }
    // go out of browse by volume mode
    if( pDoc->m_bBrowseEntitiesMode) pDoc->OnBrowseEntitiesMode();
    // mark that selections have been changed
    pDoc->m_chSelections.MarkChanged();
    // obtain main frame ptr
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    // and refresh property combo manualy calling on idle
    pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
    // update all views
    pDoc->UpdateAllViews( NULL);
  }
  else if( pDoc->m_iMode == POLYGON_MODE)
  {
    pDoc->OnFilterSelection();
  }
  else if( pDoc->m_iMode == VERTEX_MODE)
  {
    SnapSelectedVerticesToPlane();
  }
  else if( pDoc->m_iMode == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_FILTER;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::SnapSelectedVerticesToPlane(void)
{
	CWorldEditorDoc* pDoc = GetDocument();
  pDoc->RememberUndo();
  pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    CBrushPolygon *pbpo = crRayHit.cr_pbpoBrushPolygon;
    CEntity *pen = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    CSimpleProjection3D_DOUBLE pr;
    pr.ObjectPlacementL() = pen->GetPlacement();
    pr.ViewerPlacementL() = _plOrigin;
    pr.Prepare();
    DOUBLEplane3D plAbsPrecise;
    pr.Project(pbpo->bpo_pbplPlane->bpl_pldPreciseRelative, plAbsPrecise);

    // snap vertices to plane
    {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
    {
      CEntity *pen = itbvx->bvx_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
      CSimpleProjection3D_DOUBLE pr;
      pr.ObjectPlacementL() = pen->GetPlacement();
      pr.ViewerPlacementL() = _plOrigin;
      pr.Prepare();
      DOUBLE3D vAbsPrecise;
      pr.ProjectCoordinate(itbvx->bvx_vdPreciseRelative, vAbsPrecise);
      DOUBLE3D vOnPlaneAbsPrecise=plAbsPrecise.ProjectPoint(vAbsPrecise);
      itbvx->SetAbsolutePosition(vOnPlaneAbsPrecise);
    }}
  }
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
  pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
  pDoc->UpdateAllViews( NULL);
  pDoc->SetModifiedFlag();
}

void CWorldEditorView::OnCrossroadForCtrlF() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if(pDoc->GetEditingMode() == POLYGON_MODE)
  {
    OnFindTexture();
  }
  else if(pDoc->GetEditingMode() == VERTEX_MODE)
  {
    CDlgFilterVertexSelection dlg;
    dlg.DoModal();
  }
  else if(pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iFilter=(theApp.m_iFilter+1)%FLT_COUNT;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    GetDocument()->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnFindTexture()
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    // get polygon's texture
    CTextureData *pTD = (CTextureData *)crRayHit.cr_pbpoBrushPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData();
    if( pTD != NULL)
    {
      // get name from serial object
      CTFileName fnTextureName = pTD->GetName();
      // set it as new active texture
      theApp.FindItemInBrowser( fnTextureName);
    }
  }
}

void CWorldEditorView::OnCenterEntity()
{
	CWorldEditorDoc* pDoc = GetDocument();

  if(pDoc->GetEditingMode() == CSG_MODE)
  {
    GetChildFrame()->m_mvViewer.SetTargetPlacement( pDoc->m_plSecondLayer.pl_PositionVector);
  }
  if(pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    CTerrain *ptrTerrain=GetTerrain();
    if(ptrTerrain!=NULL)
    {
      FLOATaabbox3D boxBoundingBox;
      ptrTerrain->GetAllTerrainBBox(boxBoundingBox);
      FLOAT3D vTerrainCenter=boxBoundingBox.Center();
      GetChildFrame()->m_mvViewer.SetTargetPlacement( vTerrainCenter);
    }
  }
  else
  {
    if( pDoc->m_selEntitySelection.Count() == 0) return;
    // reset position
    FLOAT3D vEntityPlacementAverage = FLOAT3D( 0.0f, 0.0f, 0.0f);
    //for all selected entities
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      // get placement of current entity
      CPlacement3D plEntityPlacement = iten->GetPlacement();
      // add movement offset to placement
      vEntityPlacementAverage += plEntityPlacement.pl_PositionVector;
    }
    // calculate average position
    vEntityPlacementAverage /= (FLOAT) pDoc->m_selEntitySelection.Count();
    // set centered entity position as new viewer's target
    GetChildFrame()->m_mvViewer.SetTargetPlacement( vEntityPlacementAverage);
  }
  // update all views
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnUpdateCenterEntity(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
  // if we are in entity mode and we have at least 1 entity selected
  pCmdUI->Enable( (pDoc->GetEditingMode() == CSG_MODE) ||
                  ((pDoc->GetEditingMode() == TERRAIN_MODE) &&
                   (GetTerrain()!=NULL)) ||
                  ((pDoc->GetEditingMode() == ENTITY_MODE) &&
                   (pDoc->m_selEntitySelection.Count() != 0)) );
}

void CWorldEditorView::OnDropMarker()
{
	CWorldEditorDoc* pDoc = GetDocument();
  CTFileName fnDropClass;
  CTString strTargetProperty;

  if(pDoc->m_selEntitySelection.Count() == 1)
  {
    CEntity &enOnly = pDoc->m_selEntitySelection.GetFirst();
    if(enOnly.DropsMarker( fnDropClass, strTargetProperty))
    {
      OnDropMarker(enOnly.GetPlacement());
    }
  }
}

void CWorldEditorView::OnDropMarker(CPlacement3D plMarker)
{
	CWorldEditorDoc* pDoc = GetDocument();

  CTFileName fnDropClass, fnDummy;
  CTString strTargetProperty;
  CEntityProperty *penpProperty;

  CEntity &enOnly = pDoc->m_selEntitySelection.GetFirst();
  // obtain drop class and target property name
  if( !enOnly.DropsMarker( fnDropClass, strTargetProperty)) return;
  penpProperty = enOnly.PropertyForName( strTargetProperty);

  CEntity *penFirstInChain;
  if (enOnly.IsMarker())
  {
    penFirstInChain = &enOnly;
  }
  else
  {
    penFirstInChain = ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer);
  }
  CEntity *penSecondInChain = NULL;
  // if first in chain exists, try getting his target
  if( penFirstInChain != NULL)
  {
    if( !penFirstInChain->DropsMarker( fnDropClass, strTargetProperty)) return;
    penpProperty = penFirstInChain->PropertyForName( strTargetProperty);
    penSecondInChain = ENTITYPROPERTY( penFirstInChain, penpProperty->ep_slOffset, CEntityPointer);
  }
  // spawn entity
  CEntity *penSpawned;
  if( !enOnly.DropsMarker( fnDropClass, strTargetProperty)) return;
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  try
  {
    // spawn one entity of class selected in browser
    penSpawned = pDoc->m_woWorld.CreateEntity_t( plMarker, fnDropClass);
    penSpawned->Initialize();
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    return;
  }
  // if first in chain entity exists, set spawned entity to be his target
  if( penFirstInChain != NULL)
  {
    if( !penFirstInChain->DropsMarker( fnDropClass, strTargetProperty)) return;
    penpProperty = penFirstInChain->PropertyForName( strTargetProperty);
    ENTITYPROPERTY( penFirstInChain, penpProperty->ep_slOffset, CEntityPointer) = penSpawned;
  }
  if( !penSpawned->DropsMarker( fnDropClass, strTargetProperty)) return;
  penpProperty = penSpawned->PropertyForName( strTargetProperty);
  if( penSecondInChain != NULL)
  {
    ENTITYPROPERTY( penSpawned, penpProperty->ep_slOffset, CEntityPointer) = penSecondInChain;
  }
  else
  {
    ENTITYPROPERTY( penSpawned, penpProperty->ep_slOffset, CEntityPointer) = penFirstInChain;
  }
  // set our only selected entity to point to spawned entity
  if( !enOnly.DropsMarker( fnDropClass, strTargetProperty)) return;
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer) = penSpawned;

  pDoc->SetModifiedFlag();
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnUpdateDropMarker(CCmdUI* pCmdUI)
{
	CTFileName fnDropClass;
  CTString strTargetProperty;

  CWorldEditorDoc* pDoc = GetDocument();
  // if we are in entity mode, we have 1 entity selected and entity can drop marker
  if( (pDoc->GetEditingMode() == ENTITY_MODE) &&
      (pDoc->m_selEntitySelection.Count() == 1) &&
      (pDoc->m_selEntitySelection.GetFirst().DropsMarker( fnDropClass, strTargetProperty)) )
  {
    pCmdUI->Enable( TRUE);
  }
  else
  {
    pCmdUI->Enable( FALSE);
  }
}

void CWorldEditorView::OnTestConnections()
{
	CWorldEditorDoc* pDoc = GetDocument();
  CEntityProperty *penpProperty;

  CEntity &enOnly = pDoc->m_selEntitySelection.GetFirst();
  CTString strTargetProperty;

  // get target pointer for curently selected entity
  enOnly.MovesByTargetedRoute( strTargetProperty);
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  CEntity *penCurrent = ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer);

  // if no target
  if( penCurrent == NULL) {
    // do nothing
    return;
  }

  // get target pointer for its target (get next)
  penCurrent->MovesByTargetedRoute( strTargetProperty);
  penpProperty = penCurrent->PropertyForName( strTargetProperty);
  CEntity *penNext = ENTITYPROPERTY( penCurrent, penpProperty->ep_slOffset, CEntityPointer);

  // if no target
  if( penNext == NULL) {
    // do nothing
    return;
  }

  // teleport to next
  enOnly.SetPlacement( penNext->GetPlacement());

  if (enOnly.en_RenderType == CEntity::RT_BRUSH)
  {
    DiscardShadows( &enOnly);
  }

  // set target pointer to next next
  enOnly.MovesByTargetedRoute( strTargetProperty);
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer) = penNext;

  // mark that document and selections are changed
  pDoc->SetModifiedFlag();
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnTestConnectionsBack() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CEntityProperty *penpProperty;

  CEntity &enOnly = pDoc->m_selEntitySelection.GetFirst();
  CTString strTargetProperty;

  // get target pointer for curently selected entity
  enOnly.MovesByTargetedRoute( strTargetProperty);
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  CEntity *penCurrent = ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer);

  // if no target
  if( penCurrent == NULL) {
    // do nothing
    return;
  }

  // start from current
  CEntity *penNext=penCurrent;

  // repeat
  FOREVER {
    // get next target pointer
    penNext->MovesByTargetedRoute( strTargetProperty);
    penpProperty = penCurrent->PropertyForName( strTargetProperty);
    CEntity *penNextNext = ENTITYPROPERTY( penNext, penpProperty->ep_slOffset, CEntityPointer);
    // if no target
    if( penNextNext == NULL) {
      // do nothing
      return;
    }
    // if the next one is same as where the entity started
    if( penNextNext==penCurrent) {
      // stop searching
      break;
    }
    // go to that target
    penNext = penNextNext;
  }
  
  // teleport to next
  enOnly.SetPlacement( penNext->GetPlacement());
  if (enOnly.en_RenderType == CEntity::RT_BRUSH)
  {
    DiscardShadows( &enOnly);
  }

  // set target pointer to next next
  enOnly.MovesByTargetedRoute( strTargetProperty);
  penpProperty = enOnly.PropertyForName( strTargetProperty);
  ENTITYPROPERTY( &enOnly, penpProperty->ep_slOffset, CEntityPointer) = penNext;

  // mark that document and selections are changed
  pDoc->SetModifiedFlag();
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnUpdateTestConnections(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
  BOOL bModeAndCount = (pDoc->GetEditingMode() == ENTITY_MODE) &&
                       ( pDoc->m_selEntitySelection.Count() == 1);
  BOOL bEnableCommand = FALSE;
  // if we have only one entity selected and we are in entity mode
  if( bModeAndCount)
  {
    CEntity &enOnly = pDoc->m_selEntitySelection.GetFirst();
    CTString strTargetProperty;
    // if we can perform test path function on this entity
    if( enOnly.MovesByTargetedRoute( strTargetProperty))
      bEnableCommand = TRUE;
  }
  pCmdUI->Enable( bEnableCommand);
}

void CWorldEditorView::OnUpdateTestConnectionsBack(CCmdUI* pCmdUI) 
{
  // update same as it is forward
  OnUpdateTestConnections( pCmdUI);
}

#define PAPER (4.0f/5.0f)

void CWorldEditorView::OnAlignVolume()
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    return;
  }
	CWorldEditorDoc* pDoc = GetDocument();
  // create a slave viewer
  CSlaveViewer svViewer(GetChildFrame()->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid,
                        pdpValidDrawPort);
  // get zoom factor
  FLOAT fZoom = svViewer.GetZoomFactor();
  PIX pixWidth = pdpValidDrawPort->GetWidth();
  PIX pixHeight = pdpValidDrawPort->GetHeight();
  FLOAT fWidth = pixWidth/fZoom;
  FLOAT fHeight = pixHeight/fZoom;
  // get point in space that user is looking
  FLOAT3D vTargetPoint = GetChildFrame()->m_mvViewer.GetTargetPlacement().pl_PositionVector;

  pDoc->m_vCreateBoxVertice0 = FLOAT3D(
    vTargetPoint(1)-fWidth*PAPER/2,
    vTargetPoint(2)-fHeight*PAPER/2,
    vTargetPoint(3)-fHeight*PAPER/2);
  pDoc->m_vCreateBoxVertice1 = FLOAT3D(
    vTargetPoint(1)+fWidth*PAPER/2,
    vTargetPoint(2)+fHeight*PAPER/2,
    vTargetPoint(3)+fHeight*PAPER/2);
  pDoc->SelectEntitiesByVolumeBox();
  // try to select first entity in volume
  pDoc->SelectGivenEntity( 0);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnAlignPrimitive()
{
  // get draw port
  CDrawPort *pdpValidDrawPort = GetDrawPort();
  // if it is not valid
  if( pdpValidDrawPort == NULL)
  {
    return;
  }
	CWorldEditorDoc* pDoc = GetDocument();
  // create a slave viewer
  CSlaveViewer svViewer(GetChildFrame()->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid,
                        pdpValidDrawPort);
  // get zoom factor
  FLOAT fZoom = svViewer.GetZoomFactor();
  PIX pixWidth = pdpValidDrawPort->GetWidth()*8/10;
  PIX pixHeight = pdpValidDrawPort->GetHeight()*8/10;
  FLOAT fWidth = pixWidth/fZoom;
  FLOAT fHeight = pixHeight/fZoom;
  // get point in space that user is looking
  FLOAT3D vTargetPoint = GetChildFrame()->m_mvViewer.GetTargetPlacement().pl_PositionVector;
  pDoc->m_plSecondLayer.pl_PositionVector = vTargetPoint;

  fWidth = FLOAT( pow( 2.0f, floor(Log2(fWidth))));
  fHeight = FLOAT( pow( 2.0f, floor(Log2(fHeight))));

  theApp.m_vfpCurrent.vfp_fXMin = -fWidth/2.0f;
  theApp.m_vfpCurrent.vfp_fXMax =  fWidth/2.0f;
  theApp.m_vfpCurrent.vfp_fYMin = -fHeight/2.0f;
  theApp.m_vfpCurrent.vfp_fYMax =  fHeight/2.0f;
  theApp.m_vfpCurrent.vfp_fZMin = -fHeight/2.0f;
  theApp.m_vfpCurrent.vfp_fZMax =  fHeight/2.0f;

  pDoc->SnapToGrid( pDoc->m_plSecondLayer, m_fGridInMeters/GRID_DISCRETE_VALUES);
  if( pDoc->m_bAutoSnap)
  {
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fXMin, m_fGridInMeters/GRID_DISCRETE_VALUES);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fXMax, m_fGridInMeters/GRID_DISCRETE_VALUES);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fYMin, m_fGridInMeters/GRID_DISCRETE_VALUES);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fYMax, m_fGridInMeters/GRID_DISCRETE_VALUES);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fZMin, m_fGridInMeters/GRID_DISCRETE_VALUES);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fZMax, m_fGridInMeters/GRID_DISCRETE_VALUES);
  }
  else
  {
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fXMin, SNAP_FLOAT_CM);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fXMax, SNAP_FLOAT_CM);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fYMin, SNAP_FLOAT_CM);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fYMax, SNAP_FLOAT_CM);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fZMin, SNAP_FLOAT_CM);
    pDoc->SnapFloat( theApp.m_vfpCurrent.vfp_fZMax, SNAP_FLOAT_CM);
  }

  pDoc->RefreshPrimitivePage();
  pDoc->CreatePrimitive();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnUpdateAlignVolume(CCmdUI* pCmdUI)
{
	CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->m_bBrowseEntitiesMode);
}

void CWorldEditorView::OnCurrentViewProperties()
{
  // copy selected preferences from view rendering preferences to temporary buffer
  theApp.m_vpViewPrefs[ VIEW_PREFERENCES_CT] = m_vpViewPrefs;
  // call edit view preferences with temporary view preferences buffer (last one)
  CDlgRenderingPreferences dlg( VIEW_PREFERENCES_CT);
  // if dialog ended with cancel or esc, dont switch to changed preferences
  if( dlg.DoModal() != IDOK)
  {
    // don't set new preferences
    return;
  }
  // copy selected preferences to view's rendering preferences
  m_vpViewPrefs = theApp.m_vpViewPrefs[ VIEW_PREFERENCES_CT];
  // see the change
  Invalidate( FALSE);
}


void CWorldEditorView::OnChooseColor()
{
  /*
  // find toll bar's rectangle
  RECT rectToolBar;
  m_wndSelectionTools.GetWindowRect( &rectToolBar);
  // Then we find tool button's index
  INDEX iToolButton = m_wndSelectionTools.CommandToIndex( ID_CHOOSE_COLOR);
	// Using given index, we obtain button's rectangle
  RECT rectButton;
  m_wndSelectionTools.GetItemRect( iToolButton, &rectButton);
  // set screen coordinates of LU point of clicked tool button
  CustomColorPicker( rectToolBar.left + rectButton.left, rectToolBar.top + rectButton.bottom);
  */

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  POINT ptCursor;
  GetCursorPos( &ptCursor);
  if( ptCursor.y < 100)
  {
    pMainFrame->CustomColorPicker( ptCursor.x-50, ptCursor.y);
  }
  else
  {
    pMainFrame->CustomColorPicker( ptCursor.x-50, ptCursor.y-100);
  }
}

void CWorldEditorView::OnUpdateChooseColor(CCmdUI* pCmdUI)
{
  // don't enable initially
  BOOL bEnable = FALSE;
  // obtain document
  CWorldEditorDoc *pDoc = GetDocument();
  // if sector mode is on and there is at least 1 selected sector
  if( (pDoc->m_iMode == SECTOR_MODE) && (pDoc->m_selSectorSelection.Count() != 0) )
  {
    bEnable = TRUE;
  }
  // if polygon mode is on and there is at least 1 selected polygon
  if( (pDoc->m_iMode == POLYGON_MODE) && (pDoc->m_selPolygonSelection.Count() != 0) )
  {
    bEnable = TRUE;
  }
  pCmdUI->Enable( bEnable);
}

void CWorldEditorView::OnCrossroadForC()
{
  PostMessage( WM_COMMAND, ID_CHOOSE_COLOR, 0);
  PostMessage( WM_COMMAND, ID_CENTER_ENTITY, 0);
}

void CWorldEditorView::SetAsCsgTarget(CEntity *pen)
{
  if( (pen != NULL) &&
      ( (pen->GetRenderType() == CEntity::RT_BRUSH) ||
        (pen->GetRenderType() == CEntity::RT_FIELDBRUSH)) )
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->m_CSGDesitnationCombo.SelectBrushEntity(pen);
  }
}

void CWorldEditorView::OnSetAsCsgTarget()
{
  SetAsCsgTarget(m_penEntityHitOnContext);
}

void CWorldEditorView::OnCsgSelectSector()
{
  if( (m_penEntityHitOnContext != NULL) &&
      ((m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) ||
       (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_FIELDBRUSH)) &&
       (m_pbpoRightClickedPolygon != NULL) )
  {
    CBrushSector *pbscHitted = m_pbpoRightClickedPolygon->bpo_pbscSector;
    CWorldEditorDoc *pDoc = GetDocument();
    pDoc->m_selSectorSelection.Clear();
    pDoc->m_selSectorSelection.Select( *pbscHitted);
  }
}

LRESULT CWorldEditorView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  CChildFrame *pCF = GetChildFrame();
  // if test game is active
  if( _pInput->IsInputEnabled())
  { // repost some messages to thread
    switch( message ) {
    case WM_CANCELMODE:
    case WM_KILLFOCUS:
    case WM_ACTIVATEAPP:
    case WM_LBUTTONUP:
      ::PostMessage(NULL, message, wParam, lParam);
      break;
    case WM_MOUSEACTIVATE:
      if( pCF->m_bTestGameOn)
      {
        return MA_ACTIVATEANDEAT;
      }
      break;
    case WM_DESTROY:
	    PostQuitMessage(0);
	    break;
    }
  }

	return CView::WindowProc(message, wParam, lParam);
}

void CWorldEditorView::OnRButtonDblClk(UINT nFlags, CPoint point)
{
  m_iaInputAction = IA_NONE;

  BOOL bSpace = (GetKeyState( VK_SPACE)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bLMB = nFlags & MK_LBUTTON;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;

  // space+ctrl+rmb zoomes out 2x
  if( (bSpace) && (bCtrl) )
  {
    if( bLMB) return;
    OnZoomLess();
    return;
  }

  // RMB dblclick and ctrl select by texture in sector
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // Alt+Ctrl requests that clicked polygon sizes its texture
    if( bAlt && bCtrl)
    {
      MultiplyMappingOnPolygon( 2.0f);
    }
    else if( bCtrl && bShift && !bAlt)
    {
      OnSelectByColorInSector();
    }
    else if( bCtrl && !bAlt)
    {
      OnSelectByTextureAdjacent();
    }
  }

	CView::OnRButtonDblClk(nFlags, point);
}

void CWorldEditorView::OnLastPrimitive()
{
  CWorldEditorDoc* pDoc = GetDocument();

  CTString strError;
  BOOL bCutEnabled = IsCutEnabled( strError);
  if( theApp.m_bCutModeOn)
  {
    if( !bCutEnabled)
    {
      // exit cut mode
      theApp.m_bCutModeOn = FALSE;
      WarningMessage( "%s", strError);
      return;
    }

    pDoc->ApplyCut();
  }
  else
  {
    // exit cut mode
    theApp.m_bCutModeOn = FALSE;
    pDoc->StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
  }
}

void CWorldEditorView::OnUpdateLastPrimitive(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetDocument()->m_pwoSecondLayer == NULL);
}

void CWorldEditorView::OnConusPrimitive()
{
  // copy last used values for conus primitive as initial values for new primitive
  theApp.m_vfpCurrent = theApp.m_vfpConus;
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
}

void CWorldEditorView::OnTorusPrimitive()
{
  // copy last used values for torus primitive as initial values for new primitive
  theApp.m_vfpCurrent = theApp.m_vfpTorus;
  // create primitive for the first time
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
}

void CWorldEditorView::OnStaircasePrimitive()
{
  // copy last used values for staircase primitive as initial values for new primitive
  theApp.m_vfpCurrent = theApp.m_vfpStaircases;
  // create primitive for the first time
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
}

void CWorldEditorView::OnSpherePrimitive()
{
  // copy last used values for sphere primitive as initial values for new primitive
  theApp.m_vfpCurrent = theApp.m_vfpSphere;
  // create primitive for the first time
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
}

void CWorldEditorView::OnTerrainPrimitive()
{
  // copy last used values for terrain primitive as initial values for new primitive
  theApp.m_vfpCurrent = theApp.m_vfpTerrain;
  // create primitive for the first time
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
}


void CWorldEditorView::OnUpdateConusPrimitive(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetDocument()->m_pwoSecondLayer == NULL);
}

void CWorldEditorView::OnUpdateSpherePrimitive(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetDocument()->m_pwoSecondLayer == NULL);
}

void CWorldEditorView::OnUpdateTerrainPrimitive(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetDocument()->m_pwoSecondLayer == NULL);
}

void CWorldEditorView::OnUpdateTorusPrimitive(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetDocument()->m_pwoSecondLayer == NULL);
}

void CWorldEditorView::OnUpdateStaircasePrimitive(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetDocument()->m_pwoSecondLayer == NULL);
}

void CWorldEditorView::OnUpdateMoveDown(CCmdUI* pCmdUI)
{
  CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable( (pDoc->m_pwoSecondLayer != NULL) &&
                  (pDoc->m_bPrimitiveMode) &&
                  (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) );
}

void CWorldEditorView::OnUpdateMoveUp(CCmdUI* pCmdUI)
{
  CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable( (pDoc->m_pwoSecondLayer != NULL) &&
                  (pDoc->m_bPrimitiveMode) &&
                  (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) );
}

void CWorldEditorView::OnSelectLights()
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  CBrushPolygon *pbpoPolygon = crRayHit.cr_pbpoBrushPolygon;
  // if we hit brush entity
  if( (crRayHit.cr_penHit == NULL) ||
      (pbpoPolygon == NULL) ||
      (pbpoPolygon->IsSelected(BPOF_SELECTED)) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      CBrushShadowMap &bsm = itbpo->bpo_smShadowMap;
      FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm.bsm_lhLayers, itbsl)
      {
        CLightSource *plsLight = itbsl->bsl_plsLightSource;
        CEntity *penLight = plsLight->ls_penEntity;
        if( !pDoc->m_selEntitySelection.IsSelected( *penLight))
        {
          pDoc->m_selEntitySelection.Select( *penLight);
        }
      }
    }
  }
  else
  {
    CBrushShadowMap &bsm = pbpoPolygon->bpo_smShadowMap;
    FOREACHINLIST(CBrushShadowLayer, bsl_lnInShadowMap, bsm.bsm_lhLayers, itbsl)
    {
      CLightSource *plsLight = itbsl->bsl_plsLightSource;
      CEntity *penLight = plsLight->ls_penEntity;
      if( !pDoc->m_selEntitySelection.IsSelected( *penLight))
      {
        pDoc->m_selEntitySelection.Select( *penLight);
      }
    }
  }
  pDoc->SetEditingMode( ENTITY_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnDiscardShadows()
{
	CWorldEditorDoc* pDoc = GetDocument();
  CEntity *penEntity = NULL;
  CBrushSector *pbscSector = NULL;
  CBrushPolygon *pbpoPolygon = NULL;
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    penEntity = crRayHit.cr_penHit;
    pbscSector = crRayHit.cr_pbpoBrushPolygon->bpo_pbscSector;
    pbpoPolygon = crRayHit.cr_pbpoBrushPolygon;
  }

  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    DiscardShadows( penEntity);
  }
  else if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    DiscardShadows( pbscSector);
  }
  else if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    DiscardShadows( pbpoPolygon);
  }
}

void CWorldEditorView::OnCopySectorAmbient()
{
  CWorldEditorDoc *pDoc = GetDocument();
  CBrushSector *pbscHitted = NULL;
  if( (m_penEntityHitOnContext != NULL) &&
      (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) &&
      (m_pbpoRightClickedPolygon != NULL) )
  {
    CopySectorAmbient( m_pbpoRightClickedPolygon->bpo_pbscSector);
  }
}

void CWorldEditorView::OnPasteSectorAmbient()
{
  CWorldEditorDoc *pDoc = GetDocument();
  CBrushSector *pbscHitted = NULL;
  if( (m_penEntityHitOnContext != NULL) &&
      (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) &&
      (m_pbpoRightClickedPolygon != NULL) )
  {
    PasteSectorAmbient( m_pbpoRightClickedPolygon->bpo_pbscSector);
  }
}

void CWorldEditorView::OnSelectAllPolygons( void)
{
  CWorldEditorDoc *pDoc = GetDocument();
  CBrushSector *pbscHitted = NULL;
  if( (m_penEntityHitOnContext != NULL) &&
      (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) &&
      (m_pbpoRightClickedPolygon != NULL) )
  {
    pbscHitted = m_pbpoRightClickedPolygon->bpo_pbscSector;
  }

  if( (pbscHitted == NULL) || pbscHitted->IsSelected( BSCF_SELECTED) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
      {
        if( !itbpo->IsSelected(BPOF_SELECTED)) pDoc->m_selPolygonSelection.Select( *itbpo);
      }
    }
  }
  else
  {
    FOREACHINSTATICARRAY(pbscHitted->bsc_abpoPolygons, CBrushPolygon, itpo)
    {
      if( !itpo->IsSelected(BPOF_SELECTED)) pDoc->m_selPolygonSelection.Select( *itpo);
    }
  }
  pDoc->SetEditingMode( POLYGON_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectAllVertices()
{
  CWorldEditorDoc *pDoc = GetDocument();
  CBrushPolygon *pbpoHitted = NULL;
  if( (m_penEntityHitOnContext != NULL) &&
      (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) &&
      (m_pbpoRightClickedPolygon != NULL) )
  {
    pbpoHitted = m_pbpoRightClickedPolygon;
  }

  if( (pbpoHitted == NULL) || pbpoHitted->IsSelected( BPOF_SELECTED) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      FOREACHINSTATICARRAY(itbpo->bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx)
      {
        if( !(*itpbvx)->IsSelected(BVXF_SELECTED)) pDoc->m_selVertexSelection.Select( **itpbvx);
      }
    }
  }
  else
  {
    FOREACHINSTATICARRAY(pbpoHitted->bpo_apbvxTriangleVertices, CBrushVertex *, itpbvx)
    {
      if( !(*itpbvx)->IsSelected(BVXF_SELECTED)) pDoc->m_selVertexSelection.Select( **itpbvx);
    }
  }
  pDoc->SetEditingMode( VERTEX_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnCopySectors()
{
  OnEditCopy();
}

void CWorldEditorView::OnPasteSectors()
{
  OnEditPaste();
}

void CWorldEditorView::OnDeleteSectors()
{
  OnDeleteEntities();
}


void CWorldEditorView::GetToolTipText( char *pToolTipText)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CPoint CursorPos;
  ::GetCursorPos(&CursorPos);
  ScreenToClient(&CursorPos);
  // obtain information about where mouse points into the world
  BOOL bHitModels = pDoc->GetEditingMode() != POLYGON_MODE;
  BOOL bHitFields = pDoc->GetEditingMode() == ENTITY_MODE;
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse, FALSE, bHitModels, bHitFields);
  CEntity *penEntity = crRayHit.cr_penHit;
  CBrushPolygon *pbpoPolygon = crRayHit.cr_pbpoBrushPolygon;
  CBrushSector *pbscSector = NULL;
  if( pbpoPolygon != NULL) pbscSector = crRayHit.cr_pbpoBrushPolygon->bpo_pbscSector;
  char *pchrCursor = pToolTipText;
  INDEX iLongestLineLetters = 1;

  CDLLEntityClass *pdecDLLClass;
  if( (pDoc->GetEditingMode() == ENTITY_MODE) && (penEntity != NULL))
  {
    // get description line for all properties with name
    pdecDLLClass = penEntity->GetClass()->ec_pdecDLLClass;
    pchrCursor += sprintf(pchrCursor, "Class: %-24.24s\n", pdecDLLClass->dec_strName);
    size_t ctLetters = strlen("Class: ")+strlen(pdecDLLClass->dec_strName);
    memset(pchrCursor, '�', ctLetters);
    pchrCursor+=ctLetters;
    *pchrCursor = '\n';
    pchrCursor++;
  }

  INDEX ctMips = 0;
  INDEX iMipIndex = 0;
  INDEX ctEntities = 0;
  INDEX ctSectors = 0;
  INDEX ctPolygons = 0;
  INDEX ctEdges = 0;
  INDEX ctVertices = 0;
  INDEX ctPlanes = 0;

  // see if info should apply to whole selection
  BOOL bCountSelection = TRUE;
  if( (pDoc->GetEditingMode() == ENTITY_MODE) &&
      (penEntity != NULL) &&
      (!penEntity->IsSelected( ENF_SELECTED)) )
  {
    bCountSelection = FALSE;
  }
  if( (pDoc->GetEditingMode() == POLYGON_MODE) &&
      (pbpoPolygon != NULL) &&
      (!pbpoPolygon->IsSelected( BPOF_SELECTED)) )
  {
    bCountSelection = FALSE;
  }
  if( (pDoc->GetEditingMode() == SECTOR_MODE) &&
      (pbscSector != NULL) &&
      (!pbscSector->IsSelected( BSCF_SELECTED)) )
  {
    bCountSelection = FALSE;
  }
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    CTerrain *ptrTerrain=GetTerrain();
    if(ptrTerrain!=NULL)
    {
      pchrCursor += sprintf(pchrCursor, "%-24s %g x %g x %g\n%-24s %d x %d\n%-24s %d x %d\n%-24s %d x %d\n%-24s %d\n%-24s %d\n%-24s %g\n",
        "Terrain size", ptrTerrain->tr_vTerrainSize(1),ptrTerrain->tr_vTerrainSize(2),ptrTerrain->tr_vTerrainSize(3),
        "Height map size", ptrTerrain->tr_pixHeightMapWidth, ptrTerrain->tr_pixHeightMapHeight,
        "Shadow map size", ptrTerrain->GetShadowMapWidth(), ptrTerrain->GetShadowMapHeight(),
        "Model shading map size", ptrTerrain->GetShadingMapWidth(), ptrTerrain->GetShadingMapHeight(),
        "Layers", ptrTerrain->tr_atlLayers.Count(),
        "Tiles", ptrTerrain->tr_ctTiles,
        "LOD switch distance", ptrTerrain->tr_fDistFactor);
    }
    else
    {
      pchrCursor += sprintf(pchrCursor, "Terrain not selected\n");
    }
  }
  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      if( (bCountSelection && iten->IsSelected( ENF_SELECTED)) ||
          (!bCountSelection && (iten ==penEntity)) )
      {
        ctEntities ++;
        CEntity::RenderType rt = iten->GetRenderType();
        if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
        {
          CBrush3D *pBrush3D = iten->GetBrush();
          if(pBrush3D != NULL)
          {
            FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();
            CBrush3D *pbrBrush = iten->GetBrush();
            if( pbrBrush != NULL)
            {
              CBrushMip *pbmCurrentMip = pbrBrush->GetBrushMipByDistance( fCurrentMipFactor);
              if( pbmCurrentMip != NULL)
              {
                ctSectors += pbmCurrentMip->bm_abscSectors.Count();
                FOREACHINDYNAMICARRAY(pbmCurrentMip->bm_abscSectors, CBrushSector, itbsc)
                {
                  ctPolygons += itbsc->bsc_abpoPolygons.Count();
                  ctEdges += itbsc->bsc_abedEdges.Count();
                  ctVertices+= itbsc->bsc_abvxVertices.Count();
                  ctPlanes += itbsc->bsc_abplPlanes.Count();
                }
              }
            }
          }
        }
      }
    }
    pchrCursor += sprintf(pchrCursor, "%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d\n",
      "No of entities:", ctEntities,
      "No of sectors:", ctSectors,
      "No of polygons:", ctPolygons,
      "No of edges:", ctEdges,
      "No of vertices:", ctVertices,
      "No of planes:", ctPlanes);
    pchrCursor += sprintf(pchrCursor, "%s\n", "������������������������������");

    if( !bCountSelection)
    {
      CEntity::RenderType rt = penEntity->GetRenderType();
      if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        CBrush3D *pBrush3D = penEntity->GetBrush();
        if(pBrush3D != NULL)
        {
          INDEX iMip = 0;
          FOREACHINLIST(CBrushMip, bm_lnInBrush, pBrush3D->br_lhBrushMips, itbm)
          {
            FLOAT fMipSwitchDistance = itbm->GetMipDistance();
            CTString strTmp;
            strTmp.PrintF("Mip %d is visible until ", iMip);
            pchrCursor += sprintf(pchrCursor, "%-24s %g m\n", strTmp, fMipSwitchDistance);
            iMip++;
          }
          pchrCursor += sprintf(pchrCursor, "%s\n", "������������������������������");
        }
      }
    }
  }

  if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    if( bCountSelection)
    {
      INDEX ctEdges = 0;
      ULONG ulShadowMemory = 0;
      FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        ctEdges += itbpo->bpo_abpePolygonEdges.Count();
        ulShadowMemory += itbpo->bpo_smShadowMap.GetShadowSize();
      }

      pchrCursor += sprintf(pchrCursor, "Selection has %d polygons with %d edges\n", pDoc->m_selPolygonSelection.Count(), ctEdges);
      pchrCursor += sprintf(pchrCursor, "Shadows on selected polygons occupy %g kb\n", ulShadowMemory/1024.0f);
    }
    if( pbpoPolygon != NULL)
    {
      CTextureData *ptd;
#define SET_MAPPING_INFO(mp, tex_name)\
ptd = (CTextureData*) mp.bpt_toTexture.GetData();\
if( ptd == NULL) pchrCursor += sprintf(pchrCursor, "%-24s None\n", tex_name);\
else {\
  pchrCursor += sprintf(pchrCursor, "%-24s %.64s %s\n", tex_name, CTString(ptd->GetName()),\
                ptd->GetDescription() );\
  pchrCursor += sprintf(pchrCursor, "%-24s %s     %s\n", " ",\
      "Scroll: \""+pDoc->m_woWorld.wo_attTextureTransformations[mp.s.bpt_ubScroll].tt_strName+"\"",\
      "Blend: \""+pDoc->m_woWorld.wo_atbTextureBlendings[mp.s.bpt_ubBlend].tb_strName+"\"");}

      CBrushPolygon &bpo = *crRayHit.cr_pbpoBrushPolygon;
	  SET_MAPPING_INFO(bpo.bpo_abptTextures[0], (const char*)"Texture 1");
      SET_MAPPING_INFO( bpo.bpo_abptTextures[1], (const char*)"Texture 2");
      SET_MAPPING_INFO( bpo.bpo_abptTextures[2], (const char*)"Texture 3");
      pchrCursor += sprintf(pchrCursor, (const char*)"Polygon under mouse has %d edges\n", bpo.bpo_abpePolygonEdges.Count());
      pchrCursor += sprintf(pchrCursor, (const char*)"Shadow on polygon under mouse occupies %g kb\n", bpo.bpo_smShadowMap.GetShadowSize()/1024.0f);
    }
  }

  if( pDoc->GetEditingMode() == SECTOR_MODE)
  {
    if( bCountSelection)
    {
      ctSectors = pDoc->m_selSectorSelection.Count();
      FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
      {
        ctPolygons += itbsc->bsc_abpoPolygons.Count();
        ctEdges += itbsc->bsc_abedEdges.Count();
        ctVertices+= itbsc->bsc_abvxVertices.Count();
        ctPlanes += itbsc->bsc_abplPlanes.Count();
      }
      pchrCursor += sprintf(pchrCursor, "%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d\n%",
		  (const char*)"No of sectors:", ctSectors,
		  (const char*)"No of polygons:", ctPolygons,
		  (const char*)"No of edges:", ctEdges,
		  (const char*)"No of vertices:", ctVertices,
		  (const char*)"No of planes:", ctPlanes);
    }
    else
    {
      ctPolygons = pbscSector->bsc_abpoPolygons.Count();
      ctEdges = pbscSector->bsc_abedEdges.Count();
      ctVertices = pbscSector->bsc_abvxVertices.Count();
      ctPlanes = pbscSector->bsc_abplPlanes.Count();

      UBYTE ubR, ubG, ubB;
      UBYTE ubH, ubS, ubV;
      ColorToRGB( pbscSector->bsc_colAmbient, ubR, ubG, ubB);
      ColorToHSV( pbscSector->bsc_colAmbient, ubH, ubS, ubV);
      CTString strSectorName = pbscSector->bsc_strName;
      if( strSectorName == "") strSectorName = "<not defined>";

      INDEX iContentType = pbscSector->GetContentType();
      CTString strContentType = pDoc->m_woWorld.wo_actContentTypes[iContentType].ct_strName;
      if( strContentType == "") strContentType = "<not defined>";

      INDEX iEnvironmentType = pbscSector->GetEnvironmentType();
      CTString strEnvironmentType = pDoc->m_woWorld.wo_aetEnvironmentTypes[iEnvironmentType].et_strName;
      if( strEnvironmentType == "") strEnvironmentType = "<not defined>";

      INDEX iForceType = pbscSector->GetForceType();
      CBrush3D *pbrBrush = pbscSector->bsc_pbmBrushMip->bm_pbrBrush;
      CTString strForceType = pbrBrush->br_penEntity->GetForceName( iForceType);
      if( strForceType == "") strForceType = "<not defined>";

      pchrCursor += sprintf(pchrCursor, "%-24.24s %.64s\n%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d\n%-24s"
        "(R=%d G=%d B=%d) (H=%d S=%d V=%d)\n%-24.24s %.64s\n%-24.24s %.64s\n%-24.24s %.64s\n",
        "Name:", strSectorName,
        "No of polygons:", ctPolygons,
        "No of edges:", ctEdges,
        "No of vertices:", ctVertices,
        "No of planes:", ctPlanes,
        "Ambient color:", ubR, ubG, ubB, ubH, ubS, ubV,
        "Content type:", strContentType,
        "Environment type:", strEnvironmentType,
        "Force type:", strForceType);
    }
  }
  if( pDoc->GetEditingMode() == CSG_MODE)
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_pwoSecondLayer->wo_cenEntities, CEntity, iten)
    {
      CEntity::RenderType rt = iten->GetRenderType();
      if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        CBrush3D *pBrush3D = iten->GetBrush();
        if( pBrush3D != NULL)
        {
          FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();
          CBrush3D *pbrBrush = iten->GetBrush();
          if( pbrBrush != NULL)
          {
            CBrushMip *pbmCurrentMip = pbrBrush->GetBrushMipByDistance( fCurrentMipFactor);
            if( pbmCurrentMip != NULL)
            {
              FOREACHINDYNAMICARRAY(pbmCurrentMip->bm_abscSectors, CBrushSector, itbsc)
              {
                ctPolygons += itbsc->bsc_abpoPolygons.Count();
                ctEdges += itbsc->bsc_abedEdges.Count();
                ctVertices+= itbsc->bsc_abvxVertices.Count();
                ctPlanes += itbsc->bsc_abplPlanes.Count();
              }
            }
          }
        }
      }
    }
    pchrCursor += sprintf(pchrCursor, "Primitive info:\n���������������\n%-24s %d\n%-24s %d\n%-24s %d\n%-24s %d",
      "No of polygons:", ctPolygons,
      "No of edges:", ctEdges,
      "No of vertices:", ctVertices,
      "No of planes:", ctPlanes);
  }

  if( (pDoc->GetEditingMode() == ENTITY_MODE) && (penEntity!=NULL) )
  {
    // add count of occupiing sectors and their names
    INDEX ctIntersectingSectors = penEntity->en_rdSectors.Count();
    INDEX ctWithName = 0;
    {FOREACHSRCOFDST(penEntity->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
      if( pbsc->bsc_strName != "") ctWithName++;
    ENDFOR}
    INDEX ctLetters = sprintf(pchrCursor, "In %d sectors (%d with name):\n", ctIntersectingSectors, ctWithName);
    if( ctLetters > iLongestLineLetters) iLongestLineLetters = ctLetters;
    pchrCursor += ctLetters;
    {FOREACHSRCOFDST(penEntity->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
      if( pbsc->bsc_strName != "")
      {
        INDEX ctLetters = sprintf(pchrCursor, "In: %-24.24s\n", pbsc->bsc_strName);
        if( ctLetters > iLongestLineLetters) iLongestLineLetters = ctLetters;
        pchrCursor += ctLetters;
      }
    ENDFOR}

    for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      CTString strValue;
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
        if( epProperty.ep_strName == CTString("")) continue;
        strValue = "!!! Unkown property type";
        switch( epProperty.ep_eptType)
        {
        case CEntityProperty::EPT_FLAGS:
          {
            strValue = "Flag array property";
          }
        case CEntityProperty::EPT_ENUM:
          {
            INDEX iCurrentEnum = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, INDEX);
            CEntityPropertyEnumType *epEnum = epProperty.ep_pepetEnumType;
            for( INDEX iEnum = 0; iEnum< epEnum->epet_ctValues; iEnum++)
            {
              INDEX iEnumID = epEnum->epet_aepevValues[ iEnum].epev_iValue;
              if( iEnumID == iCurrentEnum)
              strValue = (epEnum->epet_aepevValues[ iEnum].epev_strName);
            }
            break;
          }
        case CEntityProperty::EPT_BOOL:
          {
            if( ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, BOOL))strValue = "TRUE";
            else                                                           strValue = "FALSE";
            break;
          }
        case CEntityProperty::EPT_FLOAT:
          {
            strValue.PrintF( "%g", ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, FLOAT));
            break;
          }
        case CEntityProperty::EPT_RANGE:
          {
            strValue.PrintF( "%g m", ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, FLOAT));
            break;
          }
        case CEntityProperty::EPT_COLOR:
          {
            COLOR colColor = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, COLOR);
            UBYTE ubR, ubG, ubB, ubA;
            UBYTE ubH, ubS, ubV;
            ubA=colColor&0xFF;
            ColorToRGB( colColor, ubR, ubG, ubB);
            ColorToHSV( colColor, ubH, ubS, ubV);
            strValue.PrintF( "RGBA(%d,%d,%d,%d) HSV(%d,%d,%d)", ubR,ubG,ubB,ubA,ubH,ubS,ubV);
            break;
          }
        case CEntityProperty::EPT_STRING:
        case CEntityProperty::EPT_STRINGTRANS:
        case CEntityProperty::EPT_FILENAMENODEP:
          {
            strValue = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, CTString);
            if( strValue == "") strValue = "<no value>";
            break;
          }
        case CEntityProperty::EPT_ENTITYPTR:
          {
            CEntity *pen=ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, CEntity *);
            if( pen == NULL) strValue = "-> None";
            else strValue = "-> "+pen->GetName();
            break;
          }
        case CEntityProperty::EPT_PARENT:
          {
            CEntity *pen=penEntity->GetParent();
            if( pen == NULL) strValue = "-> None";
            else strValue = "-> "+pen->GetName();
            break;
          }
        case CEntityProperty::EPT_FILENAME:
          {
            strValue = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, CTFileName);
            if( strValue == "") strValue = "<no file>";
            break;
          }
        case CEntityProperty::EPT_INDEX:
        case CEntityProperty::EPT_ANGLE:
          {
            strValue.PrintF( "%d", ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, INDEX) );
            break;
          }
        case CEntityProperty::EPT_ANIMATION:
          {
            CAnimData *pAD = penEntity->GetAnimData( epProperty.ep_slOffset);
            if( pAD == NULL)
            {
              strValue = "<no animation>";
            }
            else
            {
              CAnimInfo aiInfo;
              INDEX iAnimation = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, INDEX);
              pAD->GetAnimInfo( iAnimation, aiInfo);
              strValue = aiInfo.ai_AnimName;
            }
            break;
          }
        case CEntityProperty::EPT_ILLUMINATIONTYPE:
          {
            INDEX iIllumination = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, INDEX);
            strValue = pDoc->m_woWorld.wo_aitIlluminationTypes[iIllumination].it_strName;
            break;
          }
        case CEntityProperty::EPT_FLOATAABBOX3D:
          {
            FLOATaabbox3D bbox = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, FLOATaabbox3D);
            FLOAT3D vMin = bbox.Min();
            FLOAT3D vMax = bbox.Max();
            strValue.PrintF( "Min(%g,%g,%g), Max(%g,%g,%g)",
              vMin(1), vMin(2), vMin(3), vMax(1), vMax(2), vMax(3));
            break;
          }
        case CEntityProperty::EPT_FLOAT3D:
          {
            FLOAT3D vVector = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, FLOAT3D);
            strValue.PrintF( "%g,%g,%g", vVector(1), vVector(2), vVector(3));
            break;
          }
        case CEntityProperty::EPT_ANGLE3D:
          {
            ANGLE3D aAngle = ENTITYPROPERTY( &*penEntity, epProperty.ep_slOffset, ANGLE3D);
            strValue.PrintF( "%g,%g,%g", DegAngle(aAngle(1)), DegAngle(aAngle(2)), DegAngle(aAngle(3)));
            break;
          }
        }
        INDEX ctLetters = sprintf(pchrCursor, "%-24.24s %.64s \n", epProperty.ep_strName, strValue);
        if( ctLetters > iLongestLineLetters) iLongestLineLetters = ctLetters;
        pchrCursor += ctLetters;
      }
    }
    if( iLongestLineLetters>2)
    {
      iLongestLineLetters-=2;
      memset(pchrCursor, '_', iLongestLineLetters);
      pchrCursor+=iLongestLineLetters;
      *pchrCursor = '\n';
      pchrCursor++;
    }

    if( penEntity->GetParent() != NULL)
    {
      pchrCursor += sprintf(pchrCursor, "Parent: %s\n", penEntity->GetParent()->GetName());
    }
    else
    {
      pchrCursor += sprintf(pchrCursor, "No parent\n");
    }

    CTString strSpawn = "";
    ULONG ulSpawn = penEntity->GetSpawnFlags();
    ULONG ulAllways = SPF_EASY|SPF_NORMAL|SPF_HARD|SPF_EXTREME|SPF_SINGLEPLAYER|SPF_COOPERATIVE|SPF_DEATHMATCH;
    if( (ulSpawn & ulAllways) == ulAllways)
    {
      pchrCursor += sprintf(pchrCursor, "%s", "Entity exists allways");
    }
    else
    {
      if( ulSpawn & SPF_EASY) strSpawn+="Easy,";
      if( ulSpawn & SPF_NORMAL) strSpawn+="Normal,";
      if( ulSpawn & SPF_HARD) strSpawn+="Hard,";
      if( ulSpawn & SPF_EXTREME) strSpawn+="Extreme,";
      if( ulSpawn & SPF_SINGLEPLAYER) strSpawn+="Single,";
      if( ulSpawn & SPF_COOPERATIVE) strSpawn+="Cooperative,";
      if( ulSpawn & SPF_DEATHMATCH) strSpawn+="Deathmatch,";
      if( strSpawn != "")
      {
        pchrCursor += sprintf(pchrCursor, "%s", strSpawn);
        *(pchrCursor-1) = 0;
      }
      else
      {
        pchrCursor += sprintf(pchrCursor, "%s", "Does not exist acording to spawn flags");
      }
    }
  }
  if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    pchrCursor += sprintf(pchrCursor, "%d %s", pDoc->m_selVertexSelection.Count(), "vertices");
  }
}

void CWorldEditorView::OnMenuCopyMapping()
{
  CWorldEditorDoc *pDoc = GetDocument();
  ASSERT( m_pbpoRightClickedPolygon != NULL);
  CopyMapping(m_pbpoRightClickedPolygon);
}

void CWorldEditorView::OnKeyPaste()
{
	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_PAINT;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
  }
  else
  {
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
    // if we hit brush entity
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      // paste mapping
      PasteMapping( crRayHit.cr_pbpoBrushPolygon, FALSE);
    }
  }
}

void CWorldEditorView::OnKeyPasteAsProjected()
{
	CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    // paste mapping
    PasteMapping( crRayHit.cr_pbpoBrushPolygon, TRUE);
  }
}

void CWorldEditorView::OnMenuPasteMapping()
{
  PasteMapping( m_pbpoRightClickedPolygon, FALSE);
}

void CWorldEditorView::OnMenuPasteAsProjectedMapping()
{
  PasteMapping( m_pbpoRightClickedPolygon, TRUE);
}

void CWorldEditorView::CopyMapping(CBrushPolygon *pbpo)
{
  CWorldEditorDoc *pDoc = GetDocument();
  ASSERT(pbpo!=NULL);
  CBrushPlane *pbpl = pbpo->bpo_pbplPlane;
  CEntity *pen = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  ASSERT(pen!=NULL);
  // get the mapping in absolute space
  theApp.m_mdMapping = pbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
  theApp.m_mdMapping.Transform(pbpl->bpl_plRelative, pen->GetPlacement(), _plOrigin);
  theApp.m_plMapping = pbpl->bpl_plAbsolute;

  theApp.m_mdMapping1 = pbpo->bpo_abptTextures[0].bpt_mdMapping;
  theApp.m_mdMapping1.Transform(pbpl->bpl_plRelative, pen->GetPlacement(), _plOrigin);
  theApp.m_plMapping1 = pbpl->bpl_plAbsolute;
  theApp.m_mdMapping2 = pbpo->bpo_abptTextures[1].bpt_mdMapping;
  theApp.m_mdMapping2.Transform(pbpl->bpl_plRelative, pen->GetPlacement(), _plOrigin);
  theApp.m_plMapping2 = pbpl->bpl_plAbsolute;
  theApp.m_mdMapping3 = pbpo->bpo_abptTextures[2].bpt_mdMapping;
  theApp.m_mdMapping3.Transform(pbpl->bpl_plRelative, pen->GetPlacement(), _plOrigin);
  theApp.m_plMapping3 = pbpl->bpl_plAbsolute;
}

void CWorldEditorView::PasteMappingOnOnePolygon(CBrushPolygon *pbpo, BOOL bAsProjected)
{
  CWorldEditorDoc *pDoc = GetDocument();
  PasteOneLayerMapping(pDoc->m_iTexture, theApp.m_mdMapping, theApp.m_plMapping, pbpo, bAsProjected);
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}
  
void CWorldEditorView::PasteOneLayerMapping(INDEX iLayer, CMappingDefinition &md,
                                            FLOATplane3D &pl, CBrushPolygon *pbpo, BOOL bAsProjected)
{
  CWorldEditorDoc *pDoc = GetDocument();
  // get the mapping in relative space of the brush polygon's entity
  CEntity *pen = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
  CSimpleProjection3D pr;
  pr.ObjectPlacementL() = _plOrigin;
  pr.ViewerPlacementL() = pen->GetPlacement();
  pr.Prepare();
  FLOATplane3D plRelative;
  pr.Project(pl, plRelative);
  CMappingDefinition mdRelative = md;
  mdRelative.Transform(pl, _plOrigin, pen->GetPlacement());

  // paste the mapping
  if( bAsProjected)
  {
    pbpo->bpo_abptTextures[iLayer].bpt_mdMapping.ProjectMapping(
       plRelative, mdRelative, pbpo->bpo_pbplPlane->bpl_plRelative);
  }
  else
  {
    pbpo->bpo_abptTextures[iLayer].bpt_mdMapping = mdRelative;
  }
}

void CWorldEditorView::PasteMapping(CBrushPolygon *pbpo, BOOL bAsProjected)
{
  CWorldEditorDoc *pDoc = GetDocument();
  // paste mapping over selection if clicked polygon is selected
  if( (pbpo == NULL) || (pbpo->IsSelected( BPOF_SELECTED)) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      PasteMappingOnOnePolygon(itbpo, bAsProjected);
    }
  }
  else
  {
    PasteMappingOnOnePolygon(pbpo, bAsProjected);
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectAllEntitiesInSectors()
{
  CWorldEditorDoc *pDoc = GetDocument();
  CBrushSector *pbscHitted = NULL;
  if( (m_penEntityHitOnContext != NULL) &&
      (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) &&
      (m_pbpoRightClickedPolygon != NULL) )
  {
    pbscHitted = m_pbpoRightClickedPolygon->bpo_pbscSector;
  }

  // if right clicked on nothing or selected sector
  if( (pbscHitted == NULL) || pbscHitted->IsSelected( BSCF_SELECTED) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      {FOREACHDSTOFSRC(itbsc->bsc_rsEntities, CEntity, en_rdSectors, pen)
        if( !pen->IsSelected( ENF_SELECTED) )
        {
          pDoc->m_selEntitySelection.Select( *pen);
        }
      ENDFOR}
    }
  }
  else
  {
    {FOREACHDSTOFSRC(pbscHitted->bsc_rsEntities, CEntity, en_rdSectors, pen)
      if( !pen->IsSelected( ENF_SELECTED) )
      {
        pDoc->m_selEntitySelection.Select( *pen);
      }
    ENDFOR}
  }
  pDoc->SetEditingMode( ENTITY_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectAllSectors()
{
  CWorldEditorDoc *pDoc = GetDocument();
  if( m_penEntityHitOnContext == NULL)
  {
    // in all entities in world
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      CEntity::RenderType rt = iten->GetRenderType();
      if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->GetBrush()->br_lhBrushMips, itbm)
        {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
          {
            // if sector is not hidden and not selected
            if( !(itbsc->bsc_ulFlags & BSCF_HIDDEN) && !itbsc->IsSelected( BSCF_SELECTED) )
            {
              // select it
              pDoc->m_selSectorSelection.Select( *itbsc);
            }
          }
        }
      }
    }
  }
  // perform select sectors on whole entity selection
  else if( m_penEntityHitOnContext->IsSelected( ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      CEntity::RenderType rt = iten->GetRenderType();
      if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->GetBrush()->br_lhBrushMips, itbm)
        {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
          {
            // if sector is not hidden and not selected
            if( !(itbsc->bsc_ulFlags & BSCF_HIDDEN) && !itbsc->IsSelected( BSCF_SELECTED) )
            {
              // select it
              pDoc->m_selSectorSelection.Select( *itbsc);
            }
          }
        }
      }
    }
  }
  else if (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH)
  {
    // select all sectors in all mip levels
    FOREACHINLIST(CBrushMip, bm_lnInBrush, m_penEntityHitOnContext->GetBrush()->br_lhBrushMips, itbm)
    {
      // select all sectors in current mip
      FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
      {
        if( !(itbsc->bsc_ulFlags & BSCF_HIDDEN) && !itbsc->IsSelected( BSCF_SELECTED) )
        {
          pDoc->m_selSectorSelection.Select( *itbsc);
        }
      }
    }
  }
  pDoc->SetEditingMode( SECTOR_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::CenterSelected(void)
{
  CWorldEditorDoc *pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CPropertyID *ppidProperty = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
  // bounding box of visible sectors
  FLOATaabbox3D boxBoundingBox;
  if( pDoc->GetEditingMode() == ENTITY_MODE && (pDoc->m_selEntitySelection.Count() != 0) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      FLOAT3D vPos = iten->GetPlacement().pl_PositionVector;
      FLOATaabbox3D boxEntity;

      if( (ppidProperty != NULL) && (ppidProperty->pid_eptType == CEntityProperty::EPT_RANGE))
      {
        // obtain property ptr
        CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
        // get editing range
        FLOAT fRange = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT);
        boxEntity |= FLOATaabbox3D( FLOAT3D(0.0f, 0.0f ,0.0f), fRange);
      }
      else
      {
        iten->GetSize( boxEntity);
      }
      boxEntity+=vPos;
      boxBoundingBox |= boxEntity;
    }
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    CTerrain *ptrTerrain=GetTerrain();
    if(ptrTerrain!=NULL)
    {
      ptrTerrain->GetAllTerrainBBox(boxBoundingBox);
    }
  }
  else if( pDoc->GetEditingMode() == POLYGON_MODE && (pDoc->m_selPolygonSelection.Count() != 0) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      boxBoundingBox |= itbpo->bpo_boxBoundingBox;
    }
  }
  else if( pDoc->GetEditingMode() == SECTOR_MODE && (pDoc->m_selSectorSelection.Count() != 0) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      boxBoundingBox |= itbsc->bsc_boxBoundingBox;
    }
  }
  else if( (pDoc->GetEditingMode() == VERTEX_MODE) && (pDoc->m_selVertexSelection.Count() != 0) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selVertexSelection, CBrushVertex, itbvtx)
    {
      FLOATaabbox3D boxVtxBox = FLOATaabbox3D( itbvtx->bvx_vAbsolute);
      boxVtxBox.Expand( 20);
      boxBoundingBox |= boxVtxBox;
    }
  }
  else
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      CEntity::RenderType rt = iten->GetRenderType();
      if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->GetBrush()->br_lhBrushMips, itbm)
        {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
          {
            // if sector is not hidden
            if( !(itbsc->bsc_ulFlags & BSCF_HIDDEN))
            {
              boxBoundingBox |= itbsc->bsc_boxBoundingBox;
            }
          }
        }
      }
      else
      {
        FLOAT3D vPos = iten->GetPlacement().pl_PositionVector;
        FLOATaabbox3D boxEntity;
        iten->GetSize( boxEntity);
        boxEntity+=vPos;
        boxBoundingBox |= boxEntity;
      }
    }
  }
  AllignBox( boxBoundingBox);
}

void CWorldEditorView::AllignBox( FLOATaabbox3D bbox)
{
  CWorldEditorDoc *pDoc = GetDocument();
  CChildFrame *pCF = GetChildFrame();
  FLOAT3D vSize = bbox.Size();
  if( vSize.Length() <= 1.0f)
    vSize = FLOAT3D( 2.0f, 2.0f, 2.0f);

  // width is alligned inside horizontal borders of draw port, height and lenght
  // are alligned to height of draw port
  FLOAT fDX = (FLOAT)m_pdpDrawPort->GetWidth()*9/10;
  FLOAT fDY = (FLOAT)m_pdpDrawPort->GetHeight()*9/10;

  FLOAT fWantedZoom = Min( Min( fDX/vSize(1), fDY/vSize(2)), fDY/vSize(3));

  if( (fWantedZoom>1E-4) && (fWantedZoom<1E4) )
  {
    // create a slave viewer
    CSlaveViewer svViewer(pCF->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid,
                          m_pdpDrawPort);
    pCF->m_mvViewer.mv_fTargetDistance = svViewer.GetDistanceForZoom(fWantedZoom);
    pCF->m_mvViewer.SetTargetPlacement( bbox.Center());
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::AllignPolygon( CBrushPolygon *pbpo)
{
  CWorldEditorDoc *pDoc = GetDocument();
  CChildFrame *pCF = GetChildFrame();
  
  FLOATaabbox3D bbox = pbpo->bpo_boxBoundingBox;
  FLOAT3D vSize = bbox.Size();
  if( vSize.Length() <= 1.0f)
    vSize = FLOAT3D( 2.0f, 2.0f, 2.0f);

  // width is alligned inside horizontal borders of draw port, height and lenght
  // are alligned to height of draw port
  FLOAT fDX = (FLOAT)m_pdpDrawPort->GetWidth()*9/10;
  FLOAT fDY = (FLOAT)m_pdpDrawPort->GetHeight()*9/10;

  FLOAT fWantedZoom = Min( Min( fDX/vSize(1), fDY/vSize(2)), fDY/vSize(3));
  if( (fWantedZoom>1E-4) && (fWantedZoom<1E4) )
  {
    // create a slave viewer
    CSlaveViewer svViewer(pCF->m_mvViewer, m_ptProjectionType, pDoc->m_plGrid, m_pdpDrawPort);
    pCF->m_mvViewer.mv_fTargetDistance = svViewer.GetDistanceForZoom(fWantedZoom);
    FLOAT3D vDirection = pbpo->bpo_pbplPlane->bpl_plAbsolute;
    DirectionVectorToAngles( -vDirection, pCF->m_mvViewer.mv_plViewer.pl_OrientationAngle);
    pCF->m_mvViewer.SetTargetPlacement( bbox.Center());
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnCloneToMorePreciseMip()
{
  OnAddMorePreciseMip(TRUE);
}

void CWorldEditorView::OnCreateEmptyMorePreciseMip()
{
  OnAddMorePreciseMip(FALSE);
}

void CWorldEditorView::OnCloneToRougherMipLevel()
{
  OnAddRougherMipLevel( TRUE);
}

void CWorldEditorView::OnCreateEmptyRougherMip()
{
  OnAddRougherMipLevel( FALSE);
}

void CWorldEditorView::OnAddMorePreciseMip(BOOL bClone)
{
  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->RememberUndo();

  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  if (pbmCurrentMip==NULL) {
    return;
  }
  CBrushMip *pbmAdded = pbmCurrentMip->bm_pbrBrush->NewBrushMipBefore(pbmCurrentMip, bClone);

  GetChildFrame()->m_fManualMipBrushingFactor = pbmAdded->GetMipDistance()-0.01f;

  // document has changed
  pDoc->SetModifiedFlag();
  // update all views
  pDoc->UpdateAllViews( NULL);
  // set text describing data that is edited
  SetEditingDataPaneInfo( TRUE);
}

void CWorldEditorView::OnAddRougherMipLevel(BOOL bClone)
{
  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->RememberUndo();

  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  if (pbmCurrentMip==NULL) {
    return;
  }
  CBrushMip *pbmAdded = pbmCurrentMip->bm_pbrBrush->NewBrushMipAfter(pbmCurrentMip, bClone);

  GetChildFrame()->m_fManualMipBrushingFactor = pbmAdded->GetMipDistance()-0.01f;

  // document has changed
  pDoc->SetModifiedFlag();
  // update all views
  pDoc->UpdateAllViews( NULL);
  // set text describing data that is edited
  SetEditingDataPaneInfo( TRUE);
}

// get current brush mip of current csg target brush
CBrushMip *CWorldEditorView::GetCurrentBrushMip(void)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CEntity *penBrush = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();
  if (penBrush==NULL) {
    return NULL;
  }
  // get entity's brush
  CBrush3D *pbrBrush = penBrush->GetBrush();
  if( pbrBrush == NULL) {
    return NULL;
  }
  // get currently active mip factor
  FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();

  return pbrBrush->GetBrushMipByDistance(fCurrentMipFactor);
}

void CWorldEditorView::OnDeleteMip()
{
  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();

  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  if (pbmCurrentMip==NULL) {
    return;
  }
  // delete currently visible mip brush
  pbmCurrentMip->bm_pbrBrush->DeleteBrushMip(pbmCurrentMip);

  CWorldEditorDoc* pDoc = GetDocument();
  // document has changed
  pDoc->SetModifiedFlag();
  // update all views
  pDoc->UpdateAllViews( NULL);
  // set text describing data that is edited
  SetEditingDataPaneInfo( TRUE);
}


void CWorldEditorView::OnPreviousMipBrush()
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CEntity *penBrush = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();

  if (penBrush == NULL) return;

  CBrush3D &brBrush = *penBrush->en_pbrBrush;

  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();

  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  CBrushMip *pbmPrevMip = NULL;
  
  if (pbmCurrentMip==NULL)
  {
    pbmPrevMip = brBrush.GetLastMip();
  }
  else
  {
    pbmPrevMip = pbmCurrentMip->GetPrev();
  }
  if (pbmPrevMip==NULL) return;

  // set manual mip factor to show previous mip brush
  GetChildFrame()->m_fManualMipBrushingFactor = pbmPrevMip->GetMipDistance()-0.01f;

  // update all views
	CWorldEditorDoc* pDoc = GetDocument();
  pDoc->UpdateAllViews( NULL);
  // set text describing data that is edited
  SetEditingDataPaneInfo( TRUE);
}

void CWorldEditorView::OnNextMipBrush()
{
  // remember current time as time when last mip brushing option has been used
  _fLastMipBrushingOptionUsed = _pTimer->GetRealTimeTick();

  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  if (pbmCurrentMip==NULL) {
    return;
  }

  CBrushMip *pbmNextMip = pbmCurrentMip->GetNext();
  if (pbmNextMip==NULL) {
    return;
  }

  // set manual mip factor to show next mip brush
  GetChildFrame()->m_fManualMipBrushingFactor = pbmNextMip->GetMipDistance()-0.01f;

  // update all views
	CWorldEditorDoc* pDoc = GetDocument();
  pDoc->UpdateAllViews( NULL);
  // set text describing data that is edited
  SetEditingDataPaneInfo( TRUE);
}

void CWorldEditorView::OnUpdateCloneToMorePreciseMip(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetCurrentBrushMip()!=NULL && !GetChildFrame()->m_bAutoMipBrushingOn);
}

void CWorldEditorView::OnUpdateCloneToRougherMipLevel(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetCurrentBrushMip()!=NULL && !GetChildFrame()->m_bAutoMipBrushingOn);
}

void CWorldEditorView::OnUpdateCreateEmptyMorePreciseMip(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetCurrentBrushMip()!=NULL && !GetChildFrame()->m_bAutoMipBrushingOn);
}

void CWorldEditorView::OnUpdateCreateEmptyRougherMip(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( GetCurrentBrushMip()!=NULL && !GetChildFrame()->m_bAutoMipBrushingOn);
}

void CWorldEditorView::OnUpdateDeleteMip(CCmdUI* pCmdUI)
{
  CBrushMip *pbm = GetCurrentBrushMip();
  pCmdUI->Enable(
    pbm!=NULL &&
    pbm->bm_pbrBrush->br_lhBrushMips.Count()>1 &&
    !GetChildFrame()->m_bAutoMipBrushingOn);
}

void CWorldEditorView::OnUpdatePreviousMipBrush(CCmdUI* pCmdUI)
{
  CBrushMip *pbm = GetCurrentBrushMip();
  pCmdUI->Enable(
    ( (pbm==NULL) ||
      (pbm->GetPrev()!=NULL) ) &&
    !GetChildFrame()->m_bAutoMipBrushingOn);
}

void CWorldEditorView::OnUpdateNextMipBrush(CCmdUI* pCmdUI)
{
  CBrushMip *pbm = GetCurrentBrushMip();
  pCmdUI->Enable(
    pbm!=NULL &&
    pbm->GetNext()!=NULL &&
    !GetChildFrame()->m_bAutoMipBrushingOn);
}

// paste polygon properties but don't paste mapping
void CWorldEditorView::OnEditPasteAlternative()
{
  CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);

  if( theApp.m_ctLastCopyType == CT_POLYGON_PROPERTIES)
  {
    // if mouse is over polygon
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      if( crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED))
      {
        FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
        {
          // apply all parameters from remembered
          itbpo->CopyProperties( *theApp.m_pbpoClipboardPolygon);
        }
      }
      else
      {
        // apply all parameters from remembered
        crRayHit.cr_pbpoBrushPolygon->CopyProperties( *theApp.m_pbpoClipboardPolygon);
      }
    }
    pDoc->m_chSelections.MarkChanged();
    pDoc->SetModifiedFlag();
    pDoc->UpdateAllViews( NULL);
  }
  if( theApp.m_ctLastCopyType == CT_POLYGON_PROPERTIES_ALTERNATIVE)
  {
    // if mouse is over polygon
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      if( crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED))
      {
        FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
        {
          // apply all parameters from remembered
          itbpo->CopyProperties( *theApp.m_pbpoClipboardPolygon);
          PasteOneLayerMapping(0, theApp.m_mdMapping1, theApp.m_plMapping1, &*itbpo, TRUE);
          PasteOneLayerMapping(1, theApp.m_mdMapping2, theApp.m_plMapping2, &*itbpo, TRUE);
          PasteOneLayerMapping(2, theApp.m_mdMapping3, theApp.m_plMapping3, &*itbpo, TRUE);
        }
      }
      else
      {
        // apply all parameters from remembered
        crRayHit.cr_pbpoBrushPolygon->CopyProperties( *theApp.m_pbpoClipboardPolygon);
        PasteOneLayerMapping(0, theApp.m_mdMapping1, theApp.m_plMapping1, crRayHit.cr_pbpoBrushPolygon, TRUE);
        PasteOneLayerMapping(1, theApp.m_mdMapping2, theApp.m_plMapping2, crRayHit.cr_pbpoBrushPolygon, TRUE);
        PasteOneLayerMapping(2, theApp.m_mdMapping3, theApp.m_plMapping3, crRayHit.cr_pbpoBrushPolygon, TRUE);
      }
    }
    pDoc->m_chSelections.MarkChanged();
    pDoc->SetModifiedFlag();
    pDoc->UpdateAllViews( NULL);
  }  
  else if (theApp.m_ctLastCopyType == CT_ENTITY)
  {
    CWorldEditorDoc* pDoc = GetDocument();
    CPlacement3D plPaste = GetMouseInWorldPlacement();
    // load world from clipboard file and start template CSG
    pDoc->StartTemplateCSG( plPaste, (CTString)"Temp\\ClipboardEntityWorld.wld");
    // update all views
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnUpdateEditPasteAlternative(CCmdUI* pCmdUI)
{
}

void CWorldEditorView::ToggleHittedPolygon( CCastRay &crRayHit)
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( (crRayHit.cr_penHit == NULL) ||
      (crRayHit.cr_pbpoBrushPolygon == NULL) )
  {
    return;
  }

  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  // remember if first clicked polygon is to be deselected
  m_bWeDeselectedFirstPolygon = crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED) != 0;
  INDEX ctPolygonsSelectedBefore = pDoc->m_selPolygonSelection.Count();
  // if shift is not pressed
  if( !bShift)
  {
    if( ctPolygonsSelectedBefore>1)
    {
      m_bWeDeselectedFirstPolygon = FALSE;
    }
    // deselect all selected polygons
    pDoc->m_selPolygonSelection.Clear();
  }
  // if we have to deselect first polygon but we didn't deselect it with clear
  if( (m_bWeDeselectedFirstPolygon) &&
      (crRayHit.cr_pbpoBrushPolygon->IsSelected( BPOF_SELECTED)) )
  {
    // deselect clicked polygon
    pDoc->m_selPolygonSelection.Deselect( *crRayHit.cr_pbpoBrushPolygon);
  }
  // if we must select first polygon
  if( !m_bWeDeselectedFirstPolygon)
  {
    // select it
    pDoc->m_selPolygonSelection.Select( *crRayHit.cr_pbpoBrushPolygon);
  }
  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  // update all views
  pDoc->UpdateAllViews( this);
}

void CWorldEditorView::ToggleHittedSector( CCastRay &crRayHit)
{
  CWorldEditorDoc* pDoc = GetDocument();
  // if any polygon hit toggle or add its sector
  if( (crRayHit.cr_penHit == NULL) ||
      (crRayHit.cr_pbscBrushSector == NULL) )
  {
    return;
  }

  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  // remember if first clicked sector is to be deselected
  m_bWeDeselectedFirstSector = crRayHit.cr_pbscBrushSector->IsSelected( BSCF_SELECTED) != 0;
  INDEX ctSectorsSelectedBefore = pDoc->m_selSectorSelection.Count();
  // if shift is not pressed
  if( !bShift)
  {
    if( ctSectorsSelectedBefore>1)
    {
      m_bWeDeselectedFirstSector = FALSE;
    }
    // deselect all selected sectors
    pDoc->m_selSectorSelection.Clear();
  }
  // if we have to deselect first sector but we didn't deselect it with clear
  if( (m_bWeDeselectedFirstSector) &&
      (crRayHit.cr_pbscBrushSector->IsSelected( BSCF_SELECTED)) )
  {
    // deselect clicked sector
    pDoc->m_selSectorSelection.Deselect( *crRayHit.cr_pbscBrushSector);
  }
  // if we must select first sector
  if( !m_bWeDeselectedFirstSector)
  {
    // select it
    pDoc->m_selSectorSelection.Select( *crRayHit.cr_pbscBrushSector);
  }
  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  // update all views
  pDoc->UpdateAllViews( this);
}

void CWorldEditorView::OnSelectAllEntitiesInWorld()
{
  CWorldEditorDoc* pDoc = GetDocument();
  // deselect all entities
  pDoc->m_selEntitySelection.Clear();

  // add each entity in world
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    CBrushSector *pbscSector = iten->GetFirstSector();
    BOOL bSectorVisible = (pbscSector == NULL) ||
                         !(pbscSector->bsc_ulFlags & BSCF_HIDDEN);
    // if it isn't classified in hidden sector and is not hidden itself
    if( bSectorVisible && !(iten->en_ulFlags&ENF_HIDDEN))
    {
      // add it to selection
      pDoc->m_selEntitySelection.Select( *iten);
    }
  }
  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  // update all views
  pDoc->UpdateAllViews( NULL);
}


void CWorldEditorView::OnSelectSectorsWithSameName()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( m_pbpoRightClickedPolygon == NULL) return;

  // select all sectors in world with same name
  CBrushSector *pbscSrc = m_pbpoRightClickedPolygon->bpo_pbscSector;
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    CEntity::RenderType rt = iten->GetRenderType();
    if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
    {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->GetBrush()->br_lhBrushMips, itbm)
      {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
        {
          // if sector is not hidden and has same name, select it
          if( !(itbsc->bsc_ulFlags & BSCF_HIDDEN) &&
               (itbsc->bsc_strName==pbscSrc->bsc_strName) )
          {
            if( !itbsc->IsSelected( BSCF_SELECTED))
            {
              pDoc->m_selSectorSelection.Select( *itbsc);
            }
          }
        }
      }
    }
  }
  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  // update all views
  pDoc->UpdateAllViews( this);
}

void CWorldEditorView::OnSelectSectorsArroundEntity()
{
  CWorldEditorDoc *pDoc = GetDocument();
  // if none selected
  if(pDoc->m_selEntitySelection.Count() == 0)
  {
    // perform on ray-hitted entity
    BOOL bHitModels = pDoc->GetEditingMode() != POLYGON_MODE;
    BOOL bHitFields = pDoc->GetEditingMode() == ENTITY_MODE;
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse, FALSE, bHitModels, bHitFields);
    // if none of entities is hitted
    if(crRayHit.cr_penHit == NULL)  return;
    {FOREACHSRCOFDST(crRayHit.cr_penHit->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
      // if sector is not hidden and not selected
      if( !(pbsc->bsc_ulFlags & BSCF_HIDDEN) && !pbsc->IsSelected( BSCF_SELECTED) )
      {
        // select it
        pDoc->m_selSectorSelection.Select( *pbsc);
      }
    ENDFOR}
  }
  else
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      {FOREACHSRCOFDST(iten->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
        // if sector is not hidden and not selected
        if( !(pbsc->bsc_ulFlags & BSCF_HIDDEN) && !pbsc->IsSelected( BSCF_SELECTED) )
        {
          // select it
          pDoc->m_selSectorSelection.Select( *pbsc);
        }
      ENDFOR}
    }
  }

  pDoc->SetEditingMode( SECTOR_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectSectorsArroundEntityOnContext()
{
  CWorldEditorDoc *pDoc = GetDocument();
  if( m_penEntityHitOnContext == NULL)
  {
    return;
  }

  // perform select sectors on whole entity selection
  if( m_penEntityHitOnContext->IsSelected( ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      {FOREACHSRCOFDST(iten->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
        // if sector is not hidden and not selected
        if( !(pbsc->bsc_ulFlags & BSCF_HIDDEN) && !pbsc->IsSelected( BSCF_SELECTED) )
        {
          // select it
          pDoc->m_selSectorSelection.Select( *pbsc);
        }
      ENDFOR}
    }
  }
  else
  {
    {FOREACHSRCOFDST(m_penEntityHitOnContext->en_rdSectors, CBrushSector, bsc_rsEntities, pbsc)
      // if sector is not hidden and not selected
      if( !(pbsc->bsc_ulFlags & BSCF_HIDDEN) && !pbsc->IsSelected( BSCF_SELECTED) )
      {
        // select it
        pDoc->m_selSectorSelection.Select( *pbsc);
      }
    ENDFOR}
  }

  pDoc->SetEditingMode( SECTOR_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::CopySectorAmbient( CBrushSector *pbscSector)
{
  theApp.m_colSectorAmbientClipboard = pbscSector->bsc_colAmbient;
}

void CWorldEditorView::PasteSectorAmbient( CBrushSector *pbscSector)
{
	CWorldEditorDoc* pDoc = GetDocument();
  if( (pbscSector == NULL) || (pbscSector->IsSelected( BSCF_SELECTED)) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      itbsc->bsc_colAmbient = theApp.m_colSectorAmbientClipboard;
      itbsc->UncacheLightMaps();
    }
  }
  else
  {
    pbscSector->bsc_colAmbient = theApp.m_colSectorAmbientClipboard;
    pbscSector->UncacheLightMaps();
  }
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::PasteTexture( CBrushPolygon *pbpoPolygon)
{
  if( theApp.m_ptdActiveTexture==NULL) return;
  CWorldEditorDoc* pDoc = GetDocument();
  CTFileName fnTextureName = theApp.m_ptdActiveTexture->GetName();
  try
  {
    if( (pbpoPolygon == NULL) || (pbpoPolygon->IsSelected( BPOF_SELECTED)) )
    {
      FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.SetData_t( fnTextureName);
      }
    }
    else
    {
      pbpoPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.SetData_t( fnTextureName);
    }
    pDoc->m_chSelections.MarkChanged();
    pDoc->SetModifiedFlag( TRUE);
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    return;
  }
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::StorePolygonSelection(CBrushPolygonSelection &selPolygons,
                                             CDynamicContainer<CBrushPolygon> &dcPolygons)
{
  dcPolygons.Clear();
  FOREACHINDYNAMICCONTAINER( selPolygons, CBrushPolygon, itbpo)
  {
    dcPolygons.Add( itbpo);
  }
}

void CWorldEditorView::RestorePolygonSelection(CBrushPolygonSelection &selPolygons,
                                               CDynamicContainer<CBrushPolygon> &dcPolygons)
{
  selPolygons.Clear();
  FOREACHINDYNAMICCONTAINER( dcPolygons, CBrushPolygon, itbpo)
  {
    selPolygons.Select( *itbpo);
  }
}

void CWorldEditorView::DiscardShadows( CEntity *penEntity)
{
	CWorldEditorDoc* pDoc = GetDocument();
  
  CDynamicContainer<CBrushPolygon> dcPolygons;
  StorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);
  pDoc->m_selPolygonSelection.Clear();

  FLOATaabbox3D boxForDiscard;
  if( (penEntity == NULL) || (penEntity->IsSelected( ENF_SELECTED)) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      // if it is brush
      if (iten->en_RenderType == CEntity::RT_BRUSH)
      {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm)
        {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
          {
            // for all polygons in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
            {
              boxForDiscard |= itbpo->bpo_boxBoundingBox;
              itbpo->bpo_smShadowMap.DiscardAllLayers();
              itbpo->InitializeShadowMap();
              pDoc->m_selPolygonSelection.Select( *itbpo);
            }
            itbsc->UncacheLightMaps();
          }
        }
      }
    }
  }
  else
  {
    // if it is brush
    if (penEntity->en_RenderType == CEntity::RT_BRUSH)
    {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, penEntity->en_pbrBrush->br_lhBrushMips, itbm)
      {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc)
        {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
          {
            boxForDiscard |= itbpo->bpo_boxBoundingBox;
            itbpo->bpo_smShadowMap.DiscardAllLayers();
            itbpo->InitializeShadowMap();
            pDoc->m_selPolygonSelection.Select( *itbpo);
          }
          itbsc->UncacheLightMaps();
        }
      }
    }
  }
  pDoc->m_woWorld.FindShadowLayers( boxForDiscard, TRUE);
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
  RestorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);
}

void CWorldEditorView::DiscardShadows( CBrushSector *pbscSector)
{
  CWorldEditorDoc* pDoc = GetDocument();

  CDynamicContainer<CBrushPolygon> dcPolygons;
  StorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);
  pDoc->m_selPolygonSelection.Clear();

  FLOATaabbox3D boxForDiscard;
  if( (pbscSector == NULL) || (pbscSector->IsSelected( BSCF_SELECTED)) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
      {
        boxForDiscard |= itbpo->bpo_boxBoundingBox;
        itbpo->bpo_smShadowMap.DiscardAllLayers();
        itbpo->InitializeShadowMap();
        pDoc->m_selPolygonSelection.Select( *itbpo);
      }
      itbsc->UncacheLightMaps();
    }
  }
  else
  {
    FOREACHINSTATICARRAY(pbscSector->bsc_abpoPolygons, CBrushPolygon, itbpo)
    {
      boxForDiscard |= itbpo->bpo_boxBoundingBox;
      itbpo->bpo_smShadowMap.DiscardAllLayers();
      itbpo->InitializeShadowMap();
      pDoc->m_selPolygonSelection.Select( *itbpo);
    }
    pbscSector->UncacheLightMaps();
  }
  pDoc->m_woWorld.FindShadowLayers( boxForDiscard, TRUE);
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
  RestorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);
}

void CWorldEditorView::DiscardShadows( CBrushPolygon *pbpoPolygon)
{
  CWorldEditorDoc* pDoc = GetDocument();

  CDynamicContainer<CBrushPolygon> dcPolygons;
  StorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);

  FLOATaabbox3D boxForDiscard;
  if( (pbpoPolygon == NULL) || (pbpoPolygon->IsSelected( BPOF_SELECTED)) )
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      // calculate bounding box for all polygons
      boxForDiscard |= itbpo->bpo_boxBoundingBox;
      itbpo->bpo_smShadowMap.DiscardAllLayers();
      itbpo->InitializeShadowMap();
    }
  }
  else
  {
    pbpoPolygon->bpo_smShadowMap.DiscardAllLayers();
    pbpoPolygon->InitializeShadowMap();
    boxForDiscard |= pbpoPolygon->bpo_boxBoundingBox;
    pDoc->m_selPolygonSelection.Clear();
    pDoc->m_selPolygonSelection.Select( *pbpoPolygon);
  }
  pDoc->m_woWorld.FindShadowLayers( boxForDiscard, TRUE);
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
  RestorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);
}

void CWorldEditorView::OnInsertVertex()
{
  CWorldEditorDoc* pDoc = GetDocument();
  MarkClosestVtxAndEdgeOnPrimitiveBase( m_ptMouse);
  if( m_iDragEdge != -1) pDoc->InsertPrimitiveVertex( m_iDragEdge, m_vMouseDownSecondLayer);
}

void CWorldEditorView::OnDeleteVertex()
{
  CWorldEditorDoc* pDoc = GetDocument();
  MarkClosestVtxAndEdgeOnPrimitiveBase( m_ptMouse);
  if( m_iDragVertice != -1) pDoc->DeletePrimitiveVertex( m_iDragVertice);
}

void CWorldEditorView::OnKeyBackslash()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    CPoint ptMouse;
    GetCursorPos( &ptMouse); 
    if( theApp.m_iTerrainEditMode==TEM_LAYER)
    {
      CTerrainLayer *ptlLayer=GetLayer();
      if(ptlLayer==NULL) return;

      if( ptlLayer->tl_ltType==LT_TILE)
      {
        InvokeTerrainTilePalette(ptMouse.x, ptMouse.y);
        return;
      }
    }
    InvokeTerrainBrushPalette( ptMouse.x-BRUSH_PALETTE_WIDTH/2, ptMouse.y+BRUSH_PALETTE_HEIGHT/2);
  }
  else
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    if( pDoc->m_selEntitySelection.Count() == 1)
    {
      pDoc->m_selEntitySelection.Lock();
      CEntity *penOnlySelected = &pDoc->m_selEntitySelection[0];
      pDoc->m_selEntitySelection.Unlock();

      CEntity *penToSelect = NULL;
      CPropertyID *ppidProperty = pMainFrame->m_PropertyComboBar.GetSelectedProperty();
      if( ppidProperty == NULL) return;
      if( ppidProperty->pid_eptType == CEntityProperty::EPT_PARENT)
      {
        penToSelect = penOnlySelected->GetParent();
      }
      else if( ppidProperty->pid_eptType == CEntityProperty::EPT_ENTITYPTR)
      {
        // obtain entity class ptr
        CDLLEntityClass *pdecDLLClass = penOnlySelected->GetClass()->ec_pdecDLLClass;
        // for all classes in hierarchy of this entity
        for(;
            pdecDLLClass!=NULL;
            pdecDLLClass = pdecDLLClass->dec_pdecBase) {
          // for all properties
          for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) {
            CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
            if( (ppidProperty->pid_strName == epProperty.ep_strName) &&
                (ppidProperty->pid_eptType == epProperty.ep_eptType) )
            {
              // get target entity
              penToSelect = ENTITYPROPERTY( &*penOnlySelected, epProperty.ep_slOffset, CEntityPointer);
            }
          }
        }
      }
      if( penToSelect == NULL) return;
      pDoc->m_selEntitySelection.Clear();
      pDoc->m_selEntitySelection.Select( *penToSelect);
      pMainFrame->m_PropertyComboBar.UpdateData( FALSE);
      pDoc->m_chSelections.MarkChanged();
    }
  }
}


void CWorldEditorView::OnSavePicturesForEnvironment()
{
  CWorldEditorDoc* pDoc = GetDocument();
  pDoc->m_selEntitySelection.Lock();
  CEntity *pen = &pDoc->m_selEntitySelection[0];
  pDoc->m_selEntitySelection.Unlock();

  CTFileName fnName = _EngineGUI.FileRequester( "Save pictures as ...",
    FILTER_TGA FILTER_ALL FILTER_END,"Environment pictures directory", "Textures\\");
  if( fnName == "") return;
  CTFileName fnBase = fnName.NoExt();

  CDrawPort *pdp;
  CImageInfo II;
  CTextureData TD;
  CAnimData AD;
  ULONG flags = NONE;

  CChildFrame *pChild = GetChildFrame();
  // create canvas to render picture
  _pGfx->CreateWorkCanvas( 256, 256, &pdp);
  if( pdp != NULL)
  {
    if( pdp->Lock())
    {
      CViewPrefs vpOld = m_vpViewPrefs;
      // set rendering type for world
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetHiddenLinesOn( FALSE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetBackgroundTextureOn( TRUE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetVerticesFillType( CWorldRenderPrefs::FT_NONE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
      m_vpViewPrefs.m_wrpWorldRenderPrefs.SetLensFlaresType( CWorldRenderPrefs::LFT_REFLECTIONS_AND_GLARE);
      // for models
      m_vpViewPrefs.m_mrpModelRenderPrefs.SetRenderType( RT_TEXTURE);
      m_vpViewPrefs.m_mrpModelRenderPrefs.SetShadingType( RT_SHADING_PHONG);
      m_vpViewPrefs.m_mrpModelRenderPrefs.SetShadowQuality( 0);
      m_vpViewPrefs.m_mrpModelRenderPrefs.SetWire( FALSE);
      m_vpViewPrefs.m_mrpModelRenderPrefs.SetHiddenLines( FALSE);
      m_vpViewPrefs.m_mrpModelRenderPrefs.BBoxFrameShow( FALSE);
      m_vpViewPrefs.m_mrpModelRenderPrefs.BBoxAllShow( FALSE);

      // remember old viewer settings
      CPlacement3D plOrgPlacement = pChild->m_mvViewer.mv_plViewer;
      FLOAT fOldTargetDistance = pChild->m_mvViewer.mv_fTargetDistance;
      enum CSlaveViewer::ProjectionType ptOld = m_ptProjectionType;
      m_ptProjectionType = CSlaveViewer::PT_PERSPECTIVE;
      // set new viewer settings
      pChild->m_mvViewer.mv_plViewer = pen->GetPlacement();
      pChild->m_mvViewer.mv_fTargetDistance = 10.0f;

#define GRAB_BCG( ext, angDelta) \
  pChild->m_mvViewer.mv_plViewer = pen->GetPlacement();\
  pChild->m_mvViewer.mv_plViewer.pl_OrientationAngle+=angDelta;\
  RenderView( pdp);\
  pdp->GrabScreen(II);\
  try { II.SaveTGA_t( fnBase+ext); } \
  catch (const char *strError)\
  { AfxMessageBox(CString(strError)); }

      GRAB_BCG( "N.tga", ANGLE3D( 0, 0, 0));
      GRAB_BCG( "W.tga", ANGLE3D( 90.0f, 0, 0));
      GRAB_BCG( "S.tga", ANGLE3D( 180.0f, 0, 0));
      GRAB_BCG( "E.tga", ANGLE3D( -90.0f, 0, 0));
      GRAB_BCG( "C.tga", ANGLE3D( 0, 90.0f, 0));
      GRAB_BCG( "F.tga", ANGLE3D( 0, -90.0f, 0));

      // restore original settings
      m_ptProjectionType = ptOld;
      pChild->m_mvViewer.mv_plViewer = plOrgPlacement;
      pChild->m_mvViewer.mv_fTargetDistance = fOldTargetDistance;
      m_vpViewPrefs = vpOld;
      pdp->Unlock();
    }

    _pGfx->DestroyWorkCanvas( pdp);
    pdp = NULL;
  }
}

void CWorldEditorView::OnUpdateSavePicturesForEnvironment(CCmdUI* pCmdUI)
{
  CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->m_selEntitySelection.Count() == 1);
}

BOOL CWorldEditorView::OnEraseBkgnd(CDC* pDC)
{
  return TRUE;
}

void CWorldEditorView::OnMenuAlignMappingU()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_selPolygonSelection.Count() == 0) return;

  FLOAT fLastU = 0;
  // for each selected polygon
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo) {
    CBrushPolygon &bpo = *itbpo;
    CMappingDefinition &md = bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
    // find min and max u for the polygon
    md.md_fUOffset = 0;
    FLOAT fMinU = UpperLimit(0.0f);
    FLOAT fMaxU = LowerLimit(0.0f);
    for(INDEX ibpe=0; ibpe<bpo.bpo_abpePolygonEdges.Count(); ibpe++) {
      CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[ibpe];
      FLOAT3D v0, v1;
      MEX2D(vTex0);
      bpe.GetVertexCoordinatesRelative(v0, v1);
      md.GetTextureCoordinates(
        bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, v0, vTex0);
      FLOAT fU = vTex0(1)/1024.0f;
      fMinU = Min(fMinU, fU);
      fMaxU = Max(fMaxU, fU);
    }

    // add the offsets to its mapping
    md.md_fUOffset=fMinU-fLastU;
    fLastU += fMaxU-fMinU;
  }
  CPrintF("Total length u: %fm\n", fLastU);

  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnKeyCtrlShiftK() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    ApplyRndNoiseOntoTerrain();
  }
  else
  {
    OnMenuAlignMappingV();
  }
}

void CWorldEditorView::OnMenuAlignMappingV()
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_selPolygonSelection.Count() == 0) return;

  FLOAT fLastV = 0;
  // for each selected polygon
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo) {
    CBrushPolygon &bpo = *itbpo;
    CMappingDefinition &md = bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
    // find min and max v for the polygon
    md.md_fVOffset = 0;
    FLOAT fMinV = UpperLimit(0.0f);
    FLOAT fMaxV = LowerLimit(0.0f);
    for(INDEX ibpe=0; ibpe<bpo.bpo_abpePolygonEdges.Count(); ibpe++) {
      CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[ibpe];
      FLOAT3D v0, v1;
      MEX2D(vTex0);
      bpe.GetVertexCoordinatesRelative(v0, v1);
      md.GetTextureCoordinates(
        bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, v0, vTex0);
      FLOAT fV = vTex0(2)/1024.0f;
      fMinV = Min(fMinV, fV);
      fMaxV = Max(fMaxV, fV);
    }

    // add the offsets to its mapping
    md.md_fVOffset=fMinV-fLastV;
    fLastV += fMaxV-fMinV;
  }
  CPrintF("Total length v %fm\n", fLastV);

  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}



void CWorldEditorView::OnDestroy()
{
	// destroy canvas that is currently used
  _pGfx->DestroyWindowCanvas( m_pvpViewPort);
  m_pvpViewPort = NULL;

  CView::OnDestroy();

}

void CWorldEditorView::OnFallDown()
{
  CWorldEditorDoc *pDoc = GetDocument();
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    iten->FallDownToFloor();
  }
  pDoc->UpdateAllViews( NULL);
  pDoc->SetModifiedFlag( TRUE);
}

void CWorldEditorView::OnUpdatePrevious(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc *pDoc = GetDocument();
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  // for browsing trough properties
  if( bShift && (pDoc->GetEditingMode() == ENTITY_MODE) && (pDoc->m_selEntitySelection.Count() != 0) )
  {
    pCmdUI->Enable( TRUE);
  }
  else if( (pDoc->GetEditingMode() == POLYGON_MODE) && (pDoc->m_selPolygonSelection.Count() != 0) )
  {
    pCmdUI->Enable( TRUE);
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    pCmdUI->Enable( TRUE);
  }
  else if( (pDoc->m_bBrowseEntitiesMode) || (pDoc->m_cenEntitiesSelectedByVolume.Count()>0) )
  {
    pDoc->OnUpdatePreviousSelectedEntity(pCmdUI);
  }
  else
  {
    OnUpdatePreviousMipBrush( pCmdUI);
  }
}

void CWorldEditorView::OnUpdateNext(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc *pDoc = GetDocument();
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  // for browsing trough properties
  if( bShift && (pDoc->GetEditingMode() == ENTITY_MODE) && (pDoc->m_selEntitySelection.Count() != 0) )
  {
    pCmdUI->Enable( TRUE);
  }
  else if( (pDoc->GetEditingMode() == POLYGON_MODE) && (pDoc->m_selPolygonSelection.Count() != 0) )
  {
    pCmdUI->Enable( TRUE);
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    pCmdUI->Enable( TRUE);
  }
  else if( (pDoc->m_bBrowseEntitiesMode) || (pDoc->m_cenEntitiesSelectedByVolume.Count()>0) )
  {
    pDoc->OnUpdateNextSelectedEntity(pCmdUI);
  }
  else
  {
    OnUpdateNextMipBrush( pCmdUI);
  }
}

void CWorldEditorView::OnPrevious() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CPropertyComboBar &dlgProperty = pMainFrame->m_PropertyComboBar;

  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  // in entity mode use shift to select previous property
  if( (pDoc->GetEditingMode() == ENTITY_MODE) && bShift )
  {
    // shift only is used to select previous empty target property
    if( !bCtrl)
    {
      dlgProperty.SelectPreviousEmptyTarget();
    }
    // ctrl+shift is used to select previous property
    else
    {
      dlgProperty.SelectPreviousProperty();
    }
  }
  else if( (pDoc->GetEditingMode() == POLYGON_MODE) && (pDoc->m_selPolygonSelection.Count() != 0) )
  {
    OnPreviousPolygon();
  }
  else if( (pDoc->m_bBrowseEntitiesMode) || (pDoc->m_cenEntitiesSelectedByVolume.Count()>0) )
  {
    pDoc->OnPreviousSelectedEntity();
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    CTerrainLayer *ptlLayer=GetLayer();
    if(theApp.m_iTerrainEditMode==TEM_LAYER && ptlLayer!=NULL && ptlLayer->tl_ltType==LT_TILE)
    {
      CDynamicContainer<CTileInfo> dcTileInfo;
      INDEX ctTilesPerRaw=0;
      ObtainLayerTileInfo( &dcTileInfo, ptlLayer->tl_ptdTexture, ctTilesPerRaw);
      INDEX ctTiles=dcTileInfo.Count();
      ptlLayer->tl_iSelectedTile= Clamp( ptlLayer->tl_iSelectedTile+INDEX(1), (INDEX)0, INDEX(ctTiles-1) );
      // free allocated tile info structures
      for(INDEX i=0; i<dcTileInfo.Count(); i++)
      {
        delete &dcTileInfo[i];
      }
      dcTileInfo.Clear();
    }
    else
    {
      theApp.m_fCurrentTerrainBrush=ClampUp(theApp.m_fCurrentTerrainBrush+1.0f,FLOAT(CT_BRUSHES-1));
      theApp.m_ctTerrainPageCanvas.MarkChanged();
    }
  }
  else
  {
    OnPreviousMipBrush();
  }
}

void CWorldEditorView::OnNext() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CPropertyComboBar &dlgProperty = pMainFrame->m_PropertyComboBar;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  // in entity mode use shift to select next property
  if( (pDoc->GetEditingMode() == ENTITY_MODE) && bShift )
  {
    // shift only is used to select next empty target property
    if( !bCtrl)
    {
      dlgProperty.SelectNextEmptyTarget();
    }
    // ctrl+shift is used to select next property
    else
    {
      dlgProperty.SelectNextProperty();
    }
  }
  else if( (pDoc->GetEditingMode() == POLYGON_MODE) && (pDoc->m_selPolygonSelection.Count() != 0) )
  {
    OnNextPolygon();
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    CTerrainLayer *ptlLayer=GetLayer();
    if(theApp.m_iTerrainEditMode==TEM_LAYER && ptlLayer!=NULL && ptlLayer->tl_ltType==LT_TILE)
    {
      CDynamicContainer<CTileInfo> dcTileInfo;
      INDEX ctTilesPerRaw=0;
      ObtainLayerTileInfo( &dcTileInfo, ptlLayer->tl_ptdTexture, ctTilesPerRaw);
      INDEX ctTiles=dcTileInfo.Count();
      ptlLayer->tl_iSelectedTile= Clamp( ptlLayer->tl_iSelectedTile-INDEX(1), (INDEX)0, INDEX(ctTiles-1) );
      // free allocated tile info structures
      for(INDEX i=0; i<dcTileInfo.Count(); i++)
      {
        delete &dcTileInfo[i];
      }
      dcTileInfo.Clear();
    }
    else
    {
      theApp.m_fCurrentTerrainBrush=ClampDn(theApp.m_fCurrentTerrainBrush-1.0f,0.0f);
      theApp.m_ctTerrainPageCanvas.MarkChanged();
    }
  }
  else if( (pDoc->m_bBrowseEntitiesMode) || (pDoc->m_cenEntitiesSelectedByVolume.Count()>0) )
  {
    pDoc->OnNextSelectedEntity();
  }
  else
  {
    OnNextMipBrush();
  }
}

void CWorldEditorView::OnPreviousPolygon(void)
{
  CWorldEditorDoc *pDoc = GetDocument();
  pDoc->m_selPolygonSelection.Lock();
  ASSERT( pDoc->m_selPolygonSelection.Count() != 0);
  INDEX iSelectedPolygon=0;

  if( !pDoc->m_selPolygonSelection.IsMember( pDoc->m_pbpoLastCentered))
  {
    pDoc->m_pbpoLastCentered = &pDoc->m_selPolygonSelection[0];
    iSelectedPolygon=0;
  }
  else
  {
    INDEX iCurrent = pDoc->m_selPolygonSelection.Index( pDoc->m_pbpoLastCentered);
    INDEX iNext = (iCurrent+1) % pDoc->m_selPolygonSelection.Count();
    pDoc->m_pbpoLastCentered = &pDoc->m_selPolygonSelection[iNext];
    iSelectedPolygon=iNext;
  }
  AllignPolygon( pDoc->m_pbpoLastCentered);

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTString strMessage;
  strMessage.PrintF("Polygon %d/%d", iSelectedPolygon, pDoc->m_selPolygonSelection.Count());
  pMainFrame->SetStatusBarMessage( strMessage, STATUS_LINE_PANE, 2);  

  pDoc->m_selPolygonSelection.Unlock();
}

void CWorldEditorView::OnNextPolygon(void)
{
  CWorldEditorDoc *pDoc = GetDocument();
  pDoc->m_selPolygonSelection.Lock();
  ASSERT( pDoc->m_selPolygonSelection.Count() != 0);
  INDEX iSelectedPolygon=0;

  if( !pDoc->m_selPolygonSelection.IsMember( pDoc->m_pbpoLastCentered))
  {
    pDoc->m_pbpoLastCentered = &pDoc->m_selPolygonSelection[0];
    iSelectedPolygon=0;
  }
  else
  {
    INDEX iCurrent = pDoc->m_selPolygonSelection.Index( pDoc->m_pbpoLastCentered);
    INDEX iPrev = (iCurrent+pDoc->m_selPolygonSelection.Count()-1) % pDoc->m_selPolygonSelection.Count();
    pDoc->m_pbpoLastCentered = &pDoc->m_selPolygonSelection[iPrev];
    iSelectedPolygon=iPrev;
  }
  AllignPolygon( pDoc->m_pbpoLastCentered);

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTString strMessage;
  strMessage.PrintF("Polygon %d/%d", iSelectedPolygon, pDoc->m_selPolygonSelection.Count());
  pMainFrame->SetStatusBarMessage( strMessage, STATUS_LINE_PANE, 2);  

  pDoc->m_selPolygonSelection.Unlock();
}


void CWorldEditorView::OnRemoveUnusedTextures() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  
  try
  {
    // for each entity in the world
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten) {
      // if it is brush entity
      if (iten->en_RenderType == CEntity::RT_BRUSH) {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
            // for all polygons in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
            {
              // if it is non translucent portal same texture
              if ( ((itbpo->bpo_ulFlags&BPOF_PORTAL) && !(itbpo->bpo_ulFlags&BPOF_TRANSLUCENT)) &&
                   (itbpo->bpo_bppProperties.bpp_uwPretenderDistance==0) )
              {
                itbpo->bpo_abptTextures[0].bpt_toTexture.SetData_t( CTString(""));
                itbpo->bpo_abptTextures[1].bpt_toTexture.SetData_t( CTString(""));
                itbpo->bpo_abptTextures[2].bpt_toTexture.SetData_t( CTString(""));
              }
            }
          }
        }
      }
    }
  }
  // if failed
  catch (const char *err_str)
  {
    // report error
    AfxMessageBox( CString(err_str));
    return;
  }
}

void CWorldEditorView::OnRotate() 
{
  Rotate(90.0f, 0.0f);
}

void CWorldEditorView::OnRotateBack() 
{
  Rotate(-90.0f, 0.0f);
}

#define ANGLE_KEY_DELTA 15.0f
void CWorldEditorView::OnRotateLeft() 
{
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  Rotate(ANGLE_KEY_DELTA, 0.0f, bShift);
}

void CWorldEditorView::OnRotateRight() 
{
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  Rotate(-ANGLE_KEY_DELTA, 0.0f, bShift);
}

void CWorldEditorView::OnRotateUp() 
{
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  CWorldEditorDoc *pDoc = GetDocument();
  if( (pDoc->m_pwoSecondLayer != NULL) &&
      (pDoc->m_bPrimitiveMode) &&
      (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) )
  {
    OnMoveUp();
  }
  else
  {
    Rotate(0.0f,-ANGLE_KEY_DELTA, bShift);
  }
}

void CWorldEditorView::OnRotateDown() 
{
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  CWorldEditorDoc *pDoc = GetDocument();
  if( (pDoc->m_pwoSecondLayer != NULL) &&
      (pDoc->m_bPrimitiveMode) &&
      (theApp.m_vfpCurrent.vfp_ttTriangularisationType != TT_NONE) )
  {
    OnMoveDown();
  }
  else
  {
    Rotate(0.0f,ANGLE_KEY_DELTA, bShift);
  }
}

void CWorldEditorView::Rotate( FLOAT fAngleLR, FLOAT fAngleUD, BOOL bSmooth/*=FALSE*/)
{
  if( bSmooth && Abs(fAngleLR)!=90.0f)
  {
    fAngleLR/=10;
    fAngleUD/=10;
  }
  CWorldEditorDoc *pDoc = GetDocument();
  ANGLE3D a3dForRotation = ANGLE3D(0.0f, 0.0f, 0.0f);
  switch( m_ptProjectionType)
  {
  case CSlaveViewer::PT_ISOMETRIC_FRONT:
  case CSlaveViewer::PT_ISOMETRIC_BACK:
    {
      a3dForRotation(3) = fAngleLR;
      a3dForRotation(2) = fAngleUD;
      break;
    }
  case CSlaveViewer::PT_ISOMETRIC_RIGHT:
  case CSlaveViewer::PT_ISOMETRIC_LEFT:
    {
      a3dForRotation(2) = fAngleLR;
      a3dForRotation(3) = -fAngleUD;
      break;
    }
  default:
    {
      a3dForRotation(1) = fAngleLR;
      a3dForRotation(2) = fAngleUD;
    }
  }
  
  if( (pDoc->GetEditingMode() == POLYGON_MODE))
  {
    if( Abs(fAngleLR)!=90.0f)
    {
      fAngleLR/=50;
      fAngleUD/=50;
    }
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
    // if mouse is over polygon
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      CBrushPolygon &bpo = *crRayHit.cr_pbpoBrushPolygon;
      FLOAT3D vCenter=bpo.bpo_boxBoundingBox.Center();
      vCenter=bpo.bpo_pbplPlane->bpl_plAbsolute.ProjectPoint( vCenter);

      CEntity *pen = bpo.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
      CSimpleProjection3D pr;
      pr.ObjectPlacementL() = _plOrigin;
      pr.ViewerPlacementL() = pen->GetPlacement();
      pr.Prepare();
      FLOATplane3D vRelative;
      pr.ProjectCoordinate(vCenter, vRelative);
      
      // rotate it
      ANGLE3D angMappingRotation=ANGLE3D(0,0,0);
      angMappingRotation(1)=fAngleLR;
      bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.Rotate(
        bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative,
        vRelative, angMappingRotation(1));
    }
  }
  else if( (pDoc->GetEditingMode() == CSG_MODE))
  {
    pDoc->m_plSecondLayer.pl_OrientationAngle += a3dForRotation;
  }
  else if( (pDoc->GetEditingMode() == ENTITY_MODE))
  {
    CEntity *penBrush = NULL;
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      if (iten->en_RenderType == CEntity::RT_BRUSH) {
        penBrush = &*iten;
      }
      if( !(iten->GetFlags() & ENF_ANCHORED) ||
           GetChildFrame()->m_bAncoredMovingAllowed)
      {
        CPlacement3D plEntity = iten->GetPlacement();
        plEntity.pl_OrientationAngle += a3dForRotation;
        iten->SetPlacement( plEntity);
      }
    }}

    // check for terrain updating
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      CLightSource *pls = iten->GetLightSource();
      if (pls!=NULL)
      {
        // if light is directional
        if(pls->ls_ulFlags &LSF_DIRECTIONAL)
        {
          CTerrain *ptrTerrain=GetTerrain();
          if(ptrTerrain!=NULL) ptrTerrain->UpdateShadowMap();
        }
      }
    }}

    if( penBrush != NULL) 
    {
      DiscardShadows( penBrush);
    }
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}
    
void CWorldEditorView::MultiplyMappingOnPolygon( FLOAT fFactor)
{
  CWorldEditorDoc *pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if mouse is over polygon
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    CMappingDefinitionUI mdui;
    CBrushPolygon *pbpo = crRayHit.cr_pbpoBrushPolygon;
    pbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.ToUI( mdui);
    mdui.mdui_fUStretch *= fFactor;
    mdui.mdui_fVStretch *= fFactor;
    pbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.FromUI( mdui);
    pDoc->m_chSelections.MarkChanged();
    pDoc->SetModifiedFlag( TRUE);
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnSelectVisibleSectors() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  pDoc->m_selSectorSelection.Clear();
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    // don't select for hidden entities
    if( !(iten->en_ulFlags & ENF_HIDDEN))
    {
      continue;
    }
    CEntity::RenderType rt = iten->GetRenderType();
    if (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
    {
      CBrush3D *pBrush3D = iten->GetBrush();
      if(pBrush3D != NULL)
      {
        FLOAT fCurrentMipFactor = GetCurrentlyActiveMipFactor();
        CBrush3D *pbrBrush = iten->GetBrush();
        if( pbrBrush != NULL)
        {
          CBrushMip *pbmCurrentMip = pbrBrush->GetBrushMipByDistance( fCurrentMipFactor);
          if( pbmCurrentMip != NULL)
          {
            FOREACHINDYNAMICARRAY(pbmCurrentMip->bm_abscSectors, CBrushSector, itbsc)
            {
              if( !(itbsc->bsc_ulFlags & BSCF_HIDDEN))
              {
                // add it to selection
                pDoc->m_selSectorSelection.Select( *itbsc);
              }
            }
          }
        }
      }
    }
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnEditCopyAlternative() 
{
  EditCopy( TRUE);
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

void CWorldEditorView::DrawAxis(const PIX2D &pixC, PIX len, COLOR colAxisColor, COLOR colTextColor,
                                CTString strU, CTString strV)
{
  if( strU=="" || strV=="") return;
  // draw arrow-headed axis
  DrawArrow( *m_pdpDrawPort, pixC(1), pixC(2), pixC(1)+len, pixC(2), colAxisColor, _FULL_);
  DrawArrow( *m_pdpDrawPort, pixC(1), pixC(2), pixC(1), pixC(2)-len, colAxisColor, _FULL_);
  // type axis text line
  m_pdpDrawPort->SetFont( _pfdConsoleFont);
  m_pdpDrawPort->SetTextAspect( 1.0f);
  m_pdpDrawPort->SetTextScaling( 1.0f);  
  m_pdpDrawPort->PutTextCXY( strU, pixC(1)+len+8, pixC(2), colTextColor);
  m_pdpDrawPort->PutTextCXY( strV, pixC(1), pixC(2)-len-8, colTextColor);
}

void CWorldEditorView::DrawArrowAndTypeText( CProjection3D &prProjection,
  const FLOAT3D &v0, const FLOAT3D &v1, COLOR colColor, CTString strText)
{
  // get transformed end vertices
  FLOAT3D tv0, tv1;
  prProjection.PreClip(v0, tv0);
  prProjection.PreClip(v1, tv1);

  // clip the edge line
  FLOAT3D vClipped0 = tv0;
  FLOAT3D vClipped1 = tv1;
  ULONG ulClipFlags = prProjection.ClipLine(vClipped0, vClipped1);
  // if the edge remains after clipping to front plane
  if (ulClipFlags != LCF_EDGEREMOVED) {
    // project the vertices
    FLOAT3D v3d0, v3d1;
    prProjection.PostClip(vClipped0, v3d0);
    prProjection.PostClip(vClipped1, v3d1);
    // make 2d vertices
    FLOAT2D v2d0, v2d1;
    v2d0(1) = v3d0(1); v2d0(2) = v3d0(2);
    v2d1(1) = v3d1(1); v2d1(2) = v3d1(2);

    if( (Abs(v2d1(1)-v2d0(1)) > 8) || (Abs(v2d1(2)-v2d0(2)) > 8) )
    {
      // draw arrow-headed line between vertices
      DrawArrow( *m_pdpDrawPort, (PIX)v2d0(1), (PIX)v2d0(2),
                                 (PIX)v2d1(1), (PIX)v2d1(2), colColor, _FULL_);
      // type text over line
      if( strText != "")
        m_pdpDrawPort->PutTextCXY( strText, (v2d0(1)+v2d1(1))/2, (v2d0(2)+v2d1(2))/2);
    }
  }
}


void CWorldEditorView::SelectWhoTargets( CDynamicContainer<CEntity> &dcTargetedEntities)
{
  CWorldEditorDoc *pDoc = GetDocument();
  pDoc->m_selEntitySelection.Clear();  
  CEntityProperty *pepSelected = NULL;
  // for each that can be targeted
  FOREACHINDYNAMICCONTAINER(dcTargetedEntities, CEntity, itenTargets)
  {
    // for each entity in world
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      // obtain entity class ptr
      CDLLEntityClass *pdecDLLClass = iten->GetClass()->ec_pdecDLLClass;
      // for all classes in hierarchy of this entity
      for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
      {
        // for all properties
        for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
        {
          CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ENTITYPTR)
          {
            // obtain property ptr
            CEntity *penCurrentPtr = ENTITYPROPERTY( &*iten, pepProperty->ep_slOffset, CEntityPointer);
            if( (penCurrentPtr == &*itenTargets) && !iten->IsSelected( ENF_SELECTED) )
            {
              pepSelected = pepProperty;
              pDoc->m_selEntitySelection.Select( *iten);
            }
          }
        }
      }
    }
  }
  pDoc->m_chSelections.MarkChanged();
  
  // if only one entity targets
  if( pDoc->m_selEntitySelection.Count() == 1)
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->m_PropertyComboBar.SelectProperty( pepSelected);
  }

  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectWhoTargetsOnContext() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  // to hold entities for selecting
  CDynamicContainer<CEntity> dcEntities;
  // if right-clicked on void or selected entity
  if( (m_penEntityHitOnContext == NULL) ||
      (m_penEntityHitOnContext->IsSelected( ENF_SELECTED)) )
  {
    // perform on whole selection
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      dcEntities.Add( iten);
    }}
  }
  else
  {
    // perform only on right-clicked entity
    dcEntities.Add( m_penEntityHitOnContext);    
  }
  // select all entities that target ones from dynamic container
  SelectWhoTargets( dcEntities);
}

void CWorldEditorView::OnSelectWhoTargets() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  // to hold entities for selecting
  CDynamicContainer<CEntity> dcEntities;
  // perform on whole selection
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    dcEntities.Add( iten);
  }}
  // select all entities that target ones from dynamic container
  SelectWhoTargets( dcEntities);
}

void CWorldEditorView::OnSelectAllTargetsOnContext() 
{
  CEntity *pen = m_penEntityHitOnContext;
  if( pen==NULL || pen->IsSelected( ENF_SELECTED))
  {
    SelectAllTargetsOfSelectedEntities();
    return;
  }
  else
  {
    SelectAllTargetsOfEntity(pen);
  }
}

void CWorldEditorView::OnSelectAllTargets() 
{
  SelectAllTargetsOfSelectedEntities();
}

void CWorldEditorView::SelectAllTargetsOfEntity(CEntity *pen) 
{
  ASSERT( pen!= NULL);
  CWorldEditorDoc *pDoc = GetDocument();
  // obtain entity class ptr
  CDLLEntityClass *pdecDLLClass = pen->GetClass()->ec_pdecDLLClass;

  // to hold entities for selecting
  CDynamicContainer<CEntity> dcEntities;

  // for all classes in hierarchy of this entity
  for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
  {
    // for all properties
    for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
    {
      CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
      if( epProperty.ep_eptType == CEntityProperty::EPT_ENTITYPTR &&
          epProperty.ep_strName!=NULL &&
          CTString(epProperty.ep_strName)!="")
      {
        // obtain property ptr
        CEntity *penTarget = ENTITYPROPERTY( &*pen, epProperty.ep_slOffset, CEntityPointer);
        if( penTarget != NULL)
        {
          // add it to container
          dcEntities.Add( penTarget);
        }
      }
    }
  }

  // for all entities in dynamic container
  FOREACHINDYNAMICCONTAINER(dcEntities, CEntity, iten)
  {
    // if not yet selected
    if( !iten->IsSelected( ENF_SELECTED))
    {
      // select it
      pDoc->m_selEntitySelection.Select( *iten);
    }
  }
  
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::SelectAllTargetsOfSelectedEntities(void) 
{
  CWorldEditorDoc *pDoc = GetDocument();
  // to hold entities for selecting
  CDynamicContainer<CEntity> dcEntities;
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // obtain entity class ptr
    CDLLEntityClass *pdecDLLClass = iten->GetClass()->ec_pdecDLLClass;
    // for all classes in hierarchy of this entity
    for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
        if(epProperty.ep_eptType==CEntityProperty::EPT_ENTITYPTR &&
          epProperty.ep_strName!=NULL &&
          CTString(epProperty.ep_strName)!="")
        {
          // obtain property ptr
          CEntity *penTarget = ENTITYPROPERTY( &*iten, epProperty.ep_slOffset, CEntityPointer);
          if( penTarget != NULL)
          {
            // add it to container
            dcEntities.Add( penTarget);
          }
        }
      }
    }
  }}

  // for all entities in dynamic container
  FOREACHINDYNAMICCONTAINER( dcEntities, CEntity, iten)
  {
    // if not yet selected
    if( !iten->IsSelected( ENF_SELECTED))
    {
      // select it
      pDoc->m_selEntitySelection.Select( *iten);
    }
  }

  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
  return;
}

void CWorldEditorView::OnSelectInvalidTris() 
{
  CWorldEditorDoc *pDoc = GetDocument();

  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            // if it has invalid triangles
            if ((itbpo->bpo_ulFlags&BPOF_INVALIDTRIANGLES)&&
               !(itbpo->IsSelected(BPOF_SELECTED))) {
              // select it
              pDoc->m_selPolygonSelection.Select( *itbpo);
            }
          }
        }
      }
    }
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

BOOL CWorldEditorView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
  INDEX iCount = zDelta/120;

  BOOL bSpace = (GetKeyState( ' ') & 128) != 0;
  BOOL bCtrl = nFlags & MK_CONTROL;
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;

  CWorldEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

  // ctrl + mouse wheel toggle modes
  /*
  if( bCtrl && !bSpace && !bShift && !bAlt)
  {
    for( INDEX iKnee=0; iKnee<Abs(iCount); iKnee++)
    {
      if(iCount<0)
      {
        switch(pDoc->GetEditingMode())
        {
        case( ENTITY_MODE):
          {
            pDoc->SetEditingMode( SECTOR_MODE);
            break;
          }
        case( SECTOR_MODE):
          {
            pDoc->SetEditingMode( POLYGON_MODE);
            break;
          }
        case( POLYGON_MODE):
          {
            pDoc->SetEditingMode( VERTEX_MODE);
            break;
          }
        case( VERTEX_MODE):
          {
            pDoc->SetEditingMode( TERRAIN_MODE);
            break;
          }
        case( TERRAIN_MODE):
          {
            pDoc->SetEditingMode( ENTITY_MODE);
            break;
          }
        }
      }
      else
      {
        switch(pDoc->GetEditingMode())
        {
        case( ENTITY_MODE):
          {
            pDoc->SetEditingMode( TERRAIN_MODE);
            break;
          }
        case( SECTOR_MODE):
          {
            pDoc->SetEditingMode( ENTITY_MODE);
            break;
          }
        case( POLYGON_MODE):
          {
            pDoc->SetEditingMode( SECTOR_MODE);
            break;
          }
        case( VERTEX_MODE):
          {
            pDoc->SetEditingMode( POLYGON_MODE);
            break;
          }
        case( TERRAIN_MODE):
          {
            pDoc->SetEditingMode( VERTEX_MODE);
            break;
          }
        }
      }
      pDoc->m_chSelections.MarkChanged();
      pDoc->UpdateAllViews( NULL);
    }
  }
  */
  // space+ctrl+lmb zoomes in 2x
  if( bSpace && bCtrl)
  {
    BOOL bHitModels = (pDoc->GetEditingMode() == ENTITY_MODE) || bSpace;
    BOOL bHitFields = (pDoc->GetEditingMode() == ENTITY_MODE) || bSpace;
    // obtain information about where mouse points into the world
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse, FALSE, bHitModels, bHitFields);
    // set new target
    GetChildFrame()->m_mvViewer.SetTargetPlacement( crRayHit.cr_vHit);
    // move mouse to center of screen
    CPoint ptCenter = CPoint( m_pdpDrawPort->GetWidth()/2, m_pdpDrawPort->GetHeight()/2);
    ClientToScreen( &ptCenter);
    SetCursorPos(ptCenter.x, ptCenter.y);
    OnZoomMore();
    pDoc->UpdateAllViews( NULL);
    m_ptMouseDown = ptCenter;

    for( INDEX iKnee=0; iKnee<Abs(iCount); iKnee++)
    {
      if(iCount<0) OnZoomLess();
      else         OnZoomMore();
    }
  }
  else
  {
    for( INDEX iKnee=0; iKnee<Abs(iCount); iKnee++)
    {
      if(iCount<0) PostMessage( WM_COMMAND, ID_PREVIOUS, 0);
      else         PostMessage( WM_COMMAND, ID_NEXT, 0);
    }
  }

  return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CWorldEditorView::OnSelectSectorsOtherSide() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon->bpo_ulFlags & BPOF_PORTAL))
  {
    CBrushPolygon &bpo = *crRayHit.cr_pbpoBrushPolygon;
    // for all sectors behind portal
    {FOREACHDSTOFSRC(bpo.bpo_rsOtherSideSectors, CBrushSector, bsc_rdOtherSidePortals, pbsc)
      if( !pbsc->IsSelected( BSCF_SELECTED))
      {
        pDoc->m_selSectorSelection.Select( *pbsc);
      }
    ENDFOR}
    
    pDoc->SetEditingMode( SECTOR_MODE);
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnSelectLinksToSector() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL))
  {
    CBrushSector &bsc = *crRayHit.cr_pbpoBrushPolygon->bpo_pbscSector;

    // for all sectors behind portal
    {FOREACHSRCOFDST(bsc.bsc_rdOtherSidePortals, CBrushPolygon, bpo_rsOtherSideSectors, pbpo)
      if( !pbpo->IsSelected( BPOF_SELECTED))
      {
        pDoc->m_selPolygonSelection.Select( *pbpo);
      }
    ENDFOR}
    
    pDoc->SetEditingMode( POLYGON_MODE);
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}


void CWorldEditorView::OnRemainSelectedByOrientation() 
{
  OnRemainSelectedByOrientation(TRUE);
}

void CWorldEditorView::OnRemainSelectedbyOrientationSingle() 
{
  OnRemainSelectedByOrientation(FALSE);
}

void CWorldEditorView::OnRemainSelectedByOrientation(BOOL bBothSides) 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  CBrushPolygon *pbpoPolygon = crRayHit.cr_pbpoBrushPolygon;
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (pbpoPolygon != NULL) )
  {
    FLOAT3D vReference = pbpoPolygon->bpo_pbplPlane->bpl_plAbsolute;
    // copy polygon selection to container
    CDynamicContainer<CBrushPolygon> dcTemp;
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      dcTemp.Add( itbpo);
    }}
    pDoc->m_selPolygonSelection.Clear();
    {FOREACHINDYNAMICCONTAINER( dcTemp, CBrushPolygon, itbpo)
    {
      FLOAT fCos;
      if( bBothSides)
      {
        fCos = Abs(vReference % itbpo->bpo_pbplPlane->bpl_plAbsolute);
      }
      else
      {
        fCos = vReference % itbpo->bpo_pbplPlane->bpl_plAbsolute;
      }

      if( fCos > CosFast(360.0f/8.0f))
      {
        pDoc->m_selPolygonSelection.Select( *itbpo);
      }
    }}
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnDeselectByOrientation() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  CBrushPolygon *pbpoPolygon = crRayHit.cr_pbpoBrushPolygon;
  // if we hit brush entity
  if( (crRayHit.cr_penHit != NULL) &&
      (pbpoPolygon != NULL) )
  {
    FLOAT3D vReference = pbpoPolygon->bpo_pbplPlane->bpl_plAbsolute;
    // copy polygon selection to container
    CDynamicContainer<CBrushPolygon> dcTemp;
    {FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      dcTemp.Add( itbpo);
    }}
    pDoc->m_selPolygonSelection.Clear();
    {FOREACHINDYNAMICCONTAINER( dcTemp, CBrushPolygon, itbpo)
    {
      FLOAT fCos = Abs(vReference % itbpo->bpo_pbplPlane->bpl_plAbsolute);
      if( fCos < CosFast(360.0f/8.0f))
      {
        pDoc->m_selPolygonSelection.Select( *itbpo);
      }
    }}
    pDoc->m_chSelections.MarkChanged();
    pDoc->UpdateAllViews( NULL);
  }
}


void CWorldEditorView::OnReoptimizeBrushes() 
{
	CWorldEditorDoc* pDoc = GetDocument();

  CWaitCursor wc;

  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( (crRayHit.cr_penHit != NULL) &&
      (crRayHit.cr_pbpoBrushPolygon != NULL) )
  {
    pDoc->ClearSelections();
    pDoc->m_chSelections.MarkChanged();
    CEntity *penEntityHit = crRayHit.cr_penHit;
    BOOL bSelected = crRayHit.cr_penHit->IsSelected( ENF_SELECTED);

    if( bSelected && (pDoc->GetEditingMode() == ENTITY_MODE))
    {
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        if (iten->en_RenderType == CEntity::RT_BRUSH)
        {
          CBrush3D *pbrBrush = iten->GetBrush();
          // for each mip in the brush
          FOREACHINLIST(CBrushMip, bm_lnInBrush, pbrBrush->br_lhBrushMips, itbm)
          {
            // reoptimize it
            itbm->Reoptimize();
          }
          DiscardShadows( &*iten);
          if( iten->GetFlags() & ENF_ZONING)
          {
            pbrBrush->SwitchToZoning();
          }
          else
          {
            pbrBrush->SwitchToNonZoning();
          }
        }
      }}
    }
    else
    {
      if (penEntityHit->en_RenderType == CEntity::RT_BRUSH)
      {
        CBrush3D *pbrBrush = penEntityHit->GetBrush();
        // for each mip in the brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, pbrBrush->br_lhBrushMips, itbm)
        {
          // reoptimize it
          itbm->Reoptimize();
        }
        DiscardShadows( penEntityHit);
        if( penEntityHit->GetFlags() & ENF_ZONING)
        {
          pbrBrush->SwitchToZoning();
        }
        else
        {
          pbrBrush->SwitchToNonZoning();
        }
      }
    }
    // automaticly update portal links
    pDoc->m_woWorld.RebuildLinks();

    pDoc->UpdateAllViews( NULL);
    pDoc->SetModifiedFlag();
  }
  // ray-hit didn't hit any entity, reoptimize the whole world
  else
  {
    // for each entity in the world
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      // if it is brush entity
      if (iten->en_RenderType == CEntity::RT_BRUSH)
      {
        CBrush3D *pbrBrush = iten->GetBrush();
        // for each mip in the brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, pbrBrush->br_lhBrushMips, itbm)
        {
          // reoptimize it
          itbm->Reoptimize();
        }
        DiscardShadows( iten);
        if( iten->GetFlags() & ENF_ZONING)
        {
          pbrBrush->SwitchToZoning();
        }
        else
        {
          pbrBrush->SwitchToNonZoning();
        }
      }
    }
  }
}

void CWorldEditorView::OnMergeVertices() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  pDoc->RememberUndo();
  pDoc->ClearSelections( ST_VERTEX);

  CDynamicContainer<CBrushVertex> cbvxAllreadyDone;

  DOUBLE3D vMergeToVertex;
  {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
  {
    vMergeToVertex = FLOATtoDOUBLE(itbvx->bvx_vAbsolute);
  }}

  // deselect vertices that are on right spot, to avoid unnecessary triangularisation
  {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
  {
    if( FLOATtoDOUBLE(itbvx->bvx_vAbsolute) == vMergeToVertex)
    {
      cbvxAllreadyDone.Add(itbvx);
    }
  }}
  {FOREACHINDYNAMICCONTAINER( cbvxAllreadyDone, CBrushVertex, itbvx)
  {
    pDoc->m_selVertexSelection.Deselect( *itbvx);
  }}

  // triangularize all influenced polygons
  pDoc->m_woWorld.TriangularizeForVertices( pDoc->m_selVertexSelection);

  // for each vertex
  {FOREACHINDYNAMICCONTAINER( pDoc->m_selVertexSelection, CBrushVertex, itbvx)
  {
    // set its new position
    itbvx->SetAbsolutePosition( vMergeToVertex);
  }}

  // reselect vertices
  {FOREACHINDYNAMICCONTAINER( cbvxAllreadyDone, CBrushVertex, itbvx)
  {
    pDoc->m_selVertexSelection.Select( *itbvx);
  }}

  // update sectors
  pDoc->m_woWorld.UpdateSectorsDuringVertexChange( pDoc->m_selVertexSelection);
  pDoc->m_woWorld.UpdateSectorsAfterVertexChange( pDoc->m_selVertexSelection);
  pDoc->UpdateAllViews( NULL);
  pDoc->SetModifiedFlag();
}

void CWorldEditorView::OnExportDisplaceMap() 
{
  CImageInfo ii;
  
  CDlgDisplaceMapSize dlgDisplaceMapSize;
  if( dlgDisplaceMapSize.DoModal() != IDOK)
  {
    return;
  }  

  CWorldEditorDoc* pDoc = GetDocument();
  // find bounding box of all selected polygons
  FLOATaabbox3D boxSelection;
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    // calculate bounding box for all polygons
    boxSelection |= itbpo->bpo_boxBoundingBox;
  }

  PIX pixWidth = dlgDisplaceMapSize.m_pixWidth;
  PIX pixHeight = dlgDisplaceMapSize.m_pixHeight;

  FLOAT fMinX = boxSelection.Min()(1);
  FLOAT fMaxX = boxSelection.Max()(1);
  FLOAT fMinY = boxSelection.Min()(2);
  FLOAT fMaxY = boxSelection.Max()(2);
  FLOAT fSizeY = boxSelection.Size()(2);
  FLOAT fMinZ = boxSelection.Min()(3);
  FLOAT fMaxZ = boxSelection.Max()(3);
  FLOAT fDeltaX;
  FLOAT fDeltaZ;
  if (dlgDisplaceMapSize.m_bMidPixSample)
  {
    fDeltaX = (fMaxX-fMinX)/pixWidth;
    fDeltaZ = (fMaxZ-fMinZ)/pixHeight;
  }
  else
  {
    fDeltaX = (fMaxX-fMinX-0.5f)/pixWidth;
    fDeltaZ = (fMaxZ-fMinZ-0.5f)/pixHeight;
  }

  CPrintF("Export displace map...\n");
  CPrintF("Min: (%gf, %gf, %gf), Max: (%gf, %gf, %gf), Delta X=%g, Delta Z=%g\n",
    fMinX, fMinY, fMinZ, 
    fMaxX, fMaxY, fMaxZ, 
    fDeltaX, fDeltaZ);

  CTFileName fnDocName = CTString(CStringA(pDoc->GetPathName()));
  CTFileName fnDirectory = fnDocName.FileDir();
  CTFileName fnDefaultSelected = fnDocName.FileName()+CTString("DisplaceMap.tga");

  try
  {
    fnDirectory.RemoveApplicationPath_t();
  }
  catch (const char *str_err)
  {
    AfxMessageBox( CString(str_err));
    return;
  }

  // note: NULL given instead of profile string name, to avoid using last directory
  CTFileName fnDisplaceMap = 
    _EngineGUI.FileRequester( "Select name to export displace map",
    "Pictures (*.tga)\0*.tga\0" FILTER_END,
    NULL, fnDirectory, fnDefaultSelected, NULL, FALSE);

  if( fnDisplaceMap == "") return;

  ULONG ulSize = pixWidth*pixHeight*3;
  UBYTE *pubPicture = (UBYTE *) AllocMemory( ulSize);
  
  // initializes structure members and attaches pointer to image
  ii.Attach( pubPicture, pixWidth, pixHeight, 24);

  // for each line of pixels in height map
  for( INDEX iY=0; iY<pixHeight; iY++)
  {
    // for each horizontal pixel in height map
    for( INDEX iX=0; iX<pixWidth; iX++)
    {
      
      FLOAT fX;
      FLOAT fZ;
      
      // calculate coordinate in world
      if (dlgDisplaceMapSize.m_bMidPixSample)
      {
        fX = fMinX+fDeltaX*iX+fDeltaX/2;
        fZ = fMinZ+fDeltaZ*iY+fDeltaZ/2;
      } 
      else
      {
        fX = fMinX+fDeltaX*iX+0.1;
        fZ = fMinZ+fDeltaZ*iY+0.1;
      }

      // cast ray to recive height
      CCastRay crRayHit = CCastRay( NULL, CPlacement3D(FLOAT3D(fX, fMaxY+1.0f, fZ), ANGLE3D(0,-90,0)) );
      crRayHit.cr_bAllowOverHit = TRUE;
      crRayHit.cr_bHitPortals = FALSE;
      crRayHit.cr_ttHitModels = CCastRay::TT_NONE;
      // cast ray, go for hit data
      pDoc->m_woWorld.CastRay( crRayHit);

      if (!dlgDisplaceMapSize.m_bHighResolution){
        // set black pixel if nothing hit
        UBYTE ubPixelColor = 0;
        if( crRayHit.cr_penHit != NULL)
        {
          FLOAT fYHit = crRayHit.cr_vHit(2);
          ubPixelColor = (UBYTE) ((fYHit-fMinY)/fSizeY*255.0f);
        }
        
        UBYTE *pPixel=pubPicture+iY*pixWidth*3+iX*3;
        *(pPixel+0) = ubPixelColor;
        *(pPixel+1) = ubPixelColor;
        *(pPixel+2) = ubPixelColor;
      }
      else
      {
        // set black pixel if nothing hit
        unsigned short usPixelColor = 0;
        if( crRayHit.cr_penHit != NULL)
        {
          FLOAT fYHit = crRayHit.cr_vHit(2);
          usPixelColor = (unsigned short) ((fYHit-fMinY)/fSizeY*65535);
        }
        
        UBYTE *pPixel=pubPicture+iY*pixWidth*3+iX*3;
        *(pPixel+0) = 0;
        *(pPixel+1) = usPixelColor>>8;
        *(pPixel+2) = usPixelColor&0xFF;
      }
    }
  }

  // try to
  try {
    // save displace map as TGA
    ii.SaveTGA_t( fnDisplaceMap);
  } // if failed
  catch (const char *strError) {
    // report error
    AfxMessageBox(CString(strError));
  }
}

void CWorldEditorView::OnCutMode() 
{
  // if we should enter cut mode
  if( !theApp.m_bCutModeOn)
  {
    CTString strError;
    BOOL bCutEnabled = IsCutEnabled( strError);
    if( !bCutEnabled)
    {
      // warn user
      WarningMessage( "%s", strError);
      return;
    }
  }
	theApp.m_bCutModeOn = !theApp.m_bCutModeOn;
	theApp.m_bMeasureModeOn = FALSE;
}

void CWorldEditorView::OnUpdateCutMode(CCmdUI* pCmdUI) 
{
	CWorldEditorDoc* pDoc = GetDocument();

  // cut is enabled only in sector and polygon mode
  if( (pDoc->GetEditingMode() == POLYGON_MODE) ||
      (pDoc->GetEditingMode() == ENTITY_MODE) ||
      (pDoc->GetEditingMode() == SECTOR_MODE) )
  {
    pCmdUI->Enable( TRUE);
  }
  else
  {
    pCmdUI->Enable( FALSE);
  }
  pCmdUI->SetCheck( theApp.m_bCutModeOn);
  //pDoc->UpdateAllViews( NULL); !!!!!!!!!!!!!!!!!!!!! unbelievable !!!!!!!!!!!!!!!!!
}

BOOL CWorldEditorView::IsCutEnabled(CTString &strError)
{
	CWorldEditorDoc* pDoc = GetDocument();
  
  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    // there must be at least one entity in selection
    if( pDoc->m_selEntitySelection.Count() == 0)
    {
      strError = "You must select entity(s) that you want to mirror!";
      return FALSE;
    }
  }
  else if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // there must be at least one polygon in selection
    if( pDoc->m_selPolygonSelection.Count() == 0)
    {
      strError = "You must select polygon(s) that you want to cut!";
      return FALSE;
    }
    // all polygons must be of the same brush
    CBrush3D *pbrBrush = NULL;
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      // remember first polygon's brush
      if( pbrBrush == NULL)
      {
        pbrBrush = itbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush;
      }
      else
      {
        // if any of the other polygons are not from same brush
        if( pbrBrush != itbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush)
        {
          // don't allow cut
          strError = "All polygons must be from the same brush to be able to cut them!";
          return FALSE;
        }
      }
    }
  }
  else if(pDoc->GetEditingMode() == SECTOR_MODE)
  {
    // there must be at least one sector in selection
    if( pDoc->m_selSectorSelection.Count() == 0)
    {
      strError = "You must select sector(s) that you want to cut!";
      return FALSE;
    }
    // all sectors must be of the same brush
    CBrush3D *pbrBrush = NULL;
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      // remember first sector's brush
      if( pbrBrush == NULL)
      {
        pbrBrush = itbsc->bsc_pbmBrushMip->bm_pbrBrush;
      }
      else
      {
        // if any of the other sectors are not from same brush
        if( pbrBrush != itbsc->bsc_pbmBrushMip->bm_pbrBrush)
        {
          // don't allow cut
          strError = "All sector must be from the same brush to be able to cut them!";
          return FALSE;
        }
      }
    }
  }
  return TRUE;
}

void CWorldEditorView::OnUpdateSelectClones(CCmdUI* pCmdUI) 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // allow clone selecting only if we have only one entity selected
  if( pDoc->m_selEntitySelection.Count() == 1)
  {
    pCmdUI->Enable( TRUE);
    return;
  }
  pCmdUI->Enable( FALSE);
}

void CWorldEditorView::OnSelectClones() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CEntity *pen = pDoc->m_selEntitySelection.GetFirstInSelection();

  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    // if has same name
    if( pen->GetName() == iten->GetName() )
    {
      // if is not yet selected
      if( !iten->IsSelected( ENF_SELECTED))
      {
        // select it
        pDoc->m_selEntitySelection.Select( *iten);
      }
    }
  }
  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

BOOL CWorldEditorView::IsSelectClonesOnContextEnabled( void) 
{
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( crRayHit.cr_penHit != NULL)
  {
    return TRUE;
  }
  return FALSE;
}

void CWorldEditorView::OnSelectClonesOnContext() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( crRayHit.cr_penHit != NULL)
  {
    // get hitted entity
    CEntity *pen = crRayHit.cr_penHit;

    // for each entity in the world
    FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      // if has same name
      if( pen->GetName() == iten->GetName() )
      {
        // if is not yet selected
        if( !iten->IsSelected( ENF_SELECTED))
        {
          // select it
          pDoc->m_selEntitySelection.Select( *iten);
        }
      }
    }
  }
  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnKeyCtrlShiftE() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if(pDoc->GetEditingMode()==TERRAIN_MODE)
  {
    ApplyEqualizeOntoTerrain();
  }
  else
  {
    OnSelectOfSameClass();
  }
}

void CWorldEditorView::OnSelectOfSameClass() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CEntity *pen = pDoc->m_selEntitySelection.GetFirstInSelection();

  // to hold entities for selecting
  CDynamicContainer<CEntity> dcEntities;

  // for all selected entities
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    dcEntities.Add( &*iten);
  }}

  // for each non-hidden entity in the world
  {FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    CBrushSector *pbscSector = iten->GetFirstSector();
    BOOL bSectorVisible = (pbscSector == NULL) ||
                         !(pbscSector->bsc_ulFlags & BSCF_HIDDEN);
    // if it isn't classified in hidden sector and is not hidden itself
    if( !bSectorVisible || (iten->en_ulFlags&ENF_HIDDEN) )
    {
      continue;
    }

    // obtain this entity's class ptr
    CEntityClass *pdecClass = iten->GetClass();

    // test all classes
    {FOREACHINDYNAMICCONTAINER(dcEntities, CEntity, itenClass)
    {
      CEntityClass *pdecClassTest = itenClass->GetClass();
      // if of same class
      if( pdecClassTest == pdecClass)
      {
        // if not yet selected
        if( !iten->IsSelected( ENF_SELECTED))
        {
          // select it
          pDoc->m_selEntitySelection.Select( *iten);
        }
      }
    }}
  }}

  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectOfSameClassOnContext() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( crRayHit.cr_penHit != NULL)
  {
    // get hitted entity
    CEntity *pen = crRayHit.cr_penHit;
    // obtain this entity's class ptr
    CEntityClass *pdecClassTest = pen->GetClass();

    // for each non-hidden entity in the world
    {FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
    {
      CBrushSector *pbscSector = iten->GetFirstSector();
      BOOL bSectorVisible = (pbscSector == NULL) ||
                           !(pbscSector->bsc_ulFlags & BSCF_HIDDEN);
      // if it isn't classified in hidden sector and is not hidden itself
      if( !bSectorVisible || (iten->en_ulFlags&ENF_HIDDEN) )
      {
        continue;
      }
      
      CEntityClass *pdecClass = iten->GetClass();
      // if of same class
      if( pdecClassTest == pdecClass)
      {
        // if not yet selected
        if( !iten->IsSelected( ENF_SELECTED))
        {
          // select it
          pDoc->m_selEntitySelection.Select( *iten);
        }
      }
    }}
  }

  // mark that selections have been changed
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

BOOL CWorldEditorView::IsSelectOfSameClassOnContextEnabled( void) 
{
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( crRayHit.cr_penHit != NULL)
  {
    return TRUE;
  }
  return FALSE;
}

// limit current frame rate if neeeded
void LimitFrameRate(void)
{
  // measure passed time for each loop
  /*
  static CTimerValue tvLast(-1.0f);
  CTimerValue tvNow   = _pTimer->GetHighPrecisionTimer();
  TIME tmCurrentDelta = (tvNow-tvLast).GetSeconds();

  // limit maximum frame rate
  wed_iMaxFPSActive   = ClampDn( (INDEX)wed_iMaxFPSActive,   1L);
  TIME tmWantedDelta  = 1.0f / wed_iMaxFPSActive;
  if( tmCurrentDelta<tmWantedDelta) Sleep( (tmWantedDelta-tmCurrentDelta)*1000.0f);
  
  // remember new time
  tvLast = _pTimer->GetHighPrecisionTimer();*/
}

void CWorldEditorView::ApplyFreeModeControls( CPlacement3D &pl, ANGLE3D &aAbs, FLOAT &fSpeedMultiplier, BOOL bPrescan)
{
  CChildFrame *pcf = GetChildFrame();
	CWorldEditorDoc* pDoc = GetDocument();
#define FB_SPEED 1.0f
#define LR_SPEED 1.0f
#define UD_SPEED 1.0f
#define ROT_SPEED 0.75f

  FLOAT fFB = 0.0f; // forward/backward movement
  FLOAT fLR = 0.0f; // left/right movement
  FLOAT fUD = 0.0f; // up/down movement
  FLOAT fRLR = 0.0f; // rotate left/right
  FLOAT fRUD = 0.0f; // rotate up/down

  _pInput->GetInput(bPrescan);

  if (!bPrescan) {
    // additional key indentifiers are not on
    BOOL bShift = _pInput->GetButtonState( KID_LSHIFT) || _pInput->GetButtonState( KID_RSHIFT);
    BOOL bAlt = _pInput->GetButtonState( KID_LALT) || _pInput->GetButtonState( KID_RALT);
    BOOL bCtrl =  _pInput->GetButtonState( KID_LCONTROL) || _pInput->GetButtonState( KID_RCONTROL);

    // apply view configuration from first perspective view of child configuration
    if(_pInput->GetButtonState( KID_NUM0)) pcf->ApplySettingsFromPerspectiveView( this, 0);
    if(_pInput->GetButtonState( KID_NUM1)) pcf->ApplySettingsFromPerspectiveView( this, 1);
    if(_pInput->GetButtonState( KID_NUM2)) pcf->ApplySettingsFromPerspectiveView( this, 2);
    if(_pInput->GetButtonState( KID_NUM3)) pcf->ApplySettingsFromPerspectiveView( this, 3);
    if(_pInput->GetButtonState( KID_NUM4)) pcf->ApplySettingsFromPerspectiveView( this, 4);
    if(_pInput->GetButtonState( KID_NUM5)) pcf->ApplySettingsFromPerspectiveView( this, 5);
    if(_pInput->GetButtonState( KID_NUM6)) pcf->ApplySettingsFromPerspectiveView( this, 6);
    if(_pInput->GetButtonState( KID_NUM7)) pcf->ApplySettingsFromPerspectiveView( this, 7);
    if(_pInput->GetButtonState( KID_NUM8)) pcf->ApplySettingsFromPerspectiveView( this, 8);
    if(_pInput->GetButtonState( KID_NUM9)) pcf->ApplySettingsFromPerspectiveView( this, 9);

    // apply view configuration from available buffers
    if(_pInput->GetButtonState( KID_0)) OnKeyBuffer( ID_BUFFER10);
    if(_pInput->GetButtonState( KID_1)) OnKeyBuffer( ID_BUFFER01);
    if(_pInput->GetButtonState( KID_2)) OnKeyBuffer( ID_BUFFER02);
    if(_pInput->GetButtonState( KID_3)) OnKeyBuffer( ID_BUFFER03);
    if(_pInput->GetButtonState( KID_4)) OnKeyBuffer( ID_BUFFER04);
    if(_pInput->GetButtonState( KID_5)) OnKeyBuffer( ID_BUFFER05);
    if(_pInput->GetButtonState( KID_6)) OnKeyBuffer( ID_BUFFER06);
    if(_pInput->GetButtonState( KID_7)) OnKeyBuffer( ID_BUFFER07);
    if(_pInput->GetButtonState( KID_8)) OnKeyBuffer( ID_BUFFER08);
    if(_pInput->GetButtonState( KID_9)) OnKeyBuffer( ID_BUFFER09);

    // ---------- Simulate moving as in fly mode
    // forward
    if(
      _pInput->GetButtonState( KID_W) ||
      _pInput->GetButtonState( KID_MOUSE2) ||
      _pInput->GetButtonState( KID_ARROWUP))
    {
      fFB = -FB_SPEED*fSpeedMultiplier;
    }
    // backward
    if(
      _pInput->GetButtonState( KID_S) ||
      _pInput->GetButtonState( KID_MOUSE1) ||
      _pInput->GetButtonState( KID_ARROWDOWN) )
    {
      fFB = +FB_SPEED*fSpeedMultiplier;
    }
    /*
    if( _pInput->GetButtonState( KID_MOUSE1))
    {
      fLR = -_pInput->GetAxisRelative(1)/15.0f;
    }*/
    // strife left
    if( 
      _pInput->GetButtonState( KID_Q) ||
      _pInput->GetButtonState( KID_A) ||
      _pInput->GetButtonState( KID_ARROWLEFT) )
    {
      fLR = -LR_SPEED*fSpeedMultiplier;
    }
    // strife right
    if(
      _pInput->GetButtonState( KID_E) ||
      _pInput->GetButtonState( KID_D) ||
      _pInput->GetButtonState( KID_ARROWRIGHT) )
    {
      fLR = +LR_SPEED*fSpeedMultiplier;
    }
    // up
    if( (_pInput->GetButtonState( KID_R) && !bAlt) ||
        _pInput->GetButtonState( KID_SPACE) )
    {
      fUD = +UD_SPEED*fSpeedMultiplier;
    }
    // down
    if( _pInput->GetButtonState( KID_F))
    {
      fUD = -UD_SPEED*fSpeedMultiplier;
    }
    // drop marker here
    if( _pInput->GetButtonState( KID_C))
    {
      CTFileName fnDropClass;
      CTString strTargetProperty;
      if(pDoc->m_selEntitySelection.Count() == 1)
      {
        CEntity &enOnly = pDoc->m_selEntitySelection.GetFirst();
        if(enOnly.DropsMarker( fnDropClass, strTargetProperty))
        {
          OnDropMarker(pl);
        }
      }
    }

    static BOOL bLastTurboDown = FALSE;
    static FLOAT fSpeedMultiplierBeforeTurbo = 1.0f;
    if( _pInput->GetButtonState( KID_TAB))
    {
      if( !bLastTurboDown)
      {
        fSpeedMultiplierBeforeTurbo = fSpeedMultiplier;
      }
      bLastTurboDown = TRUE;
      fSpeedMultiplier = Clamp( fSpeedMultiplier+0.5f, 1.0f, 20.0f);
    }
    else if( bLastTurboDown)
    {
      bLastTurboDown = FALSE;
      fSpeedMultiplier = fSpeedMultiplierBeforeTurbo;
    }
  }
  

  // get current rotation
  fRLR = _pInput->GetAxisValue(1)*ROT_SPEED;
  fRUD = -_pInput->GetAxisValue(2)*ROT_SPEED;

  // apply translation
  if (!bPrescan) {
    pl.Translate_OwnSystem( FLOAT3D(fLR, fUD, fFB));
    // apply rotation
    pl.pl_OrientationAngle = aAbs;//.Rotate_HPB( ANGLE3D(-fRLR, -fRUD, 0));
  }
  aAbs+=ANGLE3D(-fRLR, -fRUD, 0);
}

// placements and orientation used in free fly mode
static CPlacement3D _plNew;
static CPlacement3D _plOld;
static ANGLE3D _aAbs;

void CWorldEditorView::PumpWindowsMessagesInFreeMode(BOOL &bRunning, FLOAT &fSpeedMultiplier)
{
  CChildFrame *pcf = GetChildFrame();
	CWorldEditorDoc* pDoc = GetDocument();

  // additional key indentifiers are not on
  BOOL bShift = _pInput->GetButtonState( KID_LSHIFT) || _pInput->GetButtonState( KID_RSHIFT);
  BOOL bAlt = _pInput->GetButtonState( KID_LALT) || _pInput->GetButtonState( KID_RALT);
  BOOL bCtrl =  _pInput->GetButtonState( KID_LCONTROL) || _pInput->GetButtonState( KID_RCONTROL);
  // while there are any messages in the message queue
  MSG msg;
  while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    // process scripts that are invoked on shortcuts from within WED
    ProcesWEDConsoleShortcuts( &msg);
    
    if(msg.message==_uiMessengerMsg)
    {
      // if one application allready started
      HWND hwndMessenger = ::FindWindow(NULL, L"Croteam Messenger");
      if(hwndMessenger != NULL)
      {
        // force messenger to popup
        ::PostMessage( hwndMessenger, _uiMessengerForcePopup, 0, 0);
      }
    }    
    
    // ---------- Simulate some important commands of wed
    if( msg.message==WM_KEYDOWN || msg.message==WM_SYSKEYDOWN)
    {
      // 'G' links on/off
      if( msg.wParam=='G' && !bCtrl) pcf->OnRenderTargets();
      // 'G' links on/off
      if( msg.wParam=='G' && bCtrl) pcf->OnToggleEntityNames();
      // 'H' shadow on off
      if( msg.wParam=='H') pcf->OnViewShadowsOnoff();
      // 'Num -' toggle frame rate
      if( msg.wParam == VK_SUBTRACT) pcf->OnSceneRenderingTime();
      // 'B' toggle auto mip leveling
      if( msg.wParam=='B') pcf->OnAutoMipLeveling();
      // 'Alt+R' toggle frame rate
      if( msg.wParam=='R' && bAlt) pcf->OnSceneRenderingTime();
      // 'Shift+Backspace' center background viewer
      if( (msg.wParam == VK_BACK) && bShift)
      {
        OnCenterBcgViewer();
        _plNew = pcf->m_mvViewer.mv_plViewer;
        _plOld = _plNew;
        _aAbs = _plNew.pl_OrientationAngle;
      }
      // 'Backspace' center to 0
      else if( msg.wParam == VK_BACK)
      {
        OnResetViewer();
        _plNew = pcf->m_mvViewer.mv_plViewer;
        _plOld = _plNew;
        _aAbs = _plNew.pl_OrientationAngle;
      }
      // 'F5, F6, F7, F8' type of projections
      if( (msg.wParam == VK_F5) && bShift) OnIsometricBottom();
      else if( msg.wParam == VK_F5) OnIsometricTop();
      if( (msg.wParam == VK_F6) && bShift) OnIsometricLeft();
      else if( msg.wParam == VK_F6) OnIsometricRight();
      if( (msg.wParam == VK_F7) && bShift) OnIsometricBack();
      else if( msg.wParam == VK_F7) OnIsometricFront();
      if( (msg.wParam == VK_F8)) OnPerspective();
      // 'Home', 'End', 'Page Up', 'Page Dn' 'Ctrl+' combinations for storing and restoring positions
      if( (msg.wParam == VK_HOME) && bCtrl) pcf->OnStorePosition01();
      else if( msg.wParam == VK_HOME)
      {
        pcf->OnRestorePosition01();
        _plNew = pcf->m_mvViewer.mv_plViewer;
        _plOld = _plNew;
        _aAbs = _plNew.pl_OrientationAngle;
      }
      if( (msg.wParam == VK_END) && bCtrl) pcf->OnStorePosition02();
      else if( msg.wParam == VK_END)
      {
        pcf->OnRestorePosition02();
        _plNew = pcf->m_mvViewer.mv_plViewer;
        _plOld = _plNew;
        _aAbs = _plNew.pl_OrientationAngle;
      }
      if( (msg.wParam == VK_PRIOR) && bCtrl) pcf->OnStorePosition03();
      else if( msg.wParam == VK_PRIOR)
      {
        pcf->OnRestorePosition03();
        _plNew = pcf->m_mvViewer.mv_plViewer;
        _plOld = _plNew;
        _aAbs = _plNew.pl_OrientationAngle;
      }
      if( (msg.wParam == VK_NEXT) && bCtrl) pcf->OnStorePosition04();
      else if( msg.wParam == VK_NEXT)
      {
        pcf->OnRestorePosition04();
        _plNew = pcf->m_mvViewer.mv_plViewer;
        _plOld = _plNew;
        _aAbs = _plNew.pl_OrientationAngle;
      }
      // 'X', 'Alt+X', 'Alt+A' hide selected, unselected, show all
      if( (msg.wParam == 'X') && bAlt) pDoc->OnHideUnselected();
      else if( msg.wParam == 'X') pDoc->OnHideSelected();
      if( (msg.wParam == 'A') && bAlt) pDoc->OnShowAll();
      // 'Z' for toggle view selection
      if( msg.wParam == 'Z')  pcf->OnViewSelection();
    }

    // use mouse wheel to increase/decrease moving speed
    if( msg.message==WM_MOUSEWHEEL)
    {
      short zDelta = (short) HIWORD(msg.wParam);
      INDEX iCount = zDelta/120;
      for( INDEX iKnee=0; iKnee<Abs(iCount); iKnee++)
      {
        if(iCount<0) fSpeedMultiplier/=2;
        else         fSpeedMultiplier*=2;
      }
    }

    // if it is not a mouse message
    if(!(msg.message>=WM_MOUSEFIRST&&msg.message<=WM_MOUSELAST))
    {
      // if not system key messages
      if (!(msg.message==WM_KEYDOWN && msg.wParam==VK_F10
          ||msg.message==WM_SYSKEYDOWN))
      {
        // translate it
        TranslateMessage(&msg);
      }
      // if paint message
      if (msg.message==WM_PAINT)
      {
        // dispatch it
        DispatchMessage(&msg);
      }
    }
    // if should stop
    if( (msg.message==WM_KEYDOWN && msg.wParam==VK_ESCAPE)
      ||(msg.message==WM_ACTIVATE)
      ||(msg.message==WM_CANCELMODE)
      ||(msg.message==WM_KILLFOCUS)
      ||(msg.message==WM_ACTIVATEAPP))
    {
      // stop running
      bRunning = FALSE;
      break;
    }
  }
}

void CWorldEditorView::OnAlternativeMovingMode() 
{
  INDEX iAllowMouseAcceleration = _pShell->GetINDEX("inp_bAllowMouseAcceleration");
  INDEX iFilterMouse = _pShell->GetINDEX("inp_bFilterMouse");

  _pShell->SetINDEX("inp_bAllowMouseAcceleration", 1);
  _pShell->SetINDEX("inp_bFilterMouse", 1);

	CWorldEditorDoc* pDoc = GetDocument();
  CViewPrefs vpOrg = m_vpViewPrefs;
  // obtain child frame ptr
  CChildFrame *pcf = GetChildFrame();
  _plNew = pcf->m_mvViewer.mv_plViewer;
  _plOld = _plNew;
  _aAbs = _plNew.pl_OrientationAngle;

  BOOL bAutoMipBrushingBefore = pcf->m_bAutoMipBrushingOn;
  BOOL bDisableVisibilityTweaksBefore = pcf->m_bDisableVisibilityTweaks;

  // enable input
  _pInput->EnableInput(m_pvpViewPort);

  // initialy, game is running
  BOOL bRunning = TRUE;
  INDEX iLastTick = 0;
  CTimerValue tvStart = _pTimer->GetHighPrecisionTimer();

  // while it is still running
  while( bRunning)
  {
    // process all windows messages
    PumpWindowsMessagesInFreeMode( bRunning, _fFlyModeSpeedMultiplier);

    CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
    FLOAT fPassed = (tvNow-tvStart).GetSeconds();
    INDEX iNowTick = INDEX(fPassed/_pTimer->TickQuantum);
    // apply controls for frame rates below tick quantum
    while(iLastTick<iNowTick-1)
    {
      _plOld = _plNew;
      ApplyFreeModeControls( _plNew, _aAbs, _fFlyModeSpeedMultiplier, FALSE);
      iLastTick++;
    }
    ApplyFreeModeControls( _plNew, _aAbs, _fFlyModeSpeedMultiplier, TRUE);

    // set new viewer position
    FLOAT fLerpFactor = (fPassed-iNowTick*_pTimer->TickQuantum)/_pTimer->TickQuantum;
    ASSERT(fLerpFactor>0 && fLerpFactor<1.0f);
    pcf->m_mvViewer.mv_plViewer.Lerp( _plOld, _plNew, Clamp(fLerpFactor, 0.0f, 1.0f));
    pcf->m_mvViewer.mv_plViewer.pl_OrientationAngle = _aAbs;

    // disable auto rendering range
    m_vpViewPrefs.m_bAutoRenderingRange = FALSE;
    m_vpViewPrefs.m_fRenderingRange = 2.0f;
    pcf->m_bAutoMipBrushingOn = TRUE;
    pcf->m_bDisableVisibilityTweaks=FALSE;    

    // render view again
    CDC *pDC = GetDC();
    OnDraw( pDC);
    ReleaseDC( pDC);
    // smooth frame rate
    LimitFrameRate();
  }
  _pInput->DisableInput();
  // restore rendering range settings from remembered stat view prefs
  m_vpViewPrefs.m_bAutoRenderingRange = vpOrg.m_bAutoRenderingRange;
  m_vpViewPrefs.m_wrpWorldRenderPrefs.SetMinimumRenderRange(vpOrg.m_wrpWorldRenderPrefs.GetMinimumRenderRange());
  pcf->m_bAutoMipBrushingOn = bAutoMipBrushingBefore;
  pcf->m_bDisableVisibilityTweaks=bDisableVisibilityTweaksBefore;

  _pShell->SetINDEX("inp_bAllowMouseAcceleration", iAllowMouseAcceleration);
  _pShell->SetINDEX("inp_bFilterMouse", iFilterMouse);
}

void CWorldEditorView::OnReTriple() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CBrushPolygon *pbpo = pDoc->m_selPolygonSelection.GetFirstInSelection();
  pbpo->bpo_pbscSector->ReTriple( pDoc->m_selPolygonSelection);
}

void CWorldEditorView::OnClearAllTargets(void) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	CWorldEditorDoc* pDoc = GetDocument();

  if( m_bEntityHitedOnContext && m_penEntityHitOnContext != NULL)
  {
    if( ::MessageBoxA( pMainFrame->m_hWnd, "Are you sure that you want to clear all targets?",
      "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2|
      MB_SYSTEMMODAL | MB_TOPMOST) != IDYES)
    {
      return;
    }

    // clear all targets of right-clicked entity
    pMainFrame->m_PropertyComboBar.ClearAllTargets( m_penEntityHitOnContext);
    pDoc->UpdateAllViews( NULL);
    pDoc->SetModifiedFlag();
  }
}

void CWorldEditorView::OnSelectCsgTarget() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  if (pbmCurrentMip!=NULL)
  {
    // obtain entity
    CEntity *pen = pbmCurrentMip->bm_pbrBrush->br_penEntity;
    if( !pen->IsSelected( ENF_SELECTED))
    {
      pDoc->m_selEntitySelection.Select( *pen);
      pDoc->m_chSelections.MarkChanged();
      pDoc->UpdateAllViews( NULL);
    }
  }
}

void CWorldEditorView::OnUpdateSelectCsgTarget(CCmdUI* pCmdUI) 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CBrushMip *pbmCurrentMip = GetCurrentBrushMip();
  pCmdUI->Enable(pbmCurrentMip!=NULL);
}


void CWorldEditorView::OnUpdateReTriple(CCmdUI* pCmdUI) 
{
  BOOL bEnableRetripling = FALSE;
	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_selPolygonSelection.Count() != 0)
  {
    CBrushPolygon *pbpo = pDoc->m_selPolygonSelection.GetFirstInSelection();
    bEnableRetripling = pbpo->bpo_pbscSector->IsReTripleAvailable( pDoc->m_selPolygonSelection);
  }
  pCmdUI->Enable(bEnableRetripling);
}


void CWorldEditorView::OnTriangularizePolygon() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  CBrushPolygon *pbpoHitted = NULL;
  if( (m_penEntityHitOnContext != NULL) &&
      (m_penEntityHitOnContext->GetRenderType() == CEntity::RT_BRUSH) &&
      (m_pbpoRightClickedPolygon != NULL) )
  {
    pbpoHitted = m_pbpoRightClickedPolygon;
  }

  if( pbpoHitted != NULL)
  {
    pDoc->m_selVertexSelection.Clear();
    if( pbpoHitted->IsSelected(BPOF_SELECTED))
    {
      OnTriangularizeSelection();
      return;
    }
    else
    {
      pDoc->m_selVertexSelection.Clear();
      pDoc->m_selPolygonSelection.Clear();
      pbpoHitted->bpo_pbscSector->TriangularizePolygon( pbpoHitted);
    }
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}


void CWorldEditorView::OnEntityContextHelp() 
{
  if( m_penEntityHitOnContext != NULL)
  {
    theApp.DisplayHelp(m_penEntityHitOnContext->GetClass()->GetName(), HH_DISPLAY_TOPIC, NULL);
  }
}

void CWorldEditorView::AutoFitMapping(CBrushPolygon *pbpo, BOOL bInvert/*=FALSE*/, BOOL bFitBoth/*=FALSE*/)
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pbpo==NULL) return;

  if( pbpo->IsSelected(BPOF_SELECTED))
  {
    // for each selected polygon
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      AutoFitMappingOnPolygon(*itbpo, bInvert, bFitBoth);
    }
  }
  else
  {
    AutoFitMappingOnPolygon(*pbpo, bInvert, bFitBoth);
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::AutoFitMappingOnPolygon(CBrushPolygon &bpo, BOOL bInvert/*=FALSE*/, BOOL bFitBoth/*=FALSE*/)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CMappingDefinition &md = bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
  
  // reset u,v stretches
  CMappingDefinitionUI mdui;
  md.ToUI(mdui);
  mdui.mdui_fUStretch = 1.0f;
  mdui.mdui_fVStretch = 1.0f;
  mdui.mdui_fUOffset = 0.0f;
  mdui.mdui_fVOffset = 0.0f;
  md.FromUI(mdui);

  // find min and max u and v for the polygon
  FLOAT fMinU = UpperLimit(0.0f);
  FLOAT fMaxU = LowerLimit(0.0f);
  FLOAT fMinV = UpperLimit(0.0f);
  FLOAT fMaxV = LowerLimit(0.0f);
  INDEX ibpe=0;
  for(; ibpe<bpo.bpo_abpePolygonEdges.Count(); ibpe++)
  {
    CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[ibpe];
    FLOAT3D v0, v1;
    MEX2D(vTex0);
    bpe.GetVertexCoordinatesRelative(v0, v1);
    md.GetTextureCoordinates( bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative, v0, vTex0);
    FLOAT fU = vTex0(1)/1024.0f;
    FLOAT fV = vTex0(2)/1024.0f;
    fMinU = Min(fMinU, fU);
    fMaxU = Max(fMaxU, fU);
    fMinV = Min(fMinV, fV);
    fMaxV = Max(fMaxV, fV);
  }
  // calculate u,v fit stretches
  FLOAT fFitStretchU = fMaxU-fMinU;
  FLOAT fFitStretchV = fMaxV-fMinV;
  // get fit stretch
  FLOAT fFitStretch;
  if( !bInvert)
  {
    fFitStretch = Min(fFitStretchV,fFitStretchU);
  }
  else
  {
    fFitStretch = Max(fFitStretchV,fFitStretchU);
  }
  // adjust stretch for texture size
  CTextureData *ptd = (CTextureData *)bpo.bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData();
  if( ptd!=NULL)
  {
    MEX mexW=ptd->GetWidth();
    MEX mexH=ptd->GetHeight();
    // if fitting U
    if( fFitStretch==fFitStretchU)
    {
      fFitStretch/=mexW/1024.0f;
    }
    // if fitting V
    else
    {
      fFitStretch/=mexH/1024.0f;
    }
    fFitStretchU/=mexW/1024.0f;
    fFitStretchV/=mexH/1024.0f;
  }
  if( bFitBoth)
  {
    mdui.mdui_fUStretch=fFitStretchU;
    mdui.mdui_fVStretch=fFitStretchV;
  }
  else
  {
    mdui.mdui_fUStretch=fFitStretch;
    mdui.mdui_fVStretch=fFitStretch;
  }
  md.FromUI(mdui);
  CMappingVectors &mv = bpo.bpo_pbplPlane->bpl_pwplWorking->wpl_mvRelative;

  // find vtx that has min u and v
  fMinU = UpperLimit(0.0f);
  fMaxV = LowerLimit(0.0f);
  FLOAT3D vMinU, vMaxV;
  for(ibpe=0; ibpe<bpo.bpo_abpePolygonEdges.Count(); ibpe++)
  {
    CBrushPolygonEdge &bpe = bpo.bpo_abpePolygonEdges[ibpe];
    FLOAT3D v0, v1, va0, va1;
    MEX2D(vTex0);
    bpe.GetVertexCoordinatesRelative(v0, v1);
    bpe.GetVertexCoordinatesAbsolute(va0, va1);
    md.GetTextureCoordinates( mv, v0, vTex0);
    FLOAT fU = vTex0(1)/1024.0f;
    FLOAT fV = vTex0(2)/1024.0f;
    if( fU<fMinU)
    {
      vMinU=v0;
      fMinU=fU;
    }
    if( fV>fMaxV)
    {
      vMaxV=v0;
      fMaxV=fV;
    }
  }

  // remember u offset
  md.Center(mv, vMinU);
  md.ToUI(mdui);
  fMinU = mdui.mdui_fUOffset;

  // remember v offset
  md.Center(mv, vMaxV);
  md.ToUI(mdui);
  fMaxV = mdui.mdui_fVOffset;

  // set offsets
  mdui.mdui_fUOffset=fMinU;
  mdui.mdui_fVOffset=fMaxV;
  md.FromUI(mdui);
}

void CWorldEditorView::OnTriangularizeSelection() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  CDynamicContainer<CBrushPolygon> dcPolygons;
  // copy polygon selection
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    dcPolygons.Add(&*itbpo);
  }  
  // clear selections
  pDoc->m_selPolygonSelection.Clear();
  pDoc->m_selVertexSelection.Clear();
  // apply triangularisation
  pDoc->m_woWorld.TriangularizePolygons( dcPolygons);
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnUpdateTriangularizeSelection(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable(pDoc->m_selPolygonSelection.Count()!=0);
}


void CWorldEditorView::OnPopupAutoFitMapping() 
{
  if(m_pbpoRightClickedPolygon != NULL)
  {
    AutoFitMapping( m_pbpoRightClickedPolygon);
  }
}

void CWorldEditorView::OnPopupAutoFitMappingSmall() 
{
  if(m_pbpoRightClickedPolygon != NULL)
  {
    AutoFitMapping( m_pbpoRightClickedPolygon, TRUE);
  }
}

void CWorldEditorView::OnPopupAutoFitMappingBoth() 
{
  if(m_pbpoRightClickedPolygon != NULL)
  {
    AutoFitMapping( m_pbpoRightClickedPolygon, TRUE, TRUE);
  }
}

void CWorldEditorView::ApplyDefaultMapping(CBrushPolygon *pbpo, BOOL bRotation, BOOL bOffset, BOOL bStretch)
{
  if(m_pbpoRightClickedPolygon == NULL) return;

  CWorldEditorDoc* pDoc = GetDocument();
  // if is selected
  if( m_pbpoRightClickedPolygon->IsSelected(BPOF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      CBrushPolygon &po=*itbpo;
      CMappingDefinition &md=po.bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
      CMappingDefinitionUI mdui;
      md.ToUI(mdui);
      if( bRotation)
      {
        mdui.mdui_aURotation=0.0f;
        mdui.mdui_aVRotation=0.0f;
      }
      if( bStretch)
      {
        mdui.mdui_fUStretch = 1.0f;
        mdui.mdui_fVStretch = 1.0f;
      }
      if( bOffset)
      {
        mdui.mdui_fUOffset = 0.0f;
        mdui.mdui_fVOffset = 0.0f;
      }
      md.FromUI(mdui);
    }
  }    
  else
  {
    CMappingDefinition &md=m_pbpoRightClickedPolygon->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping;
    CMappingDefinitionUI mdui;
    md.ToUI(mdui);
    if( bRotation)
    {
      mdui.mdui_aURotation=0.0f;
      mdui.mdui_aVRotation=0.0f;
    }
    if( bStretch)
    {
      mdui.mdui_fUStretch = 1.0f;
      mdui.mdui_fVStretch = 1.0f;
    }
    if( bOffset)
    {
      mdui.mdui_fUOffset = 0.0f;
      mdui.mdui_fVOffset = 0.0f;
    }
    md.FromUI(mdui);
  }
}

void CWorldEditorView::OnResetMappingOffset() 
{
  ApplyDefaultMapping( m_pbpoRightClickedPolygon, FALSE, TRUE, FALSE);
}

void CWorldEditorView::OnResetMappingRotation() 
{
  ApplyDefaultMapping( m_pbpoRightClickedPolygon, TRUE, FALSE, FALSE);
}

void CWorldEditorView::OnResetMappingStretch() 
{
  ApplyDefaultMapping( m_pbpoRightClickedPolygon, FALSE, FALSE, TRUE);
}

void CWorldEditorView::ShowLinkTree(CEntity *pen, BOOL bWhoTargets/*=FALSE*/, BOOL bPropertyNames/*=FALSE*/)
{
	CWorldEditorDoc* pDoc = GetDocument();
  CPoint ptScr=m_ptMouse;
  ClientToScreen( &ptScr);
  if( pen!=NULL || (pDoc->m_selEntitySelection.Count() != 0))
  {
    CDlgLinkTree dlg( pen, ptScr, bWhoTargets, bPropertyNames);
    dlg.DoModal();
  }
}

void CWorldEditorView::OnCrossroadForL() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    ShowLinkTree(NULL);
  }
  else if(pDoc->m_pwoSecondLayer != NULL)
  {
    pDoc->OnJoinLayers();
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainEditMode=TEM_LAYER;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    GetDocument()->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnSelectUsingTargetTree() 
{
  ShowLinkTree(m_penEntityHitOnContext);
}

void CWorldEditorView::OnTargetTree() 
{
  ShowLinkTree(NULL);
}

void CWorldEditorView::OnUpdateTargetTree(CCmdUI* pCmdUI) 
{
	CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable(pDoc->m_selEntitySelection.Count() != 0);
}


void SwapLayers( CBrushPolygon &bpo, INDEX il1, INDEX il2)
{
  CBrushPolygonTexture bptTemp;
  bptTemp.CopyTextureProperties(bpo.bpo_abptTextures[il1], TRUE);
  bpo.bpo_abptTextures[il1].CopyTextureProperties(bpo.bpo_abptTextures[il2], TRUE);
  bpo.bpo_abptTextures[il2].CopyTextureProperties(bptTemp, TRUE);
}

void CWorldEditorView::OnSwapLayers12() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    SwapLayers( *itbpo, 0, 1);
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSwapLayers23() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    SwapLayers( *itbpo, 1, 2);
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectDescendants() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    SelectDescendents( pDoc->m_selEntitySelection, *iten);
  }
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}


void CWorldEditorView::OnRotateToTargetOrigin() 
{
  if(m_penEntityHitOnContext != NULL)
  {
   	CWorldEditorDoc* pDoc = GetDocument();
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      const CPlacement3D &plDst=iten->GetPlacement();
      const CPlacement3D &plSrc=m_penEntityHitOnContext->GetPlacement();
      FLOAT3D vDir=plSrc.pl_PositionVector-plDst.pl_PositionVector;
      vDir.Normalize();
      ANGLE3D ang;
      DirectionVectorToAngles(vDir, ang);
      CPlacement3D plNew=plDst;
      plNew.pl_OrientationAngle=ang;
      iten->SetPlacement(plNew);
    }
    pDoc->SetModifiedFlag( TRUE);
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnRotateToTargetCenter() 
{
  if(m_penEntityHitOnContext != NULL)
  {
   	CWorldEditorDoc* pDoc = GetDocument();
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      const CPlacement3D &plDst=iten->GetPlacement();
      FLOATaabbox3D box;
      m_penEntityHitOnContext->GetSize(box);
      FLOAT3D vTarget=box.Center();
      FLOAT3D vDir=vTarget-plDst.pl_PositionVector;
      vDir.Normalize();
      ANGLE3D ang;
      DirectionVectorToAngles(vDir, ang);
      CPlacement3D plNew=plDst;
      plNew.pl_OrientationAngle=ang;
      iten->SetPlacement(plNew);
    }
    pDoc->SetModifiedFlag( TRUE);
    pDoc->UpdateAllViews( NULL);
  }
}

void CWorldEditorView::OnCopyPlacement() 
{
  if(m_penEntityHitOnContext==NULL) return;
  theApp.m_plClipboard2=m_penEntityHitOnContext->GetPlacement();
}

void CWorldEditorView::OnCopyOrientation() 
{
  if(m_penEntityHitOnContext==NULL) return;
  theApp.m_plClipboard1.pl_OrientationAngle=m_penEntityHitOnContext->GetPlacement().pl_OrientationAngle;
}

void CWorldEditorView::OnCopyPosition() 
{
  if(m_penEntityHitOnContext==NULL) return;
  theApp.m_plClipboard1.pl_PositionVector=m_penEntityHitOnContext->GetPlacement().pl_PositionVector;
}

void CWorldEditorView::OnPastePlacement() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if(m_penEntityHitOnContext==NULL) return;
  if( m_penEntityHitOnContext->IsSelected(ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      iten->SetPlacement(theApp.m_plClipboard2);
    }
  }
  else
  {
    m_penEntityHitOnContext->SetPlacement(theApp.m_plClipboard2);
  }
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnPasteOrientation() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if(m_penEntityHitOnContext==NULL) return;
  if( m_penEntityHitOnContext->IsSelected(ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      CPlacement3D pl=iten->GetPlacement();
      pl.pl_OrientationAngle=theApp.m_plClipboard1.pl_OrientationAngle;
      iten->SetPlacement(pl);
    }
  }
  else
  {
    CPlacement3D pl=m_penEntityHitOnContext->GetPlacement();
    pl.pl_OrientationAngle=theApp.m_plClipboard1.pl_OrientationAngle;
    m_penEntityHitOnContext->SetPlacement(pl);
  }
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnPastePosition() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if(m_penEntityHitOnContext==NULL) return;
  if( m_penEntityHitOnContext->IsSelected(ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      CPlacement3D pl=iten->GetPlacement();
      pl.pl_PositionVector=theApp.m_plClipboard1.pl_PositionVector;
      iten->SetPlacement(pl);
    }
  }
  else
  {
    CPlacement3D pl=m_penEntityHitOnContext->GetPlacement();
    pl.pl_PositionVector=theApp.m_plClipboard1.pl_PositionVector;
    m_penEntityHitOnContext->SetPlacement(pl);
  }
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::Align(BOOL bX,BOOL bY,BOOL bZ,BOOL bH,BOOL bP,BOOL bB)
{
 	CWorldEditorDoc* pDoc = GetDocument();
  INDEX ctSelected=pDoc->m_woWorld.wo_cenEntities.Count();
  CEntity *penLast=NULL;
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    penLast=&*iten;
  }}
  CPlacement3D penSrc=penLast->GetPlacement();
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    CPlacement3D pl=iten->GetPlacement();
    if( bX) {pl.pl_PositionVector(1)=penSrc.pl_PositionVector(1);};
    if( bY) {pl.pl_PositionVector(2)=penSrc.pl_PositionVector(2);};
    if( bZ) {pl.pl_PositionVector(3)=penSrc.pl_PositionVector(3);};
    if( bH) {pl.pl_OrientationAngle(1)=penSrc.pl_OrientationAngle(1);};
    if( bP) {pl.pl_OrientationAngle(2)=penSrc.pl_OrientationAngle(2);};
    if( bB) {pl.pl_OrientationAngle(3)=penSrc.pl_OrientationAngle(3);};
    iten->SetPlacement(pl);
  }}
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnAlignB() 
{
  Align(FALSE,FALSE,FALSE,FALSE,FALSE,TRUE);
}

void CWorldEditorView::OnAlignH() 
{
  Align(FALSE,FALSE,FALSE,TRUE,FALSE,FALSE);
}

void CWorldEditorView::OnAlignP() 
{
  Align(FALSE,FALSE,FALSE,FALSE,TRUE,FALSE);
}

void CWorldEditorView::OnAlignX() 
{
  Align(TRUE,FALSE,FALSE,FALSE,FALSE,FALSE);
}

void CWorldEditorView::OnAlignY() 
{
  Align(FALSE,TRUE,FALSE,FALSE,FALSE,FALSE);
}

void CWorldEditorView::OnAlignZ() 
{
  Align(FALSE,FALSE,TRUE,FALSE,FALSE,FALSE);
}

struct CameraProjectionInfo {
  FLOAT cpi_fOffsetRatioX;
  FLOAT cpi_fOffsetRatioY;
  FLOAT cpi_fOffsetRatioZ;
  ANGLE cpi_angH;
  ANGLE cpi_angP;
  ANGLE cpi_angB;
  double cpi_rMinI;
  double cpi_rMinJ;
  double cpi_rSizeI;
  double cpi_rSizeJ;
};

CameraProjectionInfo *_pacpi=NULL;
INDEX _ctProjections=0;
CameraProjectionInfo acpiF[] = 
{
  { 0, 0,  1,   0,  0, 0,   0,   0, 1, 1}
};

CameraProjectionInfo acpi2x2[] = 
{
  { 0, 0,  1,   0,  0, 0,   0,   0, 0.5, 0.5},
  { 1, 0,  0,  90,  0, 0, 0.5,   0, 0.5, 0.5},
  { 0, 0, -1, 180,  0, 0,   0, 0.5, 0.5, 0.5},
  {-1, 0,  0, 270,  0, 0, 0.5, 0.5, 0.5, 0.5}
};

CameraProjectionInfo acpi3x2[] = 
{
  { 0, 0,  1,   0,  0, 0,   0,   0, 0.3, 0.3},
  { 1, 0,  0,  90,  0, 0, 0.3,   0, 0.3, 0.3},
  { 0, 0, -1, 180,  0, 0, 0.6,   0, 0.3, 0.3},
  {-1, 0,  0, 270,  0, 0,   0, 0.3, 0.3, 0.3},
  { 0, 1,  0,   0,-90, 0, 0.3, 0.3, 0.3, 0.3},
  { 0,-1,  0,   0, 90, 0, 0.6, 0.3, 0.3, 0.3},
};

// setup viewing parameters for saving pretender textures
void SetupView(INDEX iFrame, CDrawPort *pdp, CAnyProjection3D &apr, FLOATaabbox3D box)
{
  // init projection parameters
  CIsometricProjection3D prIsometricProjection;
  prIsometricProjection.ScreenBBoxL() = FLOATaabbox2D(
    FLOAT2D(0.0f, 0.0f),
    FLOAT2D((FLOAT)pdp->GetWidth(), (FLOAT)pdp->GetHeight())
  );
  prIsometricProjection.NearClipDistanceL() = 0.0f;
  // determine far clip plane
  FLOAT3D vSize=box.Size();
  FLOAT3D vHSize=vSize/2.0f;
  FLOAT3D vRatio=FLOAT3D(
    _pacpi[iFrame].cpi_fOffsetRatioX,
    _pacpi[iFrame].cpi_fOffsetRatioY,
    _pacpi[iFrame].cpi_fOffsetRatioZ);
  ANGLE3D ang=ANGLE3D(
    _pacpi[iFrame].cpi_angH,
    _pacpi[iFrame].cpi_angP,
    _pacpi[iFrame].cpi_angB);
  FLOAT fFCD=Max(
    Abs(vRatio(1))*vSize(1), Max(
    Abs(vRatio(2))*vSize(2),
    Abs(vRatio(3))*vSize(3)));
  prIsometricProjection.FarClipDistanceL() = fFCD;
  prIsometricProjection.AspectRatioL() = 1.0f;
  // set up viewer position
  apr = prIsometricProjection;
  FLOAT3D vPos=FLOAT3D(
    vRatio(1)*vHSize(1),
    vRatio(2)*vHSize(2),
    vRatio(3)*vHSize(3))+box.Center();


  FLOAT fMaxSize;
  if( vRatio(1)!=0)
  {
    fMaxSize=Max(vSize(2),vSize(3));
  }
  else if( vRatio(2)!=0)
  {
    fMaxSize=Max(vSize(1),vSize(3));
  }
  else
  {
    fMaxSize=Max(vSize(1),vSize(2));
  }
  
  prIsometricProjection.ZoomFactorL()=pdp->GetWidth()/fMaxSize;

  CPlacement3D plViewer=CPlacement3D(vPos, ang);
  //plViewer.RelativeToAbsolute(plEntity);
  apr=prIsometricProjection;
  apr->ViewerPlacementL() = plViewer;
  apr->ObjectPlacementL() = CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  apr->Prepare();
}

BOOL CWorldEditorView::SaveAutoTexture(FLOATaabbox3D boxBrush, CTFileName &fnTex)
{
  CWorldEditorDoc* pDoc = GetDocument();
  CDlgAutTexturize dlg;
  if( dlg.DoModal()!=IDOK) return FALSE;

  // call file requester for pretender texture name
  CTFileName fnPretenderTexture = _EngineGUI.FileRequester( "Pretender texture name",
    FILTER_TGA FILTER_END, "Auto save texture", "", "", NULL, TRUE);
  if( fnPretenderTexture == "") return FALSE;

  switch( dlg.m_iPretenderStyle)
  {
  case 0: _pacpi=acpiF; _ctProjections=1; break;
  case 1: _pacpi=acpi2x2; _ctProjections=4; break;
  case 2: _pacpi=acpi3x2; _ctProjections=6; break;
  default: _pacpi=acpi3x2; _ctProjections=6; break;
  }

  CDrawPort *pdp;
  CImageInfo II;
  CTextureData TD;
  CAnimData AD;
  ULONG flags = NONE;

  CChildFrame *pChild = GetChildFrame();
  // create canvas to render picture
  _pGfx->CreateWorkCanvas( dlg.m_pixWidth, dlg.m_pixHeight, &pdp);
  if( pdp != NULL)
  {
    FLOAT3D vSize=boxBrush.Size();
    for(INDEX iFrame=0; iFrame<_ctProjections; iFrame++)
    {
      CameraProjectionInfo &cpi=_pacpi[iFrame];
      CDrawPort dpClone( pdp, cpi.cpi_rMinI, cpi.cpi_rMinJ, cpi.cpi_rSizeI, cpi.cpi_rSizeJ);
      // clone draw port
      if( dpClone.Lock())
      {
        dpClone.FillZBuffer(ZBUF_BACK);
        CAnyProjection3D apr;
        SetupView(iFrame, &dpClone, apr, boxBrush);

        // store rendering preferences
        BOOL bWasBcgOn = _wrpWorldRenderPrefs.IsBackgroundTextureOn();
        CWorldRenderPrefs::SelectionType stBefore=_wrpWorldRenderPrefs.GetSelectionType();
        _wrpWorldRenderPrefs.SetBackgroundTextureOn( FALSE);
        _wrpWorldRenderPrefs.SetSelectionType( CWorldRenderPrefs::ST_NONE);
        COLOR colOld=pDoc->m_woWorld.GetBackgroundColor();
        pDoc->m_woWorld.SetBackgroundColor(dlg.m_colBcg.GetColor());
        // render view
        ::RenderView(pDoc->m_woWorld, *(CEntity*)NULL, apr, dpClone);
        // restore rendering preferences
        pDoc->m_woWorld.SetBackgroundColor(colOld);
        _wrpWorldRenderPrefs.SetSelectionType(stBefore);
        _wrpWorldRenderPrefs.SetBackgroundTextureOn( bWasBcgOn);
      }
      dpClone.Unlock();
    }
    pdp->GrabScreen( II, 2);
    if( dlg.m_bExpandEdges)
    {
      II.ExpandEdges();
    }
    // try to
    try {
      // save screen shot as TGA
      II.SaveTGA_t( fnPretenderTexture);
      fnTex=fnPretenderTexture.NoExt()+".tex";
      // create temporary texture
      CreateTexture_t( fnPretenderTexture, fnTex,
                       dlg.m_pixWidth, MAX_MEX_LOG2+1, FALSE);
      _EngineGUI.CreateTexture( fnTex);
    } // if failed
    catch (const char *strError) {
      // report error
      AfxMessageBox(CString(strError));
    }
  }
  return TRUE;
}

INDEX _aiMinMax[] =
{
  0,0,1,
  1,0,1,
  1,0,0,
  0,0,0,
  0,1,1,
  1,1,1,
  1,1,0,
  0,1,0
};

INDEX _aiPlaneVtx[] =
{
  4,0,1,5,
  5,1,2,6,
  6,2,3,7,
  7,3,0,4,
  7,4,5,6,
  0,3,2,1,
};

FLOAT3D _avBoxVtx[8];

struct BoxPlaneParams {
  FLOATplane3D bpp_plPlane;
  CMappingVectors bpp_mvDefault;
  CMappingVectors bpp_mvVectors;
  CMappingDefinition bpp_mdMappingDefinition;
};

BoxPlaneParams _abpp[6];

BOOL CWorldEditorView::SetAutoTextureBoxParams(FLOATaabbox3D boxBrush, CTFileName &fnTex)
{
  CTextureData *pTD=NULL;
  try
  {
    pTD=_pTextureStock->Obtain_t( fnTex);
  }
  catch (const char *strError)
  {
    WarningMessage(strError);
    return FALSE;
  }
  
  // create box vertices
  FLOAT3D vMin=boxBrush.Min();
  FLOAT3D vMax=boxBrush.Max();
  FLOAT3D vSize=boxBrush.Size();
  for( INDEX iVtx=0; iVtx<8; iVtx++)
  {
    FLOAT fX=_aiMinMax[iVtx*3+0]*vSize(1)+vMin(1);
    FLOAT fY=_aiMinMax[iVtx*3+1]*vSize(2)+vMin(2);
    FLOAT fZ=_aiMinMax[iVtx*3+2]*vSize(3)+vMin(3);
    _avBoxVtx[iVtx]=FLOAT3D(fX,fY,fZ);
  }
  
  // create box planes
  for( INDEX iPlane=0; iPlane<_ctProjections; iPlane++)
  {
    CameraProjectionInfo &cpi=_pacpi[iPlane];
    // create plane
    BoxPlaneParams &bpp=_abpp[iPlane];
    FLOAT3D &vVtx0=_avBoxVtx[_aiPlaneVtx[iPlane*4+0]];
    FLOAT3D &vVtx1=_avBoxVtx[_aiPlaneVtx[iPlane*4+1]];
    FLOAT3D &vVtx2=_avBoxVtx[_aiPlaneVtx[iPlane*4+2]];
    FLOAT3D &vVtx3=_avBoxVtx[_aiPlaneVtx[iPlane*4+3]];
    bpp.bpp_plPlane=FLOATplane3D(vVtx0, vVtx1, vVtx2);
    // create mapping vectors
    bpp.bpp_mvDefault.FromPlane(bpp.bpp_plPlane);
    bpp.bpp_mvVectors.mv_vO=(vVtx0+vVtx2)/2.0f;
    FLOAT3D vDeltaU=vVtx3-vVtx0;
    FLOAT3D vDeltaV=vVtx1-vVtx0;
    FLOAT fStretch = Max(vDeltaU.Length(), vDeltaV.Length())/(pTD->GetWidth()/1024.0f*cpi.cpi_rSizeI);
    bpp.bpp_mvVectors.mv_vU=vDeltaU.Normalize()*fStretch;
    bpp.bpp_mvVectors.mv_vV=vDeltaV.Normalize()*fStretch;
    bpp.bpp_mdMappingDefinition.FromMappingVectors(bpp.bpp_mvDefault, bpp.bpp_mvVectors);
    MEX mexOffsetU = pTD->GetWidth() *(cpi.cpi_rMinI+cpi.cpi_rSizeI/2);
    MEX mexOffsetV = pTD->GetHeight()*(cpi.cpi_rMinJ+cpi.cpi_rSizeJ/2);
    bpp.bpp_mdMappingDefinition.md_fUOffset-=mexOffsetU/1024.0f;
    bpp.bpp_mdMappingDefinition.md_fVOffset-=mexOffsetV/1024.0f;
  }
  return TRUE;
}

void CWorldEditorView::AutoApplyTextureOntoPolygon(CBrushPolygon &bpo, FLOATaabbox3D boxBrush, CTFileName &fnTex)
{
  // get box polygon that has smallest scalar product with our polygon
  FLOAT fMaxScalar=-1e6f;
  INDEX iMaxPlane=0;
  FLOATplane3D &plPlaneAbs=bpo.bpo_pbplPlane->bpl_plAbsolute;
  FLOATplane3D &plPlaneRel=bpo.bpo_pbplPlane->bpl_plRelative;
  for(INDEX iPlane=0; iPlane<6; iPlane++)
  {
    BoxPlaneParams &bpp=_abpp[iPlane];
    FLOAT fScalar=plPlaneAbs%bpp.bpp_plPlane;
    if( fScalar>fMaxScalar)
    {
      iMaxPlane=iPlane;
      fMaxScalar=fScalar;
    }
  }
  
  BoxPlaneParams &bpp=_abpp[iMaxPlane];
  CBrushPolygonTexture &bbpt0=bpo.bpo_abptTextures[0];
  CBrushPolygonTexture &bbpt1=bpo.bpo_abptTextures[1];
  CBrushPolygonTexture &bbpt2=bpo.bpo_abptTextures[2];
  try
  {
    bbpt0.CopyTextureProperties(CBrushPolygonTexture(),TRUE);
    bbpt0.bpt_toTexture.SetData_t(fnTex);
    bbpt1.bpt_toTexture.SetData(NULL);
    bbpt2.bpt_toTexture.SetData(NULL);

    // get the mapping in relative space of the brush polygon's entity
    CEntity *pen = bpo.bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush->br_penEntity;
    CSimpleProjection3D pr;
    pr.ObjectPlacementL() = _plOrigin;
    pr.ViewerPlacementL() = pen->GetPlacement();
    pr.Prepare();
    FLOATplane3D plRelative;
    pr.Project(bpp.bpp_plPlane, plRelative);
    CMappingDefinition mdRelative = bpp.bpp_mdMappingDefinition;
    mdRelative.Transform(bpp.bpp_plPlane, _plOrigin, pen->GetPlacement());

    bbpt0.bpt_mdMapping.ProjectMapping(plRelative, mdRelative, plPlaneRel);
    
    bpo.bpo_ulFlags |= BPOF_FULLBRIGHT;
    bpo.bpo_smShadowMap.Uncache();
    bpo.DiscardShadows();
  }
  catch (const char *strError)
  {
    WarningMessage(strError);
  }
}

void CWorldEditorView::AutoApplyTexture(FLOATaabbox3D boxBrush, CTFileName &fnTex)
{
  if( !SetAutoTextureBoxParams(boxBrush, fnTex)) return;

  CWorldEditorDoc* pDoc = GetDocument();
  // textureize mip levels
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    CEntity *pen=&*iten;
    if( pen->GetRenderType() == CEntity::RT_BRUSH)
    {
      CBrush3D *pbrBrush=pen->GetBrush();
      // for each mip in the brush
      BOOL bFirstMip=TRUE;
      FOREACHINLIST(CBrushMip, bm_lnInBrush, pbrBrush->br_lhBrushMips, itbm)
      {
        if( bFirstMip)
        {
          bFirstMip=FALSE;
          continue;
        }
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for each polygon in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo) {
            CBrushPolygon &bpo = *itbpo;
            AutoApplyTextureOntoPolygon(bpo, boxBrush, fnTex);
          }
        }        
      }
    }
  }
}

void CWorldEditorView::OnAutotexturizeMips() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  FLOATaabbox3D boxBrush;
  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    CEntity *pen=&*iten;
    if( pen->GetRenderType() == CEntity::RT_BRUSH)
    {
      CBrush3D *pbrBrush=pen->GetBrush();
      CBrushMip *pbrMip=pbrBrush->GetFirstMip();
      boxBrush|=pbrMip->bm_boxBoundingBox;
    }
  }}

  CTFileName fnTex;
  BOOL bSuccess=SaveAutoTexture( boxBrush, fnTex);
  if( !bSuccess) return;

  AutoApplyTexture( boxBrush, fnTex);

  // mark that selections have been changed
  pDoc->UpdateAllViews( NULL);
  pDoc->SetModifiedFlag();
}

void CWorldEditorView::OnUpdateAutotexturizeMips(CCmdUI* pCmdUI) 
{
	CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable(pDoc->m_selEntitySelection.Count()!=0);
}

void CWorldEditorView::OnRandomOffsetU() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // for each selected polygon
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    CTextureData *pTD = (CTextureData *) itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData();
    if( pTD != NULL)
    {
      // add rnd offsets to mapping
      itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fUOffset+=((FLOAT)rand())/(float)(RAND_MAX)*pTD->GetWidth();
    }
  }
}

void CWorldEditorView::OnRandomOffsetV() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  // for each selected polygon
  FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    CTextureData *pTD = (CTextureData *) itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData();
    if( pTD != NULL)
    {
      // add rnd offsets to mapping
      itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_mdMapping.md_fVOffset+=((FLOAT)rand())/(float)(RAND_MAX)*pTD->GetHeight();
    }
  }
}

void CWorldEditorView::OnStretchRelativeOffset() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if(m_penEntityHitOnContext==NULL) return;
  CDlgStretchChildOffset dlg;
  if( dlg.DoModal()!=IDOK) return;
  if( m_penEntityHitOnContext->IsSelected(ENF_SELECTED))
  {
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      if( iten->GetParent()!=NULL)
      {
        CPlacement3D pl=iten->GetPlacement();
        CPlacement3D plParent=iten->GetParent()->GetPlacement();
        pl.AbsoluteToRelative(plParent);
        FLOAT3D vRelPos=pl.pl_PositionVector;
        vRelPos*=dlg.m_fStretchValue;
        // set stretched offset
        pl.pl_PositionVector=vRelPos;
        pl.RelativeToAbsolute(plParent);
        iten->SetPlacement(pl);
      }
    }
  }
  else
  {
    if( m_penEntityHitOnContext->GetParent()!=NULL)
    {
      CPlacement3D pl=m_penEntityHitOnContext->GetPlacement();
      CPlacement3D plParent=m_penEntityHitOnContext->GetParent()->GetPlacement();
      pl.AbsoluteToRelative(plParent);
      FLOAT3D vRelPos=pl.pl_PositionVector;
      vRelPos*=dlg.m_fStretchValue;
      // set stretched offset
      pl.pl_PositionVector=vRelPos;
      pl.RelativeToAbsolute(plParent);
      m_penEntityHitOnContext->SetPlacement(pl);
    }
  }
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnDeselectHidden() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  // for all of the world's entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    // if the entity is hidden and selected
    if (iten->en_ulFlags&ENF_HIDDEN && iten->IsSelected( ENF_SELECTED))
    {
      // deselect it
      pDoc->m_selEntitySelection.Deselect( *iten);
    }
  }
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSelectHidden() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  // for all of the world's entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
  {
    // if the entity is hidden and selected
    if (iten->en_ulFlags&ENF_HIDDEN && !iten->IsSelected( ENF_SELECTED))
    {
      // select it
      pDoc->m_selEntitySelection.Select( *iten);
    }
  }
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnSectorsToBrush() 
{
  CBrushPolygonSelection selPolygons;
 	CWorldEditorDoc* pDoc = GetDocument();
  if(pDoc->m_selSectorSelection.Count()==0) return;

  pDoc->RememberUndo();
  pDoc->ClearSelections(ST_SECTOR);
  // make polygon selection
  FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
  {
    FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
    {
      selPolygons.Select( *itbpo);
    }
  }
  PolygonsToBrush(selPolygons, FALSE, TRUE);
  selPolygons.CDynamicContainer<CBrushPolygon>::Clear(); 
}

void CWorldEditorView::PolygonsToBrush(CBrushPolygonSelection &selPolygons, BOOL bDeleteSectors, BOOL bZoning)
{
 	CWorldEditorDoc* pDoc = GetDocument();

  FLOATaabbox3D boxPolygons;
  // get bbox of selected polygons
  FOREACHINDYNAMICCONTAINER(selPolygons, CBrushPolygon, itbpo)
  {
    boxPolygons |= itbpo->bpo_boxBoundingBox;
  }

  CPlacement3D plCenterDown=CPlacement3D(FLOAT3D(0,0,0), ANGLE3D(0,0,0));
  plCenterDown.pl_PositionVector=FLOAT3D(boxPolygons.Center()(1),boxPolygons.Min()(2),boxPolygons.Center()(3));

  CEntity *penwb=theApp.CreateWorldBaseEntity(pDoc->m_woWorld, bZoning, plCenterDown);
  if( penwb==NULL) return;

  pDoc->m_woWorld.CopyPolygonsToBrush(selPolygons, penwb);
  if(bDeleteSectors)
  {
    // delete old sectors
    pDoc->m_woWorld.DeleteSectors( pDoc->m_selSectorSelection, FALSE);
  }

  pDoc->ClearSelections();
  pDoc->m_chSelections.MarkChanged();
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnPolygonsToBrush() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_selPolygonSelection.Count()==0) return;
  pDoc->RememberUndo();

  CDynamicContainer<CBrushPolygon> dcPolygons;
  StorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);

  PolygonsToBrush(pDoc->m_selPolygonSelection, FALSE, FALSE);
  pDoc->m_woWorld.DeletePolygons(dcPolygons);
}

void CWorldEditorView::OnClonePolygons() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_selPolygonSelection.Count()==0) return;
  pDoc->RememberUndo();
  PolygonsToBrush(pDoc->m_selPolygonSelection, FALSE, FALSE);
}

void CWorldEditorView::OnDeletePolygons() 
{
  CDynamicContainer<CBrushPolygon> dcPolygons;
 	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->m_selPolygonSelection.Count()==0) return;
  pDoc->RememberUndo();

  StorePolygonSelection( pDoc->m_selPolygonSelection, dcPolygons);
  pDoc->ClearSelections();

  pDoc->m_woWorld.DeletePolygons(dcPolygons);

  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnKeyU() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    OnKeyPasteAsProjected();
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainEditMode=TEM_HEIGHTMAP;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    GetDocument()->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnKeyD() 
{
 	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == ENTITY_MODE)
  {
    OnDropMarker();
  }
  if( (pDoc->GetEditingMode() == POLYGON_MODE) ||
      (pDoc->GetEditingMode() == VERTEX_MODE) )
  {
    OnFlipPolygon();
  }
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_ERASE;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnFlipPolygon() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  BOOL bSelection=TRUE;

  // obtain information about where mouse points into the world
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse, FALSE, FALSE, FALSE);
  CBrushPolygon *pbpoHit=crRayHit.cr_pbpoBrushPolygon;

  if( (pbpoHit!=NULL) && !pbpoHit->IsSelected( BPOF_SELECTED) )
  {
    bSelection=FALSE;
  }

  CDynamicContainer<CBrushPolygon> dcPolygons;
  CDynamicContainer<CBrushSector> dcSectors;
  if( bSelection)
  {
    FOREACHINDYNAMICCONTAINER( pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      dcPolygons.Add( itbpo);
      if( !dcSectors.IsMember(itbpo->bpo_pbscSector))
      {
        dcSectors.Add(itbpo->bpo_pbscSector);
      }
    }
  }
  else
  {
    pDoc->m_selPolygonSelection.Clear();
    dcPolygons.Add( pbpoHit);
    dcSectors.Add(pbpoHit->bpo_pbscSector);
  }

  pDoc->ClearSelections();
  FOREACHINDYNAMICCONTAINER(dcPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpo = *itbpo;
    pDoc->m_woWorld.FlipPolygon(bpo);
  }

  FOREACHINDYNAMICCONTAINER( dcSectors, CBrushSector, itbsc)
  {
    itbsc->UpdateSector();
  }

  {FOREACHINDYNAMICCONTAINER(dcPolygons, CBrushPolygon, itbpo)
  {
    CBrushPolygon &bpo = *itbpo;
    INDEX iTriVtx=bpo.bpo_aiTriangleElements.Count();
    INDEX tmpa=0;
  }}

  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
}

void CWorldEditorView::OnKeyM() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    // obtain information about where mouse points into the world
    CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
    // if we hit brush entity
    if( (crRayHit.cr_penHit != NULL) &&
        (crRayHit.cr_pbpoBrushPolygon != NULL) )
    {
      CopyMapping(crRayHit.cr_pbpoBrushPolygon);
    }
  }
  else if( pDoc->GetEditingMode() == VERTEX_MODE)
  {
    OnMergeVertices();
  }
  else if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_MINIMUM;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    pDoc->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnSelectBrush() 
{
  CPoint ptMouse;
  GetCursorPos( &ptMouse); 
  InvokeTerrainBrushPalette( ptMouse.x-BRUSH_PALETTE_WIDTH/2, ptMouse.y+BRUSH_PALETTE_HEIGHT/2);
}

void CWorldEditorView::OnSelectTerrain() 
{
	CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit = GetMouseHitInformation( m_ptMouse);
  if( (crRayHit.cr_penHit!=NULL) && (crRayHit.cr_penHit->GetRenderType() == CEntity::RT_TERRAIN) &&
      (crRayHit.cr_penHit!=GetTerrainEntity()) )
  {
    pDoc->m_ptrSelectedTerrain=crRayHit.cr_penHit->GetTerrain();
    theApp.m_ctTerrainPage.MarkChanged();
  }
}

void CWorldEditorView::OnAltitudeEditMode() 
{
  theApp.m_iTerrainEditMode=TEM_HEIGHTMAP;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnLayerTextureEditMode() 
{
  theApp.m_iTerrainEditMode=TEM_LAYER;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnOptimizeTerrain() 
{
  OptimizeLayers();
}

void CWorldEditorView::OnRecalculateTerrainShadows() 
{
  CTerrain *ptrTerrain=GetTerrain();
  if( ptrTerrain==NULL) return;
  ptrTerrain->UpdateShadowMap();
}

void CWorldEditorView::OnViewHeightmap() 
{
  POINT pt;
  GetCursorPos(&pt);
  pt.x-=32;
  pt.y+=32;
  DisplayHeightMapWindow(pt);
}

void CWorldEditorView::OnImportHeightmap() 
{
  ApplyImportExport(0);
}

void CWorldEditorView::OnImportHeightmap16() 
{
  ApplyImportExport(1);
}

void CWorldEditorView::OnExportHeightmap() 
{
  ApplyImportExport(2);
}

void CWorldEditorView::OnExportHeightmap16() 
{
  ApplyImportExport(3);
}

void CWorldEditorView::OnSelectLayer() 
{
  InvokeSelectLayerCombo();
}

void CWorldEditorView::OnPickLayer() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  CCastRay crRayHit=GetMouseHitInformation( m_ptMouse);
  if( (crRayHit.cr_penHit!=NULL) && (crRayHit.cr_penHit->GetRenderType() == CEntity::RT_TERRAIN))
  {
    CTerrain *ptrTerrain=crRayHit.cr_penHit->GetTerrain();
    Point pt=Calculate2dHitPoint(ptrTerrain, crRayHit.cr_vHit);

    // in altitude mode
    if(theApp.m_iTerrainEditMode==TEM_HEIGHTMAP)
    {
      // pick altitude
      INDEX iWidth=ptrTerrain->tr_pixHeightMapWidth;
      UWORD *puw=ptrTerrain->tr_auwHeightMap+iWidth*pt.pt_iY+pt.pt_iX;
      theApp.m_uwEditAltitude=*puw;
    }
    // in layer mode
    else
    {
      // pick layer
      INDEX ctLayers=ptrTerrain->tr_atlLayers.Count();
      for(INDEX iLayer=ctLayers-1; iLayer>=0; iLayer--)
      {
        UBYTE ubPower=GetValueFromMask(ptrTerrain, iLayer, crRayHit.cr_vHit);
        if(ubPower>0)
        {
          SelectLayer(iLayer);
          CTerrainLayer *ptlLayer=&ptrTerrain->tr_atlLayers[iLayer];
          if(ptlLayer->tl_ltType==LT_TILE)
          {
            CDynamicContainer<CTileInfo> dcTileInfo;
            INDEX ctTilesPerRaw=0;
            ObtainLayerTileInfo( &dcTileInfo, ptlLayer->tl_ptdTexture, ctTilesPerRaw);
            INDEX ctTiles=dcTileInfo.Count();
            for(INDEX iTile=0; iTile<ctTiles; iTile++)
            {
              CTileInfo &ti=dcTileInfo[iTile];
              if( ((ubPower&TL_TILE_INDEX)==(ti.ti_iy*ptlLayer->tl_ctTilesInRow+ti.ti_ix) ) &&
                  (ubPower&TL_VISIBLE) &&
                  (!(ubPower&TL_FLIPX)  == !ti.ti_bFlipX) &&
                  (!(ubPower&TL_FLIPY)  == !ti.ti_bFlipY) &&
                  (!(ubPower&TL_SWAPXY) == !ti.ti_bSwapXY) )
              {
                ptlLayer->tl_iSelectedTile=iTile;
                break;
              }
            }
          }
          else
          {
            theApp.m_ctTerrainPageCanvas.MarkChanged();
          }
          break;
        }
      }
    }
  }
}

void CWorldEditorView::OnIdle(void)
{
  POINT point;
  GetCursorPos(&point);
  HWND hwndUnderMouse = ::WindowFromPoint(point);
  if( ::GetParent(hwndUnderMouse)==m_hWnd)
  {
    UpdateCursor();
  }
  
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && m_iaInputAction==IA_NONE)
  {
    UpdateLayerDistribution();
  }
}

void CWorldEditorView::OnPosterize() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_POSTERIZE;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    GetDocument()->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnFlatten() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode() == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_FLATTEN;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    GetDocument()->SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorView::OnApplyFilter() 
{
  ApplyFilterOntoTerrain();
}

void CWorldEditorView::OnTeSmooth() 
{
  ApplySmoothOntoTerrain();
}

void CWorldEditorView::OnEditTerrainPrefs() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CDlgTEOperationSettings dlg(pMainFrame);
  dlg.DoModal();
}

void CWorldEditorView::OnUpdateEditTerrainPrefs(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->GetEditingMode()==TERRAIN_MODE);
}


void CWorldEditorView::OnKeyCtrlShiftG() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if(pDoc->GetEditingMode()==TERRAIN_MODE)
  {
    RandomizeWhiteNoise();
    ApplyGenerateTerrain();
  }
  else
  {
    GetChildFrame()->OnGridOnOff();
  }
}

void CWorldEditorView::OnUpdateKeyCtrlShiftG(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if(pDoc->GetEditingMode()==TERRAIN_MODE)
  {
    pCmdUI->Enable( TRUE);
  }
  else
  {
    GetChildFrame()->OnUpdateGridOnOff(pCmdUI);
  }
}

void CWorldEditorView::OnTerrainLayerOptions() 
{
  CTerrainLayer *ptlLayer=GetLayer();
  if(ptlLayer==NULL) return;

  CDlgEditTerrainLayer dlg;
  if( dlg.DoModal()==IDOK)
  {
    INDEX iLayer=GetLayerIndex();
    GenerateLayerDistribution(iLayer);
  }
}

void CWorldEditorView::OnUpdateTerrainLayerOptions(CCmdUI* pCmdUI) 
{
  CWorldEditorDoc* pDoc = GetDocument();
  pCmdUI->Enable( pDoc->GetEditingMode()==TERRAIN_MODE);
}


void CWorldEditorView::OnApplyContinousNoise() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    ApplyContinousNoiseOntoTerrain();
  }
}

void CWorldEditorView::OnApplyMinimum() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    ApplyMinimumOntoTerrain();
  }
}

void CWorldEditorView::OnApplyMaximum() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    ApplyMaximumOntoTerrain();
  }
}

void CWorldEditorView::OnApplyFlatten() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    ApplyFlattenOntoTerrain();
  }
}

void CWorldEditorView::OnApplyPosterize() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    ApplyPosterizeOntoTerrain();
  }
}

void CWorldEditorView::OnOptimizeLayers() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    OptimizeLayers();
  }
}

void CWorldEditorView::OnTbrushAltitude() 
{
  theApp.m_iTerrainBrushMode=TBM_PAINT;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushSmooth() 
{
  theApp.m_iTerrainBrushMode=TBM_SMOOTH;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushEquilaze() 
{
  theApp.m_iTerrainBrushMode=TBM_FLATTEN;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushErase() 
{
  theApp.m_iTerrainBrushMode=TBM_ERASE;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushNoise() 
{
  theApp.m_iTerrainBrushMode=TBM_RND_NOISE;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushContinousNoise() 
{
  theApp.m_iTerrainBrushMode=TBM_CONTINOUS_NOISE;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushFilter() 
{
  theApp.m_iTerrainBrushMode=TBM_FILTER;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushFlatten() 
{
  theApp.m_iTerrainBrushMode=TBM_FLATTEN;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushMaximum() 
{
  theApp.m_iTerrainBrushMode=TBM_MAXIMUM;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushMinimum() 
{
  theApp.m_iTerrainBrushMode=TBM_MINIMUM;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTbrushPosterize() 
{
  theApp.m_iTerrainBrushMode=TBM_POSTERIZE;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
  GetDocument()->SetStatusLineModeInfoMessage();
}

void CWorldEditorView::OnTerrainProperties() 
{
  CWorldEditorDoc* pDoc = GetDocument();
  if( pDoc->GetEditingMode()==TERRAIN_MODE && GetTerrain()!=NULL)
  {
    CDlgTerrainProperties dlg;
    dlg.DoModal();
  }
}
