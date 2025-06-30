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

// Browser.h : header file
//
#ifndef BROWSER_H
#define BROWSER_H 1

#define VIRTUAL_TREE_VERSION "V008"
#define DIRECTORY_SHORTCT_CT 10

// Constants defining borders, tree and scroll bar sizes
#define H_BORDER      8
#define V_BORDER      8
#define CLOSED_TREE   40
#define OPEN_TREE    200

class CBrowser : public CDialogBar
{
// Construction
public:
  CBrowser();
  BOOL Create( CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle,
               UINT nID, BOOL = TRUE);
 
// Attributes
public:
  CTString m_astrVTreeBuffer[DIRECTORY_SHORTCT_CT][32];
  INDEX m_aiSubDirectoriesCt[DIRECTORY_SHORTCT_CT];
  CSize m_Size;
  SLONG m_TreeHeight;

  PIXaabbox2D m_boxBrowseWnd;
  PIXaabbox2D m_boxTreeWnd;

  CBrowseWindow m_BrowseWindow;
  CImageList m_IconsImageList;
  CVirtualTreeCtrl m_TreeCtrl;
  CVirtualTreeNode m_VirtualTree;  // Internal virtual tree structure list head
  BOOL m_bVirtualTreeChanged;

// Operations
  void AddDirectoryRecursiv(CVirtualTreeNode *pOneDirectory, HTREEITEM hParent);
  void LoadVirtualTree_t( CTFileName fnVirtulTree, CVirtualTreeNode *pvtnRoot);
  // separate subdirectory names along current path
  INDEX GetSelectedDirectory( CTString strArray[]);
  // selects sub directory using given path
  void SelectVirtualDirectory( CTString strArray[], INDEX iSubDirsCt);
  HTREEITEM GetVirtualDirectoryItem( CTString strArray[], INDEX iSubDirsCt);
  // functions for finding items inside virtal tree
  void SelectItemDirectory( CTFileName fnItemFileName);
  CVirtualTreeNode *FindItemDirectory( CVirtualTreeNode *pvtnInDirectory, CTFileName fnItem);
  void SaveVirtualTree(CTFileName fnSave, CVirtualTreeNode *pvtn);
  void DeleteDirectory(void);
  CVirtualTreeNode *GetSelectedDirectory(void);
  void OpenSelectedDirectory(void);
  void CloseSelectedDirectory(void);
  void OnLoadVirtualTreeInternal(CTFileName fnVirtulTree, CVirtualTreeNode *pvtnRoot);
  CTFileName GetIOFileName(CTString strTitle, BOOL bSave);

// Overrides
// ClassWizard generated virtual function overrides
//{{AFX_VIRTUAL(CBrowser)
	//}}AFX_VIRTUAL
 
// Implementation
public:
  virtual CSize CalcDynamicLayout( int nLength, DWORD dwMode );
  void OnUpdateVirtualTreeControl(void);
 
// Generated message map functions
//protected:
    //{{AFX_MSG(CBrowser)
	afx_msg void OnCreateDirectory();
	afx_msg void OnDeleteDirectory();
	afx_msg void OnSaveVirtualTree();
	afx_msg void OnLoadVirtualTree();
	afx_msg void OnRenameDirectory();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSaveAsVirtualTree();
	afx_msg void OnImportVirtualTree();
	afx_msg void OnExportVirtualTree();
	afx_msg void OnUpdateImportVirtualTree(CCmdUI* pCmdUI);
	afx_msg void OnUpdateExportVirtualTree(CCmdUI* pCmdUI);
	afx_msg void OnDumpVt();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
 
/////////////////////////////////////////////////////////////////////
#endif // BROWSER_H
