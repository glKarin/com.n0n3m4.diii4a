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

#if !defined(AFX_DLGLIGHTANIMATIONEDITOR_H__4C1D90B3_7FA9_11D2_844A_004095812ACC__INCLUDED_)
#define AFX_DLGLIGHTANIMATIONEDITOR_H__4C1D90B3_7FA9_11D2_844A_004095812ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgLightAnimationEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgLightAnimationEditor dialog

class CDlgLightAnimationEditor : public CDialog
{
// Construction
public:
  CTFileName m_fnSaveName;
  CDlgLightAnimationEditor(CWnd* pParent = NULL);   // standard constructor
  ~CDlgLightAnimationEditor();

  BOOL m_bChanged;
  BOOL m_bCustomWindowsCreated;
  CAnimData *m_padAnimData;
  CWndAnimationFrames m_wndAnimationFrames;
  CWndTestAnimation m_wndTestAnimation;
  void InitLightAnimationCombo(void);
  void SpreadFrames(void);
  void InitializeData(void);
  void StoreData(void);
  // obtain index of curently selected light animation
  INDEX GetSelectedLightAnimation(void);


// Dialog Data
	//{{AFX_DATA(CDlgLightAnimationEditor)
	enum { IDD = IDD_LIGHT_ANIMATION_EDITOR };
	CComboBox	m_LightAnimationCombo;
	CString	m_strCurrentFrame;
	float	m_fLightAnimationSpeed;
	int		m_iAnimationFrames;
	CString	m_strLightAnimationName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgLightAnimationEditor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgLightAnimationEditor)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnDeleteMarker();
	afx_msg void OnChangeLightAnimationFrames();
	afx_msg void OnSelchangeLightAnimationNameCombo();
	afx_msg void OnChangeLightAnimationSpeed();
	afx_msg void OnScrollLeft();
	afx_msg void OnScrollRight();
	afx_msg void OnScrollPgLeft();
	afx_msg void OnScrollPgRight();
	afx_msg void OnDeleteAnimation();
	afx_msg void OnAddAnimation();
	afx_msg void OnChangeLightAnimationName();
	afx_msg void OnLoadAnimation();
	afx_msg void OnSaveAnimation();
	afx_msg void OnSaveAsAnimation();
	afx_msg void OnButtonClose();
	afx_msg void OnClose();
	afx_msg void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGLIGHTANIMATIONEDITOR_H__4C1D90B3_7FA9_11D2_844A_004095812ACC__INCLUDED_)
