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

// DlgNumericAlpha.cpp : implementation file
//

#include "stdafx.h"
#include "DlgNumericAlpha.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgNumericAlpha dialog


CDlgNumericAlpha::CDlgNumericAlpha(int iAlpha, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgNumericAlpha::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgNumericAlpha)
	m_iAlpha = 0;
	//}}AFX_DATA_INIT
  m_iAlpha = iAlpha;
}


void CDlgNumericAlpha::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgNumericAlpha)
	DDX_Text(pDX, IDC_ALPHA, m_iAlpha);
	DDV_MinMaxInt(pDX, m_iAlpha, 0, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgNumericAlpha, CDialog)
	//{{AFX_MSG_MAP(CDlgNumericAlpha)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgNumericAlpha message handlers
