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

#if !defined(AFX_DLGCREATESPECULARTEXTURE_H__8B415FA3_E1DD_11D2_8534_004095812ACC__INCLUDED_)
#define AFX_DLGCREATESPECULARTEXTURE_H__8B415FA3_E1DD_11D2_8534_004095812ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgCreateSpecularTexture.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateSpecularTexture dialog

class CDlgCreateSpecularTexture : public CDialog
{                                                 
// Construction
public:
  BOOL m_bCustomWindowsCreated;
  
  CPlacement3D m_plPlacement;
  CModelObject m_moModel;
  CTextureObject m_toBackground;

  int m_iTimerID;

  CWnd m_wndGraph;
  CWnd m_wndPreview;

  CDrawPort *m_pGraphDrawPort;
  CViewPort *m_pGraphViewPort;

  CDrawPort *m_pPreviewDrawPort;
  CViewPort *m_pPreviewViewPort;

	CDlgCreateSpecularTexture(CWnd* pParent = NULL);   // standard constructor
  FLOAT GetFactorForPercentage( INDEX iSlider);
  void CreateTexture( CTFileName fnTexture, FLOAT fExp);
  void DrawGraph( CDrawPort *pdp, FLOAT fExp);
  void DrawPreview( CDrawPort *pdp, FLOAT fExp);
  void RenderGraph(void);
  void RenderPreview(void);

// Dialog Data
	//{{AFX_DATA(CDlgCreateSpecularTexture)
	enum { IDD = IDD_CREATE_SPECULAR_TEXTURE };
	CSliderCtrl	m_sliderSpecularExponent;
	CColoredButton	m_colorSpecular;
	CComboBox	m_comboSizeInPixels;
	CColoredButton	m_colorLight;
	CColoredButton	m_colorAmbient;
	CString	m_strNumericalExponent;
	BOOL	m_bAutoRotate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgCreateSpecularTexture)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgCreateSpecularTexture)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnAutoRotate();
	afx_msg void OnSelchangeSizeInPixels();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGCREATESPECULARTEXTURE_H__8B415FA3_E1DD_11D2_8534_004095812ACC__INCLUDED_)
