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

// DlgPgGlobal.h : header file
//
#ifndef DLGPGGLOBAL_H
#define DLGPGGLOBAL_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgPgGlobal dialog

class CDlgPgGlobal : public CPropertyPage
{
	DECLARE_DYNCREATE(CDlgPgGlobal)

// Construction
public:
	CDlgPgGlobal();
	~CDlgPgGlobal();
  BOOL OnIdle(LONG lCount);
	CActiveTextureWnd	m_wndActiveTexture;

  CUpdateableRT m_udSelectionCounts;

// Dialog Data
	//{{AFX_DATA(CDlgPgGlobal)
	enum { IDD = IDD_PG_GLOBAL };
	CString	m_strTextureInfo;
	CString	m_strSelectedEntitiesCt;
	CString	m_strSelectedPolygonsCt;
	CString	m_strSelectedSectorsCt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDlgPgGlobal)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlgPgGlobal)
	virtual BOOL OnInitDialog();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
#endif // DLGPGGLOBAL_H
