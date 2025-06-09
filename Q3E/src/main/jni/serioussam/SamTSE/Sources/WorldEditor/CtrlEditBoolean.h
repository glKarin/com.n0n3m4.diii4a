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

// CtrlEditBoolean.h : header file
//
#ifndef CTRLEDITBOOLEAN_H
#define CTRLEDITBOOLEAN_H 1

class CPropertyComboBar;
/////////////////////////////////////////////////////////////////////////////
// CCtrlEditBoolean window

class CCtrlEditBoolean : public CButton
{
// Construction
public:
	CCtrlEditBoolean();

// Attributes
public:
  // ptr to parent dialog
  CWnd *m_pDialog;

// Operations
public:
  // sets ptr to parent dialog
  void SetDialogPtr( CWnd *pDialog);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlEditBoolean)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlEditBoolean();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlEditBoolean)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif  // CTRLEDITBOOLEAN_H
