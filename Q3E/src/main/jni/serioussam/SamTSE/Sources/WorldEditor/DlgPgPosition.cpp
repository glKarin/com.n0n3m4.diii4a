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

// DlgPgPosition.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgPosition.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgPosition property page

IMPLEMENT_DYNCREATE(CDlgPgPosition, CPropertyPage)

CDlgPgPosition::CDlgPgPosition() : CPropertyPage(CDlgPgPosition::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgPosition)
	m_fBanking = 0.0f;
	m_fHeading = 0.0f;
	m_fPitch = 0.0f;
	m_fX = 0.0f;
	m_fY = 0.0f;
	m_fZ = 0.0f;
	//}}AFX_DATA_INIT
}

CDlgPgPosition::~CDlgPgPosition()
{
}

void CDlgPgPosition::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

  CPropertyPage::DoDataExchange(pDX);

  SetModified( TRUE);

  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  // if document doesn't exist, return
  if( pDoc == NULL)  return;
  // get active view 
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();

  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
	  // is CSG on?
    if( pDoc->m_pwoSecondLayer != NULL)
    {
      // yes, pick up coordinates for editting from second layer
      m_fHeading = DegAngle( pDoc->m_plSecondLayer.pl_OrientationAngle(1));
	    m_fPitch   = DegAngle( pDoc->m_plSecondLayer.pl_OrientationAngle(2));
	    m_fBanking = DegAngle( pDoc->m_plSecondLayer.pl_OrientationAngle(3));

      m_fX = pDoc->m_plSecondLayer.pl_PositionVector(1);
      m_fY = pDoc->m_plSecondLayer.pl_PositionVector(2);
      m_fZ = pDoc->m_plSecondLayer.pl_PositionVector(3);
    }
    // otherwise if we are in entity mode and there is only one entity selected
    else if( (pDoc->m_iMode == ENTITY_MODE) && ( pDoc->m_selEntitySelection.Count() == 1) )
    {
      // lock selection's dynamic container
      pDoc->m_selEntitySelection.Lock();
      // get first entity
      CEntity *penEntityOne = pDoc->m_selEntitySelection.Pointer(0);
      // unlock selection's dynamic container
      pDoc->m_selEntitySelection.Unlock();

      // get placement of first entity
      CPlacement3D plEntityOnePlacement = penEntityOne->GetPlacement();
      m_fHeading = DegAngle( plEntityOnePlacement.pl_OrientationAngle(1));
	    m_fPitch   = DegAngle( plEntityOnePlacement.pl_OrientationAngle(2));
	    m_fBanking = DegAngle( plEntityOnePlacement.pl_OrientationAngle(3));

      m_fX = plEntityOnePlacement.pl_PositionVector(1);
      m_fY = plEntityOnePlacement.pl_PositionVector(2);
      m_fZ = plEntityOnePlacement.pl_PositionVector(3);
    }
    m_udSelection.MarkUpdated();
  }

	//{{AFX_DATA_MAP(CDlgPgPosition)
	DDX_Text(pDX, IDC_EDIT_BANKING, m_fBanking);
	DDX_Text(pDX, IDC_EDIT_HEADING, m_fHeading);
	DDX_Text(pDX, IDC_EDIT_PITCH, m_fPitch);
	DDX_Text(pDX, IDC_EDIT_X, m_fX);
	DDX_Text(pDX, IDC_EDIT_Y, m_fY);
	DDX_Text(pDX, IDC_EDIT_Z, m_fZ);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
	  // is CSG on?
    if( pDoc->m_pwoSecondLayer != NULL)
    {
      // yes, copy coordinates from editting controls into second layer
      pDoc->m_plSecondLayer.pl_OrientationAngle(1) = AngleDeg( m_fHeading);
	    pDoc->m_plSecondLayer.pl_OrientationAngle(2) = AngleDeg( m_fPitch);
	    pDoc->m_plSecondLayer.pl_OrientationAngle(3) = AngleDeg( m_fBanking);
      pDoc->m_plSecondLayer.pl_PositionVector(1) = m_fX;
      pDoc->m_plSecondLayer.pl_PositionVector(2) = m_fY;
      pDoc->m_plSecondLayer.pl_PositionVector(3) = m_fZ;

      // snap values to grid
      pDoc->SnapToGrid( pDoc->m_plSecondLayer, SNAP_FLOAT_12);
      theApp.m_vfpCurrent.vfp_plPrimitive = pDoc->m_plSecondLayer;

      // update all document's views
      pDoc->UpdateAllViews( NULL);
    }
    // otherwise if we are in entity mode
    else if( pDoc->m_iMode == ENTITY_MODE)
    {
      // there must be only one entity selected
      ASSERT( pDoc->m_selEntitySelection.Count() == 1);
      
      // lock selection's dynamic container
      pDoc->m_selEntitySelection.Lock();
      // get first entity
      CEntity *penEntityOne = pDoc->m_selEntitySelection.Pointer(0);
      // unlock selection's dynamic container
      pDoc->m_selEntitySelection.Unlock();

      // get placement of first entity
      CPlacement3D plEntityOnePlacement = penEntityOne->GetPlacement();
      plEntityOnePlacement.pl_OrientationAngle(1) = AngleDeg( m_fHeading);
	    plEntityOnePlacement.pl_OrientationAngle(2) = AngleDeg( m_fPitch);
	    plEntityOnePlacement.pl_OrientationAngle(3) = AngleDeg( m_fBanking);

      plEntityOnePlacement.pl_PositionVector(1) = m_fX;
      plEntityOnePlacement.pl_PositionVector(2) = m_fY;
      plEntityOnePlacement.pl_PositionVector(3) = m_fZ;

      // snap entity's placement
      pDoc->SnapToGrid( plEntityOnePlacement, SNAP_FLOAT_12);
      
      // set placement back to entity
      penEntityOne->SetPlacement( plEntityOnePlacement);

      pDoc->SetModifiedFlag( TRUE);
      pDoc->UpdateAllViews( NULL);
      m_udSelection.MarkUpdated();

      // update all document's views
      pDoc->UpdateAllViews( NULL);
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgPgPosition, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgPosition)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgPosition message handlers

BOOL CDlgPgPosition::OnIdle(LONG lCount)
{
  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( (pDoc == NULL) || !IsWindow(m_hWnd)) return FALSE;

  // if selections have been changed (they are not up to date)
  if( !pDoc->m_chSelections.IsUpToDate( m_udSelection))
  {
    // update dialog data
    UpdateData( FALSE);
  }

  return TRUE;
}

BOOL CDlgPgPosition::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
  {
    // move coordinates from page to entity and snap them
    UpdateData( TRUE);
    // place snapped coordinates back to dialog
    UpdateData( FALSE);
    // the message is handled
    return TRUE;
  }
	return CPropertyPage::PreTranslateMessage(pMsg);
}
