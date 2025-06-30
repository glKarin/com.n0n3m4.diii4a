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

// DlgProgress.h : header file
//

#ifndef DLGPROGRESS_H
#define DLGPROGRESS_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgProgress dialog

class CDlgProgress : public CDialog
{
// Construction
public:
	CDlgProgress(CWnd* pParent = NULL, BOOL bCanCancel=FALSE);   // standard constructor
  void SetProgressMessageAndPosition( char *strProgressMessage, INDEX iCurrentPos);
  
  BOOL m_bCancelPressed;
  BOOL m_bHasCancel;

// Dialog Data
	//{{AFX_DATA(CDlgProgress)
	enum { IDD = IDD_PROGRESS_DIALOG };
	CProgressCtrl	m_ctrlProgres;
	CString	m_strProgressMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgProgress)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgProgress)
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif  // DLGPROGRESS_H
