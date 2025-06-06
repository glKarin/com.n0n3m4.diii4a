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

// AnimComboBox.cpp : implementation file 
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnimComboBox

CAnimComboBox::CAnimComboBox()
{
  m_pvLastUpdatedView = NULL;
}

CAnimComboBox::~CAnimComboBox()
{
}


BEGIN_MESSAGE_MAP(CAnimComboBox, CComboBox)
	//{{AFX_MSG_MAP(CAnimComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnimComboBox message handlers

BOOL CAnimComboBox::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( (pModelerView == NULL) && (m_pvLastUpdatedView != NULL) )
  {
    m_pvLastUpdatedView = NULL;
    ResetContent();
    AddString( L"None available");
    SetCurSel( 0);
  }
  else if( (pModelerView != NULL) &&
           ( (m_pvLastUpdatedView != pModelerView) ||
             ( (pModelerView->m_ModelObject.GetAnim() != GetCurSel()) &&
               (GetDroppedState() == FALSE))))

  {
    m_pvLastUpdatedView = pModelerView;
    CAnimInfo aiInfo;
    ResetContent();
    for( INDEX i=0; i<pModelerView->m_ModelObject.GetAnimsCt(); i++)
    {
      pModelerView->m_ModelObject.GetAnimInfo( i, aiInfo);
      AddString( CString(aiInfo.ai_AnimName));
      SetCurSel( pModelerView->m_ModelObject.GetAnim());
    }
  }
  return TRUE;
}

void CAnimComboBox::OnSelchange() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView != NULL)
  {
    pModelerView->m_ModelObject.SetAnim( GetCurSel());
  }	
}

void CAnimComboBox::OnDropdown() 
{
  INDEX ctItems = GetCount();
  if( ctItems == CB_ERR) return;
  
  CRect rectCombo;
  GetWindowRect( &rectCombo);
  
  PIX pixScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
  PIX pixMaxHeight = pixScreenHeight - rectCombo.top;

  CWnd *pwndParent = GetParent();
  if( pwndParent == NULL) return;
  pwndParent->ScreenToClient( &rectCombo);
  PIX pixNewHeight = GetItemHeight(0)*(ctItems+2);
  rectCombo.bottom = rectCombo.top + ClampUp( pixNewHeight, pixMaxHeight);
  MoveWindow( rectCombo);
}
