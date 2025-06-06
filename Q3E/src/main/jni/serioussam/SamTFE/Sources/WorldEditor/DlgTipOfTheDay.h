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

#if !defined(AFX_DLGTIPOFTHEDAY_H__F7006AF6_44F1_11D4_93A2_004095812ACC__INCLUDED_)
#define AFX_DLGTIPOFTHEDAY_H__F7006AF6_44F1_11D4_93A2_004095812ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgTipOfTheDay.h : header file
//

#include "CtlTipOfTheDayText.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgTipOfTheDay dialog

class CDlgTipOfTheDay : public CDialog
{
public:
  CStaticStackArray<CTString> m_astrTips;
// Construction
public:
	CDlgTipOfTheDay(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgTipOfTheDay)
	enum { IDD = IDD_TIPOFTHEDAY };
	CCtlTipOfTheDayText	m_wndTipText;
	BOOL	m_bShowTipsAtStartup;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgTipOfTheDay)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgTipOfTheDay)
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnClose();
	afx_msg void OnNextTip();
	afx_msg void OnPrevTip();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGTIPOFTHEDAY_H__F7006AF6_44F1_11D4_93A2_004095812ACC__INCLUDED_)
