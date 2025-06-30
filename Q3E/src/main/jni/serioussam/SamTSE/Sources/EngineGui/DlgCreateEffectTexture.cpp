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

// DlgCreateEffectTexture.cpp : implementation file
//

#include "EngineGui/StdH.h"
#include "DlgCreateEffectTexture.h"
#include <Engine/Graphics/TextureEffects.h>
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateEffectTexture dialog

#define MAX_ALLOWED_MEX_SIZE 1024*1024
static CDlgCreateEffectTexture *pDialog;
static PIX pixStartU;
static PIX pixStartV;

// called when user pressed LMB on preview effect window
void _OnLeftMouseDown( PIX pixU, PIX pixV)
{
  // store starting point
  pixStartU = pixU;
  pixStartV = pixV;
}

// called when user released LMB on preview effect window
void _OnLeftMouseUp( PIX pixU, PIX pixV)
{
  // obtain currently selected effect source type
  ULONG ulEffectSourceType = pDialog->m_ctrlEffectTypeCombo.GetCurSel();
  // add new effect source
  pDialog->m_tdCreated.td_ptegEffect->AddEffectSource( ulEffectSourceType, pixStartU, pixStartV, pixU, pixV);
}

// called when user pressed RMB on preview effect window
void _OnRightMouseDown( PIX pixU, PIX pixV)
{
  // obtain currently selected effect source type
  ULONG ulEffectSourceType = pDialog->m_ctrlEffectTypeCombo.GetCurSel();
  // add new effect source
  pDialog->m_tdCreated.td_ptegEffect->AddEffectSource( ulEffectSourceType, pixU, pixV, pixU, pixV);
}

// called when user pressed RMB and move mouse on preview effect window
void _OnRightMouseMove( PIX pixU, PIX pixV)
{
  // obtain currently selected effect source type
  ULONG ulEffectSourceType = pDialog->m_ctrlEffectTypeCombo.GetCurSel();
  // add new effect source
  pDialog->m_tdCreated.td_ptegEffect->AddEffectSource( ulEffectSourceType, pixU, pixV, pixU, pixV);
}

CDlgCreateEffectTexture::CDlgCreateEffectTexture(CTFileName fnInputFile/*=""*/, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCreateEffectTexture::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgCreateEffectTexture)
	m_strCreatedTextureName = _T("");
	m_strBaseTextureName = _T("");
	m_strRendSpeed = _T("");
	//}}AFX_DATA_INIT

  // set dialog ptr
  pDialog = this;
  // register left mouse button call back function
  m_wndViewCreatedTexture.SetLeftMouseButtonClicked( _OnLeftMouseDown);
  // register left mouse release button call back function
  m_wndViewCreatedTexture.SetLeftMouseButtonReleased( _OnLeftMouseUp);
  // register right mouse button call back function
  m_wndViewCreatedTexture.SetRightMouseButtonClicked( _OnRightMouseDown);
  // register right mouse button move call back function
  m_wndViewCreatedTexture.SetRightMouseButtonMoved( _OnRightMouseMove);

  // set invalid initial mip level and size for created texture
  m_mexInitialCreatedWidth = 2048;
  m_pixInitialCreatedWidth = 256;
  m_pixInitialCreatedHeight = 256;

  BOOL bCreateNew = TRUE;
  if( fnInputFile != "")
  {
    m_fnCreatedTextureName = fnInputFile;
    try
    {
      // load texture with the same name (if allready exists)
      m_tdCreated.Load_t( m_fnCreatedTextureName);
      // remember existing texture's width in mexels
      m_mexInitialCreatedWidth = m_tdCreated.GetWidth();
      m_pixInitialCreatedWidth = m_tdCreated.GetPixWidth();
      m_pixInitialCreatedHeight = m_tdCreated.GetPixHeight();
      bCreateNew = FALSE;
    }
    // if texture can't be obtained
    catch (const char *err_str)
    {
      // never mind
      (void) err_str;
    }
  }
  else
  {
    // set texture name to unnamed
    m_fnCreatedTextureName = CTString("Unnamed");
  }
  
  // if we should create a new texture
  if( bCreateNew)
  {
    CTextureData *pBaseTexture;
    try
    {
      // obtain default texture as base
      pBaseTexture = _pTextureStock->Obtain_t( CTFILENAME("Textures\\Editor\\Default.tex"));
    }
    // if texture can't be obtained
    catch (const char *err_str)
    {
      // stop executing program
      FatalError( "%s", err_str);
    }

    // create empty effect texture
    m_tdCreated.CreateEffectTexture(m_pixInitialCreatedWidth, m_pixInitialCreatedHeight,
      m_mexInitialCreatedWidth, pBaseTexture, 0);
    // release default texture
    _pTextureStock->Release(pBaseTexture);
  }

  m_wndViewCreatedTexture.m_toTexture.SetData( &m_tdCreated);

  // copy texture name to text control
  m_strCreatedTextureName = m_fnCreatedTextureName;
  m_bPreviewWindowsCreated = FALSE;
}

CDlgCreateEffectTexture::~CDlgCreateEffectTexture()
{
  m_tdCreated.MarkUsed();
  m_wndViewCreatedTexture.m_toTexture.SetData( NULL);
  m_tdCreated.MarkUnused();
}

void CDlgCreateEffectTexture::SetNewBaseTexture( CTFileName fnNewBase)
{
	if( fnNewBase != "")
  {
    // try to
    try
    {
      // obtain texture with the same name (if allready exists)
      CTextureData *pTD = _pTextureStock->Obtain_t( fnNewBase);
      pTD->Reload();
      if( pTD->td_ptegEffect != NULL)
      {
        _pTextureStock->Release( pTD);
        ThrowF_t( "Texture '%s' is an effect texture.", (CTString&)fnNewBase);
      }
      // if there is base texture obtained, release it
      if( m_tdCreated.td_ptdBaseTexture!= NULL) 
      {
        _pTextureStock->Release( m_tdCreated.td_ptdBaseTexture);
        // reset base texture ptr
        m_tdCreated.td_ptdBaseTexture = NULL;
      }    
      // set new base texture ptr
      m_tdCreated.td_ptdBaseTexture = pTD;
      m_pixInitialCreatedWidth  = pTD->GetPixWidth();
      m_pixInitialCreatedHeight = pTD->GetPixHeight();
      UpdateData( FALSE);
      SelectPixSizeCombo();
    }
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
      return;
    }
  }
}

void CDlgCreateEffectTexture::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // if dialog is receiving data
  if(pDX->m_bSaveAndValidate == FALSE)
  {
    m_strBaseTextureName = "None";
    CTFileName fnNewBase;
    // if there is base texture obtained, get name
    if( m_tdCreated.td_ptdBaseTexture != NULL)
    {
      PIX pixBaseTextureWidth  = m_tdCreated.td_ptdBaseTexture->GetPixWidth();
      PIX pixBaseTextureHeight = m_tdCreated.td_ptdBaseTexture->GetPixHeight();
      char achrBaseTextureName[ 256];
      sprintf( achrBaseTextureName, "%s    (%d x %d)",
        (CTString&)m_tdCreated.td_ptdBaseTexture->GetName(),
        pixBaseTextureWidth, pixBaseTextureHeight);
      m_strBaseTextureName = achrBaseTextureName;
    }
  }

	//{{AFX_DATA_MAP(CDlgCreateEffectTexture)
	DDX_Control(pDX, IDC_CHEQUERED_ALPHA, m_ctrlCheckButton);
	DDX_Control(pDX, IDC_MEX_SIZE, m_ctrlMexSizeCombo);
	DDX_Control(pDX, IDC_PIX_WIDTH, m_ctrlPixWidthCombo);
	DDX_Control(pDX, IDC_PIX_HEIGHT, m_ctrlPixHeightCombo);
	DDX_Control(pDX, IDC_EFFECT_CLASS, m_ctrlEffectClassCombo);
	DDX_Control(pDX, IDC_EFFECT_TYPE, m_ctrlEffectTypeCombo);
	DDX_Text(pDX, IDC_CREATED_TEXTURE_NAME, m_strCreatedTextureName);
	DDX_Text(pDX, IDC_BASE_TEXTURE_NAME, m_strBaseTextureName);
	DDX_Text(pDX, IDC_REND_SPEED, m_strRendSpeed);
	//}}AFX_DATA_MAP
  
  // if dialog is giving data
  if(pDX->m_bSaveAndValidate != FALSE)
  {
  }
}


BEGIN_MESSAGE_MAP(CDlgCreateEffectTexture, CDialog)
	//{{AFX_MSG_MAP(CDlgCreateEffectTexture)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CHEQUERED_ALPHA, OnChequeredAlpha)
	ON_BN_CLICKED(ID_BROWSE_BASE, OnBrowseBase)
	ON_BN_CLICKED(ID_CREATE_AS, OnCreateAs)
	ON_BN_CLICKED(ID_REMOVE_ALL_EFFECTS, OnRemoveAllEffects)
	ON_CBN_SELCHANGE(IDC_PIX_HEIGHT, OnSelchangePixHeight)
	ON_CBN_SELCHANGE(IDC_PIX_WIDTH, OnSelchangePixWidth)
	ON_CBN_SELCHANGE(IDC_MEX_SIZE, OnSelchangeMexSize)
	ON_BN_CLICKED(ID_CREATE, OnCreate)
	ON_CBN_SELCHANGE(IDC_EFFECT_CLASS, OnSelchangeEffectClass)
	ON_CBN_SELCHANGE(IDC_EFFECT_TYPE, OnSelchangeEffectType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgCreateEffectTexture message handlers

void CDlgCreateEffectTexture::OnPaint() 
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

void CDlgCreateEffectTexture::OnChequeredAlpha() 
{
  // toggle chequered alpha on/off
  m_wndViewCreatedTexture.m_bChequeredAlpha = !m_wndViewCreatedTexture.m_bChequeredAlpha;
}

void CDlgCreateEffectTexture::InitializeSizeCombo(void)
{
  // clear combo box
  m_ctrlMexSizeCombo.ResetContent();

  INDEX iSelectedWidth = m_ctrlPixWidthCombo.GetCurSel();
  INDEX iSelectedHeight = m_ctrlPixHeightCombo.GetCurSel();
  // obtain selected size in pixels
  m_pixInitialCreatedWidth = 1<<iSelectedWidth;
  m_pixInitialCreatedHeight = 1<<iSelectedHeight;
  // obtain all available potentions of source picture's dimensions
  INDEX iPotention = 0;
  // set default created texture's size
  INDEX iInitialSelectedSize = 0;
  FOREVER
  {
    char strSize[ 64];
    MEX mexPotentionWidth = m_pixInitialCreatedWidth * (1 << iPotention);
    MEX mexPotentionHeight = m_pixInitialCreatedHeight * (1 << iPotention);
    if( (mexPotentionWidth > MAX_ALLOWED_MEX_SIZE) ||
        (mexPotentionHeight > MAX_ALLOWED_MEX_SIZE))
        break;
    sprintf( strSize, "%.2f x %.2f", 
             METERS_MEX( mexPotentionWidth), METERS_MEX( mexPotentionHeight));
    INDEX iAddedAs = m_ctrlMexSizeCombo.AddString( CString(strSize));
    // connect item and represented mex value
    m_ctrlMexSizeCombo.SetItemData( iAddedAs, mexPotentionWidth);
    // try to select consistent size
    if( mexPotentionWidth == m_mexInitialCreatedWidth)
    {
      iInitialSelectedSize = iPotention;
    }
    // next potention
    iPotention ++;
  }
  // select size
  m_ctrlMexSizeCombo.SetCurSel( iInitialSelectedSize);
}

void CDlgCreateEffectTexture::InitializeEffectTypeCombo( void)
{
  // get selected effect class
  INDEX iClass = m_ctrlEffectClassCombo.GetCurSel();
  // obtain effect source table for current effect class
  struct TextureEffectSourceType *patestSourceEffectTypes = 
    _ategtTextureEffectGlobalPresets[ iClass].tet_atestEffectSourceTypes;
  INDEX ctSourceEffectTypes = _ategtTextureEffectGlobalPresets[ iClass].tet_ctEffectSourceTypes;
  // initialize effect groups combo
  m_ctrlEffectTypeCombo.ResetContent();
  for( INDEX iEffectType=0; iEffectType<ctSourceEffectTypes; iEffectType++)
  {
    m_ctrlEffectTypeCombo.AddString(
      CString(patestSourceEffectTypes[ iEffectType].test_strName));
  }
  m_ctrlEffectTypeCombo.SetCurSel( 0);
}

void CDlgCreateEffectTexture::SelectPixSizeCombo(void)
{
  m_ctrlPixWidthCombo.SetCurSel( 0);
  m_ctrlPixHeightCombo.SetCurSel( 0);
  for( INDEX iWidthItem=0; iWidthItem<m_ctrlPixWidthCombo.GetCount(); iWidthItem++)
  {
    if( (1<<iWidthItem) == m_pixInitialCreatedWidth)
    {
      m_ctrlPixWidthCombo.SetCurSel( iWidthItem);
    }
  }
  for( INDEX iHeightItem=0; iHeightItem<m_ctrlPixWidthCombo.GetCount(); iHeightItem++)
  {
    if( (1<<iHeightItem) == m_pixInitialCreatedHeight)
    {
      m_ctrlPixHeightCombo.SetCurSel( iHeightItem);
    }
  }
  InitializeSizeCombo();
}

BOOL CDlgCreateEffectTexture::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // set default created texture's size
  INDEX iInitialSelectedSize = 0;
  // first combo entry is selected by default (requesting maximum mip maps to be created)
  INDEX iInitialSelectedMipMapCombo = 0;

  // fill width combo
  m_ctrlPixWidthCombo.ResetContent();
  m_ctrlPixWidthCombo.AddString(L"1");
  m_ctrlPixWidthCombo.AddString(L"2");
  m_ctrlPixWidthCombo.AddString(L"4");
  m_ctrlPixWidthCombo.AddString(L"8");
  m_ctrlPixWidthCombo.AddString(L"16");
  m_ctrlPixWidthCombo.AddString(L"32");
  m_ctrlPixWidthCombo.AddString(L"64");
  m_ctrlPixWidthCombo.AddString(L"128");
  m_ctrlPixWidthCombo.AddString(L"256");
  // fill height combo
  m_ctrlPixHeightCombo.ResetContent();
  m_ctrlPixHeightCombo.AddString(L"1");
  m_ctrlPixHeightCombo.AddString(L"2");
  m_ctrlPixHeightCombo.AddString(L"4");
  m_ctrlPixHeightCombo.AddString(L"8");
  m_ctrlPixHeightCombo.AddString(L"16");
  m_ctrlPixHeightCombo.AddString(L"32");
  m_ctrlPixHeightCombo.AddString(L"64");
  m_ctrlPixHeightCombo.AddString(L"128");
  m_ctrlPixHeightCombo.AddString(L"256");
  
  // initialize effect groups combo
  m_ctrlEffectClassCombo.ResetContent();
  for( INDEX iEffectClass=0; iEffectClass<_ctTextureEffectGlobalPresets; iEffectClass++)
  {
    m_ctrlEffectClassCombo.AddString(
      CString(_ategtTextureEffectGlobalPresets[ iEffectClass].tegt_strName));
  }
  INDEX iSelectedEffectClass = m_tdCreated.td_ptegEffect->teg_ulEffectType;
  m_ctrlEffectClassCombo.SetCurSel( iSelectedEffectClass);

  // initialize sub effects
  InitializeEffectTypeCombo();

  m_ctrlCheckButton.SetCheck( 1);
	
  SelectPixSizeCombo();
	return TRUE;
}

void CDlgCreateEffectTexture::OnBrowseBase() 
{
  CTFileName fnNewBase;
  // if there is base texture obtained, release it
  if( m_tdCreated.td_ptdBaseTexture != NULL)
  {
    fnNewBase = _EngineGUI.BrowseTexture( m_tdCreated.td_ptdBaseTexture->GetName(),
                                      KEY_NAME_BASE_TEXTURE_DIR, "Browse base texture");
  }
  else
  {
    fnNewBase = _EngineGUI.BrowseTexture( CTString(""), KEY_NAME_BASE_TEXTURE_DIR,
                                      "Browse base texture");
  }
  SetNewBaseTexture( fnNewBase);
  CreateTexture();
}

void CDlgCreateEffectTexture::OnRemoveAllEffects() 
{
	CreateTexture();
}

void CDlgCreateEffectTexture::OnSelchangeMexSize() 
{
  CreateTexture();
}

void CDlgCreateEffectTexture::OnSelchangePixHeight() 
{
  INDEX iSelectedWidth = m_ctrlPixWidthCombo.GetCurSel();
  INDEX iSelectedHeight = m_ctrlPixHeightCombo.GetCurSel();
  InitializeSizeCombo();
  CreateTexture();
}

void CDlgCreateEffectTexture::OnSelchangePixWidth() 
{
  INDEX iSelectedWidth = m_ctrlPixWidthCombo.GetCurSel();
  INDEX iSelectedHeight = m_ctrlPixHeightCombo.GetCurSel();
  InitializeSizeCombo();
  CreateTexture();
}

void CDlgCreateEffectTexture::CreateTexture( void)
{
  // obtain selected sizes
  m_pixInitialCreatedWidth  = 1<<m_ctrlPixWidthCombo.GetCurSel();
  m_pixInitialCreatedHeight = 1<<m_ctrlPixHeightCombo.GetCurSel();
  m_mexInitialCreatedWidth  = m_ctrlMexSizeCombo.GetItemData( m_ctrlMexSizeCombo.GetCurSel());
  // get selected effect class
  INDEX iClass = m_ctrlEffectClassCombo.GetCurSel();
  // create empty effect texture
  m_tdCreated.CreateEffectTexture( m_pixInitialCreatedWidth, m_pixInitialCreatedHeight,
                                   m_mexInitialCreatedWidth, m_tdCreated.td_ptdBaseTexture, iClass);
}

void CDlgCreateEffectTexture::OnCreateAs() 
{
  // call save texture file requester
  CTFileName fnNewTexName = _EngineGUI.BrowseTexture( CTString(CStringA(m_strCreatedTextureName)),
    KEY_NAME_CREATE_TEXTURE_DIR, "Create texture as", FALSE);
  // if picked valid name
  if( fnNewTexName != "")
  {
    // save as final texture
    try
    {
      m_tdCreated.Save_t( fnNewTexName);
    }
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
      return;
    }
    // remember new name
    m_strCreatedTextureName = fnNewTexName;
    m_fnCreatedTextureName = fnNewTexName;
    // show the change
    UpdateData( FALSE);
  }
}

void CDlgCreateEffectTexture::OnCreate() 
{
  if( m_strCreatedTextureName == "Unnamed")
  {
    OnCreateAs();
  }
  else
  {
    // save as final texture
    try
    {
      m_fnCreatedTextureName = CTString( CStringA(m_strCreatedTextureName));
      m_tdCreated.Save_t( m_fnCreatedTextureName);
    }
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
      return;
    }
  }
  if( m_strCreatedTextureName != "Unnamed")
  {
  	EndDialog( IDOK);
  }
}

void CDlgCreateEffectTexture::OnSelchangeEffectClass() 
{
  // initialize sub effects
  InitializeEffectTypeCombo();
  CreateTexture();
}

void CDlgCreateEffectTexture::OnSelchangeEffectType() 
{
}

