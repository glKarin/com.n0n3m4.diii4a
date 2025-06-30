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

// WndTerrainTilePalette.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "WndTerrainTilePalette.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PIX_TILE_WIDTH 64
#define PIX_TILE_HEIGHT 64

/////////////////////////////////////////////////////////////////////////////
// CWndTerrainTilePalette

CWndTerrainTilePalette::CWndTerrainTilePalette()
{
  m_ptd=NULL;
  m_pDrawPort = NULL;
  m_pViewPort = NULL;
  m_iTimerID = -1;
}

CWndTerrainTilePalette::~CWndTerrainTilePalette()
{
  // free allocated tile info structures
  for(INDEX i=0; i<m_dcTileInfo.Count(); i++)
  {
    delete &m_dcTileInfo[i];
  }
  m_dcTileInfo.Clear();

  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }
}


BEGIN_MESSAGE_MAP(CWndTerrainTilePalette, CWnd)
	//{{AFX_MSG_MAP(CWndTerrainTilePalette)
	ON_WM_PAINT()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWndTerrainTilePalette message handlers

PIXaabbox2D CWndTerrainTilePalette::GetTileBBox( INDEX iTile)
{
  PIXaabbox2D boxScr=PIXaabbox2D(
    PIX2D( (iTile%m_ctPaletteTilesH)*PIX_TILE_WIDTH+1, 
           (iTile/m_ctPaletteTilesH)*PIX_TILE_HEIGHT+1),
    PIX2D( (iTile%m_ctPaletteTilesH+1)*PIX_TILE_WIDTH+1, 
           (iTile/m_ctPaletteTilesH+1)*PIX_TILE_HEIGHT+1) );
  // return calculated box
  return boxScr;
}

void CWndTerrainTilePalette::OnPaint() 
{
  CTerrainLayer *ptlLayer=GetLayer();
  if(ptlLayer==NULL) return;

  {
  CPaintDC dc(this); // device context for painting
  }

  if( m_iTimerID == -1)
  {
    m_iTimerID = (int) SetTimer( 1, 10, NULL);
  }

  POINT ptMouse;
  GetCursorPos( &ptMouse); 
  ScreenToClient( &ptMouse);

  // if there is a valid drawport, and the drawport can be locked
  if( (m_pDrawPort != NULL) && (m_pDrawPort->Lock()) )
  {
    CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
    ASSERT( pWorldEditorView != NULL);
    // clear background
    m_pDrawPort->Fill( C_BLACK|CT_OPAQUE);
    // erase z-buffer
    m_pDrawPort->FillZBuffer(ZBUF_BACK);

    CTextureObject to;
    to.SetData(m_ptd);
    PIX pixTexW=m_ptd->GetPixWidth();
    PIX pixTexH=m_ptd->GetPixHeight();
    PIX pixTileSize=pixTexW/m_ctTilesPerRaw;
    PIX pixdpw=m_pDrawPort->GetWidth();
    PIX pixdph=m_pDrawPort->GetHeight();

    for(INDEX iTile=0; iTile<m_dcTileInfo.Count(); iTile++)
    {
      CTileInfo &ti=m_dcTileInfo[iTile];
      MEXaabbox2D boxTex=MEXaabbox2D(
        MEX2D(ti.ti_ix*pixTileSize, ti.ti_iy*pixTileSize),
        MEX2D((ti.ti_ix+1)*pixTileSize, (ti.ti_iy+1)*pixTileSize) );
      PIXaabbox2D boxScr=GetTileBBox(iTile);

      // draw tile
      FLOAT fU0=boxTex.Min()(1)/(FLOAT)pixTexW;
      FLOAT fV0=boxTex.Min()(2)/(FLOAT)pixTexH;
      FLOAT fU1=boxTex.Min()(1)/(FLOAT)pixTexW;
      FLOAT fV1=boxTex.Max()(2)/(FLOAT)pixTexH;
      FLOAT fU2=boxTex.Max()(1)/(FLOAT)pixTexW;
      FLOAT fV2=boxTex.Max()(2)/(FLOAT)pixTexH;
      FLOAT fU3=boxTex.Max()(1)/(FLOAT)pixTexW;
      FLOAT fV3=boxTex.Min()(2)/(FLOAT)pixTexH;

      FLOAT fI0=boxScr.Min()(1);
      FLOAT fJ0=boxScr.Min()(2);
      FLOAT fI1=boxScr.Min()(1);
      FLOAT fJ1=boxScr.Max()(2);
      FLOAT fI2=boxScr.Max()(1);
      FLOAT fJ2=boxScr.Max()(2);
      FLOAT fI3=boxScr.Max()(1);
      FLOAT fJ3=boxScr.Min()(2);

      if(ti.ti_bFlipX)
      {
        Swap(fI0,fI3);
        Swap(fJ0,fJ3);
        Swap(fI1,fI2);
        Swap(fJ1,fJ2);
      }

      if(ti.ti_bFlipY)
      {
        Swap(fI0,fI1);
        Swap(fJ0,fJ1);
        Swap(fI2,fI3);
        Swap(fJ2,fJ3);
      }

      if(ti.ti_bSwapXY)
      {
        Swap(fI1,fI3);
        Swap(fJ1,fJ3);
      }

      m_pDrawPort->InitTexture( &to);
      COLOR col=C_WHITE|CT_OPAQUE;
      m_pDrawPort->AddTexture(fI0, fJ0, fU0, fV0, col,
                              fI1, fJ1, fU1, fV1, col,
                              fI2, fJ2, fU2, fV2, col,
                              fI3, fJ3, fU3, fV3, col);
      m_pDrawPort->FlushRenderingQueue(); 
      
      // draw border
      m_pDrawPort->DrawBorder( boxScr.Min()(1), boxScr.Min()(2), boxScr.Size()(1), boxScr.Size()(2), C_lGRAY|CT_OPAQUE);
    }
    m_pDrawPort->DrawBorder( 0,0, m_pDrawPort->GetWidth(),m_pDrawPort->GetHeight(), C_vlGRAY|CT_OPAQUE);

    // draw selected tile
    CTileInfo &ti=m_dcTileInfo[ptlLayer->tl_iSelectedTile];    
    MEXaabbox2D boxTex=MEXaabbox2D(
      MEX2D(ti.ti_ix*pixTileSize, ti.ti_iy*pixTileSize),
      MEX2D((ti.ti_ix+1)*pixTileSize, (ti.ti_iy+1)*pixTileSize) );
    PIXaabbox2D boxScr=GetTileBBox(ptlLayer->tl_iSelectedTile);
    TIME tm=_pTimer->GetRealTimeTick();
    FLOAT fFactor=sin(tm*8)/2.0f+0.5f;
    COLOR colSelected=LerpColor(C_vlGRAY,C_RED,fFactor);
    m_pDrawPort->DrawBorder(boxScr.Min()(1), boxScr.Min()(2), 
                            boxScr.Size()(1), boxScr.Size()(2),
                            colSelected|CT_OPAQUE);

    // draw tile under mouse
    PIXaabbox2D boxPoint( PIX2D( ptMouse.x, ptMouse.y), PIX2D(ptMouse.x, ptMouse.y));
    for( INDEX itum=0; itum<m_dcTileInfo.Count(); itum++)
    {
      CTileInfo &ti=m_dcTileInfo[itum];    
      PIXaabbox2D boxScr=GetTileBBox(itum);
      if( (boxScr & boxPoint) == boxPoint)
      {
        INDEX iRot=((ULONG)(tm*25.0f))&7;
        ULONG ulLineType=0x0f0f0f0f<<iRot;
        m_pDrawPort->DrawBorder(boxScr.Min()(1), boxScr.Min()(2), 
                                boxScr.Size()(1), boxScr.Size()(2),
                                C_BLUE|CT_OPAQUE, ulLineType);
        break;
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

BOOL CWndTerrainTilePalette::Initialize(PIX pixX, PIX pixY, CTextureData *ptd, BOOL bCenter/*=TRUE*/)
{
  m_ptd=ptd;

  // obtain tile info array
  ObtainLayerTileInfo( &m_dcTileInfo, ptd, m_ctTilesPerRaw);

  INDEX ctTiles=m_dcTileInfo.Count();
  if(ctTiles==0) return FALSE;
  m_ctPaletteTilesH=sqrt((FLOAT)ctTiles);

  // calculate window's size
  CRect rectWindow;
  PIX pixWidth=PIX_TILE_WIDTH*m_ctPaletteTilesH;
  PIX pixHeight=((ctTiles-1)/m_ctPaletteTilesH+1)*PIX_TILE_HEIGHT;

  if(bCenter)
  {
    rectWindow.left = pixX-pixWidth/2;
    rectWindow.top = pixY-pixHeight/2;
  }
  else
  {
    rectWindow.left = pixX;
    rectWindow.top = pixY;
  }

  PIX pixScreenWidth  = ::GetSystemMetrics(SM_CXSCREEN);
  PIX pixScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

  if( rectWindow.left+pixWidth>pixScreenWidth)
  {
    rectWindow.left=pixScreenWidth-pixWidth;
  }
  if( rectWindow.top+pixHeight>pixScreenHeight)
  {
    rectWindow.top=pixScreenHeight-pixHeight;
  }

  rectWindow.right = rectWindow.left + pixWidth+2;
  rectWindow.bottom = rectWindow.top + pixHeight+2;

  if( IsWindow(m_hWnd))
  {
    SetWindowPos( NULL, rectWindow.left, rectWindow.top,
      rectWindow.right-rectWindow.left, rectWindow.top-rectWindow.bottom,
      SWP_NOZORDER | SWP_NOACTIVATE);
    ShowWindow(SW_SHOW);
  }
  else
  {
    // create window
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    BOOL bResult = CreateEx( WS_EX_TOOLWINDOW,
      NULL, L"Terrain tile palette", WS_CHILD|WS_POPUP|WS_VISIBLE,
      rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
      pMainFrame->m_hWnd, NULL, NULL);
    if( !bResult)
    {
      AfxMessageBox( L"Error: Failed to create terrain tile palette window!");
      return FALSE;
    }
    _pGfx->CreateWindowCanvas( m_hWnd, &m_pViewPort, &m_pDrawPort);
  }
  return TRUE;
}

void CWndTerrainTilePalette::OnKillFocus(CWnd* pNewWnd) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if(pNewWnd!=pMainFrame->m_pwndToolTip && pNewWnd!=this)
  {
    DestroyWindow();
    DeleteTempMap();
  }
}

BOOL CWndTerrainTilePalette::PreTranslateMessage(MSG* pMsg) 
{
  if( pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_ESCAPE)
  {
    DestroyWindow();
    DeleteTempMap();
    return TRUE;
  }
	return CWnd::PreTranslateMessage(pMsg);
}

void CWndTerrainTilePalette::OnLButtonDown(UINT nFlags, CPoint point) 
{
}

void CWndTerrainTilePalette::OnTimer(UINT_PTR nIDEvent)
{
  POINT pt;
  GetCursorPos( &pt);
  CRect rectWnd;
  GetWindowRect(rectWnd);
  if(pt.x<rectWnd.left || pt.x>rectWnd.right ||
     pt.y<rectWnd.top  || pt.y>rectWnd.bottom)
  {
    DestroyWindow();
    DeleteTempMap();
    return;
  }

  Invalidate(FALSE);	
	CWnd::OnTimer(nIDEvent);
}

void CWndTerrainTilePalette::OnLButtonUp(UINT nFlags, CPoint point) 
{
  PIXaabbox2D boxPoint( PIX2D( point.x, point.y), PIX2D(point.x, point.y) );
  // for all tiles
  for( INDEX iTile=0; iTile<m_dcTileInfo.Count(); iTile++)
  {
    if( (GetTileBBox(iTile) & boxPoint) == boxPoint)
    {
      CTerrainLayer *ptlLayer=GetLayer();
      if(ptlLayer==NULL) return;
      if( ptlLayer->tl_ltType==LT_TILE)
      {
        ptlLayer->tl_iSelectedTile=iTile;
      }
      break;
    }
  }
  DestroyWindow();
  DeleteTempMap();
}
