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

// CtrlEnumCombo.cpp : implementation file
//

#include "stdafx.h"
#include "CtrlEnumCombo.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtrlEnumCombo

CCtrlEnumCombo::CCtrlEnumCombo()
{
}

CCtrlEnumCombo::~CCtrlEnumCombo()
{
}

void CCtrlEnumCombo::SetDialogPtr( CPropertyComboBar *pDialog)
{
  m_pDialog = pDialog;
}


BEGIN_MESSAGE_MAP(CCtrlEnumCombo, CComboBox)
	//{{AFX_MSG_MAP(CCtrlEnumCombo)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtrlEnumCombo message handlers

void CCtrlEnumCombo::OnSelchange() 
{
  theApp.GetDocument()->SetModifiedFlag( TRUE);
  // update dialog data (to reflect data change)
	m_pDialog->UpdateData( TRUE);
	m_pDialog->SetIntersectingEntityClassName();
  m_pDialog->ArrangeControls();
	m_pDialog->UpdateData( FALSE);
}

void CCtrlEnumCombo::OnDropdown() 
{
  INDEX ctItems = GetCount();
  if( ctItems == CB_ERR) return;
  
  CRect rectCombo;
  GetWindowRect( &rectCombo);
  
  PIX pixScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);
  PIX pixMaxHeight = pixScreenHeight - rectCombo.top;

  m_pDialog->ScreenToClient( &rectCombo);
  PIX pixNewHeight = GetItemHeight(0)*(ctItems+2);
  rectCombo.bottom = rectCombo.top + ClampUp( pixNewHeight, pixMaxHeight);
  MoveWindow( rectCombo);
}
