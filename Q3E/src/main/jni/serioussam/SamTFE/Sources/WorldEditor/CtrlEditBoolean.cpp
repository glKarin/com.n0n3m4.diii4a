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

// CtrlEditBoolean.cpp : implementation file
//

#include "stdafx.h"
#include "CtrlEditBoolean.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditBoolean

CCtrlEditBoolean::CCtrlEditBoolean()
{
}

CCtrlEditBoolean::~CCtrlEditBoolean()
{
}

void CCtrlEditBoolean::SetDialogPtr( CWnd *pDialog)
{
  m_pDialog = pDialog;
}

BEGIN_MESSAGE_MAP(CCtrlEditBoolean, CButton)
	//{{AFX_MSG_MAP(CCtrlEditBoolean)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditBoolean message handlers

void CCtrlEditBoolean::OnClicked() 
{
  // don't do anything if document doesn't exist
  if( theApp.GetDocument() == NULL) return;
  // because this is three state button, we will make it working as two-state, but
  // it will still be able to be grayed
  if( GetCheck() == 2)
  {
    SetCheck( 0);
  }
  theApp.GetDocument()->m_chSelections.MarkChanged();
  // mark that document is changed
  theApp.GetDocument()->SetModifiedFlag( TRUE);
  // update dialog data (to reflect data change)
	m_pDialog->UpdateData( TRUE);
}
