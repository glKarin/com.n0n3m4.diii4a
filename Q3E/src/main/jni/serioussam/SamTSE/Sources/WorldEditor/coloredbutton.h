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

// ColoredButton.h : header file
//
#ifndef COLOREDBUTTON_H
#define COLOREDBUTTON_H 1

/////////////////////////////////////////////////////////////////////////////
// CColoredButton window

class CColoredButton : public CButton
{
// Construction
public:
  enum PickerType {
    PT_CUSTOM = 0,
    PT_MFC,
  };

	CColoredButton();

// Attributes
public:
  COLOR m_colColor;
  COLOR m_colLastColor;
  UBYTE m_ubComponents[2][4];
  BOOL m_bMixedColor;
  enum PickerType m_ptPickerType;
  CWnd *m_pwndParentDialog;
  RECT m_rectButton;
  PIX m_dx;
  PIX m_dy;
  INDEX m_iColorIndex;
  INDEX m_iComponentIndex;
  CPoint m_ptStarting;
  CPoint m_ptCenter;
// Operations
public:
  void SetColor(COLOR clrNew);
  inline void SetDialogPtr(CWnd *pwndParentDialog){ m_pwndParentDialog = pwndParentDialog;};
  inline void SetMixedColor(void){ m_bMixedColor = TRUE;};
  inline void SetPickerType( enum PickerType ptPickerType) { m_ptPickerType = ptPickerType;};
  inline COLOR GetColor(void) { return m_colColor;};
  inline BOOL IsColorValid(void) { return !m_bMixedColor;};
  void SetOverButtonInfo( CPoint point);
  void ColorToComponents(void);
  INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColoredButton)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColoredButton();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColoredButton)
	afx_msg void OnClicked();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnCopyColor();
	afx_msg void OnPasteColor();
	afx_msg void OnNumericAlpha();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // COLOREDBUTTON_H
