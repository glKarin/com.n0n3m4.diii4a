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

#if !defined(AFX_DLGSELECTPLAYER_H__FCDE2F73_3A65_11D2_8384_004095812ACC__INCLUDED_)
#define AFX_DLGSELECTPLAYER_H__FCDE2F73_3A65_11D2_8384_004095812ACC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgSelectPlayer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectPlayer dialog

class CDlgSelectPlayer : public CDialog
{
// Construction
public:
	CDlgSelectPlayer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgSelectPlayer)
	enum { IDD = IDD_SELECT_PLAYER };
	CComboBox	m_comboAvailablePlayers;
	CComboBox	m_comboAvailableControls;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSelectPlayer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSelectPlayer)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboAvailablePlayers();
	afx_msg void OnSelchangeComboAvailableControls();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGSELECTPLAYER_H__FCDE2F73_3A65_11D2_8384_004095812ACC__INCLUDED_)
