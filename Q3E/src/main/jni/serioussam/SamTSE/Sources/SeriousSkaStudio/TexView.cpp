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

// TexView.cpp : implementation file
//

#include "stdafx.h"
#include "seriousskastudio.h"
#include "TexView.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTexView

CTexView::CTexView()
{
  m_pDrawPort=NULL;
  m_pViewPort=NULL;
}

CTexView::~CTexView()
{
}


BEGIN_MESSAGE_MAP(CTexView, CWnd)
	//{{AFX_MSG_MAP(CTexView)
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTexView message handlers

void CTexView::OnPaint() 
{
  {CPaintDC dc(this);}

  if (m_pDrawPort==NULL || !m_pDrawPort->Lock()) {
    return;
  }

  // clear browsing window
  m_pDrawPort->FillZBuffer( ZBUF_BACK);
  m_pDrawPort->Fill( C_BLACK | CT_OPAQUE);
  if(m_ptoPreview.GetData()!=NULL) {
    PIXaabbox2D rectPict;
    rectPict = PIXaabbox2D( PIX2D(0, 0), PIX2D(m_pDrawPort->GetWidth(), m_pDrawPort->GetHeight()));
    m_pDrawPort->PutTexture(&m_ptoPreview,rectPict);
  }
  m_pDrawPort->Unlock();
  if (m_pViewPort!=NULL) {
    // swap it
    m_pViewPort->SwapBuffers();
  }
}

BOOL CTexView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
  _pGfx->CreateWindowCanvas( pParentWnd->m_hWnd, &m_pViewPort, &m_pDrawPort);
  return TRUE;
}

void CTexView::ChangeTexture(CTString strNewTexObject)
{
  m_ptoPreview.SetData_t(strNewTexObject);
  Invalidate(TRUE);
}

void CTexView::OnSize(UINT nType, int cx, int cy) 
{
  CRect rc;
  GetParent()->GetClientRect(&rc);
  INDEX iWidth = cx;
  if(cy<cx) iWidth=cy;
  INDEX iX = rc.right/2-cx/2;
  if(iX!=0 && iWidth>0)
  {
    ::SetWindowPos(m_pViewPort->vp_hWndParent,wndTop,iX,55,iWidth,cy,SWP_NOZORDER);
    m_pViewPort->Resize();
  }
}
