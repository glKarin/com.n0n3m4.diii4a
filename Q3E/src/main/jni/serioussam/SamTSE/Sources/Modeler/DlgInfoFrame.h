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


// DlgInfoFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoFrame frame

class CDlgInfoFrame : public CMiniFrameWnd
{
// Constructor
public:
	CDlgInfoFrame();
  ~CDlgInfoFrame();
  void SetSizes();

// Attributes
public:
  int m_PageWidth;
  int m_PageHeight;
	CDlgInfoSheet *m_pInfoSheet;

// Operations
public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgInfoFrame)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Handlers
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlgInfoFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
