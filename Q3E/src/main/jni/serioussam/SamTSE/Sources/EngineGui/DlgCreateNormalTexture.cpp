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

// DlgCreateNormalTexture.cpp : implementation file
//

#include "EngineGui/StdH.h"
#include "DlgCreateNormalTexture.h"
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static BOOL _bWasForced32 = FALSE;

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateNormalTexture dialog


CDlgCreateNormalTexture::CDlgCreateNormalTexture( CTFileName fnInputFile, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateNormalTexture::IDD, pParent)
{
  //{{AFX_DATA_INIT(CDlgCreateNormalTexture)
	m_strCreatedTextureName = _T("");
	m_strSizeInPixels = _T("");
	m_bCreateMipmaps = FALSE;
	//}}AFX_DATA_INIT

  m_bSourcePictureValid = FALSE;
  m_bPreviewWindowsCreated = FALSE;
  m_ptdCreated = NULL;
  
  CTFileName fnTexFileName = fnInputFile.FileDir() + fnInputFile.FileName() + ".tex";
  // remember source and created texture name
  m_fnSourceFileName  = fnInputFile;
  m_fnCreatedFileName = fnTexFileName;
  // set initial values for create mip maps flag and size for create texture dialog
  m_bCreateMipmaps  = AfxGetApp()->GetProfileInt(L"Creating textures", L"Create mipmaps", 1);
  m_mexCreatedWidth = AfxGetApp()->GetProfileInt(L"Creating textures", L"Mex width",  -1);
  _bWasForced32     = FALSE;

  try
  {
    // if can't get picture file information
    CImageInfo iiImageInfo;
    if (iiImageInfo.GetGfxFileInfo_t( m_fnSourceFileName)==UNSUPPORTED_FILE)
    {
      // throw error
      ThrowF_t("File '%s' has unsupported file format", 
        (CTString&)(_fnmApplicationPath+m_fnSourceFileName));
    }
    // get dimensions
    m_pixSourceWidth  = iiImageInfo.ii_Width;
    m_pixSourceHeight = iiImageInfo.ii_Height;
    // test if dimensions are at power of 2
    if( (((1<<((int)Log2(m_pixSourceWidth)))  != m_pixSourceWidth)) ||
        (((1<<((int)Log2(m_pixSourceHeight))) != m_pixSourceHeight))) {
      ThrowF_t( "Picture %s has wrong dimensions (%d,%d).\n"
                "Both width and height must be at power of 2.",
                (CTString&)m_fnSourceFileName, m_pixSourceWidth, m_pixSourceHeight);
    }
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
    return;
  }
  m_bSourcePictureValid = TRUE;

  // try to
  try
  { // obtain texture with the same name (if already exists)
    CTextureData *pTD = _pTextureStock->Obtain_t( fnTexFileName);
    pTD->Reload();
    // now pick up initial number of mip levels
    m_bCreateMipmaps = pTD->td_ctFineMipLevels>1;
    // remember existing texture's width in mexels
    m_mexCreatedWidth = pTD->GetWidth();
    // remember existing texture's flags
    _bWasForced32 = pTD->td_ulFlags & TEX_32BIT;
    // release texture
    _pTextureStock->Release( pTD);
  }
  // if texture can't be obtained
  catch (const char *err_str)
  { // never mind
    (void) err_str;
  }

  m_wndViewCreatedTexture.m_bForce32 = FALSE;
  if( _bWasForced32) m_wndViewCreatedTexture.m_bForce32 = TRUE;
  RefreshCreatedTexture();
  // set created texture name
  m_strCreatedTextureName = fnTexFileName;
  // prepare string telling size of source picture
  char strSize[ 64];
  sprintf( strSize, "%d x %d", m_pixSourceWidth, m_pixSourceHeight);
  m_strSizeInPixels = strSize;
} 

CDlgCreateNormalTexture::~CDlgCreateNormalTexture()
{
  ReleaseCreatedTexture();
}


void CDlgCreateNormalTexture::RefreshCreatedTexture(void)
{
  ReleaseCreatedTexture();

  // prepare forced upload quality
  _iTexForcedQuality = 16;
  if( m_wndViewCreatedTexture.m_bForce32) _iTexForcedQuality = 32;
  // create temporary texture to show how texture will look like
  try
  {
    // create temporary texture
    CreateTexture_t( m_fnSourceFileName, CTString( "Temp\\Temp.tex"),
                     m_pixSourceWidth, MAX_MEX_LOG2+1, FALSE);
    m_ptdCreated = _pTextureStock->Obtain_t( CTString( "Temp\\Temp.tex"));
    m_ptdCreated->Reload();
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
    return;
  }
  // set texture data to texture preview window so it could show preview picture
  m_wndViewCreatedTexture.m_toTexture.SetData( m_ptdCreated);
}



void CDlgCreateNormalTexture::ReleaseCreatedTexture(void)
{
  // if there is texture obtained, release it
  if( m_ptdCreated != NULL)
  {
    // free obtained texture
    _pTextureStock->Release( m_ptdCreated);
    m_ptdCreated = NULL;
    m_wndViewCreatedTexture.m_toTexture.SetData( NULL);
  }
  // reset forced upload quality
  _iTexForcedQuality = 0;
}


void CDlgCreateNormalTexture::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // if dialog is recieving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {
  }

	//{{AFX_DATA_MAP(CDlgCreateNormalTexture)
	DDX_Control(pDX, IDC_FORCE32, m_ctrlForce32);
	DDX_Control(pDX, IDC_CHEQUERED_ALPHA, m_ctrlCheckButton);
	DDX_Control(pDX, IDC_MEX_SIZE, m_ctrlMexSizeCombo);
	DDX_Text(pDX, IDC_CREATED_TEXTURE_NAME, m_strCreatedTextureName);
	DDX_Text(pDX, IDC_SIZE_IN_PIXELS, m_strSizeInPixels);
	DDX_Check(pDX, IDC_CREATE_MIPMAPS, m_bCreateMipmaps);
	//}}AFX_DATA_MAP
  
  // if dialog is giving data
  if(pDX->m_bSaveAndValidate != FALSE)
  {
  }
}


BEGIN_MESSAGE_MAP(CDlgCreateNormalTexture, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateNormalTexture)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CHEQUERED_ALPHA, OnChequeredAlpha)
	ON_BN_CLICKED(IDC_FORCE32, OnForce32)
	ON_BN_CLICKED(ID_CREATE_TEXTURE, OnCreateTexture)
	ON_BN_CLICKED(IDC_CREATE_MIPMAPS, OnCreateMipmaps)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateNormalTexture message handlers

void CDlgCreateNormalTexture::OnPaint() 
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

#define MAX_ALLOWED_MEX_SIZE 32*1024*1024

BOOL CDlgCreateNormalTexture::OnInitDialog() 
{
	CDialog::OnInitDialog();
  char strSize[ 64];

  // set default created texture's size
  INDEX iInitialSelectedSize = 0;
  // first combo entry is selected by default (requesting maximum mip maps to be created)
  INDEX iInitialSelectedMipMapCombo = 0;

  // obtain all available potentions of source picture's dimensions
  INDEX iPotention = 0;
  FOREVER
  {
    MEX mexPotentionWidth = m_pixSourceWidth * (1 << iPotention);
    MEX mexPotentionHeight = m_pixSourceHeight * (1 << iPotention);
    if( (mexPotentionWidth > MAX_ALLOWED_MEX_SIZE) ||
        (mexPotentionHeight > MAX_ALLOWED_MEX_SIZE))
        break;
    sprintf( strSize, "%.2f x %.2f", 
             METERS_MEX( mexPotentionWidth), METERS_MEX( mexPotentionHeight));
    INDEX iAddedAs = m_ctrlMexSizeCombo.AddString( CString(strSize));
    // connect item and represented mex value
    m_ctrlMexSizeCombo.SetItemData( iAddedAs, mexPotentionWidth);
    // try to select consistent size
    if( mexPotentionWidth == m_mexCreatedWidth)
    {
      iInitialSelectedSize = iPotention;
    }
    // next potention
    iPotention ++;
  }
  // select size
  m_ctrlMexSizeCombo.SetCurSel( iInitialSelectedSize);
  m_ctrlCheckButton.SetCheck(1);
  // determine correct texture quality
  if( _bWasForced32) {
    m_ctrlForce32.SetCheck(1);
    m_wndViewCreatedTexture.m_bForce32 = TRUE;
  } else {
    m_ctrlForce32.SetCheck(0);
    m_wndViewCreatedTexture.m_bForce32 = FALSE;
  }
  // return TRUE unless you set the focus to a control
	return TRUE;  
}


void CDlgCreateNormalTexture::OnCreateTexture() 
{
  MEX mexWidth   = m_ctrlMexSizeCombo.GetItemData( m_ctrlMexSizeCombo.GetCurSel());
  INDEX iMipMaps = 16;
  if( !m_bCreateMipmaps)
  {
    iMipMaps = 1;
  }
  // create texture
  try {
    CreateTexture_t( m_fnSourceFileName, m_fnCreatedFileName, mexWidth, iMipMaps,
                     m_wndViewCreatedTexture.m_bForce32);
  }
  catch (const char *err_str) {
    AfxMessageBox( CString(err_str));
    return;                     
  }
  AfxGetApp()->WriteProfileInt(L"Creating textures", L"Create mipmaps", m_bCreateMipmaps);
  AfxGetApp()->WriteProfileInt(L"Creating textures", L"Mex width", mexWidth);
  // end dialog
  EndDialog( IDOK);
}


void CDlgCreateNormalTexture::OnChequeredAlpha() 
{
  // toggle chequered alpha on/off
  m_wndViewCreatedTexture.m_bChequeredAlpha = !m_wndViewCreatedTexture.m_bChequeredAlpha;
  RefreshCreatedTexture();
}

void CDlgCreateNormalTexture::OnForce32() 
{
  // toggle force32 on/off
  m_wndViewCreatedTexture.m_bForce32 = !m_wndViewCreatedTexture.m_bForce32;
  RefreshCreatedTexture();
}

void CDlgCreateNormalTexture::OnCreateMipmaps() 
{
	m_bCreateMipmaps = !m_bCreateMipmaps;
  UpdateData(FALSE);
}
