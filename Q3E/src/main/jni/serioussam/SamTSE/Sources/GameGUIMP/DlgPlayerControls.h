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

// DlgPlayerControls.h : header file
//
#ifndef DLGPLAYERCONTROLS_H
#define DLGPLAYERCONTROLS_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerControls dialog

class CDlgPlayerControls : public CDialog
{
// Construction
public:
  INDEX m_iSelectedAction;
  CControls &m_ctrlControls;
  
  CDlgPlayerControls( CControls &ctrlControls, CWnd* pParent = NULL);   // standard constructor
  void ActivatePressKey( char *pFirstOrSecond);
  void FillActionsList(void);
  void FillAxisList(void);
  void SetFirstAndSecondButtonNames(void);
  CButtonAction *GetSelectedButtonAction();

// Dialog Data
	//{{AFX_DATA(CDlgPlayerControls)
	enum { IDD = IDD_PLAYER_CONTROLS };
	CAxisListCtrl	m_listAxisActions;
	CActionsListControl	m_listButtonActions;
	CPressKeyEditControl	m_editSecondControl;
	CPressKeyEditControl	m_editFirstControl;
	CSliderCtrl	m_sliderControlerSensitivity;
	CComboBox	m_comboControlerAxis;
	BOOL	m_bInvertControler;
	int		m_iRelativeAbsoluteType;
	CString	m_strPressNewButton;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPlayerControls)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
//protected:
public:

	// Generated message map functions
	//{{AFX_MSG(CDlgPlayerControls)
	virtual BOOL OnInitDialog();
	afx_msg void OnSetfocusEditFirstControl();
	afx_msg void OnSetfocusEditSecondControl();
	afx_msg void OnFirstControlNone();
	afx_msg void OnSecondControlNone();
	afx_msg void OnDefault();
	afx_msg void OnSelchangeControlerAxis();
	afx_msg void OnMoveControlUp();
	afx_msg void OnMoveControlDown();
	afx_msg void OnButtonActionAdd();
	afx_msg void OnButtonActionEdit();
	afx_msg void OnButtonActionRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // DLGPLAYERCONTROLS_H
