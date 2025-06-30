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

// LinkedSurfaceList.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLinkedSurfaceList

CLinkedSurfaceList::CLinkedSurfaceList()
{
}

CLinkedSurfaceList::~CLinkedSurfaceList()
{
}


BEGIN_MESSAGE_MAP(CLinkedSurfaceList, CCheckListBox)
	//{{AFX_MSG_MAP(CLinkedSurfaceList)
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLinkedSurfaceList message handlers

void CLinkedSurfaceList::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  INDEX ctItems = GetCount();

  CRect rectItem;
  for( INDEX iItem=0; iItem<ctItems; iItem++)
  {
    GetItemRect( iItem, &rectItem);
    if( rectItem.PtInRect( point))  SetCheck( iItem, 1);
    else                            SetCheck( iItem, 0);
  }
  if( m_pdlgParentDialog != NULL) 
  {
    m_pdlgParentDialog->UpdateData( TRUE);
    m_pdlgParentDialog->EndDialog( IDOK);
  }

  CCheckListBox::OnLButtonDblClk(nFlags, point);
}

BOOL CLinkedSurfaceList::PreTranslateMessage(MSG* pMsg) 
{
  if( (pMsg->message==WM_KEYDOWN) && (pMsg->wParam=='Z') )
  {
    if( m_pdlgParentDialog != NULL) 
    {
      m_pdlgParentDialog->EndDialog( IDCANCEL);
      return FALSE;
    }
  }

	return CCheckListBox::PreTranslateMessage(pMsg);
}
