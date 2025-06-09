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

// WndTestAnimation.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "WndTestAnimation.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWndTestAnimation

CWndTestAnimation::CWndTestAnimation()
{
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  // mark that timer is not yet started
  m_iTimerID = -1;
}

CWndTestAnimation::~CWndTestAnimation()
{
}


BEGIN_MESSAGE_MAP(CWndTestAnimation, CWnd)
	//{{AFX_MSG_MAP(CWndTestAnimation)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndTestAnimation message handlers

void CWndTestAnimation::OnPaint() 
{
  {
  CPaintDC dc(this); // device context for painting
  }

  if( m_iTimerID == -1)
  {
    m_iTimerID = (int) SetTimer( 1, 50, NULL);
  }

  if( (m_pViewPort == NULL) && (m_pDrawPort == NULL) )
  {
    // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()) )
  {
    // get curently selected light animation combo member
    INDEX iLightAnimation = m_pParentDlg->GetSelectedLightAnimation();
    // if animation has changed
    if( iLightAnimation != m_aoAnimObject.GetAnim() )
    {
      // start new animation
      m_aoAnimObject.StartAnim( iLightAnimation);
    }
    // get current frame (color)
    SLONG col0, col1;
    FLOAT fRatio;
    m_aoAnimObject.GetFrame(col0, col1, fRatio);
    COLOR colorFrame = LerpColor(col0, col1, fRatio);

    // clear window background to black
    m_pDrawPort->Fill( colorFrame | CT_OPAQUE);
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

static TIME timeLastTick=TIME(0);
void CWndTestAnimation::OnTimer(UINT_PTR nIDEvent) 
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

void CWndTestAnimation::OnDestroy() 
{
  KillTimer( m_iTimerID);
  _pTimer->SetCurrentTick( 0.0f);
	CWnd::OnDestroy();

  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }

  m_pViewPort = NULL;
  m_pDrawPort = NULL;
}
