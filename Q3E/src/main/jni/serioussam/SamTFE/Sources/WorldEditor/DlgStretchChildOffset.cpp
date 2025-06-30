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

// DlgStretchChildOffset.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgStretchChildOffset.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgStretchChildOffset dialog


CDlgStretchChildOffset::CDlgStretchChildOffset(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgStretchChildOffset::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgStretchChildOffset)
	m_fStretchValue = 0.0f;
	//}}AFX_DATA_INIT
	m_fStretchValue = 1.0f;
}


void CDlgStretchChildOffset::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgStretchChildOffset)
	DDX_Text(pDX, IDC_CHILD_STRETCH, m_fStretchValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgStretchChildOffset, CDialog)
	//{{AFX_MSG_MAP(CDlgStretchChildOffset)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgStretchChildOffset message handlers
