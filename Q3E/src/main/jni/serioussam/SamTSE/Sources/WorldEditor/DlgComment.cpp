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

// DlgComment.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgComment.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgComment dialog


CDlgComment::CDlgComment(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgComment::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgComment)
	m_strComment = _T("");
	//}}AFX_DATA_INIT
}


void CDlgComment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgComment)
	DDX_Text(pDX, IDC_COMMENT, m_strComment);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgComment, CDialog)
	//{{AFX_MSG_MAP(CDlgComment)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgComment message handlers
