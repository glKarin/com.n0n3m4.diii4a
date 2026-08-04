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
#include "DialogThick.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogThick dialog


CDialogThick::CDialogThick(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogThick::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialogThick)
	m_bSeams = TRUE;
	m_nAmount = 8;
	//}}AFX_DATA_INIT
}


void CDialogThick::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogThick)
	DDX_Check(pDX, IDC_CHECK_SEAMS, m_bSeams);
	DDX_Text(pDX, IDC_EDIT_AMOUNT, m_nAmount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialogThick, CDialog)
	//{{AFX_MSG_MAP(CDialogThick)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogThick message handlers
