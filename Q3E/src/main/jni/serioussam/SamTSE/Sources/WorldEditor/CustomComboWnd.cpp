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

// CustomComboWnd.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "CustomComboWnd.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SPACING_H PIX(4)
#define SPACING_V PIX(4)
/*
#define COLOR_UNDER_MOUSE (C_GRAY|CT_OPAQUE)
#define COLOR_SELECTED (C_vdRED|CT_OPAQUE)
#define COLOR_SELECTED_UNDER_MOUSE (C_RED|CT_OPAQUE)
*/

#define COLOR_UNDER_MOUSE (C_RED|CT_OPAQUE)
#define COLOR_SELECTED (C_mdGRAY|CT_OPAQUE)
#define COLOR_SELECTED_UNDER_MOUSE (C_RED|CT_OPAQUE)

/////////////////////////////////////////////////////////////////////////////
// CCustomComboWnd

CCustomComboWnd::CCustomComboWnd()
{
  m_pfResult=NULL;
  m_pOnSelect=NULL;
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  // mark that timer is not yet started
  m_iTimerID = -1;
}

PIX GetFixedTextWidth( const CTString &strText, CFontData *pfd)
{
  return strText.Length()*pfd->fd_pixCharWidth;
}

PIX GetFixedTextHeight( CFontData *pfd)
{
  return pfd->fd_pixCharHeight;
}

void CCustomComboWnd::GetComboLineSize(PIX &pixMaxWidth, PIX &pixMaxHeight)
{
  pixMaxWidth=0;
  pixMaxHeight=GetFixedTextHeight(_pfdConsoleFont)+SPACING_V;
  FOREACHINDYNAMICCONTAINER(m_dcComboLines, CComboLine, itcl)
  {
    CComboLine &cl=*itcl;
    PIX pixIconW=cl.cl_boxIcon.Max()(1)-cl.cl_boxIcon.Min()(1);
    PIX pixIconH=cl.cl_boxIcon.Max()(2)-cl.cl_boxIcon.Min()(2);
    PIX pixLineWidth=GetFixedTextWidth(cl.cl_strText, _pfdConsoleFont)+pixIconW;
    pixMaxWidth=Max(pixMaxWidth,pixLineWidth+SPACING_H);
    pixMaxHeight=Max(pixMaxHeight,pixIconH+SPACING_V);
  }
}

CCustomComboWnd::~CCustomComboWnd()
{
  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }
}

#define LINES_PER_X 1
#define LINES_PER_Y 10
#define CLIENT_BORDER 2

PIXaabbox2D CCustomComboWnd::GetLineBBox( INDEX iLine)
{
  PIX pixWidth, pixHeight;
  GetComboLineSize(pixWidth, pixHeight);
  // return calculated box
  return PIXaabbox2D( PIX2D(0, pixHeight*iLine), PIX2D(pixWidth, pixHeight*(iLine+1)-1) );
}

BEGIN_MESSAGE_MAP(CCustomComboWnd, CWnd)
	//{{AFX_MSG_MAP(CCustomComboWnd)
	ON_WM_PAINT()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CCustomComboWnd message handlers

void CCustomComboWnd::OnPaint() 
{
  {
  CPaintDC dc(this); // device context for painting
  }

  if( m_iTimerID == -1)
  {
    m_iTimerID = (int) SetTimer( 1, 10, NULL);
  }

  POINT ptMouse;
  GetCursorPos( &ptMouse); 
  ScreenToClient( &ptMouse);

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()) )
  {
    CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
    ASSERT( pWorldEditorView != NULL);
    // clear background
    m_pDrawPort->Fill( C_vdGRAY|CT_OPAQUE);
    // erase z-buffer
    m_pDrawPort->FillZBuffer(ZBUF_BACK);
    // for all lines
    for( INDEX iLine=0; iLine<m_dcComboLines.Count(); iLine++)
    {
      // get current line's box in pixels inside window
      PIXaabbox2D boxLine = GetLineBBox( iLine);
      PIX2D pixMin=boxLine.Min();
      PIX2D pixMax=boxLine.Max();
      pixMax(1)-=1;
      pixMax(2)-=1;
      PIXaabbox2D boxLineDecreased=PIXaabbox2D(pixMin,pixMax);

      PIXaabbox2D boxPoint( PIX2D( ptMouse.x, ptMouse.y), PIX2D(ptMouse.x, ptMouse.y) );
      BOOL bUnderMouse=(boxLine & boxPoint) == boxPoint;
      BOOL bSelected=(m_pfResult!=NULL && iLine==*m_pfResult);

      COLOR colFill=C_BLACK|CT_TRANSPARENT;
      if(bUnderMouse&&bSelected)  colFill=COLOR_SELECTED_UNDER_MOUSE;
      else if(bUnderMouse)        colFill=COLOR_UNDER_MOUSE;
      else if(bSelected)          colFill=COLOR_SELECTED;
      
      RenderOneLine( iLine, boxLineDecreased, m_pDrawPort, colFill);
    }

    m_pDrawPort->DrawBorder( 0,0, m_pDrawPort->GetWidth(),m_pDrawPort->GetHeight(), C_mdGRAY|CT_OPAQUE);
    // unlock the drawport
    m_pDrawPort->Unlock();

    // if there is a valid viewport
    if (m_pViewPort!=NULL)
    {
      m_pViewPort->SwapBuffers();
    }
  }
}

BOOL CCustomComboWnd::Initialize(FLOAT *pfResult, void (*pOnSelect)(INDEX iSelected), 
                                 PIX pixX, PIX pixY, BOOL bDown/*=FALSE*/)
{
  m_pfResult=pfResult;
  m_pOnSelect=pOnSelect;
  // calculate window's size
  CRect rectWindow;
  rectWindow.left = pixX;
  rectWindow.bottom = pixY;
  PIX pixWidth, pixHeight;
  GetComboLineSize(pixWidth, pixHeight);
  pixHeight=pixHeight*m_dcComboLines.Count()-1;
  rectWindow.right = rectWindow.left + pixWidth;
  if( bDown)
  {
    rectWindow.top = rectWindow.bottom;
    rectWindow.bottom+=pixHeight;
  }
  else
  {
    rectWindow.top = rectWindow.bottom - pixHeight;
  }

  if( IsWindow(m_hWnd))
  {
    SetWindowPos( NULL, rectWindow.left, rectWindow.top,
      rectWindow.right-rectWindow.left, rectWindow.top-rectWindow.bottom,
      SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(SW_SHOW);
  }
  else
  {
    // create window
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    BOOL bResult = CreateEx( WS_EX_TOOLWINDOW,
      NULL, L"Custom combo", WS_CHILD|WS_POPUP|WS_VISIBLE,
      rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
      pMainFrame->m_hWnd, NULL, NULL);
    if( !bResult)
    {
      AfxMessageBox( L"Error: Failed to create custom combo!");
      return FALSE;
    }
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }
  return TRUE;
}

INDEX CCustomComboWnd::InsertItem(CTString strText, CTFileName fnmIcons/*=""*/, MEXaabbox2D boxIcon/*=dummy box*/)
{
  CComboLine *pcl=new(CComboLine);
  pcl->cl_fnmTexture=fnmIcons;
  pcl->cl_boxIcon=boxIcon;
  pcl->cl_strText=strText;
  m_dcComboLines.Add(pcl);
  INDEX iCount=m_dcComboLines.Count();
  pcl->cl_ulValue=iCount-1;
  pcl->cl_colText=C_YELLOW|CT_OPAQUE;
  return iCount-1;
}

void CCustomComboWnd::SetItemValue(INDEX iItem, ULONG ulValue)
{
  m_dcComboLines[iItem].cl_ulValue=ulValue;
}

void CCustomComboWnd::SetItemColor(INDEX iItem, COLOR col)
{
  m_dcComboLines[iItem].cl_colText=col;
}

void CCustomComboWnd::RenderOneLine( INDEX iLine, PIXaabbox2D rectLine, CDrawPort *pdp, COLOR colFill)
{
  CComboLine &cl=m_dcComboLines[iLine];
  pdp->Fill(rectLine.Min()(1)-1, rectLine.Min()(2)-1, 
                          rectLine.Max()(1)-rectLine.Min()(1)+1, rectLine.Max()(2)-rectLine.Min()(2)+1,
                          colFill);
  if(cl.cl_fnmTexture!="")
  {
    CTextureObject to;
    try
    {
      to.SetData_t(cl.cl_fnmTexture);
      PIXaabbox2D rectIcon=PIXaabbox2D( rectLine.Min()+PIX2D(SPACING_H/2,SPACING_V/2),
        PIX2D(rectLine.Min()+cl.cl_boxIcon.Size())-PIX2D(SPACING_H/2,SPACING_V/2));
      pdp->PutTexture( &to, rectIcon, cl.cl_boxIcon);
    }
    catch (const char *strError)
    {
      (void) strError;
    }
  }
  pdp->SetFont( _pfdConsoleFont);
  pdp->SetTextAspect( 1.0f);
  pdp->SetTextScaling( 1.0f);
  PIX pixX=rectLine.Min()(1)+cl.cl_boxIcon.Size()(1)+SPACING_H/2;

  PIX pixTextH= GetFixedTextHeight( _pfdConsoleFont);
  PIX pixY=rectLine.Min()(2)+(rectLine.Size()(2)-pixTextH)/2;
  pdp->PutText( cl.cl_strText, pixX, pixY, cl.cl_colText);
}

void CCustomComboWnd::OnKillFocus(CWnd* pNewWnd) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( pNewWnd!=pMainFrame->m_pwndToolTip && pNewWnd!=this)
  {
    // destroy combo
    DestroyWindow();
    DeleteTempMap();
  }
}

void CCustomComboWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
}

void CCustomComboWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
  Invalidate(FALSE);
	CWnd::OnMouseMove(nFlags, point);
}

void CCustomComboWnd::OnDestroy() 
{
  KillTimer( m_iTimerID);
	CWnd::OnDestroy();
}

void CCustomComboWnd::OnTimer(UINT_PTR nIDEvent) 
{
  Invalidate(FALSE);	
	CWnd::OnTimer(nIDEvent);
}

BOOL CCustomComboWnd::PreTranslateMessage(MSG* pMsg) 
{
  if( pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_ESCAPE)
  {
    DestroyWindow();
    DeleteTempMap();
    return TRUE;
  }
	return CWnd::PreTranslateMessage(pMsg);
}

void CCustomComboWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
  PIXaabbox2D boxPoint( PIX2D( point.x, point.y), PIX2D(point.x, point.y) );

  // for all lines
  for( INDEX iLine=0; iLine<m_dcComboLines.Count(); iLine++)
  {
    if( (GetLineBBox( iLine) & boxPoint) == boxPoint)
    {
      if(m_pfResult!=NULL)
      {
        *m_pfResult= m_dcComboLines[iLine].cl_ulValue;
        // destroy combo
        DestroyWindow();
        DeleteTempMap();
        return;
      }
      if(m_pOnSelect!=NULL)
      {
        void (*pOnSelect)(INDEX iSelected)=m_pOnSelect;
        INDEX iValue=m_dcComboLines[iLine].cl_ulValue;
        // destroy combo
        DestroyWindow();
        DeleteTempMap();
        pOnSelect(iValue);
        return;
      }
    }
  }
	
	CWnd::OnLButtonUp(nFlags, point);
}
