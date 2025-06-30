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

// ActiveTextureWnd.cpp : implementation file
//

#include "stdafx.h"
#include "ActiveTextureWnd.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActiveTextureWnd

CActiveTextureWnd::CActiveTextureWnd()
{
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
}

CActiveTextureWnd::~CActiveTextureWnd()
{
}


BEGIN_MESSAGE_MAP(CActiveTextureWnd, CWnd)
	//{{AFX_MSG_MAP(CActiveTextureWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CActiveTextureWnd message handlers

void CActiveTextureWnd::OnPaint() 
{
  {
  CPaintDC dc(this);
  }

  if( (m_pViewPort == NULL) && (m_pDrawPort == NULL) )
  {
    // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()) )
  {
    PIXaabbox2D rectPict;
    rectPict = PIXaabbox2D( PIX2D(0, 0),
                            PIX2D(m_pDrawPort->GetWidth(), m_pDrawPort->GetHeight()));
    // clear texture area to black
    m_pDrawPort->Fill( C_BLACK | CT_OPAQUE);
    // erase z-buffer
    m_pDrawPort->FillZBuffer(ZBUF_BACK);
    
    // if there is valid active texture
    if( theApp.m_ptdActiveTexture != NULL)
    {
      CTextureObject toActiveTexture;
      toActiveTexture.SetData( theApp.m_ptdActiveTexture);
      m_pDrawPort->PutTexture( &toActiveTexture, rectPict);
    }
    else
    {
      // type text saying none selected
      m_pDrawPort->SetFont( theApp.m_pfntSystem);
      m_pDrawPort->SetTextAspect( 1.0f);
      m_pDrawPort->PutText( "None", 25, 20);
      m_pDrawPort->PutText( "selected", 8, 50);
    }

    // unlock the drawport
    m_pDrawPort->Unlock();

    if (m_pViewPort!=NULL)
    {
      m_pViewPort->SwapBuffers();
    }
  }
}

void CActiveTextureWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
  if( theApp.m_ptdActiveTexture == NULL) return;
  HGLOBAL hglobal = CreateHDrop( theApp.m_ptdActiveTexture->GetName());
  m_DataSource.CacheGlobalData( CF_HDROP, hglobal);
  m_DataSource.DoDragDrop( DROPEFFECT_COPY);
  CWnd::OnLButtonDown(nFlags, point);
}

void CActiveTextureWnd::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CWnd::OnRButtonDown(nFlags, point);
}

void CActiveTextureWnd::OnDestroy() 
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
