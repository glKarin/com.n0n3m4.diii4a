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

// BrowseWindow.h : header file
//
#ifndef BROWSEWINDOW_H
#define BROWSEWINDOW_H 1

/////////////////////////////////////////////////////////////////////////////
// CBrowseWindow window

#define STRING_HEIGHT 16

HGLOBAL CreateHDrop( const CTFileName &fnToDrag, BOOL bAddAppPath=TRUE);

class CBrowseWindow : public CWnd
{
// Construction
public:
	BOOL AttachToControl( CWnd *pwndParent);
	CBrowseWindow();
  void SetBrowserPtr( CBrowser *pBrowser);

// Attributes
public:
  INDEX m_iLastHittedItem;
  BOOL m_bDirectoryOpen;    // If directory is opened (valid)
  CBrowser *m_pBrowser;
  CListHead m_IconsList;
  INDEX m_IconsInLine;
  INDEX m_IconsInColumn;
  INDEX m_IconsVisible;
  PIX m_IconWidth;
  PIX m_IconHeight;
  PIX m_BrowseWndWidth;
  PIX m_BrowseWndHeight;

// Operations
public:
  void OpenDirectory( CVirtualTreeNode *pVTN);
  void CloseDirectory( CVirtualTreeNode *pVTN);
  void InsertItem( CTFileName fnItem, CPoint pt);
	void DeleteSelectedItems();
  CVirtualTreeNode *GetItem( INDEX iItem) const;
  INDEX GetItemNo( CVirtualTreeNode *pVTN);
  void SetItemSize( PIX pixWidth, PIX pixHeight);
  INDEX HitItem( CPoint pt, FLOAT &fHitXOffset, FLOAT &fHitYOffset) const;
  void Refresh();
  void SelectByTextures( BOOL bInSelectedSectors, BOOL bExceptSelected);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseWindow)
	//}}AFX_VIRTUAL

// Implementation
public:
  COleDataSource m_DataSource;

	virtual ~CBrowseWindow();
	void OnContextMenu( CPoint point);
  void GetToolTipText( char *pToolTipText);

  CDrawPort *m_pDrawPort;
  CViewPort *m_pViewPort;
	
  // Generated message map functions
	//{{AFX_MSG(CBrowseWindow)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnInsertItems();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDeleteItems();
	afx_msg void OnBigIcons();
	afx_msg void OnMediumIcons();
	afx_msg void OnSmallIcons();
	afx_msg void OnShowDescription();
	afx_msg void OnShowFilename();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRecreateTexture(); 
	afx_msg void OnCreateAndAddTexture();
	afx_msg void OnSelectByTextureInSelectedSectors();
	afx_msg void OnSelectByTextureInWorld();
	afx_msg void OnSelectForDropMarker();
	afx_msg void OnSetAsCurrentTexture();
	afx_msg void OnConvertClass();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMicroIcons();
	afx_msg void OnSelectExceptTextures();
	afx_msg void OnAddTexturesFromWorld();
	afx_msg void OnShowTreeShortcuts();
	afx_msg void OnExportTexture();
	afx_msg void OnBrowserContextHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif // BROWSEWINDOW_H
