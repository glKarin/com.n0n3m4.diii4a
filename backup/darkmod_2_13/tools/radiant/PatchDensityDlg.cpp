/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "qe3.h"
#include "Radiant.h"
#include "PatchDensityDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchDensityDlg dialog


CPatchDensityDlg::CPatchDensityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchDensityDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPatchDensityDlg)
	//}}AFX_DATA_INIT
}


void CPatchDensityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatchDensityDlg)
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_wndWidth);
	DDX_Control(pDX, IDC_COMBO_HEIGHT, m_wndHeight);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPatchDensityDlg, CDialog)
	//{{AFX_MSG_MAP(CPatchDensityDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchDensityDlg message handlers

int g_nXLat[] = {3,5,7,9,11,13,15};

void CPatchDensityDlg::OnOK() 
{
  int nWidth = m_wndWidth.GetCurSel();
  int nHeight = m_wndHeight.GetCurSel();

  if (nWidth >= 0 && nWidth <= 6 && nHeight >= 0 && nHeight <= 6)
  {
	  Patch_GenericMesh(g_nXLat[nWidth], g_nXLat[nHeight], g_pParentWnd->ActiveXY()->GetViewType());
    Sys_UpdateWindows(W_ALL);
  }

  CDialog::OnOK();
}

BOOL CPatchDensityDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

  m_wndWidth.SetCurSel(0);
  m_wndHeight.SetCurSel(0);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
