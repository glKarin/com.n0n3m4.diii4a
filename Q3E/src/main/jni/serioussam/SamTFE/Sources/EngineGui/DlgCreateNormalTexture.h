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

#if !defined(AFX_DLGCREATENORMALTEXTURE_H__C517CED2_FA6C_11D1_82E9_000000000000__INCLUDED_)
#define AFX_DLGCREATENORMALTEXTURE_H__C517CED2_FA6C_11D1_82E9_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgCreateNormalTexture.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateNormalTexture dialog

class CDlgCreateNormalTexture : public CDialog
{
// Construction
public:
	CDlgCreateNormalTexture(CTFileName fnInputFile, CWnd* pParent = NULL);   // standard constructor
	~CDlgCreateNormalTexture();
  void RefreshCreatedTexture(void);
  void ReleaseCreatedTexture(void);
  
  BOOL  m_bSourcePictureValid;
  BOOL  m_bPreviewWindowsCreated;
  PIX   m_pixSourceWidth;
  PIX   m_pixSourceHeight;
  MEX   m_mexCreatedWidth;
  CTFileName m_fnSourceFileName;
  CTFileName m_fnCreatedFileName;
  CWndDisplayTexture m_wndViewDetailTexture;
  CWndDisplayTexture m_wndViewCreatedTexture;
  CTextureData *m_ptdCreated;
     
// Dialog Data
	//{{AFX_DATA(CDlgCreateNormalTexture)
	enum { IDD = IDD_CREATE_NORMAL_TEXTURE };
	CButton	m_ctrlForce32;
	CButton	m_ctrlCheckButton;
	CComboBox	m_ctrlNoOfMipMapsCombo;
	CComboBox	m_ctrlMexSizeCombo;
	CString	m_strCreatedTextureName;
	CString	m_strSizeInPixels;
	BOOL	m_bCreateMipmaps;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreateNormalTexture)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCreateNormalTexture)
	afx_msg void OnPaint();
	afx_msg void OnChequeredAlpha();
	afx_msg void OnForce32();
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowseDetail();
	afx_msg void OnDetailNone();
	afx_msg void OnCreateTexture();
	afx_msg void OnCreateMipmaps();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCREATENORMALTEXTURE_H__C517CED2_FA6C_11D1_82E9_000000000000__INCLUDED_)
