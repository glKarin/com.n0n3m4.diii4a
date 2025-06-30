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

// CtrlEditString.cpp : implementation file
//

#include "stdafx.h"
#include "CtrlEditString.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditString

CCtrlEditString::CCtrlEditString()
{
}

CCtrlEditString::~CCtrlEditString()
{
}

void CCtrlEditString::SetDialogPtr( CPropertyComboBar *pDialog)
{
  m_pDialog = pDialog;
}

BEGIN_MESSAGE_MAP(CCtrlEditString, CEdit)
	//{{AFX_MSG_MAP(CCtrlEditString)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditString message handlers

void CCtrlEditString::OnChange() 
{
}

BOOL CCtrlEditString::PreTranslateMessage(MSG* pMsg)
{
	// if we caught key down message
  if( pMsg->message==WM_KEYDOWN)
  {
    if((int)pMsg->wParam==VK_RETURN)
    {
      // don't do anything if document doesn't exist
      if( theApp.GetDocument() == NULL) return TRUE;
      // mark that document is changed
      theApp.GetDocument()->SetModifiedFlag( TRUE);
      theApp.GetDocument()->m_chSelections.MarkChanged();
      // update dialog data (to reflect data change)
	    m_pDialog->UpdateData( TRUE);
    } else {
      TranslateMessage(pMsg);
      SendMessage( WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
    }    
    return TRUE;
  }

	return CEdit::PreTranslateMessage(pMsg);
}
