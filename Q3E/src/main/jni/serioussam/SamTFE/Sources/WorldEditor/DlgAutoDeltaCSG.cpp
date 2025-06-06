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

// DlgAutoDeltaCSG.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgAutoDeltaCSG.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgAutoDeltaCSG dialog


CDlgAutoDeltaCSG::CDlgAutoDeltaCSG(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAutoDeltaCSG::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAutoDeltaCSG)
	m_ctNumberOfClones = 0;
	//}}AFX_DATA_INIT
}


void CDlgAutoDeltaCSG::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // if dialog is receiving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    // get last used number of CSG clones
    m_ctNumberOfClones = theApp.GetProfileInt(L"World editor", L"Number of CSG clones", 2);
  }

	//{{AFX_DATA_MAP(CDlgAutoDeltaCSG)
	DDX_Text(pDX, IDC_NO_OF_CLONES, m_ctNumberOfClones);
	DDV_MinMaxUInt(pDX, m_ctNumberOfClones, 1, 100);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgAutoDeltaCSG, CDialog)
	//{{AFX_MSG_MAP(CDlgAutoDeltaCSG)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgAutoDeltaCSG message handlers
