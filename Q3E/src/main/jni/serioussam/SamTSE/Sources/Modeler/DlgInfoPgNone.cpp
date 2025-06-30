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

// DlgInfoPgNone.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgNone property page


CDlgInfoPgNone::CDlgInfoPgNone() : CPropertyPage(CDlgInfoPgNone::IDD)
{
	//{{AFX_DATA_INIT(CDlgInfoPgNone)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
  theApp.m_pPgInfoNone = this;
}


void CDlgInfoPgNone::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfoPgNone)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgInfoPgNone, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgInfoPgNone)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgNone message handlers

BOOL CDlgInfoPgNone::OnIdle(LONG lCount)
{
  // refresh info frame size
  ((CMainFrame *)( theApp.m_pMainWnd))->m_pInfoFrame->SetSizes();
  return TRUE;
}

