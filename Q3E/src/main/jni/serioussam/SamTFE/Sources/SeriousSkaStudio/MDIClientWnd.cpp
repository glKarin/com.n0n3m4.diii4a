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

// MDIClientWnd.cpp : implementation file
//

#include "stdafx.h"
#include "seriousskastudio.h"
#include "MDIClientWnd.h"
#include "MainFrm.h"
#include <afxadv.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMDIClientWnd

CMDIClientWnd::CMDIClientWnd()
{
}

CMDIClientWnd::~CMDIClientWnd()
{
}


BEGIN_MESSAGE_MAP(CMDIClientWnd, CWnd)
	//{{AFX_MSG_MAP(CMDIClientWnd)
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMDIClientWnd message handlers
void CMDIClientWnd::GetCurrentRect(CRect &rc)
{
  rc = ClientRect;
}

void CMDIClientWnd::SetCurrentRect(CRect &rc)
{
  ClientRect = rc;
}


void CMDIClientWnd::OnSize(UINT nType, int cx, int cy) 
{
	CPoint pt;
  CRect rc;


  const int iLogRightSpace = 90;

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  SetCurrentRect(CRect(0,0,cx,cy));
  theApp.m_dlgBarTreeView.GetWindowRect(rc);
  pt.x = rc.left;
  pt.y = rc.top;
  CDockState state;

  if(theApp.m_dlgBarTreeView.IsWindowVisible()) {
    if(!theApp.m_dlgBarTreeView.IsFloating()) {
      theApp.m_dlgBarTreeView.ShowWindow(SW_HIDE);
      INDEX iDockSide = theApp.m_dlgBarTreeView.GetDockingSide();
      pMainFrame->FloatControlBar(&theApp.m_dlgBarTreeView,pt);
      pMainFrame->DockControlBar(&theApp.m_dlgBarTreeView,iDockSide);
      theApp.m_dlgBarTreeView.UpdateWindow();
      theApp.m_dlgBarTreeView.ShowWindow(SW_SHOW);
    }
  }
  /*
  if(theApp.m_dlgErrorList.IsWindowVisible()) {
    if(!theApp.m_dlgErrorList.IsFloating()) {
      theApp.m_dlgErrorList.ShowWindow(SW_HIDE);
      INDEX iDockSide = theApp.m_dlgErrorList.GetDockingSide();
      pMainFrame->FloatControlBar(&theApp.m_dlgErrorList,pt);
      pMainFrame->DockControlBar(&theApp.m_dlgErrorList,iDockSide);
      theApp.m_dlgErrorList.UpdateWindow();
      theApp.m_dlgErrorList.ShowWindow(SW_SHOW);
    }
  }*/
  
  // set position of error list
  CListCtrl *plcErrList = (CListCtrl*)theApp.m_dlgErrorList.GetDlgItem(IDC_LC_ERROR_LIST);
  CButton   *pcbClose = (CButton*)theApp.m_dlgErrorList.GetDlgItem(IDC_BT_CLOSE);
  CButton   *pcbClear = (CButton*)theApp.m_dlgErrorList.GetDlgItem(IDC_BT_CLEAR);
  SIZE szLogDlg = theApp.GetLogDlgSize();
  plcErrList->SetWindowPos(&wndTopMost,5,5,szLogDlg.cx - iLogRightSpace,szLogDlg.cy-10,SWP_NOZORDER);
  pcbClose->SetWindowPos(&wndTopMost,cx - 80,15,70,22,SWP_NOZORDER);
  pcbClear->SetWindowPos(&wndTopMost,cx - 80,40,70,22,SWP_NOZORDER);
  plcErrList->SetColumnWidth(0,szLogDlg.cx-iLogRightSpace-25);
  

  CWnd::OnSize(nType, cx, cy);
}

void CMDIClientWnd::OnSizing(UINT fwSide, LPRECT pRect) 
{
	CWnd::OnSizing(fwSide, pRect);
}

void CMDIClientWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
  static CTimerValue tvLast;
  static CPoint ptLast;
  CPoint ptNow;
  GetCursorPos( &ptNow);
  CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
  FLOAT tmDelta = (tvNow-tvLast).GetSeconds();
  if( tmDelta<0.5f && abs(ptNow.x-ptLast.x)<5 && abs(ptNow.y-ptLast.y)<5) {
    theApp.OnFileOpen();
  }
  tvLast=tvNow;
  ptLast = ptNow;
	CWnd::OnLButtonDown(nFlags, point);
}
