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

// DlgWorldSettings.h : header file
//
#ifndef DLGWORLDSETTINGS_H
#define DLGWORLDSETTINGS_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgWorldSettings dialog

class CDlgWorldSettings : public CDialog
{
// Construction
public:
	CDlgWorldSettings(CWnd* pParent = NULL);   // standard constructor
  void SetupBcgSettings( BOOL bOnNewDocument); // setups background settings dialog

// Dialog Data
	//{{AFX_DATA(CDlgWorldSettings)
	enum { IDD = IDD_WORLD_SETTINGS };
	CColoredButton	m_BackgroundColor;
	CString	m_fnBackgroundPicture;
	CString	m_strMissionDescription;
	float	m_fFrontViewCenterX;
	float	m_fFrontViewCenterY;
	float	m_fFrontViewHeight;
	CString	m_strFrontViewPicture;
	float	m_fFrontViewWidth;
	float	m_fRightViewCenterX;
	float	m_fRightViewCenterY;
	float	m_fRightViewHeight;
	CString	m_strRightViewPicture;
	float	m_fRightViewWidth;
	float	m_fTopViewCenterX;
	float	m_fTopViewCenterY;
	float	m_fTopViewHeight;
	CString	m_strTopViewPicture;
	float	m_fTopViewWidth;
	CString	m_strBackdropObject;
	CString	m_strLevelName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgWorldSettings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgWorldSettings)
	afx_msg void OnBrowseBackgroundPicture();
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowseFrontViewPicture();
	afx_msg void OnBrowseRightViewPicture();
	afx_msg void OnBrowseTopViewPicture();
	afx_msg void OnBrowseBackdropObject();
	afx_msg void OnApply();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // DLGWORLDSETTINGS_H
