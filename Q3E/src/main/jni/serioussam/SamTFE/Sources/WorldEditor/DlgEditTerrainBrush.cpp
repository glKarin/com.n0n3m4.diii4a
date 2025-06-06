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

// DlgEditTerrainBrush.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgEditTerrainBrush.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgEditTerrainBrush dialog


CDlgEditTerrainBrush::CDlgEditTerrainBrush(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgEditTerrainBrush::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgEditTerrainBrush)
	m_fFallOff = 0.0f;
	m_fHotSpot = 0.0f;
	//}}AFX_DATA_INIT
}

void CDlgEditTerrainBrush::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  // if dialog is reciving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    m_fHotSpot=atebCustomEditBrushes[m_iBrush].teb_fHotSpot;
    m_fFallOff=atebCustomEditBrushes[m_iBrush].teb_fFallOff;
  }
	//{{AFX_DATA_MAP(CDlgEditTerrainBrush)
	DDX_Text(pDX, IDC_FALLOFF, m_fFallOff);
	DDV_MinMaxFloat(pDX, m_fFallOff, 0.f, 1.e+008f);
	DDX_Text(pDX, IDC_HOTSPOT, m_fHotSpot);
	DDV_MinMaxFloat(pDX, m_fHotSpot, 0.f, 1.e+007f);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    atebCustomEditBrushes[m_iBrush].teb_fHotSpot=m_fHotSpot;
    atebCustomEditBrushes[m_iBrush].teb_fFallOff=m_fFallOff;
  }
}


BEGIN_MESSAGE_MAP(CDlgEditTerrainBrush, CDialog)
	//{{AFX_MSG_MAP(CDlgEditTerrainBrush)
	ON_BN_CLICKED(ID_GENERATE_TERRAIN_BRUSH, OnGenerateTerrainBrush)
	ON_BN_CLICKED(IDC_IMPORT_TERRAIN_BRUSH, OnImportTerrainBrush)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgEditTerrainBrush message handlers

void CDlgEditTerrainBrush::OnGenerateTerrainBrush() 
{
  UpdateData(TRUE);
  GenerateTerrainBrushTexture( m_iBrush, m_fHotSpot, m_fFallOff);
}

void CDlgEditTerrainBrush::OnImportTerrainBrush() 
{
  CTFileName fnBrush =  _EngineGUI.FileRequester(
    "Choose picture for brush", FILTER_TGA FILTER_PCX FILTER_ALL FILTER_END,
    "Terrain brush directory", "Textures\\");

  SetFocus();
  SetActiveWindow();

  if( fnBrush== "") return;

  CImageInfo ii;
  ii.LoadAnyGfxFormat_t( fnBrush);
  // both dimension must be potentions of 2
  if( (ii.ii_Width  == 1<<((int)Log2( (FLOAT)ii.ii_Width))) &&
      (ii.ii_Height == 1<<((int)Log2( (FLOAT)ii.ii_Height))) )
  {
    CTFileName fnTexture = GetBrushTextureName(m_iBrush);
    // creates new texture with one frame
    CTextureData tdPicture;
    tdPicture.Create_t( &ii, ii.ii_Width, 16, TRUE);
    tdPicture.Save_t( fnTexture);
  }
}
