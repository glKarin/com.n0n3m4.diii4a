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

// DlgProgress.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgProgress.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgProgress dialog


CDlgProgress::CDlgProgress( CWnd* pParent /*=NULL*/, BOOL bCanCancel/*=FALSE*/)
	: CDialog(CDlgProgress::IDD, pParent)
{
  //{{AFX_DATA_INIT(CDlgProgress)
	m_strProgressMessage = _T("");
	//}}AFX_DATA_INIT

  m_bHasCancel = bCanCancel;
  m_bCancelPressed = FALSE;
}


void CDlgProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  if( m_bHasCancel)
  {
    GetDlgItem(IDCANCEL)->ShowWindow( SW_SHOW);
    GetDlgItem(IDCANCEL)->EnableWindow( TRUE);
  }
  else
  {
    GetDlgItem(IDCANCEL)->ShowWindow( SW_HIDE);
    GetDlgItem(IDCANCEL)->EnableWindow( FALSE);
  }

	//{{AFX_DATA_MAP(CDlgProgress)
	DDX_Control(pDX, IDC_PROGRESS, m_ctrlProgres);
	DDX_Text(pDX, IDC_PROGRESS_MESSAGE, m_strProgressMessage);
	//}}AFX_DATA_MAP
}

void CDlgProgress::SetProgressMessageAndPosition( char *strProgressMessage, INDEX iCurrentPos)
{
  // set new message
  m_strProgressMessage = strProgressMessage;
  // set current progress state
  m_ctrlProgres.SetPos( iCurrentPos);
  UpdateData( FALSE);
}

BEGIN_MESSAGE_MAP(CDlgProgress, CDialog)
	//{{AFX_MSG_MAP(CDlgProgress)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgProgress message handlers

void CDlgProgress::OnCancel() 
{
  m_bCancelPressed = TRUE;
}
