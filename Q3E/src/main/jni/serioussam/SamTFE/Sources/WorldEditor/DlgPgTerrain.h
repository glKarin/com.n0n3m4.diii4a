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

#if !defined(AFX_DLGPGTERRAIN_H__975072E6_95F9_4E23_B861_89835AF46634__INCLUDED_)
#define AFX_DLGPGTERRAIN_H__975072E6_95F9_4E23_B861_89835AF46634__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgPgTerrain.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgPgTerrain dialog

class CDlgPgTerrain : public CPropertyPage
{
	DECLARE_DYNCREATE(CDlgPgTerrain)

// Construction
public:
	CDlgPgTerrain();
	~CDlgPgTerrain();  

  BOOL OnIdle(LONG lCount);
	CTerrainInterface	m_wndTerrainInterface;

// Dialog Data
	//{{AFX_DATA(CDlgPgTerrain)
	enum { IDD = IDD_PG_TERRAIN };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDlgPgTerrain)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlgPgTerrain)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPGTERRAIN_H__975072E6_95F9_4E23_B861_89835AF46634__INCLUDED_)
