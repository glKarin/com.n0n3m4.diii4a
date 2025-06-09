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

// DlgFrameSheet.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoFrame

CDlgInfoFrame::CDlgInfoFrame()
{
	m_pInfoSheet = NULL;
}

CDlgInfoFrame::~CDlgInfoFrame()
{
  // there is no need to destroy or delete sheet object m_pInfoSheet because
  // it is created as child window and will be automatically deleted trough its
  // PostNcDestroy() and DestroyWindow() member functions. (second one will be called
  // by its parent and will end with "delete this;"
}

BEGIN_MESSAGE_MAP(CDlgInfoFrame, CMiniFrameWnd)
	//{{AFX_MSG_MAP(CDlgInfoFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoFrame message handlers

int CDlgInfoFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  if (CMiniFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  
  // create sheet object to hold numerous pages
  m_pInfoSheet = new CDlgInfoSheet(this);
  // if creation fails, delete allocated object
  if (!m_pInfoSheet->Create(this, WS_CHILD | WS_VISIBLE, 0))
	{
		delete m_pInfoSheet;
		m_pInfoSheet = NULL;
		return -1;
	}
	
	// resize the mini frame so that it fits around the child property sheet
	CRect rectClient, rectWindow;
  // property sheet's window rectangle
  m_pInfoSheet->GetWindowRect(rectClient);
  // becomes mini frame's client rectangle
	rectWindow = rectClient;
	// add the width and height needed from the mini frame's borders
	CalcWindowRect(rectWindow);
  // get screen size
  int iScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
  int iScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
  // rectangle to hold status bar size
  CRect rectStatusBar;
  // get status bar rectangle
  pMainFrame->m_wndStatusBar.GetWindowRect( rectStatusBar);
  // offset mini frame window coordinates so it fits in lower left part of the screen
  rectWindow.OffsetRect( 3, iScreenHeight - rectWindow.Height() - rectStatusBar.Height() - 3);
	// move frame window to new position
  SetWindowPos( NULL, rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
		            SWP_NOZORDER | SWP_NOACTIVATE);
  // set property sheet position and type
  m_pInfoSheet->SetWindowPos( NULL, 0, 0, rectClient.Width(), rectClient.Height(),
		                          SWP_NOZORDER | SWP_NOACTIVATE);
  return 0;
}

void CDlgInfoFrame::OnClose() 
{
	// Instead of closing the modeless property sheet, just hide it.
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	pMainFrame->HideModelessInfoSheet();
}

BOOL CDlgInfoFrame::PreTranslateMessage(MSG* pMsg) 
{
  if( pMsg->message == WM_SYSCOMMAND)
  {
    if( (pMsg->wParam == SC_MINIMIZE) || ( pMsg->wParam == SC_MAXIMIZE))
      pMsg->wParam = SC_RESTORE;
  }
	return CMiniFrameWnd::PreTranslateMessage(pMsg);
}

void CDlgInfoFrame::OnSize(UINT nType, int cx, int cy) 
{
  CMiniFrameWnd::OnSize(nType, cx, cy);
  SetSizes();
}

void CDlgInfoFrame::SetSizes() 
{
  return;
  // first we calculate current page's bounding rectangle
  CPropertyPage* pPage = m_pInfoSheet->GetActivePage();
  ASSERT( pPage != NULL);
  
  // we will obtain dialog template structure and use it to extract width and height
  // of document template
  DLGTEMPLATE *pdlgTemplate;
  char strResourceID[ 10];
  // create string containing string "#xxx" where xxx is resource (dialog) ID
  sprintf( strResourceID, "#%d", pPage->m_psp.pszTemplate);
  HRSRC hrsrc = FindResource(NULL, CString(strResourceID), RT_DIALOG); 
  HGLOBAL hglb = LoadResource(NULL, hrsrc); 
  pdlgTemplate = (DLGTEMPLATE *) LockResource(hglb);
  // get base units needed to calculate right width and height
  DWORD dwDlgBase = GetDialogBaseUnits();
  m_PageWidth = pdlgTemplate->cx * LOWORD(dwDlgBase) / 4;
  m_PageHeight = pdlgTemplate->cy * HIWORD(dwDlgBase) / 8;
  CRect rectPage = CRect( 0, 0, m_PageWidth, m_PageHeight);

  // we will set tab control's size so it fits arround active property page
  CRect rectTabCtrl;
  // get tab control object
  CTabCtrl *pTabCtrl = m_pInfoSheet->GetTabControl();
  ASSERT( pTabCtrl != NULL);
  pTabCtrl->MoveWindow( &rectPage);
  m_pInfoSheet->MoveWindow( &rectPage);

  // now we will resize frame so that it fits around the property sheet.
	CRect rectFrame;
  // get sheet's size into frame's rectangle
  rectFrame = rectPage;
  // add border and title bar sizes to gain bounding's frame dimensions
	CalcWindowRect( rectFrame);
	// convert coodinates from client into screen
  ClientToScreen( rectFrame);
  // set new frame position
  MoveWindow( rectFrame);

  // resize page
  #define PAGE_OFFSET_X 2
  #define PAGE_OFFSET_Y 25
  #define PAGE_SUB_WIDTH 2
  #define PAGE_SUB_HEIGHT 2
  rectPage = CRect( PAGE_OFFSET_X, PAGE_OFFSET_Y,
                    m_PageWidth-PAGE_SUB_WIDTH, m_PageHeight-PAGE_SUB_HEIGHT);
  pPage->MoveWindow( rectPage);
}
