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
#if !defined(AFX_FINDTEXTUREDLG_H__34B75D32_9F3A_11D1_B570_00AA00A410FC__INCLUDED_)
#define AFX_FINDTEXTUREDLG_H__34B75D32_9F3A_11D1_B570_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// FindTextureDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFindTextureDlg dialog

class CFindTextureDlg : public CDialog
{
// Construction
public:
	static void setReplaceStr(const char* p);
	static void setFindStr(const char* p);
	static bool isOpen();
  static void show();
  static void updateTextures(const char* p);
	CFindTextureDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFindTextureDlg)
	enum { IDD = IDD_DIALOG_FINDREPLACE };
	BOOL	m_bSelectedOnly;
	CString	m_strFind;
	CString	m_strReplace;
	BOOL	m_bForce;
	BOOL	m_bLive;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindTextureDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFindTextureDlg)
	afx_msg void OnBtnApply();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSetfocusEditFind();
	afx_msg void OnSetfocusEditReplace();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINDTEXTUREDLG_H__34B75D32_9F3A_11D1_B570_00AA00A410FC__INCLUDED_)
