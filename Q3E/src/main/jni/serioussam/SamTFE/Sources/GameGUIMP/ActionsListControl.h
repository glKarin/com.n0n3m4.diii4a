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

// ActionsListControl.h : header file
//

#ifndef ACTIONSLISTCONTROL_H
#define ACTIONSLISTCONTROL_H 1

/////////////////////////////////////////////////////////////////////////////
// CActionsListControl window

class CActionsListControl : public CListCtrl
{
// Construction
public:
	CActionsListControl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActionsListControl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CActionsListControl();

	// Generated message map functions
//protected:
public:
	//{{AFX_MSG(CActionsListControl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnButtonActionAdd();
	afx_msg void OnButtonActionEdit();
	afx_msg void OnButtonActionRemove();
	afx_msg void OnUpdateButtonActionEdit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonActionRemove(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // ACTIONSLISTCONTROL_H 1
