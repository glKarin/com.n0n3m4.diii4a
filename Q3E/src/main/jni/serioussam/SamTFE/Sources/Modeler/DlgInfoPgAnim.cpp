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

// DlgInfoPgAnim.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgAnim property page

IMPLEMENT_DYNCREATE(CDlgInfoPgAnim, CPropertyPage)

CDlgInfoPgAnim::CDlgInfoPgAnim() : CPropertyPage(CDlgInfoPgAnim::IDD)
{
	//{{AFX_DATA_INIT(CDlgInfoPgAnim)
	m_strCurrentFrame = _T("");
	m_strFramesInAnim = _T("");
	m_strAnimName = _T("");
	m_strTimePassed = _T("");
	m_strAnimState = _T("");
	m_strAnimationLenght = _T("");
	m_strNoOfAnimations = _T("");
	m_strNoOfFrames = _T("");
	m_fAnimSpeed = 0.0f;
	//}}AFX_DATA_INIT

  theApp.m_pPgInfoAnim = this;
}

CDlgInfoPgAnim::~CDlgInfoPgAnim()
{
}

void CDlgInfoPgAnim::SetAnimPageFromView(CModelerView* pModelerView)
{
  CAnimInfo aiInfo;
  CModelInfo miModelInfo;
  char value[20];

  ASSERT( pModelerView != NULL);
  
  /* Get current model's info */
  pModelerView->m_ModelObject.GetModelInfo( miModelInfo);
  INDEX iMipModel = pModelerView->m_ModelObject.GetMipModel( pModelerView->m_fCurrentMipFactor);

  /* Number of animations in model */
  sprintf( value, "%d", pModelerView->m_ModelObject.GetAnimsCt());
  m_strNoOfAnimations = value;
  sprintf( value, "%d", miModelInfo.mi_FramesCt);
  m_strNoOfFrames = value;
  
  INDEX i = pModelerView->m_ModelObject.GetAnim();
  pModelerView->m_ModelObject.GetAnimInfo( i, aiInfo);
  
  FLOAT tmPassed = pModelerView->m_ModelObject.GetPassedTime();
  INDEX iCurrentFrame = pModelerView->m_ModelObject.GetFrame();
  BOOL bIsPaused = pModelerView->m_ModelObject.IsPaused();

  /* Now we will fill animation page variables ...*/
  m_strAnimName = aiInfo.ai_AnimName;
  
  sprintf( value, "%d", aiInfo.ai_NumberOfFrames);
  m_strFramesInAnim = value;

  sprintf( value, "%d (%d)", INDEX(tmPassed/aiInfo.ai_SecsPerFrame), iCurrentFrame);
  m_strCurrentFrame = value;

  sprintf( value, "%.4f", tmPassed);
  m_strTimePassed = value;

  sprintf( value, "%.4f", aiInfo.ai_NumberOfFrames*aiInfo.ai_SecsPerFrame);
  m_strAnimationLenght = value;

  if( bIsPaused)
  {
    m_strAnimState = "Paused";
  }
  else
  {
    m_strAnimState = "Playing";
  }
}

void CDlgInfoPgAnim::DoDataExchange(CDataExchange* pDX)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView == NULL) return;
  CModelerDoc* pDoc = theApp.GetDocument();

  // if dialog is recieving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {                    
    SetAnimPageFromView( pModelerView);
    // mark that the values have been updated to reflect the state of the view
    m_udAllValues.MarkUpdated();
  }
	
  CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfoPgAnim)
	DDX_Text(pDX, IDC_ANIM_CURRENT_FRAME, m_strCurrentFrame);
	DDX_Text(pDX, IDC_ANIM_FRAMES, m_strFramesInAnim);
	DDX_Text(pDX, IDC_ANIM_NAME, m_strAnimName); 
	DDX_Text(pDX, IDC_ANIM_TIME_PASSED, m_strTimePassed);
	DDX_Text(pDX, IDC_ANIM_STATE, m_strAnimState);
	DDX_Text(pDX, IDC_ANIMATION_LENGHT, m_strAnimationLenght);
	DDX_Text(pDX, IDC_NO_OF_ANIMATIONS, m_strNoOfAnimations);
	DDX_Text(pDX, IDC_NO_OF_FRAMES, m_strNoOfFrames);
	//}}AFX_DATA_MAP

  CAnimInfo aiInfo;
  INDEX iAnim = pModelerView->m_ModelObject.GetAnim();
  pModelerView->m_ModelObject.GetAnimInfo( iAnim, aiInfo);
  if(pDX->m_bSaveAndValidate == TRUE)
  {
    DDX_SkyFloat(pDX, IDC_ANIM_SPEED, m_fAnimSpeed);
  }
  else if( m_fAnimSpeed != aiInfo.ai_SecsPerFrame)
  {
    m_fAnimSpeed = aiInfo.ai_SecsPerFrame;
    DDX_SkyFloat(pDX, IDC_ANIM_SPEED, m_fAnimSpeed);
  }

  // if dialog is giving data
  if(pDX->m_bSaveAndValidate == TRUE)
  {                    
    if( (m_fAnimSpeed > 0) && (m_fAnimSpeed<100) )
    {
      pDoc->m_emEditModel.edm_md.SetSpeed( iAnim, m_fAnimSpeed);
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgInfoPgAnim, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgInfoPgAnim)
	ON_EN_CHANGE(IDC_ANIM_SPEED, OnChangeAnimSpeed)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgAnim message handlers

BOOL CDlgInfoPgAnim::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  ASSERT(pModelerView != NULL);

  // refresh info frame size
  ((CMainFrame *)( theApp.m_pMainWnd))->m_pInfoFrame->SetSizes();
  UpdateData(FALSE);
  return TRUE;
}

void CDlgInfoPgAnim::OnChangeAnimSpeed() 
{
  UpdateData(TRUE);
}

BOOL CDlgInfoPgAnim::OnSetActive() 
{
  m_fAnimSpeed = -1;
	return CPropertyPage::OnSetActive();
}
