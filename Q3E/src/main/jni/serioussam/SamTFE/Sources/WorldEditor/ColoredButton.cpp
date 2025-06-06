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

// ColoredButton.cpp : implementation file
// 

#include "stdafx.h"
#include "ColoredButton.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL _bMouseMoveEnabled;
static BOOL _bColorChanging;
static BOOL _bCursorHidden;

/////////////////////////////////////////////////////////////////////////////
// CColoredButton

COLOR _colColorClipboard = 0xFFFFFFFF;

CColoredButton::CColoredButton()
{
  m_colColor = 0xFFFFFFFF;
  m_colLastColor = 0xFFFFFFFF;
  m_ptPickerType = PT_CUSTOM;
  m_pwndParentDialog = NULL;
  m_bMixedColor = FALSE;
  _bColorChanging = FALSE;
  _bCursorHidden = FALSE;

  UBYTE ubR, ubG, ubB;
  UBYTE ubH, ubS, ubV;
  ColorToRGB( m_colColor, ubR, ubG, ubB);
  ColorToHSV( m_colColor, ubH, ubS, ubV);
  UBYTE ubA = (UBYTE) (m_colColor&255);
  m_ubComponents[0][0] = ubH;
  m_ubComponents[0][1] = ubS;
  m_ubComponents[0][2] = ubV;
  m_ubComponents[0][3] = ubA;
  m_ubComponents[1][0] = ubR;
  m_ubComponents[1][1] = ubG;
  m_ubComponents[1][2] = ubB;
}

CColoredButton::~CColoredButton()
{
}


BEGIN_MESSAGE_MAP(CColoredButton, CButton)
	//{{AFX_MSG_MAP(CColoredButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_COPY_COLOR, OnCopyColor)
	ON_COMMAND(ID_PASTE_COLOR, OnPasteColor)
	ON_COMMAND(ID_NUMERIC_ALPHA, OnNumericAlpha)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CColoredButton::SetColor(COLOR clrNew)
{
  m_colColor = clrNew;
  m_bMixedColor = FALSE;
  if( ::IsWindow(m_hWnd)) Invalidate( FALSE);
}

void CColoredButton::ColorToComponents(void)
{
  if( m_colColor == m_colLastColor) return;
  UBYTE ubR, ubG, ubB;
  UBYTE ubH, ubS, ubV;
  ColorToRGB( m_colColor, ubR, ubG, ubB);
  ColorToHSV( m_colColor, ubH, ubS, ubV);

  UBYTE ubA = (UBYTE) (m_colColor&255);
  m_ubComponents[0][0] = ubH;
  m_ubComponents[0][1] = ubS;
  m_ubComponents[0][2] = ubV;
  m_ubComponents[0][3] = ubA;
  m_ubComponents[1][0] = ubR;
  m_ubComponents[1][1] = ubG;
  m_ubComponents[1][2] = ubB;
  m_colLastColor = m_colColor;
}

// CColoredButton message handlers
void CColoredButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
  EnableToolTips( TRUE);
  CDC *pDC = GetDC();
  m_rectButton = lpDrawItemStruct->rcItem;
  m_rectButton.top+=2;
  m_rectButton.bottom-=2;
  m_dx = (m_rectButton.right-m_rectButton.left)/6;
  m_dy = (m_rectButton.top-m_rectButton.bottom)/2;
  
  if( m_bMixedColor && IsWindowEnabled())
  {
    CBrush brush;
    brush.CreateHatchBrush(HS_BDIAGONAL, CLRF_CLR( RGBToColor(100,100,100)));
    pDC->FillRect( &m_rectButton, &brush);
  }
  else
  {
    ColorToComponents();
    UBYTE ubR, ubG, ubB;
    UBYTE ubH, ubS, ubV, ubA;
    ubH = m_ubComponents[0][0];
    ubS = m_ubComponents[0][1];
    ubV = m_ubComponents[0][2];
    ubA = m_ubComponents[0][3];
    ubR = m_ubComponents[1][0];
    ubG = m_ubComponents[1][1];
    ubB = m_ubComponents[1][2];

  #define FILL_RECT( col, x, y, w, h) {\
      RECT rectToFill;\
      rectToFill.left = m_rectButton.left+m_dx*x;\
      if( w<0) rectToFill.right = m_rectButton.right-2;\
      else rectToFill.right = m_rectButton.left+rectToFill.left+m_dx*w;\
      rectToFill.top = m_rectButton.top-m_dy*y;\
      rectToFill.bottom = m_rectButton.top+rectToFill.top-m_dy*h;\
      COLORREF clrfColor = CLRF_CLR( col);\
      if( !IsWindowEnabled()) clrfColor = CLRF_CLR( 0xBBBBBBBB);\
      pDC->FillSolidRect( &rectToFill, clrfColor);\
      pDC->DrawEdge( &rectToFill, EDGE_SUNKEN, BF_RECT);}

    FILL_RECT( HSVToColor( ubH, 255, 255), 0, 0, 1, 1);
    FILL_RECT( HSVToColor( ubH, ubS, 255), 1, 0, 1, 1);
    FILL_RECT( HSVToColor( ubH, 0, ubV), 2, 0, 1, 1);
    FILL_RECT( RGBToColor( ubA, ubA, ubA), 3, 0, 1, 2);
    FILL_RECT( RGBToColor( ubR, 0, 0), 0, 1, 1, 1);
    FILL_RECT( RGBToColor( 0, ubG, 0), 1, 1, 1, 1);
    FILL_RECT( RGBToColor( 0, 0, ubB), 2, 1, 1, 1);
    FILL_RECT( m_colColor, 4, 0, 2, 2);
  }

  pDC->DrawEdge( &lpDrawItemStruct->rcItem, EDGE_BUMP, BF_RECT);
  ReleaseDC( pDC);
}

void CColoredButton::OnClicked() 
{
  if( m_iColorIndex != -1) return;
  // colored button can call eather custom palette window for choosing colors (where variable
  // to receive result color is pointed with _pcolColorToSet) eather trough MFC-provided
  // color picker
  if( m_ptPickerType == PT_CUSTOM)
  {
    // instantiate new choose color palette window
    CColorPaletteWnd *pColorPalette = new CColorPaletteWnd;
    // calculate palette window's rectangle
    CRect rectWindow;
  
    CPoint ptMousePoint( 0,0);
    ClientToScreen( &ptMousePoint);

    // set screen coordinates of LU point of clicked tool button
    rectWindow.left = ptMousePoint.x;
    rectWindow.top = ptMousePoint.y - 200;
    rectWindow.right = rectWindow.left + 100;
    rectWindow.bottom = ptMousePoint.y;
    // create window
    BOOL bResult = pColorPalette->CreateEx( WS_EX_TOOLWINDOW,
      NULL, L"Palette", WS_CHILD|WS_POPUP|WS_VISIBLE,
      rectWindow.left, rectWindow.top, rectWindow.Width(), rectWindow.Height(),
      m_hWnd, NULL, NULL);
    if( !bResult)
    {
      AfxMessageBox( L"Error: Failed to create color palette");
      return;
    }
    // initialize canvas for active texture button
    _pGfx->CreateWindowCanvas( pColorPalette->m_hWnd, &pColorPalette->m_pViewPort,
                               &pColorPalette->m_pDrawPort);
    // get new color
    _pcolColorToSet = &m_colColor;
  }
  // request was made for MFC-type color picker
  else
  {
    COLORREF TmpColor = CLRF_CLR( m_colColor);
    if( MyChooseColor( TmpColor, *GetParent()))
    {
      m_bMixedColor = FALSE;
      // restore alpha value
      m_colColor = CLR_CLRF( TmpColor) | m_colColor&0x000000FF;
      Invalidate( FALSE);
    }
  }
  // invalidate parent dialog
  if( m_pwndParentDialog != NULL) m_pwndParentDialog->UpdateData( TRUE);
}

void CColoredButton::SetOverButtonInfo( CPoint point) 
{
#define SET_OVER_BUTTON_INFO( x, y, w, h, color_index, component_index) {\
  RECT rectToClick;\
  rectToClick.left = m_rectButton.left+m_dx*x;\
  if( w<0) rectToClick.right = m_rectButton.right-2;\
  else rectToClick.right = m_rectButton.left+rectToClick.left+m_dx*w;\
  rectToClick.top = m_rectButton.top-m_dy*y;\
  rectToClick.bottom = m_rectButton.top+rectToClick.top-m_dy*h;\
  if( PtInRect( &rectToClick, point)) {\
  m_iColorIndex = color_index; m_iComponentIndex = component_index;}}

  SET_OVER_BUTTON_INFO( 0, 0, 1, 1, 0, 0); // H
  SET_OVER_BUTTON_INFO( 1, 0, 1, 1, 0, 1); // S
  SET_OVER_BUTTON_INFO( 2, 0, 1, 1, 0, 2); // V
  SET_OVER_BUTTON_INFO( 3, 0, 1, 2, 0, 3); // A
  SET_OVER_BUTTON_INFO( 0, 1, 1, 1, 1, 0); // R
  SET_OVER_BUTTON_INFO( 1, 1, 1, 1, 1, 1); // G
  SET_OVER_BUTTON_INFO( 2, 1, 1, 1, 1, 2); // B
  SET_OVER_BUTTON_INFO( 4, 0, 2, 2,-1,-1); // Color
}

static void GetToolTipText(void *pwndColorButton, char *pToolTipText)
{
  CColoredButton *pthis = (CColoredButton *) pwndColorButton;

  UBYTE ubR, ubG, ubB;
  UBYTE ubH, ubS, ubV, ubA;
  ubH = pthis->m_ubComponents[0][0];
  ubS = pthis->m_ubComponents[0][1];
  ubV = pthis->m_ubComponents[0][2];
  ubA = pthis->m_ubComponents[0][3];
  ubR = pthis->m_ubComponents[1][0];
  ubG = pthis->m_ubComponents[1][1];
  ubB = pthis->m_ubComponents[1][2];

  if( (pthis->m_iColorIndex == 0) && (pthis->m_iComponentIndex == 3) ) // Alpha
  {
    sprintf(pToolTipText, "Alpha=%d", ubA);
  }
  else if( pthis->m_iColorIndex == 0) // HSV
  {
    sprintf(pToolTipText, "HSV=(%d,%d,%d)", ubH, ubS, ubV);
  }
  else // RGB
  {
    sprintf(pToolTipText, "RGB=(%d,%d,%d)", ubR, ubG, ubB);
  }
}

void CColoredButton::OnLButtonDown(UINT nFlags, CPoint point) 
{
  SetOverButtonInfo( point);
  if( m_bMixedColor)  m_iColorIndex = -1;

  if( m_iColorIndex != -1)
  {
    CRect rectWindow;
    GetWindowRect( &rectWindow);

    theApp.m_cttToolTips.ManualOn( rectWindow.left, rectWindow.bottom, &::GetToolTipText, this);

    m_ptCenter.x = ::GetSystemMetrics(SM_CXSCREEN)/2;
	  m_ptCenter.y = ::GetSystemMetrics(SM_CYSCREEN)/2;
    GetCursorPos( &m_ptStarting);

    _bMouseMoveEnabled = TRUE;
    _bColorChanging = TRUE;
    if( !_bCursorHidden)
    {
      // hide mouse
      while (ShowCursor(FALSE)>=0);
      _bCursorHidden = TRUE;
    }
    SetCursorPos(m_ptCenter.x, m_ptCenter.y);
  }
  CButton::OnLButtonDown(nFlags, point);
}

void CColoredButton::OnLButtonUp(UINT nFlags, CPoint point) 
{
  if( m_iColorIndex != -1)
  {
    theApp.m_cttToolTips.ManualOff();
    if( _bCursorHidden)
    {
      // show mouse
      while (ShowCursor(TRUE)<0);
      _bCursorHidden = FALSE;
    }
    _bMouseMoveEnabled = FALSE;
    SetCursorPos(m_ptStarting.x, m_ptStarting.y);
  }
  CButton::OnLButtonUp(nFlags, point);
  _bColorChanging = FALSE;
}

void CColoredButton::OnMouseMove(UINT nFlags, CPoint point) 
{
  if( (m_iColorIndex != -1) && (nFlags & MK_LBUTTON) && _bMouseMoveEnabled)
  {
    SetOverButtonInfo( point);
    EnableToolTips( TRUE);
  
    theApp.m_cttToolTips.ManualUpdate();

    CPoint ptCurrent;
    GetCursorPos( &ptCurrent);

    ColorToComponents();
    SLONG slResult = m_ubComponents[ m_iColorIndex][m_iComponentIndex];
    slResult += ptCurrent.x-m_ptCenter.x;
    slResult = Min(Max(slResult,0L), 255L);
    m_ubComponents[ m_iColorIndex][m_iComponentIndex] = UBYTE( slResult);

    COLOR colResult;
    if( m_iColorIndex == 0) {
      colResult = HSVToColor( m_ubComponents[0][0], m_ubComponents[0][1], m_ubComponents[0][2]);
      ColorToRGB(colResult, m_ubComponents[1][0], m_ubComponents[1][1], m_ubComponents[1][2]);
    } else {
      colResult = RGBToColor( m_ubComponents[1][0], m_ubComponents[1][1], m_ubComponents[1][2]);
      ColorToHSV(colResult, m_ubComponents[0][0], m_ubComponents[0][1], m_ubComponents[0][2]);
    }
    // add alpha
    colResult |= m_ubComponents[0][3];
    m_colColor = colResult;
    m_colLastColor = colResult;
    m_bMixedColor = FALSE;
    Invalidate( FALSE);
    SendMessage( WM_PAINT);
    // invalidate parent dialog
    if( m_pwndParentDialog != NULL) m_pwndParentDialog->UpdateData( TRUE);
    if(ptCurrent != m_ptCenter)
    {
      SetCursorPos( m_ptCenter.x, m_ptCenter.y);
    }
  }

  CButton::OnMouseMove(nFlags, point);
}

INT_PTR CColoredButton::OnToolHitTest( CPoint point, TOOLINFO* pTI ) const
{
  UBYTE ubR, ubG, ubB;
  UBYTE ubH, ubS, ubV, ubA;
  ubH = m_ubComponents[0][0];
  ubS = m_ubComponents[0][1];
  ubV = m_ubComponents[0][2];
  ubA = m_ubComponents[0][3];
  ubR = m_ubComponents[1][0];
  ubG = m_ubComponents[1][1];
  ubB = m_ubComponents[1][2];
  
  CTString strColor;
  if( m_bMixedColor)
  {
    strColor.PrintF( "Mixed color");
  }
  else
  {
    strColor.PrintF( "HSV=(%d,%d,%d),   RGB=(%d,%d,%d),    Alpha=%d", ubH, ubS, ubV, ubR, ubG, ubB, ubA);
  }
  pTI->lpszText = (wchar_t *)malloc( sizeof(wchar_t) * (strlen(strColor)+1));
  wcscpy( pTI->lpszText, CString(strColor));
  RECT rectToolTip;
  rectToolTip.left = 50;
  rectToolTip.right = 60;
  rectToolTip.top = 50;
  rectToolTip.bottom = 60;
  pTI->hwnd = GetParent()->m_hWnd;
  pTI->uId = (DWORD_PTR) m_hWnd;
  pTI->rect = rectToolTip;
  pTI->uFlags = TTF_IDISHWND;
  return 1;
}

void CColoredButton::OnContextMenu(CWnd* pWnd, CPoint point) 
{
  CMenu menu;
  if( menu.LoadMenu(IDR_COLOR_BUTTON))
  {
    CMenu* pPopup = menu.GetSubMenu(0);
    pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								 point.x, point.y, this);
  }
}

void CColoredButton::OnCopyColor() 
{
  _colColorClipboard = m_colColor;
}

void CColoredButton::OnPasteColor() 
{
  SetColor( _colColorClipboard);
  // invalidate parent dialog
  if( m_pwndParentDialog != NULL) m_pwndParentDialog->UpdateData( TRUE);
}

void CColoredButton::OnNumericAlpha() 
{
  CDlgNumericAlpha dlgNumericAlpha( GetColor()&0xFF);
  if( dlgNumericAlpha.DoModal() == IDOK)
  {
    m_ubComponents[0][3] = dlgNumericAlpha.m_iAlpha;
    m_colColor &= 0xFFFFFF00;
    m_colColor |= (dlgNumericAlpha.m_iAlpha & 0xFF);
    m_colLastColor = m_colColor;
    if( ::IsWindow(m_hWnd)) Invalidate( FALSE);
    // invalidate parent dialog
    if( m_pwndParentDialog != NULL) m_pwndParentDialog->UpdateData( TRUE);
  }
}

void CColoredButton::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
  OnNumericAlpha();
	CButton::OnLButtonDblClk(nFlags, point);
}

void CColoredButton::OnKillFocus(CWnd* pNewWnd) 
{
  if( (m_iColorIndex != -1) && (_bColorChanging) )
  {
    theApp.m_cttToolTips.ManualOff();
    _bColorChanging = FALSE;
    if( _bCursorHidden)
    {
      // show mouse
      while (ShowCursor(TRUE)<0);
      _bCursorHidden = FALSE;
    }
    _bMouseMoveEnabled = FALSE;
    SetCursorPos(m_ptStarting.x, m_ptStarting.y);
  }

	CButton::OnKillFocus(pNewWnd);
}
