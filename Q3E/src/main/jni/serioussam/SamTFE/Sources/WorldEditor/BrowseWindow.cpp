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

// BrowseWindow.cpp : implementation file
//

#include "stdafx.h"
#include "BrowseWindow.h"
#include <shlobj.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CEntityClass.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

INDEX _iLastHittedItem = -1;
static CTFileName _fnRightClickedItemFileName;
static BOOL _bRightClickedIsSelected;
/////////////////////////////////////////////////////////////////////////////
// CBrowseWindow

CBrowseWindow::CBrowseWindow()
{
  m_pDrawPort = NULL;
  m_pViewPort = NULL;

  m_iLastHittedItem = 0;
  m_bDirectoryOpen = FALSE;
}

CBrowseWindow::~CBrowseWindow()
{
  if( m_pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( m_pViewPort);
    m_pViewPort = NULL;
  }
}

BEGIN_MESSAGE_MAP(CBrowseWindow, CWnd)
	//{{AFX_MSG_MAP(CBrowseWindow)
	ON_WM_VSCROLL()
	ON_WM_PAINT()
	ON_WM_DROPFILES()
	ON_WM_SIZE()
	ON_COMMAND(ID_INSERT_ITEMS, OnInsertItems)
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_DELETE_ITEMS, OnDeleteItems)
	ON_COMMAND(ID_BIG_ICONS, OnBigIcons)
	ON_COMMAND(ID_MEDIUM_ICONS, OnMediumIcons)
	ON_COMMAND(ID_SMALL_ICONS, OnSmallIcons)
	ON_COMMAND(ID_SHOW_DESCRIPTION, OnShowDescription)
	ON_COMMAND(ID_SHOW_FILENAME, OnShowFilename)
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_RECREATE_TEXTURE, OnRecreateTexture)
	ON_COMMAND(ID_CREATE_AND_ADD_TEXTURE, OnCreateAndAddTexture)
	ON_COMMAND(ID_SELECT_BY_TEXTURE_IN_SELECTED_SECTORS, OnSelectByTextureInSelectedSectors)
	ON_COMMAND(ID_SELECT_BY_TEXTURE_IN_WORLD, OnSelectByTextureInWorld)
	ON_COMMAND(ID_SELECT_FOR_DROP_MARKER, OnSelectForDropMarker)
	ON_COMMAND(ID_SET_AS_CURRENT_TEXTURE, OnSetAsCurrentTexture)
	ON_COMMAND(ID_CONVERT_CLASS, OnConvertClass)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_MICRO_ICONS, OnMicroIcons)
	ON_COMMAND(ID_SELECT_EXCEPT_TEXTURES, OnSelectExceptTextures)
	ON_COMMAND(ID_ADD_TEXTURES_FROM_WORLD, OnAddTexturesFromWorld)
	ON_COMMAND(ID_SHOW_TREE_SHORTCUTS, OnShowTreeShortcuts)
	ON_COMMAND(ID_EXPORT_TEXTURE, OnExportTexture)
	ON_COMMAND(ID_BROWSER_CONTEXT_HELP, OnBrowserContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBrowseWindow message handlers
BOOL CBrowseWindow::AttachToControl( CWnd *pwndParent)
{
  BOOL bResult = TRUE;
  bResult = Create( NULL, NULL, WS_BORDER|WS_VISIBLE|WS_VSCROLL, CRect(0,0,10,10),
                    pwndParent, IDW_BROWSER);
  if( bResult)
  {
    DragAcceptFiles();
  }
  SetFocus();
  EnableToolTips( TRUE);
  return bResult;
}

void CBrowseWindow::SetBrowserPtr( CBrowser *pBrowser)
{
  m_pBrowser = pBrowser;
}

void CBrowseWindow::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
  int iPosition = GetScrollPos(SB_VERT);
  int intMin, intMax;
  GetScrollRange( SB_VERT, &intMin, &intMax);
  INDEX iMin = intMin;
  INDEX iMax = intMax;
	switch( nSBCode )
  {
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
    {
      SetScrollPos(SB_VERT, nPos);
      break;
    }
    case SB_LINEDOWN:
    {
      iPosition = Clamp( iPosition+m_IconHeight, iMin, iMax);
      SetScrollPos(SB_VERT, iPosition);
      break;
    }
    case SB_PAGEDOWN:
    {
      iPosition = Clamp( iPosition+m_IconsInColumn*m_IconHeight, iMin, iMax);
      SetScrollPos(SB_VERT, iPosition);
      break;
    }
    case SB_LINEUP:
    {
      iPosition = Clamp( iPosition-m_IconHeight, iMin, iMax);
      SetScrollPos(SB_VERT, iPosition);
      break;
    }
    case SB_PAGEUP:
    {
      iPosition = Clamp( iPosition-m_IconsInColumn*m_IconHeight, iMin, iMax);
      SetScrollPos(SB_VERT, iPosition);
      break;
    }
  }

  CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
  Invalidate(FALSE);
}

#define BOX( dp, xs, ys, w, h, color, type)         \
  dp->DrawLine( xs, ys, xs, ys+h, color, type);     \
  dp->DrawLine( xs, ys+h, xs+w, ys+h, color, type); \
  dp->DrawLine( xs+w, ys+h, xs+w, ys, color, type); \
  dp->DrawLine( xs+w, ys, xs, ys, color, type);

#define ICONS_TRAY_WIDTH 16
#define ICONS_TRAY_HEIGHT 16
void CBrowseWindow::OnPaint()
{
  {
  CPaintDC dc(this); // device context for painting (does BeginPaint()/EndPaint()!)
  }
  CWorldEditorApp *pApp = (CWorldEditorApp *)AfxGetApp();
  PIXaabbox2D rectPict;
  INDEX i;

  if (m_pDrawPort==NULL || !m_pDrawPort->Lock()) {
    return;
  }

  // To write description
  m_pDrawPort->SetFont( theApp.m_pfntSystem);
  m_pDrawPort->SetTextAspect( 1.0f);
  
  // clear browsing window
  m_pDrawPort->FillZBuffer( ZBUF_BACK);
  m_pDrawPort->Fill( C_BLACK | CT_OPAQUE);
  
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if (pVTNDir==NULL) {
    m_pDrawPort->Unlock();
    return;
  }
  
  if(m_bDirectoryOpen)
  {
    INDEX iYOffset = GetScrollPos( SB_VERT);
    INDEX iLineVisible = iYOffset/m_IconHeight;
    PIX pixStart = iYOffset%m_IconHeight;
    for( i=0; i<m_IconsVisible+m_IconsInLine; i++)
    {
      CVirtualTreeNode *pVTN = GetItem( i + iLineVisible*m_IconsInLine);
      if( pVTN == NULL)
      {
        break;
      }
      
      PIX x = (i%m_IconsInLine) * m_IconWidth;
      PIX y = (i/m_IconsInLine) * m_IconHeight - pixStart + ICONS_TRAY_HEIGHT + 1;
      
      switch( pVTNDir->vtn_bmBrowsingMode)
      {
      case BM_ICONS_MICRO:
      case BM_ICONS_SMALL:
      case BM_ICONS_MEDIUM:
      case BM_ICONS_LARGE:
        {
          PIX pixIconWidth = m_IconWidth;
          PIX pixIconHeight = m_IconHeight;
          // if we are browsing large icons, leave some space for icon text
          if( pVTNDir->vtn_bmBrowsingMode == BM_ICONS_LARGE)
          {
            pixIconHeight -= STRING_HEIGHT;
          }
          // set error texture
          CTextureData *ptdIcon = pApp->m_ptdError;
          if( pVTN->vtn_pTextureData != NULL)
          {
            ptdIcon = pVTN->vtn_pTextureData;
          }
          // get texture width and height
          PIX pixTextureWidth = ptdIcon->GetWidth();
          PIX pixTextureHeight= ptdIcon->GetHeight();
          // apply texture aspect ratio to icon
          if( pixTextureWidth>pixTextureHeight)
          {
            pixIconHeight /= pixTextureWidth/pixTextureHeight;
          }
          else
          {
            pixIconWidth /= pixTextureHeight/pixTextureWidth;
          }
          
          // if we have large icons
          if( pVTNDir->vtn_bmBrowsingMode == BM_ICONS_LARGE)
          {
            // rectangle for texture polygon is smaller because of subtitling info text
            rectPict = PIXaabbox2D(
              PIX2D(x+m_IconWidth/2-pixIconWidth/2+1, y+m_IconHeight/2-pixIconHeight/2+1),
              PIX2D(x+m_IconWidth/2+pixIconWidth/2-2, y+m_IconHeight/2+pixIconHeight/2-2-STRING_HEIGHT));
          }
          else
          {
            // texture box covers maximum space
            rectPict = PIXaabbox2D(
              PIX2D(x+m_IconWidth/2-pixIconWidth/2+1, y+m_IconHeight/2-pixIconHeight/2+1),
              PIX2D(x+m_IconWidth/2+pixIconWidth/2-2, y+m_IconHeight/2+pixIconHeight/2-2));
          }
          
          // draw icon
          CTextureObject toIcon;
          toIcon.SetData( ptdIcon);
          m_pDrawPort->PutTexture( &toIcon, rectPict);
          
          // if we have large icons
          if( pVTNDir->vtn_bmBrowsingMode == BM_ICONS_LARGE)
          {
            // type info text
            if( pVTN->vtn_fnItem.FileExt() == ".tex")
            {
              m_pDrawPort->PutText( pVTN->vtn_fnItem.FileName(), x, y + m_IconHeight-2-STRING_HEIGHT);
            }
            else
            {
              m_pDrawPort->PutText( pVTN->vtn_strName, x, y + m_IconHeight-2-STRING_HEIGHT);
            }
          }
          
          if( pVTN->vtn_bSelected)
          {
            BOX( m_pDrawPort, x, y, m_IconWidth-1, m_IconHeight-1, C_WHITE|CT_OPAQUE, _FULL_);
            BOX( m_pDrawPort, x, y, m_IconWidth-1, m_IconHeight-1,  C_lRED|CT_OPAQUE, _POINT_);
          }
          break;
        }
      case BM_DESCRIPTION:
      case BM_FILENAME:
        {
          // First paint little icon
          rectPict = PIXaabbox2D( PIX2D(x, y), PIX2D(x+STRING_HEIGHT, y+STRING_HEIGHT));
          if( pVTN->vtn_pTextureData != NULL)
          {
            CTextureObject toIcon;
            toIcon.SetData( pVTN->vtn_pTextureData);
            m_pDrawPort->PutTexture( &toIcon, rectPict);
          }
          else
          {
            m_pDrawPort->PutTexture( pApp->m_ptoError, rectPict);
          }
          // if we are using descriptive name
          if( pVTNDir->vtn_bmBrowsingMode == BM_DESCRIPTION)
          {
            if( pVTN->vtn_fnItem.FileExt() == ".tex")
            {
              m_pDrawPort->PutText( pVTN->vtn_fnItem.FileName(), x + STRING_HEIGHT, y);
            }
            else
            {
              m_pDrawPort->PutText( pVTN->vtn_strName, x + STRING_HEIGHT, y);
            }
          }
          // if we are using file name
          else
          {
            m_pDrawPort->PutText( pVTN->vtn_fnItem, x + STRING_HEIGHT, y);
          }
          
          if( pVTN->vtn_bSelected)
          {
            BOX( m_pDrawPort, x, y, m_IconWidth-1, m_IconHeight-1, C_WHITE|CT_OPAQUE, _FULL_);
            BOX( m_pDrawPort, x, y, m_IconWidth-1, m_IconHeight-1,  C_lRED|CT_OPAQUE, _POINT_);
          }
          break;
        }
      default:
        {
          ASSERTALWAYS( "Unrecognizible browsing type found !");
        }
      }
    }
  }
  
  // draw icons tray
  CTextureObject to;
  to.SetData(theApp.m_ptdIconsTray);
  INDEX iSelected=0;
  switch( pVTNDir->vtn_bmBrowsingMode)
  {
  case BM_ICONS_MICRO:  iSelected = 0; break;
  case BM_ICONS_SMALL:  iSelected = 1; break;
  case BM_ICONS_MEDIUM: iSelected = 2; break;
  case BM_ICONS_LARGE:  iSelected = 3; break;
  case BM_DESCRIPTION:  iSelected = 4; break;
  case BM_FILENAME:     iSelected = 5; break;
  }
  
  FLOAT fRatio = theApp.m_ptdIconsTray->GetWidth()/theApp.m_ptdIconsTray->GetPixWidth();
  MEX2D mex2dStart = MEX2D(0, MEX(iSelected*16*fRatio));
  MEX2D mex2dEnd = MEX2D(MEX(16*8*fRatio), MEX((iSelected*16+16)*fRatio));
  MEXaabbox2D boxTexture = MEXaabbox2D( mex2dStart, mex2dEnd);
  PIXaabbox2D boxScreen = PIXaabbox2D( PIX2D(0, 0), PIX2D(8*16, 16));
  m_pDrawPort->Fill( 0, 0, m_pDrawPort->GetWidth(), 17, C_BLACK|CT_OPAQUE);
  m_pDrawPort->PutTexture( &to, boxScreen, boxTexture);
  
  m_pDrawPort->Unlock();
  m_pViewPort->SwapBuffers();
}

void CBrowseWindow::OnContextMenu( CPoint point)
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL)
  {
    AfxMessageBox( L"Virtual tree doesn't yet exists. Create at least one virtual directory "
                   L"to be able to insert items into it.");
    return;
  }

  CWorldEditorDoc *pDoc = theApp.GetDocument();

  // convert coordinates from screen to client
  CPoint ptClientCoordinates = point;
  ScreenToClient( &ptClientCoordinates);

  // Add hit icon to selection
  FLOAT fDummyX, fDummyY;
  INDEX iHittedItem = HitItem( ptClientCoordinates, fDummyX, fDummyY);
  _fnRightClickedItemFileName = CTString("");
  if( iHittedItem != -1)
  {
    CVirtualTreeNode *pVTN = GetItem( iHittedItem);
    ASSERT( pVTN != NULL);
    _fnRightClickedItemFileName = pVTN->vtn_fnItem;
    _bRightClickedIsSelected = pVTN->vtn_bSelected;
  }

  CMenu menu;
  if( menu.LoadMenu(IDR_BROWSERPOPUP))
  {
    CMenu* pPopup = menu.GetSubMenu(0);
    UINT iRecreateTextureCommandState = MF_DISABLED|MF_GRAYED;
    if( _fnRightClickedItemFileName.FileExt()==CTString(".tex") ||
        _fnRightClickedItemFileName.FileExt()==CTString(".tbn") )
    {
      iRecreateTextureCommandState = MF_ENABLED;
    }
    // enable recreate texture and set as current texture commands if texture right-clicked
    pPopup->EnableMenuItem(ID_RECREATE_TEXTURE, iRecreateTextureCommandState);
    pPopup->EnableMenuItem(ID_SET_AS_CURRENT_TEXTURE, iRecreateTextureCommandState);
    pPopup->EnableMenuItem(ID_EXPORT_TEXTURE, iRecreateTextureCommandState);

    UINT iSelectForDropMarkerCommandState = MF_DISABLED|MF_GRAYED;
    if( _fnRightClickedItemFileName.FileExt()==CTString(".ecl"))
    {
      iSelectForDropMarkerCommandState = MF_ENABLED;
    }
    // enable recreate texture command if texture right-clicked
    pPopup->EnableMenuItem(ID_SELECT_FOR_DROP_MARKER, iSelectForDropMarkerCommandState);
    if( pDoc == NULL)
    {
      pPopup->EnableMenuItem(ID_ADD_TEXTURES_FROM_WORLD, MF_DISABLED|MF_GRAYED);
      pPopup->EnableMenuItem(ID_SELECT_EXCEPT_TEXTURES, MF_DISABLED|MF_GRAYED);
    }

    // enable select by texture commands only if texture is right-clicked and document exists
    UINT iSelectByTextureCommandState = MF_DISABLED|MF_GRAYED;
    if( (_fnRightClickedItemFileName.FileExt()==CTString(".tex")) && (pDoc!=NULL))
    {
      iSelectByTextureCommandState = MF_ENABLED;
    }

    UINT iConvertClassCommandState = MF_DISABLED|MF_GRAYED;
    if( (_fnRightClickedItemFileName.FileExt()==CTString(".ecl")) &&
        (pDoc!=NULL) &&
        (pDoc->m_selEntitySelection.Count() != 0) &&
        (pDoc->m_iMode == ENTITY_MODE) )
    {
      iConvertClassCommandState = MF_ENABLED;
    }
    pPopup->EnableMenuItem(ID_SELECT_BY_TEXTURE_IN_SELECTED_SECTORS, iSelectByTextureCommandState);
    pPopup->EnableMenuItem(ID_SELECT_BY_TEXTURE_IN_WORLD, iSelectByTextureCommandState);
    pPopup->EnableMenuItem(ID_CONVERT_CLASS, iConvertClassCommandState);
    
    UINT iClassHelp = MF_DISABLED|MF_GRAYED;
    if( _fnRightClickedItemFileName.FileExt()==CTString(".ecl"))
    {
      iClassHelp = MF_ENABLED;
    }
    pPopup->EnableMenuItem(ID_BROWSER_CONTEXT_HELP, iClassHelp);

    pPopup->TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
								 point.x, point.y, this);
  }
}

INDEX CBrowseWindow::HitItem( CPoint point, FLOAT &fHitXOffset, FLOAT &fHitYOffset) const
{
  PIXaabbox2D rectItem;
  PIXaabbox2D boxPoint;
  INDEX i;
  INDEX iYOffset = GetScrollPos( SB_VERT);
  INDEX iLineVisible = iYOffset/m_IconHeight;
  PIX pixStart = iYOffset%m_IconHeight;
  for( i=0; i<m_IconsVisible; i++)
  {
    CVirtualTreeNode *pVTN = GetItem( i + iLineVisible*m_IconsInLine);
    if( pVTN == NULL)
    {
      break;
    }
    PIX x = i%m_IconsInLine * m_IconWidth;
    PIX y = i/m_IconsInLine * m_IconHeight - pixStart + ICONS_TRAY_HEIGHT + 1;
    rectItem = PIXaabbox2D( PIX2D(x, y), PIX2D(x+m_IconWidth, y+m_IconHeight));
    boxPoint = PIXaabbox2D( PIX2D(point.x, point.y) );
    if( (rectItem & boxPoint) == boxPoint)
    {
      fHitXOffset = FLOAT((point.x-x))/m_IconWidth;
      fHitYOffset = FLOAT((point.y-y))/m_IconHeight;
      return (i + iLineVisible*m_IconsInLine);
    }
  }
  return -1;
}

HGLOBAL CreateHDrop( const CTFileName &fnToDrag, BOOL bAddAppPath/*=TRUE*/)
{
  CTFileName fnFullToDrag;
  if( bAddAppPath)
  {
    fnFullToDrag = _fnmApplicationPath + fnToDrag;
  }
  else
  {
    fnFullToDrag = fnToDrag;
  }

  HGLOBAL hGlobal;
	// allocate space for DROPFILE structure plus the number of file and one extra byte for final NULL terminator
	hGlobal = GlobalAlloc(GHND|GMEM_SHARE,(DWORD) (sizeof(DROPFILES)+strlen(fnFullToDrag)+2));
	if(hGlobal == NULL)
			return hGlobal;

	LPDROPFILES pDropFiles;
  char *pchDropFileName;
	pDropFiles = (LPDROPFILES)GlobalLock(hGlobal);
  pchDropFileName = ((char *)pDropFiles)+sizeof(DROPFILES);
  // set the offset where the starting point of the file start
  pDropFiles->pFiles = sizeof(DROPFILES);
	// filename does not contain wide characters
  pDropFiles->fWide = FALSE;
  // we want drop point's coordinates in client area
  pDropFiles->fNC = FALSE;

	strcpy(pchDropFileName, (const char *)fnFullToDrag);
	// final null terminator as per CF_HDROP Format specs.
	pchDropFileName[strlen(pchDropFileName)+1]=0;
	GlobalUnlock(hGlobal);
 	return hGlobal;
}


void CBrowseWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL) return;

  // if we hitted icons tray
  if( (point.x < ICONS_TRAY_WIDTH*6) && (point.y < (ICONS_TRAY_HEIGHT + 1)) )
  {
    INDEX iSelected = point.x/ICONS_TRAY_WIDTH;
    switch( iSelected)
    {
    case 0: pVTNDir->vtn_bmBrowsingMode = BM_ICONS_MICRO; break;
    case 1: pVTNDir->vtn_bmBrowsingMode = BM_ICONS_SMALL; break;
    case 2: pVTNDir->vtn_bmBrowsingMode = BM_ICONS_MEDIUM; break;
    case 3: pVTNDir->vtn_bmBrowsingMode = BM_ICONS_LARGE; break;
    case 4: pVTNDir->vtn_bmBrowsingMode = BM_DESCRIPTION; break;
    case 5: pVTNDir->vtn_bmBrowsingMode = BM_FILENAME; break;
    }
    Refresh();
    return;
  }

  // toggle hitted icon selection status
  FLOAT fDummyX, fDummyY;
  INDEX iHittedItem = HitItem( point, fDummyX, fDummyY);
  if( iHittedItem == -1) return;

  BOOL bShift = (nFlags & MK_SHIFT);
  BOOL bCtrl = (nFlags & MK_CONTROL);

  if( !bCtrl)
  {
    FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
    {
      it->vtn_bSelected = FALSE;
    }
  }

  INDEX ctItems = 0;
  {FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    if( !it->vtn_bIsDirectory) ctItems++;
  }}

  INDEX iMin = iHittedItem;
  INDEX iMax = iHittedItem;
  if( bShift)
  {
    iMin = ClampDn( Min( m_iLastHittedItem, iHittedItem), (INDEX)0);
    iMax = ClampUp( Max( m_iLastHittedItem, iHittedItem), ctItems);
  }

  INDEX iCurrent = 0;
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    CVirtualTreeNode &vtn = *it;
    if( !vtn.vtn_bIsDirectory)
    {
      if( (iCurrent >= iMin) && (iCurrent <= iMax) )
      {
        if( bCtrl&&!bShift)
        {
          vtn.vtn_bSelected = !vtn.vtn_bSelected;
        }
        else
        {
          vtn.vtn_bSelected = TRUE;
        }
      }
      iCurrent++;
    }
  }
  if( !bShift) m_iLastHittedItem = iHittedItem;

  CVirtualTreeNode *pVTNHit = GetItem( iHittedItem);
  HGLOBAL hglobal = CreateHDrop( pVTNHit->vtn_fnItem);
  m_DataSource.CacheGlobalData( CF_HDROP, hglobal);
  m_DataSource.DoDragDrop( DROPEFFECT_COPY);
  Invalidate( FALSE);
}

void CBrowseWindow::OnDropFiles(HDROP hDropInfo)
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  INDEX iNoOfFiles = DragQueryFile( hDropInfo, 0xFFFFFFFF, NULL, 0);
	char chrFile[ 256];

  // get dropped coordinates
  CPoint point;
  DragQueryPoint( hDropInfo, &point);

  CVirtualTreeNode *pVTN = m_pBrowser->GetSelectedDirectory();
  if( pVTN != NULL)
  {
    CloseDirectory( pVTN);
    for( INDEX i=0; i<iNoOfFiles; i++)
    {
      DragQueryFileA( hDropInfo, i, chrFile, 256);
      CTFileName fnDroped = CTString(chrFile);
      if( fnDroped != CTString("") )
      {
        try
        {
          fnDroped.RemoveApplicationPath_t();
          InsertItem( fnDroped, point);
        }
        catch (const char *err_str)
        {
          AfxMessageBox( CString(err_str));
        }
      }
    }
    OpenDirectory( pVTN);
  }
  else
  {
    AfxMessageBox( L"ERROR: Virtual tree doesn't exist.");
  }
  CWnd::OnDropFiles(hDropInfo);
}

CVirtualTreeNode *CBrowseWindow::GetItem( INDEX iItem) const
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL)
  {
    return NULL;
  }
  INDEX ct=0;
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    //
    if( !it->vtn_bIsDirectory)
    {
      if( ct == iItem)
      {
        return( &it.Current());
      }
      ct++;
    }
  }
  return NULL;
}

INDEX CBrowseWindow::GetItemNo( CVirtualTreeNode *pVTN)
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL)
  {
    return -1;
  }
  if( pVTNDir->vtn_lhChildren.IsEmpty())
  {
    return -1;
  }

  INDEX ct=0;
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    if( !it->vtn_bIsDirectory)
    {
      if( &it.Current() == pVTN)
      {
        return( ct);
      }
      ct++;
    }
  }
  return -1;
}

void CBrowseWindow::InsertItem( CTFileName fnItem, CPoint pt)
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL)
  {
    return;
  }

  FLOAT fHitXRatio;
  FLOAT fHitYRatio;
  INDEX iHittedItem = HitItem( pt, fHitXRatio, fHitYRatio);
  CVirtualTreeNode *pVTNHit = NULL;
  if( iHittedItem != -1)
  {
    pVTNHit = GetItem( iHittedItem);
  }

  CVirtualTreeNode *pvtnToRemove = NULL;
  // check for all items in current virtual tree directory
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    // if it isn't directory means that it is item
    if( !it->vtn_bIsDirectory)
    {
      // don't allow inserting same item twice
      if( it->vtn_fnItem == fnItem)
      {
        if( pt.x == -1) return;
        pvtnToRemove = &it.Current();
      }
    }
  }

  if( (pVTNHit == pvtnToRemove) && (pVTNHit != NULL) ) return;
  if( pvtnToRemove != NULL)
  {
    pvtnToRemove->vtn_lnInDirectory.Remove();
    if( pvtnToRemove->vtn_pTextureData != NULL)
    {
      _pTextureStock->Release( pvtnToRemove->vtn_pTextureData);
    }
    delete pvtnToRemove;
  }

  // if item is texture
  if( fnItem.FileExt() == ".tex")
  {
    CTextureData *ptdTexture;
    // try to
    try
    {
      // obtain texture
      ptdTexture = _pTextureStock->Obtain_t( fnItem);
    }
    // catch and
    catch (const char *err_str)
    {
      // report errors
      AfxMessageBox( CString(err_str));
      return;
    }
    // now it must be valid
    ASSERT( ptdTexture != NULL);
    // get texture dimensions
    MEX mexWidth = ptdTexture->GetWidth();
    MEX mexHeight = ptdTexture->GetHeight();
    // release texture, we don't need it any more
    _pTextureStock->Release( ptdTexture);
    // mark both dimensions as incorrect
    BOOL bWidthOk = FALSE;
    BOOL bHeightOk = FALSE;
    // see if both width and height are potentions of 2
    for( INDEX i=0; i<32; i++)
    {
      // check width, mark if correct
      if( (1L << i) == mexWidth)  bWidthOk  = TRUE;
      if( (1L << i) == mexHeight) bHeightOk = TRUE;
    }
    // if width or height are not potentios of 2
    if( !bWidthOk || !bHeightOk)
    {
      char err_str[ 256];
      sprintf( err_str, "Dropped texture \"%s\" has incorrect dimensions %.2f x %.2f."
                        "All textures must have dimensions that are potentions of 2.",
                        (CTString&)fnItem, METERS_MEX( mexWidth), METERS_MEX( mexHeight));
      AfxMessageBox( CString(err_str));
      return;
    }
  }
  else if( fnItem.FileExt() == ".ecl")
  {
    // obtain class
    CEntityClass *pec = _pEntityClassStock->Obtain_t( fnItem);
    // get thumbnail file name from the class
    CTFileName fnThumbnail = CTString(pec->ec_pdecDLLClass->dec_strIconFileName);
    // release class
    _pEntityClassStock->Release( pec);
    // if thumbnail's name is "", don't add this item
    if( fnThumbnail == CTString("") )
    {
      return;
    }
  }

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CVirtualTreeNode *pVTN = new CVirtualTreeNode;
  pVTN->vtn_fnItem = fnItem;
  pVTN->vtn_bIsDirectory = FALSE;

  pVTN->vtn_strName = fnItem.FileName();
  if( pVTNDir->vtn_lhChildren.Count() == 0)
  {
    pVTN->vtn_bSelected = TRUE;
  }
  if( pt.x == -1)
  {
    pVTNDir->vtn_lhChildren.AddTail( pVTN->vtn_lnInDirectory);
  }
  else
  {
    if( iHittedItem == -1)
    {
      pVTNDir->vtn_lhChildren.AddTail( pVTN->vtn_lnInDirectory);
    }
    else if( ((m_IconsInLine != 1) && (fHitXRatio < 0.5f)) ||
             ((m_IconsInLine == 1) && (fHitYRatio < 0.5f)) )
    {
      ASSERT( pVTNHit != NULL);
      pVTNHit->vtn_lnInDirectory.AddBefore( pVTN->vtn_lnInDirectory);
    }
    else
    {
      ASSERT( pVTNHit != NULL);
      pVTNHit->vtn_lnInDirectory.AddAfter( pVTN->vtn_lnInDirectory);
    }
  }

  m_pBrowser->m_bVirtualTreeChanged = TRUE;
  Invalidate(FALSE);
}

void CBrowseWindow::DeleteSelectedItems()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL)
  {
    return;
  }

  FORDELETELIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    if( !it->vtn_bIsDirectory)
    {
      if( it->vtn_bSelected)
      {
        it->vtn_lnInDirectory.Remove();
        if( it->vtn_pTextureData != NULL)
        {
          _pTextureStock->Release( it->vtn_pTextureData);
        }
        delete &it.Current();
      }
    }
  }
  m_pBrowser->m_bVirtualTreeChanged = TRUE;
  Invalidate(FALSE);
}

void CBrowseWindow::Refresh(void)
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( (pVTNDir == NULL) || (!m_bDirectoryOpen) )
  {
    return;
  }
  switch( pVTNDir->vtn_bmBrowsingMode)
  {
  case BM_ICONS_MICRO:
    {
      m_IconWidth  = 16;
      m_IconHeight = 16;
      break;
    }
  case BM_ICONS_SMALL:
    {
      m_IconWidth  = 32;
      m_IconHeight = 32;
      break;
    }
  case BM_ICONS_MEDIUM:
    {
      m_IconWidth  = 64;
      m_IconHeight = 64;
      break;
    }
  case BM_ICONS_LARGE:
    {
      m_IconWidth  = 128;
      // allow some space for text
      m_IconHeight = 128 + STRING_HEIGHT;
      break;
    }
  case BM_DESCRIPTION:
  case BM_FILENAME:
    {
      m_IconWidth  = m_BrowseWndWidth - 8;
      m_IconHeight = STRING_HEIGHT;
      break;
    }
  default:
    {
      ASSERTALWAYS( "Unrecognizible browsing type found !");
    }
  }
  m_IconsInLine = m_BrowseWndWidth/m_IconWidth;
  if( m_IconsInLine == 0)
  {
    m_IconsInLine = 1;
  }
  m_IconsInColumn = m_BrowseWndHeight/m_IconHeight + 1;
  m_IconsVisible = m_IconsInLine * m_IconsInColumn;

  INDEX iItemsCt = -1;
  if( !pVTNDir->vtn_lhChildren.IsEmpty())
  {
    iItemsCt = GetItemNo( LIST_TAIL( pVTNDir->vtn_lhChildren,
                                     CVirtualTreeNode, vtn_lnInDirectory) );
  }
  INDEX ctLines = iItemsCt/m_IconsInLine;
  SetScrollRange( SB_VERT, 0, ctLines*m_IconHeight);
  Invalidate(FALSE);
}

void CBrowseWindow::OpenDirectory( CVirtualTreeNode *pVTNDir)
{
  CWorldEditorApp *pApp = (CWorldEditorApp *)AfxGetApp();
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  if(m_bDirectoryOpen || pVTNDir==NULL)
  {
    return;
  }

  ASSERT( pVTNDir != NULL);
  // remember name of last opened virtual tree
  theApp.m_strOpenedVTDirectory = theApp.GetNameForVirtualTreeNode( pVTNDir);
  wchar_t achrOpenedDirectoryMessage[ 256];
  swprintf( achrOpenedDirectoryMessage, L"Opened directory: \"%s\"",
    CString(theApp.m_strOpenedVTDirectory));
  // put selected directory name into status line
  pMainFrame->m_wndStatusBar.SetPaneText( STATUS_LINE_PANE, achrOpenedDirectoryMessage, TRUE);

  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
  {
    try
    {
      // if not directory
      if( !it->vtn_bIsDirectory)
      {
        CTFileName fnThumbnail;              // thumbnail's file name
        // if texture
        if( it->vtn_fnItem.FileExt() == ".tex")
        {
          // use same texture for thumbnail
          fnThumbnail = it->vtn_fnItem;
        }
        // if world
        else if ( it->vtn_fnItem.FileExt() == ".wld")
        {
          // use file name with extension .tbn for thumbnail
          fnThumbnail = it->vtn_fnItem.FileDir() + it->vtn_fnItem.FileName() + ".tbn";
          // use base name for description
          it->vtn_strName = it->vtn_fnItem.FileName();
        }
        // if class
        else if ( it->vtn_fnItem.FileExt() == ".ecl")
        {
          // obtain class
          CEntityClass *pec = _pEntityClassStock->Obtain_t( it->vtn_fnItem);
          // get thumbnail file name from the class
          fnThumbnail = CTString(pec->ec_pdecDLLClass->dec_strIconFileName);
          // get description name from the class
          it->vtn_strName = pec->ec_pdecDLLClass->dec_strName;
          // release class
          _pEntityClassStock->Release( pec);
        }
        // if unknown extension
        else
        {
          // use no thumbnail
          fnThumbnail = CTString("");
        }

        // if no thumbnail
        if( fnThumbnail == "")
        {
          it->vtn_pTextureData = NULL;
        }
        // if there is valid thumbnail file name
        else
        {
          // obtain thumbnail
          it->vtn_pTextureData = _pTextureStock->Obtain_t( fnThumbnail);
          // must be valid
          ASSERT( it->vtn_pTextureData != NULL);
          // if it is really texture, type full info
          if( it->vtn_fnItem.FileExt() == ".tex")
          {
            // use base name and dimension for desription
            it->vtn_strName = (CTString&)it->vtn_fnItem+" "+it->vtn_pTextureData->GetDescription();
          }
          // else description is just item's file name
          else
          {
            it->vtn_strName = it->vtn_fnItem.FileName();
          }
        }
      }
    }
    catch (const char *error)
    {
      // ingnore errors
      (void) error;
      it->vtn_pTextureData = NULL;
    }
  }

  SetScrollPos(SB_VERT, 0);
  m_bDirectoryOpen = TRUE;
  Refresh();
  Invalidate(FALSE);
}

void CBrowseWindow::CloseDirectory( CVirtualTreeNode *pVTN)
{
  if( !m_bDirectoryOpen)
  {
    return;
  }
  ASSERT( pVTN != NULL);
  FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTN->vtn_lhChildren, it)
  {
    if( !it->vtn_bIsDirectory)
    {
      if( it->vtn_pTextureData != NULL)
      {
        _pTextureStock->Release( it->vtn_pTextureData);
        it->vtn_pTextureData = NULL;
      }
    }
  }
  m_bDirectoryOpen = FALSE;
}

void CBrowseWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

  // if window canvas is valid
  if( m_pViewPort!=NULL)
  {
		// resize it
    m_pViewPort->Resize();
    Refresh();
  }
}

void CBrowseWindow::OnInsertItems()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  if( pVTNDir == NULL)
  {
    AfxMessageBox( L"ERROR: Virtual tree doesn't exist.");
    return;
  }

  char *pFilters = "Items (*.tex, *.wld, *.ecl)\0*.tex;*.wld;*.ecl\0"
                         "World Files (*.wld)\0*.wld\0"
                         "Texture files (*.tex)\0*.tex\0"
                         "Class files (*.ecl)\0*.ecl\0"
                         "All files (*.*)\0*.*\0\0";
  // call file requester for opening textures
  CDynamicArray<CTFileName> afnItems;
  _EngineGUI.FileRequester( "Insert items", pFilters, KEY_NAME_CREATE_TEXTURE_DIR,
                        "Textures\\", "", &afnItems);
  if( afnItems.Count() == 0) return;

  // insert items
  FOREACHINDYNAMICARRAY( afnItems, CTFileName, itItem)
  {
    try
    {
      InsertItem( *itItem, CPoint(-1, -1));
    }
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
    }
  }
  CloseDirectory( pVTNDir);
  OpenDirectory( pVTNDir);
}

void CBrowseWindow::OnDeleteItems()
{
	DeleteSelectedItems();
}

void CBrowseWindow::OnBigIcons()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  ASSERT( pVTNDir != NULL);
	pVTNDir->vtn_bmBrowsingMode = BM_ICONS_LARGE;
  Refresh();
}

void CBrowseWindow::OnMediumIcons()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  ASSERT( pVTNDir != NULL);
	pVTNDir->vtn_bmBrowsingMode = BM_ICONS_MEDIUM;
  Refresh();
}

void CBrowseWindow::OnMicroIcons()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  ASSERT( pVTNDir != NULL);
	pVTNDir->vtn_bmBrowsingMode = BM_ICONS_MICRO;
  Refresh();
}

void CBrowseWindow::OnSmallIcons()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  ASSERT( pVTNDir != NULL);
	pVTNDir->vtn_bmBrowsingMode = BM_ICONS_SMALL;
  Refresh();
}

void CBrowseWindow::OnShowDescription()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  ASSERT( pVTNDir != NULL);
	pVTNDir->vtn_bmBrowsingMode = BM_DESCRIPTION;
  Refresh();
}

void CBrowseWindow::OnShowFilename()
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  ASSERT( pVTNDir != NULL);
	pVTNDir->vtn_bmBrowsingMode = BM_FILENAME;
  Refresh();
}

void CBrowseWindow::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // get hitted item's index
  FLOAT fDummyX, fDummyY;
  INDEX iHittedItem = HitItem( point, fDummyX, fDummyY);
  // if hit none reported, return
  if( iHittedItem == -1)
  {
    return;
  }

  // get item object from hitted index
  CVirtualTreeNode *pVTN = GetItem( iHittedItem);
  // must not be null
  ASSERT( pVTN != NULL);
  // get item's file name
  CTFileName fnItem = pVTN->vtn_fnItem;
  // if it is texture
  if( fnItem.FileExt() == ".tex")
  {
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    if(pDoc!=NULL && pDoc->GetEditingMode()==TERRAIN_MODE)
    {
      CTerrainLayer *ptlLayer=GetLayer();
      CTerrain *ptTerrain=GetTerrain();
      if(ptlLayer!=NULL && ptTerrain!=NULL)
      {
        try
        {
          ptlLayer->SetLayerTexture_t(fnItem);
          theApp.m_ctTerrainPageCanvas.MarkChanged();
          ptTerrain->RefreshTerrain();
        }
        catch (const char *strError)
        {
          (void) strError;
        }
      }
    }
    else
    {
      // set it as new primitive's material default texture
      theApp.SetNewActiveTexture( _fnmApplicationPath + fnItem);
      // paste new active texture over polygon selection
      theApp.TexturizeSelection();
    }
  }
  // if it is world (template)
  else if( fnItem.FileExt() == ".wld")
  {
  	// open document with item's file name
    theApp.m_pDocTemplate->OpenDocumentFile( CString(_fnmApplicationPath + fnItem));
  }
}

void CBrowseWindow::OnRecreateTexture()
{
  if( _bRightClickedIsSelected)
  {
    CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
    ASSERT( pVTNDir != NULL);
    FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
    {
      if( it->vtn_bSelected && it->vtn_fnItem.FileExt()==CTString(".tex") )
      {
        _EngineGUI.CreateTexture( it->vtn_fnItem);
      }
    }
  }
  else if( _fnRightClickedItemFileName.FileExt()==CTString(".tex"))
  {
    _EngineGUI.CreateTexture( _fnRightClickedItemFileName);
  }
  Refresh();

  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc != NULL) pDoc->UpdateAllViews( NULL);
}

void CBrowseWindow::OnSelectByTextureInSelectedSectors()
{
  SelectByTextures( TRUE, FALSE);
}

void CBrowseWindow::OnCreateAndAddTexture()
{
  CDynamicArray<CTFileName> afnCreatedTextures;
  CTFileName fnCreatedTexture = _EngineGUI.CreateTexture( CTString(""), &afnCreatedTextures);
  if( afnCreatedTextures.Count() != 0)
  {
    CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
    if( pVTNDir != NULL)
    {
      CloseDirectory( pVTNDir);
    }

    // insert created textures
    FOREACHINDYNAMICARRAY( afnCreatedTextures, CTFileName, itTexture)
    {
      CTFileName &fn=*itTexture;
      InsertItem( fn, CPoint(-1, -1));
    }
    if( pVTNDir != NULL)
    {
      OpenDirectory( pVTNDir);
    }
  }
}


void CBrowseWindow::OnSelectForDropMarker()
{
  theApp.m_fnClassForDropMarker = _fnRightClickedItemFileName;
}

void CBrowseWindow::OnSetAsCurrentTexture()
{
  // set it as new primitive's material default texture
  theApp.SetNewActiveTexture( _fnmApplicationPath + _fnRightClickedItemFileName);
}

void CBrowseWindow::OnConvertClass()
{
  if( _fnRightClickedItemFileName.FileExt()!=CTString(".ecl"))
  {
    WarningMessage( "Only classes can be used for converting classe");
    return;
  }

  try
  {
    CWorldEditorDoc *pDoc = theApp.GetDocument();
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      // create the entity of requested class
      CEntity *penNewClass;
      penNewClass = pDoc->m_woWorld.CreateEntity_t(
        iten->GetPlacement(), _fnRightClickedItemFileName);
      // try to copy entity properties
      CDLLEntityClass *pdecDLLClassNew = penNewClass->GetClass()->ec_pdecDLLClass;
      for(;pdecDLLClassNew!=NULL; pdecDLLClassNew = pdecDLLClassNew->dec_pdecBase)
      {
        for(INDEX iPropertyNew=0; iPropertyNew<pdecDLLClassNew->dec_ctProperties; iPropertyNew++)
        {
          CEntityProperty &epPropertyNew = pdecDLLClassNew->dec_aepProperties[iPropertyNew];
          CDLLEntityClass *pdecDLLClassOld = iten->GetClass()->ec_pdecDLLClass;
          for(;pdecDLLClassOld!=NULL; pdecDLLClassOld = pdecDLLClassOld->dec_pdecBase)
          {
            for(INDEX iPropertyOld=0; iPropertyOld<pdecDLLClassOld->dec_ctProperties; iPropertyOld++)
            {
              CEntityProperty &epPropertyOld = pdecDLLClassOld->dec_aepProperties[iPropertyOld];
              if( (CTString(epPropertyNew.ep_strName) == epPropertyOld.ep_strName) &&
                  (epPropertyNew.ep_eptType == epPropertyOld.ep_eptType) )
              {
                penNewClass->CopyOneProperty( epPropertyOld, epPropertyNew, *iten, FALSE);
              }
            }
          }
        }
      }
      // prepare the entity
      penNewClass->Initialize();
    }
    CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
    if( pWorldEditorView != NULL)
    {
      pWorldEditorView->OnDeleteEntities();
    }
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

static void GetToolTipText(void *pBrowser, char *pToolTipText)
{
  CBrowseWindow *pBrowseWindow = (CBrowseWindow *) pBrowser;
  pBrowseWindow->GetToolTipText( pToolTipText);
}

void CBrowseWindow::GetToolTipText( char *pToolTipText)
{
  CPoint point;
  ::GetCursorPos(&point);
  ScreenToClient(&point);

  // get hitted item's index
  FLOAT fDummyX, fDummyY;
  INDEX iHittedItem = HitItem( point, fDummyX, fDummyY);
  // if hit none reported, return
  if( iHittedItem == -1)
  {
    strcpy( pToolTipText, "");
    return;
  }
  // get item object from hitted index
  CVirtualTreeNode *pVTN = GetItem( iHittedItem);
  strcpy(pToolTipText, pVTN->vtn_strName);
}

void CBrowseWindow::OnMouseMove(UINT nFlags, CPoint point)
{
  theApp.m_cttToolTips.MouseMoveNotify( m_hWnd, 500, &::GetToolTipText, this);

	CWnd::OnMouseMove(nFlags, point);
}

void CBrowseWindow::OnSelectByTextureInWorld()
{
  SelectByTextures( FALSE, FALSE);
}

void CBrowseWindow::OnSelectExceptTextures( void) 
{
  SelectByTextures( FALSE, TRUE);
}

void CBrowseWindow::SelectByTextures( BOOL bInSelectedSectors, BOOL bExceptSelected)
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;
  
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // if sector is selected
          if( !bInSelectedSectors || itbsc->IsSelected(BSCF_SELECTED))
          {
            // for all polygons in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
            {
              // if it is not non translucent portal and is not selected and has same texture
              if ( /*(!(itbpo->bpo_ulFlags&BPOF_PORTAL) || (itbpo->bpo_ulFlags&BPOF_TRANSLUCENT) ||
                     (itbpo->bpo_bppProperties.bpp_uwPretenderDistance!=0) ) &&*/
                  !itbpo->IsSelected(BPOF_SELECTED) &&
                  (itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData() != NULL) )
              {
                CTFileName fnTexture = itbpo->bpo_abptTextures[pDoc->m_iTexture].bpt_toTexture.GetData()->GetName();
                BOOL bSelect = bExceptSelected;
                FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
                {
                  if( _bRightClickedIsSelected)
                  {
                    if(it->vtn_bSelected && it->vtn_fnItem.FileExt()==CTString(".tex") &&
                       fnTexture == it->vtn_fnItem)
                    {
                      bSelect = !bExceptSelected;
                      break;
                    }
                  }
                  else if( fnTexture == _fnRightClickedItemFileName)
                  {
                    bSelect = !bExceptSelected;
                    break;
                  }
                }
                if( bSelect) pDoc->m_selPolygonSelection.Select(*itbpo);
              }
            }
          }
        }
      }
    }
  }
  pDoc->SetEditingMode( POLYGON_MODE);
  pDoc->m_chSelections.MarkChanged();
  pDoc->UpdateAllViews( NULL);
}

void CBrowseWindow::OnAddTexturesFromWorld() 
{
  CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if (pDoc == NULL) return;
  
  CloseDirectory( pVTNDir);
  // for each entity in the world
  FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten) {
    // if it is brush entity
    if (iten->en_RenderType == CEntity::RT_BRUSH) {
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
          {
            try
            {  
              CTextureObject &to1 = itbpo->bpo_abptTextures[0].bpt_toTexture;
              if(to1.GetData() != NULL)
                InsertItem( to1.GetData()->GetName(), CPoint(-1, -1));
              CTextureObject &to2 = itbpo->bpo_abptTextures[1].bpt_toTexture;
              if(to2.GetData() != NULL)
                InsertItem( to2.GetData()->GetName(), CPoint(-1, -1));
              CTextureObject &to3 = itbpo->bpo_abptTextures[2].bpt_toTexture;
              if(to3.GetData() != NULL)
                InsertItem( to3.GetData()->GetName(), CPoint(-1, -1));
            }
            catch (const char *err_str)
            {
              AfxMessageBox( CString(err_str));
            }
          }
        }
      }
    }
  }
  OpenDirectory( pVTNDir);
}

void CBrowseWindow::OnShowTreeShortcuts() 
{
	CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->OnShowTreeShortcuts();
}

void ExportTexture( CTFileName fnTexture)
{
  CTextureData *ptd;
  CImageInfo ii;

  try
  {
    ptd = _pTextureStock->Obtain_t( fnTexture);
    for( INDEX iFrame=0; iFrame<ptd->td_ctFrames; iFrame++)
    {
      ptd->Export_t( ii, iFrame);
      // obtain name for export file
      CTFileName fnFrame;
      if( ptd->td_ctFrames == 1)
      {
        fnFrame = fnTexture.NoExt()+".tga";
      }
      else
      {
        fnFrame.PrintF("%s%03d.tga", (const char *)fnTexture.NoExt(), iFrame);
      }
      // if file exists, ask for substitution name
      if( FileExists( fnFrame) && iFrame==0 )
      {
        CTString strDefaultDir = fnFrame.FileDir();
        CTString strDefaultFile = fnFrame.FileName()+fnFrame.FileExt();
        // invoke "Save as" dialog
        fnFrame = _EngineGUI.FileRequester( "Save As", FILTER_TGA FILTER_END,
                  "Export texture directory", strDefaultDir, strDefaultFile, NULL, FALSE);
      }
      if( fnFrame != "")
      {
        ii.SaveTGA_t(fnFrame);
        ii.Clear();
      }
    }
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    ii.Clear();
  }
}

void CBrowseWindow::OnExportTexture() 
{
  if( _bRightClickedIsSelected)
  {
    CVirtualTreeNode *pVTNDir = m_pBrowser->GetSelectedDirectory();
    ASSERT( pVTNDir != NULL);
    FOREACHINLIST( CVirtualTreeNode, vtn_lnInDirectory, pVTNDir->vtn_lhChildren, it)
    {
      if( it->vtn_bSelected && it->vtn_fnItem.FileExt()==CTString(".tex") ||
          it->vtn_bSelected && it->vtn_fnItem.FileExt()==CTString(".tbn") )
      {
        ExportTexture( it->vtn_fnItem);
      }
    }
  }
  else if( _fnRightClickedItemFileName.FileExt()==CTString(".tex") ||
           _fnRightClickedItemFileName.FileExt()==CTString(".tbn") )
  {
    ExportTexture( _fnRightClickedItemFileName);
  }
}

void CBrowseWindow::OnBrowserContextHelp() 
{
  if( _fnRightClickedItemFileName.FileExt()==CTString(".ecl"))
  {
    theApp.DisplayHelp(_fnRightClickedItemFileName, HH_DISPLAY_TOPIC, NULL);
  }
}
