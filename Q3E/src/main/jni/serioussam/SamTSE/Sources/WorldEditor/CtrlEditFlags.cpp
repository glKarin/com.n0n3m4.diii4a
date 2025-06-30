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

// CtrlEditFlags.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "CtrlEditFlags.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditFlags

CCtrlEditFlags::CCtrlEditFlags()
{
  m_iCurrentBank=0;
  m_ulValue=0;
  m_ulDefined=-1;
  m_ulEditable=0;
  m_iLastArea=0;
  m_iMouseDownArea=0;
  for(INDEX iBit=0; iBit<32; iBit++)
  {
    CTString strDescription;
    strDescription.PrintF("Bit %d", iBit);
    SetBitDescription(iBit, strDescription);
  }
}

CCtrlEditFlags::~CCtrlEditFlags()
{
}

void CCtrlEditFlags::SetBitDescription(INDEX iBit, CTString strBitName)
{
  m_astrBitDescription[iBit]=strBitName;
}

void CCtrlEditFlags::SetDialogPtr( CWnd *pDialog)
{
  m_pDialog = pDialog;
}

void CCtrlEditFlags::SetEditableMask(ULONG ulEditable)
{
  m_ulEditable=ulEditable;
  SetFirstEditableBank();
  if( ::IsWindow(m_hWnd)) Invalidate( FALSE);
}

void CCtrlEditFlags::SetDefaultValue(ULONG ulDefault)
{
  m_ulDefault=ulDefault;
}

void CCtrlEditFlags::SetFlags(ULONG ulFlags)
{
  m_ulValue=ulFlags;
  m_ulDefined=-1;
  if( ::IsWindow(m_hWnd)) Invalidate( FALSE);
}

void CCtrlEditFlags::MergeFlags(ULONG ulFlags)
{
  m_ulDefined&=~(ulFlags^m_ulValue);
  if( ::IsWindow(m_hWnd)) Invalidate( FALSE);
}

void CCtrlEditFlags::ApplyChange(ULONG &ulOldFlags)
{
  ULONG ulKeepMask=~m_ulEditable|~m_ulDefined;
  ulOldFlags=(ulOldFlags&ulKeepMask)|(m_ulValue&~ulKeepMask);
}

void CCtrlEditFlags::SetPrevEditableBank( void)
{
  if(m_iCurrentBank==0 && m_ulEditable&0xff000000) m_iCurrentBank=3;
  if(m_iCurrentBank==1 && m_ulEditable&0x000000ff) m_iCurrentBank=0;
  if(m_iCurrentBank==2 && m_ulEditable&0x0000ff00) m_iCurrentBank=1;
  if(m_iCurrentBank==3 && m_ulEditable&0x00ff0000) m_iCurrentBank=2;
}

void CCtrlEditFlags::SetNextEditableBank( void)
{
  INDEX iOldCurrentBank=m_iCurrentBank;
  if(m_iCurrentBank==0 && m_ulEditable&0x0000ff00) m_iCurrentBank=1;
  if(m_iCurrentBank==1 && m_ulEditable&0x00ff0000) m_iCurrentBank=2;
  if(m_iCurrentBank==2 && m_ulEditable&0xff000000) m_iCurrentBank=3;
  if(m_iCurrentBank==3 && m_ulEditable&0x000000ff) m_iCurrentBank=0;
  
  if(iOldCurrentBank==m_iCurrentBank)
  {
    SetFirstEditableBank();
  }
  Invalidate(FALSE);
}

void CCtrlEditFlags::SetFirstEditableBank( void)
{
  m_iCurrentBank=0;
  if(m_ulEditable&0x000000ff) m_iCurrentBank=0;
  else if(m_ulEditable&0x0000ff00) m_iCurrentBank=1;
  else if(m_ulEditable&0x00ff0000) m_iCurrentBank=2;
  else if(m_ulEditable&0xff000000) m_iCurrentBank=3;
}

BEGIN_MESSAGE_MAP(CCtrlEditFlags, CButton)
	//{{AFX_MSG_MAP(CCtrlEditFlags)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtrlEditFlags message handlers
RECT CCtrlEditFlags::GetRectForArea(INDEX iArea) const
{
  RECT rect;
  rect.left = m_rectButton.left+m_dx*iArea;
  rect.right = m_rectButton.left+m_dx*(iArea+1);
  rect.top = m_rectButton.top;
  rect.bottom = m_rectButton.bottom;
  return rect;
}

void CCtrlEditFlags::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
  EnableToolTips( TRUE);
  CDC *pDC = GetDC();
  m_rectButton = lpDrawItemStruct->rcItem;
  COLORREF clrrefBcg=GetSysColor(COLOR_MENU);
  pDC->FillSolidRect(&m_rectButton, clrrefBcg);
  m_rectButton.top+=2;
  m_rectButton.bottom-=2;
  m_dx = (m_rectButton.right-m_rectButton.left)/9;

  INDEX ix=0;
  for( INDEX iBit=(m_iCurrentBank+1)*8-1; iBit>=m_iCurrentBank*8; iBit--)
  {
    RECT rectToFill=GetRectForArea(ix);
    ULONG ulBit=(1<<iBit);
    COLORREF clrfColor=CLRF_CLR( C_GRAY);
    // if is editable
    if( m_ulEditable&ulBit)
    {
      // if is not defined
      if(!(m_ulDefined&ulBit))
      {
        clrfColor=CLRF_CLR( C_GRAY);
      }
      else
      {
        // if is checked
        if(m_ulValue&ulBit)
        {
          clrfColor=CLRF_CLR( C_BLACK);
        }
        // it is cleared
        else
        {
          clrfColor=CLRF_CLR( C_WHITE);
        }
      }
      if( !IsWindowEnabled())
      {
        clrfColor=clrrefBcg;
      }
      pDC->FillSolidRect( &rectToFill, clrfColor);
      pDC->DrawEdge( &rectToFill, EDGE_SUNKEN, BF_RECT);
    }
    else
    {
      pDC->DrawEdge( &rectToFill, EDGE_ETCHED, BF_RECT);
    }
    ix++;
  }
  
  if( IsWindowEnabled())
  {
    RECT rectText=GetRectForArea(8);
    CTString strCTBankNo;
    strCTBankNo.PrintF("%d",m_iCurrentBank);
    CString strBankNo=strCTBankNo;
    pDC->SetBkMode( TRANSPARENT);
    pDC->SetTextAlign(TA_CENTER);
    pDC->ExtTextOut( (rectText.left+rectText.right)/2, rectText.top, ETO_CLIPPED, &rectText, strBankNo, 1, NULL);
  }
}

INDEX CCtrlEditFlags::GetAreaUnderMouse( CPoint point) const
{
  INDEX iResult=-1;
  for( INDEX iArea=0; iArea<=8; iArea++)
  {
    RECT rectToTest=GetRectForArea(iArea);
    if( PtInRect( &rectToTest, point))
    {
      iResult=iArea;
      break;;
    }
  }
  return iResult;
}

CTString CCtrlEditFlags::GetTipForArea(INDEX iArea) const
{
  CTString strResult;
  if( iArea==8)
  {
    strResult.PrintF("Bits (%d-%d)", m_iCurrentBank*8, (m_iCurrentBank+1)*8-1);
    return strResult;
  }
  else if( iArea!=-1)
  {
    ULONG ulBit=1<<((7-iArea)+m_iCurrentBank*8);
    if( m_ulEditable&ulBit)
    {
      INDEX iBit=(7-iArea)+m_iCurrentBank*8;
      return m_astrBitDescription[iBit];
    }
    else
    {
      strResult.PrintF("Not editable");
    }
    return strResult;
  }
  return "Not available";
}

INT_PTR CCtrlEditFlags::OnToolHitTest( CPoint point, TOOLINFO* pTI ) const
{
  INDEX iArea=GetAreaUnderMouse( point);
  if( iArea==-1) return 0;

  CTString strToolTip=GetTipForArea(iArea);
  pTI->lpszText = (wchar_t *)malloc( sizeof(wchar_t) * (strlen(strToolTip)+1));
  wcscpy( pTI->lpszText, CString(strToolTip));
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

void CCtrlEditFlags::OnLButtonDown(UINT nFlags, CPoint point) 
{
  INDEX iArea=GetAreaUnderMouse( point);
  m_iMouseDownArea=iArea;

	CButton::OnLButtonDown(nFlags, point);
}

void CCtrlEditFlags::OnLButtonUp(UINT nFlags, CPoint point) 
{
  INDEX iArea=GetAreaUnderMouse( point);
  if(m_iMouseDownArea!=iArea) return;
  if( iArea==8)
  {
    SetNextEditableBank();
  }
  else if( iArea>=0 && iArea<=7)
  {
    ULONG ulBit=1<<((7-iArea)+m_iCurrentBank*8);
    // if bit was defined
    if( m_ulDefined&ulBit)
    {
      // change bit state
      m_ulValue^=ulBit;
    }
    // set bit
    else
    {
      m_ulValue|=ulBit;
    }
    // note change
    if( m_pDialog!=NULL)
    {
      m_pDialog->UpdateData(TRUE);
    }
    m_ulDefined|=ulBit;
  }
  Invalidate(FALSE);

	CButton::OnLButtonUp(nFlags, point);
}

void CCtrlEditFlags::OnMouseMove(UINT nFlags, CPoint point) 
{
  m_iLastArea=GetAreaUnderMouse( point);
	CButton::OnMouseMove(nFlags, point);
}

void CCtrlEditFlags::OnKillFocus(CWnd* pNewWnd) 
{
	CButton::OnKillFocus(pNewWnd);
}
