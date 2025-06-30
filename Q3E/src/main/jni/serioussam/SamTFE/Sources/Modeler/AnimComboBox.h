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

// AnimComboBox.h : header file
//
#ifndef ANIMCOMBOBOX_H
#define ANIMCOMBOBOX_H 1

/////////////////////////////////////////////////////////////////////////////
// CAnimComboBox window

class CAnimComboBox : public CComboBox
{
// Construction
public:
	BOOL OnIdle(LONG lCount);
	CModelerView *m_pvLastUpdatedView;
	CAnimComboBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAnimComboBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAnimComboBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CAnimComboBox)
	afx_msg void OnSelchange();
	afx_msg void OnDropdown();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // ANIMCOMBOBOX_H
