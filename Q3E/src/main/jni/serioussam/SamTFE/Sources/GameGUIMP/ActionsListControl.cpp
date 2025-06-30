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

// ActionsListControl.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionsListControl

CActionsListControl::CActionsListControl()
{
}

CActionsListControl::~CActionsListControl()
{
}


BEGIN_MESSAGE_MAP(CActionsListControl, CListCtrl)
	//{{AFX_MSG_MAP(CActionsListControl)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_BUTTON_ACTION_ADD, OnButtonActionAdd)
	ON_COMMAND(ID_BUTTON_ACTION_EDIT, OnButtonActionEdit)
	ON_COMMAND(ID_BUTTON_ACTION_REMOVE, OnButtonActionRemove)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ACTION_EDIT, OnUpdateButtonActionEdit)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_ACTION_REMOVE, OnUpdateButtonActionRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionsListControl message handlers

void CActionsListControl::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // get no of items
  INDEX iButtonsCt = ((CDlgPlayerControls *)GetParent())->m_listButtonActions.GetItemCount();
  // get no of items
  for( INDEX iListItem=0; iListItem<iButtonsCt; iListItem++)
  {
    ((CDlgPlayerControls *)GetParent())->m_listButtonActions.SetItemState( iListItem, 0, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
  }

  CListCtrl::OnLButtonDown(nFlags, point);

  /*
  // select wanted item
  ((CDlgPlayerControls *)GetParent())->m_listButtonActions.SetItemState( iSelectedButton+1, 
    LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
  ((CDlgPlayerControls *)GetParent())->m_listButtonActions.EnsureVisible( iSelectedButton+1, FALSE); 
  ((CDlgPlayerControls *)GetParent())->m_listButtonActions.SetFocus();
  */
	((CDlgPlayerControls *)GetParent())->SetFirstAndSecondButtonNames();
}

void CActionsListControl::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
	((CDlgPlayerControls *)GetParent())->SetFirstAndSecondButtonNames();
}

void CActionsListControl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
	((CDlgPlayerControls *)GetParent())->SetFirstAndSecondButtonNames();
}


void CActionsListControl::OnSetFocus(CWnd* pOldWnd) 
{
  // get selected action
  INDEX iSelectedAction = GetNextItem( -1, LVIS_SELECTED);
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

void CActionsListControl::OnKillFocus(CWnd* pNewWnd) 
{
	CListCtrl::OnKillFocus(pNewWnd);

  // get selected action
  INDEX iSelectedAction = GetNextItem( -1, LVNI_SELECTED);
  // set hilighted state
  SetItemState( iSelectedAction, LVIS_DROPHILITED, LVIS_DROPHILITED);
}	


BOOL CActionsListControl::PreTranslateMessage(MSG* pMsg) 
{
	// if return pressed
  if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
  {
    ((CDlgPlayerControls *)GetParent())->m_editFirstControl.SetFocus();
    // don't translate messages
    return TRUE;
  }
	return CListCtrl::PreTranslateMessage(pMsg);
}

void CActionsListControl::OnContextMenu(CWnd* pWnd, CPoint point) 
{
  CMenu menu;
  if( menu.LoadMenu( IDR_BUTTON_ACTION_POPUP))
  {
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
    pPopup->TrackPopupMenu( TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
          								  point.x, point.y, this);
  }
}


void CActionsListControl::OnButtonActionAdd() 
{
  CDlgPlayerControls *pDlg = (CDlgPlayerControls *)GetParent();
  // create new button action
  CButtonAction *pbaAddedButtonAction = &pDlg->m_ctrlControls.AddButtonAction();
  // call edit button dialog
  CDlgEditButtonAction dlgEditButtonAction( pbaAddedButtonAction);
  dlgEditButtonAction.DoModal();
  // refresh list of button actions
  pDlg->FillActionsList();
}

void CActionsListControl::OnButtonActionEdit() 
{
  // obtain selected button action
  CButtonAction *pbaToEdit = ((CDlgPlayerControls *)GetParent())->GetSelectedButtonAction();
  ASSERT( pbaToEdit != NULL);

  // call edit button dialog
  CDlgEditButtonAction dlgEditButtonAction( pbaToEdit);
  dlgEditButtonAction.DoModal();

  // refresh list of button actions
  ((CDlgPlayerControls *)GetParent())->FillActionsList();
}

void CActionsListControl::OnButtonActionRemove() 
{
  // obtain selected button action
  CButtonAction *pbaToRemove = ((CDlgPlayerControls *)GetParent())->GetSelectedButtonAction();
  ASSERT( pbaToRemove != NULL);
  ((CDlgPlayerControls *)GetParent())->m_ctrlControls.RemoveButtonAction( *pbaToRemove);

  // refresh list of button actions
  ((CDlgPlayerControls *)GetParent())->FillActionsList();
}

void CActionsListControl::OnUpdateButtonActionEdit(CCmdUI* pCmdUI) 
{
  // obtain selected button action
  CButtonAction *pbaToEdit = ((CDlgPlayerControls *)GetParent())->GetSelectedButtonAction();
  pCmdUI->Enable( pbaToEdit != NULL);
}

void CActionsListControl::OnUpdateButtonActionRemove(CCmdUI* pCmdUI) 
{
  // obtain selected button action
  CButtonAction *pbaToEdit = ((CDlgPlayerControls *)GetParent())->GetSelectedButtonAction();
  pCmdUI->Enable( pbaToEdit != NULL);
}
