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

// DlgLightAnimationEditor.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgLightAnimationEditor.h"
#include <Engine/Templates/Stock_CAnimData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgLightAnimationEditor dialog

#define DEFAULT_ANIMATION_FILE "Temp\\DefaultAnimation.ani"

INDEX CDlgLightAnimationEditor::GetSelectedLightAnimation(void)
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_LightAnimationCombo.GetCurSel();
  if( iLightAnimation == CB_ERR)
  {
    return 0;
  }
  return iLightAnimation;
}

CDlgLightAnimationEditor::CDlgLightAnimationEditor(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgLightAnimationEditor::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgLightAnimationEditor)
	m_fLightAnimationSpeed = 0.0f;
	m_iAnimationFrames = 0;
	m_strLightAnimationName = _T("");
	//}}AFX_DATA_INIT


  m_bCustomWindowsCreated = FALSE;
  m_fnSaveName = CTString("");

  // save default animation into temporary directory
  CAnimData adDefault;
  adDefault.DefaultAnimation();
  CTFileName fnDefaultAnimation = CTString( DEFAULT_ANIMATION_FILE);
  try {
    adDefault.Save_t( fnDefaultAnimation);
  } catch (const char *pError) {
    FatalError( "Unable to save default animation: \"%s\", %s", (CTString&)fnDefaultAnimation, pError);
  }

  // try to load animation that was last edited
  try
  {
    CTFileName fnLastEditted = CTString( CStringA(theApp.GetProfileString(L"World editor", L"Last edited light animation", CString(DEFAULT_ANIMATION_FILE))));
    m_padAnimData = _pAnimStock->Obtain_t( fnLastEditted);
    m_fnSaveName = fnLastEditted;
  }
  catch (const char *pError)
  {
    (void) pError;
    // try to load default animation
    try
    {
      m_padAnimData = _pAnimStock->Obtain_t( fnDefaultAnimation);
    }
    catch (const char *pError2)
    {
      FatalError( "Unable to save and obtain default animation: \"%s\", %s", (CTString&)fnDefaultAnimation, pError2);
    }
  }

  // set animation data
  m_wndTestAnimation.m_aoAnimObject.SetData( m_padAnimData);
  m_bChanged = FALSE;
}

CDlgLightAnimationEditor::~CDlgLightAnimationEditor()
{
  theApp.WriteProfileString(L"World editor", L"Last edited light animation", CString(m_padAnimData->GetName()));
  m_wndTestAnimation.m_aoAnimObject.SetData( NULL);
  _pAnimStock->Release( m_padAnimData);
}

void CDlgLightAnimationEditor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // if dialog is receiving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    InitializeData();
  }

	//{{AFX_DATA_MAP(CDlgLightAnimationEditor)
	DDX_Control(pDX, IDC_LIGHT_ANIMATION_NAME_COMBO, m_LightAnimationCombo);
	DDX_Text(pDX, IDC_CURRENT_FRAME, m_strCurrentFrame);
	DDX_SkyFloat(pDX, IDC_LIGHT_ANIMATION_SPEED, m_fLightAnimationSpeed);
	DDX_Text(pDX, IDC_LIGHT_ANIMATION_FRAMES, m_iAnimationFrames);
	DDV_MinMaxInt(pDX, m_iAnimationFrames, 1, 999);
	DDX_Text(pDX, IDC_LIGHT_ANIMATION_NAME, m_strLightAnimationName);
	DDV_MaxChars(pDX, m_strLightAnimationName, 30);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE )
  {
    StoreData();
  }
}


BEGIN_MESSAGE_MAP(CDlgLightAnimationEditor, CDialog)
	//{{AFX_MSG_MAP(CDlgLightAnimationEditor)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_DELETE_MARKER, OnDeleteMarker)
	ON_EN_CHANGE(IDC_LIGHT_ANIMATION_FRAMES, OnChangeLightAnimationFrames)
	ON_CBN_SELCHANGE(IDC_LIGHT_ANIMATION_NAME_COMBO, OnSelchangeLightAnimationNameCombo)
	ON_EN_CHANGE(IDC_LIGHT_ANIMATION_SPEED, OnChangeLightAnimationSpeed)
	ON_BN_CLICKED(IDC_SCROLL_LEFT, OnScrollLeft)
	ON_BN_CLICKED(IDC_SCROLL_RIGHT, OnScrollRight)
	ON_BN_CLICKED(IDC_SCROLL_PG_LEFT, OnScrollPgLeft)
	ON_BN_CLICKED(IDC_SCROLL_PG_RIGHT, OnScrollPgRight)
	ON_BN_CLICKED(ID_DELETE_ANIMATION, OnDeleteAnimation)
	ON_BN_CLICKED(ID_ADD_ANIMATION, OnAddAnimation)
	ON_EN_CHANGE(IDC_LIGHT_ANIMATION_NAME, OnChangeLightAnimationName)
	ON_BN_CLICKED(ID_LOAD_ANIMATION, OnLoadAnimation)
	ON_BN_CLICKED(ID_SAVE_ANIMATION, OnSaveAnimation)
	ON_BN_CLICKED(ID_SAVE_AS_ANIMATION, OnSaveAsAnimation)
	ON_BN_CLICKED(ID_CLOSE, OnButtonClose)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgLightAnimationEditor message handlers

void CDlgLightAnimationEditor::InitializeData(void)
{
  if( !IsWindow( m_LightAnimationCombo.m_hWnd)) return;
  // set dialog window title
  CTString strTitle = m_padAnimData->GetName();
  if( (strTitle == "") || (strTitle ==DEFAULT_ANIMATION_FILE) )
  {
    strTitle = "<unnamed>";
  }
  SetWindowText( CString(CTString("Editing animation: ") + strTitle));

  // get currently selected light animation combo member
  INDEX iLightAnimation = m_LightAnimationCombo.GetCurSel();
  if( iLightAnimation != CB_ERR)
  {
    m_wndTestAnimation.m_aoAnimObject.StartAnim( iLightAnimation);
    // obtain information about animation
    CAnimInfo aiInfo;
    m_padAnimData->GetAnimInfo(iLightAnimation, aiInfo);
    // set info into controls
    m_fLightAnimationSpeed = aiInfo.ai_SecsPerFrame;
    m_iAnimationFrames = aiInfo.ai_NumberOfFrames;
    m_strLightAnimationName = aiInfo.ai_AnimName;

    // set current frame text
    char achrFrame[ 64];
    sprintf( achrFrame, "Frame: %d", m_wndAnimationFrames.m_iSelectedFrame);
    m_strCurrentFrame = achrFrame;
    // redraw frames
    if( IsWindow(m_wndAnimationFrames.m_hWnd))
    {
      // enable/disable delete key-frame button
      GetDlgItem(IDC_DELETE_MARKER)->EnableWindow(
        m_wndAnimationFrames.IsSelectedFrameKeyFrame() );
      m_wndAnimationFrames.Invalidate( FALSE);
    }
  }
}

void CDlgLightAnimationEditor::StoreData(void)
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_LightAnimationCombo.GetCurSel();
  if( iLightAnimation == CB_ERR)
  {
    return;
  }

  // and set new name to anim data
  m_padAnimData->SetName( iLightAnimation, CTString(CStringA(m_strLightAnimationName)));
  //------------ Prepare new array of frames for current animation (key frames changing is
  // not applied here but in control LMB down handler)
  CAnimData *pAD = m_padAnimData;
  // obtain information about animation
  CAnimInfo aiInfo;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  // get count of old frames
  INDEX ctOldFrames = aiInfo.ai_NumberOfFrames;
  // create array for new frames
  INDEX *piNewFrames = new INDEX[ m_iAnimationFrames];
  // now copy old array over new one but if new one is longer than old one, added
  // frames will be filled with last value (RGBA format)
  for( INDEX iFrame=0; iFrame<m_iAnimationFrames; iFrame++)
  {
    // if we are copying old frames
    if( iFrame<ctOldFrames)
    {
      piNewFrames[iFrame]=pAD->GetFrame(iLightAnimation, iFrame);
      // if we are adding last frame, delete its key-frame marker
      if( (m_iAnimationFrames>ctOldFrames) && (iFrame==(ctOldFrames-1)) && (iFrame != 0) )
      {
        piNewFrames[iFrame] &= 0xFFFFFF00;
      }
    }
    // if we are adding new frames
    else
    {
      // obtain last frame in old animation
      INDEX iLastFrame = pAD->GetFrame(iLightAnimation, ctOldFrames-1);
      // clear alpha chanell (we don't want added frames to be key frames)
      iLastFrame &= 0xFFFFFF00;
      // clone last frame from ole animation as added frames in new animation
      piNewFrames[iFrame]=iLastFrame;
    }
  }
  // set alpha to 0xFF to mark key frame for last frame (first and last frames must be keys)
  piNewFrames[m_iAnimationFrames-1] |= 0x000000FF;
  // set new speed
  pAD->SetSpeed( iLightAnimation, m_fLightAnimationSpeed);
  // set new frames
  pAD->SetFrames( iLightAnimation, m_iAnimationFrames, piNewFrames);
  // spread frames
  SpreadFrames();
  // delete allocated array
  delete piNewFrames;
  m_bChanged = TRUE;
}

void CDlgLightAnimationEditor::SpreadFrames(void)
{
  // get curently selected light animation combo member
  INDEX iLightAnimation = m_LightAnimationCombo.GetCurSel();
  if( iLightAnimation == CB_ERR)
  {
    return;
  }

  // obtain information about animation
  CAnimInfo aiInfo;
  CAnimData *pAD = m_padAnimData;
  pAD->GetAnimInfo(iLightAnimation, aiInfo);
  // get count of frames
  INDEX ctFrames = aiInfo.ai_NumberOfFrames;
  // create array to be copy of current frames array
  INDEX *piFrames = new INDEX[ ctFrames];
  // now copy old array over new one
  for( INDEX iFrame=0; iFrame<m_iAnimationFrames; iFrame++)
  {
    piFrames[iFrame]=pAD->GetFrame(iLightAnimation, iFrame);
  }
  // we will start spreading from first frame
  INDEX iStart=0;
  // now spread frames beetween key-frames
  do
  {
    // find first next key frame
    INDEX iKeySearcher=iStart+1;
    for( ; iKeySearcher<ctFrames; iKeySearcher++)
    {
      // is this gradient key frame?
      if( (piFrames[iKeySearcher] & 0x000000FF) != 0)
      {
        // yes it is, stop searching
        break;
      }
    }
    // get starting R, G, B values
    FLOAT fStartR = (FLOAT) ((piFrames[iStart]>>24) & 0x000000FF);
    FLOAT fStartG = (FLOAT) ((piFrames[iStart]>>16) & 0x000000FF);
    FLOAT fStartB = (FLOAT) ((piFrames[iStart]>>8 ) & 0x000000FF);

    // calculate R, G and B deltas
    FLOAT fdR = ( (FLOAT)((piFrames[iKeySearcher]>>24) & 0x000000FF)-
                  ((piFrames[iStart]>>24)       & 0x000000FF) )/(iKeySearcher-iStart);
    FLOAT fdG = ( (FLOAT)((piFrames[iKeySearcher]>>16) & 0x000000FF)-
                  ((piFrames[iStart]>>16)       & 0x000000FF) )/(iKeySearcher-iStart);
    FLOAT fdB = ( (FLOAT)((piFrames[iKeySearcher]>>8)  & 0x000000FF)-
                  ((piFrames[iStart]>>8)        & 0x000000FF) )/(iKeySearcher-iStart);
    INDEX iDeltaTimes=1;
    // create gradients beetween  iStart and iKeySearcher
    for( INDEX iGradient=iStart+1; iGradient<iKeySearcher; iGradient++)
    {
      FLOAT fCurrentR = fStartR+fdR*iDeltaTimes;
      FLOAT fCurrentG = fStartG+fdG*iDeltaTimes;
      FLOAT fCurrentB = fStartB+fdB*iDeltaTimes;
      COLOR colNR = (COLOR) fCurrentR;
      COLOR colNG = (COLOR) fCurrentG;
      COLOR colNB = (COLOR) fCurrentB;
      piFrames[iGradient]= (colNR<<24) | (colNG<<16) | (colNB<<8);
      // next delta
      iDeltaTimes ++;
    }
    // next gradient will start at last key frame
    iStart = iKeySearcher;
  }
  while( iStart < ctFrames);
  // set new frames
  pAD->SetFrames( iLightAnimation, ctFrames, piFrames);
  // delete allocated array
  delete piFrames;
  // redraw frames
  if( IsWindow(m_wndAnimationFrames.m_hWnd))
  {
    m_wndAnimationFrames.Invalidate( FALSE);
  }
}
void CDlgLightAnimationEditor::OnPaint()
{
  {
  CPaintDC dc(this); // device context for painting
  }

  if( !m_bCustomWindowsCreated)
  {
    // ---------------- Create custom window that will hold graphical representation of frames
    // obtain frames area window
    CWnd *pWndFramesArea = GetDlgItem(IDC_FRAMES_AREA);
    ASSERT(pWndFramesArea != NULL);
    // obtain frames area frame control's rectangle
    CRect rectFramesArea;
    pWndFramesArea->GetWindowRect(&rectFramesArea);
    ScreenToClient(&rectFramesArea);
    // create window for area to contain frames
    m_wndAnimationFrames.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rectFramesArea,
                                 this, IDW_ANIMATION_FRAMES);
    m_wndAnimationFrames.SetParentDlg( this);
    // ---------------- Create custom window that show how light animation looks like
    CWnd *pWndTestArea = GetDlgItem(IDC_TEST_AREA);
    ASSERT(pWndTestArea != NULL);
    CRect rectTestArea;
    pWndTestArea->GetWindowRect(&rectTestArea);
    ScreenToClient(&rectTestArea);
    // create window for for animation testing
    m_wndTestAnimation.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rectTestArea,
                               this, IDW_ANIMATION_FRAMES);
    m_wndTestAnimation.SetParentDlg( this);
    // mark that custom windows are created
    m_bCustomWindowsCreated = TRUE;
  }
}

BOOL CDlgLightAnimationEditor::OnInitDialog()
{
	CDialog::OnInitDialog();

  // initialize light animation combo
  InitLightAnimationCombo();
  m_LightAnimationCombo.SetCurSel(0);

  // initialize
  UpdateData( FALSE);
	return TRUE;
}

void CDlgLightAnimationEditor::OnDeleteMarker()
{
  // delete selected frame
  m_wndAnimationFrames.DeleteSelectedFrame();
}

void CDlgLightAnimationEditor::OnChangeLightAnimationFrames()
{
  // store data state from dialog into animation data
  if( IsWindow(m_LightAnimationCombo.m_hWnd))
  {
    // reset starting and selected frames
    m_wndAnimationFrames.m_iStartingFrame = 0;
    m_wndAnimationFrames.m_iSelectedFrame = 0;
    UpdateData( TRUE);
  }
}

void CDlgLightAnimationEditor::InitLightAnimationCombo(void)
{
  // clear combo box
  m_LightAnimationCombo.ResetContent();
  // limit animation lenght
  m_LightAnimationCombo.LimitText(NAME_SIZE);
  // initialize light animations combo box
  for(INDEX iAnimation=0;iAnimation<m_padAnimData->GetAnimsCt(); iAnimation++)
  {
    // obtain information about animation
    CAnimInfo aiInfo;
    m_padAnimData->GetAnimInfo(iAnimation, aiInfo);
    // add animation name
    m_LightAnimationCombo.AddString( CString(aiInfo.ai_AnimName));
  }
}

void CDlgLightAnimationEditor::OnScrollLeft()
{
	m_wndAnimationFrames.ScrollLeft();
}

void CDlgLightAnimationEditor::OnScrollRight()
{
	m_wndAnimationFrames.ScrollRight();
}

void CDlgLightAnimationEditor::OnScrollPgLeft()
{
	m_wndAnimationFrames.ScrollPgLeft();
}

void CDlgLightAnimationEditor::OnScrollPgRight()
{
	m_wndAnimationFrames.ScrollPgRight();
}

void CDlgLightAnimationEditor::OnSelchangeLightAnimationNameCombo()
{
  // reset starting and selected frames
  m_wndAnimationFrames.m_iStartingFrame = 0;
  m_wndAnimationFrames.m_iSelectedFrame = 0;
  UpdateData( FALSE);
}

void CDlgLightAnimationEditor::OnChangeLightAnimationSpeed()
{
  // store data state from dialog into animation data
  if( IsWindow(m_LightAnimationCombo.m_hWnd))
  {
    UpdateData( TRUE);
  }
}

void CDlgLightAnimationEditor::OnDeleteAnimation()
{
  // get newly selected animation
  INDEX iAnimation = m_LightAnimationCombo.GetCurSel();
  m_padAnimData->DeleteAnimation( iAnimation);
  InitLightAnimationCombo();
  m_LightAnimationCombo.SetCurSel( 0);
  UpdateData( FALSE);
}

void CDlgLightAnimationEditor::OnAddAnimation()
{
  m_padAnimData->AddAnimation();
  // select newly added animation
  InitLightAnimationCombo();
  m_LightAnimationCombo.SetCurSel(m_padAnimData->GetAnimsCt()-1);
  // reset starting and selected frames
  m_wndAnimationFrames.m_iStartingFrame = 0;
  m_wndAnimationFrames.m_iSelectedFrame = 0;
  UpdateData( FALSE);
}

void CDlgLightAnimationEditor::OnChangeLightAnimationName()
{
  UpdateData( TRUE);

  INDEX iAnimation = m_LightAnimationCombo.GetCurSel();
  InitLightAnimationCombo();
  m_LightAnimationCombo.SetCurSel( iAnimation);
}

void CDlgLightAnimationEditor::OnLoadAnimation()
{
  CTFileName fnAnimation = _EngineGUI.FileRequester(
    "Load animation file",
    "Animation file (*.ani)\0*.ani\0" FILTER_ALL FILTER_END,
    "Animation directory", "");
  if( fnAnimation == "") return;

  try
  {
    _pAnimStock->Release( m_padAnimData);
    m_padAnimData = _pAnimStock->Obtain_t( fnAnimation);
    m_fnSaveName = fnAnimation;
  }
  catch (const char *strError)
  {
    WarningMessage( strError);
    try
    {
      CTFileName fnDefaultAnimation = CTString( DEFAULT_ANIMATION_FILE);
      m_padAnimData = _pAnimStock->Obtain_t( fnDefaultAnimation);
    }
    catch (const char *strError2)
    {
      FatalError( strError2);
    }
  }
  // initialize light animation combo
  InitLightAnimationCombo();
  // set animation data
  m_wndTestAnimation.m_aoAnimObject.SetData( m_padAnimData);
  m_LightAnimationCombo.SetCurSel(0);
  UpdateData( FALSE);
}

void CDlgLightAnimationEditor::OnSaveAnimation()
{
  if( m_fnSaveName == "")
  {
    OnSaveAsAnimation();
  }
  else
  {
    try
    {
      CTFileStream strmFile;
      strmFile.Create_t( m_fnSaveName);
      // write animation from file
      m_padAnimData->Write_t( &strmFile);
      strmFile.Close();
      // refresh animation
      CAnimData *pad = _pAnimStock->Obtain_t(m_fnSaveName);
      pad->Reload();
      _pAnimStock->Release(pad);
    }
    catch (const char *strError)
    {
      WarningMessage( strError);
    }
  }
}

void CDlgLightAnimationEditor::OnSaveAsAnimation()
{
  CTFileName fnAnimation = _EngineGUI.FileRequester(
    "Save animation file",
    "Animation file (*.ani)\0*.ani\0" FILTER_ALL FILTER_END,
    "Animation directory", "", "", NULL, FALSE);
  if( fnAnimation == "") return;

  try
  {
    CTFileStream strmFile;
    strmFile.Create_t( fnAnimation);
    // write animation into file
    m_padAnimData->Write_t( &strmFile);
    m_fnSaveName = fnAnimation;
    strmFile.Close();
    // refresh animation
    CAnimData *pad = _pAnimStock->Obtain_t(m_fnSaveName);
    pad->Reload();
    _pAnimStock->Release(pad);
  }
  catch (const char *strError)
  {
    WarningMessage( strError);
  }
}

void CDlgLightAnimationEditor::OnButtonClose()
{
  if( m_bChanged) OnSaveAnimation();
  EndDialog( 0);
}

void CDlgLightAnimationEditor::OnCancel()
{
  if( m_bChanged) OnSaveAnimation();
  EndDialog( 0);
}

void CDlgLightAnimationEditor::OnClose()
{
  if( m_bChanged) OnSaveAnimation();
	CDialog::OnClose();
}

void CDlgLightAnimationEditor::OnOK()
{
}
