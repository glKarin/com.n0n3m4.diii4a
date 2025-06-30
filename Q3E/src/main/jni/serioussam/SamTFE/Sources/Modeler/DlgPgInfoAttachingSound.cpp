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

// DlgPgInfoAttachingSound.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgInfoAttachingSound property page

IMPLEMENT_DYNCREATE(CDlgPgInfoAttachingSound, CPropertyPage)

CDlgPgInfoAttachingSound::CDlgPgInfoAttachingSound() : CPropertyPage(CDlgPgInfoAttachingSound::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgInfoAttachingSound)
	m_strAttachedSound = _T("");
	m_bLooping = FALSE;
	m_bPlaying = FALSE;
	m_fDelay = 0.0f;
	//}}AFX_DATA_INIT
}

CDlgPgInfoAttachingSound::~CDlgPgInfoAttachingSound()
{
}

void CDlgPgInfoAttachingSound::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

  CModelerView *pModelerView = CModelerView::GetActiveView();
  if(pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  CAttachedSound &asSound = pDoc->m_emEditModel.edm_aasAttachedSounds[pModelerView->m_ModelObject.GetAnim()];

  // if transfering data from document to dialog
  if( !pDX->m_bSaveAndValidate)
  {
    m_strAttachedSound = asSound.as_fnAttachedSound;
    if( m_strAttachedSound == "")
      m_strAttachedSound = "<No sound>";
    m_bLooping = asSound.as_bLooping;
    m_bPlaying = asSound.as_bPlaying;
    m_fDelay = asSound.as_fDelay;
    // mark that the values have been updated to reflect the state of the view
    m_udAllValues.MarkUpdated();
  }
  
  //{{AFX_DATA_MAP(CDlgPgInfoAttachingSound)
	DDX_Text(pDX, IDC_ATTACHING_SOUND_T, m_strAttachedSound);
	DDX_Check(pDX, IDC_IS_LOOPING, m_bLooping);
	DDX_Check(pDX, IDC_IS_PLAYING, m_bPlaying);
	DDX_SkyFloat(pDX, IDC_SOUND_START_DELAY, m_fDelay);
	//}}AFX_DATA_MAP

  // if transfering data from dialog to document
  if( pDX->m_bSaveAndValidate)
  {
    if( m_strAttachedSound != "<No sound>")
    {
      asSound.as_fnAttachedSound = CTString( CStringA(m_strAttachedSound));
      asSound.as_bLooping = m_bLooping;
      asSound.as_bPlaying = m_bPlaying;
      asSound.as_fDelay = m_fDelay;
    
      pDoc->SetModifiedFlag();
      pDoc->UpdateAllViews( NULL);
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgPgInfoAttachingSound, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgInfoAttachingSound)
	ON_BN_CLICKED(IDC_BROWSE_SOUND, OnBrowseSound)
	ON_BN_CLICKED(IDC_IS_LOOPING, OnIsLooping)
	ON_BN_CLICKED(IDC_IS_PLAYING, OnIsPlaying)
	ON_BN_CLICKED(IDC_ATTACHING_SOUND_NONE, OnAttachingSoundNone)
	ON_EN_CHANGE(IDC_SOUND_START_DELAY, OnChangeSoundStartDelay)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgInfoAttachingSound message handlers

BOOL CDlgPgInfoAttachingSound::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	return TRUE;
}

BOOL CDlgPgInfoAttachingSound::OnIdle(LONG lCount)
{
  return TRUE;   
}

void CDlgPgInfoAttachingSound::OnBrowseSound() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  CAttachedSound &asSound = pDoc->m_emEditModel.edm_aasAttachedSounds[pModelerView->m_ModelObject.GetAnim()];

  // request sound
  CTFileName fnNewSound = _EngineGUI.FileRequester(
    "Select sound to attach to animation", 
    FILTER_WAV FILTER_END,
    "Sounds directory", "Sounds",
    asSound.as_fnAttachedSound.FileName()+asSound.as_fnAttachedSound.FileExt());
  if( fnNewSound == "") return;

  asSound.as_fnAttachedSound = fnNewSound;
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
  UpdateData( FALSE);
}

void CDlgPgInfoAttachingSound::OnAttachingSoundNone() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView == NULL) return;
  CModelerDoc* pDoc = pModelerView->GetDocument();  
  CAttachedSound &asSound = pDoc->m_emEditModel.edm_aasAttachedSounds[pModelerView->m_ModelObject.GetAnim()];
  asSound.as_fnAttachedSound = CTString("");
  pDoc->SetModifiedFlag();
  pDoc->UpdateAllViews( NULL);
  UpdateData( FALSE);
}

void CDlgPgInfoAttachingSound::OnIsLooping() 
{
  UpdateData( TRUE);
}

void CDlgPgInfoAttachingSound::OnIsPlaying() 
{
  UpdateData( TRUE);
}

void CDlgPgInfoAttachingSound::OnChangeSoundStartDelay() 
{
  UpdateData( TRUE);
}
