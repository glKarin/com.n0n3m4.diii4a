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
#if !defined(AFX_DIALOGINFO_H__81DF2A33_A552_11D1_B58E_00AA00A410FC__INCLUDED_)
#define AFX_DIALOGINFO_H__81DF2A33_A552_11D1_B58E_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DialogInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialogInfo dialog
void HideInfoDialog();
void ShowInfoDialog(const char* pText);

class CDialogInfo : public CDialog
{
// Construction
public:
	CDialogInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialogInfo)
	enum { IDD = IDD_DLG_INFORMATION };
	CEdit	m_wndInfo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialogInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialogInfo)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOGINFO_H__81DF2A33_A552_11D1_B58E_00AA00A410FC__INCLUDED_)
