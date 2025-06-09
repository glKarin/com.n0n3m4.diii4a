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

#if !defined(AFX_WNDTERRAINTILEPALETTE_H__C3228E4D_AEA7_4300_B202_77149FD3B92C__INCLUDED_)
#define AFX_WNDTERRAINTILEPALETTE_H__C3228E4D_AEA7_4300_B202_77149FD3B92C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WndTerrainTilePalette.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWndTerrainTilePalette window

class CWndTerrainTilePalette : public CWnd
{
// Construction
public:
	CWndTerrainTilePalette();

// Attributes
public:
  CTextureData *m_ptd;
  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;
  INDEX m_ctPaletteTilesH;
  INDEX m_ctTilesPerRaw;
  CDynamicContainer<CTileInfo> m_dcTileInfo;
  INDEX m_iTimerID;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWndTerrainTilePalette)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWndTerrainTilePalette();
  BOOL Initialize(PIX pixX, PIX pixY, CTextureData *ptd, BOOL bCenter=TRUE);
  PIXaabbox2D GetTileBBox( INDEX iTile);

	// Generated message map functions
protected:
	//{{AFX_MSG(CWndTerrainTilePalette)
	afx_msg void OnPaint();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WNDTERRAINTILEPALETTE_H__C3228E4D_AEA7_4300_B202_77149FD3B92C__INCLUDED_)
