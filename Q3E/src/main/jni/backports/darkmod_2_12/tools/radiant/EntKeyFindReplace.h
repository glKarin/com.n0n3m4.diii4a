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
#if !defined(AFX_ENTKEYFINDREPLACE_H__1AE54C31_FC22_11D3_8A60_00500424438B__INCLUDED_)
#define AFX_ENTKEYFINDREPLACE_H__1AE54C31_FC22_11D3_8A60_00500424438B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EntKeyFindReplace.h : header file
//


// return vals for modal dialogue, any values will do that don't clash with the first 9 or so defined by IDOK etc
//
#define ID_RET_REPLACE	100
#define ID_RET_FIND		101


/////////////////////////////////////////////////////////////////////////////
// CEntKeyFindReplace dialog

class CEntKeyFindReplace : public CDialog
{
// Construction
public:
	CEntKeyFindReplace(CString* p_strFindKey, 
					   CString* p_strFindValue, 
					   CString* p_strReplaceKey, 
					   CString* p_strReplaceValue, 
					   bool*	p_bWholeStringMatchOnly,
					   bool*	p_bSelectAllMatchingEnts,
					   CWnd*	pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEntKeyFindReplace)
	enum { IDD = IDD_ENTFINDREPLACE };
	CString	m_strFindKey;
	CString	m_strFindValue;
	CString	m_strReplaceKey;
	CString	m_strReplaceValue;
	BOOL	m_bWholeStringMatchOnly;
	BOOL	m_bSelectAllMatchingEnts;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntKeyFindReplace)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEntKeyFindReplace)
	virtual void OnCancel();
	afx_msg void OnReplace();
	afx_msg void OnFind();
	afx_msg void OnKeycopy();
	afx_msg void OnValuecopy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CString* m_pStrFindKey;
	CString* m_pStrFindValue;
	CString* m_pStrReplaceKey;
	CString* m_pStrReplaceValue;
	bool*	 m_pbWholeStringMatchOnly;
	bool*	 m_pbSelectAllMatchingEnts;

	void CopyFields();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTKEYFINDREPLACE_H__1AE54C31_FC22_11D3_8A60_00500424438B__INCLUDED_)
