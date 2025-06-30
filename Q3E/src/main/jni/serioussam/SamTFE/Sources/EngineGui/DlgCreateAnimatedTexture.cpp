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

// DlgCreateAnimatedTexture.cpp : implementation file
//

#include "EngineGui/StdH.h"
#include "DlgCreateAnimatedTexture.h"
#include <Engine/Templates/Stock_CTextureData.h>


#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateAnimatedTexture dialog


#define TEMPORARY_SCRIPT_NAME "Temp\\Temp.scr"
#define TEMPORARY_TEXTURE_NAME "Temp\\Temp.tex"


void CDlgCreateAnimatedTexture::ReleaseCreatedTexture(void)
{
  // if there is texture obtained, release it
  if( m_ptdCreated!= NULL)
  {
    // free obtained texture
    _pTextureStock->Release( m_ptdCreated);
    m_ptdCreated = NULL;
    m_wndViewCreatedTexture.m_toTexture.SetData( NULL);
  }
}

void CDlgCreateAnimatedTexture::InitAnimationsCombo(void)
{
  m_ctrlAnimationsCombo.ResetContent();
  if( m_ptdCreated != NULL)
  {
    CAnimInfo aiInfo;
    for( INDEX iAnim=0; iAnim<m_ptdCreated->GetAnimsCt(); iAnim++)
    {
      m_ptdCreated->GetAnimInfo( iAnim, aiInfo);
      m_ctrlAnimationsCombo.AddString(CString(aiInfo.ai_AnimName));
    }
  }
  else
  {
    m_ctrlAnimationsCombo.AddString( L"None");
  }
  m_ctrlAnimationsCombo.SetCurSel( 0);
}

void CDlgCreateAnimatedTexture::OnSelchangeTextureAnimations() 
{
  if( m_ptdCreated != NULL)
  {
    // set selected animation
    INDEX iAnim = m_ctrlAnimationsCombo.GetCurSel();
    m_wndViewCreatedTexture.m_toTexture.SetAnim( iAnim);
  }
}

void CDlgCreateAnimatedTexture::RefreshTexture(void)
{
  // refresh script string from edit control
  UpdateData( TRUE);
  // prepare names for temporary script and texture
  CTFileName fnTempScript = CTString(TEMPORARY_SCRIPT_NAME);
  CTFileName fnTemptexture = CTString(TEMPORARY_TEXTURE_NAME);
  try
  {
    // write context of edit ctrl to temporary script file
    CTFileStream fileScript;
    fileScript.Create_t( fnTempScript);
    CTString strEditScript = CStringA(m_strEditScript);
    char *pScript = (char *) AllocMemory( strlen(strEditScript)+1);
    strcpy( pScript, strEditScript);
    fileScript.WriteRawChunk_t( pScript, strlen(strEditScript)+1);
    fileScript.Close();
    FreeMemory( pScript);
  
    // process script
    ProcessScript_t( fnTempScript);

    // release old texture if it exists and obtain new texture
    ReleaseCreatedTexture();
    // obtain newly created texture
    m_ptdCreated = _pTextureStock->Obtain_t( fnTemptexture);
    m_ptdCreated->Reload();
    // set texture data to texture preview window so it could display texture
    m_wndViewCreatedTexture.m_toTexture.SetData( m_ptdCreated);

    char achrSize[64];
    sprintf( achrSize, "%d x %d", 
      m_ptdCreated->td_mexWidth>>m_ptdCreated->td_iFirstMipLevel,
      m_ptdCreated->td_mexHeight>>m_ptdCreated->td_iFirstMipLevel);
    m_strSizeInPixels = achrSize;
    UpdateData( FALSE);

    // init animations combo
    InitAnimationsCombo();
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

CDlgCreateAnimatedTexture::CDlgCreateAnimatedTexture(
  CDynamicArray<CTFileName> &afnPictures, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateAnimatedTexture::IDD, pParent)
{
  //{{AFX_DATA_INIT(CDlgCreateAnimatedTexture)
	m_strEditScript = _T("");
	m_strSizeInPixels = _T("");
	m_strCreatedTextureName = _T("");
	//}}AFX_DATA_INIT

  // remember array of selected frames
  m_pafnPictures = &afnPictures;
  // set first frame as input file name
  afnPictures.Lock();
  CTFileName fnInputFile = afnPictures[0];
  afnPictures.Unlock();
  
  if( (fnInputFile != "") && 
      ((fnInputFile.FileExt() == ".tex") || (fnInputFile.FileExt() == ".scr")) )
  {
    m_strCreatedTextureName = fnInputFile.FileDir() + fnInputFile.FileName() + ".tex";
  }
  else
  {
    m_strCreatedTextureName = "Unnamed";
  }

  m_bPreviewWindowsCreated = FALSE;
  m_ptdCreated = NULL;
  m_pixSourceWidth = -1;  
  m_pixSourceHeight = -1;  

	// remember source and destination file names
  m_fnSourceFileName = fnInputFile;
  m_fnCreatedFileName = fnInputFile.FileDir()+fnInputFile.FileName()+".tex";
}

CDlgCreateAnimatedTexture::~CDlgCreateAnimatedTexture()
{
  ReleaseCreatedTexture();
}

void CDlgCreateAnimatedTexture::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // if dialog is recieving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {
  }

	//{{AFX_DATA_MAP(CDlgCreateAnimatedTexture)
	DDX_Control(pDX, IDC_CHEQUERED_ALPHA, m_ctrlCheckButton);
	DDX_Control(pDX, IDC_TEXTURE_ANIMATIONS, m_ctrlAnimationsCombo);
	DDX_Text(pDX, IDC_EDIT_SCRIPT, m_strEditScript);
	DDX_Text(pDX, IDC_SIZE_IN_PIXELS, m_strSizeInPixels);
	DDX_Text(pDX, IDC_TEXTURE_NAME, m_strCreatedTextureName);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if(pDX->m_bSaveAndValidate != FALSE)
  {
  }
}


BEGIN_MESSAGE_MAP(CDlgCreateAnimatedTexture, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateAnimatedTexture)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CHEQUERED_ALPHA, OnChequeredAlpha)
	ON_BN_CLICKED(ID_CREATE_TEXTURE, OnCreateTexture)
	ON_BN_CLICKED(ID_REFRESH_TEXTURE, OnRefreshTexture)
	ON_CBN_SELCHANGE(IDC_TEXTURE_ANIMATIONS, OnSelchangeTextureAnimations)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateAnimatedTexture message handlers

void CDlgCreateAnimatedTexture::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
  // if texture preview windows are not yet created
  if( !m_bPreviewWindowsCreated)
  {
    // ---------------- Create custom window that will show how created texture will look like
    CWnd *pWndCreatedTexturePreview = GetDlgItem(IDC_TEXTURE_PREVIEW_WINDOW);
    ASSERT(pWndCreatedTexturePreview != NULL);
    CRect rectPreviewCreatedTextureWnd;
    // get rectangle occupied by preview texture window
    pWndCreatedTexturePreview->GetWindowRect( &rectPreviewCreatedTextureWnd);
    ScreenToClient( &rectPreviewCreatedTextureWnd);
    // create window for for showing created texture
    m_wndViewCreatedTexture.Create( NULL, NULL, WS_BORDER|WS_VISIBLE, rectPreviewCreatedTextureWnd,
                                    this, IDW_VIEW_CREATED_TEXTURE);
    // mark that custom windows are created
    m_bPreviewWindowsCreated = TRUE;
  }
}

void CDlgCreateAnimatedTexture::OnChequeredAlpha() 
{
  // toggle chequered alpha on/off
  m_wndViewCreatedTexture.m_bChequeredAlpha = !m_wndViewCreatedTexture.m_bChequeredAlpha;
}


void CDlgCreateAnimatedTexture::OnRefreshTexture() 
{
  RefreshTexture();
}

void CDlgCreateAnimatedTexture::OnCreateTexture() 
{
  // refresh (recreate) texture in temporary directory
  RefreshTexture();
  // prepare names for temporary script and texture
  CTFileName fnFullTempTexture = _fnmApplicationPath+CTString(TEMPORARY_TEXTURE_NAME);
  CTFileName fnFullTempScript = _fnmApplicationPath+CTString(TEMPORARY_SCRIPT_NAME);
  // and for supposed final texture name
  CTFileName fnFullFinalTexture = _fnmApplicationPath+m_fnCreatedFileName;
  
  CTFileName fnSaveName;
  if( m_strCreatedTextureName == "Unnamed")
  {
    // extract last sub directory name
    char achrLastSubDir[ 256];
    strcpy( achrLastSubDir, m_fnSourceFileName.FileDir());
    achrLastSubDir[ strlen(achrLastSubDir)-1]=0;  // remove last '\'
    CTString strLastSubDir = CTFileName(CTString(achrLastSubDir)).FileName();

    // call save texture requester
    fnSaveName = _EngineGUI.BrowseTexture( 
      strLastSubDir+".tex", // default name
      KEY_NAME_CREATE_ANIMATED_TEXTURE_DIR, "Choose texture name",
      FALSE/* bOpenFileRequester*/);
    if( fnSaveName == "") return;
  }
  else
  {
    fnSaveName = CTString( CStringA(m_strCreatedTextureName));
  }

  // set newly picked names for final script and texture
  fnFullFinalTexture = _fnmApplicationPath+fnSaveName;
  CTFileName fnFullFinalScript = 
    fnFullFinalTexture.FileDir()+fnFullFinalTexture.FileName()+".scr";
  // copy temporary script and texture files into real their place
  CopyFileA( fnFullTempScript, fnFullFinalScript, FALSE);
  CopyFileA( fnFullTempTexture, fnFullFinalTexture, FALSE);
  m_fnCreatedFileName =fnSaveName;
  // end dialog
  EndDialog( IDOK);
}

BOOL CDlgCreateAnimatedTexture::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // if we received script as input
  if( m_fnSourceFileName.FileExt() == ".scr")
  {
    // load script file into edit control
    try
    {
      CTFileStream fileScript;
      fileScript.Open_t( m_fnSourceFileName);
      // get size of script file
      ULONG ulScriptFileSize = fileScript.GetStreamSize();
      char *pchrFile = new char[ ulScriptFileSize+1];
      // set eol character
      pchrFile[ ulScriptFileSize] = 0;
      fileScript.Read_t( pchrFile, ulScriptFileSize);
      // copy script to edit ctrl
      m_strEditScript = CTString( pchrFile);
      delete pchrFile;
    }
    // catch errors
    catch (const char *strError)
    {
      // and do nothing
      (void) strError;
    }
  }
  // we will create temporary script
  else
  {
    try
    {
      // if can't get picture file information
      CImageInfo iiImageInfo;
      if (iiImageInfo.GetGfxFileInfo_t(m_fnSourceFileName)==UNSUPPORTED_FILE)
      {
        // throw error
        ThrowF_t("File '%s' has unsupported file format", 
          (CTString&)(_fnmApplicationPath+m_fnSourceFileName));
      }
      // get dimensions
      m_pixSourceWidth = iiImageInfo.ii_Width;
      m_pixSourceHeight = iiImageInfo.ii_Height;
    }
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
    }

    // allocate 16k for script
    char achrDefaultScript[ 16384];
    // default script into edit control
    sprintf( achrDefaultScript,
      ";* Texture description\r\n"
      "TEXTURE_WIDTH %.4f\r\n"
      "TEXTURE_MIPMAPS 8\r\n"
      "ANIM_START\r\n"
      "DIRECTORY %s\r\n\r\n"
      ";* Animations\r\n"
      "ANIMATION Default_Animation\r\n"
      "SPEED 0.1\r\n"
      "FRAMES %d\r\n",
      METERS_MEX(m_pixSourceWidth * (1 << 5) ),
      (CTString&)m_fnCreatedFileName.FileDir(),
      m_pafnPictures->Count());
    // add name for each frame
    FOREACHINDYNAMICARRAY( *m_pafnPictures, CTFileName, itPicture)
    {
      CTFileName &fn=*itPicture;
      CTString strName=fn.FileName();
      CTString strExt=fn.FileExt();
      // add finishing part of script
      sprintf( achrDefaultScript, "%s    %s%s\r\n", achrDefaultScript, strName, strExt);
    }
    // add finishing part of script
    sprintf( achrDefaultScript, "%sANIM_END\r\nEND\r\n", achrDefaultScript);
    // copy default script into edit ctrl
    m_strEditScript = achrDefaultScript;
  }

  CTFileName fnTexFileName = m_fnSourceFileName.FileDir() + m_fnSourceFileName.FileName() + ".tex";
  // try to
  try
  {
    // obtain texture with the same name (if exists)
    CTextureData *pTD = _pTextureStock->Obtain_t( fnTexFileName);
    pTD->Reload();
    // release texture
    _pTextureStock->Release( pTD);
  }
  // if texture can't be obtained
  catch (const char *err_str)
  {
    // never mind
    (void) err_str;
  }

  m_ctrlCheckButton.SetCheck( 1);

  // force edit script control to pick up default script string
  UpdateData( FALSE);
  // and refresh (recreate) texture in temporary directory
  RefreshTexture();
	return TRUE;
}
