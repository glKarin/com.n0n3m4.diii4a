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

// DlgPreferences.cpp : implementation file
//

#include "stdafx.h"
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPreferences dialog
#define PREF_COMBO_HEIGHT 100
extern UINT APIENTRY ModelerFileRequesterHook( HWND hdlg, UINT uiMsg, WPARAM wParam,	LPARAM lParam);

CDlgPreferences::CDlgPreferences(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPreferences::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgPreferences)
	m_AllwaysLamp = FALSE;
	m_PrefsCopy = FALSE;
	m_AutoMaximize = FALSE;
	m_SetDefaultColors = FALSE;
	m_WindowFit = FALSE;
	m_bIsFloorVisibleByDefault = FALSE;
	m_fDefaultBanking = 0.0f;
	m_fDefaultHeading = 0.0f;
	m_fDefaultPitch = 0.0f;
	m_fDefaultFOW = 0.0f;
	m_bIsBcgVisibleByDefault = FALSE;
	m_bAllowSoundLock = FALSE;
	//}}AFX_DATA_INIT
  m_Prefs = theApp.m_Preferences;
}


void CDlgPreferences::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE )
  {
    m_MappingPaper.SetColor( m_Prefs.ap_MappingPaperColor);
    m_MappingActiveInk.SetColor( m_Prefs.ap_MappingActiveSurfaceColor);
    m_MappingInactiveInk.SetColor( m_Prefs.ap_MappingInactiveSurfaceColor);
    m_ModelPaper.SetColor( m_Prefs.ap_DefaultPaperColor);
    m_ModelInk.SetColor( m_Prefs.ap_DefaultInkColor);
    m_MappingWinBcgColor.SetColor( m_Prefs.ap_MappingWinBcgColor);

    m_PrefsCopy = m_Prefs.ap_CopyExistingWindowPrefs;
    m_bIsFloorVisibleByDefault = m_Prefs.ap_bIsFloorVisibleByDefault;
    m_bIsBcgVisibleByDefault = m_Prefs.ap_bIsBcgVisibleByDefault;
    m_AutoMaximize = m_Prefs.ap_AutoMaximizeWindow;
    m_AllwaysLamp = m_Prefs.ap_AllwaysSeeLamp;
    m_WindowFit = m_Prefs.ap_AutoWindowFit;
    m_SetDefaultColors = m_Prefs.ap_SetDefaultColors;
	  m_colorDefaultAmbientColor.SetColor( m_Prefs.ap_colDefaultAmbientColor);
	  m_fDefaultBanking = m_Prefs.ap_fDefaultBanking;
	  m_fDefaultHeading = m_Prefs.ap_fDefaultHeading;
	  m_fDefaultPitch = m_Prefs.ap_fDefaultPitch;
	  m_fDefaultFOW = m_Prefs.ap_fDefaultFOW;
    m_bAllowSoundLock = m_Prefs.ap_bAllowSoundLock;
  }
	//{{AFX_DATA_MAP(CDlgPreferences)
	DDX_Control(pDX, IDC_API, m_ctrlGfxApi);
	DDX_Control(pDX, IDC_DEFAULT_AMBIENT_COLOR, m_colorDefaultAmbientColor);
	DDX_Control(pDX, IDC_WIN_BCG_COLOR, m_MappingWinBcgColor);
	DDX_Control(pDX, IDOK, m_OkButton);
	DDX_Control(pDX, IDC_COMBO_WIN_BCG_TEXTURE, m_ComboWinBcgTexture);
	DDX_Control(pDX, IDC_MAPPING_PAPER, m_MappingPaper);
	DDX_Control(pDX, IDC_MODEL_PAPER, m_ModelPaper);
	DDX_Control(pDX, IDC_MODEL_INK, m_ModelInk);
	DDX_Control(pDX, IDC_MAPPING_INACTIVE_INK, m_MappingInactiveInk);
	DDX_Control(pDX, IDC_MAPPING_ACTIVE_INK, m_MappingActiveInk);
	DDX_Check(pDX, IDC_PREFS_ALLWAYS_LAMP, m_AllwaysLamp);
	DDX_Check(pDX, IDC_PREFS_COPY, m_PrefsCopy);
	DDX_Check(pDX, IDC_PREFS_MAXIMIZE, m_AutoMaximize);
	DDX_Check(pDX, IDC_PREFS_SET_DEFAULT_COLORS, m_SetDefaultColors);
	DDX_Check(pDX, IDC_PREFS_WINDOW_FIT, m_WindowFit);
	DDX_Check(pDX, IDC_FLOOR_IS_VISIBLE_BY_DEFAULT, m_bIsFloorVisibleByDefault);
	DDX_Text(pDX, IDC_DEFAULT_BANKING, m_fDefaultBanking);
	DDX_Text(pDX, IDC_DEFAULT_HEADING, m_fDefaultHeading);
	DDX_Text(pDX, IDC_DEFAULT_PITCH, m_fDefaultPitch);
	DDX_Text(pDX, IDC_DEFAULT_FOW, m_fDefaultFOW);
	DDV_MinMaxFloat(pDX, m_fDefaultFOW, 1.f, 179.f);
	DDX_Check(pDX, IDC_PREFS_ALLWAYS_BCG, m_bIsBcgVisibleByDefault);
	DDX_Check(pDX, IDC_PREFS_ALLOW_SOUND_LOCK, m_bAllowSoundLock);
	//}}AFX_DATA_MAP
  
  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    m_Prefs.ap_MappingPaperColor = m_MappingPaper.GetColor();
    m_Prefs.ap_MappingActiveSurfaceColor = m_MappingActiveInk.GetColor();
    m_Prefs.ap_MappingInactiveSurfaceColor = m_MappingInactiveInk.GetColor();
    m_Prefs.ap_DefaultPaperColor = m_ModelPaper.GetColor();
    m_Prefs.ap_DefaultInkColor = m_ModelInk.GetColor();
    m_Prefs.ap_MappingWinBcgColor = m_MappingWinBcgColor.GetColor();

    m_Prefs.ap_CopyExistingWindowPrefs = m_PrefsCopy;
    m_Prefs.ap_bIsFloorVisibleByDefault = m_bIsFloorVisibleByDefault;
    m_Prefs.ap_bIsBcgVisibleByDefault = m_bIsBcgVisibleByDefault;
    m_Prefs.ap_AutoMaximizeWindow = m_AutoMaximize;
    m_Prefs.ap_AllwaysSeeLamp = m_AllwaysLamp;
    m_Prefs.ap_AutoWindowFit = m_WindowFit;
    m_Prefs.ap_SetDefaultColors = m_SetDefaultColors;
    m_Prefs.ap_bAllowSoundLock = m_bAllowSoundLock;

	  m_Prefs.ap_colDefaultAmbientColor = m_colorDefaultAmbientColor.GetColor();
	  m_Prefs.ap_fDefaultHeading = m_fDefaultHeading;
	  m_Prefs.ap_fDefaultPitch = m_fDefaultPitch;
	  m_Prefs.ap_fDefaultBanking = m_fDefaultBanking;
	  m_Prefs.ap_fDefaultFOW = m_fDefaultFOW;

    if( !theApp.m_WorkingTextures.IsEmpty())
    {
      m_Prefs.ap_DefaultWinBcgTexture = 
        ((CBcgTexture *) m_ComboWinBcgTexture.GetItemDataPtr( 
        m_ComboWinBcgTexture.GetCurSel()))->wt_FileName;
    }
    else
    {
    }
    INDEX iCurSel=m_ctrlGfxApi.GetCurSel();
    INDEX iOldGfxApi=theApp.m_iApi;
    if( iCurSel!=CB_ERR)
    {
      switch(iCurSel)
      {
      case 0:
        theApp.m_iApi=GAT_OGL;
        break;
#ifdef SE1_D3D
      case 1:
        theApp.m_iApi=GAT_D3D;
        break;
#endif // SE1_D3D
      default:
        {
        }
      }
      if( iOldGfxApi!=theApp.m_iApi)
      {
        _pGfx->ResetDisplayMode((enum GfxAPIType) theApp.m_iApi);
      }
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgPreferences, CDialog)
	//{{AFX_MSG_MAP(CDlgPreferences)
	ON_BN_CLICKED(IDC_ADD_WORKING_TEXTURE, OnAddWorkingTexture)
	ON_BN_CLICKED(IDC_REMOVE_WORKING_TEXTURE, OnRemoveWorkingTexture)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPreferences message handlers

BOOL CDlgPreferences::OnInitDialog() 
{
  CDialog::OnInitDialog();

  m_ctrlGfxApi.ResetContent();
  m_ctrlGfxApi.AddString(L"OpenGL");
#ifdef SE1_D3D
  m_ctrlGfxApi.AddString(L"DirectX");
#endif // SE1_D3D
  if( IsWindow(m_ctrlGfxApi.m_hWnd))
  {
    switch(theApp.m_iApi)
    {
    case GAT_OGL:
      m_ctrlGfxApi.SetCurSel(0);
      break;
#ifdef SE1_D3D
    case GAT_D3D:
      m_ctrlGfxApi.SetCurSel(1);
      break;
#endif // SE1_D3D
    }
  }

  InitTextureCombos();
  return TRUE;
}

void CDlgPreferences::InitTextureCombos()
{
  int iIndex;
  
  m_ComboWinBcgTexture.ResetContent();
  INDEX iChoosedWinBcg = 0;
  if( theApp.m_WorkingTextures.IsEmpty())
  {
    m_ComboWinBcgTexture.AddString( L"None available");
    m_ComboWinBcgTexture.EnableWindow( FALSE);
  }
  else
  {
    m_ComboWinBcgTexture.EnableWindow( TRUE);
    INDEX iTexCt = 0;      
    FOREACHINLIST( CBcgTexture, wt_ListNode, theApp.m_WorkingTextures, it_wt)
    {
      if( it_wt->wt_FileName == m_Prefs.ap_DefaultWinBcgTexture)
        iChoosedWinBcg = iTexCt;
      
      iIndex = m_ComboWinBcgTexture.AddString( CString(it_wt->wt_FileName.FileName()));
      m_ComboWinBcgTexture.SetItemDataPtr( iIndex, &it_wt.Current());
      iTexCt ++;
    }
  }
  m_ComboWinBcgTexture.SetCurSel( iChoosedWinBcg);
}

void CDlgPreferences::OnAddWorkingTexture() 
{
  // call file requester for opening documents
  CDynamicArray<CTFileName> afnWorkingTextures;
  _EngineGUI.FileRequester( "Choose textures to add", FILTER_TEX FILTER_END,
    "Working textures directory", "Textures\\", "", &afnWorkingTextures);
  // insert selected textures
  FOREACHINDYNAMICARRAY( afnWorkingTextures, CTFileName, itTexture)
  {
    // add new working texture
    theApp.AddModelerWorkingTexture( itTexture.Current());
  }
  if( afnWorkingTextures.Count() != 0)
  {
    InitTextureCombos();
    INDEX iTextures = m_ComboWinBcgTexture.GetCount();
    if( (iTextures > 0) && (iTextures != CB_ERR) )
    {
      // select last added texture as current
      m_ComboWinBcgTexture.SetCurSel( iTextures-1);
    }
  }
}

void CDlgPreferences::OnRemoveWorkingTexture() 
{
  if( theApp.m_WorkingTextures.Count() == 0)
  {
    return;
  }

  INDEX cur_sel = m_ComboWinBcgTexture.GetCurSel();
  if( cur_sel != LB_ERR)
  {
    CBcgTexture *pWT = (CBcgTexture *) m_ComboWinBcgTexture.GetItemDataPtr( cur_sel);
    pWT->wt_ListNode.Remove();
    
    _pTextureStock->Release( pWT->wt_TextureData);
    delete pWT;
    UpdateData( FALSE);
  }
  InitTextureCombos();
}

