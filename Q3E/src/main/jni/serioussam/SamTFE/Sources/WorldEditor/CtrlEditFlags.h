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

#if !defined(AFX_CTRLEDITFLAGS_H__759EB1B4_55C4_11D5_86CA_00002103143B__INCLUDED_)
#define AFX_CTRLEDITFLAGS_H__759EB1B4_55C4_11D5_86CA_00002103143B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CtrlEditFlags.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditFlags window

class CCtrlEditFlags : public CButton
{
// Construction
public:
  CTString m_astrBitDescription[32];
	CCtrlEditFlags();
  CWnd *m_pDialog;
  ULONG m_ulValue;
  ULONG m_ulDefined;
  ULONG m_ulEditable;
  ULONG m_ulDefault;
  INDEX m_iCurrentBank;
  RECT m_rectButton;
  PIX m_dx;
  INDEX m_iLastArea;
  INDEX m_iMouseDownArea;

  void SetBitDescription(INDEX iBit, CTString strBitName);
  void SetDialogPtr( CWnd *pDialog);
  void SetDefaultValue(ULONG ulDefault);
  void SetEditableMask(ULONG ulEditable);
  void SetFlags(ULONG ulFlags);
  void MergeFlags(ULONG ulFlags);
  void SetPrevEditableBank( void);
  void SetNextEditableBank( void);
  void SetFirstEditableBank( void);
  CTString GetTipForArea(INDEX iArea) const;
  void ApplyChange(ULONG &ulOldFlags);
  RECT GetRectForArea(INDEX iArea) const;
  INDEX GetAreaUnderMouse( CPoint point) const;
  INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlEditFlags)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlEditFlags();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlEditFlags)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTRLEDITFLAGS_H__759EB1B4_55C4_11D5_86CA_00002103143B__INCLUDED_)
