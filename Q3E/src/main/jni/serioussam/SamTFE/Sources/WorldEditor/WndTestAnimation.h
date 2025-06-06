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

// WndTestAnimation.h : header file
//
#ifndef WNDTESTANIMATION_H
#define WNDTESTANIMATION_H 1

class CDlgLightAnimationEditor;

/////////////////////////////////////////////////////////////////////////////
// CWndTestAnimation window

class CWndTestAnimation : public CWnd
{
// Construction 
public:
	CWndTestAnimation();
	inline void SetParentDlg( CDlgLightAnimationEditor *pParentDlg) {m_pParentDlg=pParentDlg;};

// Attributes
public:
  CAnimObject m_aoAnimObject;
  CDlgLightAnimationEditor *m_pParentDlg;
  int m_iTimerID;
  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndTestAnimation)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndTestAnimation();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndTestAnimation)
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // WNDTESTANIMATION_H
