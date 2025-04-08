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
#include "DialogInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogInfo dialog
CDialogInfo g_dlgInfo;

void ShowInfoDialog(const char* pText)
{
  if (g_dlgInfo.GetSafeHwnd())
  {
    g_dlgInfo.m_wndInfo.SetWindowText(pText);
    g_dlgInfo.ShowWindow(SW_SHOW);
  }
  else
  {
    g_dlgInfo.Create(IDD_DLG_INFORMATION);
    g_dlgInfo.m_wndInfo.SetWindowText(pText);
    g_dlgInfo.ShowWindow(SW_SHOW);
  }
  g_pParentWnd->SetFocus();
}

void HideInfoDialog()
{
  if (g_dlgInfo.GetSafeHwnd())
    g_dlgInfo.ShowWindow(SW_HIDE);
}


CDialogInfo::CDialogInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialogInfo)
	//}}AFX_DATA_INIT
}


void CDialogInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogInfo)
	DDX_Control(pDX, IDC_EDIT1, m_wndInfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialogInfo, CDialog)
	//{{AFX_MSG_MAP(CDialogInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogInfo message handlers

BOOL CDialogInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
