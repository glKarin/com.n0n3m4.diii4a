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

// DlgTreeShortcuts.cpp : implementation file
//

#include "stdafx.h"
#include "DlgTreeShortcuts.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgTreeShortcuts dialog


CDlgTreeShortcuts::CDlgTreeShortcuts(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgTreeShortcuts::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgTreeShortcuts)
	//}}AFX_DATA_INIT
  m_iPressedShortcut = -1;
}


void CDlgTreeShortcuts::DoDataExchange(CDataExchange* pDX)
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  CDialog::DoDataExchange(pDX);

  // if dialog is recieving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {
    CTString astrShortcutNames[ DIRECTORY_SHORTCT_CT];
    // obtain tree shortcut names
    for( INDEX iShortcut=0;iShortcut<DIRECTORY_SHORTCT_CT; iShortcut++)
    {
      // obtain tree item
      INDEX iSubDirsCt;
      iSubDirsCt = pMainFrame->m_Browser.m_aiSubDirectoriesCt[ iShortcut];
      // get virtual directory item
      HTREEITEM pItem = pMainFrame->m_Browser.GetVirtualDirectoryItem( 
        pMainFrame->m_Browser.m_astrVTreeBuffer[iShortcut], iSubDirsCt);
      ASSERT( pItem != NULL);
      // obtain shortcut's virtual tree node
      CVirtualTreeNode *pvtnNode = 
        (CVirtualTreeNode *) pMainFrame->m_Browser.m_TreeCtrl.GetItemData( pItem);
      char achrShortcutIndex[ 16];
      sprintf( achrShortcutIndex, "%d.  ", iShortcut+1);
      // obtain full path name
      astrShortcutNames[ iShortcut] = achrShortcutIndex+theApp.GetNameForVirtualTreeNode( pvtnNode);
    }
    for(INDEX iCtrl=0; iCtrl<10; iCtrl++)
    {
      // set names to buttons
      GetDlgItem( IDC_SHORTCUT01+iCtrl)->SetWindowText( CString(astrShortcutNames[ iCtrl]));
    }
  }

	//{{AFX_DATA_MAP(CDlgTreeShortcuts)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgTreeShortcuts, CDialog)
	//{{AFX_MSG_MAP(CDlgTreeShortcuts)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
  ON_COMMAND_RANGE(IDC_SHORTCUT01, IDC_SHORTCUT10, OnTreeShortcut)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgTreeShortcuts message handlers

BOOL CDlgTreeShortcuts::PreTranslateMessage(MSG* pMsg) 
{
  if( pMsg->message==WM_KEYDOWN)
  {
    if(pMsg->wParam == VK_RETURN)
    {
      pMsg->wParam = VK_SPACE;
    }
	  // if we caught key down message
    if( (pMsg->wParam!=VK_CONTROL) &&
        (pMsg->wParam!=VK_ESCAPE) &&
        (pMsg->wParam!=VK_TAB) &&
        (pMsg->wParam!=VK_UP) &&
        (pMsg->wParam!=VK_DOWN) &&
        (pMsg->wParam!='W') )
    {
      if( (pMsg->wParam>='0') && (pMsg->wParam<='9') )
      {
        // remap key ID to number 0-9
        if( pMsg->wParam == '0') m_iPressedShortcut = 9;
        else                     m_iPressedShortcut = pMsg->wParam-'1';
      }
      EndDialog( 0);
      return TRUE;
    }
    if( pMsg->wParam == 'W')
    {
      EndDialog( 0);
    }
  }
  return CDialog::PreTranslateMessage(pMsg);
}

void CDlgTreeShortcuts::OnTreeShortcut(UINT nID)
{
  m_iPressedShortcut = nID-IDC_SHORTCUT01;
  EndDialog( 0);
}

