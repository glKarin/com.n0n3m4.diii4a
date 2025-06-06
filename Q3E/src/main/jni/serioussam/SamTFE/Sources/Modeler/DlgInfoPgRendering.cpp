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

// DlgInfoPgRendering.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgRendering property page

IMPLEMENT_DYNCREATE(CDlgInfoPgRendering, CPropertyPage)

CDlgInfoPgRendering::CDlgInfoPgRendering() : CPropertyPage(CDlgInfoPgRendering::IDD)
{
	//{{AFX_DATA_INIT(CDlgInfoPgRendering)
	m_strMipModel = _T("");
	m_strSurfaceName = _T("");
	//}}AFX_DATA_INIT
  
	m_IsDoubleSided.SetDialogPtr( this);
	m_IsInvisible.SetDialogPtr( this);
	m_IsDiffuse.SetDialogPtr( this);
	m_IsReflections.SetDialogPtr( this);
	m_IsSpecular.SetDialogPtr( this);
	m_IsBump.SetDialogPtr( this);
	m_IsDetail.SetDialogPtr( this);

  m_colorDiffuse.m_pwndParentDialog = this;
  m_colorBump.m_pwndParentDialog = this;
  m_colorSpecular.m_pwndParentDialog = this;
  m_colorReflections.m_pwndParentDialog = this;
  theApp.m_pPgInfoRendering = this;
}

CDlgInfoPgRendering::~CDlgInfoPgRendering()
{
}

void CDlgInfoPgRendering::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
  
  CModelerDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;

  // if dialog is recieving data
  if( (!pDX->m_bSaveAndValidate) && IsWindow(m_colorDiffuse.m_hWnd) )
  {
    enum SurfaceShadingType sstFirstShading = SST_INVALID;
    enum SurfaceTranslucencyType sttFirstTranslucency = STT_INVALID;
    BOOL bSameShading = TRUE;
    BOOL bSameTranslucency = TRUE;
    BOOL bSameDiffuse = TRUE;
    BOOL bSameReflections = TRUE;
    BOOL bSameSpecular = TRUE;
    BOOL bSameBump = TRUE;
    COLOR colFirstDiffuse;
    COLOR colFirstReflections;
    COLOR colFirstSpecular;
    COLOR colFirstBump;
    CTString strFirstName = "Invalid name";

    ULONG ulFlagsOn = MAX_ULONG;
    ULONG ulFlagsOff = MAX_ULONG;

    INDEX ctSelectedSurfaces = pDoc->GetCountOfSelectedSurfaces();

    BOOL bSelectionExists = ctSelectedSurfaces != 0;
    BOOL bFirstSelected = TRUE;
    ModelMipInfo &mmi = pDoc->m_emEditModel.edm_md.md_MipInfos[ pDoc->m_iCurrentMip];
    for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
    {
      MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
      // skip non selected surfaces
      if( !(ms.ms_ulRenderingFlags&SRF_SELECTED)) continue;
      strFirstName = ms.ms_Name;
      ULONG ulRenderFlags = ms.ms_ulRenderingFlags;
      ulFlagsOn &= ulRenderFlags;
      ulFlagsOff &= ~ulRenderFlags;
      
      if( sstFirstShading == SST_INVALID) sstFirstShading = ms.ms_sstShadingType;
      if( sttFirstTranslucency == STT_INVALID) sttFirstTranslucency = ms.ms_sttTranslucencyType;

      if( bFirstSelected)
      {
        bFirstSelected = FALSE;
        colFirstDiffuse = ms.ms_colDiffuse;
        colFirstReflections = ms.ms_colReflections;
        colFirstSpecular = ms.ms_colSpecular;
        colFirstBump = ms.ms_colBump;
      }
      else
      {
        if( ms.ms_colDiffuse != colFirstDiffuse) bSameDiffuse = FALSE;
        if( ms.ms_colReflections != colFirstReflections) bSameReflections = FALSE;
        if( ms.ms_colSpecular != colFirstSpecular) bSameSpecular = FALSE;
        if( ms.ms_colBump != colFirstBump) bSameBump = FALSE;
      }
      
      if( sstFirstShading != ms.ms_sstShadingType) bSameShading = FALSE;
      if( sttFirstTranslucency != ms.ms_sttTranslucencyType) bSameTranslucency = FALSE;
    }
	  
    if( bSameDiffuse) m_colorDiffuse.SetColor( colFirstDiffuse);
    else m_colorDiffuse.SetMixedColor();
    if( bSameReflections) m_colorReflections.SetColor( colFirstReflections);
    else m_colorReflections.SetMixedColor();
    if( bSameSpecular) m_colorSpecular.SetColor( colFirstSpecular);
    else m_colorSpecular.SetMixedColor();
    if( bSameBump) m_colorBump.SetColor( colFirstBump);
    else m_colorBump.SetMixedColor();

    CTString strText;
    if( ctSelectedSurfaces == 0) strText = "No surfaces selected";
    else if( ctSelectedSurfaces > 1) strText.PrintF( "%d surfaces selected", ctSelectedSurfaces);
    else strText.PrintF( "Surface: %s", strFirstName);
    m_strSurfaceName = strText;

    strText.PrintF( "Mip: %d", pDoc->m_iCurrentMip);
    m_strMipModel = strText;

// apply flags to controls
#define SET_TRI_STATE_TO_CTRL( ctrl, flag)\
  if((ulFlagsOn & flag) && !(ulFlagsOff & flag)) ctrl.SetCheck( 1);\
  else if(!(ulFlagsOn & flag) && (ulFlagsOff & flag)) ctrl.SetCheck( 0);\
  else ctrl.SetCheck( 2);

    SET_TRI_STATE_TO_CTRL( m_IsBump,        SRF_BUMP);
    SET_TRI_STATE_TO_CTRL( m_IsDetail,      SRF_DETAIL);
    SET_TRI_STATE_TO_CTRL( m_IsDiffuse,     SRF_DIFFUSE);
    SET_TRI_STATE_TO_CTRL( m_IsReflections, SRF_REFLECTIONS);
    SET_TRI_STATE_TO_CTRL( m_IsSpecular,    SRF_SPECULAR);
    SET_TRI_STATE_TO_CTRL( m_IsInvisible,   SRF_INVISIBLE);
    SET_TRI_STATE_TO_CTRL( m_IsDoubleSided, SRF_DOUBLESIDED);

    if( !bSameShading) m_comboShading.SetCurSel( -1);
    else
    {
      INDEX iShadingCombo = -1;
      switch( sstFirstShading)
      {
      case SST_FULLBRIGHT:   { iShadingCombo = 0; break;};
      case SST_MATTE:        { iShadingCombo = 1; break;};
      case SST_FLAT:         { iShadingCombo = 2; break;};
      }
      m_comboShading.SetCurSel( iShadingCombo);
    }
  
    if( !bSameTranslucency) m_comboTranslucency.SetCurSel( -1);
    else
    {
      INDEX iTranslucency = -1;
      switch( sttFirstTranslucency)
      {
      case STT_OPAQUE: {            iTranslucency = 0; break;};
      case STT_TRANSPARENT:{        iTranslucency = 1; break;};
      case STT_TRANSLUCENT:{        iTranslucency = 2; break;};
      case STT_ADD:{                iTranslucency = 3; break;};
      case STT_MULTIPLY:{           iTranslucency = 4; break;};
      }
      m_comboTranslucency.SetCurSel( iTranslucency);
    }

    m_comboTranslucency.EnableWindow( bSelectionExists);
    m_comboShading.EnableWindow( bSelectionExists);

    m_IsDoubleSided.EnableWindow( bSelectionExists);
	  m_IsInvisible.EnableWindow( bSelectionExists);
    m_IsDiffuse.EnableWindow( bSelectionExists);
	  m_IsReflections.EnableWindow( bSelectionExists);
	  m_IsSpecular.EnableWindow( bSelectionExists);
	  m_IsBump.EnableWindow( bSelectionExists);
	  m_IsDetail.EnableWindow( bSelectionExists);
    
	  m_colorSpecular.EnableWindow( bSelectionExists);
	  m_colorReflections.EnableWindow( bSelectionExists);
	  m_colorDiffuse.EnableWindow( bSelectionExists);
	  m_colorBump.EnableWindow( bSelectionExists);
	
    GetDlgItem( IDC_TRANSLUCENCY)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_TRANSLUCENCY_T)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_MIP_MODEL)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_SURFACE_NAME)->EnableWindow( bSelectionExists);
    
    GetDlgItem( IDC_SHADING_T)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_DIFFUSE_COLOR_T)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_REFLECTION_COLOR_T2)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_SPECULAR_COLOR_T2)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_BUMP_COLOR_T)->EnableWindow( bSelectionExists);
    
    Invalidate( FALSE);
    m_udAllValues.MarkUpdated();
  }

	//{{AFX_DATA_MAP(CDlgInfoPgRendering)
	DDX_Control(pDX, IDC_TRANSLUCENCY, m_comboTranslucency);
	DDX_Control(pDX, IDC_SHADING, m_comboShading);
	DDX_Control(pDX, IDC_SPECULAR_COLOR, m_colorSpecular);
	DDX_Control(pDX, IDC_REFLECTION_COLOR, m_colorReflections);
	DDX_Control(pDX, IDC_DIFUSE_COLOR, m_colorDiffuse);
	DDX_Control(pDX, IDC_BUMP_COLOR, m_colorBump);
	DDX_Text(pDX, IDC_MIP_MODEL, m_strMipModel);
	DDX_Text(pDX, IDC_SURFACE_NAME, m_strSurfaceName);
  DDX_Control(pDX, IDC_DOUBLE_SIDED, m_IsDoubleSided);
	DDX_Control(pDX, IDC_INVISIBLE, m_IsInvisible);
	DDX_Control(pDX, IDC_USE_DIFUSE, m_IsDiffuse);
	DDX_Control(pDX, IDC_USE_REFLECTIVITY, m_IsReflections);
	DDX_Control(pDX, IDC_USE_SPECULAR, m_IsSpecular);
	DDX_Control(pDX, IDC_USE_BUMP, m_IsBump);
	DDX_Control(pDX, IDC_DETAIL, m_IsDetail);
	//}}AFX_DATA_MAP

// set flags back to surface
#define TRI_STATE_CTRL_TO_FLAGS( ctrl, flag)\
  if( (ctrl.GetCheck() == 1) && !(ms.ms_ulRenderingFlags & flag) ) {\
    ms.ms_ulRenderingFlags |= flag;\
  } else if( (ctrl.GetCheck() == 0) && (ms.ms_ulRenderingFlags & flag) ) {\
    ms.ms_ulRenderingFlags &= ~flag;\
  }
  // if dialog gives data
  if( pDX->m_bSaveAndValidate)
  {
    INDEX iComboShadow = m_comboShading.GetCurSel();
    INDEX iComboTranslucency = m_comboTranslucency.GetCurSel();

    ModelMipInfo &mmi = pDoc->m_emEditModel.edm_md.md_MipInfos[ pDoc->m_iCurrentMip];
    for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
    {
      MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
      ULONG ulFlagsBefore = ms.ms_ulRenderingFlags;
      // skip non selected surfaces
      if( !(ms.ms_ulRenderingFlags&SRF_SELECTED)) continue;
      TRI_STATE_CTRL_TO_FLAGS( m_IsBump,        SRF_BUMP);
      TRI_STATE_CTRL_TO_FLAGS( m_IsDetail,      SRF_DETAIL);
      TRI_STATE_CTRL_TO_FLAGS( m_IsDiffuse,     SRF_DIFFUSE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsReflections, SRF_REFLECTIONS);
      TRI_STATE_CTRL_TO_FLAGS( m_IsSpecular,    SRF_SPECULAR);
      TRI_STATE_CTRL_TO_FLAGS( m_IsInvisible,   SRF_INVISIBLE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsDoubleSided, SRF_DOUBLESIDED);
      ULONG ulFlagsAfter = ms.ms_ulRenderingFlags;
      
      BOOL bBumpSet = ((ulFlagsBefore&SRF_BUMP)!=(ulFlagsAfter&SRF_BUMP))&&(ulFlagsAfter&SRF_BUMP);
      BOOL bDetailSet = ((ulFlagsBefore&SRF_DETAIL)!=(ulFlagsAfter&SRF_DETAIL))&&(ulFlagsAfter&SRF_DETAIL);
      // if bump set
      if( bBumpSet)
      {
        // turn off detail
        ms.ms_ulRenderingFlags &= ~SRF_DETAIL;
        theApp.m_chGlobal.MarkChanged();
      }
      // if detail set
      if( bDetailSet)
      {
        // turn off bump
        ms.ms_ulRenderingFlags &= ~SRF_BUMP;
        theApp.m_chGlobal.MarkChanged();
      }

      switch( iComboShadow)
      {
      case 0:{ ms.ms_sstShadingType = SST_FULLBRIGHT; break;};
      case 1:{ ms.ms_sstShadingType = SST_MATTE; break;};
      case 2:{ ms.ms_sstShadingType = SST_FLAT; break;};
      }

      switch( iComboTranslucency)
      {
      case 0:{ ms.ms_sttTranslucencyType = STT_OPAQUE; break;};
      case 1:{ ms.ms_sttTranslucencyType = STT_TRANSPARENT; break;};
      case 2:{ ms.ms_sttTranslucencyType = STT_TRANSLUCENT; break;};
      case 3:{ ms.ms_sttTranslucencyType = STT_ADD; break;};
      case 4:{ ms.ms_sttTranslucencyType = STT_MULTIPLY; break;};
      }

      ms.ms_colDiffuse = m_colorDiffuse.GetColor();
      ms.ms_colReflections = m_colorReflections.GetColor();
      ms.ms_colSpecular = m_colorSpecular.GetColor();
      ms.ms_colBump = m_colorBump.GetColor();
    }

    // update view
    pDoc->m_emEditModel.edm_md.md_bPreparedForRendering = FALSE;
    pDoc->SetModifiedFlag();
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgInfoPgRendering, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgInfoPgRendering)
	ON_CBN_SELCHANGE(IDC_SHADING, OnSelchangeShading)
	ON_CBN_SELCHANGE(IDC_TRANSLUCENCY, OnSelchangeTranslucency)
	ON_BN_CLICKED(IDC_SELECT_ALL_SURFACES, OnSelectAllSurfaces)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgRendering message handlers

BOOL CDlgInfoPgRendering::OnIdle(LONG lCount)
{
  // if view, surface or mip changed
  if (!theApp.m_chGlobal.IsUpToDate(m_udAllValues) ) {
    UpdateData(FALSE);
  }
  return TRUE;
}

void CDlgInfoPgRendering::OnSelchangeShading() 
{
  UpdateData(TRUE);
}

void CDlgInfoPgRendering::OnSelchangeTranslucency() 
{
  UpdateData(TRUE);
}

BOOL CDlgInfoPgRendering::PreTranslateMessage(MSG* pMsg) 
{
  CModelerDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL) return TRUE;
	BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  
  if(pMsg->message==WM_KEYDOWN)
  {
    if( pMsg->wParam==VK_TAB)
    {
      if( bShift) pDoc->SelectPreviousSurface();
      else        pDoc->SelectNextSurface();
      return TRUE;
    }
    if( pMsg->wParam=='Z')
    {
      pDoc->OnLinkSurfaces();
      return TRUE;
    }
  }

	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CDlgInfoPgRendering::OnSelectAllSurfaces() 
{
  CModelerDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;
  pDoc->SelectAllSurfaces();
  UpdateData( FALSE);
}
