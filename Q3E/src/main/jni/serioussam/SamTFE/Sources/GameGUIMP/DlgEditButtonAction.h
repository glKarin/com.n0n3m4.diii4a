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

#if !defined(AFX_DLGEDITBUTTONACTION_H__3BD6B3E1_EE74_11D1_9AC2_00409580357B__INCLUDED_)
#define AFX_DLGEDITBUTTONACTION_H__3BD6B3E1_EE74_11D1_9AC2_00409580357B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgEditButtonAction.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgEditButtonAction dialog

class CDlgEditButtonAction : public CDialog
{
// Construction
public:
	CButtonAction *m_pbaButtonAction;
  CDlgEditButtonAction(CButtonAction *pbaButtonAction, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgEditButtonAction)
	enum { IDD = IDD_EDIT_BUTTON_ACTION };
	CString	m_strButtonActionName;
	CString	m_strButtonDownCommand;
	CString	m_strButtonUpCommand;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgEditButtonAction)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgEditButtonAction)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGEDITBUTTONACTION_H__3BD6B3E1_EE74_11D1_9AC2_00409580357B__INCLUDED_)
