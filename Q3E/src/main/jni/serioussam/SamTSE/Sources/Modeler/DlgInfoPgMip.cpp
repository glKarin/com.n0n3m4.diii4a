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

// DlgInfoPgMip.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgMip property page

IMPLEMENT_DYNCREATE(CDlgInfoPgMip, CPropertyPage)

CDlgInfoPgMip::CDlgInfoPgMip() : CPropertyPage(CDlgInfoPgMip::IDD)
{
	//{{AFX_DATA_INIT(CDlgInfoPgMip)
	m_strCurrentMipModel = _T("");
	m_strModelDistance = _T("");
	m_strCurrentMipFactor = _T("");
	m_strModelMipSwitchFactor = _T("");
	m_strNoOfPolygons = _T("");
	m_strNoOfVertices = _T("");
	m_strNoOfTriangles = _T("");
	m_bHasPatches = FALSE;
	m_bHasAttachedModels = FALSE;
	//}}AFX_DATA_INIT
  theApp.m_pPgInfoMip = this;
}

void CDlgInfoPgMip::SetMipPageFromView(CModelerView* pModelerView)
{
  CModelInfo miModelInfo;
  char value[20];
  ASSERT( pModelerView != NULL);
  CModelerDoc *pDoc = pModelerView->GetDocument();

  /* Get current model's info */
  pModelerView->m_ModelObject.GetModelInfo( miModelInfo);
  INDEX iMipModel = pModelerView->m_ModelObject.GetMipModel( pModelerView->m_fCurrentMipFactor);

  if( !pModelerView->m_ModelObject.IsModelVisible(pModelerView->m_fCurrentMipFactor) )
  {
    m_strCurrentMipModel = "Not visible";
    m_strNoOfTriangles = "0";
    m_strNoOfVertices = "0";
    m_strNoOfPolygons = "0";
  }
  else
  {
    sprintf( value, "%d/%d", iMipModel, miModelInfo.mi_MipCt);
    m_strCurrentMipModel = value;
    sprintf( value, "%d", miModelInfo.mi_MipInfos[ iMipModel].mi_TrianglesCt);
    m_strNoOfTriangles = value;
    sprintf( value, "%d", miModelInfo.mi_MipInfos[ iMipModel].mi_VerticesCt);
    m_strNoOfVertices = value;
    sprintf( value, "%d", miModelInfo.mi_MipInfos[ iMipModel].mi_PolygonsCt);
    m_strNoOfPolygons = value;
  }
  sprintf( value, "%.1f m", pModelerView->GetModelToViewerDistance());  
  m_strModelDistance = value;
  sprintf( value, "%.3f", pModelerView->m_fCurrentMipFactor);
  m_strCurrentMipFactor = value;
  sprintf( value, "%.3f", pDoc->m_emEditModel.edm_md.md_MipSwitchFactors[iMipModel]);
  m_strModelMipSwitchFactor = value;

  ModelMipInfo *pMMIFirst = &pDoc->m_emEditModel.edm_md.md_MipInfos[ iMipModel];
  // if patches are visible for current mip model
  if( pMMIFirst->mmpi_ulFlags & MM_PATCHES_VISIBLE)
  {
    m_bHasPatches = TRUE;
  }
  else
  {
    m_bHasPatches = FALSE;
  }

  // if patches are visible for current mip model
  if( pMMIFirst->mmpi_ulFlags & MM_ATTACHED_MODELS_VISIBLE)
  {
    m_bHasAttachedModels = TRUE;
  }
  else
  {
    m_bHasAttachedModels = FALSE;
  }
}

void CDlgInfoPgMip::SetViewFromMipPage(CModelerView* pModelerView)
{
}

CDlgInfoPgMip::~CDlgInfoPgMip()
{
}

void CDlgInfoPgMip::DoDataExchange(CDataExchange* pDX)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if(pModelerView == NULL) return;

  if( !pDX->m_bSaveAndValidate)  // if zero, model sets property sheet's data
  {
    SetMipPageFromView( pModelerView);
    // mark that the values have been updated to reflect the state of the view
    m_udAllValues.MarkUpdated();
  }

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfoPgMip)
	DDX_Text(pDX, IDC_CURRENT_MIP_MODEL, m_strCurrentMipModel);
	DDX_Text(pDX, IDC_MODEL_DISTANCE, m_strModelDistance);
	DDX_Text(pDX, IDC_CURRENT_FACTOR, m_strCurrentMipFactor);
	DDX_Text(pDX, IDC_MODEL_SWITCH_FACTOR, m_strModelMipSwitchFactor);
	DDX_Text(pDX, IDC_NO_OF_POLYGONS, m_strNoOfPolygons);
	DDX_Text(pDX, IDC_NO_OF_VERTICES, m_strNoOfVertices);
	DDX_Text(pDX, IDC_NO_OF_TRIANGLES, m_strNoOfTriangles);
	DDX_Check(pDX, IDC_HAS_PATCHES, m_bHasPatches);
	DDX_Check(pDX, IDC_HAS_ATTACHED_MODELS, m_bHasAttachedModels);
	//}}AFX_DATA_MAP

  if( pDX->m_bSaveAndValidate)
  {
    SetViewFromMipPage( pModelerView);
  }
}


BEGIN_MESSAGE_MAP(CDlgInfoPgMip, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgInfoPgMip)
	ON_BN_CLICKED(IDC_HAS_PATCHES, OnHasPatches)
	ON_BN_CLICKED(IDC_HAS_ATTACHED_MODELS, OnHasAttachedModels)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgMip message handlers

BOOL CDlgInfoPgMip::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();

  ASSERT(pModelerView != NULL);

  if (!theApp.m_chGlobal.IsUpToDate(m_udAllValues) ||
      !pModelerView->m_ModelObject.IsUpToDate(m_udAllValues) ||
      !theApp.m_chPlacement.IsUpToDate(m_udAllValues)) {
    UpdateData(FALSE);
  }

  // refresh info frame size
  ((CMainFrame *)( theApp.m_pMainWnd))->m_pInfoFrame->SetSizes();
  return TRUE;
}

void CDlgInfoPgMip::ToggleMipFlag( ULONG ulFlag)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  ASSERT(pModelerView != NULL);
  CModelerDoc *pDoc = pModelerView->GetDocument();
  ASSERT(pDoc != NULL);

  ModelMipInfo *pMMIFirst = &pDoc->m_emEditModel.edm_md.md_MipInfos[ pDoc->m_iCurrentMip];
  BOOL bSetting = (pMMIFirst->mmpi_ulFlags & ulFlag) == 0;
  // if setting, set just for this mip model
  if( bSetting)
  {
    pMMIFirst->mmpi_ulFlags |= ulFlag;
  }
  // if clearing, clear for all further mip models
  else
  {
    for( INDEX iMip=pDoc->m_iCurrentMip; iMip<pDoc->m_emEditModel.edm_md.md_MipCt; iMip++)
    {
      // get requested mip model
      ModelMipInfo *pMMI = &pDoc->m_emEditModel.edm_md.md_MipInfos[ iMip];
      // if setting, set just for this mip model
      pMMI->mmpi_ulFlags &= ~ulFlag;
    }
  }
  
  // for patcehs
  if( ulFlag == MM_PATCHES_VISIBLE)
  {
    // reaclculate patch-polygon connections
    pDoc->m_emEditModel.CalculatePatchesPerPolygon();
  }
  
  pDoc->UpdateAllViews(NULL);
  UpdateData( FALSE);
}

void CDlgInfoPgMip::OnHasPatches() 
{
  ToggleMipFlag( MM_PATCHES_VISIBLE);
}

void CDlgInfoPgMip::OnHasAttachedModels() 
{
  ToggleMipFlag( MM_ATTACHED_MODELS_VISIBLE);
}
