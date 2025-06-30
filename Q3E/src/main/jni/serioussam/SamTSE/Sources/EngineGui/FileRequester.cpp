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

#include "EngineGui/StdH.h"
#include <Engine/Templates/Stock_CTextureData.h>

// thumbnail window
static CWnd _wndThumbnail;
CDrawPort *_pDrawPort = NULL;
CViewPort *_pViewPort = NULL;
static INDEX gui_bEnableRequesterThumbnails=TRUE;

UINT APIENTRY FileOpenRequesterHook( HWND hdlg, UINT uiMsg, WPARAM wParam,	LPARAM lParam)
{
  _pShell->DeclareSymbol("persistent user INDEX gui_bEnableRequesterThumbnails;", &gui_bEnableRequesterThumbnails);
  if( !gui_bEnableRequesterThumbnails)
  {
    return 0;
  }

  CTextureData *pTextureData = NULL;
  if( uiMsg == WM_NOTIFY)
  {
    // obtain file open notification structure
    OFNOTIFY *pONNotify = (OFNOTIFY *) lParam;
    // obtain notification message header structure
    NMHDR *pNMHeader = &pONNotify->hdr;
    if(pNMHeader->code == CDN_INITDONE)
    {
      HWND hwnd = GetDlgItem( hdlg, IDC_THUMBNAIL_RECT);
      RECT rect;
      BOOL bSuccess = GetClientRect( hwnd, &rect);
      POINT pointParent;
      pointParent.x=0;
      pointParent.y=0;
      POINT pointThumbnail;
      pointThumbnail.x=0;
      pointThumbnail.y=0;
      ClientToScreen(hdlg, &pointParent);
      ClientToScreen(hwnd, &pointThumbnail);
      POINT point;
      point.x = pointThumbnail.x-pointParent.x;
      point.y = pointThumbnail.y-pointParent.y;
      OffsetRect(&rect, point.x, point.y);
      
      if( !bSuccess)
      {
        ASSERT( FALSE);
        return 0;
      }
      PIX pixLeft = 120;
      PIX pixTop = 250;
      // try to create thumbnail window
      _wndThumbnail.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rect,
                            CWnd::FromHandle(hdlg), IDW_FILE_THUMBNAIL);
      /*_wndThumbnail.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, 
                            CRect( pixLeft, pixTop, pixLeft+128, pixTop+128),
                            CWnd::FromHandle(hdlg), IDW_FILE_THUMBNAIL);
                            */
      // initialize canvas for thumbnail window
      _pGfx->CreateWindowCanvas( _wndThumbnail.m_hWnd, &_pViewPort, &_pDrawPort);
    }
    char strSelectedFullPath[ PATH_MAX];
    CTFileName fnSelectedFileFullPath;
    // obtain selected file's path
    CommDlg_OpenSave_GetFilePath(GetParent(hdlg), &strSelectedFullPath, PATH_MAX);
    // if multi select was turned on 
    fnSelectedFileFullPath = CTString(strSelectedFullPath);

    // try to
    try
    {
      // remove application path
      fnSelectedFileFullPath.RemoveApplicationPath_t();
      CTFileName fnThumbnail = CTString("");
      if( (fnSelectedFileFullPath.FileExt() == ".wld") ||
          (fnSelectedFileFullPath.FileExt() == ".mdl") )
      {
        fnThumbnail = fnSelectedFileFullPath.FileDir()+fnSelectedFileFullPath.FileName()+".tbn";
      }
      else if( (fnSelectedFileFullPath.FileExt() == ".tex") ||
               (fnSelectedFileFullPath.FileExt() == ".tbn") )
      {
        fnThumbnail = fnSelectedFileFullPath;
      }
      else if( (fnSelectedFileFullPath.FileExt() == ".pcx") ||
               (fnSelectedFileFullPath.FileExt() == ".tga") )
      {
        CImageInfo iiImageInfo;
        iiImageInfo.LoadAnyGfxFormat_t( fnSelectedFileFullPath);
        // both dimension must be potentions of 2
        if( (iiImageInfo.ii_Width  == 1<<((int)Log2( iiImageInfo.ii_Width))) &&
            (iiImageInfo.ii_Height == 1<<((int)Log2( iiImageInfo.ii_Height))) )
        {
          fnThumbnail = CTString( "Temp\\Temp.tex");
          // creates new texture with one frame
          CTextureData tdForPictureConverting;
          tdForPictureConverting.Create_t( &iiImageInfo, iiImageInfo.ii_Width, 1, FALSE);
          tdForPictureConverting.Save_t( fnThumbnail);
        }
      }
      if( fnThumbnail != "")
      {
        // obtain thumbnail
        pTextureData = _pTextureStock->Obtain_t( fnThumbnail);
        pTextureData->Reload();
      }
    }
    catch (const char* err_str)
    {
      (void)err_str;
      pTextureData = NULL;
    }

    if( IsWindow( _wndThumbnail) )
    {
      // if there is a valid drawport, and the drawport can be locked
      if( (_pDrawPort != NULL) && (_pDrawPort->Lock()) )
      {
        PIXaabbox2D rectPict;
        rectPict = PIXaabbox2D( PIX2D(0, 0),
                                PIX2D(_pDrawPort->GetWidth(), _pDrawPort->GetHeight()));
        // clear texture area to black
        _pDrawPort->Fill( C_BLACK|CT_OPAQUE);
        // erase z-buffer
        _pDrawPort->FillZBuffer(ZBUF_BACK);
        // if there is valid active texture
        if( pTextureData != NULL)
        {
          CTextureObject toPreview;
          toPreview.SetData( pTextureData);
          _pDrawPort->PutTexture( &toPreview, rectPict);
          CWnd::FromHandle( GetDlgItem( hdlg, IDC_THUMBNAIL_DESCRIPTION))->SetWindowText( 
            CString(pTextureData->GetDescription()));
          // release the texture
          _pTextureStock->Release( pTextureData);
        }
        else
        {
          // no thumbnail, draw crossed lines 
          PIX pixRad = Max( _pDrawPort->GetWidth(), _pDrawPort->GetWidth()) * 3/4/2;
          PIX CX = _pDrawPort->GetWidth()/2;
          PIX CY = _pDrawPort->GetWidth()/2;
          for( INDEX iPix=-2;iPix<2;iPix++)
          {
            _pDrawPort->DrawLine( CX-pixRad+iPix, CY-pixRad, CX+pixRad+iPix, CY+pixRad, C_RED|CT_OPAQUE);
            _pDrawPort->DrawLine( CX-pixRad+iPix, CY+pixRad, CX+pixRad+iPix, CY-pixRad, C_RED|CT_OPAQUE);
          }
          CWnd::FromHandle( GetDlgItem( hdlg, IDC_THUMBNAIL_DESCRIPTION))->SetWindowText( L"No thumbnail");
        }
        // unlock the drawport
        _pDrawPort->Unlock();
      }

      // if there is a valid viewport
      if (_pViewPort!=NULL)
      {
        // swap it
        _pViewPort->SwapBuffers();
      }
    }
  }
  return 0;
}

CTFileName CEngineGUI::FileRequester( 
        char *pchrTitle/*="Choose file"*/, 
        char *pchrFilters/*=FILTER_ALL FILTER_END*/,
        char *pchrRegistry/*="KEY_NAME_REQUEST_FILE_DIR"*/,
        CTString strDefaultDir/*=""*/, 
        CTString strFileSelectedByDefault/*=""*/,
        CDynamicArray<CTFileName> *pafnSelectedFiles/*=NULL*/,
        BOOL bIfOpen/*=TRUE*/)
{
  _pDrawPort = NULL;
  _pViewPort = NULL;
  // stupid way to change resources, but it must be done
  HANDLE hOldResource = AfxGetResourceHandle();
  // activate CTGfx resources
  AfxSetResourceHandle( GetModuleHandleA(ENGINEGUI_DLL_NAME) );

  // call multiple file requester
  char chrFiles[ 2048];
  OPENFILENAMEA ofnRequestFiles;

  memset( &ofnRequestFiles, 0, sizeof( OPENFILENAME));
  ofnRequestFiles.lStructSize = sizeof(OPENFILENAME);
  ofnRequestFiles.hwndOwner = AfxGetMainWnd()->m_hWnd;
  ofnRequestFiles.lpstrFilter = pchrFilters;
  ofnRequestFiles.lpstrFile = chrFiles;
  sprintf( chrFiles, "%s", strFileSelectedByDefault);
  ofnRequestFiles.nMaxFile = 2048;

  CString strRequestInDirectory = _fnmApplicationPath+strDefaultDir;
  if( pchrRegistry != NULL)
  {
    strRequestInDirectory = AfxGetApp()->GetProfileString(L"Scape", CString(pchrRegistry), 
      CString(_fnmApplicationPath+strDefaultDir));
  }

  // if directory is not inside engine dir
  CTString strTest = CStringA(strRequestInDirectory);
  if (!strTest.RemovePrefix(_fnmApplicationPath)) {
    // force it there
    strRequestInDirectory = _fnmApplicationPath;
  }
  

  ofnRequestFiles.lpstrInitialDir = CStringA(strRequestInDirectory);
  ofnRequestFiles.lpstrTitle = pchrTitle;
  ofnRequestFiles.Flags = OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_HIDEREADONLY;
  // setup preview dialog
  ofnRequestFiles.lpfnHook = (LPOFNHOOKPROC)&::FileOpenRequesterHook;
  ofnRequestFiles.hInstance = GetModuleHandleA(ENGINEGUI_DLL_NAME);
  ofnRequestFiles.lpTemplateName = MAKEINTRESOURCEA( IDD_GFX_FILE_REQUESTER);
  // allow multi select
  if( pafnSelectedFiles != NULL) ofnRequestFiles.Flags |= OFN_ALLOWMULTISELECT;
  
  ofnRequestFiles.lpstrDefExt = "";

  BOOL bResult;
  if( bIfOpen)
  {
    bResult = GetOpenFileNameA( &ofnRequestFiles);
  }
  else
  {
    bResult = GetSaveFileNameA( &ofnRequestFiles);
  }

  if( bResult)
  {
    // if multiple file requester is called
    if( pafnSelectedFiles != NULL)
    {
      chrFiles[ ofnRequestFiles.nFileOffset-1] = 0;
      if( pchrRegistry != NULL)
      {
        AfxGetApp()->WriteProfileString(L"Scape", CString(pchrRegistry), CString(chrFiles));
      }
      CTFileName fnDirectory = CTString( chrFiles) + "\\";

      INDEX iOffset = ofnRequestFiles.nFileOffset;
      FOREVER
      {
        try
        {
          if( CTString( chrFiles + iOffset) == "")
          {
            // restore resources
            AfxSetResourceHandle( (HINSTANCE) hOldResource);
            return( CTString( "Multiple files selected"));
          }
          CTFileName fnSource = fnDirectory + CTString( chrFiles + iOffset);
          // remove application path
          fnSource.RemoveApplicationPath_t();
          CTFileName *pfnSelectedFile = pafnSelectedFiles->New();
          *pfnSelectedFile = fnSource;
        }
        catch (const char *strError)
        {
          WarningMessage( strError);
          // restore resources
          AfxSetResourceHandle( (HINSTANCE) hOldResource);
          // return error
          return( CTString( ""));
        }
        iOffset += strlen( chrFiles + iOffset) + 1;
      }
    }
    else
    {
      CString strChooseFilePath = chrFiles;
      strChooseFilePath.SetAt( ofnRequestFiles.nFileOffset, 0);
      if( pchrRegistry != NULL)
      {
        AfxGetApp()->WriteProfileString(L"Scape", CString(pchrRegistry), strChooseFilePath);
      }
      CTFileName fnResult = CTString( chrFiles);
      try
      {
        fnResult.RemoveApplicationPath_t();
      }
      catch (const char *strError)
      {
        WarningMessage( strError);
        // restore resources
        AfxSetResourceHandle( (HINSTANCE) hOldResource);
        // return error
        return( CTString( ""));
      }
      // restore resources
      AfxSetResourceHandle( (HINSTANCE) hOldResource);
      return fnResult;
    }
  }
  if( _pViewPort != NULL)
  {
    _pGfx->DestroyWindowCanvas( _pViewPort);
    _pViewPort = NULL;
  }
  // restore resources
  AfxSetResourceHandle( (HINSTANCE) hOldResource);
  // return error
  return CTString( "");
}

ENGINEGUI_API CTFileName FileRequester(
  char *pchrTitle, 
  char *pchrFilters,
  char *pchrRegistry,
  char *pchrFileSelectedByDefault)
{
  return _EngineGUI.FileRequester(pchrTitle, pchrFilters, pchrRegistry, "", pchrFileSelectedByDefault);
}


CTFileName CEngineGUI::BrowseTexture(CTFileName fnDefaultSelected/*=""*/,
                                      char *pchrIniKeyName/*=KEY_NAME_REQUEST_FILE_DIR*/,
                                      char *pchrWindowTitle/*="Choose texture"*/,
                                      BOOL bIfOpen/*=TRUE*/)
{
  return FileRequester( pchrWindowTitle, FILTER_TEX FILTER_END, pchrIniKeyName,
                        "Textures\\", fnDefaultSelected, NULL, bIfOpen);
}

