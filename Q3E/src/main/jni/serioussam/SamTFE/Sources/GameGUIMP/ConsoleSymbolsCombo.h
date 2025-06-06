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

#if !defined(AFX_CONSOLESYMBOLSCOMBO_H__E417EA91_1B23_11D2_834D_004095812ACC__INCLUDED_)
#define AFX_CONSOLESYMBOLSCOMBO_H__E417EA91_1B23_11D2_834D_004095812ACC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConsoleSymbolsCombo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConsoleSymbolsCombo window

class CConsoleSymbolsCombo : public CComboBox
{
// Construction
public:
	CConsoleSymbolsCombo();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsoleSymbolsCombo)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConsoleSymbolsCombo();

	// Generated message map functions
protected:
	//{{AFX_MSG(CConsoleSymbolsCombo)
	afx_msg void OnSelchange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONSOLESYMBOLSCOMBO_H__E417EA91_1B23_11D2_834D_004095812ACC__INCLUDED_)
