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

// DlgInfoPgGlobal.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgGlobal property page

IMPLEMENT_DYNCREATE(CDlgInfoPgGlobal, CPropertyPage)

CDlgInfoPgGlobal::CDlgInfoPgGlobal() : CPropertyPage(CDlgInfoPgGlobal::IDD)
{
	//{{AFX_DATA_INIT(CDlgInfoPgGlobal)
  m_strFlat = _T("");
	m_strTextureSize = _T("");
	m_strWndSize = _T("");
	m_strReflections = _T("");
	m_strDifuse = _T("");
	m_strHighQuality = _T("");
	m_strSpecular = _T("");
	m_strMaxShadow = _T("");
	m_strBump = _T("");
	//}}AFX_DATA_INIT

  theApp.m_pPgInfoGlobal = this;
  m_colorDiffuse.m_pwndParentDialog = this;
  m_colorBump.m_pwndParentDialog = this;
  m_colorSpecular.m_pwndParentDialog = this;
  m_colorReflection.m_pwndParentDialog = this;
}

CDlgInfoPgGlobal::~CDlgInfoPgGlobal()
{
}

void CDlgInfoPgGlobal::SetGlobalPageFromView(CModelerView* pModelerView)
{
  CModelerDoc *pDoc = pModelerView->GetDocument();
  CModelInfo miModelInfo;
  char value[20];
  ASSERT( pModelerView != NULL);
  
  /* Get current model's info */
  pModelerView->m_ModelObject.GetModelInfo( miModelInfo);
  INDEX iMipModel = pModelerView->m_ModelObject.GetMipModel( pModelerView->m_fCurrentMipFactor);
  
  /* Texture size */
  sprintf( value, "%.2f x %.2f m", METERS_MEX( miModelInfo.mi_Width),
                                   METERS_MEX( miModelInfo.mi_Height) );
  m_strTextureSize = value;

  /* Flat (YES/NO) */
  if( miModelInfo.mi_Flags & MF_FACE_FORWARD) m_strFlat = "Yes";
  else                                        m_strFlat = "No";

  /* High quality (YES/NO) */
  if( miModelInfo.mi_Flags & MF_COMPRESSED_16BIT) m_strHighQuality = "Yes";
  else                                            m_strHighQuality = "No";

  /* Set max shadow */
  sprintf( value, "%d", pDoc->m_emEditModel.edm_md.md_ShadowQuality);
  m_strMaxShadow = value;

  /* Set window size */
  sprintf( value, "%d x %d", pModelerView->m_pDrawPort->GetWidth(),
                             pModelerView->m_pDrawPort->GetHeight());
  m_strWndSize = value;

  m_colorDiffuse.SetColor(pDoc->m_emEditModel.edm_md.md_colDiffuse);
  m_colorReflection.SetColor(pDoc->m_emEditModel.edm_md.md_colReflections);
  m_colorSpecular.SetColor(pDoc->m_emEditModel.edm_md.md_colSpecular);
  m_colorBump.SetColor(pDoc->m_emEditModel.edm_md.md_colBump);

  m_strDifuse = pModelerView->m_ModelObject.mo_toTexture.GetName();
  m_strReflections = pModelerView->m_ModelObject.mo_toReflection.GetName();
  m_strSpecular = pModelerView->m_ModelObject.mo_toSpecular.GetName();
  m_strBump = pModelerView->m_ModelObject.mo_toBump.GetName();

  if( m_strDifuse == "") m_strDifuse = "<none>";
  if( m_strReflections == "") m_strReflections = "<none>";
  if( m_strSpecular == "") m_strSpecular = "<none>";
  if( m_strBump == "") m_strBump = "<none>";
}

void CDlgInfoPgGlobal::SetViewFromGlobalPage(CModelerView* pModelerView)
{
  CModelerDoc *pDoc = pModelerView->GetDocument();
  pDoc->m_emEditModel.edm_md.md_colDiffuse = m_colorDiffuse.GetColor();
  pDoc->m_emEditModel.edm_md.md_colReflections = m_colorReflection.GetColor();
  pDoc->m_emEditModel.edm_md.md_colSpecular = m_colorSpecular.GetColor();
  pDoc->m_emEditModel.edm_md.md_colBump = m_colorBump.GetColor();
  pDoc->SetModifiedFlag( TRUE);
}

void CDlgInfoPgGlobal::DoDataExchange(CDataExchange* pDX)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  if(pModelerView == NULL) return;

  //if(!::IsWindow( m_colorSpecular.m_hWnd)) return;
  if( !pDX->m_bSaveAndValidate)  // if zero, model sets property sheet's data
  {
    SetGlobalPageFromView( pModelerView);
    // mark that the values have been updated to reflect the state of the view
    m_udAllValues.MarkUpdated();
  }

  CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgInfoPgGlobal)
	DDX_Control(pDX, IDC_DIFUSE_COLOR, m_colorDiffuse);
	DDX_Control(pDX, IDC_BUMP_COLOR, m_colorBump);
	DDX_Control(pDX, IDC_SPECULAR_COLOR, m_colorSpecular);
	DDX_Control(pDX, IDC_REFLECTION_COLOR, m_colorReflection);
	DDX_Text(pDX, IDC_FLAT, m_strFlat);
	DDX_Text(pDX, IDC_TEX_SIZE, m_strTextureSize);
	DDX_Text(pDX, IDC_WINDOW_SIZE, m_strWndSize);
	DDX_Text(pDX, IDC_REFLECTIONS, m_strReflections);
	DDX_Text(pDX, IDC_DIFUSE, m_strDifuse);
	DDX_Text(pDX, IDC_HIGH_QUALITY, m_strHighQuality);
	DDX_Text(pDX, IDC_SPECULAR, m_strSpecular);
	DDX_Text(pDX, IDC_MAX_SHADOW, m_strMaxShadow);
	DDX_Text(pDX, IDC_BUMP, m_strBump);
	//}}AFX_DATA_MAP

  if( pDX->m_bSaveAndValidate) {
    SetViewFromGlobalPage( pModelerView);
  }
}

BEGIN_MESSAGE_MAP(CDlgInfoPgGlobal, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgInfoPgGlobal)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgGlobal message handlers

BOOL CDlgInfoPgGlobal::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveMappingNormalView();
  ASSERT(pModelerView != NULL);

  if (!theApp.m_chGlobal.IsUpToDate(m_udAllValues) ||
      !pModelerView->m_ModelObject.IsUpToDate(m_udAllValues)) {
    UpdateData(FALSE);
  }
  return TRUE;
}

BOOL CDlgInfoPgGlobal::OnSetActive() 
{
  // mark that all values should be updated
	return CPropertyPage::OnSetActive();
}

