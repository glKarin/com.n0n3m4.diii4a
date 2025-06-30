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

// SplitterFrame.cpp : implementation file
//

#include "stdafx.h"
#include "seriousskastudio.h"
#include "SplitterFrame.h"
#include "MainFrm.h"
#include "DlgTemplate.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CSplitterFrame

IMPLEMENT_DYNCREATE(CSplitterFrame, CWnd)

#define SET_BAR_SIZE( bar, dx, dy)   \
	bar.m_Size.cx = dx;                 \
	bar.m_Size.cy = dy;                 
  //bar.CalcDynamicLayout(0, LM_HORZDOCK)

CSplitterFrame::CSplitterFrame()
{
  pchCursor = IDC_NO;
  iSplitterSize = 6;
}

CSplitterFrame::~CSplitterFrame()
{
}

BEGIN_MESSAGE_MAP(CSplitterFrame, CWnd)
	//{{AFX_MSG_MAP(CSplitterFrame)
	ON_WM_SETCURSOR()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSplitterFrame message handlers


// Resize splitter
void CSplitterFrame::SetSize(INDEX iWidth,INDEX iHeight)
{
  SetWindowPos(&wndTop,0,0,iWidth,iHeight,SWP_NOMOVE);
}

// Set new position for splitter
void CSplitterFrame::SetAbsPosition(CPoint pt)
{
  CWnd *pParent = GetParent();
  ASSERT(pParent!=NULL);

  pParent->ScreenToClient(&pt);
  SetWindowPos(&wndTop,pt.x,pt.y,0,0,SWP_NOSIZE);
}

// Get position of splitter
CPoint CSplitterFrame::GetAbsPosition()
{
  CRect rc;
  GetWindowRect(&rc);
  return CPoint(rc.left,rc.top);
}

// Change parent of splitter
void CSplitterFrame::ChangeParent(CWnd *pNewParent)
{
  CPoint &ptCurrent = GetAbsPosition();
  SetParent(pNewParent);
  SetAbsPosition(ptCurrent);
}

void CSplitterFrame::EnableDocking()
{
  sp_bDockingEnabled = TRUE;
}


void CSplitterFrame::SetDockingSide(UINT uiDockSide)
{
  CWnd *pParent = GetParent();
  CRect rcParent;
  pParent->GetWindowRect(&rcParent);
  sp_uiDockSide = uiDockSide;

  // is splitter attached on left side
  if(uiDockSide == AFX_IDW_DOCKBAR_LEFT) {
    SetSize(iSplitterSize,rcParent.bottom - rcParent.top);
    SetAbsPosition(CPoint(rcParent.left,rcParent.top));
    pchCursor = IDC_SIZEWE;
    ShowWindow(SW_SHOW);
  // is splitter attached on right side
  } else if(uiDockSide == AFX_IDW_DOCKBAR_RIGHT) {
    SetSize(iSplitterSize,rcParent.bottom - rcParent.top);
    SetAbsPosition(CPoint(rcParent.right - iSplitterSize,rcParent.top));
    pchCursor = IDC_SIZEWE;
    ShowWindow(SW_SHOW);
  // is splitter attached on top side
  } else if(uiDockSide == AFX_IDW_DOCKBAR_TOP) {
    SetSize(rcParent.right - rcParent.left,iSplitterSize);
    SetAbsPosition(CPoint(rcParent.left,rcParent.top));
    pchCursor = IDC_SIZENS;
    ShowWindow(SW_SHOW);
  // is splitter attached on bottom side
  } else if(uiDockSide == AFX_IDW_DOCKBAR_BOTTOM) {
    SetSize(rcParent.right - rcParent.left,iSplitterSize);
    SetAbsPosition(CPoint(rcParent.left,rcParent.bottom-iSplitterSize));
    pchCursor = IDC_SIZENS;
    ShowWindow(SW_SHOW);
  // is splitter floating
  } else if(uiDockSide == AFX_IDW_DOCKBAR_FLOAT) {
    ShowWindow(SW_HIDE);
  } else {
    ASSERT(FALSE);
  }
}

// Resize parent 
void CSplitterFrame::UpdateParent()
{
  CRect rcParent;
  CRect rcMainFrame;
  CRect rcMainClientFrame;
  CPoint pt = GetAbsPosition();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  
  CDlgTemplate &wndParent = *(CDlgTemplate*)GetParent();
  pMainFrame->GetClientRect(&rcMainClientFrame);
  pMainFrame->GetWindowRect(&rcMainFrame);

  pt.x -=rcMainFrame.left;
  pt.y -=rcMainFrame.top;

  wndParent.GetWindowRect(&rcParent);
  wndParent.ShowWindow(SW_HIDE);


  INDEX iWidth = rcParent.right - rcParent.left;
  INDEX iHeight = rcParent.bottom - rcParent.top;

  if(sp_bDockingEnabled) {
    pMainFrame->FloatControlBar(&wndParent,CPoint(rcParent.left,rcParent.top));
    // is splitter attached on left side
    if(sp_uiDockSide == AFX_IDW_DOCKBAR_LEFT) {
      SET_BAR_SIZE(wndParent,rcMainClientFrame.right - pt.x,iHeight);
    // is splitter attached on right side
    } else if(sp_uiDockSide == AFX_IDW_DOCKBAR_RIGHT) {
      SET_BAR_SIZE(wndParent,pt.x,iHeight);
    // is splitter attached on top side
    } else if(sp_uiDockSide == AFX_IDW_DOCKBAR_TOP) {
      SET_BAR_SIZE(wndParent,iWidth,iHeight + sp_ptStartPoint.y - pt.y);
    // is splitter attached on bottom side
    } else if(sp_uiDockSide == AFX_IDW_DOCKBAR_BOTTOM) {
      ASSERT(FALSE);
    }

    // Chose docking side
    INDEX iDockSide = 0;
    // is splitter attached on left side
    if(sp_uiDockSide==AFX_IDW_DOCKBAR_LEFT) {
      iDockSide = AFX_IDW_DOCKBAR_RIGHT;
    // is splitter attached on right side
    } else if(sp_uiDockSide==AFX_IDW_DOCKBAR_RIGHT) {
      iDockSide = AFX_IDW_DOCKBAR_LEFT;
    // is splitter attached on top side
    } else if(sp_uiDockSide==AFX_IDW_DOCKBAR_TOP) {
      iDockSide = AFX_IDW_DOCKBAR_BOTTOM;
    // is splitter attached on bottom side
    } else if(sp_uiDockSide==AFX_IDW_DOCKBAR_BOTTOM) {
      iDockSide = AFX_IDW_DOCKBAR_TOP;
    }
    pMainFrame->DockControlBar(&wndParent,iDockSide);
  } else {
      SET_BAR_SIZE(wndParent,300,300);
    // wndParent.SetWindowPos(&wndBottom,0,0,100,300,SWP_NOZORDER);
    //pMainFrame->FloatControlBar(&wndParent,CPoint(rcParent.left,rcParent.top));
    //pMainFrame->DockControlBar(&wndParent,AFX_IDW_DOCKBAR_BOTTOM);
  }
  


  wndParent.ShowWindow(SW_SHOW);
  wndParent.UpdateWindow();
}


// on left mouse button down
void CSplitterFrame::OnLButtonDown(UINT nFlags, CPoint point) 
{
  INDEX ctParentLevels = 0;
  if(sp_bDockingEnabled) {
    ctParentLevels = 1;
  }
  CRect rc;
  ASSERT(GetParent()!=NULL);
  
  pDockedParent = GetParent();
  pFloatingParent = pDockedParent->GetParent();
  ASSERT(pFloatingParent!=NULL);

  for(INDEX ipar=0;ipar<ctParentLevels;ipar++) {
    pFloatingParent = pFloatingParent->GetParent();
    ASSERT(pFloatingParent!=NULL);
  }

  /*
  CPoint pt = GetAbsPosition();
  pt.x += point.x;
  SetAbsPosition(pt);
  */
  SetCapture();
  sp_ptStartPoint = GetAbsPosition();
  ChangeParent(pFloatingParent);
	CWnd::OnLButtonDown(nFlags, point);
}

// on left mouse button up
void CSplitterFrame::OnLButtonUp(UINT nFlags, CPoint point) 
{
  if(GetCapture()==this) {
    ReleaseCapture();
    ChangeParent(pDockedParent);
    UpdateParent();
    // SetOrientation(uiOrientation);
  }
	CWnd::OnLButtonUp(nFlags, point);
}

// on mouse move
void CSplitterFrame::OnMouseMove(UINT nFlags, CPoint point) 
{
  if(GetCapture()==this) {
    CPoint ptCursor;
    GetCursorPos(&ptCursor);
    CPoint pt = GetAbsPosition();
    if(sp_uiDockSide==AFX_IDW_DOCKBAR_LEFT || sp_uiDockSide==AFX_IDW_DOCKBAR_RIGHT) {
      pt.x = ptCursor.x;
    } else if(sp_uiDockSide==AFX_IDW_DOCKBAR_TOP || sp_uiDockSide==AFX_IDW_DOCKBAR_BOTTOM) {
      pt.y = ptCursor.y;
    }
    SetAbsPosition(pt);
  }
	CWnd::OnMouseMove(nFlags, point);
}


/*
void CSplitterFrame::OnLButtonUp(UINT nFlags, CPoint point) 
{
  if(!theApp.m_dlgBarTreeView.IsFloating())
  {
    SetParent(&theApp.m_dlgBarTreeView);
    ReleaseCapture();
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    // move dlgbar
    CRect rc;
    CRect rcMain;
    pMainFrame->GetWindowRect(rcMain);
    theApp.m_dlgBarTreeView.GetWindowRect(&rc);
    CPoint pt = CPoint(rc.left,rc.top);

    theApp.m_dlgBarTreeView.ShowWindow(SW_HIDE);
    // undock
    INDEX iDockSide = theApp.m_dlgBarTreeView.GetDockingSide();
    pMainFrame->FloatControlBar(&theApp.m_dlgBarTreeView,pt);
    // resize 
    if(iDockSide == AFX_IDW_DOCKBAR_LEFT)
    {
      INDEX iPosXadd = 2+rcMain.right-rcMain.left;
      INDEX iPosX = GetPosX();
      if(iPosX<10) iPosX=10;
      else if(iPosX>iPosXadd-10) iPosX=iPosXadd-10;
      SET_BAR_SIZE(theApp.m_dlgBarTreeView,iPosX,rc.bottom-rc.top);
    }
    else if(iDockSide == AFX_IDW_DOCKBAR_RIGHT)
    {
      INDEX iPosXadd = 2+rcMain.right-rcMain.left;
      INDEX iPosX = GetPosX();
      if(iPosX>iPosXadd-10) iPosX=iPosXadd-10;
      else if(iPosX<10) iPosX=10;

      SET_BAR_SIZE(theApp.m_dlgBarTreeView,iPosXadd-iPosX,rc.bottom-rc.top);
    }
    pMainFrame->DockControlBar(&theApp.m_dlgBarTreeView,iDockSide);
    theApp.m_dlgBarTreeView.ShowWindow(SW_SHOW);
  }

	CWnd::OnLButtonUp(nFlags, point);
}
*/

BOOL CSplitterFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
  ::SetCursor(AfxGetApp()->LoadStandardCursor(pchCursor));
  return TRUE;
  return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CSplitterFrame::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
}
