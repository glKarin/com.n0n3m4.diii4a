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

// DlgSelectByName.h : header file
//
#ifndef DLGSELECTBYNAME_H
#define DLGSELECTBYNAME_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectByName dialog
class CWorldEditorDoc;

class CDlgSelectByName : public CDialog
{
// Construction
public:
	// place to store document ptr
  CWorldEditorDoc *m_pDoc;
  CDlgSelectByName(CWorldEditorDoc *pDoc, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgSelectByName)
	enum { IDD = IDD_SELECT_BY_NAME };
	CEntitySelectionListBox	m_ListBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSelectByName)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSelectByName)
	virtual BOOL OnInitDialog();
	afx_msg void OnDeselectAll();
	afx_msg void OnSelectAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // DLGSELECTBYNAME_H
