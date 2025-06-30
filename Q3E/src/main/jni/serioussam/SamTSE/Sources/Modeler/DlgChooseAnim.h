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

// DChooseAnim.h : header file
//
#ifndef DLGCHOOSEANIM_H
#define DLGCHOOSEANIM_H 1

/////////////////////////////////////////////////////////////////////////////
// CDChooseAnim dialog

class CDChooseAnim : public CDialog
{
// Construction
public:
	CAnimObject *m_pAnimObject;
	CDChooseAnim(CAnimObject *pAO, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDChooseAnim)
	enum { IDD = IDD_CHOOSEANIMATION };
	CListBox	m_ListBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDChooseAnim)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDChooseAnim)
	afx_msg void OnDblclkList1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // DLGCHOOSEANIM_H
