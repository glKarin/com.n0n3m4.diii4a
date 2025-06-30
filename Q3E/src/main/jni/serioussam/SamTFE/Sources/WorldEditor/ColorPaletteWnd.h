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

// ColorPaletteWnd.h : header file
//
#ifndef COLORPALETTEWND_H
#define COLORPALETTEWND_H

/////////////////////////////////////////////////////////////////////////////
// CColorPaletteWnd window

extern COLOR *_pcolColorToSet;

class CColorPaletteWnd : public CWnd
{
// Construction
public:
	CColorPaletteWnd();

// Attributes
public:
  INDEX m_iSelectedColor;

  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;

// Operations
public:
  // calculate given color's box in pixels
  PIXaabbox2D GetColorBBox( INDEX iColor);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorPaletteWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CColorPaletteWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorPaletteWnd)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // COLORPALETTEWND_H
