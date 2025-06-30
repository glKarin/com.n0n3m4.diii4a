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

#if !defined(AFX_DLGSELECTMODE_H__F71966B3_C31A_11D1_8231_000000000000__INCLUDED_)
#define AFX_DLGSELECTMODE_H__F71966B3_C31A_11D1_8231_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgSelectMode.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectMode dialog

class CDlgSelectMode : public CDialog
{
public:
  CDisplayMode *m_pdm;
  CDisplayMode *m_pdmAvailableModes;
  INDEX m_ctAvailableDisplayModes;
  enum GfxAPIType *m_pGfxAPI;
// Construction
public:
	CDlgSelectMode( CDisplayMode &dm, enum GfxAPIType &gfxAPI, CWnd* pParent = NULL);
  ~CDlgSelectMode();
  void ApplySettings( CDisplayMode *pdm, enum GfxAPIType *pGfxAPI);

// Dialog Data
	//{{AFX_DATA(CDlgSelectMode)
	enum { IDD = IDD_SELECT_MODE_DIALOG };
	CComboBox	m_ctrlResCombo;
	CComboBox	m_ctrlDriverCombo;
	CString	m_strCurrentMode;
	CString	m_strCurrentDriver;
	int		m_iColor;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSelectMode)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSelectMode)
	afx_msg void OnTestButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGSELECTMODE_H__F71966B3_C31A_11D1_8231_000000000000__INCLUDED_)
