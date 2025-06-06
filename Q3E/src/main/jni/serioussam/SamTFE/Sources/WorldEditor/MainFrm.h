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

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef MAINFRAME_H
#define MAINFRAME_H 1

#define STATUS_LINE_PANE 0
#define EDITING_MODE_ICON_PANE 1
#define EDITING_MODE_PANE 2
#define GRID_PANE 3
#define POSITION_PANE 4

#define DOCK_UP 0
#define DOCK_DOWN 1
#define DOCK_LEFT 2
#define DOCK_RIGHT 3

// get pressed key buffer (keys 0-9)
extern INDEX TestKeyBuffers(void);

// macro used for writing text into status line
#define STATUS_LINE_MESASGE( message) if(theApp.m_bShowStatusInfo){\
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd()); \
  ASSERT( pMainFrame != NULL); \
  pMainFrame->m_wndStatusBar.SetPaneText( STATUS_LINE_PANE, message, TRUE);};

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	// old window position and styles before full screen mode
	
  WINDOWPLACEMENT m_OldPlacement;
	LONG m_OldStyleEx;
	LONG m_OldStyle;

  // main menu
  HMENU m_hMenu;
  // Tool bars
  CStatusBar m_wndStatusBar;
	CToolBar m_wndToolBar;
	CToolBar m_wndWorkTools;
  CToolBar m_wndCSGTools;
  CToolBar m_wndMipTools;
  CToolBar m_wndProjections;
  CToolBar m_wndSettingsAndUtility;
  CToolBar m_wndShadowsAndTexture;
  CToolBar m_wndSelectEntity; 
  CToolBar m_wndViewTools;
  CToolBar m_wndViewTools2;
  
  // CSG destination combo box
  CCSGDesitnationCombo m_CSGDesitnationCombo;
  // Triangularisation type combo box
  CTriangularisationCombo m_TriangularisationCombo;
  // Edit control for editing mip switch distance
  CEditMipSwitchDistance m_ctrlEditMipSwitchDistance;

  // Tool dialogs
  CBrowser m_Browser;
  CPropertyComboBar m_PropertyComboBar;
  // mini frame used to hold property sheet with dialog pages
  CInfoFrame *m_pInfoFrame;
  CTFileName m_fnLastVirtualTree;
  // color palette
  CColorPaletteWnd *m_pColorPalette;
  CToolTipWnd *m_pwndToolTip;
// Operations
public:
  void DockControlBarRelativeTo(CControlBar* Bar,CControlBar* LeftOf,
                                ULONG ulDockDirection = DOCK_RIGHT);
  BOOL OnIdle(LONG lCount);
  // toggle info window
  void ToggleInfoWindow();
  // shows info window
  void ShowInfoWindow();
  // reset info window pos
  void ResetInfoWindowPos();
  // hides info window
  void HideInfoWindow();
  // startes application
  void StartApplication( CTString strApplicationToRun);
  // store or restore given virtual shortcut
  void ApplyTreeShortcut( INDEX iVDirBuffer, BOOL bCtrl);
  // call custom color picker at given coordinates
  void CustomColorPicker( PIX pixX, PIX pixY);
  void ManualToolTipOn( PIX pixManualX, PIX pixManualY);
  void ManualToolTipUpdate( void);
  void SetStatusBarMessage( CTString strMessage, INDEX iPane, FLOAT fTime);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
public:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnVirtualTree();
	afx_msg void OnClose();
	afx_msg void OnCancelMode();
	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnViewInfowindow();
	afx_msg void OnUpdateViewInfowindow(CCmdUI* pCmdUI);
	afx_msg void OnViewCsgtools();
	afx_msg void OnUpdateViewCsgtools(CCmdUI* pCmdUI);
	afx_msg void OnViewProjectionsBar();
	afx_msg void OnUpdateViewProjectionsBar(CCmdUI* pCmdUI);
	afx_msg void OnViewWorkBar();
	afx_msg void OnUpdateViewWorkBar(CCmdUI* pCmdUI);
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	afx_msg void OnCreateTexture();
	afx_msg void OnCallModeler();
	afx_msg void OnCallTexmaker();
	afx_msg void OnViewSettingsAndUtilityBar();
	afx_msg void OnUpdateViewSettingsAndUtilityBar(CCmdUI* pCmdUI);
	afx_msg void OnViewShadowsAndTextureBar();
	afx_msg void OnUpdateViewShadowsAndTextureBar(CCmdUI* pCmdUI);
	afx_msg void OnViewSelectEntityBar();
	afx_msg void OnUpdateViewSelectEntityBar(CCmdUI* pCmdUI);
	afx_msg void OnViewViewToolsBar();
	afx_msg void OnUpdateViewViewToolsBar(CCmdUI* pCmdUI);
	afx_msg void OnViewViewToolsBar2();
	afx_msg void OnUpdateViewViewToolsBar2(CCmdUI* pCmdUI);
	afx_msg void OnGameAudio();
	afx_msg void OnGameVideo();
	afx_msg void OnGamePlayer();
	afx_msg void OnGameSelectPlayer();
	afx_msg void OnShowTreeShortcuts();
	afx_msg void OnMenuShortcut01();
	afx_msg void OnMenuShortcut02();
	afx_msg void OnMenuShortcut03();
	afx_msg void OnMenuShortcut04();
	afx_msg void OnMenuShortcut05();
	afx_msg void OnMenuShortcut06();
	afx_msg void OnMenuShortcut07();
	afx_msg void OnMenuShortcut08();
	afx_msg void OnMenuShortcut09();
	afx_msg void OnMenuShortcut10();
	afx_msg void OnStoreMenuShortcut01();
	afx_msg void OnStoreMenuShortcut02();
	afx_msg void OnStoreMenuShortcut03();
	afx_msg void OnStoreMenuShortcut04();
	afx_msg void OnStoreMenuShortcut05();
	afx_msg void OnStoreMenuShortcut06();
	afx_msg void OnStoreMenuShortcut07();
	afx_msg void OnStoreMenuShortcut08();
	afx_msg void OnStoreMenuShortcut09();
	afx_msg void OnStoreMenuShortcut10();
	afx_msg void OnConsole();
	afx_msg void OnViewMipToolsBar();
	afx_msg void OnUpdateViewMipToolsBar(CCmdUI* pCmdUI);
	afx_msg void OnToolRecreateTexture();
	afx_msg void OnRecreateCurrentTexture();
	afx_msg void OnLightAnimation();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnHelpFinder();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // MAINFRAME_H
