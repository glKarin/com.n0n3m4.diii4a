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

// BrushPaletteWnd.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "BrushPaletteWnd.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrushPaletteWnd

CBrushPaletteWnd::CBrushPaletteWnd()
{
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  // mark that timer is not yet started
  m_iTimerID = -1;
}

CBrushPaletteWnd::~CBrushPaletteWnd()
{
  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }
}


BEGIN_MESSAGE_MAP(CBrushPaletteWnd, CWnd)
	//{{AFX_MSG_MAP(CBrushPaletteWnd)
	ON_WM_PAINT()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBrushPaletteWnd message handlers

#define BRUSHES_PER_X 4
#define BRUSHES_PER_Y 8
#define CLIENT_BORDER 2

PIXaabbox2D CBrushPaletteWnd::GetBrushBBox( INDEX iBrush)
{
  CRect rectClient;
  // get window's client area
  GetClientRect( &rectClient);
  PIX DX = (rectClient.Width()+1  - 2*CLIENT_BORDER)/BRUSHES_PER_X;
  PIX DY = (rectClient.Height()+1 - 2*CLIENT_BORDER)/BRUSHES_PER_Y;
  // calculate starting pixel for current Brush
  PIX pixXS = CLIENT_BORDER + (iBrush%BRUSHES_PER_X)*DX;
  PIX pixYS = CLIENT_BORDER + (iBrush/BRUSHES_PER_X)*DY;
  // return calculated box
  return PIXaabbox2D( PIX2D(pixXS, pixYS), PIX2D(pixXS+DX-1, pixYS+DY-1) );
}

void CBrushPaletteWnd::OnPaint() 
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
    m_pDrawPort->Fill( C_lGRAY|CT_OPAQUE);
    // erase z-buffer
    m_pDrawPort->FillZBuffer(ZBUF_BACK);
    // for all brushes
    for( INDEX iBrush=0; iBrush<CT_BRUSHES; iBrush++)
    {
      // get current brush's box in pixels inside window
      PIXaabbox2D boxBrush = GetBrushBBox( iBrush);
      RenderBrushShape( iBrush, boxBrush, m_pDrawPort);

      TIME tm=_pTimer->GetRealTimeTick();
      // if we are drawing selected brush
      if(iBrush==theApp.m_fCurrentTerrainBrush)
      {
        if(m_pDrawPort->Lock())
        {
          FLOAT fFactor=sin(tm*8)/2.0f+0.5f;
          COLOR colSelected=LerpColor(C_lGRAY,C_RED,fFactor);
          m_pDrawPort->DrawBorder(boxBrush.Min()(1)-1, boxBrush.Min()(2)-1, 
                                  boxBrush.Max()(1)-boxBrush.Min()(1)+2, boxBrush.Max()(2)-boxBrush.Min()(2)+2,
                                  colSelected|CT_OPAQUE);
          m_pDrawPort->Unlock();
        }
      }
      PIXaabbox2D boxPoint( PIX2D( ptMouse.x, ptMouse.y), PIX2D(ptMouse.x, ptMouse.y) );
      if( (boxBrush & boxPoint) == boxPoint)
      {
        if(m_pDrawPort->Lock())
        {
          INDEX iRot=((ULONG)(tm*25.0f))&7;
          ULONG ulLineType=0x0f0f0f0f<<iRot;
          m_pDrawPort->DrawBorder(boxBrush.Min()(1)-1, boxBrush.Min()(2)-1, 
                                  boxBrush.Max()(1)-boxBrush.Min()(1)+2, boxBrush.Max()(2)-boxBrush.Min()(2)+2,
                                  C_BLUE|CT_OPAQUE, ulLineType);
          m_pDrawPort->Unlock();
        }
      }
    }

    // unlock the drawport
    m_pDrawPort->Unlock();

    // if there is a valid viewport
    if (m_pViewPort!=NULL)
    {
      m_pViewPort->SwapBuffers();
    }
  }
}

void CBrushPaletteWnd::OnKillFocus(CWnd* pNewWnd) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( pNewWnd!=pMainFrame->m_pwndToolTip && pNewWnd!=this)
  {
    // destroy brush palette
    _pBrushPalette = NULL;
    delete this;
  }
}

void CBrushPaletteWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
}

void CBrushPaletteWnd::OnRButtonDown(UINT nFlags, CPoint point) 
{
  PIXaabbox2D boxPoint( PIX2D( point.x, point.y), PIX2D(point.x, point.y) );
  // for all brushes
  for( INDEX iBrush=0; iBrush<CT_BRUSHES; iBrush++)
  {
    if( (GetBrushBBox( iBrush) & boxPoint) == boxPoint)
    {
      // destroy brush palette
      _pBrushPalette = NULL;
      delete this;
      // invoke edit terrain dlg
      CDlgEditTerrainBrush dlg;
      dlg.m_iBrush=iBrush;
      dlg.DoModal();
      break;
    }
  }
}

void CBrushPaletteWnd::OnMouseMove(UINT nFlags, CPoint point) 
{
  Invalidate(FALSE);
	CWnd::OnMouseMove(nFlags, point);
}

void CBrushPaletteWnd::OnDestroy() 
{
  KillTimer( m_iTimerID);
	CWnd::OnDestroy();
}

void CBrushPaletteWnd::OnTimer(UINT_PTR nIDEvent) 
{
  POINT pt;
  GetCursorPos( &pt);
  CRect rectWnd;
  GetWindowRect(rectWnd);
  if(pt.x<rectWnd.left || pt.x>rectWnd.right ||
     pt.y<rectWnd.top  || pt.y>rectWnd.bottom)
  {
    DestroyWindow();
    DeleteTempMap();
    return;
  }

  Invalidate(FALSE);	
	CWnd::OnTimer(nIDEvent);
}

BOOL CBrushPaletteWnd::PreTranslateMessage(MSG* pMsg) 
{
  if( pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_ESCAPE)
  {
    DestroyWindow();
    DeleteTempMap();
    return TRUE;
  }

	return CWnd::PreTranslateMessage(pMsg);
}

void CBrushPaletteWnd::OnLButtonUp(UINT nFlags, CPoint point) 
{
  PIXaabbox2D boxPoint( PIX2D( point.x, point.y), PIX2D(point.x, point.y) );

  // for all brushes
  for( INDEX iBrush=0; iBrush<CT_BRUSHES; iBrush++)
  {
    if( (GetBrushBBox( iBrush) & boxPoint) == boxPoint)
    {
      theApp.m_fCurrentTerrainBrush=iBrush;
      break;
    }
  }
  // destroy brush palette
  _pBrushPalette = NULL;
  delete this;
  theApp.m_ctTerrainPageCanvas.MarkChanged();
}
