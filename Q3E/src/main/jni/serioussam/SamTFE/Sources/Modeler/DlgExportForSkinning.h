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

#if !defined(AFX_DLGEXPORTFORSKINNING_H__74008183_5EEE_11D4_84EB_000021291DC7__INCLUDED_)
#define AFX_DLGEXPORTFORSKINNING_H__74008183_5EEE_11D4_84EB_000021291DC7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CDlgExportForSkinning.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgExportForSkinning dialog

class CDlgExportForSkinning : public CDialog
{
// Construction
public:
  INDEX m_iTextureWidth;

	CDlgExportForSkinning(CTFileName fnExportFile, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgExportForSkinning)
	enum { IDD = IDD_EXPORT_FOR_SKINNIG };
	CColoredButton	m_ctrlWireColor;
	CColoredButton	m_ctrlPaperColor;
	CComboBox	m_ctrlExportPictureSize;
	BOOL	m_bColoredSurfaces;
	BOOL	m_bSurfaceNumbers;
	BOOL	m_bWireFrame;
	CString	m_strExportedFileName;
	CString	m_strSurfaceListFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgExportForSkinning)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgExportForSkinning)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGEXPORTFORSKINNING_H__74008183_5EEE_11D4_84EB_000021291DC7__INCLUDED_)
