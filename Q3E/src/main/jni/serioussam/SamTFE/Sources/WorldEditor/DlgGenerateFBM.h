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

#if !defined(AFX_DLGGENERATEFBM_H__E9D18A4D_B090_48B0_A0D5_C1CEAADEFBA8__INCLUDED_)
#define AFX_DLGGENERATEFBM_H__E9D18A4D_B090_48B0_A0D5_C1CEAADEFBA8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgGenerateFBM.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgGenerateFBM dialog

class CDlgGenerateFBM : public CDialog  
{
// Construction
public:
  CWnd m_wndTexture;
  BOOL m_bCustomWindowCreated;

  CDrawPort *m_pdp;
  CViewPort *m_pvp;

  CDlgGenerateFBM(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgGenerateFBM)
	enum { IDD = IDD_GENERATE_FBM };
	CSpinButtonCtrl	m_ctrlCtOctavesSpin;
	float	m_fMaxAltitude;
	float	m_fOctaveAmplitudeDecreaser;
	float	m_fOctaveAmplitudeStep;
	float	m_fHighFrequencyStep;
	int		m_ctOctaves;
	BOOL	m_bAddNegativeValues;
	BOOL	m_bRandomOffset;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgGenerateFBM)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgGenerateFBM)
	afx_msg void OnPaint();
	afx_msg void OnFbmRandomize();
	afx_msg void OnChangeFbmHighFrequencyStep();
	afx_msg void OnChangeFbmMaxAltitude();
	afx_msg void OnChangeFbmOctaveAmplitudeDecrease();
	afx_msg void OnChangeFbmOctaveStep();
	afx_msg void OnChangeFbmOctaves();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnAddNegativeValues();
	afx_msg void OnFbmExport();
	afx_msg void OnRandomOffset();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGGENERATEFBM_H__E9D18A4D_B090_48B0_A0D5_C1CEAADEFBA8__INCLUDED_)
