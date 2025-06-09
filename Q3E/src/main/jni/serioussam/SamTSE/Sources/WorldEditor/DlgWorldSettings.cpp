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

// DlgWorldSettings.cpp : implementation file
//

#include "stdafx.h"
#include "DlgWorldSettings.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgWorldSettings dialog


CDlgWorldSettings::CDlgWorldSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgWorldSettings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgWorldSettings)
	m_fnBackgroundPicture = _T("");
	m_strMissionDescription = _T("");
	m_fFrontViewCenterX = 0.0f;
	m_fFrontViewCenterY = 0.0f;
	m_fFrontViewHeight = 0.0f;
	m_strFrontViewPicture = _T("");
	m_fFrontViewWidth = 0.0f;
	m_fRightViewCenterX = 0.0f;
	m_fRightViewCenterY = 0.0f;
	m_fRightViewHeight = 0.0f;
	m_strRightViewPicture = _T("");
	m_fRightViewWidth = 0.0f;
	m_fTopViewCenterX = 0.0f;
	m_fTopViewCenterY = 0.0f;
	m_fTopViewHeight = 0.0f;
	m_strTopViewPicture = _T("");
	m_fTopViewWidth = 0.0f;
	m_strBackdropObject = _T("");
	m_strLevelName = _T("");
	//}}AFX_DATA_INIT
}


#define DDX_SPAWN_FLAG_GET(mask, ctrl) \
  if( ulSpawnFlags & (mask)) {\
  ((CButton *)GetDlgItem( ctrl))->SetCheck( TRUE);\
  } else {\
  ((CButton *)GetDlgItem( ctrl))->SetCheck( FALSE);}

#define DDX_SPAWN_FLAG_SET(mask, ctrl) \
  if( ((CButton *)GetDlgItem( ctrl) )->GetCheck() == 1) {\
  ulSpawnFlags |= (mask);\
  } else {\
  ulSpawnFlags &= ~(mask);\
  }


void CDlgWorldSettings::DoDataExchange(CDataExchange* pDX)
{
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    if( m_fnBackgroundPicture == "")
    {
      // disable ok button
      //GetDlgItem( IDOK)->EnableWindow( FALSE);
    }
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    // obtain data for pictures used as view background
    m_strTopViewPicture = pDoc->m_woWorld.wo_strBackdropUp;
    m_fTopViewWidth = pDoc->m_woWorld.wo_fUpW;
	  m_fTopViewHeight = pDoc->m_woWorld.wo_fUpL;
	  m_fTopViewCenterX = pDoc->m_woWorld.wo_fUpCX;
	  m_fTopViewCenterY = pDoc->m_woWorld.wo_fUpCZ;

    m_strFrontViewPicture = pDoc->m_woWorld.wo_strBackdropFt;
    m_fFrontViewWidth = pDoc->m_woWorld.wo_fFtW;
	  m_fFrontViewHeight = pDoc->m_woWorld.wo_fFtH;
	  m_fFrontViewCenterX = pDoc->m_woWorld.wo_fFtCX;
	  m_fFrontViewCenterY = pDoc->m_woWorld.wo_fFtCY;

    m_strRightViewPicture = pDoc->m_woWorld.wo_strBackdropRt;
    m_fRightViewWidth = pDoc->m_woWorld.wo_fRtL;
	  m_fRightViewHeight = pDoc->m_woWorld.wo_fRtH;
	  m_fRightViewCenterX = pDoc->m_woWorld.wo_fRtCZ;
	  m_fRightViewCenterY = pDoc->m_woWorld.wo_fRtCY;

    m_strBackdropObject = pDoc->m_woWorld.wo_strBackdropObject;

    m_strLevelName = pDoc->m_woWorld.GetName();

    // get spawn flags
    ULONG ulSpawnFlags = pDoc->m_woWorld.GetSpawnFlags();
    DDX_SPAWN_FLAG_GET( SPF_EASY, IDC_EASY);
    DDX_SPAWN_FLAG_GET( SPF_NORMAL, IDC_NORMAL);
    DDX_SPAWN_FLAG_GET( SPF_HARD, IDC_HARD);
    DDX_SPAWN_FLAG_GET( SPF_EXTREME, IDC_EXTREME);
    DDX_SPAWN_FLAG_GET( SPF_EXTREME<<1, IDC_DIFFICULTY_1B);
    DDX_SPAWN_FLAG_GET( SPF_EXTREME<<2, IDC_DIFFICULTY_2B);
    DDX_SPAWN_FLAG_GET( SPF_EXTREME<<3, IDC_DIFFICULTY_3B);
    DDX_SPAWN_FLAG_GET( SPF_EXTREME<<4, IDC_DIFFICULTY_4B);
    DDX_SPAWN_FLAG_GET( SPF_EXTREME<<5, IDC_DIFFICULTY_5B);
    DDX_SPAWN_FLAG_GET( SPF_SINGLEPLAYER, IDC_SINGLE);
    DDX_SPAWN_FLAG_GET( SPF_DEATHMATCH, IDC_DEATHMATCH);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE, IDC_COOPERATIVE);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE<<1, IDC_GAME_MODE_1B);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE<<2, IDC_GAME_MODE_2B);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE<<3, IDC_GAME_MODE_3B);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE<<4, IDC_GAME_MODE_4B);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE<<5, IDC_GAME_MODE_5B);
    DDX_SPAWN_FLAG_GET( SPF_COOPERATIVE<<6, IDC_GAME_MODE_6B);
  }

  CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgWorldSettings)
	DDX_Control(pDX, IDC_BACKGROUND_COLOR, m_BackgroundColor);
	DDX_Text(pDX, IDC_PICTURE_FILE_T, m_fnBackgroundPicture);
	DDX_Text(pDX, IDC_MISSION_DESCRIPTION, m_strMissionDescription);
	DDX_Text(pDX, IDC_FRONT_VIEW_CENTER_X, m_fFrontViewCenterX);
	DDX_Text(pDX, IDC_FRONT_VIEW_CENTER_Y, m_fFrontViewCenterY);
	DDX_Text(pDX, IDC_FRONT_VIEW_HEIGHT, m_fFrontViewHeight);
	DDX_Text(pDX, IDC_FRONT_VIEW_PICTURE_T, m_strFrontViewPicture);
	DDX_Text(pDX, IDC_FRONT_VIEW_WIDTH, m_fFrontViewWidth);
	DDX_Text(pDX, IDC_RIGHT_VIEW_CENTER_X, m_fRightViewCenterX);
	DDX_Text(pDX, IDC_RIGHT_VIEW_CENTER_Y, m_fRightViewCenterY);
	DDX_Text(pDX, IDC_RIGHT_VIEW_HEIGHT, m_fRightViewHeight);
	DDX_Text(pDX, IDC_RIGHT_VIEW_PICTURE_T, m_strRightViewPicture);
	DDX_Text(pDX, IDC_RIGHT_VIEW_WIDTH, m_fRightViewWidth);
	DDX_Text(pDX, IDC_TOP_VIEW_CENTER_X, m_fTopViewCenterX);
	DDX_Text(pDX, IDC_TOP_VIEW_CENTER_Y, m_fTopViewCenterY);
	DDX_Text(pDX, IDC_TOP_VIEW_HEIGHT, m_fTopViewHeight);
	DDX_Text(pDX, IDC_TOP_VIEW_PICTURE_T, m_strTopViewPicture);
	DDX_Text(pDX, IDC_TOP_VIEW_WIDTH, m_fTopViewWidth);
	DDX_Text(pDX, IDC_BACKDROP_OBJECT_T, m_strBackdropObject);
	DDX_Text(pDX, IDC_EDIT_LEVEL_NAME, m_strLevelName);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    pDoc->m_woWorld.wo_strBackdropUp = CStringA(m_strTopViewPicture);
    pDoc->m_woWorld.wo_fUpW = m_fTopViewWidth;
	  pDoc->m_woWorld.wo_fUpL = m_fTopViewHeight;
	  pDoc->m_woWorld.wo_fUpCX = m_fTopViewCenterX;
	  pDoc->m_woWorld.wo_fUpCZ = m_fTopViewCenterY;

    pDoc->m_woWorld.wo_strBackdropFt = CStringA(m_strFrontViewPicture);
    pDoc->m_woWorld.wo_fFtW = m_fFrontViewWidth;
	  pDoc->m_woWorld.wo_fFtH = m_fFrontViewHeight;
	  pDoc->m_woWorld.wo_fFtCX = m_fFrontViewCenterX;
	  pDoc->m_woWorld.wo_fFtCY = m_fFrontViewCenterY;

    pDoc->m_woWorld.wo_strBackdropRt = CStringA(m_strRightViewPicture);
    pDoc->m_woWorld.wo_fRtL = m_fRightViewWidth;
	  pDoc->m_woWorld.wo_fRtH = m_fRightViewHeight;
	  pDoc->m_woWorld.wo_fRtCZ = m_fRightViewCenterX;
	  pDoc->m_woWorld.wo_fRtCY = m_fRightViewCenterY;

    pDoc->m_woWorld.wo_strBackdropObject = CStringA(m_strBackdropObject);
    // try to load object for backdrops
    if( pDoc->m_woWorld.wo_strBackdropObject != "")
    {
      // try to
      try
      {
        pDoc->m_o3dBackdropObject.Clear();
        // load 3D object
        FLOATmatrix3D mStretch;
        mStretch.Diagonal(1.0f);
        pDoc->m_o3dBackdropObject.LoadAny3DFormat_t( pDoc->m_woWorld.wo_strBackdropObject, mStretch);
      }
      // catch and
      catch (const char *strError)
      {
        // report errors
        AfxMessageBox( CString(strError));
        return;
      }
    }

    if( m_fnBackgroundPicture != "")
    {
      theApp.WriteProfileString( L"World editor prefs", L"Default background picture",
        m_fnBackgroundPicture);
    }

    char chrColor[ 16];
    sprintf( chrColor, "0x%08x", m_BackgroundColor.GetColor());
    _strupr( chrColor);
    theApp.WriteProfileString( L"World editor prefs", L"Default background color", CString(chrColor));

    pDoc->m_woWorld.SetName( CTString(CStringA(m_strLevelName)));

    // apply new spawn flags
    ULONG ulSpawnFlags = 0;
    DDX_SPAWN_FLAG_SET( SPF_EASY, IDC_EASY);
    DDX_SPAWN_FLAG_SET( SPF_NORMAL, IDC_NORMAL);
    DDX_SPAWN_FLAG_SET( SPF_HARD, IDC_HARD);
    DDX_SPAWN_FLAG_SET( SPF_EXTREME, IDC_EXTREME);
    DDX_SPAWN_FLAG_SET( SPF_EXTREME<<1, IDC_DIFFICULTY_1B);
    DDX_SPAWN_FLAG_SET( SPF_EXTREME<<2, IDC_DIFFICULTY_2B);
    DDX_SPAWN_FLAG_SET( SPF_EXTREME<<3, IDC_DIFFICULTY_3B);
    DDX_SPAWN_FLAG_SET( SPF_EXTREME<<4, IDC_DIFFICULTY_4B);
    DDX_SPAWN_FLAG_SET( SPF_EXTREME<<5, IDC_DIFFICULTY_5B);
    DDX_SPAWN_FLAG_SET( SPF_SINGLEPLAYER, IDC_SINGLE);
    DDX_SPAWN_FLAG_SET( SPF_DEATHMATCH, IDC_DEATHMATCH);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE, IDC_COOPERATIVE);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE<<1, IDC_GAME_MODE_1B);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE<<2, IDC_GAME_MODE_2B);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE<<3, IDC_GAME_MODE_3B);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE<<4, IDC_GAME_MODE_4B);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE<<5, IDC_GAME_MODE_5B);
    DDX_SPAWN_FLAG_SET( SPF_COOPERATIVE<<6, IDC_GAME_MODE_6B);
    pDoc->m_woWorld.SetSpawnFlags(ulSpawnFlags);
  }
}


BEGIN_MESSAGE_MAP(CDlgWorldSettings, CDialog)
	//{{AFX_MSG_MAP(CDlgWorldSettings)
	ON_BN_CLICKED(IDC_BROWSE_BACKGROUND_PICTURE, OnBrowseBackgroundPicture)
	ON_BN_CLICKED(IDC_BROWSE_FRONT_VIEW_PICTURE, OnBrowseFrontViewPicture)
	ON_BN_CLICKED(IDC_BROWSE_RIGHT_VIEW_PICTURE, OnBrowseRightViewPicture)
	ON_BN_CLICKED(IDC_BROWSE_TOP_VIEW_PICTURE, OnBrowseTopViewPicture)
	ON_BN_CLICKED(IDC_BROWSE_BACKDROP_OBJECT, OnBrowseBackdropObject)
	ON_BN_CLICKED(ID_APPLY, OnApply)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgWorldSettings message handlers

void CDlgWorldSettings::OnBrowseBackgroundPicture()
{
  CTFileName fnChoosedFile = _EngineGUI.FileRequester( "Select background texture",
    FILTER_TEX FILTER_ALL FILTER_END, KEY_NAME_BACKGROUND_TEXTURE_DIR, "Textures\\Background");
  if( fnChoosedFile == "") return;

  // substract last two letters of background's file name
  char achrShortenedBcgName[ PATH_MAX];
  strcpy( achrShortenedBcgName, fnChoosedFile.FileDir()+fnChoosedFile.FileName());
  // there must be at least two letters in selected texture name
  if( strlen( achrShortenedBcgName) > 2)
  {
    // shorten file name for two letters
    achrShortenedBcgName[ strlen( achrShortenedBcgName)-2] = 0;
    // assign new background texture name
    m_fnBackgroundPicture = CTString(achrShortenedBcgName)+fnChoosedFile.FileExt();
  }
  // enable ok button
  //GetDlgItem( IDOK)->EnableWindow( TRUE);
  UpdateData( FALSE);
}

// setups background settings dialog
void CDlgWorldSettings::SetupBcgSettings( BOOL bOnNewDocument)
{
  char chrColor[ 16];
  COLOR colBackground;

  // if change bcg settings dialog was called on new document
  if( bOnNewDocument)
  {
    // obtain background color form INI file
    strcpy( chrColor, CStringA(theApp.GetProfileString( L"World editor prefs",
                                               L"Default background color", L"0XFF000000")));
    sscanf( chrColor, "0X%08x", &colBackground);
    // set background color to color button
    m_BackgroundColor.SetColor( colBackground);
    // set default texture for background
    m_fnBackgroundPicture = "Textures\\Editor\\Default.tex";
    // set default mission description
    m_strMissionDescription = "No mission description";
  }
  else
  {
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    // obtain picture used for background from world
    m_fnBackgroundPicture = CTString(); //!!!!pDoc->m_woWorld.GetBackgroundTexture();
    // set world's background color to color button
    m_BackgroundColor.SetColor( pDoc->m_woWorld.GetBackgroundColor());
    // pick-up mission description from world
    m_strMissionDescription = pDoc->m_woWorld.GetDescription();
  }
}

BOOL CDlgWorldSettings::OnInitDialog()
{
	CDialog::OnInitDialog();

  // call MFC color picker (windows)
  m_BackgroundColor.SetPickerType( CColoredButton::PT_MFC);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgWorldSettings::OnBrowseTopViewPicture()
{
  CTFileName fnPicture = _EngineGUI.FileRequester( "Picture for top view",
    FILTER_PICTURES FILTER_ALL FILTER_END, "Picture for view directory", "");
  if( fnPicture == "") return;
  GetDlgItem( IDC_TOP_VIEW_PICTURE_T)->SetWindowText( CString(fnPicture));
	m_strTopViewPicture = fnPicture;
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  pDoc->SetupBackdropTextureObject( CTString(CStringA(m_strTopViewPicture)), pDoc->m_toBackdropUp);
}

void CDlgWorldSettings::OnBrowseFrontViewPicture()
{
  CTFileName fnPicture = _EngineGUI.FileRequester( "Picture for front view",
    FILTER_PICTURES FILTER_ALL FILTER_END, "Picture for view directory", "");
  if( fnPicture == "") return;
  GetDlgItem( IDC_FRONT_VIEW_PICTURE_T)->SetWindowText( CString(fnPicture));
	m_strFrontViewPicture = fnPicture;
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  pDoc->SetupBackdropTextureObject( CTString(CStringA(m_strFrontViewPicture)), pDoc->m_toBackdropFt);
}

void CDlgWorldSettings::OnBrowseRightViewPicture()
{
  CTFileName fnPicture = _EngineGUI.FileRequester( "Picture for right view",
    FILTER_PICTURES FILTER_ALL FILTER_END, "Picture for view directory", "");
  if( fnPicture == "") return;
  GetDlgItem( IDC_RIGHT_VIEW_PICTURE_T)->SetWindowText( CString(fnPicture));
	m_strRightViewPicture = fnPicture;
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  pDoc->SetupBackdropTextureObject( CTString(CStringA(m_strRightViewPicture)), pDoc->m_toBackdropRt);
}


void CDlgWorldSettings::OnBrowseBackdropObject()
{
  CTFileName fnObject = _EngineGUI.FileRequester( "Select background object",
    FILTER_3DOBJ FILTER_LWO FILTER_OBJ FILTER_3DS FILTER_ALL FILTER_END, "Backdrop object directory", "");
  if( fnObject == "") return;
  GetDlgItem( IDC_BACKDROP_OBJECT_T)->SetWindowText( CString(fnObject));
	m_strBackdropObject = fnObject;
}

void CDlgWorldSettings::OnOK()
{
}

void CDlgWorldSettings::OnApply()
{
  CDialog::OnOK();
}
