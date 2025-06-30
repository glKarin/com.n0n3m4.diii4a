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

// CtrlBrowseFile.h : header file
//
#ifndef CTRLBROWSEFILE_H
#define CTRLBROWSEFILE_H 1

class CPropertyComboBar;
class CWorldEditorDoc;
/////////////////////////////////////////////////////////////////////////////
// CCtrlBrowseFile window

class CCtrlBrowseFile : public CButton
{
// Construction
public:
	CCtrlBrowseFile();

// Attributes
public:
  // ptr to parent dialog
  CPropertyComboBar *m_pDialog;
  BOOL m_bFileNameNoDep;

// Operations
public:
  // sets ptr to parent dialog
  void SetDialogPtr( CPropertyComboBar *pDialog);
  CTFileName GetIntersectingFile();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlBrowseFile)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlBrowseFile();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlBrowseFile)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // CTRLBROWSEFILE_H
