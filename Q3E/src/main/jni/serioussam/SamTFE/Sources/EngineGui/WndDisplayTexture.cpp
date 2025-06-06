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

#include "EngineGui/StdH.h"
#include <Engine/Graphics/TextureEffects.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndDisplayTexture

TIME timeLastTick;

CWndDisplayTexture::CWndDisplayTexture()
{
  m_pLeftMouseButtonClicked = NULL;
  m_pLeftMouseButtonReleased = NULL;
  m_pRightMouseButtonClicked = NULL;
  m_pRightMouseButtonMoved = NULL;
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  m_iTimerID = -1;
  m_bChequeredAlpha = TRUE;
  m_bForce32  = FALSE;
  m_bDrawLine = FALSE;
}

CWndDisplayTexture::~CWndDisplayTexture()
{
}


BEGIN_MESSAGE_MAP(CWndDisplayTexture, CWnd)
	//{{AFX_MSG_MAP(CWndDisplayTexture)
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndDisplayTexture message handlers

// converts window mouse coordinates to buffer coordinates
static void ConvertCoords( CTextureData *pTD, PIX &pixU, PIX &pixV)
{

}

void CWndDisplayTexture::OnPaint() 
{
  { CPaintDC dc(this); } // device context for painting
	
  if( m_iTimerID==-1) m_iTimerID = (int)SetTimer( 1, 50, NULL);

  if( m_pViewPort==NULL && m_pDrawPort==NULL)
  { // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }

  // get texture data
  CTextureData *pTD = (CTextureData*)m_toTexture.GetData();
  BOOL bAlphaChannel = FALSE;
  // if there is a valid drawport, and the drawport can be locked
  if( m_pDrawPort!=NULL && m_pDrawPort->Lock())
  { // if it has any texture
    if( pTD!=NULL) {
      PIX pixWidth  = pTD->GetPixWidth();
      PIX pixHeight = pTD->GetPixHeight();
      // adjust for effect texture
      if( pTD->td_ptegEffect != NULL) {
        pixWidth  = pTD->td_pixBufferWidth;
        pixHeight = pTD->td_pixBufferHeight;
      }
      // get window / texture factor
      if( pixWidth >= pixHeight) {
        m_fWndTexRatio = FLOAT(m_pDrawPort->GetWidth())  /pixWidth;
      } else {
        m_fWndTexRatio = FLOAT(m_pDrawPort->GetHeight()) /pixHeight;
      }
      // get width and height of texture window
      m_pixWinWidth   = pixWidth *m_fWndTexRatio;
      m_pixWinHeight  = pixHeight*m_fWndTexRatio;
      m_pixWinOffsetU = (m_pDrawPort->GetWidth() -m_pixWinWidth ) /2;
      m_pixWinOffsetV = (m_pDrawPort->GetHeight()-m_pixWinHeight) /2;
      // determine texture's alpha channel presence
      bAlphaChannel = pTD->td_ulFlags&TEX_ALPHACHANNEL;
    } else {
      // whole screen
      m_fWndTexRatio  = 1.0f;
      m_pixWinWidth   = m_pDrawPort->GetWidth();
      m_pixWinHeight  = m_pDrawPort->GetHeight();
      m_pixWinOffsetU = 0;
      m_pixWinOffsetV = 0;
    }

    // clear window background
    m_pDrawPort->Fill( C_GRAY|CT_OPAQUE);
    // if chequered alpha flag is turned on
#define CHESS_BOX_WIDTH  16
#define CHESS_BOX_HEIGHT 16
    if( m_bChequeredAlpha && bAlphaChannel) 
    { // create chess looking background
      for( INDEX iVert=0; iVert<(m_pixWinHeight/CHESS_BOX_HEIGHT)+1; iVert++) {
        // create chess looking background
        for( INDEX iHoriz=0; iHoriz<(m_pixWinWidth/CHESS_BOX_WIDTH)+1; iHoriz++) {
          COLOR colBox = C_WHITE;
          if( (iHoriz+iVert)&1) colBox = C_lGRAY;
          // fill part of a drawport with a given color
          m_pDrawPort->Fill( iHoriz*CHESS_BOX_WIDTH +m_pixWinOffsetU, 
                             iVert *CHESS_BOX_HEIGHT+m_pixWinOffsetV,
                             CHESS_BOX_WIDTH, CHESS_BOX_HEIGHT, colBox|CT_OPAQUE);
        }
      }
    }

    // if it has any texture
    if( pTD!=NULL) {
      // create rectangle proportional with texture ratio covering whole draw port
      PIXaabbox2D rectPict = PIXaabbox2D( PIX2D( m_pixWinOffsetU, m_pixWinOffsetV),
                                          PIX2D( m_pixWinOffsetU+m_pixWinWidth, m_pixWinOffsetV+m_pixWinHeight));
      // draw texture
      m_pDrawPort->PutTexture( &m_toTexture, rectPict);
    } 

    // draw line on left mouse move
    if( m_bDrawLine) {
      m_pDrawPort->DrawLine(m_pixLineStartU, m_pixLineStartV, 
                            m_pixLineStopU,  m_pixLineStopV, C_WHITE|CT_OPAQUE, 0xCCCCCCCC);
      m_pDrawPort->DrawLine(m_pixLineStartU, m_pixLineStartV, 
                            m_pixLineStopU,  m_pixLineStopV, C_BLACK|CT_OPAQUE, 0x33333333);
    }

    // unlock the drawport
    m_pDrawPort->Unlock();
    // swap if there is a valid viewport
    if( m_pViewPort!=NULL) m_pViewPort->SwapBuffers();
  }

  // if this is effect texture
  if( pTD!=NULL && pTD->td_ptegEffect!=NULL)
  { // display rendering speed
    DOUBLE dMS = pTD->td_ptegEffect->GetRenderingTime() * 1000.0;
    // only if valid
    if( dMS>0) {
      char achrSpeed[256];
      CDlgCreateEffectTexture *pDialog = (CDlgCreateEffectTexture*)GetParent();
      sprintf( achrSpeed, "Rendering speed: %.2f ms", dMS);
      pDialog->m_strRendSpeed = achrSpeed;
      pDialog->UpdateData( FALSE);
    }
    // reset statistics
    STAT_Reset();
  }
}


void CWndDisplayTexture::OnTimer(UINT_PTR nIDEvent)
{
	// on our timer discard test animation window
  if( nIDEvent == 1)
  {
    TIME timeCurrentTick = _pTimer->GetRealTimeTick();
    if( timeCurrentTick > timeLastTick )
    {
      _pTimer->SetCurrentTick( timeCurrentTick);
      timeLastTick = timeCurrentTick;
    }
    Invalidate(FALSE);	
  }

	CWnd::OnTimer(nIDEvent);
}


void CWndDisplayTexture::OnDestroy() 
{
  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }

  KillTimer( m_iTimerID);
  _pTimer->SetCurrentTick( 0.0f);
	CWnd::OnDestroy();
}


void CWndDisplayTexture::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // start drawing line
  m_bDrawLine = TRUE;
  // store mouse start coordinates
  m_pixLineStartU = point.x;
  m_pixLineStartV = point.y;
  m_pixLineStopU  = point.x;
  m_pixLineStopV  = point.y;

  // get texture data from surface
  CTextureData *pTD = (CTextureData*)m_toTexture.GetData();
  PIX pixU = point.x-m_pixWinOffsetU;
  PIX pixV = point.y-m_pixWinOffsetV;
  if( pixU<0 || pixU>m_pixWinWidth ) return;
  if( pixV<0 || pixV>m_pixWinHeight) return;
  pixU = PIX(pixU/m_fWndTexRatio);
  pixV = PIX(pixV/m_fWndTexRatio);
  
  // if there is valid mouse down function set
  if( m_pLeftMouseButtonClicked != NULL)
  { // call it
    m_pLeftMouseButtonClicked(pixU, pixV);
  }

	CWnd::OnLButtonDown(nFlags, point);
}

void CWndDisplayTexture::OnLButtonUp(UINT nFlags, CPoint point) 
{
  // stop drawing line
  m_bDrawLine = FALSE;

  // get texture data from surface
  CTextureData *pTD = (CTextureData*)m_toTexture.GetData();
  PIX pixU = point.x-m_pixWinOffsetU;
  PIX pixV = point.y-m_pixWinOffsetV;
  if( pixU<0 || pixU>m_pixWinWidth ) return;
  if( pixV<0 || pixV>m_pixWinHeight) return;
  pixU = PIX(pixU/m_fWndTexRatio);
  pixV = PIX(pixV/m_fWndTexRatio);
  
  // if there is valid mouse down function set
  if( m_pLeftMouseButtonReleased != NULL)
  { // call it
    m_pLeftMouseButtonReleased(pixU, pixV);
  }

	CWnd::OnLButtonUp(nFlags, point);
}

void CWndDisplayTexture::OnRButtonDown(UINT nFlags, CPoint point) 
{
  // get texture data from surface
  CTextureData *pTD = (CTextureData*)m_toTexture.GetData();
  PIX pixU = point.x-m_pixWinOffsetU;
  PIX pixV = point.y-m_pixWinOffsetV;
  if( pixU<0 || pixU>m_pixWinWidth ) return;
  if( pixV<0 || pixV>m_pixWinHeight) return;
  pixU = PIX(pixU/m_fWndTexRatio);
  pixV = PIX(pixV/m_fWndTexRatio);
  
  // if there is valid mouse down function set
  if( m_pRightMouseButtonClicked != NULL)
  { // call it
    m_pRightMouseButtonClicked(pixU, pixV);
  }

	CWnd::OnRButtonDown(nFlags, point);
}

void CWndDisplayTexture::OnMouseMove(UINT nFlags, CPoint point) 
{
  // if right mouse is down
  if (nFlags&MK_RBUTTON) {
    // get texture data from surface
    CTextureData *pTD = (CTextureData*)m_toTexture.GetData();
    PIX pixU = point.x-m_pixWinOffsetU;
    PIX pixV = point.y-m_pixWinOffsetV;
    if( pixU<0 || pixU>m_pixWinWidth ) return;
    if( pixV<0 || pixV>m_pixWinHeight) return;
    pixU = PIX(pixU/m_fWndTexRatio);
    pixV = PIX(pixV/m_fWndTexRatio);
  
    // if there is valid mouse down function set
    if( m_pRightMouseButtonMoved != NULL)
    { // call it
      m_pRightMouseButtonMoved(pixU, pixV);
    }
  // if left mouse is down
  } else if (nFlags&MK_LBUTTON) {
    // store mouse coordinates
    m_pixLineStopU = point.x;
    m_pixLineStopV = point.y;
  }
	
	CWnd::OnMouseMove(nFlags, point);
}
