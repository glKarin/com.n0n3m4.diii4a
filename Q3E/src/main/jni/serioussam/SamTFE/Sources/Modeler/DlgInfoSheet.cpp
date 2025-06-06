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

// DlgInfoSheet.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define INFO_WIDTH 200

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoSheet

IMPLEMENT_DYNAMIC(CDlgInfoSheet, CPropertySheet)

#define CALLACTIVEPAGE(function, parameter)         \
  if( pgActivPage == &m_PgInfoGlobal)               \
    m_PgInfoGlobal.function( parameter);            \
  else if( pgActivPage == &m_PgInfoMip)             \
    m_PgInfoMip.function( parameter);               \
  else if( pgActivPage == &m_PgInfoNone)            \
    m_PgInfoNone.function( parameter);              \
  else if( pgActivPage == &m_PgInfoPos)             \
    m_PgInfoPos.function( parameter);               \
  else if( pgActivPage == &m_PgInfoRendering)       \
    m_PgInfoRendering.function( parameter);         \
  else if( pgActivPage == &m_PgInfoAnim)            \
    m_PgInfoAnim.function( parameter);              \
  else if( pgActivPage == &m_PgInfoCollision)       \
    m_PgInfoCollision.function( parameter);         \
  else if( pgActivPage == &m_PgAttachingPlacement)  \
    m_PgAttachingPlacement.function( parameter);    \
  else if( pgActivPage == &m_PgAttachingSound)      \
    m_PgAttachingSound.function( parameter);

CDlgInfoSheet::CDlgInfoSheet(CWnd* pWndParent)
	: CPropertySheet(AFX_IDS_APP_TITLE, pWndParent)
{
  m_LastViewUpdated = NULL;
  m_InfoMode = -1;

  // Add all pages so frame could get bounding sizes of all of them
  AddPage( &m_PgInfoNone);
	AddPage( &m_PgInfoGlobal);
	AddPage( &m_PgInfoMip);
	AddPage( &m_PgInfoRendering);
	AddPage( &m_PgInfoPos);
	AddPage( &m_PgInfoAnim);
	AddPage( &m_PgInfoCollision);
	AddPage( &m_PgAttachingPlacement);
	AddPage( &m_PgAttachingSound);
  SetActivePage(0);
}

CDlgInfoSheet::CDlgInfoSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CDlgInfoSheet::CDlgInfoSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CDlgInfoSheet::~CDlgInfoSheet()
{
}


BEGIN_MESSAGE_MAP(CDlgInfoSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CDlgInfoSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoSheet message handlers

void CDlgInfoSheet::PostNcDestroy() 
{
	CPropertySheet::PostNcDestroy();
	delete this;
}

void CDlgInfoSheet::DoDataExchange(CDataExchange* pDX) 
{
  CPropertyPage *pgActivPage = GetActivePage();

  CALLACTIVEPAGE(UpdateData, pDX->m_bSaveAndValidate);

	CPropertySheet::DoDataExchange(pDX);
}

static INDEX iLastNormalPage = 2;
static INDEX iLastMappingPage = 2;

BOOL CDlgInfoSheet::OnIdle(LONG lCount)
{
  CModelerDoc* pDoc = theApp.GetDocument();
  CModelerView *pAnyView = CModelerView::GetActiveMappingNormalView();
  CModelerView *pModelerView = CModelerView::GetActiveView();
  CModelerView *pModelerMappingView = CModelerView::GetActiveMappingView();
  INDEX ctPages = GetPageCount();
  
  if( m_InfoMode == IM_NORMAL) iLastNormalPage = GetPageIndex(GetActivePage());
  if( m_InfoMode == IM_MAPPING) iLastMappingPage = GetPageIndex(GetActivePage());

  // If no view found and info mode is not IM_NONE
  if( pAnyView == NULL)
  {
    if( m_InfoMode != IM_NONE)
    {
      HWND hwndActive = ::GetActiveWindow();
      m_InfoMode = IM_NONE;
      m_LastViewUpdated = NULL;
      
      for( INDEX iPage=0; iPage<ctPages; iPage++) RemovePage( 0);
      AddPage(&m_PgInfoNone);
      SetActivePage( 0);
      ::SetActiveWindow( hwndActive);
    }
    return TRUE;
  }
  else if( pModelerMappingView != NULL)
  {
    if( (m_InfoMode != IM_MAPPING) ||
        (ctPages != 3) )
    {
      HWND hwndActive = ::GetActiveWindow();
      m_InfoMode = IM_MAPPING;
      for( INDEX iPage=0; iPage<ctPages; iPage++) RemovePage( 0);
      AddPage(&m_PgInfoGlobal);
      AddPage(&m_PgInfoMip);
  	  AddPage(&m_PgInfoRendering);

      ctPages = GetPageCount();
      if( iLastMappingPage<ctPages) SetActivePage( iLastMappingPage);
      else                          SetActivePage( 0);
      ::SetActiveWindow( hwndActive);
    }
  }
  else if( m_InfoMode != IM_NORMAL)
  {
    HWND hwndActive = ::GetActiveWindow();
    m_InfoMode = IM_NORMAL;
    for( INDEX iPage=0; iPage<ctPages; iPage++) RemovePage( 0);
  	AddPage(&m_PgInfoGlobal);
  	AddPage(&m_PgInfoMip);
  	AddPage(&m_PgInfoRendering);
    AddPage(&m_PgInfoPos);
    AddPage(&m_PgInfoCollision);
    AddPage(&m_PgAttachingPlacement);
    AddPage(&m_PgAttachingSound);
    AddPage(&m_PgInfoAnim);
    ctPages = GetPageCount();
    if( iLastNormalPage<ctPages) SetActivePage( iLastNormalPage);
    else                         SetActivePage( 0);
    ::SetActiveWindow( hwndActive);
  }
  
  BOOL bRefresh = FALSE;
  if( GetActivePage() == &m_PgInfoAnim) bRefresh = TRUE;
  if(m_LastViewUpdated != pAnyView) bRefresh = TRUE;
  m_LastViewUpdated = pAnyView;
  if(!theApp.m_chGlobal.IsUpToDate( m_Updateable)) bRefresh = TRUE;
  if(!pAnyView->m_ModelObject.IsUpToDate( m_Updateable)) bRefresh = TRUE;

  if( bRefresh)
  {
    UpdateData( FALSE);
    m_Updateable.MarkUpdated();
  }

  CPropertyPage *pgActivPage = GetActivePage();
  CALLACTIVEPAGE(OnIdle, lCount);
  return TRUE;
}

BOOL CDlgInfoSheet::PreTranslateMessage(MSG* pMsg) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
	if(pMsg->message==WM_KEYDOWN)
  {
    // if any of string edit controls has input
    BOOL bEditingString = 
      (::IsWindow(m_PgAttachingPlacement.m_hWnd)&&
      m_PgAttachingPlacement.GetDlgItem( IDC_ATTACHING_PLACEMENT_NAME) == CWnd::GetFocus()) ||
      (::IsWindow(m_PgInfoCollision.m_hWnd)&&
      m_PgInfoCollision.GetDlgItem( IDC_COLLISION_BOX_NAME) == CWnd::GetFocus());
      
    if( (pMsg->wParam==VK_SPACE) && !bEditingString)
    {
      return TRUE;
    }
	  else if( pMsg->wParam=='Q' && !bEditingString)
    {
      pMainFrame->ToggleInfoWindow();
      return TRUE;
    }
	  else if( pMsg->wParam=='C' && !bEditingString)
    {
      CustomSetActivePage( &m_PgInfoCollision);
      return FALSE;
    }
	  else if( pMsg->wParam=='R' && !bEditingString)
    {
      CustomSetActivePage( &m_PgInfoRendering);
      return TRUE;
    }
	  else if( pMsg->wParam=='A' && !bEditingString)
    {
      CustomSetActivePage( &m_PgAttachingPlacement);
      return TRUE;
    }
	  else if( pMsg->wParam=='S' && !bEditingString)
    {
      CustomSetActivePage( &m_PgAttachingSound);
      return TRUE;
    }
	  else if( pMsg->wParam=='P' && !bEditingString)
    {
      CustomSetActivePage( &m_PgInfoPos);
      return TRUE;
    }
	  else if( pMsg->wParam=='G' && !bEditingString)
    {
      CustomSetActivePage( &m_PgInfoGlobal);
      return TRUE;
    }
	  else if( pMsg->wParam=='M' && !bEditingString)
    {
      CustomSetActivePage( &m_PgInfoMip);
      return TRUE;
    }
	  else if( pMsg->wParam=='N' && !bEditingString)
    {
      CustomSetActivePage( &m_PgInfoAnim);
      return TRUE;
    }
    
    if( (pMsg->wParam>='A') && (pMsg->wParam<='Z') && !bEditingString)
    {
      return TRUE;
    }
  }
	return CPropertySheet::PreTranslateMessage(pMsg);
}

void CDlgInfoSheet::CustomSetActivePage( CPropertyPage *pppToActivate)
{
  if( GetActivePage() == pppToActivate) return;
  INDEX ctPages = GetPageCount();
  for( INDEX iPage=0; iPage < ctPages; iPage++)
  {
    if( pppToActivate == GetPage( iPage))
    {
      CPropertySheet::SetActivePage( iPage);
      return;
    }
  }
}
