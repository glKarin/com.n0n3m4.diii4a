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

// ChildFrm.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef CHILDFRAME_H
#define CHILDFRAME_H 1

class CChildFrame : public CMDIChildWnd
{ 
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
public:
  BOOL m_bDisableVisibilityTweaks;
  BOOL m_bShowVisibilityTweaks;
  BOOL m_bTestGameOn;
  CPlacement3D wo_plStored01;
  CPlacement3D wo_plStored02;
  CPlacement3D wo_plStored03;
  CPlacement3D wo_plStored04;
  FLOAT wo_fStored01;
  FLOAT wo_fStored02;
  FLOAT wo_fStored03;
  FLOAT wo_fStored04;
  BOOL m_bShowTargets;
  BOOL m_bShowEntityNames;

	LONG m_OldStyleEx;
	LONG m_OldStyle;
  BOOL m_bShadowsVisible;
  BOOL m_bShadowsCalculate;
  BOOL m_bViewFromEntity;
  BOOL m_bSelectionVisible;  // if selection should be visible
  BOOL m_bLastAutoMipBrushingOn;
  BOOL m_bAutoMipBrushingOn;
  FLOAT m_fManualMipBrushingFactor;
  BOOL m_bAncoredMovingAllowed;
  UINT m_iAnchoredResetTimerID;
  BOOL m_bSceneRenderingTime;
  BOOL m_bInfoVisible;
  BOOL m_bRenderViewPictures;
  CMasterViewer m_mvViewer;      // master viewer for all views inside this frame
  // grid on/off flag
  BOOL m_bGridOn;
  // selected configuration
  INDEX m_iSelectedConfiguration;

  CWorldRenderPrefs::ShadowsType m_stShadowType;

// Operations
public:
  // called from view
  void KeyPressed(UINT nChar, UINT nRepCnt, UINT nFlags);
  void RememberChildConfiguration( INDEX iViewConfiguration);
  void SetChildConfiguration( INDEX iViewConfiguration);
  void ApplySettingsFromPerspectiveView( CWorldEditorView *pwedView, INDEX iViewConfiguration);
  void DeleteViewsExcept( CWnd *pwndViewToLeave);
  inline INDEX CChildFrame::GetHSplitters(void) {	return m_wndSplitter.GetColumnCount();}
  inline INDEX CChildFrame::GetVSplitters(void) {	return m_wndSplitter.GetRowCount();}
  CWorldEditorView *GetPerspectiveView(void);
  void TestGame( BOOL bFullScreen);
  void SetAdjusters(float ratio);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow = -1);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
public:
	//{{AFX_MSG(CChildFrame)
	afx_msg void OnGridOnOff();
	afx_msg void OnUpdateGridOnOff(CCmdUI* pCmdUI);
	afx_msg void OnTestGameWindowed();
	afx_msg void OnTestGameFullScreen();
	afx_msg void OnUpdateTestGame(CCmdUI* pCmdUI);
	afx_msg void OnRenderTargets();
	afx_msg void OnUpdateRenderTargets(CCmdUI* pCmdUI);
	afx_msg void OnMoveAnchored();
	afx_msg void OnUpdateMoveAnchored(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSceneRenderingTime();
	afx_msg void OnUpdateSceneRenderingTime(CCmdUI* pCmdUI);
	afx_msg void OnAutoMipLeveling();
	afx_msg void OnUpdateAutoMipLeveling(CCmdUI* pCmdUI);
	afx_msg void OnWindowClose();
	afx_msg void OnViewSelection();
	afx_msg void OnUpdateViewSelection(CCmdUI* pCmdUI);
	afx_msg void OnMaximizeView();
	afx_msg void OnToggleViewPictures();
	afx_msg void OnUpdateToggleViewPictures(CCmdUI* pCmdUI);
	afx_msg void OnViewFromEntity();
	afx_msg void OnUpdateViewFromEntity(CCmdUI* pCmdUI);
	afx_msg void OnViewShadowsOnoff();
	afx_msg void OnCalculateShadowsOnoff();
	afx_msg void OnUpdateViewShadowsOnoff(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCalculateShadowsOnoff(CCmdUI* pCmdUI);
	afx_msg void OnStorePosition01();
	afx_msg void OnStorePosition02();
	afx_msg void OnRestorePosition01();
	afx_msg void OnRestorePosition02();
	afx_msg void OnToggleEntityNames();
	afx_msg void OnUpdateToggleEntityNames(CCmdUI* pCmdUI);
	afx_msg void OnToggleVisibilityTweaks();
	afx_msg void OnUpdateToggleVisibilityTweaks(CCmdUI* pCmdUI);
	afx_msg void OnEnableVisibilityTweaks();
	afx_msg void OnUpdateEnableVisibilityTweaks(CCmdUI* pCmdUI);
	afx_msg void OnRestorePosition03();
	afx_msg void OnRestorePosition04();
	afx_msg void OnStorePosition03();
	afx_msg void OnStorePosition04();
	afx_msg void OnKeyB();
	afx_msg void OnUpdateKeyB(CCmdUI* pCmdUI);
	afx_msg void OnKeyG();
	afx_msg void OnUpdateKeyG(CCmdUI* pCmdUI);
	afx_msg void OnKeyY();
	afx_msg void OnUpdateKeyY(CCmdUI* pCmdUI);
	afx_msg void OnKeyCtrlG();
	afx_msg void OnUpdateKeyCtrlG(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // CHILDFRAME_H
