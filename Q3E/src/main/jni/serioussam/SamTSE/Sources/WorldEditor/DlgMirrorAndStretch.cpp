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

// DlgMirrorAndStretch.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgMirrorAndStretch.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgMirrorAndStretch dialog


CDlgMirrorAndStretch::CDlgMirrorAndStretch(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMirrorAndStretch::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgMirrorAndStretch)
	m_fStretch = 0.0f;
	m_iMirror = -1;
	//}}AFX_DATA_INIT
	
  m_fStretch = 1.0f;
	m_iMirror = 0;
}


void CDlgMirrorAndStretch::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgMirrorAndStretch)
	DDX_Text(pDX, IDC_STRETCH, m_fStretch);
	DDX_Radio(pDX, IDC_MIRROR_NONE, m_iMirror);
	//}}AFX_DATA_MAP

  ASSERT(IsWindow(m_hWnd));
  SetWindowText(CString(m_strName));
}


BEGIN_MESSAGE_MAP(CDlgMirrorAndStretch, CDialog)
	//{{AFX_MSG_MAP(CDlgMirrorAndStretch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgMirrorAndStretch message handlers
