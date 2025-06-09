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
#ifndef MAINFRM_H
#define MAINFRM_H 1

#define CLOSEST_SURFACE_PANE 0
#define ACTIVE_SURFACE_PANE 1

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:  // control bar embedded members
  WINDOWPLACEMENT m_OldPlacement;
	LONG m_OldStyleEx;
	LONG m_OldStyle;

  // edit control for editing z translation speed
  CEdit m_ctrlZSpeed;
  // edit control that controls how many times will animation be played before looping
  CEdit m_ctrlZLoop;

	void DockControlBarRelativeTo(CToolBar* Bar,CToolBar* LeftOf, int dx, int dy);
  void EnableSound(void);
  void DisableSound(void);
  void ToggleInfoWindow(void);

  // main menu
  HMENU m_hMenu;
  // bars
  BOOL m_bwndStatusBar;
  BOOL m_bwndToolBar;
	CStatusBar m_wndStatusBar;
	CToolBar   m_wndToolBar;

public:
  CDlgNewProgress m_NewProgress;
	CFont m_Font;
  CAnimComboBox m_AnimComboBox;
	CTextureComboBox m_SkinComboBox;
	CStainsComboBox m_StainsComboBox;
	// Color palette dialog
  BOOL OnIdle(LONG lCount);
  BOOL           m_bColorsPalette;
	CPaletteDialog *m_dlgPaletteDialog;

  // Property sheet - info dialog
  CDlgInfoFrame *m_pInfoFrame;
  BOOL m_bInfoVisible;
	void HideModelessInfoSheet();

  // Patches palette dialog
  CPatchPalette *m_dlgPatchesPalette;
  BOOL          m_bPatchesPalette;

  // Anim controll bar
  CToolBar m_AnimToolBar;
  CToolBar m_TextureToolBar;
  CToolBar m_FXToolBar;
  CToolBar m_StainsToolBar;
  CToolBar m_ScriptToolBar;
  CToolBar m_MipAndLightToolBar;
  CToolBar m_RenderControlBar;
  CToolBar m_RotateToolBar;
  CToolBar m_MappingToolBar;

	CMainFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
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
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnViewAnimControl();
	afx_msg void OnUpdateViewAnimControl(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShowInfo(CCmdUI* pCmdUI);
	afx_msg void OnViewInfo();
	afx_msg void OnViewColorPalette();
	afx_msg void OnUpdateViewColorPalette(CCmdUI* pCmdUI);
	afx_msg void OnFileCreateTexture();
	afx_msg void OnViewTexturecontrol();
	afx_msg void OnUpdateViewTexturecontrol(CCmdUI* pCmdUI);
	afx_msg void OnViewMiplightcontrol();
	afx_msg void OnUpdateViewMiplightcontrol(CCmdUI* pCmdUI);
	afx_msg void OnViewScriptcontrol();
	afx_msg void OnUpdateViewScriptcontrol(CCmdUI* pCmdUI);
	afx_msg void OnViewRendercontrol();
	afx_msg void OnUpdateViewRendercontrol(CCmdUI* pCmdUI);
	afx_msg void OnCancelMode();
	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnViewStainscontrol();
	afx_msg void OnUpdateViewStainscontrol(CCmdUI* pCmdUI);
	afx_msg void OnStainsAdd();
	afx_msg void OnStainsRemove();
	afx_msg void OnViewPatchesPalette();
	afx_msg void OnUpdateStainsRemove(CCmdUI* pCmdUI);
	afx_msg void OnViewRotate();
	afx_msg void OnUpdateViewRotate(CCmdUI* pCmdUI);
	afx_msg void OnToggleAllBars();
	afx_msg void OnViewMapping();
	afx_msg void OnUpdateViewMapping(CCmdUI* pCmdUI);
	afx_msg void OnActivateApp(BOOL bActive, DWORD hTask);
	afx_msg void OnEditSpecular();
	afx_msg void OnCreateReflectionTexture();
	afx_msg void OnHelpFinder();
	afx_msg void OnViewFxcontrol();
	afx_msg void OnUpdateViewFxcontrol(CCmdUI* pCmdUI);
	afx_msg void OnTessellateLess();
	afx_msg void OnTessellateMore();
	//}}AFX_MSG
	
public:
	afx_msg void OnWindowTogglemax();
  afx_msg void OnColorFromPallete(UINT nID);
  afx_msg void OnUpdateColorFromPallete(CCmdUI* pCmdUI);
  
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // MAINFRM_H
