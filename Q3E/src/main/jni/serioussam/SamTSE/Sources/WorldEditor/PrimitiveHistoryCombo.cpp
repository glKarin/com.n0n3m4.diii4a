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

// PrimitiveHistoryCombo.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "PrimitiveHistoryCombo.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPrimitiveHistoryCombo

CPrimitiveHistoryCombo::CPrimitiveHistoryCombo()
{
}

CPrimitiveHistoryCombo::~CPrimitiveHistoryCombo()
{
}


BEGIN_MESSAGE_MAP(CPrimitiveHistoryCombo, CComboBox)
	//{{AFX_MSG_MAP(CPrimitiveHistoryCombo)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrimitiveHistoryCombo message handlers

void CPrimitiveHistoryCombo::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
  INDEX iItem = lpDrawItemStruct->itemID;
  BOOL bSelected = lpDrawItemStruct->itemState & (ODS_FOCUS|ODS_SELECTED);
  COLORREF clrfBcgColor;
  UINT ulEdgeType;
  if( bSelected)
  {
    clrfBcgColor = CLRF_CLR( 0xDFDFFF00UL); 
    ulEdgeType = EDGE_SUNKEN;
  }
  else
  {
    clrfBcgColor = CLRF_CLR( C_WHITE);
    ulEdgeType = EDGE_RAISED;
  }
  
  CValuesForPrimitive *pVFP = (CValuesForPrimitive *) GetItemData( iItem);
  if( (ULONG_PTR) pVFP == CB_ERR) return;

  CDC *pDC = CDC::FromHandle( lpDrawItemStruct->hDC);
  RECT rectItem = lpDrawItemStruct->rcItem;
  pDC->SetBkMode( TRANSPARENT);
  pDC->SetBkColor( clrfBcgColor);
  pDC->FillSolidRect( &rectItem, clrfBcgColor);
  
  // polygon color
  RECT rectPolygon = rectItem;
  rectPolygon.top+=1;
  rectPolygon.bottom-=1;
  rectPolygon.left += 2;
  rectPolygon.right = rectPolygon.left+16;
  COLORREF clrfColor = CLRF_CLR( pVFP->vfp_colPolygonsColor);
  pDC->FillSolidRect( &rectPolygon, clrfColor);

  // sector color
  RECT rectSector = rectPolygon;
  rectSector.left += 16;
  rectSector.right += 16;
  clrfColor = CLRF_CLR( pVFP->vfp_colSectorsColor);
  pDC->FillSolidRect( &rectSector, clrfColor);

  RECT rectBorder = rectPolygon;
  rectBorder.right += 16;
  pDC->DrawEdge( &rectBorder, ulEdgeType, BF_RECT);

  // used for rendering from left to right
  PIX pixCursor = 38;
  
  UINT uiPrimitiveIconID;
  switch( pVFP->vfp_ptPrimitiveType)
  {
  case PT_CONUS:{       uiPrimitiveIconID = IDR_ICON_CONUS; break;}
  case PT_TORUS:{       uiPrimitiveIconID = IDR_ICON_TORUS; break;}
  case PT_STAIRCASES:
    {
    if( pVFP->vfp_bLinearStaircases) uiPrimitiveIconID = IDR_ICON_LINEAR_STAIRCASES;
    else                             uiPrimitiveIconID = IDR_ICON_SPIRAL_STAIRCASES; 
    break;
    }
  case PT_SPHERE:{      uiPrimitiveIconID = IDR_ICON_SPHERE; break;}
  case PT_TERRAIN:{     uiPrimitiveIconID = IDR_ICON_TERRAIN; break;}
  }
  HICON hPrimitiveIcon = theApp.LoadIcon( uiPrimitiveIconID);
  pDC->DrawIcon( pixCursor, rectItem.top, hPrimitiveIcon);
  pixCursor += 22;

  UINT uiPrimitiveOperationID;
  switch( pVFP->vfp_csgtCSGOperation)
  {
  case CSG_ADD: {             uiPrimitiveOperationID = IDR_ICON_ADD; break;}
  case CSG_ADD_REVERSE: {     uiPrimitiveOperationID = IDR_ICON_ADD_REVERSE; break;}
  case CSG_REMOVE:  {         uiPrimitiveOperationID = IDR_ICON_REMOVE; break;}
  case CSG_REMOVE_REVERSE:  { uiPrimitiveOperationID = IDR_ICON_REMOVE_REVERSE; break;}
  case CSG_SPLIT_SECTORS: {   uiPrimitiveOperationID = IDR_ICON_SPLIT_SECTORS; break;}
  case CSG_SPLIT_POLYGONS:  { uiPrimitiveOperationID = IDR_ICON_SPLIT_POLYGONS; break;}
  case CSG_JOIN_LAYERS: {     uiPrimitiveOperationID = IDR_ICON_JOIN_LAYERS; break;}
  }
  HICON hOperationIcon = theApp.LoadIcon( uiPrimitiveOperationID);
  pDC->DrawIcon( pixCursor, rectItem.top, hOperationIcon);
  pixCursor += 22;

  HICON hRoomIcon;
  if( pVFP->vfp_bClosed) hRoomIcon = theApp.LoadIcon( IDR_ICON_ROOM);
  else                   hRoomIcon = theApp.LoadIcon( IDR_ICON_MATERIAL);
  pDC->DrawIcon( pixCursor, rectItem.top, hRoomIcon);
  pixCursor += 22;

  if( !pVFP->vfp_bOuter &&
      (pVFP->vfp_csgtCSGOperation != PT_SPHERE) &&
      (pVFP->vfp_csgtCSGOperation != PT_TERRAIN) )
  {
    HICON hOuterIcon;
    hOuterIcon = theApp.LoadIcon( IDR_ICON_INNER);
    pDC->DrawIcon( pixCursor, rectItem.top, hOuterIcon);
    pixCursor += 22;
  }

  if( pVFP->vfp_ptPrimitiveType == PT_STAIRCASES)
  {
    HICON hTopStairsIcon;
    if(      pVFP->vfp_iTopShape == 0) hTopStairsIcon = theApp.LoadIcon( IDR_ICON_TOP_STAIRS);
    else if( pVFP->vfp_iTopShape == 1) hTopStairsIcon = theApp.LoadIcon( IDR_ICON_TOP_SLOPE);
    else                               hTopStairsIcon = theApp.LoadIcon( IDR_ICON_TOP_CEILING);
    pDC->DrawIcon( pixCursor, rectItem.top, hTopStairsIcon);
    pixCursor += 22;

    HICON hBottomStairsIcon;
    if(      pVFP->vfp_iTopShape == 0) hBottomStairsIcon = theApp.LoadIcon( IDR_ICON_BOTTOM_STAIRS);
    else if( pVFP->vfp_iTopShape == 1) hBottomStairsIcon = theApp.LoadIcon( IDR_ICON_BOTTOM_SLOPE);
    else                               hBottomStairsIcon = theApp.LoadIcon( IDR_ICON_BOTTOM_FLOOR);
    pDC->DrawIcon( pixCursor, rectItem.top, hBottomStairsIcon);
    pixCursor += 22;
  }

  // obtain text of drawing item
  CString strText;
  GetLBText( iItem, strText);
  pDC->TextOut( pixCursor, rectItem.top, strText);
}

void CPrimitiveHistoryCombo::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
}
