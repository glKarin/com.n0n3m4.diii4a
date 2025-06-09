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

#include "stdafx.h"
#include "SeriousSkaStudio.h"
#include "MainFrm.h"
#include "resource.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LOAD_BAR_STATE( WName, HName, bar, dx, dy)                                      \
	bar.m_Size.cx = (AfxGetApp()->GetProfileInt(_T("General"),_T(WName),dx));             \
	bar.m_Size.cy = (AfxGetApp()->GetProfileInt(_T("General"),_T(HName),dy));             \
  bar.CalcDynamicLayout(0, LM_HORZDOCK)

#define SAVE_BAR_STATE( WName, HName, bar)                                              \
  AfxGetApp()->WriteProfileInt( _T("General"),_T(WName), bar.m_Size.cx);                \
	AfxGetApp()->WriteProfileInt( _T("General"),_T(HName), bar.m_Size.cy)

#define STD_TREEVIEW_WIDTH 230
#define STD_TREEVIEW_HEIGHT 550

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_ACTIVATEAPP()
	ON_COMMAND(ID_VIEW_TREEVIEW, OnViewTreeview)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TREEVIEW, OnUpdateViewTreeview)
	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_COMMAND(ID_VIEW_TOOLBAR, OnViewToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateViewToolbar)
	ON_COMMAND(ID_FILE_CREATE_TEXTURE, OnFileCreateTexture)
	ON_BN_CLICKED(IDC_BT_CLOSE, OnBtClose)
	ON_BN_CLICKED(IDC_BT_CLEAR, OnBtClear)
	ON_COMMAND(ID_VIEW_ERRORLIST, OnViewErrorlist)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ERRORLIST, OnUpdateViewErrorlist)
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
END_MESSAGE_MAP()

static UINT indicators[] =
{
  ID_STATUS_BAR_TEXT,
	ID_SEPARATOR,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

#define STD_DRAGBAR_WIDTH  230
#define STD_DRAGBAR_HEIGHT 230

CMainFrame::CMainFrame()
{
}
CMainFrame::~CMainFrame()
{
}

BOOL _bApplicationActive = TRUE;
void CMainFrame::OnActivateApp(BOOL bActive, DWORD hTask) 
{
  _bApplicationActive = bActive;
	CMDIFrameWnd::OnActivateApp(bActive, hTask);
}

// create main frame
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  // set same styles for use with all toolbars
  CRect rectDummy(0,0,0,0);
  DWORD dwToolBarStyles = TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC | 
                          CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_GRIPPER;

  if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
  // subclass mdiclient
  if (!m_wndMDIClient.SubclassWindow(m_hWndMDIClient)) {
      TRACE ("Failed to subclass MDI client window\n");
      return (-1);
  }                                                       
  // create toolbar IDR_MAINFRAME
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy, IDW_TOOLBAR_MAINFRAME) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}
  // create toolbar IDR_NAVIGATION
  if( (!m_wndNavigationToolBar.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy,IDW_TOOLBAR_NAVIGATION)) ||
		  (!m_wndNavigationToolBar.LoadToolBar(IDR_NAVIGATION)) )
	{
		TRACE0("Failed to create FX toolbar\n");
		return -1;      // fail to create fx tool bar
	}
  // create toolbar IDR_MANAGE
  if (!m_wndToolBarManage.CreateEx(this, TBSTYLE_FLAT, dwToolBarStyles, rectDummy,IDW_TOOLBAR_MANAGE) ||
	!m_wndToolBarManage.LoadToolBar(IDR_MANAGE))
  {
	  TRACE0("Failed to create toolbar\n");
	  return -1;      // fail to create
  }
  // create status bar
  if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

  EnableDocking(CBRS_ALIGN_ANY);
  // enable docking for toolbars
  m_wndToolBar.EnableDocking(CBRS_ALIGN_TOP);
  m_wndNavigationToolBar.EnableDocking(CBRS_ALIGN_TOP);
	m_wndToolBarManage.EnableDocking(CBRS_ALIGN_TOP);
  // dock toolbars
	DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndNavigationToolBar);
	DockControlBar(&m_wndToolBarManage);


  // Set model instance stretch edit ctrl
  m_wndNavigationToolBar.SetButtonInfo(STRETCH_BUTTON_INDEX, ID_MI_STRETCH, TBBS_SEPARATOR, 40);
  CRect rcEditStretch;
	m_wndNavigationToolBar.GetItemRect(STRETCH_BUTTON_INDEX, &rcEditStretch);
  rcEditStretch.top = 2;
  rcEditStretch.right -= 2;
  rcEditStretch.bottom = rcEditStretch.top + 18;
  if (!m_ctrlMIStretch.Create( WS_VISIBLE|WS_BORDER,
    rcEditStretch, &m_wndNavigationToolBar, ID_MI_STRETCH) )
	{
		TRACE0("Failed to create model instance stretch edit control\n");
		return FALSE;
	}

  m_ctrlMIStretch.SetWindowText(L"1");

  // Initialize dialog bar m_dlgBarTreeView
	if (!theApp.m_dlgBarTreeView.Create(this, IDD_TREEBAR,
		CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE | CBRS_SIZE_DYNAMIC,
		ID_VIEW_TREEVIEW))
	{
		TRACE0("Failed to create dialog bar m_dlgBarTreeView\n");
		return -1;		// fail to create
	}

  if(!theApp.m_dlgErrorList.Create(this,IDD_ERROR_LIST,
		CBRS_BOTTOM/* | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE | CBRS_SIZE_DYNAMIC*/,
    ID_VIEW_ERRORLIST)) {
		TRACE0("Failed to create dialog bar m_dlgErrorList\n");
		return -1;		// fail to create
	}
  theApp.GetErrorList()->InsertColumn(0,L"Error");
  theApp.GetErrorList()->SetImageList( &theApp.m_dlgBarTreeView.m_IconsImageList, LVSIL_SMALL);
  // theApp.m_dlgErrorList.m_Size = theApp.m_dlgErrorList.m_sizeDefault;  
  // theApp.m_dlgErrorList.SetSplitterControlID(IDC_SPLITER_LOG_FRAME);


  theApp.m_dlgBarTreeView.SetWindowText(L"Tree view");
  theApp.m_dlgErrorList.SetWindowText(L"Log");

  theApp.m_dlgBarTreeView.EnableDockingSides(CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);
  // theApp.m_dlgErrorList.EnableDockingSides(CBRS_ALIGN_BOTTOM);
  //theApp.m_dlgErrorList.dlg_ulEnabledDockingSides = CBRS_ALIGN_BOTTOM;

  theApp.m_dlgBarTreeView.DockCtrlBar();
  // theApp.m_dlgErrorList.DockCtrlBar();
  // DockControlBar(&theApp.m_dlgErrorList);

  // set size of panels in status bar
  m_wndStatusBar.SetPaneInfo(0,ID_STATUS_BAR_TEXT,SBPS_NORMAL,600);
  m_wndStatusBar.SetPaneInfo(1,ID_SEPARATOR,SBPS_STRETCH,100);

  // try to load dialog width and height from reg.
  LOAD_BAR_STATE("TreeView width", "TreeView height", theApp.m_dlgBarTreeView,
    STD_TREEVIEW_WIDTH, STD_TREEVIEW_HEIGHT);
  // try to load toolbars positions
  LoadBarState(_T("General"));

  theApp.bAppInitialized = TRUE;
  return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
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

// Show/hide treeview
void CMainFrame::OnViewTreeview() 
{
	// OnBarCheck();

  if(theApp.m_dlgBarTreeView.IsWindowVisible() )
  {
    ShowControlBar(&theApp.m_dlgBarTreeView, FALSE, FALSE);
    theApp.m_dlgBarTreeView.SetFocus();
  }
  else
  {
    ShowControlBar(&theApp.m_dlgBarTreeView, TRUE, FALSE);
    theApp.m_dlgBarTreeView.m_TreeCtrl.SetFocus();
  }
}
// Update treeview check
void CMainFrame::OnUpdateViewTreeview(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.m_dlgBarTreeView.IsWindowVisible());
}

// Show/hide error dlg
void CMainFrame::OnViewErrorlist() 
{
  theApp.ShowErrorDlg(!theApp.IsErrorDlgVisible());
}
// Update error dlg
void CMainFrame::OnUpdateViewErrorlist(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.IsErrorDlgVisible());
}

// close main frame
void CMainFrame::OnClose() 
{
  // try to save all documents before closing them
  POSITION pos = theApp.m_pdtDocTemplate->GetFirstDocPosition();
  while (pos!=NULL)
  {
    CSeriousSkaStudioDoc *pmdCurrent = (CSeriousSkaStudioDoc *)theApp.m_pdtDocTemplate->GetNextDoc(pos);
    if(!pmdCurrent->BeforeDocumentClose()) {
       return;
    }
  }

  // save toolbars positions
  SaveBarState(_T("General"));
  // savedialog width and height in reg
  SAVE_BAR_STATE("TreeView width", "TreeView height", theApp.m_dlgBarTreeView);
	CMDIFrameWnd::OnClose();
}
// resize main frame
void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CMDIFrameWnd::OnSize(nType, cx, cy);
  m_wndStatusBar.SetPaneInfo(0,ID_STATUS_BAR_TEXT,SBPS_NORMAL,cx/1.5);  
}

// show/hide toolbars
void CMainFrame::OnViewToolbar() 
{
	BOOL bVisible = ((m_wndToolBar.GetStyle() & WS_VISIBLE) != 0);
  bVisible &= ((m_wndNavigationToolBar.GetStyle() & WS_VISIBLE) != 0);
  bVisible &= ((m_wndToolBarManage.GetStyle() & WS_VISIBLE) != 0);

  ShowControlBar(&m_wndToolBar, !bVisible, FALSE);
  ShowControlBar(&m_wndNavigationToolBar, !bVisible, FALSE);
  ShowControlBar(&m_wndToolBarManage, !bVisible, FALSE);
}
// update toolbars check
void CMainFrame::OnUpdateViewToolbar(CCmdUI* pCmdUI) 
{
	BOOL bVisible = ((m_wndToolBar.GetStyle() & WS_VISIBLE) != 0);
  bVisible &= ((m_wndNavigationToolBar.GetStyle() & WS_VISIBLE) != 0);
  bVisible &= ((m_wndToolBarManage.GetStyle() & WS_VISIBLE) != 0);
	pCmdUI->SetCheck(bVisible);
}

CTString CMainFrame::CreateTexture()
{
  // call create texture dialog
  CTFileName fnCreated = _EngineGUI.CreateTexture();
  return fnCreated;
}

void CMainFrame::OnFileCreateTexture() 
{
  CreateTexture();
}

void CMainFrame::OnBtClose() 
{
  theApp.ShowErrorDlg(FALSE);
}

void CMainFrame::OnBtClear() 
{
  theApp.GetErrorList()->DeleteAllItems();
  OnBtClose();
}
