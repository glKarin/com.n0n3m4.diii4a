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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

ENGINE_API extern INDEX snd_iFormat;


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

extern UINT APIENTRY ModelerFileRequesterHook( HWND hdlg, UINT uiMsg, WPARAM wParam,	LPARAM lParam);

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_ANIMCONTROL, OnViewAnimControl)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ANIMCONTROL, OnUpdateViewAnimControl)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INFO, OnUpdateShowInfo)
	ON_COMMAND(ID_VIEW_INFO, OnViewInfo)
	ON_COMMAND(ID_VIEW_COLOR_PALETTE, OnViewColorPalette)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COLOR_PALETTE, OnUpdateViewColorPalette)
	ON_COMMAND(ID_FILE_CREATE_TEXTURE, OnFileCreateTexture)
	ON_COMMAND(ID_VIEW_TEXTURECONTROL, OnViewTexturecontrol)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TEXTURECONTROL, OnUpdateViewTexturecontrol)
	ON_COMMAND(ID_VIEW_MIPLIGHTCONTROL, OnViewMiplightcontrol)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MIPLIGHTCONTROL, OnUpdateViewMiplightcontrol)
	ON_COMMAND(ID_VIEW_SCRIPTCONTROL, OnViewScriptcontrol)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SCRIPTCONTROL, OnUpdateViewScriptcontrol)
	ON_COMMAND(ID_VIEW_RENDERCONTROL, OnViewRendercontrol)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RENDERCONTROL, OnUpdateViewRendercontrol)
	ON_WM_CANCELMODE()
	ON_WM_INITMENU()
	ON_COMMAND(ID_VIEW_STAINSCONTROL, OnViewStainscontrol)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STAINSCONTROL, OnUpdateViewStainscontrol)
	ON_COMMAND(ID_STAINS_ADD, OnStainsAdd)
	ON_COMMAND(ID_STAINS_REMOVE, OnStainsRemove)
	ON_COMMAND(ID_VIEW_PATCHES_PALETTE, OnViewPatchesPalette)
	ON_UPDATE_COMMAND_UI(ID_STAINS_REMOVE, OnUpdateStainsRemove)
	ON_COMMAND(ID_VIEW_ROTATE, OnViewRotate)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ROTATE, OnUpdateViewRotate)
	ON_COMMAND(ID_TOGGLE_ALL_BARS, OnToggleAllBars)
	ON_COMMAND(ID_VIEW_MAPPING, OnViewMapping)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MAPPING, OnUpdateViewMapping)
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(ID_EDIT_SPECULAR, OnEditSpecular)
	ON_COMMAND(ID_CREATE_REFLECTION_TEXTURE, OnCreateReflectionTexture)
	ON_COMMAND(ID_HELP_FINDER, OnHelpFinder)
	ON_COMMAND(ID_VIEW_FXCONTROL, OnViewFxcontrol)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FXCONTROL, OnUpdateViewFxcontrol)
	ON_COMMAND(ID_WINDOW_TOGGLEMAX, OnWindowTogglemax)
	ON_COMMAND(ID_TESSELLATE_LESS, OnTessellateLess)
	ON_COMMAND(ID_TESSELLATE_MORE, OnTessellateMore)
	//}}AFX_MSG_MAP

  // Global help commands - modified to use html-help
	//ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
	ON_COMMAND(ID_HELP, OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_SEPARATOR,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// sets message for modeler's "new progress dialog"
void SetProgressMessage( char *strMessage)
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_NewProgress.m_strNewMessage = CTString( strMessage);
  pMainFrame->m_NewProgress.UpdateData( FALSE);
}

// sets range of modeler's "new progress dialog"
void SetProgressRange( INDEX iProgresSteps)
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_NewProgress.m_NewProgressLine.SetRange( 0, (short)iProgresSteps);
}

// sets current modeler's "new progress dialog" state
void SetProgressState( INDEX iCurrentStep)
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_NewProgress.m_NewProgressLine.SetPos( iCurrentStep);
}
/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
  m_pInfoFrame = NULL;
  m_dlgPaletteDialog = NULL;
  m_dlgPatchesPalette = NULL;
  ProgresRoutines.SetProgressMessage = SetProgressMessage;
  ProgresRoutines.SetProgressRange = SetProgressRange;
  ProgresRoutines.SetProgressState = SetProgressState;
}

CMainFrame::~CMainFrame()
{
  // if exists, destroy palette dialog
  if( m_dlgPaletteDialog != NULL)
    delete( m_dlgPaletteDialog);
	if(	m_dlgPatchesPalette != NULL)
    delete m_dlgPatchesPalette;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	const int nDropHeight = 100;

  // set same styles for use with all toolbars
  DWORD dwToolBarStyles = WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC |
			CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_GRIPPER;
  CRect rectDummy(0,0,0,0);

	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
  if( (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_MAIN)) ||
		  (!m_wndToolBar.LoadToolBar(IDR_MAINFRAME)) )
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create main tool bar
	}

	if( (!m_wndStatusBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_BOTTOM, IDW_STATUS_BAR)) ||
  		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)) )
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

  if( (!m_AnimToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_ANIMATION)) ||
		  (!m_AnimToolBar.LoadToolBar(IDR_ANIMCONTROL)) )
	{
		TRACE0("Failed to create animation toolbar\n");
		return -1;      // fail to create animation tool bar
	}
	
  if( (!m_TextureToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_TEXTURE)) ||
		  (!m_TextureToolBar.LoadToolBar(IDR_TEXTURECONTROL)) )
	{
		TRACE0("Failed to create texture toolbar\n");
		return -1;      // fail to create texture tool bar
	}

  if( (!m_FXToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_FX)) ||
		  (!m_FXToolBar.LoadToolBar(IDR_FX_CONTROL)) )
	{
		TRACE0("Failed to create FX toolbar\n");
		return -1;      // fail to create fx tool bar
	}

  if( (!m_StainsToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_STAINS)) ||
		  (!m_StainsToolBar.LoadToolBar(IDR_STAINSCONTROL)) )
	{
		TRACE0("Failed to create stains toolbar\n");
		return -1;      // fail to create stains tool bar
	}

  if( (!m_ScriptToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_SCRIPT)) ||
		  (!m_ScriptToolBar.LoadToolBar(IDR_SCRIPTCONTROL)) )
	{
		TRACE0("Failed to create script toolbar\n");
		return -1;      // fail to create script tool bar
	}

  if( (!m_MipAndLightToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_MIP_AND_LIGHT)) ||
		  (!m_MipAndLightToolBar.LoadToolBar(IDR_MIPANDLIGHT)) )
	{
		TRACE0("Failed to create mip and light toolbar\n");
		return -1;      // fail to create mip and light tool bar
	}

  if( (!m_RenderControlBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_RENDER)) ||
		  (!m_RenderControlBar.LoadToolBar(IDR_RENDERCONTROL)) )
	{
		TRACE0("Failed to create render control toolbar\n");
		return -1;      // fail to create render control tool bar
	}
  static UINT aidRenderControlBar[15] =
  {
    ID_REND_BBOX_FRAME,
    ID_REND_BBOX_ALL,
    ID_REND_WIRE_ONOFF,
    ID_REND_HIDDEN_LINES,
    ID_REND_FLOOR,
    ID_REND_NO_TEXTURE,
    ID_REND_WHITE_TEXTURE,
    ID_REND_SURFACE_COLORS,
    ID_REND_ON_COLORS,
    ID_REND_OFF_COLORS,
    ID_REND_USE_TEXTURE,
    ID_SHADOW_WORSE,
    ID_SHADOW_BETTER,
    ID_TOGGLE_MEASURE_VTX,
    ID_VIEW_AXIS,
  };
  m_RenderControlBar.SetButtons( aidRenderControlBar, 15);

  if( (!m_RotateToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_ROTATION)) ||
		  (!m_RotateToolBar.LoadToolBar(IDR_ROTATE)) )
	{
		TRACE0("Failed to create rotate surface control toolbar\n");
		return -1;      // fail to create rotate control tool bar
	}
  
  if( (!m_MappingToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_MAPPING)) ||
		  (!m_MappingToolBar.LoadToolBar(IDR_MAPPING)) )
	{
		TRACE0("Failed to create mapping control toolbar\n");
		return -1;      // fail to create mapping control tool bar
	}
  
  // Set z-speed edit ctrl
  m_AnimToolBar.SetButtonInfo(0, ID_Z_SPEED, TBBS_SEPARATOR, 40);
  CRect rectEditSpeed;
	m_AnimToolBar.GetItemRect(0, &rectEditSpeed);
  rectEditSpeed.top = 2;
  rectEditSpeed.right -= 2;
  rectEditSpeed.bottom = rectEditSpeed.top + 18;
  if (!m_ctrlZSpeed.Create( WS_VISIBLE|WS_BORDER,
    rectEditSpeed, &m_AnimToolBar, ID_Z_SPEED) )
	{
		TRACE0("Failed to create model speed edit control\n");
		return FALSE;
	}
  m_ctrlZSpeed.SetWindowText(L"0");
  
  // Set z-loop edit ctrl
  m_AnimToolBar.SetButtonInfo(1, ID_Z_LOOP_TIMES, TBBS_SEPARATOR, 25);
  CRect rectEditLoop;
	m_AnimToolBar.GetItemRect(1, &rectEditLoop);
  rectEditLoop.top = 2;
  rectEditLoop.bottom = rectEditLoop.top + 18;
  if (!m_ctrlZLoop.Create( WS_VISIBLE|WS_BORDER,
    rectEditLoop, &m_AnimToolBar, ID_Z_LOOP_TIMES) )
	{
		TRACE0("Failed to create model loop edit control\n");
		return FALSE;
	}
  m_ctrlZLoop.SetWindowText(L"4");

  m_AnimToolBar.SetButtonInfo(6, ID_ANIM_CHOOSE, TBBS_SEPARATOR, 150);
  m_TextureToolBar.SetButtonInfo(2, ID_TEXTURE_CHOOSE, TBBS_SEPARATOR, 150);
  m_StainsToolBar.SetButtonInfo(3, ID_STAINS_CHOOSE, TBBS_SEPARATOR, 150);

  CRect rect;
	m_AnimToolBar.GetItemRect(6, &rect);
	rect.top = 2;
	rect.bottom = rect.top + nDropHeight;
  if (!m_AnimComboBox.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL,
			rect, &m_AnimToolBar, ID_ANIM_CHOOSE))
	{
		TRACE0("Failed to create animation combo-box\n");
		return FALSE;
	}
	
  m_TextureToolBar.GetItemRect(2, &rect);
	rect.top = 2;
	rect.bottom = rect.top + nDropHeight;
  if (!m_SkinComboBox.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL,
			rect, &m_TextureToolBar, ID_TEXTURE_CHOOSE))
	{
		TRACE0("Failed to create texture combo-box\n");
		return FALSE;
	}

  m_StainsToolBar.GetItemRect(3, &rect);
	rect.top = 2;
	rect.bottom = rect.top + nDropHeight;
  if (!m_StainsComboBox.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL,
			rect, &m_StainsToolBar, ID_STAINS_CHOOSE))
	{
		TRACE0("Failed to create stains combo-box\n");
		return FALSE;
	}

  m_AnimComboBox.AddString( L"None available");
  m_AnimComboBox.SetCurSel( 0);
	
  m_SkinComboBox.AddString( L"None available");
  m_SkinComboBox.SetCurSel( 0);
	
  m_StainsComboBox.AddString( L"None available");
  m_StainsComboBox.SetCurSel( 0);
	
  //  Create a font for the comboboxes
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));

	if (!::GetSystemMetrics(SM_DBCSENABLED))
	{
		logFont.lfHeight = -11;
		logFont.lfWeight = FW_REGULAR;
		logFont.lfPitchAndFamily = FF_ROMAN;
    logFont.lfOrientation = 10;
    logFont.lfQuality = PROOF_QUALITY;
    logFont.lfItalic = TRUE;
		
    CString strDefaultFont;
		strDefaultFont.LoadString(IDS_DEFAULT_ARIAL);
		lstrcpy(logFont.lfFaceName, strDefaultFont);
    
    if( !m_Font.CreateFontIndirect(&logFont))
			TRACE0("Could Not create font for combo\n");
		else
    {
      m_ctrlZSpeed.SetFont(&m_Font);
      m_ctrlZLoop.SetFont(&m_Font);
      m_AnimComboBox.SetFont(&m_Font);
      m_SkinComboBox.SetFont(&m_Font);
      m_StainsComboBox.SetFont(&m_Font);
    }
	}
	else
	{
    m_Font.Attach(::GetStockObject(SYSTEM_FONT));
    m_ctrlZSpeed.SetFont(&m_Font);
    m_ctrlZLoop.SetFont(&m_Font);
    m_AnimComboBox.SetFont(&m_Font);
    m_SkinComboBox.SetFont(&m_Font);
    m_StainsComboBox.SetFont(&m_Font);
	}

  // create pane for closest surface
  UINT nID;
  UINT nStyle;
  int cxWidth;
  m_wndStatusBar.GetPaneInfo( CLOSEST_SURFACE_PANE, nID, nStyle, cxWidth);
  cxWidth = 160;
  m_wndStatusBar.SetPaneInfo( CLOSEST_SURFACE_PANE, nID, nStyle, cxWidth);

	EnableDocking(CBRS_ALIGN_ANY);

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);

  m_AnimToolBar.SetBarStyle(m_AnimToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_AnimToolBar.EnableDocking(CBRS_ALIGN_ANY);

  m_TextureToolBar.SetBarStyle(m_TextureToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_TextureToolBar.EnableDocking(CBRS_ALIGN_ANY);

  m_FXToolBar.SetBarStyle(m_FXToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_FXToolBar.EnableDocking(CBRS_ALIGN_ANY);

  m_StainsToolBar.SetBarStyle(m_StainsToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_StainsToolBar.EnableDocking(CBRS_ALIGN_ANY);

  m_ScriptToolBar.SetBarStyle(m_ScriptToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_ScriptToolBar.EnableDocking(CBRS_ALIGN_ANY);

  m_MipAndLightToolBar.SetBarStyle(m_MipAndLightToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_MipAndLightToolBar.EnableDocking(CBRS_ALIGN_ANY);
  
  m_RenderControlBar.SetBarStyle(m_RenderControlBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_RenderControlBar.EnableDocking(CBRS_ALIGN_ANY);
  
  m_RotateToolBar.SetBarStyle(m_RotateToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_RotateToolBar.EnableDocking(CBRS_ALIGN_ANY);
  
  m_MappingToolBar.SetBarStyle(m_MappingToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_MappingToolBar.EnableDocking(CBRS_ALIGN_ANY);
  
	RecalcLayout();
	DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP);
	DockControlBarRelativeTo(&m_AnimToolBar, &m_wndToolBar, 1, 0);
	DockControlBarRelativeTo(&m_TextureToolBar,&m_FXToolBar, 1, 0);
	DockControlBarRelativeTo(&m_FXToolBar,&m_AnimToolBar, 1, 0);

  DockControlBarRelativeTo(&m_ScriptToolBar,&m_wndToolBar, 0, 15);
	DockControlBarRelativeTo(&m_MipAndLightToolBar,&m_ScriptToolBar, 1, 0);
  DockControlBarRelativeTo(&m_RotateToolBar, &m_MipAndLightToolBar, 1, 0);
  DockControlBarRelativeTo(&m_MappingToolBar, &m_RotateToolBar, 1, 0);
	DockControlBarRelativeTo(&m_StainsToolBar,&m_MappingToolBar, 1, 0);
	
  DockControlBar(&m_RenderControlBar, AFX_IDW_DOCKBAR_LEFT);
  DockControlBarRelativeTo(&m_MappingToolBar, &m_RenderControlBar, 0, 1);

	LoadBarState(_T("General"));
  return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;
  //cs.style |= WS_MAXIMIZE;
  
  return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnClose() 
{
	SaveBarState(_T("General"));
	CMDIFrameWnd::OnClose();
}

void CMainFrame::OnViewAnimControl()
{
	BOOL bVisible = ((m_AnimToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_AnimToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewAnimControl(CCmdUI* pCmdUI) 
{
	BOOL bVisible = ((m_AnimToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnWindowTogglemax() 
{
	BOOL bIfMaximized;
  CChildFrame *pActiveWnd = (CChildFrame *) MDIGetActive( &bIfMaximized);
  if( bIfMaximized)
  {
    MDIRestore( pActiveWnd);
    if( !pActiveWnd->m_bAllreadyUnmaximized)
    {
      pActiveWnd->MoveWindow( 0, 0, 256, 256);
      RECT rectClient;
      pActiveWnd->GetClientRect( &rectClient);
      PIX pixRightWidth = 256+256-rectClient.right+4;
      PIX pixRightHeight = 256+256-rectClient.bottom+4;
      pActiveWnd->MoveWindow( 0, 0, pixRightWidth, pixRightHeight);
      pActiveWnd->m_bAllreadyUnmaximized = TRUE;
    }
  }
  else
  {
    MDIMaximize( pActiveWnd);
  }
  theApp.m_chGlobal.MarkChanged();
} 

void CMainFrame::HideModelessInfoSheet()
{
	ASSERT(m_pInfoFrame != NULL);
  m_pInfoFrame->ShowWindow(SW_HIDE);
}

void CMainFrame::OnUpdateShowInfo(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( TRUE);
	pCmdUI->SetCheck(!(m_pInfoFrame == NULL || !m_pInfoFrame->IsWindowVisible()));
}

void CMainFrame::OnViewInfo() 
{
  ToggleInfoWindow();
}

void CMainFrame::OnViewColorPalette() 
{
  if( m_dlgPaletteDialog == NULL) // if doesn't exist, call create
  {
    m_dlgPaletteDialog = new CPaletteDialog();
    ASSERT( m_dlgPaletteDialog != NULL);
    if( !m_dlgPaletteDialog->Create(IDD_COLORS_PALETTE)) return;
    
    CRect rectMainFrame, rectPalPos, rectNewPos;
    
    m_dlgPaletteDialog->GetWindowRect( rectPalPos);
    GetClientRect( rectMainFrame);
    rectNewPos.left = rectMainFrame.right - rectPalPos.Width() - 3;
    rectNewPos.right = rectNewPos.left + rectPalPos.Width();
    if( m_pInfoFrame == NULL)
    {
      rectNewPos.top = TOOLS_INIT_TOP;
    }
    else
    {
      CRect rectInfo;
      m_pInfoFrame->GetWindowRect( rectInfo);
      rectNewPos.top = TOOLS_INIT_TOP + rectInfo.Height() + 1;
    }
    rectNewPos.bottom = rectNewPos.top + rectPalPos.Height();
    m_dlgPaletteDialog->MoveWindow( rectNewPos, TRUE);
  }
  
 	BOOL bVisible = ((m_dlgPaletteDialog->GetStyle() & WS_VISIBLE) != 0);
  if( !bVisible)          // if dialog isn't visible, show it
  {
    m_dlgPaletteDialog->ShowWindow(SW_SHOW);
  }
  else                    // otherwise, hide it
  {
    m_dlgPaletteDialog->ShowWindow(SW_HIDE);
  }
}

void CMainFrame::OnUpdateViewColorPalette(CCmdUI* pCmdUI) 
{
 	BOOL bVisible;
  
  if( m_dlgPaletteDialog == NULL)
  {
    bVisible = FALSE;
  }
  else
  {
    bVisible = (m_dlgPaletteDialog->GetStyle() & WS_VISIBLE) != 0;
  }
  pCmdUI->SetCheck( bVisible);	
}

BOOL CMainFrame::OnIdle(LONG lCount)
{
  // Call OnIdle for animation combo box
  m_AnimComboBox.OnIdle( lCount);
  // Call OnIdle for texture combo box
  m_SkinComboBox.OnIdle( lCount);

  // Call OnIdle for color palette dialog
  if( m_dlgPaletteDialog != NULL)
  {
    m_dlgPaletteDialog->OnIdle( lCount);
  }
  
  // Call OnIdle for info property sheet
  if( m_pInfoFrame != NULL)
  {
    m_pInfoFrame->m_pInfoSheet->OnIdle( lCount);
  }
  
  // Call OnIdle for patches dialog
  if( m_dlgPatchesPalette != NULL)
  {
    m_dlgPatchesPalette->OnIdle( lCount);
  }
  
  return TRUE;
}


void CMainFrame::OnFileCreateTexture() 
{
  CModelerView *pView = (CModelerView *) CModelerView::GetActiveView();
  CModelerDoc *pDoc = NULL;
  if( pView != NULL)
  {
    pDoc = pView->GetDocument();
    // setup create texture directory
    theApp.WriteProfileString(L"Scape", CString(KEY_NAME_CREATE_TEXTURE_DIR), 
      CString(_fnmApplicationPath+pDoc->GetModelDirectory()));
  }
  // call create texture dialog
  CTFileName fnCreated = _EngineGUI.CreateTexture();
  if( (fnCreated != "") && pDoc != NULL)
  {
    CTextureDataInfo *pNewTDI;
    try
    {
      pNewTDI = pDoc->m_emEditModel.AddTexture_t( fnCreated, 
                pDoc->m_emEditModel.GetWidth(),
                pDoc->m_emEditModel.GetHeight() );
    }
    catch( char *err_str)
    {
      AfxMessageBox( CString(err_str));
      pNewTDI = NULL;
    }
    if( pNewTDI != NULL)
    {
      pDoc->SetModifiedFlag();
      pView->m_ptdiTextureDataInfo = pNewTDI;
      // switch to texture mode
      pView->OnRendUseTexture();
    }
  }
}

void CMainFrame::DockControlBarRelativeTo(CToolBar* Bar,CToolBar* LeftOf, int dx, int dy)
{
	CRect rect;
	DWORD dw;
	UINT n;

	// get MFC to adjust the dimensions of all docked ToolBars
	// so that GetWindowRect will be accurate
	RecalcLayout();
	LeftOf->GetWindowRect(&rect);
	rect.OffsetRect(dx, dy);
	dw=LeftOf->GetBarStyle();
	n = 0;
	n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
	n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
	n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;

	// When we take the default parameters on rect, DockControlBar will dock
	// each Toolbar on a seperate line.  By calculating a rectangle, we in effect
	// are simulating a Toolbar being dragged to that location and docked.
	DockControlBar(Bar,n,&rect);
}

void CMainFrame::OnViewTexturecontrol() 
{
	BOOL bVisible = ((m_TextureToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_TextureToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewTexturecontrol(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_TextureToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewFxcontrol() 
{
	BOOL bVisible = ((m_FXToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_FXToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewFxcontrol(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_FXToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewStainscontrol() 
{
	BOOL bVisible = ((m_StainsToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_StainsToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewStainscontrol(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_StainsToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewMiplightcontrol() 
{
  BOOL bVisible = ((m_MipAndLightToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_MipAndLightToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewMiplightcontrol(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_MipAndLightToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnViewScriptcontrol() 
{
  BOOL bVisible = ((m_ScriptToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_ScriptToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewScriptcontrol(CCmdUI* pCmdUI) 
{
{
  BOOL bVisible = ((m_ScriptToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}
}

void CMainFrame::OnViewRendercontrol() 
{
  BOOL bVisible = ((m_RenderControlBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_RenderControlBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewRendercontrol(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_RenderControlBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

static BOOL _bSoundEnabled = FALSE;
void CMainFrame::EnableSound(void) 
{
  if( _bSoundEnabled || !theApp.m_Preferences.ap_bAllowSoundLock) return;
  _bSoundEnabled = TRUE;
  snd_iFormat = Clamp( snd_iFormat, (INDEX)CSoundLibrary::SF_NONE, (INDEX)CSoundLibrary::SF_44100_16);  
  _pSound->SetFormat( (enum CSoundLibrary::SoundFormat)snd_iFormat);
}

void CMainFrame::DisableSound(void) 
{
  //if( !_bSoundEnabled) return;
  _bSoundEnabled = FALSE;
  _pSound->SetFormat( CSoundLibrary::SF_NONE);
}

void CMainFrame::OnCancelMode() 
{
  DisableSound();
  CMDIFrameWnd::OnCancelMode();
}

void CMainFrame::OnInitMenu(CMenu* pMenu) 
{
	CMDIFrameWnd::OnInitMenu(pMenu);
}

void CMainFrame::OnStainsAdd() 
{
  // call file requester for opening documents
  CDynamicArray<CTFileName> afnWorkingStains;
  _EngineGUI.FileRequester( "Choose textures to add", FILTER_TEX FILTER_END,
    "Working textures directory", "Textures\\", "", &afnWorkingStains);
  // insert selected textures
  FOREACHINDYNAMICARRAY( afnWorkingStains, CTFileName, itStain)
  {
    // add new working texture
    theApp.AddModelerWorkingPatch( itStain.Current());
  }
  m_StainsComboBox.Refresh();
}

void CMainFrame::OnStainsRemove() 
{
  CWorkingPatch *pWP = NULL;
  int iSelected = m_StainsComboBox.GetCurSel();

  if( iSelected != CB_ERR)
  {
    INDEX iCt = 0;
    FOREACHINLIST( CWorkingPatch, wp_ListNode, theApp.m_WorkingPatches, it)
    {
      if( iCt == iSelected)
      {
        pWP = &it.Current();
      }
      iCt ++;
    }
  }
  ASSERT(pWP != NULL);

  pWP->wp_ListNode.Remove();
  _pTextureStock->Release( pWP->wp_TextureData);
  delete pWP;
  m_StainsComboBox.Refresh();
}

void CMainFrame::OnUpdateStainsRemove(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( !theApp.m_WorkingPatches.IsEmpty());
}

void CMainFrame::OnViewPatchesPalette() 
{
  if( m_dlgPatchesPalette == NULL) // if doesn't exist, call create
  {
    m_dlgPatchesPalette = new CPatchPalette();
    ASSERT( m_dlgPatchesPalette != NULL);
    if( !m_dlgPatchesPalette->Create(IDD_PATCH_PALETTE)) return;
    
    CRect rectMainFrame, rectPalPos, rectNewPos;
    
    m_dlgPatchesPalette->GetWindowRect( rectPalPos);
    GetClientRect( rectMainFrame);
    rectNewPos.left = rectMainFrame.right - rectPalPos.Width() - 3;
    rectNewPos.right = rectNewPos.left + rectPalPos.Width();
    rectNewPos.top = TOOLS_INIT_TOP;
    if( m_pInfoFrame != NULL)
    {
      CRect rectInfo;
      m_pInfoFrame->GetWindowRect( rectInfo);
      rectNewPos.top += rectInfo.Height() + 1;
    }
    if( m_dlgPaletteDialog != NULL)
    {
      CRect rectInfo;
      m_dlgPaletteDialog->GetWindowRect( rectInfo);
      rectNewPos.top += rectInfo.Height() + 1;
    }

    rectNewPos.bottom = rectNewPos.top + rectPalPos.Height();
    m_dlgPatchesPalette->MoveWindow( rectNewPos, TRUE);
  }
  
 	BOOL bVisible = ((m_dlgPatchesPalette->GetStyle() & WS_VISIBLE) != 0);
  if( !bVisible)          // if dialog isn't visible, show it
  {
    m_dlgPatchesPalette->ShowWindow(SW_SHOW);
  }
  else                    // otherwise, hide it
  {
    m_dlgPatchesPalette->ShowWindow(SW_HIDE);
  }
}

void CMainFrame::OnViewRotate() 
{
  BOOL bVisible = ((m_RotateToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_RotateToolBar, !bVisible, FALSE);
	RecalcLayout();
}

void CMainFrame::OnUpdateViewRotate(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_RotateToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

void CMainFrame::OnToggleAllBars() 
{
	ShowControlBar(&m_wndStatusBar, (m_wndStatusBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_wndToolBar, (m_wndToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_AnimToolBar, (m_AnimToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_TextureToolBar, (m_TextureToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_StainsToolBar, (m_StainsToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_ScriptToolBar, (m_ScriptToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_MipAndLightToolBar, (m_MipAndLightToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_RenderControlBar, (m_RenderControlBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_RotateToolBar, (m_RotateToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_MappingToolBar, (m_MappingToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
	ShowControlBar(&m_FXToolBar, (m_FXToolBar.GetStyle() & WS_VISIBLE) == 0, FALSE);
}

void CMainFrame::OnViewMapping() 
{
  BOOL bVisible = ((m_MappingToolBar.GetStyle() & WS_VISIBLE) != 0);

	ShowControlBar(&m_MappingToolBar, !bVisible, FALSE);
	RecalcLayout();
}


void CMainFrame::OnUpdateViewMapping(CCmdUI* pCmdUI) 
{
  BOOL bVisible = ((m_MappingToolBar.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

BOOL _bApplicationActive = TRUE;
void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask) 
{
  _bApplicationActive = bActive;
	CMDIFrameWnd::OnActivateApp(bActive, hTask);
  if (!bActive) {
    DisableSound();
  }
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
  BOOL bAltPressed = (GetKeyState( VK_MENU)&0x8000) != 0;
  // alt is pressed
  BOOL bAlt = FALSE;
  
  if( pMsg->message==WM_LBUTTONDOWN)
  {
    BOOL bHasDocument = FALSE;
    // check for models
    POSITION posMdl = theApp.m_pdtModelDocTemplate->GetFirstDocPosition();
    while (posMdl!=NULL) {
      CModelerDoc *pdocCurrent = (CModelerDoc *)theApp.m_pdtModelDocTemplate->GetNextDoc(posMdl);
      bHasDocument = pdocCurrent!=NULL;
    }
    if( !bHasDocument)
    {
      // check for scripts
      POSITION posScr = theApp.m_pdtScriptTemplate->GetFirstDocPosition();
      while (posScr!=NULL) {
        CModelerDoc *pdocCurrent = (CModelerDoc *)theApp.m_pdtScriptTemplate->GetNextDoc(posScr);
        bHasDocument = pdocCurrent!=NULL;
      }
    }

    BOOL bMainFrameHasFocus = (this == CWnd::GetForegroundWindow());
    if( !bHasDocument && bMainFrameHasFocus)
    {
      static CTimerValue tvLast;
      static CPoint ptLast;
      CPoint ptNow;
      GetCursorPos( &ptNow);
      CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
      FLOAT tmDelta = (tvNow-tvLast).GetSeconds();
      if( tmDelta<0.5f && abs(ptNow.x-ptLast.x)<5 && abs(ptNow.y-ptLast.y)<5)
      {
        theApp.OnFileOpen();
      }
      tvLast=tvNow;
      ptLast = ptNow;
    }
  }

  // if we caught alt key message
  if( pMsg->message==WM_SYSKEYDOWN)
  {
    // get key data
    int lKeyData = pMsg->lParam;
    // test if it is ghost Alt-F4 situation
    if( lKeyData & (1L<<29))
    {
      // Alt key was really pressed
      bAlt = TRUE;
    }
  }

  if( (pMsg->message==WM_KEYDOWN) || bAlt)
  {
    int iVirtKey = (int) pMsg->wParam;
    int lKeyData = pMsg->lParam;
    
    if( DYNAMIC_DOWNCAST(CScriptView, GetActiveFrame()->GetActiveView())==NULL)
    {
      if( (iVirtKey == 'Q') && !bAltPressed)
      {
        ToggleInfoWindow();
      }
    }
  }
	return CMDIFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::ToggleInfoWindow(void)
{
  if( m_pInfoFrame == NULL)
  {
	  // create frame for holding info sheet
	  m_pInfoFrame = new CDlgInfoFrame;
    // set initial size of rect window
    CRect rectInfoWindow(0, 0, 0, 0);
    if( !m_pInfoFrame->Create( NULL, L"Tools info",
        MFS_SYNCACTIVE|WS_POPUP|WS_CAPTION|WS_SYSMENU, rectInfoWindow, this))
	  {
		  TRACE0("Failed to create Tools info window!\n");
      return;
	  }
  }

  if (m_pInfoFrame != NULL)
  {
    if (!m_pInfoFrame->IsWindowVisible())
    {
      m_pInfoFrame->ShowWindow(SW_SHOW);
      m_pInfoFrame->m_pInfoSheet->SetFocus();
      CModelerView *pModelerView = CModelerView::GetActiveView();
      if( (pModelerView != NULL) && (pModelerView->m_bCollisionMode) )
      {
        m_pInfoFrame->m_pInfoSheet->CustomSetActivePage( 
          &m_pInfoFrame->m_pInfoSheet->m_PgInfoCollision);
      }
    }
    else
    {
      m_pInfoFrame->ShowWindow(SW_HIDE);
    }
  }
} 

void CMainFrame::OnEditSpecular() 
{
  CDlgCreateSpecularTexture dlgEditSpecular;
  dlgEditSpecular.DoModal();
}

void CMainFrame::OnCreateReflectionTexture() 
{
  CDlgCreateReflectionTexture dlgEditReflection;
  dlgEditReflection.DoModal();
}


LRESULT CMainFrame::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  if( message==WM_SYSCOMMAND)
  {
    switch( wParam & ~0x0F)
    {
    case SC_SCREENSAVE:
    case SC_MONITORPOWER:
      return 0;
    }
  }
	
	return CMDIFrameWnd::DefWindowProc(message, wParam, lParam);
}

void CMainFrame::OnHelpFinder() 
{
  HtmlHelp(NULL); // Hmmm...
  /*HtmlHelp(NULL, 
    _fnmApplicationPath+"Help\\ToolsHelp.chm::/SeriousModeler/Overview.htm", 
    HH_DISPLAY_TOPIC, NULL);*/
}

void CMainFrame::OnTessellateLess() 
{
  _pShell->SetINDEX( "gap_bForceTruform", 1); // make sure truform is enabled for all models
  INDEX iTruform = _pShell->GetINDEX( "gap_iTruformLevel");
  iTruform = Clamp( iTruform-1L, 0L, _pGfx->gl_iMaxTessellationLevel);
  _pShell->SetINDEX( "gap_iTruformLevel", iTruform);
  CTString strVar;
  strVar.PrintF( "Tessellation level = %d", iTruform);
  m_wndStatusBar.SetPaneText( STATUS_LINE_PANE, CString(strVar));
}

void CMainFrame::OnTessellateMore() 
{
  _pShell->SetINDEX( "gap_bForceTruform", 1); // make sure truform is enabled for all models
  INDEX iTruform = _pShell->GetINDEX( "gap_iTruformLevel");
  iTruform = Clamp( iTruform+1L, 0L, _pGfx->gl_iMaxTessellationLevel);
  _pShell->SetINDEX( "gap_iTruformLevel", iTruform);
  CTString strVar;
  strVar.PrintF( "Tessellation level = %d", iTruform);
  m_wndStatusBar.SetPaneText( STATUS_LINE_PANE, CString(strVar));
}
