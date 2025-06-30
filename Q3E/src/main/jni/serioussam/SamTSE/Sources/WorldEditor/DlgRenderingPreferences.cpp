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

// DlgRenderingPreferences.cpp : implementation file
//

#include "stdafx.h"
#include "DlgRenderingPreferences.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgRenderingPreferences dialog


CDlgRenderingPreferences::CDlgRenderingPreferences( INDEX iBuffer, CWnd* pParent /*=NULL*/)
	: CDialog(CDlgRenderingPreferences::IDD, pParent)
{
  m_iBuffer = iBuffer;
  //{{AFX_DATA_INIT(CDlgRenderingPreferences)
	m_bBoundingBox = FALSE;
	m_bHidenLines = FALSE;
	m_bShadows = FALSE;
	m_bWireFrame = FALSE;
	m_fRenderingRange = 0.0f;
	m_bAutoRenderingRange = FALSE;
	m_bRenderEditorModels = FALSE;
	m_bUseTextureForBcg = FALSE;
	m_bRenderFieldBrushes = FALSE;
	m_bRenderFog = FALSE;
	m_bRenderHaze = FALSE;
	m_bRenderMirrors = FALSE;
	m_strBcgTexture = _T("");
	m_fFarClipPlane = 0.0f;
	m_bApplyFarClipInIsometricProjection = FALSE;
	//}}AFX_DATA_INIT
  m_VertexColors.SetPickerType( CColoredButton::PT_MFC);
  m_EdgesColors.SetPickerType( CColoredButton::PT_MFC);
  m_PolygonColors.SetPickerType( CColoredButton::PT_MFC);
  m_PaperColor.SetPickerType( CColoredButton::PT_MFC);
  m_SelectionColor.SetPickerType( CColoredButton::PT_MFC);
  m_GridColor.SetPickerType( CColoredButton::PT_MFC);
}


void CDlgRenderingPreferences::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  CModelRenderPrefs &pmrpPrefs = theApp.m_vpViewPrefs[ m_iBuffer].m_mrpModelRenderPrefs;
  CWorldRenderPrefs &pwrpPrefs = theApp.m_vpViewPrefs[ m_iBuffer].m_wrpWorldRenderPrefs;
  // if dialog is recieving data
  if( (pDX->m_bSaveAndValidate == FALSE) && IsWindow( m_hWnd) &&
      IsWindow( m_VertexFillType.m_hWnd) &&
      IsWindow( m_EdgesFillType.m_hWnd) &&
      IsWindow( m_PolygonFillType.m_hWnd) &&
      IsWindow( m_TextureFillType.m_hWnd) )      
  {
    // world rendering preferences
    m_VertexColors.SetColor( pwrpPrefs.GetVerticesInkColor());
    m_EdgesColors.SetColor( pwrpPrefs.GetEdgesInkColor());
    m_PolygonColors.SetColor( pwrpPrefs.GetPolygonsInkColor());
    m_PaperColor.SetColor( theApp.m_vpViewPrefs[ m_iBuffer].m_PaperColor);
    m_SelectionColor.SetColor( theApp.m_vpViewPrefs[ m_iBuffer].m_SelectionColor);
    m_GridColor.SetColor( theApp.m_vpViewPrefs[ m_iBuffer].m_GridColor);
    m_fFarClipPlane=pwrpPrefs.wrp_fFarClipPlane;
    m_bApplyFarClipInIsometricProjection=pwrpPrefs.wrp_bApplyFarClipPlaneInIsometricProjection;
    
    m_strBcgTexture = CTFileName(CTString( theApp.m_vpViewPrefs[ m_iBuffer].m_achrBcgPicture)).FileName();

    // get render editor models flag
    m_bRenderEditorModels = pwrpPrefs.IsEditorModelsOn();
    // get render field brushes flag
    m_bRenderFieldBrushes = pwrpPrefs.IsFieldBrushesOn();
    // get use texture for background flag
    m_bUseTextureForBcg = pwrpPrefs.IsBackgroundTextureOn();

    m_bRenderMirrors = pwrpPrefs.IsMirrorsOn();
    m_bRenderFog = pwrpPrefs.IsFogOn();
    m_bRenderHaze = pwrpPrefs.IsHazeOn();

    // get auto rendering range flag
    m_bAutoRenderingRange = theApp.m_vpViewPrefs[ m_iBuffer].m_bAutoRenderingRange;
    // get rendering range
    m_fRenderingRange = theApp.m_vpViewPrefs[ m_iBuffer].m_fRenderingRange;
    // enable/disable edit render range control depending on auto range flag
    UpdateEditRangeControl();

    // fill values for vertice's combo box
    m_VertexFillType.ResetContent();
    m_VertexFillType.AddString( L"No vertices");
    m_VertexFillType.AddString( L"Vertices ink");
    m_VertexFillType.AddString( L"Polygon color");
    m_VertexFillType.AddString( L"Sector color");
    // fill values for edges's combo box
    m_EdgesFillType.ResetContent();
    m_EdgesFillType.AddString( L"No edges");
    m_EdgesFillType.AddString( L"Edges ink");
    m_EdgesFillType.AddString( L"Polygon color");
    m_EdgesFillType.AddString( L"Sector color");
    // fill values for polygons's combo box
    m_PolygonFillType.ResetContent();
    m_PolygonFillType.AddString( L"No polygons");
    m_PolygonFillType.AddString( L"Polygons ink");
    m_PolygonFillType.AddString( L"Polygon color");
    m_PolygonFillType.AddString( L"Sector color");
    m_PolygonFillType.AddString( L"Texture");
    // fill values for model's texture combo
    m_TextureFillType.ResetContent();
    m_TextureFillType.AddString( L"No fill");
    m_TextureFillType.AddString( L"White color");
    m_TextureFillType.AddString( L"Surface colors");
    m_TextureFillType.AddString( L"On colors");
    m_TextureFillType.AddString( L"Off colors");
    m_TextureFillType.AddString( L"Texture");
    // fill values for flare FX combo
    m_comboFlareFX.ResetContent();
    m_comboFlareFX.AddString( L"None");
    m_comboFlareFX.AddString( L"Single flare");
    m_comboFlareFX.AddString( L"Reflections");
    m_comboFlareFX.AddString( L"Reflections and glare");
  	  
    INDEX iFillType;
    // set current fill type to vertices combo box
    iFillType = pwrpPrefs.GetVerticesFillType();
    m_VertexFillType.SetCurSel( iFillType);
    // set current fill type to edges combo box
    iFillType = pwrpPrefs.GetEdgesFillType();
    m_EdgesFillType.SetCurSel( iFillType);
    // set current fill type to polygons combo box
    iFillType = pwrpPrefs.GetPolygonsFillType();
    m_PolygonFillType.SetCurSel( iFillType);
    // set currently selected modeler's texture rendering type
    ULONG rtRenderType = pmrpPrefs.GetRenderType();
    iFillType = 0;
    if( (rtRenderType & RT_NO_POLYGON_FILL) != 0)       iFillType = 0;
    else if( (rtRenderType & RT_WHITE_TEXTURE) != 0)    iFillType = 1;
    else if( (rtRenderType & RT_SURFACE_COLORS) != 0)   iFillType = 2;
    else if( (rtRenderType & RT_ON_COLORS) != 0)        iFillType = 3;
    else if( (rtRenderType & RT_OFF_COLORS) != 0)       iFillType = 4;
    else if( (rtRenderType & RT_TEXTURE) != 0)          iFillType = 5;
    m_TextureFillType.SetCurSel( iFillType);

    enum CWorldRenderPrefs::LensFlaresType lftFlareFX = pwrpPrefs.GetLensFlares();
    m_comboFlareFX.SetCurSel( (INDEX) lftFlareFX);

    // set model rendering flags
    m_bBoundingBox = pmrpPrefs.BBoxAllVisible();
    m_bHidenLines = pmrpPrefs.HiddenLines();
    m_bShadows = pmrpPrefs.GetShadowQuality() == 0;
    m_bWireFrame = pmrpPrefs.WireOn();
  }

  //{{AFX_DATA_MAP(CDlgRenderingPreferences)
	DDX_Control(pDX, IDC_FLARE_FX, m_comboFlareFX);
	DDX_Control(pDX, IDC_SELECTION_COLOR, m_SelectionColor);
	DDX_Control(pDX, IDC_GRID_COLOR, m_GridColor);
	DDX_Control(pDX, IDC_PAPER_COLOR, m_PaperColor);
	DDX_Control(pDX, IDC_TEXTURE_FILL_TYPE, m_TextureFillType);
	DDX_Control(pDX, IDC_EDGES_FILL_TYPE, m_EdgesFillType);
	DDX_Control(pDX, IDC_POLYGON_FILL_TYPE, m_PolygonFillType);
	DDX_Control(pDX, IDC_VERTEX_FILL_TYPE, m_VertexFillType);
	DDX_Control(pDX, IDC_VERTEX_COLORS, m_VertexColors);
	DDX_Control(pDX, IDC_POLYGON_COLORS, m_PolygonColors);
	DDX_Control(pDX, IDC_EDGES_COLORS, m_EdgesColors);
	DDX_Check(pDX, IDC_BBOX, m_bBoundingBox);
	DDX_Check(pDX, IDC_HIDEN_LINES, m_bHidenLines);
	DDX_Check(pDX, IDC_SHADOWS, m_bShadows);
	DDX_Check(pDX, IDC_WIRE_FRAME, m_bWireFrame);
	DDX_Text(pDX, IDC_RENDERING_RANGE, m_fRenderingRange);
	DDV_MinMaxFloat(pDX, m_fRenderingRange, 1.f, 10000.f);
	DDX_Check(pDX, IDC_AUTO_RENDERING_RANGE, m_bAutoRenderingRange);
	DDX_Check(pDX, IDC_RENDER_EDITOR_MODELS, m_bRenderEditorModels);
	DDX_Check(pDX, IDC_USE_TEXTURE_FOR_BCG, m_bUseTextureForBcg);
	DDX_Check(pDX, IDC_RENDER_FIELDS, m_bRenderFieldBrushes);
	DDX_Check(pDX, IDC_RENDER_FOG, m_bRenderFog);
	DDX_Check(pDX, IDC_RENDER_HAZE, m_bRenderHaze);
	DDX_Check(pDX, IDC_RENDER_MIRRORS, m_bRenderMirrors);
	DDX_Text(pDX, IDC_BCG_PICTURE_T, m_strBcgTexture);
	DDX_Text(pDX, IDC_FAR_CLIP_PLANE, m_fFarClipPlane);
	DDV_MinMaxFloat(pDX, m_fFarClipPlane, -1.f, 1.e+007f);
	DDX_Check(pDX, IDC_APPLY_CLIP_FOR_ISOMETRIC, m_bApplyFarClipInIsometricProjection);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    // set auto rendering range flag
    theApp.m_vpViewPrefs[ m_iBuffer].m_bAutoRenderingRange = m_bAutoRenderingRange;

    // set rendering range
    theApp.m_vpViewPrefs[ m_iBuffer].m_fRenderingRange = m_fRenderingRange;

    // set drawing of editor models on or off
    pwrpPrefs.SetEditorModelsOn( m_bRenderEditorModels);
    // set render field brushes flag
    pwrpPrefs.SetFieldBrushesOn( m_bRenderFieldBrushes);
    // set use texture for background flag
    pwrpPrefs.SetBackgroundTextureOn( m_bUseTextureForBcg);

    pwrpPrefs.SetMirrorsOn( m_bRenderMirrors);
    pwrpPrefs.SetFogOn( m_bRenderFog);
    pwrpPrefs.SetHazeOn( m_bRenderHaze);

    // enable/disable edit render range control depending on auto range flag
    UpdateEditRangeControl();

    // set colors
    pwrpPrefs.SetVerticesInkColor( m_VertexColors.GetColor());
    pwrpPrefs.SetEdgesInkColor( m_EdgesColors.GetColor());
    pwrpPrefs.SetPolygonsInkColor( m_PolygonColors.GetColor());
    theApp.m_vpViewPrefs[ m_iBuffer].m_PaperColor = m_PaperColor.GetColor();
    theApp.m_vpViewPrefs[ m_iBuffer].m_GridColor = m_GridColor.GetColor();
    theApp.m_vpViewPrefs[ m_iBuffer].m_SelectionColor = m_SelectionColor.GetColor();
    pwrpPrefs.wrp_fFarClipPlane=m_fFarClipPlane;
    pwrpPrefs.wrp_bApplyFarClipPlaneInIsometricProjection=m_bApplyFarClipInIsometricProjection;
    enum CWorldRenderPrefs::FillType ftFillType;
    // get current fill type from vertices combo box
    ftFillType = (enum CWorldRenderPrefs::FillType) m_VertexFillType.GetCurSel();
    pwrpPrefs.SetVerticesFillType( ftFillType);
    // get current fill type from edges combo box
    ftFillType = (enum CWorldRenderPrefs::FillType) m_EdgesFillType.GetCurSel();
    pwrpPrefs.SetEdgesFillType( ftFillType);
    // get current fill type from polygons combo box
    ftFillType = (enum CWorldRenderPrefs::FillType) m_PolygonFillType.GetCurSel();
    pwrpPrefs.SetPolygonsFillType( ftFillType);

    enum CWorldRenderPrefs::LensFlaresType lftFlares;
    // get type for rendering flares
    lftFlares = (enum CWorldRenderPrefs::LensFlaresType) m_comboFlareFX.GetCurSel();
    pwrpPrefs.SetLensFlaresType( lftFlares);

    // get current model's texturizing type from model's texture combo box
    ULONG ulMdlFillType;
    ulMdlFillType = m_TextureFillType.GetCurSel();
    switch( ulMdlFillType)
    {
      case 0: {
        pmrpPrefs.SetRenderType( RT_NO_POLYGON_FILL);
        break;
      }
      case 1: {
        pmrpPrefs.SetRenderType( RT_WHITE_TEXTURE);
        break;
      }
      case 2: {
        pmrpPrefs.SetRenderType( RT_SURFACE_COLORS);
        break;
      }
      case 3: {
        pmrpPrefs.SetRenderType( RT_ON_COLORS);
        break;
      }
      case 4: {
        pmrpPrefs.SetRenderType( RT_OFF_COLORS);
        break;
      }
      case 5: {
        pmrpPrefs.SetRenderType( RT_TEXTURE);
        break;
      }
      default: {
        ASSERTALWAYS( "Invalid selection found in model's texture combo box");
      }
    }
    // get rest of model's rendering preferences
    pmrpPrefs.SetWire( m_bWireFrame);
    pmrpPrefs.SetHiddenLines( m_bHidenLines);
    pmrpPrefs.BBoxAllShow( m_bBoundingBox);
    if( m_bShadows) {
      pmrpPrefs.SetShadowQuality( 0);
    } else {
      pmrpPrefs.SetShadowQuality( 255);
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgRenderingPreferences, CDialog)
	//{{AFX_MSG_MAP(CDlgRenderingPreferences)
	ON_BN_CLICKED(ID_LOAD_PREFERENCES, OnLoadPreferences)
	ON_BN_CLICKED(ID_SAVE_PREFERENCES, OnSavePreferences)
	ON_BN_CLICKED(IDC_AUTO_RENDERING_RANGE, OnAutoRenderingRange)
	ON_BN_CLICKED(IDC_BROWSE_BCG_PICTURE, OnBrowseBcgPicture)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgRenderingPreferences message handlers

BOOL CDlgRenderingPreferences::OnInitDialog() 
{
  CDialog::OnInitDialog();

  CModelRenderPrefs &pmrpPrefs = theApp.m_vpViewPrefs[ m_iBuffer].m_mrpModelRenderPrefs;
  CWorldRenderPrefs &pwrpPrefs = theApp.m_vpViewPrefs[ m_iBuffer].m_wrpWorldRenderPrefs;
  // we will set window's name so we know on which buffer we are working on
  char chrWndTitle[ 64];
  // create new name
  if( m_iBuffer!=10)
  {
    sprintf( chrWndTitle, "Change rendering preferences of buffer %d.", m_iBuffer+1);
  }
  else
  {
    sprintf( chrWndTitle, "Change rendering preferences");
  }
  // set it as window new title
  SetWindowText( CString(chrWndTitle));
  
  // set dialog data
  UpdateData( FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgRenderingPreferences::OnLoadPreferences() 
{
  // load world and model's rendering preferences
  theApp.LoadRenderingPreferences();
  // set dialog data
  UpdateData( FALSE);
}

void CDlgRenderingPreferences::OnSavePreferences() 
{
  // get data from dialog
  UpdateData( TRUE);
  // load world and model's rendering preferences
  theApp.SaveRenderingPreferences();
}

void CDlgRenderingPreferences::UpdateEditRangeControl()
{
  // get edit rendering range control
  CWnd *pWnd = GetDlgItem( IDC_RENDERING_RANGE);
  // must exists
  ASSERT( pWnd != NULL);
  // if rendering range flag is on
  if( m_bAutoRenderingRange)
  {
    // disable edit rendering range control
    pWnd->EnableWindow( FALSE);
  }
  else
  {
    // otherwise enable it
    pWnd->EnableWindow( TRUE);
  }
}

void CDlgRenderingPreferences::OnAutoRenderingRange() 
{
  m_bAutoRenderingRange = !m_bAutoRenderingRange;
  // set dialog data
  UpdateData( TRUE);
  // enable/disable edit range control
  UpdateEditRangeControl();
}

void CDlgRenderingPreferences::OnBrowseBcgPicture() 
{
  CTFileName fnBcgPicture = _EngineGUI.FileRequester( "Select background texture",
    "Texture (*.tex)\0*.tex\0" FILTER_TEX FILTER_END, "Background textures", "");
  if( fnBcgPicture == "") return;
  sprintf( theApp.m_vpViewPrefs[ m_iBuffer].m_achrBcgPicture, "%s", fnBcgPicture);
  m_strBcgTexture = fnBcgPicture.FileName();
  UpdateData( FALSE);
}
