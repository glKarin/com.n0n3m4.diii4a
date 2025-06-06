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

// PressKeyEditControl.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPressKeyEditControl

CPressKeyEditControl::CPressKeyEditControl()
{
}

CPressKeyEditControl::~CPressKeyEditControl()
{
}


BEGIN_MESSAGE_MAP(CPressKeyEditControl, CEdit)
	//{{AFX_MSG_MAP(CPressKeyEditControl)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPressKeyEditControl message handlers


BOOL CPressKeyEditControl::PreTranslateMessage(MSG* pMsg) 
{
  // if direct input is curently on
  if( _pInput->IsInputEnabled())
  {
	  // and if we caught alt key message
    if( pMsg->message==WM_SYSKEYDOWN)
    {
      // get key data
      int lKeyData = pMsg->lParam;
      // test if it is ghost Alt-F4 situation
      if( lKeyData & (1L<<29))
      {
	      // don't continue translating the message
        return TRUE;
      }
    }
    // test if game is paused, stop messages otherwise let messages trough
    if (pMsg->message==WM_KEYDOWN) 
    {
      // direct input is enabled, so don't translate messages
      return TRUE;
    }
  }
	
	return CEdit::PreTranslateMessage(pMsg);
}
