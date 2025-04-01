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
#if !defined(NEWTEXWND_H)
#define NEWTEXWND_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TexWnd.h : header file
//
#include "../../renderer/tr_local.h"
//#include "texwnd.h"

/////////////////////////////////////////////////////////////////////////////
// CTexWnd window

class CNewTexWnd : public CWnd
{
  DECLARE_DYNCREATE(CNewTexWnd);
// Construction
public:
	CNewTexWnd();
  void UpdateFilter(const char* pFilter);
  void UpdatePrefs();
  void FocusEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewTexWnd)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void EnsureTextureIsVisible(const char *name);
	void LoadMaterials();
	virtual ~CNewTexWnd();
	BOOL OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult );
	int CNewTexWnd::OnToolHitTest(CPoint point, TOOLINFO * pTI);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
  //CTexEdit m_wndFilter;
  //CButton  m_wndShaders;
  bool m_bNeedRange;
	HGLRC hglrcTexture;
	CDC	 *hdcTexture;
	CPoint cursor;
	CPoint origin;
	CPoint draw;
	CPoint drawRow;
	CPoint current;
	CRect rectClient;
	int currentRow;
	int currentIndex;
	idList<const idMaterial*> materialList;

	// Generated message map functions
protected:
	const idMaterial* NextPos();
	const idMaterial *getMaterialAtPoint(CPoint point);
	void InitPos();
	//{{AFX_MSG(CNewTexWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	afx_msg void OnShaderClick();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(NEWTEXWND_H)
