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

#if !defined(AFX_MDICLIENTWND_H__BA0F202B_D135_430A_A145_5068DD6DE1F2__INCLUDED_)
#define AFX_MDICLIENTWND_H__BA0F202B_D135_430A_A145_5068DD6DE1F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MDIClientWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMDIClientWnd window

class CMDIClientWnd : public CWnd
{
// Construction
public:
	CMDIClientWnd();

// Attributes
public:
  CRect ClientRect;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMDIClientWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMDIClientWnd();
  void GetCurrentRect(CRect &rc);
  void SetCurrentRect(CRect &rc);

	// Generated message map functions
protected:
	//{{AFX_MSG(CMDIClientWnd)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MDICLIENTWND_H__BA0F202B_D135_430A_A145_5068DD6DE1F2__INCLUDED_)
