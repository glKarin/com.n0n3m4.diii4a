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

// DlgInfoPgPos.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgPos property page

IMPLEMENT_DYNCREATE(CDlgInfoPgPos, CPropertyPage)

CDlgInfoPgPos::CDlgInfoPgPos() : CPropertyPage(CDlgInfoPgPos::IDD)
{
	//{{AFX_DATA_INIT(CDlgInfoPgPos)
	m_fLightDist = 0.0f;
	m_fHeading = 0.0f;
	m_fPitch = 0.0f;
	m_fBanking = 0.0f;
	m_fX = 0.0f;
	m_fY = 0.0f;
	m_fZ = 0.0f;
	m_fFOW = 0.0f;
	//}}AFX_DATA_INIT

  theApp.m_pPgInfoPos = this;
}

CDlgInfoPgPos::~CDlgInfoPgPos()
{
}

void CDlgInfoPgPos::DoDataExchange(CDataExchange* pDX)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  
  if( !pDX->m_bSaveAndValidate) {
    m_fHeading = DegAngle(pModelerView->m_plModelPlacement.pl_OrientationAngle(1));
    m_fPitch   = DegAngle(pModelerView->m_plModelPlacement.pl_OrientationAngle(2));
    m_fBanking = DegAngle(pModelerView->m_plModelPlacement.pl_OrientationAngle(3));

    m_fX = (FLOAT)(pModelerView->m_plModelPlacement.pl_PositionVector(1));
    m_fY = (FLOAT)(pModelerView->m_plModelPlacement.pl_PositionVector(2));
    m_fZ = -(FLOAT)(pModelerView->m_plModelPlacement.pl_PositionVector(3));
    
    m_fLightDist = pModelerView->m_LightDistance;
    m_fFOW = pModelerView->m_fFOW;
    // mark that the values have been updated to reflect the state of the view
    m_udAllValues.MarkUpdated();
  }

  CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfoPgPos)
	DDX_Text(pDX, IDC_EDIT_LIGHT_DISTANCE, m_fLightDist);
	DDV_MinMaxFloat(pDX, m_fLightDist, 0.5f, 40.f);
	DDX_SkyFloat(pDX, IDC_EDIT_HEADING, m_fHeading);
	DDX_SkyFloat(pDX, IDC_EDIT_PITCH, m_fPitch);
	DDX_SkyFloat(pDX, IDC_EDIT_BANKING, m_fBanking);
	DDX_SkyFloat(pDX, IDC_EDIT_X, m_fX);
	DDX_SkyFloat(pDX, IDC_EDIT_Y, m_fY);
	DDX_SkyFloat(pDX, IDC_EDIT_Z, m_fZ);
	DDX_Text(pDX, IDC_EDIT_FOW, m_fFOW);
	DDV_MinMaxFloat(pDX, m_fFOW, 1.f, 179.f);
	//}}AFX_DATA_MAP

  if( pDX->m_bSaveAndValidate) {
    pModelerView->m_plModelPlacement.pl_OrientationAngle(1) = AngleDeg( (FLOAT)m_fHeading);
    pModelerView->m_plModelPlacement.pl_OrientationAngle(2) = AngleDeg( (FLOAT)m_fPitch);
    pModelerView->m_plModelPlacement.pl_OrientationAngle(3) = AngleDeg( (FLOAT)m_fBanking);
    pModelerView->m_plModelPlacement.pl_PositionVector(1) = (FLOAT)m_fX;
    pModelerView->m_plModelPlacement.pl_PositionVector(2) = (FLOAT)m_fY;
    pModelerView->m_plModelPlacement.pl_PositionVector(3) = -(FLOAT)m_fZ;
    pModelerView->m_LightDistance = m_fLightDist;
    pModelerView->m_fFOW = m_fFOW;
    pModelerView->m_plModelPlacement.pl_PositionVector(3) = -(FLOAT)m_fZ;
    pModelerView->Invalidate( FALSE);
  }
}


BEGIN_MESSAGE_MAP(CDlgInfoPgPos, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgInfoPgPos)
	ON_EN_CHANGE(IDC_EDIT_HEADING, OnChangeEditHeading)
	ON_EN_CHANGE(IDC_EDIT_BANKING, OnChangeEditBanking)
	ON_EN_CHANGE(IDC_EDIT_PITCH, OnChangeEditPitch)
	ON_EN_CHANGE(IDC_EDIT_X, OnChangeEditX)
	ON_EN_CHANGE(IDC_EDIT_Y, OnChangeEditY)
	ON_EN_CHANGE(IDC_EDIT_Z, OnChangeEditZ)
	ON_EN_CHANGE(IDC_EDIT_LIGHT_DISTANCE, OnChangeEditLightDistance)
	ON_EN_CHANGE(IDC_EDIT_FOW, OnChangeEditFow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgPos message handlers

BOOL CDlgInfoPgPos::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  ASSERT(pModelerView != NULL);

  if (!theApp.m_chGlobal.IsUpToDate(m_udAllValues) ||
      !theApp.m_chPlacement.IsUpToDate(m_udAllValues) ||
      !pModelerView->m_ModelObject.IsUpToDate(m_udAllValues)) {
    UpdateData(FALSE);
  }

  // refresh info frame size
  ((CMainFrame *)( theApp.m_pMainWnd))->m_pInfoFrame->SetSizes();
  return TRUE;   
}

void CDlgInfoPgPos::OnChangeEditHeading() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditBanking() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditPitch() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditX() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditY() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditZ() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditLightDistance() 
{
	UpdateData(TRUE);	
}

void CDlgInfoPgPos::OnChangeEditFow() 
{
	UpdateData(TRUE);	
}
