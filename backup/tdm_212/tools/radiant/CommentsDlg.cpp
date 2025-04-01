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
#include "CommentsDlg.h"


// CCommentsDlg dialog

IMPLEMENT_DYNAMIC(CCommentsDlg, CDialog)
CCommentsDlg::CCommentsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCommentsDlg::IDD, pParent)
	, strName(_T(""))
	, strPath(_T(""))
	, strComments(_T(""))
{
}

CCommentsDlg::~CCommentsDlg()
{
}

void CCommentsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_NAME, strName);
	DDX_Text(pDX, IDC_EDIT_PATH, strPath);
	DDX_Text(pDX, IDC_EDIT_COMMENTS, strComments);
}


BEGIN_MESSAGE_MAP(CCommentsDlg, CDialog)
END_MESSAGE_MAP()

	
// CCommentsDlg message handlers
