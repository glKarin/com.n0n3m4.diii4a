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

// Browser.cpp : implementation file
//
 
#include "stdafx.h"
#include "Browser.h"
 
#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////
// CBrowser Construction/Destruction
 
CBrowser::CBrowser(void)
{
  // set default number of subdirectories (none)
  for( INDEX i=0; i<DIRECTORY_SHORTCT_CT; i++)
  {
    m_aiSubDirectoriesCt[ i] = 0;
  }
}

BOOL CBrowser::Create( CWnd* pParentWnd, UINT nIDTemplate,
                               UINT nStyle, UINT nID, BOOL bChange)
{
  if(!CDialogBar::Create(pParentWnd,nIDTemplate,nStyle,nID))
  {
    return FALSE;
  }

  m_bVirtualTreeChanged = FALSE;

  m_TreeHeight = CLOSED_TREE;
  m_Size = m_sizeDefault;

  m_BrowseWindow.AttachToControl( this);
  m_BrowseWindow.SetBrowserPtr( this);
  
  m_TreeCtrl.SetBrowserPtr( this);
  m_TreeCtrl.SubclassDlgItem(IDC_VIRTUALTREE, this);
  m_TreeCtrl.DragAcceptFiles();
  
  m_IconsImageList.Create( IDB_DIRECTORY_ICONS, 16, 1, CLR_NONE);
  m_TreeCtrl.SetImageList( &m_IconsImageList, TVSIL_NORMAL);
	
  return TRUE;
}
 
////////////////////////////////////////////////////////////////////
// Overloaded functions
 
CSize CBrowser::CalcDynamicLayout(int nLength, DWORD nMode)
{
  CSize csResult;
  // Return default if it is being docked or floated
  if ((nMode & LM_VERTDOCK) || (nMode & LM_HORZDOCK))
  {
    if (nMode & LM_STRETCH) // if not docked stretch to fit
    {
      csResult = CSize((nMode & LM_HORZ) ? 32767 : m_Size.cx,
                       (nMode & LM_HORZ) ? m_Size.cy : 32767);
    }
    else
    {
      csResult = m_Size;
    }
  }
  else if (nMode & LM_MRUWIDTH)
  {
    csResult = m_Size;
  }
  // In all other cases, accept the dynamic length
  else
  {
    if (nMode & LM_LENGTHY)
    {
      csResult = CSize( m_Size.cx, m_Size.cy = nLength);
    }
    else
    {
      csResult = CSize( m_Size.cx = nLength, m_Size.cy);
    }
  }
  
  CRect NewBrowserPos;
  CRect NewTreePos;
  
  ULONG ulNewTreeHeight = m_TreeHeight;
  if( csResult.cy < (m_TreeHeight + V_BORDER*2 + 10) )
  {
    ulNewTreeHeight = csResult.cy - +V_BORDER*2 - 10;
  }
  
  // First we calculate and set browsing window position
  NewBrowserPos = CRect( H_BORDER,
                         V_BORDER + ulNewTreeHeight,
                         csResult.cx - H_BORDER,
                         csResult.cy - V_BORDER);
  m_BrowseWindow.MoveWindow( NewBrowserPos);

  // Finaly we set new virtual tree control size
  NewTreePos = CRect(  H_BORDER,
                       V_BORDER,
                       csResult.cx - H_BORDER,
                       V_BORDER + ulNewTreeHeight);
  CWnd *pwndTree = GetDlgItem( IDC_VIRTUALTREE);
  pwndTree->MoveWindow( NewTreePos);

  m_boxBrowseWnd = PIXaabbox2D( PIX2D(NewBrowserPos.TopLeft().x, NewBrowserPos.TopLeft().y),
                      PIX2D(NewBrowserPos.BottomRight().x, NewBrowserPos.BottomRight().y) );
  m_BrowseWindow.m_BrowseWndWidth = NewBrowserPos.Width();
  m_BrowseWindow.m_BrowseWndHeight = NewBrowserPos.Height();
  m_boxTreeWnd = PIXaabbox2D( PIX2D(NewTreePos.TopLeft().x, NewTreePos.TopLeft().y),
                      PIX2D(NewTreePos.BottomRight().x, NewTreePos.BottomRight().y) );

  return csResult;
}
 
BEGIN_MESSAGE_MAP(CBrowser, CDialogBar)
  //{{AFX_MSG_MAP(CBrowser)
	ON_COMMAND(ID_CREATE_DIRECTORY, OnCreateDirectory)
	ON_COMMAND(ID_DELETE_DIRECTORY, OnDeleteDirectory)
	ON_COMMAND(ID_SAVE_VIRTUAL_TREE, OnSaveVirtualTree)
	ON_COMMAND(ID_LOAD_VIRTUAL_TREE, OnLoadVirtualTree)
	ON_COMMAND(ID_RENAME_DIRECTORY, OnRenameDirectory)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_SAVE_AS_VIRTUAL_TREE, OnSaveAsVirtualTree)
	ON_COMMAND(ID_IMPORT_VIRTUAL_TREE, OnImportVirtualTree)
	ON_COMMAND(ID_EXPORT_VIRTUAL_TREE, OnExportVirtualTree)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_VIRTUAL_TREE, OnUpdateImportVirtualTree)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_VIRTUAL_TREE, OnUpdateExportVirtualTree)
	ON_COMMAND(ID_DUMP_VT, OnDumpVt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
 
/////////////////////////////////////////////////////////////////////
// CBrowser message handlers
/////////////////////////////////////////////////////////////////////
 
void CBrowser::AddDirectoryRecursiv(CVirtualTreeNode *pOneDirectory, HTREEITEM hParent)
{
  // Insert one entry into virtual directory tree
  HTREEITEM InsertedDir;
  InsertedDir = m_TreeCtrl.InsertItem( 0, L"", 0, 0, TVIS_SELECTED, TVIF_STATE, 0,
                                       hParent, TVI_SORT );

  pOneDirectory->vtn_Handle = (HTREEITEM) InsertedDir;
  m_TreeCtrl.SetItemData( InsertedDir, (DWORD_PTR)(pOneDirectory));
  m_TreeCtrl.SetItemText( InsertedDir, CString(pOneDirectory->vtn_strName));
  m_TreeCtrl.SetItemImage( InsertedDir, pOneDirectory->vtn_itIconType,
                           pOneDirectory->vtn_itIconType + NO_OF_ICONS);

  // Now add this directory's subdirectories recursively
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pOneDirectory->vtn_lhChildren, it)
  {
    CVirtualTreeNode &vtn=*it;
    if( vtn.vtn_bIsDirectory)
    {
      AddDirectoryRecursiv( &vtn, InsertedDir);
    }
  }
}

void CBrowser::OnCreateDirectory() 
{
  CDlgCreateVirtualDirectory dlg;

  if( dlg.DoModal() == IDOK)
  {
    CloseSelectedDirectory();
    CVirtualTreeNode *pvtnCurrent;
    // Is this root directory ?
    if( m_TreeCtrl.GetCount() == 0)
    {
      // Set data in instanciated CVirtualTreeNode because it is Root
      m_VirtualTree.vtn_bIsDirectory = TRUE;
      m_VirtualTree.vtn_itIconType = dlg.m_iSelectedIconType;
      m_VirtualTree.vtn_strName = CTString( CStringA(dlg.m_strDirectoryName));
      // Root's parent is NULL
      m_VirtualTree.vnt_pvtnParent = NULL;
      pvtnCurrent = &m_VirtualTree;
    }
    else
    {
      // Allocate new CVirtualTreeNode to carry new directory data
      CVirtualTreeNode *pvtnNewDir = new CVirtualTreeNode();
      pvtnCurrent = pvtnNewDir;
      pvtnNewDir->vtn_bIsDirectory = TRUE;
      pvtnNewDir->vtn_itIconType = dlg.m_iSelectedIconType;
      pvtnNewDir->vtn_strName = CTString( CStringA(dlg.m_strDirectoryName));
      // Get ptr to parent CVirtualTreeNode
      HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
      if( pSelectedItem == NULL) return;
      pvtnNewDir->vnt_pvtnParent = (CVirtualTreeNode *)m_TreeCtrl.GetItemData( pSelectedItem);
      // And add this new directory into its list
      pvtnNewDir->vnt_pvtnParent->vtn_lhChildren.AddTail( pvtnNewDir->vtn_lnInDirectory);
    }
    
    m_bVirtualTreeChanged = TRUE;
    m_TreeCtrl.DeleteAllItems();
    AddDirectoryRecursiv( &m_VirtualTree, TVI_ROOT);   // Fill CTreeCtrl using recursion
    m_TreeCtrl.SortChildren( NULL);
    // Now select it
    m_TreeCtrl.SelectItem( (HTREEITEM) pvtnCurrent->vtn_Handle);
    OpenSelectedDirectory();
  }
  m_TreeCtrl.SetFocus();
}

void CBrowser::OnUpdateVirtualTreeControl(void) 
{
  m_bVirtualTreeChanged = TRUE;
  m_TreeCtrl.DeleteAllItems();
  AddDirectoryRecursiv( &m_VirtualTree, TVI_ROOT);
  m_TreeCtrl.SortChildren( NULL);
  // Now select it
  m_TreeCtrl.SelectItem( (HTREEITEM) m_VirtualTree.vtn_Handle);
  OpenSelectedDirectory();
  m_TreeCtrl.SetFocus();
}

void CBrowser::OnDeleteDirectory() 
{
  if( m_TreeCtrl.GetCount() == 0)
    return;

  if( ::MessageBoxA( this->m_hWnd, "Do You really want to delete directory and all its subdirectories?",
                    "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1 | 
                    MB_TASKMODAL | MB_TOPMOST) != IDYES)
  {
    m_TreeCtrl.SetFocus();
    return;
  }
  CloseSelectedDirectory();
  DeleteDirectory();
  OpenSelectedDirectory();
}

void CBrowser::DeleteDirectory(void)
{
  if( m_TreeCtrl.GetCount() == 0)
    return;

  CVirtualTreeNode *pvtnParent = NULL;

  HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
  if( pSelectedItem == NULL) return;
  CVirtualTreeNode *pVTN = (CVirtualTreeNode *)m_TreeCtrl.GetItemData( pSelectedItem);

  // Delete all subdirectories
  FORDELETELIST( CVirtualTreeNode, vtn_lnInDirectory, pVTN->vtn_lhChildren, litDel)
  {
    delete &litDel.Current();
  }

  // If this is not root directory, remove it from list and delete it
  if( pVTN->vnt_pvtnParent != NULL)
  {
    pvtnParent = pVTN->vnt_pvtnParent;
    pVTN->vtn_lnInDirectory.Remove();
    delete pVTN;
  }
  
  m_TreeCtrl.DeleteAllItems();
  // If it wasn't root directory, fill TreeCtrl with data
  if( pvtnParent != NULL)
  {
    AddDirectoryRecursiv( &m_VirtualTree, TVI_ROOT);   // Fill CTreeCtrl using recursion
    m_TreeCtrl.SortChildren( NULL);
    HTREEITEM NewActiveDir = (HTREEITEM) pvtnParent->vtn_Handle;
    
    if( m_TreeCtrl.ItemHasChildren(NewActiveDir) )
    {
      NewActiveDir = m_TreeCtrl.GetChildItem( NewActiveDir);
    }
    m_TreeCtrl.SelectItem( NewActiveDir);
  }
  m_bVirtualTreeChanged = TRUE;
  m_TreeCtrl.SetFocus();
}

CVirtualTreeNode *CBrowser::FindItemDirectory(CVirtualTreeNode *pvtnCurrentDirectory,
                                                 CTFileName fnItemFileName)
{
  // Now search this directory recursivly
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pvtnCurrentDirectory->vtn_lhChildren, it)
  {
    if( it->vtn_bIsDirectory)
    {
      CVirtualTreeNode *pvtnResult = FindItemDirectory( &it.Current(), fnItemFileName);
      if( pvtnResult != NULL) return pvtnResult;
    }
    else
    {
      if( it->vtn_fnItem == fnItemFileName)
      {
        // deselect all items in directory
        FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pvtnCurrentDirectory->vtn_lhChildren, it2)
        {
          it2->vtn_bSelected = FALSE;
        }
        // selected requested item
        it->vtn_bSelected = TRUE;
        return pvtnCurrentDirectory;
      }
    }
  }
  return NULL;
}

void CBrowser::SelectItemDirectory( CTFileName fnItemFileName)
{
  CVirtualTreeNode *pvtnItemDirectory = FindItemDirectory( &m_VirtualTree, fnItemFileName);
  if( pvtnItemDirectory != NULL)
  {
    CloseSelectedDirectory();
    // Now select it
    m_TreeCtrl.SelectItem( (HTREEITEM) pvtnItemDirectory->vtn_Handle);
    OpenSelectedDirectory();
  }
}

/*
 * Separate subdirectory names along current path
 */
INDEX CBrowser::GetSelectedDirectory( CTString strArray[])
{
  INDEX iSubDirsCt = 0;
  HTREEITEM pItem = m_TreeCtrl.GetSelectedItem();
  if( pItem == NULL) return -1;
  FOREVER
  {
    CTString strItemName = CTString( CStringA(m_TreeCtrl.GetItemText(pItem)));
    strArray[ iSubDirsCt] = strItemName;
    iSubDirsCt ++;

    if( m_TreeCtrl.GetParentItem(pItem) == NULL)
      break;
    pItem = m_TreeCtrl.GetParentItem(pItem);
  }
  return iSubDirsCt;
}

HTREEITEM CBrowser::GetVirtualDirectoryItem( CTString strArray[], INDEX iSubDirsCt)
{
  INDEX j;
  // pick up root item ptr if path finding fails, it will be selected
  HTREEITEM pRootItem = m_TreeCtrl.GetRootItem();
  HTREEITEM pItem = pRootItem;
  // loop all directories starting from root and search for selected subdirectory name 
  for( j=iSubDirsCt-1; j>=0; j--)
  {
    BOOL bSucess;
    HTREEITEM pStartItem = pItem;
    FOREVER
    {
      if( pItem == NULL)
      {
        bSucess = FALSE;
        break;
      }
      // get current directory's (starting from root) first subdirectory 
      CTString strItemName = CTString( CStringA(m_TreeCtrl.GetItemText(pItem)));
      // is this subdirectory's name same as one in list of selected along given path
      if( strArray[ j] == strItemName)
      {
        // subdirectory found, if it has children or selected subdirectories counter reached 0
        if( m_TreeCtrl.ItemHasChildren( pItem) || (j==0) )
        {
          // mark that we found current subdirectory
          bSucess = TRUE;
          // if counter didn't reached 0
          if( j!=0)
          {
            // it becomes current and we will try to find his selected subdirectory
            pItem = m_TreeCtrl.GetChildItem( pItem);
          }
          break;
        }
        // we have no more children and counter is not 0 which means
        else
        {
          // that we didn't succeeded
          bSucess = FALSE;
          break;
        }
      }
      // this directory's name is not one we are searching for
      else
      {
        // so skip to next item in current directory
        pItem = m_TreeCtrl.GetNextItem( pItem, TVGN_NEXT);
        // if we have wrapped back to start of items list, we didn't find any dir with
        // selected name
        if( pStartItem == pItem)
        {
          // so we weren't successful
          bSucess = FALSE;
          break;
        }
      }
    }
    // directory search finished, if selected subdirectory wasn't found
    if( !bSucess)
    {
      // select root item
      pItem = pRootItem;
      // stop searching
      break;
    }
  }
  return pItem;
}

/*
 * Selects subdirectory using given path
 */
void CBrowser::SelectVirtualDirectory( CTString strArray[], INDEX iSubDirsCt)
{
  // get virtual directory item
  HTREEITEM pItem = GetVirtualDirectoryItem( strArray, iSubDirsCt);
  // select last found directory
  m_TreeCtrl.SelectItem( pItem);
  // make it visible
  m_TreeCtrl.EnsureVisible( pItem);
}

CTFileName CBrowser::GetIOFileName(CTString strTitle, BOOL bSave) 
{
  // You can't save with no directories
  if( bSave && m_TreeCtrl.GetCount()==0)
    return CTString("");

  CWorldEditorApp *pApp = (CWorldEditorApp *)AfxGetApp();
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  char chrChoosedFileName[ 256];

  OPENFILENAMEA ofnSaveVirtualTree;
  const char *pFilters = "Virtual tree files (*.vrt)\0*.vrt\0\0";
  
  strcpy( chrChoosedFileName, pMainFrame->m_fnLastVirtualTree.FileName());
  strcat( chrChoosedFileName, pMainFrame->m_fnLastVirtualTree.FileExt());

  memset( &ofnSaveVirtualTree, 0, sizeof( OPENFILENAME));
  ofnSaveVirtualTree.lStructSize = sizeof(OPENFILENAME);
  ofnSaveVirtualTree.lpstrFilter = pFilters;
  ofnSaveVirtualTree.lpstrFile = chrChoosedFileName;
  ofnSaveVirtualTree.nMaxFile = 256;

  char strInitDir[ 256];
  strcpy( strInitDir, _fnmApplicationPath + pMainFrame->m_fnLastVirtualTree.FileDir());
  ofnSaveVirtualTree.lpstrInitialDir = strInitDir;
  ofnSaveVirtualTree.lpstrTitle = strTitle;
  ofnSaveVirtualTree.Flags = OFN_EXPLORER | OFN_ENABLEHOOK;
  ofnSaveVirtualTree.lpstrDefExt = "vrt";
  ofnSaveVirtualTree.hwndOwner = pMainFrame->m_hWnd;
  BOOL bResult=FALSE;
  if( bSave)
  {
    bResult=GetSaveFileNameA( &ofnSaveVirtualTree);
  }
  else
  {
    bResult=GetOpenFileNameA( &ofnSaveVirtualTree);
  }
  if( !bResult)
  {
    m_TreeCtrl.SetFocus();
    return CTString("");
  }
  CTFileName fnTree = CTString(chrChoosedFileName);
  try
  {
    fnTree.RemoveApplicationPath_t();
  }
  catch (const char *str_err)
  {
    AfxMessageBox( CString(str_err));
    return CTString("");
  }

  return fnTree;
}

void CBrowser::SaveVirtualTree(CTFileName fnSave, CVirtualTreeNode *pvtn)
{
  ASSERT(pvtn!=NULL);
  if(pvtn==NULL) return;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTFileStream File;
  try
  {
    File.Create_t( fnSave, CTStream::CM_BINARY);
    File.WriteID_t( CChunkID("VRTT"));
    File.WriteID_t( CChunkID(VIRTUAL_TREE_VERSION));
    pvtn->Write_t( &File);         // This will recursivly save tree beneath given tree node
  
    // Now we will try to save selected item's path
    File.WriteID_t( CChunkID("SELC"));

    // write ID of virtual tree buffer
    File.WriteID_t( CChunkID("VTBF"));
    // obtain selected directpry's path
    CTString strArray[ 32];
    INDEX iSubDirsCt = GetSelectedDirectory( strArray);
    // write count of sub-directories
    File << iSubDirsCt;
    // write names of all subdirectories along path into file
    INDEX i=0;
    for( ; i<iSubDirsCt; i++)
    {
      File << strArray[ i];
    }

    // now save all of virtual tree buffers
    for( i=0; i<DIRECTORY_SHORTCT_CT; i++)
    {
      // write ID of virtual tree buffer
      File.WriteID_t( CChunkID("VTBF"));
      // write count of sub-directories
      INDEX iSubDirsCt = m_aiSubDirectoriesCt[ i];
      File << iSubDirsCt;
      // write names of all subdirectories along path into file
      for( INDEX j=0; j<iSubDirsCt; j++)
      {
        File << m_astrVTreeBuffer[i][j];
      }
    }

    // save marker for end of virtual tree
    File.WriteID_t( CChunkID("END_"));
    // close the file
    File.Close();
  }
  catch (const char *str_err)
  {
    AfxMessageBox( CString(str_err));
    return;
  }
  
  m_TreeCtrl.SetFocus();
  m_bVirtualTreeChanged = FALSE;
}

void CBrowser::OnSaveVirtualTree() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // save whole virtual tree
  SaveVirtualTree(pMainFrame->m_fnLastVirtualTree, &m_VirtualTree);
}

void CBrowser::OnSaveAsVirtualTree() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTFileName fnSave=GetIOFileName("Save virtual tree to file", TRUE);
  if( fnSave!="")
  {
    // remember choosed file name as last used
    pMainFrame->m_fnLastVirtualTree = fnSave;
    // save virtual tree
    SaveVirtualTree(fnSave, &m_VirtualTree);
  }
}

void CBrowser::OnLoadVirtualTree() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  if( m_bVirtualTreeChanged)
  {
    if( ::MessageBoxA( this->m_hWnd, "Current virtual directory not saved. Do You want to continue anyway?",
                    "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1 | 
                    MB_TASKMODAL | MB_TOPMOST) != IDYES)
    {
      m_TreeCtrl.SetFocus();
      return;
    }
  }

  CTFileName fnOpen=GetIOFileName("Load virtual tree", FALSE);  
  if( fnOpen!="")
  {
    // remember choosed file name as last used
    pMainFrame->m_fnLastVirtualTree = fnOpen;
    // open virtual tree branch
    OnLoadVirtualTreeInternal(fnOpen, NULL);
  }
}

void CBrowser::OnImportVirtualTree() 
{
  CTFileName fnImport=GetIOFileName("Import virtual tree", FALSE);  
  if( fnImport!="")
  {
    HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
    if( pSelectedItem == NULL) return;
    CVirtualTreeNode *pVTN = (CVirtualTreeNode *)m_TreeCtrl.GetItemData( pSelectedItem);
    // open virtual tree branch
    OnLoadVirtualTreeInternal(fnImport, pVTN);
  }
}

void CBrowser::OnExportVirtualTree() 
{
  HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
  if( pSelectedItem == NULL) return;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CTFileName fnExport=GetIOFileName("Export virtual tree", TRUE);
  if( fnExport!="")
  {
    CVirtualTreeNode *pVTN = (CVirtualTreeNode *)m_TreeCtrl.GetItemData( pSelectedItem);
    // save virtual tree branch
    SaveVirtualTree(fnExport, pVTN);
  }
}

void CBrowser::OnUpdateImportVirtualTree(CCmdUI* pCmdUI) 
{
  HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
  pCmdUI->Enable( pSelectedItem != NULL);
}

void CBrowser::OnUpdateExportVirtualTree(CCmdUI* pCmdUI) 
{
  HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
  pCmdUI->Enable( pSelectedItem != NULL);
}

void CBrowser::OnLoadVirtualTreeInternal(CTFileName fnVirtulTree, CVirtualTreeNode *pvtnRoot) 
{
  CloseSelectedDirectory();
  try
  {
    LoadVirtualTree_t( fnVirtulTree, pvtnRoot);
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    FORDELETELIST( CVirtualTreeNode, vtn_lnInDirectory, m_VirtualTree.vtn_lhChildren, litDel)
    {
      delete &litDel.Current();
    }
    return;
  }
  m_TreeCtrl.SetFocus();
  m_bVirtualTreeChanged = FALSE;
  OpenSelectedDirectory();
}

void CBrowser::LoadVirtualTree_t( CTFileName fnVirtulTree, CVirtualTreeNode *pvtnRoot)
{
  if( pvtnRoot==NULL)
  {
    // If current tree is not empty, delete it
    if( m_TreeCtrl.GetCount() != 0)
    {
      FORDELETELIST( CVirtualTreeNode, vtn_lnInDirectory, m_VirtualTree.vtn_lhChildren, litDel)
      {
        delete &litDel.Current();
      }
      m_TreeCtrl.SelectItem( m_TreeCtrl.GetRootItem());
      DeleteDirectory();
    }
  }

  CTFileStream File;
  File.Open_t( fnVirtulTree, CTStream::OM_READ);
  if( !(CChunkID("VRTT") == File.GetID_t()))
  {
    throw( "This is not valid virtual tree file");
  }
  if( !(CChunkID(VIRTUAL_TREE_VERSION) == File.GetID_t()))
  {
    throw( "Invalid version of virtual tree file.");
  }

  if( pvtnRoot==NULL)
  {
    m_VirtualTree.Read_t( &File, NULL);
  }
  else
  {
    CVirtualTreeNode *pVTNNew = new CVirtualTreeNode;
    pVTNNew->Read_t( &File, pvtnRoot);
  }
  
  // delete all items
  m_TreeCtrl.DeleteAllItems();
  AddDirectoryRecursiv( &m_VirtualTree, TVI_ROOT);   // Fill CTreeCtrl using recursion

  if( pvtnRoot!=NULL)
  {
    // don't read rest of data
    File.Close();
    return;
  }

  // Now we will try to load selected directory definition
  File.ExpectID_t( CChunkID("SELC"));

  // expect ID for virtual tree buffer
  File.ExpectID_t( CChunkID("VTBF"));
  INDEX iSubDirsCt;
  // read count of sub-directories
  File >> iSubDirsCt;
  // limited number of sub directories
  ASSERT( iSubDirsCt <= 32);

  // we will load selected directories from root up to finaly selected
  CTString strArray[ 32];
  // load as many subdirectory names as count says
  INDEX i=0;
  for( ; i<iSubDirsCt; i++)
  {
    File >> strArray[ i];
  }
  // try to select directory
  SelectVirtualDirectory( strArray, iSubDirsCt);

  // now read all four virtual tree buffers
  for( i=0; i<DIRECTORY_SHORTCT_CT; i++)
  {
    // expect ID for virtual tree buffer
    File.ExpectID_t( CChunkID("VTBF"));
    // read count of sub-directories
    File >> iSubDirsCt;
    // store it in array of counters
    m_aiSubDirectoriesCt[ i] = iSubDirsCt;
    // limited number of sub directories
    ASSERT( iSubDirsCt <= 32);

    // load as many subdirectory names as count says
    for( INDEX j=0; j<iSubDirsCt; j++)
    {
      File >> m_astrVTreeBuffer[i][j];
    }
  }

  File.ExpectID_t( CChunkID("END_"));
  File.Close();
}

void CBrowser::OnRenameDirectory() 
{
  if( m_TreeCtrl.GetCount() != 0)
  {
    HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
    if( pSelectedItem == NULL) return;
    CVirtualTreeNode *pVTN = (CVirtualTreeNode *)m_TreeCtrl.GetItemData( pSelectedItem);
      
    CDlgCreateVirtualDirectory dlg( pVTN->vtn_strName, CTString("Rename virtual directory") );

    if( dlg.DoModal() == IDOK)
    {
      pVTN->vtn_itIconType = dlg.m_iSelectedIconType;
      pVTN->vtn_strName = CTString( CStringA(dlg.m_strDirectoryName));
      m_TreeCtrl.SetItemImage( pSelectedItem, pVTN->vtn_itIconType,
                                              pVTN->vtn_itIconType + NO_OF_ICONS);
      m_TreeCtrl.SetItemText( pSelectedItem, CString(pVTN->vtn_strName));
      m_bVirtualTreeChanged = TRUE;
    }
    m_TreeCtrl.SetFocus();
  }
}

void CBrowser::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CRect rectBrowser;
  
  GetWindowRect( &rectBrowser);
  CPoint ptInBrowser = CPoint( point.x - rectBrowser.TopLeft().x, 
                               point.y - rectBrowser.TopLeft().y);
  PIXaabbox2D boxPoint = PIXaabbox2D( PIX2D(ptInBrowser.x, ptInBrowser.y) );
  
  if( (m_boxBrowseWnd & boxPoint) == boxPoint)
  {
    m_BrowseWindow.OnContextMenu( point);
  }	
  else if( (m_boxTreeWnd & boxPoint) == boxPoint)
  {
    m_TreeCtrl.OnContextMenu( point);
  }	
}

CVirtualTreeNode *CBrowser::GetSelectedDirectory(void)
{
	if (m_TreeCtrl.GetCount() != 0)
	{
		HTREEITEM pSelectedItem = m_TreeCtrl.GetSelectedItem();
		if (pSelectedItem != NULL)
		{
			DWORD_PTR hItem = m_TreeCtrl.GetItemData(pSelectedItem);
			CVirtualTreeNode * cthItem = reinterpret_cast<CVirtualTreeNode *>(hItem);
			return cthItem;
			//return (CVirtualTreeNode *)m_TreeCtrl.GetItemData(pSelectedItem);
		}
	}
	return NULL;
}

void CBrowser::OpenSelectedDirectory(void)
{
  CVirtualTreeNode *pVTN = GetSelectedDirectory();
  if( pVTN != NULL)
  {
    m_BrowseWindow.OpenDirectory( pVTN);
  }
}

void CBrowser::CloseSelectedDirectory(void)
{
  CVirtualTreeNode *pVTN = GetSelectedDirectory();
  if( pVTN != NULL)
  {
    m_BrowseWindow.CloseDirectory( pVTN);
  }
}

void CBrowser::OnDumpVt() 
{
  CTFileName fnName = _EngineGUI.FileRequester( "Dump virtual tree as ...",
    "Text file\0*.txt\0" FILTER_ALL FILTER_END, "Dump virtual tree directory", "VirtualTrees\\");
  if( fnName == "") return;

  CTFileStream FileDump;
  try
  {
    FileDump.Create_t( fnName, CTStream::CM_TEXT);
    m_VirtualTree.Dump(&FileDump);
    FileDump.Close();
  }
  catch (const char *str_err)
  {
    AfxMessageBox( CString(str_err));
  }
}
