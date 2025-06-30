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

// WndDisplayTexture.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "WndDisplayTexture.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndDisplayTexture

CWndDisplayTexture::CWndDisplayTexture()
{
  m_ptd=NULL;
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
}

CWndDisplayTexture::~CWndDisplayTexture()
{
  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }
}


BEGIN_MESSAGE_MAP(CWndDisplayTexture, CWnd)
	//{{AFX_MSG_MAP(CWndDisplayTexture)
	ON_WM_PAINT()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndDisplayTexture message handlers

void CWndDisplayTexture::OnPaint() 
{
  {
  CPaintDC dc(this); // device context for painting
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
    
    CTextureObject to;
    to.SetData(m_ptd);
    m_pDrawPort->PutTexture( &to, m_boxTexture);
    m_pDrawPort->DrawBorder( 0,0, m_pDrawPort->GetWidth(),m_pDrawPort->GetHeight(), C_mdGRAY|CT_OPAQUE);

    m_pDrawPort->SetFont( _pfdConsoleFont);
    m_pDrawPort->SetTextAspect( 1.0f);
    m_pDrawPort->SetTextScaling( 1.0f);
    m_pDrawPort->PutTextCXY( m_strText1, m_boxText1.Center()(1), m_boxText1.Center()(2), C_YELLOW|CT_OPAQUE);
    m_pDrawPort->PutTextCXY( m_strText2, m_boxText2.Center()(1), m_boxText2.Center()(2), C_GRAY|CT_OPAQUE);

    // unlock the drawport
    m_pDrawPort->Unlock();

    // if there is a valid viewport
    if (m_pViewPort!=NULL)
    {
      m_pViewPort->SwapBuffers();
    }
  }
}

BOOL CWndDisplayTexture::Initialize(PIX pixX, PIX pixY, CTextureData *ptd,
                                    CTString strText1/*""*/, CTString strText2/*""*/, BOOL bDown/*=FALSE*/)
{
  m_ptd=ptd;
  m_strText1=strText1;
  m_strText2=strText2;
  // calculate window's size
  CRect rectWindow;
  rectWindow.left = pixX;
  rectWindow.bottom = pixY;
  PIX pixWidth=ptd->GetPixWidth();
  PIX pixHeight=ptd->GetPixHeight();

  PIX pixScreenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
  PIX pixScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

  if( pixX+pixWidth>pixScreenWidth)
  {
    pixX=pixScreenWidth-pixWidth;
  }
  if( pixY+pixHeight>pixScreenHeight)
  {
    pixY=pixScreenHeight-pixHeight;
  }

  m_boxTexture=PIXaabbox2D( PIX2D(0,0), PIX2D(pixWidth,pixHeight));
  if( strText1!="")
  {
    pixHeight+=_pfdConsoleFont->fd_pixCharHeight+4;
  }
  m_boxText1=PIXaabbox2D( PIX2D(0,m_boxTexture.Max()(2)), PIX2D(pixWidth,pixHeight));
  if( strText2!="")
  {
    pixHeight+=_pfdConsoleFont->fd_pixCharHeight+4;
  }
  m_boxText2=PIXaabbox2D( PIX2D(0,m_boxText1.Max()(2)), PIX2D(pixWidth,pixHeight));

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
      NULL, L"Display texture", WS_CHILD|WS_POPUP|WS_VISIBLE,
      rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
      pMainFrame->m_hWnd, NULL, NULL);
    if( !bResult)
    {
      AfxMessageBox( L"Error: Failed to create display texture window!");
      return FALSE;
    }
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }
  return TRUE;
}

void CWndDisplayTexture::OnKillFocus(CWnd* pNewWnd) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if(pNewWnd!=pMainFrame->m_pwndToolTip && pNewWnd!=this)
  {
    DestroyWindow();
    DeleteTempMap();
  }
}

BOOL CWndDisplayTexture::PreTranslateMessage(MSG* pMsg) 
{
  if( pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_ESCAPE)
  {
    DestroyWindow();
    DeleteTempMap();
    return TRUE;
  }
	return CWnd::PreTranslateMessage(pMsg);
}
