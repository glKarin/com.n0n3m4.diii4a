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

// VirtualTreeCtrl.h : header file
//
#ifndef VIRTUALTREECTRL_H
#define VIRTUALTREECTRL_H 1

/////////////////////////////////////////////////////////////////////////////
// CVirtualTreeCtrl window

class CBrowser;

class CVirtualTreeCtrl : public CTreeCtrl
{
// Construction
public:
	CVirtualTreeCtrl();

// Attributes
public:
  CBrowser *m_pBrowser;
  COleDataSource m_DataSource;
  BOOL m_bIsOpen;

// Operations
public:
  void OpenTreeCtrl(void);
  void CloseTreeCtrl(void);
  void SetBrowserPtr( CBrowser *pBrowser);
	void OnContextMenu( CPoint point);
  CVirtualTreeNode *ItemForCoordinate(CPoint pt);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVirtualTreeCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVirtualTreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CVirtualTreeCtrl)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // VIRTUALTREECTRL_H
