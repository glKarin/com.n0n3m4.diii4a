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

// DlgTerrainProperties.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgTerrainProperties.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static INDEX _iShadowMapShift=0;
static INDEX _iShadingMapShift=1;
static PIX _pixGlobalPretenderTextureWidth=64;

/////////////////////////////////////////////////////////////////////////////
// CDlgTerrainProperties dialog


CDlgTerrainProperties::CDlgTerrainProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgTerrainProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgTerrainProperties)
	m_strHeightmapSize = _T("");
	m_strShadowMapSize = _T("");
	m_strTerrainPretender = _T("");
	m_strTilePretender = _T("");
	m_fTerrainLength = 0.0f;
	m_fTerrainHeight = 0.0f;
	m_fTerrainWidth = 0.0f;
	m_fLODSwitch = 0.0f;
	m_strShadingMapSize = _T("");
	m_strMemoryConsumption = _T("");
	m_strLayerMemory = _T("");
	m_strEdgeMap = _T("");
	//}}AFX_DATA_INIT
  
  CTerrain *ptrTerrain=GetTerrain();
  if(ptrTerrain==NULL) return;

  _iShadowMapShift=ptrTerrain->tr_iShadowMapSizeAspect;
  _iShadingMapShift=ptrTerrain->tr_iShadingMapSizeAspect;
  _pixGlobalPretenderTextureWidth=ptrTerrain->tr_pixTopMapWidth;
}


void CDlgTerrainProperties::DoDataExchange(CDataExchange* pDX)
{
  CTString strTemp;

  CTerrain *ptrTerrain=GetTerrain();
  if(ptrTerrain==NULL) return;

  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE && IsWindow(m_ctrlShadowMapSlider))
  {
    m_fTerrainWidth =ptrTerrain->tr_vTerrainSize(1);
    m_fTerrainHeight=ptrTerrain->tr_vTerrainSize(2);
    m_fTerrainLength=ptrTerrain->tr_vTerrainSize(3);

    INDEX iItemW=m_ctrlHeightMapWidth.GetCurSel();
    PIX pixW=m_ctrlHeightMapWidth.GetItemData( iItemW);

    INDEX iItemH=m_ctrlHeightMapHeight.GetCurSel();
    PIX pixH=m_ctrlHeightMapHeight.GetItemData( iItemH);
    INDEX iMemHeightMap=pixW*pixH*sizeof(UWORD)/1024;
    strTemp.PrintF("Height map size: %d x %d, Memory: %d KB", pixW, pixH, iMemHeightMap);
    m_strHeightmapSize=strTemp;

    PrepareGlobalPretenderCombo();

    INDEX iShadowMapShift=m_ctrlShadowMapSlider.GetPos();
    PIX pixSwW=Clamp(PIX((pixW-1)*pow(2.0f,iShadowMapShift)),PIX(4),PIX(2048));
    PIX pixSwH=Clamp(PIX((pixH-1)*pow(2.0f,iShadowMapShift)),PIX(4),PIX(2048));
    INDEX iMemShadowMap=pixSwW*pixSwH*sizeof(COLOR)/1024*1.333f;
    strTemp.PrintF("Size: %d x %d, Memory: %d KB", pixSwW, pixSwH, iMemShadowMap);
    m_strShadowMapSize=strTemp;

    INDEX iShadingMapShift=10-m_ctrlShadingMapSlider.GetPos();
    PIX pixSdW=Clamp(PIX(pixSwW*pow(2.0f,-iShadingMapShift)),PIX(4),PIX(pixSwW));
    PIX pixSdH=Clamp(PIX(pixSwH*pow(2.0f,-iShadingMapShift)),PIX(4),PIX(pixSwH));
    INDEX iMemShadingMap=pixSdW*pixSdW*sizeof(UWORD)/1024;
    strTemp.PrintF("Size: %d x %d, Memory: %d KB", pixSdW, pixSdH, iMemShadingMap);
    m_strShadingMapSize=strTemp;

    INDEX iGlobalPretender=m_ctrlGlobalPretenderTexture.GetCurSel();
    INDEX pixSize=m_ctrlGlobalPretenderTexture.GetItemData(iGlobalPretender);
    INDEX iMemTerrainPretender=pixSize*pixSize*sizeof(UWORD)/1024*1.333f;
    strTemp.PrintF("Size: %d x %d, Memory: %d KB", pixSize, pixSize, iMemTerrainPretender);
    m_strTerrainPretender=strTemp;

    INDEX iTlp=m_ctrlTilePretender.GetCurSel();
    PIX pixTlpW=m_ctrlTilePretender.GetItemData( iTlp);
    INDEX iMemTilePretender=pixTlpW*pixTlpW*sizeof(UWORD)/1024*1.333f;
    strTemp.PrintF("Tile pretender texture (%d KB)", iMemTilePretender);
    m_strTilePretender=strTemp;

    INDEX ctLayers=ptrTerrain->tr_atlLayers.Count();
    INDEX iMemLayers=(pixW*pixH*ctLayers)/1024;
    strTemp.PrintF("Layer memory consumption (%d layers): %d KB", ctLayers, iMemLayers);
    m_strLayerMemory=strTemp;

    INDEX iMemEdgeMap=(pixW*pixH)/1024;
    strTemp.PrintF("Edge map memory consumption: %d KB", iMemEdgeMap);
    m_strEdgeMap=strTemp;

    strTemp.PrintF("Terrain total uncompressed static data memory consumption (without tile pretender textures and geometry): %d KB",
                    iMemHeightMap+iMemShadowMap+iMemShadingMap+iMemTerrainPretender+iMemLayers+iMemEdgeMap);
    m_strMemoryConsumption=strTemp;
    
    m_fLODSwitch=ptrTerrain->tr_fDistFactor;
  }

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgTerrainProperties)
	DDX_Control(pDX, IDC_GLOBAL_PRETENDER, m_ctrlGlobalPretenderTexture);
	DDX_Control(pDX, IDC_TILE_PRETENDER, m_ctrlTilePretender);
	DDX_Control(pDX, IDC_SHADING_MAP, m_ctrlShadingMapSlider);
	DDX_Control(pDX, IDC_TERRAIN_QUADS_PER_TILE, m_ctrlQuadsPerTile);
	DDX_Control(pDX, IDC_TERRAIN_HM_WIDTH, m_ctrlHeightMapWidth);
	DDX_Control(pDX, IDC_TERRAIN_HM_HEIGHT, m_ctrlHeightMapHeight);
	DDX_Control(pDX, IDC_SHADOW_MAP, m_ctrlShadowMapSlider);
	DDX_Text(pDX, IDC_HEIGHTIMAP_SIZE_T, m_strHeightmapSize);
	DDX_Text(pDX, IDC_SHADOW_MAP_T, m_strShadowMapSize);
	DDX_Text(pDX, IDC_TERRAIN_PRETENDER_T, m_strTerrainPretender);
	DDX_Text(pDX, IDC_TILE_PRETENDER_T, m_strTilePretender);
	DDX_Text(pDX, IDC_TERRAIN_LENGTH, m_fTerrainLength);
	DDV_MinMaxFloat(pDX, m_fTerrainLength, 0.f, 1.e+007f);
	DDX_Text(pDX, IDC_TERRAIN_HEIGHT, m_fTerrainHeight);
	DDV_MinMaxFloat(pDX, m_fTerrainHeight, 0.f, 1.e+007f);
	DDX_Text(pDX, IDC_TERRAIN_WIDTH, m_fTerrainWidth);
	DDV_MinMaxFloat(pDX, m_fTerrainWidth, 0.f, 1.e+007f);
	DDX_Text(pDX, IDC_TERRAIN_LOD_SWITCH, m_fLODSwitch);
	DDV_MinMaxFloat(pDX, m_fLODSwitch, 0.f, 1.e+007f);
	DDX_Text(pDX, IDC_SHADING_MAP_T, m_strShadingMapSize);
	DDX_Text(pDX, IDC_TERRAIN_MEMORY_T, m_strMemoryConsumption);
	DDX_Text(pDX, IDC_LAYER_MEMORY_T, m_strLayerMemory);
	DDX_Text(pDX, IDC_TERRAIN_EDGE_MEMORY_T, m_strEdgeMap);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    BOOL bUpdateTerrain=FALSE;

    // handle heightmap size change
    INDEX iItemW=m_ctrlHeightMapWidth.GetCurSel();
    PIX pixW=m_ctrlHeightMapWidth.GetItemData( iItemW);
    INDEX iItemH=m_ctrlHeightMapHeight.GetCurSel();
    PIX pixH=m_ctrlHeightMapHeight.GetItemData( iItemH);
    if( ptrTerrain->tr_pixHeightMapWidth != pixW ||
        ptrTerrain->tr_pixHeightMapHeight!= pixH )
    {
      ptrTerrain->ReAllocateHeightMap(pixW, pixH);
      bUpdateTerrain=TRUE;
    }

    // handle terrain size change
    if( ptrTerrain->tr_vTerrainSize(1)!=m_fTerrainWidth ||
        ptrTerrain->tr_vTerrainSize(2)!=m_fTerrainHeight||
        ptrTerrain->tr_vTerrainSize(3)!=m_fTerrainLength)
    {
      ptrTerrain->SetTerrainSize(FLOAT3D(m_fTerrainWidth,m_fTerrainHeight,m_fTerrainLength));
      bUpdateTerrain=TRUE;
    }

    // handle tile pretender texture size change
    INDEX iTlp=m_ctrlTilePretender.GetCurSel();
    PIX pixTlpW=m_ctrlTilePretender.GetItemData( iTlp);
    if( ptrTerrain->tr_pixFirstMipTopMapWidth != pixTlpW)
    {
      ptrTerrain->SetTileTopMapSize(pixTlpW);
      bUpdateTerrain=TRUE;
    }

    // handle quads per tile change
    INDEX iQuadItem=m_ctrlQuadsPerTile.GetCurSel();
    INDEX ctQuads=m_ctrlQuadsPerTile.GetItemData( iQuadItem);
    if(ctQuads!=ptrTerrain->tr_ctQuadsInTileRow)
    {
      ptrTerrain->SetQuadsPerTileRow(ctQuads);
      bUpdateTerrain=TRUE;
    }
    
    // handle LOD distance change
    if( m_fLODSwitch!=ptrTerrain->tr_fDistFactor)
    {
      ptrTerrain->SetLodDistanceFactor(m_fLODSwitch);
    }

    // handle global pretender texture change
    INDEX iGlobalPretender=m_ctrlGlobalPretenderTexture.GetCurSel();
    INDEX pixSize=m_ctrlGlobalPretenderTexture.GetItemData(iGlobalPretender);
    if(pixSize!=ptrTerrain->tr_pixTopMapWidth)
    {
      ptrTerrain->SetGlobalTopMapSize(pixSize);
      bUpdateTerrain=TRUE;
    }

    // handle shadow changes
    if( (m_ctrlShadowMapSlider.GetPos()!=_iShadowMapShift) ||
        (m_ctrlShadingMapSlider.GetPos()!=10-_iShadingMapShift) )
    {
      _iShadowMapShift=m_ctrlShadowMapSlider.GetPos();
      _iShadingMapShift=10-m_ctrlShadingMapSlider.GetPos();
      ptrTerrain->SetShadowMapsSize(_iShadowMapShift,_iShadingMapShift);
      bUpdateTerrain=TRUE;
    }

    // update terrain
    if(bUpdateTerrain)
    {
      ptrTerrain->tr_penEntity->TerrainChangeNotify();
      GenerateLayerDistribution(-1);
      ptrTerrain->RefreshTerrain();
      ptrTerrain->UpdateShadowMap();
      theApp.m_ctTerrainPage.MarkChanged();
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgTerrainProperties, CDialog)
	//{{AFX_MSG_MAP(CDlgTerrainProperties)
	ON_WM_HSCROLL()
	ON_CBN_SELCHANGE(IDC_TERRAIN_HM_WIDTH, OnSelchangeTerrainHmWidth)
	ON_CBN_SELCHANGE(IDC_TERRAIN_HM_HEIGHT, OnSelchangeTerrainHmHeight)
	ON_CBN_SELCHANGE(IDC_TILE_PRETENDER, OnSelchangeTilePretender)
	ON_CBN_SELCHANGE(IDC_GLOBAL_PRETENDER, OnSelchangeGlobalPretender)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgTerrainProperties message handlers

void CDlgTerrainProperties::InitComboBoxes(void)
{
  INDEX iToSelect, iWidth;
  CTerrain *ptrTerrain=GetTerrain();
  if(ptrTerrain==NULL) return;

  // prepare quads per tile combo box
  m_ctrlQuadsPerTile.ResetContent();
  iToSelect=3;
  for(INDEX iQuads=4; iQuads<=2048; iQuads*=2)
  {
    CTString strItem;
    strItem.PrintF("%d x %d", iQuads, iQuads);
	  INDEX iAddedAs=m_ctrlQuadsPerTile.AddString(CString(strItem));
	  m_ctrlQuadsPerTile.SetItemData(iAddedAs,iQuads);
    if(ptrTerrain->tr_ctQuadsInTileRow==iQuads)
    {
      iToSelect=iAddedAs;
    }
  }
	m_ctrlQuadsPerTile.SetCurSel(iToSelect);

  // prepare heightmap size combo boxes
	m_ctrlHeightMapWidth.ResetContent();
	m_ctrlHeightMapHeight.ResetContent();
  INDEX iToSelectW=3;
  INDEX iToSelectH=3;
  for(iWidth=32; iWidth<=2048; iWidth*=2)
  {
    CTString strItem;
    strItem.PrintF("%d", iWidth+1);
	  // width
    INDEX iW=m_ctrlHeightMapWidth.AddString(CString(strItem));
    m_ctrlHeightMapWidth.SetItemData(iW,iWidth+1);
    if(ptrTerrain->tr_pixHeightMapWidth==iWidth+1)    iToSelectW=iW;
	  // height
    INDEX iH=m_ctrlHeightMapHeight.AddString(CString(strItem));
    m_ctrlHeightMapHeight.SetItemData(iH,iWidth+1);
    if(ptrTerrain->tr_pixHeightMapHeight==iWidth+1)    iToSelectH=iH;
  }
  m_ctrlHeightMapWidth.SetCurSel( iToSelectW);
  m_ctrlHeightMapHeight.SetCurSel(iToSelectH);

  PrepareTilePretenderCombo();
  PrepareGlobalPretenderCombo();
}

// prepare tile pretender combo boxes
void CDlgTerrainProperties::PrepareTilePretenderCombo(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if(ptrTerrain==NULL) return;

  m_ctrlTilePretender.ResetContent();
  INDEX iToSelect=6;
  for(INDEX iWidth=4; iWidth<=2048; iWidth*=2)
  {
    CTString strItem;
    strItem.PrintF("%d x %d", iWidth, iWidth);
	  INDEX iAddedAs=m_ctrlTilePretender.AddString(CString(strItem));
    m_ctrlTilePretender.SetItemData(iAddedAs, iWidth);

    if(ptrTerrain->tr_pixFirstMipTopMapWidth==iWidth)
    {
      iToSelect=iAddedAs;
    }
  }
	m_ctrlTilePretender.SetCurSel(iToSelect);
  
}

// prepare tile pretender combo boxes
void CDlgTerrainProperties::PrepareGlobalPretenderCombo(void)
{
  CTerrain *ptrTerrain=GetTerrain();
  if(ptrTerrain==NULL) return;

  INDEX iHeightMapWidthItem=m_ctrlHeightMapWidth.GetCurSel();
  PIX pixHeightMapWidth=m_ctrlHeightMapWidth.GetItemData(iHeightMapWidthItem);
  INDEX iHeightMapHeightItem=m_ctrlHeightMapHeight.GetCurSel();
  PIX pixHeightMapHeight=m_ctrlHeightMapHeight.GetItemData(iHeightMapHeightItem);

  m_ctrlGlobalPretenderTexture.ResetContent();
  INDEX iToSelect=_pixGlobalPretenderTextureWidth;
  FLOAT fAspect=FLOAT(pixHeightMapWidth-1)/(pixHeightMapHeight-1);
  for(INDEX iWidth=4; iWidth<=2048; iWidth*=2)
  {
    CTString strItem;
    strItem.PrintF("%d x %d", iWidth, INDEX(iWidth/fAspect));
	  INDEX iAddedAs=m_ctrlGlobalPretenderTexture.AddString(CString(strItem));
    m_ctrlGlobalPretenderTexture.SetItemData(iAddedAs, iWidth);
    if(_pixGlobalPretenderTextureWidth==iWidth)
    {
      iToSelect=iAddedAs;
    }
  }
	m_ctrlGlobalPretenderTexture.SetCurSel(iToSelect);
}

BOOL CDlgTerrainProperties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // initialize combo boxes
  InitComboBoxes();

	m_ctrlShadowMapSlider.SetRange(-10, 10, TRUE);
	m_ctrlShadingMapSlider.SetRange(0, 10, TRUE);

  m_ctrlShadowMapSlider.SetPos(_iShadowMapShift);
  m_ctrlShadowMapSlider.Invalidate(FALSE);
  m_ctrlShadingMapSlider.SetPos(10-_iShadingMapShift);
  m_ctrlShadingMapSlider.Invalidate(FALSE);

  UpdateData(FALSE);
	return TRUE;
}

BOOL _bUpdateDlg=TRUE;
void CDlgTerrainProperties::OnSelchangeTerrainHmWidth() 
{
  if(_bUpdateDlg)
  {
    _bUpdateDlg=FALSE;
    m_ctrlHeightMapHeight.SetCurSel(m_ctrlHeightMapWidth.GetCurSel());
    PrepareGlobalPretenderCombo();
    UpdateData(FALSE);
    _bUpdateDlg=TRUE;
  }
}

void CDlgTerrainProperties::OnSelchangeTerrainHmHeight() 
{
  if(_bUpdateDlg)
  {
    _bUpdateDlg=FALSE;
    m_ctrlHeightMapWidth.SetCurSel(m_ctrlHeightMapHeight.GetCurSel());
    PrepareGlobalPretenderCombo();
    UpdateData(FALSE);
    _bUpdateDlg=TRUE;
  }
}

void CDlgTerrainProperties::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
  UpdateData( FALSE);
}

void CDlgTerrainProperties::OnOK() 
{
	CDialog::OnOK();
}

void CDlgTerrainProperties::OnSelchangeTilePretender() 
{
  UpdateData( FALSE);
}

void CDlgTerrainProperties::OnSelchangeGlobalPretender() 
{
  INDEX iItem=m_ctrlGlobalPretenderTexture.GetCurSel();
  PIX pixValue=m_ctrlGlobalPretenderTexture.GetItemData(iItem);
  _pixGlobalPretenderTextureWidth=pixValue;
}
