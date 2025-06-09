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

// AxisListCtrl.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAxisListCtrl

CAxisListCtrl::CAxisListCtrl()
{
}

CAxisListCtrl::~CAxisListCtrl()
{
}


BEGIN_MESSAGE_MAP(CAxisListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CAxisListCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAxisListCtrl message handlers

void CAxisListCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // remember current state of controler's attributes
  ((CDlgPlayerControls *)GetParent())->UpdateData( TRUE);
	CListCtrl::OnLButtonDown(nFlags, point);
  // set state of new controler's attributes
	((CDlgPlayerControls *)GetParent())->UpdateData(FALSE);
}

void CAxisListCtrl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  // remember current state of controler's attributes
  ((CDlgPlayerControls *)GetParent())->UpdateData( TRUE);
	CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
  // set state of new controler's attributes
	((CDlgPlayerControls *)GetParent())->UpdateData(FALSE);
}

void CAxisListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  // remember current state of controler's attributes
  ((CDlgPlayerControls *)GetParent())->UpdateData( TRUE);
	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
  // set state of new controler's attributes
	((CDlgPlayerControls *)GetParent())->UpdateData(FALSE);
}


void CAxisListCtrl::OnSetFocus(CWnd* pOldWnd) 
{
  // get selected action
  INDEX iSelectedAction = GetNextItem( -1, LVNI_DROPHILITED);
  // if none is selected (initial state)
  if( iSelectedAction == -1)
  {
    iSelectedAction = 0;
    SetItemState( iSelectedAction, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
  }
  else
  {
    // clear hilighted state
    SetItemState( iSelectedAction, 0, LVIS_DROPHILITED);
  }

  CListCtrl::OnSetFocus(pOldWnd);
}

void CAxisListCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	CListCtrl::OnKillFocus(pNewWnd);

  // get selected action
  INDEX iSelectedAction = GetNextItem( -1, LVNI_SELECTED);
  // set hilighted state
  SetItemState( iSelectedAction, LVIS_DROPHILITED, LVIS_DROPHILITED);
}

BOOL CAxisListCtrl::PreTranslateMessage(MSG* pMsg) 
{
	// if return pressed
  if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
  {
    ((CDlgPlayerControls *)GetParent())->m_comboControlerAxis.SetFocus();
    // don't translate messages
    return TRUE;
  }
	return CListCtrl::PreTranslateMessage(pMsg);
}
