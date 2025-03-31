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



#include "../../sys/win32/rc/common_resource.h"
#include "DialogName.h"

/////////////////////////////////////////////////////////////////////////////
// DialogName dialog


DialogName::DialogName(const char *pName, CWnd* pParent /*=NULL*/)
	: CDialog(DialogName::IDD, pParent)
{
	//{{AFX_DATA_INIT(DialogName)
	m_strName = _T("");
	//}}AFX_DATA_INIT
	m_strCaption = pName;
}


void DialogName::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DialogName)
	DDX_Text(pDX, IDC_TOOLS_EDITNAME, m_strName);
	//}}AFX_DATA_MAP
}

BOOL DialogName::OnInitDialog() 
{
	CDialog::OnInitDialog();

	SetWindowText(m_strCaption);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(DialogName, CDialog)
	//{{AFX_MSG_MAP(DialogName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DialogName message handlers

void DialogName::OnOK() 
{
	CDialog::OnOK();
}
