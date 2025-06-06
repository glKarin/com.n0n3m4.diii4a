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

// WndAnimationFrames.h : header file
//

#ifndef WNDANIMATIONFRAMES_H
#define WNDANIMATIONFRAMES_H 1

class CDlgLightAnimationEditor;

/////////////////////////////////////////////////////////////////////////////
// CWndAnimationFrames window

class CWndAnimationFrames : public CWnd
{
// Construction
public:
	CWndAnimationFrames();
	inline void SetParentDlg( CDlgLightAnimationEditor *pParentDlg) {m_pParentDlg=pParentDlg;};
	void DeleteSelectedFrame( void);
  INDEX GetFrame( INDEX iFramePosition);
  BOOL IsFrameVisible(INDEX iFrame);
  BOOL IsSelectedFrameKeyFrame(void);
  void ScrollLeft(void);
  void ScrollRight(void);
  void ScrollPgLeft(void);
  void ScrollPgRight(void);

// Attributes
public:
  INDEX m_iFramesInLine;
  INDEX m_iStartingFrame;
  INDEX m_iSelectedFrame;
  CDlgLightAnimationEditor *m_pParentDlg;
  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndAnimationFrames)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndAnimationFrames();

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndAnimationFrames)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // WNDANIMATIONFRAMES_H
