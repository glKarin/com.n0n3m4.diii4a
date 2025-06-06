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

// DlgMarkLinkedSurfaces.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgMarkLinkedSurfaces dialog


CDlgMarkLinkedSurfaces::CDlgMarkLinkedSurfaces( CWnd* pParent /*=NULL*/)
	: CDialog(CDlgMarkLinkedSurfaces::IDD, pParent)
{
  //{{AFX_DATA_INIT(CDlgMarkLinkedSurfaces)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
  m_listSurfaces.m_pdlgParentDialog = this;
}


void CDlgMarkLinkedSurfaces::DoDataExchange(CDataExchange* pDX)
{
  CModelerDoc* pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;

  CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgMarkLinkedSurfaces)
	DDX_Control(pDX, IDC_SURFACE_LIST, m_listSurfaces);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    // for all surfaces added to list of surfaces
    for( INDEX iEntry=0; iEntry<m_listSurfaces.GetCount(); iEntry++)
    {
      MappingSurface *pms = (MappingSurface*)m_listSurfaces.GetItemData( iEntry);
      if( m_listSurfaces.GetCheck(iEntry)==1) pms->ms_ulRenderingFlags |=  SRF_SELECTED;
      else                                    pms->ms_ulRenderingFlags &= ~SRF_SELECTED;
    }
    theApp.m_chGlobal.MarkChanged();
  }
}


BEGIN_MESSAGE_MAP(CDlgMarkLinkedSurfaces, CDialog)
	//{{AFX_MSG_MAP(CDlgMarkLinkedSurfaces)
	ON_BN_CLICKED(ID_CLEAR_SELECTION, OnClearSelection)
	ON_BN_CLICKED(ID_SELECT_ALL, OnSelectAll)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgMarkLinkedSurfaces message handlers

BOOL CDlgMarkLinkedSurfaces::OnInitDialog() 
{
	CDialog::OnInitDialog();
  
  CModelerDoc* pDoc = theApp.GetDocument();
  // get cont of surfaces
  INDEX ctSurfaces = pDoc->GetCountOfSelectedSurfaces();
  
  ModelMipInfo &mmi = pDoc->m_emEditModel.edm_md.md_MipInfos[ pDoc->m_iCurrentMip];
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
  {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    CTString strListEntry;
    strListEntry.PrintF("%.02d %s (%d)", iSurface, ms.ms_Name, ms.ms_aiPolygons.Count());
    int iAddedAs = m_listSurfaces.AddString( CString(strListEntry));
    m_listSurfaces.SetItemData( iAddedAs, (DWORD_PTR) &ms);
    if( ms.ms_ulRenderingFlags&SRF_SELECTED) m_listSurfaces.SetCheck( iAddedAs, 1);
    else m_listSurfaces.SetCheck( iAddedAs, 0);
  }
  return TRUE;
}

void CDlgMarkLinkedSurfaces::OnClearSelection() 
{
  // for all surfaces added to list of surfaces
  for( INDEX iEntry=0; iEntry<m_listSurfaces.GetCount(); iEntry++)
  {
    m_listSurfaces.SetCheck( iEntry, 0);
  }
}

void CDlgMarkLinkedSurfaces::OnSelectAll() 
{
  // for all surfaces added to list of surfaces
  for( INDEX iEntry=0; iEntry<m_listSurfaces.GetCount(); iEntry++)
  {
    m_listSurfaces.SetCheck( iEntry, 1);
  }
}
