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

// WndAnimationFrames.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "WndAnimationFrames.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define FRAME_BOX_WIDTH 10
#define KEY_FRAME_BOX_SIZE 4

/////////////////////////////////////////////////////////////////////////////
// CWndAnimationFrames

CWndAnimationFrames::CWndAnimationFrames()
{
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  m_iSelectedFrame = 0;
  m_iStartingFrame = 0;
  m_iFramesInLine = 0;
}

CWndAnimationFrames::~CWndAnimationFrames()
{
}


BEGIN_MESSAGE_MAP(CWndAnimationFrames, CWnd)
	//{{AFX_MSG_MAP(CWndAnimationFrames)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndAnimationFrames message handlers

BOOL CWndAnimationFrames::IsSelectedFrameKeyFrame(void)
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
  // get animation data
  CAnimData *pAD = m_pParentDlg->m_padAnimData;
  // obtain information about animation
  CAnimInfo aiInfo;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  // get frame (color)
  COLOR colorFrame = pAD->GetFrame(iLightAnimation, m_iSelectedFrame);
  // if selected frame is visible and is key-frame and is not first or last frame
  return ( ((colorFrame & 0x000000FF) == 0xFF) && 
           IsFrameVisible( m_iSelectedFrame) &&
           (m_iSelectedFrame != aiInfo.ai_NumberOfFrames-1) &&
           (m_iSelectedFrame != 0) );
}

void CWndAnimationFrames::OnPaint() 
{
  {
  CPaintDC dc(this); // device context for painting
  }

  if( (m_pViewPort == NULL) && (m_pDrawPort == NULL) )
  {
    // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }

  // calculate how many frames we have in one line
  m_iFramesInLine = m_pDrawPort->GetWidth()/FRAME_BOX_WIDTH;

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()) )
  {
    // clear window background
    m_pDrawPort->Fill( C_GRAY | CT_OPAQUE);

    // get curently selected light animation combo member
    INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
    // get animation data
    CAnimData *pAD = m_pParentDlg->m_padAnimData;
    // obtain information about animation
    CAnimInfo aiInfo;
    pAD->GetAnimInfo(iLightAnimation, aiInfo);
    // for all frames
    for( INDEX iFrame=0; iFrame<aiInfo.ai_NumberOfFrames; iFrame++)
    {
      PIX pixX=(iFrame-m_iStartingFrame)*FRAME_BOX_WIDTH;
      PIX pixY=0;
      PIX pixDX=FRAME_BOX_WIDTH;
      PIX pixDY=m_pDrawPort->GetHeight()-KEY_FRAME_BOX_SIZE*2;
      // if frame is visible
      if( IsFrameVisible( iFrame))
      {
        // get frame (color)
        COLOR colorFrame = pAD->GetFrame(iLightAnimation, iFrame);
        // draw solid rect
        m_pDrawPort->Fill( pixX, pixY, pixDX, pixDY, colorFrame|CT_OPAQUE);
        
        // if this is key-frame marker
        if( (colorFrame & 0x000000FF) == 0xFF)
        {
          // calculate key-frame marker coordinates
          PIX pixKFX = pixX+(FRAME_BOX_WIDTH-KEY_FRAME_BOX_SIZE)/2;
          PIX pixKFY = pixDY+KEY_FRAME_BOX_SIZE;
          // draw key-frame marker
          m_pDrawPort->Fill( pixKFX, pixKFY, KEY_FRAME_BOX_SIZE, KEY_FRAME_BOX_SIZE, C_GREEN|CT_OPAQUE);
        }

        // if this is selected frame
        if( iFrame == m_iSelectedFrame)
        {
          // set looks of rectangle
          ULONG ulLineType = _FULL_;
          // draw 3D button (outer edges black)
          m_pDrawPort->DrawLine( pixX,pixY,pixX,pixY+pixDY, C_BLACK|CT_OPAQUE, ulLineType);
          m_pDrawPort->DrawLine( pixX,pixY+pixDY,pixX+pixDX-1,pixY+pixDY, C_BLACK|CT_OPAQUE, ulLineType);
          m_pDrawPort->DrawLine( pixX+pixDX-1,pixY+pixDY,pixX+pixDX-1,pixY, C_BLACK|CT_OPAQUE, ulLineType);
          m_pDrawPort->DrawLine( pixX+pixDX-1,pixY,pixX,pixY, C_BLACK|CT_OPAQUE, ulLineType);
          // draw inner edges
          m_pDrawPort->DrawLine( pixX+1,pixY+1,pixX+1,pixY+pixDY-1, C_WHITE|CT_OPAQUE, ulLineType);
          m_pDrawPort->DrawLine( pixX+1,pixY+pixDY-1,pixX+pixDX-2,pixY+pixDY-1, C_GRAY|CT_OPAQUE, ulLineType);
          m_pDrawPort->DrawLine( pixX+pixDX-2,pixY+pixDY-1,pixX+pixDX-2,pixY+1, C_dGRAY|CT_OPAQUE, ulLineType);
          m_pDrawPort->DrawLine( pixX+pixDX-2,pixY+1,pixX+1,pixY+1, C_WHITE|CT_OPAQUE, ulLineType);
        }
      }
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

BOOL CWndAnimationFrames::IsFrameVisible(INDEX iFrame) 
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
  // obtain information about animation
  CAnimInfo aiInfo;
  // get animation data
  CAnimData *pAD = m_pParentDlg->m_padAnimData;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  if( (iFrame>=m_iStartingFrame) && 
      (iFrame<(m_iStartingFrame+m_iFramesInLine)) &&
      (iFrame>=0) &&
      (iFrame<aiInfo.ai_NumberOfFrames) )
  {
    return TRUE;
  }
  return FALSE;
}

// clears key-frame marker on selected frame
void CWndAnimationFrames::DeleteSelectedFrame( void)
{
  if( !IsFrameVisible( m_iSelectedFrame)) return;
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
  // get animation data
  CAnimData *pAD = m_pParentDlg->m_padAnimData;
  // obtain information about animation
  CAnimInfo aiInfo;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  // if this is first or last frame in animation, no deletion is allowed
  if( (m_iSelectedFrame == 0) || (m_iSelectedFrame == (aiInfo.ai_NumberOfFrames-1)) ) return;
  // get frame (color) for selected frame
  COLOR colorFrame = pAD->GetFrame(iLightAnimation, m_iSelectedFrame);
  // clear key-frame marker
  pAD->SetFrame(iLightAnimation, m_iSelectedFrame, colorFrame&0xFFFFFF00);
  // spread frames
  m_pParentDlg->SpreadFrames();
  // redraw window
  Invalidate( FALSE);
}

void CWndAnimationFrames::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // get clicked frame
  INDEX iFrame = point.x/FRAME_BOX_WIDTH+m_iStartingFrame;
  // return if frame is not visible (clicked beyond last frame)
  if( !IsFrameVisible(iFrame)) return;
  
  Invalidate(FALSE);
  // if clicked frame was not selected previously
  if( iFrame != m_iSelectedFrame)
  {
    // select clicked frame
    m_iSelectedFrame = iFrame;
    m_pParentDlg->UpdateData( FALSE);
    // return
    return;
  }
  // select clicked frame
  m_iSelectedFrame = iFrame;
  m_pParentDlg->UpdateData( FALSE);

  // get curently selected light animation combo member
  INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
  // get animation data
  CAnimData *pAD = m_pParentDlg->m_padAnimData;
  COLORREF newFrameColor = CLRF_CLR( pAD->GetFrame(iLightAnimation, iFrame));
  if( MyChooseColor( newFrameColor, *m_pParentDlg) )
  {
    // set new key frame value
    pAD->SetFrame(iLightAnimation, iFrame, CLR_CLRF(newFrameColor)|0x000000FF);
    // spread frames
    m_pParentDlg->SpreadFrames();
    // redraw window
    Invalidate( FALSE);
    m_pParentDlg->UpdateData( FALSE);
    m_pParentDlg->m_bChanged = TRUE;
  }
	CWnd::OnLButtonDown(nFlags, point);
}

void CWndAnimationFrames::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	OnLButtonDown(nFlags, point);	
	CWnd::OnLButtonDblClk(nFlags, point);
}

void CWndAnimationFrames::ScrollLeft(void)
{
  if( m_iStartingFrame > 0)
  {
    // scroll left
    m_iStartingFrame --;
    // redraw
    Invalidate(FALSE);
    m_pParentDlg->UpdateData( FALSE);
  }
}

void CWndAnimationFrames::ScrollRight(void)
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
  // obtain information about animation
  CAnimInfo aiInfo;
  // get animation data
  CAnimData *pAD = m_pParentDlg->m_padAnimData;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  // calculate possible new starting frame
  INDEX iNewLastDisplayedFrame = m_iStartingFrame+1+m_iFramesInLine;
  // if new last displayed frame is valid
  if( iNewLastDisplayedFrame <= aiInfo.ai_NumberOfFrames)
  {
    // scroll page right
    m_iStartingFrame ++;
    // redraw
    Invalidate(FALSE);
    m_pParentDlg->UpdateData( FALSE);
  }
}

void CWndAnimationFrames::ScrollPgLeft() 
{
  if( (m_iStartingFrame-m_iFramesInLine) > 0)
  {
    // scroll page left
    m_iStartingFrame -= m_iFramesInLine;
  }
  else
  {
    m_iStartingFrame=0;
  }
  // redraw
  Invalidate(FALSE);
  m_pParentDlg->UpdateData( FALSE);
}

void CWndAnimationFrames::ScrollPgRight() 
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
  // obtain information about animation
  CAnimInfo aiInfo;
  // get animation data
  CAnimData *pAD = m_pParentDlg->m_padAnimData;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  // calculate possible new starting frame
  INDEX iNewStart = m_iStartingFrame+m_iFramesInLine;
  // if new start frame is valid one
  if( iNewStart < aiInfo.ai_NumberOfFrames)
  {
    // scroll page right
    m_iStartingFrame = iNewStart;
    // redraw
    Invalidate(FALSE);
    m_pParentDlg->UpdateData( FALSE);
  }
}

void CWndAnimationFrames::OnDestroy() 
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
