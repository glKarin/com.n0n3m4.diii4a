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

// CtrlEditString.h : header file
//
#ifndef CTRLEDITSTRING_H
#define CTRLEDITSTRING_H 1

class CPropertyComboBar;
/////////////////////////////////////////////////////////////////////////////
// CCtrlEditString window

class CCtrlEditString : public CEdit
{
// Construction
public:
	CCtrlEditString();

// Attributes
public:
  // ptr to parent dialog
  CPropertyComboBar *m_pDialog;

// Operations
public:
  // sets ptr to parent dialog
  void SetDialogPtr( CPropertyComboBar *pDialog);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlEditString)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlEditString();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlEditString)
	afx_msg void OnChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif  // CTRLEDITSTRING_H
