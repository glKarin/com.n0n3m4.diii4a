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
#include "CameraTargetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCameraTargetDlg dialog


CCameraTargetDlg::CCameraTargetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCameraTargetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCameraTargetDlg)
	m_nType = 0;
	m_strName = _T("");
	//}}AFX_DATA_INIT
}


void CCameraTargetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCameraTargetDlg)
	DDX_Radio(pDX, IDC_RADIO_FIXED, m_nType);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCameraTargetDlg, CDialog)
	//{{AFX_MSG_MAP(CCameraTargetDlg)
	ON_COMMAND(ID_POPUP_NEWCAMERA_FIXED, OnPopupNewcameraFixed)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCameraTargetDlg message handlers

void CCameraTargetDlg::OnPopupNewcameraFixed() 
{
	// TODO: Add your command handler code here
	
}
