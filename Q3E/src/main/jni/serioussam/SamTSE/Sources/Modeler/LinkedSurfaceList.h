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

// LinkedSurfaceList.h : header file
//

#ifndef LINKEDSURFACELIST_H
#define LINKEDSURFACELIST_H 1

/////////////////////////////////////////////////////////////////////////////
// CLinkedSurfaceList window

class CLinkedSurfaceList : public CCheckListBox
{
// Construction
public:
	CLinkedSurfaceList();
  CDialog *m_pdlgParentDialog;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLinkedSurfaceList)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLinkedSurfaceList();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLinkedSurfaceList)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // LINKEDSURFACELIST_H
