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

// ToolTipWnd.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "ToolTipWnd.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolTipWnd

CStaticArray<PIX> _saPixLineHeights;
CToolTipWnd::CToolTipWnd()
{
}

CToolTipWnd::~CToolTipWnd()
{
  _saPixLineHeights.Clear();
}


BEGIN_MESSAGE_MAP(CToolTipWnd, CWnd)
	//{{AFX_MSG_MAP(CToolTipWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CToolTipWnd message handlers

CTString CToolTipWnd::GetLine(INDEX iLine)
{
  INDEX ctLines=0;

  const char *pText = (const char *) m_strText;
  while( *pText != 0)
  {
    if( iLine == ctLines)
    {
      char achrLine[1024];
      INDEX iChar = 0;
      while( (*pText != '\n') && (*pText != 0) )
      {
        achrLine[iChar] = *pText;
        iChar++;
        pText++;
      }
      achrLine[iChar] = 0;
      return CTString( achrLine);
    }

    if( *pText == '\n')
    {
      ctLines++;
    }
    pText++;
  }
  ctLines++;
  return CTString("Line not found !!!");
}

INDEX CToolTipWnd::GetLinesCount( void)
{
  INDEX ctLines=0;
  const char *pText = (const char *) m_strText;
  while( *pText != 0)
  {
    if( *pText == '\n')
    {
      ctLines++;
      if( *(pText+1) == 0) return ctLines;
    }
    pText++;
  }
  return ctLines+1;
}

void CToolTipWnd::ObtainTextSize(PIX &pixMaxWidth, PIX &pixMaxHeight) 
{
  CDC *pDC = GetDC();
  if( pDC == NULL) return;

  pixMaxWidth = 0;
  _saPixLineHeights.Clear();
  PIX pixStartY = 0;
  INDEX ctLines = GetLinesCount();
  _saPixLineHeights.New( ctLines);
  for(INDEX iLine = 0; iLine<ctLines; iLine++)
  {
    CTString strLine = GetLine(iLine);
    CSize size = pDC->GetOutputTextExtent( CString(strLine));
    if( size.cx>pixMaxWidth)  pixMaxWidth = size.cx;
    _saPixLineHeights[iLine] = pixStartY;
    pixStartY += size.cy;
  }
  pixMaxHeight = pixStartY;
  ReleaseDC( pDC);
}

void CToolTipWnd::OnPaint() 
{
	CPaintDC dc(this);
  
  CRect rectWindow;
  GetClientRect(rectWindow);
  DWORD colPaper = GetSysColor( COLOR_INFOBK);
  DWORD colInk = GetSysColor( COLOR_INFOTEXT);
  dc.FillSolidRect( rectWindow, colPaper);
  dc.SetTextColor( colInk);
  
  CDC *pDC = GetDC();
  if (pDC == NULL) return;
  pDC->SelectObject( &theApp.m_FixedFont);
  
  INDEX ctLines = GetLinesCount();
  for(INDEX iLine = 0; iLine<ctLines; iLine++)
  {
    CTString strLine = GetLine(iLine);
    dc.TextOut( 0, _saPixLineHeights[iLine], CString(strLine));
  }
  ReleaseDC( pDC);
}

int CToolTipWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
  SetupWindowSizeAndPosition();

  if( !m_bManualControl)
  {
    SetTimer( 0, 10, NULL);
  }
  return 0;
}

void CToolTipWnd::SetupWindowSizeAndPosition(void)
{
  CDC *pDC = GetDC();
  pDC->SelectObject( &theApp.m_FixedFont);

  PIX pixWidth, pixHeight;
  ObtainTextSize( pixWidth, pixHeight);
  pixWidth+=2;
  pixHeight+=2;
  
  GetCursorPos( &m_ptMouse); 
	int iCursorX = 12;
	int iCursorY = 18;
  CRect rectWindow;
  rectWindow.left = m_ptMouse.x+iCursorX;
  rectWindow.top = m_ptMouse.y+iCursorY;

  if( m_bManualControl)
  {
    rectWindow.left = m_pixManualX;
    rectWindow.top = m_pixManualY;
  }

  rectWindow.right = rectWindow.left + pixWidth;
  rectWindow.bottom = rectWindow.top + pixHeight;
  
	int iEdgeX	 = ::GetSystemMetrics(SM_CXEDGE);		// window edge width
	int iEdgeY   = ::GetSystemMetrics(SM_CYEDGE);
	PIX pixScreenX = ::GetSystemMetrics(SM_CXMAXIMIZED)-4*iEdgeX;	// screen size
	PIX pixScreenY = ::GetSystemMetrics(SM_CYMAXIMIZED)-4*iEdgeX;
  if(rectWindow.bottom > pixScreenY)
  {
    rectWindow.top -= rectWindow.bottom-pixScreenY;
  }
  if(rectWindow.right > pixScreenX)
  {
    rectWindow.left -= rectWindow.right-pixScreenX;
  }
  MoveWindow( rectWindow);
	
  ReleaseDC( pDC);
}

void CToolTipWnd::OnSetFocus(CWnd* pOldWnd) 
{
  if ( pOldWnd!=NULL && IsWindow(pOldWnd->m_hWnd))
  {
	  pOldWnd->SetFocus();
  }
}

void CToolTipWnd::ManualUpdate(void) 
{
  SetupWindowSizeAndPosition();
  Invalidate( FALSE);
}

void CToolTipWnd::ManualOff(void)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( pMainFrame->m_pwndToolTip != NULL)
  {
    delete pMainFrame->m_pwndToolTip;
    pMainFrame->m_pwndToolTip = NULL;
  }
}

void CToolTipWnd::OnTimer(UINT_PTR nIDEvent)
{
  ASSERT( m_bManualControl == FALSE);
  if( nIDEvent == 0)
  {
    POINT ptMouse;
    GetCursorPos( &ptMouse); 
    if( ((Abs(ptMouse.x-m_ptMouse.x)) > 2) ||
        ((Abs(ptMouse.y-m_ptMouse.y)) > 2) )
    {
      KillTimer( 0);
      CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
      delete pMainFrame->m_pwndToolTip;
      pMainFrame->m_pwndToolTip = NULL;
    }
  }
}
