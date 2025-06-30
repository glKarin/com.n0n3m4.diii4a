/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#if !defined(AFX_DLGTREESHORTCUTS_H__A1C15CF3_D8A2_11D1_8270_000000000000__INCLUDED_)
#define AFX_DLGTREESHORTCUTS_H__A1C15CF3_D8A2_11D1_8270_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgTreeShortcuts.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgTreeShortcuts dialog

class CDlgTreeShortcuts : public CDialog
{
// Construction
public:
  INDEX m_iPressedShortcut;
  CDlgTreeShortcuts(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgTreeShortcuts)
	enum { IDD = IDD_TREE_SHORTCUTS };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgTreeShortcuts)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
  afx_msg void OnTreeShortcut(UINT nID);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgTreeShortcuts)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGTREESHORTCUTS_H__A1C15CF3_D8A2_11D1_8270_000000000000__INCLUDED_)
