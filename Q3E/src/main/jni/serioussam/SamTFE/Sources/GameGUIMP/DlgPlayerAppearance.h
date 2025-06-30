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

// DlgPlayerAppearance.h : header file
//
#ifndef DLGPLAYERAPPEARANCE_H
#define DLGPLAYERAPPEARANCE_H 1

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerAppearance dialog

class CDlgPlayerAppearance : public CDialog
{
// Construction
public:
	CPlayerCharacter m_pcPlayerCharacter;
	CDlgPlayerAppearance(CPlayerCharacter &pcPlayerCharacter, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgPlayerAppearance)
	enum { IDD = IDD_PLAYER_APPEARANCE };
	CComboBox	m_comboAvailableAppearances;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPlayerAppearance)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgPlayerAppearance)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif // DLGPLAYERAPPEARANCE_H
