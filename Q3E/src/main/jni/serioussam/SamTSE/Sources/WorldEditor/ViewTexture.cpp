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

// ViewTexture.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "ViewTexture.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewTexture

CViewTexture::CViewTexture()
{
  m_pViewPort = NULL;
  m_pDrawPort = NULL;
}

CViewTexture::~CViewTexture()
{
}


BEGIN_MESSAGE_MAP(CViewTexture, CWnd)
	//{{AFX_MSG_MAP(CViewTexture)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_RECREATE_TEXTURE, OnRecreateTexture)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CViewTexture message handlers

void CViewTexture::OnPaint()
{
  {
  CPaintDC dc(this); // device context for painting
  }

  CWnd *pwndRect = GetParent()->GetDlgItem( IDC_PREVIEW_FRAME);
  if( pwndRect != NULL)
  {
    CRect rectBorder;
    pwndRect->GetWindowRect( rectBorder);
    GetParent()->ScreenToClient( rectBorder);
    MoveWindow( rectBorder);
  }

  if( (m_pViewPort == NULL) && (m_pDrawPort == NULL) )
  {
    // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()))
  {
    PIX pixWidth = m_pDrawPort->GetWidth();
    PIX pixHeight = m_pDrawPort->GetHeight();

    PIXaabbox2D rectPict;
    rectPict = PIXaabbox2D( PIX2D(0, 0),
                            PIX2D(m_pDrawPort->GetWidth(), m_pDrawPort->GetHeight()));
    // clear texture area to black
    m_pDrawPort->Fill( C_BLACK | CT_OPAQUE);
    // erase z-buffer
    m_pDrawPort->FillZBuffer(ZBUF_BACK);

    CTextureObject toTexture;
    try
    {
      toTexture.SetData_t( m_strTexture);
    }
    catch (const char *strError)
    {
      (void) strError;
    }

    if( toTexture.GetData() != NULL)
    {
      m_pDrawPort->PutTexture( &toTexture, rectPict);
    }
    else
    {
      // type text saying none selected
      m_pDrawPort->SetFont( theApp.m_pfntSystem);
      m_pDrawPort->SetTextAspect( 1.0f);
      m_pDrawPort->PutTextC( m_strTexture, pixWidth/2, pixHeight*1/3-10);
    }

    // unlock the drawport
    m_pDrawPort->Unlock();
    // if there is a valid viewport
    if (m_pViewPort!=NULL)
    {
      // swap it
      m_pViewPort->SwapBuffers();
    }
  }
}

void CViewTexture::OnLButtonDown(UINT nFlags, CPoint point)
{
  if( !GetParent()->GetDlgItem( IDC_PREVIEW_FRAME)->IsWindowEnabled())
  {
    return;
  }
  HGLOBAL hglobal = CreateHDrop( m_strTexture);
  m_DataSource.CacheGlobalData( CF_HDROP, hglobal);
  m_DataSource.DoDragDrop( DROPEFFECT_COPY);
  CWnd::OnLButtonDown(nFlags, point);
}

void CViewTexture::OnLButtonDblClk(UINT nFlags, CPoint point)
{
  OnRecreateTexture();
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CViewTexture::OnContextMenu(CWnd* pWnd, CPoint point)
{
  CMenu menu;
  if( menu.LoadMenu(IDR_THUMBNAIL_TEXTURE_POPUP))
  {
    CMenu* pPopup = menu.GetSubMenu(0);
    pPopup->TrackPopupMenu( TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								            point.x, point.y, this);
  }
}

void CViewTexture::OnRecreateTexture()
{
  CTextureObject toTexture;
  try
  {
    toTexture.SetData_t( m_strTexture);
  }
  catch (const char *strError)
  {
    (void) strError;
  }

  if( toTexture.GetData() != NULL)
  {
    _EngineGUI.CreateTexture( CTString(m_strTexture));
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    if( pDoc != NULL)
    {
      pDoc->UpdateAllViews( NULL);
    }
  }
}

void CViewTexture::OnDestroy()
{
	CWnd::OnDestroy();

  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }

  m_pViewPort = NULL;
  m_pDrawPort = NULL;
}
