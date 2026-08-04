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
#include "CapDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCapDialog dialog


CCapDialog::CCapDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCapDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCapDialog)
	m_nCap = 0;
	//}}AFX_DATA_INIT
}


void CCapDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCapDialog)
	DDX_Radio(pDX, IDC_RADIO_CAP, m_nCap);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCapDialog, CDialog)
	//{{AFX_MSG_MAP(CCapDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCapDialog message handlers
