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

// DlgPgSector.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgSector.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgSector property page

IMPLEMENT_DYNCREATE(CDlgPgSector, CPropertyPage)

CDlgPgSector::CDlgPgSector() : CPropertyPage(CDlgPgSector::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgSector)
	m_iBrowseModeRadio = -1;
	m_strSectorName = _T("");
	m_radioInclude = -1;
	//}}AFX_DATA_INIT
  
  if( 
    CTString( CStringA(theApp.GetProfileString(L"World editor", L"Color browsing mode (info)"))) ==
    CTString("RGB"))
  {
	  m_iBrowseModeRadio = 0;
  }
  else
  {
	  m_iBrowseModeRadio = 1;
  }
  
  m_colLastSectorAmbientColor = -1;
  m_bLastSectorAmbientColorMixed = FALSE;
  m_SectorAmbientColor.SetPickerType(  CColoredButton::PT_MFC);
  m_ctrlClassificationFlags.SetDialogPtr(this);
  m_ctrlVisibilityFlags.SetDialogPtr(this);
  m_ctrlClassificationFlags.SetEditableMask(0xFFFF0000);
  m_ctrlVisibilityFlags.SetEditableMask(0x0000FFFF);
}

CDlgPgSector::~CDlgPgSector()
{
}

#define MIXED_NAME "Mixed name"
void CDlgPgSector::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

  CPropertyPage::DoDataExchange(pDX);

  // mark that property page has been modified
  SetModified( TRUE);

  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL)  return;
  // sector mode must be on
  if( pDoc->GetEditingMode() != SECTOR_MODE)  return;

  // if dialog is receiving data and control windows are valid
  if( (pDX->m_bSaveAndValidate == FALSE) && IsWindow( m_SectorAmbientColor.m_hWnd) )
  {
    BOOL bSelectionExists = pDoc->m_selSectorSelection.Count() != 0;
    m_comboContentType.ResetContent();
    for(INDEX iContentType=0; iContentType<MAX_UBYTE; iContentType++)
    {
      CTString &strContent = pDoc->m_woWorld.wo_actContentTypes[iContentType].ct_strName;
      if( strContent == "") break;
      m_comboContentType.AddString( CString(strContent));
    }

    m_comboEnvironmentType.ResetContent();
    for(INDEX iEnvironmentType=0; iEnvironmentType<MAX_UBYTE; iEnvironmentType++)
    {
      CTString &strEnvironment = pDoc->m_woWorld.wo_aetEnvironmentTypes[iEnvironmentType].et_strName;
      if( strEnvironment == "") break;
      m_comboEnvironmentType.AddString( CString(strEnvironment));
    }

    if( bSelectionExists)
    {
      BOOL bSameForceField = TRUE;
      BOOL bSameFog = TRUE;
      BOOL bSameHaze = TRUE;
      INDEX iFirstForceField;
      INDEX iFirstFog;
      INDEX iFirstHaze;
      m_comboForceField.ResetContent();
      m_comboFog.ResetContent();
      m_comboHaze.ResetContent();
      CBrush3D *pbrBrush = NULL;
      INDEX iSector = 0;
      // for all sectors
      FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
      {
        if( iSector == 0)
        {
          iFirstForceField = itbsc->GetForceType();
          iFirstFog = itbsc->GetFogType();
          iFirstHaze = itbsc->GetHazeType();
        }
        INDEX iForceField = itbsc->GetForceType();
        if( iForceField != iFirstForceField) bSameForceField = FALSE;
        INDEX iFog = itbsc->GetFogType();
        if( iFog != iFirstFog) bSameFog = FALSE;
        INDEX iHaze = itbsc->GetHazeType();
        if( iHaze != iFirstHaze) bSameHaze = FALSE;
        // get sector brush
        if( (pbrBrush == NULL) || (pbrBrush == itbsc->bsc_pbmBrushMip->bm_pbrBrush) )
        {
          if( pbrBrush == NULL)
          {
            pbrBrush = itbsc->bsc_pbmBrushMip->bm_pbrBrush;
            for(INDEX iForceField=0; iForceField<MAX_UBYTE; iForceField++)
            {
              CTString strForceName = pbrBrush->br_penEntity->GetForceName( iForceField);
              if( strForceName == "") break;
              m_comboForceField.AddString( CString(strForceName));
            }
            for(INDEX iFog=0; iFog<MAX_UBYTE; iFog++)
            {
              CTString strFogName = pbrBrush->br_penEntity->GetFogName( iFog);
              if( strFogName == "") break;
              m_comboFog.AddString( CString(strFogName));
            }
            for(INDEX iHaze=0; iHaze<MAX_UBYTE; iHaze++)
            {
              CTString strHazeName = pbrBrush->br_penEntity->GetHazeName( iHaze);
              if( strHazeName == "") break;
              m_comboHaze.AddString( CString(strHazeName));
            }
          }
        }
        else
        {
          m_comboForceField.ResetContent();
          m_comboFog.ResetContent();
          m_comboHaze.ResetContent();
          bSameForceField = FALSE;
          bSameFog = FALSE;
          bSameHaze = FALSE;
          GetDlgItem( IDC_STATIC_FORCE_FIELD_T)->EnableWindow( FALSE);
          GetDlgItem( IDC_FORCE_FIELD_COMBO)->EnableWindow( FALSE);
          GetDlgItem( IDC_STATIC_FOG_T)->EnableWindow( FALSE);
          GetDlgItem( IDC_FOG_COMBO)->EnableWindow( FALSE);
          GetDlgItem( IDC_STATIC_HAZE_T)->EnableWindow( FALSE);
          GetDlgItem( IDC_HAZE_COMBO)->EnableWindow( FALSE);
        }
        iSector++;
      }
      if( bSameForceField) m_comboForceField.SetCurSel( iFirstForceField);
      if( bSameFog) m_comboFog.SetCurSel( iFirstFog);
      if( bSameHaze) m_comboHaze.SetCurSel( iFirstHaze);
    }

    BOOL bDisableForce = FALSE;
    BOOL bDisableFog = FALSE;
    BOOL bDisableHaze = FALSE;
    if( m_comboForceField.GetCount() == 0) bDisableForce = TRUE;
    if( m_comboFog.GetCount() == 0) bDisableFog = TRUE;
    if( m_comboHaze.GetCount() == 0) bDisableHaze = TRUE;
    GetDlgItem( IDC_STATIC_FORCE_FIELD_T)->EnableWindow( bSelectionExists&&!bDisableForce);
    GetDlgItem( IDC_FORCE_FIELD_COMBO)->EnableWindow( bSelectionExists&&!bDisableForce);
    GetDlgItem( IDC_STATIC_FOG_T)->EnableWindow( bSelectionExists&&!bDisableFog);
    GetDlgItem( IDC_FOG_COMBO)->EnableWindow( bSelectionExists&&!bDisableFog);
    GetDlgItem( IDC_STATIC_HAZE_T)->EnableWindow( bSelectionExists&&!bDisableHaze);
    GetDlgItem( IDC_HAZE_COMBO)->EnableWindow( bSelectionExists&&!bDisableHaze);
    
    m_ctrlVisibilityFlags.EnableWindow( bSelectionExists);
    m_ctrlClassificationFlags.EnableWindow( bSelectionExists);
    

    INDEX iSector = 0;
    BOOL bSameContent = TRUE;
    INDEX iFirstContent;
    BOOL bSameEnvironment = TRUE;
    INDEX iFirstEnvironment;
    INDEX iInclude=-1;
    // for all sectors
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      CBrushSector &bsc=*itbsc;
      if( iSector == 0)
      {
        iFirstContent = bsc.GetContentType();
        iFirstEnvironment = bsc.GetEnvironmentType();
        m_SectorAmbientColor.SetColor( bsc.bsc_colAmbient);
        m_strSectorName = bsc.bsc_strName;
        m_ctrlVisibilityFlags.SetFlags(bsc.bsc_ulVisFlags);
        m_ctrlClassificationFlags.SetFlags(bsc.bsc_ulVisFlags);
        iInclude=(bsc.bsc_ulFlags2&BSCF2_VISIBILITYINCLUDE) ? 0 : 1;
      }
      else
      {
        if( bsc.GetContentType() != iFirstContent) bSameContent = FALSE;
        if( bsc.GetEnvironmentType() != iFirstEnvironment) bSameEnvironment = FALSE;
        if(m_SectorAmbientColor.GetColor() != bsc.bsc_colAmbient)
        {
          m_SectorAmbientColor.SetMixedColor();
        }
        if( CTString( CStringA(m_strSectorName)) != bsc.bsc_strName)
        {
          m_strSectorName = MIXED_NAME;
        }
        m_ctrlVisibilityFlags.MergeFlags(bsc.bsc_ulVisFlags);
        m_ctrlClassificationFlags.MergeFlags(bsc.bsc_ulVisFlags);
        INDEX iNewInclude=(bsc.bsc_ulFlags2&BSCF2_VISIBILITYINCLUDE) ? 0 : 1;
        if( iInclude!=-1 && iNewInclude!=iInclude)
        {
          iInclude=-1;
        }
      }
      iSector++;
    }

    m_radioInclude=iInclude;

    m_colLastSectorAmbientColor = m_SectorAmbientColor.GetColor();
    m_bLastSectorAmbientColorMixed = !m_SectorAmbientColor.IsColorValid();
    m_SectorAmbientColor.EnableWindow( bSelectionExists);
	  m_comboContentType.EnableWindow( bSelectionExists);
	  m_comboEnvironmentType.EnableWindow( bSelectionExists);

    GetDlgItem( IDC_STATIC_CONTENT_TYPE_T)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_ENVIRONMENT_TYPE_T)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_SECTOR_NAME)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_SECTOR_NAME)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_AMBIENT_COLOR_T)->EnableWindow( bSelectionExists);

    GetDlgItem( IDC_SECTOR_INCLUDE)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_SECTOR_EXCLUDE)->EnableWindow( bSelectionExists);    

    if( bSameContent) m_comboContentType.SetCurSel( iFirstContent);
    else m_comboContentType.SetCurSel(-1);
    if( bSameEnvironment) m_comboEnvironmentType.SetCurSel( iFirstEnvironment);
    else m_comboEnvironmentType.SetCurSel(-1);

    m_udSectorsData.MarkUpdated();
  }

	//{{AFX_DATA_MAP(CDlgPgSector)
	DDX_Control(pDX, ID_CLASSIFICATION_FLAGS, m_ctrlClassificationFlags);
	DDX_Control(pDX, ID_VISIBILITY_FLAGS, m_ctrlVisibilityFlags);
	DDX_Control(pDX, IDC_STATIC_ENVIRONMENT_TYPE, m_comboEnvironmentType);
	DDX_Control(pDX, IDC_HAZE_COMBO, m_comboHaze);
	DDX_Control(pDX, IDC_FOG_COMBO, m_comboFog);
	DDX_Control(pDX, IDC_FORCE_FIELD_COMBO, m_comboForceField);
	DDX_Control(pDX, IDC_CONTENT_TYPE_COMBO, m_comboContentType);
	DDX_Control(pDX, ID_SECTOR_COLOR, m_SectorAmbientColor);
	DDX_Text(pDX, IDC_SECTOR_NAME, m_strSectorName);
	DDX_Radio(pDX, IDC_SECTOR_INCLUDE, m_radioInclude);
	//}}AFX_DATA_MAP

  // if dialog is giving data and control windows are valid
  if( (pDX->m_bSaveAndValidate != FALSE) && IsWindow( m_SectorAmbientColor.m_hWnd) )
  {
    // for all sectors
    FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
    {
      CBrushSector &bsc=*itbsc;
      INDEX iNewContent = m_comboContentType.GetCurSel();
      if( iNewContent!=CB_ERR)
      {
        bsc.SetContentType( iNewContent);
      }
      INDEX iNewEnvironment = m_comboEnvironmentType.GetCurSel();
      if( iNewEnvironment!=CB_ERR)
      {
        bsc.SetEnvironmentType( iNewEnvironment);
      }
      if( m_strSectorName != MIXED_NAME)
      {
        bsc.bsc_strName = CStringA(m_strSectorName);
      }
      
      INDEX iNewForceField = m_comboForceField.GetCurSel();
      if( iNewForceField != CB_ERR) bsc.SetForceType( iNewForceField);

      INDEX iNewFog = m_comboFog.GetCurSel();
      if( iNewFog != CB_ERR) bsc.SetFogType( iNewFog);

      INDEX iNewHaze = m_comboHaze.GetCurSel();
      if( iNewHaze != CB_ERR) bsc.SetHazeType( iNewHaze);

      m_ctrlVisibilityFlags.ApplyChange(bsc.bsc_ulVisFlags);
      m_ctrlClassificationFlags.ApplyChange(bsc.bsc_ulVisFlags);
      if( m_radioInclude!=-1)
      {
        if( m_radioInclude==0) bsc.bsc_ulFlags2|=BSCF2_VISIBILITYINCLUDE;
        else                   bsc.bsc_ulFlags2&=~BSCF2_VISIBILITYINCLUDE;
      }
    }

    COLOR colAmbient = m_SectorAmbientColor.GetColor();
    BOOL bColorChanged =
      (colAmbient != m_colLastSectorAmbientColor) ||
      (m_bLastSectorAmbientColorMixed && m_SectorAmbientColor.IsColorValid());
      
    if( bColorChanged)
    {
      // for all sectors
      FOREACHINDYNAMICCONTAINER(pDoc->m_selSectorSelection, CBrushSector, itbsc)
      {
        // set new color to sectors
        itbsc->bsc_colAmbient = colAmbient;
        itbsc->UncacheLightMaps();
      }
    }
    m_udSectorsData.MarkUpdated();
    pDoc->SetModifiedFlag( TRUE);
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgPgSector, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgSector)
	ON_WM_HSCROLL()
	ON_CBN_DROPDOWN(IDC_CONTENT_TYPE_COMBO, OnDropdownContentTypeCombo)
	ON_CBN_SELCHANGE(IDC_CONTENT_TYPE_COMBO, OnSelchangeContentTypeCombo)
	ON_CBN_SELCHANGE(IDC_FORCE_FIELD_COMBO, OnSelchangeForceFieldCombo)
	ON_CBN_DROPDOWN(IDC_FORCE_FIELD_COMBO, OnDropdownForceFieldCombo)
	ON_CBN_DROPDOWN(IDC_FOG_COMBO, OnDropdownFogCombo)
	ON_CBN_SELCHANGE(IDC_FOG_COMBO, OnSelchangeFogCombo)
	ON_CBN_DROPDOWN(IDC_HAZE_COMBO, OnDropdownHazeCombo)
	ON_CBN_SELCHANGE(IDC_HAZE_COMBO, OnSelchangeHazeCombo)
	ON_CBN_DROPDOWN(IDC_STATIC_ENVIRONMENT_TYPE, OnDropdownStaticEnvironmentType)
	ON_CBN_SELCHANGE(IDC_STATIC_ENVIRONMENT_TYPE, OnSelchangeStaticEnvironmentType)
	ON_BN_CLICKED(IDC_SECTOR_INCLUDE, OnSectorInclude)
	ON_BN_CLICKED(IDC_SECTOR_EXCLUDE, OnSectorExclude)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgSector message handlers

BOOL CDlgPgSector::OnIdle(LONG lCount)
{
  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( (pDoc == NULL) || !IsWindow(m_hWnd)) return FALSE;

  COLOR colSectorAmbientColor = m_SectorAmbientColor.GetColor();

  BOOL bColorChanged =
    (colSectorAmbientColor != m_colLastSectorAmbientColor) ||
    (m_bLastSectorAmbientColorMixed && m_SectorAmbientColor.IsColorValid());
  // if color was changed from last idle
  if( bColorChanged)
  {
    // view the color change
    UpdateData(TRUE);
    GetDlgItem( ID_SECTOR_COLOR)->Invalidate();
    // set new sector color
    m_colLastSectorAmbientColor = colSectorAmbientColor;
    m_bLastSectorAmbientColorMixed = FALSE;
  }
  // if selections have been changed (they are not up to date)
  if( !pDoc->m_chSelections.IsUpToDate( m_udSectorsData))
  {
    // update dialog data
    UpdateData( FALSE);
  }
  return TRUE;
}


void CDlgPgSector::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
  UpdateData( TRUE);
  m_colLastSectorAmbientColor = m_SectorAmbientColor.GetColor();
  m_bLastSectorAmbientColorMixed = FALSE;
  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;
  pDoc->SetModifiedFlag(TRUE);
}


void CDlgPgSector::OnDropdownContentTypeCombo() 
{
  m_comboContentType.SetDroppedWidth( 256);
}

void CDlgPgSector::OnSelchangeContentTypeCombo() 
{
  UpdateData( TRUE);
}

void CDlgPgSector::OnSelchangeForceFieldCombo() 
{
  UpdateData( TRUE);
}

void CDlgPgSector::OnSelchangeFogCombo() 
{
  UpdateData( TRUE);
}

void CDlgPgSector::OnSelchangeHazeCombo() 
{
  UpdateData( TRUE);
}

BOOL CDlgPgSector::PreTranslateMessage(MSG* pMsg) 
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


void CDlgPgSector::OnDropdownForceFieldCombo() 
{
  m_comboForceField.SetDroppedWidth( 256);
}

void CDlgPgSector::OnDropdownFogCombo() 
{
  m_comboFog.SetDroppedWidth( 256);
}

void CDlgPgSector::OnDropdownHazeCombo() 
{
  m_comboHaze.SetDroppedWidth( 256);
}

void CDlgPgSector::OnDropdownStaticEnvironmentType() 
{
  m_comboEnvironmentType.SetDroppedWidth( 256);
}

void CDlgPgSector::OnSelchangeStaticEnvironmentType() 
{
  UpdateData( TRUE);
}

void CDlgPgSector::OnSectorInclude() 
{
  UpdateData( TRUE);
}

void CDlgPgSector::OnSectorExclude() 
{
  UpdateData( TRUE);
}
