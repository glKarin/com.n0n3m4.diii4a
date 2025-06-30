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

#if !defined(AFX_CHECKBOX_H__E1BBA3F7_AEB5_47EA_9593_99FA773F60C0__INCLUDED_)
#define AFX_CHECKBOX_H__E1BBA3F7_AEB5_47EA_9593_99FA773F60C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CheckBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCheckBox window

class CCheckBox : public CButton
{
// Construction
public:
	CCheckBox();
  void SetIndex(INDEX iFlagIndex, ULONG ulFlags);

// Attributes
public:
  CTString m_strID;  // ID of control
  INDEX    m_iIndex; // Index of flag in ulong
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCheckBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCheckBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCheckBox)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHECKBOX_H__E1BBA3F7_AEB5_47EA_9593_99FA773F60C0__INCLUDED_)
