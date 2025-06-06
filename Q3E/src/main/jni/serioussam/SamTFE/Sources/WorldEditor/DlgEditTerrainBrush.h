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

#if !defined(AFX_DLGEDITTERRAINBRUSH_H__A349A7AF_EA7F_4EC3_8B44_7C191475F493__INCLUDED_)
#define AFX_DLGEDITTERRAINBRUSH_H__A349A7AF_EA7F_4EC3_8B44_7C191475F493__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgEditTerrainBrush.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgEditTerrainBrush dialog

class CDlgEditTerrainBrush : public CDialog
{
// Construction
public:
	CDlgEditTerrainBrush(CWnd* pParent = NULL);   // standard constructor
  INDEX m_iBrush;

// Dialog Data
	//{{AFX_DATA(CDlgEditTerrainBrush)
	enum { IDD = IDD_EDIT_TERRAIN_BRUSH };
	float	m_fFallOff;
	float	m_fHotSpot;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgEditTerrainBrush)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgEditTerrainBrush)
	afx_msg void OnGenerateTerrainBrush();
	afx_msg void OnImportTerrainBrush();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGEDITTERRAINBRUSH_H__A349A7AF_EA7F_4EC3_8B44_7C191475F493__INCLUDED_)
