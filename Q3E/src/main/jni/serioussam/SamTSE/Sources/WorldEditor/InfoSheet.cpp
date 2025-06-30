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

// InfoSheet.cpp : implementation file
//

#include "stdafx.h"
#include "InfoSheet.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoSheet
#define CALLACTIVEPAGE(function, parameter)         \
  if( pgActivPage == &m_PgGlobal)                   \
    m_PgGlobal.function( parameter);                \
  if( pgActivPage == &m_PgTerrain)                  \
    m_PgTerrain.function( parameter);               \
  if( pgActivPage == &m_PgPosition)                 \
    m_PgPosition.function( parameter);              \
  if( pgActivPage == &m_PgRenderingStatistics)      \
    m_PgRenderingStatistics.function( parameter);   \
  if( pgActivPage == &m_PgPolygon)                  \
    m_PgPolygon.function( parameter);               \
  if( pgActivPage == &m_PgShadow)                   \
    m_PgShadow.function( parameter);                \
  if( pgActivPage == &m_PgSector)                   \
    m_PgSector.function( parameter);                \
  if( pgActivPage == &m_PgTexture)                  \
    m_PgTexture.function( parameter);               \
  if( pgActivPage == &m_PgPrimitive)                \
    m_PgPrimitive.function( parameter);


IMPLEMENT_DYNAMIC(CInfoSheet, CPropertySheet)

CInfoSheet::CInfoSheet(CWnd* pWndParent)
	: CPropertySheet(AFX_IDS_APP_TITLE, pWndParent)
{
  // Add all pages so frame could get bounding sizes of all of them
  AddPage( &m_PgGlobal);
  AddPage( &m_PgTerrain);
  AddPage( &m_PgPosition);
  AddPage( &m_PgPrimitive);
  //AddPage( &m_PgRenderingStatistics);
  AddPage( &m_PgPolygon);
  AddPage( &m_PgShadow);
  AddPage( &m_PgSector);
  AddPage( &m_PgTexture);
  SoftSetActivePage(0);
  // set mode that will be discarded on first OnIdle()
  m_ModeID = INFO_MODE_ALL;
}

CInfoSheet::~CInfoSheet()
{
}

// don't take focus
void CInfoSheet::SoftSetActivePage( INDEX iActivePage)
{
  // get active view 
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
  SetActivePage( iActivePage);
  if( pWorldEditorView != NULL)
  {
//    pWorldEditorView->SetFocus();
  }
}

BEGIN_MESSAGE_MAP(CInfoSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CInfoSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInfoSheet message handlers

void CInfoSheet::DoDataExchange(CDataExchange* pDX) 
{
  CPropertyPage *pgActivPage = GetActivePage();
  CALLACTIVEPAGE(UpdateData, pDX->m_bSaveAndValidate);
	
  CPropertySheet::DoDataExchange(pDX);
}

void CInfoSheet::DeleteAllPages()
{
  // disable data exchange
  theApp.m_bDisableDataExchange = TRUE;
  INDEX iPagesCt, i;
  iPagesCt = GetPageCount();
  for( i=0; i<iPagesCt; i++)
    RemovePage( 0);
  // enable data exchange
  theApp.m_bDisableDataExchange = FALSE;
}

void CInfoSheet::SetInfoModeGlobal(void)
{
  m_ModeID = INFO_MODE_GLOBAL;
  DeleteAllPages();
  AddPage( &m_PgGlobal);
  //AddPage( &m_PgRenderingStatistics);
  SoftSetActivePage(0);
}

void CInfoSheet::SetInfoModePosition(void)
{
  m_ModeID = INFO_MODE_POSITION;
  DeleteAllPages();
  AddPage( &m_PgGlobal);
  //AddPage( &m_PgRenderingStatistics);
  AddPage( &m_PgPosition);
  SoftSetActivePage(1);
}

void CInfoSheet::SetInfoModePrimitive(void)
{
  m_ModeID = INFO_MODE_PRIMITIVE;
  DeleteAllPages();
  AddPage( &m_PgGlobal);
  //AddPage( &m_PgRenderingStatistics);
  AddPage( &m_PgPosition);
  AddPage( &m_PgPrimitive);
  SoftSetActivePage(2);
}

INDEX _iLastActivePgInPolygonMode = 1;
void CInfoSheet::SetInfoModePolygon(void)
{
  m_ModeID = INFO_MODE_POLYGON;
  DeleteAllPages();
  AddPage( &m_PgGlobal);
  //AddPage( &m_PgRenderingStatistics);
  AddPage( &m_PgPolygon);
  AddPage( &m_PgShadow);
  AddPage( &m_PgTexture);
  SoftSetActivePage( _iLastActivePgInPolygonMode);
}

void CInfoSheet::SetInfoModeSector(void)
{
  m_ModeID = INFO_MODE_SECTOR;
  DeleteAllPages();
  AddPage( &m_PgGlobal);
  //AddPage( &m_PgRenderingStatistics);
  AddPage( &m_PgSector);
  SoftSetActivePage(1);
}

void CInfoSheet::SetInfoModeTerrain(void)
{
  m_ModeID = INFO_MODE_TERRAIN;
  DeleteAllPages();
  AddPage( &m_PgGlobal);
  AddPage( &m_PgTerrain);
  SoftSetActivePage(1);
}

BOOL CInfoSheet::OnIdle(LONG lCount)
{
  // get active view 
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
  // get active document 
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  
  // if we don't have view
  if( pDoc == NULL)
  {
    // force info mode: INFO_MODE_GLOBAL
    if( m_ModeID != INFO_MODE_GLOBAL)
    {
      SetInfoModeGlobal();
      CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
      pMainFrame->SetFocus();
    }
  }
  else
  {
    // if CSG is on
    if( pDoc->m_iMode == CSG_MODE)
    {
      ASSERT(pDoc->m_pwoSecondLayer != NULL);
      // if CSG is done with primitive force info mode: INFO_MODE_PRIMITIVE
      if( pDoc->m_bPrimitiveMode)
      {
        if( m_ModeID != INFO_MODE_PRIMITIVE)
        {
          // primitive mode includes position page
          SetInfoModePrimitive();
        }
      }
      // else force info mode: INFO_MODE_POSITION
      else
      {
        if( m_ModeID != INFO_MODE_POSITION)
        {
          // no primitive page, only position page
          SetInfoModePosition();
        }
      }
    }
    // else if we are in entity mode and only one entity is selected,
    // force info mode: INFO_MODE_POSITION
    else if( (pDoc->m_iMode == ENTITY_MODE) && (pDoc->m_selEntitySelection.Count() == 1) )
    {
      if( m_ModeID != INFO_MODE_POSITION)
      {
        SetInfoModePosition();
      }
    }
    // else if we are in polygon mode
    else if( pDoc->m_iMode == POLYGON_MODE)
    {
      if( m_ModeID == INFO_MODE_POLYGON)
      {
        if( GetActivePage() == &m_PgTexture)    _iLastActivePgInPolygonMode = 1;
        else if( GetActivePage() == &m_PgShadow)_iLastActivePgInPolygonMode = 2;
        else                                    _iLastActivePgInPolygonMode = 3;
      }
      if( m_ModeID != INFO_MODE_POLYGON)
      {
        SetInfoModePolygon();
      }
    }
    // else if we are in sector mode
    else if( pDoc->m_iMode == SECTOR_MODE)
    {
      if( m_ModeID != INFO_MODE_SECTOR)
      {
        SetInfoModeSector();
      }
    }
    // else if we are in terrain mode
    else if( pDoc->m_iMode == TERRAIN_MODE)
    {
      if( m_ModeID != INFO_MODE_TERRAIN)
      {
        SetInfoModeTerrain();
      }      
    }
    // we are not in CSG mode nor in single entity mode, force info mode: INFO_MODE_GLOBAL
    else
    {
      if( m_ModeID != INFO_MODE_GLOBAL)
      {
        SetInfoModeGlobal();
      }
    }
  }
  // call OnIdle() for active page
  CPropertyPage *pgActivPage = GetActivePage();
  CALLACTIVEPAGE(OnIdle, lCount);
  return TRUE;
}

void CInfoSheet::PostNcDestroy() 
{
	CPropertySheet::PostNcDestroy();
	delete this;
}

BOOL CInfoSheet::PreTranslateMessage(MSG* pMsg) 
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
	if(pMsg->message==WM_KEYDOWN)
  {
    // get active document 
    BOOL bSectorNameTyping = FALSE;
    CPropertyPage *pgActivPage = GetActivePage();
    if(pgActivPage == &m_PgSector)
    {
      bSectorNameTyping = m_PgSector.GetDlgItem( IDC_SECTOR_NAME) == CWnd::GetFocus();
    }
    if( (pMsg->wParam==VK_SPACE) && !bSectorNameTyping)
    {
      // don't translate message
      return TRUE;
    }
    // if we have valid document
    if( (pDoc != NULL) && !bSectorNameTyping)
    {
      if( (pMsg->wParam==VK_ADD) ||
          (pMsg->wParam==VK_SUBTRACT) ||
          (pMsg->wParam=='E') ||
          (pMsg->wParam=='S') ||
          (pMsg->wParam=='P') ||
          (pMsg->wParam=='Z') ||
          (pMsg->wParam=='Q') ||
          (pMsg->wParam== VK_DECIMAL) )
      {
        // post this message to main frame
        ::PostMessage( pMainFrame->m_hWnd, WM_KEYDOWN, pMsg->wParam, pMsg->lParam);
        // don't translate message
        return TRUE;
      }
    }
  }
	
	return CPropertySheet::PreTranslateMessage(pMsg);
}
