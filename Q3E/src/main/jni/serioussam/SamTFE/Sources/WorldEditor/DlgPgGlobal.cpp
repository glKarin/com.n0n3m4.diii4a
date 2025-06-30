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

// DlgPgGlobal.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgGlobal.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgGlobal property page

IMPLEMENT_DYNCREATE(CDlgPgGlobal, CPropertyPage)

CDlgPgGlobal::CDlgPgGlobal() : CPropertyPage(CDlgPgGlobal::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgGlobal)
	m_strTextureInfo = _T("");
	m_strSelectedEntitiesCt = _T("");
	m_strSelectedPolygonsCt = _T("");
	m_strSelectedSectorsCt = _T("");
	//}}AFX_DATA_INIT
}

CDlgPgGlobal::~CDlgPgGlobal()
{
}

void CDlgPgGlobal::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

  // get document ptr
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();

  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    // if there isn't any document available
    if( pDoc == NULL)
    {
      // write default messages
      m_strSelectedEntitiesCt = "none";
      m_strSelectedPolygonsCt = "none";
      m_strSelectedSectorsCt = "none";  
    }
    else
    {
      // type selection container counts again
      char strSelectionCt[ 16];
      // set string saying how many entities are curently selected
      sprintf( strSelectionCt, "%d", pDoc->m_selEntitySelection.Count());
      m_strSelectedEntitiesCt = strSelectionCt;
      // set string saying how many polygons are curently selected
      sprintf( strSelectionCt, "%d", pDoc->m_selPolygonSelection.Count());
      m_strSelectedPolygonsCt = strSelectionCt;
      // set string saying how many sectors are curently selected
      sprintf( strSelectionCt, "%d", pDoc->m_selSectorSelection.Count());
      m_strSelectedSectorsCt = strSelectionCt;
      // mark that selection container counts are now updated
      m_udSelectionCounts.MarkUpdated();
    }

    if( theApp.m_ptdActiveTexture != NULL)
    {
      m_strTextureInfo = 
        (CTString&)theApp.m_ptdActiveTexture->GetName()+" "+
        theApp.m_ptdActiveTexture->GetDescription();
    }
  }

  CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPgGlobal)
	DDX_Text(pDX, IDC_TEXTURE_INFO, m_strTextureInfo);
	DDX_Text(pDX, IDC_SELECTED_ENTITIES, m_strSelectedEntitiesCt);
	DDX_Text(pDX, IDC_SELECTED_POLYGONS, m_strSelectedPolygonsCt);
	DDX_Text(pDX, IDC_SELECTED_SECTORS, m_strSelectedSectorsCt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPgGlobal, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgGlobal)
	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgGlobal message handlers

BOOL CDlgPgGlobal::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
  
  PIX pixLeft = 10;
  PIX pixTop = 10;

  // create window for active default primitive texture
  m_wndActiveTexture.Create( NULL, NULL, WS_BORDER|WS_VISIBLE,
    CRect( pixLeft, pixTop, pixLeft+96, pixTop+96), 
    this, IDW_ACTIVE_TEXTURE);

  DragAcceptFiles();
  return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDlgPgGlobal::OnIdle(LONG lCount)
{
  // get active document 
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  // if there isn't any document available
  if( pDoc == NULL)
  {
    // update data (write default messages)
    UpdateData( FALSE);
  }
  else
  {
    // if selections have been changed (they are not up to date)
    if( !pDoc->m_chSelections.IsUpToDate( m_udSelectionCounts))
    {
      // update dialog data
      UpdateData( FALSE);
    }
  }
  return TRUE;
}

void CDlgPgGlobal::OnDropFiles(HDROP hDropInfo) 
{
  INDEX iNoOfFiles = DragQueryFile( hDropInfo, 0xFFFFFFFF, NULL, 0);
  
  if( iNoOfFiles != 1)
  {
    AfxMessageBox( L"You can drop only one file at a time.");
    return;
  }

	// buffer for dropped file name
  char chrFile[ 256];
  // place dropped file name into buffer
  DragQueryFileA( hDropInfo, 0, chrFile, 256);
  // create file name from buffer
  CTFileName fnDropped = CTString(chrFile);
  // if it is not texture, report error
  if( fnDropped.FileExt() != ".tex" )
  {
    AfxMessageBox( L"You can only drop textures here.");
    return;
  }
  theApp.SetNewActiveTexture( fnDropped);
  // paste new active texture over polygon selection
  theApp.TexturizeSelection();
}
