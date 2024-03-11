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
#include "CurveDlg.h"


// CCurveDlg dialog

IMPLEMENT_DYNAMIC(CCurveDlg, CDialog)
CCurveDlg::CCurveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCurveDlg::IDD, pParent)
{
}

CCurveDlg::~CCurveDlg()
{
}

void CCurveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CURVES, comboCurve);
}

void CCurveDlg::OnOK() {
	UpdateData(TRUE);
	CString str;
	comboCurve.GetWindowText( str );
	strCurveType = str;
	CDialog::OnOK();
}

BEGIN_MESSAGE_MAP(CCurveDlg, CDialog)
END_MESSAGE_MAP()


// CCurveDlg message handlers
