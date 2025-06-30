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

// DlgPgInfoAttachingPlacement.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UPDATE_DATA_AND_REFRESH 				      \
  ASSERT( m_iActivePlacement != -1);				      \
  UpdateData(TRUE);						      \
  CModelerDoc* pDoc = theApp.GetDocument();			      \
  ASSERT( pDoc != NULL);					      \
  pDoc->UpdateAllViews( NULL);

/////////////////////////////////////////////////////////////////////////////
// CDlgPgInfoAttachingPlacement property page

IMPLEMENT_DYNCREATE(CDlgPgInfoAttachingPlacement, CPropertyPage)

CDlgPgInfoAttachingPlacement::CDlgPgInfoAttachingPlacement() : CPropertyPage(CDlgPgInfoAttachingPlacement::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgInfoAttachingPlacement)
	m_fBanking = 0.0f;
	m_fHeading = 0.0f;
	m_fPitch = 0.0f;
	m_fXOffset = 0.0f;
	m_fYOffset = 0.0f;
	m_fZOffset = 0.0f;
	m_strName = _T("");
	m_strAttachingVertices = _T("");
	m_strPlacementIndex = _T("");
	m_strAttachingModel = _T("");
	m_bIsVisible = FALSE;
	//}}AFX_DATA_INIT

  theApp.m_pPgAttachingPlacement = this;
  m_iActivePlacement = -1;
}

CDlgPgInfoAttachingPlacement::~CDlgPgInfoAttachingPlacement()
{
}

INDEX CDlgPgInfoAttachingPlacement::GetCurrentAttachingPlacement(void)
{
  return m_iActivePlacement;
}

void CDlgPgInfoAttachingPlacement::SetPlacementReferenceVertex(INDEX iCenter, INDEX iFront, INDEX iUp)
{
  // patch for calling before page is refreshed
  if(this == NULL) return;
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;

  pMD->md_aampAttachedPosition.Lock();
  CAttachedModelPosition &amp = pMD->md_aampAttachedPosition[ m_iActivePlacement];

  // --------- Set axis-defining vertices, but swap if owerlaping
  // Center vertex
  if( iCenter != -1)
  {
    if(amp.amp_iFrontVertex == iCenter)
    {
      amp.amp_iFrontVertex = amp.amp_iCenterVertex;
    }
    else if(amp.amp_iUpVertex == iCenter)
    {
      amp.amp_iUpVertex = amp.amp_iCenterVertex;
    }
    amp.amp_iCenterVertex = iCenter;
  }
  // Front vertex
  if( iFront != -1)
  {
    if(amp.amp_iCenterVertex == iFront)
    {
      amp.amp_iCenterVertex = amp.amp_iFrontVertex;
    }
    else if(amp.amp_iUpVertex == iFront)
    {
      amp.amp_iUpVertex = amp.amp_iFrontVertex;
    }
    amp.amp_iFrontVertex = iFront;
  }
  // Up vertex  
  if( iUp != -1)
  {
    if(amp.amp_iCenterVertex == iUp)
    {
      amp.amp_iCenterVertex = amp.amp_iUpVertex;
    }
    else if(amp.amp_iFrontVertex == iUp)
    {
      amp.amp_iFrontVertex = amp.amp_iUpVertex;
    }
    amp.amp_iUpVertex = iUp;
  }

  pMD->md_aampAttachedPosition.Unlock();

  theApp.m_chGlobal.MarkChanged();
  pDoc->ClearAttachments();
  pDoc->SetupAttachments();
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgInfoAttachingPlacement::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;

  INDEX ctPositions = pDoc->m_emEditModel.edm_aamAttachedModels.Count();
  if( (m_iActivePlacement == -1) && ( ctPositions != 0) ) m_iActivePlacement = 0;
  if( m_iActivePlacement >= ctPositions)
  {
    if( ctPositions != 0) m_iActivePlacement = 0;
    else                  m_iActivePlacement = -1;
  }

  // if transfering data from document to dialog
  if( !pDX->m_bSaveAndValidate)
  {
    
    BOOL bAttachmentExists = ( m_iActivePlacement != -1);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_INDEX_T     )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_PREVIOUS_ATTACHING_PLACEMENT    )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_NAME	      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_NEXT_ATTACHING_PLACEMENT	      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_REMOVE_ATTACHING_PLACEMENT      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_X_OFFSET_T  )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_X_OFFSET    )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_Y_OFFSET_T  )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_Y_OFFSET    )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_Z_OFFSET_T  )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_Z_OFFSET    )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_HEADING_T   )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_HEADING     )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_PITCH_T     )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_PITCH       )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_BANKING_T   )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_PLACEMENT_BANKING     )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_IS_VISIBLE		      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_BROWSE_MODEL		      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_MODEL_T			      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_MODEL_T			      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_MODEL_ANIMATION_T 	      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHMENT_MODEL_ANIMATION_COMBO)->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_VERTICES_T		      )->EnableWindow( bAttachmentExists);
    GetDlgItem( IDC_ATTACHING_VERTICES              )->EnableWindow( bAttachmentExists);
    
    if( bAttachmentExists)
    {
      pMD->md_aampAttachedPosition.Lock();
      pDoc->m_emEditModel.edm_aamAttachedModels.Lock();

      CPlacement3D plCurrent = pMD->md_aampAttachedPosition[ m_iActivePlacement].amp_plRelativePlacement;
	    m_fHeading	= DegAngle( plCurrent.pl_OrientationAngle(1));
	    m_fPitch	= DegAngle( plCurrent.pl_OrientationAngle(2));
      m_fBanking  = DegAngle( plCurrent.pl_OrientationAngle(3));
	    m_fXOffset	= plCurrent.pl_PositionVector(1);
	    m_fYOffset	= plCurrent.pl_PositionVector(2);
	    m_fZOffset	= plCurrent.pl_PositionVector(3);

      CAttachedModel *pam = &pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement];
      m_strName  = pam->am_strName;
      m_strAttachingModel = pam->am_moAttachedModel.GetName();
      char achrLine[ 256];
      sprintf( achrLine, "center:%d, front:%d, up:%d",
	pMD->md_aampAttachedPosition[ m_iActivePlacement].amp_iCenterVertex,
	pMD->md_aampAttachedPosition[ m_iActivePlacement].amp_iFrontVertex,
	pMD->md_aampAttachedPosition[ m_iActivePlacement].amp_iUpVertex);
      m_strAttachingVertices = achrLine;
      sprintf( achrLine, "%d.", m_iActivePlacement);
      m_strPlacementIndex = achrLine;

      m_bIsVisible =
	pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement].am_bVisible != 0;

      pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();
      pMD->md_aampAttachedPosition.Unlock();

      if( IsWindow( m_comboAttachmentModelAnimation.m_hWnd))
      {
	m_comboAttachmentModelAnimation.EnableWindow( m_bIsVisible);
	FillAttachmentModelAnimationCombo();
      }

      // mark that the values have been updated to reflect the state of the view
      m_udAllValues.MarkUpdated();
    }
  }

	//{{AFX_DATA_MAP(CDlgPgInfoAttachingPlacement)
	DDX_Control(pDX, IDC_ATTACHMENT_MODEL_ANIMATION_COMBO, m_comboAttachmentModelAnimation);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_BANKING, m_fBanking);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_HEADING, m_fHeading);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_PITCH, m_fPitch);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_X_OFFSET, m_fXOffset);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_Y_OFFSET, m_fYOffset);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_Z_OFFSET, m_fZOffset);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_NAME, m_strName);
	DDX_Text(pDX, IDC_ATTACHING_MODEL_T, m_strAttachingModel);
	DDX_Text(pDX, IDC_ATTACHING_VERTICES, m_strAttachingVertices);
	DDX_Text(pDX, IDC_ATTACHING_PLACEMENT_INDEX_T, m_strPlacementIndex);
	DDX_Check(pDX, IDC_IS_VISIBLE, m_bIsVisible);
	//}}AFX_DATA_MAP
  // if transfering data from dialog to document

  if( pDX->m_bSaveAndValidate)
  {
    if( m_iActivePlacement == -1) return;
    pMD->md_aampAttachedPosition.Lock();
    pDoc->m_emEditModel.edm_aamAttachedModels.Lock();

    CPlacement3D plCurrent;
	  plCurrent.pl_OrientationAngle(1) = AngleDeg( m_fHeading);
	  plCurrent.pl_OrientationAngle(2) = AngleDeg( m_fPitch);
    plCurrent.pl_OrientationAngle(3) = AngleDeg( m_fBanking);
	  plCurrent.pl_PositionVector(1) = m_fXOffset;
	  plCurrent.pl_PositionVector(2) = m_fYOffset;
	  plCurrent.pl_PositionVector(3) = m_fZOffset;
    pMD->md_aampAttachedPosition[ m_iActivePlacement].amp_plRelativePlacement = plCurrent;
    pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement].am_strName = CStringA(m_strName);
    pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement].am_bVisible = m_bIsVisible;
    pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();
    pMD->md_aampAttachedPosition.Unlock();

    pDoc->ClearAttachments();
    pDoc->SetupAttachments();

    pDoc->SetModifiedFlag();
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgPgInfoAttachingPlacement, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgInfoAttachingPlacement)
	ON_BN_CLICKED(IDC_ADD_ATTACHING_PLACEMENT, OnAddAttachingPlacement)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_BANKING, OnChangeAttachingPlacementBanking)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_HEADING, OnChangeAttachingPlacementHeading)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_NAME, OnChangeAttachingPlacementName)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_PITCH, OnChangeAttachingPlacementPitch)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_X_OFFSET, OnChangeAttachingPlacementXOffset)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_Y_OFFSET, OnChangeAttachingPlacementYOffset)
	ON_EN_CHANGE(IDC_ATTACHING_PLACEMENT_Z_OFFSET, OnChangeAttachingPlacementZOffset)
	ON_BN_CLICKED(IDC_BROWSE_MODEL, OnBrowseModel)
	ON_BN_CLICKED(IDC_NEXT_ATTACHING_PLACEMENT, OnNextAttachingPlacement)
	ON_BN_CLICKED(IDC_PREVIOUS_ATTACHING_PLACEMENT, OnPreviousAttachingPlacement)
	ON_BN_CLICKED(IDC_REMOVE_ATTACHING_PLACEMENT, OnRemoveAttachingPlacement)
	ON_BN_CLICKED(IDC_IS_VISIBLE, OnIsVisible)
	ON_CBN_SELCHANGE(IDC_ATTACHMENT_MODEL_ANIMATION_COMBO, OnSelchangeAttachmentModelAnimationCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgInfoAttachingPlacement message handlers

BOOL CDlgPgInfoAttachingPlacement::OnIdle(LONG lCount)
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


void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementXOffset()
{
	UPDATE_DATA_AND_REFRESH;
}

void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementYOffset()
{
	UPDATE_DATA_AND_REFRESH;
}

void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementZOffset()
{
	UPDATE_DATA_AND_REFRESH;
}

void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementHeading()
{
	UPDATE_DATA_AND_REFRESH;
}

void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementPitch()
{
	UPDATE_DATA_AND_REFRESH;
}

void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementBanking()
{
	UPDATE_DATA_AND_REFRESH;
}

void CDlgPgInfoAttachingPlacement::OnChangeAttachingPlacementName()
{
	UPDATE_DATA_AND_REFRESH;
}

BOOL CDlgPgInfoAttachingPlacement::BrowseAttachement( CAttachedModel *pam)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return FALSE;
  CModelerDoc* pDoc = pModelerView->GetDocument();

  CTFileName fnOldModel = pam->am_moAttachedModel.GetName();
  CTFileName fnModel = _EngineGUI.FileRequester( "Select model to attach",
				  FILTER_MDL FILTER_END, "Attaching models directory",
				  _fnmApplicationPath + fnOldModel.FileDir(),
				  fnOldModel.FileName()+fnOldModel.FileExt());
  if( fnModel == "") return FALSE;

  try
  {
    pam->SetModel_t( fnModel);
  }
  catch( char *strError)
  {
    (void) strError;
    return FALSE;
  }
  pam->am_bVisible = TRUE;

  pDoc->ClearAttachments();
  pDoc->SetupAttachments();

  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);

  UpdateData(FALSE);
  return TRUE;
}

void CDlgPgInfoAttachingPlacement::OnAddAttachingPlacement()
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;
  CAttachedModelPosition *ampModelPosition = pMD->md_aampAttachedPosition.New();
  CAttachedModel *pAttachedModel = pDoc->m_emEditModel.edm_aamAttachedModels.New();
  if( !BrowseAttachement( pAttachedModel))
  {
    pMD->md_aampAttachedPosition.Delete( ampModelPosition);
    pDoc->m_emEditModel.edm_aamAttachedModels.Delete( pAttachedModel);
    return;
  }
  m_iActivePlacement = pMD->md_aampAttachedPosition.Count()-1;
}

void CDlgPgInfoAttachingPlacement::OnBrowseModel()
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;

  if( m_iActivePlacement != -1)
  {
    pDoc->m_emEditModel.edm_aamAttachedModels.Lock();
    BrowseAttachement( &pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement]);
    pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();
  }
}

void CDlgPgInfoAttachingPlacement::OnPreviousAttachingPlacement()
{
  if( m_iActivePlacement <= 0) return;
  m_iActivePlacement -= 1;
  UpdateData(FALSE);
  CModelerDoc* pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);
  if( pDoc == NULL) return;
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgInfoAttachingPlacement::OnNextAttachingPlacement()
{
  CModelerDoc* pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);
  if( pDoc == NULL) return;
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;
  if( m_iActivePlacement < pDoc->m_emEditModel.edm_aamAttachedModels.Count()-1)
  {
    m_iActivePlacement += 1;
    UpdateData(FALSE);
    pDoc->UpdateAllViews( NULL);
  }
}

void CDlgPgInfoAttachingPlacement::OnRemoveAttachingPlacement()
{
  ASSERT( m_iActivePlacement != -1);
  CModelerDoc* pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);
  if( pDoc == NULL) return;
  CModelData *pMD = &pDoc->m_emEditModel.edm_md;

  pMD->md_aampAttachedPosition.Lock();
  pDoc->m_emEditModel.edm_aamAttachedModels.Lock();

  // get currently active placement from edit model
  CAttachedModel *pamAttachedModel =
    &pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement];
  // and from model data
  CAttachedModelPosition *pampModelPosition =
    &pMD->md_aampAttachedPosition[ m_iActivePlacement];

  pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();
  pMD->md_aampAttachedPosition.Unlock();

  pDoc->ClearAttachments();

  pDoc->m_emEditModel.edm_aamAttachedModels.Delete( pamAttachedModel);
  pMD->md_aampAttachedPosition.Delete( pampModelPosition);

  pDoc->SetupAttachments();

  if( pDoc->m_emEditModel.edm_aamAttachedModels.Count() == 0)
  {
    m_iActivePlacement = -1;
  }
  if( m_iActivePlacement == pDoc->m_emEditModel.edm_aamAttachedModels.Count())
  {
    m_iActivePlacement = pDoc->m_emEditModel.edm_aamAttachedModels.Count()-1;
  }
  UpdateData(FALSE);
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgInfoAttachingPlacement::FillAttachmentModelAnimationCombo()
{
  if( m_iActivePlacement == -1) return;
  CModelerDoc* pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);
  if( pDoc == NULL) return;
  m_comboAttachmentModelAnimation.ResetContent();

  ASSERT( m_iActivePlacement < pDoc->m_emEditModel.edm_aamAttachedModels.Count());
  // obtain info about active attached model
  pDoc->m_emEditModel.edm_aamAttachedModels.Lock();
  CAttachedModel *pamAttachedModel = &pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement];
  pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();

  CModelData *pMD = (CModelData *) pamAttachedModel->am_moAttachedModel.GetData();
  ASSERT(pMD != NULL);

  for( INDEX iAnim=0; iAnim<pMD->GetAnimsCt(); iAnim++)
  {
    CAnimInfo aiInfo;
    pMD->GetAnimInfo( iAnim, aiInfo);
    m_comboAttachmentModelAnimation.AddString( CString(aiInfo.ai_AnimName));
  }
  m_comboAttachmentModelAnimation.SetCurSel(pamAttachedModel->am_iAnimation);
}

void CDlgPgInfoAttachingPlacement::OnSelchangeAttachmentModelAnimationCombo()
{
  if( m_iActivePlacement == -1) return;
  CModelerDoc* pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);
  if( pDoc == NULL) return;
  INDEX iCombo = m_comboAttachmentModelAnimation.GetCurSel();
  if( iCombo != CB_ERR)
  {
    pDoc->m_emEditModel.edm_aamAttachedModels.Lock();
    CAttachedModel *pamAttachedModel =
      &pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement];
    pamAttachedModel->am_iAnimation = iCombo;
    pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();
  }
  pDoc->ClearAttachments();
  pDoc->SetupAttachments();
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
}

void CDlgPgInfoAttachingPlacement::OnIsVisible()
{
  if( m_iActivePlacement == -1) return;
  CModelerDoc* pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);
  if( pDoc == NULL) return;

  pDoc->m_emEditModel.edm_aamAttachedModels.Lock();
  CAttachedModel *pamAttachedModel =
    &pDoc->m_emEditModel.edm_aamAttachedModels[ m_iActivePlacement];
  pamAttachedModel->am_bVisible = !pamAttachedModel->am_bVisible;

  pDoc->m_emEditModel.edm_aamAttachedModels.Unlock();
  pDoc->ClearAttachments();
  pDoc->SetupAttachments();
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);

  UpdateData(FALSE);
}
