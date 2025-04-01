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
#pragma once
#include "afxwin.h"


// CCommentsDlg dialog

class CCommentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CCommentsDlg)

public:
	CCommentsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCommentsDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_COMMENTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString strName;
	CString strPath;
	CString strComments;
};
