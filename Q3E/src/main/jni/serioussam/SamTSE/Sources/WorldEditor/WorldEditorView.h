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

// WorldEditorView.h : interface of the CWorldEditorView class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef WORLDEDITORVIEW_H
#define WORLDEDITORVIEW_H 1


extern BOOL MyChooseColor( COLORREF &clrNewColor, CWnd &wndOwner);

#define WM_CHANGE_EDITING_MODE WM_USER
#define GRID_DISCRETE_VALUES (theApp.m_bDecadicGrid ? 10:8)

// mip modes (modes for setting or changing mip switch factors)
#define MM_NONE 0       // not active
#define MM_SETTING 1    // setting mip switch factor for current brush
#define MM_MANUAL 2     // setting manual mip factor

#define OVXF_CLOSEST  (1L<<8)
#define OVXF_SELECTED (1L<<9)

enum InputAction {
  IA_NONE = 0,
  IA_DRAG_VERTEX_ON_PRIMITIVE,
  IA_DRAG_VERTEX_ON_PRIMITIVE_BASE,
  IA_SELECT_SINGLE_BRUSH_VERTEX,
  IA_SELECT_LASSO_BRUSH_VERTEX,
  IA_DRAG_BRUSH_VERTEX_IN_FLOOR_PLANE,
  IA_DRAG_BRUSH_VERTEX_IN_VIEW_PLANE,
  IA_ROTATE_BRUSH_VERTEX,
  IA_STRETCH_BRUSH_VERTEX,
  IA_STRETCHING_PRIMITIVE,
  IA_SHEARING_PRIMITIVE,
  IA_TERRAIN_EDITING_LMB,
  IA_TERRAIN_EDITING_CTRL_LMB,
  IA_MOVING_VIEWER_IN_FLOOR_PLANE,
  IA_MOVING_VIEWER_IN_VIEW_PLANE,
  IA_MOVING_SECOND_LAYER_IN_FLOOR_PLANE,
  IA_MOVING_SECOND_LAYER_IN_VIEW_PLANE,
  IA_MOVING_ENTITY_SELECTION_IN_FLOOR_PLANE,
  IA_MOVING_ENTITY_SELECTION_IN_VIEW_PLANE,
  IA_MOVING_POLYGON_MAPPING,
  IA_ROTATING_VIEWER,
  IA_ROTATING_ENTITY_SELECTION,
  IA_ROTATING_SECOND_LAYER,
  IA_ROTATING_POLYGON_MAPPING,
  IA_MEASURING,
  IA_CUT_MODE,
  IA_MOVING_CUT_LINE_START,
  IA_MOVING_CUT_LINE_END,
  IA_SIZING_SELECT_BY_VOLUME_BOX,
  IA_SELECTING_POLYGONS,
  IA_SELECTING_SECTORS,
  IA_SELECTING_ENTITIES,
  IA_SELECT_LASSO_ENTITY,
  IA_MIP_SETTING,
  IA_MANUAL_MIP_SWITCH_FACTOR_CHANGING,
  IA_CHANGING_RANGE_PROPERTY,
  IA_CHANGING_ANGLE3D_PROPERTY,
};

class CWorldEditorView : public CView      
{
protected: // create from serialization only
	CWorldEditorView();
	DECLARE_DYNCREATE(CWorldEditorView)

// Attributes
public:
	InputAction m_iaInputAction;
  InputAction m_iaLastInputAction;
  BOOL m_bTestGameOn;

  CBrushMip *m_pbmToSetMipSwitch;
  COleDataSource m_DataSource;  // to enable drag and drop
  CTimerValue m_tvLastTime;     // last time view was refreshed
  CViewPrefs m_vpViewPrefs;     // current rendering preferences
  CTextureObject m_toBcgPicture;// used for background texture 
  CTString m_strMeasuring;

  CChangeableRT m_chViewChanged;// when view has been rendered
  BOOL m_bCutMode;              // if we are in cut mode

  // type of view
  enum CSlaveViewer::ProjectionType m_ptProjectionType;
  BOOL m_bWeDeselectedFirstPolygon;
  BOOL m_bWeDeselectedFirstSector;
  CPoint m_ptMouseDown;         // mouse position at mouse down time
  CStaticStackArray<PIX2D> m_avpixLaso; // coordinates for laso
  BOOL m_bRequestVtxClickSelect;    // if vertex select test requested from renderer
  BOOL m_bRequestVtxLassoSelect;    // if vertex laso select test requested from renderer
  BOOL m_bRequestEntityLassoSelect; // if vertex laso select test requested from renderer
  BOOL m_bOnSelectVertexAltDown;    // if alt was down during vertex select start
  BOOL m_bOnSelectVertexShiftDown;  // if shift was down during vertex select start
  BOOL m_bOnSelectEntityAltDown;    // if alt was down during entity lasso select start
  BOOL m_bOnSelectEntityShiftDown;  // if shift was down during entity lasso select start
  FLOAT3D m_vHitOnMouseDown;
	CPoint m_ptMouse;             // current mouse position
  FLOAT m_fpixGridSteep;        // steep in float pixels f of grid line
  FLOAT3D m_f3dRotationOrigin;  // used as rotation origin while changing mapping coordinates
  FLOATplane3D m_plTranslationPlane;  // plane used for polgon mapping translation
  CPlacement3D m_plMouseMove;   // used for continous mouse editting
  CPlacement3D m_plMouseOffset; // used for offseted mouse editting
  CBrushPolygon *m_pbpoTranslationPlane;
  // current grid in meters
  FLOAT m_fGridInMeters;
  FLOAT m_fGridX;
  FLOAT m_fGridY;
  CTString m_strTest;
  
  BOOL m_bEntityHitedOnContext;
  CPlacement3D m_plEntityHitOnContext;
  CEntity *m_penEntityHitOnContext;
  CBrushPolygon *m_pbpoRightClickedPolygon;
  CTString m_strTerrainDataPaneText;

  // index of vertice on primitive base that user is currently dragging
  INDEX m_iDragVertice;
  INDEX m_iDragEdge;
  // index of size control vertice
  INDEX m_iSizeControlVertice;
  FLOAT3D m_vMouseDownSecondLayer;
  // position of vertice on primitive base before drag started
  DOUBLE3D m_vStartDragVertice;
  // position of edge center on primitive base before drag started
  DOUBLE3D m_vStartDragEdge;
  // position of vertice of object 3D before drag started
  DOUBLE3D m_vStartDragO3DVertice;

  // values for primitive are remembered here when LMB is pressed (for latere continous moving)
  CValuesForPrimitive m_VFPMouseDown;
  CDrawPort *m_pdpDrawPort;
  CViewPort *m_pvpViewPort;
  COleDropTarget m_DropTarget;

	CTFileName m_fnWinBcgTexture;
  COLORREF m_SelectionColor;
	COLORREF m_PaperColor;
	COLORREF m_InkColor;
	BOOL m_IsWinBcgTexture;
// Operations
public:
  // obtain draw port
  inline CDrawPort *GetDrawPort( void) {
    if( theApp.m_bChangeDisplayModeInProgress)
      return NULL;
    else
      return m_pdpDrawPort;
  };
  // obtain view port
  inline CViewPort *GetViewPort( void)
  {
    if( theApp.m_bChangeDisplayModeInProgress)
      return NULL;
    else
      return m_pvpViewPort;
  };
  void GetToolTipText( char *pToolTipText);
  // renders one picture
  void RenderView( CDrawPort *pDP);
  // obtain information about what was hit with mouse
  CCastRay GetMouseHitInformation( CPoint point, BOOL bHitPortals = FALSE,
    BOOL bHitModels = TRUE, BOOL bHitFields = TRUE, CEntity *penSourceEntity = NULL, BOOL bHitBrushes=TRUE);
  FLOAT3D GetMouseHitOnPlane( CPoint point, const FLOATplane3D &plPlane);
  // Start and stop functions that are called for start moving/rotating 
  void ToggleHittedPolygon( CCastRay &crRayHit);
  void ToggleHittedSector( CCastRay &crRayHit);
  void StartMouseInput( CPoint point);
  void StopMouseInput( void);
  void MarkClosestVtxOnPrimitive( BOOL bToggleSelection);
  void MarkClosestVtxAndEdgeOnPrimitiveBase(CPoint point);
  void DragVerticesOnPrimitive(FLOAT fDX,FLOAT fDY,FLOAT fDZ, BOOL bAbsolute, BOOL bSnap=TRUE);
  void DragBrushVertex(FLOAT fDX,FLOAT fDY,FLOAT fDZ);
  void RotateOrStretchBrushVertex(FLOAT fDX, FLOAT fDY, BOOL bRotate);
  // Fills status line's editing data info pane
  void SetEditingDataPaneInfo(BOOL bImidiateRepainting);
	CWorldEditorDoc* GetDocument(void);
	/* gets pointer to MDIFrameWnd main frame of application */
	CMainFrame *GetMainFrame(void);
  /* set parameters for projection depending on current rendering preferences. */
  void SetProjection(CDrawPort *pDP);
  /* called by document at the beginning of CSG */
  void AtStartCSG(void);
  /* called by document at the end of CSG */
  void AtStopCSG(void);
  /* get pointer to the child frame of this view */
  CChildFrame *GetChildFrame(void);
  /* if delete entity operation is allowed returns true */
  BOOL IsDeleteEntityEnabled(void);
  // remove given entity from linked chain
  void RemoveFromLinkedChain(CEntity *pen);
  /* Returns curently active mip factor (auto or manual one) */
  FLOAT GetCurrentlyActiveMipFactor(void);
  /* obtain point in the world where mouse pointed last time it was moved */
  CPlacement3D GetMouseInWorldPlacement(void);

  void CreatePrimitiveCalledFromPopup();
  void RenderBackdropTexture(CDrawPort *pDP,FLOAT3D v0, FLOAT3D v1, FLOAT3D v2, FLOAT3D v3,
                             CTextureObject &to);
  void OnAlignPrimitive(void);
  void CenterSelected(void);
  void AllignBox( FLOATaabbox3D bbox);
  void AllignPolygon( CBrushPolygon *pbpo);
  void EditCopy( BOOL bAlternative);
  void CopyMapping(CBrushPolygon *pbpo);
  void PasteMappingOnOnePolygon(CBrushPolygon *pbpo, BOOL bAsProjected);
  void PasteMapping(CBrushPolygon *pbpo, BOOL bAsProjected);
  void PasteOneLayerMapping(INDEX iLayer, CMappingDefinition &md,
    FLOATplane3D &pl, CBrushPolygon *pbpo, BOOL bAsProjected);
  void PasteTexture( CBrushPolygon *pbpoPolygon);
  void CopySectorAmbient( CBrushSector *pbscSector);
  void PasteSectorAmbient( CBrushSector *pbscSector);
  void StorePolygonSelection(CBrushPolygonSelection &selPolygons,
                                               CDynamicContainer<CBrushPolygon> &dcPolygons);
  void RestorePolygonSelection(CBrushPolygonSelection &selPolygons,
                                                 CDynamicContainer<CBrushPolygon> &dcPolygons);
  void DiscardShadows( CEntity *penEntity);
  void DiscardShadows( CBrushSector *pbscSector);
  void DiscardShadows( CBrushPolygon *pbpoPolygon);
  void ApplyDefaultMapping(CBrushPolygon *pbpo, BOOL bRotation, BOOL bOffset, BOOL bStretch);

  // get current brush mip of current csg target brush
  CBrushMip *GetCurrentBrushMip(void);
  void SetMipBrushFactor(void);
  void OnAddMorePreciseMip(BOOL bClone);
  void OnAddRougherMipLevel(BOOL bClone);
  
  void OnNextPolygon(void);
  void OnPreviousPolygon(void);
  void Rotate( FLOAT fAngleLR, FLOAT fAngleUD, BOOL bSmooth=FALSE);
  void MultiplyMappingOnPolygon( FLOAT fFactor);
  void CallPopupMenu(CPoint point);
  void DrawArrowAndTypeText( CProjection3D &prProjection,
    const FLOAT3D &v0, const FLOAT3D &v1, COLOR colColor, CTString strText);
  void DrawAxis( const PIX2D &pixC, PIX len, COLOR colColor, COLOR colTextColor, CTString strU, CTString strV);
  
  void SnapVector(FLOAT3D &vToSnap);
  void SnapVector(DOUBLE3D &vToSnap);
  FLOAT3D ProjectVectorToWorldSpace(FLOAT fDX,FLOAT fDY,FLOAT fDZ);
  FLOAT3D Get3DCoordinateFrom2D( POINT &pt);
  POINT Get2DCoordinateFrom3D( FLOAT3D vPoint);
  BOOL IsCutEnabled(CTString &strError);
  void SelectAllTargetsOfEntity(CEntity *pen);
  void SelectAllTargetsOfSelectedEntities(void);
  BOOL IsSelectClonesOnContextEnabled( void);
  BOOL IsSelectOfSameClassOnContextEnabled( void);
  void ApplyFreeModeControls( CPlacement3D &pl, ANGLE3D &a, FLOAT &fSpeedMultiplier, BOOL bPrescan);
  void PumpWindowsMessagesInFreeMode(BOOL &bRunning, FLOAT &fSpeedMultiplier);
  void SelectWhoTargets( CDynamicContainer<CEntity> &dcTargetedEntities);
	void OnRemainSelectedByOrientation(BOOL bBothSides);
  void OnDropMarker(CPlacement3D plMarker);
  // functions for mapping fitting
  void AutoFitMapping(CBrushPolygon *pbpo, BOOL bInvert=FALSE, BOOL bFitBoth=FALSE);
  void AutoFitMappingOnPolygon(CBrushPolygon &bpo, BOOL bInvert=FALSE, BOOL bFitBoth=FALSE);
  void SetAsCsgTarget(CEntity *pen);
  void ShowLinkTree(CEntity *pen, BOOL bWhoTargets=FALSE, BOOL bPropertyNames=FALSE);
  BOOL SaveAutoTexture(FLOATaabbox3D boxBrush, CTFileName &fnTex);
  void AutoApplyTexture(FLOATaabbox3D boxBrush, CTFileName &fnTex);
  void AutoApplyTextureOntoPolygon(CBrushPolygon &bpo, FLOATaabbox3D boxBrush, CTFileName &fnTex);
  BOOL SetAutoTextureBoxParams(FLOATaabbox3D boxBrush, CTFileName &fnTex);
  void Align(BOOL bX,BOOL bY,BOOL bZ,BOOL bH,BOOL bP,BOOL bB);
  void PolygonsToBrush(CBrushPolygonSelection &selPolygons, BOOL bDeleteSectors, BOOL bZoning);
  void SnapSelectedVerticesToPlane(void);
  void RenderAndApplyTerrainEditBrush(FLOAT3D vHit);
  void InvokeSelectLayerCombo(void);
  void OnIdle(void);
  void UpdateCursor(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorldEditorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWorldEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
public:
	//{{AFX_MSG(CWorldEditorView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnIsometricFront();
	afx_msg void OnIsometricBack();
	afx_msg void OnIsometricBottom();
	afx_msg void OnIsometricLeft();
	afx_msg void OnIsometricRight();
	afx_msg void OnIsometricTop();
	afx_msg void OnPerspective();
	afx_msg void OnZoomLess();
	afx_msg void OnZoomMore();
	afx_msg void OnMoveDown();
	afx_msg void OnMoveUp();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMeasurementTape();
	afx_msg void OnUpdateMeasurementTape(CCmdUI* pCmdUI);
	afx_msg void OnCircleModes();
	afx_msg void OnUpdateCircleModes(CCmdUI* pCmdUI);
	afx_msg void OnDeselectAll();
	afx_msg void OnDeleteEntities();
	afx_msg void OnUpdateDeleteEntities(CCmdUI* pCmdUI);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTakeSs();
	afx_msg void OnUpdateEntityMode(CCmdUI* pCmdUI);
	afx_msg void OnSectorMode();
	afx_msg void OnUpdateSectorMode(CCmdUI* pCmdUI);
	afx_msg void OnPolygonMode();
	afx_msg void OnUpdatePolygonMode(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnEditCopy();
	afx_msg void OnCloneCSG();
	afx_msg void OnUpdateCloneCsg(CCmdUI* pCmdUI);
	afx_msg void OnMeasureOn();
	afx_msg void OnUpdateMeasureOn(CCmdUI* pCmdUI);
	afx_msg void OnResetViewer();
	afx_msg void OnCopyTexture();
	afx_msg void OnPasteTexture();
	afx_msg void OnCenterEntity();
	afx_msg void OnFunction();
	afx_msg void OnUpdateCenterEntity(CCmdUI* pCmdUI);
	afx_msg void OnDropMarker();
	afx_msg void OnUpdateDropMarker(CCmdUI* pCmdUI);
	afx_msg void OnTestConnections();
	afx_msg void OnUpdateTestConnections(CCmdUI* pCmdUI);
	afx_msg void OnAlignVolume();
	afx_msg void OnUpdateAlignVolume(CCmdUI* pCmdUI);
	afx_msg void OnCurrentViewProperties();
	afx_msg void OnDeleteMip();
	afx_msg void OnUpdateDeleteMip(CCmdUI* pCmdUI);
	afx_msg void OnPreviousMipBrush();
	afx_msg void OnUpdatePreviousMipBrush(CCmdUI* pCmdUI);
	afx_msg void OnNextMipBrush();
	afx_msg void OnUpdateNextMipBrush(CCmdUI* pCmdUI);
	afx_msg void OnCrossroadForC();
	afx_msg void OnChooseColor();
	afx_msg void OnUpdateChooseColor(CCmdUI* pCmdUI);
	afx_msg void OnMenuCopyMapping();
	afx_msg void OnMenuPasteMapping();
	afx_msg void OnSetAsCsgTarget();
	afx_msg void OnKeyPaste();
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSelectByTextureAdjacent();
	afx_msg void OnSelectByTextureInSector();
	afx_msg void OnSelectByColorInSector();
	afx_msg void OnConusPrimitive();
	afx_msg void OnTorusPrimitive();
	afx_msg void OnTerrainPrimitive();
	afx_msg void OnSpherePrimitive();
	afx_msg void OnStaircasePrimitive();
	afx_msg void OnUpdateConusPrimitive(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSpherePrimitive(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTerrainPrimitive(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTorusPrimitive(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStaircasePrimitive(CCmdUI* pCmdUI);
	afx_msg void OnPopupConus();
	afx_msg void OnPopupSphere();
	afx_msg void OnPopupStairs();
	afx_msg void OnPopupTerrain();
	afx_msg void OnPopupTorus();
	afx_msg void OnUpdateMoveDown(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveUp(CCmdUI* pCmdUI);
	afx_msg void OnSelectLights();
	afx_msg void OnDiscardShadows();
	afx_msg void OnCopySectorAmbient();
	afx_msg void OnPasteSectorAmbient();
	afx_msg void OnSelectAllPolygons();
	afx_msg void OnCopySectors();
	afx_msg void OnDeleteSectors();
	afx_msg void OnPasteSectors();
	afx_msg void OnCenterBcgViewer();
	afx_msg void OnMenuPasteAsProjectedMapping();
	afx_msg void OnKeyPasteAsProjected();
	afx_msg void OnSelectAllEntitiesInSectors();
	afx_msg void OnSelectAllSectors();
	afx_msg void OnLastPrimitive();
	afx_msg void OnUpdateLastPrimitive(CCmdUI* pCmdUI);
	afx_msg void OnCloneToMorePreciseMip();
	afx_msg void OnCloneToRougherMipLevel();
	afx_msg void OnCreateEmptyMorePreciseMip();
	afx_msg void OnCreateEmptyRougherMip();
	afx_msg void OnUpdateCloneToMorePreciseMip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCloneToRougherMipLevel(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCreateEmptyMorePreciseMip(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCreateEmptyRougherMip(CCmdUI* pCmdUI);
	afx_msg void OnEditPasteAlternative();
	afx_msg void OnSelectAllEntitiesInWorld();
	afx_msg void OnUpdateEditPasteAlternative(CCmdUI* pCmdUI);
	afx_msg void OnEntityMode();
	afx_msg void OnFindTexture();
	afx_msg void OnSelectSectorsWithSameName();
	afx_msg void OnSelectSectorsArroundEntity();
	afx_msg void OnSelectSectorsArroundEntityOnContext();
	afx_msg void OnInsertVertex();
	afx_msg void OnDeleteVertex();
	afx_msg void OnSavePicturesForEnvironment();
	afx_msg void OnUpdateSavePicturesForEnvironment(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnCsgSelectSector();
	afx_msg void OnMenuAlignMappingU();
	afx_msg void OnMenuAlignMappingV();
	afx_msg void OnDestroy();
	afx_msg void OnFallDown();
	afx_msg void OnPrevious();
	afx_msg void OnNext();
	afx_msg void OnUpdatePrevious(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNext(CCmdUI* pCmdUI);
	afx_msg void OnRemoveUnusedTextures();
	afx_msg void OnRotate();
	afx_msg void OnRotateBack();
	afx_msg void OnSelectVisibleSectors();
	afx_msg void OnEditCopyAlternative();
	afx_msg void OnRotateLeft();
	afx_msg void OnRotateRight();
	afx_msg void OnRotateUp();
	afx_msg void OnRotateDown();
	afx_msg void OnSelectWhoTargets();
	afx_msg void OnSelectInvalidTris();
	afx_msg void OnTestConnectionsBack();
	afx_msg void OnUpdateTestConnectionsBack(CCmdUI* pCmdUI);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnSelectSectorsOtherSide();
	afx_msg void OnSelectLinksToSector();
	afx_msg void OnRemainSelectedByOrientation();
	afx_msg void OnDeselectByOrientation();
	afx_msg void OnVertexMode();
	afx_msg void OnReoptimizeBrushes();
	afx_msg void OnMergeVertices();
	afx_msg void OnExportDisplaceMap();
	afx_msg void OnCutMode();
	afx_msg void OnUpdateCutMode(CCmdUI* pCmdUI);
	afx_msg void OnSelectAllTargets();
	afx_msg void OnSelectAllTargetsOnContext();
	afx_msg void OnSelectClones();
	afx_msg void OnSelectClonesOnContext();
	afx_msg void OnUpdateSelectClones(CCmdUI* pCmdUI);
	afx_msg void OnSelectAllVertices();
	afx_msg void OnSelectOfSameClass();
	afx_msg void OnSelectOfSameClassOnContext();
	afx_msg void OnAlternativeMovingMode();
	afx_msg void OnReTriple();
	afx_msg void OnSelectWhoTargetsOnContext();
	afx_msg void OnClearAllTargets();
	afx_msg void OnSelectCsgTarget();
	afx_msg void OnUpdateSelectCsgTarget(CCmdUI* pCmdUI);
	afx_msg void OnRemainSelectedbyOrientationSingle();
	afx_msg void OnUpdateReTriple(CCmdUI* pCmdUI);
	afx_msg void OnTriangularizePolygon();
	afx_msg void OnEntityContextHelp();
	afx_msg void OnPopupAutoFitMapping();
	afx_msg void OnTriangularizeSelection();
	afx_msg void OnUpdateTriangularizeSelection(CCmdUI* pCmdUI);
	afx_msg void OnPopupAutoFitMappingSmall();
	afx_msg void OnPopupAutoFitMappingBoth();
	afx_msg void OnResetMappingOffset();
	afx_msg void OnResetMappingRotation();
	afx_msg void OnResetMappingStretch();
	afx_msg void OnCrossroadForL();
	afx_msg void OnSelectUsingTargetTree();
	afx_msg void OnTargetTree();
	afx_msg void OnUpdateTargetTree(CCmdUI* pCmdUI);
	afx_msg void OnSwapLayers12();
	afx_msg void OnSwapLayers23();
	afx_msg void OnSelectDescendants();
	afx_msg void OnCrossroadForCtrlF();
	afx_msg void OnRotateToTargetCenter();
	afx_msg void OnRotateToTargetOrigin();
	afx_msg void OnCopyOrientation();
	afx_msg void OnCopyPlacement();
	afx_msg void OnCopyPosition();
	afx_msg void OnPasteOrientation();
	afx_msg void OnPastePlacement();
	afx_msg void OnPastePosition();
	afx_msg void OnAlignB();
	afx_msg void OnAlignH();
	afx_msg void OnAlignP();
	afx_msg void OnAlignX();
	afx_msg void OnAlignY();
	afx_msg void OnAlignZ();
	afx_msg void OnAutotexturizeMips();
	afx_msg void OnUpdateAutotexturizeMips(CCmdUI* pCmdUI);
	afx_msg void OnRandomOffsetU();
	afx_msg void OnRandomOffsetV();
	afx_msg void OnStretchRelativeOffset();
	afx_msg void OnDeselectHidden();
	afx_msg void OnSelectHidden();
	afx_msg void OnSectorsToBrush();
	afx_msg void OnPolygonsToBrush();
	afx_msg void OnClonePolygons();
	afx_msg void OnDeletePolygons();
	afx_msg void OnKeyU();
	afx_msg void OnKeyD();
	afx_msg void OnFlipPolygon();
	afx_msg void OnTerrainMode();
	afx_msg void OnUpdateTerrainMode(CCmdUI* pCmdUI);
	afx_msg void OnKeyM();
	afx_msg void OnKeyBackslash();
	afx_msg void OnSelectBrush();
	afx_msg void OnSelectTerrain();
	afx_msg void OnAltitudeEditMode();
	afx_msg void OnLayerTextureEditMode();
	afx_msg void OnTbrushAltitude();
	afx_msg void OnTbrushEquilaze();
	afx_msg void OnTbrushErase();
	afx_msg void OnTbrushNoise();
	afx_msg void OnTbrushSmooth();
	afx_msg void OnOptimizeTerrain();
	afx_msg void OnRecalculateTerrainShadows();
	afx_msg void OnViewHeightmap();
	afx_msg void OnImportHeightmap();
	afx_msg void OnExportHeightmap();
	afx_msg void OnImportHeightmap16();
	afx_msg void OnExportHeightmap16();
	afx_msg void OnSelectLayer();
	afx_msg void OnPickLayer();
	afx_msg void OnKeyO();
	afx_msg void OnUpdateKeyO(CCmdUI* pCmdUI);
	afx_msg void OnPosterize();
	afx_msg void OnFlatten();
	afx_msg void OnApplyFilter();
	afx_msg void OnTeSmooth();
	afx_msg void OnEditTerrainPrefs();
	afx_msg void OnUpdateEditTerrainPrefs(CCmdUI* pCmdUI);
	afx_msg void OnKeyCtrlShiftE();
	afx_msg void OnKeyCtrlShiftG();
	afx_msg void OnUpdateKeyCtrlShiftG(CCmdUI* pCmdUI);
	afx_msg void OnTerrainLayerOptions();
	afx_msg void OnUpdateTerrainLayerOptions(CCmdUI* pCmdUI);
	afx_msg void OnKeyCtrlShiftK();
	afx_msg void OnApplyContinousNoise();
	afx_msg void OnApplyMinimum();
	afx_msg void OnApplyMaximum();
	afx_msg void OnApplyFlatten();
	afx_msg void OnApplyPosterize();
	afx_msg void OnOptimizeLayers();
	afx_msg void OnTbrushContinousNoise();
	afx_msg void OnTbrushFilter();
	afx_msg void OnTbrushFlatten();
	afx_msg void OnTbrushMaximum();
	afx_msg void OnTbrushMinimum();
	afx_msg void OnTbrushPosterize();
	afx_msg void OnTerrainProperties();
	//}}AFX_MSG

  afx_msg void OnKeyBuffer(UINT nID);
  afx_msg void OnKeyEditBuffer(UINT nID); 
	
  DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in WorldEditorView.cpp
inline CWorldEditorDoc* CWorldEditorView::GetDocument()
   { return (CWorldEditorDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
#endif // WORLDEDITORVIEW_H
