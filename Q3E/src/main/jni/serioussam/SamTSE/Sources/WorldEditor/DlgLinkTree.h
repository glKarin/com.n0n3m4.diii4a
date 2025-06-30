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

#if !defined(AFX_DLGLINKTREE_H__F16898A5_5BF2_11D5_86D8_00002103143B__INCLUDED_)
#define AFX_DLGLINKTREE_H__F16898A5_5BF2_11D5_86D8_00002103143B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgLinkTree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgLinkTree dialog

class CDlgLinkTree : public CDialog 
{
// Construction
public:
	CDlgLinkTree(CEntity *pen, CPoint pt, BOOL bWhoTargets, BOOL bPropertyNames, CWnd* pParent = NULL);
  void AddEntityPtrsRecursiv(CEntity *pen, HTREEITEM hParent, CTString strPropertyName);
  void ExpandTree(HTREEITEM pItem, BOOL bExpand, INDEX iMaxLevel=-1, BOOL bNoNextSibling=FALSE);
  void ExpandRecursivly(HTREEITEM pItem, BOOL bExpand, BOOL bNoNextSibling);
  void CalculateOccupiedSpace(HTREEITEM hItem, CRect &rect);
  INDEX GetItemLevel(HTREEITEM item);
  void SetNewWindowOrigin(void);
  void InitializeTree(void);
  CPoint m_pt;
  CEntity *m_pen;
  HTREEITEM m_HitItem;
  CPoint m_ptLastMouse;
  CPoint m_ptMouseDown;
  CRect m_rectWndOnMouseDown;
// Dialog Data
	//{{AFX_DATA(CDlgLinkTree)
	enum { IDD = IDD_LINK_TREE };
	CTreeCtrl	m_ctrTree;
	BOOL	m_bClass;
	BOOL	m_bName;
	BOOL	m_bProperty;
	BOOL	m_bWho;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgLinkTree)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgLinkTree)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkLinkTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLtContractAll();
	afx_msg void OnLtContractBranch();
	afx_msg void OnLtExpandAll();
	afx_msg void OnLtExpandBranch();
	afx_msg void OnLtLeaveBranch();
	afx_msg void OnLtLastLevel();
	afx_msg void OnLtClass();
	afx_msg void OnLtName();
	afx_msg void OnLtProperty();
	afx_msg void OnLtWho();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGLINKTREE_H__F16898A5_5BF2_11D5_86D8_00002103143B__INCLUDED_)
