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

#if !defined(AFX_DLGCREATEEFFECTTEXTURE_H__C517CED4_FA6C_11D1_82E9_000000000000__INCLUDED_)
#define AFX_DLGCREATEEFFECTTEXTURE_H__C517CED4_FA6C_11D1_82E9_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgCreateEffectTexture.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateEffectTexture dialog

class CDlgCreateEffectTexture : public CDialog
{
// Construction
public:
	CDlgCreateEffectTexture(CTFileName fnInputFile=CTString(""),CWnd* pParent = NULL);   // standard constructor
	~CDlgCreateEffectTexture();   // standard destructor
  void SetNewBaseTexture( CTFileName fnNewBase);
  void SelectPixSizeCombo(void);
  void InitializeSizeCombo(void);
  void InitializeEffectTypeCombo(void);
  void CreateTexture( void);

  BOOL m_bPreviewWindowsCreated;
  MEX m_mexInitialCreatedWidth;
  PIX m_pixInitialCreatedWidth;
  PIX m_pixInitialCreatedHeight;
  CTFileName m_fnCreatedTextureName;
  CWndDisplayTexture m_wndViewCreatedTexture;
  CTextureData m_tdCreated;

// Dialog Data
	//{{AFX_DATA(CDlgCreateEffectTexture)
	enum { IDD = IDD_CREATE_EFFECT_TEXTURE };
	CButton	m_ctrlCheckButton;
	CComboBox	m_ctrlMexSizeCombo;
	CComboBox	m_ctrlPixWidthCombo;
	CComboBox	m_ctrlPixHeightCombo;
	CComboBox	m_ctrlEffectClassCombo;
	CComboBox	m_ctrlEffectTypeCombo;
	CString	m_strCreatedTextureName;
	CString	m_strBaseTextureName;
	CString	m_strRendSpeed;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreateEffectTexture)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCreateEffectTexture)
	afx_msg void OnPaint();
	afx_msg void OnChequeredAlpha();
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowseBase();
	afx_msg void OnCreateAs();
	afx_msg void OnRemoveAllEffects();
	afx_msg void OnSelchangePixHeight();
	afx_msg void OnSelchangePixWidth();
	afx_msg void OnSelchangeMexSize();
	afx_msg void OnCreate();
	afx_msg void OnSelchangeEffectClass();
	afx_msg void OnSelchangeEffectType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCREATEEFFECTTEXTURE_H__C517CED4_FA6C_11D1_82E9_000000000000__INCLUDED_)
