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
#if !defined(AFX_WAITDLG_H__2B7A6C91_8D3F_4BEE_B564_33A0CFFA241B__INCLUDED_)
#define AFX_WAITDLG_H__2B7A6C91_8D3F_4BEE_B564_33A0CFFA241B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WaitDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog

class CWaitDlg : public CDialog
{
// Construction
public:
	CWaitDlg(CWnd* pParent = NULL, const char *msg = "Wait...");   // standard constructor
	~CWaitDlg();
	void SetText(const char *msg, bool append = false);
	void AllowCancel( bool enable );
	bool CancelPressed( void );

// Dialog Data
	//{{AFX_DATA(CWaitDlg)
	enum { IDD = IDD_DLG_WAIT };
	CString	waitStr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWaitDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWaitDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	idStr	text;
	bool	cancelPressed;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WAITDLG_H__2B7A6C91_8D3F_4BEE_B564_33A0CFFA241B__INCLUDED_)
