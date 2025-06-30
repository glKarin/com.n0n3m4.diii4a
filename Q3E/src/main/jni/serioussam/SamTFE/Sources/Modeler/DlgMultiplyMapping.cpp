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

// DlgMultiplyMapping.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgMultiplyMapping dialog


CDlgMultiplyMapping::CDlgMultiplyMapping(
  FLOAT3D f3InitialValues /*= FLOAT3D(1.0f, 1.0f, 1.0f)*/, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMultiplyMapping::IDD, pParent)
{
  m_f3MultiplyValues = f3InitialValues;
  
  //{{AFX_DATA_INIT(CDlgMultiplyMapping)
	m_fMultiplyXOffset = 0.0f;
	m_fMultiplyYOffset = 0.0f;
	m_fMultiplyZoom = 0.0f;
	//}}AFX_DATA_INIT
}


void CDlgMultiplyMapping::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  
  // if dialog is recieving data
  if( !pDX->m_bSaveAndValidate)
  {
	  m_fMultiplyXOffset = m_f3MultiplyValues(1);
	  m_fMultiplyYOffset = m_f3MultiplyValues(2);
	  m_fMultiplyZoom = m_f3MultiplyValues(3);
  }

	//{{AFX_DATA_MAP(CDlgMultiplyMapping)
	DDX_SkyFloat(pDX, IDC_SURFACE_X_OFFSET, m_fMultiplyXOffset);
	DDX_SkyFloat(pDX, IDC_SURFACE_Y_OFFSET, m_fMultiplyYOffset);
	DDX_SkyFloat(pDX, IDC_SURFACE_ZOOM, m_fMultiplyZoom);
	//}}AFX_DATA_MAP
  
  // if dialog is giving data
  if( pDX->m_bSaveAndValidate)
  {
	  m_f3MultiplyValues(1) = m_fMultiplyXOffset;
	  m_f3MultiplyValues(2) = m_fMultiplyYOffset;
	  m_f3MultiplyValues(3) = m_fMultiplyZoom;
  }
}


BEGIN_MESSAGE_MAP(CDlgMultiplyMapping, CDialog)
	//{{AFX_MSG_MAP(CDlgMultiplyMapping)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgMultiplyMapping message handlers
