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

// DlgPgShadow.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgShadow.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgShadow property page

IMPLEMENT_DYNCREATE(CDlgPgShadow, CPropertyPage)

CDlgPgShadow::CDlgPgShadow() : CPropertyPage(CDlgPgShadow::IDD)
{
  m_ctrlShadowColor.SetPickerType(  CColoredButton::PT_MFC);
	//{{AFX_DATA_INIT(CDlgPgShadow)
	//}}AFX_DATA_INIT
}

CDlgPgShadow::~CDlgPgShadow()
{
}

void CDlgPgShadow::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

  CPropertyPage::DoDataExchange(pDX);
  // mark that property page has been modified
  SetModified( TRUE);

  // obtain document
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL)  return;
  // polygon mode must be on
  if( pDoc->GetEditingMode() != POLYGON_MODE)  return;
  // get flags of control activity
  BOOL bSelectionExists = pDoc->m_selPolygonSelection.Count() != 0;
  
  // if dialog is recieving data and control window is valid
  if( (pDX->m_bSaveAndValidate == FALSE) && IsWindow( m_NoShadow.m_hWnd) )  
  {
    // initialize combo boxes
    InitComboBoxes();

    // polygon controls exist if polygon selection exists
    m_bNoPlaneDiffusion.EnableWindow( bSelectionExists);
    m_bDarkCorners.EnableWindow( bSelectionExists);
    m_bDynamicLightsOnly.EnableWindow( bSelectionExists);
    m_bHasDirectionalShadows.EnableWindow( bSelectionExists);    
    m_bHasPreciseShadows.EnableWindow( bSelectionExists);    
    m_bHasDirectionalAmbient.EnableWindow( bSelectionExists);    
    m_IsLightBeamPassable.EnableWindow( bSelectionExists);
    m_bDontReceiveShadows.EnableWindow( bSelectionExists);
    m_bNoDynamicLights.EnableWindow( bSelectionExists);
    m_NoShadow.EnableWindow( bSelectionExists);
	  m_ctrlComboClusterSize.EnableWindow( bSelectionExists);
	  m_comboShadowBlend.EnableWindow( bSelectionExists);
	  m_ComboIllumination.EnableWindow( bSelectionExists);
	  m_ctrlComboGradient.EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_CLUSTER_SIZE)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_ILLUMINATION)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_GRADIENT)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_SHADOW_COLOR)->EnableWindow( bSelectionExists);
    GetDlgItem( IDC_STATIC_SHADOW_BLEND)->EnableWindow( bSelectionExists);    
	  m_ctrlShadowColor.EnableWindow( bSelectionExists);

    // if selection exists, calculate tri-state value of attribute intersection
    if( bSelectionExists)
    {
      // get properties from first polygon
      UBYTE ubFirstIllumination;
      SBYTE sbFirstShadowClusterSize;
      UBYTE ubFirstShadowBlend;
      UBYTE ubFirstGradient;
      BOOL bSameIllumination = TRUE;
      BOOL bSameShadowClusterSize = TRUE;
      BOOL bSameShadowBlend = TRUE;
      BOOL bSameGradient = TRUE;

      ULONG ulFlagsOn = MAX_ULONG;
      ULONG ulFlagsOff = MAX_ULONG;

      INDEX iPolygon = 0;
      // for each of the selected polygons
      FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        ulFlagsOn &= itbpo->bpo_ulFlags;
        ulFlagsOff &= ~itbpo->bpo_ulFlags;

        if( iPolygon == 0)
        {
          ubFirstIllumination = itbpo->bpo_bppProperties.bpp_ubIlluminationType;
          sbFirstShadowClusterSize = itbpo->bpo_bppProperties.bpp_sbShadowClusterSize;
          ubFirstShadowBlend = itbpo->bpo_bppProperties.bpp_ubShadowBlend;
          ubFirstGradient = itbpo->bpo_bppProperties.bpp_ubGradientType;
      	  m_ctrlShadowColor.SetColor( itbpo->bpo_colShadow);
        }
        else
        {
        	if( m_ctrlShadowColor.GetColor() != itbpo->bpo_colShadow)
          {
            m_ctrlShadowColor.SetMixedColor();
          }
          if( itbpo->bpo_bppProperties.bpp_ubIlluminationType != ubFirstIllumination) bSameIllumination = FALSE;
          if( itbpo->bpo_bppProperties.bpp_sbShadowClusterSize != sbFirstShadowClusterSize) bSameShadowClusterSize = FALSE;
          if( itbpo->bpo_bppProperties.bpp_ubShadowBlend != ubFirstShadowBlend) bSameShadowBlend = FALSE;
          if( itbpo->bpo_bppProperties.bpp_ubGradientType != ubFirstGradient) bSameGradient = FALSE;
        }
        iPolygon++;
      }

// apply flags to controls
#define SET_TRI_STATE_TO_CTRL( ctrl, flag)\
  if((ulFlagsOn & flag) && !(ulFlagsOff & flag)) ctrl.SetCheck( 1);\
  else if(!(ulFlagsOn & flag) && (ulFlagsOff & flag)) ctrl.SetCheck( 0);\
  else ctrl.SetCheck( 2);

      SET_TRI_STATE_TO_CTRL( m_bHasDirectionalShadows, BPOF_HASDIRECTIONALLIGHT);
      SET_TRI_STATE_TO_CTRL( m_bHasPreciseShadows, BPOF_ACCURATESHADOWS);      
      SET_TRI_STATE_TO_CTRL( m_bHasDirectionalAmbient, BPOF_HASDIRECTIONALAMBIENT);      
      SET_TRI_STATE_TO_CTRL( m_bNoPlaneDiffusion, BPOF_NOPLANEDIFFUSION);
      SET_TRI_STATE_TO_CTRL( m_bDarkCorners, BPOF_DARKCORNERS);
      SET_TRI_STATE_TO_CTRL( m_bDynamicLightsOnly, BPOF_DYNAMICLIGHTSONLY);
      SET_TRI_STATE_TO_CTRL( m_IsLightBeamPassable, BPOF_DOESNOTCASTSHADOW);
      SET_TRI_STATE_TO_CTRL( m_bDontReceiveShadows, BPOF_DOESNOTRECEIVESHADOW);
      SET_TRI_STATE_TO_CTRL( m_bNoDynamicLights, BPOF_NODYNAMICLIGHTS);
      SET_TRI_STATE_TO_CTRL( m_NoShadow, BPOF_FULLBRIGHT);

      if( bSameIllumination)
      {
        for( INDEX iIllumination=0; iIllumination<m_ComboIllumination.GetCount(); iIllumination++)
        {
          INDEX iIlluminationData = m_ComboIllumination.GetItemData( iIllumination);
          if( iIlluminationData==ubFirstIllumination)
          {
            m_ComboIllumination.SetCurSel( iIllumination);
            break;
          }
        }        
      }
      else m_ComboIllumination.SetCurSel(-1);
      if( bSameShadowClusterSize) m_ctrlComboClusterSize.SetCurSel( sbFirstShadowClusterSize+4);
      else m_ctrlComboClusterSize.SetCurSel(-1);
      if( bSameShadowBlend) m_comboShadowBlend.SetCurSel( ubFirstShadowBlend);
      else m_comboShadowBlend.SetCurSel(-1);
      if( bSameGradient) m_ctrlComboGradient.SetCurSel( ubFirstGradient);
      else m_ctrlComboGradient.SetCurSel(-1);
    }
    // mark that page is updated
    m_udPolygonSelection.MarkUpdated();
  }

  //{{AFX_DATA_MAP(CDlgPgShadow)
	DDX_Control(pDX, IDC_GRADIENT_COMBO, m_ctrlComboGradient);
	DDX_Control(pDX, IDC_DARK_CORNERS, m_bDarkCorners);
	DDX_Control(pDX, IDC_NO_DYNAMIC_LIGHTS, m_bNoDynamicLights);
	DDX_Control(pDX, IDC_IS_RECEIVING_SHADOWS, m_bDontReceiveShadows);
	DDX_Control(pDX, IDC_DYNAMIC_LIGHTS_ONLY, m_bDynamicLightsOnly);
  DDX_Control(pDX, IDC_HAS_DIRECTIONAL_AMBIENT, m_bHasDirectionalAmbient);
	DDX_Control(pDX, IDC_HAS_PRECISE_SHADOWS, m_bHasPreciseShadows);
	DDX_Control(pDX, IDC_NO_PLANE_DIFFUSION, m_bNoPlaneDiffusion);
	DDX_Control(pDX, IDC_HAS_DIRECTIONAL_SHADOWS, m_bHasDirectionalShadows);
	DDX_Control(pDX, IDC_NO_SHADOW, m_NoShadow);
	DDX_Control(pDX, IDC_IS_LIGHT_BEAM_PASSABLLE, m_IsLightBeamPassable);
	DDX_Control(pDX, ID_SHADOW_COLOR, m_ctrlShadowColor);
	DDX_Control(pDX, IDC_SHADOW_BLEND_COMBO, m_comboShadowBlend);
	DDX_Control(pDX, IDC_CLUSTER_SIZE_COMBO, m_ctrlComboClusterSize);
	DDX_Control(pDX, IDC_ILLUMINATION_COMBO, m_ComboIllumination);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    BOOL bFindShadowLayers = FALSE;
    BOOL bOnlySelected = TRUE;

    // calculate bounding box for all polygons
    FLOATaabbox3D boxBoundingBoxPolygonSelection;
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      boxBoundingBoxPolygonSelection |= itbpo->bpo_boxBoundingBox;

      if( m_comboShadowBlend.GetCurSel()!=-1) itbpo->bpo_bppProperties.bpp_ubShadowBlend = m_comboShadowBlend.GetCurSel();
      INDEX iItem = m_ComboIllumination.GetCurSel();
      if( iItem!=CB_ERR)
      {
        INDEX iIlluminationToSet = m_ComboIllumination.GetItemData( iItem);
        if( itbpo->bpo_bppProperties.bpp_ubIlluminationType != iIlluminationToSet)
        {
          itbpo->bpo_bppProperties.bpp_ubIlluminationType = (UBYTE) iIlluminationToSet;

          itbpo->bpo_smShadowMap.Uncache();
          itbpo->DiscardShadows();
          bFindShadowLayers = TRUE;
          bOnlySelected = FALSE;
        }
      }
      INDEX iShadowClusterSize = m_ctrlComboClusterSize.GetCurSel()-4;
      if( (m_ctrlComboClusterSize.GetCurSel()!=-1) &&
          (itbpo->bpo_bppProperties.bpp_sbShadowClusterSize != iShadowClusterSize) )
      {
        itbpo->bpo_bppProperties.bpp_sbShadowClusterSize = (SBYTE) iShadowClusterSize;
        // discard all shadow layers on the polygon.
        itbpo->bpo_smShadowMap.DiscardAllLayers();
        itbpo->InitializeShadowMap();
        bFindShadowLayers = TRUE;
      }

      INDEX iGradient = m_ctrlComboGradient.GetCurSel();
      if( iGradient!=CB_ERR)
      {
        if( itbpo->bpo_bppProperties.bpp_ubGradientType != iGradient)
        {
          itbpo->bpo_bppProperties.bpp_ubGradientType = (UBYTE) iGradient;
          itbpo->bpo_smShadowMap.DiscardAllLayers();
          itbpo->InitializeShadowMap();
          bFindShadowLayers = TRUE;
        }
      }

// set polygon's flags acording witg given tri-state ctrl
#define TRI_STATE_CTRL_TO_FLAGS( ctrl, flag, buncache, bdiscardshadows, bfindshadowlayers, bonlyselected)\
  if( (ctrl.GetCheck() == 1) && !(itbpo->bpo_ulFlags & flag) ) {\
    itbpo->bpo_ulFlags |= flag;\
    if( buncache) itbpo->bpo_smShadowMap.Uncache();\
    if( bdiscardshadows) itbpo->DiscardShadows();\
    bFindShadowLayers |= bfindshadowlayers;\
    bOnlySelected &= bonlyselected ;\
  } else if( (ctrl.GetCheck() == 0) && (itbpo->bpo_ulFlags & flag) ) {\
    itbpo->bpo_ulFlags &= ~flag;\
    if( buncache) itbpo->bpo_smShadowMap.Uncache();\
    if( bdiscardshadows) itbpo->DiscardShadows();\
    bFindShadowLayers |= bfindshadowlayers;\
    bOnlySelected &= bonlyselected ;\
  }
      TRI_STATE_CTRL_TO_FLAGS( m_bNoPlaneDiffusion, BPOF_NOPLANEDIFFUSION, TRUE, FALSE, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_bDarkCorners, BPOF_DARKCORNERS, TRUE, TRUE, TRUE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_bDynamicLightsOnly, BPOF_DYNAMICLIGHTSONLY, TRUE, FALSE, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_bHasDirectionalShadows, BPOF_HASDIRECTIONALLIGHT, TRUE, TRUE, TRUE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_bHasPreciseShadows, BPOF_ACCURATESHADOWS, TRUE, TRUE, TRUE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_bHasDirectionalAmbient, BPOF_HASDIRECTIONALAMBIENT, TRUE, FALSE, FALSE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_IsLightBeamPassable, BPOF_DOESNOTCASTSHADOW, TRUE, TRUE, TRUE, FALSE);
      TRI_STATE_CTRL_TO_FLAGS( m_bDontReceiveShadows, BPOF_DOESNOTRECEIVESHADOW, TRUE, TRUE, TRUE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_bNoDynamicLights, BPOF_NODYNAMICLIGHTS, FALSE, FALSE, FALSE, TRUE);
      TRI_STATE_CTRL_TO_FLAGS( m_NoShadow, BPOF_FULLBRIGHT, TRUE, TRUE, TRUE, TRUE);

      if( m_ctrlShadowColor.IsColorValid()) {
        itbpo->bpo_colShadow = m_ctrlShadowColor.GetColor();
      }
    }
    // if we should find shadow layers
    if( bFindShadowLayers)
    {
      pDoc->m_woWorld.FindShadowLayers( boxBoundingBoxPolygonSelection, bOnlySelected);
    }
    // mark that document is changed
    theApp.GetDocument()->SetModifiedFlag( TRUE);
    // redraw to show changes
    pDoc->UpdateAllViews( NULL);
  }
}


BEGIN_MESSAGE_MAP(CDlgPgShadow, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgShadow)
	ON_CBN_SELCHANGE(IDC_CLUSTER_SIZE_COMBO, OnSelchangeShadowClusterSizeCombo)
	ON_CBN_SELCHANGE(IDC_ILLUMINATION_COMBO, OnSelchangeIlluminationCombo)
	ON_CBN_DROPDOWN(IDC_ILLUMINATION_COMBO, OnDropdownIlluminationCombo)
	ON_CBN_SELCHANGE(IDC_SHADOW_BLEND_COMBO, OnSelchangeShadowBlendCombo)
	ON_CBN_DROPDOWN(IDC_SHADOW_BLEND_COMBO, OnDropdownShadowBlendCombo)
	ON_CBN_DROPDOWN(IDC_CLUSTER_SIZE_COMBO, OnDropdownClusterSizeCombo)
	ON_WM_CONTEXTMENU()
	ON_CBN_DROPDOWN(IDC_GRADIENT_COMBO, OnDropdownGradientCombo)
	ON_CBN_SELCHANGE(IDC_GRADIENT_COMBO, OnSelchangeGradientCombo)
  ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnToolTipNotify )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgShadow message handlers
void CDlgPgShadow::InitComboBoxes(void)
{
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  CTString strGradientName;
  CTString strIlluminationName;

	m_ComboIllumination.ResetContent();
	m_ctrlComboClusterSize.ResetContent();
	m_comboShadowBlend.ResetContent();
	m_ctrlComboGradient.ResetContent();
  // add all available illuminations
  for(INDEX iIllumination=0; iIllumination<MAX_UBYTE; iIllumination++)
  {
    strIlluminationName = pDoc->m_woWorld.wo_aitIlluminationTypes[iIllumination].it_strName;
    if(strIlluminationName == "") break;
    INDEX iAddedAs = m_ComboIllumination.AddString( CString(strIlluminationName));
    m_ComboIllumination.SetItemData( iAddedAs, (DWORD_PTR) iIllumination);
  }
  for(INDEX iBlend=0; iBlend<256; iBlend++)
  {
    CTString strBlendName = pDoc->m_woWorld.wo_atbTextureBlendings[iBlend].tb_strName;
    if( strBlendName != CTString("") ) m_comboShadowBlend.AddString( CString(strBlendName));
  }    

  // none must exist
  m_ctrlComboGradient.AddString( L"None");

  // add gradients
  if( pDoc->m_selPolygonSelection.Count() != 0)
  {
    // obtain first polygon's brush
    CBrush3D *pbrBrush = NULL;
    pDoc->m_selPolygonSelection.Lock();
    if( !pDoc->m_selPolygonSelection.IsMember( pDoc->m_pbpoLastCentered))
    {
      pbrBrush = pDoc->m_selPolygonSelection[0].bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush;
    }
    pDoc->m_selPolygonSelection.Unlock();

    BOOL bEnableGradient = TRUE;
    // for each of the selected polygons
    FOREACHINDYNAMICCONTAINER(pDoc->m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      // disable gradient combo box if all polygons are not from same brush
      if( pbrBrush != itbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush)
      {
        bEnableGradient = FALSE;
        break;
      }
    }
  
    // if gradient combo is enabled
    if( bEnableGradient)
    {
      // add gradients
      for(INDEX iGradient=0; iGradient<MAX_UBYTE; iGradient++)
      {
        CTString strGradientName = pbrBrush->br_penEntity->GetGradientName( iGradient);
        if( strGradientName == "") break;
        m_ctrlComboGradient.AddString( CString(strGradientName));
      }
    }
  }

  // add all available shadow cluster sizes
  m_ctrlComboClusterSize.AddString( L"3.125 cm");
  m_ctrlComboClusterSize.AddString( L"6.25 cm");
  m_ctrlComboClusterSize.AddString( L"12.5 cm");
  m_ctrlComboClusterSize.AddString( L"25 cm");
  m_ctrlComboClusterSize.AddString( L"0.5 m");
  m_ctrlComboClusterSize.AddString( L"1 m");
  m_ctrlComboClusterSize.AddString( L"2 m");
  m_ctrlComboClusterSize.AddString( L"4 m");
  m_ctrlComboClusterSize.AddString( L"8 m");
  m_ctrlComboClusterSize.AddString( L"16 m");
  m_ctrlComboClusterSize.AddString( L"32 m");
  m_ctrlComboClusterSize.AddString( L"64 m");
  m_ctrlComboClusterSize.AddString( L"128 m");
  m_ctrlComboClusterSize.AddString( L"256 m");
  m_ctrlComboClusterSize.AddString( L"512 m");
  m_ctrlComboClusterSize.AddString( L"1024 m");
}

BOOL CDlgPgShadow::OnIdle(LONG lCount)
{
  CWorldEditorDoc* pDoc = theApp.GetDocument();
  if( (pDoc == NULL) || !IsWindow(m_hWnd) )
  {
    return TRUE;
  }
  // if selections have been changed (they are not up to date)
  if( !pDoc->m_chSelections.IsUpToDate( m_udPolygonSelection) )
  {
    // update dialog data
    UpdateData( FALSE);
  }
  return TRUE;
}

BOOL CDlgPgShadow::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
  {
    // move data from page to polygon
    UpdateData( TRUE);
    // the message is handled
    return TRUE;
  }
	return CPropertyPage::PreTranslateMessage(pMsg);
}

BOOL CDlgPgShadow::OnInitDialog() 
{
  EnableToolTips( TRUE);
	CPropertyPage::OnInitDialog();
  if( IsWindow( m_ComboIllumination.m_hWnd))
  {
    InitComboBoxes();
  }
  m_ctrlShadowColor.SetDialogPtr( this);
	m_bNoPlaneDiffusion.SetDialogPtr( this);
	m_bDarkCorners.SetDialogPtr( this);
	m_bDynamicLightsOnly.SetDialogPtr( this);
	m_bHasPreciseShadows.SetDialogPtr( this);
	m_bHasDirectionalAmbient.SetDialogPtr( this);
	m_bHasDirectionalShadows.SetDialogPtr( this);
	m_NoShadow.SetDialogPtr( this);
	m_IsLightBeamPassable.SetDialogPtr( this);
  m_bDontReceiveShadows.SetDialogPtr( this);
  m_bNoDynamicLights.SetDialogPtr( this);
  return TRUE;
}

void CDlgPgShadow::OnSelchangeShadowClusterSizeCombo() 
{
  // update dialog data (to reflect data change)
	UpdateData( TRUE);
}

void CDlgPgShadow::OnSelchangeIlluminationCombo() 
{
	UpdateData( TRUE);
}

void CDlgPgShadow::OnSelchangeShadowBlendCombo() 
{
	UpdateData( TRUE);
}

void CDlgPgShadow::OnSelchangeGradientCombo() 
{
	UpdateData( TRUE);
}

void CDlgPgShadow::OnDropdownIlluminationCombo() 
{
	m_ComboIllumination.SetDroppedWidth( 256);
}

void CDlgPgShadow::OnDropdownShadowBlendCombo() 
{
	m_comboShadowBlend.SetDroppedWidth( 256);
}


void CDlgPgShadow::OnDropdownClusterSizeCombo() 
{
	m_ctrlComboClusterSize.SetDroppedWidth( 256);
}

void CDlgPgShadow::OnDropdownGradientCombo() 
{
  m_ctrlComboGradient.SetDroppedWidth( 256);
}

void CDlgPgShadow::OnContextMenu(CWnd* pWnd, CPoint point) 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  
  CMenu menu;
  if( pDoc->GetEditingMode() == POLYGON_MODE)
  {
    if( menu.LoadMenu(IDR_INFO_POLYGON_POPUP))
    {
		  CMenu* pPopup = menu.GetSubMenu(0);
      if( pDoc->m_selPolygonSelection.Count() != 1)
      {
        menu.EnableMenuItem(ID_SET_AS_DEFAULT, MF_DISABLED|MF_GRAYED);
      }

      pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								   point.x, point.y, this);
    }
  }
}

BOOL CDlgPgShadow::OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
  TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
  DWORD_PTR nID =pNMHDR->idFrom;
  if (pTTT->uFlags & TTF_IDISHWND)
  {
    // idFrom is actually the HWND of the tool
    nID = ::GetDlgCtrlID((HWND)nID);
    if(nID)
    {
      pTTT->lpszText = MAKEINTRESOURCE(nID);
      pTTT->hinst = AfxGetResourceHandle();
      return(TRUE);
    }
  }
  return(FALSE);
}

