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

// DlgMarkLinkedSurfaces.h : header file
//

#ifndef DLGMARKLINKEDSURFACES_H
#define DLGMARKLINKEDSURFACES_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgMarkLinkedSurfaces dialog

class CDlgMarkLinkedSurfaces : public CDialog
{
// Construction
public:
  CDlgMarkLinkedSurfaces( CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgMarkLinkedSurfaces)
	enum { IDD = IDD_LINKED_SURFACES };
	CLinkedSurfaceList	m_listSurfaces;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgMarkLinkedSurfaces)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgMarkLinkedSurfaces)
	virtual BOOL OnInitDialog();
	afx_msg void OnClearSelection();
	afx_msg void OnSelectAll();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif //DLGMARKLINKEDSURFACES_H
