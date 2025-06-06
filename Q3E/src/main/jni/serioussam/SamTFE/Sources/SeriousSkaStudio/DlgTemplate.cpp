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

#include "stdafx.h"
#include "SeriousSkaStudio.h"
#include "DlgTemplate.h"
#include "MainFrm.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CDlgTemplate, CDialogBar)
	//{{AFX_MSG_MAP(CDlgBarTreeView)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDlgTemplate::CDlgTemplate()
{
  dlg_iSplitterID = -1;
  dlg_bDockingEnabled = FALSE;
}
CDlgTemplate::~CDlgTemplate()
{
}

CSize CDlgTemplate::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
  ASSERT(FALSE);
  return CSize(300,300);
}

CSize CDlgTemplate::CalcDynamicLayout(int nLength, DWORD nMode)
{
  CSize csResult;
  // Return default if it is being docked or floated
  if(nMode & LM_VERTDOCK) {
    csResult = m_Size;
    CRect rc;
    // get main frm
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    // get his child
    CMDIClientWnd *pMDIClient = &pMainFrame->m_wndMDIClient;
    pMDIClient->GetWindowRect(rc);
    csResult.cy = rc.bottom - rc.top;
  } else if((nMode & LM_VERTDOCK) || (nMode & LM_HORZDOCK)) {
    // if not docked stretch to fit
    if (nMode & LM_STRETCH) {
      csResult = CSize((nMode & LM_HORZ) ? 32767 : m_Size.cx,
                       (nMode & LM_HORZ) ? m_Size.cy : 32767);
    } else {
      csResult = m_Size;
    }
  } else if (nMode & LM_MRUWIDTH) {
    csResult = m_Size;
  // In all other cases, accept the dynamic length
  } else {
    if (nMode & LM_LENGTHY) {
      // Note that we don't change m_Size.cy because we disabled vertical sizing
      csResult = CSize( m_Size.cx, m_Size.cy = nLength);
    } else {
      csResult = CSize( m_Size.cx = nLength, m_Size.cy);
    }
  }

  AdjustSplitter();
  
  // csResult = CSize(300,300);
  return csResult;

/*
  CSize csResult;
  // Return default if it is being docked or floated
  if ((nMode & LM_VERTDOCK) || (nMode & LM_HORZDOCK)) {
    if (nMode & LM_STRETCH) {
      csResult = CSize((nMode & LM_HORZ) ? 32767 : m_Size.cx,
                       (nMode & LM_HORZ) ? m_Size.cy : 32767);
    } else {
      csResult = m_Size;
    }
  } else if (nMode & LM_MRUWIDTH) {
    csResult = m_Size;
  // In all other cases, accept the dynamic length
  } else {
    if (nMode & LM_LENGTHY) {
      csResult = CSize( m_Size.cx, m_Size.cy = nLength);
    } else {
      csResult = CSize( m_Size.cx = nLength, m_Size.cy);
    }
  }
  return csResult;
*/
}

INDEX CDlgTemplate::GetDockingSide()
{

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if(pMainFrame==NULL) return -1;

  CWnd *pParent = GetParent(); ASSERT(pParent!=NULL);
//CWnd *pParentsParent = pParent->GetParent(); ASSERT(pParentsParent!=NULL);

  CRect rcParent;
  CRect rcMainFrame;

  pParent->GetWindowRect(&rcParent);
  pMainFrame->GetWindowRect(&rcMainFrame);

  // is left docking enable
  if(dlg_ulEnabledDockingSides&CBRS_ALIGN_LEFT) {
    // check if docked to left side
    if(rcParent.left-4 == rcMainFrame.left) {
      return AFX_IDW_DOCKBAR_LEFT;
    }
  }

  // is right docking enable
  if(dlg_ulEnabledDockingSides&CBRS_ALIGN_RIGHT) {
    // check if docked to right side
    if(rcParent.right+4 == rcMainFrame.right) {
      return AFX_IDW_DOCKBAR_RIGHT;
    }
  }

  // is bottom docking enable
  if(dlg_ulEnabledDockingSides&CBRS_ALIGN_BOTTOM) {
    // check if docked to bottom side
    if(rcParent.bottom+4 == rcMainFrame.bottom) {
      return AFX_IDW_DOCKBAR_BOTTOM;
    }
  }

  if(dlg_ulEnabledDockingSides&CBRS_ALIGN_TOP) {
    ASSERT(FALSE);
  }
/*
  } else if(rcParent.right+4 == rcMainFrame.right) {
    return AFX_IDW_DOCKBAR_RIGHT;
  } else if(rcParent.bottom+4 == rcMainFrame.bottom) {
    return AFX_IDW_DOCKBAR_BOTTOM;
  } */

  // it could be also floating
  return AFX_IDW_DOCKBAR_BOTTOM;
/*
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if(pMainFrame==NULL) return -1;

  CWnd *pParent = GetParent();
  ASSERT(pParent!=NULL);
  CWnd *pParentsParent = pParent->GetParent();
  if(pParentsParent==NULL) {
    pParentsParent = pParent;
  }

  CRect rcParent;
  CRect rcMainFrame;

  pParent->GetWindowRect(&rcParent);
  pMainFrame->GetWindowRect(&rcMainFrame);

  if(rcParent.left-4 == rcMainFrame.left) {
    return AFX_IDW_DOCKBAR_LEFT;
  } else if(rcParent.right+4 == rcMainFrame.right) {
    return AFX_IDW_DOCKBAR_RIGHT;
  } else if(rcParent.bottom+4 == rcMainFrame.bottom) {
    return AFX_IDW_DOCKBAR_BOTTOM;
  } 

  // it could be also floating
  return AFX_IDW_DOCKBAR_TOP;
*/
}

void CDlgTemplate::AdjustSplitter()
{
  if(!theApp.bAppInitialized) return;
  if(dlg_iSplitterID==(-1)) return;

  INDEX iDockSide = GetDockingSide();
  INDEX iSplitterSide = AFX_IDW_DOCKBAR_FLOAT;
  if(iDockSide==AFX_IDW_DOCKBAR_LEFT) {
    iSplitterSide = AFX_IDW_DOCKBAR_RIGHT;
  } else if(iDockSide==AFX_IDW_DOCKBAR_RIGHT) {
    iSplitterSide = AFX_IDW_DOCKBAR_LEFT;
  } else if(iDockSide==AFX_IDW_DOCKBAR_TOP) {
    iSplitterSide = AFX_IDW_DOCKBAR_BOTTOM;
  } else if(iDockSide==AFX_IDW_DOCKBAR_BOTTOM) {
    iSplitterSide = AFX_IDW_DOCKBAR_TOP;
  }
  dlg_spSlitter.SetDockingSide(iSplitterSide);
}

void CDlgTemplate::EnableDockingSides(ULONG ulDockingSides)
{
  CDialogBar::EnableDocking(ulDockingSides);
  dlg_ulEnabledDockingSides = ulDockingSides;
}

void CDlgTemplate::DockCtrlBar()
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->DockControlBar(this);

  ASSERT(dlg_iSplitterID!=(-1));
  dlg_spSlitter.EnableDocking();
  dlg_bDockingEnabled = TRUE;
}

void CDlgTemplate::SetSplitterControlID(INDEX iSplitterID)
{
  // subclass splitter
  if(!dlg_spSlitter.SubclassDlgItem(iSplitterID,this)) {
    FatalError("Error in subclassing dlg item\n");
  }

  // AdjustSplitter();
  // remember ID
  dlg_iSplitterID = iSplitterID;
}

void CDlgTemplate::OnSize(UINT nType, int cx, int cy)
{
  AdjustSplitter();
	CDialogBar::OnSize(nType, cx, cy);
}

