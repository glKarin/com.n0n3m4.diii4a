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

// ColorPaletteWnd.cpp : implementation file
//

#include "stdafx.h"
#include "ColorPaletteWnd.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// this ptr to color is used as result ptr meaning when one of colors from palette is clicked,
// color that is pointed trough this ptr is filled with choosed color value
COLOR *_pcolColorToSet;

COLOR acol_ColorizePallete[] =
{
  C_RED, C_dGREEN, C_BLUE, C_vdCYAN,
  C_MAGENTA, C_vdYELLOW, C_ORANGE, C_BROWN,
  C_PINK, C_dGRAY, C_GRAY, C_vdGRAY,
  C_dRED, C_lRED, C_vdGREEN, C_mdGREEN,
  C_dBLUE, C_lBLUE, C_dCYAN, C_vdBLUE,
  C_vdMAGENTA, C_dMAGENTA, C_dYELLOW, C_mdBROWN,
  C_vdORANGE, C_dORANGE, C_vdBROWN, C_BROWN,
  C_dPINK, C_lPINK, C_mdGRAY, C_vdRED,
};

/////////////////////////////////////////////////////////////////////////////
// CColorPaletteWnd

CColorPaletteWnd::CColorPaletteWnd()
{
  m_iSelectedColor = -1;

  _pcolColorToSet = NULL;
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
}

CColorPaletteWnd::~CColorPaletteWnd()
{
  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }
}


BEGIN_MESSAGE_MAP(CColorPaletteWnd, CWnd)
	//{{AFX_MSG_MAP(CColorPaletteWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CColorPaletteWnd message handlers

// constants for defining palette window's looks
#define COLORS_PER_X 4
#define COLORS_PER_Y 8
#define CLIENT_BORDER 6

/*
 * Calculate given color's box in pixels
 */
PIXaabbox2D CColorPaletteWnd::GetColorBBox( INDEX iColor)
{
  CRect rectClient;
  // get window's client area
  GetClientRect( &rectClient);
  PIX DX = (rectClient.Width()  - 2*CLIENT_BORDER)/COLORS_PER_X;
  PIX DY = (rectClient.Height() - 2*CLIENT_BORDER)/COLORS_PER_Y;
  // calculate starting pixel for current color
  PIX pixXS = CLIENT_BORDER + (iColor%COLORS_PER_X)*DX;
  PIX pixYS = CLIENT_BORDER + (iColor/COLORS_PER_X)*DY;
  // return calculated box
  return PIXaabbox2D( PIX2D(pixXS, pixYS), PIX2D(pixXS+DX, pixYS+DY) );
}

void CColorPaletteWnd::OnPaint() 
{
  {
  CPaintDC dc(this); // device context for painting
  }

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()) )
  {
    CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
    ASSERT( pWorldEditorView != NULL);
    // clear background
    m_pDrawPort->Fill( C_BLACK | CT_OPAQUE);
    // erase z-buffer
    m_pDrawPort->FillZBuffer(ZBUF_BACK);
    // for all colors
    for( INDEX i=0; i<32; i++)
    {
      // get current color's box in pixels inside window
      PIXaabbox2D boxColor = GetColorBBox( i);
      // fill rectangle in current color
      m_pDrawPort->Fill( boxColor.Min()(1)+1, boxColor.Min()(2)+1,
                         boxColor.Max()(1)-boxColor.Min()(1)-2, boxColor.Max()(2)-boxColor.Min()(2)-2,
                         acol_ColorizePallete[ i] | CT_OPAQUE);
      // if we are drawing selected color
      if( i == m_iSelectedColor)
      {
        m_pDrawPort->DrawLine( boxColor.Min()(1), boxColor.Min()(2),
                               boxColor.Min()(1), boxColor.Max()(2), C_WHITE|CT_OPAQUE, _POINT_);
        m_pDrawPort->DrawLine( boxColor.Min()(1), boxColor.Max()(2),
                               boxColor.Max()(1), boxColor.Max()(2), C_WHITE|CT_OPAQUE, _POINT_);
        m_pDrawPort->DrawLine( boxColor.Max()(1), boxColor.Max()(2),
                               boxColor.Max()(1), boxColor.Min()(2), C_WHITE|CT_OPAQUE, _POINT_);
        m_pDrawPort->DrawLine( boxColor.Max()(1), boxColor.Min()(2), 
                               boxColor.Min()(1), boxColor.Min()(2), C_WHITE|CT_OPAQUE, _POINT_);
      }
    }

    // unlock the drawport
    m_pDrawPort->Unlock();

    // if there is a valid viewport
    if (m_pViewPort!=NULL)
    {
      m_pViewPort->SwapBuffers();
    }
  }
}

void CColorPaletteWnd::OnLButtonDown(UINT nFlags, CPoint point) 
{
  PIXaabbox2D boxPoint( PIX2D( point.x, point.y), PIX2D(point.x, point.y) );

  // for all colors
  for( INDEX iSelectedColor=0; iSelectedColor<32; iSelectedColor++)
  {
    if( (GetColorBBox( iSelectedColor) & boxPoint) == boxPoint)
    {
      // obtain document
      CWorldEditorDoc *pDoc = theApp.GetDocument();
      // must not be null
      ASSERT( pDoc != NULL);
      // if polygon mode
      if( pDoc->m_iMode == POLYGON_MODE)
      {
        // polygon selection must contain selected polygons
        ASSERT(pDoc->m_selPolygonSelection.Count() != 0);
        // for each of the selected polygons
        FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
        {
          // set new polygon's color
          itbpo->bpo_colColor = acol_ColorizePallete[ iSelectedColor];
        }
      }
      else if( pDoc->m_iMode == SECTOR_MODE)
      {
        // sector selection must contain selected sectors
        ASSERT(pDoc->m_selSectorSelection.Count() != 0);
        // for each of the selected sectors
        FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
        {
          // set new polygon's color
          itbsc->bsc_colColor = acol_ColorizePallete[ iSelectedColor];
        }
      }
      if( _pcolColorToSet != NULL)
      {
        // fill result
        *_pcolColorToSet = acol_ColorizePallete[ iSelectedColor];
      }
      // update all document's views
      pDoc->UpdateAllViews( NULL);
      break;
    }
  }

  // destroy color palette
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_pColorPalette = NULL;
  // destroy color palette
  delete this;
}

static BOOL _bCanBeDestroyed = TRUE;
void CColorPaletteWnd::OnKillFocus(CWnd* pNewWnd) 
{
  if( !_bCanBeDestroyed) return;
  // destroy color palette
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_pColorPalette = NULL;
  delete this;
}

void CColorPaletteWnd::OnRButtonDown(UINT nFlags, CPoint point) 
{
  PIXaabbox2D boxPoint( PIX2D( point.x, point.y), PIX2D(point.x, point.y) );

  // for all colors
  for( INDEX iSelectedColor=0; iSelectedColor<32; iSelectedColor++)
  {
    if( (GetColorBBox( iSelectedColor) & boxPoint) == boxPoint)
    {
      COLORREF TmpColor = CLRF_CLR( acol_ColorizePallete[ iSelectedColor]);
      _bCanBeDestroyed = FALSE;
      if( MyChooseColor( TmpColor, *GetParent()))
      {
        // remember result
        acol_ColorizePallete[ iSelectedColor] = CLR_CLRF( TmpColor);
        Invalidate( FALSE);
      }
      _bCanBeDestroyed = TRUE;
    }
  }
	CWnd::OnRButtonDown(nFlags, point);
  // destroy color palette
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_pColorPalette = NULL;
  delete this;
}
