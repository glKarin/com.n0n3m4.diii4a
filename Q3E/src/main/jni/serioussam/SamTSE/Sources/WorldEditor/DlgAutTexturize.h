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

#if !defined(AFX_DLGAUTTEXTURIZE_H__17E4B1E3_B1A2_11D5_8748_00002103143B__INCLUDED_)
#define AFX_DLGAUTTEXTURIZE_H__17E4B1E3_B1A2_11D5_8748_00002103143B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgAutTexturize.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgAutTexturize dialog

class CDlgAutTexturize : public CDialog
{
// Construction
public:
  PIX m_pixWidth;
  PIX m_pixHeight;
  INDEX m_iPretenderStyle;
  CDlgAutTexturize(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgAutTexturize)
	enum { IDD = IDD_AUTO_TEXTURIZE };
	CComboBox	m_ctrlPretenderTextureStyle;
	CColoredButton	m_colBcg;
	CComboBox	m_ctrPretenderTextureSize;
	BOOL	m_bExpandEdges;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgAutTexturize)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgAutTexturize)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGAUTTEXTURIZE_H__17E4B1E3_B1A2_11D5_8748_00002103143B__INCLUDED_)
