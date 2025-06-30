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

// DlgPgCollision.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgCollision property page

IMPLEMENT_DYNCREATE(CDlgPgCollision, CPropertyPage)

CDlgPgCollision::CDlgPgCollision() : CPropertyPage(CDlgPgCollision::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgCollision)
	m_fWidth = 0.0f;
	m_fHeight = 0.0f;
	m_fLenght = 0.0f;
	m_fXCenter = 0.0f;
	m_fYDown = 0.0f;
	m_fZCenter = 0.0f;
	m_EqualityRadio = -1;
	m_strCollisionBoxName = _T("");
	m_strCollisionBoxIndex = _T("");
	m_bCollideAsBox = FALSE;
	//}}AFX_DATA_INIT

  theApp.m_pPgInfoCollision = this;
}

CDlgPgCollision::~CDlgPgCollision()
{
}

void CDlgPgCollision::DoDataExchange(CDataExchange* pDX)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  

  // if transfering data from document to dialog
  if( !pDX->m_bSaveAndValidate)
  {
    // get collision min vector
    FLOAT3D vMinCollisionBox = pDoc->m_emEditModel.GetCollisionBoxMin();
    // get collision max vector
    FLOAT3D vMaxCollisionBox = pDoc->m_emEditModel.GetCollisionBoxMax();

    FLOATaabbox3D bboxCollision = FLOATaabbox3D( vMinCollisionBox, vMaxCollisionBox);

    m_fWidth   = bboxCollision.Size()(1);
    m_fHeight  = bboxCollision.Size()(2);
    m_fLenght  = bboxCollision.Size()(3);
    m_fXCenter = bboxCollision.Center()(1);
    m_fYDown   = vMinCollisionBox(2);
    m_fZCenter = bboxCollision.Center()(3);
    
    // set equality radio initial value
    INDEX iEqualityType = pDoc->m_emEditModel.GetCollisionBoxDimensionEquality();

    // get index of curently selected collision box
    char achrString[ 256];
    sprintf( achrString, "%d.", pDoc->m_emEditModel.GetActiveCollisionBoxIndex());
    m_strCollisionBoxIndex = achrString;
    // get name of curently selected collision box
    sprintf( achrString, "%s", pDoc->m_emEditModel.GetCollisionBoxName());
    m_strCollisionBoxName = achrString;

    // enable all controls
    GetDlgItem( IDC_STATIC_WIDTH)->EnableWindow( TRUE);
    GetDlgItem( IDC_EDIT_WIDTH)->EnableWindow( TRUE);
    GetDlgItem( IDC_STATIC_HEIGHT)->EnableWindow( TRUE);
    GetDlgItem( IDC_EDIT_HEIGHT)->EnableWindow( TRUE);
    GetDlgItem( IDC_STATIC_LENGHT)->EnableWindow( TRUE);
    GetDlgItem( IDC_EDIT_LENGHT)->EnableWindow( TRUE);

    m_bCollideAsBox = pDoc->m_emEditModel.edm_md.md_bCollideAsCube;
    // if we are colliding using sphere approximation
    switch( iEqualityType)
    {
      case HEIGHT_EQ_WIDTH:
      {
        m_EqualityRadio = 0;
        if( !m_bCollideAsBox)
        {
          GetDlgItem( IDC_STATIC_HEIGHT)->EnableWindow( FALSE);
          GetDlgItem( IDC_EDIT_HEIGHT)->EnableWindow( FALSE);
          m_fHeight = m_fWidth;
        }
        break;
      }
      case LENGTH_EQ_WIDTH:
      {
        m_EqualityRadio = 1;
        if( !m_bCollideAsBox)
        {
          GetDlgItem( IDC_STATIC_LENGHT)->EnableWindow( FALSE);
          GetDlgItem( IDC_EDIT_LENGHT)->EnableWindow( FALSE);
          m_fLenght = m_fWidth;
        }
        break;
      }
      case LENGTH_EQ_HEIGHT:
      {
        m_EqualityRadio = 2;
        if( !m_bCollideAsBox)
        {
          GetDlgItem( IDC_STATIC_LENGHT)->EnableWindow( FALSE);
          GetDlgItem( IDC_EDIT_LENGHT)->EnableWindow( FALSE);
          m_fLenght = m_fHeight;
        }
        break;
      }
      default:
      {
        ASSERTALWAYS( "None of collision dimensions are the same and that can't be.");
      }
    }
    // mark that the values have been updated to reflect the state of the view
    m_udAllValues.MarkUpdated();
  }
    
  CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPgCollision)
	DDX_SkyFloat(pDX, IDC_EDIT_WIDTH, m_fWidth);
	DDX_SkyFloat(pDX, IDC_EDIT_HEIGHT, m_fHeight);
	DDX_SkyFloat(pDX, IDC_EDIT_LENGHT, m_fLenght);
	DDX_SkyFloat(pDX, IDC_EDIT_XCENTER, m_fXCenter);
	DDX_SkyFloat(pDX, IDC_EDIT_YDOWN, m_fYDown);
	DDX_SkyFloat(pDX, IDC_EDIT_ZCENTER, m_fZCenter);
	DDX_Radio(pDX, IDC_H_EQ_W, m_EqualityRadio);
	DDX_Text(pDX, IDC_COLLISION_BOX_NAME, m_strCollisionBoxName);
	DDX_Text(pDX, IDC_COLLISION_BOX_INDEX, m_strCollisionBoxIndex);
	DDX_Check(pDX, IDC_COLLIDE_AS_BOX, m_bCollideAsBox);
	//}}AFX_DATA_MAP

  // if transfering data from dialog to document
  if( pDX->m_bSaveAndValidate)
  {
    // if we are colliding using sphere approximation
    if( !pDoc->m_emEditModel.edm_md.md_bCollideAsCube)
    {
      INDEX iEqualityType;
      switch( m_EqualityRadio)
      {
        case 0:
        {
          iEqualityType = HEIGHT_EQ_WIDTH;
          CString strWidth;
          GetDlgItem( IDC_EDIT_WIDTH)->GetWindowText(strWidth);
          GetDlgItem( IDC_EDIT_HEIGHT)->SetWindowText(strWidth);
          break;
        }
        case 1:
        {
          iEqualityType = LENGTH_EQ_WIDTH;
          CString strWidth;
          GetDlgItem( IDC_EDIT_WIDTH)->GetWindowText(strWidth);
          GetDlgItem( IDC_EDIT_LENGHT)->SetWindowText( strWidth );
          break;
        }
        case 2:
        {
          iEqualityType = LENGTH_EQ_HEIGHT;
          CString strHeight;
          GetDlgItem( IDC_EDIT_HEIGHT)->GetWindowText(strHeight);
          GetDlgItem( IDC_EDIT_LENGHT)->SetWindowText( strHeight);
          break;
        }
        default:
        {
          ASSERTALWAYS( "Illegal value found in collision dimensions equality radio.");
        }
      }
      // set collision equality value
      if( pDoc->m_emEditModel.GetCollisionBoxDimensionEquality() != iEqualityType)
      {
        pDoc->m_emEditModel.SetCollisionBoxDimensionEquality( iEqualityType);
      }
    }

    // set name of curently selected collision box
    pDoc->m_emEditModel.SetCollisionBoxName( CTString( CStringA(m_strCollisionBoxName)) );
    
    // get collision min and max vectors
    FLOAT3D vMinCollisionBox;
    FLOAT3D vMaxCollisionBox;
    // get sizing values
    vMinCollisionBox(1) = m_fXCenter-m_fWidth/2.0f;
    vMinCollisionBox(2) = m_fYDown;
    vMinCollisionBox(3) = m_fZCenter-m_fLenght/2.0f;
    // get origin coordinates
    vMaxCollisionBox(1) = m_fXCenter+m_fWidth/2.0f; 
    vMaxCollisionBox(2) = m_fYDown+m_fHeight;
    vMaxCollisionBox(3) = m_fZCenter+m_fLenght/2.0f;
    
    // transfer data from dialog to document
    pDoc->m_emEditModel.SetCollisionBox( vMinCollisionBox, vMaxCollisionBox);

    pDoc->SetModifiedFlag();
    // update all views
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgPgCollision, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgCollision)
	ON_EN_CHANGE(IDC_EDIT_WIDTH, OnChangeEditWidth)
	ON_EN_CHANGE(IDC_EDIT_HEIGHT, OnChangeEditHeight)
	ON_EN_CHANGE(IDC_EDIT_LENGHT, OnChangeEditLenght)
	ON_EN_CHANGE(IDC_EDIT_XCENTER, OnChangeEditXCenter)
	ON_EN_CHANGE(IDC_EDIT_YDOWN, OnChangeEditYDown)
	ON_EN_CHANGE(IDC_EDIT_ZCENTER, OnChangeEditZCenter)
	ON_BN_CLICKED(IDC_H_EQ_W, OnHEqW)
	ON_BN_CLICKED(IDC_L_EQ_W, OnLEqW)
	ON_BN_CLICKED(IDC_L_EQ_H, OnLEqH)
	ON_BN_CLICKED(IDC_ADD_COLLISION_BOX, OnAddCollisionBox)
	ON_EN_CHANGE(IDC_COLLISION_BOX_NAME, OnChangeCollisionBoxName)
	ON_BN_CLICKED(IDC_NEXT_COLLISION_BOX, OnNextCollisionBox)
	ON_BN_CLICKED(IDC_PREVIOUS_COLLISION_BOX, OnPreviousCollisionBox)
	ON_BN_CLICKED(IDC_REMOVE_COLLISION_BOX, OnRemoveCollisionBox)
	ON_BN_CLICKED(IDC_COLLIDE_AS_BOX, OnCollideAsBox)
	ON_BN_CLICKED(IDC_ALLIGN_TO_SIZE, OnAllignToSize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgCollision message handlers

BOOL CDlgPgCollision::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  ASSERT(pModelerView != NULL);

  // update data
  if (!theApp.m_chGlobal.IsUpToDate(m_udAllValues) )
  {
    UpdateData(FALSE);
  }
  return TRUE;   
}

BOOL _bAvoidingLooping = FALSE;
void CDlgPgCollision::OnChangeEditWidth() 
{
	if( !_bAvoidingLooping)
  {
	  _bAvoidingLooping = TRUE;
    UpdateData(TRUE);
    //UpdateData(FALSE);
	  _bAvoidingLooping = FALSE;
  }
}

void CDlgPgCollision::OnChangeEditHeight() 
{
	if( !_bAvoidingLooping)
  {
	  _bAvoidingLooping = TRUE;
    UpdateData(TRUE);
    //UpdateData(FALSE);
	  _bAvoidingLooping = FALSE;
  }
}

void CDlgPgCollision::OnChangeEditLenght() 
{
	if( !_bAvoidingLooping)
  {
	  _bAvoidingLooping = TRUE;
    UpdateData(TRUE);
    //UpdateData(FALSE);
	  _bAvoidingLooping = FALSE;
  }
}

void CDlgPgCollision::OnChangeEditXCenter() 
{
	UpdateData(TRUE);	
  //UpdateData(FALSE);
}

void CDlgPgCollision::OnChangeEditYDown() 
{
	UpdateData(TRUE);	
  //UpdateData(FALSE);
}

void CDlgPgCollision::OnChangeEditZCenter() 
{
	UpdateData(TRUE);	
  //UpdateData(FALSE);
}

void CDlgPgCollision::OnHEqW() 
{
  m_EqualityRadio = 0;
	UpdateData(TRUE);	
	UpdateData(FALSE);	
}

void CDlgPgCollision::OnLEqW() 
{
  m_EqualityRadio = 1;
	UpdateData(TRUE);	
	UpdateData(FALSE);	
}

void CDlgPgCollision::OnLEqH() 
{
  m_EqualityRadio = 2;
	UpdateData(TRUE);	
	UpdateData(FALSE);	
}

void CDlgPgCollision::OnChangeCollisionBoxName() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  // document has been changed
  pDoc->SetModifiedFlag();
	UpdateData( TRUE);
}

void CDlgPgCollision::OnNextCollisionBox() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  pDoc->m_emEditModel.ActivateNextCollisionBox();
  UpdateData(FALSE);
  // update all views
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgCollision::OnPreviousCollisionBox() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  pDoc->m_emEditModel.ActivatePreviousCollisionBox();
  UpdateData(FALSE);
  // update all views
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgCollision::OnAddCollisionBox() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  pDoc->m_emEditModel.AddCollisionBox();
  UpdateData(FALSE);

  // document has been changed
  pDoc->SetModifiedFlag();
  // update all views
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgCollision::OnRemoveCollisionBox() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  pDoc->m_emEditModel.DeleteCurrentCollisionBox();
  UpdateData(FALSE);

  // document has been changed
  pDoc->SetModifiedFlag();
  // update all views
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgCollision::OnCollideAsBox() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();
  pDoc->m_emEditModel.edm_md.md_bCollideAsCube = !pDoc->m_emEditModel.edm_md.md_bCollideAsCube;
  UpdateData(TRUE);
  UpdateData(FALSE);
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}


void CDlgPgCollision::OnAllignToSize() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();
  FLOATaabbox3D MaxBB;
  pDoc->m_emEditModel.edm_md.GetAllFramesBBox( MaxBB);
  pDoc->m_emEditModel.SetCollisionBox( MaxBB.Min(), MaxBB.Max());
  UpdateData(FALSE);
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}
