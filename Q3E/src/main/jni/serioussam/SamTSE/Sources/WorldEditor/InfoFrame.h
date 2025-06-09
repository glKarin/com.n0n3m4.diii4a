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

// InfoFrame.h : header file
//
#ifndef INFOFRAME_H
#define INFOFRAME_H 1

/////////////////////////////////////////////////////////////////////////////
// CInfoFrame frame

class CInfoFrame : public CMiniFrameWnd
{
	DECLARE_DYNCREATE(CInfoFrame)
protected:

// Attributes
public:
  CInfoSheet *m_pInfoSheet;
  int m_PageWidth;
  int m_PageHeight;

// Operations
public:
	CInfoFrame();           // protected constructor used by dynamic creation
	virtual ~CInfoFrame();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInfoFrame)
	//}}AFX_VIRTUAL

// Implementation
protected:
public:

	// Generated message map functions
	//{{AFX_MSG(CInfoFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // INFOFRAME_H
