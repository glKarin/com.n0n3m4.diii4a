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

#if !defined(AFX_WNDDISPLAYTEXTURE_H__D9AF4DBF_9E24_4F24_AC82_3A9440AFD256__INCLUDED_)
#define AFX_WNDDISPLAYTEXTURE_H__D9AF4DBF_9E24_4F24_AC82_3A9440AFD256__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WndDisplayTexture.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWndDisplayTexture window

class CWndDisplayTexture : public CWnd
{
// Construction
public:
	CWndDisplayTexture();

// Attributes
public:
  CTextureData *m_ptd;
  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;
  CTString m_strText1;
  CTString m_strText2;
  PIXaabbox2D m_boxTexture;
  PIXaabbox2D m_boxText1;
  PIXaabbox2D m_boxText2;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndDisplayTexture)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndDisplayTexture();
  BOOL Initialize(PIX pixX, PIX pixY, CTextureData *ptd, CTString strText1="", CTString strText2="", BOOL bDown=FALSE);

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndDisplayTexture)
	afx_msg void OnPaint();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WNDDISPLAYTEXTURE_H__D9AF4DBF_9E24_4F24_AC82_3A9440AFD256__INCLUDED_)
