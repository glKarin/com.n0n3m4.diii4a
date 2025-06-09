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

// DlgPlayerSettings.h : header file
//
#ifndef DLGPLAYERSETTINGS_H
#define DLGPLAYERSETTINGS_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerSettings dialog

class CDlgPlayerControls;

class CDlgPlayerSettings : public CDialog
{
// Construction
public:
	CDlgPlayerSettings(CWnd* pParent = NULL);   // standard constructor
  void InitPlayersAndControlsLists(void); 

// Dialog Data
	//{{AFX_DATA(CDlgPlayerSettings)
	enum { IDD = IDD_SETTINGS_PLAYERS };
	CListBox	m_listAvailableControls;
	CListBox	m_listAvailablePlayers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPlayerSettings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
	//{{AFX_MSG(CDlgPlayerSettings)
	afx_msg void OnPlayerAppearance();
	virtual BOOL OnInitDialog();
	afx_msg void OnEditControls();
	afx_msg void OnRenameControls();
	afx_msg void OnRenamePlayer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // DLGPLAYERSETTINGS_H
