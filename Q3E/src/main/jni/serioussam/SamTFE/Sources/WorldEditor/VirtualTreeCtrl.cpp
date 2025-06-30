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

// VirtualTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "VirtualTreeCtrl.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVirtualTreeCtrl

CVirtualTreeCtrl::CVirtualTreeCtrl()
{
  m_bIsOpen = FALSE;
}

CVirtualTreeCtrl::~CVirtualTreeCtrl()
{
}

void CVirtualTreeCtrl::SetBrowserPtr( CBrowser *pBrowser)
{
  m_pBrowser = pBrowser;
}

BEGIN_MESSAGE_MAP(CVirtualTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CVirtualTreeCtrl)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVirtualTreeCtrl message handlers

void CVirtualTreeCtrl::CloseTreeCtrl(void) 
{
	if( m_bIsOpen)
  {
    m_pBrowser->m_TreeHeight = CLOSED_TREE;
    m_pBrowser->CalcDynamicLayout(0, LM_HORZDOCK);
	  m_bIsOpen = FALSE;

    if( GetCount() != 0)
    {
      HTREEITEM pSelectedItem = GetSelectedItem();
      ASSERT( pSelectedItem != NULL);
      EnsureVisible( pSelectedItem);
      m_pBrowser->OpenSelectedDirectory();
    }
  }
  // Enable drag/drop open
	DragAcceptFiles();
}

void CVirtualTreeCtrl::OpenTreeCtrl(void)
{
	if( !m_bIsOpen)
  {
    m_pBrowser->m_TreeHeight = OPEN_TREE;
    m_pBrowser->CalcDynamicLayout(0, LM_HORZDOCK);
    m_bIsOpen = TRUE;
    if( GetCount() != 0)
    {
      HTREEITEM pSelectedItem = GetSelectedItem();
      ASSERT( pSelectedItem != NULL);
      m_pBrowser->CloseSelectedDirectory();
    }
  }
}

void CVirtualTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
  CVirtualTreeNode *pVTN = ItemForCoordinate(point);
  // if is not null and not root
  if( pVTN!=NULL && pVTN->vnt_pvtnParent!=NULL)
  {
    CTString strAddr;
    strAddr.PrintF("VTN%d", pVTN);
    HGLOBAL hglobal = CreateHDrop( strAddr, FALSE);
    m_DataSource.CacheGlobalData( CF_HDROP, hglobal);
    m_DataSource.DoDragDrop( DROPEFFECT_COPY);
  }

  OpenTreeCtrl();
	CTreeCtrl::OnLButtonDown(nFlags, point);
}


CVirtualTreeNode *CVirtualTreeCtrl::ItemForCoordinate(CPoint pt)
{
  UINT ulFlags;
  HTREEITEM pItem=HitTest(pt, &ulFlags);
  if( pItem==NULL || !(ulFlags&TVHT_ONITEMICON)) return NULL;
  CVirtualTreeNode *pVTN = (CVirtualTreeNode *)GetItemData( pItem);
  return pVTN;
}

void CVirtualTreeCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CloseTreeCtrl();
	
	CTreeCtrl::OnLButtonDblClk(nFlags, point);
}

void CVirtualTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	UWORD uwScanCode = nFlags & 255;
  BOOL bAlt = (nFlags & (1L<<13)) != 0;
	
  /*
  if( uwScanCode == 82)                // Insert
  {
    m_pBrowser->OnCreateDirectory();
  }
	else if( uwScanCode == 83)           // Delete
  {
    m_pBrowser->OnDeleteDirectory();
  }
	else if( bAlt && (uwScanCode == 19)) // Alt-R - rename
  {
    m_pBrowser->OnRenameDirectory();
  }
	else if( bAlt && (uwScanCode == 38)) // Alt+O - open
  {
    m_pBrowser->OnLoadVirtualTree();
  }
	else if( bAlt && (uwScanCode == 31)) // Alt+S - save
  {
    m_pBrowser->OnSaveVirtualTree();
  }
  */
	
  CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CVirtualTreeCtrl::OnContextMenu( CPoint point) 
{
  CMenu menu;
  if( menu.LoadMenu(IDR_VTREEPOPUP))
  {
		CMenu* pPopup = menu.GetSubMenu(0);
    pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								 point.x, point.y, m_pBrowser);
  }
}

void CVirtualTreeCtrl::OnDropFiles(HDROP hDropInfo) 
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  INDEX ctFiles = DragQueryFile( hDropInfo, 0xFFFFFFFF, NULL, 0);

  // get dropped coordinates
  CPoint point;
  DragQueryPoint( hDropInfo, &point);
  
  CVirtualTreeNode *pVTNDst = ItemForCoordinate(point);
  if( pVTNDst!=NULL)
  {
    for( INDEX i=0; i<ctFiles; i++)
    {
	    wchar_t chrFile[ 256];
      DragQueryFile( hDropInfo, i, chrFile, 256);
      CTString strAddr = CTString(CStringA(chrFile));
      if( strAddr != "")
      {
        CVirtualTreeNode *pVTNSrc;
        strAddr.ScanF("VTN%d", &pVTNSrc);
        if(pVTNSrc==pVTNDst) return;
        pVTNSrc->MoveToDirectory( pVTNDst);
        // delete all items
        DeleteAllItems();
        m_pBrowser->AddDirectoryRecursiv( &m_pBrowser->m_VirtualTree, TVI_ROOT);   // Fill CTreeCtrl using recursion
        SortChildren( NULL);
        SelectItem( (HTREEITEM) pVTNSrc->vtn_Handle);
        m_pBrowser->m_bVirtualTreeChanged = TRUE;
        m_pBrowser->OpenSelectedDirectory();
      }
    }
  }

	CTreeCtrl::OnDropFiles(hDropInfo);
}
