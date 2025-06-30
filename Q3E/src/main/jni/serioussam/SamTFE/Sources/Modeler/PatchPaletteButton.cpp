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

// PatchPaletteButton.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchPaletteButton

CPatchPaletteButton::CPatchPaletteButton()
{
}

CPatchPaletteButton::~CPatchPaletteButton()
{
}


BEGIN_MESSAGE_MAP(CPatchPaletteButton, CButton)
	//{{AFX_MSG_MAP(CPatchPaletteButton)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchPaletteButton message handlers

void CPatchPaletteButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
  CDC *pDC = GetDC();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if( pModelerView == NULL)
  {
    pDC->FillSolidRect( &lpDrawItemStruct->rcItem, 0x00aaaaaa);
    pDC->DrawEdge( &lpDrawItemStruct->rcItem, EDGE_RAISED, BF_RECT);
  }
  else
  {
    INDEX iButtonNo = lpDrawItemStruct->CtlID - IDC_PATCH_BUTTON_BASE;
    ULONG ulCurrentMask = pModelerView->m_ModelObject.GetPatchesMask();
    CModelerDoc* pDoc = pModelerView->GetDocument();
    ULONG ulExistingMask = pDoc->m_emEditModel.GetExistingPatchesMask();
    pDC->FillSolidRect( &lpDrawItemStruct->rcItem, 0x00aaaaaa);
    // If this patch doesn't exist
    if( (ulExistingMask & (1UL << iButtonNo)) == 0)
    {
      pDC->DrawEdge( &lpDrawItemStruct->rcItem, EDGE_RAISED, BF_RECT);
    }
    // If this patch exists but it is not turned on
    else if( (ulCurrentMask & (1UL << iButtonNo)) == 0)
    {
      // If this is active patch
      if( iButtonNo == pModelerView->m_iActivePatchBitIndex)
        pDC->DrawIcon( 2, 2, pMainFrame->m_dlgPatchesPalette->m_PatchActiveIcon);
      else
        pDC->DrawIcon( 2, 2, pMainFrame->m_dlgPatchesPalette->m_PatchExistIcon);
      pDC->DrawEdge( &lpDrawItemStruct->rcItem, EDGE_RAISED, BF_RECT);
    }
    // If this patch is turned on and it is active patch
    else if( iButtonNo == pModelerView->m_iActivePatchBitIndex)
    {
      pDC->DrawIcon( 2, 2, pMainFrame->m_dlgPatchesPalette->m_PatchActiveIcon);
      pDC->DrawEdge( &lpDrawItemStruct->rcItem, EDGE_SUNKEN, BF_RECT);
    }
    // If this patch is turned on and it is not active patch
    else
    {
      pDC->DrawIcon( 2 ,2, pMainFrame->m_dlgPatchesPalette->m_PatchInactiveIcon);
      pDC->DrawEdge( &lpDrawItemStruct->rcItem, EDGE_SUNKEN, BF_RECT);
    }
  }
  ReleaseDC( pDC);
}          


void CPatchPaletteButton::OnLButtonDown(UINT nFlags, CPoint point) 
{
	INDEX iButtonNo = GetDlgCtrlID()-IDC_PATCH_BUTTON_BASE;
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if( pModelerView != NULL)
  {
    ULONG ulCurrentMask = pModelerView->m_ModelObject.GetPatchesMask();
    CModelerDoc* pDoc = pModelerView->GetDocument();
    ULONG ulExistingMask = pDoc->m_emEditModel.GetExistingPatchesMask();
    if( (ulExistingMask & (1UL << iButtonNo)) != 0)
    {
      pModelerView->m_iActivePatchBitIndex = iButtonNo;
      CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
      pMainFrame->m_dlgPatchesPalette->UpdateData( FALSE);
      pMainFrame->m_dlgPatchesPalette->Invalidate( FALSE);
    }
  }

	CButton::OnLButtonDown(nFlags, point);
}

void CPatchPaletteButton::OnRButtonDown(UINT nFlags, CPoint point) 
{
	INDEX iButtonNo = GetDlgCtrlID()-IDC_PATCH_BUTTON_BASE;
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if( pModelerView != NULL)
  {
    ULONG ulCurrentMask = pModelerView->m_ModelObject.GetPatchesMask();
    CModelerDoc* pDoc = pModelerView->GetDocument();
    ULONG ulExistingMask = pDoc->m_emEditModel.GetExistingPatchesMask();
    if( (ulExistingMask & (1UL << iButtonNo)) != 0)
    {
      if( (ulCurrentMask & (1UL << iButtonNo)) != 0)
        pModelerView->m_ModelObject.HidePatch( iButtonNo);
      else
        pModelerView->m_ModelObject.ShowPatch( iButtonNo);
      CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
      pMainFrame->m_dlgPatchesPalette->Invalidate( FALSE);
    }
  }

	CButton::OnRButtonDown(nFlags, point);
}
