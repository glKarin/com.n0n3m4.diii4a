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

// DlgDisplaceMapSize.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgDisplaceMapSize dialog


CDlgDisplaceMapSize::CDlgDisplaceMapSize(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgDisplaceMapSize::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgDisplaceMapSize)
	m_pixWidth = 0;
	m_pixHeight = 0;
	m_bMidPixSample = FALSE;
	m_bHighResolution = FALSE;
	//}}AFX_DATA_INIT

  m_pixWidth = 33;
	m_pixHeight = 33;
}


void CDlgDisplaceMapSize::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgDisplaceMapSize)
	DDX_Text(pDX, IDC_EDIT_DISPLACE_X_SIZE, m_pixWidth);
	DDV_MinMaxUInt(pDX, m_pixWidth, 1, 1024);
	DDX_Text(pDX, IDC_EDIT_DISPLACE_Y_SIZE, m_pixHeight);
	DDV_MinMaxUInt(pDX, m_pixHeight, 1, 1024);
	DDX_Check(pDX, IDC_EDIT_DISPLACE_MIDPIXELSAMPLING, m_bMidPixSample);
	DDX_Check(pDX, IDC_EDIT_DISPLACE_16BITRESOLUTION, m_bHighResolution);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgDisplaceMapSize, CDialog)
	//{{AFX_MSG_MAP(CDlgDisplaceMapSize)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDisplaceMapSize message handlers
