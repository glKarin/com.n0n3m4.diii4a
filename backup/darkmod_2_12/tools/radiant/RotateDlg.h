/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#if !defined(AFX_ROTATEDLG_H__D4B79152_7A7E_11D1_B541_00AA00A410FC__INCLUDED_)
#define AFX_ROTATEDLG_H__D4B79152_7A7E_11D1_B541_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// RotateDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRotateDlg dialog

class CRotateDlg : public CDialog
{
// Construction
public:
	CRotateDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRotateDlg)
	enum { IDD = IDD_ROTATE };
	CSpinButtonCtrl	m_wndSpin3;
	CSpinButtonCtrl	m_wndSpin2;
	CSpinButtonCtrl	m_wndSpin1;
	CString	m_strX;
	CString	m_strY;
	CString	m_strZ;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRotateDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void ApplyNoPaint();

	// Generated message map functions
	//{{AFX_MSG(CRotateDlg)
	virtual void OnOK();
	afx_msg void OnApply();
	virtual BOOL OnInitDialog();
	afx_msg void OnDeltaposSpin1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpin2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpin3(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ROTATEDLG_H__D4B79152_7A7E_11D1_B541_00AA00A410FC__INCLUDED_)
