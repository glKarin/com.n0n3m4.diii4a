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

#if !defined(AFX_TERRAININTERFACE_H__539D0786_D5B8_4079_A285_DA2C86EC7543__INCLUDED_)
#define AFX_TERRAININTERFACE_H__539D0786_D5B8_4079_A285_DA2C86EC7543__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TerrainInterface.h : header file
//

#define TEM_HEIGHTMAP 0
#define TEM_LAYER     1

#define CT_EDIT_MODES 2


#define TBM_PAINT           0
#define TBM_SMOOTH          1
#define TBM_FILTER          2
#define TBM_MINIMUM         3
#define TBM_MAXIMUM         4
#define TBM_FLATTEN         5
#define TBM_POSTERIZE       6
#define TBM_RND_NOISE       7
#define TBM_CONTINOUS_NOISE 8
#define TBM_ERASE           9

#define CT_BRUSH_MODES  10

struct CTerrainEditBrush {
  FLOAT teb_fHotSpot;
  FLOAT teb_fFallOff;
};

extern CTFileName GetBrushTextureName(INDEX iBrush);
extern CTerrainEditBrush atebDefaultEditBrushValues[];
extern CTerrainEditBrush atebCustomEditBrushes[];
extern void InvokeTerrainTilePalette( PIX pixX, PIX pixY);
extern void InvokeTerrainBrushPalette( PIX pixX, PIX pixY);
extern void RenderBrushShape( INDEX iBrush, PIXaabbox2D rect, CDrawPort *pdp);
extern void GetEditingModeInfo(INDEX iMode, INDEX &iIcon, CTString &strText);
extern CBrushPaletteWnd *_pBrushPalette;
extern void GenerateTerrainBrushTexture( INDEX iBrush, FLOAT fHotSpot, FLOAT fFallOff);
extern void GenerateNonExistingTerrainEditBrushes(void);
extern void ApplyImportExport(INDEX iSelectedItem);
extern void DisplayHeightMapWindow(CPoint pt);
void GetBrushModeInfo(INDEX iMode, INDEX &iIcon, CTString &strText);
#define CT_BRUSHES 32
#define BRUSH_PALETTE_WIDTH (128-1)
#define BRUSH_PALETTE_HEIGHT (256-1)
#define TILE_PALETTE_WIDTH (256-1)
#define TILE_PALETTE_HEIGHT (256-1)

/////////////////////////////////////////////////////////////////////////////
// CTerrainInterface window

struct CTIButton {
  FLOAT tib_fx;
  FLOAT tib_fy;
  FLOAT tib_fdx;
  FLOAT tib_fdy;
  INDEX tib_iIcon;
  COLOR tib_colBorderColor;
  COLOR tib_colFill;
  FLOAT tib_fDataMin;
  FLOAT tib_fDataMax;
  FLOAT tib_fDataDelta;
  BOOL tib_bWrapData;
  BOOL tib_bMouseTrapForMove;
  BOOL tib_bContinueTesting;
  FLOAT *tib_pfData1;
  FLOAT *tib_pfData2;
  INDEX tib_iLayer;
  CTString tib_strToolTip;
  void (*tib_pOnRender)(CTIButton *ptib, CDrawPort *pdp);
  void (*tib_pOnLeftClick)(CTIButton *ptib, CPoint pt, CDrawPort *pdp);
  void (*tib_pOnLeftClickMove)(CTIButton *ptib, FLOAT fdx, FLOAT fdy, CDrawPort *pdp);
  void (*tib_pOnRightClick)(CTIButton *ptib, CPoint pt, CDrawPort *pdp);
  void (*tib_pOnRightClickMove)(CTIButton *ptib, FLOAT fdx, FLOAT fdy, CDrawPort *pdp);
  void (*tib_pPreRender)(CTIButton *ptib, CDrawPort *pdp); 
  // misc functions
  void (*tib_pOnDropFiles)(CTIButton *ptib, CPoint pt, CDrawPort *pdp, CTFileName fnFile);
  CTString (*tib_pGetClickMoveData)(CTIButton *ptib, CPoint pt, CDrawPort *pdp, BOOL bLmb);
  BOOL (*tib_pIsEnabled)(CTIButton *ptib);

  // construction
  CTIButton();

  void SetData( FLOAT fDataMin, FLOAT fDataMax, FLOAT fDataDelta, 
    BOOL bWrap=FALSE, FLOAT *pfData1=NULL, FLOAT *pfData2=NULL);
  void SetFunctions(
    void (*pOnRender)(CTIButton *ptib, CDrawPort *pdp)=NULL,
    void (*pOnLeftClick)(CTIButton *ptib, CPoint pt, CDrawPort *pdp)=NULL,
    void (*pOnLeftClickMove)(CTIButton *ptib, FLOAT fdx, FLOAT fdy, CDrawPort *pdp)=NULL,
    void (*pOnRightClick)(CTIButton *ptib, CPoint pt, CDrawPort *pdp)=NULL,
    void (*pOnRighClickMove)(CTIButton *ptib, FLOAT fdx, FLOAT fdy, CDrawPort *pdp)=NULL,
    void (*pPreRender)(CTIButton *ptib, CDrawPort *pdp)=NULL);
};


class CTerrainInterface : public CWnd 
{ 
// Construction
public:
	CTerrainInterface();
  void InitializeInterface(CDrawPort *pdp);

  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;
  COleDataSource m_DataSource;

  INDEX m_iBrush;
  INDEX m_iOpacity;
  INDEX m_iEditMode;

  CPoint m_ptMouse;

  CPoint m_ptMouseDown;
  CPoint m_ptMouseDownScreen;
  CPoint m_ptMouseCenter;
  CPoint m_ptMouseCenterScreen;

  CUpdateableRT m_udTerrainPage;
  CUpdateableRT m_udTerrainPageCanvas;
// Attributes
public:

// Operations
public:
  void OnIdle(void);
  INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
  void HideCursor(void);
  void UnhideCursor(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTerrainInterface)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTerrainInterface();
  void RenderInterface(CDrawPort *pDP);
  BOOL IsClicked(CTIButton &tib, CPoint pt) const;

	// Generated message map functions
protected:
	//{{AFX_MSG(CTerrainInterface)
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TERRAININTERFACE_H__539D0786_D5B8_4079_A285_DA2C86EC7543__INCLUDED_)
