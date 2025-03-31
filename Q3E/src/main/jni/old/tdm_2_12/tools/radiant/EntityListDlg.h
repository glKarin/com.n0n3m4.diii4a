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
#include "afxwin.h"
#if !defined(AFX_ENTITYLISTDLG_H__C241B9A3_819F_11D1_B548_00AA00A410FC__INCLUDED_)
#define AFX_ENTITYLISTDLG_H__C241B9A3_819F_11D1_B548_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EntityListDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEntityListDlg dialog

class CEntityListDlg : public CDialog
{
// Construction
public:
	CEntityListDlg(CWnd* pParent = NULL);   // standard constructor
	void UpdateList();
	static void ShowDialog();

// Dialog Data
	//{{AFX_DATA(CEntityListDlg)
	enum { IDD = IDD_DLG_ENTITYLIST };
	CListCtrl	m_lstEntity;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntityListDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEntityListDlg)
	afx_msg void OnSelect();
	afx_msg void OnClose();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID,  LPARAM lParam);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	CListBox listEntities;
	afx_msg void OnLbnSelchangeListEntities();
	afx_msg void OnLbnDblclkListEntities();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTITYLISTDLG_H__C241B9A3_819F_11D1_B548_00AA00A410FC__INCLUDED_)
