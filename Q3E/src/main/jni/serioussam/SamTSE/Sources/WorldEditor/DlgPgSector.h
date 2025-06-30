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

#if !defined(AFX_DLGPGSECTOR_H__92165BC6_C826_11D1_8244_000000000000__INCLUDED_)
#define AFX_DLGPGSECTOR_H__92165BC6_C826_11D1_8244_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgPgSector.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgPgSector dialog

class CDlgPgSector : public CPropertyPage
{  
	DECLARE_DYNCREATE(CDlgPgSector)

// Construction
public:
	CDlgPgSector();
	~CDlgPgSector();
  BOOL OnIdle(LONG lCount);

  COLOR m_colLastSectorAmbientColor;
  BOOL m_bLastSectorAmbientColorMixed;
  CUpdateableRT m_udSectorsData;

// Dialog Data
	//{{AFX_DATA(CDlgPgSector)
	enum { IDD = IDD_PG_SECTOR };
	CCtrlEditFlags	m_ctrlClassificationFlags;
	CCtrlEditFlags	m_ctrlVisibilityFlags;
	CComboBox	m_comboEnvironmentType;
	CComboBox	m_comboHaze;
	CComboBox	m_comboFog;
	CComboBox	m_comboForceField;
	CComboBox	m_comboContentType;
	CColoredButton	m_SectorAmbientColor;
	int		m_iBrowseModeRadio;
	CString	m_strSectorName;
	int		m_radioInclude;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDlgPgSector)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlgPgSector)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDropdownContentTypeCombo();
	afx_msg void OnSelchangeContentTypeCombo();
	afx_msg void OnSelchangeForceFieldCombo();
	afx_msg void OnDropdownForceFieldCombo();
	afx_msg void OnDropdownFogCombo();
	afx_msg void OnSelchangeFogCombo();
	afx_msg void OnDropdownHazeCombo();
	afx_msg void OnSelchangeHazeCombo();
	afx_msg void OnDropdownStaticEnvironmentType();
	afx_msg void OnSelchangeStaticEnvironmentType();
	afx_msg void OnSectorInclude();
	afx_msg void OnSectorExclude();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPGSECTOR_H__92165BC6_C826_11D1_8244_000000000000__INCLUDED_)
