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

// PaletteDialog.h : header file
//
#ifndef COLORS_PALLETE_DIALOG_H
#define COLORS_PALLETE_DIALOG_H 1

/////////////////////////////////////////////////////////////////////////////
// CPaletteDialog dialog
#include "ChoosedColorButton.h"
#include "PaletteButton.h"

class CPaletteDialog : public CDialog
{
// Construction
public:
	CModelerView *m_LastViewUpdated;
  BOOL OnIdle(LONG lCount);
	CPaletteDialog(CWnd* pParent = NULL);   // standard constructor
// Dialog Data
	//{{AFX_DATA(CPaletteDialog)
	enum { IDD = IDD_COLORS_PALETTE };
	CChoosedColorButton	m_ChoosedColorButton;
	CButton	m_PickColor;
	CPaletteButton	m_ColorPaletteButton9;
	CPaletteButton	m_ColorPaletteButton8;
	CPaletteButton	m_ColorPaletteButton7;
	CPaletteButton	m_ColorPaletteButton6;
	CPaletteButton	m_ColorPaletteButton5;
	CPaletteButton	m_ColorPaletteButton;
	CPaletteButton	m_ColorPaletteButton32;
	CPaletteButton	m_ColorPaletteButton31;
	CPaletteButton	m_ColorPaletteButton30;
	CPaletteButton	m_ColorPaletteButton3;
	CPaletteButton	m_ColorPaletteButton29;
	CPaletteButton	m_ColorPaletteButton28;
	CPaletteButton	m_ColorPaletteButton27;
	CPaletteButton	m_ColorPaletteButton26;
	CPaletteButton	m_ColorPaletteButton25;
	CPaletteButton	m_ColorPaletteButton24;
	CPaletteButton	m_ColorPaletteButton23;
	CPaletteButton	m_ColorPaletteButton22;
	CPaletteButton	m_ColorPaletteButton21;
	CPaletteButton	m_ColorPaletteButton20;
	CPaletteButton	m_ColorPaletteButton2;
	CPaletteButton	m_ColorPaletteButton19;
	CPaletteButton	m_ColorPaletteButton18;
	CPaletteButton	m_ColorPaletteButton17;
	CPaletteButton	m_ColorPaletteButton16;
	CPaletteButton	m_ColorPaletteButton15;
	CPaletteButton	m_ColorPaletteButton14;
	CPaletteButton	m_ColorPaletteButton13;
	CPaletteButton	m_ColorPaletteButton12;
	CPaletteButton	m_ColorPaletteButton11;
	CPaletteButton	m_ColorPaletteButton10;
	CPaletteButton	m_ColorPaletteButton1;
	CString	m_ColorName;
	CString	m_ModeString;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPaletteDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPaletteDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditColorName();
	//}}AFX_MSG

  afx_msg void OnColorPalleteButton(UINT nID);
  afx_msg void OnUpdateColorPalleteButton(CCmdUI* pCmdUI);
  
	DECLARE_MESSAGE_MAP()
};

#endif // COLORS_PALLETE_DIALOG_H
