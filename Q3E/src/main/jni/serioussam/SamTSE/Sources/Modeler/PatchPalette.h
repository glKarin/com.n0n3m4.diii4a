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

// PatchPalette.h : header file
//
#ifndef PATCH_PALLETE_DIALOG_H
#define PATCH_PALLETE_DIALOG_H 1

/////////////////////////////////////////////////////////////////////////////
// CPatchPalette dialog

class CPatchPalette : public CDialog
{
// Construction
public:
	CPatchPalette(CWnd* pParent = NULL);   // standard constructor
  BOOL OnIdle(LONG lCount);           
  CModelerView *m_LastViewUpdated;
  HICON m_PatchExistIcon;
  HICON m_PatchActiveIcon;
  HICON m_PatchInactiveIcon;

// Dialog Data
	//{{AFX_DATA(CPatchPalette)
	enum { IDD = IDD_PATCH_PALETTE };
	CPatchPaletteButton	m_PatchButton9;
	CPatchPaletteButton	m_PatchButton8;
	CPatchPaletteButton	m_PatchButton7;
	CPatchPaletteButton	m_PatchButton6;
	CPatchPaletteButton	m_PatchButton5;
	CPatchPaletteButton	m_PatchButton4;
	CPatchPaletteButton	m_PatchButton32;
	CPatchPaletteButton	m_PatchButton31;
	CPatchPaletteButton	m_PatchButton30;
	CPatchPaletteButton	m_PatchButton3;
	CPatchPaletteButton	m_PatchButton29;
	CPatchPaletteButton	m_PatchButton28;
	CPatchPaletteButton	m_PatchButton27;
	CPatchPaletteButton	m_PatchButton26;
	CPatchPaletteButton	m_PatchButton25;
	CPatchPaletteButton	m_PatchButton24;
	CPatchPaletteButton	m_PatchButton23;
	CPatchPaletteButton	m_PatchButton22;
	CPatchPaletteButton	m_PatchButton21;
	CPatchPaletteButton	m_PatchButton20;
	CPatchPaletteButton	m_PatchButton2;
	CPatchPaletteButton	m_PatchButton19;
	CPatchPaletteButton	m_PatchButton18;
	CPatchPaletteButton	m_PatchButton17;
	CPatchPaletteButton	m_PatchButton16;
	CPatchPaletteButton	m_PatchButton15;
	CPatchPaletteButton	m_PatchButton14;
	CPatchPaletteButton	m_PatchButton13;
	CPatchPaletteButton	m_PatchButton12;
	CPatchPaletteButton	m_PatchButton11;
	CPatchPaletteButton	m_PatchButton10;
	CPatchPaletteButton	m_PatchButton1;
	CString	m_PatchName;
	float	m_fStretch;
	CString	m_strPatchFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPatchPalette)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPatchPalette)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditPatchName();
	afx_msg void OnChangeEditPatchStretch();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // PATCH_PALLETE_DIALOG_H
