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

#if !defined(AFX_CTRLCOLORBROWSINGMODERADIO_H__9322F8B3_0DAA_11D2_8327_004095812ACC__INCLUDED_)
#define AFX_CTRLCOLORBROWSINGMODERADIO_H__9322F8B3_0DAA_11D2_8327_004095812ACC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CtrlColorBrowsingModeRadio.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCtrlColorBrowsingModeRadio window

class CCtrlColorBrowsingModeRadio : public CButton
{
// Construction
public:
	CCtrlColorBrowsingModeRadio();

// Attributes
public:
  // ptr to parent dialog
  CPropertyComboBar *m_pDialog;

// Operations
public:
  // sets ptr to parent dialog
  void SetDialogPtr( CPropertyComboBar *pDialog);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlColorBrowsingModeRadio)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlColorBrowsingModeRadio();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlColorBrowsingModeRadio)
	afx_msg void OnClicked();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTRLCOLORBROWSINGMODERADIO_H__9322F8B3_0DAA_11D2_8327_004095812ACC__INCLUDED_)
