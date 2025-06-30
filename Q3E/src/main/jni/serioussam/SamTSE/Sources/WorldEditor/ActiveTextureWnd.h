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

// ActiveTextureWnd.h : header file
//
#ifndef ACTIVETEXTUREWND_H
#define ACTIVETEXTUREWND_H 1

/////////////////////////////////////////////////////////////////////////////
// CActiveTextureWnd window

class CActiveTextureWnd : public CWnd
{
// Construction
public:
	CActiveTextureWnd();

// Attributes
public:
  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;
  COleDataSource m_DataSource;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActiveTextureWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CActiveTextureWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CActiveTextureWnd)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // ACTIVETEXTUREWND_H
