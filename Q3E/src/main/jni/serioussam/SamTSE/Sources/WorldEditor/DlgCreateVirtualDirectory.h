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

// DlgCreateVirtualDirectory.h : header file
//
#ifndef DLGCREATEVIRTUALDIRECTORY_H
#define DLGCREATEVIRTUALDIRECTORY_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateVirtualDirectory dialog

class CDlgCreateVirtualDirectory : public CDialog
{
// Construction
public:
	CDlgCreateVirtualDirectory(CTString strOldName = CTString(""),
    CTString strTitle = CTString("Create virtual directory"), CWnd* pParent = NULL);

  CTString m_strTitle;
  CTString m_strCreatedDirName;
  CImageList m_IconsImageList;
  INDEX m_iSelectedIconType;

// Dialog Data
	//{{AFX_DATA(CDlgCreateVirtualDirectory)
	enum { IDD = IDD_CREATE_VIRTUAL_DIRECTORY };
	CListCtrl	m_DirectoryIconsList;
	CString	m_strDirectoryName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreateVirtualDirectory)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCreateVirtualDirectory)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkDirectoryIconList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // DLGCREATEVIRTUALDIRECTORY_H
