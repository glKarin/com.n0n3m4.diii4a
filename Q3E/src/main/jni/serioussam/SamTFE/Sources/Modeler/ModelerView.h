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

// ModelerView.h : interface of the CModelerView class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef MODELERVIEW_H
#define MODELERVIEW_H 1
extern BOOL MyChooseColor( COLORREF &clrNewColor, CWnd &wndOwner);
class CMainFrame;

#define STATUS_LINE_PANE 0
// macro used for writing text into status line
#define STATUS_LINE_MESSAGE( message) {\
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd()); \
  ASSERT( pMainFrame != NULL); \
  pMainFrame->m_wndStatusBar.SetPaneText( STATUS_LINE_PANE, message, TRUE);};

enum AxisType {
  AT_NONE = 0,
  AT_MAIN,
  AT_ALL,
};

enum InputAction {
  IA_NONE = 0,
  // model placement
  IA_MOVING_VIEWER,
  IA_ZOOMING_VIEWER,
  IA_ROTATING_VIEWER,
  // model placement
  IA_MOVING_MODEL,
  IA_ZOOMING_MODEL,
  IA_ROTATING_MODEL,
  // mapping placement
  IA_MOVING_MAPPING,
  IA_ZOOMING_MAPPING,
  // patch positions
  IA_MOVING_PATCH,
  IA_ZOOMING_PATCH,
  // light placement
  IA_ZOOMING_LIGHT,
  IA_ROTATING_LIGHT,
  // mip ranging
  IA_MIP_RANGING,
  // context menu
  IA_CONTEXT_MENU,
  // measure vertex
  IA_MOVING_MEASURE_VERTEX,
  IA_ZOOMING_MEASURE_VERTEX,
};

class CModelerView : public CView 
{
protected: // create from serialization only
	CModelerView(); 
	DECLARE_DYNCREATE(CModelerView)

// Attributes
public:
  BOOL m_bViewMeasureVertex;
  FLOAT3D m_vViewMeasureVertex;
  AxisType m_atAxisType;
  TIME m_timeLastTick;
  FLOAT m_fFOW;
  BOOL m_bTileMappingBCG;
  BOOL m_bAnyKeyPressed;
  CPoint m_MousePosition;
  BOOL m_bPrintSurfaceNumbers;
  BOOL m_bFrameRate;
  BOOL m_LightModeOn;
  BOOL m_bCollisionMode;
  BOOL m_bMappingMode;
  BOOL m_ShowAllSurfaces;
	BOOL m_FloorOn;
	BOOL m_bDollyViewer;
	BOOL m_bRenderMappingInSurfaceColors;
	FLOAT m_fDollySpeedMipModeling;
	BOOL m_bDollyMipModeling;
  BOOL m_bDollyLight;
  BOOL m_bDollyLightColor;
  PIX m_offx, m_offy;
  FLOAT m_MagnifyFactor;
  FLOAT m_LightDistance;
  COLOR m_LightColor;
  COLOR m_colAmbientColor;
  CTextureDataInfo *m_ptdiTextureDataInfo;
	COLORREF m_PaperColor;
	COLORREF m_InkColor;
	BOOL m_IsWinBcgTexture;
	CTFileName m_fnBcgTexture;
	CModelRenderPrefs m_RenderPrefs;
	InputAction m_InputAction;
	BOOL m_bOnColorMode;
	BOOL m_IsMappingBcgTexture;
	INDEX m_iChoosedColor;
  INDEX m_iActivePatchBitIndex;
  CPlacement3D m_plLightPlacement;
  CPlacement3D m_plModelPlacement;
	FLOAT m_fTargetDistance;
	FLOAT3D m_vTarget;
	ANGLE3D m_angViewerOrientation;
  CModelObject m_ModelObject;

  CUpdateable m_udViewPicture;

  ModelTextureVertex *m_pmtvClosestVertex;

  INDEX m_iClossestSurface;
  float m_fCurrentMipFactor;
  INDEX m_iCurrentFrame;
	BOOL m_AutoRotating;
  BOOL m_bAutoMipModelingBeforeMapping;
	CPoint m_MouseDownLocation;
  CPoint m_BoxStart;

  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;

// Operations
public:
	void OnIdle(void);
  void ClearBcg( COLOR color, CDrawPort *pDrawPort);
  void RenderView( CDrawPort *pDrawPort);
  static CModelerView *GetActiveView(void);
  static CModelerView *GetActiveMappingView(void);
  static CModelerView *GetActiveMappingNormalView(void);
  void SetProjectionData( CPerspectiveProjection3D &prProjection, CDrawPort *pDP);
  INDEX GetClosestVertex(FLOAT3D &vClosestVertex);
  void ResetViewerPosition(void);
  void MagnifyMapping(CPoint point, FLOAT fMagnification);
  void FastZoomIn( CPoint point);
  void FastZoomOut( void);
  BOOL AssureValidTDI();
	/* gets pointer to MDIFrameWnd main frame of application */
	CMainFrame *GetMainFrame();
	CModelerDoc *GetDocument(void);
  BOOL UpdateAnimations(void);
  
  void FillThumbnailSettings( CThumbnailSettings &tsToReceive);
  void ApplyThumbnailSettings( CThumbnailSettings &tsToApply);
  void SaveThumbnail();

  void RenderAxis( CPerspectiveProjection3D &prProjection, CPlacement3D &pl, FLOAT fSize);
  void DrawArrowAndTypeText(CPerspectiveProjection3D &prProjection,
    const FLOAT3D &v0, const FLOAT3D &v1, COLOR colColor, CTString strText);
  void RenderAxisOfAllAttachments(CPerspectiveProjection3D &prProjection, CPlacement3D &plParent, CModelObject &mo);
  FLOAT GetModelToViewerDistance(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModelerView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CModelerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
public:
	//{{AFX_MSG(CModelerView)
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnAnimPlay();
	afx_msg void OnAnimPrevAnim();
	afx_msg void OnAnimNextAnim();
	afx_msg void OnAnimNextFrame();
	afx_msg void OnAnimPrevFrame();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnUpdateAnimPlay(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimNextframe(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimPrevframe(CCmdUI* pCmdUI);
	afx_msg void OnAnimChoose();
	afx_msg void OnAnimMipPrecize();
	afx_msg void OnAnimMipRough();
	afx_msg void OnUpdateAnimMipPrecize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimMipRough(CCmdUI* pCmdUI);
	afx_msg void OnOptAutoMipModeling();
	afx_msg void OnUpdateOptAutoMipModeling(CCmdUI* pCmdUI);
	afx_msg void OnAnimRotation();
	afx_msg void OnUpdateAnimRotation(CCmdUI* pCmdUI);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnPrefsChangeInk();
	afx_msg void OnPrefsChangePaper();
	afx_msg void OnFileRemoveTexture();
	afx_msg void OnScriptOpen();
	afx_msg void OnScriptUpdateAnimations();
	afx_msg void OnScriptUpdateMipmodels();
	afx_msg void OnLightOn();
	afx_msg void OnLightColor();
	afx_msg void OnUpdateLightOn(CCmdUI* pCmdUI);
	afx_msg void OnRendBboxAll();
	afx_msg void OnRendBboxFrame();
	afx_msg void OnRendWireOnoff();
	afx_msg void OnRendHiddenLines();
	afx_msg void OnRendNoTexture();
	afx_msg void OnRendOnColors();
	afx_msg void OnRendOffColors();
	afx_msg void OnRendSurfaceColors();
	afx_msg void OnRendWhiteTexture();
	afx_msg void OnRendUseTexture();
	afx_msg void OnUpdateLightColor(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMappingOn(CCmdUI* pCmdUI);
	afx_msg void OnUpdateScriptOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateScriptUpdateAnimations(CCmdUI* pCmdUI);
	afx_msg void OnUpdateScriptUpdateMipmodels(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimNextanim(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimPrevanim(CCmdUI* pCmdUI);
	afx_msg void OnUpdateAnimChoose(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileRemoveTexture(CCmdUI* pCmdUI);
	afx_msg void OnMagnifyLess();
	afx_msg void OnMagnifyMore();
	afx_msg void OnWindowFit();
	afx_msg void OnWindowCenter();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnViewAllSurfaces();
	afx_msg void OnBackgroundTexture();
	afx_msg void OnUpdateRendHiddenLines(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendWireOnoff(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendNoTexture(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendWhiteTexture(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendSurfaceColors(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendOnColors(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendOffColors(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendUseTexture(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendBboxAll(CCmdUI* pCmdUI);
	afx_msg void OnUpdateRendBboxFrame(CCmdUI* pCmdUI);
	afx_msg void OnTakeScreenShoot();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnBackgPicture();
	afx_msg void OnBackgColor();
	afx_msg void OnRendFloor();
	afx_msg void OnUpdateRendFloor(CCmdUI* pCmdUI);
	afx_msg void OnStainsInsert();
	afx_msg void OnStainsDelete();
	afx_msg void OnUpdateStainsDelete(CCmdUI* pCmdUI);
	afx_msg void OnStainsPreviousStain();
	afx_msg void OnStainsNextStain();
	afx_msg void OnUpdateStainsInsert(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStainsNextStain(CCmdUI* pCmdUI);
	afx_msg void OnUpdateStainsPreviousStain(CCmdUI* pCmdUI);
	afx_msg void OnShadowWorse();
	afx_msg void OnShadowBetter();
	afx_msg void OnFallDown();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSaveThumbnail();
	afx_msg void OnUpdateSaveThumbnail(CCmdUI* pCmdUI);
	afx_msg void OnRestartAnimations();
	afx_msg void OnFrameRate();
	afx_msg void OnUpdateFrameRate(CCmdUI* pCmdUI);
	afx_msg void OnHeading();
	afx_msg void OnPitch();
	afx_msg void OnBanking();
	afx_msg void OnUpdateCollisionBox(CCmdUI* pCmdUI);
	afx_msg void OnCollisionBox();
	afx_msg void OnResetViewer();
	afx_msg void OnUpdateResetViewer(CCmdUI* pCmdUI);
	afx_msg void OnDollyViewer();
	afx_msg void OnUpdateDollyViewer(CCmdUI* pCmdUI);
	afx_msg void OnDollyLight();
	afx_msg void OnUpdateDollyLight(CCmdUI* pCmdUI);
	afx_msg void OnDollyLightColor();
	afx_msg void OnUpdateDollyLightColor(CCmdUI* pCmdUI);
	afx_msg void OnNextTexture();
	afx_msg void OnUpdateNextTexture(CCmdUI* pCmdUI);
	afx_msg void OnPreviousTexture();
	afx_msg void OnUpdatePreviousTexture(CCmdUI* pCmdUI);
	afx_msg void OnRecreateTexture();
	afx_msg void OnUpdateRecreateTexture(CCmdUI* pCmdUI);
	afx_msg void OnCreateMipModels();
	afx_msg void OnPickVertex();
	afx_msg void OnUpdatePickVertex(CCmdUI* pCmdUI);
	afx_msg void OnDollyMipModeling();
	afx_msg void OnUpdateDollyMipModeling(CCmdUI* pCmdUI);
	afx_msg void OnAnimPlayOnce();
	afx_msg void OnUpdateAnimPlayOnce(CCmdUI* pCmdUI);
	afx_msg void OnTileTexture();
	afx_msg void OnUpdateTileTexture(CCmdUI* pCmdUI);
	afx_msg void OnAddReflectionTexture();
	afx_msg void OnAddSpecular();
	afx_msg void OnRemoveReflection();
	afx_msg void OnUpdateRemoveReflection(CCmdUI* pCmdUI);
	afx_msg void OnRemoveSpecular();
	afx_msg void OnUpdateRemoveSpecular(CCmdUI* pCmdUI);
	afx_msg void OnAddBumpTexture();
	afx_msg void OnRemoveBumpMap();
	afx_msg void OnUpdateRemoveBumpMap(CCmdUI* pCmdUI);
	afx_msg void OnMappingOn();
	afx_msg void OnSkinTexture();
	afx_msg void OnUpdateSkinTexture(CCmdUI* pCmdUI);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSurfaceNumbers();
	afx_msg void OnUpdateSurfaceNumbers(CCmdUI* pCmdUI);
	afx_msg void OnExportSurfaces();
	afx_msg void OnPreviousBcgTexture();
	afx_msg void OnNextBcgTexture();
	afx_msg void OnUpdatePreviousBcgTexture(CCmdUI* pCmdUI);
	afx_msg void OnUpdateNextBcgTexture(CCmdUI* pCmdUI);
	afx_msg void OnWindowTogglemax();
	afx_msg void OnExportForSkining();
	afx_msg void OnRenderSurfacesInColors();
	afx_msg void OnUpdateRenderSurfacesInColors(CCmdUI* pCmdUI);
	afx_msg void OnViewAxis();
	afx_msg void OnUpdateViewAxis(CCmdUI* pCmdUI);
	afx_msg void OnViewInfo();
	afx_msg void OnListAnimations();
	afx_msg void OnUpdateListAnimations(CCmdUI* pCmdUI);
	afx_msg void OnChangeAmbient();
	afx_msg void OnUpdateChangeAmbient(CCmdUI* pCmdUI);
	afx_msg void OnToggleAllSurfaces();
	afx_msg void OnUpdateToggleAllSurfaces(CCmdUI* pCmdUI);
	afx_msg void OnKeyA();
	afx_msg void OnKeyT();
	afx_msg void OnAnimFirst();
	afx_msg void OnUpdateAnimFirst(CCmdUI* pCmdUI);
	afx_msg void OnAnimLast();
	afx_msg void OnUpdateAnimLast(CCmdUI* pCmdUI);
	afx_msg void OnToggleMeasureVtx();
	afx_msg void OnUpdateToggleMeasureVtx(CCmdUI* pCmdUI);
	afx_msg void OnFirstFrame();
	afx_msg void OnLastFrame();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ModelerView.cpp
inline CModelerDoc* CModelerView::GetDocument()
   { return (CModelerDoc*)m_pDocument; }
#endif

#endif // MODELERVIEW_H
/////////////////////////////////////////////////////////////////////////////
