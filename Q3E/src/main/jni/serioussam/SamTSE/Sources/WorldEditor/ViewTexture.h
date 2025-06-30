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

#if !defined(AFX_VIEWTEXTURE_H__00278257_91BC_11D2_8478_004095812ACC__INCLUDED_)
#define AFX_VIEWTEXTURE_H__00278257_91BC_11D2_8478_004095812ACC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewTexture.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CViewTexture window

class CViewTexture : public CWnd
{
// Construction
public:
	CViewTexture();
// Attributes
public:
  CTString m_strTexture;
  COleDataSource m_DataSource;

  CViewPort *m_pViewPort;
  CDrawPort *m_pDrawPort;
 
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewTexture)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CViewTexture();

	// Generated message map functions
protected:
	//{{AFX_MSG(CViewTexture)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRecreateTexture();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWTEXTURE_H__00278257_91BC_11D2_8478_004095812ACC__INCLUDED_)
