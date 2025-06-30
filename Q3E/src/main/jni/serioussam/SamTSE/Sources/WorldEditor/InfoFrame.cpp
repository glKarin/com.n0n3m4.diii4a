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

// InfoFrame.cpp : implementation file
//

#include "stdafx.h"
#include "InfoFrame.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoFrame

IMPLEMENT_DYNCREATE(CInfoFrame, CMiniFrameWnd)

CInfoFrame::CInfoFrame()
{
	m_pInfoSheet = NULL;
}

CInfoFrame::~CInfoFrame()
{
  // there is no need to destroy or delete sheet object m_pInfoSheet because
  // it is created as child window and will be automatically deleted trough its
  // PostNcDestroy() and DestroyWindow() member functions. (second one will be called
  // by its parent and will end with "delete this;"
}


BEGIN_MESSAGE_MAP(CInfoFrame, CMiniFrameWnd)
	//{{AFX_MSG_MAP(CInfoFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInfoFrame message handlers


int CInfoFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMiniFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  // create sheet object to hold numerous pages
  m_pInfoSheet = new CInfoSheet(this);
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

  // rectangle to hold mainframe height
  CRect rectMainFrame;
  // get main frame rectangle
  pMainFrame->GetWindowRect( rectMainFrame);

  PIX pixStatusHeight = rectStatusBar.Height();
  PIX pixInfoHeight = rectWindow.Height();
  PIX pixMainFrameHeight = rectMainFrame.Height();

  // offset mini frame window coordinates so it fits in lower left part of the screen
  //rectWindow.OffsetRect( 3, iScreenHeight - pixInfoHeight - pixStatusHeight - 3);
  rectWindow.OffsetRect( 0, pixMainFrameHeight - pixInfoHeight - pixStatusHeight - 9);
  
	// move frame window to new position
  SetWindowPos( NULL, rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
		            SWP_NOZORDER | SWP_NOACTIVATE);
  // set property sheet position and type
  m_pInfoSheet->SetWindowPos( NULL, 0, 0, rectClient.Width(), rectClient.Height(),
		                          SWP_NOZORDER | SWP_NOACTIVATE);
  return 0;
}

void CInfoFrame::OnClose() 
{
	// Instead of closing the modeless property sheet, just hide it.
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	pMainFrame->m_pInfoFrame->ShowWindow(SW_HIDE);
}
