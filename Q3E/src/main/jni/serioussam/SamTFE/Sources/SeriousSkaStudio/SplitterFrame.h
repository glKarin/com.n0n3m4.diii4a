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

#if !defined(AFX_SPLITTERFRAME_H__1F06C2AF_EF87_4728_9A83_7BD10DD1F70E__INCLUDED_)
#define AFX_SPLITTERFRAME_H__1F06C2AF_EF87_4728_9A83_7BD10DD1F70E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SplitterFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSplitterFrame frame
#define SPLITTER_WITDH 6

class CSplitterFrame : public CWnd
{
	DECLARE_DYNCREATE(CSplitterFrame)
public:
	CSplitterFrame();           // protected constructor used by dynamic creation
/*
  int GetPosX();
  int iStartPosX;
*/

  void ChangeParent(CWnd *pNewParent);
  void SetAbsPosition(CPoint pt);
  CPoint GetAbsPosition();
  void SetDockingSide(UINT uiDockSide);
  void SetSpliterSize(INDEX iNewSize);
  void SetSize(INDEX iWidth,INDEX iHeight);
  void UpdateParent();
  void EnableDocking();


// Attributes
protected:
  wchar_t *pchCursor;
  CWnd *pDockedParent;
  CWnd *pFloatingParent;
  CPoint sp_ptStartPoint;
  INDEX iSplitterSize;
  UINT  sp_uiDockSide;
  BOOL  sp_bDockingEnabled;
// Operations
public:
//	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSplitterFrame();

	// Generated message map functions
	//{{AFX_MSG(CSplitterFrame)
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERFRAME_H__1F06C2AF_EF87_4728_9A83_7BD10DD1F70E__INCLUDED_)
